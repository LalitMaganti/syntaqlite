// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Dialect descriptor.
//
// A concrete dialect (e.g. SQLite, Perfetto) fills one static
// SyntaqliteDialectTemplate and exposes it via a SyntaqliteDialect accessor:
//
//   SyntaqliteDialect syntaqlite_<name>_dialect(void);
//
// ── Custom include ──────────────────────────────────────────────────────
//
// Define SYNTAQLITE_CUSTOM_INCLUDE to a filename to have it included before
// any macro decisions. This follows the SQLite SQLITE_CUSTOM_INCLUDE pattern.
//
//   cc -DSYNTAQLITE_CUSTOM_INCLUDE=synq_config.h -I. ...
//
// The config file can set SYNTAQLITE_SQLITE_VERSION, SYNTAQLITE_SQLITE_CFLAGS,
// and individual SYNTAQLITE_CFLAG_* defines.

#ifndef SYNTAQLITE_DIALECT_H
#define SYNTAQLITE_DIALECT_H

#ifdef SYNTAQLITE_CUSTOM_INCLUDE
#define SYNQ_STRINGIFY_(x) #x
#define SYNQ_STRINGIFY(x) SYNQ_STRINGIFY_(x)
#include SYNQ_STRINGIFY(SYNTAQLITE_CUSTOM_INCLUDE)
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "syntaqlite/cflags.h"
#include "syntaqlite/config.h"

