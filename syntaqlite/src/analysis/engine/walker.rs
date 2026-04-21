// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Semantic-role-driven AST walker.
//!
//! [`SemanticWalker`] owns the statement, catalog, role table, and scope
//! stack for the duration of one walk, and drives role-dispatched
//! recursion. It hands each event to a [`SemanticVisitor`] implementor via
//! hook methods.
//!
//! ## Composition
//!
//! There is no pipeline builder. When multiple visitors need to run
//! together, write a concrete struct that holds them and implements
//! [`SemanticVisitor`] by delegating to each at every hook — the
//! composition is explicit and grep-able.

use syntaqlite_syntax::any::{
    AnyNodeId, AnyParseError, AnyParsedStatement, FieldValue, NodeFields,
};
use syntaqlite_syntax::source::DocRange;

use crate::analysis::catalog::{Catalog, ColumnResolution, FunctionCheckResult};
use crate::analysis::stmt_reader::StmtReader;
use crate::dialect::{FIELD_ABSENT, SemanticRole};

use super::query_scope::{QueryScope, RowIdPolicy};

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
/// `cx.catalog.table_source_info(ev.name)`; the walker does not
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
}

/// A column reference (qualified or bare) inside an expression.
#[derive(Copy, Clone)]
pub(crate) struct ColumnRefEvent<'a> {
    pub(crate) node_id: AnyNodeId,
    pub(crate) column_idx: u8,
    pub(crate) range: DocRange,
    pub(crate) column: &'a str,
    pub(crate) table: Option<&'a str>,
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
/// `cx.catalog.function_signature(ev.name)` themselves.
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

    /// Called once per CTE binding, BEFORE its body is walked. For
    /// recursive CTEs the body will observe the binding's own name as a
    /// resolved catalog source; listeners that build a `name -> body`
    /// map should populate it here so body-side references resolve
    /// correctly.
    fn on_cte_binding(&mut self, _stmt: &mut AnyParsedStatement<'_>, _ev: CteBindingEvent<'_>) {}

    /// Called for a subquery-in-FROM after its body has been walked.
    fn on_scoped_source(&mut self, _stmt: &mut AnyParsedStatement<'_>, _ev: ScopedSourceEvent<'_>) {
    }

    /// Called before a `Query` node's fields are walked. Paired with
    /// [`exit_query`](Self::exit_query). The first `enter_query` call
    /// for a statement corresponds to the outermost query.
    fn enter_query(&mut self, _stmt: &mut AnyParsedStatement<'_>, _node_id: AnyNodeId) {}

    /// Called after a `Query` node's fields have been walked.
    fn exit_query(&mut self, _stmt: &mut AnyParsedStatement<'_>, _node_id: AnyNodeId) {}

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

// ── SemanticWalker ────────────────────────────────────────────────────────────

/// Owns the state threaded through one walk. Constructed fresh per
/// statement; `run` drives role-dispatched recursion from `root`.
pub(crate) struct SemanticWalker<'a, 'b> {
    stmt: &'a mut AnyParsedStatement<'b>,
    catalog: &'a mut Catalog,
    scope: QueryScope,
    roles: &'static [SemanticRole],
}

impl<'a, 'b> SemanticWalker<'a, 'b> {
    pub(crate) fn new(
        stmt: &'a mut AnyParsedStatement<'b>,
        catalog: &'a mut Catalog,
        roles: &'static [SemanticRole],
    ) -> Self {
        Self {
            stmt,
            catalog,
            scope: QueryScope::default(),
            roles,
        }
    }

    /// Walk `root`, firing events to `visitor` via the
    /// [`SemanticVisitor`] hooks as each role is encountered.
    pub(crate) fn run<V: SemanticVisitor>(&mut self, visitor: &mut V, root: AnyNodeId) {
        self.walk_node(visitor, root);
    }

    // ── Node dispatch ─────────────────────────────────────────────────────────

    fn walk_node<V: SemanticVisitor>(&mut self, visitor: &mut V, node_id: AnyNodeId) {
        if node_id.is_null() {
            return;
        }
        if let Some(children) = self.stmt.list_children(node_id) {
            // `children` has lifetime 'b (tied to the statement buffer, not
            // the &self borrow), so we can iterate it while re-borrowing
            // self for the recursive walk.
            for &child in children {
                if !child.is_null() {
                    self.walk_node(visitor, child);
                }
            }
            return;
        }
        let Some((tag, fields)) = self.stmt.extract_fields(node_id) else {
            return;
        };
        let role = StmtReader::new(self.stmt, self.roles).role_for_tag(tag);

        match role {
            SemanticRole::DefineTable { select, .. } | SemanticRole::DefineView { select, .. } => {
                if V::WANTS_RELATION_DEFINITION || V::WANTS_COLUMN_DEFINITION {
                    self.emit_ddl_definitions(visitor, node_id);
                }
                self.walk_opt(visitor, fields.node_id_at(select));
            }
            SemanticRole::DefineFunction { select, .. } => {
                self.walk_opt(visitor, fields.node_id_at(select));
            }
            SemanticRole::ReturnSpec { .. } | SemanticRole::Import { .. } => {}

            SemanticRole::Transparent
            | SemanticRole::ColumnDef { .. }
            | SemanticRole::ResultColumn { .. }
            | SemanticRole::CteBinding { .. } => self.walk_children(visitor, node_id),

            SemanticRole::Call { name, args } => {
                self.walk_call(visitor, node_id, &fields, name, args);
            }
            SemanticRole::ColumnRef { column, table } => {
                self.walk_column_ref(visitor, node_id, &fields, column, table);
            }
            SemanticRole::SourceRef { name, alias, .. } => {
                self.walk_source_ref(visitor, node_id, &fields, name, alias);
            }
            SemanticRole::ScopedSource { body, alias } => {
                self.walk_scoped_source(visitor, &fields, body, alias);
            }
            SemanticRole::Query {
                from,
                columns,
                where_clause,
                groupby,
                having,
                orderby,
                limit_clause,
            } => self.walk_query(
                visitor,
                node_id,
                &fields,
                from,
                columns,
                where_clause,
                groupby,
                having,
                orderby,
                limit_clause,
            ),
            SemanticRole::CteScope {
                recursive,
                bindings,
                body,
            } => self.walk_cte_scope(visitor, &fields, recursive, bindings, body),
            SemanticRole::TriggerScope {
                target: _,
                when,
                body,
            } => self.walk_trigger_scope(visitor, &fields, when, body),
            SemanticRole::DmlScope {
                with_recursive,
                with_ctes,
            } => self.walk_dml_scope(visitor, &fields, with_recursive, with_ctes),
        }
    }

    fn walk_children<V: SemanticVisitor>(&mut self, visitor: &mut V, node_id: AnyNodeId) {
        for child in self.stmt.child_node_ids(node_id) {
            if !child.is_null() {
                self.walk_node(visitor, child);
            }
        }
    }

    fn walk_opt<V: SemanticVisitor>(&mut self, visitor: &mut V, id: Option<AnyNodeId>) {
        if let Some(id) = id {
            self.walk_node(visitor, id);
        }
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    fn emit_ddl_definitions<V: SemanticVisitor>(&mut self, visitor: &mut V, node_id: AnyNodeId) {
        let reader = StmtReader::new(self.stmt, self.roles);
        let Some((table_name, table_range)) = reader.name_span(node_id) else {
            return;
        };
        if V::WANTS_RELATION_DEFINITION {
            visitor.on_relation_definition(&table_name, table_range);
        }
        if V::WANTS_COLUMN_DEFINITION {
            for (col_name, col_range) in reader.column_spans(node_id) {
                visitor.on_column_definition(&table_name, &col_name, col_range);
            }
        }
    }

    // ── Role walkers ──────────────────────────────────────────────────────────

    fn walk_source_ref<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        name_idx: u8,
        alias_idx: u8,
    ) {
        let Some((name, range)) = self.stmt.span_field_range(fields, name_idx) else {
            return;
        };

        let is_known =
            self.catalog.resolve_relation(name) || self.catalog.resolve_table_function(name);
        let (columns, without_rowid) = self.catalog.table_source_info(name);
        let alias_text = self.stmt.name_text(fields.node_id_at(alias_idx)).0;
        let alias = if alias_text.is_empty() {
            None
        } else {
            Some(alias_text)
        };

        if V::WANTS_SOURCE_REF {
            let ev = SourceRefEvent {
                node_id,
                name_idx,
                range,
                name,
                alias,
                resolved: is_known,
            };
            let mut cx = WalkCtx {
                catalog: self.catalog,
                scope: &mut self.scope,
            };
            visitor.on_source_ref(self.stmt, &mut cx, ev);
        }

        let scope_name = alias.unwrap_or(name);
        self.scope
            .add_table(scope_name, columns, without_rowid.into());
    }

    fn walk_call<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        name_idx: u8,
        args_idx: u8,
    ) {
        if let Some((name, range)) = self.stmt.span_field_range(fields, name_idx) {
            let arg_count = fields
                .node_id_at(args_idx)
                .and_then(|id| self.stmt.list_children(id))
                .map_or(0, <[_]>::len);
            let result = self.catalog.check_function(name, arg_count);

            if V::WANTS_CALL {
                let ev = CallEvent {
                    node_id,
                    name_idx,
                    range,
                    name,
                    arg_count,
                    result: &result,
                };
                let mut cx = WalkCtx {
                    catalog: self.catalog,
                    scope: &mut self.scope,
                };
                visitor.on_call(self.stmt, &mut cx, ev);
            }
        }
        self.walk_children(visitor, node_id);
    }

    fn walk_column_ref<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        column_idx: u8,
        table_idx: u8,
    ) {
        // ColumnRef outside any query scope (e.g. ATTACH ... AS scratch) is
        // just a bare identifier — skip resolution.
        if !self.scope.has_frames() {
            return;
        }
        let FieldValue::Span(col_sp) = fields[column_idx as usize] else {
            return;
        };
        if col_sp.is_empty() {
            return;
        }
        let column = self.stmt.span_expanded_text(col_sp);
        let table = match fields[table_idx as usize] {
            FieldValue::Span(sp) if !sp.is_empty() => Some(self.stmt.span_expanded_text(sp)),
            _ => None,
        };
        let (_, range) = self.stmt.span_text_abs(col_sp);
        // SQLite's DQS bug-compat applies only to identifiers that were
        // written with `"..."` quotes (not backticks or brackets) and that
        // came from the original source (not a macro expansion).
        let dqs_candidate = col_sp.is_macro_free() && col_sp.quote_char() == Some('"');

        let resolution = self.scope.resolve_column(table, column);

        if V::WANTS_COLUMN_REF {
            let ev = ColumnRefEvent {
                node_id,
                column_idx,
                range,
                column,
                table,
                resolution: &resolution,
                dqs_candidate,
            };
            let mut cx = WalkCtx {
                catalog: self.catalog,
                scope: &mut self.scope,
            };
            visitor.on_column_ref(self.stmt, &mut cx, ev);
        }
    }

    fn walk_scoped_source<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        fields: &NodeFields,
        body_idx: u8,
        alias_idx: u8,
    ) {
        let body_id = fields.node_id_at(body_idx);
        self.scope.push();
        self.walk_opt(visitor, body_id);
        self.scope.pop();

        let alias_text = self.stmt.name_text(fields.node_id_at(alias_idx)).0;
        let alias = if alias_text.is_empty() {
            None
        } else {
            Some(alias_text)
        };

        if V::WANTS_SCOPED_SOURCE {
            visitor.on_scoped_source(self.stmt, ScopedSourceEvent { alias, body_id });
        }

        let cols =
            body_id.and_then(|id| StmtReader::new(self.stmt, self.roles).columns_from_select(id));
        match alias {
            None => self.scope.add_anonymous(cols),
            Some(name) => self.scope.add_table(name, cols, RowIdPolicy::WithRowId),
        }
    }

    #[expect(clippy::too_many_arguments)]
    fn walk_query<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        from: u8,
        columns: u8,
        where_clause: u8,
        groupby: u8,
        having: u8,
        orderby: u8,
        limit_clause: u8,
    ) {
        if V::WANTS_QUERY {
            visitor.enter_query(self.stmt, node_id);
        }

        // Push a fresh scope so that tables registered by walk_source_ref
        // are visible when we visit SELECT columns, WHERE, ORDER BY, etc.
        self.scope.push();
        self.walk_opt(visitor, fields.node_id_at(from));
        self.walk_opt(visitor, fields.node_id_at(columns));

        // Collect SELECT aliases so they are visible in WHERE, GROUP BY,
        // HAVING, ORDER BY, and LIMIT — matching SQLite's resolution rules.
        let aliases = self.collect_select_aliases(fields, columns);
        if !aliases.is_empty() {
            self.scope
                .add_table("", Some(aliases), RowIdPolicy::WithRowId);
        }

        for idx in [where_clause, groupby, having, orderby, limit_clause] {
            self.walk_opt(visitor, fields.node_id_at(idx));
        }
        self.scope.pop();

        if V::WANTS_QUERY {
            visitor.exit_query(self.stmt, node_id);
        }
    }

    fn collect_select_aliases(&self, fields: &NodeFields, columns_idx: u8) -> Vec<String> {
        let mut aliases = Vec::new();
        let Some(list_id) = fields.node_id_at(columns_idx) else {
            return aliases;
        };
        let reader = StmtReader::new(self.stmt, self.roles);
        reader.for_each_result_column(list_id, |child_fields, _flags, alias_idx, _expr| {
            let alias_node = child_fields.node_id_at(alias_idx);
            let (alias_text, _) = reader.stmt().name_text(alias_node);
            if !alias_text.is_empty() {
                aliases.push(alias_text.to_string());
            }
            true
        });
        aliases
    }

    fn walk_trigger_scope<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        fields: &NodeFields,
        when_idx: u8,
        body_idx: u8,
    ) {
        self.scope.push();
        self.scope.add_table("OLD", None, RowIdPolicy::WithRowId);
        self.scope.add_table("NEW", None, RowIdPolicy::WithRowId);
        self.walk_opt(visitor, fields.node_id_at(when_idx));
        self.walk_opt(visitor, fields.node_id_at(body_idx));
        self.scope.pop();
    }

    // ── CTE / DML handling ────────────────────────────────────────────────────

    fn walk_cte_scope<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        fields: &NodeFields,
        recursive_idx: u8,
        bindings_idx: u8,
        body_idx: u8,
    ) {
        self.catalog.push_query_scope();
        self.register_cte_bindings(visitor, fields, recursive_idx, bindings_idx);
        self.walk_opt(visitor, fields.node_id_at(body_idx));
        self.catalog.pop_query_scope();
    }

    fn walk_dml_scope<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        fields: &NodeFields,
        recursive_idx: u8,
        bindings_idx: u8,
    ) {
        let has_ctes = bindings_idx != FIELD_ABSENT;
        if has_ctes {
            self.catalog.push_query_scope();
            self.register_cte_bindings(visitor, fields, recursive_idx, bindings_idx);
        }
        self.scope.push();
        self.walk_dml_children_except_ctes(visitor, fields, bindings_idx);
        self.scope.pop();
        if has_ctes {
            self.catalog.pop_query_scope();
        }
    }

    fn walk_dml_children_except_ctes<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        fields: &NodeFields,
        bindings_idx: u8,
    ) {
        let skip = bindings_idx as usize;
        for idx in 0..fields.len() {
            if idx == skip {
                continue;
            }
            let FieldValue::NodeId(child_id) = fields[idx] else {
                continue;
            };
            if child_id.is_null() {
                continue;
            }
            if let Some(children) = self.stmt.list_children(child_id) {
                for &grandchild in children {
                    if !grandchild.is_null() {
                        self.walk_node(visitor, grandchild);
                    }
                }
            } else {
                self.walk_node(visitor, child_id);
            }
        }
    }

    fn register_cte_bindings<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        fields: &NodeFields,
        recursive_idx: u8,
        bindings_idx: u8,
    ) {
        if bindings_idx == FIELD_ABSENT {
            return;
        }
        let is_recursive = recursive_idx != FIELD_ABSENT
            && matches!(fields[recursive_idx as usize], FieldValue::Bool(true));
        let cte_ids: &[AnyNodeId] = fields
            .node_id_at(bindings_idx)
            .and_then(|id| self.stmt.list_children(id))
            .unwrap_or(&[]);

        // Clone to a Vec so we don't borrow from self.stmt across the
        // recursive walk.
        let cte_ids: Vec<AnyNodeId> = cte_ids.to_vec();

        for cte_id in cte_ids {
            let Some(binding) = self.extract_cte_binding(cte_id) else {
                continue;
            };

            // Fire on_cte_binding BEFORE the body walk so visitors that
            // build name -> body_id maps (e.g. LineageCapture) have the
            // binding registered when the body walks — covers both the
            // recursive self-reference case and outer-body uses.
            if V::WANTS_CTE_BINDING && !binding.name.is_empty() {
                let ev = CteBindingEvent {
                    name: binding.name,
                    body_id: binding.body_id,
                };
                visitor.on_cte_binding(self.stmt, ev);
            }

            // For recursive CTEs, register the name before visiting the
            // body so catalog source lookups inside the body resolve.
            if is_recursive && !binding.name.is_empty() {
                let cols = binding
                    .declared_cols
                    .as_ref()
                    .map(|v| v.iter().map(|(s, _)| (*s).to_string()).collect());
                self.catalog.add_query_table(binding.name, cols);
            }

            self.scope.push();
            self.walk_opt(visitor, binding.body_id);
            self.scope.pop();

            if binding.name.is_empty() {
                continue;
            }

            if V::WANTS_RELATION_DEFINITION {
                visitor.on_relation_definition(binding.name, binding.name_range);
            }

            let cols = if let Some(declared) = binding.declared_cols.as_ref() {
                if V::WANTS_CTE_COLUMN_COUNT
                    && let Some(actual) = self.count_result_columns(binding.body_id)
                    && actual != declared.len()
                {
                    let ev = CteColumnCountMismatchEvent {
                        name: binding.name,
                        name_range: binding.name_range,
                        declared: declared.len(),
                        actual,
                    };
                    visitor.on_cte_column_count_mismatch(self.stmt, ev);
                }
                if V::WANTS_COLUMN_DEFINITION {
                    for &(col_name, col_range) in declared {
                        visitor.on_column_definition(binding.name, col_name, col_range);
                    }
                }
                Some(declared.iter().map(|(s, _)| (*s).to_string()).collect())
            } else {
                if V::WANTS_COLUMN_DEFINITION {
                    self.emit_select_column_definitions(visitor, binding.body_id, binding.name);
                }
                binding
                    .body_id
                    .and_then(|id| StmtReader::new(self.stmt, self.roles).columns_from_select(id))
            };
            self.catalog.add_query_table(binding.name, cols);
        }
    }

    fn extract_cte_binding(&mut self, cte_id: AnyNodeId) -> Option<CteBindingInfo<'b>> {
        let reader = StmtReader::new(self.stmt, self.roles);
        let (
            SemanticRole::CteBinding {
                name: name_idx,
                body: body_idx,
                ..
            },
            fields,
        ) = reader.role_for_node(cte_id)?
        else {
            return None;
        };

        let (name, name_range) = reader
            .stmt()
            .span_field_range(&fields, name_idx)
            .unwrap_or_default();
        Some(CteBindingInfo {
            name,
            name_range,
            body_id: fields.node_id_at(body_idx),
            declared_cols: reader.cte_declared_cols(cte_id),
        })
    }

    fn count_result_columns(&mut self, body_id: Option<AnyNodeId>) -> Option<usize> {
        let body_id = body_id?;
        let reader = StmtReader::new(self.stmt, self.roles);
        let (
            SemanticRole::Query {
                columns: cols_idx, ..
            },
            body_fields,
        ) = reader.role_for_node(body_id)?
        else {
            return None;
        };
        let list_id = body_fields.node_id_at(cols_idx)?;

        let mut count = 0usize;
        let mut saw_star = false;
        reader.for_each_result_column(list_id, |fields, flags_idx, _alias, _expr| {
            if let FieldValue::Flags(f) = fields[flags_idx as usize]
                && f & 1 != 0
            {
                saw_star = true;
                return false;
            }
            count += 1;
            true
        });
        if saw_star { None } else { Some(count) }
    }

    fn emit_select_column_definitions<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        body_id: Option<AnyNodeId>,
        table_name: &str,
    ) {
        let Some(body_id) = body_id else { return };
        let reader = StmtReader::new(self.stmt, self.roles);
        let Some((
            SemanticRole::Query {
                columns: cols_idx, ..
            },
            fields,
        )) = reader.role_for_node(body_id)
        else {
            return;
        };
        let Some(list_id) = fields.node_id_at(cols_idx) else {
            return;
        };
        reader.for_each_result_column(list_id, |child_fields, _flags, alias_idx, _expr| {
            let alias_node = child_fields.node_id_at(alias_idx);
            let (alias_text, alias_range) = reader.stmt().name_text(alias_node);
            if !alias_text.is_empty() {
                visitor.on_column_definition(table_name, alias_text, alias_range);
            }
            true
        });
    }
}

// ── CteBindingInfo ────────────────────────────────────────────────────────────

struct CteBindingInfo<'b> {
    name: &'b str,
    name_range: DocRange,
    body_id: Option<AnyNodeId>,
    declared_cols: Option<Vec<(&'b str, DocRange)>>,
}
