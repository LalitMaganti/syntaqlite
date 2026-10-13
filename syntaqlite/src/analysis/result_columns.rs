// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! The columns a query produces, with `*` and `t.*` expanded the way
//! `SQLite` expands them.

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue, NodeFields};

use super::catalog::Catalog;
use super::name_key::NameKey;
use super::stmt_reader::StmtReader;
use crate::dialect::SemanticRole;

/// A FROM clause entry that `t.*` can name.
pub(crate) struct FromItem {
    /// The alias, or the relation name when there is none. `None` for an
    /// unaliased subquery.
    pub(crate) name: Option<String>,
    /// `None` when the entry's columns are unknown.
    pub(crate) columns: Option<Vec<String>>,
}

/// One column of a FROM clause's `*` expansion.
pub(crate) enum StarColumn {
    /// Column `name` of `items[item]`.
    Column { item: usize, name: String },
    /// The columns of an entry are unknown, so this part of the expansion
    /// is too.
    Unknown,
}

/// A FROM clause's entries, and what `*` expands to over them.
#[derive(Default)]
pub(crate) struct FromColumns {
    pub(crate) items: Vec<FromItem>,
    pub(crate) star: Vec<StarColumn>,
}

impl FromColumns {
    /// What `qualifier.*` expands to, or `None` when no entry has that
    /// name or its columns are unknown.
    pub(crate) fn qualified_star(&self, qualifier: &str) -> Option<(usize, &[String])> {
        let item = self.items.iter().position(|item| {
            item.name
                .as_deref()
                .is_some_and(|name| name.eq_ignore_ascii_case(qualifier))
        })?;
        Some((item, self.items[item].columns.as_deref()?))
    }
}

/// Resolves the result columns of queries in one statement. Relations
/// resolve against the CTEs the query itself binds, then the catalog,
/// whose query scope holds any CTEs bound around it.
pub(crate) struct ColumnResolver<'a, 'b> {
    reader: StmtReader<'a, 'b>,
    catalog: &'a Catalog,
    /// CTEs bound by the query being resolved, innermost last.
    ctes: Vec<(NameKey, Option<Vec<String>>)>,
}

impl<'a, 'b> ColumnResolver<'a, 'b> {
    pub(crate) fn new(
        stmt: &'a AnyParsedStatement<'b>,
        roles: &'a [SemanticRole],
        catalog: &'a Catalog,
    ) -> Self {
        Self {
            reader: StmtReader::new(stmt, roles),
            catalog,
            ctes: Vec::new(),
        }
    }

    /// The names of the columns `query` produces, or `None` when any of
    /// them can't be determined: a `*` over a source with unknown columns,
    /// or a VALUES clause whose rows differ in width.
    pub(crate) fn result_columns(&mut self, query: AnyNodeId) -> Option<Vec<String>> {
        let mut names = Names(Vec::new());
        self.resolve(query, &mut names).then_some(names.0)
    }

    /// How many columns `query` produces, under the same rules as
    /// [`result_columns`](Self::result_columns).
    pub(crate) fn result_column_count(&mut self, query: AnyNodeId) -> Option<usize> {
        let mut count = Count(0);
        self.resolve(query, &mut count).then_some(count.0)
    }

    /// Feed `query`'s columns to `sink`; `false` when they can't be
    /// determined.
    fn resolve(&mut self, query: AnyNodeId, sink: &mut impl Sink) -> bool {
        let (role, fields) = self
            .reader
            .role_for_node(query)
            .expect("a query node has fields");
        match role {
            SemanticRole::Query { from, columns, .. } => {
                self.select_columns(&fields, from, columns, sink)
            }
            // A compound takes its column names from its leftmost arm.
            SemanticRole::CompoundScope { left, .. } => {
                let left = fields.node_id_at(left).expect("a compound has a left arm");
                self.resolve(left, sink)
            }
            SemanticRole::Values { rows } => {
                let Some(width) = self.values_width(&fields, rows) else {
                    return false;
                };
                (1..=width).all(|i| sink.push(|| Some(format!("column{i}"))))
            }
            SemanticRole::CteScope { bindings, body, .. } => {
                let depth = self.ctes.len();
                self.bind_ctes(&fields, bindings);
                let known = fields
                    .node_id_at(body)
                    .is_some_and(|body| self.resolve(body, sink));
                self.ctes.truncate(depth);
                known
            }
            // A dialect's own kind of query, whose columns aren't modelled.
            _ => false,
        }
    }

    /// The width of a VALUES clause, or `None` when its rows disagree.
    pub(crate) fn values_width(&self, fields: &NodeFields, rows: u8) -> Option<usize> {
        let stmt = self.reader.stmt();
        let rows = fields
            .node_id_at(rows)
            .and_then(|id| stmt.list_children(id))
            .expect("VALUES has a row list");
        let width = |row: AnyNodeId| {
            stmt.list_children(row)
                .expect("a VALUES row is a list")
                .len()
        };
        let first = width(*rows.first().expect("VALUES has a row"));
        rows.iter().all(|&row| width(row) == first).then_some(first)
    }

