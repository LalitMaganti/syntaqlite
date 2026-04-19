# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import DiffTestBlueprint, TestSuite


class Functions(TestSuite):
    def test_unknown_function_flagged(self):
        return DiffTestBlueprint(
            sql="SELECT totally_unknown_fn(1)",
            strict_schema=True,
            out="""\
            error: unknown function 'totally_unknown_fn'
             --> <stdin>:1:8
              |
            1 | SELECT totally_unknown_fn(1)
              |        ^~~~~~~~~~~~~~~~~~
""",
        )

    def test_builtin_abs_no_error(self):
        return DiffTestBlueprint(
            sql="SELECT abs(-1)",
            strict_schema=True,
            out="",
        )

    def test_abs_wrong_arity(self):
        return DiffTestBlueprint(
            sql="SELECT abs(1, 2, 3)",
            strict_schema=True,
            out="""\
            error: function 'abs' expects 0 or 1 argument(s), got 3
             --> <stdin>:1:8
              |
            1 | SELECT abs(1, 2, 3)
              |        ^~~
""",
        )

    def test_math_function_unknown_without_cflag(self):
        return DiffTestBlueprint(
            sql="SELECT acos(1.0)",
            strict_schema=True,
            out="""\
            error: unknown function 'acos'
             --> <stdin>:1:8
              |
            1 | SELECT acos(1.0)
              |        ^~~~
              = help: did you mean 'abs'?
""",
        )

    def test_math_function_known_with_cflag(self):
        return DiffTestBlueprint(
            sql="SELECT acos(1.0)",
            strict_schema=True,
            cflags=["SQLITE_ENABLE_MATH_FUNCTIONS"],
            out="",
        )
