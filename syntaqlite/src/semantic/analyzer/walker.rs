// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Generic AST walker driven by the `SemanticRole` table.
//!
//! The walker owns the scope stack, catalog query scope, role dispatch, and
//! event construction. It hands each concrete event to a [`WalkPass`]
//! implementor via hook methods.
//!
//! ## Composition
//!
//! There is no pipeline builder. When multiple passes need to run together,
//! write a concrete struct that holds them and implements [`WalkPass`] by
//! delegating to each pass at every hook — the composition is explicit and
//! grep-able.

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue, NodeFields};
use syntaqlite_syntax::source::DocRange;

use crate::dialect::{FIELD_ABSENT, SemanticRole};
use crate::semantic::catalog::{
    AritySpec, Catalog, ColumnResolution, FunctionCategory, FunctionCheckResult,
};
use crate::semantic::ddl::DdlReader;

use super::query_scope::{QueryScope, RowIdPolicy};

// ── WalkCtx ───────────────────────────────────────────────────────────────────

/// Shared state threaded through the walk. Passes can read the catalog and
/// peek at scope; the walker is responsible for push/pop.
pub(crate) struct WalkCtx<'a> {
    pub(crate) roles: &'static [SemanticRole],
    pub(crate) catalog: &'a mut Catalog,
    pub(crate) scope: QueryScope,
}

// ── Events ────────────────────────────────────────────────────────────────────

/// A table/view/table-function reference in a FROM / JOIN / DML target.
pub(crate) struct SourceRefEvent<'a> {
    pub(crate) node_id: AnyNodeId,
    pub(crate) name_idx: u8,
    pub(crate) range: DocRange,
    pub(crate) name: &'a str,
    pub(crate) resolved: bool,
    /// Columns of the relation if known to the catalog, else `None`.
    pub(crate) columns: Option<Vec<String>>,
}

/// A column reference (qualified or bare) inside an expression.
pub(crate) struct ColumnRefEvent<'a> {
    pub(crate) node_id: AnyNodeId,
    pub(crate) column_idx: u8,
    pub(crate) range: DocRange,
    pub(crate) column: &'a str,
    pub(crate) table: Option<&'a str>,
    pub(crate) resolution: ColumnResolution,
}

/// A function / table-function / aggregate call.
pub(crate) struct CallEvent<'a> {
    pub(crate) node_id: AnyNodeId,
    pub(crate) name_idx: u8,
    pub(crate) range: DocRange,
    pub(crate) name: &'a str,
    pub(crate) arg_count: usize,
    pub(crate) result: FunctionCheckResult,
    /// Populated only for `FunctionCheckResult::Ok`.
    pub(crate) signature: Option<(FunctionCategory, Vec<AritySpec>)>,
}

/// A CTE body's actual result-column count differs from its declared count.
pub(crate) struct CteColumnCountMismatchEvent<'a> {
    pub(crate) name: &'a str,
    pub(crate) name_range: DocRange,
    pub(crate) declared: usize,
    pub(crate) actual: usize,
}

// ── WalkPass trait ────────────────────────────────────────────────────────────

/// A walk-time pass: receives pre-resolved events from the walker and decides
/// what to do with them. Every hook has an empty default, so implementors
/// override only the events they care about.
///
/// The `WANTS_*` constants let the walker skip event construction when no pass
/// in the composition wants it. For a multi-pass composition, the composition
/// struct should OR the flags of its constituent passes.
pub(crate) trait WalkPass {
    const WANTS_SOURCE_REF: bool = false;
    const WANTS_COLUMN_REF: bool = false;
    const WANTS_CALL: bool = false;
    const WANTS_RELATION_DEFINITION: bool = false;
    const WANTS_COLUMN_DEFINITION: bool = false;
    const WANTS_CTE_COLUMN_COUNT: bool = false;

    fn on_source_ref(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _cx: &mut WalkCtx<'_>,
        _ev: SourceRefEvent<'_>,
    ) {
    }

    fn on_column_ref(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _cx: &mut WalkCtx<'_>,
        _ev: ColumnRefEvent<'_>,
    ) {
    }

    fn on_call(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _cx: &mut WalkCtx<'_>,
        _ev: CallEvent<'_>,
    ) {
    }

    fn on_relation_definition(&mut self, _name: &str, _range: DocRange) {}

    fn on_column_definition(&mut self, _table: &str, _column: &str, _range: DocRange) {}

