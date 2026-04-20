# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""LSP declarative diff-test suite.

Runs `tests/lsp_diff_tests/` against the real `syntaqlite lsp` server over
JSON-RPC. Each test declares an SQL input with a `<|>` cursor marker, an
operation (hover / definition / completion), and expected text output.
"""

from python.dev.integration_tests.suite import SuiteContext

NAME = "lsp-diff"
DESCRIPTION = "LSP declarative diff tests (tests/lsp_diff_tests/)"


def run(ctx: SuiteContext) -> int:
    from python.dev.diff_tests.lsp_runner import main

    argv = [
        "--binary", str(ctx.binary),
        "--test-dir", "tests/lsp_diff_tests",
    ]
    if ctx.filter_pattern:
        argv += ["--filter", ctx.filter_pattern]
    if ctx.rebaseline:
        argv.append("--rebaseline")
    if ctx.verbose >= 1:
        argv.append("-v")
    return main(argv)
