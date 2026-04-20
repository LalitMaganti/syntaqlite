// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! CTE (`WITH ...`) handling for [`ValidationPass`].
//!
//! Walking a CTE block has its own choreography — extract bindings, register
//! the CTE in the catalog (recursive bindings register before the body),
//! validate declared-vs-actual column counts, and emit per-column definition
//! events to the observer — so it lives in its own file.

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue, NodeFields};
use syntaqlite_syntax::source::DocRange;

use crate::dialect::{FIELD_ABSENT, SemanticRole};
use crate::semantic::ddl::DdlReader;
use crate::semantic::diagnostics::DiagnosticMessage;

use super::ValidationPass;

/// Extracted info for a single CTE binding.
struct CteBindingInfo<'a> {
    name: &'a str,
    /// Document-absolute byte range of the CTE name.  For CTEs inside a
    /// macro expansion, points at the macro call site.
    name_range: DocRange,
    body_id: Option<AnyNodeId>,
    /// Each declared column: text and document-absolute byte range.
    declared_cols: Option<Vec<(&'a str, DocRange)>>,
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
        self.catalog.push_query_scope();
        self.register_cte_bindings(stmt, fields, recursive_idx, bindings_idx);
        self.visit_opt(stmt, Self::field_node_id(fields, body_idx));
        self.catalog.pop_query_scope();
    }

    /// Visit a DML statement whose optional `WITH` prefix is stored inline on
    /// the statement itself. Registers CTE bindings (when present) before
    /// recursing into DML children, so subsequent `SourceRef`s resolve.
    pub(super) fn visit_dml_scope(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        fields: &NodeFields,
        recursive_idx: u8,
        bindings_idx: u8,
    ) {
        let has_ctes = bindings_idx != FIELD_ABSENT;
        if has_ctes {
            self.catalog.push_query_scope();
            self.register_cte_bindings(stmt, fields, recursive_idx, bindings_idx);
        }
        self.scope.push();
        // `register_cte_bindings` already visited each CTE body, so skip the
        // CTE list field when walking the remaining DML children to avoid
        // double-visitation.
        self.visit_dml_children_except_ctes(stmt, fields, bindings_idx);
        self.scope.pop();
        if has_ctes {
            self.catalog.pop_query_scope();
        }
    }

    fn visit_dml_children_except_ctes(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        fields: &NodeFields,
        bindings_idx: u8,
    ) {
        let skip = bindings_idx as usize;
        let mut child_ids: Vec<AnyNodeId> = Vec::new();
        for idx in 0..fields.len() {
            if idx == skip {
                continue;
            }
            if let FieldValue::NodeId(child_id) = fields[idx]
                && !child_id.is_null()
            {
                if let Some(children) = stmt.list_children(child_id) {
                    child_ids.extend(children.iter().copied().filter(|id| !id.is_null()));
                } else {
                    child_ids.push(child_id);
                }
            }
        }
        for child in child_ids {
            self.visit(stmt, child);
        }
    }

    /// Walk a CTE binding list and register each binding in the current
    /// catalog query scope. Caller is responsible for pushing/popping the
    /// catalog query scope around this call.
    fn register_cte_bindings(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        fields: &NodeFields,
        recursive_idx: u8,
        bindings_idx: u8,
    ) {
        if bindings_idx == FIELD_ABSENT {
            return;
        }
        let is_recursive = recursive_idx != FIELD_ABSENT
            && matches!(fields[recursive_idx as usize], FieldValue::Bool(true));
        let cte_ids = Self::field_node_id(fields, bindings_idx)
            .and_then(|id| stmt.list_children(id))
            .unwrap_or(&[]);

        for &cte_id in cte_ids {
            let Some(binding) = self.extract_cte_binding(stmt, cte_id) else {
                continue;
            };

            // For recursive CTEs, register the name before visiting the body.
            if is_recursive && !binding.name.is_empty() {
                let cols = binding
                    .declared_cols
                    .as_ref()
                    .map(|v| v.iter().map(|(s, _)| s.to_string()).collect());
                self.catalog.add_query_table(binding.name, cols);
            }

            self.scope.push();
            self.visit_opt(stmt, binding.body_id);
            self.scope.pop();

            if binding.name.is_empty() {
                continue;
            }

            if self.observer.wants_definitions() {
                self.observer
                    .on_relation_definition(binding.name, binding.name_range);
            }

            let cols = if let Some(ref declared) = binding.declared_cols {
                let col_names: Vec<&str> = declared.iter().map(|(s, _)| *s).collect();
                self.check_cte_column_count(
                    stmt,
                    binding.name,
                    binding.name_range,
                    &col_names,
                    binding.body_id,
                );
                if self.observer.wants_definitions() {
                    for &(col_name, col_range) in declared {
                        self.observer
                            .on_column_definition(binding.name, col_name, col_range);
                    }
                }
                Some(declared.iter().map(|(s, _)| s.to_string()).collect())
            } else {
                if self.observer.wants_definitions() {
                    self.emit_select_column_definitions(stmt, binding.body_id, binding.name);
                }
                binding
                    .body_id
                    .and_then(|id| DdlReader::new(stmt, self.roles).columns_from_select(id))
            };
            self.catalog.add_query_table(binding.name, cols);
        }
    }

    /// Emit definition events for columns inferred from a SELECT body.
    ///
    /// For `WITH foo AS (SELECT 1 AS a, 2 AS b)`, emits events for the alias
    /// names `a` and `b` so observers can wire up go-to-definition.
    fn emit_select_column_definitions(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        body_id: Option<AnyNodeId>,
        table_name: &str,
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
            let (alias_text, alias_range) = Self::name_text(stmt, alias_node);
            if !alias_text.is_empty() {
                self.observer
                    .on_column_definition(table_name, alias_text, alias_range);
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
                let (_, range) = stmt.span_text_abs(sp);
                (name, range)
            }
            _ => ("", DocRange::default()),
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
    ) -> Option<Vec<(&'b str, DocRange)>> {
        if cols_idx == FIELD_ABSENT {
            return None;
        }
        let list_id = Self::field_node_id(fields, cols_idx)?;
        let children = stmt.list_children(list_id)?;
        let names: Vec<(&'b str, DocRange)> = children
            .iter()
            .copied()
            .filter(|id| !id.is_null())
            .map(|id| Self::name_text(stmt, Some(id)))
            .filter(|(s, _)| !s.is_empty())
            .collect();
        if names.is_empty() { None } else { Some(names) }
    }

    /// Emit a diagnostic if the CTE body has a different column count than declared.
    fn check_cte_column_count(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        cte_name: &str,
        cte_name_range: DocRange,
        declared: &[&str],
        body_id: Option<AnyNodeId>,
    ) {
        if let Some(actual) = self.count_result_columns(stmt, body_id)
            && actual != declared.len()
        {
            self.emit_at(
                cte_name_range,
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