    fn on_cte_column_count_mismatch(
        &mut self,
        _stmt: &mut AnyParsedStatement<'_>,
        _ev: CteColumnCountMismatchEvent<'_>,
    ) {
    }
}

// ── Entry point ───────────────────────────────────────────────────────────────

/// Walk `root` under `stmt`, firing events to `pass` via the [`WalkPass`]
/// hooks as each role is encountered.
pub(crate) fn walk<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
    root: AnyNodeId,
) {
    walk_node(stmt, cx, pass, root);
}

// ── Node dispatch ─────────────────────────────────────────────────────────────

fn walk_node<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
    node_id: AnyNodeId,
) {
    if node_id.is_null() {
        return;
    }
    if let Some(children) = stmt.list_children(node_id) {
        let ids: Vec<AnyNodeId> = children
            .iter()
            .copied()
            .filter(|id| !id.is_null())
            .collect();
        for child in ids {
            walk_node(stmt, cx, pass, child);
        }
        return;
    }
    let Some((tag, fields)) = stmt.extract_fields(node_id) else {
        return;
    };
    let idx = u32::from(tag) as usize;
    let role = cx
        .roles
        .get(idx)
        .copied()
        .unwrap_or(SemanticRole::Transparent);

    match role {
        SemanticRole::DefineTable { select, .. }
        | SemanticRole::DefineView { select, .. }
        | SemanticRole::DefineFunction { select, .. } => {
            if select != FIELD_ABSENT {
                walk_opt(stmt, cx, pass, field_node_id(&fields, select));
            }
        }
        SemanticRole::ReturnSpec { .. } | SemanticRole::Import { .. } => {}

        SemanticRole::Transparent
        | SemanticRole::ColumnDef { .. }
        | SemanticRole::ResultColumn { .. }
        | SemanticRole::CteBinding { .. } => walk_children(stmt, cx, pass, node_id),

        SemanticRole::Call { name, args } => {
            walk_call(stmt, cx, pass, node_id, &fields, name, args);
        }
        SemanticRole::ColumnRef { column, table } => {
            walk_column_ref(stmt, cx, pass, node_id, &fields, column, table);
        }
        SemanticRole::SourceRef { name, alias, .. } => {
            walk_source_ref(stmt, cx, pass, node_id, &fields, name, alias);
        }
        SemanticRole::ScopedSource { body, alias } => {
            walk_scoped_source(stmt, cx, pass, &fields, body, alias);
        }
        SemanticRole::Query {
            from,
            columns,
            where_clause,
            groupby,
            having,
            orderby,
            limit_clause,
        } => walk_query(
            stmt,
            cx,
            pass,
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
        } => walk_cte_scope(stmt, cx, pass, &fields, recursive, bindings, body),
        SemanticRole::TriggerScope {
            target: _,
            when,
            body,
        } => walk_trigger_scope(stmt, cx, pass, &fields, when, body),
        SemanticRole::DmlScope {
            with_recursive,
            with_ctes,
        } => walk_dml_scope(stmt, cx, pass, &fields, with_recursive, with_ctes),
    }
}

fn walk_children<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
    node_id: AnyNodeId,
) {
    for child in stmt.child_node_ids(node_id) {
        if !child.is_null() {
            walk_node(stmt, cx, pass, child);
        }
    }
}

fn walk_opt<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
    id: Option<AnyNodeId>,
) {
    if let Some(id) = id {
        walk_node(stmt, cx, pass, id);
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

pub(crate) fn field_node_id(fields: &NodeFields, idx: u8) -> Option<AnyNodeId> {
    match fields[idx as usize] {
        FieldValue::NodeId(id) if !id.is_null() => Some(id),
        _ => None,
    }
}

/// Extract `(text, range)` from a `Name` node (`IdentName` or `Error`) whose
/// span lives at field 0.
pub(crate) fn name_text<'b>(
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
            let name = stmt.span_expanded_text(sp);
            let (_, range) = stmt.span_text_abs(sp);
            (name, range)
        }
        _ => ("", DocRange::default()),
    }
}

// ── Role walkers ──────────────────────────────────────────────────────────────

