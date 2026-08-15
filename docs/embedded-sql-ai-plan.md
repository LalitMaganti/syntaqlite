# Embedded SQL Plan

Status: Prototype (error recovery infrastructure landed)
Last updated: 2026-02-28

## Goal

Add LSP support for SQL embedded inside host language strings: Python f-strings, JavaScript template literals, etc. The primary consumers are AI coding tools (Claude, Gemini, Codex) that speak LSP, not just VS Code.

The approach is **server-side**: the smarts live in the syntaqlite engine, not in editor-specific extensions. Any LSP client gets embedded SQL support for free.

## Design Principles

1. **Server-side extraction**: the syntaqlite engine understands host languages, not the client
2. **Holes, not heuristics**: interpolation expressions (`{x}`, `${x}`) become typed ErrorNode placeholders in the AST, not synthetic identifiers
3. **Reuse existing infrastructure**: `begin_macro`/`end_macro` for hole regions, Lemon error recovery for hole parsing, the validation walker already handles `Other` nodes gracefully
4. **Incremental**: start with Python f-strings, extend to JS/TS template literals, then others

## Architecture

```
  Host language file (Python, JS, ...)
          │
          ▼
  ┌─────────────────────┐
  │  String Extractor    │  Language-specific: finds SQL strings,
  │  (per-language)      │  identifies hole positions + types
  └──────────┬──────────┘
             │  EmbeddedSqlFragment[]
             ▼
  ┌─────────────────────┐
  │  Hole-Aware Parser   │  LowLevelParser + begin_macro/end_macro
  │  (language-agnostic) │  Feeds TK_ILLEGAL for each hole
  └──────────┬──────────┘
             │  ParseError { root: Some(tree) }
             ▼
  ┌─────────────────────┐
  │  Validation Walker   │  Existing walker: ErrorNodes are
  │  (unchanged)         │  silently skipped in all positions
  └──────────┬──────────┘
             │  Vec<Diagnostic> with SQL-relative offsets
             ▼
  ┌─────────────────────┐
  │  Offset Mapper       │  Maps SQL offsets back to host file
  │  (per-fragment)      │  positions, accounting for quotes,
  └──────────┬──────────┘  prefixes, escape sequences
             │
             ▼
     LSP Diagnostics at host file positions
```

## What's Already Built

### Error recovery grammar rules

Fine-grained error recovery so holes don't consume the rest of the statement:

```
# syntaqlite-syntax/parser-actions/expressions.y
expr(A) ::= error. {
    A = synq_parse_error_node(pCtx, pCtx->error_offset, pCtx->error_length);
}

# syntaqlite-syntax/parser-actions/identifiers.y
nm(A) ::= error. {
    A.z = NULL;
    A.n = 0;
}
```

These are in `ALLOWED_EXTRA_RULES` in `grammar_verify.rs`.

### ParseError with recovered trees

`ParseError` now carries `root: Option<NodeId>`:

```rust
pub struct ParseError {
    pub message: String,
    pub offset: Option<usize>,
    pub length: Option<usize>,
    pub root: Option<NodeId>,  // recovered partial tree
}
```

The C parser returns code `2` for "tree completed with error recovery". The Rust side maps this to `Err(ParseError { root: Some(id), .. })`.

### Validation continues past errors

`validate_dialect` collects recovered trees and validates them:

```rust
while let Some(result) = cursor.next_statement() {
    match result {
        Ok(id) => stmt_ids.push(id),
        Err(err) => {
            if let Some(id) = err.root {
                stmt_ids.push(id);  // validate recovered tree
            }
        }
    }
}
```

### Walker handles ErrorNodes safely

ErrorNode (tag 0) flows through the AST dispatch as `Other`:
- In expr position → `ExprKind::Other` → `walk_other_node` → no children → no-op
- In name position → `nm ::= error { A.z = NULL }` → `table_name()` returns empty → skipped
- As root → `StmtKind::Other` → `walk_other_node` → no-op

### Macro regions for verbatim formatting

`begin_macro(call_offset, call_length)` / `end_macro()` already mark regions where the formatter emits the original source text verbatim. This is exactly right for interpolation holes: `{user_id}` should format as `{user_id}`, not get reformatted.

### Prototype tests

`syntaqlite/tests/holes.rs` has 6 passing tests covering:
- Hole in expression position (`WHERE id = {user_id}`)
- Hole in table name position (`FROM {table}`)
- Table name hole with trailing valid clause
- Multiple holes in one statement
- Hole as trailing clause
- Baseline: TK_ID inside macro region

## What Needs Building

### Layer 1: String Extractor (per-language)

A lightweight tokenizer for each host language that:
1. Finds SQL-containing string literals
2. Identifies the string boundaries (excluding quotes, prefix characters)
3. Locates interpolation holes and their byte ranges

**Not a full parser.** We only need to find strings and their holes. The extractor doesn't need to understand the host language's AST: just its string literal syntax.

#### Python f-strings

```python
query = f"SELECT * FROM {table} WHERE id = {user_id}"
#         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^  SQL content
#                        ^~~~~^       ^~~~~~~^   holes
```

