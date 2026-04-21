// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! DDL-shaped reads against a parsed statement.
//!
//! The catalog, the analyzer, the lineage resolver, and the LSP all need to
//! pull definition names, column lists, SELECT-result columns, and assorted
//! span/role data out of a parsed statement. Every operation here takes only
//! `(stmt, roles)`, so they're grouped on a single [`DdlReader`] handle.

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue, NodeFields};
use syntaqlite_syntax::source::{DocRange, StmtLen, StmtOffset, StmtRange};

use crate::analysis::catalog::AritySpec;
use crate::analysis::model::DefinedRelation;
use crate::dialect::{FIELD_ABSENT, SemanticRole};

/// Read DDL- and AST-shaped data out of a parsed statement.
///
/// Cheap to construct (just two references); construct a fresh handle at each
/// caller rather than threading one through.
#[derive(Clone, Copy)]
pub(crate) struct DdlReader<'a, 'stmt> {
    stmt: &'a AnyParsedStatement<'stmt>,
    roles: &'a [SemanticRole],
}

impl<'a, 'stmt> DdlReader<'a, 'stmt> {
    pub(crate) fn new(stmt: &'a AnyParsedStatement<'stmt>, roles: &'a [SemanticRole]) -> Self {
        Self { stmt, roles }
    }

    pub(crate) fn stmt(&self) -> &'a AnyParsedStatement<'stmt> {
        self.stmt
    }

    // ── Role lookup ──────────────────────────────────────────────────────

    /// Role for `tag`; defaults to [`SemanticRole::Transparent`] for unknown
    /// tags (matching the behavior expected by the walker).
    pub(crate) fn role_for_tag(&self, tag: impl Into<u32>) -> SemanticRole {
        self.roles
            .get(tag.into() as usize)
            .copied()
            .unwrap_or(SemanticRole::Transparent)
    }

    /// `(role, fields)` for `node_id`, or `None` if the node has no fields.
    pub(crate) fn role_for_node(&self, node_id: AnyNodeId) -> Option<(SemanticRole, NodeFields)> {
        if node_id.is_null() {
            return None;
        }
        let (tag, fields) = self.stmt.extract_fields(node_id)?;
        Some((self.role_for_tag(tag), fields))
    }

    // ── Field accessors ──────────────────────────────────────────────────

    /// `NodeId` stored at field `idx`, or `None` if absent / null / non-NodeId.
    pub(crate) fn node_field(fields: &NodeFields, idx: u8) -> Option<AnyNodeId> {
        if idx == FIELD_ABSENT {
            return None;
        }
        match fields[idx as usize] {
            FieldValue::NodeId(id) if !id.is_null() => Some(id),
            _ => None,
        }
    }

