// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Analysis C API — validates SQL against a catalog of known
// tables, columns, and functions.
//
// The validator works incrementally: DDL statements (CREATE TABLE, etc.)
// accumulate in the catalog as they are analyzed, so later statements can
// reference earlier definitions.
//
// Lifecycle:
//   SyntaqliteAnalyzer* v = syntaqlite_analyzer_create_sqlite();
//   uint32_t n = syntaqlite_analyzer_analyze(v, sql, len);
//   const SyntaqliteDiagnostic* d = syntaqlite_analyzer_diagnostics(v);
//   for (uint32_t i = 0; i < n; i++) {
//     d[i].severity, d[i].message, d[i].start_offset, d[i].end_offset
//   }
//   syntaqlite_analyzer_destroy(v);
//
// The catalog persists across analyze() calls — each call accumulates DDL
// from the analyzed source. Call syntaqlite_analyzer_reset_catalog() to
// clear accumulated schema.

#ifndef SYNTAQLITE_ANALYSIS_H
#define SYNTAQLITE_ANALYSIS_H

#include <stdint.h>
#include "syntaqlite/config.h"
#include "syntaqlite/dialect.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque validator handle. Owns an Analyzer + Catalog internally.
typedef struct SyntaqliteAnalyzer SyntaqliteAnalyzer;

// Diagnostic severity levels.
typedef enum {
  SYNTAQLITE_SEVERITY_ERROR = 0,
  SYNTAQLITE_SEVERITY_WARNING = 1,
  SYNTAQLITE_SEVERITY_INFO = 2,
  SYNTAQLITE_SEVERITY_HINT = 3,
} SyntaqliteSeverity;

// Severity level for a check category.
//
// Follows the Rust/Clippy convention: ALLOW suppresses the diagnostic,
// WARN emits a warning, DENY emits an error.
typedef enum {
  SYNTAQLITE_CHECK_ALLOW = 0,
  SYNTAQLITE_CHECK_WARN  = 1,
  SYNTAQLITE_CHECK_DENY  = 2,
} SyntaqliteCheckLevel;

// Machine-readable diagnostic kind. Mirrors the Rust DiagnosticMessage
// enum variants so consumers can branch without regexing `message`.
typedef enum {
  SYNTAQLITE_DIAG_PARSE_ERROR              = 0,
  SYNTAQLITE_DIAG_UNKNOWN_TABLE            = 1,
  SYNTAQLITE_DIAG_UNKNOWN_COLUMN           = 2,
  SYNTAQLITE_DIAG_UNKNOWN_FUNCTION         = 3,
  SYNTAQLITE_DIAG_UNKNOWN_MODULE           = 4,
  SYNTAQLITE_DIAG_FUNCTION_ARITY           = 5,
  SYNTAQLITE_DIAG_CTE_COLUMN_COUNT_MISMATCH = 6,
} SyntaqliteDiagnosticCode;

// A single diagnostic from validation. Pointers are valid until the next
// analyze() or destroy() call.
typedef struct {
  SyntaqliteSeverity severity;
  const char* message;
  uint32_t start_offset;
  uint32_t end_offset;
  uint32_t kind_code;  // SyntaqliteDiagnosticCode value
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

// Category for a user-registered function overload.
typedef enum {
  SYNTAQLITE_FUNCTION_SCALAR    = 0,
  SYNTAQLITE_FUNCTION_AGGREGATE = 1,
  SYNTAQLITE_FUNCTION_WINDOW    = 2,
} SyntaqliteFunctionCategory;

// Arity spec kind for a user-registered function overload.
//   EXACT    — accepts exactly `arity_value` arguments.
//   AT_LEAST — accepts `arity_value` or more arguments (variadic).
//   ANY      — accepts any number of arguments; `arity_value` is ignored.
typedef enum {
  SYNTAQLITE_ARITY_EXACT    = 0,
  SYNTAQLITE_ARITY_AT_LEAST = 1,
  SYNTAQLITE_ARITY_ANY      = 2,
} SyntaqliteAritySpecKind;

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
} SyntaqlitePhysicalTableAccess;

// A relation defined by a DDL statement (CREATE TABLE, CREATE VIEW).
typedef struct {
  const char* name;
  uint32_t is_view;   // 0 = table, 1 = view
} SyntaqliteDefinedRelation;

