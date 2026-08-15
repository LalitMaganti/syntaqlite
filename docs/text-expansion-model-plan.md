# Text / Expanded-Text Model Plan

## Context

Today the parser exposes a multi-buffer model for macro expansion: spans
carry a `_buf_idx` that identifies which expansion buffer their offset lives
in, and consumers call `resolve_span` to walk the parent chain up to source.
This model has leaked across the public API in inconsistent ways:

- `FieldValue::Span::text` refers to the *expansion-buffer* slice, not the
  authored text: backwards from how most consumers think about "text".
- `analyzer.rs::statement_source()` slices source using raw token offsets,
  which is silently broken for any statement containing a macro call (token
  offsets for expansion-buffer tokens aren't source positions).
- Issue #89 asks for the authored text of a specific statement subtree
  (e.g. the `select` body of `CREATE PERFETTO TABLE … AS <select>`), and
  there's no principled way to reach it today.
- `expansion_traceback` is powerful but low-fidelity: it doesn't track
  argument provenance, so an error at byte N inside a substituted argument
  collapses to the whole `foo!(…)` call site rather than pointing at the
  user's authored arg expression.

The goal of this plan is to replace the ad-hoc multi-buffer surface with a
coherent vocabulary and API, and restore Perfetto-style traceback fidelity
(argument-level). Issue #89 is addressed via targeted per-node annotations
rather than a generalized subtree-text API: see the Step 8 reshape in
the session log.

## Core vocabulary

Exactly two words, used consistently at every scope:

- **`text`** = what the user authored. Always a slice of the input you
  handed to `parser_reset`.
- **`expanded_text`** = what the parser's tokenizer actually saw, after
  all macro expansion. For statements with no macros, `expanded_text`
  equals `text` byte-for-byte.

These two words are the only names used in the public API. `source` is
dropped from user-facing names (it remains fine as an internal
implementation term where appropriate).

### Duality, stated crisply

|                     | Whole statement        | Span                        |
| ------------------- | ---------------------- | --------------------------- |
| **Authored**        | `text()`               | `span_text(span)`           |
| **Expanded**        | `expanded_text()`      | `span_expanded_text(span)`  |
| **Range in text()** |:                      | `span_text_range(span)`     |

Invariants:

- `span_text(span)` is *always* a slice of `text()`.
- `span_expanded_text(span)` is *content-equivalent* to the corresponding
  slice of `expanded_text()`, but may live in a different backing buffer
  (a private expansion layer). Users should treat it as an owned `&str`,
  not as "a pointer into some known buffer".
- For macro-free statements, all four collapse: `text() == expanded_text()`
  and `span_text == span_expanded_text` for every element.

**Subtree-level text is not a generalized API.** Specific grammar rules
that need to forward authored SQL (e.g. the `select` body of
`CREATE PERFETTO TABLE/VIEW/FUNCTION`) get a targeted span field (like
`select_span`) populated explicitly by the grammar action. Consumers
read that field like any other span. See the Step 8 reshape for
details: there is no generalized `subtree_text(node)` accessor.

## Architectural decision: lazy layer tree, no materialization

After considering two alternatives: (a) eager materialization into a single
flat `expanded_text` string at end of parse, and (b) a Perfetto-style
preprocessor pass producing a pre-built tree of `SqlSource` nodes: we
landed on a **lazy** model:

- During parse, the parser maintains an in-memory tree of "expansion
  layers" (the same multi-buffer structure we have today, with richer
  per-layer metadata).
- `parser_next` returns immediately after Lemon finishes: there is no
  post-parse materialization pass.
- Queries (`text`, `expanded_text`, `traceback`) are computed on demand
  from the layer tree. The traceback walk writes into a small
  parser-owned scratch buffer (`traceback_buf`) that is cleared and
  rewritten on each call.

### Why lazy?

- **Zero baseline cost.** Parsers that don't ask about expanded text: the
  common case for formatters on macro-free SQL, validators, autocompletion
 : pay nothing for the machinery. No allocation, no walk.
- **No temporary state to reason about.** The parse result is the final
  state; there's no "materialized view" transition point that could go
  stale or be forgotten.
- **Minimally disruptive.** The existing `expand_and_feed` / tokenizer-swap
  / recursive expansion pipeline stays intact. We add fields to the layer
  struct, add an arg-segment recording step inside the existing parameter
  substitution code, and layer new accessors on top.

### Why *not* Perfetto preprocessor style?

Perfetto's preprocessor is a separate pass that consumes tokens and produces
a `SqlSource` tree via a frame stack of mutable `Rewriter` builders. The
complexity is real: non-movable frames in a linked list, a custom Lemon
grammar for recognizing macro syntax, `Rewriter::Build`'s 4-phase
recomputation, `Node::Substr` as a tree-rewriting op, macro bodies stored as
pre-built `SqlSource` at registration time. All of that exists because the
preprocessor has no parser context and must construct a standalone
`SqlSource` eagerly.

We expand macros lazily during parse (the tokenizer hits a call and expands
inline), so we already have the parser context Perfetto's preprocessor was
rebuilding from scratch. We can borrow Perfetto's **conceptual model**
(authored layer → rewrite tree → full fidelity traceback with arguments
traced back to caller positions) without importing any of its machinery.

### Why *not* eager materialization?

An eager "build full expanded_text at end of parse" approach works, but
it unconditionally allocates O(statement size) on every parse, even for
the vast majority of parses that never query expanded text. Lazy gives
us the same end-state queryability without the baseline cost.

