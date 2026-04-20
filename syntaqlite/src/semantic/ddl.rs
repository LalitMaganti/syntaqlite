// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! DDL-shaped reads against a parsed statement.
//!
//! The catalog, the analyzer, and the lineage resolver all need to pull
//! definition names, column lists, and SELECT-result columns out of a parsed
//! statement.  Every operation here takes only `(stmt, roles)`, so they're
//! grouped on a single [`DdlReader`] handle.

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue, NodeFields};
use syntaqlite_syntax::source::{DocRange, StmtLen, StmtOffset, StmtRange};

use crate::dialect::{FIELD_ABSENT, SemanticRole};
use crate::semantic::catalog::AritySpec;

/// Read DDL-shaped data out of a parsed statement.
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

    fn role_for(&self, tag: impl Into<u32>) -> Option<SemanticRole> {
        self.roles.get(tag.into() as usize).copied()
    }

    /// Expanded text of a span field, or `None` when the field is absent or
    /// the span is empty.
    pub(super) fn span_field_text(&self, fields: &NodeFields, idx: u8) -> Option<String> {
        match fields[idx as usize] {
            FieldValue::Span(sp) if !sp.is_empty() => {
                Some(self.stmt.span_expanded_text(sp).to_string())
            }
            _ => None,
        }
    }

    // ── DDL definition spans (go-to-definition) ──────────────────────────

    /// `(lowercase_name, range)` for `CREATE TABLE` / `CREATE VIEW`.
    /// Returns `None` for non-DDL statements.
    pub(crate) fn name_span(&self, root: AnyNodeId) -> Option<(String, DocRange)> {
        let (tag, fields) = self.stmt.extract_fields(root)?;
        let (SemanticRole::DefineTable { name: name_idx, .. }
        | SemanticRole::DefineView { name: name_idx, .. }) = self.role_for(tag)?
        else {
            return None;
        };
        let FieldValue::Span(sp) = fields[name_idx as usize] else {
            return None;
        };
        if sp.is_empty() {
            return None;
        }
        let (s, range) = self.stmt.span_text_abs(sp);
        Some((s.to_ascii_lowercase(), range))
    }

    /// Per-column `(lowercase_name, range)` pairs for a `CREATE TABLE`.
    pub(crate) fn column_spans(&self, root: AnyNodeId) -> Vec<(String, DocRange)> {
        let mut out = Vec::new();
        let Some((tag, fields)) = self.stmt.extract_fields(root) else {
            return out;
        };
        let Some(SemanticRole::DefineTable { columns, .. }) = self.role_for(tag) else {
            return out;
        };
        if columns == FIELD_ABSENT {
            return out;
        }
        let FieldValue::NodeId(col_list_id) = fields[columns as usize] else {
            return out;
        };
        if col_list_id.is_null() {
            return out;
        }
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
        let (tag, fields) = self.stmt.extract_fields(node_id)?;
        let SemanticRole::ColumnDef { name: name_idx, .. } = self.role_for(tag)? else {
            return None;
        };
        let FieldValue::NodeId(name_id) = fields[name_idx as usize] else {
            return None;
        };
        if name_id.is_null() {
            return None;
        }
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

    // ── Catalog accumulation inputs ───────────────────────────────────────

    /// Columns for a table or view DDL contribution.
    ///
    /// Tries the explicit column list first; falls back to inferring names
    /// from the SELECT body.  `None` means inference was impossible (e.g.
    /// `SELECT *`) and the caller should accept any column reference.
    pub(super) fn extract_columns(
        &self,
        fields: &NodeFields,
        columns_field: Option<u8>,
        select_field: Option<u8>,
    ) -> Option<Vec<String>> {
        if let Some(col_idx) = columns_field
            && let FieldValue::NodeId(col_list_id) = fields[col_idx as usize]
            && !col_list_id.is_null()
        {
            let mut columns = Vec::new();
            self.columns_from_column_list(col_list_id, &mut columns);
            if !columns.is_empty() {
                return Some(columns);
            }
        }
        if let Some(sel_idx) = select_field
            && let FieldValue::NodeId(select_id) = fields[sel_idx as usize]
            && !select_id.is_null()
        {
            return self.columns_from_select(select_id);
        }
        None
    }

    fn columns_from_column_list(&self, list_id: AnyNodeId, out: &mut Vec<String>) {
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
            let Some(SemanticRole::ColumnDef { name: name_idx, .. }) = self.role_for(child_tag)
            else {
                continue;
            };
            let FieldValue::NodeId(name_id) = child_fields[name_idx as usize] else {
                continue;
            };
            if name_id.is_null() {
                continue;
            }
            let Some((_, name_fields)) = self.stmt.extract_fields(name_id) else {
                continue;
            };
            for j in 0..name_fields.len() {
                if let FieldValue::Span(sp) = name_fields[j]
                    && !sp.is_empty()
                {
                    out.push(self.stmt.span_expanded_text(sp).to_ascii_lowercase());
                    break;
                }
            }
        }
    }

    /// Whether a DDL function returns a table (has a `ReturnSpec` with
    /// non-empty columns).
    pub(super) fn is_table_returning(
        &self,
        fields: &NodeFields,
        return_type_field: Option<u8>,
    ) -> bool {
        let Some(rt_idx) = return_type_field else {
            return false;
        };
        let FieldValue::NodeId(rt_id) = fields[rt_idx as usize] else {
            return false;
        };
        if rt_id.is_null() {
            return false;
        }
        let Some((rt_tag, rt_fields)) = self.stmt.extract_fields(rt_id) else {
            return false;
        };
        let Some(SemanticRole::ReturnSpec { columns }) = self.role_for(rt_tag) else {
            return false;
        };
        if columns == FIELD_ABSENT {
            return false;
        }
        matches!(rt_fields[columns as usize], FieldValue::NodeId(id) if !id.is_null())
    }

    /// Argument count for a DDL function declaration.
    pub(super) fn function_arity(&self, fields: &NodeFields, args_field: Option<u8>) -> AritySpec {
        let Some(args_idx) = args_field else {
            return AritySpec::Any;
        };
        let FieldValue::NodeId(args_id) = fields[args_idx as usize] else {
            return AritySpec::Any;
        };
        if args_id.is_null() {
            return AritySpec::Any;
        }
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
        let (select_tag, select_fields) = self.stmt.extract_fields(select_id)?;
        let SemanticRole::Query {
            columns: cols_idx, ..
        } = self.role_for(select_tag)?
        else {
            return None;
        };
        let FieldValue::NodeId(list_id) = select_fields[cols_idx as usize] else {
            return None;
        };
        if list_id.is_null() {
            return None;
        }
        let children = self.stmt.list_children(list_id)?;
        let mut out = Vec::new();
        for &child_id in children {
            if child_id.is_null() {
                continue;
            }
            let (child_tag, child_fields) = self.stmt.extract_fields(child_id)?;
            let SemanticRole::ResultColumn {
                flags: flags_idx,
                alias: alias_idx,
                expr: expr_idx,
            } = self
                .role_for(child_tag)
                .unwrap_or(SemanticRole::Transparent)
            else {
                continue;
            };
            // STAR flag (bit 0) → wildcard: can't enumerate columns.
            if let FieldValue::Flags(f) = child_fields[flags_idx as usize]
                && f & 1 != 0
            {
                return None;
            }
            match self.infer_result_col_name(&child_fields, alias_idx, expr_idx) {
                Some(name) => out.push(name),
                None => return None,
            }
        }
        if out.is_empty() { None } else { Some(out) }
    }

    /// The output column name for a single result column.
    ///
    /// Mirrors `SQLite`'s `sqlite3ExprListSetName` / `sqlite3ExprListSetSpan`:
    /// 1. Explicit alias → use alias text.
    /// 2. Bare `ColumnRef` with no alias → use the column-name span.
    /// 3. Any other expression → use the raw source text of the expression
    ///    (`SQLite` calls this `ENAME_SPAN`).
    fn infer_result_col_name(
        &self,
        child_fields: &NodeFields,
        alias_idx: u8,
        expr_idx: u8,
    ) -> Option<String> {
        if let FieldValue::NodeId(alias_id) = child_fields[alias_idx as usize]
            && !alias_id.is_null()
            && let Some((_, alias_fields)) = self.stmt.extract_fields(alias_id)
        {
            for j in 0..alias_fields.len() {
                if let FieldValue::Span(sp) = alias_fields[j]
                    && !sp.is_empty()
                {
                    return Some(self.stmt.span_expanded_text(sp).to_ascii_lowercase());
                }
            }
        }
        let FieldValue::NodeId(expr_id) = child_fields[expr_idx as usize] else {
            return None;
        };
        if expr_id.is_null() {
            return None;
        }
        let (expr_tag, expr_fields) = self.stmt.extract_fields(expr_id)?;
        if let Some(SemanticRole::ColumnRef {
            column: col_idx, ..
        }) = self.role_for(expr_tag)
            && let FieldValue::Span(sp) = expr_fields[col_idx as usize]
            && !sp.is_empty()
        {
            return Some(self.stmt.span_expanded_text(sp).to_ascii_lowercase());
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