fn walk_source_ref<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
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
    let name = stmt.span_expanded_text(sp);
    let (_, range) = stmt.span_text_abs(sp);

    let is_known = cx.catalog.resolve_relation(name) || cx.catalog.resolve_table_function(name);
    let (columns, without_rowid) = cx.catalog.table_source_info(name);

    if P::WANTS_SOURCE_REF {
        let ev = SourceRefEvent {
            node_id,
            name_idx,
            range,
            name,
            resolved: is_known,
            columns: columns.clone(),
        };
        pass.on_source_ref(stmt, cx, ev);
    }

    let (alias, _) = name_text(stmt, field_node_id(fields, alias_idx));
    let scope_name = if alias.is_empty() { name } else { alias };
    cx.scope
        .add_table(scope_name, columns, without_rowid.into());
}

fn walk_call<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
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
        let args_id = field_node_id(fields, args_idx);
        let arg_count = args_id
            .and_then(|id| stmt.list_children(id))
            .map_or(0, <[_]>::len);
        let result = cx.catalog.check_function(name, arg_count);
        let signature = match result {
            FunctionCheckResult::Ok => cx.catalog.function_signature(name),
            _ => None,
        };

        if P::WANTS_CALL {
            let ev = CallEvent {
                node_id,
                name_idx,
                range,
                name,
                arg_count,
                result,
                signature,
            };
            pass.on_call(stmt, cx, ev);
        }
    }
    walk_children(stmt, cx, pass, node_id);
}

fn walk_column_ref<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
    node_id: AnyNodeId,
    fields: &NodeFields,
    column_idx: u8,
    table_idx: u8,
) {
    // ColumnRef outside any query scope (e.g. ATTACH ... AS scratch) is just a
    // bare identifier — skip resolution.
    if !cx.scope.has_frames() {
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

    let resolution = cx.scope.resolve_column(table, column);

    if P::WANTS_COLUMN_REF {
        let ev = ColumnRefEvent {
            node_id,
            column_idx,
            range,
            column,
            table,
            resolution,
        };
        pass.on_column_ref(stmt, cx, ev);
    }
}

fn walk_scoped_source<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
    fields: &NodeFields,
    body_idx: u8,
    alias_idx: u8,
) {
    cx.scope.push();
    walk_opt(stmt, cx, pass, field_node_id(fields, body_idx));
    cx.scope.pop();

    let (alias, _) = name_text(stmt, field_node_id(fields, alias_idx));
    let cols = field_node_id(fields, body_idx)
        .and_then(|id| DdlReader::new(stmt, cx.roles).columns_from_select(id));
    if alias.is_empty() {
        cx.scope.add_anonymous(cols);
    } else {
        cx.scope.add_table(alias, cols, RowIdPolicy::WithRowId);
    }
}

#[expect(clippy::too_many_arguments)]
fn walk_query<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
    fields: &NodeFields,
    from: u8,
    columns: u8,
    where_clause: u8,
    groupby: u8,
    having: u8,
    orderby: u8,
    limit_clause: u8,
) {
    // Push a fresh scope so that tables registered by walk_source_ref are
    // visible when we visit SELECT columns, WHERE, ORDER BY, etc.
    cx.scope.push();
    walk_opt(stmt, cx, pass, field_node_id(fields, from));
    walk_opt(stmt, cx, pass, field_node_id(fields, columns));

    // Collect SELECT aliases so they are visible in WHERE, GROUP BY, HAVING,
    // ORDER BY, and LIMIT — matching SQLite's resolution rules.
    let aliases = collect_select_aliases(stmt, cx.roles, fields, columns);
    if !aliases.is_empty() {
        cx.scope
            .add_table("", Some(aliases), RowIdPolicy::WithRowId);
    }

    for idx in [where_clause, groupby, having, orderby, limit_clause] {
        walk_opt(stmt, cx, pass, field_node_id(fields, idx));
    }
    cx.scope.pop();
}

fn collect_select_aliases(
    stmt: &mut AnyParsedStatement<'_>,
    roles: &'static [SemanticRole],
    fields: &NodeFields,
    columns_idx: u8,
) -> Vec<String> {
    let mut aliases = Vec::new();
    let Some(list_id) = field_node_id(fields, columns_idx) else {
        return aliases;
    };
    let Some(children) = stmt.list_children(list_id) else {
        return aliases;
    };
    let child_ids: Vec<AnyNodeId> = children.to_vec();
    for child_id in child_ids {
        if child_id.is_null() {
            continue;
        }
        let Some((child_tag, child_fields)) = stmt.extract_fields(child_id) else {
            continue;
        };
        let child_role = roles
            .get(u32::from(child_tag) as usize)
            .copied()
            .unwrap_or(SemanticRole::Transparent);
        let SemanticRole::ResultColumn {
            alias: alias_idx, ..
        } = child_role
        else {
            continue;
        };
        let alias_node = field_node_id(&child_fields, alias_idx);
        let (alias_text, _) = name_text(stmt, alias_node);
        if !alias_text.is_empty() {
            aliases.push(alias_text.to_string());
        }
    }
    aliases
}

