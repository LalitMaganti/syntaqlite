# Text / Expanded-Text Model Plan

## Context

Today the parser exposes a multi-buffer model for macro expansion: spans
carry a `_buf_idx` that identifies which expansion buffer their offset lives
in, and consumers call `resolve_span` to walk the parent chain up to source.
This model has leaked across the public API in inconsistent ways:

- `FieldValue::Span::text` refers to the *expansion-buffer* slice, not the
  authored text — backwards from how most consumers think about "text".
- `analyzer.rs::statement_source()` slices source using raw token offsets,
  which is silently broken for any statement containing a macro call (token
  offsets for expansion-buffer tokens aren't source positions).
- The formatter's `try_macro_verbatim` has a bespoke scan over macro
  regions to decide when to emit an un-expanded call site, rather than
  using a shared primitive.
- Issue #89 asks for a subtree-level equivalent of `statement_source`, but
  there's no single concept of "the text of a subtree" that consumers can
  rely on.
- `expansion_traceback` is powerful but low-fidelity: it doesn't track
  argument provenance, so an error at byte N inside a substituted argument
  collapses to the whole `foo!(…)` call site rather than pointing at the
  user's authored arg expression.

The goal of this plan is to replace the ad-hoc multi-buffer surface with a
coherent vocabulary and API, restore Perfetto-style traceback fidelity
(argument-level), and make issue #89 fall out trivially as a side effect.

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

|                     | Whole statement        | Span                        | Subtree                        |
| ------------------- | ---------------------- | --------------------------- | ------------------------------ |
| **Authored**        | `text()`               | `span_text(span)`           | `subtree_text(node)`           |
| **Expanded**        | `expanded_text()`      | `span_expanded_text(span)`  | `subtree_expanded_text(node)`  |
| **Range in text()** | —                      | `span_text_range(span)`     | (derivable)                    |

Invariants:

- `span_text(span)` is *always* a slice of `text()`.
- `span_expanded_text(span)` is *content-equivalent* to the corresponding
  slice of `expanded_text()`, but may live in a different backing buffer
  (a private expansion layer). Users should treat it as an owned `&str`,
  not as "a pointer into some known buffer".
- For macro-free statements, all four collapse: `text() == expanded_text()`
  and `span_text == span_expanded_text` for every element.

## Architectural decision: lazy layer tree, no materialization

After considering two alternatives — (a) eager materialization into a single
flat `expanded_text` string at end of parse, and (b) a Perfetto-style
preprocessor pass producing a pre-built tree of `SqlSource` nodes — we
landed on a **lazy** model:

- During parse, the parser maintains an in-memory tree of "expansion
  layers" (the same multi-buffer structure we have today, with richer
  per-layer metadata).
- `parser_next` returns immediately after Lemon finishes — there is no
  post-parse materialization pass.
- Queries (`text`, `expanded_text`, `traceback`, subtree accessors) are
  computed on demand from the layer tree, with caching for the expensive
  ones (subtree extents, full expanded text splicer).

### Why lazy?

- **Zero baseline cost.** Parsers that don't ask about expanded text — the
  common case for formatters on macro-free SQL, validators, autocompletion
  — pay nothing for the machinery. No allocation, no walk.
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

An eager "build full expanded_text + per-node extents at end of parse"
approach works, but it unconditionally allocates O(statement size) on every
parse, even for the vast majority of parses that never query expanded text.
Lazy gives us the same end-state queryability without the baseline cost.

