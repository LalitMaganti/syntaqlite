# Macro Expansion Plan

## Overview

Support full textual macro expansion in the parser: dialect-defined macros
are registered with the parser, and call sites like `foo!(a, b)` are expanded
by substituting `$param` references in the macro body with the supplied
arguments. Lemon only ever sees the expanded token stream.

The formatter preserves macro call sites verbatim via existing macro-region
tracking.

## Design Principles

- **Lemon never sees macro syntax.** Expansion happens in the tokenizer loop;
  Lemon receives fully expanded SQL tokens.
- **Blue paint, not depth limit.** A macro cannot expand itself (direct or
  indirect recursion). The parser tracks an expansion stack and errors on
  re-entrant calls to the same name.
- **Registration is explicit.** `CREATE PERFETTO MACRO` produces an AST node
  but does *not* auto-register. Callers (LSP host, CLI) register macros via
  the C/Rust API. This keeps the parser stateless w.r.t. cross-statement
  side effects.
- **Intrinsic macros.** A dialect can register macros backed by an opaque
  function pointer instead of a body template, for built-in expansions that
  require custom logic.

---

## Data Structures

### Macro Registry Entry

```c
// A single registered macro.
typedef struct SyntaqliteMacroEntry {
  const char* name;           // Owned copy of the macro name.
  uint32_t name_len;

  // --- Template macros ---
  const char* body;           // Body text with $param placeholders. Owned.
  uint32_t body_len;
  const char** param_names;   // Array of param name strings. Owned.
  uint32_t param_count;

  // --- Intrinsic macros ---
  // If non-NULL, called instead of template expansion.
  // Receives the raw arg texts and writes expanded SQL into out_buf.
  // Returns 0 on success, non-zero on error.
  int (*intrinsic)(
      void* user_data,
      const char* const* arg_texts,   // arg_count null-terminated strings
      const uint32_t* arg_lens,
      uint32_t arg_count,
      char** out_buf,                 // callee allocates via mem->malloc
      uint32_t* out_len,
      SyntaqliteMemMethods mem
  );
  void* user_data;
} SyntaqliteMacroEntry;
```

### Expansion Stack (Blue Paint)

```c
// Stack of macro names currently being expanded.
// Stored on SyntaqliteParser. Small fixed-size array (max 16 deep).
#define SYNQ_MAX_MACRO_DEPTH 16

// On SyntaqliteParser:
const char* macro_expansion_stack[SYNQ_MAX_MACRO_DEPTH];
uint32_t macro_expansion_depth;  // replaces current macro_depth
```

### Expansion Buffer Stack

Nested macro calls require a stack of expansion buffers. When macro `A`
expands to text containing `B!(...)`, the tokenizer pushes a new frame
for `B`'s expansion and resumes `A`'s buffer when `B` is drained.

```c
typedef struct SynqExpansionFrame {
  char* buf;              // Expanded SQL text (owned).
  uint32_t buf_len;
  uint32_t buf_cap;
  uint32_t offset;        // Current read position in this frame's buffer.
  uint32_t source_resume; // p->offset to restore when this frame drains
                          // (only meaningful for the outermost frame).
} SynqExpansionFrame;

// On SyntaqliteParser:
SynqExpansionFrame expansion_stack_bufs[SYNQ_MAX_MACRO_DEPTH];
uint32_t expansion_stack_depth;  // 0 = reading from source, >0 = reading
                                 // from expansion_stack_bufs[depth-1].
```

The tokenizer reads from the top frame (`expansion_stack_bufs[depth-1]`).
When that frame is drained, it pops and resumes the previous frame (or the
original source if depth reaches 0).

### Macro Registry (Hashmap)

Simple open-addressing hashmap on `SyntaqliteParser`:

```c
SyntaqliteMacroEntry* macro_table;   // Power-of-2 sized array.
uint32_t macro_table_size;           // Capacity (power of 2).
uint32_t macro_table_count;          // Number of live entries.
```

Key = macro name, hashed with FNV-1a or similar. Tombstone deletion for
deregister support.

---

## C API

```c
// Register a template macro. Copies all strings.
// Returns 0 on success, non-zero if name is already registered.
int syntaqlite_parser_register_macro(
    SyntaqliteParser* p,
    const char* name,
    uint32_t name_len,
    const char* const* param_names,
    uint32_t param_count,
    const char* body,
    uint32_t body_len
);

// Register an intrinsic macro backed by a function pointer.
int syntaqlite_parser_register_intrinsic_macro(
    SyntaqliteParser* p,
    const char* name,
    uint32_t name_len,
    int (*intrinsic)(void*, const char* const*, const uint32_t*,
                     uint32_t, char**, uint32_t*, SyntaqliteMemMethods),
    void* user_data
);

// Deregister a macro by name. Returns 0 on success, non-zero if not found.
int syntaqlite_parser_deregister_macro(
    SyntaqliteParser* p,
    const char* name,
    uint32_t name_len
);
```

