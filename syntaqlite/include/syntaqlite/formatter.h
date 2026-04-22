// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// SQL formatter C API — formats SQL source text with configurable style.
//
// Lifecycle:
//   SyntaqliteFormatter* f = syntaqlite_formatter_create_sqlite();
//   if (syntaqlite_formatter_format(f, sql, len) == 0) {
//     const char* out = syntaqlite_formatter_output(f);
//     SyntaqliteLength out_len = syntaqlite_formatter_output_len(f);
//     // use out[0..out_len]
//   } else {
//     const char* err = syntaqlite_formatter_error_msg(f);
//     // handle parse error
//   }
//   syntaqlite_formatter_destroy(f);
//
// The formatter is reusable — call format() multiple times on the same handle.
// Output and error pointers are valid until the next format() or destroy().

#ifndef SYNTAQLITE_FORMATTER_H
#define SYNTAQLITE_FORMATTER_H

#include <stdint.h>
#include "syntaqlite/config.h"
#include "syntaqlite/dialect.h"
#include "syntaqlite/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque formatter handle. Owns a Formatter internally.
typedef struct SyntaqliteFormatter SyntaqliteFormatter;

// Keyword casing options.
typedef enum {
  SYNTAQLITE_KEYWORD_UPPER = 0,
  SYNTAQLITE_KEYWORD_LOWER = 1,
} SyntaqliteKeywordCase;

// Formatter configuration.
typedef struct {
  uint32_t line_width;                // Max line width before breaking (default: 80)
  uint32_t indent_width;              // Spaces per indent level (default: 2)
  SyntaqliteKeywordCase keyword_case; // Keyword casing (default: UPPER)
  uint32_t semicolons;                // Append semicolons (0 = no, nonzero = yes, default: 1)
} SyntaqliteFormatConfig;

// Return codes from syntaqlite_formatter_format().
#define SYNTAQLITE_FORMAT_OK 0
#define SYNTAQLITE_FORMAT_ERROR (-1)

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Free the formatter and all associated resources. No-op if f is NULL.
SYNTAQLITE_API void syntaqlite_formatter_destroy(SyntaqliteFormatter* f);

// ---------------------------------------------------------------------------
// Formatting
// ---------------------------------------------------------------------------

// Format a SQL source string. The source may contain multiple statements.
//
// Returns SYNTAQLITE_FORMAT_OK (0) on success, SYNTAQLITE_FORMAT_ERROR (-1)
// on parse error.
// The source buffer must remain valid only for the duration of this call.
SYNTAQLITE_API int32_t syntaqlite_formatter_format(SyntaqliteFormatter* f,
                                                    const char* source,
                                                    SyntaqliteLength len);

// ---------------------------------------------------------------------------
// Result access (valid until next format() or destroy())
// ---------------------------------------------------------------------------

// Pointer to the NUL-terminated formatted output from the last successful
// format() call. Returns NULL if format() has not been called or failed.
SYNTAQLITE_API const char* syntaqlite_formatter_output(
    const SyntaqliteFormatter* f);

// Byte length of the formatted output (excluding NUL terminator).
// Returns 0 if format() has not been called or failed.
SYNTAQLITE_API SyntaqliteLength syntaqlite_formatter_output_len(
    const SyntaqliteFormatter* f);

// NUL-terminated error message from the last failed format() call.
// Returns NULL if format() has not been called or succeeded.
SYNTAQLITE_API const char* syntaqlite_formatter_error_msg(
    const SyntaqliteFormatter* f);

// ---------------------------------------------------------------------------
// Dialect-generic constructor
// ---------------------------------------------------------------------------

// Create a formatter for any dialect with optional config.
// Pass NULL for config to use defaults.
SYNTAQLITE_API SyntaqliteFormatter* syntaqlite_formatter_create_with_dialect(
    SyntaqliteDialect dialect,
    const SyntaqliteFormatConfig* config);

// ---------------------------------------------------------------------------
// SQLite convenience (opt-out: -DSYNTAQLITE_OMIT_SQLITE_API)
// ---------------------------------------------------------------------------

#ifndef SYNTAQLITE_OMIT_SQLITE_API

// Create a formatter for the built-in SQLite dialect with default config.
SYNTAQLITE_API SyntaqliteFormatter* syntaqlite_formatter_create_sqlite(void);

// Create a formatter for the built-in SQLite dialect with custom config.
SYNTAQLITE_API SyntaqliteFormatter* syntaqlite_formatter_create_sqlite_with_config(
    const SyntaqliteFormatConfig* config);

#endif  // SYNTAQLITE_OMIT_SQLITE_API

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_FORMATTER_H
