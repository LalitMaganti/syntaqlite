// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Statement-level validation pass.
//!
//! [`ValidationPass`] is a [`WalkPass`] implementor: the generic walker in
//! [`super::walker`] handles role dispatch, scope maintenance, and event
//! construction; `ValidationPass` decides what to do with each event
//! (emit a diagnostic, forward to the observer, etc.).

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement};
use syntaqlite_syntax::source::{DocRange, LayerRange};

use crate::dialect::AnyDialect;
use crate::semantic::catalog::{Catalog, ColumnResolution, FunctionCheckResult};
use crate::semantic::diagnostics::{Diagnostic, DiagnosticMessage, Help};
use crate::semantic::fuzzy::best_suggestion;
use crate::semantic::observer::AnalysisObserver;
use crate::semantic::{CheckConfig, CheckLevel, ValidationConfig};

use super::query_scope::QueryScope;
use super::walker::{
    self, CallEvent, ColumnRefEvent, CteColumnCountMismatchEvent, SourceRefEvent, WalkCtx, WalkPass,
};

pub(super) struct ValidationPass<'a> {
    config: &'a ValidationConfig,
    diagnostics: &'a mut Vec<Diagnostic>,
    observer: &'a mut dyn AnalysisObserver,
}

impl CheckConfig {
    /// Get the check level for a diagnostic message's category.
    pub(crate) fn level_for(self, message: &DiagnosticMessage) -> CheckLevel {
        match message {
            DiagnosticMessage::UnknownTable { .. } | DiagnosticMessage::UnknownModule { .. } => {
                self.unknown_table
            }
            DiagnosticMessage::UnknownColumn { .. } => self.unknown_column,
            DiagnosticMessage::UnknownFunction { .. } => self.unknown_function,
            DiagnosticMessage::FunctionArity { .. } => self.function_arity,
            DiagnosticMessage::CteColumnCountMismatch { .. } => self.cte_columns,
            DiagnosticMessage::ParseError(_) => self.parse_errors,
        }
    }
}

impl<'a> ValidationPass<'a> {
    pub(super) fn run<'b>(
        stmt: &mut AnyParsedStatement<'b>,
        root: AnyNodeId,
        dialect: &AnyDialect,
        catalog: &'a mut Catalog,
        config: &'a ValidationConfig,
        diagnostics: &'a mut Vec<Diagnostic>,
        observer: &'a mut dyn AnalysisObserver,
    ) {
        let mut pass = ValidationPass {
            config,
            diagnostics,
            observer,
        };
        let mut cx = WalkCtx {
            roles: dialect.roles(),
            catalog,
            scope: QueryScope::default(),
        };
        walker::walk(stmt, &mut cx, &mut pass, root);
    }

    /// Push a diagnostic anchored to a span field of a node.
    fn emit(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        node_id: AnyNodeId,
        field_idx: u8,
        range: DocRange,
        message: DiagnosticMessage,
        help: Option<Help>,
    ) {
        let Some(severity) = self.config.checks().level_for(&message).to_severity() else {
            return;
        };
        let frames = stmt
            .traceback(node_id, field_idx)
            .map(|f| crate::semantic::diagnostics::DiagnosticFrame {
                buffer: f.snippet.as_str().to_string(),
                range: LayerRange::from_offset_len(f.offset_in_snippet, f.length_in_snippet),
            })
            .collect::<Vec<_>>();
        // Only attach if there's actual expansion (more than 1 frame).
        let expansion_frames = if frames.len() > 1 { frames } else { Vec::new() };
        self.diagnostics.push(Diagnostic {
            range,
            message,
            severity,
            help,
            expansion_frames,
        });
    }

    /// Push a diagnostic with an explicit source range and no expansion
    /// traceback.
    fn emit_at(&mut self, range: DocRange, message: DiagnosticMessage, help: Option<Help>) {
        if let Some(severity) = self.config.checks().level_for(&message).to_severity() {
            self.diagnostics.push(Diagnostic {
                range,
                message,
                severity,
                help,
                expansion_frames: Vec::new(),
            });
        }
    }
}

