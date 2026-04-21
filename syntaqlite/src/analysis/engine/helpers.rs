// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Pure helper functions used by the analyzer: parse-error span lookup,
//! macro registration extraction, rowid aliasing.

use syntaqlite_syntax::any::{AnyNodeId, AnyParseError, AnyParsedStatement, FieldValue};
use syntaqlite_syntax::source::{DocLen, DocOffset, DocRange, StmtLen};

use crate::dialect::{FIELD_ABSENT, MacroDef};

pub(super) fn parse_error_span(err: &AnyParseError<'_>, source: &str) -> DocRange {
    let base = err.statement_base();
    let source_end = DocOffset::from_raw(u32::try_from(source.len()).unwrap_or(u32::MAX));
    match (err.offset(), err.length()) {
        (Some(off), Some(len)) if len > StmtLen::default() => {
            let start = off.to_doc(base);
            DocRange::from_offset_len(start, len.into())
        }
        (Some(off), _) => {
            let abs = off.to_doc(base);
            if abs >= source_end && !source.is_empty() {
                DocRange {
                    start: source_end - DocLen::from_raw(1),
                    end: source_end,
                }
            } else {
                DocRange {
                    start: abs,
                    end: std::cmp::min(abs + DocLen::from_raw(1), source_end),
                }
            }
        }
        _ => {
            let one = DocLen::from_raw(1);
            let start = if source_end > DocOffset::default() {
                source_end - one
            } else {
                DocOffset::default()
            };
            DocRange {
                start,
                end: source_end,
            }
        }
    }
}

/// Check if the root node of a statement defines a template macro and, if so,
/// extract the macro name, parameter names, and body text as owned strings.
///
/// Returns `None` when the statement is not a macro definition or when any
/// required field is missing.
pub(super) fn extract_macro_registration(
    stmt: &AnyParsedStatement<'_>,
    root: AnyNodeId,
    macro_defs: &[MacroDef],
) -> Option<(String, Vec<String>, String)> {
    if macro_defs.is_empty() || root.is_null() {
        return None;
    }
    let (tag, fields) = stmt.extract_fields(root)?;
    let tag_u32 = u32::from(tag);

    let def = macro_defs.iter().find(|d| d.node_tag() == tag_u32)?;

    let name = match fields[def.name_field as usize] {
        FieldValue::Span(sp) if !sp.is_empty() => stmt.span_text(sp).0.to_string(),
        _ => return None,
    };

    let body = match fields[def.body_field as usize] {
        FieldValue::Span(sp) if !sp.is_empty() => stmt.span_expanded_text(sp).to_string(),
        _ => return None,
    };

    let params = if def.args_field == FIELD_ABSENT {
        Vec::new()
    } else {
        let args_id = match fields[def.args_field as usize] {
            FieldValue::NodeId(id) if !id.is_null() => id,
            _ => return Some((name, Vec::new(), body)),
        };
        let children = stmt.list_children(args_id)?;
        let mut param_names = Vec::with_capacity(children.len());
        for &child_id in children {
            if child_id.is_null() {
                continue;
            }
            let (_, child_fields) = stmt.extract_fields(child_id)?;
            match child_fields[def.arg_name_field as usize] {
                FieldValue::Span(sp) if !sp.is_empty() => {
                    param_names.push(stmt.span_text(sp).0.to_string());
                }
                _ => return None,
            }
        }
        param_names
    };

    Some((name, params, body))
}

pub(super) fn is_rowid_alias(column: &str) -> bool {
    column.eq_ignore_ascii_case("rowid")
        || column.eq_ignore_ascii_case("oid")
        || column.eq_ignore_ascii_case("_rowid_")
}