Input to extractor: full Python source text.
Output: list of `EmbeddedSqlFragment`:

```rust
pub struct EmbeddedFragment { /* private fields */ }

impl EmbeddedFragment {
    pub fn sql_range(&self) -> &Range<usize>;  // byte range in host file
    pub fn sql_text(&self) -> &str;            // SQL with holes replaced
    pub fn holes(&self) -> &[Hole];            // interpolation holes
}

pub struct Hole { /* private fields */ }

impl Hole {
    pub fn host_range(&self) -> &Range<usize>; // byte range in host file
    pub fn sql_offset(&self) -> usize;         // offset within sql_text
}
```

#### JavaScript template literals

```javascript
const query = sql`SELECT * FROM ${table} WHERE id = ${userId}`;
```

Same structure, different string syntax. The extractor needs to handle `${}` instead of `{}`, backtick strings, and tagged templates.

#### Detecting SQL strings

Heuristic options (in order of preference):

| Method | Pros | Cons |
|--------|------|------|
| Tag/prefix detection (`sql\`...\``, `f"SELECT..."`) | Simple, low false positives | Misses untagged strings |
| SQL keyword detection (string starts with SELECT/INSERT/etc) | Catches more strings | May false-positive on non-SQL |
| Explicit annotation (`# syntaqlite: sql`) | Zero false positives | Requires user action |
| Client-provided ranges | Maximally flexible | Shifts work to client |

Phase 1 should support tag/prefix detection plus client-provided ranges. Keyword detection can be added later.

### Layer 2: Hole-Aware Feed

Given an `EmbeddedSqlFragment`, feed it through `LowLevelParser`:

```rust
fn parse_embedded_sql(
    parser: &mut LowLevelParser,
    fragment: &EmbeddedSqlFragment,
) -> Result<NodeId, ParseError> {
    let mut tokenizer = Tokenizer::with_dialect(dialect);
    let mut cursor = parser.feed(&fragment.sql_text);

    let mut hole_idx = 0;
    for tok in tokenizer.tokenize(&fragment.sql_text) {
        // Check if this token overlaps with a hole
        if hole_idx < fragment.holes.len() {
            let hole = &fragment.holes[hole_idx];
            if tok.start == hole.sql_offset {
                // Feed the hole as TK_ILLEGAL inside a macro region
                cursor.begin_macro(
                    hole.host_range.start as u32,
                    hole.host_range.len() as u32,
                );
                cursor.feed_token(TK_ILLEGAL, tok.start..tok.end)?;
                cursor.end_macro();
                hole_idx += 1;
                continue;
            }
        }
        cursor.feed_token(tok.type_, tok.start..tok.end)?;
    }

    // finish() returns Err with root on recovery
    match cursor.finish()? {
        Some(id) => Ok(id),
        None => Err(ParseError { message: "empty".into(), ..Default::default() }),
    }
}
```

The `begin_macro`/`end_macro` wrapping serves dual purposes:
1. **Formatter**: emits the original `{user_id}` text verbatim
2. **Offset tracking**: the macro region records the host-file byte range

### Layer 3: Offset Mapper

Diagnostics from the validation walker use byte offsets into the SQL text. These need mapping back to the host file.

```rust
pub struct OffsetMapper {
    /// Base offset of the SQL content in the host file.
    sql_base: usize,

    /// Sorted list of (sql_offset, host_offset_delta) adjustments.
    /// Each hole introduces a delta because the hole text in the host file
    /// (e.g., `{user_id}`) may differ in length from the placeholder in
    /// the SQL text.
    adjustments: Vec<(usize, isize)>,
}

impl OffsetMapper {
    pub fn to_host_offset(&self, sql_offset: usize) -> usize {
        // Binary search adjustments, accumulate deltas
        let delta: isize = self.adjustments.iter()
            .take_while(|(off, _)| *off <= sql_offset)
            .map(|(_, d)| d)
            .sum();
        (self.sql_base as isize + sql_offset as isize + delta) as usize
    }
}
```

### Layer 4: AnalysisHost Integration

New method on `AnalysisHost`:

```rust
impl<'d> AnalysisHost<'d> {
    /// Open/update a host language document containing embedded SQL.
    pub fn open_host_document(
        &mut self,
        uri: &str,
        version: i32,
        text: String,
        language: HostLanguage,
    );

    /// Get diagnostics for embedded SQL in a host language document.
    /// Returns diagnostics with offsets mapped to the host file.
    pub fn host_diagnostics(
        &mut self,
        uri: &str,
    ) -> &[Diagnostic];
}

pub enum HostLanguage {
    Python,
    JavaScript,
    TypeScript,
}
```

Internally, `open_host_document` runs the string extractor, parses each fragment with hole-aware feeding, validates, maps offsets, and caches the result.

### Layer 5: CLI + LSP Integration

#### CLI

```sh
# Validate embedded SQL in Python files
syntaqlite validate --lang python myapp.py

# Validate SQL in JS files
syntaqlite validate --lang javascript query.js

# Auto-detect from file extension
syntaqlite validate myapp.py
```

#### LSP

