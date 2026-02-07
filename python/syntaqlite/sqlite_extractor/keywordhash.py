# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Shared keywordhash processing for both base and extension builds.

Both extract_sqlite.py and build_extension_grammar.py need to process
raw mkkeywordhash output. This module provides a single shared function
that uses the Pipeline/Transform system for all symbol renames, then
does clean structural splitting to separate data from function.
"""

from __future__ import annotations

import re
from dataclasses import dataclass

from .transforms import Pipeline, SymbolRename, RemoveFunctionCalls
from .tools import (
    create_symbol_rename_pipeline,
    create_keywordhash_rename_pipeline,
)

# Structural markers in mkkeywordhash output
KEYWORDHASH_SCORE_MARKER = "/* Hash score:"
KEYWORD_CODE_FUNC_MARKER = "static int keywordCode("


@dataclass
class KeywordhashResult:
    """Result of processing mkkeywordhash output.

    data_section: Tables + hash param #defines (swapped by extensions).
    keyword_code_func: Parameterized keywordCode function (generic, lives in base).
    """
    data_section: str
    keyword_code_func: str


def _extract_hash_params(func_text: str) -> tuple[int, int, int, int]:
    """Extract hash formula parameters from a keywordCode function.

    The function contains a line like:
        i = ((charMap(z[0])*4) ^ (charMap(z[n-1])*3) ^ n*1) % 127;
    (or with renamed symbols like synq_sqlite3Tolower instead of charMap)

    Returns (c0, c1, c2, mod) — the four magic numbers.
    """
    m = re.search(
        r'z\[0\]\)\*(\d+)\)\s*\^\s*\(\w+\(z\[n-1\]\)\*(\d+)\)\s*\^\s*n\*(\d+)\)\s*%\s*(\d+)',
        func_text,
    )
    if not m:
        raise ValueError("Could not extract hash parameters from keywordCode function")
    return int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4))


def _format_hash_param_defines(prefix: str, c0: int, c1: int, c2: int, mod: int) -> str:
    """Format hash parameter #defines for the data section."""
    p = prefix.upper()
    return (
        f"#define {p}_KW_HASH_C0  {c0}\n"
        f"#define {p}_KW_HASH_C1  {c1}\n"
        f"#define {p}_KW_HASH_C2  {c2}\n"
        f"#define {p}_KW_HASH_MOD {mod}\n"
    )


def _replace_hash_formula_with_defines(func_text: str, prefix: str) -> str:
    """Replace hardcoded hash numbers in keywordCode with #define references."""
    p = prefix.upper()
    return re.sub(
        r'(\w+\(z\[0\]\)\*)\d+(\)\s*\^\s*\(\w+\(z\[n-1\]\)\*)\d+(\)\s*\^\s*n\*)\d+(\)\s*%\s*)\d+',
        rf'\g<1>{p}_KW_HASH_C0\2{p}_KW_HASH_C1\3{p}_KW_HASH_C2\4{p}_KW_HASH_MOD',
        func_text,
    )


def process_keywordhash(raw_output: str, prefix: str) -> KeywordhashResult:
    """Process mkkeywordhash output into data section and keywordCode function.

    Processing steps:
    1. Pipeline — apply all symbol renames to the entire raw output at once.
    2. Structural split — find markers to separate data from function.
    3. Hash param extraction — parse magic numbers, emit as #defines.
    4. Function rename — make keywordCode non-static with prefix.

    Args:
        raw_output: Raw stdout from mkkeywordhash.
        prefix: Symbol prefix (e.g. "synq").

    Returns:
        KeywordhashResult with data_section and keyword_code_func.
    """
    sqlite3_prefix = f"{prefix}_sqlite3"

    # Step 1: Pipeline — all renames at once
    pipeline = Pipeline([
        # sqlite3 -> synq_sqlite3, charMap -> Tolower, testcase -> renamed
        *create_symbol_rename_pipeline(prefix).transforms,
        # TK_ -> SYNTAQLITE_TOKEN_
        SymbolRename("TK_", "SYNTAQLITE_TOKEN_"),
        # Array names: zKWText -> synq_zKWText, etc.
        *create_keywordhash_rename_pipeline(prefix).transforms,
        # Strip renamed testcase calls
        RemoveFunctionCalls(f"{sqlite3_prefix}Testcase"),
    ])
    output = pipeline.apply(raw_output)

    # Step 2: Structural split — find boundaries in generated output
    start = output.find(KEYWORDHASH_SCORE_MARKER)
    if start == -1:
        start = 0
    generated = output[start:]

    keyword_code_start = generated.find(KEYWORD_CODE_FUNC_MARKER)
    if keyword_code_start == -1:
        raise ValueError("Could not find keywordCode function in mkkeywordhash output")

    data_section = generated[:keyword_code_start].strip()

    logic_section = generated[keyword_code_start:]
    keyword_code_end = logic_section.find('\n}\n')
    if keyword_code_end != -1:
        logic_section = logic_section[:keyword_code_end + 3]

    # Step 3: Hash param extraction — parse magic numbers, add #defines to data
    c0, c1, c2, mod = _extract_hash_params(logic_section)
    data_section += "\n\n" + _format_hash_param_defines(prefix, c0, c1, c2, mod)

    # Replace hardcoded numbers with #define references in function
    logic_section = _replace_hash_formula_with_defines(logic_section, prefix)

    # Step 4: Function rename — remove static, add prefix
    logic_section = logic_section.replace(
        "static int keywordCode(", f"int {prefix}_keywordCode("
    )

    return KeywordhashResult(
        data_section=data_section.strip(),
        keyword_code_func=logic_section.strip(),
    )
