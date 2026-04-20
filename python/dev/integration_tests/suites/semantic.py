# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Semantic analyzer diff test suite.

Runs declarative input-and-rendered-diagnostic tests against the `validate`
subcommand for the default SQLite dialect. Catalog population is done inline
via DDL in the test SQL.
"""

from python.dev.integration_tests.suite import SuiteContext

NAME = "semantic"
DESCRIPTION = "Semantic analyzer diff tests (tests/semantic_diff_tests/)"


def run(ctx: SuiteContext) -> int:
    from python.dev.diff_tests.runner import main

    argv = [
        "--binary", str(ctx.binary),
        "--subcommand", "analyze",
        "--test-dir", "tests/semantic_diff_tests",
    ]
    if ctx.filter_pattern:
        argv += ["--filter", ctx.filter_pattern]
    if ctx.rebaseline:
        argv.append("--rebaseline")
    if ctx.verbose >= 1:
        argv.append("-v")
    if ctx.jobs is not None:
        argv += ["--jobs", str(ctx.jobs)]
    return main(argv)