### Rust API (on `TypedParser` / `AnyParser`)

```rust
impl<G: TypedGrammar> TypedParser<G> {
    pub fn register_macro(
        &mut self,
        name: &str,
        params: &[&str],
        body: &str,
    ) -> Result<(), MacroRegistrationError>;

    pub fn register_intrinsic_macro(
        &mut self,
        name: &str,
        callback: Box<dyn FnMut(&[&str]) -> Result<String, String>>,
    ) -> Result<(), MacroRegistrationError>;

    pub fn deregister_macro(&mut self, name: &str) -> bool;
}
```

---

## Arg Parsing

Rework `scan_rust_macro_call` → `scan_macro_args`. Instead of just returning
the end offset, it returns an array of `(offset, length)` pairs for each
comma-separated argument (respecting nested parens). The arg text is raw
source: the tokenizer handles turning it into tokens during expansion.

```c
typedef struct SynqMacroArg {
  uint32_t offset;  // Byte offset in source.
  uint32_t length;  // Byte length of the argument text.
} SynqMacroArg;

// Scan balanced parens after '!' and split into comma-separated args.
// Returns arg count on success, 0 if not a valid macro call.
// Args are written into caller-provided buffer.
static uint32_t scan_macro_args(
    SyntaqliteParser* p,
    uint32_t bang_offset,
    SynqMacroArg* out_args,
    uint32_t max_args,
    uint32_t* out_end_offset    // set to byte past closing ')'
);
```

---

## Expansion Logic

### Template Expansion

Given macro entry `{params: ["x", "y"], body: "SELECT $x + $y FROM t"}` and
call args `["42", "'hello'"]`:

1. Walk the body text.
2. When `$` is encountered, read the following identifier.
3. Look up the identifier in the param list.
4. If found, append the corresponding arg text to the expansion buffer.
5. If not found, emit error (unknown parameter `$name`).
6. Otherwise, copy body characters to the expansion buffer verbatim.

The result is a fully expanded SQL string that the tokenizer loop will
process.

### Intrinsic Expansion

Call the function pointer with the arg texts. It returns an expanded SQL
string that is copied into the expansion buffer.

---

## Tokenizer Loop Changes

The main loop in `syntaqlite_parser_next` gets a multi-source tokenizer:

```c
// Pseudocode for the "next token" logic:
if (expansion_stack_depth > 0) {
    SynqExpansionFrame* top = &expansion_stack_bufs[expansion_stack_depth - 1];
    if (top->offset < top->buf_len) {
        // Read from top expansion frame.
        tokenize from top->buf + top->offset
    } else {
        // Frame drained: pop and resume previous frame or source.
        expansion_stack_depth--;
        pop blue-paint stack, end_macro()
        if (expansion_stack_depth == 0) {
            advance p->offset past original call site
        }
        continue;  // re-enter loop
    }
} else {
    // Read from original source.
    tokenize from source + offset
}
```

When the loop detects `ID + TK_ILLEGAL` and `try_macro_call` finds a
registered macro:

1. Parse args via `scan_macro_args`.
2. Check blue-paint stack for self-recursion → error if found.
3. Push macro name onto blue-paint expansion stack.
4. Push a new `SynqExpansionFrame` onto the expansion buffer stack.
5. Expand body (template or intrinsic) into the new frame's buffer.
6. `begin_macro(call_offset, call_length)` for formatter region tracking.
7. The loop naturally starts reading from the new top frame on the next
   iteration.

Nested macros work naturally: when the tokenizer encounters `ID + !`
while reading from an expansion frame, it pushes another frame on top.

---

## Error Diagnostics

Parse errors during macro expansion produce stacked "caused by" tracebacks.
Each frame on the expansion stack contributes a traceback entry showing the
call site location. The innermost frame shows the fully expanded statement.

Example error output:

```
Fully expanded statement
  SELECT * FROM slice
  ^
Traceback (most recent call last):
  File "stdin" line 1 col 1
    macro!()
    ^
  Trace Processor Internal line 1 col 1
    nested!()
    ^
  Trace Processor Internal line 1 col 1
    SELECT * FROM slice
    ^
```

Each expansion frame records the source location (file, line, column) of
the call site so the traceback can be reconstructed. The expansion stack
already has the macro names; we additionally store the call-site offset
and source identifier (e.g. "stdin", "Trace Processor Internal") per frame.

```c
typedef struct SynqExpansionFrame {
  // ... existing fields ...
  uint32_t call_site_offset;   // Byte offset of the macro call in the
                               // parent frame (or original source).
  const char* source_name;     // E.g. "stdin", "Trace Processor Internal".
  uint32_t source_name_len;
} SynqExpansionFrame;
```

---

## Perfetto Dialect: `CREATE PERFETTO MACRO`

### Grammar (`dialects/perfetto/actions/perfetto.y`)

The existing rule already parses the statement:

