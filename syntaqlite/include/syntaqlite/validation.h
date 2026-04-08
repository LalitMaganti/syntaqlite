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
//   SyntaqliteValidator* v = syntaqlite_validator_create_sqlite();
//   uint32_t n = syntaqlite_validator_analyze(v, sql, len);
//   const SyntaqliteDiagnostic* d = syntaqlite_validator_diagnostics(v);
//   for (uint32_t i = 0; i < n; i++) {
//     d[i].severity, d[i].message, d[i].start_offset, d[i].end_offset
//   }
//   syntaqlite_validator_destroy(v);
//
// The catalog persists across analyze() calls — each call accumulates DDL
// from the analyzed source. Call syntaqlite_validator_reset_catalog() to
// clear accumulated schema.

#ifndef SYNTAQLITE_VALIDATION_H
#define SYNTAQLITE_VALIDATION_H

#include <stdint.h>
#include "syntaqlite/config.h"
#include "syntaqlite/dialect.h"

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

// A single diagnostic from validation. Pointers are valid until the next
// analyze() or destroy() call.
typedef struct {
  SyntaqliteSeverity severity;
  const char* message;
  uint32_t start_offset;
  uint32_t end_offset;
} SyntaqliteDiagnostic;

// Relation definition for batch catalog registration (tables and views).
typedef struct {
  const char* name;
  const char* const* columns;  // NULL = columns unknown
  uint32_t column_count;       // ignored when columns is NULL
} SyntaqliteRelationDef;

// Relation kind (table vs. view).
typedef enum {
  SYNTAQLITE_RELATION_TABLE = 0,
  SYNTAQLITE_RELATION_VIEW  = 1,
} SyntaqliteRelationKind;

// Origin of a result column — which table.column it traces back to.
// Both fields are NULL when the column is an expression, literal, or
// aggregate with no single-column origin.
typedef struct {
  const char* table;   // NULL when origin unknown
  const char* column;  // NULL when origin unknown
} SyntaqliteColumnOrigin;

// Lineage information for a single result column.
typedef struct {
  const char* name;             // output column name (alias or inferred)
  uint32_t index;               // zero-based position in result column list
  SyntaqliteColumnOrigin origin;
} SyntaqliteColumnLineage;

// A catalog relation (table or view) referenced in a FROM clause.
typedef struct {
  const char* name;
  SyntaqliteRelationKind kind;
} SyntaqliteRelationAccess;

// A physical table accessed by the query.
typedef struct {
  const char* name;
} SyntaqliteTableAccess;

// A relation defined by a DDL statement (CREATE TABLE, CREATE VIEW).
typedef struct {
  const char* name;
  uint32_t is_view;   // 0 = table, 1 = view
} SyntaqliteDefinedRelation;

// Analysis mode — controls whether DDL persists across analyze() calls.
typedef enum {
  // Statements are being analyzed (e.g. editing a SQL file).
  // DDL resets between analyze() calls.
  SYNTAQLITE_MODE_DOCUMENT = 0,
  // Statements are being executed sequentially.
  // DDL accumulates across analyze() calls.
  SYNTAQLITE_MODE_EXECUTE = 1,
} SyntaqliteAnalysisMode;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Free the validator and all associated resources. No-op if v is NULL.
SYNTAQLITE_API void syntaqlite_validator_destroy(SyntaqliteValidator* v);

// Set the analysis mode. See SyntaqliteAnalysisMode for details.
SYNTAQLITE_API void syntaqlite_validator_set_mode(SyntaqliteValidator* v,
                                                   SyntaqliteAnalysisMode mode);

// ---------------------------------------------------------------------------
// Analysis
// ---------------------------------------------------------------------------

// Analyze a SQL source string. The source may contain multiple statements
// separated by semicolons. DDL statements (CREATE TABLE, etc.) accumulate
// in the internal catalog so that later statements can reference them.
//
// Returns the number of diagnostics produced.
// The source buffer must remain valid only for the duration of this call.
SYNTAQLITE_API uint32_t syntaqlite_validator_analyze(SyntaqliteValidator* v,
                                                      const char* source,
                                                      uint32_t len);

