// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! The events the walker reports and the visitor that receives them.

use syntaqlite_syntax::any::{AnyNodeId, AnyParseError, AnyParsedStatement};
use syntaqlite_syntax::source::DocRange;

use crate::analysis::catalog::{Catalog, ColumnResolution, FunctionCheckResult};
use crate::analysis::engine::query_scope::QueryScope;
use crate::analysis::{BuiltinChange, QualifiedRefContext};

// ── WalkCtx ───────────────────────────────────────────────────────────────────

/// Visitor-facing view into the walker's mutable state. Exposes the
/// catalog and the scope stack; the walker is responsible for push/pop.
pub(crate) struct WalkCtx<'a> {
    pub(crate) catalog: &'a mut Catalog,
    pub(crate) scope: &'a mut QueryScope,
}

// ── Events ────────────────────────────────────────────────────────────────────
//
// Events hold borrowed data so they're `Copy` and can be forwarded to multiple
// visitors in a hand-written composition without cloning. The owning storage
// lives on the walker's stack for the duration of each hook call.

/// A table/view/table-function reference in a FROM / JOIN / DML target.
///
/// Visitors that need the relation's columns look them up via
/// `cx.catalog.table_source_info(&NameKey::new(ev.name))`; the walker does not
/// pre-compute that (it would be wasted work when no visitor wants it).
#[derive(Copy, Clone)]
pub(crate) struct SourceRefEvent<'a> {
    pub(crate) node_id: AnyNodeId,
    pub(crate) name_idx: u8,
    pub(crate) range: DocRange,
    pub(crate) name: &'a str,
    /// The display alias for this source (`None` when no `AS` clause
    /// was written — callers use `name` as the scope key in that case).
    pub(crate) alias: Option<&'a str>,
    pub(crate) resolved: bool,
    /// Set when `SQLite` doesn't check this name, so failures are expected.
    pub(crate) unchecked: Option<UncheckedRef>,
}

/// A column reference (qualified or bare) inside an expression.
#[derive(Copy, Clone)]
pub(crate) struct ColumnRefEvent<'a> {
    pub(crate) node_id: AnyNodeId,
    pub(crate) column_idx: u8,
    pub(crate) range: DocRange,
    pub(crate) column: &'a str,
    pub(crate) table: Option<&'a str>,
    pub(crate) table_idx: u8,
    /// Document-absolute range of the qualifier, when present.
    pub(crate) table_range: Option<DocRange>,
    /// The ref is inside a CREATE VIEW body, where `SQLite` defers name
    /// resolution until the view is first used.
    pub(crate) in_view_body: bool,
    /// Set when `SQLite` doesn't check this name, so failures are expected.
    pub(crate) unchecked: Option<UncheckedRef>,
    /// The ref is where `SQLite` forbids qualified names.
    pub(crate) unqualified_only: Option<QualifiedRefContext>,
    pub(crate) resolution: &'a ColumnResolution,
    /// `SQLite`'s double-quoted-string bug-compat: a `"foo"` identifier in
    /// expression position that doesn't resolve to a column is re-interpreted
    /// as a string literal.  True iff this column ref was written with
    /// surrounding `"..."` (not `` `...` `` or `[...]`, which stay
    /// identifier-only).
    pub(crate) dqs_candidate: bool,
}

/// A function / table-function / aggregate call.
///
/// Visitors that need the resolved signature look it up via
/// `cx.catalog.function_signature(&NameKey::new(ev.name))` themselves.
#[derive(Copy, Clone)]
pub(crate) struct CallEvent<'a> {
    pub(crate) node_id: AnyNodeId,
    pub(crate) name_idx: u8,
    pub(crate) range: DocRange,
    pub(crate) name: &'a str,
    pub(crate) arg_count: usize,
    pub(crate) result: &'a FunctionCheckResult,
}

/// A CTE body's actual result-column count differs from its declared count.
#[derive(Copy, Clone)]
pub(crate) struct CteColumnCountMismatchEvent<'a> {
    pub(crate) name: &'a str,
    pub(crate) name_range: DocRange,
    pub(crate) declared: usize,
    pub(crate) actual: usize,
}

/// Why a name is one `SQLite` doesn't check.
#[derive(Copy, Clone)]
pub(crate) enum UncheckedRef {
    /// A foreign key's parent table or column, checked only when enforced.
    ForeignKeyParent,
    /// A trigger's `UPDATE OF` column.
    TriggerUpdateOf,
    /// The target of a `DROP ... IF EXISTS`.
    DropIfExists,
}

/// An ON clause referencing a table joined to its right.
#[derive(Copy, Clone)]
pub(crate) struct OnClauseOrderEvent<'a> {
    pub(crate) name: &'a str,
    pub(crate) range: DocRange,
}

