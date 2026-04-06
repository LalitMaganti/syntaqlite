// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Recursive AST-walking lineage resolver.

use std::collections::{HashMap, HashSet};

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue};

use crate::dialect::SemanticRole;
use crate::semantic::catalog::Catalog;

use super::types::{
    ColumnLineage, ColumnOrigin, QueryLineage, RelationAccess, RelationKind, TableAccess,
};

/// What kind of FROM source this is.
#[derive(Debug, Clone)]
enum SourceKind {
    /// A CTE — body node is stored for transitive tracing.
    Cte(AnyNodeId),
    /// A subquery — body node is stored for transitive tracing.
    Subquery(AnyNodeId),
    /// A physical table in the catalog.
    Table,
    /// A view in the catalog.
    View,
}

/// Tracks information about a source in the FROM clause.
#[derive(Debug, Clone)]
struct SourceInfo {
    /// Canonical relation name (before aliasing).
    canonical: String,
    /// Known columns for this source (None = unknown).
    columns: Option<Vec<String>>,
    /// What kind of source this is.
    kind: SourceKind,
}

/// Walks the AST to compute column lineage.
pub(super) struct LineageResolver<'a, 'b> {
    stmt: &'a AnyParsedStatement<'b>,
    catalog: &'a Catalog,
    roles: &'a [SemanticRole],
    /// CTE name -> body node ID (from WITH clause, before FROM is walked).
    cte_bodies: HashMap<String, AnyNodeId>,
    /// Body nodes currently being traced (cycle detection for recursive CTEs).
    tracing: HashSet<AnyNodeId>,
    /// Whether all sources were fully resolved.
    complete: bool,
}

impl<'a, 'b> LineageResolver<'a, 'b> {
    pub(super) fn new(
        stmt: &'a AnyParsedStatement<'b>,
        catalog: &'a Catalog,
        roles: &'a [SemanticRole],
    ) -> Self {
        Self {
            stmt,
            catalog,
            roles,
            cte_bodies: HashMap::new(),
            tracing: HashSet::new(),
            complete: true,
        }
    }

    /// Entry point: find the outermost SELECT and resolve its lineage.
    pub(super) fn resolve(&mut self, root: AnyNodeId) -> Option<QueryLineage> {
        self.resolve_node(root)
    }

    fn resolve_node(&mut self, node_id: AnyNodeId) -> Option<QueryLineage> {
        if node_id.is_null() {
            return None;
        }
        let (tag, fields) = self.stmt.extract_fields(node_id)?;
        let role = self.role_for(tag);

        match role {
            SemanticRole::CteScope { bindings, body, .. } => {
                if let FieldValue::NodeId(bindings_id) = fields[bindings as usize] {
                    self.collect_cte_bindings(bindings_id);
                }
                if let FieldValue::NodeId(body_id) = fields[body as usize] {
                    return self.resolve_node(body_id);
                }
                None
            }
            SemanticRole::Query { .. } => self.resolve_select(node_id),
            SemanticRole::Transparent | SemanticRole::DmlScope => {
                for child_id in self.stmt.child_node_ids(node_id) {
                    if let Some(result) = self.resolve_node(child_id) {
                        return Some(result);
                    }
                }
                None
            }
            _ => None,
        }
    }

    fn collect_cte_bindings(&mut self, bindings_id: AnyNodeId) {
        if bindings_id.is_null() {
            return;
        }
        let Some(children) = self.stmt.list_children(bindings_id) else {
            self.try_register_cte(bindings_id);
            return;
        };
        for &child_id in children {
            self.try_register_cte(child_id);
        }
    }

    fn try_register_cte(&mut self, node_id: AnyNodeId) {
        if node_id.is_null() {
            return;
        }
        let Some((tag, fields)) = self.stmt.extract_fields(node_id) else {
            return;
        };
        let role = self.role_for(tag);

        if let SemanticRole::CteBinding { name, body, .. } = role {
            let cte_name = match fields[name as usize] {
                FieldValue::Span { text: s, .. } if !s.is_empty() => Some(s.to_ascii_lowercase()),
                FieldValue::NodeId(id) if !id.is_null() => {
                    self.span_text(id).map(|s| s.to_ascii_lowercase())
                }
                _ => None,
            };
            if let Some(cte_name) = cte_name
                && let FieldValue::NodeId(body_id) = fields[body as usize]
            {
                self.cte_bodies.insert(cte_name, body_id);
            }
        }
    }