// A view whose body was not available for expansion during lineage
// resolution. Its presence means the statement's lineage result is
// Partial; the name identifies which view body would have been needed.
typedef struct {
  const char* name;
} SyntaqliteUnexpandedView;

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
SYNTAQLITE_API void syntaqlite_analyzer_destroy(SyntaqliteAnalyzer* v);

// Set the analysis mode. See SyntaqliteAnalysisMode for details.
SYNTAQLITE_API void syntaqlite_analyzer_set_mode(SyntaqliteAnalyzer* v,
                                                   SyntaqliteAnalysisMode mode);

// Set the severity level for a check category.
//
// Category names: "parse-errors", "unknown-table", "unknown-column",
// "unknown-function", "function-arity", "cte-columns", plus the groups
// "schema" (schema-related categories) and "all" (every category).
//
// Returns 0 on success, or -1 when `name` is unknown or `level` is out
// of range.
//
// Check-level, strict-schema, and suggestion-threshold settings are
// *sticky*: they survive across add_tables, add_views, load_schema_ddl,
// and analyze calls. Only syntaqlite_analyzer_reset_catalog (or
// destroy) clears them.
SYNTAQLITE_API int32_t syntaqlite_analyzer_set_check_level(
    SyntaqliteAnalyzer* v,
    const char* name,
    SyntaqliteCheckLevel level);

// Toggle strict-schema mode. When non-zero, all schema checks
// (unknown-table, unknown-column, unknown-function, function-arity) are
// promoted to errors. When zero, checks revert to default (warnings).
SYNTAQLITE_API void syntaqlite_analyzer_set_strict_schema(
    SyntaqliteAnalyzer* v,
    uint32_t enabled);

// Set the maximum Levenshtein distance for "did you mean?" suggestions.
// Pass 0 to disable suggestions entirely. Default is 2.
SYNTAQLITE_API void syntaqlite_analyzer_set_suggestion_threshold(
    SyntaqliteAnalyzer* v,
    uint32_t threshold);

// ---------------------------------------------------------------------------
// Module resolution
// ---------------------------------------------------------------------------

// Module resolver callback. Given a NUL-terminated dotted module path
// (e.g. "slices.flow"), return the SQL source text for that module as a
// NUL-terminated malloc-allocated string, or NULL if the module is not
// found. The validator frees the returned string.
typedef char* (*SyntaqliteModuleResolverFn)(const char* module_path,
                                            void* user_data);

// Set a module resolver callback. When the analyzer encounters an import
// statement (e.g. INCLUDE PERFETTO MODULE), it calls resolve_fn to obtain
// the module's SQL source. Pass NULL for resolve_fn to clear the resolver.
SYNTAQLITE_API void syntaqlite_analyzer_set_module_resolver(
    SyntaqliteAnalyzer* v,
    SyntaqliteModuleResolverFn resolve_fn,
    void* user_data);

// ---------------------------------------------------------------------------
// Analysis
// ---------------------------------------------------------------------------

// Analyze a SQL source string. The source may contain multiple statements
// separated by semicolons. DDL statements (CREATE TABLE, etc.) accumulate
// in the internal catalog so that later statements can reference them.
//
// Returns the number of diagnostics produced.
// The source buffer must remain valid only for the duration of this call.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_analyze(SyntaqliteAnalyzer* v,
                                                      const char* source,
                                                      uint32_t len);

// Clear accumulated DDL from the catalog (document + connection layers).
// The dialect layer (built-in functions, etc.) is preserved.
SYNTAQLITE_API void syntaqlite_analyzer_reset_catalog(SyntaqliteAnalyzer* v);

// Add tables to the database layer of the catalog. These tables will be
// visible to all subsequent analyze() calls until reset_catalog() is called.
SYNTAQLITE_API void syntaqlite_analyzer_add_tables(
    SyntaqliteAnalyzer* v,
    const SyntaqliteRelationDef* tables,
    uint32_t count);

// Add views to the database layer of the catalog. Uses the same struct as
// add_tables — name is the view name, columns are output columns.
SYNTAQLITE_API void syntaqlite_analyzer_add_views(
    SyntaqliteAnalyzer* v,
    const SyntaqliteRelationDef* views,
    uint32_t count);

