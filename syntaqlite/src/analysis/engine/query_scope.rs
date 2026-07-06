// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Scoped FROM/JOIN table tracking for column resolution.
//!
//! Completely separate from [`Catalog`](super::super::catalog::Catalog)
//! (schema-level). Frames are pushed/popped as the validator enters/leaves
//! SELECT statements, subqueries, and DML blocks. Inner frames are searched
//! first (correlated subqueries). CTE definitions are never stored here —
//! CTEs only contribute columns when they appear in a FROM clause.

use std::collections::HashMap;

use crate::analysis::catalog::ColumnResolution;
use crate::analysis::name_key::NameKey;

use super::helpers::is_rowid_alias;

#[derive(Clone, Copy, PartialEq, Eq)]
pub(crate) enum RowIdPolicy {
    /// Normal table — `rowid`, `oid`, `_rowid_` are valid implicit columns.
    WithRowId,
    /// WITHOUT ROWID table — no implicit rowid column exists.
    WithoutRowId,
}

impl From<bool> for RowIdPolicy {
    fn from(without_rowid: bool) -> Self {
        if without_rowid {
            Self::WithoutRowId
        } else {
            Self::WithRowId
        }
    }
}

pub(crate) struct ActiveTable {
    /// `None` = table exists but column list is unknown (accept any ref).
    columns: Option<Vec<String>>,
    rowid: RowIdPolicy,
}

impl ActiveTable {
    fn has_column(&self, column: &str) -> bool {
        match &self.columns {
            Some(cs) => {
                cs.iter().any(|c| c.eq_ignore_ascii_case(column))
                    || (self.rowid == RowIdPolicy::WithRowId && is_rowid_alias(column))
            }
            None => true,
        }
    }
}

#[derive(Default)]
pub(crate) struct ScopeFrame {
    /// Named tables/aliases, keyed by normalized name.
    named: HashMap<NameKey, ActiveTable>,
    /// Anonymous sources (unaliased subqueries). Participate in unqualified
    /// column resolution but cannot be referenced by name.
    anonymous: Vec<ActiveTable>,
}

#[derive(Default)]
pub(crate) struct QueryScope {
    frames: Vec<ScopeFrame>,
}

impl QueryScope {
    pub(crate) fn has_frames(&self) -> bool {
        !self.frames.is_empty()
    }

    pub(crate) fn push(&mut self) {
        self.frames.push(ScopeFrame::default());
    }

    pub(crate) fn pop(&mut self) {
        self.frames.pop();
    }

    pub(crate) fn add_table(
        &mut self,
        name: &str,
        columns: Option<Vec<String>>,
        rowid: RowIdPolicy,
    ) {
        if let Some(frame) = self.frames.last_mut() {
            frame
                .named
                .insert(NameKey::new(name), ActiveTable { columns, rowid });
        }
    }

    pub(crate) fn add_anonymous(&mut self, columns: Option<Vec<String>>) {
        if let Some(frame) = self.frames.last_mut() {
            frame.anonymous.push(ActiveTable {
                columns,
                rowid: RowIdPolicy::WithRowId,
            });
        }
    }

    pub(crate) fn resolve_column(&self, table: Option<&NameKey>, column: &str) -> ColumnResolution {
        if let Some(tbl) = table {
            self.resolve_qualified(tbl, column)
        } else {
            self.resolve_unqualified(column)
        }
    }

    fn resolve_qualified(&self, table: &NameKey, column: &str) -> ColumnResolution {
        for frame in self.frames.iter().rev() {
            let Some(entry) = frame.named.get(table) else {
                continue;
            };
            if entry.has_column(column) {
                return ColumnResolution::Found {
                    table: table.as_str().to_string(),
                    all_columns: entry.columns.clone().unwrap_or_default(),
                };
            }
            return match &entry.columns {
                Some(_) => ColumnResolution::TableFoundColumnMissing,
                None => ColumnResolution::Found {
                    table: table.as_str().to_string(),
                    all_columns: Vec::new(),
                },
            };
        }
        ColumnResolution::TableNotFound
    }

    fn resolve_unqualified(&self, column: &str) -> ColumnResolution {
        for frame in self.frames.iter().rev() {
            let mut frame_has_unknown = false;

            for (tbl_name, entry) in &frame.named {
                if entry.has_column(column) {
                    return ColumnResolution::Found {
                        table: tbl_name.as_str().to_string(),
                        all_columns: entry.columns.clone().unwrap_or_default(),
                    };
                }
                if entry.columns.is_none() {
                    frame_has_unknown = true;
                }
            }

            for entry in &frame.anonymous {
                if entry.has_column(column) {
                    return ColumnResolution::Found {
                        table: String::new(),
                        all_columns: entry.columns.clone().unwrap_or_default(),
                    };
                }
                if entry.columns.is_none() {
                    frame_has_unknown = true;
                }
            }

            // A source with unknown columns in this frame could own the column —
            // don't leak into outer scopes.
            if frame_has_unknown {
                return ColumnResolution::Found {
                    table: String::new(),
                    all_columns: Vec::new(),
                };
            }
        }
        ColumnResolution::NotFound
    }

    /// Columns of the sole named table in the innermost frame, if there is
    /// exactly one (the DML target when called from an INSERT's upsert
    /// clause). `None` = unknown columns (accept any ref).
    pub(crate) fn sole_named_table_columns(&self) -> Option<Vec<String>> {
        let frame = self.frames.last()?;
        if frame.named.len() != 1 || !frame.anonymous.is_empty() {
            return None;
        }
        frame.named.values().next()?.columns.clone()
    }

    /// Collect all source names in the active frames, for fuzzy
    /// suggestions on an unresolved qualifier.
    pub(crate) fn all_source_names(&self) -> Vec<String> {
        let mut names: Vec<String> = self
            .frames
            .iter()
            .flat_map(|f| f.named.keys())
            .map(|k| k.as_str().to_string())
            .collect();
        names.sort_unstable();
        names.dedup();
        names
    }

    /// Collect all known column names (for fuzzy suggestions).
    pub(crate) fn all_column_names(&self, table: Option<&str>) -> Vec<String> {
        let mut names: Vec<String> = Vec::new();
        for frame in self.frames.iter().rev() {
            for (tbl_name, entry) in &frame.named {
                if table.is_none_or(|t| tbl_name.as_str().eq_ignore_ascii_case(t))
                    && let Some(cs) = &entry.columns
                {
                    names.extend(cs.iter().map(|c| c.to_ascii_lowercase()));
                }
            }
            if table.is_none() {
                for entry in &frame.anonymous {
                    if let Some(cs) = &entry.columns {
                        names.extend(cs.iter().map(|c| c.to_ascii_lowercase()));
                    }
                }
            }
        }
        names.sort_unstable();
        names.dedup();
        names
    }
}
