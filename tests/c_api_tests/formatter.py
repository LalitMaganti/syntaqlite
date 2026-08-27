# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Formatter C API scenarios.

Each test method returns a `CApiScenario` with a line-protocol `input`
fed to `formatter_driver.c` and the byte-for-byte `expected` stdout.
See `tests/c_api_tests/formatter_driver.c` for the supported verbs.
"""

from python.dev.integration_tests.suites.c_api import CApiScenario, CApiTestSuite


class BasicFormatter(CApiTestSuite):
    def test_basic(self):
        return CApiScenario(
            input="""\
create
format
select 1 from t;
.
""",
            expected="""\
create ok
format ok len=17
SELECT 1 FROM t;
.
""",
        )

    def test_empty_input(self):
        return CApiScenario(
            input="""\
create
format
.
""",
            expected="""\
create ok
format ok len=0

.
""",
        )

    def test_comments(self):
        return CApiScenario(
            input="""\
create
format
-- leading comment
select 1; -- trailing comment
.
""",
            expected="""\
create ok
format ok len=49
-- leading comment
SELECT 1; -- trailing comment
.
""",
        )

    def test_multi_statement(self):
        return CApiScenario(
            input="""\
create
format
select 1;select 2;select 3;
.
""",
            expected="""\
create ok
format ok len=32
SELECT 1;

SELECT 2;

SELECT 3;
.
""",
        )


class ConfigFormatter(CApiTestSuite):
    def test_keyword_lower(self):
        return CApiScenario(
            input="""\
create keyword_case=lower
format
SELECT * FROM Tbl;
.
""",
            expected="""\
create ok
format ok len=19
select * from Tbl;
.
""",
        )

    def test_line_width_narrow(self):
        return CApiScenario(
            input="""\
create line_width=20
format
SELECT a, b, c, d, e FROM long_table_name;
.
""",
            expected="""\
create ok
format ok len=43
SELECT a, b, c, d, e
FROM long_table_name;
.
""",
        )

    def test_indent_width(self):
        return CApiScenario(
            input="""\
create indent_width=4 line_width=30
format
CREATE TABLE t (a INTEGER, b TEXT NOT NULL, c REAL DEFAULT 0);
.
""",
            expected="""\
create ok
format ok len=76
CREATE TABLE t(
    a INTEGER,
    b TEXT NOT NULL,
    c REAL DEFAULT 0
);
.
""",
        )

    def test_semicolons_off(self):
        return CApiScenario(
            input="""\
create semicolons=0
format
select 1 from t;
.
""",
            expected="""\
create ok
format ok len=16
SELECT 1 FROM t
.
""",
        )


class LifecycleFormatter(CApiTestSuite):
    def test_reuse_after_success(self):
        return CApiScenario(
            input="""\
create
format
select 1;
.
format
select 2 from t;
.
""",
            expected="""\
create ok
format ok len=10
SELECT 1;
.
format ok len=17
SELECT 2 FROM t;
.
""",
        )

    def test_reuse_after_error(self):
        return CApiScenario(
            input="""\
create
format
select from where;
.
format
select 1;
.
""",
            expected="""\
create ok
format err syntax error near 'from'
format ok len=10
SELECT 1;
.
""",
        )

    def test_reconfigure(self):
        return CApiScenario(
            input="""\
create
format
select 1 from t;
.
create keyword_case=lower
format
select 1 from t;
.
""",
            expected="""\
create ok
format ok len=17
SELECT 1 FROM t;
.
create ok
format ok len=17
select 1 from t;
.
""",
        )

    def test_destroy_no_handle(self):
        return CApiScenario(
            input="""\
destroy
create
format
select 1;
.
destroy
destroy
""",
            expected="""\
destroy ok
create ok
format ok len=10
SELECT 1;
.
destroy ok
destroy ok
""",
        )


class ErrorsFormatter(CApiTestSuite):
    def test_parse_error(self):
        return CApiScenario(
            input="""\
create
format
select from where;
.
""",
            expected="""\
create ok
format err syntax error near 'from'
""",
        )

    def test_no_handle(self):
        return CApiScenario(
            input="""\
format
select 1;
.
""",
            expected="""\
format err no_handle
""",
        )

    def test_unknown_verb(self):
        return CApiScenario(
            input="""\
nonsense
create
format
select 1;
.
""",
            expected="""\
error unknown_verb nonsense
create ok
format ok len=10
SELECT 1;
.
""",
        )