// Load schema from DDL statements (CREATE TABLE, CREATE VIEW, etc.).
// Parses `source` as SQL and accumulates all DDL into the catalog.
// Returns the number of parse errors encountered (0 on success).
SYNTAQLITE_API uint32_t syntaqlite_analyzer_load_schema_ddl(
    SyntaqliteAnalyzer* v,
    const char* source,
    uint32_t len);

// Register a scalar / aggregate / window function overload in the database
// layer. Repeat calls with the same `name` build up an overload set — any
// registered arity will be accepted by the analyzer; calls that match no
// overload produce a FunctionArity diagnostic.
//
// `arity_value` is ignored when `arity_kind` is SYNTAQLITE_ARITY_ANY.
// Out-of-range `category` or `arity_kind` is a no-op.
SYNTAQLITE_API void syntaqlite_analyzer_add_function_overload(
    SyntaqliteAnalyzer* v,
    const char* name,
    SyntaqliteFunctionCategory category,
    SyntaqliteAritySpecKind arity_kind,
    uint32_t arity_value);

// Register a table-valued function (usable in FROM clauses) in the database
// layer.
//
// `arity_value` is ignored when `arity_kind` is SYNTAQLITE_ARITY_ANY.
// `output_columns` may be NULL; when provided, references to the listed
// columns are validated (otherwise any column reference is accepted).
// Out-of-range `arity_kind` is a no-op.
SYNTAQLITE_API void syntaqlite_analyzer_add_table_function(
    SyntaqliteAnalyzer* v,
    const char* name,
    SyntaqliteAritySpecKind arity_kind,
    uint32_t arity_value,
    const char* const* output_columns,
    uint32_t output_column_count);

// ---------------------------------------------------------------------------
// Diagnostic access (valid until next analyze() or destroy())
// ---------------------------------------------------------------------------

// Number of diagnostics from the last analyze() call.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_diagnostic_count(
    const SyntaqliteAnalyzer* v);

// Pointer to the diagnostic array from the last analyze() call.
// Returns NULL when diagnostic_count is 0.
SYNTAQLITE_API const SyntaqliteDiagnostic* syntaqlite_analyzer_diagnostics(
    const SyntaqliteAnalyzer* v);

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
SYNTAQLITE_API const char* syntaqlite_analyzer_render_diagnostics(
    SyntaqliteAnalyzer* v,
    const char* file);

// ---------------------------------------------------------------------------
// Lineage access (valid until next analyze() or destroy())
// ---------------------------------------------------------------------------

// Whether lineage was fully resolved (1) or partially resolved (0).
// Returns 0 if the last analyzed statement was not a query.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_lineage_complete(
    const SyntaqliteAnalyzer* v);

// Number of result columns with lineage information.
// Returns 0 if the last analyzed statement was not a query.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_column_lineage_count(
    const SyntaqliteAnalyzer* v);

// Pointer to the column lineage array from the last analyze() call.
// Returns NULL when column_lineage_count is 0.
SYNTAQLITE_API const SyntaqliteColumnLineage* syntaqlite_analyzer_column_lineage(
    const SyntaqliteAnalyzer* v);

// Number of relations (tables/views) directly referenced in FROM clauses.
// Returns 0 if the last analyzed statement was not a query.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_relation_count(
    const SyntaqliteAnalyzer* v);

// Pointer to the relation access array from the last analyze() call.
// Returns NULL when relation_count is 0.
SYNTAQLITE_API const SyntaqliteRelationAccess* syntaqlite_analyzer_relations(
    const SyntaqliteAnalyzer* v);

// Number of physical tables accessed (after resolving CTEs/views).
// Returns 0 if the last analyzed statement was not a query.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_physical_table_count(
    const SyntaqliteAnalyzer* v);

// Pointer to the table access array from the last analyze() call.
// Returns NULL when table_count is 0.
SYNTAQLITE_API const SyntaqlitePhysicalTableAccess* syntaqlite_analyzer_physical_tables(
    const SyntaqliteAnalyzer* v);

// Number of views whose bodies were not available for expansion during
// lineage resolution across all statements. A non-zero count means at
// least one statement's lineage is Partial.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_unexpanded_view_count(
    const SyntaqliteAnalyzer* v);

// Pointer to the unexpanded views array from the last analyze() call.
// Returns NULL when the count is 0.
SYNTAQLITE_API const SyntaqliteUnexpandedView* syntaqlite_analyzer_unexpanded_views(
    const SyntaqliteAnalyzer* v);