impl WalkPass for ValidationPass<'_> {
    const WANTS_SOURCE_REF: bool = true;
    const WANTS_COLUMN_REF: bool = true;
    const WANTS_CALL: bool = true;
    const WANTS_RELATION_DEFINITION: bool = true;
    const WANTS_COLUMN_DEFINITION: bool = true;
    const WANTS_CTE_COLUMN_COUNT: bool = true;

    fn on_source_ref(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: SourceRefEvent<'_>,
    ) {
        if ev.resolved {
            if self.observer.wants_references() {
                self.observer
                    .on_table_reference(ev.range, ev.name, ev.columns.as_deref());
            }
            return;
        }
        let mut candidates = cx.catalog.all_relation_names();
        candidates.extend(cx.catalog.all_table_function_names());
        let suggestion = best_suggestion(ev.name, &candidates, self.config.suggestion_threshold());
        self.emit(
            stmt,
            ev.node_id,
            ev.name_idx,
            ev.range,
            DiagnosticMessage::UnknownTable {
                name: ev.name.to_string(),
            },
            suggestion.map(Help::Suggestion),
        );
    }

    fn on_column_ref(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: ColumnRefEvent<'_>,
    ) {
        match ev.resolution {
            ColumnResolution::Found {
                table: resolved_table,
                all_columns,
            } => {
                if self.observer.wants_references() && !resolved_table.is_empty() {
                    self.observer.on_column_reference(
                        ev.range,
                        &resolved_table,
                        ev.column,
                        &all_columns,
                    );
                }
            }
            ColumnResolution::TableNotFound => {}
            ColumnResolution::TableFoundColumnMissing => {
                let tbl = ev
                    .table
                    .expect("qualifier present when TableFoundColumnMissing");
                let candidates = cx.scope.all_column_names(Some(tbl));
                let suggestion =
                    best_suggestion(ev.column, &candidates, self.config.suggestion_threshold());
                self.emit(
                    stmt,
                    ev.node_id,
                    ev.column_idx,
                    ev.range,
                    DiagnosticMessage::UnknownColumn {
                        column: ev.column.to_string(),
                        table: Some(tbl.to_string()),
                    },
                    suggestion.map(Help::Suggestion),
                );
            }
            ColumnResolution::NotFound => {
                // SQLite resolves bare TRUE/FALSE identifiers to integer literals.
                if ev.column.eq_ignore_ascii_case("true") || ev.column.eq_ignore_ascii_case("false")
                {
                    return;
                }
                let candidates = cx.scope.all_column_names(None);
                let suggestion =
                    best_suggestion(ev.column, &candidates, self.config.suggestion_threshold());
                self.emit(
                    stmt,
                    ev.node_id,
                    ev.column_idx,
                    ev.range,
                    DiagnosticMessage::UnknownColumn {
                        column: ev.column.to_string(),
                        table: None,
                    },
                    suggestion.map(Help::Suggestion),
                );
            }
        }
    }

    fn on_call(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: CallEvent<'_>,
    ) {
        match ev.result {
            FunctionCheckResult::Ok => {
                if self.observer.wants_references()
                    && let Some((cat, arities)) = ev.signature
                {
                    self.observer
                        .on_function_reference(ev.range, ev.name, cat, &arities);
                }
            }
            FunctionCheckResult::Unknown => {
                let candidates = cx.catalog.all_function_names();
                let suggestion =
                    best_suggestion(ev.name, &candidates, self.config.suggestion_threshold());
                self.emit(
                    stmt,
                    ev.node_id,
                    ev.name_idx,
                    ev.range,
                    DiagnosticMessage::UnknownFunction {
                        name: ev.name.to_string(),
                    },
                    suggestion.map(Help::Suggestion),
                );
            }
            FunctionCheckResult::WrongArity { expected } => {
                self.emit(
                    stmt,
                    ev.node_id,
                    ev.name_idx,
                    ev.range,
                    DiagnosticMessage::FunctionArity {
                        name: ev.name.to_string(),
                        expected,
                        got: ev.arg_count,
                    },
                    None,
                );
            }
        }
    }

    fn on_relation_definition(&mut self, name: &str, range: DocRange) {
        if self.observer.wants_definitions() {
            self.observer.on_relation_definition(name, range);
        }
    }

    fn on_column_definition(&mut self, table: &str, column: &str, range: DocRange) {
        if self.observer.wants_definitions() {
            self.observer.on_column_definition(table, column, range);
        }
    }

    fn on_cte_column_count_mismatch(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        ev: CteColumnCountMismatchEvent<'_>,
    ) {
        self.emit_at(
            ev.name_range,
            DiagnosticMessage::CteColumnCountMismatch {
                name: ev.name.to_string(),
                declared: ev.declared,
                actual: ev.actual,
            },
            None,
        );
    }
}