/// A window specification naming a window its query doesn't define.
#[derive(Copy, Clone)]
pub(crate) struct UnknownWindowEvent<'a> {
    pub(crate) name: &'a str,
    pub(crate) range: DocRange,
}

/// A VALUES row with a different number of terms than the first row.
#[derive(Copy, Clone)]
pub(crate) struct ValuesTermsEvent {
    pub(crate) range: DocRange,
    pub(crate) first: usize,
    pub(crate) row: usize,
}

/// An INSERT supplying a different number of values than it has columns.
#[derive(Copy, Clone)]
pub(crate) struct InsertValuesEvent<'a> {
    pub(crate) table: &'a str,
    pub(crate) range: DocRange,
    pub(crate) column_list: bool,
    pub(crate) columns: usize,
    pub(crate) values: usize,
}

/// A subquery returning a different number of columns than its position
/// takes.
#[derive(Copy, Clone)]
pub(crate) struct SubqueryColumnsEvent {
    pub(crate) range: DocRange,
    pub(crate) got: usize,
    pub(crate) expected: usize,
}

/// A row value used as a single value, or compared with a row value of a
/// different width.
#[derive(Copy, Clone)]
pub(crate) struct RowValueEvent {
    pub(crate) range: DocRange,
    pub(crate) width: usize,
    pub(crate) compared_with: Option<usize>,
}

/// A compound SELECT whose arms have different numbers of columns.
#[derive(Copy, Clone)]
pub(crate) struct CompoundColumnsEvent<'a> {
    pub(crate) operator: &'a str,
    pub(crate) range: DocRange,
    pub(crate) left: usize,
    pub(crate) right: usize,
}

/// A DDL statement that changes one of the dialect's built-in tables.
#[derive(Copy, Clone)]
pub(crate) struct BuiltinModifyEvent<'a> {
    pub(crate) name: &'a str,
    pub(crate) range: DocRange,
    pub(crate) change: BuiltinChange,
}

/// A `DROP TABLE` naming a view, or a `DROP VIEW` naming a table.
#[derive(Copy, Clone)]
pub(crate) struct DropWrongKindEvent<'a> {
    pub(crate) name: &'a str,
    pub(crate) range: DocRange,
    pub(crate) is_view: bool,
}

/// A `CREATE INDEX` whose target is a view.
#[derive(Copy, Clone)]
pub(crate) struct IndexOnViewEvent<'a> {
    pub(crate) name: &'a str,
    pub(crate) range: DocRange,
}

/// A single CTE binding, fired BEFORE its body is walked so visitors
/// can register the `name -> body` mapping in time for body
/// source-refs (including recursive self-references) to look it up.
#[derive(Copy, Clone)]
pub(crate) struct CteBindingEvent<'a> {
    pub(crate) name: &'a str,
    pub(crate) body_id: Option<AnyNodeId>,
}

/// A subquery-in-FROM (`FROM (SELECT …) AS x`), fired after the body
/// has been walked. Carries the alias and the body's Query node id so
/// lineage can tie it into the enclosing Query's source map.
#[derive(Copy, Clone)]
pub(crate) struct ScopedSourceEvent<'a> {
    pub(crate) alias: Option<&'a str>,
    pub(crate) body_id: Option<AnyNodeId>,
}

// ── SemanticVisitor trait ────────────────────────────────────────────────────

/// A walk-time visitor: receives pre-resolved events from the walker and
/// decides what to do with them. Every hook has an empty default, so
/// implementors override only the events they care about.
///
/// The `WANTS_*` constants let the walker skip event construction when no
/// visitor in the composition wants it. For a multi-visitor composition,
/// the composition struct should OR the flags of its constituent
/// visitors.
pub(crate) trait SemanticVisitor {
    const WANTS_SOURCE_REF: bool = false;
    const WANTS_COLUMN_REF: bool = false;
    const WANTS_CALL: bool = false;
    const WANTS_RELATION_DEFINITION: bool = false;
    const WANTS_COLUMN_DEFINITION: bool = false;
    const WANTS_CTE_COLUMN_COUNT: bool = false;
    const WANTS_INDEX_ON_VIEW: bool = false;
    const WANTS_DROP_WRONG_KIND: bool = false;
    const WANTS_BUILTIN_MODIFY: bool = false;
    const WANTS_COMPOUND_COLUMNS: bool = false;
    const WANTS_UNKNOWN_WINDOW: bool = false;
    const WANTS_ON_CLAUSE_ORDER: bool = false;
    const WANTS_VALUES_TERMS: bool = false;
    const WANTS_INSERT_VALUES: bool = false;
    const WANTS_SUBQUERY_COLUMNS: bool = false;
    const WANTS_ROW_VALUE: bool = false;
    const WANTS_CTE_BINDING: bool = false;
    const WANTS_SCOPED_SOURCE: bool = false;
    /// Return true to receive [`enter_query`](Self::enter_query) /
    /// [`exit_query`](Self::exit_query) hooks around each `Query` node.
    const WANTS_QUERY: bool = false;
    /// Return true to receive [`on_parsed_statement`](Self::on_parsed_statement)
    /// and [`on_parse_error`](Self::on_parse_error) hooks. Visitors that
    /// want tokens or comments iterate them from the statement/error
    /// themselves — the walker does not pre-materialize per-token data.
    const WANTS_STATEMENT_CONTEXT: bool = false;

