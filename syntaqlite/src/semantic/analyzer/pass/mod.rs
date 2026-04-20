// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! The statement-level validation pass: visits an AST once, emitting
//! diagnostics and forwarding resolution events to the observer as it goes.

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue, NodeFields};
use syntaqlite_syntax::source::{DocRange, LayerRange};

use crate::dialect::{AnyDialect, FIELD_ABSENT, SemanticRole};
use crate::semantic::catalog::{Catalog, ColumnResolution, FunctionCheckResult};
use crate::semantic::ddl::DdlReader;
use crate::semantic::diagnostics::{Diagnostic, DiagnosticMessage, Help};
use crate::semantic::fuzzy::best_suggestion;
use crate::semantic::observer::AnalysisObserver;
use crate::semantic::{CheckConfig, CheckLevel, ValidationConfig};

use super::query_scope::{QueryScope, RowIdPolicy};

mod cte;

pub(super) struct ValidationPass<'a> {
    roles: &'static [SemanticRole],
    catalog: &'a mut Catalog,
    config: &'a ValidationConfig,
    diagnostics: &'a mut Vec<Diagnostic>,
    observer: &'a mut dyn AnalysisObserver,
    scope: QueryScope,
}

impl CheckConfig {
    /// Get the check level for a diagnostic message's category.
    pub(crate) fn level_for(self, message: &DiagnosticMessage) -> CheckLevel {
        match message {
            DiagnosticMessage::UnknownTable { .. } | DiagnosticMessage::UnknownModule { .. } => {
                self.unknown_table
            }
            DiagnosticMessage::UnknownColumn { .. } => self.unknown_column,
            DiagnosticMessage::UnknownFunction { .. } => self.unknown_function,
            DiagnosticMessage::FunctionArity { .. } => self.function_arity,
            DiagnosticMessage::CteColumnCountMismatch { .. } => self.cte_columns,
            DiagnosticMessage::ParseError(_) => self.parse_errors,
        }
    }
}

impl<'a> ValidationPass<'a> {
    /// Push a diagnostic anchored to a span field of a node.  `text_range`
    /// is `(start, end)` authored byte offsets from the field; `node_id` +
    /// `field_idx` are used to build the macro expansion traceback (if
    /// any).  Severity is determined entirely by the check level —
    /// callers do not specify it.
    fn emit(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        node_id: AnyNodeId,
        field_idx: u8,
        range: DocRange,
        message: DiagnosticMessage,
        help: Option<Help>,
    ) {
        let Some(severity) = self.config.checks().level_for(&message).to_severity() else {
            return;
        };
        let frames = stmt
            .traceback(node_id, field_idx)
            .map(|f| crate::semantic::diagnostics::DiagnosticFrame {
                buffer: f.snippet.as_str().to_string(),
                range: LayerRange::from_offset_len(f.offset_in_snippet, f.length_in_snippet),
            })
            .collect::<Vec<_>>();
        // Only attach if there's actual expansion (more than 1 frame).
        let expansion_frames = if frames.len() > 1 { frames } else { Vec::new() };
        self.diagnostics.push(Diagnostic {
            range,
            message,
            severity,
            help,
            expansion_frames,
        });
    }

    /// Push a diagnostic with an explicit source range and no expansion
    /// traceback.  Use only when there is no associated span field
    /// (e.g. computed-from-tokens locations).
    fn emit_at(&mut self, range: DocRange, message: DiagnosticMessage, help: Option<Help>) {
        if let Some(severity) = self.config.checks().level_for(&message).to_severity() {
            self.diagnostics.push(Diagnostic {
                range,
                message,
                severity,
                help,
                expansion_frames: Vec::new(),
            });
        }
    }