    /// The entries of a FROM clause and its `*` expansion. Without a FROM
    /// clause, `*` is an error of its own, so its expansion is unknown.
    pub(crate) fn source_columns(&mut self, from: Option<AnyNodeId>) -> FromColumns {
        let mut out = FromColumns::default();
        match from {
            Some(from) => self.add_source(from, &mut out),
            None => out.star.push(StarColumn::Unknown),
        }
        out
    }

    fn select_columns(
        &mut self,
        fields: &NodeFields,
        from: u8,
        columns: u8,
        sink: &mut impl Sink,
    ) -> bool {
        let reader = self.reader;
        let list = fields
            .node_id_at(columns)
            .expect("a SELECT has result columns");
        // The FROM clause only matters to `*`, so only expand it for one.
        let mut has_star = false;
        reader.for_each_result_column(list, |col_fields, flags, _, _| {
            has_star = is_star(col_fields, flags);
            !has_star
        });
        let from = if has_star {
            self.source_columns(fields.node_id_at(from))
        } else {
            FromColumns::default()
        };
        let mut known = true;
        reader.for_each_result_column(list, |col_fields, flags, alias, expr| {
            known = if !is_star(col_fields, flags) {
                sink.push(|| reader.infer_result_col_name(col_fields, alias, expr))
            } else if let Some(qualifier) = col_fields.node_id_at(expr) {
                let qualifier = reader
                    .stmt()
                    .first_span_ident_text(qualifier)
                    .expect("a qualified * names its table");
                from.qualified_star(&qualifier).is_some_and(|(_, columns)| {
                    columns
                        .iter()
                        .all(|column| sink.push(|| Some(column.clone())))
                })
            } else {
                from.star.iter().all(|column| match column {
                    StarColumn::Column { name, .. } => sink.push(|| Some(name.clone())),
                    StarColumn::Unknown => false,
                })
            };
            known
        });
        known
    }

    /// Register the CTEs of a WITH clause in order, each visible to the
    /// ones after it.
    fn bind_ctes(&mut self, fields: &NodeFields, bindings: u8) {
        let stmt = self.reader.stmt();
        let bindings = fields
            .node_id_at(bindings)
            .and_then(|id| stmt.list_children(id))
            .expect("a WITH clause has bindings");
        for &binding in bindings {
            let Some((SemanticRole::CteBinding { name, body, .. }, binding_fields)) =
                self.reader.role_for_node(binding)
            else {
                unreachable!("a WITH clause holds CTE bindings");
            };
            let name = stmt
                .span_field_ident_text(&binding_fields, name)
                .expect("a CTE has a name");
            let declared = self.reader.cte_declared_cols(binding).map(|cols| {
                cols.into_iter()
                    .map(|(name, _)| name.into_owned())
                    .collect()
            });
            let key = NameKey::new(&name);
            // A recursive CTE's body can name itself; until the body is
            // resolved, only its declared columns are known.
            self.ctes.push((key.clone(), declared.clone()));
            let columns = declared.or_else(|| {
                let body = binding_fields.node_id_at(body)?;
                self.result_columns(body)
            });
            self.ctes.pop();
            self.ctes.push((key, columns));
        }
    }

    fn add_source(&mut self, node: AnyNodeId, out: &mut FromColumns) {
        let stmt = self.reader.stmt();
        if let Some(children) = stmt.list_children(node) {
            for &child in children {
                if !child.is_null() {
                    self.add_source(child, out);
                }
            }
            return;
        }
        let (role, fields) = self
            .reader
            .role_for_node(node)
            .expect("a FROM node has fields");
        match role {
            SemanticRole::SourceRef { name, alias, .. } => {
                let relation = stmt
                    .span_field_ident_text(&fields, name)
                    .expect("a FROM table has a name");
                let columns = self.relation_columns(&relation);
                let (alias, _) = stmt.name_ident_text(fields.node_id_at(alias));
                let name = if alias.is_empty() { relation } else { alias };
                push_item(out, Some(name.into_owned()), columns);
            }
            SemanticRole::ScopedSource { body, alias } => {
                let columns = fields
                    .node_id_at(body)
                    .and_then(|body| self.result_columns(body));
                let (alias, _) = stmt.name_ident_text(fields.node_id_at(alias));
                let name = (!alias.is_empty()).then(|| alias.into_owned());
                push_item(out, name, columns);
            }
            SemanticRole::Join {
                kind,
                left,
                right,
                using,
                ..
            } => {
                let left = fields.node_id_at(left).expect("a join has a left source");
                let right = fields.node_id_at(right).expect("a join has a right source");
                self.add_source(left, out);
                let split = out.star.len();
                self.add_source(right, out);
                let natural = stmt.enum_name(node, kind).starts_with("NATURAL_");
                self.merge_join_columns(out, split, natural, fields.node_id_at(using));
            }
            // A parenthesised join, and any other wrapper, contributes the
            // sources inside it.
            _ => {
                for child in stmt.child_node_ids(node) {
                    if !child.is_null() {
                        self.add_source(child, out);
                    }
                }
            }
        }
    }