    /// Expanded text of a span field, or `None` if absent / empty / non-Span.
    pub(crate) fn span_field_text(&self, fields: &NodeFields, idx: u8) -> Option<&'stmt str> {
        if idx == FIELD_ABSENT {
            return None;
        }
        match fields[idx as usize] {
            FieldValue::Span(sp) if !sp.is_empty() => Some(self.stmt.span_expanded_text(sp)),
            _ => None,
        }
    }

    /// `(text, range)` of a span field; absolute document range.
    pub(crate) fn span_field_range(
        &self,
        fields: &NodeFields,
        idx: u8,
    ) -> Option<(&'stmt str, DocRange)> {
        if idx == FIELD_ABSENT {
            return None;
        }
        match fields[idx as usize] {
            FieldValue::Span(sp) if !sp.is_empty() => {
                let text = self.stmt.span_expanded_text(sp);
                let (_, range) = self.stmt.span_text_abs(sp);
                Some((text, range))
            }
            _ => None,
        }
    }

    /// `(text, range)` of a Name node's field-0 span (used by `IdentName` and
    /// `Error` shapes where the identifier sits at field 0). Returns empty
    /// strings when the node is null or shaped differently.
    pub(crate) fn name_text(&self, node_id: Option<AnyNodeId>) -> (&'stmt str, DocRange) {
        let Some(node_id) = node_id else {
            return ("", DocRange::default());
        };
        let Some((_, fields)) = self.stmt.extract_fields(node_id) else {
            return ("", DocRange::default());
        };
        if fields.is_empty() {
            return ("", DocRange::default());
        }
        match fields[0] {
            FieldValue::Span(sp) => {
                let text = self.stmt.span_expanded_text(sp);
                let (_, range) = self.stmt.span_text_abs(sp);
                (text, range)
            }
            _ => ("", DocRange::default()),
        }
    }

    /// First non-empty span anywhere in `node_id`'s fields. Used as a generic
    /// "give me whatever identifier this node carries" probe.
    pub(crate) fn first_span_text(&self, node_id: AnyNodeId) -> Option<&'stmt str> {
        if node_id.is_null() {
            return None;
        }
        let (_, fields) = self.stmt.extract_fields(node_id)?;
        for i in 0..fields.len() {
            if let FieldValue::Span(sp) = fields[i]
                && !sp.is_empty()
            {
                return Some(self.stmt.span_expanded_text(sp));
            }
        }
        None
    }

    /// Text of a "name-shaped" field that may be either a direct `Span` or a
    /// `NodeId` pointing at a Name node. Mirrors the dual representation used
    /// by `SourceRef.alias`, CTE names, and similar fields.
    pub(crate) fn name_field_text(&self, fields: &NodeFields, idx: u8) -> Option<&'stmt str> {
        if idx == FIELD_ABSENT {
            return None;
        }
        match fields[idx as usize] {
            FieldValue::Span(sp) if !sp.is_empty() => Some(self.stmt.span_expanded_text(sp)),
            FieldValue::NodeId(id) if !id.is_null() => self.first_span_text(id),
            _ => None,
        }
    }

    // ── Result-column iteration ──────────────────────────────────────────

    /// Visit each `ResultColumn` child of a column-list node, calling `f` with
    /// the result column's fields and the indices of its `(flags, alias, expr)`
    /// fields. `f` returns `false` to stop iteration early.
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

    // ── DDL definition spans (go-to-definition, etc.) ────────────────────

    /// `(lowercase_name, range)` for `CREATE TABLE` / `CREATE VIEW`.
    /// Returns `None` for non-DDL statements.
    pub(crate) fn name_span(&self, root: AnyNodeId) -> Option<(String, DocRange)> {
        let (role, fields) = self.role_for_node(root)?;
        let (SemanticRole::DefineTable { name: name_idx, .. }
        | SemanticRole::DefineView { name: name_idx, .. }) = role
        else {
            return None;
        };
        let (text, range) = self.span_field_range(&fields, name_idx)?;
        Some((text.to_ascii_lowercase(), range))
    }

    /// Per-column `(lowercase_name, range)` pairs for a `CREATE TABLE`.
    pub(crate) fn column_spans(&self, root: AnyNodeId) -> Vec<(String, DocRange)> {
        let mut out = Vec::new();
        let Some((SemanticRole::DefineTable { columns, .. }, fields)) = self.role_for_node(root)
        else {
            return out;
        };
        let Some(col_list_id) = Self::node_field(&fields, columns) else {
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
        let name_id = Self::node_field(&fields, name_idx)?;
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
        match self.span_field_text(&fields, name_idx) {
            Some(name) => vec![DefinedRelation {
                name: name.to_string(),
                is_view,
            }],
            None => Vec::new(),
        }
    }

    // ── Catalog accumulation inputs ───────────────────────────────────────

    /// Columns for a table or view DDL contribution.
    ///
    /// Tries the explicit column list first; falls back to inferring names
    /// from the SELECT body. `None` means inference was impossible (e.g.
    /// `SELECT *`) and the caller should accept any column reference.
    pub(super) fn extract_columns(
        &self,
        fields: &NodeFields,
        columns_field: u8,
        select_field: u8,
    ) -> Option<Vec<String>> {
        if let Some(col_list_id) = Self::node_field(fields, columns_field) {
            let mut columns = Vec::new();
            self.columns_from_column_list(col_list_id, &mut columns);
            if !columns.is_empty() {
                return Some(columns);
            }
        }
        Self::node_field(fields, select_field).and_then(|id| self.columns_from_select(id))
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
        let Some(rt_id) = Self::node_field(fields, return_type_field) else {
            return false;
        };
        let Some((SemanticRole::ReturnSpec { columns }, rt_fields)) = self.role_for_node(rt_id)
        else {
            return false;
        };
        Self::node_field(&rt_fields, columns).is_some()
    }

    /// Argument count for a DDL function declaration.
    pub(super) fn function_arity(&self, fields: &NodeFields, args_field: u8) -> AritySpec {
        let Some(args_id) = Self::node_field(fields, args_field) else {
            return AritySpec::Any;
        };
        let Some(children) = self.stmt.list_children(args_id) else {
            return AritySpec::Any;
        };
        AritySpec::Exact(children.len())
    }

    // ── SELECT-shaped column inference (used by analyzer + lineage) ──────

    /// Names produced by a SELECT body.
    ///
    /// Returns `Some(names)` when every result column has an inferable name
    /// (alias, bare column ref, or expression source text). Returns `None`
    /// when any column is `*` or otherwise unnameable, telling the caller to
    /// register the table conservatively.
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
        let list_id = Self::node_field(&select_fields, cols_idx)?;
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
    /// 3. Any other expression → use the raw source text of the expression
    ///    (`SQLite` calls this `ENAME_SPAN`).
    pub(crate) fn infer_result_col_name(
        &self,
        child_fields: &NodeFields,
        alias_idx: u8,
        expr_idx: u8,
    ) -> Option<String> {
        if let Some(alias_id) = Self::node_field(child_fields, alias_idx)
            && let Some(name) = self.first_span_text(alias_id)
        {
            return Some(name.to_ascii_lowercase());
        }
        let expr_id = Self::node_field(child_fields, expr_idx)?;
        if let Some((
            SemanticRole::ColumnRef {
                column: col_idx, ..
            },
            expr_fields,
        )) = self.role_for_node(expr_id)
            && let Some(name) = self.span_field_text(&expr_fields, col_idx)
        {
            return Some(name.to_ascii_lowercase());
        }
        self.expr_source_text(expr_id).map(str::to_ascii_lowercase)
    }

    /// Source slice spanning every byte covered by `id`'s subtree.
    pub(crate) fn expr_source_text(&self, id: AnyNodeId) -> Option<&'stmt str> {
        let mut min = StmtOffset::from_raw(u32::MAX);
        let mut max = StmtOffset::default();
        self.collect_spans(id, &mut min, &mut max);
        if min < max {
            Some(
                &self.stmt.text()[StmtRange {
                    start: min,
                    end: max,
                }],
            )
        } else {
            None
        }
    }

    fn collect_spans(&self, id: AnyNodeId, min: &mut StmtOffset, max: &mut StmtOffset) {
        if id.is_null() {
            return;
        }
        if let Some((_, fields)) = self.stmt.extract_fields(id) {
            for i in 0..fields.len() {
                match fields[i] {
                    FieldValue::Span(sp) if !sp.is_empty() => {
                        let (text, off) = self.stmt.span_text(sp);
                        let start = off;
                        let end = start
                            + StmtLen::from_raw(u32::try_from(text.len()).unwrap_or(u32::MAX));
                        if start < *min {
                            *min = start;
                        }
                        if end > *max {
                            *max = end;
                        }
                    }
                    FieldValue::NodeId(child) if !child.is_null() => {
                        self.collect_spans(child, min, max);
                    }
                    _ => {}
                }
            }
        }
        if let Some(children) = self.stmt.list_children(id) {
            for &child in children {
                self.collect_spans(child, min, max);
            }
        }
    }
}
