# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class PerfettoTableValidation(TestSuite):
    def test_create_perfetto_table_unknown_table(self):
        """Key regression test for walk_other_node Expr-before-Stmt fix.

        CREATE PERFETTO TABLE routes through walk_other_node which must
        dispatch the inner SELECT via walk_stmt (not walk_expr) so that
        FROM-clause table resolution runs before column ref checks.
        The result should be a single 'unknown table' warning, NOT an
        'unknown column' diagnostic.
        """
        return DiffTestBlueprint(
            sql="CREATE PERFETTO TABLE t AS SELECT dur FROM slice",
            out="""\
                warning: unknown table 'slice'
                 --> <stdin>:1:44
                  |
                1 | CREATE PERFETTO TABLE t AS SELECT dur FROM slice
                  |                                            ^~~~~""",
        )

    def test_create_perfetto_table_known_table(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE slice(dur INT); CREATE PERFETTO TABLE t AS SELECT dur FROM slice",
            out="",
        )


class PerfettoViewValidation(TestSuite):
    def test_create_perfetto_view_unknown_table(self):
        return DiffTestBlueprint(
            sql="CREATE PERFETTO VIEW v AS SELECT dur FROM slice",
            out="""\
                warning: unknown table 'slice'
                 --> <stdin>:1:43
                  |
                1 | CREATE PERFETTO VIEW v AS SELECT dur FROM slice
                  |                                           ^~~~~""",
        )


class PerfettoFunctionValidation(TestSuite):
    def test_create_perfetto_function_unknown_table(self):
        return DiffTestBlueprint(
            sql="CREATE PERFETTO FUNCTION f() RETURNS INT AS SELECT dur FROM slice",
            out="""\
                warning: unknown table 'slice'
                 --> <stdin>:1:61
                  |
                1 | CREATE PERFETTO FUNCTION f() RETURNS INT AS SELECT dur FROM slice
                  |                                                             ^~~~~""",
        )


class MacroExpansionSpanRegression(TestSuite):
    def test_macro_expansion_unknown_column(self):
        """Macro expansion (#84): a column reference produced inside a macro
        expansion should report "unknown column 'a'" pointing at the entire
        macro call site `_d!(a)` in the original source, plus a traceback
        showing the position inside the expansion buffer.
        """
        return DiffTestBlueprint(
            sql="CREATE PERFETTO MACRO _d(name ColumnName) RETURNS Expr AS $name;\nSELECT _d!(a);",
            out="""warning: unknown column 'a'
 --> <stdin>:2:8
  |
2 | SELECT _d!(a);
  |        ^~~~~~
note: in macro expansion
 --> <macro expansion>:1:1
  |
1 | a
  | ^""",
        )


class BaselineValidation(TestSuite):
    def test_plain_select_unknown_table(self):
        return DiffTestBlueprint(
            sql="SELECT dur FROM slice",
            out="""\
                warning: unknown table 'slice'
                 --> <stdin>:1:17
                  |
                1 | SELECT dur FROM slice
                  |                 ^~~~~""",
        )

    def test_known_table_no_warnings(self):
        return DiffTestBlueprint(
            sql="CREATE TABLE slice(dur INT); SELECT dur FROM slice",
            out="",
        )