    fn on_source_ref(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _cx: &mut WalkCtx<'_>,
        _ev: SourceRefEvent<'_>,
    ) {
    }

    fn on_column_ref(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _cx: &mut WalkCtx<'_>,
        _ev: ColumnRefEvent<'_>,
    ) {
    }

    fn on_call(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _cx: &mut WalkCtx<'_>,
        _ev: CallEvent<'_>,
    ) {
    }

    fn on_relation_definition(&mut self, _name: &str, _range: DocRange) {}

    fn on_column_definition(&mut self, _table: &str, _column: &str, _range: DocRange) {}

    fn on_cte_column_count_mismatch(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _ev: CteColumnCountMismatchEvent<'_>,
    ) {
    }

    fn on_index_on_view(&mut self, _stmt: &mut AnyParsedStatement<'_>, _ev: IndexOnViewEvent<'_>) {}

    fn on_drop_wrong_kind(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _ev: DropWrongKindEvent<'_>,
    ) {
    }

    fn on_builtin_modify(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _ev: BuiltinModifyEvent<'_>,
    ) {
    }

    fn on_compound_columns(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _ev: CompoundColumnsEvent<'_>,
    ) {
    }

    fn on_unknown_window(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _ev: UnknownWindowEvent<'_>,
    ) {
    }

    fn on_on_clause_order(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _ev: OnClauseOrderEvent<'_>,
    ) {
    }

    fn on_values_terms(&mut self, _stmt: &mut AnyParsedStatement<'_>, _ev: ValuesTermsEvent) {}

    fn on_insert_values(&mut self, _stmt: &mut AnyParsedStatement<'_>, _ev: InsertValuesEvent<'_>) {
    }

    fn on_subquery_columns(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _ev: SubqueryColumnsEvent,
    ) {
    }

    fn on_row_value(&mut self, _stmt: &mut AnyParsedStatement<'_>, _ev: RowValueEvent) {}

    /// Called once per CTE binding, BEFORE its body is walked. The body
    /// will observe the binding's own name as a resolved catalog source
    /// (`SQLite` allows any CTE to self-reference, with or without the
    /// RECURSIVE keyword); listeners that build a `name -> body` map
    /// should populate it here so body-side references resolve correctly.
    fn on_cte_binding(&mut self, _stmt: &mut AnyParsedStatement<'_>, _ev: CteBindingEvent<'_>) {}

    /// Called for a subquery-in-FROM after its body has been walked.
    fn on_scoped_source(&mut self, _stmt: &mut AnyParsedStatement<'_>, _ev: ScopedSourceEvent<'_>) {
    }

    /// Called before a `Query` node's fields are walked. Paired with
    /// [`exit_query`](Self::exit_query). The first `enter_query` call
    /// for a statement corresponds to the outermost query.
    fn enter_query(&mut self, _stmt: &mut AnyParsedStatement<'_>, _node_id: AnyNodeId) {}

    /// Called after a `Query` node's fields have been walked. `catalog`
    /// is as it stood while the query was walked.
    fn exit_query(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _catalog: &Catalog,
        _node_id: AnyNodeId,
    ) {
    }

    /// Called once before the AST walk for each successfully parsed
    /// statement. Visitors that want tokens or comments iterate
    /// `stmt.tokens()` / `stmt.comments()` directly inside this hook — no
    /// per-element dispatch.
    fn on_parsed_statement(&mut self, _stmt: &AnyParsedStatement<'_>) {}

    /// Called once per failed parse. Visitors can iterate `err.tokens()` /
    /// `err.comments()` to capture partial token/comment data.
    fn on_parse_error(&mut self, _err: &AnyParseError<'_>) {}
}

/// A [`SemanticVisitor`] that ignores every event. Used as the default
/// "extras" visitor when a caller only cares about diagnostics.
pub(crate) struct NoopVisitor;

impl SemanticVisitor for NoopVisitor {}
