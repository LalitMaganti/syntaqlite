# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Build syntaqlite grammar from SQLite's parse.y.

This module extracts and reconstructs a clean grammar file from SQLite's parse.y
that can be processed by Lemon with our custom templates.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .tools import ToolRunner


@dataclass
class LemonGrammarOutput:
    """Parsed output from lemon -g."""

    terminals: list[str] = field(default_factory=list)  # Token names
    nonterminals: list[str] = field(default_factory=list)
    rules: list[str] = field(default_factory=list)


def _parse_symbol_line(line: str) -> list[tuple[int, str]]:
    """Parse a symbol line from lemon -g output.

    Lines look like: "//   0 $                      162 INDEX"
    Each line has pairs of (id, name).
    """
    parts = line[2:].split()  # Strip leading "//"
    symbols = []
    for i in range(0, len(parts) - 1, 2):
        if parts[i].isdigit():
            symbols.append((int(parts[i]), parts[i + 1]))
    return symbols


def parse_lemon_g_output(output: str) -> LemonGrammarOutput:
    """Parse the output of lemon -g.

    Args:
        output: Raw output from lemon -g.

    Returns:
        LemonGrammarOutput with terminals, nonterminals, and rules.
    """
    result = LemonGrammarOutput()
    in_symbols = False

    for line in output.strip().split("\n"):
        if line.startswith("// Symbols:"):
            in_symbols = True
        elif in_symbols and line.startswith("//"):
            for _, name in _parse_symbol_line(line):
                if name == "$":
                    continue
                if name[0].islower():
                    result.nonterminals.append(name)
                else:
                    result.terminals.append(name)
        elif "::=" in line:
            in_symbols = False
            result.rules.append(line.strip())
        elif result.rules and line.strip() and not line.startswith("//"):
            result.rules.append(line.strip())

    return result


def split_extension_grammar(extension_grammar: str) -> tuple[str, str]:
    """Split extension grammar into directives and rules.

    Extension grammars need special handling:
    - %token and other directives go BEFORE base grammar (for token ordering)
    - Rules (containing ::=) go AFTER base grammar (to reference its nonterminals)

    Args:
        extension_grammar: Extension grammar content.

    Returns:
        Tuple of (directives, rules).
    """
    lines = extension_grammar.split('\n')
    # Filter out empty lines and comments
    meaningful = [ln for ln in lines if ln.strip() and not ln.strip().startswith('//')]
    directives = [ln for ln in meaningful if ln.strip().startswith('%') and '::=' not in ln]
    rules = [ln for ln in meaningful if '::=' in ln]
    return '\n'.join(directives), '\n'.join(rules)


def extract_fallback_from_grammar(grammar_content: str, valid_tokens: set[str]) -> tuple[str, list[str]]:
    """Extract fallback declaration from grammar, filtering to valid tokens.

    Args:
        grammar_content: Original grammar file content.
        valid_tokens: Set of valid token names to include.

    Returns:
        Tuple of (fallback_id, list of fallback tokens).
    """
    # Find %fallback declaration
    match = re.search(
        r"^%fallback\s+(\w+)\s+([\s\S]*?)\.\s*$",
        grammar_content,
        re.MULTILINE,
    )
    if not match:
        return "", []

    fallback_id = match.group(1)
    tokens_text = match.group(2)

    # Remove SQLite conditional directives
    tokens_text = re.sub(r"%ifdef\s+\w+", "", tokens_text)
    tokens_text = re.sub(r"%ifndef\s+\w+", "", tokens_text)
    tokens_text = re.sub(r"%endif\s+\w+", "", tokens_text)

    # Extract token names and filter to valid ones
    all_tokens = re.findall(r"\b([A-Z][A-Z0-9_]*)\b", tokens_text)
    filtered = [t for t in all_tokens if t in valid_tokens]

    # Dedupe while preserving order
    seen = set()
    unique = []
    for t in filtered:
        if t not in seen:
            seen.add(t)
            unique.append(t)

    return fallback_id, unique


def extract_precedence_from_grammar(grammar_content: str) -> list[str]:
    """Extract %left/%right/%nonassoc precedence rules from grammar.

    Args:
        grammar_content: Original grammar file content.

    Returns:
        List of precedence rule strings.
    """
    rules = []
    for match in re.finditer(
        r"^%(left|right|nonassoc)\s+([^.]+)\.", grammar_content, re.MULTILINE
    ):
        direction = match.group(1)
        tokens = match.group(2).strip()
        rules.append(f"%{direction} {tokens}.")
    return rules


def extract_wildcard_from_grammar(grammar_content: str) -> str | None:
    """Extract %wildcard directive from grammar.

    Args:
        grammar_content: Original grammar file content.

    Returns:
        Wildcard token name, or None if not present.
    """
    match = re.search(r"^%wildcard\s+(\w+)\.", grammar_content, re.MULTILINE)
    if match:
        return match.group(1)
    return None


