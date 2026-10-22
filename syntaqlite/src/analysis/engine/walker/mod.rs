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

mod query;
mod statements;
mod visitor;

pub(crate) use visitor::*;

use std::collections::HashSet;

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, NodeFields};

use crate::analysis::QualifiedRefContext;
use crate::analysis::catalog::Catalog;
use crate::analysis::name_key::NameKey;
use crate::analysis::result_columns::ColumnResolver;
use crate::analysis::stmt_reader::StmtReader;
use crate::dialect::SemanticRole;

use super::query_scope::{QueryScope, RowIdPolicy};

// ── SemanticWalker ────────────────────────────────────────────────────────────

/// Owns the state threaded through one walk. Constructed fresh per
/// statement; `run` drives role-dispatched recursion from `root`.
pub(crate) struct SemanticWalker<'a, 'b> {
    stmt: &'a mut AnyParsedStatement<'b>,
    catalog: &'a mut Catalog,
    scope: QueryScope,
    roles: &'a [SemanticRole],
    /// Depth of nested CREATE VIEW bodies currently being walked.
    view_body_depth: u32,
    /// The table whose CREATE TABLE column list is being walked.
    defining_table: Option<DefiningTable>,
    /// Names `SQLite` doesn't check are being walked.
    unchecked: Option<UncheckedRef>,
    /// Window names defined by each enclosing query, innermost last.
    windows: Vec<HashSet<NameKey>>,
    /// Whether the current query's FROM clause has a RIGHT or FULL join.
    right_join_in_from: bool,
    /// Sources an ON clause being walked may not reference.
    on_clause: Option<OnClauseLimit>,
    /// Expressions in positions that accept a row value, such as a
    /// comparison's operands.
    row_value_positions: HashSet<AnyNodeId>,
    /// Terms of the WHERE, ON and HAVING clauses being walked.
    where_terms: HashSet<AnyNodeId>,
    /// Expressions are being walked where `SQLite` forbids qualified names.
    unqualified_only: Option<QualifiedRefContext>,
    /// Reusable lookup key — catalog/scope probes share one allocation.
    key_scratch: NameKey,
}

/// The names an ON clause may not reference: sources in frame `frame` that
/// are joined to its right.
struct OnClauseLimit {
    frame: usize,
    later: HashSet<NameKey>,
}

/// A table being created, as seen by expressions in its own definition.
struct DefiningTable {
    name: String,
    columns: Option<Vec<String>>,
    rowid: RowIdPolicy,
}

