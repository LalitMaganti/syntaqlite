// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Semantic validation C API — validates SQL against a catalog of known
// tables, columns, and functions.
//
// The validator works incrementally: DDL statements (CREATE TABLE, etc.)
// accumulate in the catalog as they are analyzed, so later statements can
// reference earlier definitions.
//
// Lifecycle:
//   SyntaqliteValidator* v = syntaqlite_validator_create(grammar);
//   syntaqlite_validator_analyze(v, sql, len);
//   uint32_t n = syntaqlite_validator_diagnostic_count(v);
//   for (uint32_t i = 0; i < n; i++) {
//     SyntaqliteSeverity sev = syntaqlite_diagnostic_severity(v, i);
//     const char* msg = syntaqlite_diagnostic_message(v, i);
//     ...
//   }
//   syntaqlite_validator_destroy(v);
//
// The catalog persists across analyze() calls — each call accumulates DDL
// from the analyzed source. Call syntaqlite_validator_reset_catalog() to
// clear accumulated schema.

#ifndef SYNTAQLITE_VALIDATION_H
#define SYNTAQLITE_VALIDATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque validator handle. Owns a SemanticAnalyzer + Catalog internally.
typedef struct SyntaqliteValidator SyntaqliteValidator;

// Diagnostic severity levels.
typedef enum {
  SYNTAQLITE_SEVERITY_ERROR = 0,
  SYNTAQLITE_SEVERITY_WARNING = 1,
  SYNTAQLITE_SEVERITY_INFO = 2,
  SYNTAQLITE_SEVERITY_HINT = 3,
} SyntaqliteSeverity;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Create a validator for the built-in SQLite dialect.
SyntaqliteValidator* syntaqlite_validator_create_sqlite(void);

// Free the validator and all associated resources. No-op if v is NULL.
void syntaqlite_validator_destroy(SyntaqliteValidator* v);

// ---------------------------------------------------------------------------
// Analysis
// ---------------------------------------------------------------------------

// Analyze a SQL source string. The source may contain multiple statements
// separated by semicolons. DDL statements (CREATE TABLE, etc.) accumulate
// in the internal catalog so that later statements can reference them.
//
// Returns the number of diagnostics produced.
// The source buffer must remain valid only for the duration of this call.
uint32_t syntaqlite_validator_analyze(SyntaqliteValidator* v,
                                      const char* source,
                                      uint32_t len);

// Clear accumulated DDL from the catalog (document + connection layers).
// The dialect layer (built-in functions, etc.) is preserved.
void syntaqlite_validator_reset_catalog(SyntaqliteValidator* v);

// Add a table to the database layer of the catalog. This table will be
// visible to all subsequent analyze() calls until reset_catalog() is called.
// column_names may be NULL (table exists but columns are unknown).
// column_count is ignored when column_names is NULL.
void syntaqlite_validator_add_table(SyntaqliteValidator* v,
                                    const char* table_name,
                                    const char* const* column_names,
                                    uint32_t column_count);

// ---------------------------------------------------------------------------
// Diagnostic access (valid until next analyze() or destroy())
// ---------------------------------------------------------------------------

// Number of diagnostics from the last analyze() call.
uint32_t syntaqlite_validator_diagnostic_count(
    const SyntaqliteValidator* v);

// Severity of the i-th diagnostic.
SyntaqliteSeverity syntaqlite_diagnostic_severity(
    const SyntaqliteValidator* v,
    uint32_t index);

// Human-readable message for the i-th diagnostic. The returned pointer is
// valid until the next analyze() or destroy() call.
const char* syntaqlite_diagnostic_message(const SyntaqliteValidator* v,
                                          uint32_t index);

// Byte offset of the start of the i-th diagnostic's source range.
uint32_t syntaqlite_diagnostic_start_offset(const SyntaqliteValidator* v,
                                            uint32_t index);

// Byte offset of the end of the i-th diagnostic's source range.
uint32_t syntaqlite_diagnostic_end_offset(const SyntaqliteValidator* v,
                                          uint32_t index);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_VALIDATION_H