The eager design also committed us to a single flat offset space, which
would have made every span carry a rewritten-sql offset rather than a
layer-local one. That is cleaner conceptually but required us to rewrite
every AST span during materialization: more code churn for no functional
benefit once the public API is behind accessors.

## Encapsulation: layers are internal

A strict rule:

> Layers are never exposed in the public API. `_layer_id` lives on spans
> and tokens as an underscore-prefixed implementation detail field.
> Consumers interact with spans only through parser accessors. There is
> no `Layer` type, no `layers()` accessor, no `Frame` field pointing at
> a layer.

Everything user-visible goes through accessors that take opaque `Span`s and
`NodeId`s and return `&str` slices or self-contained value types. A user who
holds a `Span` can pass it to `span_text`, `span_expanded_text`,
`span_text_range`, or `traceback` and nothing else. No peeking.

## Data model

### `SynqExpansionLayer` (internal)

One per macro invocation, plus one for the root statement. Replaces the
current `SynqMacroRegion` / `macro_expansions` split with a single unified
structure.

```c
typedef struct SynqExpansionLayer {
    // Tree structure
    uint32_t parent_layer_id;           // UINT32_MAX for the root layer
    uint32_t call_offset_in_parent;     // position of foo!(...) in parent's buf
    uint32_t call_length_in_parent;     // length of foo!(...) in parent's buf

    // The buffer this layer's tokens were tokenized from.
    // - Root: pointer into user source, not owned.
    // - Macro call: template body with $params substituted + any nested
    //   macros further expanded inline, owned via p->mem.
    const char* buf;
    uint32_t buf_len;
    uint8_t owns_buf;                   // 0 for root, 1 for expansions

    // Parameter substitutions: one per $param that was substituted with
    // caller argument text. Sorted by sub_offset, non-overlapping.
    // Empty for the root layer.
    SynqArgSegment* arg_segments;
    uint32_t arg_segment_count;

    // Macro template text and definition provenance for traceback
    // rendering. All NULL / zero for the root layer.
    const char* template_body;          // borrowed from macro registry
    uint32_t template_body_len;
    const char* name;                   // e.g. "Macro 'foo'" (owned)
    uint32_t def_line;                  // macro definition line
    uint32_t def_col;
} SynqExpansionLayer;
```

### `SynqArgSegment`

Records one parameter substitution within an expansion layer. The key piece
of new metadata that enables argument-level traceback fidelity.

```c
typedef struct SynqArgSegment {
    // Where the substituted arg text lives in this layer's buf.
    uint32_t sub_offset;
    uint32_t sub_length;

    // Where the arg came from (in its origin layer's buf).
    uint32_t origin_layer_id;
    uint32_t origin_offset;
    uint32_t origin_length;
} SynqArgSegment;
```

When `synq_parser_expand_macro` substitutes `$1` with the bytes of the
first argument, it copies arg text from the caller's buffer into the new
layer's buf. At the moment of copy, all the information for a
`SynqArgSegment` is available: record it.

### Parser state

```c
struct SyntaqliteParser {
    // ... existing lifecycle / tokenizer / grammar state ...

    // The layer tree. layers.data[0] is the root, always present.
    SYNQ_VEC(SynqExpansionLayer) layers;

    // Scratch buffer owned by the parser for `syntaqlite_parser_traceback`.
    // Cleared and rewritten on each call; the returned pointer is
    // invalidated by the next traceback() call or by reset_stmt.
    SYNQ_VEC(SyntaqliteTracebackFrame) traceback_buf;
};
```

### Public types

```c
// Public struct; _layer_id is an implementation-detail field.
typedef struct SyntaqliteTextSpan {
    uint32_t offset;
    uint16_t length;
    uint8_t flags;
    uint8_t _layer_id;   // Internal. Do not access.
} SyntaqliteTextSpan;

// Byte range in text() (the user's authored input).
typedef struct SyntaqliteTextRange {
    uint32_t start;
    uint32_t end;
} SyntaqliteTextRange;

// Self-contained traceback frame. Borrows name/snippet from parser-
// owned memory (macro registry for `name`, layer buffers for `snippet`).
typedef struct SyntaqliteTracebackFrame {
    const char* name;             // NULL for root, macro name otherwise
    uint32_t name_len;
    uint32_t line;
    uint32_t col;
    const char* snippet;          // the buffer to render the caret against
    uint32_t snippet_len;
    uint32_t offset_in_snippet;
    uint32_t length_in_snippet;
} SyntaqliteTracebackFrame;
```

### Token layer_id

Tokens need to know which layer they came from so `span_expanded_text`
equivalents can resolve their buffer. Two options:

**(a) Add `uint8_t layer_id` (+ padding) to `SyntaqliteParserToken`.**
Same discipline as spans: public struct, underscore-prefixed field,
treated as internal. Grows tokens from 16 bytes to 20.

**(b) Side vector `token_layer_ids` indexed parallel to `p->tokens`.**
Public struct stays unchanged. Accessors need to go through the parser to
map index → layer_id.

**Decision: (a).** Consistent with how spans handle layer_id, no
parallel-index correctness risk, the 4-byte growth per token is
negligible. `SyntaqliteParserToken` becomes:

```c
typedef struct SyntaqliteParserToken {
    uint32_t offset;
    uint32_t length;
    uint32_t type;
    uint32_t flags;
    uint8_t _layer_id;
    uint8_t _pad[3];
} SyntaqliteParserToken;
```