    // ── SELECT resolution ────────────────────────────────────────────────

    fn resolve_select(&mut self, select_id: AnyNodeId) -> Option<QueryLineage> {
        let (tag, fields) = self.stmt.extract_fields(select_id)?;
        let role = self.role_for(tag);

        let SemanticRole::Query {
            from,
            columns: cols_idx,
            ..
        } = role
        else {
            return None;
        };

        // 1. Walk FROM to build the source map.
        let mut sources = HashMap::new();
        if let FieldValue::NodeId(from_id) = fields[from as usize] {
            self.collect_sources(from_id, &mut sources);
        }

        // 2. Resolve result columns.
        let columns = self.resolve_result_columns(select_id, cols_idx, &sources)?;

        // 3. Build relations (catalog only) and tables (physical, transitive).
        let mut relations = Vec::new();
        let mut tables = Vec::new();
        for info in sources.values() {
            match info.kind {
                SourceKind::Table => {
                    relations.push(RelationAccess {
                        name: info.canonical.clone(),
                        kind: RelationKind::Table,
                    });
                    tables.push(TableAccess {
                        name: info.canonical.clone(),
                    });
                }
                SourceKind::View => {
                    relations.push(RelationAccess {
                        name: info.canonical.clone(),
                        kind: RelationKind::View,
                    });
                    tables.push(TableAccess {
                        name: info.canonical.clone(),
                    });
                    self.complete = false;
                }
                SourceKind::Cte(body) | SourceKind::Subquery(body) => {
                    if self.tracing.insert(body) {
                        self.collect_physical_tables(body, &mut relations, &mut tables);
                        self.tracing.remove(&body);
                    }
                }
            }
        }

        relations.sort_by(|a, b| a.name.cmp(&b.name));
        relations.dedup_by(|a, b| a.name == b.name);
        tables.sort_by(|a, b| a.name.cmp(&b.name));
        tables.dedup_by(|a, b| a.name == b.name);

        Some(QueryLineage {
            complete: self.complete,
            columns,
            relations,
            tables,
        })
    }

    // ── FROM source collection ───────────────────────────────────────────

    /// Walk a FROM clause and populate `target` with source entries.
    ///
    /// Used both for the outer query and when tracing through CTE/subquery bodies.
    fn collect_sources(&self, from_id: AnyNodeId, target: &mut HashMap<String, SourceInfo>) {
        if from_id.is_null() {
            return;
        }
        let Some((tag, fields)) = self.stmt.extract_fields(from_id) else {
            return;
        };
        let role = self.role_for(tag);

        match role {
            SemanticRole::SourceRef { name, alias, .. } => {
                let Some(ref sn) = self.span_text_from_field(&fields, name) else {
                    return;
                };
                let alias_name = self.span_text_from_field(&fields, alias);
                let display = alias_name.as_ref().unwrap_or(sn).to_ascii_lowercase();
                let canonical = sn.to_ascii_lowercase();

                let (columns, kind) = if let Some(&body) = self.cte_bodies.get(&canonical) {
                    let cols =
                        super::super::catalog::columns_from_select(self.stmt, body, self.roles);
                    (cols, SourceKind::Cte(body))
                } else {
                    let (cols, _) = self.catalog.table_source_info(&canonical);
                    let kind = if self.catalog.is_view(&canonical) {
                        SourceKind::View
                    } else {
                        SourceKind::Table
                    };
                    (cols, kind)
                };

                target.insert(
                    display,
                    SourceInfo {
                        canonical,
                        columns,
                        kind,
                    },
                );
            }
            SemanticRole::ScopedSource { body, alias } => {
                if let Some(alias_text) = self.span_text_from_field(&fields, alias)
                    && let FieldValue::NodeId(body_id) = fields[body as usize]
                {
                    let alias_lower = alias_text.to_ascii_lowercase();
                    let cols =
                        super::super::catalog::columns_from_select(self.stmt, body_id, self.roles);
                    target.insert(
                        alias_lower.clone(),
                        SourceInfo {
                            canonical: alias_lower,
                            columns: cols,
                            kind: SourceKind::Subquery(body_id),
                        },
                    );
                }
            }
            _ => {
                for child_id in self.stmt.child_node_ids(from_id) {
                    self.collect_sources(child_id, target);
                }
            }
        }
    }

