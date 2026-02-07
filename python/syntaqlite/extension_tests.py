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
import subprocess
import sys
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .sqlite_extractor.tools import ToolRunner

from .sqlite_extractor.grammar_build import build_synq_grammar

ROOT_DIR = Path(__file__).parent.parent.parent
PERFETTO_GRAMMAR = ROOT_DIR / "tests" / "extensions" / "grammars" / "perfetto.y"


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


def _load_perfetto_grammar() -> str:
    """Load the cleaned Perfetto extension grammar."""
    if not PERFETTO_GRAMMAR.exists():
        raise FileNotFoundError(f"Perfetto grammar not found: {PERFETTO_GRAMMAR}")
    return PERFETTO_GRAMMAR.read_text()


def test_perfetto_grammar_build(runner: ToolRunner) -> bool:
    """Verify the Perfetto extension grammar merges with SQLite's grammar."""
    print("\n=== Perfetto Grammar Build ===")

    perfetto = _load_perfetto_grammar()
    print("Building grammar with Perfetto extension...")
    grammar = build_synq_grammar(runner, prefix="synq", extension_grammar=perfetto)

    if not grammar:
        print("FAIL: build_synq_grammar returned empty", file=sys.stderr)
        return False

    # Verify extension rules made it into the merged grammar
    expected_rules = [
        "PERFETTO",
        "DELEGATES",
        "macro_body",
        "module_name",
    ]
    missing = [r for r in expected_rules if r not in grammar]
    if missing:
        print(f"FAIL: Missing expected content in merged grammar: {missing}",
              file=sys.stderr)
        return False

    print("PASS: Perfetto grammar merged successfully")
    return True


def test_perfetto_token_stability(runner: ToolRunner) -> bool:
    """Verify Perfetto extension doesn't shift base token IDs."""
    print("\n=== Perfetto Token Stability ===")

    perfetto = _load_perfetto_grammar()

    print("Building grammar WITHOUT extension...")
    base_grammar = build_synq_grammar(runner, prefix="synq")

    print("Building grammar WITH Perfetto extension...")
    ext_grammar = build_synq_grammar(
        runner, prefix="synq", extension_grammar=perfetto,
    )

    print("Extracting token IDs...")
    base_ids = _extract_token_ids(base_grammar, runner)
    ext_ids = _extract_token_ids(ext_grammar, runner)

    if not base_ids:
        print("FAIL: No tokens found in base grammar", file=sys.stderr)
        return False

    failures = []
    for name, base_id in sorted(base_ids.items()):
        ext_id = ext_ids.get(name)
        if ext_id is None:
            failures.append(f"  {name}: missing from extension build")
        elif ext_id != base_id:
            failures.append(f"  {name}: base={base_id} ext={ext_id}")

    if failures:
        print("FAIL: Base token IDs shifted with Perfetto extension:")
        for f in failures:
            print(f)
        return False

    max_base_id = max(base_ids.values())
    ext_only = {n: t for n, t in ext_ids.items() if n not in base_ids}

    if not ext_only:
        print("FAIL: No Perfetto-only tokens found", file=sys.stderr)
        return False

    bad = {n: t for n, t in ext_only.items() if t <= max_base_id}
    if bad:
        print(f"FAIL: Perfetto tokens have IDs <= max base ID ({max_base_id}):")
        for name, tid in sorted(bad.items()):
            print(f"  {name}: {tid}")
        return False

    print(f"PASS: {len(base_ids)} base tokens stable, "
          f"{len(ext_only)} Perfetto tokens all > {max_base_id}")
    return True


def test_perfetto_parser_generation(runner: ToolRunner) -> bool:
    """Verify full parser data can be generated from Perfetto grammar."""
    print("\n=== Perfetto Parser Generation ===")

    # Import here to avoid circular deps at module level
    from python.tools.build_extension_grammar import generate_parser_data

    print("Generating parser data from Perfetto grammar...")
    parser_data = generate_parser_data(runner, PERFETTO_GRAMMAR, prefix="synq")

    if not parser_data:
        print("FAIL: generate_parser_data returned empty", file=sys.stderr)
        return False

    # Check for expected sections in the output
    expected = [
        "SYNTAQLITE_TOKEN_",         # Token defines
        "Parsing tables",            # Parser tables section
        "synq_parser_reduce",        # Reduce function
    ]
    missing = [e for e in expected if e not in parser_data]
    if missing:
        print(f"FAIL: Missing expected sections: {missing}", file=sys.stderr)
        return False

    print(f"PASS: Parser data generated ({len(parser_data)} bytes)")
    return True


BUILD_DIR = ROOT_DIR / "out" / "perfetto_ext"
# Path within gen dir — resolves via -Igen which is already on the include path.
_EXT_HEADER_REL = "tests/extensions/perfetto.h"
GN = ROOT_DIR / "tools" / "dev" / "gn"
NINJA = ROOT_DIR / "tools" / "dev" / "build-lock"