fn walk_trigger_scope<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
    fields: &NodeFields,
    when_idx: u8,
    body_idx: u8,
) {
    cx.scope.push();
    cx.scope.add_table("OLD", None, RowIdPolicy::WithRowId);
    cx.scope.add_table("NEW", None, RowIdPolicy::WithRowId);
    walk_opt(stmt, cx, pass, field_node_id(fields, when_idx));
    walk_opt(stmt, cx, pass, field_node_id(fields, body_idx));
    cx.scope.pop();
}

// ── CTE / DML handling ────────────────────────────────────────────────────────

fn walk_cte_scope<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
    fields: &NodeFields,
    recursive_idx: u8,
    bindings_idx: u8,
    body_idx: u8,
) {
    cx.catalog.push_query_scope();
    register_cte_bindings(stmt, cx, pass, fields, recursive_idx, bindings_idx);
    walk_opt(stmt, cx, pass, field_node_id(fields, body_idx));
    cx.catalog.pop_query_scope();
}

fn walk_dml_scope<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
    fields: &NodeFields,
    recursive_idx: u8,
    bindings_idx: u8,
) {
    let has_ctes = bindings_idx != FIELD_ABSENT;
    if has_ctes {
        cx.catalog.push_query_scope();
        register_cte_bindings(stmt, cx, pass, fields, recursive_idx, bindings_idx);
    }
    cx.scope.push();
    walk_dml_children_except_ctes(stmt, cx, pass, fields, bindings_idx);
    cx.scope.pop();
    if has_ctes {
        cx.catalog.pop_query_scope();
    }
}

fn walk_dml_children_except_ctes<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
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
        walk_node(stmt, cx, pass, child);
    }
}

struct CteBindingInfo<'a> {
    name: &'a str,
    name_range: DocRange,
    body_id: Option<AnyNodeId>,
    declared_cols: Option<Vec<(&'a str, DocRange)>>,
}

fn register_cte_bindings<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    cx: &mut WalkCtx<'_>,
    pass: &mut P,
    fields: &NodeFields,
    recursive_idx: u8,
    bindings_idx: u8,
) {
    if bindings_idx == FIELD_ABSENT {
        return;
    }
    let is_recursive = recursive_idx != FIELD_ABSENT
        && matches!(fields[recursive_idx as usize], FieldValue::Bool(true));
    let cte_ids: Vec<AnyNodeId> = field_node_id(fields, bindings_idx)
        .and_then(|id| stmt.list_children(id))
        .map(<[AnyNodeId]>::to_vec)
        .unwrap_or_default();

    for cte_id in cte_ids {
        let Some(binding) = extract_cte_binding(stmt, cx.roles, cte_id) else {
            continue;
        };

        // For recursive CTEs, register the name before visiting the body so
        // the body can reference it.
        if is_recursive && !binding.name.is_empty() {
            let cols = binding
                .declared_cols
                .as_ref()
                .map(|v| v.iter().map(|(s, _)| s.to_string()).collect());
            cx.catalog.add_query_table(binding.name, cols);
        }

        cx.scope.push();
        walk_opt(stmt, cx, pass, binding.body_id);
        cx.scope.pop();

        if binding.name.is_empty() {
            continue;
        }

        if P::WANTS_RELATION_DEFINITION {
            pass.on_relation_definition(binding.name, binding.name_range);
        }

        let cols = if let Some(declared) = binding.declared_cols.as_ref() {
            let col_names: Vec<&str> = declared.iter().map(|(s, _)| *s).collect();
            if P::WANTS_CTE_COLUMN_COUNT
                && let Some(actual) = count_result_columns(stmt, cx.roles, binding.body_id)
                && actual != col_names.len()
            {
                let ev = CteColumnCountMismatchEvent {
                    name: binding.name,
                    name_range: binding.name_range,
                    declared: col_names.len(),
                    actual,
                };
                pass.on_cte_column_count_mismatch(stmt, ev);
            }
            if P::WANTS_COLUMN_DEFINITION {
                for &(col_name, col_range) in declared {
                    pass.on_column_definition(binding.name, col_name, col_range);
                }
            }
            Some(declared.iter().map(|(s, _)| s.to_string()).collect())
        } else {
            if P::WANTS_COLUMN_DEFINITION {
                emit_select_column_definitions(stmt, cx.roles, pass, binding.body_id, binding.name);
            }
            binding
                .body_id
                .and_then(|id| DdlReader::new(stmt, cx.roles).columns_from_select(id))
        };
        cx.catalog.add_query_table(binding.name, cols);
    }
}

