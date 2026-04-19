// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Pure helper functions used by the analyzer: source extraction, definition
//! span lookup, macro registration extraction, rowid aliasing.

use syntaqlite_syntax::any::{AnyNodeId, AnyParseError, AnyParsedStatement, FieldValue};

use crate::dialect::{FIELD_ABSENT, MacroDef, SemanticRole};
#[cfg(feature = "lsp")]
use crate::semantic::catalog::AritySpec;
use crate::semantic::model::DefinedRelation;
#[cfg(any(feature = "lsp", feature = "experimental-embedded"))]
use crate::semantic::model::{StoredComment, StoredToken};

/// Extract relations defined by a DDL statement (CREATE TABLE / CREATE VIEW).
pub(super) fn extract_defined_relations(
    stmt: &AnyParsedStatement<'_>,
    root: AnyNodeId,
    roles: &[SemanticRole],
) -> Vec<DefinedRelation> {
    let Some((tag, fields)) = stmt.extract_fields(root) else {
        return Vec::new();
    };
    let idx = u32::from(tag) as usize;
    let Some(&role) = roles.get(idx) else {
        return Vec::new();
    };
    let (name_field, is_view) = match role {
        SemanticRole::DefineTable { name, .. } => (name, false),
        SemanticRole::DefineView { name, .. } => (name, true),
        _ => return Vec::new(),
    };
    if let FieldValue::Span(sp) = fields[name_field as usize]
        && !sp.is_empty()
    {
        vec![DefinedRelation {
            name: stmt.span_expanded_text(sp).to_string(),
            is_view,
        }]
    } else {
        Vec::new()
    }
}

#[cfg(feature = "lsp")]
pub(super) fn format_arity(name: &str, arity: AritySpec) -> String {
    match arity {
        AritySpec::Exact(n) => {
            let params: Vec<String> = (0..n).map(|i| format!("arg{}", i + 1)).collect();
            format!("{}({})", name, params.join(", "))
        }
        AritySpec::AtLeast(n) => {
            let mut params: Vec<String> = (0..n).map(|i| format!("arg{}", i + 1)).collect();
            params.push("...".to_string());
            format!("{}({})", name, params.join(", "))
        }
        AritySpec::Any => format!("{name}(...)"),
    }
}

#[cfg(any(feature = "lsp", feature = "experimental-embedded"))]
pub(super) fn collect_tokens<'a>(
    iter: impl Iterator<Item = syntaqlite_syntax::any::AnyParserToken<'a>>,
    stmt_base: u32,
    tokens: &mut Vec<StoredToken>,
) {
    let base = stmt_base as usize;
    for tok in iter {
        tokens.push(StoredToken {
            offset: base + tok.offset() as usize,
            length: tok.length() as usize,
            token_type: tok.token_type(),
            flags: tok.flags(),
        });
    }
}

#[cfg(any(feature = "lsp", feature = "experimental-embedded"))]
pub(super) fn collect_comments<'a>(
    iter: impl Iterator<Item = syntaqlite_syntax::Comment<'a>>,
    stmt_base: u32,
    comments: &mut Vec<StoredComment>,
) {
    let base = stmt_base as usize;
    for c in iter {
        comments.push(StoredComment {
            offset: base + c.offset() as usize,
            length: c.length() as usize,
        });
    }
}

pub(super) fn parse_error_span(err: &AnyParseError<'_>, source: &str) -> (usize, usize) {
    let base = err.statement_base_offset() as usize;
    match (err.offset(), err.length()) {
        (Some(off), Some(len)) if len > 0 => (base + off, base + off + len),
        (Some(off), _) => {
            let abs = base + off;
            if abs >= source.len() && !source.is_empty() {
                (source.len() - 1, source.len())
            } else {
                (abs, (abs + 1).min(source.len()))
            }
        }
        _ => {
            let end = source.len();
            let start = if end > 0 { end - 1 } else { 0 };
            (start, end)
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
