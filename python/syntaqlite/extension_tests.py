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

import argparse
import re
import subprocess
import sys
import time
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .sqlite_extractor.tools import ToolRunner

from .sqlite_extractor.grammar_build import build_synq_grammar
from .diff_tests.utils import Colors, colorize

ROOT_DIR = Path(__file__).parent.parent.parent
PERFETTO_GRAMMAR = ROOT_DIR / "tests" / "extensions" / "grammars" / "perfetto.y"

# Module-level verbosity, set by main(). 0=quiet, 1=results, 2=progress.
_verbosity = 0


def log(msg: str) -> None:
    """Print a message only at verbosity >= 2 (-vv)."""
    if _verbosity >= 2:
        print(msg)


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


def test_token_stability(runner: ToolRunner) -> tuple[bool, str]:
    """Verify that extension grammars never shift base token IDs."""
    log("Building grammar WITHOUT extension...")
    base_grammar = build_synq_grammar(runner, prefix="synq")

    log("Building grammar WITH fake extension...")
    ext_grammar = build_synq_grammar(
        runner, prefix="synq", extension_grammar=_FAKE_EXTENSION,
    )

    log("Extracting token IDs from both grammars...")
    base_ids = _extract_token_ids(base_grammar, runner)
    ext_ids = _extract_token_ids(ext_grammar, runner)

    if not base_ids:
        return False, "No tokens found in base grammar"

    # Every base token must have the same ID in the extension build
    failures = []
    for name, base_id in sorted(base_ids.items()):
        ext_id = ext_ids.get(name)
        if ext_id is None:
            failures.append(f"  {name}: missing from extension build")
        elif ext_id != base_id:
            failures.append(f"  {name}: base={base_id} ext={ext_id}")

    if failures:
        msg = "Base token IDs shifted when extension was added:\n" + "\n".join(failures)
        return False, msg

    # Extension tokens must have IDs greater than all base tokens
    max_base_id = max(base_ids.values())
    ext_only_tokens = {
        name: tid for name, tid in ext_ids.items() if name not in base_ids
    }

    if not ext_only_tokens:
        return False, "No extension-only tokens found"

    bad_ext = {
        name: tid for name, tid in ext_only_tokens.items() if tid <= max_base_id
    }
    if bad_ext:
        lines = [f"  {name}: {tid}" for name, tid in sorted(bad_ext.items())]
        msg = f"Extension tokens have IDs <= max base ID ({max_base_id}):\n" + "\n".join(lines)
        return False, msg

    return True, ""


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


def test_no_token_range_checks() -> tuple[bool, str]:
    """Verify we never use >=TK_SPACE style range checks on token IDs."""
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
        msg = ("Found token-ID range checks (use equality checks instead):\n"
               + "\n".join(violations)
               + "\n\nUpstream SQLite uses `tokenType >= TK_SPACE` but we avoid this"
               "\nso that dialect extensions can add tokens without ID constraints.")
        return False, msg

    return True, ""


def _load_perfetto_grammar() -> str:
    """Load the cleaned Perfetto extension grammar."""
    if not PERFETTO_GRAMMAR.exists():
        raise FileNotFoundError(f"Perfetto grammar not found: {PERFETTO_GRAMMAR}")
    return PERFETTO_GRAMMAR.read_text()


def test_perfetto_grammar_build(runner: ToolRunner) -> tuple[bool, str]:
    """Verify the Perfetto extension grammar merges with SQLite's grammar."""
    perfetto = _load_perfetto_grammar()
    log("Building grammar with Perfetto extension...")
    grammar = build_synq_grammar(runner, prefix="synq", extension_grammar=perfetto)

    if not grammar:
        return False, "build_synq_grammar returned empty"

    # Verify extension rules made it into the merged grammar
    expected_rules = [
        "PERFETTO",
        "DELEGATES",
        "macro_body",
        "module_name",
    ]
    missing = [r for r in expected_rules if r not in grammar]
    if missing:
        return False, f"Missing expected content in merged grammar: {missing}"

    return True, ""