    /// Recursively collect physical tables and catalog relations from a CTE/subquery body.
    fn collect_physical_tables(
        &mut self,
        body_id: AnyNodeId,
        relations: &mut Vec<RelationAccess>,
        tables: &mut Vec<TableAccess>,
    ) {
        let Some(select_id) = self.find_select_node(body_id) else {
            return;
        };
        let Some((tag, fields)) = self.stmt.extract_fields(select_id) else {
            return;
        };
        let SemanticRole::Query { from, .. } = self.role_for(tag) else {
            return;
        };
        let FieldValue::NodeId(from_id) = fields[from as usize] else {
            return;
        };

        let mut inner_sources = HashMap::new();
        self.collect_sources(from_id, &mut inner_sources);
        for info in inner_sources.values() {
            match info.kind {
                SourceKind::Table => {
                    relations.push(RelationAccess {
                        name: info.canonical.clone(),
                        kind: RelationKind::Table,
                    });
                    tables.push(TableAccess {
                        name: info.canonical.clone(),
                    });
                }
                SourceKind::View => {
                    relations.push(RelationAccess {
                        name: info.canonical.clone(),
                        kind: RelationKind::View,
                    });
                    tables.push(TableAccess {
                        name: info.canonical.clone(),
                    });
                    self.complete = false;
                }
                SourceKind::Cte(body) | SourceKind::Subquery(body) => {
                    if self.tracing.insert(body) {
                        self.collect_physical_tables(body, relations, tables);
                        self.tracing.remove(&body);
                    }
                }
            }
        }
    }

    // ── Result column resolution ─────────────────────────────────────────

    fn resolve_result_columns(
        &mut self,
        select_id: AnyNodeId,
        cols_idx: u8,
        sources: &HashMap<String, SourceInfo>,
    ) -> Option<Vec<ColumnLineage>> {
        let (_, fields) = self.stmt.extract_fields(select_id)?;
        let FieldValue::NodeId(list_id) = fields[cols_idx as usize] else {
            return None;
        };
        if list_id.is_null() {
            return None;
        }

        let children = self.stmt.list_children(list_id)?;
        let mut result = Vec::new();
        let mut index: u32 = 0;

        for &child_id in children {
            if child_id.is_null() {
                continue;
            }
            let Some((child_tag, child_fields)) = self.stmt.extract_fields(child_id) else {
                continue;
            };
            let child_role = self.role_for(child_tag);

            let SemanticRole::ResultColumn {
                flags: flags_idx,
                alias: alias_idx,
                expr: expr_idx,
            } = child_role
            else {
                continue;
            };

            // Check for STAR (SELECT *).
            if let FieldValue::Flags(f) = child_fields[flags_idx as usize]
                && f & 1 != 0
            {
                for info in sources.values() {
                    if let Some(cols) = &info.columns {
                        for col in cols {
                            let origin = self.trace_column(&info.canonical, col, sources);
                            result.push(ColumnLineage {
                                name: col.to_ascii_lowercase(),
                                index,
                                origin,
                            });
                            index += 1;
                        }
                    }
                }
                continue;
            }

            let col_name = self.infer_column_name(&child_fields, alias_idx, expr_idx);
            let origin = self.trace_expr_origin(&child_fields, expr_idx, sources);

            result.push(ColumnLineage {
                name: col_name.unwrap_or_default(),
                index,
                origin,
            });
            index += 1;
        }

        Some(result)
    }