The `syntaqlite lsp` server auto-detects host languages by `documentSelector` and file extension:

```json
{
  "documentSelector": [
    { "language": "sql" },
    { "language": "python" },
    { "language": "javascript" },
    { "language": "typescript" }
  ]
}
```

For `.py`/`.js`/`.ts` files, the server uses `open_host_document` instead of `open_document`. Diagnostics are returned as normal LSP `publishDiagnostics` with correct host-file positions.

## Phasing

### Phase 1: Python f-string extraction + validation

- Python string extractor (f-strings only, no raw strings or multi-line initially)
- Hole-aware feed using `LowLevelParser`
- Offset mapping back to Python file
- CLI: `syntaqlite validate --lang python`
- Tests: diff tests with Python files containing embedded SQL

### Phase 2: JavaScript/TypeScript template literals

- JS/TS string extractor (template literals, tagged templates)
- Handle `${}` interpolation syntax
- Tagged template detection (`sql\`...\``, `db.query\`...\``)

### Phase 3: AnalysisHost integration

- `open_host_document` / `host_diagnostics` API
- LSP server support for Python/JS/TS files
- Semantic tokens for SQL regions within host files

### Phase 4: Formatting within strings

- Format SQL inside f-strings/template literals in-place
- Preserve interpolation holes, string delimiters, indentation
- Use `begin_macro`/`end_macro` verbatim emission for holes

### Phase 5: Completions within strings

- Complete table/column/function names inside embedded SQL
- Use `expected_tokens_at_offset` with offset mapping
- Schema-aware completions via `SessionContext`

## Python f-string Extraction Details

### String types to support

| String type | Example | SQL detection |
|-------------|---------|---------------|
| f-string | `f"SELECT * FROM {t}"` | Starts with SQL keyword |
| Tagged f-string | _(N/A in Python)_ |: |
| Raw f-string | `rf"SELECT * FROM {t}"` | Same as f-string |
| Multi-line f-string | `f"""SELECT ..."""` | Same, handle triple quotes |
| Concatenated | `f"SELECT " + f"FROM {t}"` | Future: join fragments |

### Hole parsing

Python f-string holes can contain arbitrary expressions:

```python
f"SELECT * FROM {get_table()}"          # function call
f"WHERE id = {obj.user_id}"             # attribute access
f"LIMIT {10 if debug else 100}"         # conditional
f"WHERE name = '{name}'"                # nested quotes
f"SELECT {','.join(cols)} FROM t"       # complex expression
```

We don't need to parse the Python expression: just find matching braces. Handle:
- Nested `{}` (dict literals, set comprehensions)
- Strings within holes (which may contain `}`)
- `{{` / `}}` escape sequences (literal braces, not holes)

### Tokenizer approach

A minimal Python string tokenizer that:
1. Scans for string prefixes (`f"`, `f'`, `f"""`, `f'''`, `rf"`, etc.)
2. Inside strings, scans for `{` not preceded by `{` (not `{{`)
3. Tracks brace depth to find matching `}`
4. Records hole byte ranges and SQL content byte ranges

This is ~100–200 lines of Rust, not a full Python parser.

## JavaScript Template Literal Extraction Details

### String types to support

| String type | Example | SQL detection |
|-------------|---------|---------------|
| Tagged template | `` sql`SELECT * FROM ${t}` `` | Tag name contains `sql`/`query`/`db` |
| Untagged template | `` `SELECT * FROM ${t}` `` | Starts with SQL keyword |
| Regular string | `"SELECT * FROM " + t` | Future: concatenation analysis |

### Hole parsing

JS template literal holes also contain arbitrary expressions:

```javascript
sql`SELECT * FROM ${getTable()}`
sql`WHERE id = ${obj.userId}`
sql`LIMIT ${debug ? 10 : 100}`
```

Same approach: find `${`, track brace depth to find matching `}`.

## Open Questions

1. **Multi-statement strings**: `f"CREATE TABLE t(x); INSERT INTO t VALUES({v})"`: do we handle multiple statements in one string? The parser already supports multi-statement parsing, so this should work.

2. **String concatenation**: `f"SELECT * FROM " + f"{table}"`: joining fragments across concatenation operators is harder. Defer to Phase 2+.

3. **Nested f-strings**: Python 3.12+ allows `f"{'SELECT' if x else 'INSERT'}"`: the SQL content itself could be dynamic. These are fundamentally unparseable; skip them.

4. **Schema context**: How does the LSP know about the database schema for a Python project? Options: parse `CREATE TABLE` in `.sql` files, read `alembic` migrations, accept a config file, or let the client provide `SessionContext`. Start with client-provided context.

5. **False positive suppression**: What if a string looks like SQL but isn't? Provide a `# syntaqlite: ignore` comment directive, or only activate on tagged strings / explicit opt-in.

6. **Format specifiers**: `f"WHERE id = {uid!r}"`: the `!r` is a Python format spec, not SQL. The hole extractor must strip format specifiers from the hole range.

7. **Escape sequences**: `f"WHERE name = 'O\\'Brien'"`: Python string escapes affect the SQL content. The extractor must unescape before feeding to the SQL parser.