    /// Drop the right side's copies of the columns a NATURAL or USING join
    /// matches; `*` shows each once, from the left. The right side starts
    /// at `out.star[split]`.
    fn merge_join_columns(
        &self,
        out: &mut FromColumns,
        split: usize,
        natural: bool,
        using: Option<AnyNodeId>,
    ) {
        let (left, right) = out.star.split_at(split);
        let shared: Vec<String> = if natural {
            let known =
                |cols: &[StarColumn]| cols.iter().all(|c| matches!(c, StarColumn::Column { .. }));
            if !known(left) || !known(right) {
                // The shared columns can't be determined.
                out.star.truncate(split);
                out.star.push(StarColumn::Unknown);
                return;
            }
            left.iter()
                .filter_map(|c| match c {
                    StarColumn::Column { name, .. } => Some(name.clone()),
                    StarColumn::Unknown => None,
                })
                .collect()
        } else if let Some(using) = using {
            let stmt = self.reader.stmt();
            stmt.list_children(using)
                .expect("USING is a list")
                .iter()
                .map(|&id| {
                    let (name, _) = self
                        .reader
                        .column_ref_name_span(id)
                        .expect("a USING entry names a column");
                    name.into_owned()
                })
                .collect()
        } else {
            return;
        };
        let mut index = 0;
        out.star.retain(|column| {
            index += 1;
            index <= split
                || !matches!(column, StarColumn::Column { name, .. }
                    if shared.iter().any(|s| s.eq_ignore_ascii_case(name)))
        });
    }

    fn relation_columns(&self, name: &str) -> Option<Vec<String>> {
        let key = NameKey::new(name);
        match self.ctes.iter().rev().find(|(cte, _)| *cte == key) {
            Some((_, columns)) => columns.clone(),
            None => self.catalog.star_columns(&key),
        }
    }
}

/// Receives a query's columns, in order. Names are computed only for
/// sinks that keep them.
trait Sink {
    /// Add a column; `false` when the sink needs its name and there is none.
    fn push(&mut self, name: impl FnOnce() -> Option<String>) -> bool;
}

struct Names(Vec<String>);

impl Sink for Names {
    fn push(&mut self, name: impl FnOnce() -> Option<String>) -> bool {
        name().map(|name| self.0.push(name)).is_some()
    }
}

struct Count(usize);

impl Sink for Count {
    fn push(&mut self, _name: impl FnOnce() -> Option<String>) -> bool {
        self.0 += 1;
        true
    }
}

fn push_item(out: &mut FromColumns, name: Option<String>, columns: Option<Vec<String>>) {
    let item = out.items.len();
    match &columns {
        Some(columns) => out
            .star
            .extend(columns.iter().map(|name| StarColumn::Column {
                item,
                name: name.clone(),
            })),
        None => out.star.push(StarColumn::Unknown),
    }
    out.items.push(FromItem { name, columns });
}

/// Whether a result column is `*` or `t.*`.
pub(crate) fn is_star(fields: &NodeFields, flags: u8) -> bool {
    matches!(fields[usize::from(flags)], FieldValue::Flags(f) if f & 1 != 0)
}

#[cfg(test)]
mod tests {
    use crate::analysis::{
        AnalysisContext, Analyzer, Catalog, CatalogLayer, Column, DiagnosticMessage, TableDef,
    };

    /// The compound column counts reported for `sql` against a table `ft`
    /// with a hidden column.
    fn compound_mismatches(sql: &str) -> Vec<(usize, usize)> {
        let mut catalog = Catalog::new(crate::sqlite::dialect::dialect());
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            TableDef::new("ft").columns([Column::from("body"), Column::hidden("rank")]),
        );
        let mut ctx = AnalysisContext::new(&mut catalog);
        let model = Analyzer::new().analyze(sql, &mut ctx);
        model
            .diagnostics()
            .filter_map(|d| match d.message() {
                DiagnosticMessage::CompoundColumns { left, right, .. } => Some((*left, *right)),
                _ => None,
            })
            .collect()
    }

    #[test]
    fn star_skips_hidden_columns() {
        assert_eq!(compound_mismatches("SELECT * FROM ft UNION SELECT 1"), []);
        assert_eq!(
            compound_mismatches("SELECT * FROM ft UNION SELECT 1, 2"),
            [(1, 2)]
        );
    }

    #[test]
    fn hidden_columns_can_be_named() {
        assert_eq!(
            compound_mismatches("SELECT body, rank FROM ft UNION SELECT 1, 2"),
            []
        );
    }
}