## Public API

### C side

```c
// ── Whole-statement text ─────────────────────────────────────────────────

// The user's authored input. Same as syntaqlite_parser_text() today,
// renamed for vocabulary consistency. (Deferred: see Step 15.)
SYNTAQLITE_API const char*
syntaqlite_parser_text(SyntaqliteParser* p, uint32_t* out_len);

// ── Span accessors ───────────────────────────────────────────────────────

// Authored slice of text() corresponding to this span. For spans inside
// a macro expansion, walks the expansion chain: drills through
// substituted arg segments, otherwise collapses to the outermost
// `name!(...)` call site in text(). Always a direct slice of text():
// no allocation.
SYNTAQLITE_API const char*
syntaqlite_parser_span_text(SyntaqliteParser* p,
                            const SyntaqliteTextSpan* span,
                            uint32_t* out_len);

// What the parser's tokenizer saw for this span. For spans outside a
// macro, equals span_text. For spans inside a macro, a slice of the
// expansion layer's buffer. Always a direct slice: no allocation.
SYNTAQLITE_API const char*
syntaqlite_parser_span_expanded_text(SyntaqliteParser* p,
                                     const SyntaqliteTextSpan* span,
                                     uint32_t* out_len);

// Byte range of span_text in text(). Always a valid range in the user's
// authored input.
SYNTAQLITE_API SyntaqliteTextRange
syntaqlite_parser_span_text_range(SyntaqliteParser* p,
                                  const SyntaqliteTextSpan* span);

// ── Traceback ────────────────────────────────────────────────────────────

// Build a traceback for a span and return a pointer to a parser-owned
// frame array. Frames are ordered outermost (the root source frame) to
// innermost (the position inside the deepest macro expansion layer).
//
// The returned pointer is backed by `p->traceback_buf`, which is cleared
// and rewritten on every call. It remains valid until the next call to
// syntaqlite_parser_traceback on the same parser or until the next
// syntaqlite_parser_next resets the current statement: callers that
// need to retain frames must copy them out.
//
// Writes the frame count to `*out_count`. Returns NULL (and 0) for
// empty or invalid spans.
SYNTAQLITE_API const SyntaqliteTracebackFrame*
syntaqlite_parser_traceback(SyntaqliteParser* p,
                            const SyntaqliteTextSpan* span,
                            uint32_t* out_count);
```

### Rust side

Mirror of the C API, returning `&'a str` with the parser's per-statement
lifetime.

```rust
impl<'a> AnyParsedStatement<'a> {
    // Whole (text() to land with Step 15).
    pub fn text(&self) -> &'a str;

    // Span
    pub fn span_text(&self, span: Span) -> &'a str;
    pub fn span_expanded_text(&self, span: Span) -> &'a str;
    pub fn span_text_range(&self, span: Span) -> SourceRange;

    // Traceback: `&mut self` because the iterator borrows from the
    // parser's internal `traceback_buf` scratch vec, which the next
    // traceback() call overwrites. The borrow checker enforces "one
    // live traceback iterator at a time"; callers who need to retain
    // frames across another call must `.collect::<Vec<_>>()`.
    pub fn traceback(
        &mut self,
        node_id: AnyNodeId,
        field_idx: u8,
    ) -> impl Iterator<Item = TracebackFrame<'a>> + use<'_, 'a>;
}
```

**Subtree-level text is not part of this API.** Consumers that need
the authored text of a specific grammar rule (e.g. the `select` body
of `CREATE PERFETTO TABLE`) read a dedicated span field placed on the
parent AST node by the grammar action. See the Step 8 reshape.

### `FieldValue::Span`: final shape

```rust
#[derive(Clone, Copy, Debug)]
pub enum FieldValue<'a> {
    NodeId(AnyNodeId),
    Span {
        /// Authored text: slice of `text()`.
        text: &'a str,
        /// What the parser saw: slice of whichever layer buf
        /// this field's span came from.
        expanded_text: &'a str,
        /// Byte range of `text` in `text()`.
        text_range: TextRange,
        /// Whether the identifier was quoted in source.
        quoted: bool,
    },
    Bool(bool),
    Flags(u8),
    Enum(u32),
}
```

Both `text` and `expanded_text` are populated in `extract_fields` by
calling the parser's span accessors. No extra per-field storage.

## Internal mechanics

### `span_text(span)`: O(depth)

Walk the span's layer parent chain:

1. Start at `layer = layers[span._layer_id]`, `off = span.offset`,
   `len = span.length`.
2. While `layer.parent_layer_id != UINT32_MAX`:
   a. Check if `[off, off+len)` falls inside any of `layer`'s parent's
      arg segments (looking up from the child side: is this arg a copy
      that came from our layer's buf into our parent's?). If the span
      was tokenized inside an arg region of an ancestor layer, jump to
      the arg's origin layer at the corresponding position.
   b. Otherwise, collapse to the call site: `off = layer.call_offset_in_parent`,
      `len = layer.call_length_in_parent`, `layer = layers[layer.parent_layer_id]`.
3. At layer 0, `off` and `len` are a range in user source. Return
   `source[off..off+len]`.

Bounded by `SYNQ_MAX_MACRO_DEPTH` (16). Typical case is 1-2 iterations.

### `span_expanded_text(span)`: O(1)

`&layers[span._layer_id].buf[span.offset..span.offset + span.length]`.
Trivial.

