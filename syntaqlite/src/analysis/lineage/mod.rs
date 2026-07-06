// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Column lineage analysis, computed bottom-up during the analysis walk.
//!
//! - [`LineageCapture`] is a [`SemanticVisitor`] that tracks an in-progress
//!   Query stack, captures outer-WITH CTE bindings, and finalizes each
//!   Query's lineage at `exit_query` using that Query's FROM sources plus
//!   any nested Query's already-finalized summary.
//! - [`Finalizer`] is a short-lived helper constructed per `exit_query`.
//!   It owns the references finalization needs (captured maps, AST access)
//!   and holds the per-column / per-source trace logic as methods.
//! - [`build_lineage`] is a map lookup — no post-walk trace.

use std::collections::HashMap;

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue, NodeFields};

use super::catalog::Catalog;
use super::engine::walker::{
    CteBindingEvent, ScopedSourceEvent, SemanticVisitor, SourceRefEvent, WalkCtx,
};
use super::name_key::NameKey;
use super::stmt_reader::StmtReader;
use crate::dialect::SemanticRole;

mod types;

pub(crate) use types::QueryLineage;
pub use types::{
    ColumnLineage, ColumnOrigin, LineageResult, PhysicalTableAccess, RelationAccess, RelationKind,
};

// ── Finalized per-Query state ─────────────────────────────────────────────────

/// Lineage for a single `Query` node, finalized at `exit_query` time.
#[derive(Debug, Clone)]
struct QuerySummary {
    /// Output column names — enclosing Queries use these when this
    /// Query appears as a CTE/subquery source.
    column_names: Vec<String>,
    lineage: QueryLineage,
}

// ── In-progress per-Query state ───────────────────────────────────────────────

struct InProgressQuery {
    node_id: AnyNodeId,
    sources: HashMap<String, SourceInfo>,
}

#[derive(Debug, Clone)]
struct SourceInfo {
    /// Canonical relation name (lowercase).
    canonical: String,
    /// Known columns for this source, when inferable.
    columns: Option<Vec<String>>,
    kind: SourceKind,
}

#[derive(Debug, Clone, Copy)]
enum SourceKind {
    Table,
    View,
    /// A CTE body — body node is used to chase the nested summary.
    Cte(AnyNodeId),
    /// A subquery-in-FROM — same handling as CTE for lineage.
    Subquery(AnyNodeId),
}

// ── LineageCapture ────────────────────────────────────────────────────────────

pub(crate) struct LineageCapture {
    roles: &'static [SemanticRole],
    /// Outer-WITH CTE bindings by lowercase name. Populated from
    /// `on_cte_binding` events fired at stack depth 0 (matches legacy
    /// scope — nested WITH clauses are deliberately excluded).
    cte_bodies: HashMap<String, AnyNodeId>,
    /// Finalized summaries keyed by Query node id.
    per_query: HashMap<AnyNodeId, QuerySummary>,
    /// Stack of in-progress Queries; innermost on top.
    stack: Vec<InProgressQuery>,
    /// The outermost statement-level Query. Set from the first
    /// `enter_query` at stack depth 0 that isn't a registered CTE body.
    /// `None` when the statement isn't a query.
    outer_query: Option<AnyNodeId>,
}

impl LineageCapture {
    pub(crate) fn new(roles: &'static [SemanticRole]) -> Self {
        Self {
            roles,
            cte_bodies: HashMap::new(),
            per_query: HashMap::new(),
            stack: Vec::new(),
            outer_query: None,
        }
    }

    /// Classify a catalog/CTE relation source and return its kind +
    /// known columns. For CTEs, prefers the finalized per-Query
    /// summary's column names; falls back to the catalog's declared
    /// columns when the body hasn't been finalized yet (recursive
    /// self-reference).
    fn classify(&self, canonical: &str, catalog: &Catalog) -> (SourceKind, Option<Vec<String>>) {
        let key = NameKey::new(canonical);
        if let Some(&body) = self.cte_bodies.get(canonical) {
            let cols = self
                .per_query
                .get(&body)
                .map(|s| s.column_names.clone())
                .or_else(|| catalog.table_source_info(&key).0);
            (SourceKind::Cte(body), cols)
        } else {
            let (cols, _) = catalog.table_source_info(&key);
            let kind = if catalog.is_view(&key) {
                SourceKind::View
            } else {
                SourceKind::Table
            };
            (kind, cols)
        }
    }
}

