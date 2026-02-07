# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Shared Lemon parser pipeline.

Encapsulates the common steps of building a grammar, running Lemon with a
template, and applying symbol renaming. Used by both extract_sqlite.py
(base parser) and build_extension_grammar.py (extension parser).
"""

from __future__ import annotations

import tempfile
from pathlib import Path

from .tools import ToolRunner, create_parser_symbol_rename_pipeline


def run_lemon_pipeline(
    runner: ToolRunner,
    grammar_content: str,
    prefix: str,
) -> tuple[str, str]:
    """Build grammar -> run Lemon -> apply symbol renaming.

    Args:
        runner: ToolRunner instance for running Lemon.
        grammar_content: Full grammar text to feed to Lemon.
        prefix: Symbol prefix for renaming (e.g. "synq").

    Returns:
        (parse_c_content, parse_h_content) after symbol renaming.
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir_path = Path(tmpdir)

        # Write grammar
        grammar_path = tmpdir_path / "synq_parse.y"
        grammar_path.write_text(grammar_content)

        # Copy stock lempar.c template
        lempar_path = tmpdir_path / "lempar.c"
        lempar_path.write_text(runner.get_lempar_path().read_text())

        # Run Lemon with the stock template
        parse_c_path, parse_h_path = runner.run_lemon_with_template(
            grammar_path, lempar_path, tmpdir_path
        )
        parse_c_content = parse_c_path.read_text()
        parse_h_content = parse_h_path.read_text()

        # Apply symbol renaming
        rename_pipeline = create_parser_symbol_rename_pipeline(prefix)
        parse_c_content = rename_pipeline.apply(parse_c_content)

        return parse_c_content, parse_h_content