// Clear accumulated DDL from the catalog (document + connection layers).
// The dialect layer (built-in functions, etc.) is preserved.
SYNTAQLITE_API void syntaqlite_validator_reset_catalog(SyntaqliteValidator* v);

// Add tables to the database layer of the catalog. These tables will be
// visible to all subsequent analyze() calls until reset_catalog() is called.
SYNTAQLITE_API void syntaqlite_validator_add_tables(
    SyntaqliteValidator* v,
    const SyntaqliteRelationDef* tables,
    uint32_t count);

// Add views to the database layer of the catalog. Uses the same struct as
// add_tables — name is the view name, columns are output columns.
SYNTAQLITE_API void syntaqlite_validator_add_views(
    SyntaqliteValidator* v,
    const SyntaqliteRelationDef* views,
    uint32_t count);

// Load schema from DDL statements (CREATE TABLE, CREATE VIEW, etc.).
// Parses `source` as SQL and accumulates all DDL into the catalog.
// Returns the number of parse errors encountered (0 on success).
SYNTAQLITE_API uint32_t syntaqlite_validator_load_schema_ddl(
    SyntaqliteValidator* v,
    const char* source,
    uint32_t len);

// ---------------------------------------------------------------------------
// Diagnostic access (valid until next analyze() or destroy())
// ---------------------------------------------------------------------------

// Number of diagnostics from the last analyze() call.
SYNTAQLITE_API uint32_t syntaqlite_validator_diagnostic_count(
    const SyntaqliteValidator* v);

// Pointer to the diagnostic array from the last analyze() call.
// Returns NULL when diagnostic_count is 0.
SYNTAQLITE_API const SyntaqliteDiagnostic* syntaqlite_validator_diagnostics(
    const SyntaqliteValidator* v);

// ---------------------------------------------------------------------------
// Diagnostic rendering
// ---------------------------------------------------------------------------

// Render all diagnostics from the last analyze() call as a rustc-style
// human-readable string. Example output:
//
//   error: unknown table 'usr'
//    --> query.sql:1:15
//     |
//   1 | SELECT id FROM usr WHERE id = 1
//     |               ^~~
//     = help: did you mean 'users'?
//
// `file` is a NUL-terminated label for the "--> file:line:col" header.
// Pass NULL to use the default label "<input>".
//
// Returns a NUL-terminated UTF-8 string. The pointer is valid until the
// next analyze(), render_diagnostics(), or destroy() call.
// Returns an empty string when there are no diagnostics.
SYNTAQLITE_API const char* syntaqlite_validator_render_diagnostics(
    SyntaqliteValidator* v,
    const char* file);

// ---------------------------------------------------------------------------
// Lineage access (valid until next analyze() or destroy())
// ---------------------------------------------------------------------------

// Whether lineage was fully resolved (1) or partially resolved (0).
// Returns 0 if the last analyzed statement was not a query.
SYNTAQLITE_API uint32_t syntaqlite_validator_lineage_complete(
    const SyntaqliteValidator* v);

// Number of result columns with lineage information.
// Returns 0 if the last analyzed statement was not a query.
SYNTAQLITE_API uint32_t syntaqlite_validator_column_lineage_count(
    const SyntaqliteValidator* v);

// Pointer to the column lineage array from the last analyze() call.
// Returns NULL when column_lineage_count is 0.
SYNTAQLITE_API const SyntaqliteColumnLineage* syntaqlite_validator_column_lineage(
    const SyntaqliteValidator* v);

// Number of relations (tables/views) directly referenced in FROM clauses.
// Returns 0 if the last analyzed statement was not a query.
SYNTAQLITE_API uint32_t syntaqlite_validator_relation_count(
    const SyntaqliteValidator* v);

// Pointer to the relation access array from the last analyze() call.
// Returns NULL when relation_count is 0.
SYNTAQLITE_API const SyntaqliteRelationAccess* syntaqlite_validator_relations(
    const SyntaqliteValidator* v);

