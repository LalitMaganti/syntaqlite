// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Per-statement [`SemanticVisitor`] used by the analyzer.
//!
//! [`AnalysisVisitor`] is a single [`SemanticVisitor`] that does two
//! things on every event: emits semantic diagnostics
//! (`unknown-table`, `unknown-column`, etc.) into a `Vec<Diagnostic>`,
//! and forwards the same event to a user-supplied visitor. The
//! forwarding is written out by hand, not generated — explicit and
//! grep-able.

use syntaqlite_syntax::any::{AnyNodeId, AnyParseError, AnyParsedStatement};
use syntaqlite_syntax::source::{DocRange, LayerRange};

use crate::analysis::catalog::{ColumnResolution, FunctionCheckResult};
use crate::analysis::diagnostics::fuzzy::best_suggestion;
use crate::analysis::diagnostics::{Diagnostic, DiagnosticMessage, Help};
use crate::analysis::lineage::LineageCapture;
use crate::analysis::{AnalysisConfig, CheckConfig, CheckLevel};
use crate::dialect::SemanticRole;

use super::walker::{
    CallEvent, ColumnRefEvent, CteBindingEvent, CteColumnCountMismatchEvent, ScopedSourceEvent,
    SemanticVisitor, SourceRefEvent, WalkCtx,
};

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

// ── AnalysisVisitor ───────────────────────────────────────────────────────────

/// The analyzer's per-statement visitor. Emits diagnostics into
/// `diagnostics`, feeds CTE / Query / scoped-source events into an
/// internal [`LineageCapture`], and forwards every event to `extra`.
pub(super) struct AnalysisVisitor<'a, V: SemanticVisitor> {
    config: &'a AnalysisConfig,
    diagnostics: &'a mut Vec<Diagnostic>,
    lineage: LineageCapture,
    extra: &'a mut V,
}

impl<'a, V: SemanticVisitor> AnalysisVisitor<'a, V> {
    pub(super) fn new(
        config: &'a AnalysisConfig,
        diagnostics: &'a mut Vec<Diagnostic>,
        roles: &'static [SemanticRole],
        extra: &'a mut V,
    ) -> Self {
        Self {
            config,
            diagnostics,
            lineage: LineageCapture::new(roles),
            extra,
        }
    }

    pub(super) fn into_lineage(self) -> LineageCapture {
        self.lineage
    }

    /// Push a diagnostic anchored to a span field of a node, including a
    /// macro-expansion traceback if the span crossed an expansion boundary.
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
            .map(|f| crate::analysis::diagnostics::DiagnosticFrame {
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

impl<V: SemanticVisitor> SemanticVisitor for AnalysisVisitor<'_, V> {
    const WANTS_SOURCE_REF: bool = true;
    const WANTS_COLUMN_REF: bool = true;
    const WANTS_CALL: bool = true;
    const WANTS_RELATION_DEFINITION: bool = V::WANTS_RELATION_DEFINITION;
    const WANTS_COLUMN_DEFINITION: bool = V::WANTS_COLUMN_DEFINITION;
    const WANTS_CTE_COLUMN_COUNT: bool = true;
    // LineageCapture wants source refs, scoped sources, CTE bindings,
    // and enter/exit — all forwarded below.
    const WANTS_CTE_BINDING: bool = true;
    const WANTS_SCOPED_SOURCE: bool = true;
    const WANTS_QUERY: bool = true;
    const WANTS_STATEMENT_CONTEXT: bool = V::WANTS_STATEMENT_CONTEXT;

    fn on_source_ref(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: SourceRefEvent<'_>,
    ) {
        if !ev.resolved {
            let mut candidates = cx.catalog.all_relation_names();
            candidates.extend(cx.catalog.all_table_function_names());
            let suggestion =
                best_suggestion(ev.name, &candidates, self.config.suggestion_threshold());
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
        self.lineage.on_source_ref(stmt, cx, ev);
        self.extra.on_source_ref(stmt, cx, ev);
    }

    fn on_column_ref(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: ColumnRefEvent<'_>,
    ) {
        match ev.resolution {
            ColumnResolution::Found { .. } | ColumnResolution::TableNotFound => {}
            ColumnResolution::TableFoundColumnMissing => {
                // DQS bug-compat: unresolved `"foo"` is re-interpreted as a
                // string literal by SQLite. Don't FP here.
                if !ev.dqs_candidate {
                    let tbl = ev
                        .table
                        .expect("qualifier present when TableFoundColumnMissing");
                    let candidates = cx.scope.all_column_names(Some(tbl));
                    let suggestion = best_suggestion(
                        ev.column,
                        &candidates,
                        self.config.suggestion_threshold(),
                    );
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
            }
            ColumnResolution::NotFound => {
                // SQLite resolves bare TRUE/FALSE identifiers to integer
                // literals.
                let is_bool_literal = ev.column.eq_ignore_ascii_case("true")
                    || ev.column.eq_ignore_ascii_case("false");
                if !is_bool_literal && !ev.dqs_candidate {
                    let candidates = cx.scope.all_column_names(None);
                    let suggestion = best_suggestion(
                        ev.column,
                        &candidates,
                        self.config.suggestion_threshold(),
                    );
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
        self.extra.on_column_ref(stmt, cx, ev);
    }

    fn on_call(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: CallEvent<'_>,
    ) {
        match ev.result {
            FunctionCheckResult::Ok => {}
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
                        expected: expected.clone(),
                        got: ev.arg_count,
                    },
                    None,
                );
            }
        }
        self.extra.on_call(stmt, cx, ev);
    }

    fn on_relation_definition(&mut self, name: &str, range: DocRange) {
        self.extra.on_relation_definition(name, range);
    }

    fn on_column_definition(&mut self, table: &str, column: &str, range: DocRange) {
        self.extra.on_column_definition(table, column, range);
    }

    fn on_cte_column_count_mismatch(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
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
        self.extra.on_cte_column_count_mismatch(stmt, ev);
    }

    fn on_cte_binding(&mut self, stmt: &mut AnyParsedStatement<'_>, ev: CteBindingEvent<'_>) {
        self.lineage.on_cte_binding(stmt, ev);
        self.extra.on_cte_binding(stmt, ev);
    }

    fn on_scoped_source(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        ev: ScopedSourceEvent<'_>,
    ) {
        self.lineage.on_scoped_source(stmt, ev);
        self.extra.on_scoped_source(stmt, ev);
    }

    fn enter_query(&mut self, stmt: &mut AnyParsedStatement<'_>, node_id: AnyNodeId) {
        self.lineage.enter_query(stmt, node_id);
        self.extra.enter_query(stmt, node_id);
    }

    fn exit_query(&mut self, stmt: &mut AnyParsedStatement<'_>, node_id: AnyNodeId) {
        self.lineage.exit_query(stmt, node_id);
        self.extra.exit_query(stmt, node_id);
    }

    fn on_parsed_statement(&mut self, stmt: &AnyParsedStatement<'_>) {
        self.extra.on_parsed_statement(stmt);
    }

    fn on_parse_error(&mut self, err: &AnyParseError<'_>) {
        self.extra.on_parse_error(err);
    }
}