The eager design also committed us to a single flat offset space, which
would have made every span carry a rewritten-sql offset rather than a
layer-local one. That is cleaner conceptually but required us to rewrite
every AST span during materialization — more code churn for no functional
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

    // Side vector parallel to p->tokens, storing layer_id per token.
    // Only needed if we keep the public SyntaqliteParserToken struct
    // unchanged. See "Token layer_id" below.
    SYNQ_VEC(uint8_t) token_layer_ids;

    // Lazy caches (allocated on first query, freed on parser_next):
    SynqSubtreeExtentCache* extent_cache;       // per-node text ranges
    SynqExpandedTextCache* expanded_cache;      // spliced expanded strings
};
```

### Public types

```c
// Public struct; _layer_id is an implementation-detail field.
typedef struct SyntaqliteSourceSpan {
    uint32_t offset;
    uint16_t length;
    uint8_t flags;
    uint8_t _layer_id;   // Internal. Do not access.
} SyntaqliteSourceSpan;

// Byte range in text() (the user's authored input).
typedef struct SyntaqliteTextRange {
    uint32_t start;
    uint32_t end;
} SyntaqliteTextRange;

// Self-contained traceback frame. No pointers back to layers.
typedef struct SyntaqliteTracebackFrame {
    const char* name;             // "File \"stdin\"" or "Macro 'foo'"
    uint32_t line;
    uint32_t col;
    const char* snippet;          // the buffer to render the caret against
    uint32_t snippet_len;
    uint32_t offset_in_snippet;
} SyntaqliteTracebackFrame;
```

### Token layer_id

Tokens need to know which layer they came from so `span_expanded_text`
equivalents can resolve their buffer. Two options:

**(a) Add `uint8_t layer_id` (+ padding) to `SyntaqliteParserToken`.**
Same discipline as spans — public struct, underscore-prefixed field,
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

// The user's authored input. Same as syntaqlite_parser_source() today,
// renamed for vocabulary consistency.
SYNTAQLITE_API const char*
syntaqlite_parser_text(SyntaqliteParser* p, uint32_t* out_len);

// The full post-expansion text. Lazily built on first call; cached. For
// macro-free statements, aliases parser_text() (no allocation).
SYNTAQLITE_API const char*
syntaqlite_parser_expanded_text(SyntaqliteParser* p, uint32_t* out_len);

// ── Span accessors ───────────────────────────────────────────────────────

// Authored slice of text() corresponding to this span. For spans inside
// a macro expansion, collapses to the outermost call site's range in
// text(). Always a direct slice of text() — no allocation.
SYNTAQLITE_API const char*
syntaqlite_parser_span_text(SyntaqliteParser* p,
                            const SyntaqliteSourceSpan* span,
                            uint32_t* out_len);

// What the parser's tokenizer saw for this span. For spans outside a
// macro, equals span_text. For spans inside a macro, a slice of the
// expansion layer's buffer. Always a direct slice — no allocation.
SYNTAQLITE_API const char*
syntaqlite_parser_span_expanded_text(SyntaqliteParser* p,
                                     const SyntaqliteSourceSpan* span,
                                     uint32_t* out_len);

// Byte range of span_text in text(). Always a valid range in the user's
// authored input.
SYNTAQLITE_API SyntaqliteTextRange
syntaqlite_parser_span_text_range(SyntaqliteParser* p,
                                  const SyntaqliteSourceSpan* span);

// ── Subtree accessors ────────────────────────────────────────────────────

// Authored text covering the entire subtree rooted at node_id. Lazily
// computes per-node extents on first call; cached for subsequent queries.
SYNTAQLITE_API const char*
syntaqlite_parser_subtree_text(SyntaqliteParser* p,
                               uint32_t node_id,
                               uint32_t* out_len);

// Post-expansion text of the subtree. May require splicing across
// layer buffers when the subtree spans macro boundaries. Lazily built
// and cached.
SYNTAQLITE_API const char*
syntaqlite_parser_subtree_expanded_text(SyntaqliteParser* p,
                                        uint32_t node_id,
                                        uint32_t* out_len);

// ── Traceback ────────────────────────────────────────────────────────────

// Write up to max_frames traceback frames for the given span, ordered
// outermost (root) to innermost (deepest macro expansion). Returns the
// total number of frames available (caller can call with max_frames=0
// to query the count).
SYNTAQLITE_API uint32_t
syntaqlite_parser_traceback(SyntaqliteParser* p,
                            const SyntaqliteSourceSpan* span,
                            SyntaqliteTracebackFrame* frames,
                            uint32_t max_frames);
```