def test_perfetto_token_stability(runner: ToolRunner) -> tuple[bool, str]:
    """Verify Perfetto extension doesn't shift base token IDs."""
    perfetto = _load_perfetto_grammar()

    log("Building grammar WITHOUT extension...")
    base_grammar = build_synq_grammar(runner, prefix="synq")

    log("Building grammar WITH Perfetto extension...")
    ext_grammar = build_synq_grammar(
        runner, prefix="synq", extension_grammar=perfetto,
    )

    log("Extracting token IDs...")
    base_ids = _extract_token_ids(base_grammar, runner)
    ext_ids = _extract_token_ids(ext_grammar, runner)

    if not base_ids:
        return False, "No tokens found in base grammar"

    failures = []
    for name, base_id in sorted(base_ids.items()):
        ext_id = ext_ids.get(name)
        if ext_id is None:
            failures.append(f"  {name}: missing from extension build")
        elif ext_id != base_id:
            failures.append(f"  {name}: base={base_id} ext={ext_id}")

    if failures:
        msg = "Base token IDs shifted with Perfetto extension:\n" + "\n".join(failures)
        return False, msg

    max_base_id = max(base_ids.values())
    ext_only = {n: t for n, t in ext_ids.items() if n not in base_ids}

    if not ext_only:
        return False, "No Perfetto-only tokens found"

    bad = {n: t for n, t in ext_only.items() if t <= max_base_id}
    if bad:
        lines = [f"  {name}: {tid}" for name, tid in sorted(bad.items())]
        msg = f"Perfetto tokens have IDs <= max base ID ({max_base_id}):\n" + "\n".join(lines)
        return False, msg

    return True, ""


def test_perfetto_parser_generation(runner: ToolRunner) -> tuple[bool, str]:
    """Verify full parser data can be generated from Perfetto grammar."""
    # Import here to avoid circular deps at module level
    from python.tools.build_extension_grammar import generate_parser_data

    log("Generating parser data from Perfetto grammar...")
    parser_data = generate_parser_data(runner, PERFETTO_GRAMMAR, prefix="synq", verbose=_verbosity >= 2)

    if not parser_data:
        return False, "generate_parser_data returned empty"

    # Check for expected sections in the output
    expected = [
        "SYNTAQLITE_TOKEN_",         # Token defines
        "Parsing tables",            # Parser tables section
        "synq_parser_reduce",        # Reduce function
    ]
    missing = [e for e in expected if e not in parser_data]
    if missing:
        return False, f"Missing expected sections: {missing}"

    return True, ""


BUILD_DIR = ROOT_DIR / "out" / "perfetto_ext"
# Path within gen dir — resolves via -Igen which is already on the include path.
_EXT_HEADER_REL = "tests/extensions/perfetto.h"
GN = ROOT_DIR / "tools" / "dev" / "gn"
NINJA = ROOT_DIR / "tools" / "dev" / "build-lock"


def _generate_extension_header(
    runner: ToolRunner,
    output_path: Path,
) -> tuple[bool, str]:
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
        return False, "No keywords in Perfetto grammar"

    token_defines = generate_token_defines(runner, PERFETTO_GRAMMAR)
    keywordhash_data = generate_keywordhash_data(runner, extra_keywords)
    parser_data = generate_parser_data(runner, PERFETTO_GRAMMAR, prefix="synq", verbose=_verbosity >= 2)

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
        verbose=_verbosity >= 2,
    )
    gen.write(output_path, content)
    return True, ""


