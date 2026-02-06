# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Check that syntaqlite's AST action rules are in sync with upstream SQLite grammar.

Compares rule signatures from our hand-written AST action files against the
rules in SQLite's parse.y (processed via lemon -g). Only rules for nonterminals
that we have AST actions for are compared.

Usage:
    tools/dev/diff-upstream-grammar                    # Categorized summary (default)
    tools/dev/diff-upstream-grammar --diff             # Raw unified diff
    tools/dev/diff-upstream-grammar /path/to/parse.y   # Check against new grammar
"""

from __future__ import annotations

import argparse
import collections
import difflib
import sys
from pathlib import Path

ROOT_DIR = Path(__file__).parent.parent.parent
if str(ROOT_DIR) not in sys.path:
    sys.path.insert(0, str(ROOT_DIR))

from python.syntaqlite.sqlite_extractor.tools import ToolRunner
from python.syntaqlite.sqlite_extractor.grammar_build import (
    _load_all_actions,
    _parse_actions_file,
    _ACTIONS_DIR,
)

REPO_GRAMMAR = ROOT_DIR / "third_party" / "src" / "sqlite" / "src" / "parse.y"


def _extract_upstream_rules(runner: ToolRunner, grammar_path: Path) -> list[str]:
    """Run lemon -g and return sorted rule lines."""
    output = runner.run_lemon_grammar_only(grammar_path)
    return sorted(line.strip() for line in output.splitlines() if "::=" in line)


def _load_actions_by_file(actions_dir: Path) -> dict[str, str]:
    """Map each signature -> source filename (without extension)."""
    sig_to_file: dict[str, str] = {}
    if not actions_dir.is_dir():
        return sig_to_file
    for path in sorted(actions_dir.glob("*.y")):
        for sig in _parse_actions_file(path):
            sig_to_file[sig] = path.stem
    return sig_to_file


def _print_categorized(
    our_rules: set[str],
    upstream_rules: set[str],
    sig_to_file: dict[str, str],
) -> bool:
    """Print categorized comparison. Returns True if there are differences."""
    # Group by nonterminal
    our_nts: dict[str, set[str]] = collections.defaultdict(set)
    upstream_nts: dict[str, set[str]] = collections.defaultdict(set)
    for r in our_rules:
        our_nts[r.split(" ::=")[0]].add(r)
    for r in upstream_rules:
        upstream_nts[r.split(" ::=")[0]].add(r)

    all_nts = sorted(set(our_nts) | set(upstream_nts))

    in_sync = []
    extra_upstream = []
    extra_ours = []
    both_differ = []

    for nt in all_nts:
        ours = our_nts.get(nt, set())
        theirs = upstream_nts.get(nt, set())
        has_extra_upstream = bool(theirs - ours)
        has_extra_ours = bool(ours - theirs)
        if not has_extra_upstream and not has_extra_ours:
            in_sync.append(nt)
        elif has_extra_upstream and has_extra_ours:
            both_differ.append(nt)
        elif has_extra_upstream:
            extra_upstream.append(nt)
        else:
            extra_ours.append(nt)

    matched = our_rules & upstream_rules
    has_diff = our_rules != upstream_rules

    # Summary
    print(f"  In sync:          {len(in_sync)} nonterminals ({len(matched)} rules)")
    if extra_upstream:
        count = sum(len(upstream_nts[nt] - our_nts.get(nt, set())) for nt in extra_upstream)
        print(f"  Upstream extra:   {len(extra_upstream)} nonterminals ({count} rules)")
    if extra_ours:
        count = sum(len(our_nts.get(nt, set()) - upstream_nts.get(nt, set())) for nt in extra_ours)
        print(f"  AST action extra: {len(extra_ours)} nonterminals ({count} rules)")
    if both_differ:
        print(f"  Both differ:      {len(both_differ)} nonterminals")

    if not has_diff:
        print()
        print("All AST action rules are in sync with upstream grammar.")
        return False

    def _print_nt_section(nt: str) -> None:
        ours = our_nts.get(nt, set())
        theirs = upstream_nts.get(nt, set())
        missing = sorted(theirs - ours)
        extra = sorted(ours - theirs)
        files = sorted({sig_to_file[r] for r in ours if r in sig_to_file})
        file_hint = f"  ({', '.join(f + '.y' for f in files)})" if files else ""
        matched_count = len(ours & theirs)
        print(f"  {nt}{file_hint}")
        if matched_count:
            print(f"    {matched_count} rule(s) in sync")
        for r in missing:
            print(f"    + {r}")
        for r in extra:
            print(f"    - {r}")

    if extra_upstream:
        print()
        total = sum(len(upstream_nts[nt] - our_nts.get(nt, set())) for nt in extra_upstream)
        print(f"Upstream has extra rules ({total}):")
        for nt in extra_upstream:
            _print_nt_section(nt)

    if both_differ:
        print()
        print("Both sides differ:")
        for nt in both_differ:
            _print_nt_section(nt)

    if extra_ours:
        print()
        total = sum(len(our_nts.get(nt, set()) - upstream_nts.get(nt, set())) for nt in extra_ours)
        print(f"AST actions have extra rules ({total}):")
        for nt in extra_ours:
            _print_nt_section(nt)

    if in_sync:
        print()
        print(f"In sync ({len(in_sync)}): {', '.join(in_sync)}")

    return True


def _print_unified_diff(our_rules: list[str], upstream_rules: list[str]) -> bool:
    """Print unified diff. Returns True if there are differences."""
    diff = list(difflib.unified_diff(
        our_rules, upstream_rules,
        fromfile="ast_actions", tofile="upstream", lineterm=""))

    if not diff:
        print("All AST action rules are in sync with upstream grammar.")
        return False

    for line in diff:
        print(line)

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

    return True


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("upstream_grammar", nargs="?", type=Path, default=None,
                        help="Path to upstream parse.y (default: repo copy)")
    parser.add_argument("--diff", action="store_true",
                        help="Show raw unified diff instead of categorized output")
    args = parser.parse_args()

    grammar_path = args.upstream_grammar or REPO_GRAMMAR

    if not grammar_path.exists():
        print(f"Grammar not found: {grammar_path}", file=sys.stderr)
        return 1

    # Load our AST action rule signatures
    actions = _load_all_actions(_ACTIONS_DIR)
    our_nonterminals = {sig.split(" ::=")[0] for sig in actions}
    our_rules_sorted = sorted(actions.keys())
    our_rules_set = set(actions.keys())

    # Track which file each signature came from
    sig_to_file = _load_actions_by_file(_ACTIONS_DIR)

    # Extract upstream grammar rules, filtered to nonterminals we have actions for
    runner = ToolRunner(root_dir=ROOT_DIR)
    all_upstream = _extract_upstream_rules(runner, grammar_path)
    upstream_filtered = sorted(
        rule for rule in all_upstream
        if rule.split(" ::=")[0] in our_nonterminals
    )
    upstream_rules_set = set(upstream_filtered)

    try:
        grammar_label = str(grammar_path.relative_to(ROOT_DIR))
    except ValueError:
        grammar_label = str(grammar_path)

    print(f"AST actions: {_ACTIONS_DIR.relative_to(ROOT_DIR)}/  ({len(our_rules_sorted)} rules)")
    print(f"Upstream:    {grammar_label}  ({len(upstream_filtered)} matching rules)")
    print()

    if args.diff:
        has_diff = _print_unified_diff(our_rules_sorted, upstream_filtered)
    else:
        has_diff = _print_categorized(our_rules_set, upstream_rules_set, sig_to_file)

    if has_diff:
        print()
        print("Review files in:")
        print(f"  {_ACTIONS_DIR.relative_to(ROOT_DIR)}/")

    return 1 if has_diff else 0


if __name__ == "__main__":
    sys.exit(main())
