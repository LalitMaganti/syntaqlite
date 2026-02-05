# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Check that syntaqlite's AST action rules are in sync with upstream SQLite grammar.

Compares rule signatures from our hand-written AST action files against the
rules in SQLite's parse.y (processed via lemon -g). Only rules for nonterminals
that we have AST actions for are compared.

Usage:
    python3 python/tools/diff_upstream_grammar.py                    # Check against repo grammar
    python3 python/tools/diff_upstream_grammar.py /path/to/parse.y   # Check against new grammar
"""

from __future__ import annotations

import argparse
import difflib
import sys
from pathlib import Path

ROOT_DIR = Path(__file__).parent.parent.parent
if str(ROOT_DIR) not in sys.path:
    sys.path.insert(0, str(ROOT_DIR))

from python.syntaqlite.sqlite_extractor.tools import ToolRunner
from python.syntaqlite.sqlite_extractor.grammar_build import _load_all_actions, _ACTIONS_DIR

REPO_GRAMMAR = ROOT_DIR / "third_party" / "src" / "sqlite" / "src" / "parse.y"


def extract_rules(runner: ToolRunner, grammar_path: Path) -> list[str]:
    """Run lemon -g and return sorted rule lines (no actions, no symbol table)."""
    output = runner.run_lemon_grammar_only(grammar_path)
    return sorted(line.strip() for line in output.splitlines() if "::=" in line)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("upstream_grammar", nargs="?", type=Path, default=None,
                        help="Path to upstream parse.y (default: repo copy)")
    args = parser.parse_args()

    grammar_path = args.upstream_grammar or REPO_GRAMMAR

    if not grammar_path.exists():
        print(f"Grammar not found: {grammar_path}", file=sys.stderr)
        return 1

    # Load our AST action rule signatures
    actions = _load_all_actions(_ACTIONS_DIR)
    our_nonterminals = {sig.split(" ::=")[0] for sig in actions}
    our_rules = sorted(actions.keys())

    # Extract upstream grammar rules, filtered to nonterminals we have actions for
    runner = ToolRunner(root_dir=ROOT_DIR)
    all_upstream_rules = extract_rules(runner, grammar_path)
    upstream_rules = sorted(
        rule for rule in all_upstream_rules
        if rule.split(" ::=")[0] in our_nonterminals
    )

    try:
        grammar_label = str(grammar_path.relative_to(ROOT_DIR))
    except ValueError:
        grammar_label = str(grammar_path)

    print(f"AST actions: {_ACTIONS_DIR.relative_to(ROOT_DIR)}/  ({len(our_rules)} rules)")
    print(f"Upstream:    {grammar_label}  ({len(upstream_rules)} matching rules)")
    print()

    diff = list(difflib.unified_diff(
        our_rules, upstream_rules,
        fromfile="ast_actions", tofile="upstream", lineterm=""))

    if not diff:
        print("All AST action rules are in sync with upstream grammar.")
        return 0

    for line in diff:
        print(line)

    # Summarize affected nonterminals
    changed_nts = set()
    for line in diff:
        if line.startswith(("+", "-")) and "::=" in line:
            nt = line[1:].strip().split(" ::=")[0]
            changed_nts.add(nt)

    if changed_nts:
        print()
        print("Affected nonterminals:")
        for nt in sorted(changed_nts):
            print(f"  - {nt}")
        print()
        print("Review files in:")
        print(f"  {_ACTIONS_DIR.relative_to(ROOT_DIR)}/")

    return 1


if __name__ == "__main__":
    sys.exit(main())
