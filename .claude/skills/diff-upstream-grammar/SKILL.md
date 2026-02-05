---
name: diff-upstream-grammar
description: Verify that syntaqlite's hand-written AST action rules are in sync with the upstream SQLite grammar. Use when upgrading SQLite version, checking grammar compatibility, investigating AST action drift, or when the user mentions diffing grammar, upstream sync, or parse.y changes.
---

# Diff Upstream Grammar

Check whether the hand-written AST action rules in `python/syntaqlite/ast_codegen/ast_actions/*.y` are still compatible with the SQLite grammar in `third_party/src/sqlite/src/parse.y`.

Action file rule signatures **must match upstream parse.y exactly**. When SQLite updates its grammar, some signatures may change and the action files need updating.

## Running the diff

```bash
tools/dev/diff-upstream-grammar third_party/src/sqlite/src/parse.y
```

The tool uses Lemon `-g` to strip C action code from both grammars, producing clean rule signatures for comparison.

- **Exit 0**: No rule changes — AST actions are in sync.
- **Exit 1**: Changes found. The diff output shows added (`+`) / removed (`-`) rules. If any changed nonterminals have AST actions, a `WARNING` lists them with the affected files.

## When changes affect AST actions

The warning output names the nonterminals. Find the corresponding action files:

| File | Nonterminals |
|------|-------------|
| `_common.y` | input, cmdlist, ecmd, cmdx |
| `select.y` | cmd, select, selectnowith, oneselect, selcollist, sclp, scanpt, as, distinct, from, where_opt, groupby_opt, having_opt, orderby_opt, limit_opt |
| `expressions.y` | Expression-related rules |
| `identifiers.y` | nm, ids, identifier rules |
| `literals.y` | Literal value rules |

All files are in `python/syntaqlite/ast_codegen/ast_actions/`.

Update the rule signatures in the action files to match the new grammar exactly, preserving the C action code.

## After fixing action files

Regenerate extracted code and verify:

```bash
python3 python/tools/extract_sqlite.py --output-dir src
tools/dev/ninja -C out/mac_debug
out/mac_debug/syntaqlite_unittests
tools/tests/run-ast-diff-tests --binary out/mac_debug/ast_test
```

## Key files

| Path | Role |
|------|------|
| `tools/dev/diff-upstream-grammar` | CLI entry point |
| `python/tools/diff_upstream_grammar.py` | Implementation |
| `third_party/src/sqlite/src/parse.y` | Repo's current SQLite grammar |
| `python/syntaqlite/ast_codegen/ast_actions/*.y` | Hand-written AST action rules |
| `python/tools/extract_sqlite.py` | Full code regeneration |