// ---------------------------------------------------------------------------
// Per-statement access (valid until next analyze() or destroy())
// ---------------------------------------------------------------------------

// Number of statements produced by the last analyze() call.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_statement_count(
    SyntaqliteAnalyzer* v);

// Source text for statement `idx`. NULL when idx is out of bounds.
// The returned string is valid until the next analyze() or destroy() call.
SYNTAQLITE_API const char* syntaqlite_analyzer_statement_source(
    SyntaqliteAnalyzer* v, uint32_t idx);

// Number of diagnostics for statement at index `idx`.
// Returns 0 if idx is out of bounds.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_statement_diagnostic_count(
    SyntaqliteAnalyzer* v, uint32_t idx);

// Pointer to diagnostics for statement `idx`. NULL when count is 0 or
// idx is out of bounds.
SYNTAQLITE_API const SyntaqliteDiagnostic*
syntaqlite_analyzer_statement_diagnostics(
    SyntaqliteAnalyzer* v, uint32_t idx);

// Number of result columns with lineage for statement `idx`.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_statement_column_lineage_count(
    SyntaqliteAnalyzer* v, uint32_t idx);

// Column lineage array for statement `idx`. NULL when count is 0.
SYNTAQLITE_API const SyntaqliteColumnLineage*
syntaqlite_analyzer_statement_column_lineage(
    SyntaqliteAnalyzer* v, uint32_t idx);

// Number of relations referenced in FROM for statement `idx`.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_statement_relation_count(
    SyntaqliteAnalyzer* v, uint32_t idx);

// Relation access array for statement `idx`. NULL when count is 0.
SYNTAQLITE_API const SyntaqliteRelationAccess*
syntaqlite_analyzer_statement_relations(
    SyntaqliteAnalyzer* v, uint32_t idx);

// Number of physical tables accessed for statement `idx`.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_statement_physical_table_count(
    SyntaqliteAnalyzer* v, uint32_t idx);

// Physical table access array for statement `idx`. NULL when count is 0.
SYNTAQLITE_API const SyntaqlitePhysicalTableAccess*
syntaqlite_analyzer_statement_physical_tables(
    SyntaqliteAnalyzer* v, uint32_t idx);

// Number of relations defined by DDL in statement `idx`.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_statement_defined_relation_count(
    SyntaqliteAnalyzer* v, uint32_t idx);

// Defined relations for statement `idx`. NULL when count is 0.
SYNTAQLITE_API const SyntaqliteDefinedRelation*
syntaqlite_analyzer_statement_defined_relations(
    SyntaqliteAnalyzer* v, uint32_t idx);

// Number of views referenced in statement `idx` whose bodies were not
// available for expansion during lineage resolution. A non-zero count
// means the statement's lineage is Partial.
SYNTAQLITE_API uint32_t syntaqlite_analyzer_statement_unexpanded_view_count(
    SyntaqliteAnalyzer* v, uint32_t idx);

// Unexpanded views for statement `idx`. NULL when count is 0.
SYNTAQLITE_API const SyntaqliteUnexpandedView*
syntaqlite_analyzer_statement_unexpanded_views(
    SyntaqliteAnalyzer* v, uint32_t idx);

// Free a string returned by a syntaqlite_* function that documents
// ownership transfer. No-op if s is NULL.
SYNTAQLITE_API void syntaqlite_string_destroy(char* s);

// ---------------------------------------------------------------------------
// Dialect-generic constructor
// ---------------------------------------------------------------------------

// Create a validator for any dialect.
// The default analysis mode is SYNTAQLITE_MODE_DOCUMENT.
SYNTAQLITE_API SyntaqliteAnalyzer* syntaqlite_analyzer_create_with_dialect(
    SyntaqliteDialect dialect);

// ---------------------------------------------------------------------------
// SQLite convenience (opt-out: -DSYNTAQLITE_OMIT_SQLITE_API)
// ---------------------------------------------------------------------------

#ifndef SYNTAQLITE_OMIT_SQLITE_API

// Create a validator for the built-in SQLite dialect.
// The default analysis mode is SYNTAQLITE_MODE_DOCUMENT.
SYNTAQLITE_API SyntaqliteAnalyzer* syntaqlite_analyzer_create_sqlite(void);

#endif  // SYNTAQLITE_OMIT_SQLITE_API

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_ANALYSIS_H