### `span_text_range(span)`: O(depth)

Same walk as `span_text`, but return the final `(off, off+len)` pair
instead of slicing.

### `traceback(span)`: O(depth)

Start at `span._layer_id` and walk toward root, emitting one frame per
layer into a small on-stack buffer (innermost first). Drill through
arg segments when the span's position falls inside one, skipping that
layer's frame entirely. Then reverse into the parser's owned
`traceback_buf` vec so frame[0] is outermost.

```
fn traceback(span):
  layer_id = span._layer_id
  off = span.offset
  len = span.length
  tmp = []
  loop:
    layer = layers[layer_id]

    # Is our current position inside an arg segment of this layer?
    # If so, the "real" provenance at this level is the arg's origin
    # layer, not the expansion layer itself. Redirect without
    # emitting a frame.
    if arg_seg = find_arg_segment(layer, off):
      off = arg_seg.origin_offset + (off - arg_seg.sub_offset)
      layer_id = arg_seg.origin_layer_id
      continue  # retry at the origin layer

    # Emit a frame for this layer.
    tmp.push(Frame {
      name: layer.name,                  # NULL for root, macro name otherwise
      snippet: layer.expansion_data,     # source buf for root, expansion buf for macros
      offset_in_snippet: off,
      length_in_snippet: len,
      line/col: computed from (snippet, off),
    })

    if layer_id == 0:
      break  # reached root sentinel

    # Walk up to parent at this layer's call site. The call site spans
    # the whole `name!(...)` call.
    off = layer.call_offset
    len = layer.call_length
    layer_id = layer.parent_layer_id

  # Reverse into parser-owned buffer so frame[0] is outermost.
  traceback_buf.clear()
  traceback_buf.extend(tmp.iter().rev())
  return &traceback_buf[..]
```

The frame for a macro expansion renders against the layer's
`expansion_data` (the already-substituted buffer), not the raw template
body. Rendering against the substituted buffer is what the validator's
"in macro expansion" note needs.

The arg-segment drill is the fidelity-adding mechanism: when a span was
tokenized inside a substituted arg, its provenance chain follows the arg
back to where the user typed it, not to the macro body's `$param`
reference. This is what makes the common "unknown column inside a
substituted arg" diagnostic land directly on the user's code.

## What disappears

From today's codebase:

- `SyntaqliteTextSpan._buf_idx` field name (renamed to `_layer_id`).
- `SynqMacroRegion` type (unified into `SynqExpansionLayer`).
- `macro_regions` and `macro_expansions` parser vectors (unified into
  `layers`).
- `synq_span_needs_resolve` inline helper (no longer meaningful: all
  accessors handle the layer walk internally).
- `resolve_span`'s expansion-walking code path (simplified to a
  straight call into `span_text` / `span_expanded_text` /
  `span_text_range`; may be kept as a thin compat shim or removed
  entirely).
- `syntaqlite_parser_expansion_traceback` (replaced by the new
  `syntaqlite_parser_traceback` with argument fidelity).
- `analyzer.rs::statement_source()` helper (replaced by
  `stmt.text()`, which is now correct for macros instead of silently
  broken; deferred to Step 11/15).
- `FieldValue::Span::text` referring to the expansion-buffer slice (the
  name is reassigned to mean authored slice, with a new `expanded_text`
  field for the expansion-buffer slice).

## Stacked implementation plan

Each step compiles and passes tests green. Steps are ordered so that
additive/rename work lands first, then behavior changes, then deletions.

### Progress