    // ── Column tracing ───────────────────────────────────────────────────

    /// Trace a column reference to its physical table origin.
    fn trace_column(
        &mut self,
        source_name: &str,
        col_name: &str,
        sources: &HashMap<String, SourceInfo>,
    ) -> Option<ColumnOrigin> {
        let source_lower = source_name.to_ascii_lowercase();

        // Look up the source to find its canonical name and kind.
        let info = sources.get(&source_lower).or_else(|| {
            // Try finding by canonical name (handles aliases).
            sources.values().find(|i| i.canonical == source_lower)
        })?;

        match info.kind {
            SourceKind::Cte(body) | SourceKind::Subquery(body) => {
                self.trace_through_select(body, col_name)
            }
            SourceKind::Table => Some(ColumnOrigin {
                table: info.canonical.clone(),
                column: col_name.to_ascii_lowercase(),
            }),
            SourceKind::View => None,
        }
    }

    /// Trace a column through a CTE/subquery body to its physical origin.
    fn trace_through_select(&mut self, body_id: AnyNodeId, col_name: &str) -> Option<ColumnOrigin> {
        if !self.tracing.insert(body_id) {
            return None; // Cycle (recursive CTE) — stop.
        }
        let result = self.trace_through_select_inner(body_id, col_name);
        self.tracing.remove(&body_id);
        result
    }

    fn trace_through_select_inner(
        &mut self,
        body_id: AnyNodeId,
        col_name: &str,
    ) -> Option<ColumnOrigin> {
        let select_id = self.find_select_node(body_id)?;
        let (tag, fields) = self.stmt.extract_fields(select_id)?;
        let SemanticRole::Query {
            from,
            columns: cols_idx,
            ..
        } = self.role_for(tag)
        else {
            return None;
        };

        // Build inner source map for this body.
        let mut inner_sources = HashMap::new();
        if let FieldValue::NodeId(from_id) = fields[from as usize] {
            self.collect_sources(from_id, &mut inner_sources);
        }

        // Find the matching result column and trace it.
        let FieldValue::NodeId(list_id) = fields[cols_idx as usize] else {
            return None;
        };
        if list_id.is_null() {
            return None;
        }

        let children = self.stmt.list_children(list_id)?;
        for &child_id in children {
            if child_id.is_null() {
                continue;
            }
            let (child_tag, child_fields) = self.stmt.extract_fields(child_id)?;
            let SemanticRole::ResultColumn {
                alias: alias_idx,
                expr: expr_idx,
                ..
            } = self.role_for(child_tag)
            else {
                continue;
            };

            let name = self.infer_column_name(&child_fields, alias_idx, expr_idx)?;
            if !name.eq_ignore_ascii_case(col_name) {
                continue;
            }

            // Found matching column — trace its expression.
            return self.trace_expr_origin(&child_fields, expr_idx, &inner_sources);
        }

        None
    }

    fn trace_expr_origin(
        &mut self,
        child_fields: &syntaqlite_syntax::any::NodeFields<'_>,
        expr_idx: u8,
        sources: &HashMap<String, SourceInfo>,
    ) -> Option<ColumnOrigin> {
        let FieldValue::NodeId(expr_id) = child_fields[expr_idx as usize] else {
            return None;
        };
        if expr_id.is_null() {
            return None;
        }

        let (expr_tag, expr_fields) = self.stmt.extract_fields(expr_id)?;
        let SemanticRole::ColumnRef {
            column: col_idx,
            table: tbl_idx,
        } = self.role_for(expr_tag)
        else {
            return None;
        };

        let col_name = match expr_fields[col_idx as usize] {
            FieldValue::Span { text: s, .. } if !s.is_empty() => s.to_ascii_lowercase(),
            _ => return None,
        };

        let source_name = if let FieldValue::Span { text: s, .. } = expr_fields[tbl_idx as usize]
            && !s.is_empty()
        {
            s.to_ascii_lowercase()
        } else {
            find_source_for_column(sources, &col_name)?
        };

        self.trace_column(&source_name, &col_name, sources)
    }

