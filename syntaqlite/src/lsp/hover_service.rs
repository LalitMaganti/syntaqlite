// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Hover + go-to-definition data shaping.
//!
//! Pure functions that consume an already-computed [`DocumentAnalysisData`] and
//! produce the host's hover/definition/rename responses. No analysis dispatch,
//! no document store access — the [`LspHost`](super::host::LspHost) facade
//! handles those before delegating.

use syntaqlite_syntax::source::{DocOffset, DocRange};

use super::analysis_data::{
    DefinitionResult, DocumentAnalysisData, ResolvedSymbol, SymbolIdentity,
};

/// Hover information at `offset`: returns `(hover_text, token_range)` or `None`
/// if the cursor is not on a resolved symbol.
pub(crate) fn hover_info(
    data: &DocumentAnalysisData,
    offset: DocOffset,
) -> Option<(String, DocRange)> {
    let resolution = data.resolution_at(offset)?;
    let hover = format_resolved_hover(&resolution.symbol);
    Some((hover, resolution.range))
}

/// Go-to-definition target at `offset`, or `None` if the cursor is not on a
/// known reference.
pub(crate) fn definition_info(
    data: &DocumentAnalysisData,
    offset: DocOffset,
) -> Option<DefinitionResult> {
    data.definition_at(offset)
}

/// If `offset` lies on a renameable resolved symbol, return its range and
/// current name. Functions are not renameable.
pub(crate) fn prepare_rename(
    data: &DocumentAnalysisData,
    offset: DocOffset,
) -> Option<(DocRange, String)> {
    let res = data.resolution_at(offset)?;
    let name = match &res.symbol {
        ResolvedSymbol::Table { name, .. } => name.clone(),
        ResolvedSymbol::Column { column, .. } => column.clone(),
        ResolvedSymbol::Function { .. } => return None,
    };
    Some((res.range, name))
}

/// Determine the symbol identity at `offset` — either from a resolution or
/// from a definition site (CREATE TABLE / column-def).
pub(crate) fn symbol_identity_at(
    data: &DocumentAnalysisData,
    offset: DocOffset,
) -> Option<SymbolIdentity> {
    // First check resolutions (references in DML/queries).
    if let Some(res) = data.resolution_at(offset) {
        return SymbolIdentity::from_resolved(&res.symbol);
    }

    // Fall back to definition_offsets (cursor on a CREATE TABLE name, etc).
    for (key, &range) in &data.definition_offsets {
        if offset >= range.start && offset < range.end {
            return if let Some((table, col)) = key.split_once('.') {
                Some(SymbolIdentity::Column {
                    table: table.to_string(),
                    column: col.to_string(),
                })
            } else {
                Some(SymbolIdentity::Table(key.clone()))
            };
        }
    }
    None
}

/// Render the markdown hover string for a resolved symbol.
pub(crate) fn format_resolved_hover(symbol: &ResolvedSymbol) -> String {
    match symbol {
        ResolvedSymbol::Table { name, columns, .. } => match columns {
            Some(cols) => format!("**table** `{name}`\n\n```\n{}\n```", cols.join(", ")),
            None => format!("**table** `{name}`"),
        },
        ResolvedSymbol::Column {
            column,
            table,
            all_columns,
            ..
        } => {
            let col_list: Vec<String> = all_columns
                .iter()
                .map(|c| {
                    if c.eq_ignore_ascii_case(column) {
                        format!("**{c}**")
                    } else {
                        c.clone()
                    }
                })
                .collect();
            format!("**column** in `{table}`\n\n{}", col_list.join(", "))
        }
        ResolvedSymbol::Function {
            category, arities, ..
        } => {
            format!("**{category}**\n\n```\n{}\n```", arities.join("\n"))
        }
    }
}
