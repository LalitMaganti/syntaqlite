// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Statements other than SELECT: schema changes, triggers, WITH
//! clauses and INSERT, UPDATE and DELETE.

use std::borrow::Cow;

use syntaqlite_syntax::any::{AnyNodeId, FieldValue, NodeFields};
use syntaqlite_syntax::source::DocRange;

use crate::analysis::catalog::drop_kind_is_view;
use crate::analysis::engine::query_scope::RowIdPolicy;
use crate::analysis::name_key::NameKey;
use crate::analysis::stmt_reader::StmtReader;
use crate::analysis::{BuiltinChange, QualifiedRefContext};
use crate::dialect::{FIELD_ABSENT, SemanticRole};

use super::visitor::{
    BuiltinModifyEvent, CteBindingEvent, CteColumnCountMismatchEvent, DropWrongKindEvent,
    IndexOnViewEvent, InsertValuesEvent, SemanticVisitor, SourceRefEvent, UncheckedRef, WalkCtx,
};
use super::{DefiningTable, SemanticWalker};

impl<'b> SemanticWalker<'_, 'b> {
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

    /// Existence-check an `ALTER TABLE` or `DROP` target. Mirrors the DML
    /// target check: fire a source-ref event so the diagnostic visitor reports
    /// an unknown table (with suggestions). Resolution ignores tombstones so a
    /// `RENAME TO` or `DROP` — which the catalog already applied for this
    /// statement, retiring the name — doesn't flag its own target as missing.
    fn walk_schema_target<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        target_idx: u8,
    ) -> (String, DocRange) {
        // target → QualifiedName node; its object name is the first child.
        let target_id = fields
            .node_id_at(target_idx)
            .expect("a schema statement has a target");
        let (_, qf) = self
            .stmt
            .extract_fields(target_id)
            .expect("the target is a qualified name");
        let obj_id = qf
            .node_id_at(0)
            .expect("a qualified name has an object name");
        let (name, range) = self.stmt.name_text(Some(obj_id));
        // `name` keeps the raw source text for display; the catalog key uses
        // the identifier value (escape-collapsed).
        let name_ident = self.stmt.name_ident_text(Some(obj_id)).0.into_owned();
        if !V::WANTS_SOURCE_REF {
            return (name_ident, range);
        }
        self.key_scratch.set(&name_ident);
        let resolved = self.catalog.relation_defined(&self.key_scratch);
        let ev = SourceRefEvent {
            node_id,
            name_idx: target_idx,
            range,
            name,
            alias: None,
            resolved,
            unchecked: self.unchecked,
        };
        let mut cx = WalkCtx {
            catalog: self.catalog,
            scope: &mut self.scope,
        };
        visitor.on_source_ref(self.stmt, &mut cx, ev);
        (name_ident, range)
    }

    /// Record a CREATE TABLE's definitions, then walk its body with the
    /// table available to constraint expressions.
    pub(super) fn walk_define_table<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        name_idx: u8,
    ) {
        if V::WANTS_RELATION_DEFINITION || V::WANTS_COLUMN_DEFINITION {
            self.emit_ddl_definitions(visitor, node_id);
        }
        let name = self
            .stmt
            .span_field_ident_text(fields, name_idx)
            .expect("CREATE TABLE names its table");
        self.key_scratch.set(&name);
        let (columns, without_rowid) = self.catalog.table_source_info(&self.key_scratch);
        self.defining_table = Some(DefiningTable {
            name: name.into_owned(),
            columns,
            rowid: without_rowid.into(),
        });
        self.walk_children(visitor, node_id);
        self.defining_table = None;
    }

    pub(super) fn walk_define_view<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        select_idx: u8,
    ) {
        if V::WANTS_RELATION_DEFINITION || V::WANTS_COLUMN_DEFINITION {
            self.emit_ddl_definitions(visitor, node_id);
        }
        // SQLite defers view-body name resolution to first use.
        self.view_body_depth += 1;
        self.walk_opt(visitor, fields.node_id_at(select_idx));
        self.view_body_depth -= 1;
    }

    /// Resolve a constraint's CHECK expression, and its GENERATED expression
    /// or key columns, against the table being created, then its REFERENCES
    /// clause against the parent. DEFAULT is not a column scope.
    pub(super) fn walk_constraint<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        fields: &NodeFields,
        role: SemanticRole,
    ) {
        let (check_idx, no_rowid_idxs, references_idx) = match role {
            SemanticRole::ColumnConstraint {
                check,
                generated,
                references,
            } => (
                check,
                [
                    (generated, Some(QualifiedRefContext::GeneratedColumn)),
                    (FIELD_ABSENT, None),
                ],
                references,
            ),
            SemanticRole::TableConstraint {
                check,
                columns,
                fk_columns,
                references,
            } => (
                check,
                [
                    (columns, Some(QualifiedRefContext::IndexExpression)),
                    (fk_columns, None),
                ],
                references,
            ),
            _ => unreachable!("not a constraint role: {role:?}"),
        };
        let Some(table) = self.defining_table.take() else {
            return;
        };
        self.walk_in_table_scope(visitor, fields.node_id_at(check_idx), &table, table.rowid);
        // Generated columns and key columns cannot reference rowid.
        for (idx, unqualified_only) in no_rowid_idxs {
            self.unqualified_only = unqualified_only;
            self.walk_in_table_scope(
                visitor,
                fields.node_id_at(idx),
                &table,
                RowIdPolicy::WithoutRowId,
            );
        }
        self.unqualified_only = None;
        self.walk_opt(visitor, fields.node_id_at(references_idx));
        self.defining_table = Some(table);
    }

    /// Resolve a REFERENCES clause's parent table and columns. Failures are
    /// reported as unchecked so visitors can accept them.
    pub(super) fn walk_foreign_key<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        table_idx: u8,
        columns_idx: u8,
    ) {
        let (name, range) = self
            .stmt
            .span_field_range(fields, table_idx)
            .expect("REFERENCES names its table");
        let name_ident = self
            .stmt
            .span_field_ident_text(fields, table_idx)
            .expect("REFERENCES names its table");
        self.key_scratch.set(&name_ident);
        if V::WANTS_SOURCE_REF {
            let ev = SourceRefEvent {
                node_id,
                name_idx: table_idx,
                range,
                name,
                alias: None,
                resolved: self.catalog.resolve_relation(&self.key_scratch),
                unchecked: Some(UncheckedRef::ForeignKeyParent),
            };
            let mut cx = WalkCtx {
                catalog: self.catalog,
                scope: &mut self.scope,
            };
            visitor.on_source_ref(self.stmt, &mut cx, ev);
        }
        let (columns, without_rowid) = self.catalog.table_source_info(&self.key_scratch);
        self.scope.push();
        self.scope
            .add_table(&name_ident, columns, without_rowid.into());
        self.walk_unchecked(visitor, fields, columns_idx, UncheckedRef::ForeignKeyParent);
        self.scope.pop();
    }

    fn walk_in_table_scope<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node: Option<AnyNodeId>,
        table: &DefiningTable,
        rowid: RowIdPolicy,
    ) {
        let Some(node) = node else {
            return;
        };
        self.scope.push();
        self.scope
            .add_table(&table.name, table.columns.clone(), rowid);
        self.walk_node(visitor, node);
        self.scope.pop();
    }

    /// Existence-check a `CREATE INDEX` target table, then resolve the
    /// indexed columns and partial-index WHERE against it.
    pub(super) fn walk_index_scope<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        role: SemanticRole,
    ) {
        let SemanticRole::IndexScope {
            table: table_idx,
            columns,
            where_clause,
        } = role
        else {
            unreachable!("not an index role: {role:?}");
        };
        self.scope.push();
        {
            let (name, range) = self
                .stmt
                .span_field_range(fields, table_idx)
                .expect("CREATE INDEX names its table");
            let name_ident = self
                .stmt
                .span_field_ident_text(fields, table_idx)
                .expect("CREATE INDEX names its table");
            self.key_scratch.set(&name_ident);
            if V::WANTS_SOURCE_REF {
                let ev = SourceRefEvent {
                    node_id,
                    name_idx: table_idx,
                    range,
                    name,
                    alias: None,
                    resolved: self.catalog.resolve_relation(&self.key_scratch),
                    unchecked: None,
                };
                let mut cx = WalkCtx {
                    catalog: self.catalog,
                    scope: &mut self.scope,
                };
                visitor.on_source_ref(self.stmt, &mut cx, ev);
            }
            self.check_builtin(visitor, &name_ident, range, BuiltinChange::Index);
            if V::WANTS_INDEX_ON_VIEW && self.catalog.is_view(&self.key_scratch) {
                let ev = IndexOnViewEvent { name, range };
                visitor.on_index_on_view(self.stmt, ev);
            }
            let (columns, _) = self.catalog.table_source_info(&self.key_scratch);
            // Index expressions cannot reference rowid.
            self.scope
                .add_table(&name_ident, columns, RowIdPolicy::WithoutRowId);
        }
        self.unqualified_only = Some(QualifiedRefContext::IndexExpression);
        self.walk_opt(visitor, fields.node_id_at(columns));
        self.unqualified_only = None;
        self.walk_opt(visitor, fields.node_id_at(where_clause));
        self.scope.pop();
    }

    pub(super) fn walk_alter_target<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        target_idx: u8,
    ) {
        let (name, range) = self.walk_schema_target(visitor, node_id, fields, target_idx);
        self.check_builtin(visitor, &name, range, BuiltinChange::Alter);
    }

    /// Report a DDL change to one of the dialect's built-in tables. Returns
    /// whether the target is a built-in.
    fn check_builtin<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        name: &str,
        range: DocRange,
        change: BuiltinChange,
    ) -> bool {
        self.key_scratch.set(name);
        if !self
            .catalog
            .visible_relation_is_dialect_builtin(&self.key_scratch)
        {
            return false;
        }
        if V::WANTS_BUILTIN_MODIFY {
            let ev = BuiltinModifyEvent {
                name,
                range,
                change,
            };
            visitor.on_builtin_modify(self.stmt, ev);
        }
        true
    }

    /// Existence-check a `DROP TABLE` / `DROP VIEW` target. With `IF EXISTS`
    /// it still resolves for navigation, but a missing target is accepted.
    /// Indexes and triggers aren't tracked, so aren't checked.
    pub(super) fn walk_drop<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        role: SemanticRole,
    ) {
        let SemanticRole::DropObject {
            kind,
            target,
            if_exists,
        } = role
        else {
            unreachable!("not a drop role: {role:?}");
        };
        let Some(drops_view) = drop_kind_is_view(self.stmt.enum_name(node_id, kind)) else {
            return;
        };
        if matches!(fields[if_exists as usize], FieldValue::Bool(true)) {
            self.unchecked = Some(UncheckedRef::DropIfExists);
        }
        let (name, range) = self.walk_schema_target(visitor, node_id, fields, target);
        self.unchecked = None;
        if self.check_builtin(visitor, &name, range, BuiltinChange::Drop) {
            return;
        }
        if V::WANTS_DROP_WRONG_KIND {
            self.key_scratch.set(&name);
            if self.catalog.resolve_relation(&self.key_scratch) {
                let is_view = self.catalog.is_view(&self.key_scratch);
                if is_view != drops_view {
                    let ev = DropWrongKindEvent {
                        name: &name,
                        range,
                        is_view,
                    };
                    visitor.on_drop_wrong_kind(self.stmt, ev);
                }
            }
        }
    }

    /// Existence-check a trigger's target, then walk its `UPDATE OF` list,
    /// WHEN and body with OLD and NEW standing for the target's row.
    pub(super) fn walk_trigger_scope<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        node_id: AnyNodeId,
        fields: &NodeFields,
        role: SemanticRole,
    ) {
        let SemanticRole::TriggerScope {
            target,
            when,
            body,
            event,
        } = role
        else {
            unreachable!("not a trigger role: {role:?}");
        };
        let (target_name, range) = self.walk_schema_target(visitor, node_id, fields, target);
        self.check_builtin(visitor, &target_name, range, BuiltinChange::Trigger);
        self.scope.push();
        for row in ["OLD", "NEW"] {
            self.key_scratch.set(&target_name);
            let (columns, without_rowid) = self.catalog.table_source_info(&self.key_scratch);
            self.scope
                .add_aliased_table(row, &target_name, columns, without_rowid.into());
        }
        self.walk_opt(visitor, fields.node_id_at(event));
        self.walk_opt(visitor, fields.node_id_at(when));
        self.walk_opt(visitor, fields.node_id_at(body));
        self.scope.pop();
    }

    pub(super) fn walk_upsert_scope<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        fields: &NodeFields,
        columns_idx: u8,
        target_where_idx: u8,
        setlist_idx: u8,
        update_where_idx: u8,
    ) {
        // The conflict target cannot see `excluded`.
        self.walk_opt(visitor, fields.node_id_at(columns_idx));
        self.walk_opt(visitor, fields.node_id_at(target_where_idx));
        // In DO UPDATE, `excluded.<col>` is the row that would have been
        // inserted — same column set as the insert target, which is the only
        // named source in the enclosing DML frame.
        let cols = self.scope.sole_named_table_columns();
        self.scope.push();
        self.scope
            .add_table("excluded", cols, RowIdPolicy::WithRowId);
        self.walk_opt(visitor, fields.node_id_at(setlist_idx));
        self.walk_opt(visitor, fields.node_id_at(update_where_idx));
        self.scope.pop();
    }

    pub(super) fn walk_cte_scope<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        fields: &NodeFields,
        bindings_idx: u8,
        body_idx: u8,
    ) {
        self.catalog.push_query_scope();
        self.register_cte_bindings(visitor, fields, bindings_idx);
        self.walk_opt(visitor, fields.node_id_at(body_idx));
        self.catalog.pop_query_scope();
    }

    pub(super) fn walk_dml_scope<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        fields: &NodeFields,
        bindings_idx: u8,
    ) {
        let has_ctes = bindings_idx != FIELD_ABSENT;
        if has_ctes {
            self.catalog.push_query_scope();
            self.register_cte_bindings(visitor, fields, bindings_idx);
        }
        self.scope.push();
        // Bind the DML target and any UPDATE ... FROM sources before the
        // SET/WHERE expressions resolve, regardless of field order.
        for idx in 0..fields.len() {
            if idx != bindings_idx as usize
                && let FieldValue::NodeId(child_id) = fields[idx]
            {
                self.register_sources(child_id);
            }
        }
        self.walk_dml_children_except_ctes(visitor, fields, bindings_idx);
        self.scope.pop();
        if has_ctes {
            self.catalog.pop_query_scope();
        }
    }

    pub(super) fn walk_insert_scope<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        fields: &NodeFields,
        role: SemanticRole,
    ) {
        let SemanticRole::InsertScope {
            with_ctes,
            table,
            columns,
            source,
            ..
        } = role
        else {
            unreachable!("not an insert role: {role:?}");
        };
        self.walk_dml_scope(visitor, fields, with_ctes);
        // `DEFAULT VALUES` has no source.
        if V::WANTS_INSERT_VALUES
            && let Some(source) = fields.node_id_at(source)
        {
            let table = fields.node_id_at(table).expect("an INSERT names its table");
            self.check_insert_values(visitor, table, fields.node_id_at(columns), source);
        }
    }

    /// Report an INSERT whose source supplies a different number of values
    /// than its column list, or than the table has when the list is omitted.
    fn check_insert_values<V: SemanticVisitor>(
        &mut self,
        visitor: &mut V,
        table: AnyNodeId,
        columns: Option<AnyNodeId>,
        source: AnyNodeId,
    ) {
        let Some(values) = self.result_column_count(source) else {
            return;
        };
        let reader = StmtReader::new(self.stmt, self.roles);
        let Some((SemanticRole::SourceRef { name, .. }, table_fields)) =
            reader.role_for_node(table)
        else {
            unreachable!("an INSERT target is a source reference");
        };
        let (table_name, _) = self
            .stmt
            .span_field_range(&table_fields, name)
            .expect("an INSERT target has a name");
        let expected = if let Some(list) = columns {
            self.stmt
                .list_children(list)
                .expect("an INSERT column list is a list")
                .len()
        } else {
            let ident = self
                .stmt
                .span_field_ident_text(&table_fields, name)
                .expect("an INSERT target has a name");
            let target = NameKey::new(&ident);
            // SQLite copies `SELECT * FROM other` between tables of the same
            // width without counting values, generated columns included.
            if let Some(other) = reader.bare_star_source(source)
                && other != target
                && self.catalog.column_count(&other).is_some()
                && self.catalog.column_count(&other) == self.catalog.column_count(&target)
            {
                return;
            }
            let Some(count) = self.catalog.insert_column_count(&target) else {
                return;
            };
            count
        };
        if values == expected {
            return;
        }
        let range = self
            .stmt
            .node_range(source)
            .expect("an INSERT source spans source");
        let ev = InsertValuesEvent {
            table: table_name,
            range,
            column_list: columns.is_some(),
            columns: expected,
            values,
        };
        visitor.on_insert_values(self.stmt, ev);
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
        bindings_idx: u8,
    ) {
        if bindings_idx == FIELD_ABSENT {
            return;
        }
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
                    name: &binding.name,
                    body_id: binding.body_id,
                };
                visitor.on_cte_binding(self.stmt, ev);
            }

            // Register the name before visiting the body so catalog source
            // lookups inside the body resolve. SQLite treats the RECURSIVE
            // keyword as decorative: any CTE may reference itself, with or
            // without it.
            if !binding.name.is_empty() {
                let cols = binding
                    .declared_cols
                    .as_ref()
                    .map(|v| v.iter().map(|(s, _)| s.to_string()).collect());
                self.catalog.add_query_table(&binding.name, cols);
            }

            self.scope.push();
            self.walk_opt(visitor, binding.body_id);
            self.scope.pop();

            if binding.name.is_empty() {
                continue;
            }

            if V::WANTS_RELATION_DEFINITION {
                visitor.on_relation_definition(&binding.name, binding.name_range);
            }

            let cols = if let Some(declared) = binding.declared_cols.as_ref() {
                if V::WANTS_CTE_COLUMN_COUNT
                    && let Some(actual) =
                        binding.body_id.and_then(|id| self.result_column_count(id))
                    && actual != declared.len()
                {
                    let ev = CteColumnCountMismatchEvent {
                        name: &binding.name,
                        name_range: binding.name_range,
                        declared: declared.len(),
                        actual,
                    };
                    visitor.on_cte_column_count_mismatch(self.stmt, ev);
                }
                if V::WANTS_COLUMN_DEFINITION {
                    for (col_name, col_range) in declared {
                        visitor.on_column_definition(&binding.name, col_name, *col_range);
                    }
                }
                Some(declared.iter().map(|(s, _)| s.to_string()).collect())
            } else {
                if V::WANTS_COLUMN_DEFINITION {
                    self.emit_select_column_definitions(visitor, binding.body_id, &binding.name);
                }
                binding.body_id.and_then(|id| self.result_columns(id))
            };
            self.catalog.add_query_table(&binding.name, cols);
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

        let (_, name_range) = reader
            .stmt()
            .span_field_range(&fields, name_idx)
            .unwrap_or_default();
        let name = reader
            .stmt()
            .span_field_ident_text(&fields, name_idx)
            .unwrap_or(Cow::Borrowed(""));
        Some(CteBindingInfo {
            name,
            name_range,
            body_id: fields.node_id_at(body_idx),
            declared_cols: reader.cte_declared_cols(cte_id),
        })
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
        reader.for_each_result_column(list_id, |child_fields, _flags, alias_idx, expr_idx| {
            let alias_node = child_fields.node_id_at(alias_idx);
            let (alias_text, alias_range) = reader.stmt().name_text(alias_node);
            if !alias_text.is_empty() {
                visitor.on_column_definition(table_name, alias_text, alias_range);
            } else if let Some((name, range)) = child_fields
                .node_id_at(expr_idx)
                .and_then(|id| reader.column_ref_name_span(id))
            {
                // An unaliased column passes its own name through.
                visitor.on_column_definition(table_name, &name, range);
            }
            true
        });
    }
}

struct CteBindingInfo<'b> {
    /// Identifier value (escape-collapsed) — used as the catalog key.
    name: Cow<'b, str>,
    name_range: DocRange,
    body_id: Option<AnyNodeId>,
    declared_cols: Option<Vec<(Cow<'b, str>, DocRange)>>,
}
