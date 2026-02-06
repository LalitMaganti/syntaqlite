---
name: add-ast-action
description: Add new CST-to-AST folding actions for SQLite grammar rules. Use when implementing new AST node types, adding grammar action rules, or extending the parser to handle additional SQL syntax. Covers the full workflow from node definition through testing, including debugging techniques.
---

# Add AST Action

Implement new CST-to-AST folding for SQLite grammar rules. This skill covers defining AST nodes, writing grammar action rules, code generation, and debugging.

## Step-by-step workflow

### 1. Identify upstream rules to implement

Check which upstream rules don't yet have action rules:

```bash
tools/dev/diff-upstream-grammar third_party/src/sqlite/src/parse.y
```

The `+` lines show upstream rules; `-` lines show our action rules. Rules present in upstream but not in our actions are candidates to implement.

For the exact rule signature (with labels), check the actual upstream grammar:

```bash
grep -n 'your_nonterminal' third_party/src/sqlite/src/parse.y
```

**Critical**: Rule signatures in action files MUST match upstream parse.y exactly (same tokens, same alternations like `ID|INDEXED|JOIN_KW`).

### 2. Define AST nodes

Create or edit a Python node definition file in `python/syntaqlite/ast_codegen/nodes/`.

```python
# python/syntaqlite/ast_codegen/nodes/your_module.py
from ..defs import Node, List, Enum, inline, index

ENUMS = [
    Enum("YourEnum", "VALUE_A", "VALUE_B"),
]

NODES = [
    Node("YourNode",
        field1=inline("u8"),                    # Scalars, enums, source spans
        field2=inline("SyntaqliteSourceSpan"),  # Source text reference
        field3=index("Expr"),                   # Node reference (nullable)
    ),
    List("YourList", child_type="YourNode"),    # Variable-length list
]
```

**Storage rules**:
- `inline()`: Scalars, enums, flags, source spans (non-cyclic data)
- `index()`: Node IDs for references to other AST nodes (nullable via `SYNTAQLITE_NULL_NODE`)

**Abstract types** (for index fields): `Expr`, `Stmt`, `TableSource`, `InsertSource`

Register the module in `python/syntaqlite/ast_codegen/nodes/__init__.py`:
```python
from .your_module import ENUMS as _YOUR_ENUMS, NODES as _YOUR_NODES
# Append to ENUMS and NODES lists
```

**Important**: Append new items AFTER existing ones — ordering affects binary compatibility (enum values and node tag IDs are sequential).

### 3. Write grammar action rules

Create a `.y` file in `python/syntaqlite/ast_codegen/ast_actions/`:

```c
// python/syntaqlite/ast_codegen/ast_actions/your_actions.y

// Conventions:
// - pCtx: SyntaqliteParseContext*
// - pCtx->astCtx: AST context for builder calls
// - pCtx->zSql: Original SQL text
// - Terminals: SyntaqliteToken with .z (pointer), .n (length), .type
// - Non-terminals: uint32_t node IDs

your_rule(A) ::= SOME_TOKEN(B) expr(C). {
    A = ast_your_node(pCtx->astCtx, syntaqlite_span(pCtx, B), C);
}
```

**Key patterns**:
- Token to source span: `syntaqlite_span(pCtx, B)` converts SyntaqliteToken to SyntaqliteSourceSpan
- No span: `SYNTAQLITE_NO_SPAN` for empty/missing spans
- Null node: `SYNTAQLITE_NULL_NODE` (0xFFFFFFFF) for nullable index fields
- Folding away: `expr(A) ::= LP expr(B) RP. { A = B; }` — discard wrapper
- Multi-token dispatch: `expr(A) ::= expr(L) PLUS|MINUS(OP) expr(R). { ... switch(OP.type) ... }`
- List building: `list_append(ctx, list_id, child)` — pass `SYNTAQLITE_NULL_NODE` as list_id for first element

**Precedence annotations**: If the upstream rule has a precedence marker like `[BITNOT]` or `[IN]`, include it in the action file between the `.` and `{`:
```c
expr(A) ::= expr(B) in_op(C) LP exprlist(D) RP. [IN] {
    ...
}
```
The grammar builder strips `[MARKER]` from signatures for matching, then preserves it in the generated output.

### 4. Add %type declarations if needed

If your nonterminal uses a non-default type (not `uint32_t`), add a `%type` declaration in `python/syntaqlite/sqlite_extractor/grammar_build.py` around line 418:

```python
%type your_nonterminal {{SyntaqliteToken}}   # For token-passing rules
%type your_nonterminal {{int}}                # For integer-passing rules
```

**Nonterminals that need `%type`**: Any nonterminal that passes through a `SyntaqliteToken` (like `nm`, `ids`, `as`, `likeop`) or an integer value (like `distinct`, `sortorder`, `multiselect_op`, `in_op`). If in doubt, check what the upstream parse.y declares with `%type`.

