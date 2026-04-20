// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Pure data types for column lineage results.

/// Whether lineage was fully or partially resolved.
///
/// `Complete` means all sources traced to physical tables.
/// `Partial` means at least one source (e.g. a view with unavailable body)
/// could not be fully resolved.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum LineageResult<T> {
    /// All lineage fully resolved.
    Complete(T),
    /// Best-effort result — some view bodies unavailable.
    Partial(T),
}

impl<T> LineageResult<T> {
    /// Returns `true` if fully resolved.
    pub fn is_complete(&self) -> bool {
        matches!(self, LineageResult::Complete(_))
    }

    /// Unwrap the inner value regardless of completeness.
    pub fn into_inner(self) -> T {
        match self {
            LineageResult::Complete(v) | LineageResult::Partial(v) => v,
        }
    }

    /// Convert `&LineageResult<T>` to `LineageResult<&T>`.
    pub fn as_ref(&self) -> LineageResult<&T> {
        match self {
            LineageResult::Complete(v) => LineageResult::Complete(v),
            LineageResult::Partial(v) => LineageResult::Partial(v),
        }
    }

    /// Map the inner value.
    pub fn map<U, F: FnOnce(T) -> U>(self, f: F) -> LineageResult<U> {
        match self {
            LineageResult::Complete(v) => LineageResult::Complete(f(v)),
            LineageResult::Partial(v) => LineageResult::Partial(f(v)),
        }
    }
}

/// The origin of a result column — which table and column it traces back to.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ColumnOrigin {
    /// The physical table name.
    pub table: String,
    /// The column name in that table.
    pub column: String,
}

/// Lineage information for a single result column.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ColumnLineage {
    /// The output column name (alias or inferred).
    pub name: String,
    /// Zero-based index in the result column list.
    pub index: u32,
    /// The origin table.column, or `None` for expressions/literals/aggregates.
    pub origin: Option<ColumnOrigin>,
}

/// What kind of catalog relation was accessed.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RelationKind {
    /// A physical table.
    Table,
    /// A view (body may or may not be available for resolution).
    View,
}

/// A catalog relation (table or view) referenced in the query's FROM clause.
///
/// CTEs and subquery aliases are **not** included — only relations that exist
/// in the catalog.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RelationAccess {
    /// The relation name as it appears in the catalog.
    pub name: String,
    /// Whether this is a table or a view.
    pub kind: RelationKind,
}

/// A physical table accessed by the query (after resolving CTEs and subqueries).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PhysicalTableAccess {
    /// The physical table name.
    pub name: String,
}

/// Internal lineage result computed by the resolver.
#[derive(Debug, Clone)]
pub(crate) struct QueryLineage {
    /// Whether all sources were fully resolved.
    pub(crate) complete: bool,
    /// Per-column lineage information.
    pub(crate) columns: Vec<ColumnLineage>,
    /// Relations referenced directly in FROM clauses.
    pub(crate) relations: Vec<RelationAccess>,
    /// Physical tables accessed (after resolving CTEs/subqueries/views).
    pub(crate) physical_tables: Vec<PhysicalTableAccess>,
    /// Canonical names of views that could not be expanded because no body
    /// was available. Sorted and deduplicated.
    pub(crate) unexpanded_views: Vec<String>,
}