impl SemanticVisitor for LineageCapture {
    const WANTS_SOURCE_REF: bool = true;
    const WANTS_CTE_BINDING: bool = true;
    const WANTS_SCOPED_SOURCE: bool = true;
    const WANTS_QUERY: bool = true;

    fn enter_query(&mut self, _stmt: &mut AnyParsedStatement<'_>, node_id: AnyNodeId) {
        // Track the first statement-level Query. A WITH clause walks its
        // CTE bodies (each a Query at stack depth 0) before the main
        // body Query, so skip those — `on_cte_binding` has already
        // registered their ids in `cte_bodies`.
        let is_cte_body = self.cte_bodies.values().any(|id| *id == node_id);
        if self.outer_query.is_none() && self.stack.is_empty() && !is_cte_body {
            self.outer_query = Some(node_id);
        }
        self.stack.push(InProgressQuery {
            node_id,
            sources: HashMap::new(),
        });
    }

    fn exit_query(&mut self, stmt: &mut AnyParsedStatement<'_>, _node_id: AnyNodeId) {
        // The walker pairs enter/exit; an exit without a matching enter
        // is a walker bug, not a data-shape concern.
        let Some(frame) = self.stack.pop() else {
            return;
        };
        let summary = Finalizer {
            roles: self.roles,
            stmt,
            per_query: &self.per_query,
            sources: &frame.sources,
        }
        .finalize(frame.node_id);
        self.per_query.insert(frame.node_id, summary);
    }

    fn on_cte_binding(&mut self, _stmt: &mut AnyParsedStatement<'_>, ev: CteBindingEvent<'_>) {
        // Only outer-WITH bindings (depth 0) populate the flat map —
        // nested CTEs have their own scope that the flat map would
        // otherwise leak across.
        if !self.stack.is_empty() {
            return;
        }
        if let Some(body_id) = ev.body_id
            && !ev.name.is_empty()
        {
            self.cte_bodies
                .insert(ev.name.to_ascii_lowercase(), body_id);
        }
    }

    fn on_source_ref(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: SourceRefEvent<'_>,
    ) {
        let canonical = ev.name.to_ascii_lowercase();
        let display = ev.alias.unwrap_or(ev.name).to_ascii_lowercase();
        let (kind, columns) = self.classify(&canonical, cx.catalog);
        if let Some(top) = self.stack.last_mut() {
            top.sources.insert(
                display,
                SourceInfo {
                    canonical,
                    columns,
                    kind,
                },
            );
        }
    }

    fn on_scoped_source(&mut self, _stmt: &mut AnyParsedStatement<'_>, ev: ScopedSourceEvent<'_>) {
        let Some(body_id) = ev.body_id else {
            return;
        };
        // Anonymous subqueries don't bind a name for ColumnRef
        // resolution, so no source-map entry.
        let Some(alias) = ev.alias else {
            return;
        };
        let columns = self.per_query.get(&body_id).map(|s| s.column_names.clone());
        if let Some(top) = self.stack.last_mut() {
            let alias_lower = alias.to_ascii_lowercase();
            top.sources.insert(
                alias_lower.clone(),
                SourceInfo {
                    canonical: alias_lower,
                    columns,
                    kind: SourceKind::Subquery(body_id),
                },
            );
        }
    }
}

// ── Finalizer ─────────────────────────────────────────────────────────────────

/// Short-lived helper that runs the trace for one Query. Constructed
/// fresh inside `exit_query`; holds references to the already-finalized
/// nested summaries plus this Query's captured sources.
struct Finalizer<'a, 'b> {
    roles: &'static [SemanticRole],
    stmt: &'a AnyParsedStatement<'b>,
    per_query: &'a HashMap<AnyNodeId, QuerySummary>,
    sources: &'a HashMap<String, SourceInfo>,
}

