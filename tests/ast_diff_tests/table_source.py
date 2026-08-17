# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""FROM clause table source AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class TableRefBasic(TestSuite):
    """Basic table reference tests."""

    def test_simple_table(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_table_with_alias(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM t AS x",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias:
                    IdentName
                      source: "x"
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_implicit_alias(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM t x",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: (none)
                  has_parens: FALSE
                  alias:
                    IdentName
                      source: "x"
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_schema_qualified(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM main.t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: "main"
                  has_parens: FALSE
                  alias: (none)
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_schema_qualified_with_alias(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM main.t AS x",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                TableRef
                  table_name: "t"
                  schema: "main"
                  has_parens: FALSE
                  alias:
                    IdentName
                      source: "x"
                  args: (none)
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class JoinBasic(TestSuite):
    """Basic JOIN tests."""

    def test_comma_join(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a, b",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: COMMA
                  left:
                    TableRef
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "b"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr: (none)
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_three_way_comma_join(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a, b, c",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: COMMA
                  left:
                    JoinClause
                      join_type: COMMA
                      left:
                        TableRef
                          table_name: "a"
                          schema: (none)
                          has_parens: FALSE
                          alias: (none)
                          args: (none)
                          index_hint: DEFAULT
                          index_name: (none)
                      right:
                        TableRef
                          table_name: "b"
                          schema: (none)
                          has_parens: FALSE
                          alias: (none)
                          args: (none)
                          index_hint: DEFAULT
                          index_name: (none)
                      on_expr: (none)
                      using_columns: (none)
                  right:
                    TableRef
                      table_name: "c"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr: (none)
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_inner_join(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a JOIN b ON a.id = b.id",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: INNER
                  left:
                    TableRef
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "b"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr:
                    BinaryExpr
                      op: EQ
                      left:
                        ColumnRef
                          column: "id"
                          table: "a"
                          schema: (none)
                      right:
                        ColumnRef
                          column: "id"
                          table: "b"
                          schema: (none)
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_left_join(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a LEFT JOIN b ON a.id = b.id",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: LEFT
                  left:
                    TableRef
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "b"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr:
                    BinaryExpr
                      op: EQ
                      left:
                        ColumnRef
                          column: "id"
                          table: "a"
                          schema: (none)
                      right:
                        ColumnRef
                          column: "id"
                          table: "b"
                          schema: (none)
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_right_join(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a RIGHT JOIN b ON a.id = b.id",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: RIGHT
                  left:
                    TableRef
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "b"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr:
                    BinaryExpr
                      op: EQ
                      left:
                        ColumnRef
                          column: "id"
                          table: "a"
                          schema: (none)
                      right:
                        ColumnRef
                          column: "id"
                          table: "b"
                          schema: (none)
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_cross_join(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a CROSS JOIN b",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: CROSS
                  left:
                    TableRef
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "b"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr: (none)
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_full_join(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a FULL JOIN b ON a.id = b.id",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: FULL
                  left:
                    TableRef
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "b"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr:
                    BinaryExpr
                      op: EQ
                      left:
                        ColumnRef
                          column: "id"
                          table: "a"
                          schema: (none)
                      right:
                        ColumnRef
                          column: "id"
                          table: "b"
                          schema: (none)
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_left_outer_join(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a LEFT OUTER JOIN b ON a.id = b.id",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: LEFT
                  left:
                    TableRef
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "b"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr:
                    BinaryExpr
                      op: EQ
                      left:
                        ColumnRef
                          column: "id"
                          table: "a"
                          schema: (none)
                      right:
                        ColumnRef
                          column: "id"
                          table: "b"
                          schema: (none)
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class JoinNatural(TestSuite):
    """NATURAL JOIN tests."""

    def test_natural_join(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a NATURAL JOIN b",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: NATURAL_INNER
                  left:
                    TableRef
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "b"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr: (none)
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_natural_left_join(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a NATURAL LEFT JOIN b",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: NATURAL_LEFT
                  left:
                    TableRef
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "b"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr: (none)
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_natural_right_join(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a NATURAL RIGHT JOIN b",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: NATURAL_RIGHT
                  left:
                    TableRef
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "b"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr: (none)
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_natural_full_join(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a NATURAL FULL JOIN b",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: NATURAL_FULL
                  left:
                    TableRef
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "b"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr: (none)
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class JoinUsing(TestSuite):
    """JOIN with USING clause tests."""

    def test_join_using(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a JOIN b USING(id)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: INNER
                  left:
                    TableRef
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "b"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr: (none)
                  using_columns:
                    ExprList [1 items]
                      ColumnRef
                        column: "id"
                        table: (none)
                        schema: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_join_using_multiple(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a JOIN b USING(id, name)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: INNER
                  left:
                    TableRef
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "b"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr: (none)
                  using_columns:
                    ExprList [2 items]
                      ColumnRef
                        column: "id"
                        table: (none)
                        schema: (none)
                      ColumnRef
                        column: "name"
                        table: (none)
                        schema: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class JoinMultiple(TestSuite):
    """Multiple JOIN tests."""

    def test_multiple_joins(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM a JOIN b ON a.id = b.id LEFT JOIN c ON b.id = c.id",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: LEFT
                  left:
                    JoinClause
                      join_type: INNER
                      left:
                        TableRef
                          table_name: "a"
                          schema: (none)
                          has_parens: FALSE
                          alias: (none)
                          args: (none)
                          index_hint: DEFAULT
                          index_name: (none)
                      right:
                        TableRef
                          table_name: "b"
                          schema: (none)
                          has_parens: FALSE
                          alias: (none)
                          args: (none)
                          index_hint: DEFAULT
                          index_name: (none)
                      on_expr:
                        BinaryExpr
                          op: EQ
                          left:
                            ColumnRef
                              column: "id"
                              table: "a"
                              schema: (none)
                          right:
                            ColumnRef
                              column: "id"
                              table: "b"
                              schema: (none)
                      using_columns: (none)
                  right:
                    TableRef
                      table_name: "c"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr:
                    BinaryExpr
                      op: EQ
                      left:
                        ColumnRef
                          column: "id"
                          table: "b"
                          schema: (none)
                      right:
                        ColumnRef
                          column: "id"
                          table: "c"
                          schema: (none)
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class SubqueryTableSource(TestSuite):
    """Subquery table source tests."""

    def test_subquery_source(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM (SELECT 1) AS t",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                SubqueryTableSource
                  select:
                    SelectStmt
                      flags: (none)
                      columns:
                        ResultColumnList [1 items]
                          ResultColumn
                            flags: (none)
                            alias: (none)
                            expr:
                              Literal
                                literal_type: INTEGER
                                source: "1"
                      from_clause: (none)
                      where_clause: (none)
                      groupby: (none)
                      having: (none)
                      orderby: (none)
                      limit_clause: (none)
                      window_clause: (none)
                  alias:
                    IdentName
                      source: "t"
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )


class TableValuedFunction(TestSuite):
    """Table-valued function tests."""

    def test_tvf_single_arg(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM generate_series(1, 10)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                TableRef
                  table_name: "generate_series"
                  schema: (none)
                  has_parens: TRUE
                  alias: (none)
                  args:
                    ExprList [2 items]
                      Literal
                        literal_type: INTEGER
                        source: "1"
                      Literal
                        literal_type: INTEGER
                        source: "10"
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_tvf_with_alias(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM json_each('[]') AS j",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                TableRef
                  table_name: "json_each"
                  schema: (none)
                  has_parens: TRUE
                  alias:
                    IdentName
                      source: "j"
                  args:
                    ExprList [1 items]
                      Literal
                        literal_type: STRING
                        source: "'[]'"
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_tvf_multiple_args(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM generate_series(1, 100, 5)",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                TableRef
                  table_name: "generate_series"
                  schema: (none)
                  has_parens: TRUE
                  alias: (none)
                  args:
                    ExprList [3 items]
                      Literal
                        literal_type: INTEGER
                        source: "1"
                      Literal
                        literal_type: INTEGER
                        source: "100"
                      Literal
                        literal_type: INTEGER
                        source: "5"
                  index_hint: DEFAULT
                  index_name: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )

    def test_tvf_in_join(self):
        return DiffTestBlueprint(
            sql="SELECT * FROM t JOIN json_each(t.col) AS j ON 1",
            out="""\
            SelectStmt
              flags: (none)
              columns:
                ResultColumnList [1 items]
                  ResultColumn
                    flags: STAR
                    alias: (none)
                    expr: (none)
              from_clause:
                JoinClause
                  join_type: INNER
                  left:
                    TableRef
                      table_name: "t"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  right:
                    TableRef
                      table_name: "json_each"
                      schema: (none)
                      has_parens: TRUE
                      alias:
                        IdentName
                          source: "j"
                      args:
                        ExprList [1 items]
                          ColumnRef
                            column: "col"
                            table: "t"
                            schema: (none)
                      index_hint: DEFAULT
                      index_name: (none)
                  on_expr:
                    Literal
                      literal_type: INTEGER
                      source: "1"
                  using_columns: (none)
              where_clause: (none)
              groupby: (none)
              having: (none)
              orderby: (none)
              limit_clause: (none)
              window_clause: (none)
""",
        )
