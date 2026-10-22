// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Per-statement [`SemanticVisitor`] used by the analyzer.
//!
//! [`StatementVisitor`] is a single [`SemanticVisitor`] that does two
//! things on every event: emits semantic diagnostics
//! (`unknown-table`, `unknown-column`, etc.) into a `Vec<Diagnostic>`,
//! and forwards the same event to a user-supplied visitor. The
//! forwarding is written out by hand, not generated — explicit and
//! grep-able.

use syntaqlite_syntax::any::{AnyNodeId, AnyParseError, AnyParsedStatement};
use syntaqlite_syntax::source::{DocRange, LayerRange};

use crate::analysis::catalog::{Catalog, ColumnResolution, FunctionCheckResult};
use crate::analysis::diagnostics::fuzzy::best_suggestion;
use crate::analysis::diagnostics::{Diagnostic, DiagnosticMessage, Help};
use crate::analysis::lineage::LineageCapture;
use crate::analysis::{AnalysisConfig, CheckConfig, CheckLevel};
use crate::dialect::SemanticRole;

use super::walker::{
    BuiltinModifyEvent, CallEvent, ColumnRefEvent, CompoundColumnsEvent, CteBindingEvent,
    CteColumnCountMismatchEvent, DropWrongKindEvent, IndexOnViewEvent, InsertValuesEvent,
    OnClauseOrderEvent, RowValueEvent, ScopedSourceEvent, SemanticVisitor, SourceRefEvent,
    SubqueryColumnsEvent, UncheckedRef, UnknownWindowEvent, ValuesTermsEvent, WalkCtx,
};

/// Whether `name` is an embedded-SQL interpolation placeholder: an identifier of
/// only underscores. Embedded host languages (Python f-strings, etc.) mask each
/// `{expr}` hole with same-length underscores so the SQL keeps its byte offsets;
/// such names stand in for arbitrary host expressions, so they always resolve —
/// a real schema never names a table/column/function with only underscores.
fn is_hole_placeholder(name: &str) -> bool {
    !name.is_empty() && name.bytes().all(|b| b == b'_')
}

/// Why an unresolved column reference is intentionally accepted without a
/// diagnostic.
///
/// Naming a variant here is the only sanctioned way to skip a diagnostic on
/// a resolution failure — never a bare `=> {}` arm (issue #281: exactly
/// that silently hid every unknown qualifier to protect one unmodeled
/// construct). Variants either model real `SQLite` behavior (permanent) or
/// name an unmodeled construct (tracked debt: cite the construct, delete
/// the variant once the walker registers it in scope).
enum AcceptedRef {
    /// `SQLite` DQS bug-compat: an unresolved `"foo"` written with double
    /// quotes in the original source re-reads as a string literal.
    DqsStringLiteral,
    /// `SQLite` resolves bare TRUE/FALSE identifiers to integer literals.
    BooleanLiteral,
    /// Embedded-SQL hole placeholder — stands in for a host expression.
    HolePlaceholder,
    /// `SQLite` defers view-body name resolution until the view is first
    /// used; a CREATE VIEW referencing unknown qualifiers still prepares.
    ViewBodyDeferredResolution,
    /// `SQLite` doesn't check a foreign key's parent table or columns until
    /// the key is enforced, so they may be missing or defined later.
    ForeignKeyParentDeferred,
    /// `SQLite` never checks a trigger's `UPDATE OF` column names.
    TriggerUpdateOfUnchecked,
    /// `DROP ... IF EXISTS` targets may legitimately be missing.
    DropIfExistsMissing,
}

impl From<UncheckedRef> for AcceptedRef {
    fn from(r: UncheckedRef) -> Self {
        match r {
            UncheckedRef::ForeignKeyParent => Self::ForeignKeyParentDeferred,
            UncheckedRef::TriggerUpdateOf => Self::TriggerUpdateOfUnchecked,
            UncheckedRef::DropIfExists => Self::DropIfExistsMissing,
        }
    }
}

