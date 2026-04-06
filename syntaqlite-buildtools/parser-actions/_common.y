// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// AST building actions for syntaqlite grammar.
// These rules get merged with SQLite's parse.y during code generation.
//
// Rule signatures MUST match upstream parse.y exactly.
// Python tooling validates coverage and consistency.
//
// Conventions:
// - pCtx: Parse context (SynqParseCtx*), threaded via %extra_argument
// - pCtx->root: Set to root node ID at input rule
// - Terminals are SynqParseToken with .z (pointer), .n (length), .type (token ID)
// - Non-terminals default to uint32_t (node IDs)
// - synq_span(pCtx, tok) converts a SynqParseToken into SyntaqliteSourceSpan
// - SYNQ_NO_SPAN is the zero sentinel span

%token_prefix SYNTAQLITE_TK_
%start_symbol input
%extra_context {SynqParseCtx* pCtx}
%realloc synq_stack_realloc
%free    synq_stack_free

%include {
#include <string.h>
#include <limits.h>

#include "syntaqlite_dialect/sqlite_compat.h"
#include "syntaqlite_dialect/ast_builder.h"
#include "syntaqlite_dialect/dialect_macros.h"
#include "syntaqlite/types.h"
#include "@DIALECT_BUILDER_H@"

// Parser stack realloc/free macros. These expand at the Lemon call site
// where the parser struct is in scope, routing through pCtx->mem.
// YYREALLOC is called in yyGrowStack (parser variable: p).
// YYFREE is called in ParseFinalize (parser variable: pParser).
#define synq_stack_realloc(ptr, sz) (p->pCtx->mem.xRealloc((ptr), (sz)))
#define synq_stack_free(ptr)        (pParser->pCtx->mem.xFree((ptr)))

/* BEGIN GRAMMAR_TYPES */
// Grammar-specific struct types for multi-valued grammar nonterminals.
// These are used by Lemon-generated parser actions to bundle multiple
// values through a single nonterminal reduction.

// columnname: passes name span + typetoken span from column definition.
typedef struct SynqColumnNameValue {
  uint32_t name;
  SyntaqliteSourceSpan typetoken;
} SynqColumnNameValue;

// ccons / tcons / generated: a constraint node + pending constraint name.
typedef struct SynqConstraintValue {
  uint32_t node;
  SyntaqliteSourceSpan pending_name;
} SynqConstraintValue;

// carglist / conslist: accumulated constraint list + pending name for next.
typedef struct SynqConstraintListValue {
  uint32_t list;
  SyntaqliteSourceSpan pending_name;
} SynqConstraintListValue;

// on_using: ON expr / USING column-list discriminator.
typedef struct SynqOnUsingValue {
  uint32_t on_expr;
  uint32_t using_cols;
} SynqOnUsingValue;

// with: recursive flag + CTE list node ID.
typedef struct SynqWithValue {
  uint32_t cte_list;
  int is_recursive;
} SynqWithValue;

// where_opt_ret: WHERE expr + optional RETURNING column list.
typedef struct SynqWhereRetValue {
  uint32_t where_expr;
  uint32_t returning;
} SynqWhereRetValue;

// upsert: accumulated ON CONFLICT clauses + optional RETURNING column list.
typedef struct SynqUpsertValue {
  uint32_t clauses;
  uint32_t returning;
} SynqUpsertValue;
/* END GRAMMAR_TYPES */

#define YYPARSEFREENEVERNULL 1

// Map parser error bookkeeping to a best-effort source span.
static inline SyntaqliteSourceSpan synq_error_span(SynqParseCtx* pCtx) {
  if (pCtx->error_offset == 0xFFFFFFFF || pCtx->error_length == 0) {
    return SYNQ_NO_SPAN;
  }
  uint32_t len = pCtx->error_length;
  if (len > UINT16_MAX) {
    len = UINT16_MAX;
  }
  return (SyntaqliteSourceSpan){
      .offset = pCtx->error_offset,
      .length = (uint16_t)len,
      .flags = 0,
  };
}
}

// ============ Type declarations ============
//
// %token_type and %default_type are global; individual %type declarations
// live next to the rules they describe in each action file.

%token_type {SynqParseToken}
%default_type {uint32_t}

// ============ Error handlers ============