// Number of physical tables accessed (after resolving CTEs/views).
// Returns 0 if the last analyzed statement was not a query.
SYNTAQLITE_API uint32_t syntaqlite_validator_table_count(
    const SyntaqliteValidator* v);

// Pointer to the table access array from the last analyze() call.
// Returns NULL when table_count is 0.
SYNTAQLITE_API const SyntaqliteTableAccess* syntaqlite_validator_tables(
    const SyntaqliteValidator* v);

// ---------------------------------------------------------------------------
// Per-statement access (valid until next analyze() or destroy())
// ---------------------------------------------------------------------------

// Number of statements produced by the last analyze() call.
SYNTAQLITE_API uint32_t syntaqlite_validator_statement_count(
    SyntaqliteValidator* v);

// Number of diagnostics for statement at index `idx`.
// Returns 0 if idx is out of bounds.
SYNTAQLITE_API uint32_t syntaqlite_validator_statement_diagnostic_count(
    SyntaqliteValidator* v, uint32_t idx);

// Pointer to diagnostics for statement `idx`. NULL when count is 0 or
// idx is out of bounds.
SYNTAQLITE_API const SyntaqliteDiagnostic*
syntaqlite_validator_statement_diagnostics(
    SyntaqliteValidator* v, uint32_t idx);

// Number of result columns with lineage for statement `idx`.
SYNTAQLITE_API uint32_t syntaqlite_validator_statement_column_lineage_count(
    SyntaqliteValidator* v, uint32_t idx);

// Column lineage array for statement `idx`. NULL when count is 0.
SYNTAQLITE_API const SyntaqliteColumnLineage*
syntaqlite_validator_statement_column_lineage(
    SyntaqliteValidator* v, uint32_t idx);

// Number of relations referenced in FROM for statement `idx`.
SYNTAQLITE_API uint32_t syntaqlite_validator_statement_relation_count(
    SyntaqliteValidator* v, uint32_t idx);

// Relation access array for statement `idx`. NULL when count is 0.
SYNTAQLITE_API const SyntaqliteRelationAccess*
syntaqlite_validator_statement_relations(
    SyntaqliteValidator* v, uint32_t idx);

// Number of physical tables accessed for statement `idx`.
SYNTAQLITE_API uint32_t syntaqlite_validator_statement_table_count(
    SyntaqliteValidator* v, uint32_t idx);

// Physical table access array for statement `idx`. NULL when count is 0.
SYNTAQLITE_API const SyntaqliteTableAccess*
syntaqlite_validator_statement_tables(
    SyntaqliteValidator* v, uint32_t idx);

// Number of relations defined by DDL in statement `idx`.
SYNTAQLITE_API uint32_t syntaqlite_validator_statement_defined_relation_count(
    SyntaqliteValidator* v, uint32_t idx);

// Defined relations for statement `idx`. NULL when count is 0.
SYNTAQLITE_API const SyntaqliteDefinedRelation*
syntaqlite_validator_statement_defined_relations(
    SyntaqliteValidator* v, uint32_t idx);

// Free a string returned by a syntaqlite_* function that documents
// ownership transfer. No-op if s is NULL.
SYNTAQLITE_API void syntaqlite_string_destroy(char* s);

// ---------------------------------------------------------------------------
// Dialect-generic constructor
// ---------------------------------------------------------------------------

// Create a validator for any dialect.
// The default analysis mode is SYNTAQLITE_MODE_DOCUMENT.
SYNTAQLITE_API SyntaqliteValidator* syntaqlite_validator_create_with_dialect(
    SyntaqliteDialect dialect);

// ---------------------------------------------------------------------------
// SQLite convenience (opt-out: -DSYNTAQLITE_OMIT_SQLITE_API)
// ---------------------------------------------------------------------------

#ifndef SYNTAQLITE_OMIT_SQLITE_API

// Create a validator for the built-in SQLite dialect.
// The default analysis mode is SYNTAQLITE_MODE_DOCUMENT.
SYNTAQLITE_API SyntaqliteValidator* syntaqlite_validator_create_sqlite(void);

#endif  // SYNTAQLITE_OMIT_SQLITE_API

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_VALIDATION_H
