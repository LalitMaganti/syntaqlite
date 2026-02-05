# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Extract rule signatures from SQLite's parse.y.

This module parses the upstream parse.y grammar file and extracts
rule signatures (LHS ::= RHS symbols) for comparison with our
ast_grammar.y to detect upstream changes.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class RuleSignature:
    """A grammar rule signature without action code."""

    lhs: str  # Nonterminal name (without type annotation)
    rhs: list[str]  # RHS symbols (without type annotations)
    signature: str = field(default="", init=False)  # "lhs ::= rhs1 rhs2 ..."

    def __post_init__(self):
        rhs_str = " ".join(self.rhs) if self.rhs else ""
        self.signature = f"{self.lhs} ::= {rhs_str}".strip()


def _strip_type_annotation(symbol: str) -> str:
    """Strip type annotation from a symbol: 'expr(A)' -> 'expr'."""
    match = re.match(r"(\w+)(?:\([^)]*\))?", symbol)
    return match.group(1) if match else symbol


def _parse_rule_line(line: str) -> tuple[str, list[str]] | None:
    """Parse a single rule line.

    Args:
        line: A line that may contain a rule (with ::=).

    Returns:
        Tuple of (lhs, rhs_symbols) or None if not a rule.
    """
    # Skip comments
    if line.strip().startswith("//") or line.strip().startswith("/*"):
        return None

    # Look for rule separator
    if "::=" not in line:
        return None

    # Split on ::=
    parts = line.split("::=", 1)
    if len(parts) != 2:
        return None

    lhs_part = parts[0].strip()
    rhs_part = parts[1].strip()

    # Extract LHS symbol (strip type annotation)
    lhs = _strip_type_annotation(lhs_part)
    if not lhs:
        return None

    # Remove action code (everything after { ... })
    # Also remove trailing period
    rhs_part = re.sub(r"\{[^}]*\}", "", rhs_part)
    rhs_part = rhs_part.rstrip(". \t")

    # Split RHS into symbols and strip type annotations
    rhs_tokens = rhs_part.split()
    rhs = [_strip_type_annotation(tok) for tok in rhs_tokens if tok and tok != "."]

    return lhs, rhs


def extract_rules(grammar_content: str) -> dict[str, RuleSignature]:
    """Extract all rule signatures from a grammar file.

    Args:
        grammar_content: Content of a .y grammar file.

    Returns:
        Dictionary mapping signature strings to RuleSignature objects.
    """
    rules: dict[str, RuleSignature] = {}

    # Handle multi-line rules by joining continued lines
    # Lemon rules end with a period, so we accumulate until we see one
    current_rule = ""

    for line in grammar_content.split("\n"):
        # Skip directives
        stripped = line.strip()
        if stripped.startswith("%"):
            continue
        if stripped.startswith("//"):
            continue
        if stripped.startswith("/*"):
            continue

        # Accumulate rule text
        current_rule += " " + line

        # Check if rule is complete (ends with period or has action block)
        if "." in current_rule or "{" in current_rule:
            # Try to parse this as a rule
            result = _parse_rule_line(current_rule)
            if result:
                lhs, rhs = result
                sig = RuleSignature(lhs=lhs, rhs=rhs)
                rules[sig.signature] = sig
            current_rule = ""

    return rules


def extract_rules_from_file(grammar_path: Path) -> dict[str, RuleSignature]:
    """Extract rule signatures from a grammar file.

    Args:
        grammar_path: Path to a .y grammar file.

    Returns:
        Dictionary mapping signature strings to RuleSignature objects.
    """
    content = grammar_path.read_text()
    return extract_rules(content)


def extract_rules_from_lemon_g(lemon_output: str) -> dict[str, RuleSignature]:
    """Extract rule signatures from lemon -g output.

    The lemon -g output format has clean rules without action code,
    making it easier to parse than the original grammar file.

    Args:
        lemon_output: Output from running lemon -g on a grammar.

    Returns:
        Dictionary mapping signature strings to RuleSignature objects.
    """
    rules: dict[str, RuleSignature] = {}

    for line in lemon_output.split("\n"):
        line = line.strip()

        # Skip comments and empty lines
        if not line or line.startswith("//"):
            continue

        # Rules in lemon -g output look like:
        # "input ::= cmdlist"
        # They don't have action code or type annotations
        if "::=" not in line:
            continue

        parts = line.split("::=", 1)
        if len(parts) != 2:
            continue

        lhs = parts[0].strip()
        rhs_part = parts[1].strip().rstrip(".")

        rhs = rhs_part.split() if rhs_part else []

        sig = RuleSignature(lhs=lhs, rhs=rhs)
        rules[sig.signature] = sig

    return rules


def compare_rules(
    upstream_rules: dict[str, RuleSignature],
    our_rules: dict[str, RuleSignature],
) -> tuple[list[str], list[str], list[str]]:
    """Compare upstream rules against our rules.

    Args:
        upstream_rules: Rules from SQLite's parse.y.
        our_rules: Rules from our ast_grammar.y.

    Returns:
        Tuple of (added, removed, common) signature lists.
        - added: Rules in upstream but not in ours (need actions)
        - removed: Rules in ours but not in upstream (orphaned)
        - common: Rules in both
    """
    upstream_sigs = set(upstream_rules.keys())
    our_sigs = set(our_rules.keys())

    added = sorted(upstream_sigs - our_sigs)
    removed = sorted(our_sigs - upstream_sigs)
    common = sorted(upstream_sigs & our_sigs)

    return added, removed, common
