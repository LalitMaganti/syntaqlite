// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! SELECT queries and what they contain: FROM sources and joins,
//! expressions, compounds and VALUES clauses.

use std::borrow::Cow;
use std::collections::HashSet;

use syntaqlite_syntax::any::{AnyNodeId, FieldValue, NodeFields};

use crate::analysis::engine::query_scope::RowIdPolicy;
use crate::analysis::name_key::NameKey;
use crate::analysis::stmt_reader::StmtReader;
use crate::dialect::{FIELD_ABSENT, SemanticRole};

use super::visitor::{
    CallEvent, ColumnRefEvent, CompoundColumnsEvent, OnClauseOrderEvent, RowValueEvent,
    ScopedSourceEvent, SemanticVisitor, SourceRefEvent, SubqueryColumnsEvent, UnknownWindowEvent,
    ValuesTermsEvent, WalkCtx,
};
use super::{OnClauseLimit, SemanticWalker};

impl<'b> SemanticWalker<'_, 'b> {
    /// Bind every source in `node_id`'s subtree into the current frame,
    /// before any expression in that scope resolves — mirroring `SQLite`,
    /// which builds a SELECT's full source list first and resolves names
    /// against it afterwards. This is what makes within-scope forward
    /// references work: `JOIN b ON a.x = b.y` inside a longer join chain,
    /// or UPDATE ... FROM aliases used in the SET list.
    ///
    /// Fires no visitor events (the main walk does that) and does not
    /// cross into nested scopes — their sources belong to their own
    /// frames. Re-registration by the main walk is idempotent.
    pub(super) fn register_sources(&mut self, node_id: AnyNodeId) {
        if node_id.is_null() {
            return;
        }
        if let Some(children) = self.stmt.list_children(node_id) {
            for &child in children {
                if !child.is_null() {
                    self.register_sources(child);
                }
            }
            return;
        }
        let Some((tag, fields)) = self.stmt.extract_fields(node_id) else {
            return;
        };
        match StmtReader::new(self.stmt, self.roles).role_for_tag(tag) {
            SemanticRole::SourceRef { name, alias, .. } => {
                self.bind_source_ref(&fields, name, alias);
            }
            SemanticRole::ScopedSource { body, alias } => {
                // Bind the alias only; the body is its own scope.
                let (alias_ident, _) = self.stmt.name_ident_text(fields.node_id_at(alias));
                if !alias_ident.is_empty() {
                    let cols = fields
                        .node_id_at(body)
                        .and_then(|id| self.result_columns(id));
                    self.scope
                        .add_table(&alias_ident, cols, RowIdPolicy::WithRowId);
                }
            }
            // Nested scopes own their sources.
            SemanticRole::Query { .. }
            | SemanticRole::CompoundScope { .. }
            | SemanticRole::CteScope { .. }
            | SemanticRole::DmlScope { .. }
            | SemanticRole::InsertScope { .. }
            | SemanticRole::TriggerScope { .. }
            | SemanticRole::UpsertScope { .. }
            | SemanticRole::DefineTable { .. }
            | SemanticRole::DefineView { .. }
            | SemanticRole::DefineFunction { .. } => {}
            _ => {
                for idx in 0..fields.len() {
                    #[expect(
                        clippy::cast_possible_truncation,
                        reason = "NodeFields holds at most 16"
                    )]
                    if let Some(child) = fields.node_id_at(idx as u8) {
                        self.register_sources(child);
                    }
                }
            }
        }
    }

    /// Bind a source ref's addressable name (alias if present, else base
    /// name) into the current frame. Idempotent.
    fn bind_source_ref(&mut self, fields: &NodeFields, name_idx: u8, alias_idx: u8) {
        let Some(name_ident) = self.stmt.span_field_ident_text(fields, name_idx) else {
            return;
        };
        self.key_scratch.set(&name_ident);
        let (columns, without_rowid) = self.catalog.table_source_info(&self.key_scratch);
        let (alias_ident, _) = self.stmt.name_ident_text(fields.node_id_at(alias_idx));
        if alias_ident.is_empty() {
            self.scope
                .add_table(&name_ident, columns, without_rowid.into());
        } else {
            self.scope
                .add_aliased_table(&alias_ident, &name_ident, columns, without_rowid.into());
        }
    }

    pub(super) fn walk_source_ref<V: SemanticVisitor>(
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

        // `name` keeps the raw source text for display; catalog keys use
        // the identifier value (escape-collapsed).
        let name_ident = self
            .stmt
            .span_field_ident_text(fields, name_idx)
            .unwrap_or(Cow::Borrowed(name));
        self.key_scratch.set(&name_ident);
        let is_known = self.catalog.resolve_relation(&self.key_scratch)
            || self.catalog.resolve_table_function(&self.key_scratch);
        let (alias_ident, _) = self.stmt.name_ident_text(fields.node_id_at(alias_idx));
        let alias = if alias_ident.is_empty() {
            None
        } else {
            Some(&*alias_ident)
        };

        if V::WANTS_SOURCE_REF {
            let ev = SourceRefEvent {
                node_id,
                name_idx,
                range,
                name,
                alias,
                resolved: is_known,
                unchecked: None,
            };
            let mut cx = WalkCtx {
                catalog: self.catalog,
                scope: &mut self.scope,
            };
            visitor.on_source_ref(self.stmt, &mut cx, ev);
        }

        self.bind_source_ref(fields, name_idx, alias_idx);
    }

    pub(super) fn walk_scoped_source<V: SemanticVisitor>(
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

        let (alias_ident, _) = self.stmt.name_ident_text(fields.node_id_at(alias_idx));
        let alias = if alias_ident.is_empty() {
            None
        } else {
            Some(&*alias_ident)
        };

        if V::WANTS_SCOPED_SOURCE {
            visitor.on_scoped_source(self.stmt, ScopedSourceEvent { alias, body_id });
        }

        let cols = body_id.and_then(|id| self.result_columns(id));
        match alias {
            None => self.scope.add_anonymous(cols),
            Some(name) => self.scope.add_table(name, cols, RowIdPolicy::WithRowId),
        }
    }

    pub(super) fn walk_query<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        role: SemanticRole,
    ) {
        let SemanticRole::Query {
            from,
            columns,
            where_clause,
            groupby,
            having,
            orderby,
            limit_clause,
        } = role
        else {
            unreachable!("not a query role: {role:?}");
        };
        if V::WANTS_QUERY {
            visitor.enter_query(self.stmt, node_id);
        }

        // Bind all FROM sources first, then walk — so every expression in
        // this scope (including join ON clauses that precede later
        // sources) resolves against the complete source list.
        self.scope.push();
        let windows = self.window_names(fields);
        self.windows.push(windows);
        let outer_right_join = self.right_join_in_from;
        self.right_join_in_from = fields
            .node_id_at(from)
            .is_some_and(|id| self.has_right_join(id));
        if let Some(from_id) = fields.node_id_at(from) {
            self.register_sources(from_id);
        }
        self.walk_opt(visitor, fields.node_id_at(from));
        self.walk_opt(visitor, fields.node_id_at(columns));

        // Collect SELECT aliases so they are visible in WHERE, GROUP BY,
        // HAVING, ORDER BY, and LIMIT — matching SQLite's resolution rules.
        let aliases = self.collect_select_aliases(fields, columns);
        if !aliases.is_empty() {
            self.scope
                .add_table("", Some(aliases), RowIdPolicy::WithRowId);
        }

        self.note_where_terms(fields.node_id_at(where_clause));
        // SQLite moves HAVING terms without aggregates into WHERE.
        self.note_where_terms(fields.node_id_at(having));
        for idx in [where_clause, groupby, having, orderby, limit_clause] {
            self.walk_opt(visitor, fields.node_id_at(idx));
        }
        self.windows.pop();
        self.right_join_in_from = outer_right_join;
        self.scope.pop();

        if V::WANTS_QUERY {
            visitor.exit_query(self.stmt, self.catalog, node_id);
        }
    }

    /// Walk a join's sources, then its ON clause. An outer join's ON clause,
    /// or any ON clause when the FROM has a RIGHT or FULL join, may not
    /// reference tables joined to its right.
    pub(super) fn walk_join<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        role: SemanticRole,
    ) {
        let SemanticRole::Join {
            kind,
            left,
            right,
            on,
            ..
        } = role
        else {
            unreachable!("not a join role: {role:?}");
        };
        for idx in 0..fields.len() {
            #[expect(
                clippy::cast_possible_truncation,
                reason = "NodeFields holds at most 16"
            )]
            let idx = idx as u8;
            if idx != on {
                self.walk_opt(visitor, fields.node_id_at(idx));
            }
        }
        let restricted = self.right_join_in_from
            || JoinKind::from_name(self.stmt.enum_name(node_id, kind)).outer;
        let outer = self.on_clause.take();
        if V::WANTS_ON_CLAUSE_ORDER && restricted {
            let frame = self
                .scope
                .innermost()
                .expect("joins are walked inside a query scope");
            let mut inside = HashSet::new();
            for idx in [left, right] {
                let source = fields.node_id_at(idx).expect("a join has both sources");
                self.source_names(source, &mut inside);
            }
            let later = self
                .scope
                .frame_names(frame)
                .difference(&inside)
                .cloned()
                .collect();
            self.on_clause = Some(OnClauseLimit { frame, later });
        }
        self.note_where_terms(fields.node_id_at(on));
        self.walk_opt(visitor, fields.node_id_at(on));
        self.on_clause = outer;
    }

    /// Whether a FROM clause contains a RIGHT or FULL join.
    fn has_right_join(&self, node_id: AnyNodeId) -> bool {
        if let Some(children) = self.stmt.list_children(node_id) {
            return children
                .iter()
                .any(|&id| !id.is_null() && self.has_right_join(id));
        }
        let reader = StmtReader::new(self.stmt, self.roles);
        let (role, fields) = reader
            .role_for_node(node_id)
            .expect("a non-list node has fields");
        match role {
            SemanticRole::Join {
                kind, left, right, ..
            } => {
                JoinKind::from_name(self.stmt.enum_name(node_id, kind)).right
                    || [left, right]
                        .into_iter()
                        .filter_map(|idx| fields.node_id_at(idx))
                        .any(|id| self.has_right_join(id))
            }
            SemanticRole::Query { .. } | SemanticRole::CompoundScope { .. } => false,
            _ => self
                .stmt
                .child_node_ids(node_id)
                .into_iter()
                .any(|id| !id.is_null() && self.has_right_join(id)),
        }
    }

    /// Names a FROM subtree binds, without entering nested queries.
    fn source_names(&self, node_id: AnyNodeId, out: &mut HashSet<NameKey>) {
        if let Some(children) = self.stmt.list_children(node_id) {
            for &id in children {
                if !id.is_null() {
                    self.source_names(id, out);
                }
            }
            return;
        }
        let reader = StmtReader::new(self.stmt, self.roles);
        let (role, fields) = reader
            .role_for_node(node_id)
            .expect("a non-list node has fields");
        match role {
            SemanticRole::SourceRef { name, alias, .. } => {
                let (alias_ident, _) = self.stmt.name_ident_text(fields.node_id_at(alias));
                if !alias_ident.is_empty() {
                    out.insert(NameKey::new(&alias_ident));
                } else if let Some(name) = self.stmt.span_field_ident_text(&fields, name) {
                    out.insert(NameKey::new(&name));
                }
            }
            SemanticRole::ScopedSource { alias, .. } => {
                let (alias_ident, _) = self.stmt.name_ident_text(fields.node_id_at(alias));
                if !alias_ident.is_empty() {
                    out.insert(NameKey::new(&alias_ident));
                }
            }
            SemanticRole::Query { .. } | SemanticRole::CompoundScope { .. } => {}
            _ => {
                for id in self.stmt.child_node_ids(node_id) {
                    if !id.is_null() {
                        self.source_names(id, out);
                    }
                }
            }
        }
    }

    /// Names of the windows a query defines in its WINDOW clause.
    fn window_names(&self, fields: &NodeFields) -> HashSet<NameKey> {
        let reader = StmtReader::new(self.stmt, self.roles);
        let mut names = HashSet::new();
        for idx in 0..fields.len() {
            #[expect(
                clippy::cast_possible_truncation,
                reason = "NodeFields holds at most 16"
            )]
            let Some(list) = fields.node_id_at(idx as u8) else {
                continue;
            };
            for &item in self.stmt.list_children(list).unwrap_or_default() {
                if let Some((SemanticRole::WindowDefinition { name }, item_fields)) =
                    reader.role_for_node(item)
                {
                    let ident = self
                        .stmt
                        .span_field_ident_text(&item_fields, name)
                        .expect("a WINDOW clause entry has a name");
                    names.insert(NameKey::new(&ident));
                }
            }
        }
        names
    }

    fn collect_select_aliases(&self, fields: &NodeFields, columns_idx: u8) -> Vec<String> {
        let mut aliases = Vec::new();
        let Some(list_id) = fields.node_id_at(columns_idx) else {
            return aliases;
        };
        let reader = StmtReader::new(self.stmt, self.roles);
        reader.for_each_result_column(list_id, |child_fields, _flags, alias_idx, _expr| {
            let alias_node = child_fields.node_id_at(alias_idx);
            let (alias_ident, _) = reader.stmt().name_ident_text(alias_node);
            if !alias_ident.is_empty() {
                aliases.push(alias_ident.into_owned());
            }
            true
        });
        aliases
    }

    pub(super) fn walk_call<V: SemanticVisitor>(
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
            let name_ident = self
                .stmt
                .span_field_ident_text(fields, name_idx)
                .unwrap_or(Cow::Borrowed(name));
            self.key_scratch.set(&name_ident);
            let result = self.catalog.check_function(&self.key_scratch, arg_count);

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

    pub(super) fn walk_column_ref<V: SemanticVisitor>(
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
        let (table, table_range, table_ident) = match fields[table_idx as usize] {
            FieldValue::Span(sp) if !sp.is_empty() => {
                let (_, r) = self.stmt.span_text_abs(sp);
                (
                    Some(self.stmt.span_expanded_text(sp)),
                    Some(r),
                    Some(self.stmt.span_ident_text(sp)),
                )
            }
            _ => (None, None, None),
        };
        let (_, range) = self.stmt.span_text_abs(col_sp);
        // SQLite's DQS bug-compat applies only to identifiers that were
        // written with `"..."` quotes (not backticks or brackets) and that
        // came from the original source (not a macro expansion).
        let dqs_candidate = col_sp.is_macro_free() && col_sp.quote_char() == Some('"');

        // `table`/`column` above keep the raw source text for display; the
        // scope lookup uses the identifier value (escape-collapsed) to
        // match how sources and columns were registered.
        let column_ident = self.stmt.span_ident_text(col_sp);
        let table_key = match table_ident.as_deref() {
            Some(t) => {
                self.key_scratch.set(t);
                Some(&self.key_scratch)
            }
            None => None,
        };
        let resolution = self.scope.resolve_column(table_key, &column_ident);
        if V::WANTS_ON_CLAUSE_ORDER
            && let Some(limit) = &self.on_clause
            && let Some((frame, source)) = self.scope.resolving_source(table_key, &column_ident)
            && frame == limit.frame
            && limit.later.contains(&source)
        {
            let ev = OnClauseOrderEvent {
                name: table.unwrap_or(source.as_str()),
                range: table_range.unwrap_or(range),
            };
            visitor.on_on_clause_order(self.stmt, ev);
        }

        if V::WANTS_COLUMN_REF {
            let ev = ColumnRefEvent {
                node_id,
                column_idx,
                range,
                column,
                table,
                table_idx,
                table_range,
                resolution: &resolution,
                dqs_candidate,
                in_view_body: self.view_body_depth > 0,
                unchecked: self.unchecked,
                unqualified_only: self.unqualified_only,
            };
            let mut cx = WalkCtx {
                catalog: self.catalog,
                scope: &mut self.scope,
            };
            visitor.on_column_ref(self.stmt, &mut cx, ev);
        }
    }

    /// Report a window specification naming a window its query doesn't
    /// define, then walk its PARTITION BY, ORDER BY and frame.
    pub(super) fn walk_window_ref<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        role: SemanticRole,
    ) {
        let SemanticRole::WindowRef { reference, base } = role else {
            unreachable!("not a window role: {role:?}");
        };
        if V::WANTS_UNKNOWN_WINDOW
            && let Some(defined) = self.windows.last()
        {
            for idx in [reference, base] {
                if let Some((name, range)) = self.stmt.span_field_range(fields, idx)
                    && let Some(ident) = self.stmt.span_field_ident_text(fields, idx)
                    && !defined.contains(&NameKey::new(&ident))
                {
                    visitor.on_unknown_window(self.stmt, UnknownWindowEvent { name, range });
                }
            }
        }
        self.walk_children(visitor, node_id);
    }

    /// Walk an expression whose operands may be row values, after noting
    /// which ones may be.
    pub(super) fn walk_row_value_context<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        role: SemanticRole,
    ) {
        match role {
            SemanticRole::RowValueOperands {
                op,
                first,
                second,
                third,
            } => {
                let operands = [first, second, third].map(|idx| fields.node_id_at(idx));
                let op_name =
                    (op != FIELD_ABSENT).then(|| self.stmt.enum_name(node_id, op).to_owned());
                let compares = op == FIELD_ABSENT || {
                    let op = self.stmt.enum_name(node_id, op);
                    // SQLite reads `x IS NULL` and its variants as a test of
                    // one value, not a comparison.
                    is_comparison(op)
                        && !(op.starts_with("IS")
                            && operands
                                .into_iter()
                                .flatten()
                                .any(|e| self.is_null_literal(e)))
                };
                if compares {
                    for operand in operands {
                        self.allow_row_value(operand);
                    }
                    let widths: Vec<_> = operands
                        .into_iter()
                        .flatten()
                        .map(|e| self.expr_width(e))
                        .collect();
                    if let Some(&Some(first)) = widths.first()
                        && let Some(&Some(other)) = widths.iter().find(|w| **w != Some(first))
                    {
                        self.report_row_value(visitor, node_id, first, Some(other));
                    } else if op_name.as_deref().is_some_and(compares_elementwise)
                        && self.where_terms.contains(&node_id)
                        && let [Some(left), Some(right), None] = operands
                    {
                        self.allow_nested_row_values(visitor, left, right);
                    }
                }
            }
            SemanticRole::Case { operand, whens } => {
                if let Some(operand) = fields.node_id_at(operand) {
                    self.allow_row_value(Some(operand));
                    let width = self.expr_width(operand);
                    for value in self.case_when_values(fields, whens) {
                        self.allow_row_value(Some(value));
                        if let (Some(width), Some(other)) = (width, self.expr_width(value))
                            && width != other
                        {
                            self.report_row_value(visitor, value, width, Some(other));
                        }
                    }
                }
            }
            SemanticRole::In { operand, source } => {
                let operand = fields.node_id_at(operand).expect("IN has an operand");
                self.allow_row_value(Some(operand));
                // `IN ()` has no source.
                if let Some(source) = fields.node_id_at(source) {
                    self.check_in_source(visitor, operand, source);
                }
            }
            SemanticRole::Assignment { columns, value } => {
                if fields.node_id_at(columns).is_some() {
                    self.allow_row_value(fields.node_id_at(value));
                }
            }
            _ => unreachable!("not a row value context: {role:?}"),
        }
        self.walk_children(visitor, node_id);
    }

    /// Note the terms of a WHERE, ON or HAVING clause: its operands joined
    /// by AND or OR.
    pub(super) fn note_where_terms(&mut self, expr: Option<AnyNodeId>) {
        let Some(expr) = expr else {
            return;
        };
        let expr = self.unparen(expr);
        let reader = StmtReader::new(self.stmt, self.roles);
        if let Some((
            SemanticRole::RowValueOperands {
                op, first, second, ..
            },
            fields,
        )) = reader.role_for_node(expr)
            && op != FIELD_ABSENT
            && matches!(self.stmt.enum_name(expr, op), "AND" | "OR")
        {
            self.note_where_terms(fields.node_id_at(first));
            self.note_where_terms(fields.node_id_at(second));
        } else {
            self.where_terms.insert(expr);
        }
    }

    /// `SQLite`'s query planner splits a WHERE term comparing row values
    /// with `=` or `IS` into one term per element, so the elements may be
    /// row values too, as long as they match. Whether it splits an OR's
    /// branches depends on the indexes, so they are accepted too.
    fn allow_nested_row_values<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        left: AnyNodeId,
        right: AnyNodeId,
    ) {
        let (Some(left), Some(right)) = (self.row_value_items(left), self.row_value_items(right))
        else {
            return;
        };
        for (&l, &r) in left.iter().zip(right) {
            let (Some(lw), Some(rw)) = (self.expr_width(l), self.expr_width(r)) else {
                continue;
            };
            if lw == 1 && rw == 1 {
                continue;
            }
            self.allow_row_value(Some(l));
            self.allow_row_value(Some(r));
            if lw == rw {
                self.allow_nested_row_values(visitor, l, r);
            } else {
                self.report_row_value(visitor, l, lw, Some(rw));
            }
        }
    }

    /// The items of `expr` when it is a row value.
    fn row_value_items(&self, expr: AnyNodeId) -> Option<&'b [AnyNodeId]> {
        let expr = self.unparen(expr);
        let reader = StmtReader::new(self.stmt, self.roles);
        let (SemanticRole::RowValue { items }, fields) = reader.role_for_node(expr)? else {
            return None;
        };
        self.stmt.list_children(fields.node_id_at(items)?)
    }

    /// The WHEN values a CASE compares its operand with.
    fn case_when_values(&self, fields: &NodeFields, whens: u8) -> Vec<AnyNodeId> {
        let reader = StmtReader::new(self.stmt, self.roles);
        let whens = fields
            .node_id_at(whens)
            .and_then(|id| self.stmt.list_children(id))
            .expect("a CASE has WHEN branches");
        whens
            .iter()
            .map(|&branch| {
                let Some((SemanticRole::CaseWhen { when }, branch_fields)) =
                    reader.role_for_node(branch)
                else {
                    unreachable!("a CASE holds WHEN branches");
                };
                branch_fields
                    .node_id_at(when)
                    .expect("a WHEN branch has a value")
            })
            .collect()
    }

    /// Walk a row value, which must be in a position that takes one.
    pub(super) fn walk_row_value<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        items: u8,
    ) {
        self.walk_children(visitor, node_id);
        if self.row_value_positions.remove(&node_id) {
            return;
        }
        let width = fields
            .node_id_at(items)
            .and_then(|id| self.stmt.list_children(id))
            .expect("a row value has items")
            .len();
        self.report_row_value(visitor, node_id, width, None);
    }

    fn report_row_value<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node: AnyNodeId,
        width: usize,
        compared_with: Option<usize>,
    ) {
        if !V::WANTS_ROW_VALUE {
            return;
        }
        let range = self
            .stmt
            .node_range(node)
            .expect("an expression spans source");
        let ev = RowValueEvent {
            range,
            width,
            compared_with,
        };
        visitor.on_row_value(self.stmt, ev);
    }

    /// Note that `expr`, or the expression it parenthesises, may be a row
    /// value.
    fn allow_row_value(&mut self, expr: Option<AnyNodeId>) {
        if let Some(expr) = expr {
            let expr = self.unparen(expr);
            self.row_value_positions.insert(expr);
        }
    }

    fn is_null_literal(&self, expr: AnyNodeId) -> bool {
        let expr = self.unparen(expr);
        let reader = StmtReader::new(self.stmt, self.roles);
        matches!(
            reader.role_for_node(expr),
            Some((SemanticRole::Literal { kind, .. }, _)) if self.stmt.enum_name(expr, kind) == "NULL"
        )
    }

    fn unparen(&self, mut expr: AnyNodeId) -> AnyNodeId {
        let reader = StmtReader::new(self.stmt, self.roles);
        while let Some((SemanticRole::Paren { expr: inner }, fields)) = reader.role_for_node(expr) {
            expr = fields
                .node_id_at(inner)
                .expect("a parenthesised expression has an inner one");
        }
        expr
    }

    /// How many values `expr` produces: a row value's items, a subquery's
    /// columns, or 1. `None` when a subquery's columns are unknown.
    fn expr_width(&self, expr: AnyNodeId) -> Option<usize> {
        let expr = self.unparen(expr);
        let reader = StmtReader::new(self.stmt, self.roles);
        match reader.role_for_node(expr) {
            Some((SemanticRole::ScalarSubquery { select }, fields)) => {
                let select = fields.node_id_at(select).expect("a subquery has a SELECT");
                self.result_column_count(select)
            }
            Some((SemanticRole::RowValue { items }, fields)) => Some(
                fields
                    .node_id_at(items)
                    .and_then(|id| self.stmt.list_children(id))
                    .expect("a row value has items")
                    .len(),
            ),
            _ => Some(1),
        }
    }

    /// Report an IN subquery or table whose width differs from the
    /// operand's. A value list's entries are checked as values instead.
    fn check_in_source<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        operand: AnyNodeId,
        source: AnyNodeId,
    ) {
        let Some(expected) = self.expr_width(operand) else {
            return;
        };
        if let Some(values) = self.stmt.list_children(source) {
            if expected > 1 {
                for &value in values {
                    self.allow_row_value(Some(value));
                }
            }
            return;
        }
        let reader = StmtReader::new(self.stmt, self.roles);
        let got = match reader.role_for_node(source) {
            Some((SemanticRole::SourceRef { name, .. }, fields)) => {
                let table = self
                    .stmt
                    .span_field_ident_text(&fields, name)
                    .expect("an IN table has a name");
                self.key_scratch.set(&table);
                self.catalog
                    .star_columns(&self.key_scratch)
                    .map(|cols| cols.len())
            }
            _ => self.result_column_count(source),
        };
        if V::WANTS_SUBQUERY_COLUMNS
            && let Some(got) = got
            && got != expected
        {
            self.report_subquery_columns(visitor, source, got, expected);
        }
    }

    /// Walk a subquery used as a value, which must produce one column unless
    /// its position takes a row value.
    pub(super) fn walk_scalar_subquery<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        select: u8,
    ) {
        self.walk_children(visitor, node_id);
        if self.row_value_positions.remove(&node_id) || !V::WANTS_SUBQUERY_COLUMNS {
            return;
        }
        let select = fields.node_id_at(select).expect("a subquery has a SELECT");
        if let Some(got) = self.result_column_count(select)
            && got != 1
        {
            self.report_subquery_columns(visitor, node_id, got, 1);
        }
    }

    fn report_subquery_columns<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node: AnyNodeId,
        got: usize,
        expected: usize,
    ) {
        let range = self.stmt.node_range(node).expect("a subquery spans source");
        let ev = SubqueryColumnsEvent {
            range,
            got,
            expected,
        };
        visitor.on_subquery_columns(self.stmt, ev);
    }

    pub(super) fn walk_compound_scope<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        role: SemanticRole,
    ) {
        let SemanticRole::CompoundScope {
            left: left_idx,
            right: right_idx,
            orderby: orderby_idx,
            limit_clause: limit_idx,
            op,
        } = role
        else {
            unreachable!("not a compound role: {role:?}");
        };
        let left = fields
            .node_id_at(left_idx)
            .expect("a compound has a left arm");
        let right = fields
            .node_id_at(right_idx)
            .expect("a compound has a right arm");
        self.walk_node(visitor, left);
        self.walk_node(visitor, right);
        if V::WANTS_COMPOUND_COLUMNS {
            self.check_compound_columns(visitor, node_id, op, left, right);
        }

        let orderby = fields.node_id_at(orderby_idx);
        let limit = fields.node_id_at(limit_idx);
        if orderby.is_none() && limit.is_none() {
            return;
        }
        // SQLite matches compound ORDER BY terms against the arms' result
        // columns; approximate by resolving in a frame holding every arm's
        // sources and select aliases. Column-checked — but a valid arm
        // column that is not a result column is accepted (a known gap).
        self.scope.push();
        let mut aliases = Vec::new();
        self.register_arm_sources(fields.node_id_at(left_idx), &mut aliases);
        self.register_arm_sources(fields.node_id_at(right_idx), &mut aliases);
        if !aliases.is_empty() {
            self.scope
                .add_table("", Some(aliases), RowIdPolicy::WithRowId);
        }
        self.walk_opt(visitor, orderby);
        self.walk_opt(visitor, limit);
        self.scope.pop();
    }

    /// Bind a compound arm's FROM sources and collect its select aliases,
    /// recursing through nested compounds. VALUES / WITH arms contribute
    /// no addressable sources.
    fn register_arm_sources(&mut self, arm: Option<AnyNodeId>, aliases: &mut Vec<String>) {
        let Some(id) = arm else {
            return;
        };
        let Some((tag, fields)) = self.stmt.extract_fields(id) else {
            return;
        };
        match StmtReader::new(self.stmt, self.roles).role_for_tag(tag) {
            SemanticRole::Query { from, columns, .. } => {
                if let Some(from_id) = fields.node_id_at(from) {
                    self.register_sources(from_id);
                }
                aliases.extend(self.collect_select_aliases(&fields, columns));
            }
            SemanticRole::CompoundScope { left, right, .. } => {
                self.register_arm_sources(fields.node_id_at(left), aliases);
                self.register_arm_sources(fields.node_id_at(right), aliases);
            }
            _ => {}
        }
    }

    /// Walk a VALUES clause, reporting the first row whose width differs
    /// from the first row's.
    pub(super) fn walk_values<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        rows: u8,
    ) {
        self.walk_children(visitor, node_id);
        if !V::WANTS_VALUES_TERMS {
            return;
        }
        let rows = self.values_rows(fields, rows);
        let first = self.values_row_width(rows[0]);
        let Some(&row) = rows[1..]
            .iter()
            .find(|&&r| self.values_row_width(r) != first)
        else {
            return;
        };
        let range = self
            .stmt
            .node_range(row)
            .expect("a VALUES row spans source");
        let ev = ValuesTermsEvent {
            range,
            first,
            row: self.values_row_width(row),
        };
        visitor.on_values_terms(self.stmt, ev);
    }

    fn values_rows(&self, fields: &NodeFields, rows: u8) -> &'b [AnyNodeId] {
        let rows = fields
            .node_id_at(rows)
            .and_then(|id| self.stmt.list_children(id))
            .expect("VALUES has a row list");
        assert!(!rows.is_empty(), "VALUES has a row");
        rows
    }

    fn values_row_width(&self, row: AnyNodeId) -> usize {
        self.stmt
            .list_children(row)
            .expect("a VALUES row is a list")
            .len()
    }

    /// Report compound arms with different column counts. Arms whose count
    /// isn't known, such as a `*` over a source with unknown columns, are skipped.
    fn check_compound_columns<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        op: u8,
        left: AnyNodeId,
        right: AnyNodeId,
    ) {
        let (Some(left_count), Some(right_count)) = (
            self.result_column_count(left),
            self.result_column_count(right),
        ) else {
            return;
        };
        if left_count == right_count {
            return;
        }
        // Variant names spell the SQL with underscores, e.g. `UNION_ALL`.
        let operator = self.stmt.enum_name(node_id, op).replace('_', " ");
        let range = self
            .stmt
            .node_range(right)
            .expect("a compound arm spans source");
        let ev = CompoundColumnsEvent {
            operator: &operator,
            range,
            left: left_count,
            right: right_count,
        };
        visitor.on_compound_columns(self.stmt, ev);
    }
}

/// Whether `SQLite` compares row values with this operator element by
/// element.
fn compares_elementwise(name: &str) -> bool {
    matches!(name, "EQ" | "EQ_DOUBLE" | "IS" | "IS_NOT_DISTINCT")
}

/// What a join's kind implies for ON clause ordering.
struct JoinKind {
    outer: bool,
    right: bool,
}

impl JoinKind {
    /// From the dialect's name for the join type, such as `NATURAL_LEFT`.
    fn from_name(name: &str) -> Self {
        let base = name.strip_prefix("NATURAL_").unwrap_or(name);
        Self {
            outer: matches!(base, "LEFT" | "RIGHT" | "FULL"),
            right: matches!(base, "RIGHT" | "FULL"),
        }
    }
}

/// Whether the dialect's name for an operator is a comparison, whose
/// operands may be row values.
fn is_comparison(name: &str) -> bool {
    matches!(
        name,
        "EQ" | "EQ_DOUBLE"
            | "NE"
            | "NE_ANGLE"
            | "LT"
            | "LE"
            | "GT"
            | "GE"
            | "IS"
            | "IS_NOT"
            | "IS_DISTINCT"
            | "IS_NOT_DISTINCT"
    )
}
