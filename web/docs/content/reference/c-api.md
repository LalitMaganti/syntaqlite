+++
title = "C API reference"
description = "FFI function signatures, types, and memory model."
weight = 5
+++

# C API reference

## Distribution

syntaqlite's C API is available in two forms:

| Package | Contents | Header |
|---------|----------|--------|
| `syntaqlite-syntax-amalgamation` | Parser and tokenizer as compilable C source (`.c` + `.h`) | `syntaqlite_syntax.h` |
| `syntaqlite-clib` | Prebuilt shared library (parser + formatter + validator) for all platforms | `syntaqlite.h` |

Both are attached to each
[GitHub release](https://github.com/LalitMaganti/syntaqlite/releases).
See [Using syntaqlite from C](@/guides/c-api.md) for compilation
instructions.

## Parser

### Types

```c
// Opaque parser handle.
typedef struct SyntaqliteParser SyntaqliteParser;

// Allocator override. Pass NULL for system malloc/free.
typedef struct SyntaqliteMemMethods {
  void* (*xMalloc)(size_t);
  void* (*xRealloc)(void*, size_t);
  void (*xFree)(void*);
} SyntaqliteMemMethods;

// Source span (byte offset + length).
typedef struct { uint32_t offset; uint32_t length; } SyntaqliteSpan;

// Comment descriptor.
typedef struct {
  SyntaqliteSpan span;
  uint32_t is_block;  // 0 = line comment, 1 = block comment
} SyntaqliteComment;

// Token from the token side-table.
typedef struct {
  uint32_t token_type;
  SyntaqliteSpan span;
} SyntaqliteParserToken;
```

### Functions

| Function | Description |
|----------|-------------|
| `syntaqlite_parser_create(mem)` | Create a parser for the built-in SQLite dialect. `mem` may be `NULL` |
| `syntaqlite_parser_create_with_grammar(mem, grammar)` | Create with a custom grammar |
| `syntaqlite_parser_reset(p, source, len)` | Set source text for parsing |
| `syntaqlite_parser_next(p)` | Parse the next statement. Returns `0` on success, `1` on error, `-1` when done |
| `syntaqlite_parser_destroy(p)` | Free the parser. No-op if `NULL` |

### Result access

| Function | Description |
|----------|-------------|
| `syntaqlite_result_root(p)` | Root node ID of the last parsed statement |
| `syntaqlite_result_recovery_root(p)` | Root node ID of error-recovery parse (valid after error) |
| `syntaqlite_result_error_msg(p)` | NUL-terminated error message, or `NULL` on success |
| `syntaqlite_result_error_offset(p)` | Byte offset of the error in source |
| `syntaqlite_result_error_length(p)` | Length of the error token |
| `syntaqlite_result_comments(p, &count)` | Array of `SyntaqliteComment`, sets `count` |
| `syntaqlite_result_tokens(p, &count)` | Array of `SyntaqliteParserToken`, sets `count` |

### Arena access

| Function | Description |
|----------|-------------|
| `syntaqlite_parser_node(p, node_id)` | Pointer to node data in the arena |
| `syntaqlite_parser_text(p)` | Pointer to the source text |
| `syntaqlite_parser_text_length(p)` | Length of the source text |
| `syntaqlite_parser_node_count(p)` | Number of nodes in the arena |
| `syntaqlite_dump_node(p, node_id, indent)` | Pretty-print a node subtree. **Caller must `free()` the result** |

### Configuration

| Function | Description |
|----------|-------------|
| `syntaqlite_parser_set_collect_tokens(p, enable)` | Enable token side-table collection (off by default) |
| `syntaqlite_parser_set_trace(p, enable)` | Enable parser trace output (debug) |
| `syntaqlite_parser_set_macro_fallback(p, enable)` | Enable macro fallback mode |

## Tokenizer

### Functions

| Function | Description |
|----------|-------------|
| `syntaqlite_tokenizer_create(mem)` | Create a tokenizer for the built-in SQLite dialect. `mem` may be `NULL` |
| `syntaqlite_tokenizer_create_with_grammar(mem, grammar)` | Create with a custom grammar |
| `syntaqlite_tokenizer_reset(tok, source, len)` | Set source text for tokenizing |
| `syntaqlite_tokenizer_next(tok, &out)` | Read the next token into `out`. Returns the token type, `0` for EOF |
| `syntaqlite_tokenizer_destroy(tok)` | Free the tokenizer. No-op if `NULL` |

## Formatter

### Types

```c
// Opaque formatter handle.
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
```

Return codes from `syntaqlite_formatter_format()`:

| Constant | Value | Meaning |
|----------|-------|---------|
| `SYNTAQLITE_FORMAT_OK` | `0` | Success |
| `SYNTAQLITE_FORMAT_ERROR` | `-1` | Parse error |

### Functions

| Function | Description |
|----------|-------------|
| `syntaqlite_formatter_create_sqlite()` | Create with default config |
| `syntaqlite_formatter_create_sqlite_with_config(config)` | Create with custom `SyntaqliteFormatConfig` |
| `syntaqlite_formatter_format(f, sql, len)` | Format SQL source (`len` bytes). Returns `0` on success, `-1` on error |
| `syntaqlite_formatter_output(f)` | NUL-terminated formatted output, or `NULL` after error |
| `syntaqlite_formatter_output_len(f)` | Length of formatted output in bytes (excl. NUL) |
| `syntaqlite_formatter_error_msg(f)` | NUL-terminated error message, or `NULL` after success |
| `syntaqlite_formatter_destroy(f)` | Free the formatter. No-op if `NULL` |

## Validator

### Types

```c
// Opaque validator handle.
typedef struct SyntaqliteValidator SyntaqliteValidator;

// Diagnostic severity levels.
typedef enum {
  SYNTAQLITE_SEVERITY_ERROR   = 0,
  SYNTAQLITE_SEVERITY_WARNING = 1,
  SYNTAQLITE_SEVERITY_INFO    = 2,
  SYNTAQLITE_SEVERITY_HINT    = 3,
} SyntaqliteSeverity;

// A single diagnostic from validation.
typedef struct {
  SyntaqliteSeverity severity;
  const char* message;       // NUL-terminated, borrowed
  uint32_t start_offset;     // byte offset in source
  uint32_t end_offset;       // byte offset in source
} SyntaqliteDiagnostic;

// Relation definition for batch catalog registration (tables and views).
typedef struct {
  const char* name;              // NUL-terminated table name
  const char* const* columns;    // NULL = columns unknown (accepts any ref)
  uint32_t column_count;         // ignored when columns is NULL
} SyntaqliteRelationDef;

// Analysis mode.
typedef enum {
  SYNTAQLITE_MODE_DOCUMENT = 0,  // DDL resets between analyze() calls
  SYNTAQLITE_MODE_EXECUTE  = 1,  // DDL accumulates across calls
} SyntaqliteAnalysisMode;

// Relation kind.
typedef enum {
  SYNTAQLITE_RELATION_TABLE = 0,
  SYNTAQLITE_RELATION_VIEW  = 1,
} SyntaqliteRelationKind;

// Origin of a result column (NULL when origin unknown).
typedef struct {
  const char* table;
  const char* column;
} SyntaqliteColumnOrigin;

// Lineage for a single result column.
typedef struct {
  const char* name;               // output column name
  uint32_t index;                 // zero-based position
  SyntaqliteColumnOrigin origin;  // NULL fields when no single-column origin
} SyntaqliteColumnLineage;

// A catalog relation referenced in FROM.
typedef struct {
  const char* name;
  SyntaqliteRelationKind kind;
} SyntaqliteRelationAccess;

// A physical table accessed by the query.
typedef struct {
  const char* name;
} SyntaqliteTableAccess;
```

### Functions

| Function | Description |
|----------|-------------|
| `syntaqlite_validator_create_sqlite()` | Create a validator with default mode (`DOCUMENT`) |
| `syntaqlite_validator_set_mode(v, mode)` | Set `DOCUMENT` or `EXECUTE` mode |
| `syntaqlite_validator_add_tables(v, tables, count)` | Register schema tables from `SyntaqliteRelationDef` array |
| `syntaqlite_validator_add_views(v, views, count)` | Register schema views from `SyntaqliteRelationDef` array |
| `syntaqlite_validator_load_schema_ddl(v, sql, len)` | Load schema from DDL statements (CREATE TABLE/VIEW). Returns parse error count |
| `syntaqlite_validator_analyze(v, sql, len)` | Analyze SQL, returns diagnostic count |
| `syntaqlite_validator_diagnostic_count(v)` | Number of diagnostics from last `analyze()` |
| `syntaqlite_validator_diagnostics(v)` | Pointer to `SyntaqliteDiagnostic` array, or `NULL` if count is 0 |
| `syntaqlite_validator_render_diagnostics(v, filename)` | Render diagnostics as human-readable text. `filename` may be `NULL` (defaults to `"<input>"`) |
| `syntaqlite_validator_lineage_complete(v)` | Whether lineage is fully resolved (`1`) or partial (`0`). Returns `0` for non-queries |
| `syntaqlite_validator_column_lineage_count(v)` | Number of result columns with lineage. `0` for non-queries |
| `syntaqlite_validator_column_lineage(v)` | Pointer to `SyntaqliteColumnLineage` array, or `NULL` |
| `syntaqlite_validator_relation_count(v)` | Number of relations in FROM clauses. `0` for non-queries |
| `syntaqlite_validator_relations(v)` | Pointer to `SyntaqliteRelationAccess` array, or `NULL` |
| `syntaqlite_validator_table_count(v)` | Number of physical tables accessed. `0` for non-queries |
| `syntaqlite_validator_tables(v)` | Pointer to `SyntaqliteTableAccess` array, or `NULL` |
| `syntaqlite_validator_reset_catalog(v)` | Clear registered schema (preserves dialect builtins) |
| `syntaqlite_validator_destroy(v)` | Free the validator. No-op if `NULL` |

## Utility

| Function | Description |
|----------|-------------|
| `syntaqlite_string_destroy(s)` | Free a string returned by a `syntaqlite_*` function that documents ownership transfer. No-op if `NULL` |

## Analysis modes

| Mode | Value | Behavior |
|------|-------|----------|
| `SYNTAQLITE_MODE_DOCUMENT` | 0 | DDL resets between `analyze()` calls (for file editing) |
| `SYNTAQLITE_MODE_EXECUTE` | 1 | DDL accumulates across calls (for interactive sessions) |

## Memory model

- Parser, formatter, and validator handles are **reusable**: create once, call
  repeatedly.
- Output strings from `syntaqlite_formatter_output()`,
  `syntaqlite_validator_diagnostics()`, and lineage accessors
  (`syntaqlite_validator_column_lineage()`, `syntaqlite_validator_relations()`,
  `syntaqlite_validator_tables()`) are **borrowed**, valid until the next
  call to analyze or destroy.
- Strings from `syntaqlite_validator_render_diagnostics()` are **borrowed**,
  valid until the next `analyze()`, `render_diagnostics()`, or `destroy()` call.
- Strings from `syntaqlite_dump_node()` are **owned**; caller must `free()`.
- Passing `NULL` to any destroy function is a safe no-op.
