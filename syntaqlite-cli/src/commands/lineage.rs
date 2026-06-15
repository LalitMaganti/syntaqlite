// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `lineage` subcommand.

use serde::Serialize;
use syntaqlite::Diagnostic;
use syntaqlite::analysis::{
    Analyzer, DiagnosticMessage, RelationKind, Severity, StatementAnalysis,
};
use syntaqlite::any::AnyDialect;
use syntaqlite::{AnalysisConfig, Catalog};

use crate::cli::{HostLanguage, LineageArgs, LineageOutput, LineageScope};
use crate::config::{self, ConfigMode};
use crate::util::{self, Source};

pub(crate) fn run(
    dialect: &AnyDialect,
    config_mode: &ConfigMode<'_>,
    args: &LineageArgs,
) -> Result<(), String> {
    let runner = LineageRun::new(dialect, config_mode, args)?;
    let sources = util::load_sources(&args.files, args.expression.as_deref())?;
    let renderer = select_renderer(args.output);
    if runner.run(&sources, renderer.as_ref()) {
        std::process::exit(1);
    }
    Ok(())
}

struct LineageRun<'a> {
    dialect: &'a AnyDialect,
    schema_catalog: Catalog,
    validation: AnalysisConfig,
    scope: Option<LineageScope>,
    lang: Option<HostLanguage>,
}

impl<'a> LineageRun<'a> {
    fn new(
        dialect: &'a AnyDialect,
        config_mode: &ConfigMode<'_>,
        args: &LineageArgs,
    ) -> Result<Self, String> {
        let file_config = config::resolve(config_mode);
        let schemas = if args.schema.is_empty() {
            util::schemas_from_project_config(&args.files, file_config.as_ref())
        } else {
            args.schema.clone()
        };
        let schema_catalog = util::build_schema_catalog(dialect, &schemas)?;
        Ok(Self {
            dialect,
            schema_catalog,
            validation: AnalysisConfig::default(),
            scope: args.scope,
            lang: args.lang,
        })
    }

    /// Returns `true` if any statement produced an error-level diagnostic.
    fn run(&self, sources: &[Source], renderer: &dyn Renderer) -> bool {
        let mut any_errors = false;
        for src in sources {
            let records = self.analyze(src);
            any_errors |= records.iter().any(|r| matches!(r, Record::Error(_)));
            for record in &records {
                renderer.render(record);
            }
        }
        any_errors
    }

    // ── Computation ────────────────────────────────────────────────────────

    fn analyze(&self, src: &Source) -> Vec<Record> {
        match util::resolve_language(self.lang, src, self.dialect) {
            Some(lang) => {
                let mut records = Vec::new();
                let mut base = 0;
                for fragment in syntaqlite::embedded::extract(self.dialect.clone(), lang, &src.text)
                {
                    let (mut frag, count) =
                        self.analyze_text(fragment.sql_text(), &src.label, base);
                    records.append(&mut frag);
                    base = base.saturating_add(count);
                }
                records
            }
            None => self.analyze_text(&src.text, &src.label, 0).0,
        }
    }

    /// Analyze `text` as standalone SQL, returning its lineage records and the
    /// number of statements (so callers can keep `statement_index` running
    /// across the fragments of an embedded source). `base_index` offsets the
    /// per-statement index reported in records.
    fn analyze_text(&self, text: &str, label: &str, base_index: u32) -> (Vec<Record>, u32) {
        let mut analyzer = Analyzer::with_dialect(self.dialect.clone());
        let mut catalog = self.schema_catalog.clone();
        let mut ctx = syntaqlite::AnalysisContext::new(&mut catalog).with_config(self.validation);
        let model = analyzer.analyze(text, &mut ctx);
        let statements = model.statements();
        let mut records = Vec::new();
        for (idx, stmt) in statements.iter().enumerate() {
            let idx = base_index.saturating_add(u32::try_from(idx).unwrap_or(u32::MAX));
            let errors: Vec<_> = stmt
                .diagnostics()
                .iter()
                .filter(|d| d.severity() == Severity::Error)
                .collect();
            if errors.is_empty() {
                records.push(Record::Lineage(build_lineage_record(
                    stmt, label, idx, self.scope,
                )));
            } else {
                for d in &errors {
                    records.push(Record::Error(build_error_record(d, label, idx)));
                }
            }
        }
        (records, u32::try_from(statements.len()).unwrap_or(u32::MAX))
    }
}