### 5. Regenerate, build, and test

```bash
python3 python/tools/extract_sqlite.py   # Regenerate all code
tools/dev/build-lock -C out/mac_debug     # Build (use build-lock for multi-agent)
tools/tests/run-ast-diff-tests --binary out/mac_debug/ast_test  # Run tests
```

### 6. Write tests

Create `tests/ast_diff_tests/your_suite.py`:

```python
from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite

class YourTests(TestSuite):
    def test_basic(self):
        return AstTestBlueprint(
            sql="SELECT your_syntax_here",
            out="""\
SelectStmt
  flags: 0
  ResultColumnList[1]
    ResultColumn
      flags: 0
      alias: null
      YourNode
        field1: VALUE_A
        ...
""",
        )
```

**Discovering expected output**: Run the test binary manually to see what the AST printer produces:
```bash
echo "SELECT your_syntax_here;" | out/mac_debug/ast_test
```

Test suites are auto-discovered — just create a `.py` file under `tests/ast_diff_tests/`.

## Debugging techniques

### Parser tracing

When a rule produces unexpected results or a syntax error, use the `--trace` flag on the test binary to enable Lemon's built-in parser trace:

```bash
echo "SELECT 1 UNION SELECT 2;" | out/mac_debug/ast_test --trace
```

Trace output goes to stderr and shows every shift/reduce action, which state the parser is in, and which rule it's reducing. The AST output still goes to stdout. This is invaluable for diagnosing:
- Why a token causes a syntax error (the parser is in a state that doesn't accept it)
- Why a rule isn't being reduced (precedence or conflict issue)
- Which alternative rule the parser chose at an ambiguity point

To see just the trace (suppress AST output):
```bash
echo "SELECT 1 UNION SELECT 2;" | out/mac_debug/ast_test --trace > /dev/null
```

**Note**: `--trace` is only available in debug builds (where `NDEBUG` is not defined). It's a no-op in release builds.

### Lemon state file

Run lemon without `-l` to generate a `.out` file showing all parser states and conflicts:

```bash
# The .out file is generated alongside the .c file
# Look for it in the build output directory after running extract_sqlite.py
```

### Cross-referencing upstream parse.y

When debugging why a rule doesn't match, compare your action signature against the ACTUAL upstream rule (not the lemon -g stripped version):

```bash
grep -A5 'your_nonterminal(A) ::=' third_party/src/sqlite/src/parse.y
```

Watch for:
- Token alternations: upstream may combine `ID|INDEXED|JOIN_KW` where we have separate rules
- The `idj` nonterminal: upstream uses `idj(X)` for function names, which lemon -g expands to token alternations
- `%ifdef`/`%ifndef` blocks: upstream grammar has conditional compilation that affects which rules exist

### Common failure modes

1. **Syntax error on valid SQL**: Your action rule signature doesn't match upstream exactly. Check token names, alternation order.

2. **Rule silently not applied (bare rule used instead)**: Signature mismatch due to precedence markers or label differences. Check that `_parse_actions_file` strips `[MARKER]` correctly.

3. **Fallback tokens breaking things**: Tokens like UNION/EXCEPT/INTERSECT can fall back to ID if `%ifdef SQLITE_OMIT_COMPOUND_SELECT` blocks are incorrectly included. We don't define OMIT macros, so `%ifdef` blocks should be excluded.

4. **Type mismatch errors**: Your nonterminal needs a `%type` declaration in `grammar_build.py`. Error will say something like "type mismatch" or you'll get a C compiler error about incompatible types.

5. **Generated builder wrong signature**: Check that your node field names don't conflict with Python's `Node()` first positional param `name`. Use `func_name` instead of `name` for fields.

## Key files reference

| File | Purpose |
|------|---------|
| `python/syntaqlite/ast_codegen/nodes/*.py` | AST node definitions (Python DSL) |
| `python/syntaqlite/ast_codegen/ast_actions/*.y` | Grammar action rules |
| `python/syntaqlite/ast_codegen/defs.py` | Node/List/Enum/inline/index DSL |
| `python/syntaqlite/ast_codegen/codegen.py` | C code generator |
| `python/syntaqlite/ast_codegen/validator.py` | Validates index field references |
| `python/syntaqlite/ast_codegen/nodes/__init__.py` | Module registry (import order = tag order) |
| `python/syntaqlite/sqlite_extractor/grammar_build.py` | Grammar merging + %type declarations |
| `third_party/src/sqlite/src/parse.y` | Upstream SQLite grammar (ground truth) |
| `src/ast/ast_test_main.c` | Test binary entry point (add tracing here) |
| `tests/ast_diff_tests/*.py` | AST diff test suites |
| `python/syntaqlite/diff_tests/testing.py` | Test framework (AstTestBlueprint, TestSuite) |
