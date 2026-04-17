# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Lineage diff test suite."""

from python.dev.integration_tests.suite import SuiteContext

NAME = "lineage"
DESCRIPTION = "Lineage CLI diff tests (tests/lineage_diff_tests/)"


def run(ctx: SuiteContext) -> int:
    from python.dev.diff_tests.runner import main

    argv = [
        "--binary", str(ctx.binary),
        "--subcommand", "lineage -o text",
        "--test-dir", "tests/lineage_diff_tests",
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