fn extract_cte_binding<'b>(
    stmt: &mut AnyParsedStatement<'b>,
    roles: &'static [SemanticRole],
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
    } = roles
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
        body_id: field_node_id(&fields, body_idx),
        declared_cols: extract_declared_cols(stmt, &fields, cols_idx),
    })
}

fn extract_declared_cols<'b>(
    stmt: &mut AnyParsedStatement<'b>,
    fields: &NodeFields,
    cols_idx: u8,
) -> Option<Vec<(&'b str, DocRange)>> {
    if cols_idx == FIELD_ABSENT {
        return None;
    }
    let list_id = field_node_id(fields, cols_idx)?;
    let children = stmt.list_children(list_id)?;
    let ids: Vec<AnyNodeId> = children
        .iter()
        .copied()
        .filter(|id| !id.is_null())
        .collect();
    let names: Vec<(&'b str, DocRange)> = ids
        .into_iter()
        .map(|id| name_text(stmt, Some(id)))
        .filter(|(s, _)| !s.is_empty())
        .collect();
    if names.is_empty() { None } else { Some(names) }
}

fn count_result_columns(
    stmt: &mut AnyParsedStatement<'_>,
    roles: &'static [SemanticRole],
    body_id: Option<AnyNodeId>,
) -> Option<usize> {
    let body_id = body_id?;
    let (body_tag, body_fields) = stmt.extract_fields(body_id)?;
    let SemanticRole::Query {
        columns: cols_idx, ..
    } = roles
        .get(u32::from(body_tag) as usize)
        .copied()
        .unwrap_or(SemanticRole::Transparent)
    else {
        return None;
    };

    let list_id = field_node_id(&body_fields, cols_idx)?;
    let children = stmt.list_children(list_id)?;
    let child_ids: Vec<AnyNodeId> = children.to_vec();

    let mut count = 0usize;
    for child_id in child_ids {
        if child_id.is_null() {
            continue;
        }
        let Some((child_tag, child_fields)) = stmt.extract_fields(child_id) else {
            continue;
        };
        let SemanticRole::ResultColumn {
            flags: flags_idx, ..
        } = roles
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

fn emit_select_column_definitions<P: WalkPass>(
    stmt: &mut AnyParsedStatement<'_>,
    roles: &'static [SemanticRole],
    pass: &mut P,
    body_id: Option<AnyNodeId>,
    table_name: &str,
) {
    let Some(body_id) = body_id else { return };
    let Some((tag, fields)) = stmt.extract_fields(body_id) else {
        return;
    };
    let Some(&SemanticRole::Query {
        columns: cols_idx, ..
    }) = roles.get(u32::from(tag) as usize)
    else {
        return;
    };
    let Some(list_id) = field_node_id(&fields, cols_idx) else {
        return;
    };
    let Some(children) = stmt.list_children(list_id) else {
        return;
    };
    let child_ids: Vec<AnyNodeId> = children.to_vec();

    for child_id in child_ids {
        if child_id.is_null() {
            continue;
        }
        let Some((child_tag, child_fields)) = stmt.extract_fields(child_id) else {
            continue;
        };
        let SemanticRole::ResultColumn {
            alias: alias_idx, ..
        } = roles
            .get(u32::from(child_tag) as usize)
            .copied()
            .unwrap_or(SemanticRole::Transparent)
        else {
            continue;
        };
        let alias_node = field_node_id(&child_fields, alias_idx);
        let (alias_text, alias_range) = name_text(stmt, alias_node);
        if !alias_text.is_empty() {
            pass.on_column_definition(table_name, alias_text, alias_range);
        }
    }
}