def build_syntaqlite_grammar(
    runner: ToolRunner,
    prefix: str = "syntaqlite",
    extension_grammar: str | None = None,
) -> str:
    """Build a syntaqlite grammar from SQLite's parse.y using lemon -g.

    This function:
    1. Runs lemon -g to get clean rules and symbol table
    2. Extracts fallback/precedence from original grammar
    3. Generates a new grammar file with our customizations

    Args:
        runner: ToolRunner instance for running tools.
        prefix: Symbol prefix (default: "syntaqlite").
        extension_grammar: Optional extension grammar content to merge.

    Returns:
        Content of the generated grammar file.
    """
    import tempfile

    # Get SQLite's base grammar
    base_grammar = runner.get_base_grammar()

    # If we have an extension grammar, merge it with the base
    # Directives (like %token) go first for proper token ordering
    # Rules go last so they can reference base grammar nonterminals
    if extension_grammar:
        ext_directives, ext_rules = split_extension_grammar(extension_grammar)
        combined_grammar = ext_directives + "\n" + base_grammar
        if ext_rules:
            combined_grammar += "\n" + ext_rules
    else:
        combined_grammar = base_grammar

    # Write grammar to temp file and run lemon -g
    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir_path = Path(tmpdir)
        grammar_path = tmpdir_path / "parse.y"
        grammar_path.write_text(combined_grammar)

        # Get clean grammar output
        lemon_output = runner.run_lemon_grammar_only(grammar_path)

    # Parse the lemon -g output
    parsed = parse_lemon_g_output(lemon_output)

    # Extract fallback, precedence, and wildcard from original grammar
    valid_tokens = set(parsed.terminals)
    fallback_id, fallback_tokens = extract_fallback_from_grammar(combined_grammar, valid_tokens)
    precedence_rules = extract_precedence_from_grammar(combined_grammar)
    wildcard = extract_wildcard_from_grammar(combined_grammar)

    # Generate the grammar file
    return _generate_grammar_file(
        prefix=prefix,
        terminals=parsed.terminals,
        rules=parsed.rules,
        fallback_id=fallback_id,
        fallback_tokens=fallback_tokens,
        precedence_rules=precedence_rules,
        wildcard=wildcard,
    )


def _generate_grammar_file(
    prefix: str,
    terminals: list[str],
    rules: list[str],
    fallback_id: str,
    fallback_tokens: list[str],
    precedence_rules: list[str],
    wildcard: str | None = None,
) -> str:
    """Generate the grammar file content.

    Args:
        prefix: Symbol prefix.
        terminals: List of terminal (token) names.
        rules: List of grammar rules.
        fallback_id: Fallback target token (e.g., "ID").
        fallback_tokens: List of tokens that fall back.
        precedence_rules: List of precedence rules.
        wildcard: Optional wildcard token name.

    Returns:
        Generated grammar file content.
    """
    sqlite3_prefix = f"{prefix}_sqlite3"
    parts = []

    # Header comment
    parts.append(f"""// Grammar file for {prefix} parser.
// Generated from SQLite's parse.y with clean rules via lemon -g.
// DO NOT EDIT - regenerate with: python3 python/tools/extract_sqlite.py

""")

    # Parser name and token prefix first
    parts.append(f"""%name {sqlite3_prefix}Parser
%token_prefix TK_
%start_symbol input

""")

    # Include section with our defs and critical defines
    parts.append(f"""%include {{
#include "src/syntaqlite_sqlite_defs.h"

#define YYNOERRORRECOVERY 1
#define YYPARSEFREENEVERNULL 1
}}

""")

    # Token type and extra context
    parts.append(f"""%token_type {{SyntaqliteToken}}
%default_type {{SyntaqliteToken}}
%extra_context {{{prefix.capitalize()}ParseContext *pCtx}}

%syntax_error {{
  (void)yymajor;
  (void)TOKEN;
  if( pCtx && pCtx->onSyntaxError ) {{
    pCtx->onSyntaxError(pCtx);
  }}
}}

%stack_overflow {{
  if( pCtx && pCtx->onStackOverflow ) {{
    pCtx->onStackOverflow(pCtx);
  }}
}}

""")

    # Token declarations - group into lines
    # Filter out special tokens that shouldn't be declared
    skip_tokens = {"SEMI", "LP", "RP", "COMMA", "DOT", "EQ", "NE", "GT", "LT", "GE", "LE",
                   "PLUS", "MINUS", "STAR", "SLASH", "REM", "BITAND", "BITOR", "BITNOT",
                   "LSHIFT", "RSHIFT", "CONCAT", "PTR", "STRING", "INTEGER", "FLOAT",
                   "VARIABLE", "BLOB", "ID", "ILLEGAL", "SPACE", "COMMENT"}

    declarable_tokens = [t for t in terminals if t not in skip_tokens]

    if declarable_tokens:
        parts.append("// Token declarations\n")
        for i in range(0, len(declarable_tokens), 10):
            chunk = declarable_tokens[i:i + 10]
            parts.append(f"%token {' '.join(chunk)}.\n")
        parts.append("\n")

    # Precedence rules (before fallback, per Perfetto's example)
    if precedence_rules:
        parts.append("// Operator precedence\n")
        for rule in precedence_rules:
            parts.append(rule + "\n")
        parts.append("\n")

    # Fallback declaration
    if fallback_id and fallback_tokens:
        parts.append(f"// Fallback tokens to {fallback_id}\n")
        parts.append(f"%fallback {fallback_id}\n")
        for i in range(0, len(fallback_tokens), 8):
            chunk = fallback_tokens[i:i + 8]
            parts.append(f"  {' '.join(chunk)}\n")
        parts.append(".\n\n")

    # Wildcard
    if wildcard:
        parts.append(f"%wildcard {wildcard}.\n\n")

    # Grammar rules
    parts.append("// Grammar rules\n")
    for rule in rules:
        parts.append(rule + "\n")

    # Tokenizer-only tokens that must be declared but aren't used in rules
    # These must be at the end per SQLite's requirements
    parts.append("\n// Tokenizer-only tokens (not used in grammar rules)\n")
    parts.append("%token SPACE COMMENT ILLEGAL.\n")

    return "".join(parts)


