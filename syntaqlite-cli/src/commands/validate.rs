// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `validate` subcommand.

use std::io;

use serde::Serialize;
use syntaqlite::Diagnostic;
use syntaqlite::any::AnyDialect;
use syntaqlite::semantic::{Help, Severity};
use syntaqlite::util::DiagnosticRenderer;
use syntaqlite::{Catalog, SemanticAnalyzer, ValidationConfig};

use crate::cli::{HostLanguage, ValidateArgs, ValidateOutput};
use crate::config::{self, ConfigMode};
use crate::util::{self, Source};

pub(crate) fn run(
    dialect: &AnyDialect,
    config_mode: &ConfigMode<'_>,
    args: &ValidateArgs,
) -> Result<(), String> {
    let validator = Validator::new(dialect, config_mode, args)?;
    let sources = util::load_sources(&args.files, args.expression.as_deref())?;
    let renderer = select_renderer(args.output);
    if validator.run(&sources, renderer.as_ref()) {
        std::process::exit(1);
    }
    Ok(())
}

struct Validator<'a> {
    dialect: &'a AnyDialect,
    config: ValidationConfig,
    lang: Option<HostLanguage>,
    schema_catalog: Catalog,
    schema_files: Vec<String>,
    has_schema: bool,
}

impl<'a> Validator<'a> {
    fn new(
        dialect: &'a AnyDialect,
        config_mode: &ConfigMode<'_>,
        args: &ValidateArgs,
    ) -> Result<Self, String> {
        let file_config = config::resolve(config_mode);
        let schema_files = if args.schema.is_empty() {
            util::schemas_from_project_config(&args.files, file_config.as_ref())
        } else {
            args.schema.clone()
        };
        let has_schema = !schema_files.is_empty();
        let checks = util::build_check_config(
            has_schema,
            file_config.as_ref().map(|(c, _)| &c.checks),
            &args.allow,
            &args.warn,
            &args.deny,
        )?;
        let schema_catalog = util::build_schema_catalog(dialect, &schema_files)?;

        Ok(Self {
            dialect,
            config: ValidationConfig::default().with_checks(checks),
            lang: args.lang,
            schema_catalog,
            schema_files,
            has_schema,
        })
    }

    /// Returns `true` if any source produced an error-level diagnostic.
    fn run(&self, sources: &[Source], renderer: &dyn Renderer) -> bool {
        let multi = sources.len() > 1;
        let mut any_errors = false;
        let mut any_diagnostics = false;

        for src in sources {
            if multi && src.is_file() {
                renderer.on_source_start(src);
            }
            let (errors, diags) = self.validate_one(src, renderer);
            // Inline input (-e / stdin) exits eagerly on errors and never
            // contributes to the post-loop "no schema" hint — matches the
            // legacy runtime.rs behaviour the integration suites expect.
            if !src.is_file() {
                if errors {
                    std::process::exit(1);
                }
                continue;
            }
            any_errors |= errors;
            any_diagnostics |= diags;
        }

        if any_diagnostics && !self.has_schema {
            renderer.on_no_schema_hint();
        }
        any_errors
    }

    /// Validate one source; returns `(has_errors, has_any_diagnostics)`.
    fn validate_one(&self, src: &Source, renderer: &dyn Renderer) -> (bool, bool) {
        match self.analyze(src) {
            Analysis::Diagnostics(diags) => {
                let any_diags = !diags.is_empty();
                let has_errors = renderer.render(src, &diags);
                (has_errors, any_diags)
            }
            Analysis::NoEmbeddedFragments => {
                renderer.on_no_fragments(src);
                (false, false)
            }
            Analysis::CatalogError => (true, true),
        }
    }

    // ── Computation ────────────────────────────────────────────────────────

    fn analyze(&self, src: &Source) -> Analysis {
        match self.lang {
            Some(lang) => self.analyze_embedded(src, lang),
            None => Analysis::Diagnostics(self.analyze_standalone(src)),
        }
    }

    fn analyze_standalone(&self, src: &Source) -> Vec<Diagnostic> {
        let mut analyzer = SemanticAnalyzer::with_dialect(self.dialect.clone());
        let model = analyzer.analyze(&src.text, &self.schema_catalog, &self.config);
        model.diagnostics().cloned().collect()
    }

    fn analyze_embedded(&self, src: &Source, lang: HostLanguage) -> Analysis {
        let fragments = extract_fragments(&src.text, lang);
        if fragments.is_empty() {
            return Analysis::NoEmbeddedFragments;
        }
        // The embedded analyzer consumes the catalog by value, so each
        // embedded source has to build its own.
        let catalog = match util::build_schema_catalog(self.dialect, &self.schema_files) {
            Ok(c) => c,
            Err(e) => {
                eprintln!("error: {e}");
                return Analysis::CatalogError;
            }
        };
        let diags = syntaqlite::embedded::EmbeddedAnalyzer::new(self.dialect.clone())
            .with_catalog(catalog)
            .with_config(self.config)
            .validate(&fragments);
        Analysis::Diagnostics(diags)
    }
}

