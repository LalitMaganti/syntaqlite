// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Statement-level analysis passes plus the per-statement composition.
//!
//! - [`DiagnosticsPass`] consumes walker events and emits a `Vec<Diagnostic>`.
//! - [`StatementWalkPass`] is the analyzer's per-statement composition: it
//!   holds `DiagnosticsPass` plus an external user-supplied pass (LSP's
//!   capture pass, embedded-SQL extractor, etc.) and implements `WalkPass`
//!   by delegating to each at every hook. The composition is written out by
//!   hand, not generated.

use syntaqlite_syntax::any::{AnyNodeId, AnyParseError, AnyParsedStatement};
use syntaqlite_syntax::source::{DocRange, LayerRange};

use crate::semantic::catalog::{ColumnResolution, FunctionCheckResult};
use crate::semantic::diagnostics::{Diagnostic, DiagnosticMessage, Help};
use crate::semantic::fuzzy::best_suggestion;
use crate::semantic::{CheckConfig, CheckLevel, ValidationConfig};

use super::walker::{
    CallEvent, ColumnRefEvent, CteColumnCountMismatchEvent, SourceRefEvent, WalkCtx, WalkPass,
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

// ── DiagnosticsPass ───────────────────────────────────────────────────────────

/// Emits semantic diagnostics (`unknown-table`, `unknown-column`, etc.) into a
/// `Vec<Diagnostic>`.
pub(super) struct DiagnosticsPass<'a> {
    config: &'a ValidationConfig,
    diagnostics: &'a mut Vec<Diagnostic>,
}

impl<'a> DiagnosticsPass<'a> {
    pub(super) fn new(config: &'a ValidationConfig, diagnostics: &'a mut Vec<Diagnostic>) -> Self {
        Self {
            config,
            diagnostics,
        }
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

impl WalkPass for DiagnosticsPass<'_> {
    const WANTS_SOURCE_REF: bool = true;
    const WANTS_COLUMN_REF: bool = true;
    const WANTS_CALL: bool = true;
    const WANTS_CTE_COLUMN_COUNT: bool = true;

    fn on_source_ref(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: SourceRefEvent<'_>,
    ) {
        if ev.resolved {
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
            ColumnResolution::Found { .. } | ColumnResolution::TableNotFound => {}
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

// ── StatementWalkPass: hand-written composition ───────────────────────────────

/// Analyzer's per-statement composition: [`DiagnosticsPass`] plus an external
/// user-supplied pass. Implements [`WalkPass`] by writing out each hook by
/// hand, calling both inner passes in sequence.
pub(super) struct StatementWalkPass<'a, P: WalkPass> {
    pub(super) diagnostics: DiagnosticsPass<'a>,
    pub(super) extra: &'a mut P,
}

impl<P: WalkPass> WalkPass for StatementWalkPass<'_, P> {
    const WANTS_SOURCE_REF: bool = DiagnosticsPass::WANTS_SOURCE_REF || P::WANTS_SOURCE_REF;
    const WANTS_COLUMN_REF: bool = DiagnosticsPass::WANTS_COLUMN_REF || P::WANTS_COLUMN_REF;
    const WANTS_CALL: bool = DiagnosticsPass::WANTS_CALL || P::WANTS_CALL;
    const WANTS_RELATION_DEFINITION: bool =
        DiagnosticsPass::WANTS_RELATION_DEFINITION || P::WANTS_RELATION_DEFINITION;
    const WANTS_COLUMN_DEFINITION: bool =
        DiagnosticsPass::WANTS_COLUMN_DEFINITION || P::WANTS_COLUMN_DEFINITION;
    const WANTS_CTE_COLUMN_COUNT: bool =
        DiagnosticsPass::WANTS_CTE_COLUMN_COUNT || P::WANTS_CTE_COLUMN_COUNT;
    // DiagnosticsPass never wants statement context; only the extra pass can.
    const WANTS_STATEMENT_CONTEXT: bool = P::WANTS_STATEMENT_CONTEXT;

    fn on_source_ref(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: SourceRefEvent<'_>,
    ) {
        self.diagnostics.on_source_ref(stmt, cx, ev);
        self.extra.on_source_ref(stmt, cx, ev);
    }

    fn on_column_ref(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: ColumnRefEvent<'_>,
    ) {
        self.diagnostics.on_column_ref(stmt, cx, ev);
        self.extra.on_column_ref(stmt, cx, ev);
    }

    fn on_call(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: CallEvent<'_>,
    ) {
        self.diagnostics.on_call(stmt, cx, ev);
        self.extra.on_call(stmt, cx, ev);
    }

    fn on_relation_definition(&mut self, name: &str, range: DocRange) {
        self.diagnostics.on_relation_definition(name, range);
        self.extra.on_relation_definition(name, range);
    }

    fn on_column_definition(&mut self, table: &str, column: &str, range: DocRange) {
        self.diagnostics.on_column_definition(table, column, range);
        self.extra.on_column_definition(table, column, range);
    }

    fn on_cte_column_count_mismatch(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        ev: CteColumnCountMismatchEvent<'_>,
    ) {
        self.diagnostics.on_cte_column_count_mismatch(stmt, ev);
        self.extra.on_cte_column_count_mismatch(stmt, ev);
    }

    fn on_parsed_statement(&mut self, stmt: &AnyParsedStatement<'_>) {
        // DiagnosticsPass has nothing to do here; only forward to extra.
        self.extra.on_parsed_statement(stmt);
    }

    fn on_parse_error(&mut self, err: &AnyParseError<'_>) {
        self.extra.on_parse_error(err);
    }
}
