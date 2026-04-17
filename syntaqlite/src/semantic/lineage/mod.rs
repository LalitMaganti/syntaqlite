// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Column lineage analysis.
//!
//! Given a parsed SELECT statement, traces each result column back to its
//! source table.column and collects all physical tables accessed.

mod resolver;
mod types;

pub(crate) use types::QueryLineage;
pub use types::{
    ColumnLineage, ColumnOrigin, LineageResult, PhysicalTableAccess, RelationAccess, RelationKind,
};

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement};

use super::catalog::Catalog;
use crate::dialect::SemanticRole;

/// Compute column lineage for a statement.
///
/// Returns `None` if the statement is not a query (SELECT/WITH ... SELECT).
pub(super) fn compute_lineage(
    stmt: &AnyParsedStatement<'_>,
    root: AnyNodeId,
    catalog: &Catalog,
    roles: &[SemanticRole],
) -> Option<QueryLineage> {
    let mut resolver = resolver::LineageResolver::new(stmt, catalog, roles);
    resolver.resolve(root)
}