### Rust side

Mirror of the C API, returning `&'a str` with the parser's per-statement
lifetime.

```rust
impl<'a> AnyParsedStatement<'a> {
    // Whole
    pub fn text(&self) -> &'a str;
    pub fn expanded_text(&self) -> &'a str;

    // Span
    pub fn span_text(&self, span: Span) -> &'a str;
    pub fn span_expanded_text(&self, span: Span) -> &'a str;
    pub fn span_text_range(&self, span: Span) -> TextRange;

    // Subtree
    pub fn subtree_text(&self, node: NodeId) -> &'a str;
    pub fn subtree_expanded_text(&self, node: NodeId) -> &'a str;

    // Traceback
    pub fn traceback(&self, span: Span) -> impl Iterator<Item = Frame<'a>> + '_;
}
```

### `FieldValue::Span` — final shape

```rust
#[derive(Clone, Copy, Debug)]
pub enum FieldValue<'a> {
    NodeId(AnyNodeId),
    Span {
        /// Authored text — slice of `text()`.
        text: &'a str,
        /// What the parser saw — slice of whichever layer buf
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

### `span_text(span)` — O(depth)

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

### `span_expanded_text(span)` — O(1)

`&layers[span._layer_id].buf[span.offset..span.offset + span.length]`.
Trivial.

### `span_text_range(span)` — O(depth)

Same walk as `span_text`, but return the final `(off, off+len)` pair
instead of slicing.

### `subtree_text(node)` — O(subtree) first call, O(1) cached

On first call for any node in the statement: do one traversal of the AST,
populating a per-node extent array `[start, end) × node_count` with the
text-range of each subtree (computed from its descendants' direct field
spans via `span_text_range`). Cache on the parser.

Subsequent calls: direct array lookup, slice `text()`.

### `subtree_expanded_text(node)` — O(expanded size) first call, O(1) cached

More intricate because the result may need to splice across layer buffers.
On first call, walk the subtree:

1. Find the subtree's "host layer" — the layer that contains its root
   node's direct fields.
2. Inside the host layer's buf, find the start/end of the subtree as
   min/max of its direct field spans.
3. If the subtree has no descendants in other layers (no nested macro
   calls), return a direct slice of the host layer's buf.
4. Otherwise, splice: walk child layers whose `call_offset_in_parent`
   falls inside the subtree's range, recursively producing their expanded
   text and substituting at the correct positions.

Cache the result on the parser, indexed by node_id.

### `expanded_text()` — O(total) first call, O(1) cached

Same splicer as `subtree_expanded_text`, rooted at the statement's root
node. Fast path for macro-free statements: alias `text()`, no allocation.

### `traceback(span)` — O(depth)

Start at `span._layer_id` and walk toward root, emitting one frame per
layer. Drill through arg segments when the span's position falls inside
one:

```
fn traceback(span):
  layer_id = span._layer_id
  off = span.offset
  frames = []
  loop:
    layer = layers[layer_id]

    # Is our current position inside an arg segment of this layer?
    # If so, the "real" provenance at this level is the arg's origin
    # layer, not the expansion layer itself. Redirect.
    if arg_seg = find_arg_segment(layer, off):
      off = arg_seg.origin_offset + (off - arg_seg.sub_offset)
      layer_id = arg_seg.origin_layer_id
      continue  # retry at the origin layer

    # Emit a frame for this layer.
    if layer.parent_layer_id == UINT32_MAX:
      # Root — frame rendered against user source.
      frames.push(Frame {
        name: "File \"stdin\"" (or similar),
        line/col computed from off in text(),
        snippet: text(), offset_in_snippet: off,
      })
      break
    else:
      # Macro expansion — frame rendered against template body.
      frames.push(Frame {
        name: layer.name,  # e.g. "Macro 'foo'"
        line/col: layer.def_line/def_col adjusted by off,
        snippet: layer.template_body, offset_in_snippet: template_off,
      })
      # Walk up to parent at the call site.
      off = layer.call_offset_in_parent
      layer_id = layer.parent_layer_id

  reverse(frames)  # outermost first
  return frames
