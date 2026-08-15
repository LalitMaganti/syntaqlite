# Formatter Architecture Plan

## Context

syntaqlite has a working parser on `rust-rewrite`: C Lemon parser via FFI, ~95 AST node types in an arena (u32 node IDs), streaming model. The `.synq` files define a Wadler-like fmt DSL. No formatter exists yet. We need a Wadler-Lindig pretty printer with comment preservation and macro support.

---

## Design Decisions Summary

| Decision | Choice |
|----------|--------|
| Rendering algorithm | Lindig's strict (stack-based), like prettier |
| Bytecode | FmtOp arrays codegen'd as static Rust source; single interpreter at runtime |
| Trivia | Flat side array from C tokenizer (comments + blank lines). Formatter consults by source position |
| Blank lines | Preserve up to 1 between statements |
| Comment wrapping | Deferred to v2. Comments verbatim for now |
| Macros in AST | Opaque leaf nodes. Tokenizer-level expansion. Source spans track macro regions |
| Macro definition parsing | Probe-based: try synthetic wrappers to infer body type |
| Macro arg classification | Per-argument at format time: single AST subtree → structural, spans multiple → raw text |
| Keyword casing | Configurable (preserve / UPPER / lower) |
| Error recovery | Parse error → leave entire file unformatted (like prettier) |
| Semicolons | Normalize extra semicolons |
| `.synq` DSL | No macro-level extensions needed |
| Testing | Stability (format twice → same) + AST equivalence (parse original and formatted → same AST) |
| Range formatting | Deferred |

---

## Architecture Overview

```
                         ┌──────────────┐
                         │ Source text   │
                         └──────┬───────┘
                                │
                    ┌───────────▼───────────┐
                    │  C Tokenizer          │
                    │  • Emits tokens       │
                    │  • Captures trivia    │
                    │    (comments, blanks) │
                    │  • Expands macros     │
                    │    (textual subst)    │
                    │  • Records macro      │
                    │    regions + arg      │
                    │    boundaries         │
                    └───────────┬───────────┘
                                │
                    ┌───────────▼───────────┐
                    │  Lemon Parser         │
                    │  • Knows nothing      │
                    │    about macros       │
                    │  • Clean AST          │
                    │    (expanded)         │
                    └───────────┬───────────┘
                                │
              ┌─────────────────┼─────────────────┐
              │                 │                   │
     ┌────────▼──────┐  ┌──────▼──────┐  ┌────────▼────────┐
     │ AST           │  │ Trivia[]    │  │ MacroRegion[]   │
     │ (semantic,    │  │ (offset,    │  │ (name, full     │
     │  clean,       │  │  length,    │  │  span, arg      │
     │  expanded)    │  │  kind)      │  │  boundaries)    │
     └────────┬──────┘  └──────┬──────┘  └────────┬────────┘
              │                │                   │
              └────────────────┼───────────────────┘
                               │
                    ┌──────────▼──────────┐
                    │  Formatter           │
                    │  1. Interpret FmtOp  │
                    │     bytecode → Doc   │
                    │  2. Interleave       │
                    │     trivia by pos    │
                    │  3. Detect macro     │
                    │     regions, emit    │
                    │     macro syntax     │
                    │  4. Render Doc →     │
                    │     String           │
                    └─────────────────────┘
```

---

## Part 1: Doc Algebra & Renderer

### Doc Type (arena-allocated)

```rust
type DocId = u32;
const NIL_DOC: DocId = u32::MAX;

enum Doc {
    Text { start: u32, len: u16 },          // source text slice
    StaticText { ptr: *const u8, len: u16 }, // keyword / static string
    Line,                                    // space | newline+indent
    SoftLine,                                // empty | newline+indent
    HardLine,                                // always newline+indent
    Cat { left: DocId, right: DocId },
    Nest { indent: i16, child: DocId },
    Group { child: DocId },
    IfBreak { broken: DocId, flat: DocId },
    LineSuffix { child: DocId },             // trailing comments → defer to EOL
    BreakParent,                             // force enclosing group to break
}
```

`DocArena`: `Vec<Doc>`, push-to-allocate. Per-statement lifetime.

### Lindig Renderer