%syntax_error {
  (void)yymajor;
  (void)TOKEN;
  if (pCtx) {
    pCtx->error = 1;
  }
}

%stack_overflow {
  if (pCtx) {
    pCtx->error = 1;
  }
}

// ============ Tokens ============

%token ABORT ACTION AFTER ANALYZE ASC ATTACH BEFORE BEGIN BY CASCADE CAST.
%token CONFLICT DATABASE DEFERRED DESC DETACH EACH END EXCLUSIVE EXPLAIN FAIL.
%token OR AND NOT IS ISNOT MATCH LIKE_KW BETWEEN IN ISNULL NOTNULL NE EQ.
%token GT LE LT GE ESCAPE.

// The following directive causes tokens ABORT, AFTER, ASC, etc. to
// fallback to ID if they will not parse as their original value.
// This obviates the need for the "id" nonterminal.
//
%fallback ID
  ABORT ACTION AFTER ANALYZE ASC ATTACH BEFORE BEGIN BY CASCADE CAST COLUMNKW
  CONFLICT DATABASE DEFERRED DESC DETACH DO
  EACH END EXCLUSIVE EXPLAIN FAIL FOR
  IGNORE IMMEDIATE INITIALLY INSTEAD LIKE_KW MATCH NO PLAN
  QUERY KEY OF OFFSET PRAGMA RAISE RECURSIVE RELEASE REPLACE RESTRICT ROW ROWS
  ROLLBACK SAVEPOINT TEMP TRIGGER VACUUM VIEW VIRTUAL WITH WITHOUT
  NULLS FIRST LAST
  CURRENT FOLLOWING PARTITION PRECEDING RANGE UNBOUNDED
  EXCLUDE GROUPS OTHERS TIES
  GENERATED ALWAYS
  WITHIN
  MATERIALIZED
  REINDEX RENAME CTIME_KW IF
  .
%wildcard ANY.

%left OR.
%left AND.
%right NOT.
%left IS MATCH LIKE_KW BETWEEN IN ISNULL NOTNULL NE EQ.
%left GT LE LT GE.
%right ESCAPE.
%left BITAND BITOR LSHIFT RSHIFT.
%left PLUS MINUS.
%left STAR SLASH REM.
%left CONCAT PTR.
%left COLLATE.
%right BITNOT.
%nonassoc ON.

// ============ Token classes (match SQLite's parse.y) ============

%token_class id  ID|INDEXED.
%token_class ids  ID|STRING.
%token_class idj  ID|INDEXED|JOIN_KW.
%token_class number INTEGER|FLOAT.

// ============ Entry point ============

input ::= cmdlist(B). {
    pCtx->root = B;
}

// ============ Command list ============

cmdlist(A) ::= cmdlist ecmd(B). {
    A = B;  // Just use the last command for now
}

cmdlist(A) ::= ecmd(B). {
    A = B;
}

// ============ Command wrapper ============

ecmd(A) ::= SEMI. {
    A = SYNTAQLITE_NULL_NODE;
    pCtx->stmt_completed = 1;
}

// The action is on cmdx, not ecmd.  cmdx reduces when Lemon sees SEMI as its
// lookahead token, so stmt_completed fires while the C loop is processing the
// SEMI — the first token of the next statement is never consumed as a lookahead.
// This mirrors SQLite's own approach (sqlite3FinishCoding fires in cmdx ::= cmd).
ecmd(A) ::= cmdx(B) SEMI. {
    A = B;
}

// Error recovery: discard tokens until SEMI, then complete the statement.
// Lemon's built-in error token handles the synchronisation.
ecmd(A) ::= error SEMI. {
    A = SYNTAQLITE_NULL_NODE;
    pCtx->root = SYNTAQLITE_NULL_NODE;
    pCtx->stmt_completed = 1;
}

%parse_failure {
    if (pCtx) {
        pCtx->error = 1;
    }
}

cmdx(A) ::= cmd(B). {
    if (pCtx->pending_explain_mode) {
        A = synq_parse_explain_stmt(
            pCtx, (SyntaqliteExplainMode)(pCtx->pending_explain_mode - 1), B);
        pCtx->pending_explain_mode = 0;
    } else {
        A = B;
    }
    pCtx->root = A;
    synq_parse_list_flush(pCtx);
    pCtx->stmt_completed = 1;
}
