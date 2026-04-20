# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Perfetto dialect validation diff test suite."""

from python.dev.integration_tests.suite import SuiteContext

NAME = "perfetto-val"
DESCRIPTION = "Perfetto dialect validation diff tests (tests/perfetto_validation_diff_tests/)"


def run(ctx: SuiteContext) -> int:
    from python.dev.diff_tests.perfetto_common import run_perfetto_tests

    argv = [
        "--binary", str(ctx.binary),
        "--subcommand", "analyze",
        "--test-dir", "tests/perfetto_validation_diff_tests",
    ]
    if ctx.filter_pattern:
        argv += ["--filter", ctx.filter_pattern]
    if ctx.rebaseline:
        argv.append("--rebaseline")
    if ctx.verbose >= 1:
        argv.append("-v")
    if ctx.jobs is not None:
        argv += ["--jobs", str(ctx.jobs)]
    return run_perfetto_tests(
        subcommand="analyze",
        test_dir="tests/perfetto_validation_diff_tests",
        tempfile_prefix="syntaqlite_perfetto_val_",
        argv=argv,
    )