impl<'a, 'b> Finalizer<'a, 'b> {
    fn sema(&self) -> StmtReader<'a, 'b> {
        StmtReader::new(self.stmt, self.roles)
    }

    fn finalize(&self, query: AnyNodeId) -> QuerySummary {
        let (columns, column_names, cols_complete) = self.trace_result_columns(query);
        let (relations, physical_tables, unexpanded_views, sources_complete) =
            self.aggregate_sources();
        QuerySummary {
            column_names,
            lineage: QueryLineage {
                complete: cols_complete && sources_complete,
                columns,
                relations,
                physical_tables,
                unexpanded_views,
            },
        }
    }

    // ── Result-column trace ───────────────────────────────────────────────────

    /// Returns `(columns, output_names, complete)` for `query`.
    /// `complete=false` when a `*` expansion hit a source with unknown
    /// columns.
    fn trace_result_columns(&self, query: AnyNodeId) -> (Vec<ColumnLineage>, Vec<String>, bool) {
        let sema = self.sema();
        let Some((
            SemanticRole::Query {
                columns: cols_idx, ..
            },
            fields,
        )) = sema.role_for_node(query)
        else {
            return (Vec::new(), Vec::new(), true);
        };
        let Some(list_id) = fields.node_id_at(cols_idx) else {
            return (Vec::new(), Vec::new(), true);
        };

        let mut columns: Vec<ColumnLineage> = Vec::new();
        let mut names: Vec<String> = Vec::new();
        let mut complete = true;
        let mut index: u32 = 0;

        sema.for_each_result_column(list_id, |child_fields, flags_idx, alias_idx, expr_idx| {
            if is_star(child_fields, flags_idx) {
                self.expand_star(&mut columns, &mut names, &mut index, &mut complete);
            } else {
                let name = sema
                    .infer_result_col_name(child_fields, alias_idx, expr_idx)
                    .unwrap_or_default();
                let origin = child_fields
                    .node_id_at(expr_idx)
                    .and_then(|id| self.trace_expr(id));
                names.push(name.clone());
                columns.push(ColumnLineage {
                    name,
                    index,
                    origin,
                });
                index += 1;
            }
            true
        });

        (columns, names, complete)
    }

    fn expand_star(
        &self,
        columns: &mut Vec<ColumnLineage>,
        names: &mut Vec<String>,
        index: &mut u32,
        complete: &mut bool,
    ) {
        for info in self.sources.values() {
            let Some(cols) = info.columns.clone() else {
                *complete = false;
                continue;
            };
            for col in cols {
                let origin = self.origin_for_source(info, &col);
                let name = col.to_ascii_lowercase();
                names.push(name.clone());
                columns.push(ColumnLineage {
                    name,
                    index: *index,
                    origin,
                });
                *index += 1;
            }
        }
    }

    /// Trace a single expression's origin. Only `ColumnRef` resolves;
    /// literals, calls, arithmetic yield `None`.
    fn trace_expr(&self, expr_id: AnyNodeId) -> Option<ColumnOrigin> {
        let sema = self.sema();
        let (
            SemanticRole::ColumnRef {
                column: col_idx,
                table: tbl_idx,
            },
            expr_fields,
        ) = sema.role_for_node(expr_id)?
        else {
            return None;
        };

        let col_name = self
            .stmt
            .span_field_text(&expr_fields, col_idx)?
            .to_ascii_lowercase();
        let source_name = match self.stmt.span_field_text(&expr_fields, tbl_idx) {
            Some(t) => t.to_ascii_lowercase(),
            None => self.find_source_for(&col_name)?,
        };

        let info = self.sources.get(&source_name).or_else(|| {
            self.sources
                .values()
                .find(|i| i.canonical.eq_ignore_ascii_case(&source_name))
        })?;
        self.origin_for_source(info, &col_name)
    }

