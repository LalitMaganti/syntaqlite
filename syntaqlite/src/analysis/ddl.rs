// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Role-table-driven property extraction from a parsed statement.
//!
//! [`SemanticPropertyExtractor`] is the analysis crate's view of a parsed
//! statement through the dialect's `SemanticRole` table. It bundles
//! `(stmt, roles)` so call sites don't repeat both args, and exposes
//! methods that answer questions like "what does this `CREATE TABLE`
//! define?", "iterate this WITH clause's CTE bindings", "what columns
//! does this SELECT body produce?".
//!
//! Field-shaped accessors that don't need the role table
//! (`span_field_text`, `name_text`, `expr_source_text`, etc.) live on
//! [`AnyParsedStatement`] in `syntaqlite-syntax` directly — they're raw
//! parse-tree convenience, not analysis.

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue, NodeFields};
use syntaqlite_syntax::source::DocRange;

use crate::analysis::catalog::AritySpec;
use crate::analysis::model::DefinedRelation;
use crate::dialect::SemanticRole;

/// Bundles `(stmt, roles)` and exposes role-table-driven extraction.
///
/// Cheap to construct — just two references — so call sites build a
/// fresh handle locally rather than thread one through.
#[derive(Clone, Copy)]
pub(crate) struct SemanticPropertyExtractor<'a, 'stmt> {
    stmt: &'a AnyParsedStatement<'stmt>,
    roles: &'a [SemanticRole],
}

/// A single FROM-clause source, after walking through transparent wrappers
/// and joins. Aliases are extracted but not lowercased — callers do that.
pub(crate) enum FromSource<'b> {
    /// A relation reference: catalog table, view, CTE, or table-valued function.
    Relation {
        name: &'b str,
        alias: Option<&'b str>,
    },
    /// A bracketed subquery in FROM, with its body and (optional) alias.
    Subquery {
        alias: Option<&'b str>,
        body_id: AnyNodeId,
    },
}

impl<'a, 'stmt> SemanticPropertyExtractor<'a, 'stmt> {
    pub(crate) fn new(stmt: &'a AnyParsedStatement<'stmt>, roles: &'a [SemanticRole]) -> Self {
        Self { stmt, roles }
    }

    pub(crate) fn stmt(&self) -> &'a AnyParsedStatement<'stmt> {
        self.stmt
    }

    // ── Role lookup ──────────────────────────────────────────────────────

    /// Role for `tag`; defaults to [`SemanticRole::Transparent`] for
    /// unknown tags (matching what the walker assumes).
    pub(crate) fn role_for_tag(&self, tag: impl Into<u32>) -> SemanticRole {
        self.roles
            .get(tag.into() as usize)
            .copied()
            .unwrap_or(SemanticRole::Transparent)
    }

    /// `(role, fields)` for `node_id`, or `None` when the node is null or
    /// has no extractable fields.
    pub(crate) fn role_for_node(&self, node_id: AnyNodeId) -> Option<(SemanticRole, NodeFields)> {
        if node_id.is_null() {
            return None;
        }
        let (tag, fields) = self.stmt.extract_fields(node_id)?;
        Some((self.role_for_tag(tag), fields))
    }

    // ── Iteration ────────────────────────────────────────────────────────

    /// Visit each `ResultColumn` child of a column-list node, calling `f`
    /// with the result column's fields and the indices of its `(flags,
    /// alias, expr)` fields. `f` returns `false` to stop iteration early.
    ///
    /// Skips children that aren't `ResultColumn` (transparent wrappers,
    /// commas, etc.) so callers don't have to.
    pub(crate) fn for_each_result_column<F>(&self, list_id: AnyNodeId, mut f: F)
    where
        F: FnMut(&NodeFields, u8, u8, u8) -> bool,
    {
        let Some(children) = self.stmt.list_children(list_id) else {
            return;
        };
        for &child_id in children {
            if child_id.is_null() {
                continue;
            }
            let Some((child_tag, child_fields)) = self.stmt.extract_fields(child_id) else {
                continue;
            };
            let SemanticRole::ResultColumn { flags, alias, expr } = self.role_for_tag(child_tag)
            else {
                continue;
            };
            if !f(&child_fields, flags, alias, expr) {
                return;
            }
        }
    }

