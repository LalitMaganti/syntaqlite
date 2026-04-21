# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""WITH/CTE (Common Table Expression) AST tests."""

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class WithClause(TestSuite):
    """WITH clause tests."""

    def test_simple_cte(self):
        return DiffTestBlueprint(
            sql="WITH t AS (SELECT 1) SELECT * FROM t",
            out="""\
            WithClause
              recursive: FALSE
              ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "t"
                    materialized: DEFAULT
                    columns: (none)
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
              select:
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
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )

    def test_cte_with_columns(self):
        return DiffTestBlueprint(
            sql="WITH t(a, b) AS (SELECT 1, 2) SELECT * FROM t",
            out="""\
            WithClause
              recursive: FALSE
              ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "t"
                    materialized: DEFAULT
                    columns:
                      ExprList [2 items]
                        ColumnRef
                          column: "a"
                          table: (none)
                          schema: (none)
                        ColumnRef
                          column: "b"
                          table: (none)
                          schema: (none)
                    select:
                      SelectStmt
                        flags: (none)
                        columns:
                          ResultColumnList [2 items]
                            ResultColumn
                              flags: (none)
                              alias: (none)
                              expr:
                                Literal
                                  literal_type: INTEGER
                                  source: "1"
                            ResultColumn
                              flags: (none)
                              alias: (none)
                              expr:
                                Literal
                                  literal_type: INTEGER
                                  source: "2"
                        from_clause: (none)
                        where_clause: (none)
                        groupby: (none)
                        having: (none)
                        orderby: (none)
                        limit_clause: (none)
                        window_clause: (none)
              select:
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
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )

    def test_recursive_cte(self):
        return DiffTestBlueprint(
            sql="WITH RECURSIVE cnt(x) AS (SELECT 1 UNION ALL SELECT x+1 FROM cnt) SELECT x FROM cnt",
            out="""\
            WithClause
              recursive: TRUE
              ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "cnt"
                    materialized: DEFAULT
                    columns:
                      ExprList [1 items]
                        ColumnRef
                          column: "x"
                          table: (none)
                          schema: (none)
                    select:
                      CompoundSelect
                        op: UNION_ALL
                        left:
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
                        right:
                          SelectStmt
                            flags: (none)
                            columns:
                              ResultColumnList [1 items]
                                ResultColumn
                                  flags: (none)
                                  alias: (none)
                                  expr:
                                    BinaryExpr
                                      op: PLUS
                                      left:
                                        ColumnRef
                                          column: "x"
                                          table: (none)
                                          schema: (none)
                                      right:
                                        Literal
                                          literal_type: INTEGER
                                          source: "1"
                            from_clause:
                              TableRef
                                table_name: "cnt"
                                schema: (none)
                                has_parens: FALSE
                                alias: (none)
                                args: (none)
                            where_clause: (none)
                            groupby: (none)
                            having: (none)
                            orderby: (none)
                            limit_clause: (none)
                            window_clause: (none)
              select:
                SelectStmt
                  flags: (none)
                  columns:
                    ResultColumnList [1 items]
                      ResultColumn
                        flags: (none)
                        alias: (none)
                        expr:
                          ColumnRef
                            column: "x"
                            table: (none)
                            schema: (none)
                  from_clause:
                    TableRef
                      table_name: "cnt"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )

    def test_multiple_ctes(self):
        return DiffTestBlueprint(
            sql="WITH a AS (SELECT 1), b AS (SELECT 2) SELECT * FROM a",
            out="""\
            WithClause
              recursive: FALSE
              ctes:
                CteList [2 items]
                  CteDefinition
                    cte_name: "a"
                    materialized: DEFAULT
                    columns: (none)
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
                  CteDefinition
                    cte_name: "b"
                    materialized: DEFAULT
                    columns: (none)
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
                                  source: "2"
                        from_clause: (none)
                        where_clause: (none)
                        groupby: (none)
                        having: (none)
                        orderby: (none)
                        limit_clause: (none)
                        window_clause: (none)
              select:
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
                      table_name: "a"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )

    def test_materialized(self):
        return DiffTestBlueprint(
            sql="WITH t AS MATERIALIZED (SELECT 1) SELECT * FROM t",
            out="""\
            WithClause
              recursive: FALSE
              ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "t"
                    materialized: MATERIALIZED
                    columns: (none)
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
              select:
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
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )

    def test_not_materialized(self):
        return DiffTestBlueprint(
            sql="WITH t AS NOT MATERIALIZED (SELECT 1) SELECT * FROM t",
            out="""\
            WithClause
              recursive: FALSE
              ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "t"
                    materialized: NOT_MATERIALIZED
                    columns: (none)
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
              select:
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
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )

    def test_cte_body_values(self):
        return DiffTestBlueprint(
            sql="WITH t AS (VALUES (1, 2)) SELECT * FROM t",
            out="""\
            WithClause
              recursive: FALSE
              ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "t"
                    materialized: DEFAULT
                    columns: (none)
                    select:
                      ValuesClause
                        rows:
                          ValuesRowList [1 items]
                            ExprList [2 items]
                              Literal
                                literal_type: INTEGER
                                source: "1"
                              Literal
                                literal_type: INTEGER
                                source: "2"
              select:
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
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )

    def test_outer_query_compound(self):
        return DiffTestBlueprint(
            sql="WITH t AS (SELECT 1 AS x) SELECT * FROM t UNION SELECT 2",
            out="""\
            WithClause
              recursive: FALSE
              ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "t"
                    materialized: DEFAULT
                    columns: (none)
                    select:
                      SelectStmt
                        flags: (none)
                        columns:
                          ResultColumnList [1 items]
                            ResultColumn
                              flags: (none)
                              alias:
                                IdentName
                                  source: "x"
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
              select:
                CompoundSelect
                  op: UNION
                  left:
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
                      where_clause: (none)
                      groupby: (none)
                      having: (none)
                      orderby: (none)
                      limit_clause: (none)
                      window_clause: (none)
                  right:
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
                                source: "2"
                      from_clause: (none)
                      where_clause: (none)
                      groupby: (none)
                      having: (none)
                      orderby: (none)
                      limit_clause: (none)
                      window_clause: (none)
""",
        )

    def test_recursive_union(self):
        return DiffTestBlueprint(
            sql="WITH RECURSIVE cnt(x) AS (SELECT 1 UNION SELECT x+1 FROM cnt WHERE x < 10) SELECT x FROM cnt",
            out="""\
            WithClause
              recursive: TRUE
              ctes:
                CteList [1 items]
                  CteDefinition
                    cte_name: "cnt"
                    materialized: DEFAULT
                    columns:
                      ExprList [1 items]
                        ColumnRef
                          column: "x"
                          table: (none)
                          schema: (none)
                    select:
                      CompoundSelect
                        op: UNION
                        left:
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
                        right:
                          SelectStmt
                            flags: (none)
                            columns:
                              ResultColumnList [1 items]
                                ResultColumn
                                  flags: (none)
                                  alias: (none)
                                  expr:
                                    BinaryExpr
                                      op: PLUS
                                      left:
                                        ColumnRef
                                          column: "x"
                                          table: (none)
                                          schema: (none)
                                      right:
                                        Literal
                                          literal_type: INTEGER
                                          source: "1"
                            from_clause:
                              TableRef
                                table_name: "cnt"
                                schema: (none)
                                has_parens: FALSE
                                alias: (none)
                                args: (none)
                            where_clause:
                              BinaryExpr
                                op: LT
                                left:
                                  ColumnRef
                                    column: "x"
                                    table: (none)
                                    schema: (none)
                                right:
                                  Literal
                                    literal_type: INTEGER
                                    source: "10"
                            groupby: (none)
                            having: (none)
                            orderby: (none)
                            limit_clause: (none)
                            window_clause: (none)
              select:
                SelectStmt
                  flags: (none)
                  columns:
                    ResultColumnList [1 items]
                      ResultColumn
                        flags: (none)
                        alias: (none)
                        expr:
                          ColumnRef
                            column: "x"
                            table: (none)
                            schema: (none)
                  from_clause:
                    TableRef
                      table_name: "cnt"
                      schema: (none)
                      has_parens: FALSE
                      alias: (none)
                      args: (none)
                  where_clause: (none)
                  groupby: (none)
                  having: (none)
                  orderby: (none)
                  limit_clause: (none)
                  window_clause: (none)
""",
        )
