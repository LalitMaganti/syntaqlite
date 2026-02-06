# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""WITH/CTE (Common Table Expression) AST tests."""

from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class WithClause(TestSuite):
    """WITH clause tests."""

    def test_simple_cte(self):
        return AstTestBlueprint(
            sql="WITH t AS (SELECT 1) SELECT * FROM t",
            out="""\
WithClause
  recursive: FALSE
  ctes: CteList[1]
    CteDefinition
      cte_name: "t"
      materialized: DEFAULT
      columns: null
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
  select: SelectStmt
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

    def test_cte_with_columns(self):
        return AstTestBlueprint(
            sql="WITH t(a, b) AS (SELECT 1, 2) SELECT * FROM t",
            out="""\
WithClause
  recursive: FALSE
  ctes: CteList[1]
    CteDefinition
      cte_name: "t"
      materialized: DEFAULT
      columns: ExprList[2]
        ColumnRef
          column: "a"
          table: null
          schema: null
        ColumnRef
          column: "b"
          table: null
          schema: null
      select: SelectStmt
        flags: (none)
        columns: ResultColumnList[2]
          ResultColumn
            flags: (none)
            alias: null
            expr: Literal
              literal_type: INTEGER
              source: "1"
          ResultColumn
            flags: (none)
            alias: null
            expr: Literal
              literal_type: INTEGER
              source: "2"
        from_clause: null
        where: null
        groupby: null
        having: null
        orderby: null
        limit_clause: null
        window_clause: null
  select: SelectStmt
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

    def test_recursive_cte(self):
        return AstTestBlueprint(
            sql="WITH RECURSIVE cnt(x) AS (SELECT 1 UNION ALL SELECT x+1 FROM cnt) SELECT x FROM cnt",
            out="""\
WithClause
  recursive: TRUE
  ctes: CteList[1]
    CteDefinition
      cte_name: "cnt"
      materialized: DEFAULT
      columns: ExprList[1]
        ColumnRef
          column: "x"
          table: null
          schema: null
      select: CompoundSelect
        op: UNION_ALL
        left: SelectStmt
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
        right: SelectStmt
          flags: (none)
          columns: ResultColumnList[1]
            ResultColumn
              flags: (none)
              alias: null
              expr: BinaryExpr
                op: PLUS
                left: ColumnRef
                  column: "x"
                  table: null
                  schema: null
                right: Literal
                  literal_type: INTEGER
                  source: "1"
          from_clause: TableRef
            table_name: "cnt"
            schema: null
            alias: null
          where: null
          groupby: null
          having: null
          orderby: null
          limit_clause: null
          window_clause: null
  select: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: (none)
        alias: null
        expr: ColumnRef
          column: "x"
          table: null
          schema: null
    from_clause: TableRef
      table_name: "cnt"
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

    def test_multiple_ctes(self):
        return AstTestBlueprint(
            sql="WITH a AS (SELECT 1), b AS (SELECT 2) SELECT * FROM a",
            out="""\
WithClause
  recursive: FALSE
  ctes: CteList[2]
    CteDefinition
      cte_name: "a"
      materialized: DEFAULT
      columns: null
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
    CteDefinition
      cte_name: "b"
      materialized: DEFAULT
      columns: null
      select: SelectStmt
        flags: (none)
        columns: ResultColumnList[1]
          ResultColumn
            flags: (none)
            alias: null
            expr: Literal
              literal_type: INTEGER
              source: "2"
        from_clause: null
        where: null
        groupby: null
        having: null
        orderby: null
        limit_clause: null
        window_clause: null
  select: SelectStmt
    flags: (none)
    columns: ResultColumnList[1]
      ResultColumn
        flags: STAR
        alias: null
        expr: null
    from_clause: TableRef
      table_name: "a"
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

    def test_materialized(self):
        return AstTestBlueprint(
            sql="WITH t AS MATERIALIZED (SELECT 1) SELECT * FROM t",
            out="""\
WithClause
  recursive: FALSE
  ctes: CteList[1]
    CteDefinition
      cte_name: "t"
      materialized: MATERIALIZED
      columns: null
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
  select: SelectStmt
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

    def test_not_materialized(self):
        return AstTestBlueprint(
            sql="WITH t AS NOT MATERIALIZED (SELECT 1) SELECT * FROM t",
            out="""\
WithClause
  recursive: FALSE
  ctes: CteList[1]
    CteDefinition
      cte_name: "t"
      materialized: NOT_MATERIALIZED
      columns: null
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
  select: SelectStmt
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
