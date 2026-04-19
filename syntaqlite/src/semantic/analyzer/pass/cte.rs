// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! CTE (`WITH ...`) handling for [`ValidationPass`].
//!
//! Walking a CTE block has its own choreography — extract bindings, register
//! the CTE in the catalog (recursive bindings register before the body),
//! validate declared-vs-actual column counts, and record per-column LSP
//! definition offsets — so it lives in its own file.

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue, NodeFields};

use crate::dialect::{FIELD_ABSENT, SemanticRole};
use crate::semantic::ddl::DdlReader;
use crate::semantic::diagnostics::DiagnosticMessage;

use super::ValidationPass;

/// Extracted info for a single CTE binding.
struct CteBindingInfo<'a> {
    name: &'a str,
    /// Source-level byte range of the CTE name (start, end).  For CTEs
    /// inside a macro expansion, points at the macro call site.
    name_range: (usize, usize),
    body_id: Option<AnyNodeId>,
    /// Each declared column: text and source-level byte range.
    declared_cols: Option<Vec<(&'a str, usize, usize)>>,
}

impl ValidationPass<'_> {
    pub(super) fn visit_cte_scope(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        fields: &NodeFields,
        recursive_idx: u8,
        bindings_idx: u8,
        body_idx: u8,
    ) {
        let is_recursive = matches!(fields[recursive_idx as usize], FieldValue::Bool(true));
        let cte_ids = Self::field_node_id(fields, bindings_idx)
            .and_then(|id| stmt.list_children(id))
            .unwrap_or(&[]);

        // Push a catalog scope so CTE names are resolvable as table names in
        // FROM clauses.  This is purely for relation-name resolution — CTE
        // columns only become active when a CTE is actually referenced in FROM
        // (handled by visit_source_ref → scope.add_table).
        self.catalog.push_query_scope();

        for &cte_id in cte_ids {
            let Some(binding) = self.extract_cte_binding(stmt, cte_id) else {
                continue;
            };

            // For recursive CTEs, register the name before visiting the body.
            if is_recursive && !binding.name.is_empty() {
                let cols = binding
                    .declared_cols
                    .as_ref()
                    .map(|v| v.iter().map(|(s, _, _)| s.to_string()).collect());
                self.catalog.add_query_table(binding.name, cols);
            }

            self.scope.push();
            self.visit_opt(stmt, binding.body_id);
            self.scope.pop();

            if binding.name.is_empty() {
                continue;
            }

            // Record CTE definition offset for go-to-definition.
            #[cfg(feature = "lsp")]
            self.definition_offsets
                .insert(binding.name.to_ascii_lowercase(), binding.name_range);

            // Determine the CTE's column list and register it in the catalog.
            #[cfg(feature = "lsp")]
            let cte_key = binding.name.to_ascii_lowercase();
            let cols = if let Some(ref declared) = binding.declared_cols {
                let col_names: Vec<&str> = declared.iter().map(|(s, _, _)| *s).collect();
                self.check_cte_column_count(
                    stmt,
                    binding.name,
                    binding.name_range,
                    &col_names,
                    binding.body_id,
                );
                #[cfg(feature = "lsp")]
                for &(col_name, col_start, col_end) in declared {
                    let key = format!("{cte_key}.{}", col_name.to_ascii_lowercase());
                    self.definition_offsets.insert(key, (col_start, col_end));
                }
                Some(declared.iter().map(|(s, _, _)| s.to_string()).collect())
            } else {
                #[cfg(feature = "lsp")]
                self.record_select_column_offsets(stmt, binding.body_id, &cte_key);
                binding
                    .body_id
                    .and_then(|id| DdlReader::new(stmt, self.roles).columns_from_select(id))
            };
            self.catalog.add_query_table(binding.name, cols);
        }

        self.visit_opt(stmt, Self::field_node_id(fields, body_idx));
        self.catalog.pop_query_scope();
    }

    /// Record definition offsets for columns inferred from a SELECT body.
    ///
    /// For `WITH foo AS (SELECT 1 AS a, 2 AS b)`, records offsets for the
    /// alias names `a` and `b` so go-to-definition can jump to them.
    #[cfg(feature = "lsp")]
    fn record_select_column_offsets(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        body_id: Option<AnyNodeId>,
        table_key: &str,
    ) {
        let Some(body_id) = body_id else { return };
        let Some((tag, fields)) = stmt.extract_fields(body_id) else {
            return;
        };
        let Some(&SemanticRole::Query {
            columns: cols_idx, ..
        }) = self.roles.get(u32::from(tag) as usize)
        else {
            return;
        };
        let Some(list_id) = Self::field_node_id(&fields, cols_idx) else {
            return;
        };
        let Some(children) = stmt.list_children(list_id) else {
            return;
        };

        for &child_id in children {
            if child_id.is_null() {
                continue;
            }
            let Some((child_tag, child_fields)) = stmt.extract_fields(child_id) else {
                continue;
            };
            let SemanticRole::ResultColumn {
                alias: alias_idx, ..
            } = self
                .roles
                .get(u32::from(child_tag) as usize)
                .copied()
                .unwrap_or(SemanticRole::Transparent)
            else {
                continue;
            };
            let alias_node = Self::field_node_id(&child_fields, alias_idx);
            let (alias_text, alias_start, alias_end) = Self::name_text(stmt, alias_node);
            if !alias_text.is_empty() {
                let key = format!("{table_key}.{}", alias_text.to_ascii_lowercase());
                self.definition_offsets
                    .insert(key, (alias_start, alias_end));
            }
        }
    }

    fn extract_cte_binding<'b>(
        &self,
        stmt: &mut AnyParsedStatement<'b>,
        cte_id: AnyNodeId,
    ) -> Option<CteBindingInfo<'b>> {
        if cte_id.is_null() {
            return None;
        }
        let (tag, fields) = stmt.extract_fields(cte_id)?;
        let SemanticRole::CteBinding {
            name: name_idx,
            columns: cols_idx,
            body: body_idx,
        } = self
            .roles
            .get(u32::from(tag) as usize)
            .copied()
            .unwrap_or(SemanticRole::Transparent)
        else {
            return None;
        };

        let (name, name_range) = match fields[name_idx as usize] {
            FieldValue::Span(sp) => {
                let name = stmt.span_expanded_text(sp);
                let (_, start, end) = stmt.span_text_abs(sp);
                (name, (start, end))
            }
            _ => ("", (0, 0)),
        };
        Some(CteBindingInfo {
            name,
            name_range,
            body_id: Self::field_node_id(&fields, body_idx),
            declared_cols: Self::extract_declared_cols(stmt, &fields, cols_idx),
        })
    }

    /// Extract declared CTE column names from the column list field.
    fn extract_declared_cols<'b>(
        stmt: &mut AnyParsedStatement<'b>,
        fields: &NodeFields,
        cols_idx: u8,
    ) -> Option<Vec<(&'b str, usize, usize)>> {
        if cols_idx == FIELD_ABSENT {
            return None;
        }
        let list_id = Self::field_node_id(fields, cols_idx)?;
        let children = stmt.list_children(list_id)?;
        let names: Vec<(&'b str, usize, usize)> = children
            .iter()
            .copied()
            .filter(|id| !id.is_null())
            .map(|id| Self::name_text(stmt, Some(id)))
            .filter(|(s, _, _)| !s.is_empty())
            .collect();
        if names.is_empty() { None } else { Some(names) }
    }

    /// Emit a diagnostic if the CTE body has a different column count than declared.
    fn check_cte_column_count(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        cte_name: &str,
        cte_name_range: (usize, usize),
        declared: &[&str],
        body_id: Option<AnyNodeId>,
    ) {
        if let Some(actual) = self.count_result_columns(stmt, body_id)
            && actual != declared.len()
        {
            self.emit_at(
                cte_name_range.0,
                cte_name_range.1,
                DiagnosticMessage::CteColumnCountMismatch {
                    name: cte_name.to_string(),
                    declared: declared.len(),
                    actual,
                },
                None,
            );
        }
    }

    /// Count the result columns of a SELECT body node.
    ///
    /// Returns `None` if the body is not a plain `SelectStmt` or if any result
    /// column uses `*` (wildcard), which would require catalog expansion to count.
    fn count_result_columns(
        &self,
        stmt: &mut AnyParsedStatement<'_>,
        body_id: Option<AnyNodeId>,
    ) -> Option<usize> {
        let body_id = body_id?;
        let (body_tag, body_fields) = stmt.extract_fields(body_id)?;
        let SemanticRole::Query {
            columns: cols_idx, ..
        } = self
            .roles
            .get(u32::from(body_tag) as usize)
            .copied()
            .unwrap_or(SemanticRole::Transparent)
        else {
            return None;
        };

        let list_id = Self::field_node_id(&body_fields, cols_idx)?;
        let children = stmt.list_children(list_id)?;

        let mut count = 0usize;
        for child_id in children.iter().copied() {
            if child_id.is_null() {
                continue;
            }
            let Some((child_tag, child_fields)) = stmt.extract_fields(child_id) else {
                continue;
            };
            let SemanticRole::ResultColumn {
                flags: flags_idx, ..
            } = self
                .roles
                .get(u32::from(child_tag) as usize)
                .copied()
                .unwrap_or(SemanticRole::Transparent)
            else {
                continue;
            };
            // STAR flag (bit 0) means wildcard — skip count check entirely.
            if let FieldValue::Flags(f) = child_fields[flags_idx as usize]
                && f & 1 != 0
            {
                return None;
            }
            count += 1;
        }
        Some(count)
    }
}
