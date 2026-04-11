// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Private header shared by parser.c, parser_macros.c, and parser_dump.c.

#ifndef SYNTAQLITE_CSRC_PARSER_INTERNAL_H
#define SYNTAQLITE_CSRC_PARSER_INTERNAL_H

#include <stdint.h>

#include "syntaqlite/config.h"
#include "syntaqlite/dialect.h"
#include "syntaqlite_dialect/ast_builder.h"

#ifdef __cplusplus
extern "C" {
#endif

// ── Tunables ────────────────────────────────────────────────────────────────

#define SYNQ_MAX_MACRO_DEPTH 16
#define SYNQ_MACRO_TABLE_INITIAL_SIZE 16

#if defined(__GNUC__) || defined(__clang__)
#define SYNQ_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define SYNQ_NOINLINE __declspec(noinline)
#else
#define SYNQ_NOINLINE
#endif

// ── Macro registry & expansion types ────────────────────────────────────────

// A single registered macro.
typedef struct SynqMacroEntry {
  char* name;  // Owned copy of the macro name.
  uint32_t name_len;

  // --- Template macros ---
  char* body;  // Body text with $param placeholders. Owned.
  uint32_t body_len;
  char** param_names;  // Array of param name strings. Owned.
  uint32_t* param_name_lens;
  uint32_t param_count;

  // --- Definition provenance ---
  // Line/column of the `CREATE PERFETTO MACRO` statement that defined this
  // entry (1-based). Zero means "unknown". Used by traceback rendering to
  // label macro-body frames with their authoring position. The macro body
  // text itself is `body` above — traceback frames borrow that pointer.
  uint32_t def_line;
  uint32_t def_col;

  uint8_t state;  // SYNQ_MAP_EMPTY / LIVE / TOMBSTONE
} SynqMacroEntry;

// One parameter substitution recorded during macro expansion. Tracks where
// the copied arg text landed in the child layer's buffer and where it came
// from in the parent layer's buffer. Populated by expand_template during
// Step 4; consumed by argument-level traceback (Step 7).
typedef struct SynqArgSegment {
  uint32_t sub_offset;       // Where the substituted arg landed in the child
                             // layer's buffer.
  uint32_t sub_length;       // Length of the substitution.
  uint32_t origin_layer_id;  // Layer that owned the arg text.
  uint32_t origin_offset;    // Where the arg text started in the origin layer.
  uint32_t origin_length;    // Length of the origin arg text (may differ from
                             // sub_length if the arg was rewritten — presently
                             // the two are always equal).
} SynqArgSegment;

// A comma-separated argument extracted from a macro call site.
typedef struct SynqMacroArg {
  uint32_t offset;  // Byte offset in the source buffer.
  uint32_t length;  // Byte length of the argument text.
} SynqMacroArg;

// Expansion layer record.  `_layer_id` on AST spans indexes directly into
// the parser's layers vector.  Entry 0 is a sentinel for the original
// source (expansion_data = source pointer, parent_layer_id = 0,
// call_offset/call_length = 0, template_body/name/arg_segments all NULL).
// Actual expansions start at index 1.
//
// Template/name/def_line/def_col are borrowed pointers into the macro
// registry entry that was expanded to produce this layer; they outlive the
// layer because parse state is reset before any registry entries can be
// freed.  For the sentinel (layer 0) and incremental-API begin_macro (no
// registry entry), these fields are all NULL/0.
typedef struct SynqExpansionLayer {
  uint32_t call_offset;        // Byte offset of macro call in parent layer.
  uint32_t call_length;        // Byte length of entire macro call.
  const char* expansion_data;  // Expanded text (NULL for sentinel/fallback).
  uint32_t expansion_len;      // Length of expanded text.

  // Definition provenance (borrowed from macro registry entry).
  const char* template_body;   // Registry body with $params visible.
  uint32_t template_body_len;  // Length of template_body, or 0.
  const char* name;            // Macro name (borrowed), or NULL.
  uint32_t name_len;           // Length of name.
  uint32_t def_line;           // Macro definition line (1-based, 0=unknown).
  uint32_t def_col;            // Macro definition column (1-based, 0=unknown).

  // Parameter substitutions recorded during expand_template (Step 4).
  // Arg segments live in p->mem; freed in reset_stmt / destroy alongside
  // expansion_data. Empty for layers without parameter substitution.
  SynqArgSegment* arg_segments;
  uint32_t arg_segment_count;

  uint8_t parent_layer_id;  // Layer containing the call (0 = source).
} SynqExpansionLayer;

// Result of a successful macro expansion (pure template substitution).
// `data` and `arg_segments` are caller-owned (allocated via p->mem);
// ownership transfers to the layer record when fed via
// synq_parser_feed_macro_expansion().
typedef struct SynqMacroExpansion {
  const SynqMacroEntry* entry;  // Registry entry (for blue-paint).
  char* data;                   // Expanded text.
  uint32_t data_len;            // Length of expanded text.
  uint32_t end_offset;          // Position past ')' in the source buf.
  // Arg-segment list recorded during template substitution.  Sorted by
  // sub_offset (ascending), non-overlapping.  NULL when the template has
  // no $param placeholders.  Allocated via p->mem.
  SynqArgSegment* arg_segments;
  uint32_t arg_segment_count;
} SynqMacroExpansion;

// ── Parser struct ───────────────────────────────────────────────────────────

struct SyntaqliteParser {
  SyntaqliteMemMethods mem;
  SyntaqliteDialect dialect;
  void* lemon;
  SynqParseCtx ctx;
  const char* source;
  uint32_t source_len;
  uint32_t offset;           // Tokenizer cursor into source.
  uint32_t last_token_type;  // Last non-whitespace token fed to Lemon.
  uint32_t finished;         // 1 after EOF has been sent to Lemon.
  uint32_t had_comment;      // 1 if any comment token was seen this stmt.
  uint32_t had_error;        // Sticky error flag for current result.
  int32_t last_status;       // Last SYNTAQLITE_PARSE_* status returned.
  char error_msg[256];       // Error message buffer.
  uint32_t trace;
  uint32_t collect_tokens;
  uint32_t macro_fallback;  // 1 = unregistered name!(args) becomes TK_ID.
  uint32_t sealed;
  uint32_t pending_reset;  // 1 after feed_token signals completion; cleared on
                           // the next feed_token call (arena reset deferred).
  SYNQ_VEC(SyntaqliteComment) comments;
  SYNQ_VEC(SyntaqliteParserToken) tokens;
  uint32_t macro_depth;  // Nesting depth (0 = not in macro).

  // Unified layer tree.  Entry 0 is a sentinel representing the original
  // source; actual expansions start at index 1.  `_layer_id` on AST spans
  // indexes directly into this vector.
  SYNQ_VEC(SynqExpansionLayer) layers;

  // ── Macro registry (open-addressing hashmap) ──────────────────────────
  SynqMacroEntry* macro_table;
  uint32_t macro_table_size;   // Capacity (power of 2).
  uint32_t macro_table_count;  // Number of live entries.

  // ── Expansion state ───────────────────────────────────────────────────
  // Blue-paint recursion detection: names of macros currently being expanded.
  const char* expansion_names[SYNQ_MAX_MACRO_DEPTH];
  uint32_t expansion_name_lens[SYNQ_MAX_MACRO_DEPTH];
  uint32_t expansion_depth;
};

// ── Cross-file helpers ──────────────────────────────────────────────────────

// Set the parser's last_status and return it (used by both parser.c and
// parser_macros.c as a convenient exit helper).
int32_t synq_parser_set_result_status(SyntaqliteParser* p, int32_t rc);

// Feed one token to Lemon. Returns 0/1/-1 (see parser.c for semantics).
int synq_parser_feed_one_token(SyntaqliteParser* p,
                               uint32_t token_type,
                               const char* text,
                               uint32_t len,
                               uint32_t token_idx);

// Record a token into p->tokens (if enabled) and feed it to Lemon.
int synq_parser_record_and_feed(SyntaqliteParser* p,
                                uint32_t cur_type,
                                uint32_t cur_offset,
                                uint32_t cur_len);

// Scan balanced parens for macro args.  Defined in parser_macros.c.
uint32_t synq_parser_scan_macro_args(SyntaqliteParser* p,
                                     const char* source,
                                     uint32_t source_len,
                                     uint32_t bang_offset,
                                     SynqMacroArg* out_args,
                                     uint32_t max_args,
                                     uint32_t* out_end_offset);

// Pure template expansion (no token feeding).  Defined in parser_macros.c.
int synq_parser_expand_macro(SyntaqliteParser* p,
                             const char* buf,
                             uint32_t buf_len,
                             uint32_t id_offset,
                             uint32_t id_len,
                             uint32_t bang_offset,
                             SynqMacroExpansion* out);

// Register an expansion, feed its tokens, and clean up.  Defined in
// parser_macros.c.
int synq_parser_feed_macro_expansion(SyntaqliteParser* p,
                                     uint32_t call_offset,
                                     uint32_t call_length,
                                     SynqMacroExpansion* exp,
                                     uint32_t depth);

// Try to expand a Rust-style macro call: ID!(args).  Defined in
// parser_macros.c.
int synq_parser_try_macro_call(SyntaqliteParser* p,
                               uint32_t id_offset,
                               uint32_t id_len,
                               uint32_t bang_offset);

// Diagnose macro expansions that straddle AST node boundaries.  Defined in
// parser_macros.c.
int synq_parser_check_macro_straddle(SyntaqliteParser* p);

// Free a single macro registry entry's owned strings.  Defined in
// parser_macros.c.
void synq_parser_free_macro_entry(SyntaqliteParser* p, SynqMacroEntry* e);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_CSRC_PARSER_INTERNAL_H