/// Decide whether an unresolved source ref is intentionally accepted.
/// `None` means the failure deserves a diagnostic.
fn accepted_source_ref(ev: &SourceRefEvent<'_>) -> Option<AcceptedRef> {
    if let Some(r) = ev.unchecked {
        Some(r.into())
    } else if is_hole_placeholder(ev.name) {
        Some(AcceptedRef::HolePlaceholder)
    } else {
        None
    }
}

/// Decide whether an unresolved column ref is intentionally accepted.
/// `None` means the failure deserves a diagnostic.
fn accepted_ref(ev: &ColumnRefEvent<'_>) -> Option<AcceptedRef> {
    if let Some(r) = ev.unchecked
        && !matches!(ev.resolution, ColumnResolution::Found { .. })
    {
        return Some(r.into());
    }
    match ev.resolution {
        ColumnResolution::Found { .. } => None,
        ColumnResolution::TableNotFound => {
            // Only the qualifier's own text matters here — the column being
            // a hole doesn't make an unknown qualifier valid.
            if ev.table.is_some_and(is_hole_placeholder) {
                Some(AcceptedRef::HolePlaceholder)
            } else if ev.in_view_body {
                Some(AcceptedRef::ViewBodyDeferredResolution)
            } else {
                None
            }
        }
        ColumnResolution::TableFoundColumnMissing => {
            if ev.dqs_candidate {
                Some(AcceptedRef::DqsStringLiteral)
            } else if is_hole_placeholder(ev.column) {
                Some(AcceptedRef::HolePlaceholder)
            } else {
                None
            }
        }
        ColumnResolution::NotFound => {
            if ev.column.eq_ignore_ascii_case("true") || ev.column.eq_ignore_ascii_case("false") {
                Some(AcceptedRef::BooleanLiteral)
            } else if ev.dqs_candidate {
                Some(AcceptedRef::DqsStringLiteral)
            } else if is_hole_placeholder(ev.column) {
                Some(AcceptedRef::HolePlaceholder)
            } else {
                None
            }
        }
    }
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
            DiagnosticMessage::QualifiedRef { .. } => self.qualified_ref,
            DiagnosticMessage::IndexOnView { .. } => self.index_on_view,
            DiagnosticMessage::DropWrongKind { .. } => self.drop_wrong_kind,
            DiagnosticMessage::BuiltinModify { .. } => self.builtin_modify,
            DiagnosticMessage::CompoundColumns { .. } => self.compound_columns,
            DiagnosticMessage::UnknownWindow { .. } => self.unknown_window,
            DiagnosticMessage::OnClauseOrder { .. } => self.on_clause_order,
            DiagnosticMessage::ValuesTerms { .. } => self.values_terms,
            DiagnosticMessage::InsertValues { .. } => self.insert_values,
            DiagnosticMessage::SubqueryColumns { .. } => self.subquery_columns,
            DiagnosticMessage::RowValue { .. } => self.row_value,
            DiagnosticMessage::ParseError(_) => self.parse_errors,
        }
    }
}

// ── StatementVisitor ───────────────────────────────────────────────────────────

/// The analyzer's per-statement visitor. Emits diagnostics into
/// `diagnostics`, feeds CTE / Query / scoped-source events into an
/// internal [`LineageCapture`], and forwards every event to `extra`.
pub(super) struct StatementVisitor<'a, V: SemanticVisitor> {
    config: &'a AnalysisConfig,
    diagnostics: &'a mut Vec<Diagnostic>,
    lineage: LineageCapture<'a>,
    extra: &'a mut V,
}

impl<'a, V: SemanticVisitor> StatementVisitor<'a, V> {
    pub(super) fn new(
        config: &'a AnalysisConfig,
        diagnostics: &'a mut Vec<Diagnostic>,
        roles: &'a [SemanticRole],
        extra: &'a mut V,
    ) -> Self {
        Self {
            config,
            diagnostics,
            lineage: LineageCapture::new(roles),
            extra,
        }
    }

    pub(super) fn into_lineage(self) -> LineageCapture<'a> {
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