    pub(super) fn run<'b>(
        stmt: &mut AnyParsedStatement<'b>,
        root: AnyNodeId,
        dialect: &AnyDialect,
        catalog: &'a mut Catalog,
        config: &'a ValidationConfig,
        diagnostics: &'a mut Vec<Diagnostic>,
        observer: &'a mut dyn AnalysisObserver,
    ) {
        let roles = dialect.roles();
        let mut pass = ValidationPass {
            roles,
            catalog,
            config,
            diagnostics,
            observer,
            scope: QueryScope::default(),
        };
        pass.visit(stmt, root);
    }

    // ── Core visitor ─────────────────────────────────────────────────────────

    fn visit(&mut self, stmt: &mut AnyParsedStatement<'_>, node_id: AnyNodeId) {
        if node_id.is_null() {
            return;
        }
        // List nodes have no role — visit their elements directly.
        if let Some(children) = stmt.list_children(node_id) {
            for child in children.iter().copied() {
                if !child.is_null() {
                    self.visit(stmt, child);
                }
            }
            return;
        }
        let Some((tag, fields)) = stmt.extract_fields(node_id) else {
            return;
        };
        let idx = u32::from(tag) as usize;
        let role = self
            .roles
            .get(idx)
            .copied()
            .unwrap_or(SemanticRole::Transparent);

        match role {
            // Catalog roles are handled in the accumulation pass, but we
            // still recurse into the SELECT body to validate table/column refs.
            SemanticRole::DefineTable { select, .. }
            | SemanticRole::DefineView { select, .. }
            | SemanticRole::DefineFunction { select, .. } => {
                if select != FIELD_ABSENT {
                    self.visit_opt(stmt, Self::field_node_id(&fields, select));
                }
            }
            SemanticRole::ReturnSpec { .. } | SemanticRole::Import { .. } => {}

            // Transparent: recurse into children without special handling.
            // ColumnDef and ResultColumn have no validation logic yet — child
            // expressions are reached via transparent traversal.
            SemanticRole::Transparent
            | SemanticRole::ColumnDef { .. }
            | SemanticRole::ResultColumn { .. }
            | SemanticRole::CteBinding { .. } => self.visit_children(stmt, node_id),

            SemanticRole::Call { name, args } => {
                self.visit_call(stmt, node_id, &fields, name, args);
            }
            SemanticRole::ColumnRef { column, table } => {
                self.visit_column_ref(stmt, node_id, &fields, column, table);
            }
            SemanticRole::SourceRef { name, alias, .. } => {
                self.visit_source_ref(stmt, node_id, &fields, name, alias);
            }
            SemanticRole::ScopedSource { body, alias } => {
                self.visit_scoped_source(stmt, &fields, body, alias);
            }
            SemanticRole::Query {
                from,
                columns,
                where_clause,
                groupby,
                having,
                orderby,
                limit_clause,
            } => self.visit_query(
                stmt,
                &fields,
                from,
                columns,
                where_clause,
                groupby,
                having,
                orderby,
                limit_clause,
            ),
            SemanticRole::CteScope {
                recursive,
                bindings,
                body,
            } => self.visit_cte_scope(stmt, &fields, recursive, bindings, body),
            SemanticRole::TriggerScope {
                target: _,
                when,
                body,
            } => {
                self.visit_trigger_scope(stmt, &fields, when, body);
            }
            SemanticRole::DmlScope {
                with_recursive,
                with_ctes,
            } => {
                self.visit_dml_scope(stmt, &fields, with_recursive, with_ctes);
            }
        }
    }

    fn visit_children(&mut self, stmt: &mut AnyParsedStatement<'_>, node_id: AnyNodeId) {
        // `child_node_ids` owns its data (no borrow on `stmt`), so we
        // can hold it while recursively `&mut`-borrowing `stmt`.
        for child in stmt.child_node_ids(node_id) {
            if !child.is_null() {
                self.visit(stmt, child);
            }
        }
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    fn field_node_id(fields: &NodeFields, idx: u8) -> Option<AnyNodeId> {
        match fields[idx as usize] {
            FieldValue::NodeId(id) if !id.is_null() => Some(id),
            _ => None,
        }
    }

    fn visit_opt(&mut self, stmt: &mut AnyParsedStatement<'_>, id: Option<AnyNodeId>) {
        if let Some(id) = id {
            self.visit(stmt, id);
        }
    }

    /// Extract source text and source-level byte range from a `Name` node
    /// (`IdentName` or `Error`).  Both node kinds store their span at field 0.
    /// For spans inside a macro expansion, the returned range points at the
    /// macro call site.
    fn name_text<'b>(
        stmt: &AnyParsedStatement<'b>,
        node_id: Option<AnyNodeId>,
    ) -> (&'b str, DocRange) {
        let Some(node_id) = node_id else {
            return ("", DocRange::default());
        };
        let Some((_, fields)) = stmt.extract_fields(node_id) else {
            return ("", DocRange::default());
        };
        if fields.is_empty() {
            return ("", DocRange::default());
        }
        match fields[0] {
            FieldValue::Span(sp) => {
                // Identifier spelling uses the post-expansion bytes;
                // source position uses the authored byte range.
                let name = stmt.span_expanded_text(sp);
                let (_, range) = stmt.span_text_abs(sp);
                (name, range)
            }
            _ => ("", DocRange::default()),
        }
    }

    // ── Role handlers ─────────────────────────────────────────────────────────

    fn visit_source_ref(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        node_id: AnyNodeId,
        fields: &NodeFields,
        name_idx: u8,
        alias_idx: u8,
    ) {
        let FieldValue::Span(sp) = fields[name_idx as usize] else {
            return;
        };
        if sp.is_empty() {
            return;
        }
        // Identifier spelling comes from post-expansion bytes — a
        // reference produced from a macro body resolves by its expanded
        // name — while source position walks the expansion chain back
        // to the authored byte range so diagnostics anchor at the macro
        // call site.
        let name = stmt.span_expanded_text(sp);
        let (_, range) = stmt.span_text_abs(sp);

        let is_known =
            self.catalog.resolve_relation(name) || self.catalog.resolve_table_function(name);
        if !is_known {
            let mut candidates = self.catalog.all_relation_names();
            candidates.extend(self.catalog.all_table_function_names());
            let suggestion = best_suggestion(name, &candidates, self.config.suggestion_threshold());
            self.emit(
                stmt,
                node_id,
                name_idx,
                range,
                DiagnosticMessage::UnknownTable {
                    name: name.to_string(),
                },
                suggestion.map(Help::Suggestion),
            );
        }

        let (alias, _) = Self::name_text(stmt, Self::field_node_id(fields, alias_idx));
        let scope_name = if alias.is_empty() { name } else { alias };
        let (columns, without_rowid) = self.catalog.table_source_info(name);

        if is_known && self.observer.wants_references() {
            self.observer
                .on_table_reference(range, name, columns.as_deref());
        }

        self.scope
            .add_table(scope_name, columns, without_rowid.into());
    }

    fn visit_call(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        node_id: AnyNodeId,
        fields: &NodeFields,
        name_idx: u8,
        args_idx: u8,
    ) {
        if let FieldValue::Span(sp) = fields[name_idx as usize]
            && !sp.is_empty()
        {
            let name = stmt.span_expanded_text(sp);
            let (_, range) = stmt.span_text_abs(sp);
            let args_id = Self::field_node_id(fields, args_idx);
            let arg_count = args_id
                .and_then(|id| stmt.list_children(id))
                .map_or(0, <[_]>::len);
            match self.catalog.check_function(name, arg_count) {
                FunctionCheckResult::Ok => {
                    if self.observer.wants_references()
                        && let Some((cat, arities)) = self.catalog.function_signature(name)
                    {
                        self.observer
                            .on_function_reference(range, name, cat, &arities);
                    }
                }
                FunctionCheckResult::Unknown => {
                    let candidates = self.catalog.all_function_names();
                    let suggestion =
                        best_suggestion(name, &candidates, self.config.suggestion_threshold());
                    self.emit(
                        stmt,
                        node_id,
                        name_idx,
                        range,
                        DiagnosticMessage::UnknownFunction {
                            name: name.to_string(),
                        },
                        suggestion.map(Help::Suggestion),
                    );
                }
                FunctionCheckResult::WrongArity { expected } => {
                    self.emit(
                        stmt,
                        node_id,
                        name_idx,
                        range,
                        DiagnosticMessage::FunctionArity {
                            name: name.to_string(),
                            expected,
                            got: arg_count,
                        },
                        None,
                    );
                }
            }
        }
        self.visit_children(stmt, node_id);
    }

    fn visit_column_ref(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        node_id: AnyNodeId,
        fields: &NodeFields,
        column_idx: u8,
        table_idx: u8,
    ) {
        // ColumnRef outside any query scope (e.g. ATTACH ... AS scratch)
        // is just a bare identifier — skip validation.
        if !self.scope.has_frames() {
            return;
        }
        let FieldValue::Span(col_sp) = fields[column_idx as usize] else {
            return;
        };
        if col_sp.is_empty() {
            return;
        }
        let column = stmt.span_expanded_text(col_sp);
        let table = match fields[table_idx as usize] {
            FieldValue::Span(sp) if !sp.is_empty() => Some(stmt.span_expanded_text(sp)),
            _ => None,
        };
        let (_, range) = stmt.span_text_abs(col_sp);

        match self.scope.resolve_column(table, column) {
            ColumnResolution::Found {
                table: resolved_table,
                all_columns,
            } => {
                if self.observer.wants_references() && !resolved_table.is_empty() {
                    self.observer
                        .on_column_reference(range, &resolved_table, column, &all_columns);
                }
            }
            ColumnResolution::TableNotFound => {}
            ColumnResolution::TableFoundColumnMissing => {
                let tbl = table.expect("qualifier present when TableFoundColumnMissing");
                let candidates = self.scope.all_column_names(Some(tbl));
                let suggestion =
                    best_suggestion(column, &candidates, self.config.suggestion_threshold());
                self.emit(
                    stmt,
                    node_id,
                    column_idx,
                    range,
                    DiagnosticMessage::UnknownColumn {
                        column: column.to_string(),
                        table: Some(tbl.to_string()),
                    },
                    suggestion.map(Help::Suggestion),
                );
            }
            ColumnResolution::NotFound => {
                // SQLite resolves bare TRUE/FALSE identifiers to integer
                // literals 1/0 (see sqlite3ExprIdToTrueFalse), so they are
                // valid even when no column by that name exists.
                if column.eq_ignore_ascii_case("true") || column.eq_ignore_ascii_case("false") {
                    return;
                }
                let candidates = self.scope.all_column_names(None);
                let suggestion =
                    best_suggestion(column, &candidates, self.config.suggestion_threshold());
                self.emit(
                    stmt,
                    node_id,
                    column_idx,
                    range,
                    DiagnosticMessage::UnknownColumn {
                        column: column.to_string(),
                        table: None,
                    },
                    suggestion.map(Help::Suggestion),
                );
            }
        }
    }

    fn visit_scoped_source(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        fields: &NodeFields,
        body_idx: u8,
        alias_idx: u8,
    ) {
        self.scope.push();
        self.visit_opt(stmt, Self::field_node_id(fields, body_idx));
        self.scope.pop();

        let (alias, _) = Self::name_text(stmt, Self::field_node_id(fields, alias_idx));
        let cols = Self::field_node_id(fields, body_idx)
            .and_then(|id| DdlReader::new(stmt, self.roles).columns_from_select(id));
        if alias.is_empty() {
            self.scope.add_anonymous(cols);
        } else {
            self.scope.add_table(alias, cols, RowIdPolicy::WithRowId);
        }
    }

    #[expect(clippy::too_many_arguments)]
    fn visit_query(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        fields: &NodeFields,
        from: u8,
        columns: u8,
        where_clause: u8,
        groupby: u8,
        having: u8,
        orderby: u8,
        limit_clause: u8,
    ) {
        // Push a fresh scope so that tables registered by visit_source_ref
        // (via add_query_table) are visible when we visit SELECT columns,
        // WHERE, ORDER BY, etc.  Without this, add_query_table is a silent
        // no-op when no query scope frame exists (e.g. at the top level),
        // causing column refs against unknown tables to be spuriously flagged.
        self.scope.push();
        self.visit_opt(stmt, Self::field_node_id(fields, from));
        self.visit_opt(stmt, Self::field_node_id(fields, columns));

        // Collect SELECT aliases so they are visible in WHERE, GROUP BY,
        // HAVING, ORDER BY, and LIMIT — matching SQLite's resolution rules.
        let aliases = self.collect_select_aliases(stmt, fields, columns);
        if !aliases.is_empty() {
            self.scope
                .add_table("", Some(aliases), RowIdPolicy::WithRowId);
        }

        for idx in [where_clause, groupby, having, orderby, limit_clause] {
            self.visit_opt(stmt, Self::field_node_id(fields, idx));
        }
        self.scope.pop();
    }

    /// Extract alias names from the SELECT result column list.
    fn collect_select_aliases(
        &self,
        stmt: &mut AnyParsedStatement<'_>,
        fields: &NodeFields,
        columns_idx: u8,
    ) -> Vec<String> {
        let mut aliases = Vec::new();
        let Some(list_id) = Self::field_node_id(fields, columns_idx) else {
            return aliases;
        };
        let Some(children) = stmt.list_children(list_id) else {
            return aliases;
        };
        for &child_id in children {
            if child_id.is_null() {
                continue;
            }
            let Some((child_tag, child_fields)) = stmt.extract_fields(child_id) else {
                continue;
            };
            let child_role = self
                .roles
                .get(u32::from(child_tag) as usize)
                .copied()
                .unwrap_or(SemanticRole::Transparent);
            let SemanticRole::ResultColumn {
                alias: alias_idx, ..
            } = child_role
            else {
                continue;
            };
            let alias_node = Self::field_node_id(&child_fields, alias_idx);
            let (alias_text, _) = Self::name_text(stmt, alias_node);
            if !alias_text.is_empty() {
                aliases.push(alias_text.to_string());
            }
        }
        aliases
    }
    fn visit_trigger_scope(
        &mut self,
        stmt: &mut AnyParsedStatement<'_>,
        fields: &NodeFields,
        when_idx: u8,
        body_idx: u8,
    ) {
        self.scope.push();
        self.scope.add_table("OLD", None, RowIdPolicy::WithRowId);
        self.scope.add_table("NEW", None, RowIdPolicy::WithRowId);
        self.visit_opt(stmt, Self::field_node_id(fields, when_idx));
        self.visit_opt(stmt, Self::field_node_id(fields, body_idx));
        self.scope.pop();
    }
}
