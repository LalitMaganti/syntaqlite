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
    if validator.run(&sources) {
        std::process::exit(1);
    }
    Ok(())
}

struct Validator<'a> {
    dialect: &'a AnyDialect,
    config: ValidationConfig,
    lang: Option<HostLanguage>,
    output: ValidateOutput,
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
            output: args.output,
            schema_catalog,
            schema_files,
            has_schema,
        })
    }

    /// Returns `true` if any source produced an error-level diagnostic.
    fn run(&self, sources: &[Source]) -> bool {
        let multi = sources.len() > 1;
        let mut any_errors = false;
        let mut any_diagnostics = false;

        for src in sources {
            if let ValidateOutput::Text = self.output
                && multi
                && src.is_file()
            {
                println!("==> {} <==", src.label);
            }
            let (errors, diags) = self.validate_one(src);
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

        if any_diagnostics
            && !self.has_schema
            && let ValidateOutput::Text = self.output
        {
            emit_no_schema_hint();
        }
        any_errors
    }

    /// Validate one source; returns `(has_errors, has_any_diagnostics)`.
    fn validate_one(&self, src: &Source) -> (bool, bool) {
        match (self.lang, self.output) {
            (Some(lang), ValidateOutput::Text) => {
                let e = self.embedded_text(src, lang);
                (e, e)
            }
            (Some(lang), ValidateOutput::Json) => {
                let e = self.embedded_json(src, lang);
                (e, e)
            }
            (None, ValidateOutput::Text) => self.standalone_text(src),
            (None, ValidateOutput::Json) => self.standalone_json(src),
        }
    }

    fn standalone_text(&self, src: &Source) -> (bool, bool) {
        let mut analyzer = SemanticAnalyzer::with_dialect(self.dialect.clone());
        let model = analyzer.analyze(&src.text, &self.schema_catalog, &self.config);
        let any_diags = model.has_diagnostics();
        let all_diags: Vec<_> = model.diagnostics().cloned().collect();
        let has_errors = DiagnosticRenderer::new(&src.text, &src.label)
            .render_diagnostics(&all_diags, &mut io::stderr())
            .unwrap_or(false);
        (has_errors, any_diags)
    }

    fn standalone_json(&self, src: &Source) -> (bool, bool) {
        let mut analyzer = SemanticAnalyzer::with_dialect(self.dialect.clone());
        let model = analyzer.analyze(&src.text, &self.schema_catalog, &self.config);
        let mut has_errors = false;
        let mut any_diags = false;
        for diag in model.diagnostics() {
            any_diags = true;
            if let Severity::Error = diag.severity() {
                has_errors = true;
            }
            emit_diagnostic_json(&src.label, diag);
        }
        (has_errors, any_diags)
    }

    fn embedded_text(&self, src: &Source, lang: HostLanguage) -> bool {
        let fragments = extract_fragments(&src.text, lang);
        if fragments.is_empty() {
            eprintln!("no SQL fragments found in {}", src.label);
            return false;
        }
        let Some(catalog) = self.reload_embedded_catalog() else {
            return true;
        };
        let diags = syntaqlite::embedded::EmbeddedAnalyzer::new(self.dialect.clone())
            .with_catalog(catalog)
            .with_config(self.config)
            .validate(&fragments);
        DiagnosticRenderer::new(&src.text, &src.label)
            .render_diagnostics(&diags, &mut io::stderr())
            .unwrap_or(false)
    }

    fn embedded_json(&self, src: &Source, lang: HostLanguage) -> bool {
        let fragments = extract_fragments(&src.text, lang);
        if fragments.is_empty() {
            return false;
        }
        let Some(catalog) = self.reload_embedded_catalog() else {
            return true;
        };
        let diags = syntaqlite::embedded::EmbeddedAnalyzer::new(self.dialect.clone())
            .with_catalog(catalog)
            .with_config(self.config)
            .validate(&fragments);
        let mut has_errors = false;
        for diag in &diags {
            if let Severity::Error = diag.severity() {
                has_errors = true;
            }
            emit_diagnostic_json(&src.label, diag);
        }
        has_errors
    }

    /// The embedded analyzer consumes the catalog by value, so each embedded
    /// source has to build its own. Returns `None` and prints the error (the
    /// caller should treat this as an errored source).
    fn reload_embedded_catalog(&self) -> Option<Catalog> {
        match util::build_schema_catalog(self.dialect, &self.schema_files) {
            Ok(c) => Some(c),
            Err(e) => {
                eprintln!("error: {e}");
                None
            }
        }
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

fn emit_no_schema_hint() {
    eprintln!(
        "note: no schema provided; unresolved names are reported as warnings. \
         Add a `syntaqlite.toml` with `schema = [\"schema.sql\"]` or pass `--schema` \
         to treat them as errors."
    );
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
            start_offset: diag.start_offset(),
            end_offset: diag.end_offset(),
            help,
        }
    }
}