Stack-based. State: `Vec<(indent, Mode, DocId)>`, `pos` (column), `line_suffix_buf`.

- `Group`: "fits?" check in Flat mode, O(remaining_width). If fits → Flat, else → Break.
- `Line`: Flat → space, Break → flush line_suffix_buf + newline + indent.
- `LineSuffix`: Buffer trailing comments until next line break.
- `HardLine`: Always break. Flush line_suffix_buf.

### Crate: `syntaqlite-fmt`

| Module | Purpose |
|--------|---------|
| `doc.rs` | Doc enum, DocArena, DocId, builder API |
| `render.rs` | Lindig rendering algorithm |
| `ops.rs` | FmtOp bytecode types |
| `interpret.rs` | FmtOp → Doc tree builder |
| `trivia.rs` | Trivia consultation by source position |
| `macro_fmt.rs` | Macro region detection + formatting |
| `config.rs` | FormatConfig |
| `lib.rs` | Public API |
| `generated/fmt_ops.rs` | Static FmtOp arrays (codegen'd) |

---

## Part 2: FmtOp Bytecode

### Instruction Set

```rust
enum FmtOp {
    Keyword(StringId),
    Span(FieldOffset),
    Line, SoftLine, HardLine,
    GroupStart, GroupEnd,
    NestStart(i16), NestEnd,
    Child(FieldOffset),
    ForEachStart, ForEachSep(StringId), ForEachEnd,
    IfSet(FieldOffset, SkipCount),
    IfFlag(FieldOffset, SkipCount),
    IfEnum(FieldOffset, u16, SkipCount),
    Else(SkipCount), EndIf,
    Clause(StringId, FieldOffset),          // peephole: line + kw + nest(line + child)
    EnumDisplay(FieldOffset, TableId),
}
```

### Codegen Pipeline

```
.synq fmt block → parsed by node_parser.rs → FmtDsl AST
    → compiled by fmt_codegen.rs → Vec<FmtOp>
    → emitted as Rust source → generated/fmt_ops.rs
```

Dispatch: `static FMT_DISPATCH: &[Option<&[FmtOp]>]` indexed by NodeTag.

String table: `static STRING_TABLE: &[&str]` for keywords/punctuation.

---

## Part 3: Trivia System

### Capture (C tokenizer)

```c
typedef struct {
    uint32_t offset;
    uint16_t length;
    uint8_t kind;  // 0=LineComment, 1=BlockComment, 2=BlankLine
} SyntaqliteTrivia;
```

Tokenizer loop: before returning each real token, capture skipped comments and blank lines (2+ consecutive newlines → single BlankLine entry) into a growable side buffer.

Exposed via FFI to Rust.

### Formatter Consultation

No attachment pass. The formatter consults the trivia array by source position during rendering:

- Before emitting a node: scan trivia array for items between previous node's end and this node's start.
- Line comment on own line → emit `HardLine + Text(comment)` before node.
- Line comment on same line as previous node → emit `LineSuffix(Text(comment))` after previous node.
- Block comment → same heuristic.
- BlankLine → emit extra `HardLine` (preserves up to 1 blank line).

The trivia array is sorted by offset → binary search for the relevant range.

---

## Part 4: Macro System

### Overview

The macro system has two sides: definitions and invocations. The AST stays clean (fully expanded), while the formatter reconstructs macro syntax using source-position metadata. This means semantic tools see expanded SQL while the formatter preserves the author's macro usage.

### Macro Definitions

```sql
CREATE MACRO foo(x, y) AS SELECT $x FROM $y WHERE 1=1
```

**Body type inference** via probe-based parsing. At definition time, treat `$params` as identifiers and try synthetic wrappers to determine what grammar production the body represents:

1. `{body}` → parses as statement?
2. `SELECT {body}` → parses as expression/expression list?
3. `SELECT 1 FROM {body}` → parses as table expression?
4. `SELECT 1 FROM t WHERE {body}` → parses as condition?
5. No match → unstructured, store as raw text.

First match wins. The formatter knows the body type and formats the definition body accordingly.

**Formatting the definition**: Once the body type is known, the formatter can parse the body (with `$params` as identifiers) using the appropriate grammar production and format it structurally:

```sql
CREATE MACRO foo(x, y) AS
    SELECT $x
    FROM $y
    WHERE 1 = 1
```

### Macro Invocations

**Tokenizer-level expansion**: When the tokenizer encounters macro call syntax, it:
1. Records the `MacroRegion { name_span, full_span, arg_boundaries: Vec<(offset, len)> }`
2. Substitutes parameters textually
3. Emits the expanded tokens (with source positions pointing into original source text)

The parser sees expanded SQL. The AST is clean and fully semantic.

**Key insight**: Because the expanded tokens carry source positions that point into the macro call's argument text in the original source, the AST nodes produced from expansion naturally have source spans that fall within the macro region. No extra annotations needed.

### Formatter Macro Handling

When rendering, the formatter checks each node's source span against recorded macro regions:

1. **Detection**: Node's source span falls within a macro region → this node came from expansion.
2. **Grouping**: Find all sibling nodes from the same macro region.
3. **Per-argument classification**:
   - Collect AST nodes whose source spans fall within one argument's source range.
   - If they form a single subtree → **structural**: format using the AST (Wadler algebra).
   - If they span multiple subtrees → **raw text**: emit original source text from that argument.
4. **Emit**: `name!(formatted_arg1, formatted_arg2, ...)` with Wadler group/nest/softline for line-breaking between arguments.

### Structured vs Unstructured (Case 2 vs Case 1)

This distinction is made **per-argument, at format time**, based on where the expanded tokens landed in the AST:

- **Structured argument** (e.g., `a + b` expanding to a single `BinaryExpr`): the AST nodes form one subtree. The formatter formats it using the normal Wadler algebra.
- **Unstructured argument** (e.g., `FROM b WHERE c` spanning `from_clause` and `where_clause`): the AST nodes span multiple unrelated subtrees. The formatter emits the original source text with best-effort line-breaking.

No definition-time analysis or author annotation needed for this classification.

---

## Part 5: Configuration

```rust
struct FormatConfig {
    line_width: usize,          // default: 80
    indent_width: usize,        // default: 4
    keyword_case: KeywordCase,  // Preserve | Upper | Lower
}

enum KeywordCase { Preserve, Upper, Lower }
```

---

## Part 6: Error Handling

**Parse error → leave entire file unformatted.** Emit original source text unchanged (like prettier). Because the formatter is not a linter, it leaves the file unchanged when parsing fails.

**Semicolons**: Normalize (strip extra semicolons, ensure each statement ends with one).

---

## Part 7: Testing Strategy

Two complementary approaches:

1. **Stability**: `format(format(input)) == format(input)`. Format twice, output must be identical. Tests that the formatter's output is a fixed point.

2. **AST equivalence**: `parse(input).ast == parse(format(input)).ast`. The formatted output must parse to the same AST as the original. Tests that formatting preserves semantics.

Both run on a corpus of SQL files covering all node types.

---

## Implementation Phases

### Phase 1: Doc + Renderer
- `syntaqlite-fmt` crate with `doc.rs`, `render.rs`, `config.rs`
- Unit tests: hand-built Doc trees → verify rendering
- Validate Lindig algorithm on SQL-shaped documents

### Phase 2: FmtOp + Interpreter
- `ops.rs`, `interpret.rs`
- Hand-write a few FmtOp arrays (before codegen)
- Integration: parse SQL → interpret static ops → render → verify

### Phase 3: Codegen
- Extend `syntaqlite-codegen`: `.synq` fmt blocks → FmtOp → `generated/fmt_ops.rs`
- End-to-end: `.synq` → codegen → format SQL

### Phase 4: Trivia
- Modify C tokenizer for trivia side-buffer
- FFI exposure
- Formatter trivia consultation by source position
- Tests with comments in various positions

### Phase 5: Macros
- Tokenizer macro recognition + expansion + region recording
- Probe-based definition body parsing
- Formatter macro region detection + argument formatting
- Tests with structured and unstructured macros

### Phase 6: CLI + Polish
- `syntaqlite fmt` subcommand
- Stability + AST equivalence test suite
- Semicolon normalization
- Keyword casing
- Edge cases (deeply nested, very long lines, pathological input)
