# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Parse action code from our ast_grammar.y.

This module extracts rules and their action code from our grammar file,
including which builder functions are called in each action.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class RuleWithAction:
    """A grammar rule with its associated action code."""

    lhs: str  # Nonterminal name (without type annotation)
    rhs: list[str]  # RHS symbols (without type annotations)
    action_code: str  # The C code in { ... }
    builder_calls: list[str] = field(default_factory=list)  # e.g., ["ast_select"]
    signature: str = field(default="", init=False)

    def __post_init__(self):
        rhs_str = " ".join(self.rhs) if self.rhs else ""
        self.signature = f"{self.lhs} ::= {rhs_str}".strip()


def _strip_type_annotation(symbol: str) -> str:
    """Strip type annotation from a symbol: 'expr(A)' -> 'expr'."""
    match = re.match(r"(\w+)(?:\([^)]*\))?", symbol)
    return match.group(1) if match else symbol


def _extract_builder_calls(action_code: str) -> list[str]:
    """Extract builder function calls from action code.

    Builder functions follow the pattern: ast_<name>(...)

    Args:
        action_code: C code from the action block.

    Returns:
        List of builder function names.
    """
    # Match ast_* function calls
    pattern = r"\b(ast_\w+)\s*\("
    matches = re.findall(pattern, action_code)
    return list(dict.fromkeys(matches))  # Dedupe while preserving order


def _parse_grammar_file(content: str) -> list[RuleWithAction]:
    """Parse grammar file content into rules with actions.

    This parser handles multi-line rules and action blocks.
    Lemon grammar rules have the format:
        lhs(A) ::= rhs1(B) rhs2(C). { action_code }

    Args:
        content: Grammar file content.

    Returns:
        List of RuleWithAction objects.
    """
    rules: list[RuleWithAction] = []

    # State machine for parsing
    current_text = ""
    brace_depth = 0
    in_action = False
    action_start = 0

    i = 0
    while i < len(content):
        c = content[i]

        # Track brace depth for action blocks
        if c == "{":
            if brace_depth == 0:
                in_action = True
                action_start = i
            brace_depth += 1
        elif c == "}":
            brace_depth -= 1
            if brace_depth == 0 and in_action:
                # End of action block - extract the rule
                action_code = content[action_start + 1 : i].strip()

                # Parse the rule text before the action
                rule_text = current_text.strip()
                if "::=" in rule_text:
                    rule = _parse_rule_with_action(rule_text, action_code)
                    if rule:
                        rules.append(rule)

                current_text = ""
                in_action = False

        # Accumulate text outside of action blocks
        if not in_action:
            # Handle comments
            if c == "/" and i + 1 < len(content):
                if content[i + 1] == "/":
                    # Line comment - skip to end of line
                    end = content.find("\n", i)
                    if end == -1:
                        i = len(content)
                    else:
                        i = end
                    continue
                elif content[i + 1] == "*":
                    # Block comment - skip to */
                    end = content.find("*/", i + 2)
                    if end == -1:
                        i = len(content)
                    else:
                        i = end + 2
                    continue

            # Skip directive lines (but we've already handled comments)
            if c == "%" and (i == 0 or content[i - 1] == "\n"):
                # Skip to end of line
                end = content.find("\n", i)
                if end == -1:
                    i = len(content)
                else:
                    i = end
                continue

            current_text += c

        i += 1

    # Handle any remaining rule without action
    if current_text.strip() and "::=" in current_text:
        # Rule without action (just period)
        rule_text = current_text.strip()
        # Check for period ending
        if "." in rule_text:
            period_idx = rule_text.rfind(".")
            rule_text = rule_text[:period_idx].strip()
            rule = _parse_rule_with_action(rule_text, "")
            if rule:
                rules.append(rule)

    return rules


def _parse_rule_with_action(rule_text: str, action_code: str) -> RuleWithAction | None:
    """Parse a single rule with its action.

    Args:
        rule_text: The rule text (lhs ::= rhs).
        action_code: The action code (without braces).

    Returns:
        RuleWithAction or None if invalid.
    """
    # Remove trailing period if present
    rule_text = rule_text.rstrip(". \t\n")

    # Split on ::=
    if "::=" not in rule_text:
        return None

    parts = rule_text.split("::=", 1)
    if len(parts) != 2:
        return None

    lhs_part = parts[0].strip()
    rhs_part = parts[1].strip()

    # Extract LHS symbol
    lhs = _strip_type_annotation(lhs_part)
    if not lhs:
        return None

    # Split RHS into symbols
    rhs_tokens = rhs_part.split()
    rhs = [_strip_type_annotation(tok) for tok in rhs_tokens if tok]

    # Extract builder calls from action
    builder_calls = _extract_builder_calls(action_code)

    return RuleWithAction(
        lhs=lhs,
        rhs=rhs,
        action_code=action_code,
        builder_calls=builder_calls,
    )


def extract_actions(grammar_content: str) -> dict[str, RuleWithAction]:
    """Extract rules and their action code from grammar content.

    Args:
        grammar_content: Content of ast_grammar.y.

    Returns:
        Dictionary mapping signature strings to RuleWithAction objects.
    """
    rules = _parse_grammar_file(grammar_content)
    return {rule.signature: rule for rule in rules}


def extract_actions_from_file(grammar_path: Path) -> dict[str, RuleWithAction]:
    """Extract rules and their action code from a grammar file.

    Args:
        grammar_path: Path to ast_grammar.y.

    Returns:
        Dictionary mapping signature strings to RuleWithAction objects.
    """
    content = grammar_path.read_text()
    return extract_actions(content)


def get_all_builder_calls(rules: dict[str, RuleWithAction]) -> set[str]:
    """Get all unique builder function calls across all rules.

    Args:
        rules: Dictionary of rules with actions.

    Returns:
        Set of builder function names.
    """
    calls: set[str] = set()
    for rule in rules.values():
        calls.update(rule.builder_calls)
    return calls
