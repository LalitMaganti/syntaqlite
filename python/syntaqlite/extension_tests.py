# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Tests for the dialect extension system.

1. Token stability: Extension grammars must never shift base token IDs.
   Lemon assigns token IDs in first-seen order, so we use a two-pass
   grammar build to lock base terminal ordering before extensions.

2. No token range checks: Upstream SQLite uses `tokenType >= TK_SPACE` as
   an optimization that requires SPACE/COMMENT/ILLEGAL to have the highest
   IDs. We intentionally avoid this pattern so that dialect extensions can
   add tokens freely without ID-range constraints.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .sqlite_extractor.tools import ToolRunner

from .sqlite_extractor.grammar_build import build_synq_grammar

ROOT_DIR = Path(__file__).parent.parent.parent


def _extract_token_ids(grammar_content: str, runner: ToolRunner) -> dict[str, int]:
    """Run lemon on the grammar and extract token name -> ID mapping."""
    import tempfile

    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir_path = Path(tmpdir)
        grammar_path = tmpdir_path / "parse.y"
        grammar_path.write_text(grammar_content)

        parse_h = runner.run_lemon(grammar_path, output_dir=tmpdir_path)
        header = parse_h.read_text()

    token_ids = {}
    for match in re.finditer(r"#define\s+(\w+)\s+(\d+)", header):
        name = match.group(1)
        token_id = int(match.group(2))
        token_ids[name] = token_id
    return token_ids


# A fake extension grammar that adds dummy tokens and rules.
_FAKE_EXTENSION = """\
%token FAKE_EXT_ALPHA FAKE_EXT_BETA FAKE_EXT_GAMMA.

// Dummy rule using a base nonterminal
cmd ::= FAKE_EXT_ALPHA nm.
cmd ::= FAKE_EXT_BETA nm FAKE_EXT_GAMMA nm.
"""


def test_token_stability(runner: ToolRunner) -> bool:
    """Verify that extension grammars never shift base token IDs."""
    print("=== Token Stability ===")

    print("Building grammar WITHOUT extension...")
    base_grammar = build_synq_grammar(runner, prefix="synq")

    print("Building grammar WITH fake extension...")
    ext_grammar = build_synq_grammar(
        runner, prefix="synq", extension_grammar=_FAKE_EXTENSION,
    )

    print("Extracting token IDs from both grammars...")
    base_ids = _extract_token_ids(base_grammar, runner)
    ext_ids = _extract_token_ids(ext_grammar, runner)

    if not base_ids:
        print("FAIL: No tokens found in base grammar", file=sys.stderr)
        return False

    # Every base token must have the same ID in the extension build
    failures = []
    for name, base_id in sorted(base_ids.items()):
        ext_id = ext_ids.get(name)
        if ext_id is None:
            failures.append(f"  {name}: missing from extension build")
        elif ext_id != base_id:
            failures.append(f"  {name}: base={base_id} ext={ext_id}")

    if failures:
        print("FAIL: Base token IDs shifted when extension was added:")
        for f in failures:
            print(f)
        return False

    # Extension tokens must have IDs greater than all base tokens
    max_base_id = max(base_ids.values())
    ext_only_tokens = {
        name: tid for name, tid in ext_ids.items() if name not in base_ids
    }

    if not ext_only_tokens:
        print("FAIL: No extension-only tokens found", file=sys.stderr)
        return False

    bad_ext = {
        name: tid for name, tid in ext_only_tokens.items() if tid <= max_base_id
    }
    if bad_ext:
        print(f"FAIL: Extension tokens have IDs <= max base ID ({max_base_id}):")
        for name, tid in sorted(bad_ext.items()):
            print(f"  {name}: {tid}")
        return False

    print(f"PASS: {len(base_ids)} base tokens stable, "
          f"{len(ext_only_tokens)} extension tokens all > {max_base_id}")
    return True


# Patterns that indicate a token-ID range check (the upstream SQLite
# optimization we intentionally avoid).
_RANGE_CHECK_PATTERNS = [
    # >= comparisons against SPACE/COMMENT/ILLEGAL token constants
    re.compile(r'>=[^;]*(?:TOKEN_SPACE|TOKEN_COMMENT|TOKEN_ILLEGAL|TK_SPACE|TK_COMMENT|TK_ILLEGAL)'),
    # <= comparisons too (reversed operand order)
    re.compile(r'(?:TOKEN_SPACE|TOKEN_COMMENT|TOKEN_ILLEGAL|TK_SPACE|TK_COMMENT|TK_ILLEGAL)[^;]*<='),
]

# Files to scan (our code, not upstream SQLite or generated lemon internals)
_SCAN_DIRS = [
    ROOT_DIR / "src" / "parser",
    ROOT_DIR / "src" / "tokenizer",
    ROOT_DIR / "src" / "formatter",
]

# Files to exclude from scanning (generated lemon parser internals use token
# IDs in array indexing which is fine)
_EXCLUDE_FILES = {"sqlite_parse_gen.c"}


def test_no_token_range_checks() -> bool:
    """Verify we never use >=TK_SPACE style range checks on token IDs.

    Upstream SQLite uses `tokenType >= TK_SPACE` as an optimization that
    requires SPACE/COMMENT/ILLEGAL to be the highest-numbered tokens. We
    avoid this so dialect extensions can add tokens without ID constraints.
    """
    print("\n=== No Token Range Checks ===")

    violations = []
    for scan_dir in _SCAN_DIRS:
        if not scan_dir.is_dir():
            continue
        for path in sorted(scan_dir.glob("*.c")):
            if path.name in _EXCLUDE_FILES:
                continue
            content = path.read_text()
            for i, line in enumerate(content.splitlines(), 1):
                # Skip comments
                stripped = line.strip()
                if stripped.startswith("//") or stripped.startswith("*"):
                    continue
                for pattern in _RANGE_CHECK_PATTERNS:
                    if pattern.search(line):
                        violations.append(f"  {path.name}:{i}: {stripped}")

    if violations:
        print("FAIL: Found token-ID range checks (use equality checks instead):")
        for v in violations:
            print(v)
        print("\nUpstream SQLite uses `tokenType >= TK_SPACE` but we avoid this")
        print("so that dialect extensions can add tokens without ID constraints.")
        return False

    print("PASS: No token-ID range checks found in source")
    return True


def main() -> int:
    from .sqlite_extractor.tools import ToolRunner

    runner = ToolRunner(root_dir=ROOT_DIR)
    ok = True
    ok = test_token_stability(runner) and ok
    ok = test_no_token_range_checks() and ok
    return 0 if ok else 1
