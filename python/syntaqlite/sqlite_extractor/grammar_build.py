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


def parse_actions_content(content: str) -> dict[str, str]:
    """Parse grammar content with action code and extract rules with their full text.

    Args:
        content: Grammar content string with action code blocks.

    Returns:
        Dict mapping rule signatures (without type annotations) to full rule text
        (including type annotations and action code).
    """
    rules = {}

    # Parse rules with action blocks
    # Pattern: rule(A) ::= rhs. { action code }
    # Need to handle multi-line action blocks

    i = 0
    while i < len(content):
        # Skip comments
        if content[i:i+2] == '//':
            end = content.find('\n', i)
            i = end + 1 if end != -1 else len(content)
            continue

        # Look for ::=
        rule_start = content.find('::=', i)
        if rule_start == -1:
            break

        # Find the LHS (before ::=)
        lhs_start = content.rfind('\n', 0, rule_start)
        if lhs_start == -1:
            lhs_start = 0
        else:
            lhs_start += 1

        lhs = content[lhs_start:rule_start].strip()

        # Find the action block { ... }
        action_start = content.find('{', rule_start)
        if action_start == -1:
            i = rule_start + 3
            continue

        # Find the RHS (between ::= and {), excluding the . and precedence marker
        rhs_text = content[rule_start + 3:action_start].strip()
        # Extract and preserve precedence marker like [IN], [BITNOT]
        prec_marker = ""
        prec_match = re.search(r'\[(\w+)\]\s*$', rhs_text)
        if prec_match:
            prec_marker = f" [{prec_match.group(1)}]"
            rhs_text = rhs_text[:prec_match.start()].strip()
        if rhs_text.endswith('.'):
            rhs_text = rhs_text[:-1].strip()

        # Find matching closing brace
        brace_depth = 1
        j = action_start + 1
        while j < len(content) and brace_depth > 0:
            if content[j] == '{':
                brace_depth += 1
            elif content[j] == '}':
                brace_depth -= 1
            j += 1

        action_code = content[action_start:j].strip()

        # Build full rule text with type annotations and action code
        full_rule = f"{lhs} ::= {rhs_text}.{prec_marker} {action_code}"

        # Build signature without type annotations for matching, but keeping
        # the trailing dot and precedence marker to match lemon -g output.
        # "lhs(A)" -> "lhs", "rhs1(B) rhs2(C)" -> "rhs1 rhs2"
        def strip_annotations(s):
            return re.sub(r'\([A-Z]+\)', '', s).strip()

        lhs_clean = strip_annotations(lhs)
        rhs_clean = ' '.join(strip_annotations(tok) for tok in rhs_text.split())

        signature = f"{lhs_clean} ::= {rhs_clean}.{prec_marker}"
        if not rhs_clean:
            signature = f"{lhs_clean} ::=.{prec_marker}"

        rules[signature] = full_rule

        i = j

    return rules


def _parse_actions_file(actions_path: Path) -> dict[str, str]:
    """Parse a .y file with action code and extract rules with their full text."""
    if not actions_path.exists():
        return {}
    return parse_actions_content(actions_path.read_text())


# Directory containing action rule files
_ACTIONS_DIR = Path(__file__).parent.parent.parent.parent / "src" / "parser" / "actions"


def _load_all_actions(actions_dir: Path) -> dict[str, str]:
    """Load and merge action rules from all .y files in a directory.

    Args:
        actions_dir: Path to directory containing .y action files.

    Returns:
        Dict mapping rule signatures to full rule text.

    Raises:
        ValueError: If duplicate rule signatures are found across files.
    """
    merged: dict[str, str] = {}
    if not actions_dir.is_dir():
        return merged

    for path in sorted(actions_dir.glob("*.y")):
        file_rules = _parse_actions_file(path)
        for sig, text in file_rules.items():
            if sig in merged:
                raise ValueError(
                    f"Duplicate rule signature {sig!r} in {path.name} "
                    f"(already defined in a previous file)"
                )
            merged[sig] = text

    return merged


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