/// One emitted record — either a statement's lineage, or an error describing
/// why lineage could not be produced.
enum Record {
    Lineage(LineageRecord),
    Error(ErrorRecord),
}

// ── Renderer strategy ──────────────────────────────────────────────────────

/// Output strategy for emitted records. Adding a new `--output` mode is a
/// new struct + `impl Renderer`; the [`LineageRun`] doesn't change.
trait Renderer {
    fn render(&self, record: &Record);
}

fn select_renderer(output: LineageOutput) -> Box<dyn Renderer> {
    match output {
        LineageOutput::Json => Box::new(JsonRenderer),
        LineageOutput::Text => Box::new(TextRenderer),
    }
}

struct JsonRenderer;

impl Renderer for JsonRenderer {
    fn render(&self, record: &Record) {
        let result = match record {
            Record::Lineage(r) => serde_json::to_string(r),
            Record::Error(r) => serde_json::to_string(r),
        };
        match result {
            Ok(s) => println!("{s}"),
            Err(e) => eprintln!("error serializing lineage record: {e}"),
        }
    }
}

struct TextRenderer;

impl Renderer for TextRenderer {
    fn render(&self, record: &Record) {
        match record {
            Record::Lineage(r) => print_lineage_text(r),
            Record::Error(r) => print_error_text(r),
        }
    }
}

fn build_error_record(d: &Diagnostic, file: &str, index: u32) -> ErrorRecord {
    let stage = if let DiagnosticMessage::ParseError(_) = d.message() {
        ErrorStage::Parse
    } else {
        ErrorStage::Validate
    };
    ErrorRecord {
        kind: "error",
        schema_version: SCHEMA_VERSION,
        file: file.to_string(),
        statement_index: index,
        stage,
        message: format!("{}", d.message()),
    }
}

fn build_lineage_record(
    stmt: &StatementAnalysis,
    file: &str,
    index: u32,
    scope: Option<LineageScope>,
) -> LineageRecord {
    let (status, partial_reasons) = if stmt.unexpanded_views().is_empty() {
        (Status::Complete, Vec::new())
    } else {
        let reasons = stmt
            .unexpanded_views()
            .iter()
            .map(|v| JsonPartialReason::UnexpandedView { view: v.clone() })
            .collect();
        (Status::Partial, reasons)
    };

    let target = stmt.defined_relations().first().map(|d| JsonTarget {
        name: d.name.clone(),
        kind: if d.is_view {
            JsonTargetKind::View
        } else {
            JsonTargetKind::Table
        },
    });

    let include_columns = matches!(scope, None | Some(LineageScope::Columns));
    let include_tables = matches!(scope, None | Some(LineageScope::Tables));

    LineageRecord {
        kind: "lineage",
        schema_version: SCHEMA_VERSION,
        file: file.to_string(),
        statement_index: index,
        status,
        partial_reasons,
        target,
        columns: include_columns.then(|| collect_columns(stmt)),
        relations: include_tables.then(|| collect_relations(stmt)),
        physical_tables: include_tables.then(|| collect_physical_tables(stmt)),
    }
}

fn collect_columns(stmt: &StatementAnalysis) -> Vec<JsonColumn> {
    stmt.lineage()
        .map(|l| {
            l.into_inner()
                .iter()
                .map(|c| JsonColumn {
                    name: c.name.clone(),
                    index: c.index,
                    origin: c.origin.as_ref().map(|o| JsonOrigin {
                        table: o.table.clone(),
                        column: o.column.clone(),
                    }),
                })
                .collect()
        })
        .unwrap_or_default()
}

fn collect_relations(stmt: &StatementAnalysis) -> Vec<JsonRelation> {
    stmt.relations_accessed()
        .map(|r| {
            r.into_inner()
                .iter()
                .map(|r| JsonRelation {
                    name: r.name.clone(),
                    kind: match r.kind {
                        RelationKind::Table => JsonRelationKind::Table,
                        RelationKind::View => JsonRelationKind::View,
                    },
                })
                .collect()
        })
        .unwrap_or_default()
}

fn collect_physical_tables(stmt: &StatementAnalysis) -> Vec<JsonPhysicalTable> {
    stmt.physical_tables_accessed()
        .map(|t| {
            t.into_inner()
                .iter()
                .map(|t| JsonPhysicalTable {
                    name: t.name.clone(),
                })
                .collect()
        })
        .unwrap_or_default()
}