def _generate_extension_header(
    runner: ToolRunner,
    output_path: Path,
) -> bool:
    """Generate the amalgamated extension header for Perfetto."""
    from python.tools.build_extension_grammar import (
        generate_token_defines,
        generate_keywordhash_data,
        generate_parser_data,
        parse_extension_keywords,
    )
    from .sqlite_extractor.generators import HeaderGenerator

    extra_keywords = parse_extension_keywords(PERFETTO_GRAMMAR)
    if not extra_keywords:
        print("FAIL: No keywords in Perfetto grammar", file=sys.stderr)
        return False

    token_defines = generate_token_defines(runner, PERFETTO_GRAMMAR)
    keywordhash_data = generate_keywordhash_data(runner, extra_keywords)
    parser_data = generate_parser_data(runner, PERFETTO_GRAMMAR, prefix="synq")

    content = "\n".join([
        "/* Token definitions */", "",
        token_defines, "",
        "/* Keyword hash data */", "",
        keywordhash_data, "",
        "/* Parser data */", "",
        parser_data,
    ])

    gen = HeaderGenerator(
        guard="SYNQ_EXTENSION_PERFETTO_H",
        description="Amalgamated PerfettoSQL extension for syntaqlite (test).",
        regenerate_cmd="",
    )
    gen.write(output_path, content)
    return True


def _build_extension_binary() -> Path | None:
    """Build syntaqlite with extension grammar using GN + Ninja.

    Returns the path to the built binary, or None on failure.
    """
    ext_flag = f'-DSYNTAQLITE_EXTENSION_GRAMMAR=\\"{_EXT_HEADER_REL}\\"'
    gn_args = f'is_debug=true extra_cflags="{ext_flag}"'

    # gn gen
    r = subprocess.run(
        [str(GN), "gen", str(BUILD_DIR), f"--args={gn_args}"],
        capture_output=True, text=True,
        cwd=str(ROOT_DIR),
    )
    if r.returncode != 0:
        print(f"FAIL: gn gen:\n{r.stderr}", file=sys.stderr)
        return None

    # ninja build (with file lock for concurrency safety)
    r = subprocess.run(
        [str(NINJA), "-C", str(BUILD_DIR), "src:syntaqlite"],
        capture_output=True, text=True,
        cwd=str(ROOT_DIR),
    )
    if r.returncode != 0:
        output = r.stdout + r.stderr
        print(f"FAIL: ninja build:\n{output}", file=sys.stderr)
        return None

    return BUILD_DIR / "syntaqlite"


def _run_parse_check(binary: Path, sql: str, label: str) -> bool:
    """Run the binary with `ast` subcommand and check for parse errors."""
    r = subprocess.run(
        [str(binary), "ast"],
        input=sql,
        capture_output=True,
        text=True,
    )
    if r.returncode != 0:
        print(f"  FAIL [{label}]: exit={r.returncode}")
        if r.stderr:
            print(f"    stderr: {r.stderr.strip()}")
        return False
    print(f"  OK   [{label}]")
    return True


def test_perfetto_compile_and_parse(runner: ToolRunner) -> bool:
    """Compile an extension-enabled binary and parse PerfettoSQL statements."""
    print("\n=== Perfetto Compile & Parse ===")

    header_path = BUILD_DIR / "gen" / _EXT_HEADER_REL

    print("Generating amalgamated extension header...")
    if not _generate_extension_header(runner, header_path):
        return False

    print("Building extension-enabled binary (GN + Ninja)...")
    binary = _build_extension_binary()
    if not binary:
        return False

    print("Testing parse acceptance:")
    ok = True

    # PerfettoSQL statements
    ok = _run_parse_check(
        binary,
        "CREATE PERFETTO FUNCTION foo(a INT) RETURNS INT AS SELECT 1;",
        "CREATE PERFETTO FUNCTION",
    ) and ok
    ok = _run_parse_check(
        binary,
        "INCLUDE PERFETTO MODULE my_module;",
        "INCLUDE PERFETTO MODULE",
    ) and ok
    ok = _run_parse_check(
        binary,
        "CREATE PERFETTO TABLE my_table AS SELECT 1;",
        "CREATE PERFETTO TABLE",
    ) and ok

    # Standard SQL regression checks
    ok = _run_parse_check(
        binary,
        "SELECT 1;",
        "standard SELECT",
    ) and ok
    ok = _run_parse_check(
        binary,
        "CREATE TABLE t(x);",
        "standard CREATE TABLE",
    ) and ok

    if ok:
        print("PASS: All statements parsed successfully")
    return ok


def main() -> int:
    from .sqlite_extractor.tools import ToolRunner

    runner = ToolRunner(root_dir=ROOT_DIR)
    ok = True
    ok = test_token_stability(runner) and ok
    ok = test_no_token_range_checks() and ok
    ok = test_perfetto_grammar_build(runner) and ok
    ok = test_perfetto_token_stability(runner) and ok
    ok = test_perfetto_parser_generation(runner) and ok
    ok = test_perfetto_compile_and_parse(runner) and ok
    return 0 if ok else 1
