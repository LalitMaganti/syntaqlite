// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Column lineage analysis.
//!
//! Two pieces:
//!
//! - [`LineageCapture`] runs as a [`SemanticVisitor`] during the
//!   statement walk. It records CTE bindings declared at the statement's
//!   outer `WITH` (matching the legacy flat-map scope) and the outermost
//!   `Query` node's id.
//! - [`LineageBuilder`] runs after the walk. It uses the captured CTE
//!   map plus fresh [`SemanticPropertyExtractor`] reads to produce a
//!   [`QueryLineage`].
//!
//! The trace-through-CTE logic (result columns, column origins, view
//! flattening) lives in the builder; the capture is intentionally
//! minimal so each concern stays in one place.

use std::collections::HashMap;

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement};

use super::catalog::Catalog;
use super::engine::walker::{CteBindingEvent, SemanticVisitor};
use crate::dialect::SemanticRole;

mod builder;
mod types;

pub(crate) use types::QueryLineage;
pub use types::{
    ColumnLineage, ColumnOrigin, LineageResult, PhysicalTableAccess, RelationAccess, RelationKind,
};

/// Visitor that captures the bits of state lineage needs that aren't
/// trivially recoverable from the AST post-walk.
#[derive(Default)]
pub(crate) struct LineageCapture {
    /// CTE bindings declared at the statement's outer `WITH`, keyed by
    /// lowercase name. Nested `WITH` clauses (inside a subquery or a
    /// CTE body) are intentionally excluded — they match the legacy
    /// scope and keep lookups cheap.
    cte_bodies: HashMap<String, AnyNodeId>,
    /// Number of `Query` nodes currently being walked.
    query_depth: usize,
    /// The outermost `Query` node — set from the first `enter_query`.
    outer_query: Option<AnyNodeId>,
}

impl SemanticVisitor for LineageCapture {
    const WANTS_CTE_BINDING: bool = true;
    const WANTS_QUERY: bool = true;

    fn enter_query(&mut self, _stmt: &mut AnyParsedStatement<'_>, node_id: AnyNodeId) {
        if self.outer_query.is_none() {
            self.outer_query = Some(node_id);
        }
        self.query_depth += 1;
    }

    fn exit_query(&mut self, _stmt: &mut AnyParsedStatement<'_>, _node_id: AnyNodeId) {
        self.query_depth = self.query_depth.saturating_sub(1);
    }

    fn on_cte_binding(&mut self, _stmt: &mut AnyParsedStatement<'_>, ev: CteBindingEvent<'_>) {
        // Only capture outer-WITH bindings, matching legacy behavior. A
        // binding fired while a Query is being walked belongs to a
        // nested WITH clause whose scope the flat map would otherwise
        // overflow.
        if self.query_depth != 0 {
            return;
        }
        if let Some(body_id) = ev.body_id
            && !ev.name.is_empty()
        {
            self.cte_bodies
                .insert(ev.name.to_ascii_lowercase(), body_id);
        }
    }
}

/// Compute column lineage from a completed [`LineageCapture`].
pub(super) fn build_lineage(
    capture: &LineageCapture,
    stmt: &AnyParsedStatement<'_>,
    catalog: &Catalog,
    roles: &[SemanticRole],
) -> Option<QueryLineage> {
    let outer = capture.outer_query?;
    builder::LineageBuilder::new(capture, stmt, catalog, roles).build(outer)
}