/// Output of a single source analysis — common shape across standalone and
/// embedded analyzers. Rendering is the caller's problem.
enum Analysis {
    /// The analyzer ran and produced zero or more diagnostics.
    Diagnostics(Vec<Diagnostic>),
    /// Embedded-mode scan found no SQL fragments in the host source.
    NoEmbeddedFragments,
    /// Embedded-mode schema catalog failed to load. The error was already
    /// printed to stderr; the caller should treat this as an errored source.
    CatalogError,
}

// ── Renderer strategy ──────────────────────────────────────────────────────

/// Output strategy for validation results. Adding a new `--output` mode is a
/// new struct + `impl Renderer`; the [`Validator`] doesn't change.
trait Renderer {
    /// Render the diagnostics for one source; returns `true` if any were errors.
    fn render(&self, src: &Source, diagnostics: &[Diagnostic]) -> bool;
    /// Per-file header emitted only when there are multiple file sources.
    fn on_source_start(&self, _src: &Source) {}
    /// Called when an embedded-mode scan found no SQL fragments in `src`.
    fn on_no_fragments(&self, _src: &Source) {}
    /// Called at the end of a run when any diagnostics were emitted but no
    /// schema was loaded — a hint to point users at `--schema`.
    fn on_no_schema_hint(&self) {}
}

fn select_renderer(output: ValidateOutput) -> Box<dyn Renderer> {
    match output {
        ValidateOutput::Text => Box::new(TextRenderer),
        ValidateOutput::Json => Box::new(JsonRenderer),
    }
}

struct TextRenderer;

impl Renderer for TextRenderer {
    fn render(&self, src: &Source, diagnostics: &[Diagnostic]) -> bool {
        DiagnosticRenderer::new(&src.text, &src.label)
            .render_diagnostics(diagnostics, &mut io::stderr())
            .unwrap_or(false)
    }

    fn on_source_start(&self, src: &Source) {
        println!("==> {} <==", src.label);
    }

    fn on_no_fragments(&self, src: &Source) {
        eprintln!("no SQL fragments found in {}", src.label);
    }

    fn on_no_schema_hint(&self) {
        eprintln!(
            "note: no schema provided; unresolved names are reported as warnings. \
             Add a `syntaqlite.toml` with `schema = [\"schema.sql\"]` or pass `--schema` \
             to treat them as errors."
        );
    }
}

struct JsonRenderer;

impl Renderer for JsonRenderer {
    fn render(&self, src: &Source, diagnostics: &[Diagnostic]) -> bool {
        let mut has_errors = false;
        for diag in diagnostics {
            if let Severity::Error = diag.severity() {
                has_errors = true;
            }
            emit_diagnostic_json(&src.label, diag);
        }
        has_errors
    }
}

fn extract_fragments(
    source: &str,
    lang: HostLanguage,
) -> Vec<syntaqlite::embedded::EmbeddedFragment> {
    match lang {
        HostLanguage::Python => syntaqlite::embedded::extract_python(source),
        HostLanguage::Typescript => syntaqlite::embedded::extract_typescript(source),
    }
}

fn emit_diagnostic_json(file: &str, diag: &Diagnostic) {
    let record = DiagnosticRecord::from_diag(file, diag);
    match serde_json::to_string(&record) {
        Ok(s) => println!("{s}"),
        Err(e) => eprintln!("error serializing diagnostic: {e}"),
    }
}

// ── JSON output schema ─────────────────────────────────────────────────────
//
// `schema_version` is pinned at `0` during the pre-stable period. Bump when
// making a compatible schema change; document breaking schema changes in
// `CHANGELOG.md`.

const SCHEMA_VERSION: u32 = 0;

#[derive(Serialize)]
#[serde(rename_all = "lowercase")]
enum JsonSeverity {
    Error,
    Warning,
    Info,
    Hint,
}

impl From<Severity> for JsonSeverity {
    fn from(s: Severity) -> Self {
        match s {
            Severity::Error => JsonSeverity::Error,
            Severity::Warning => JsonSeverity::Warning,
            Severity::Info => JsonSeverity::Info,
            Severity::Hint => JsonSeverity::Hint,
        }
    }
}

#[derive(Serialize)]
struct DiagnosticRecord<'a> {
    kind: &'static str,
    schema_version: u32,
    file: &'a str,
    severity: JsonSeverity,
    message: String,
    start_offset: usize,
    end_offset: usize,
    #[serde(skip_serializing_if = "Option::is_none")]
    help: Option<String>,
}

impl<'a> DiagnosticRecord<'a> {
    fn from_diag(file: &'a str, diag: &Diagnostic) -> Self {
        let help = diag.help().map(|h| match h {
            Help::Suggestion(s) => s.clone(),
        });
        Self {
            kind: "diagnostic",
            schema_version: SCHEMA_VERSION,
            file,
            severity: diag.severity().into(),
            message: diag.message().to_string(),
            start_offset: diag.start().as_usize(),
            end_offset: diag.end().as_usize(),
            help,
        }
    }
}
