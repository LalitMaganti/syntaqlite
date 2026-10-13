// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Completion item assembly.
//!
//! Converts pre-computed [`CompletionInfo`] plus the active dialect and catalog
//! into a list of [`CompletionEntry`] values. Keywords, columns, tables, and
//! functions are filtered by the expected-token set and context; qualified
//! references (`t.`) return only columns of the qualifier.

use std::collections::HashSet;

use syntaqlite_syntax::TokenType;
use syntaqlite_syntax::any::AnyTokenType;
use syntaqlite_syntax::util::is_suggestable_keyword;

use crate::analysis::Catalog;
use crate::dialect::AnyDialect;

use crate::analysis::completion::CompletionContext;
use crate::analysis::engine::walker::{ColumnRefEvent, SemanticVisitor, WalkCtx};

use syntaqlite_syntax::any::AnyParsedStatement;

use super::{CompletionEntry, CompletionInfo, CompletionKind};

/// Identifier substituted at the cursor to find the names in scope there.
pub(crate) const CURSOR_MARKER: &str = "syntaqlite_completion_cursor";

/// The column and source names visible at the cursor.
pub(crate) struct CursorScope {
    columns: Vec<String>,
    sources: Vec<String>,
}

/// Records the scope at the unqualified [`CURSOR_MARKER`] column reference.
#[derive(Default)]
pub(crate) struct CursorScopeCapture {
    scope: Option<CursorScope>,
}

impl CursorScopeCapture {
    /// The captured scope, if the cursor was inside a query with sources.
    pub(crate) fn into_scope(self) -> Option<CursorScope> {
        self.scope.filter(|s| !s.sources.is_empty())
    }
}

impl SemanticVisitor for CursorScopeCapture {
    const WANTS_COLUMN_REF: bool = true;

    fn on_column_ref(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        cx: &mut WalkCtx<'_>,
        ev: ColumnRefEvent<'_>,
    ) {
        if ev.table.is_none() && ev.column == CURSOR_MARKER {
            self.scope = Some(CursorScope {
                columns: cx.scope.all_column_names(None),
                sources: cx.scope.all_source_names(),
            });
        }
    }
}

/// Assemble completion items from the given completion info, dialect, and
/// catalog, keeping only those matching the partially typed word. Columns and
/// table names come from `scope` when the cursor is inside a query. The result
/// is sorted by [`CompletionKind::sort_priority`], then by match rank.
pub(crate) fn build_completion_items(
    info: &CompletionInfo,
    dialect: &AnyDialect,
    catalog: &Catalog,
    scope: Option<&CursorScope>,
) -> Vec<CompletionEntry> {
    let mut items = candidate_items(info, dialect, catalog, scope);
    if let Some(typed) = &info.prefix {
        items.retain_mut(|item| match match_rank(typed, item.label()) {
            Some(rank) => {
                item.set_match_rank(rank);
                true
            }
            None => false,
        });
        items.sort_by_key(|a| (a.kind().sort_priority(), a.match_rank()));
    }
    items
}

/// How well `typed` matches `label`, ignoring case: 0 for a prefix, 1 for a
/// prefix of an `_`-separated part, 2 for a substring, 3 for a subsequence.
fn match_rank(typed: &str, label: &str) -> Option<u8> {
    let typed = typed.to_ascii_lowercase();
    let label = label.to_ascii_lowercase();
    if label.starts_with(&typed) {
        return Some(0);
    }
    if label.split('_').any(|part| part.starts_with(&typed)) {
        return Some(1);
    }
    if label.contains(&typed) {
        return Some(2);
    }
    let mut label_chars = label.chars();
    typed
        .chars()
        .all(|t| label_chars.by_ref().any(|l| l == t))
        .then_some(3)
}

fn candidate_items(
    info: &CompletionInfo,
    dialect: &AnyDialect,
    catalog: &Catalog,
    scope: Option<&CursorScope>,
) -> Vec<CompletionEntry> {
    let expected_set: HashSet<u32> = info.tokens.iter().map(|&t| u32::from(t)).collect();

    let mut seen: HashSet<String> = HashSet::new();
    let mut items: Vec<CompletionEntry> = Vec::new();

    for entry in dialect.keywords() {
        let code = u32::from(entry.token_type());
        if !expected_set.contains(&code) || !is_suggestable_keyword(entry.keyword()) {
            continue;
        }
        if seen.insert(entry.keyword().to_string()) {
            items.push(CompletionEntry::new(
                entry.keyword().to_string(),
                CompletionKind::Keyword,
            ));
        }
    }

    // When the cursor follows `qualifier.`, only suggest columns from that
    // table — no keywords, functions, or other tables.
    if let Some(qualifier) = info.qualifier.as_ref() {
        items.clear();
        seen.clear();
        for name in catalog.all_column_names(Some(qualifier)) {
            if seen.insert(name.clone()) {
                items.push(CompletionEntry::new(name, CompletionKind::Column));
            }
        }
        return items;
    }

    // Names are only valid where the parser accepts an identifier.
    let id = u32::from(AnyTokenType::from(TokenType::Id));
    if !info.tokens.is_empty() && !expected_set.contains(&id) {
        items.sort_by_key(|a| a.kind().sort_priority());
        return items;
    }

    // A captured scope means the cursor is a column position, whatever
    // context the parser reported.
    let context = if scope.is_some() {
        CompletionContext::Expression
    } else {
        info.context
    };
    match context {
        CompletionContext::TableRef => {
            for name in catalog.all_relation_names() {
                if seen.insert(name.clone()) {
                    items.push(CompletionEntry::new(name, CompletionKind::Table));
                }
            }
        }
        CompletionContext::Expression | CompletionContext::Unknown => {
            for name in catalog.all_function_names() {
                if seen.insert(name.clone()) {
                    items.push(CompletionEntry::new(name, CompletionKind::Function));
                }
            }
            let (columns, tables) = match scope {
                Some(scope) => (scope.columns.clone(), scope.sources.clone()),
                None => (catalog.all_column_names(None), catalog.all_relation_names()),
            };
            for name in columns {
                if seen.insert(name.clone()) {
                    items.push(CompletionEntry::new(name, CompletionKind::Column));
                }
            }
            for name in tables {
                if seen.insert(name.clone()) {
                    items.push(CompletionEntry::new(name, CompletionKind::Table));
                }
            }
        }
    }

    items.sort_by_key(|a| a.kind().sort_priority());
    items
}

#[cfg(test)]
mod tests {
    use super::match_rank;

    #[test]
    fn match_rank_prefers_contiguous_matches() {
        assert_eq!(match_rank("SLICE", "slice"), Some(0));
        assert_eq!(match_rank("slice", "thread_slice"), Some(1));
        assert_eq!(match_rank("lic", "slice"), Some(2));
        assert_eq!(match_rank("slice", "sqlite_schema"), Some(3));
        assert_eq!(match_rank("jn", "JOIN"), Some(3));
        assert_eq!(match_rank("j", "CROSS"), None);
        assert_eq!(match_rank("nj", "JOIN"), None);
    }
}