    /// Declared column list for a `CteBinding` node, when the binding
    /// wrote out the parenthesized `(col1, col2, ...)` part. Returns
    /// `None` when no column list was declared (the body's columns are
    /// inferred instead).
    pub(crate) fn cte_declared_cols(
        &self,
        binding_node: AnyNodeId,
    ) -> Option<Vec<(&'stmt str, DocRange)>> {
        let (
            SemanticRole::CteBinding {
                columns: cols_idx, ..
            },
            fields,
        ) = self.role_for_node(binding_node)?
        else {
            return None;
        };
        let list_id = fields.node_id_at(cols_idx)?;
        let children = self.stmt.list_children(list_id)?;
        let mut names: Vec<(&'stmt str, DocRange)> = Vec::with_capacity(children.len());
        for &id in children {
            let (text, range) = self.stmt.name_text(Some(id));
            if !text.is_empty() {
                names.push((text, range));
            }
        }
        if names.is_empty() { None } else { Some(names) }
    }

    /// Visit each FROM-clause source under `from_id`, recursing through
    /// transparent wrappers and join nodes. Both `SourceRef` (catalog
    /// relations) and `ScopedSource` (subqueries) are reported.
    pub(crate) fn for_each_from_source<F>(&self, from_id: AnyNodeId, mut f: F)
    where
        F: FnMut(FromSource<'stmt>),
    {
        self.walk_from(from_id, &mut f);
    }

    fn walk_from<F>(&self, from_id: AnyNodeId, f: &mut F)
    where
        F: FnMut(FromSource<'stmt>),
    {
        let Some((role, fields)) = self.role_for_node(from_id) else {
            return;
        };
        match role {
            SemanticRole::SourceRef {
                name: name_idx,
                alias: alias_idx,
                ..
            } => {
                let Some(name) = self.stmt.span_field_text(&fields, name_idx) else {
                    return;
                };
                let alias = self.stmt.name_field_text(&fields, alias_idx);
                f(FromSource::Relation { name, alias });
            }
            SemanticRole::ScopedSource {
                body: body_idx,
                alias: alias_idx,
            } => {
                let Some(body_id) = fields.node_id_at(body_idx) else {
                    return;
                };
                let alias = self.stmt.name_field_text(&fields, alias_idx);
                f(FromSource::Subquery { alias, body_id });
            }
            _ => {
                for child in self.stmt.child_node_ids(from_id) {
                    self.walk_from(child, f);
                }
            }
        }
    }

    // ── DDL definition extraction ───────────────────────────────────────

    /// `(lowercase_name, range)` for `CREATE TABLE` / `CREATE VIEW`.
    /// Returns `None` for non-DDL statements.
    pub(crate) fn name_span(&self, root: AnyNodeId) -> Option<(String, DocRange)> {
        let (role, fields) = self.role_for_node(root)?;
        let (SemanticRole::DefineTable { name: name_idx, .. }
        | SemanticRole::DefineView { name: name_idx, .. }) = role
        else {
            return None;
        };
        let (text, range) = self.stmt.span_field_range(&fields, name_idx)?;
        Some((text.to_ascii_lowercase(), range))
    }

    /// Per-column `(lowercase_name, range)` pairs for a `CREATE TABLE`.
    pub(crate) fn column_spans(&self, root: AnyNodeId) -> Vec<(String, DocRange)> {
        let mut out = Vec::new();
        let Some((SemanticRole::DefineTable { columns, .. }, fields)) = self.role_for_node(root)
        else {
            return out;
        };
        let Some(col_list_id) = fields.node_id_at(columns) else {
            return out;
        };
        let Some(children) = self.stmt.list_children(col_list_id) else {
            return out;
        };
        for &child_id in children {
            if child_id.is_null() {
                continue;
            }
            if let Some((name, range)) = self.column_def_name_span(child_id) {
                out.push((name.to_ascii_lowercase(), range));
            }
        }
        out
    }

    fn column_def_name_span(&self, node_id: AnyNodeId) -> Option<(&'stmt str, DocRange)> {
        let (SemanticRole::ColumnDef { name: name_idx, .. }, fields) =
            self.role_for_node(node_id)?
        else {
            return None;
        };
        let name_id = fields.node_id_at(name_idx)?;
        let (_, name_fields) = self.stmt.extract_fields(name_id)?;
        for j in 0..name_fields.len() {
            if let FieldValue::Span(sp) = name_fields[j]
                && !sp.is_empty()
            {
                let (s, range) = self.stmt.span_text_abs(sp);
                return Some((s, range));
            }
        }
        None
    }

    /// Relations defined by a DDL statement at `root` (`CREATE TABLE` or
    /// `CREATE VIEW`). Returns at most one relation per statement.
    pub(crate) fn defined_relations(&self, root: AnyNodeId) -> Vec<DefinedRelation> {
        let Some((role, fields)) = self.role_for_node(root) else {
            return Vec::new();
        };
        let (name_idx, is_view) = match role {
            SemanticRole::DefineTable { name, .. } => (name, false),
            SemanticRole::DefineView { name, .. } => (name, true),
            _ => return Vec::new(),
        };
        match self.stmt.span_field_text(&fields, name_idx) {
            Some(name) => vec![DefinedRelation {
                name: name.to_string(),
                is_view,
            }],
            None => Vec::new(),
        }
    }

    /// Columns for a table or view DDL contribution.
    ///
    /// Tries the explicit column list first; falls back to inferring
    /// names from the SELECT body. `None` means inference was impossible
    /// (e.g. `SELECT *`) and the caller should accept any column reference.
    pub(super) fn extract_columns(
        &self,
        fields: &NodeFields,
        columns_field: u8,
        select_field: u8,
    ) -> Option<Vec<String>> {
        if let Some(col_list_id) = fields.node_id_at(columns_field) {
            let mut columns = Vec::new();
            self.columns_from_column_list(col_list_id, &mut columns);
            if !columns.is_empty() {
                return Some(columns);
            }
        }
        fields
            .node_id_at(select_field)
            .and_then(|id| self.columns_from_select(id))
    }

    fn columns_from_column_list(&self, list_id: AnyNodeId, out: &mut Vec<String>) {
        let Some(children) = self.stmt.list_children(list_id) else {
            return;
        };
        for &child_id in children {
            if let Some((name, _)) = self.column_def_name_span(child_id) {
                out.push(name.to_ascii_lowercase());
            }
        }
    }

    /// Whether a DDL function returns a table (has a `ReturnSpec` with
    /// non-empty columns).
    pub(super) fn is_table_returning(&self, fields: &NodeFields, return_type_field: u8) -> bool {
        let Some(rt_id) = fields.node_id_at(return_type_field) else {
            return false;
        };
        let Some((SemanticRole::ReturnSpec { columns }, rt_fields)) = self.role_for_node(rt_id)
        else {
            return false;
        };
        rt_fields.node_id_at(columns).is_some()
    }

    /// Argument count for a DDL function declaration.
    pub(super) fn function_arity(&self, fields: &NodeFields, args_field: u8) -> AritySpec {
        let Some(args_id) = fields.node_id_at(args_field) else {
            return AritySpec::Any;
        };
        let Some(children) = self.stmt.list_children(args_id) else {
            return AritySpec::Any;
        };
        AritySpec::Exact(children.len())
    }

    // ── SELECT-shaped column inference (analyzer + lineage) ─────────────

    /// Names produced by a SELECT body.
    ///
    /// Returns `Some(names)` when every result column has an inferable
    /// name (alias, bare column ref, or expression source text). Returns
    /// `None` when any column is `*` or otherwise unnameable, telling
    /// the caller to register the table conservatively.
    pub(crate) fn columns_from_select(&self, select_id: AnyNodeId) -> Option<Vec<String>> {
        let (
            SemanticRole::Query {
                columns: cols_idx, ..
            },
            select_fields,
        ) = self.role_for_node(select_id)?
        else {
            return None;
        };
        let list_id = select_fields.node_id_at(cols_idx)?;
        let mut out = Vec::new();
        let mut bailed = false;
        self.for_each_result_column(list_id, |fields, flags_idx, alias_idx, expr_idx| {
            // STAR flag (bit 0) → wildcard: can't enumerate.
            if let FieldValue::Flags(f) = fields[flags_idx as usize]
                && f & 1 != 0
            {
                bailed = true;
                return false;
            }
            if let Some(name) = self.infer_result_col_name(fields, alias_idx, expr_idx) {
                out.push(name);
                true
            } else {
                bailed = true;
                false
            }
        });
        if bailed || out.is_empty() {
            None
        } else {
            Some(out)
        }
    }

    /// The output column name for a single result column.
    ///
    /// Mirrors `SQLite`'s `sqlite3ExprListSetName` / `sqlite3ExprListSetSpan`:
    /// 1. Explicit alias → use alias text.
    /// 2. Bare `ColumnRef` with no alias → use the column-name span.
    /// 3. Any other expression → use the raw source text of the
    ///    expression (`SQLite` calls this `ENAME_SPAN`).
    pub(crate) fn infer_result_col_name(
        &self,
        child_fields: &NodeFields,
        alias_idx: u8,
        expr_idx: u8,
    ) -> Option<String> {
        if let Some(alias_id) = child_fields.node_id_at(alias_idx)
            && let Some(name) = self.stmt.first_span_text(alias_id)
        {
            return Some(name.to_ascii_lowercase());
        }
        let expr_id = child_fields.node_id_at(expr_idx)?;
        if let Some((
            SemanticRole::ColumnRef {
                column: col_idx, ..
            },
            expr_fields,
        )) = self.role_for_node(expr_id)
            && let Some(name) = self.stmt.span_field_text(&expr_fields, col_idx)
        {
            return Some(name.to_ascii_lowercase());
        }
        // Fall back to the parser's recorded extent for this node — its
        // source-text slice covers the full expression including any
        // operators/punctuation. Requires `with_collect_node_extents` on
        // the parser config, which the analyzer enables.
        self.stmt
            .node_text(expr_id)
            .map(|(text, _)| text.to_ascii_lowercase())
    }
}