def _build_extension_binary() -> tuple[Path | None, str]:
    """Build syntaqlite with extension grammar using GN + Ninja.

    Returns (path_to_binary, error_message) tuple.
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
        return None, f"gn gen:\n{r.stderr}"

    # ninja build (with file lock for concurrency safety)
    r = subprocess.run(
        [str(NINJA), "-C", str(BUILD_DIR), "src:syntaqlite"],
        capture_output=True, text=True,
        cwd=str(ROOT_DIR),
    )
    if r.returncode != 0:
        output = r.stdout + r.stderr
        return None, f"ninja build:\n{output}"

    return BUILD_DIR / "syntaqlite", ""


def _run_parse_check(binary: Path, sql: str, label: str) -> tuple[bool, str]:
    """Run the binary with `ast` subcommand and check for parse errors."""
    r = subprocess.run(
        [str(binary), "ast"],
        input=sql,
        capture_output=True,
        text=True,
    )
    if r.returncode != 0:
        msg = f"[{label}]: exit={r.returncode}"
        if r.stderr:
            msg += f"\n    stderr: {r.stderr.strip()}"
        return False, msg
    return True, ""


def test_perfetto_compile_and_parse(runner: ToolRunner) -> tuple[bool, str]:
    """Compile an extension-enabled binary and parse PerfettoSQL statements."""
    log("Generating amalgamated extension header...")
    header_path = BUILD_DIR / "gen" / _EXT_HEADER_REL
    ok, err = _generate_extension_header(runner, header_path)
    if not ok:
        return False, err

    log("Building extension-enabled binary (GN + Ninja)...")
    binary, err = _build_extension_binary()
    if not binary:
        return False, err

    log("Testing parse acceptance...")

    # PerfettoSQL statements
    checks = [
        ("CREATE PERFETTO FUNCTION foo(a INT) RETURNS INT AS SELECT 1;", "CREATE PERFETTO FUNCTION"),
        ("INCLUDE PERFETTO MODULE my_module;", "INCLUDE PERFETTO MODULE"),
        ("CREATE PERFETTO TABLE my_table AS SELECT 1;", "CREATE PERFETTO TABLE"),
        # Standard SQL regression checks
        ("SELECT 1;", "standard SELECT"),
        ("CREATE TABLE t(x);", "standard CREATE TABLE"),
    ]

    failures = []
    for sql, label in checks:
        ok, err = _run_parse_check(binary, sql, label)
        if not ok:
            failures.append(err)

    if failures:
        return False, "\n".join(failures)

    return True, ""


# Test registry: (name, function, needs_runner)
_TESTS = [
    ("token_stability", test_token_stability, True),
    ("no_token_range_checks", test_no_token_range_checks, False),
    ("perfetto_grammar_build", test_perfetto_grammar_build, True),
    ("perfetto_token_stability", test_perfetto_token_stability, True),
    ("perfetto_parser_generation", test_perfetto_parser_generation, True),
    ("perfetto_compile_and_parse", test_perfetto_compile_and_parse, True),
]


def main() -> int:
    parser = argparse.ArgumentParser(description="Run dialect extension tests")
    parser.add_argument('-v', '--verbose', action='count', default=0,
                        help='Increase verbosity (-v for results, -vv for progress)')
    args = parser.parse_args()

    global _verbosity
    _verbosity = args.verbose

    from .sqlite_extractor.tools import ToolRunner

    runner = ToolRunner(root_dir=ROOT_DIR, verbose=_verbosity >= 2)

    test_count = len(_TESTS)
    if _verbosity >= 1:
        print(f"[==========] Running {test_count} tests.")

    start_time = time.time()
    passed_count = 0
    failed_names: list[str] = []

    for name, test_fn, needs_runner in _TESTS:
        t0 = time.time()

        if _verbosity >= 2:
            print(f"[ RUN      ] {name}")

        if needs_runner:
            ok, err = test_fn(runner)
        else:
            ok, err = test_fn()

        elapsed_ms = int((time.time() - t0) * 1000)

        if ok:
            passed_count += 1
            if _verbosity >= 1:
                msg = colorize("[       OK ]", Colors.GREEN)
                print(f"{msg} {name} ({elapsed_ms} ms)")
        else:
            failed_names.append(name)
            if _verbosity >= 1:
                msg = colorize("[  FAILED  ]", Colors.RED)
                print(f"{msg} {name} ({elapsed_ms} ms)")
            print(err)

    elapsed_ms = int((time.time() - start_time) * 1000)

    if _verbosity >= 1:
        print(f"[==========] {test_count} tests ran. ({elapsed_ms} ms total)")

    if passed_count > 0:
        msg = colorize("[  PASSED  ]", Colors.GREEN)
        print(f"{msg} {passed_count} tests.")

    if failed_names:
        msg = colorize("[  FAILED  ]", Colors.RED)
        print(f"{msg} {len(failed_names)} tests, listed below:")
        for name in failed_names:
            print(f"{msg} {name}")
        print()
        print(f" {len(failed_names)} FAILED TESTS")

    return 0 if not failed_names else 1