def _strip_action_blocks(content: str) -> str:
    """Strip { ... } action blocks from grammar content (brace-aware).

    This produces bare rules suitable for lemon -g, which doesn't need
    action code.
    """
    result = []
    i = 0
    while i < len(content):
        if content[i] == '{':
            # Skip to matching close brace
            depth = 1
            i += 1
            while i < len(content) and depth > 0:
                if content[i] == '{':
                    depth += 1
                elif content[i] == '}':
                    depth -= 1
                i += 1
        else:
            result.append(content[i])
            i += 1
    return ''.join(result)


def split_extension_grammar(extension_grammar: str) -> tuple[str, str]:
    """Split extension grammar into directives and bare rules.

    Extension grammars need special handling:
    - %token and other directives go BEFORE base grammar (for token ordering)
    - Rules (containing ::=) go AFTER base grammar (to reference its nonterminals)

    Action blocks { ... } are stripped so lemon -g gets clean bare rules.

    Args:
        extension_grammar: Extension grammar content (may contain action blocks).

    Returns:
        Tuple of (directives, bare_rules).
    """
    stripped = _strip_action_blocks(extension_grammar)
    lines = stripped.split('\n')
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

    # Process SQLite conditional directives properly.
    # We never define SQLITE_OMIT_* macros, so:
    #   %ifdef SQLITE_OMIT_X ... %endif  -> EXCLUDE (condition is false)
    #   %ifndef SQLITE_OMIT_X ... %endif -> INCLUDE (condition is true)
    # For %ifndef SQLITE_ENABLE_* (not defined) -> EXCLUDE
    # For %ifdef SQLITE_ENABLE_* (not defined) -> EXCLUDE
    # Simple approach: remove %ifdef blocks, keep %ifndef blocks (strip directives only)
    tokens_text = re.sub(
        r"%ifdef\s+\w+\s*([\s\S]*?)%endif\s+\w+", "", tokens_text
    )
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


def build_synq_grammar(
    runner: ToolRunner,
    prefix: str = "synq",
    extension_grammar: str | None = None,
) -> str:
    """Build a syntaqlite grammar from SQLite's parse.y using lemon -g.

    This function:
    1. Runs lemon -g on the base grammar to get stable base terminal ordering
    2. If extensions exist, runs lemon -g on combined grammar to get extension
       terminals and combined rules
    3. Extracts fallback/precedence from original grammar
    4. Generates a new grammar file with our customizations

    Extension tokens are always assigned IDs after all base tokens, so adding
    an extension never shifts base token numbers.

    Args:
        runner: ToolRunner instance for running tools.
        prefix: Symbol prefix (default: "synq").
        extension_grammar: Optional extension grammar content to merge.

    Returns:
        Content of the generated grammar file.
    """
    import tempfile

    # Get SQLite's base grammar
    base_grammar = runner.get_base_grammar()

    # Always run lemon -g on the base grammar first to get stable base terminals
    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir_path = Path(tmpdir)
        grammar_path = tmpdir_path / "parse.y"
        grammar_path.write_text(base_grammar)
        base_lemon_output = runner.run_lemon_grammar_only(grammar_path)

    base_parsed = parse_lemon_g_output(base_lemon_output)
    base_terminals = base_parsed.terminals

    if extension_grammar:
        ext_directives, ext_rules = split_extension_grammar(extension_grammar)
        combined_grammar = ext_directives + "\n" + base_grammar
        if ext_rules:
            combined_grammar += "\n" + ext_rules

        # Run lemon -g on combined grammar for rules and combined terminal list
        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir_path = Path(tmpdir)
            grammar_path = tmpdir_path / "parse.y"
            grammar_path.write_text(combined_grammar)
            combined_lemon_output = runner.run_lemon_grammar_only(grammar_path)

        combined_parsed = parse_lemon_g_output(combined_lemon_output)

        # Extension terminals = terminals in combined but not in base
        base_set = set(base_terminals)
        extension_terminals = [t for t in combined_parsed.terminals if t not in base_set]

        # Use base ordering + extension terminals appended, combined rules
        terminals = base_terminals
        rules = combined_parsed.rules
    else:
        combined_grammar = base_grammar
        extension_terminals = []
        terminals = base_terminals
        rules = base_parsed.rules

    # Extract fallback, precedence, and wildcard from original grammar
    valid_tokens = set(terminals) | set(extension_terminals)
    fallback_id, fallback_tokens = extract_fallback_from_grammar(combined_grammar, valid_tokens)
    precedence_rules = extract_precedence_from_grammar(combined_grammar)
    wildcard = extract_wildcard_from_grammar(combined_grammar)

    # Parse action rules from extension grammar (if it contains action blocks)
    extension_action_rules = parse_actions_content(extension_grammar) if extension_grammar else {}

    # Generate the grammar file
    return _generate_grammar_file(
        prefix=prefix,
        terminals=terminals,
        rules=rules,
        fallback_id=fallback_id,
        fallback_tokens=fallback_tokens,
        precedence_rules=precedence_rules,
        wildcard=wildcard,
        extension_terminals=extension_terminals,
        extension_action_rules=extension_action_rules,
    )