#ifdef __cplusplus
extern "C" {
#endif

// ── Token category ────────────────────────────────────────────────────────

// Semantic category of a SQL token, used for syntax highlighting.
// Stored as uint8_t in SyntaqliteDialectTemplate::token_categories.
typedef enum {
  SYNQ_TOKEN_CATEGORY_OTHER = 0,
  SYNQ_TOKEN_CATEGORY_KEYWORD = 1,
  SYNQ_TOKEN_CATEGORY_IDENTIFIER = 2,
  SYNQ_TOKEN_CATEGORY_STRING = 3,
  SYNQ_TOKEN_CATEGORY_NUMBER = 4,
  SYNQ_TOKEN_CATEGORY_OPERATOR = 5,
  SYNQ_TOKEN_CATEGORY_PUNCTUATION = 6,
  SYNQ_TOKEN_CATEGORY_COMMENT = 7,
  SYNQ_TOKEN_CATEGORY_VARIABLE = 8,
  SYNQ_TOKEN_CATEGORY_FUNCTION = 9,
  SYNQ_TOKEN_CATEGORY_TYPE = 10,
} SynqTokenCategory;

// ── Macro invocation style ───────────────────────────────────────────────

// How the batch parsing loop (`syntaqlite_parser_next`) auto-detects macro
// invocations in the token stream.  In incremental/embedded mode, callers
// handle macros via `begin_macro`/`end_macro` directly and this is ignored.
typedef enum {
  // No macro detection.
  SYNQ_MACRO_STYLE_NONE = 0,
  // Rust-style: `name!(...)` — ID followed by `!` then balanced parens.
  SYNQ_MACRO_STYLE_RUST = 1,
} SyntaqliteMacroStyle;

// ── Types used by the parser vtable ─────────────────────────────────────
typedef struct SynqParseCtx SynqParseCtx;

typedef struct SynqParseToken {
  const char* z;       // pointer to start of token in source text
  uint32_t n;          // byte length of token
  uint32_t type;       // token type ID (SYNTAQLITE_TK_*)
  uint32_t token_idx;  // index into parser's token vec (0xFFFFFFFF if not
                       // collecting)
} SynqParseToken;

typedef struct SyntaqliteFieldRangeMeta SyntaqliteFieldRangeMeta;
typedef struct SyntaqliteRangeMetaEntry SyntaqliteRangeMetaEntry;
typedef struct SyntaqliteFieldMeta SyntaqliteFieldMeta;

// Forward declaration for use in function pointers below.
typedef struct SyntaqliteDialect SyntaqliteDialect;

// ── Dialect template (static dialect data) ────────────────────────────────

// Static dialect descriptor: parser vtable, AST metadata, formatter bytecode,
// and semantic-role tables.  All fields are always present; optional sections
// (fmt, validation) are zeroed when the feature is not compiled in.
typedef struct SyntaqliteDialectTemplate {
  const char* name;

  // Range metadata for the macro straddle check.
  const SyntaqliteRangeMetaEntry* range_meta;

  // AST metadata — all arrays indexed by node tag, length = node_count.
  uint32_t node_count;
  const char* const* node_names;
  const SyntaqliteFieldMeta* const* field_meta;
  const uint8_t* field_meta_counts;
  const uint8_t* list_tags;  // 1 = list node

  // Parser lifecycle (Lemon parser, provided by grammar)
  void* (*parser_alloc)(void* (*mallocProc)(size_t), SynqParseCtx* pCtx);
  void (*parser_init)(void* parser, SynqParseCtx* pCtx);
  void (*parser_finalize)(void* parser);
  void (*parser_free)(void* parser, void (*freeProc)(void*));
  void (*parser_feed)(void* parser, int token_type, SynqParseToken minor);
  void (*parser_trace)(FILE* trace_file, char* prompt);
  uint32_t (*parser_expected_tokens)(void* parser,
                                     uint32_t* out_tokens,
                                     uint32_t out_cap);
  uint32_t (*parser_completion_context)(void* parser);

  // Tokenizer (provided by grammar)
  int64_t (*get_token)(const SyntaqliteDialect* env,
                       const unsigned char* z,
                       int* tokenType);

  // Keyword table exported by mkkeywordhash output (`sqlite_keyword.c`).
  const char* keyword_text;         // concatenated keyword bytes
  const uint16_t* keyword_offsets;  // keyword_count entries
  const uint8_t* keyword_lens;      // keyword_count entries
  const uint8_t* keyword_codes;   // keyword_count entries (token type ordinals)
  const uint32_t* keyword_count;  // points to keyword count scalar

  // Token metadata (indexed by token type ordinal)
  // length = token_type_count; NULL = no categories
  const uint8_t* token_categories;
  uint32_t token_type_count;

  // Macro invocation style for the batch parsing loop.
  // Determines how the parser auto-detects macro calls when tokenizing.
  // In incremental/embedded mode, callers handle macros via
  // begin_macro/end_macro directly — this field is only used by
  // syntaqlite_parser_next().
  SyntaqliteMacroStyle macro_style;

  // Formatter bytecode (zeroed when formatting is not compiled in).
  const uint8_t* fmt_str_data;
  const uint32_t* fmt_str_offsets;
  uint32_t fmt_str_count;
  const uint16_t* fmt_enum_display;
  uint32_t fmt_enum_display_count;
  const uint8_t* fmt_ops;
  uint32_t fmt_ops_count;
  const uint32_t* fmt_dispatch;
  uint32_t fmt_dispatch_count;
  const uint8_t* fmt_prec_table;
  uint32_t fmt_prec_table_count;
  const uint32_t* fmt_expr_meta;
  uint32_t fmt_expr_meta_count;

  // Semantic role tables (zeroed when validation is not compiled in).
  const uint8_t* roles_data;
  uint32_t roles_count;
  const uint8_t* macro_defs_data;
  uint32_t macro_defs_count;
} SyntaqliteDialectTemplate;

// ── Configured dialect handle ─────────────────────────────────────────────

typedef struct SyntaqliteDialect {
  const SyntaqliteDialectTemplate* tmpl;
  int32_t sqlite_version;   // Target version (e.g., 3035000). INT32_MAX =
                            // latest.
  SyntaqliteCflags cflags;  // Active compile-time flags.
} SyntaqliteDialect;

// Default dialect: latest version, no cflags.
#define SYNQ_DIALECT_DEFAULT(d) {(d), INT32_MAX, SYNQ_CFLAGS_DEFAULT}

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_DIALECT_H
