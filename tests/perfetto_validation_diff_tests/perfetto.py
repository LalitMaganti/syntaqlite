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
    def test_macro_expansion_unknown_column_in_substituted_arg(self):
        """Macro expansion (#84): a column reference produced from a `$param`
        substitution should drill through the arg segment back to the
        user's authored arg text in the original source (argument-level
        fidelity, per the text-expansion-model plan's success criterion
        #5).  Because the span lies fully inside a substituted arg, the
        new `traceback` API collapses the macro frame — the span's
        provenance at every level in the chain is the arg origin in
        user source — so the rendered diagnostic shows only the primary
        span with no "in macro expansion" note.
        """
        return DiffTestBlueprint(
            sql="CREATE PERFETTO MACRO _d(name ColumnName) RETURNS Expr AS $name;\nSELECT _d!(a);",
            out="""warning: unknown column 'a'
 --> <stdin>:2:12
  |
2 | SELECT _d!(a);
  |            ^""",
        )

    def test_macro_expansion_unknown_column_in_macro_body(self):
        """Dual of the arg-substitution case: when the unresolved reference
        is authored *in the macro body itself* (not in a `$param`
        substitution), the traceback does not drill — the span's
        authored provenance collapses to the `m!()` call site in user
        source, and a "note: in macro expansion" frame shows the
        position inside the expanded body.  This guards the
        multi-frame traceback rendering path.
        """
        return DiffTestBlueprint(
            sql="CREATE PERFETTO MACRO m() RETURNS Expr AS unknown_col;\nSELECT m!();",
            out="""warning: unknown column 'unknown_col'
 --> <stdin>:2:8
  |
2 | SELECT m!();
  |        ^~~~
note: in macro expansion
 --> <macro expansion>:1:1
  |
1 | unknown_col
  | ^~~~~~~~~~~""",
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
