# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Token preservation suite.

Measures how much of the authored token stream the AST can reproduce, by
formatting a corpus and comparing the token stream before and after.  Every
difference is a construct the AST does not yet model, and the ranked list of
those constructs is the work queue for making it lossless.

Writes tests/token_preservation/triage.md and ratchets a per-category
baseline; a category that grows past its baseline fails the run.

    tools/run-integration-tests --suite token-preservation
    tools/run-integration-tests --suite token-preservation --rebaseline
"""

import os

from python.dev.integration_tests.suite import SuiteContext

NAME = "token-preservation"
DESCRIPTION = "Measure which authored constructs the AST cannot reproduce"
NEEDS_BINARY = False


def run(ctx: SuiteContext) -> int:
    from python.dev.diff_tests.token_preservation_runner import main

    argv = []
    if ctx.filter_pattern:
        argv += ["--filter", ctx.filter_pattern]
    if os.environ.get("SYNQ_TOKEN_PRESERVATION_UPSTREAM"):
        argv.append("--upstream")
    if ctx.rebaseline:
        argv.append("--rebaseline")
    if ctx.verbose >= 1:
        argv.append("-v")
    return main(argv)