    fn origin_for_source(&self, info: &SourceInfo, col: &str) -> Option<ColumnOrigin> {
        match info.kind {
            SourceKind::Table => Some(ColumnOrigin {
                table: info.canonical.clone(),
                column: col.to_ascii_lowercase(),
            }),
            SourceKind::View => None,
            SourceKind::Cte(body) | SourceKind::Subquery(body) => {
                self.origin_from_nested(body, col)
            }
        }
    }

    /// Reach into a nested Query's finalized summary for the origin of
    /// a named column. `None` when the body is unfinalized (recursive
    /// self-reference) or the column isn't produced by the body.
    fn origin_from_nested(&self, body: AnyNodeId, col: &str) -> Option<ColumnOrigin> {
        let body_sum = self.per_query.get(&body)?;
        body_sum
            .lineage
            .columns
            .iter()
            .find(|c| c.name.eq_ignore_ascii_case(col))
            .and_then(|c| c.origin.clone())
    }

    /// Pick the source a bare (unqualified) column name refers to.
    /// Prefers a source whose known-columns list contains it; falls
    /// back to the first source otherwise (matching legacy behavior).
    fn find_source_for(&self, col_name: &str) -> Option<String> {
        for (name, info) in self.sources {
            if let Some(cols) = &info.columns
                && cols.iter().any(|c| c.eq_ignore_ascii_case(col_name))
            {
                return Some(name.clone());
            }
        }
        self.sources.keys().next().cloned()
    }

    // ── Source-set aggregation ────────────────────────────────────────────────

    /// Build `relations` / `physical_tables` / `unexpanded_views` for
    /// this Query by flattening each source against finalized nested
    /// summaries. Returns `complete=false` when a view is unexpanded or
    /// a nested summary is already incomplete.
    fn aggregate_sources(
        &self,
    ) -> (
        Vec<RelationAccess>,
        Vec<PhysicalTableAccess>,
        Vec<String>,
        bool,
    ) {
        let mut relations: Vec<RelationAccess> = Vec::new();
        let mut physical_tables: Vec<PhysicalTableAccess> = Vec::new();
        let mut unexpanded_views: Vec<String> = Vec::new();
        let mut complete = true;

        for info in self.sources.values() {
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
                    complete = false;
                }
                SourceKind::Cte(body) | SourceKind::Subquery(body) => {
                    // Unfinalized body = recursive self-reference; skip
                    // transitive contributions, matching legacy cycle
                    // behavior.
                    let Some(body_sum) = self.per_query.get(&body) else {
                        continue;
                    };
                    relations.extend(body_sum.lineage.relations.iter().cloned());
                    physical_tables.extend(body_sum.lineage.physical_tables.iter().cloned());
                    unexpanded_views.extend(body_sum.lineage.unexpanded_views.iter().cloned());
                    if !body_sum.lineage.complete {
                        complete = false;
                    }
                }
            }
        }

        relations.sort_by(|a, b| a.name.cmp(&b.name));
        relations.dedup_by(|a, b| a.name == b.name);
        physical_tables.sort_by(|a, b| a.name.cmp(&b.name));
        physical_tables.dedup_by(|a, b| a.name == b.name);
        unexpanded_views.sort();
        unexpanded_views.dedup();

        (relations, physical_tables, unexpanded_views, complete)
    }
}

fn is_star(fields: &NodeFields, flags_idx: u8) -> bool {
    matches!(
        fields[flags_idx as usize],
        FieldValue::Flags(f) if f & 1 != 0
    )
}

// ── Public entry point ────────────────────────────────────────────────────────

/// Extract the outer Query's finalized lineage from a completed
/// [`LineageCapture`]. Returns `None` when the statement wasn't a query
/// (no `enter_query` fired).
pub(super) fn build_lineage(capture: &LineageCapture) -> Option<QueryLineage> {
    let outer = capture.outer_query?;
    capture.per_query.get(&outer).map(|s| s.lineage.clone())
}
