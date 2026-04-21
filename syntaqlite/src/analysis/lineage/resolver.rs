// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Recursive AST-walking lineage resolver.

use std::collections::{HashMap, HashSet};

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue};

use crate::analysis::catalog::Catalog;
use crate::analysis::ddl::{FromSource, SemanticPropertyExtractor};
use crate::dialect::SemanticRole;

use super::types::{
    ColumnLineage, ColumnOrigin, PhysicalTableAccess, QueryLineage, RelationAccess, RelationKind,
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

/// One result-column slot, captured before tracing kicks off.
enum ColumnSpec {
    Star,
    Named {
        name: String,
        expr_id: Option<AnyNodeId>,
    },
}

/// Walks the AST to compute column lineage.
pub(super) struct LineageResolver<'a, 'b> {
    sema: SemanticPropertyExtractor<'a, 'b>,
    catalog: &'a Catalog,
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
            sema: SemanticPropertyExtractor::new(stmt, roles),
            catalog,
            cte_bodies: HashMap::new(),
            tracing: HashSet::new(),
            complete: true,
        }
    }

    fn stmt(&self) -> &'a AnyParsedStatement<'b> {
        self.sema.stmt()
    }

    /// Entry point: find the outermost SELECT and resolve its lineage.
    pub(super) fn resolve(&mut self, root: AnyNodeId) -> Option<QueryLineage> {
        self.resolve_node(root)
    }

    fn resolve_node(&mut self, node_id: AnyNodeId) -> Option<QueryLineage> {
        let (role, fields) = self.sema.role_for_node(node_id)?;
        match role {
            SemanticRole::CteScope { bindings, body, .. } => {
                if let Some(bindings_id) = fields.node_id_at(bindings) {
                    self.collect_cte_bindings(bindings_id);
                }
                fields.node_id_at(body).and_then(|id| self.resolve_node(id))
            }
            SemanticRole::Query { .. } => self.resolve_select(node_id),
            SemanticRole::DefineTable { select, .. }
            | SemanticRole::DefineView { select, .. }
            | SemanticRole::DefineFunction { select, .. } => fields
                .node_id_at(select)
                .and_then(|id| self.resolve_node(id)),
            SemanticRole::Transparent | SemanticRole::DmlScope { .. } => {
                for child_id in self.stmt().child_node_ids(node_id) {
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
        // Collect into a temp Vec so the iterator borrow on self.sema doesn't
        // conflict with the &mut self.cte_bodies write.
        let mut entries: Vec<(String, AnyNodeId)> = Vec::new();
        self.sema.for_each_cte_binding(bindings_id, |binding| {
            if binding.name.is_empty() {
                return;
            }
            if let Some(body_id) = binding.body_id {
                entries.push((binding.name.to_ascii_lowercase(), body_id));
            }
        });
        for (name, body_id) in entries {
            self.cte_bodies.insert(name, body_id);
        }
    }

    // ── SELECT resolution ────────────────────────────────────────────────

    fn resolve_select(&mut self, select_id: AnyNodeId) -> Option<QueryLineage> {
        let (
            SemanticRole::Query {
                from,
                columns: cols_idx,
                ..
            },
            fields,
        ) = self.sema.role_for_node(select_id)?
        else {
            return None;
        };

        // 1. Walk FROM to build the source map.
        let sources = match fields.node_id_at(from) {
            Some(from_id) => self.collect_sources(from_id),
            None => HashMap::new(),
        };

        // 2. Resolve result columns.
        let columns = self.resolve_result_columns(cols_idx, &fields, &sources)?;

        // 3. Build relations (catalog only) and physical_tables (transitive).
        let mut relations = Vec::new();
        let mut physical_tables = Vec::new();
        let mut unexpanded_views = Vec::new();
        let source_snapshot: Vec<SourceInfo> = sources.values().cloned().collect();
        for info in &source_snapshot {
            self.flatten_source(
                info,
                &mut relations,
                &mut physical_tables,
                &mut unexpanded_views,
            );
        }

        relations.sort_by(|a, b| a.name.cmp(&b.name));
        relations.dedup_by(|a, b| a.name == b.name);
        physical_tables.sort_by(|a, b| a.name.cmp(&b.name));
        physical_tables.dedup_by(|a, b| a.name == b.name);
        unexpanded_views.sort();
        unexpanded_views.dedup();

        Some(QueryLineage {
            complete: self.complete,
            columns,
            relations,
            physical_tables,
            unexpanded_views,
        })
    }

    /// Flatten one source into `relations`/`physical_tables`/`unexpanded_views`.
    /// CTE/subquery bodies are recursively traced.
    fn flatten_source(
        &mut self,
        info: &SourceInfo,
        relations: &mut Vec<RelationAccess>,
        physical_tables: &mut Vec<PhysicalTableAccess>,
        unexpanded_views: &mut Vec<String>,
    ) {
        match info.kind {
            SourceKind::Table => {
                relations.push(RelationAccess {
                    name: info.canonical.clone(),
                    kind: RelationKind::Table,
                });
                physical_tables.push(PhysicalTableAccess {
                    name: info.canonical.clone(),
                });
            }
            SourceKind::View => {
                relations.push(RelationAccess {
                    name: info.canonical.clone(),
                    kind: RelationKind::View,
                });
                physical_tables.push(PhysicalTableAccess {
                    name: info.canonical.clone(),
                });
                unexpanded_views.push(info.canonical.clone());
                self.complete = false;
            }
            SourceKind::Cte(body) | SourceKind::Subquery(body) => {
                if self.tracing.insert(body) {
                    self.collect_physical_tables(
                        body,
                        relations,
                        physical_tables,
                        unexpanded_views,
                    );
                    self.tracing.remove(&body);
                }
            }
        }
    }

    // ── FROM source collection ───────────────────────────────────────────

    /// Walk a FROM clause and return a map of display-name → `SourceInfo`.
    /// Uses [`SemanticPropertyExtractor::for_each_from_source`] so the
    /// SourceRef/ScopedSource role-dispatch is shared with the analyzer.
    fn collect_sources(&self, from_id: AnyNodeId) -> HashMap<String, SourceInfo> {
        let mut target: HashMap<String, SourceInfo> = HashMap::new();
        self.sema
            .for_each_from_source(from_id, |source| match source {
                FromSource::Relation { name, alias, .. } => {
                    let display = alias.unwrap_or(name).to_ascii_lowercase();
                    let canonical = name.to_ascii_lowercase();
                    let (columns, kind) = if let Some(&body) = self.cte_bodies.get(&canonical) {
                        (self.sema.columns_from_select(body), SourceKind::Cte(body))
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
                FromSource::Subquery { alias, body_id, .. } => {
                    let Some(alias_text) = alias else {
                        return;
                    };
                    let alias_lower = alias_text.to_ascii_lowercase();
                    let cols = self.sema.columns_from_select(body_id);
                    target.insert(
                        alias_lower.clone(),
                        SourceInfo {
                            canonical: alias_lower,
                            columns: cols,
                            kind: SourceKind::Subquery(body_id),
                        },
                    );
                }
            });
        target
    }

    /// Recursively collect physical tables and catalog relations from a CTE/subquery body.
    fn collect_physical_tables(
        &mut self,
        body_id: AnyNodeId,
        relations: &mut Vec<RelationAccess>,
        physical_tables: &mut Vec<PhysicalTableAccess>,
        unexpanded_views: &mut Vec<String>,
    ) {
        let Some(select_id) = self.find_select_node(body_id) else {
            return;
        };
        let Some((SemanticRole::Query { from, .. }, fields)) = self.sema.role_for_node(select_id)
        else {
            return;
        };
        let Some(from_id) = fields.node_id_at(from) else {
            return;
        };

        let inner_sources = self.collect_sources(from_id);
        let snapshot: Vec<SourceInfo> = inner_sources.into_values().collect();
        for info in &snapshot {
            self.flatten_source(info, relations, physical_tables, unexpanded_views);
        }
    }

    // ── Result column resolution ─────────────────────────────────────────

    fn resolve_result_columns(
        &mut self,
        cols_idx: u8,
        select_fields: &syntaqlite_syntax::any::NodeFields,
        sources: &HashMap<String, SourceInfo>,
    ) -> Option<Vec<ColumnLineage>> {
        let list_id = select_fields.node_id_at(cols_idx)?;
        let specs = self.collect_column_specs(list_id);

        let mut result = Vec::new();
        let mut index: u32 = 0;

        for spec in specs {
            match spec {
                ColumnSpec::Star => {
                    for info in sources.values() {
                        let Some(cols) = info.columns.clone() else {
                            continue;
                        };
                        let canonical = info.canonical.clone();
                        for col in cols {
                            let origin = self.trace_column(&canonical, &col, sources);
                            result.push(ColumnLineage {
                                name: col.to_ascii_lowercase(),
                                index,
                                origin,
                            });
                            index += 1;
                        }
                    }
                }
                ColumnSpec::Named { name, expr_id } => {
                    let origin = expr_id.and_then(|id| self.trace_expr_origin(id, sources));
                    result.push(ColumnLineage {
                        name,
                        index,
                        origin,
                    });
                    index += 1;
                }
            }
        }

        Some(result)
    }

    /// Snapshot result-column specs (Star vs. Named) so the borrow on
    /// `self.sema` from iteration doesn't conflict with `&mut self` used by
    /// the trace_* methods.
    fn collect_column_specs(&self, list_id: AnyNodeId) -> Vec<ColumnSpec> {
        let mut specs = Vec::new();
        let sema = self.sema;
        sema.for_each_result_column(list_id, |fields, flags_idx, alias_idx, expr_idx| {
            let is_star = matches!(
                fields[flags_idx as usize],
                FieldValue::Flags(f) if f & 1 != 0
            );
            if is_star {
                specs.push(ColumnSpec::Star);
            } else {
                let name = sema
                    .infer_result_col_name(fields, alias_idx, expr_idx)
                    .unwrap_or_default();
                let expr_id = fields.node_id_at(expr_idx);
                specs.push(ColumnSpec::Named { name, expr_id });
            }
            true
        });
        specs
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
        let (
            SemanticRole::Query {
                from,
                columns: cols_idx,
                ..
            },
            fields,
        ) = self.sema.role_for_node(select_id)?
        else {
            return None;
        };

        // Build inner source map for this body.
        let inner_sources = match fields.node_id_at(from) {
            Some(from_id) => self.collect_sources(from_id),
            None => HashMap::new(),
        };

        let list_id = fields.node_id_at(cols_idx)?;

        // Find the matching result column's expression node.
        let mut found_expr_id: Option<AnyNodeId> = None;
        let target = col_name.to_ascii_lowercase();
        let sema = self.sema;
        sema.for_each_result_column(list_id, |child_fields, _flags, alias_idx, expr_idx| {
            let name = sema.infer_result_col_name(child_fields, alias_idx, expr_idx);
            if name
                .as_deref()
                .is_some_and(|n: &str| n.eq_ignore_ascii_case(&target))
            {
                found_expr_id = child_fields.node_id_at(expr_idx);
                return false;
            }
            true
        });

        let expr_id = found_expr_id?;
        self.trace_expr_origin(expr_id, &inner_sources)
    }

    fn trace_expr_origin(
        &mut self,
        expr_id: AnyNodeId,
        sources: &HashMap<String, SourceInfo>,
    ) -> Option<ColumnOrigin> {
        let (
            SemanticRole::ColumnRef {
                column: col_idx,
                table: tbl_idx,
            },
            expr_fields,
        ) = self.sema.role_for_node(expr_id)?
        else {
            return None;
        };

        let stmt = self.stmt();
        let col_name = stmt
            .span_field_text(&expr_fields, col_idx)?
            .to_ascii_lowercase();
        let source_name = match stmt.span_field_text(&expr_fields, tbl_idx) {
            Some(t) => t.to_ascii_lowercase(),
            None => find_source_for_column(sources, &col_name)?,
        };

        self.trace_column(&source_name, &col_name, sources)
    }

    // ── AST navigation helpers ───────────────────────────────────────────

    fn find_select_node(&self, node_id: AnyNodeId) -> Option<AnyNodeId> {
        let (role, fields) = self.sema.role_for_node(node_id)?;
        match role {
            SemanticRole::Query { .. } => Some(node_id),
            SemanticRole::CteScope { body, .. } => fields
                .node_id_at(body)
                .and_then(|id| self.find_select_node(id)),
            SemanticRole::Transparent => {
                for child_id in self.stmt().child_node_ids(node_id) {
                    if let Some(result) = self.find_select_node(child_id) {
                        return Some(result);
                    }
                }
                None
            }
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