| Step | Status | Notes |
| ---- | ------ | ----- |
| 1. Rename `_buf_idx` → `_layer_id`                  | ✅ done | |
| 2. Rename `SynqMacroRegion` → `SynqExpansionLayer`  | ✅ done | `syntaqlite_result_macros` replaced by count + indexed getter (`_macro_count` / `_macro_at`) so `p->layers` stays the single source of truth (no parallel view). |
| 3. Add new layer metadata fields                    | ✅ done | `syntaqlite_parser_register_macro` gained `def_line`/`def_col` params; Rust wrappers currently pass `0, 0`. |
| 4. Record `SynqArgSegment` during param substitution| ✅ done | `SynqMacroExpansion` carries arg segments until transferred onto the layer in `feed_macro_expansion`; `reset_stmt` / `destroy` free both expansion buffers and segment arrays. |
| 5. Add `_layer_id` to `SyntaqliteParserToken`       | ✅ done | ABI change (16 → 20 bytes). Python bindings unaffected (they don't use the struct); Rust `CParserToken` mirror updated. |
| 6. Add `span_text` / `span_expanded_text` / `span_text_range` | ✅ done | See "Notes on Step 6" below. |
| 7. Add `traceback` with arg-segment drilling        | ✅ done | New `SyntaqliteTracebackFrame` struct + `syntaqlite_parser_traceback` C API, `TracebackFrame<'a>` + `AnyParsedStatement::traceback` Rust method. Validator migrated off the old API. The C side owns a `SYNQ_VEC(SyntaqliteTracebackFrame) traceback_buf` scratch buffer (rewritten per call, freed on destroy); the function returns `const SyntaqliteTracebackFrame*` + `*out_count` in a single call. The Rust method takes `&mut self` and returns `impl Iterator<Item = TracebackFrame<'a>>` zero-copy borrowed from that buffer: `&mut self` makes "one live iterator at a time" a compile-time invariant. See session notes. |
| 8. Targeted `select_span` on Perfetto CREATE stmts (was: subtree extent cache) | ✅ done (reshaped) | **Scope change**: the generalized `subtree_text` API was dropped in favor of a targeted grammar-annotation mechanism. Added `cur_shift_start`/`last_shifted_end` tracking on `SynqParseCtx` (updated in `record_and_feed` before/after `feed_one_token`) plus empty-marker rules `select_body_start` / `select_body_end` in `perfetto.y`. Added `select_span` field to `CreatePerfettoTableStmt` / `CreatePerfettoViewStmt` / `CreatePerfettoFunctionStmt`. Consumers read the field like any other span and call `span_text_range(select_span)` for the authored byte range. Perfetto macro body already lands as a span via existing `synq_span` conversion: no change needed. |
| 9. Lazy expanded splicer + `subtree_expanded_text` / `expanded_text` | ❌ dropped | Obviated by Step 8's reshape. Macro-containing subtrees don't need a generalized splicer; Perfetto's actual use cases are handled by the `select_span` field (whose authored bytes come from the root layer directly). |
| 10. Migrate formatter's `try_macro_verbatim`        | ❌ dropped | Obviated by Step 8's reshape. The formatter's existing `MacroRegion` + peek-next-token approach is correct and doesn't benefit from a subtree accessor. |
| 11. Migrate analyzer's `statement_source`           | ⏳ deferred | |
| 12. `FieldValue::Span` rename                       | ⏳ partial | `extract_field_value` now populates `FieldValue::Span` from the new trio (Session 1 follow-up, PR #90 review). Full rename of the variant's field names (`source` → `text_range`, adding a second authored-text field) still deferred. |
| 13. Delete old expansion traceback API              | ✅ done | Removed `syntaqlite_parser_expansion_traceback`, `SyntaqliteExpansionFrame`, `ExpansionFrame`, `field_expansion_traceback`. Validator migrated to `traceback()`. |
| 14. Delete `resolve_span` and `SyntaqliteResolvedSpan` | ✅ done | Brought forward from Session 1 follow-up (PR #90 review). Zero callers remain. |
| 15. Rename `syntaqlite_parser_text()` → `syntaqlite_parser_text()` | ⏳ deferred | |

**Notes on Step 6 (session 1):**
- The existing Rust `AnyParsedStatement::span_text` method (which returned
  the expansion-buffer slice) was renamed to `span_expanded_text` as part
  of this step: otherwise the new `span_text` (authored slice) would
  silently change the semantics of every `self.stmt_result.span_text(...)`
  call site in the generated `sqlite/ast.rs`. The codegen
  (`syntaqlite-buildtools/src/dialect_codegen/rust_ast.rs`) was updated to
  emit `span_expanded_text(...)` and generated files were regenerated.
  Generated call sites preserve their original behavior after the rename.
- The three new Rust methods started as `pub(crate)` for Step 6 since the
  only caller was tests; after the Session 1 follow-up that rewired
  `extract_field_value` on top of them, they are now genuinely used on
  every span field extraction. Promoting them to `pub` happens in the
  full Step 12 rework (still deferred).

**Red-green discipline:** Steps 1–5 are mechanical renames and internal
plumbing with no user-visible surface, so there is nothing to fail a
test against: they proceed as straight refactors, verified by existing
tests staying green. Red-green begins at Step 6, where the first new
public accessors are introduced: the unit tests for `span_text`,
`span_expanded_text`, and `span_text_range` are written before the
implementation and must fail on the pre-change tree.

**Session log:**
- **Session 1** landed Steps 1–6. New unit tests in
  `syntaqlite-syntax/src/parser/session.rs`:
  `span_text_macro_free_equals_authored_slice`,
  `span_text_inside_macro_body_collapses_to_call_site`,
  `span_text_inside_substituted_arg_drills_to_origin`. Full workspace
  tests, integration suites (`ast`, `fmt`, `grammar`), and clippy
  `-D warnings` all green at session end.
- **Session 1 follow-up (PR #90 review)** brought forward the dedupe
  work: removed `syntaqlite_parser_resolve_span` /
  `SyntaqliteResolvedSpan` (Step 14) and migrated `extract_field_value`
  to populate `FieldValue::Span` directly from `span_expanded_text` +
  `span_text_range` + `sp.is_quoted()`: a partial Step 12 landing.
  Also replaced `syntaqlite_result_macros` (array-pointer view over
  `p->layers`) with a count + `_macro_at(idx)` pair so there is no
  separate "public view" data structure. Updated C examples
  (`select_columns.c` / `.cc`) accordingly.
- **Session 2** landed Steps 7, 8 (reshaped), and 13, plus dropped
  Steps 9/10 entirely. Step 7 added the new `traceback` API with
  arg-segment drilling (`SyntaqliteTracebackFrame` C struct, `CTracebackFrame`
  FFI mirror, `TracebackFrame<'a>` public Rust type, and a
  `compute_line_col` helper). The validator's `emit()` method now
  uses `stmt.traceback(node_id, field_idx)`. Step 13 followed
  immediately: the old `expansion_traceback` / `ExpansionFrame` /
  `field_expansion_traceback` symbols are gone entirely. The Perfetto
  `MacroExpansionSpanRegression` diff test split into two cases: one
  for the arg-drill collapse (`_d!(a)` → single root frame, no "in
  macro expansion" note) and one for the macro-body case (`m!()`
  where the body contains a literal unknown ident, yielding two
  frames and the multi-frame render path). New `traceback_*` tests in
  `session.rs`. **Step 8 reshape**: during prototyping the plan's
  generalized `subtree_text` API was discarded because (a) field-span
  based extent computation misses leading keywords like `SELECT`,
  (b) Perfetto's actual use case is a *specific* grammar rule, not
  every node, and (c) the formatter's `try_macro_verbatim` doesn't
  benefit from a subtree accessor. Replaced with a targeted `select_span`
  field on `CreatePerfettoTableStmt` / `ViewStmt` / `FunctionStmt`
  populated via `cur_shift_start` / `last_shifted_end` ctx fields
  and `select_body_start` / `select_body_end` empty-marker rules.
  The span excludes leading/trailing whitespace. Macro body was
  already handled by the existing `body` span: no changes needed.
  Steps 9/10 are dropped (they only existed to support the
  generalized subtree abstraction).
- **Session 2 follow-up (PR #96 review)** reworked the `traceback`
  API shape. The C side now owns a `SYNQ_VEC(SyntaqliteTracebackFrame)
  traceback_buf` scratch buffer on the parser; the function signature
  became `const SyntaqliteTracebackFrame* syntaqlite_parser_traceback(p,
  span, *out_count)`: single call, parser-owned storage, no
  caller-provided buffer. The Rust wrapper takes `&mut self` and
  returns `impl Iterator<Item = TracebackFrame<'a>>` that borrows
  directly from that buffer (zero copy). The `&mut` receiver makes
  "only one live traceback iterator at a time" a compile-time
  invariant: a second traceback call requires re-acquiring `&mut`,
  which conflicts with any outstanding iterator. Cascade: the
  validator's `ValidationPass` methods that transitively reach
  `emit()` take `stmt: &mut AnyParsedStatement<'b>` with a
  method-level `<'b>` lifetime generic (distinct from the pass's
  own `'a` for catalog / diagnostics borrows, so the two stay
  separable). Helper methods like `name_text` keep `&` and rely on
  auto-reborrow at call sites. `visit_children` collects child IDs
  into a `Vec` up front so the `child_node_ids` borrow is dropped
  before the recursive `&mut` visits. Traceback unit tests
  `.collect::<Vec<_>>()` the iterator.

### Step 1: Rename `_buf_idx` to `_layer_id`

Pure rename at the field level. Touches:
- `include/syntaqlite/types.h`: `SyntaqliteTextSpan._buf_idx` → `_layer_id`
- `include/syntaqlite/dialect.h`: `SynqParseToken.buf_idx` → `layer_id`
- `include/syntaqlite_dialect/ast_builder.h`: `SynqParseCtx.buf_idx` → `layer_id`
- `csrc/parser*.c`: all references, including `synq_span`, `synq_span_dequote`
- `src/parser/ffi.rs`, `src/ast.rs`, `src/parser/mod.rs`: Rust mirror

No semantic change. Tests untouched.

### Step 2: Rename `SynqMacroRegion` → `SynqExpansionLayer`

Internal rename. Unify `p->macro_expansions` and `p->macro_regions` into a
single `p->layers` vector with the union of fields the two structs held.
Update all internal call sites (`parser_macros.c`, `parser.c`,
`parser_dump.c`). Public API unchanged.

### Step 3: Add new layer metadata fields

Add to `SynqExpansionLayer`:
- `template_body` / `template_body_len` (borrowed directly from the
  macro registry entry's `body` field: no separate snapshot storage)
- `name` (borrowed from the registry entry's `name` field)
- `def_line` / `def_col` (from the macro registration site)
- `arg_segments` / `arg_segment_count` (empty for now)

Populate at `begin_macro_expansion` call sites. Extend the macro registry
entry (`SynqMacroEntry`) to carry `def_line`/`def_col` (passed in at
register time). Source of these values: the statement position where
`CREATE PERFETTO MACRO` was parsed.

**Decision on macro body provenance (resolves the open question
below):** the `template_body` shown in traceback frames for a macro
layer is sourced from the registry entry's existing `body` field: no
new `def_text_snapshot` field is added. The registry already owns a
copy of each macro's template; we point `template_body` at that copy
and rely on the fact that a registered macro's body outlives any parse
using it. This is the cheapest of the three options in the Open
Question section and costs zero extra memory.

### Step 4: Record `SynqArgSegment` during param substitution

Modify `synq_parser_expand_macro` to emit a `SynqArgSegment` each time
it substitutes a `$param` with caller arg text. The segment records:
- where the arg landed in the new layer's buf (`sub_offset`, `sub_length`)
- where it came from (`origin_layer_id` = current layer, `origin_offset`,
  `origin_length`)

Attach the segment list to the new expansion layer.

### Step 5: Add `_layer_id` field to `SyntaqliteParserToken`

Extend the public struct to carry layer_id. Populate at `synq_parser_record_and_feed`
time using the current tokenizer layer. Update `SynqParseToken` shift-time
propagation so tokens emitted from expansion buffers get the right layer_id.

ABI change. Document in public API notes. In-tree consumers to update:
- `python/csrc/_syntaqlite.c` (Python bindings)
- any CLI/WASM code that reads `SyntaqliteParserToken` (audit)

### Step 6: Add `span_text` / `span_expanded_text` / `span_text_range`

New C accessors. New Rust methods on `AnyParsedStatement`. Implemented
via the layer walk described under "Internal mechanics". Unit-tested
against simple nested macro cases.

At this point, all the infrastructure is in place but nothing consumes it
yet.

### Step 7: Add `traceback` with arg-segment drilling

New C + Rust API. Replaces `expansion_traceback` semantically but is NOT
wired up yet (the old API stays in parallel during migration).

Unit tests exercising:
- Traceback from a root-level span (single frame).
- Traceback from inside a macro body with no substitution in the path
  (two frames: root + macro).
- Traceback from inside a substituted arg, where the arg-segment drill
  collapses through to a single root frame at the user's authored arg
  text. This is success criterion #5.

The C function owns a `SYNQ_VEC(SyntaqliteTracebackFrame) traceback_buf`
scratch buffer on the parser. Each call clears and rewrites it, then
returns a pointer + count. The Rust wrapper takes `&mut self` and
returns `impl Iterator<Item = TracebackFrame<'a>>` borrowed from the
buffer: the `&mut` makes "one live iterator at a time" compile-time
enforced.

### Step 8: Targeted `select_span` on Perfetto CREATE stmts

**(Reshaped from the originally-planned generalized `subtree_text` API.)**

Scope: add a dedicated `select_span: inline SyntaqliteTextSpan` field
on `CreatePerfettoTableStmt`, `CreatePerfettoViewStmt`, and
`CreatePerfettoFunctionStmt` in `perfetto.synq`, populated explicitly by
the grammar action with the byte range of the authored `select` body
(trimmed of leading/trailing whitespace around the body).

Mechanism:

1. Add `cur_shift_start: u32` and `last_shifted_end: u32` fields on
   `SynqParseCtx` (in `ast_builder.h`). The former is set at the start
   of `synq_parser_record_and_feed` *before* `feed_one_token`; the
   latter is set *after* `feed_one_token` returns. This timing ensures
   that empty-rule reductions firing inside `feed_one_token(X)` see:
   - `cur_shift_start` = start of X (the token currently being
     processed), used by BEFORE-style markers to capture "start of the
     next real token";
   - `last_shifted_end` = end of the previously shifted terminal, used
     by AFTER-style markers to capture "end of the last real terminal".
   Both fields are tracked only for tokens shifted from the root
   source layer.
2. Add two empty marker rules in `perfetto.y`:
   ```
   %type select_body_start {uint32_t}
   select_body_start(A) ::= . { A = pCtx->cur_shift_start; }
   %type select_body_end {uint32_t}
   select_body_end(A) ::= . { A = pCtx->last_shifted_end; }
   ```
3. Update the three `cmd` actions to bracket `select(E)`:
   ```
   cmd(A) ::= CREATE … AS select_body_start(BS) select(E) select_body_end(BE). {
       SyntaqliteTextSpan select_span = {
           BS, (uint16_t)(BE - BS), 0, /*layer_id=*/0,
       };
       A = synq_parse_create_perfetto_table_stmt(pCtx, …, E, select_span);
   }
   ```

Consumers read `select_span` like any other AST span field and call
`span_text_range(select_span)` / `span_text(select_span)` to obtain
the authored byte range or slice.

Perfetto `perfetto_macro_body` already produces a `SynqParseToken` with
`.z`/`.n` merged across its ANY-wildcard tokens, which lands as a
`body` span via the existing `synq_span(pCtx, BODY)` conversion: no
change needed for the macro body case.

Test: an amalg diff test for `CREATE PERFETTO TABLE foo AS    SELECT 1   `
verifies that the recorded `select_span` is exactly `"SELECT 1"` (no
leading or trailing whitespace).

### Step 9: Lazy expanded splicer + `subtree_expanded_text` / `expanded_text`

**Dropped.** This step only existed to support the generalized
`subtree_text` / `subtree_expanded_text` accessors, which were also
dropped in the Step 8 reshape. Perfetto's actual use cases (issue #89)
are handled by the `select_span` field.

### Step 10: Migrate formatter's `try_macro_verbatim`

**Dropped.** The formatter's existing `MacroRegion` + peek-next-token
approach is correct and requires no changes. This step was a
consequence of the generalized subtree API that is no longer being
built.

### Step 11: Migrate analyzer's `statement_source`

Replace `analyzer.rs::statement_source()` with `stmt.text()` at its call
site. This is a silent correctness fix for statements containing macros;
unit-test it.

### Step 12: `FieldValue::Span` rename

Replace the current two-field `Span { text, source }` with four-field
`Span { text, expanded_text, text_range, quoted }`:
- old `text` (expansion-buffer slice) → new `expanded_text`
- old `source` (byte range) → new `text_range`
- new `text` (authored slice): populated from `span_text`

Compile errors at every call site will pinpoint the rename. Update them
all in one sweep.

### Step 13: Delete old expansion traceback API

Remove `syntaqlite_parser_expansion_traceback` and its C struct. Confirm
no callers remain (likely none after #87, but verify).

### Step 14: Delete `resolve_span` and `SyntaqliteResolvedSpan`

Already done in the PR #90 follow-up: `extract_field_value` was
migrated to populate `FieldValue::Span` directly from
`span_expanded_text` + `span_text_range` + `sp.is_quoted()`, which
removed the last caller of `resolve_span`. The C function and the
`SyntaqliteResolvedSpan` struct were then deleted (hard removal, no
shim retained).

### Step 15: Rename `syntaqlite_parser_text()` → `syntaqlite_parser_text()`

Hard rename, no compat alias. Update CLI, WASM playground, Python
bindings, and any tests referencing the old name. A grep for
`parser_source\b` must return zero after this step.

## What stays unchanged

The following parts of the codebase should require **no changes**:

- `synq_parser_expand_macro` (except for the arg-segment recording addition)
- `expand_and_feed` / the tokenizer swap machinery
- `synq_parser_try_macro_call` / balanced-paren arg scanning
- `synq_parser_check_macro_straddle` (the invariant it enforces is still
  needed)
- Grammar actions, `synq_span` / `synq_span_dequote`
- Lemon parser integration
- Macro registration and lookup (registry data structures may gain
  `def_line`/`def_col`, but the hashmap logic is unchanged)
- AST arena, node construction, list handling
- The `check_macro_straddle` diagnostic: still enforced at its existing
  call site

## Deferred / out of scope

The following are intentionally not part of this plan. They can be added
later if concrete need arises:

- **Generalized `subtree_text(node)` / `subtree_expanded_text(node)`.**
  Dropped in the Step 8 reshape. Field-span based subtree extents miss
  leading keywords, and the real-world use cases (Perfetto issue #89,
  the formatter's macro verbatim pass) don't need a generalized
  accessor. If a future consumer genuinely needs per-node authored
  text, the path forward is another targeted grammar-annotated span
  field like `select_span`, not a reintroduction of the generalized
  API.
- **Public `Rewriter` API.** Perfetto has one for transpilation patterns
  (`RewriteToDummySql`, `ExecuteCreateFunction`). We have no analogous use
  case today. If we ever want one (e.g., to wrap an engine-handled
  statement with synthetic SQL while preserving traceback), we'll add it
  then, modeled on Perfetto's 4-phase Build.
- **Public `Substr` on `Span`.** Perfetto's `SqlSource::Substr` is used
  to carve macro arguments out of caller source. Our expansion pipeline
  doesn't need it at the public boundary: arg segments are internal
  metadata.
- **Flat expanded offset space.** We chose lazy / layer-local offsets.
  If a future consumer genuinely needs "the expanded offset of this span
  as a u32 in the expanded_text() string", we'd need either eager
  materialization or a per-query translation. Not planned.
- **Parameter-level source tracking in macro definitions.** Macro bodies
  are stored as `(text, def_line, def_col)` in the registry. Perfetto
  stores them as full `SqlSource` instances with their own nested trees,
  so an error inside a macro body can trace to the module/file the macro
  was defined in. We can approximate this with just `(def_line, def_col)`
  for now; full fidelity would require registering macros with an
  identifier for their containing file/module.
- **Cross-statement macros through import boundaries.** If a macro is
  defined in one file and called from another, the traceback frame for
  the macro body should show the defining file's name. This depends on
  the module resolver integration and can be added once we have the
  basic layer tree working end to end.

## Success criteria

This refactor is done when:

1. **Public API uses the new vocabulary.** Every `source` in a user-facing
   name has become `text` or `expanded_text`. All accessors are named
   per the table above. *(Partially done: `parser_source` → `text`
   rename is deferred to Step 15.)*
2. **`_layer_id` is never referenced by any public API signature.** It
   exists as an implementation-detail field on `SyntaqliteTextSpan` and
   `SyntaqliteParserToken` but no public function takes it as a parameter
   or returns it. *(Done.)*
3. **`statement_source` in `analyzer.rs` is deleted.** Its replacement
   (`stmt.text()`) produces correct output for statements containing
   macros: tested via a regression test that would have failed today.
   *(Deferred to Step 11.)*
4. **Traceback fidelity matches Perfetto.** A diagnostic at an offset
   inside a substituted macro argument produces a traceback whose
   innermost frame points at the user's authored arg text, not at the
   whole `foo!(…)` call site. Tested by the Perfetto
   `MacroExpansionSpanRegression` diff test and by
   `traceback_span_inside_substituted_arg_drills_to_origin` unit test.
   *(Done.)*
5. **Issue #89 is satisfied for Perfetto's CREATE statements.** A
   consumer reads the `select_span` field on `CreatePerfettoTableStmt`
   / `CreatePerfettoViewStmt` / `CreatePerfettoFunctionStmt` to get
   the authored byte range of the `select` body. Tested via an amalg
   diff test. *(Done.)*
6. **No allocation for macro-free parsing beyond baseline.** A parse
   of a statement with no macro calls does not allocate any expansion
   layer buffers. The parser-owned `traceback_buf` is only grown when
   `traceback()` is called, and reused across calls.

   This remains a design goal enforced by the lazy-allocation
   architecture rather than a dedicated test.

## Resolved: macro body provenance for the definition frame

When we push a layer for `foo!(…)`, the traceback frame for the macro
definition needs some buffer to render the caret against. The registry
entry already owns a copy of the macro's body text (`SynqMacroEntry.body`
in `parser_internal.h`), stored at `register_macro` time. That copy is
what `expand_template` reads from during substitution, and it outlives
any parse that uses the macro.

**Decision: point `SynqExpansionLayer.template_body` directly at the
registry's `body` field.** No snapshot of the *defining statement* is
captured: only the macro body itself, which is what the traceback
needs for caret rendering. The `def_line` / `def_col` numbers are
informational metadata for the frame header; they do not index into any
buffer.

The originally-considered options of (a) snapshotting the whole
definition statement, (b) leaving the snippet empty, and (c) threading
a module resolver, are all either more expensive or more coupled than
using what the registry already has. We ship the registry-body pointer.