```

The frame rendered against a layer's `template_body` uses the template,
not the layer's `buf` (which has $params already substituted). This gives
the Perfetto-style behavior where a macro frame shows the original
template with `$param` references visible.

The arg-segment drill is the fidelity-adding mechanism: when a span was
tokenized inside a substituted arg, its provenance chain follows the arg
back to where the user typed it, not to the macro body's `$param`
reference.

## What disappears

From today's codebase:

- `SyntaqliteSourceSpan._buf_idx` field name (renamed to `_layer_id`).
- `SynqMacroRegion` type (unified into `SynqExpansionLayer`).
- `macro_regions` and `macro_expansions` parser vectors (unified into
  `layers`).
- `synq_span_needs_resolve` inline helper (no longer meaningful — all
  accessors handle the layer walk internally).
- `resolve_span`'s expansion-walking code path (simplified to a
  straight call into `span_text` / `span_expanded_text` /
  `span_text_range`; may be kept as a thin compat shim or removed
  entirely).
- `syntaqlite_parser_expansion_traceback` (replaced by the new
  `syntaqlite_parser_traceback` with argument fidelity).
- `try_macro_verbatim`'s bespoke scan in `fmt/formatter.rs` (replaced by
  a call to `subtree_text` on the child node).
- `analyzer.rs::statement_source()` helper (replaced by
  `stmt.text()` — which is now correct for macros instead of silently
  broken).
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
| 7. Add `traceback` with arg-segment drilling        | ✅ done | New `SyntaqliteTracebackFrame` struct + `syntaqlite_parser_traceback` C API, `TracebackFrame<'a>` + `AnyParsedStatement::traceback` Rust method. Validator migrated off the old API. See session 2 notes. |
| 8. Targeted `select_span` on Perfetto CREATE stmts (was: subtree extent cache) | ✅ done (reshaped) | **Scope change**: the generalized `subtree_text` API was dropped in favor of a targeted grammar-annotation mechanism. Added `cur_shift_start`/`last_shifted_end` tracking on `SynqParseCtx` (updated in `record_and_feed` before/after `feed_one_token`) plus empty-marker rules `select_body_start` / `select_body_end` in `perfetto.y`. Added `select_span` field to `CreatePerfettoTableStmt` / `CreatePerfettoViewStmt` / `CreatePerfettoFunctionStmt`. Consumers read the field like any other span and call `span_text_range(select_span)` for the authored byte range. Perfetto macro body already lands as a span via existing `synq_span` conversion — no change needed. |
| 9. Lazy expanded splicer + `subtree_expanded_text` / `expanded_text` | ❌ dropped | Obviated by Step 8's reshape. Macro-containing subtrees don't need a generalized splicer; Perfetto's actual use cases are handled by the `select_span` field (whose authored bytes come from the root layer directly). |
| 10. Migrate formatter's `try_macro_verbatim`        | ❌ dropped | Obviated by Step 8's reshape. The formatter's existing `MacroRegion` + peek-next-token approach is correct and doesn't benefit from a subtree accessor. |
| 11. Migrate analyzer's `statement_source`           | ⏳ deferred | |
| 12. `FieldValue::Span` rename                       | ⏳ partial | `extract_field_value` now populates `FieldValue::Span` from the new trio (Session 1 follow-up, PR #90 review). Full rename of the variant's field names (`source` → `text_range`, adding a second authored-text field) still deferred. |
| 13. Delete old expansion traceback API              | ✅ done | Removed `syntaqlite_parser_expansion_traceback`, `SyntaqliteExpansionFrame`, `ExpansionFrame`, `field_expansion_traceback`. Validator migrated to `traceback()`. |
| 14. Delete `resolve_span` and `SyntaqliteResolvedSpan` | ✅ done | Brought forward from Session 1 follow-up (PR #90 review). Zero callers remain. |
| 15. Rename `syntaqlite_parser_source()` → `syntaqlite_parser_text()` | ⏳ deferred | |

**Notes on Step 6 (session 1):**
- The existing Rust `AnyParsedStatement::span_text` method (which returned
  the expansion-buffer slice) was renamed to `span_expanded_text` as part
  of this step — otherwise the new `span_text` (authored slice) would
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
test against — they proceed as straight refactors, verified by existing
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
  `span_text_range` + `sp.is_quoted()` — a partial Step 12 landing.
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
  `MacroExpansionSpanRegression` diff test split into two cases — one
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
  already handled by the existing `body` span — no changes needed.
  Steps 9/10 are dropped (they only existed to support the
  generalized subtree abstraction).

### Step 1 — Rename `_buf_idx` to `_layer_id`

Pure rename at the field level. Touches:
- `include/syntaqlite/types.h` — `SyntaqliteSourceSpan._buf_idx` → `_layer_id`
- `include/syntaqlite/dialect.h` — `SynqParseToken.buf_idx` → `layer_id`
- `include/syntaqlite_dialect/ast_builder.h` — `SynqParseCtx.buf_idx` → `layer_id`
- `csrc/parser*.c` — all references, including `synq_span`, `synq_span_dequote`
- `src/parser/ffi.rs`, `src/ast.rs`, `src/parser/mod.rs` — Rust mirror

No semantic change. Tests untouched.

### Step 2 — Rename `SynqMacroRegion` → `SynqExpansionLayer`

Internal rename. Unify `p->macro_expansions` and `p->macro_regions` into a
single `p->layers` vector with the union of fields the two structs held.
Update all internal call sites (`parser_macros.c`, `parser.c`,
`parser_dump.c`). Public API unchanged.

### Step 3 — Add new layer metadata fields

Add to `SynqExpansionLayer`:
- `template_body` / `template_body_len` (borrowed directly from the
  macro registry entry's `body` field — no separate snapshot storage)
- `name` (borrowed from the registry entry's `name` field)
- `def_line` / `def_col` (from the macro registration site)
- `arg_segments` / `arg_segment_count` (empty for now)

Populate at `begin_macro_expansion` call sites. Extend the macro registry
entry (`SynqMacroEntry`) to carry `def_line`/`def_col` (passed in at
register time). Source of these values: the statement position where
`CREATE PERFETTO MACRO` was parsed.

**Decision on macro body provenance (resolves the open question
below):** the `template_body` shown in traceback frames for a macro
layer is sourced from the registry entry's existing `body` field — no
new `def_text_snapshot` field is added. The registry already owns a
copy of each macro's template; we point `template_body` at that copy
and rely on the fact that a registered macro's body outlives any parse
using it. This is the cheapest of the three options in the Open
Question section and costs zero extra memory.

### Step 4 — Record `SynqArgSegment` during param substitution

Modify `synq_parser_expand_macro` to emit a `SynqArgSegment` each time
it substitutes a `$param` with caller arg text. The segment records:
- where the arg landed in the new layer's buf (`sub_offset`, `sub_length`)
- where it came from (`origin_layer_id` = current layer, `origin_offset`,
  `origin_length`)

Attach the segment list to the new expansion layer.

### Step 5 — Add `_layer_id` field to `SyntaqliteParserToken`

Extend the public struct to carry layer_id. Populate at `synq_parser_record_and_feed`
time using the current tokenizer layer. Update `SynqParseToken` shift-time
propagation so tokens emitted from expansion buffers get the right layer_id.

ABI change. Document in public API notes. In-tree consumers to update:
- `python/csrc/_syntaqlite.c` (Python bindings)
- any CLI/WASM code that reads `SyntaqliteParserToken` (audit)

### Step 6 — Add `span_text` / `span_expanded_text` / `span_text_range`

New C accessors. New Rust methods on `AnyParsedStatement`. Implemented
via the layer walk described under "Internal mechanics". Unit-tested
against simple nested macro cases.

At this point, all the infrastructure is in place but nothing consumes it
yet.

### Step 7 — Add `traceback` with arg-segment drilling

New C + Rust API. Replaces `expansion_traceback` semantically but is NOT
wired up yet (the old API stays in parallel during migration).

Unit tests exercising:
- Traceback from a root-level span (single frame)
- Traceback from inside a macro body (two frames)
- Traceback from inside a substituted arg (three frames: root → macro → arg origin)
- Traceback from nested macro inside an arg (root → outer → arg origin → inner)
- Per Perfetto's "Fully expanded statement" header, the root frame
  includes the expanded snippet when the statement contains any
  expansion; deeper frames don't.

### Step 8 — Lazy subtree extent cache + `subtree_text`

Add the per-statement cache structure and the single-pass extent walker.
Implement `subtree_text(node)`. Unit tests for macro-free and macro-laden
subtrees.

### Step 9 — Lazy expanded splicer + `subtree_expanded_text` / `expanded_text`

Implement the splicer for subtree_expanded_text and expanded_text. Unit
tests covering:
- Macro-free subtree (fast path: direct slice, no splice)
- Single-macro subtree
- Nested macros with arg substitutions

### Step 10 — Migrate formatter's `try_macro_verbatim`

Replace the bespoke macro region scan with `ctx.reader.subtree_text(child_id)`.
Diff-test the formatter to ensure zero behavioral change.

### Step 11 — Migrate analyzer's `statement_source`

Replace `analyzer.rs::statement_source()` with `stmt.text()` at its call
site. This is a silent correctness fix for statements containing macros;
unit-test it.

### Step 12 — `FieldValue::Span` rename

Replace the current two-field `Span { text, source }` with four-field
`Span { text, expanded_text, text_range, quoted }`:
- old `text` (expansion-buffer slice) → new `expanded_text`
- old `source` (byte range) → new `text_range`
- new `text` (authored slice) — populated from `span_text`

Compile errors at every call site will pinpoint the rename. Update them
all in one sweep.

### Step 13 — Delete old expansion traceback API

Remove `syntaqlite_parser_expansion_traceback` and its C struct. Confirm
no callers remain (likely none after #87, but verify).

### Step 14 — Delete `resolve_span` and `SyntaqliteResolvedSpan`

By this point, all in-tree callers of `syntaqlite_parser_resolve_span`
have been migrated to the new span accessors:
- `FieldValue::Span` is populated directly from `span_text` /
  `span_expanded_text` / `span_text_range` (Step 12).
- The formatter uses `subtree_text` (Step 10).
- The validator uses `stmt.text()` (Step 11).

Remove the C function and the `SyntaqliteResolvedSpan` struct entirely.
No thin shim is retained — a hard delete, verified by a final grep for
`resolve_span` turning up zero matches.

Strategy for intermediate steps: **keep `resolve_span` alive and
unchanged from Step 1 through Step 12.** It acts as a thin compat layer
for the old `FieldValue::Span.text` / `FieldValue::Span.source`
population path until Step 12 rewrites that path on top of the new
accessors. Steps 1–5 do not touch `resolve_span` at all; Step 6 adds
the new accessors alongside it; Steps 7–11 migrate consumers one at a
time; Step 12 migrates the last consumer (`FieldValue::Span`); Step 14
deletes the old API.

### Step 15 — Rename `syntaqlite_parser_source()` → `syntaqlite_parser_text()`

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
- The `check_macro_straddle` diagnostic — still enforced at its existing
  call site

## Deferred / out of scope

The following are intentionally not part of this plan. They can be added
later if concrete need arises:

- **Public `Rewriter` API.** Perfetto has one for transpilation patterns
  (`RewriteToDummySql`, `ExecuteCreateFunction`). We have no analogous use
  case today. If we ever want one (e.g., to wrap an engine-handled
  statement with synthetic SQL while preserving traceback), we'll add it
  then, modeled on Perfetto's 4-phase Build.
- **Public `Substr` on `Span`.** Perfetto's `SqlSource::Substr` is used
  to carve macro arguments out of caller source. Our expansion pipeline
  doesn't need it at the public boundary — arg segments are internal
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
   name has become `text` or `expanded_text`. `SyntaqliteTextRange`
   replaces `SourceRange`. All accessors are named per the table above.
2. **`_layer_id` is never referenced by any public API signature.** It
   exists as an implementation-detail field on `SyntaqliteSourceSpan` and
   `SyntaqliteParserToken` but no public function takes it as a parameter
   or returns it.
3. **`statement_source` in `analyzer.rs` is deleted.** Its replacement
   (`stmt.text()`) produces correct output for statements containing
   macros — tested via a regression test that would have failed today.
4. **`try_macro_verbatim` has no bespoke macro-region scan.** It calls
   `subtree_text` on the child node and nothing else.
5. **Traceback fidelity matches Perfetto.** A diagnostic at an offset
   inside a substituted macro argument produces a traceback whose
   innermost frame points at the user's authored arg text, not at the
   whole `foo!(…)` call site. Tested via a unit test that exercises
   `outer!(inner!(a+b))` and verifies the frame chain.
6. **Issue #89 is trivially satisfied.** A dialect consumer calls
   `subtree_expanded_text(subtree_root)` and gets the correct forwardable
   SQL text. Tested via an integration test.
7. **No allocation for macro-free parsing.** A parse of a statement with
   no macro calls does not allocate any expansion layer buffers, any
   subtree extent cache, or any expanded text cache unless a query
   explicitly requests it.

   This remains a design goal, enforced by the lazy-caching architecture
   rather than a dedicated test. The caches are arena-allocated (see
   "Lazy cache invalidation" below), so a parse that never touches a
   lazy query makes no extra heap allocations beyond what the baseline
   parse already does.

### Lazy cache invalidation

The per-statement lazy caches (`SynqSubtreeExtentCache`,
`SynqExpandedTextCache`) live on the parser arena and are reset
whenever `syntaqlite_parser_next` resets the arena between statements.
Nothing explicit is needed at the end of a parse — the arena reset
already invalidates them. The only cache-owned state on the parser
struct is a pair of pointers to the cache heads in arena memory; those
pointers are zeroed on reset and repopulated lazily on first query.

## Resolved: macro body provenance for the definition frame

When we push a layer for `foo!(…)`, the traceback frame for the macro
definition needs some buffer to render the caret against. The registry
entry already owns a copy of the macro's body text (`SynqMacroEntry.body`
in `parser_internal.h`), stored at `register_macro` time. That copy is
what `expand_template` reads from during substitution, and it outlives
any parse that uses the macro.

**Decision: point `SynqExpansionLayer.template_body` directly at the
registry's `body` field.** No snapshot of the *defining statement* is
captured — only the macro body itself, which is what the traceback
needs for caret rendering. The `def_line` / `def_col` numbers are
informational metadata for the frame header; they do not index into any
buffer.

The originally-considered options of (a) snapshotting the whole
definition statement, (b) leaving the snippet empty, and (c) threading
a module resolver, are all either more expensive or more coupled than
using what the registry already has. We ship the registry-body pointer.