    /// Diagnose a column ref whose resolution failed and was not accepted
    /// by [`accepted_ref`].
    fn emit_unresolved_column_ref(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: &ColumnRefEvent<'_>,
    ) {
        match ev.resolution {
            // Gated out by the caller / accepted_ref.
            // Gated out by the caller / accepted_ref.
            ColumnResolution::Found { .. } => {}
            ColumnResolution::TableNotFound => {
                let tbl = ev.table.expect("qualifier present when TableNotFound");
                let candidates = cx.scope.all_source_names();
                let suggestion =
                    best_suggestion(tbl, &candidates, self.config.suggestion_threshold());
                self.emit(
                    stmt,
                    ev.node_id,
                    ev.table_idx,
                    ev.table_range.unwrap_or(ev.range),
                    DiagnosticMessage::UnknownTable {
                        name: tbl.to_string(),
                    },
                    suggestion.map(Help::Suggestion),
                );
            }
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
}

impl<V: SemanticVisitor> SemanticVisitor for StatementVisitor<'_, V> {
    const WANTS_SOURCE_REF: bool = true;
    const WANTS_COLUMN_REF: bool = true;
    const WANTS_CALL: bool = true;
    const WANTS_RELATION_DEFINITION: bool = V::WANTS_RELATION_DEFINITION;
    const WANTS_COLUMN_DEFINITION: bool = V::WANTS_COLUMN_DEFINITION;
    const WANTS_CTE_COLUMN_COUNT: bool = true;
    const WANTS_INDEX_ON_VIEW: bool = true;
    const WANTS_DROP_WRONG_KIND: bool = true;
    const WANTS_BUILTIN_MODIFY: bool = true;
    const WANTS_COMPOUND_COLUMNS: bool = true;
    const WANTS_UNKNOWN_WINDOW: bool = true;
    const WANTS_ON_CLAUSE_ORDER: bool = true;
    const WANTS_VALUES_TERMS: bool = true;
    const WANTS_INSERT_VALUES: bool = true;
    const WANTS_SUBQUERY_COLUMNS: bool = true;
    const WANTS_ROW_VALUE: bool = true;
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
        if !ev.resolved && accepted_source_ref(&ev).is_none() {
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
        let found = matches!(ev.resolution, ColumnResolution::Found { .. });
        if let (true, Some(context), Some(table)) = (found, ev.unqualified_only, ev.table) {
            self.emit(
                stmt,
                ev.node_id,
                ev.table_idx,
                ev.table_range.unwrap_or(ev.range),
                DiagnosticMessage::QualifiedRef {
                    table: table.to_string(),
                    column: ev.column.to_string(),
                    context,
                },
                None,
            );
        } else if !found && accepted_ref(&ev).is_none() {
            self.emit_unresolved_column_ref(stmt, cx, &ev);
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
            FunctionCheckResult::Unknown if !is_hole_placeholder(ev.name) => {
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
            // `Ok`, and `Unknown` for an underscore interpolation placeholder.
            FunctionCheckResult::Ok | FunctionCheckResult::Unknown => {}
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

    fn on_index_on_view(&mut self, stmt: &mut AnyParsedStatement<'_>, ev: IndexOnViewEvent<'_>) {
        self.emit_at(
            ev.range,
            DiagnosticMessage::IndexOnView {
                name: ev.name.to_string(),
            },
            None,
        );
        self.extra.on_index_on_view(stmt, ev);
    }

    fn on_drop_wrong_kind(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        ev: DropWrongKindEvent<'_>,
    ) {
        self.emit_at(
            ev.range,
            DiagnosticMessage::DropWrongKind {
                name: ev.name.to_string(),
                is_view: ev.is_view,
            },
            None,
        );
        self.extra.on_drop_wrong_kind(stmt, ev);
    }

    fn on_builtin_modify(&mut self, stmt: &mut AnyParsedStatement<'_>, ev: BuiltinModifyEvent<'_>) {
        self.emit_at(
            ev.range,
            DiagnosticMessage::BuiltinModify {
                name: ev.name.to_string(),
                change: ev.change,
            },
            None,
        );
        self.extra.on_builtin_modify(stmt, ev);
    }

    fn on_compound_columns(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        ev: CompoundColumnsEvent<'_>,
    ) {
        self.emit_at(
            ev.range,
            DiagnosticMessage::CompoundColumns {
                operator: ev.operator.to_string(),
                left: ev.left,
                right: ev.right,
            },
            None,
        );
        self.extra.on_compound_columns(stmt, ev);
    }

    fn on_unknown_window(&mut self, stmt: &mut AnyParsedStatement<'_>, ev: UnknownWindowEvent<'_>) {
        self.emit_at(
            ev.range,
            DiagnosticMessage::UnknownWindow {
                name: ev.name.to_string(),
            },
            None,
        );
        self.extra.on_unknown_window(stmt, ev);
    }

    fn on_on_clause_order(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        ev: OnClauseOrderEvent<'_>,
    ) {
        self.emit_at(
            ev.range,
            DiagnosticMessage::OnClauseOrder {
                name: ev.name.to_string(),
            },
            None,
        );
        self.extra.on_on_clause_order(stmt, ev);
    }

    fn on_values_terms(&mut self, stmt: &mut AnyParsedStatement<'_>, ev: ValuesTermsEvent) {
        self.emit_at(
            ev.range,
            DiagnosticMessage::ValuesTerms {
                first: ev.first,
                row: ev.row,
            },
            None,
        );
        self.extra.on_values_terms(stmt, ev);
    }

    fn on_insert_values(&mut self, stmt: &mut AnyParsedStatement<'_>, ev: InsertValuesEvent<'_>) {
        self.emit_at(
            ev.range,
            DiagnosticMessage::InsertValues {
                table: ev.table.to_string(),
                column_list: ev.column_list,
                columns: ev.columns,
                values: ev.values,
            },
            None,
        );
        self.extra.on_insert_values(stmt, ev);
    }

    fn on_subquery_columns(&mut self, stmt: &mut AnyParsedStatement<'_>, ev: SubqueryColumnsEvent) {
        self.emit_at(
            ev.range,
            DiagnosticMessage::SubqueryColumns {
                got: ev.got,
                expected: ev.expected,
            },
            None,
        );
        self.extra.on_subquery_columns(stmt, ev);
    }

    fn on_row_value(&mut self, stmt: &mut AnyParsedStatement<'_>, ev: RowValueEvent) {
        self.emit_at(
            ev.range,
            DiagnosticMessage::RowValue {
                width: ev.width,
                compared_with: ev.compared_with,
            },
            None,
        );
        self.extra.on_row_value(stmt, ev);
    }

    fn on_cte_binding(&mut self, stmt: &mut AnyParsedStatement<'_>, ev: CteBindingEvent<'_>) {
        self.lineage.on_cte_binding(stmt, ev);
        self.extra.on_cte_binding(stmt, ev);
    }

    fn on_scoped_source(&mut self, stmt: &mut AnyParsedStatement<'_>, ev: ScopedSourceEvent<'_>) {
        self.lineage.on_scoped_source(stmt, ev);
        self.extra.on_scoped_source(stmt, ev);
    }

    fn enter_query(&mut self, stmt: &mut AnyParsedStatement<'_>, node_id: AnyNodeId) {
        self.lineage.enter_query(stmt, node_id);
        self.extra.enter_query(stmt, node_id);
    }

    fn exit_query(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        catalog: &Catalog,
        node_id: AnyNodeId,
    ) {
        self.lineage.exit_query(stmt, catalog, node_id);
        self.extra.exit_query(stmt, catalog, node_id);
    }

    fn on_parsed_statement(&mut self, stmt: &AnyParsedStatement<'_>) {
        self.extra.on_parsed_statement(stmt);
    }

    fn on_parse_error(&mut self, err: &AnyParseError<'_>) {
        self.extra.on_parse_error(err);
    }
}