```lemon
cmd ::= CREATE perfetto_or_replace PERFETTO MACRO nm
        LP perfetto_macro_arg_list RP RETURNS ID AS perfetto_macro_body.
```

This produces a `CreatePerfettoMacroStmt` AST node. **No changes needed**
to the grammar rule itself.

### AST Node (`dialects/perfetto/nodes/perfetto.synq`)

The existing `CreatePerfettoMacroStmt` node captures `macro_name`,
`or_replace`, `return_type`, and `args`. We may want to add a `body` field
(source span) so that callers (LSP, CLI) can extract the body text for
registration. Currently the body is consumed by `%wildcard ANY` and not
surfaced in the AST.

### Body Span

The `perfetto_macro_body` rule accumulates `ANY` tokens. The body span is
derived from the first and last tokens in the list: no special span
tracking needed. The host extracts `source[first_token.offset ..
last_token.offset + last_token.length]` to get the raw body text.

### Registration Flow

The parser does **not** auto-register macros on seeing `CREATE PERFETTO MACRO`.
Instead:

1. Parser produces `CreatePerfettoMacroStmt` AST node.
2. The host (LSP, CLI, embedder) walks the AST.
3. Extracts `macro_name`, `param_names`, and body text from the body span.
4. Calls `parser.register_macro(name, params, body)`.

This keeps the parser side-effect-free across statements.

---

## Implementation Order

### Phase 1: Core Infrastructure
Files: `parser.c`, `parser.h`, `incremental.h`

1. Add macro registry (hashmap) to `SyntaqliteParser`.
2. Add expansion buffer stack and blue-paint stack to `SyntaqliteParser`.
3. Implement `register_macro` / `deregister_macro` / `register_intrinsic_macro`
   C API.
4. Implement `scan_macro_args` (split balanced-paren content into args).
5. Implement `expand_template_macro` (textual `$param` substitution).
6. Wire up the tokenizer loop: multi-source reading (expansion buffer stack
   vs source), blue-paint stack checks, begin/end macro region tracking.
7. Implement stacked error diagnostics with "caused by" traceback (call-site
   offset and source name per expansion frame).

### Phase 2: Rust Bindings
Files: `syntaqlite-syntax/src/parser/ffi.rs`, `syntaqlite-syntax/src/parser/mod.rs`

7. Add FFI bindings for register/deregister/register_intrinsic.
8. Expose on `TypedParser` / `AnyParser` with safe Rust API.
9. Add `macro_regions()` tests that verify expanded content.

### Phase 3: Perfetto Dialect Integration
Files: `dialects/perfetto/nodes/perfetto.synq`, `dialects/perfetto/actions/perfetto.y`

10. Add `body` source-span field to `CreatePerfettoMacroStmt` (requires
    capturing the span of the `perfetto_macro_body` rule).
11. Write Perfetto-specific tests: define macro, register it, call it,
    verify expanded AST + formatter output.

### Phase 4: CLI & Formatter
Files: `syntaqlite-cli/src/codegen.rs`, `syntaqlite/src/fmt/formatter.rs`

12. Verify formatter `try_macro_verbatim` works with expanded macros
    (it should: the macro region still covers the original call site).
13. Add diff tests for macro call formatting in Perfetto.

---

## Testing Strategy

- **Unit tests (C level):** register macro, call it, verify Lemon receives
  expanded tokens. Test self-recursion error. Test arg count mismatch error.
  Test intrinsic macro.
- **Rust integration tests:** parse `SELECT foo!(42)` with `foo` registered
  as `SELECT $x`, verify AST is `SELECT (SELECT 42)`.
- **Perfetto diff tests:** `CREATE PERFETTO MACRO` followed by call site,
  verify formatted output preserves `foo!(args)` verbatim.
- **Error cases:** unknown macro name (fed as bare ID, existing behavior),
  wrong arg count, self-recursive call, malformed `$param`.

---

## Resolved Questions

1. **Unknown macro calls**: error. If `foo!(args)` is encountered and
   `foo` is not registered, emit a parse error.

2. **Body span in AST**: no special tracking needed. The
   `perfetto_macro_body` rule accumulates `ANY` tokens; the body span is
   derived from the first/last token offsets in the list.

3. **Macro body storage**: raw source text. Re-tokenized on each expansion.
   Simpler and sufficient for expected call frequency.

4. **Nested expansion**: uses a stack of expansion buffers (one per active
   macro frame), not a single flat buffer. The tokenizer always reads from
   the top frame; when drained, it pops and resumes the previous frame.

5. **`!` tokenization**: `!` is tokenized as `TK_ILLEGAL` which is already
   handled; `!=` is a separate two-char token so there is no ambiguity.

6. **Arg parsing scope**: `scan_macro_args` only needs to track balanced
   parens and commas. The arg text is raw source; the tokenizer handles
   turning it into tokens during expansion.

7. **Memory ownership**: expansion buffers, registry entries, and intrinsic
   output buffers are all owned by the parser and freed when the parser
   itself is freed.