def _generate_grammar_file(
    prefix: str,
    terminals: list[str],
    rules: list[str],
    fallback_id: str,
    fallback_tokens: list[str],
    precedence_rules: list[str],
    wildcard: str | None = None,
    extension_terminals: list[str] | None = None,
    extension_action_rules: dict[str, str] | None = None,
) -> str:
    """Generate the grammar file content.

    Args:
        prefix: Symbol prefix.
        terminals: List of base terminal (token) names.
        rules: List of grammar rules.
        fallback_id: Fallback target token (e.g., "ID").
        fallback_tokens: List of tokens that fall back.
        precedence_rules: List of precedence rules.
        wildcard: Optional wildcard token name.
        extension_terminals: Optional list of extension terminal names.
            These are emitted after base terminals to ensure stable numbering.
        extension_action_rules: Optional dict of extension action rules
            (signature -> full rule text) parsed from extension grammar.

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
%token_prefix SYNTAQLITE_TOKEN_
%start_symbol input

""")

    # Include section with our defs and critical defines
    parts.append(f"""%include {{
#include "src/common/synq_sqlite_defs.h"
#include "syntaqlite/sqlite_tokens_gen.h"
#include "src/parser/ast_base.h"
#include "src/parser/ast_builder_gen.h"

#define YYNOERRORRECOVERY 1
#define YYPARSEFREENEVERNULL 1
}}

""")

    # Token type and extra context
    # Terminals are SynqToken, non-terminals default to u32 (node IDs)
    parts.append(f"""%token_type {{SynqToken}}
%default_type {{u32}}
%extra_context {{{prefix.capitalize()}ParseContext *pCtx}}

// Non-terminals that pass through token info (not node IDs)
%type nm {{SynqToken}}
%type as {{SynqToken}}
%type scanpt {{SynqToken}}
%type likeop {{SynqToken}}
%type dbnm {{SynqToken}}
%type multiselect_op {{int}}
%type in_op {{int}}
%type typetoken {{SynqToken}}
%type typename {{SynqToken}}
%type withnm {{SynqToken}}
%type wqas {{int}}
%type collate {{int}}
%type raisetype {{int}}
%type joinop {{int}}
%type indexed_by {{SynqToken}}

// DML support
%type insert_cmd {{int}}
%type orconf {{int}}
%type resolvetype {{int}}
%type indexed_opt {{SynqToken}}

// Schema/Transaction support
%type ifexists {{int}}
%type transtype {{int}}
%type trans_opt {{int}}
%type savepoint_opt {{int}}
%type kwcolumn_opt {{int}}
%type columnname {{SynqColumnNameValue}}

// Utility/Create support
%type ifnotexists {{int}}
%type temp {{int}}
%type uniqueflag {{int}}
%type database_kw_opt {{int}}
%type explain {{int}}
%type createkw {{SynqToken}}
%type signed {{SynqToken}}
%type plus_num {{SynqToken}}
%type minus_num {{SynqToken}}
%type nmnum {{SynqToken}}

// CREATE TABLE support
%type autoinc {{int}}
%type refargs {{int}}
%type refarg {{int}}
%type refact {{int}}
%type defer_subclause {{int}}
%type init_deferred_pred_opt {{int}}
%type defer_subclause_opt {{int}}
%type table_option_set {{int}}
%type table_option {{int}}
%type ccons {{SynqConstraintValue}}
%type carglist {{SynqConstraintListValue}}
%type tcons {{SynqConstraintValue}}
%type conslist {{SynqConstraintListValue}}
%type conslist_opt {{u32}}
%type tconscomma {{int}}
%type onconf {{int}}
%type scantok {{SynqToken}}
%type generated {{SynqConstraintValue}}

// JOIN ON/USING discriminator
%type on_using {{SynqOnUsingValue}}

// WITH clause (recursive flag + cte list)
%type with {{SynqWithValue}}

// Window function support
%type range_or_rows {{int}}
%type frame_exclude_opt {{int}}
%type frame_exclude {{int}}

// Trigger support
%type trigger_time {{int}}
%type trnm {{SynqToken}}

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
    # All base terminals are declared here to lock their IDs and prevent
    # extension tokens from shifting them. SPACE/COMMENT/ILLEGAL are also
    # re-declared at the end (harmless duplicate that lemon ignores) to
    # maintain compatibility with the original grammar structure.
    declarable_tokens = list(terminals)

    if declarable_tokens:
        parts.append("// Base token declarations\n")
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

    # Load rules with action code from ast_actions/ directory
    action_rules = _load_all_actions(_ACTIONS_DIR)

    # Merge extension action rules (extension rules override in case of conflict)
    if extension_action_rules:
        action_rules.update(extension_action_rules)

    # Grammar rules - use action version if available, otherwise bare rule
    parts.append("// Grammar rules\n")
    for rule in rules:
        if rule in action_rules:
            parts.append(action_rules[rule] + "\n")
        else:
            parts.append(rule + "\n")

    # Extension token declarations (after all base tokens for stable numbering)
    if extension_terminals:
        parts.append("\n// Extension token declarations\n")
        for i in range(0, len(extension_terminals), 10):
            chunk = extension_terminals[i:i + 10]
            parts.append(f"%token {' '.join(chunk)}.\n")

    # Tokenizer-only tokens re-declared at the end for clarity.
    # These are already declared in the base token block above (lemon ignores
    # duplicates), but we keep this for documentation purposes.
    # NOTE: Unlike upstream SQLite, we do NOT use the >=TK_SPACE range check
    # optimization, so these tokens do not need to have the highest IDs.
    # See parser.c for the explicit equality checks we use instead.
    parts.append("\n// Tokenizer-only tokens (not used in grammar rules)\n")
    parts.append("%token SPACE COMMENT ILLEGAL.\n")

    return "".join(parts)


def parse_extension_keywords(grammar_content: str) -> list[str]:
    """Parse %token declarations from extension grammar content.

    Args:
        grammar_content: Extension grammar text.

    Returns:
        Deduplicated list of keyword names preserving order.
    """
    keywords = []
    for match in re.finditer(r'%token\s+([^%{]+?)(?=\n(?:%|\s*$)|$)', grammar_content, re.DOTALL):
        keywords.extend(re.findall(r'\b([A-Z][A-Z0-9_]*)\b', match.group(1)))
    # Dedupe preserving order
    return list(dict.fromkeys(keywords))