impl<'a, 'b> SemanticWalker<'a, 'b> {
    pub(crate) fn new(
        stmt: &'a mut AnyParsedStatement<'b>,
        catalog: &'a mut Catalog,
        roles: &'a [SemanticRole],
    ) -> Self {
        Self {
            stmt,
            catalog,
            scope: QueryScope::default(),
            roles,
            view_body_depth: 0,
            defining_table: None,
            unchecked: None,
            windows: Vec::new(),
            right_join_in_from: false,
            on_clause: None,
            row_value_positions: HashSet::new(),
            where_terms: HashSet::new(),
            unqualified_only: None,
            key_scratch: NameKey::default(),
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
        self.dispatch_role(visitor, node_id, role, &fields);
    }

    /// Route a node to its role-specific walker.
    fn dispatch_role<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        role: SemanticRole,
        fields: &NodeFields,
    ) {
        match role {
            SemanticRole::DefineTable { name, .. } => {
                self.walk_define_table(visitor, node_id, fields, name);
            }
            SemanticRole::DefineView { select, .. } => {
                self.walk_define_view(visitor, node_id, fields, select);
            }
            SemanticRole::DefineFunction { select, .. } => {
                self.walk_opt(visitor, fields.node_id_at(select));
            }
            SemanticRole::ReturnSpec { .. } | SemanticRole::Import { .. } => {}

            SemanticRole::Transparent
            | SemanticRole::ColumnDef { .. }
            | SemanticRole::ResultColumn { .. }
            | SemanticRole::CteBinding { .. }
            | SemanticRole::WindowDefinition { .. }
            | SemanticRole::CaseWhen { .. }
            | SemanticRole::Paren { .. }
            | SemanticRole::Literal { .. } => self.walk_children(visitor, node_id),
            SemanticRole::RowValue { items } => {
                self.walk_row_value(visitor, node_id, fields, items);
            }
            SemanticRole::WindowRef { .. } => self.walk_window_ref(visitor, node_id, fields, role),
            SemanticRole::Values { rows } => self.walk_values(visitor, node_id, fields, rows),
            SemanticRole::RowValueOperands { .. }
            | SemanticRole::Case { .. }
            | SemanticRole::In { .. }
            | SemanticRole::Assignment { .. } => {
                self.walk_row_value_context(visitor, node_id, fields, role);
            }
            SemanticRole::ScalarSubquery { select } => {
                self.walk_scalar_subquery(visitor, node_id, fields, select);
            }
            SemanticRole::Join { .. } => self.walk_join(visitor, node_id, fields, role),

            SemanticRole::Call { name, args } => {
                self.walk_call(visitor, node_id, fields, name, args);
            }
            SemanticRole::ColumnRef { column, table } => {
                self.walk_column_ref(visitor, node_id, fields, column, table);
            }
            SemanticRole::SourceRef { name, alias, .. } => {
                self.walk_source_ref(visitor, node_id, fields, name, alias);
            }
            SemanticRole::ScopedSource { body, alias } => {
                self.walk_scoped_source(visitor, fields, body, alias);
            }
            SemanticRole::Query { .. } => self.walk_query(visitor, node_id, fields, role),
            SemanticRole::CteScope {
                recursive: _,
                bindings,
                body,
            } => self.walk_cte_scope(visitor, fields, bindings, body),
            SemanticRole::TriggerScope { .. } => {
                self.walk_trigger_scope(visitor, node_id, fields, role);
            }
            SemanticRole::TriggerUpdateOf { columns } => {
                self.walk_unchecked(visitor, fields, columns, UncheckedRef::TriggerUpdateOf);
            }
            SemanticRole::DmlScope {
                with_recursive: _,
                with_ctes,
                where_clause,
            } => self.walk_dml_scope(visitor, fields, with_ctes, where_clause),
            SemanticRole::InsertScope { .. } => self.walk_insert_scope(visitor, fields, role),
            SemanticRole::UpsertScope {
                columns,
                target_where,
                setlist,
                update_where,
            } => {
                self.walk_upsert_scope(
                    visitor,
                    fields,
                    columns,
                    target_where,
                    setlist,
                    update_where,
                );
            }
            SemanticRole::CompoundScope { .. } => {
                self.walk_compound_scope(visitor, node_id, fields, role);
            }
            SemanticRole::AlterTable { target, .. } => {
                self.walk_alter_target(visitor, node_id, fields, target);
            }
            SemanticRole::IndexScope { .. } => {
                self.walk_index_scope(visitor, node_id, fields, role);
            }
            SemanticRole::ColumnConstraint { .. } | SemanticRole::TableConstraint { .. } => {
                self.walk_constraint(visitor, fields, role);
            }
            SemanticRole::DropObject { .. } => self.walk_drop(visitor, node_id, fields, role),
            SemanticRole::ForeignKey { table, columns } => {
                self.walk_foreign_key(visitor, node_id, fields, table, columns);
            }
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

    /// Walk names `SQLite` never checks, so visitors can accept failures.
    fn walk_unchecked<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        fields: &NodeFields,
        idx: u8,
        reason: UncheckedRef,
    ) {
        self.unchecked = Some(reason);
        self.walk_opt(visitor, fields.node_id_at(idx));
        self.unchecked = None;
    }

    /// The columns `query` produces, resolved against the catalog as it
    /// stands at this point of the walk.
    fn result_columns(&self, query: AnyNodeId) -> Option<Vec<String>> {
        ColumnResolver::new(self.stmt, self.roles, self.catalog).result_columns(query)
    }

    fn result_column_count(&self, query: AnyNodeId) -> Option<usize> {
        ColumnResolver::new(self.stmt, self.roles, self.catalog).result_column_count(query)
    }
}
