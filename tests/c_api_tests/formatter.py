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
