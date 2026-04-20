// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Completion item assembly.
//!
//! Converts pre-computed [`CompletionInfo`] plus the active dialect and catalog
//! into a list of [`CompletionEntry`] values. Keywords, columns, tables, and
//! functions are filtered by the expected-token set and context; qualified
//! references (`t.`) return only columns of the qualifier.

use std::collections::HashSet;

use syntaqlite_syntax::util::is_suggestable_keyword;

use crate::analysis::Catalog;
use crate::dialect::AnyDialect;

use crate::analysis::completion::CompletionContext;

use super::{CompletionEntry, CompletionInfo, CompletionKind};

/// Assemble completion items from the given completion info, dialect, and
/// catalog. The result is sorted by [`CompletionKind::sort_priority`].
pub(crate) fn build_completion_items(
    info: &CompletionInfo,
    dialect: &AnyDialect,
    catalog: &Catalog,
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

    match info.context {
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
            for name in catalog.all_column_names(None) {
                if seen.insert(name.clone()) {
                    items.push(CompletionEntry::new(name, CompletionKind::Column));
                }
            }
            for name in catalog.all_relation_names() {
                if seen.insert(name.clone()) {
                    items.push(CompletionEntry::new(name, CompletionKind::Table));
                }
            }
        }
    }

    items.sort_by_key(|a| a.kind().sort_priority());
    items
}
