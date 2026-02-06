# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""FROM clause table source AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class TableRefBasic(TestSuite):
    """Basic table reference tests."""

    def test_simple_table(self):
        return AstTestBlueprint(
            sql="SELECT * FROM t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: TableRef
    table_name: "t"
    schema: null
    alias: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_table_with_alias(self):
        return AstTestBlueprint(
            sql="SELECT * FROM t AS x",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: TableRef
    table_name: "t"
    schema: null
    alias: "x"
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_schema_qualified(self):
        return AstTestBlueprint(
            sql="SELECT * FROM main.t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: TableRef
    table_name: "t"
    schema: "main"
    alias: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_schema_qualified_with_alias(self):
        return AstTestBlueprint(
            sql="SELECT * FROM main.t AS x",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: TableRef
    table_name: "t"
    schema: "main"
    alias: "x"
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class JoinBasic(TestSuite):
    """Basic JOIN tests."""

    def test_comma_join(self):
        return AstTestBlueprint(
            sql="SELECT * FROM a, b",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: JoinClause
    join_type: COMMA
    left: TableRef
      table_name: "a"
      schema: null
      alias: null
    right: TableRef
      table_name: "b"
      schema: null
      alias: null
    on_expr: null
    using_columns: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_inner_join(self):
        return AstTestBlueprint(
            sql="SELECT * FROM a JOIN b ON a.id = b.id",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: JoinClause
    join_type: INNER
    left: TableRef
      table_name: "a"
      schema: null
      alias: null
    right: TableRef
      table_name: "b"
      schema: null
      alias: null
    on_expr: BinaryExpr
      op: EQ
      left: ColumnRef
        column: "id"
        table: "a"
        schema: null
      right: ColumnRef
        column: "id"
        table: "b"
        schema: null
    using_columns: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_left_join(self):
        return AstTestBlueprint(
            sql="SELECT * FROM a LEFT JOIN b ON a.id = b.id",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: JoinClause
    join_type: LEFT
    left: TableRef
      table_name: "a"
      schema: null
      alias: null
    right: TableRef
      table_name: "b"
      schema: null
      alias: null
    on_expr: BinaryExpr
      op: EQ
      left: ColumnRef
        column: "id"
        table: "a"
        schema: null
      right: ColumnRef
        column: "id"
        table: "b"
        schema: null
    using_columns: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_right_join(self):
        return AstTestBlueprint(
            sql="SELECT * FROM a RIGHT JOIN b ON a.id = b.id",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: JoinClause
    join_type: RIGHT
    left: TableRef
      table_name: "a"
      schema: null
      alias: null
    right: TableRef
      table_name: "b"
      schema: null
      alias: null
    on_expr: BinaryExpr
      op: EQ
      left: ColumnRef
        column: "id"
        table: "a"
        schema: null
      right: ColumnRef
        column: "id"
        table: "b"
        schema: null
    using_columns: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_cross_join(self):
        return AstTestBlueprint(
            sql="SELECT * FROM a CROSS JOIN b",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: JoinClause
    join_type: CROSS
    left: TableRef
      table_name: "a"
      schema: null
      alias: null
    right: TableRef
      table_name: "b"
      schema: null
      alias: null
    on_expr: null
    using_columns: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_full_join(self):
        return AstTestBlueprint(
            sql="SELECT * FROM a FULL JOIN b ON a.id = b.id",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: JoinClause
    join_type: FULL
    left: TableRef
      table_name: "a"
      schema: null
      alias: null
    right: TableRef
      table_name: "b"
      schema: null
      alias: null
    on_expr: BinaryExpr
      op: EQ
      left: ColumnRef
        column: "id"
        table: "a"
        schema: null
      right: ColumnRef
        column: "id"
        table: "b"
        schema: null
    using_columns: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_left_outer_join(self):
        return AstTestBlueprint(
            sql="SELECT * FROM a LEFT OUTER JOIN b ON a.id = b.id",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: JoinClause
    join_type: LEFT
    left: TableRef
      table_name: "a"
      schema: null
      alias: null
    right: TableRef
      table_name: "b"
      schema: null
      alias: null
    on_expr: BinaryExpr
      op: EQ
      left: ColumnRef
        column: "id"
        table: "a"
        schema: null
      right: ColumnRef
        column: "id"
        table: "b"
        schema: null
    using_columns: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class JoinNatural(TestSuite):
    """NATURAL JOIN tests."""

    def test_natural_join(self):
        return AstTestBlueprint(
            sql="SELECT * FROM a NATURAL JOIN b",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: JoinClause
    join_type: NATURAL_INNER
    left: TableRef
      table_name: "a"
      schema: null
      alias: null
    right: TableRef
      table_name: "b"
      schema: null
      alias: null
    on_expr: null
    using_columns: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_natural_left_join(self):
        return AstTestBlueprint(
            sql="SELECT * FROM a NATURAL LEFT JOIN b",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: JoinClause
    join_type: NATURAL_LEFT
    left: TableRef
      table_name: "a"
      schema: null
      alias: null
    right: TableRef
      table_name: "b"
      schema: null
      alias: null
    on_expr: null
    using_columns: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class JoinUsing(TestSuite):
    """JOIN with USING clause tests."""

    def test_join_using(self):
        return AstTestBlueprint(
            sql="SELECT * FROM a JOIN b USING(id)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: JoinClause
    join_type: INNER
    left: TableRef
      table_name: "a"
      schema: null
      alias: null
    right: TableRef
      table_name: "b"
      schema: null
      alias: null
    on_expr: null
    using_columns: ExprList[1]
      ColumnRef
        column: "id"
        table: null
        schema: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )

    def test_join_using_multiple(self):
        return AstTestBlueprint(
            sql="SELECT * FROM a JOIN b USING(id, name)",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: JoinClause
    join_type: INNER
    left: TableRef
      table_name: "a"
      schema: null
      alias: null
    right: TableRef
      table_name: "b"
      schema: null
      alias: null
    on_expr: null
    using_columns: ExprList[2]
      ColumnRef
        column: "id"
        table: null
        schema: null
      ColumnRef
        column: "name"
        table: null
        schema: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class JoinMultiple(TestSuite):
    """Multiple JOIN tests."""

    def test_multiple_joins(self):
        return AstTestBlueprint(
            sql="SELECT * FROM a JOIN b ON a.id = b.id LEFT JOIN c ON b.id = c.id",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: JoinClause
    join_type: LEFT
    left: JoinClause
      join_type: INNER
      left: TableRef
        table_name: "a"
        schema: null
        alias: null
      right: TableRef
        table_name: "b"
        schema: null
        alias: null
      on_expr: BinaryExpr
        op: EQ
        left: ColumnRef
          column: "id"
          table: "a"
          schema: null
        right: ColumnRef
          column: "id"
          table: "b"
          schema: null
      using_columns: null
    right: TableRef
      table_name: "c"
      schema: null
      alias: null
    on_expr: BinaryExpr
      op: EQ
      left: ColumnRef
        column: "id"
        table: "b"
        schema: null
      right: ColumnRef
        column: "id"
        table: "c"
        schema: null
    using_columns: null
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )


class SubqueryTableSource(TestSuite):
    """Subquery table source tests."""

    def test_subquery_source(self):
        return AstTestBlueprint(
            sql="SELECT * FROM (SELECT 1) AS t",
            out="""\
SelectStmt
  flags: (none)
  columns: ResultColumnList[1]
    ResultColumn
      flags: STAR
      alias: null
      expr: null
  from_clause: SubqueryTableSource
    select: SelectStmt
      flags: (none)
      columns: ResultColumnList[1]
        ResultColumn
          flags: (none)
          alias: null
          expr: Literal
            literal_type: INTEGER
            source: "1"
      from_clause: null
      where: null
      groupby: null
      having: null
      orderby: null
      limit_clause: null
      window_clause: null
    alias: "t"
  where: null
  groupby: null
  having: null
  orderby: null
  limit_clause: null
  window_clause: null
""",
        )