fn print_error_text(record: &ErrorRecord) {
    let stage = match record.stage {
        ErrorStage::Parse => "parse",
        ErrorStage::Validate => "analyze",
    };
    println!("Error");
    println!("  statement: {}", record.statement_index);
    println!("  stage: {stage}");
    println!("  message: {}", record.message);
}

fn print_lineage_text(record: &LineageRecord) {
    let status = match record.status {
        Status::Complete => "complete",
        Status::Partial => "partial",
    };
    println!("Lineage");
    println!("  statement: {}", record.statement_index);
    println!("  status: {status}");
    print_target(record.target.as_ref());
    print_columns(record.columns.as_deref());
    print_relations(record.relations.as_deref());
    print_physical_tables(record.physical_tables.as_deref());
    print_partial_reasons(&record.partial_reasons);
}

fn print_target(target: Option<&JsonTarget>) {
    if let Some(t) = target {
        let kind = match t.kind {
            JsonTargetKind::Table => "table",
            JsonTargetKind::View => "view",
        };
        println!("  target: {} ({kind})", t.name);
    } else {
        println!("  target: (none)");
    }
}

fn print_columns(columns: Option<&[JsonColumn]>) {
    let Some(columns) = columns else { return };
    if columns.is_empty() {
        println!("  columns: (none)");
        return;
    }
    println!("  columns:");
    for c in columns {
        if let Some(o) = &c.origin {
            println!("    {} <- {}.{}", c.name, o.table, o.column);
        } else {
            println!("    {} <- (transformed)", c.name);
        }
    }
}

fn print_relations(relations: Option<&[JsonRelation]>) {
    let Some(relations) = relations else { return };
    if relations.is_empty() {
        println!("  relations: (none)");
        return;
    }
    println!("  relations:");
    for r in relations {
        let kind = match r.kind {
            JsonRelationKind::Table => "table",
            JsonRelationKind::View => "view",
        };
        println!("    {} ({kind})", r.name);
    }
}

fn print_physical_tables(physical_tables: Option<&[JsonPhysicalTable]>) {
    let Some(physical_tables) = physical_tables else {
        return;
    };
    if physical_tables.is_empty() {
        println!("  physical_tables: (none)");
        return;
    }
    println!("  physical_tables:");
    for t in physical_tables {
        println!("    {}", t.name);
    }
}

fn print_partial_reasons(reasons: &[JsonPartialReason]) {
    if reasons.is_empty() {
        println!("  partial_reasons: (none)");
        return;
    }
    println!("  partial_reasons:");
    for reason in reasons {
        let JsonPartialReason::UnexpandedView { view } = reason;
        println!("    unexpanded_view: {view}");
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
enum Status {
    Complete,
    Partial,
}

#[derive(Serialize)]
struct JsonOrigin {
    table: String,
    column: String,
}

#[derive(Serialize)]
struct JsonColumn {
    name: String,
    index: u32,
    origin: Option<JsonOrigin>,
}

#[derive(Serialize)]
#[serde(rename_all = "lowercase")]
enum JsonRelationKind {
    Table,
    View,
}

#[derive(Serialize)]
struct JsonRelation {
    name: String,
    kind: JsonRelationKind,
}

#[derive(Serialize)]
struct JsonPhysicalTable {
    name: String,
}

#[derive(Serialize)]
#[serde(rename_all = "lowercase")]
enum JsonTargetKind {
    Table,
    View,
}

#[derive(Serialize)]
struct JsonTarget {
    name: String,
    kind: JsonTargetKind,
}

/// Partial-reason entry. Only `unexpanded_view` exists today; additional
/// codes will be added as more causes of partial lineage surface.
#[derive(Serialize)]
#[serde(tag = "code", rename_all = "snake_case")]
enum JsonPartialReason {
    UnexpandedView { view: String },
}

#[derive(Serialize)]
struct LineageRecord {
    kind: &'static str,
    schema_version: u32,
    file: String,
    statement_index: u32,
    status: Status,
    partial_reasons: Vec<JsonPartialReason>,
    target: Option<JsonTarget>,
    #[serde(skip_serializing_if = "Option::is_none")]
    columns: Option<Vec<JsonColumn>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    relations: Option<Vec<JsonRelation>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    physical_tables: Option<Vec<JsonPhysicalTable>>,
}

#[derive(Serialize)]
#[serde(rename_all = "lowercase")]
enum ErrorStage {
    Parse,
    Validate,
}

#[derive(Serialize)]
struct ErrorRecord {
    kind: &'static str,
    schema_version: u32,
    file: String,
    statement_index: u32,
    stage: ErrorStage,
    message: String,
}