    // ── AST navigation helpers ───────────────────────────────────────────

    fn find_select_node(&self, node_id: AnyNodeId) -> Option<AnyNodeId> {
        if node_id.is_null() {
            return None;
        }
        let (tag, fields) = self.stmt.extract_fields(node_id)?;
        match self.role_for(tag) {
            SemanticRole::Query { .. } => Some(node_id),
            SemanticRole::CteScope { body, .. } => {
                if let FieldValue::NodeId(body_id) = fields[body as usize] {
                    self.find_select_node(body_id)
                } else {
                    None
                }
            }
            SemanticRole::Transparent => {
                for child_id in self.stmt.child_node_ids(node_id) {
                    if let Some(result) = self.find_select_node(child_id) {
                        return Some(result);
                    }
                }
                None
            }
            _ => None,
        }
    }

    fn infer_column_name(
        &self,
        child_fields: &syntaqlite_syntax::any::NodeFields<'_>,
        alias_idx: u8,
        expr_idx: u8,
    ) -> Option<String> {
        // Try alias first.
        if let FieldValue::NodeId(alias_id) = child_fields[alias_idx as usize]
            && !alias_id.is_null()
            && let Some((_, alias_fields)) = self.stmt.extract_fields(alias_id)
        {
            for j in 0..alias_fields.len() {
                if let FieldValue::Span { text: s, .. } = alias_fields[j]
                    && !s.is_empty()
                {
                    return Some(s.to_ascii_lowercase());
                }
            }
        }

        // Try bare column ref.
        if let FieldValue::NodeId(expr_id) = child_fields[expr_idx as usize]
            && !expr_id.is_null()
            && let Some((expr_tag, expr_fields)) = self.stmt.extract_fields(expr_id)
            && let SemanticRole::ColumnRef {
                column: col_idx, ..
            } = self.role_for(expr_tag)
            && let FieldValue::Span { text: col_span, .. } = expr_fields[col_idx as usize]
            && !col_span.is_empty()
        {
            return Some(col_span.to_ascii_lowercase());
        }

        // Fallback: use expression source text.
        if let FieldValue::NodeId(expr_id) = child_fields[expr_idx as usize]
            && !expr_id.is_null()
        {
            return super::super::catalog::expr_source_text(self.stmt, expr_id)
                .map(str::to_ascii_lowercase);
        }

        None
    }

    fn role_for(&self, tag: syntaqlite_syntax::any::AnyNodeTag) -> SemanticRole {
        self.roles
            .get(u32::from(tag) as usize)
            .copied()
            .unwrap_or(SemanticRole::Transparent)
    }

    fn span_text(&self, node_id: AnyNodeId) -> Option<String> {
        if node_id.is_null() {
            return None;
        }
        let (_, fields) = self.stmt.extract_fields(node_id)?;
        for i in 0..fields.len() {
            if let FieldValue::Span { text: s, .. } = fields[i]
                && !s.is_empty()
            {
                return Some(s.to_owned());
            }
        }
        None
    }

    fn span_text_from_field(
        &self,
        fields: &syntaqlite_syntax::any::NodeFields<'_>,
        field_idx: u8,
    ) -> Option<String> {
        if field_idx == crate::dialect::FIELD_ABSENT {
            return None;
        }
        match fields[field_idx as usize] {
            FieldValue::Span { text: s, .. } if !s.is_empty() => Some(s.to_owned()),
            FieldValue::NodeId(id) if !id.is_null() => self.span_text(id),
            _ => None,
        }
    }
}

fn find_source_for_column(sources: &HashMap<String, SourceInfo>, col_name: &str) -> Option<String> {
    let col_lower = col_name.to_ascii_lowercase();
    for (source_name, info) in sources {
        if let Some(cols) = &info.columns
            && cols.iter().any(|c| c.eq_ignore_ascii_case(&col_lower))
        {
            return Some(source_name.clone());
        }
    }
    sources.keys().next().cloned()
}
