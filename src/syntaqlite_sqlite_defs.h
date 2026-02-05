/*
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
** SQLite compatibility definitions for syntaqlite.
** Provides type aliases, macros, and structures needed by tokenizer and parser.
*/
#ifndef SYNTAQLITE_SRC_SQLITE_DEFS_H
#define SYNTAQLITE_SRC_SQLITE_DEFS_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* SQLite type aliases */
typedef int64_t i64;
typedef uint8_t u8;
typedef uint32_t u32;

/* ASCII mode - disables EBCDIC support */
#define SQLITE_ASCII 1

/* Digit separator for numeric literals (e.g., 1_000_000) */
#define SQLITE_DIGIT_SEPARATOR '_'

/* C++17 fallthrough, C no-op */
#ifdef __cplusplus
#define deliberate_fall_through [[fallthrough]]
#else
#define deliberate_fall_through
#endif

/*
** Token structure representing a single token from the input.
** This is the minor type for terminal symbols in the parser.
*/
typedef struct SyntaqliteToken {
  const char *z;  /* Text of the token */
  int n;          /* Length of the token */
  int type;       /* Token type (TK_* constant) */
} SyntaqliteToken;

/*
** Forward declarations for AST types.
*/
struct SyntaqliteAst;
struct SyntaqliteAstContext;

/*
** Parse context passed to the parser.
** Users can set callbacks for error handling.
*/
typedef struct SyntaqliteParseContext SyntaqliteParseContext;
struct SyntaqliteParseContext {
  void *userData;                                  /* User-defined data */
  void (*onSyntaxError)(SyntaqliteParseContext *); /* Syntax error callback */
  void (*onStackOverflow)(SyntaqliteParseContext *); /* Stack overflow callback */
  struct SyntaqliteAstContext *astCtx;             /* AST building context */
  const char *zSql;                                /* Original SQL text */
  u32 root;                                        /* Root AST node ID */
};

/*
** Alternative datatype for malloc argument.
*/
#define YYMALLOCARGTYPE size_t

/*
** Unused parameter macro.
*/
#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x) (void)(x)
#endif

#endif /* SYNTAQLITE_SRC_SQLITE_DEFS_H */
