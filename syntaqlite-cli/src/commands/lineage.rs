// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `lineage` subcommand.

use serde::Serialize;
use syntaqlite::Diagnostic;
use syntaqlite::any::AnyDialect;
use syntaqlite::semantic::{
    DiagnosticMessage, RelationKind, SemanticAnalyzer, Severity, StatementModel,
};
use syntaqlite::{Catalog, ValidationConfig};

use crate::cli::{LineageArgs, LineageOutput, LineageScope};
use crate::config::{self, ConfigMode};
use crate::util::{self, Source};

pub(crate) fn run(
    dialect: &AnyDialect,
    config_mode: &ConfigMode<'_>,
    args: &LineageArgs,
) -> Result<(), String> {
    let runner = LineageRun::new(dialect, config_mode, args)?;
    let sources = util::load_sources(&args.files, args.expression.as_deref())?;
    if runner.run(&sources) {
        std::process::exit(1);
    }
    Ok(())
}

struct LineageRun<'a> {
    dialect: &'a AnyDialect,
    schema_catalog: Catalog,
    validation: ValidationConfig,
    output: LineageOutput,
    scope: Option<LineageScope>,
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
            validation: ValidationConfig::default(),
            output: args.output,
            scope: args.scope,
        })
    }

    /// Returns `true` if any statement produced an error-level diagnostic.
    fn run(&self, sources: &[Source]) -> bool {
        let mut any_errors = false;
        for src in sources {
            if self.process_source(src) {
                any_errors = true;
            }
        }
        any_errors
    }

    fn process_source(&self, src: &Source) -> bool {
        let mut analyzer = SemanticAnalyzer::with_dialect(self.dialect.clone());
        let model = analyzer.analyze(&src.text, &self.schema_catalog, &self.validation);
        let mut had_error = false;
        for (idx, stmt) in model.statements().iter().enumerate() {
            let idx = u32::try_from(idx).unwrap_or(u32::MAX);
            let errors: Vec<_> = stmt
                .diagnostics()
                .iter()
                .filter(|d| d.severity() == Severity::Error)
                .collect();
            if errors.is_empty() {
                self.emit_lineage(stmt, &src.label, idx);
            } else {
                for d in &errors {
                    self.emit_error(d, &src.label, idx);
                }
                had_error = true;
            }
        }
        had_error
    }

    fn emit_lineage(&self, stmt: &StatementModel, file: &str, index: u32) {
        let record = build_lineage_record(stmt, file, index, self.scope);
        match self.output {
            LineageOutput::Json => match serde_json::to_string(&record) {
                Ok(s) => println!("{s}"),
                Err(e) => eprintln!("error serializing lineage record: {e}"),
            },
            LineageOutput::Text => print_lineage_text(&record),
        }
    }

    fn emit_error(&self, d: &Diagnostic, file: &str, index: u32) {
        let stage = if let DiagnosticMessage::ParseError(_) = d.message() {
            ErrorStage::Parse
        } else {
            ErrorStage::Validate
        };
        let record = ErrorRecord {
            kind: "error",
            schema_version: SCHEMA_VERSION,
            file: file.to_string(),
            statement_index: index,
            stage,
            message: format!("{}", d.message()),
        };
        match self.output {
            LineageOutput::Json => match serde_json::to_string(&record) {
                Ok(s) => println!("{s}"),
                Err(e) => eprintln!("error serializing error record: {e}"),
            },
            LineageOutput::Text => print_error_text(&record),
        }
    }
}

fn build_lineage_record(
    stmt: &StatementModel,
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

fn collect_columns(stmt: &StatementModel) -> Vec<JsonColumn> {
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

fn collect_relations(stmt: &StatementModel) -> Vec<JsonRelation> {
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

fn collect_physical_tables(stmt: &StatementModel) -> Vec<JsonPhysicalTable> {
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
        ErrorStage::Validate => "validate",
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
