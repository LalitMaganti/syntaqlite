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
// - synq_span(pCtx, tok) converts a SynqParseToken into SyntaqliteTextSpan
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
  SyntaqliteTextSpan typetoken;
} SynqColumnNameValue;

// ccons / tcons / generated: a constraint node + pending constraint name.
typedef struct SynqConstraintValue {
  uint32_t node;
  SyntaqliteTextSpan pending_name;
} SynqConstraintValue;

// carglist / conslist: accumulated constraint list + pending name for next.
typedef struct SynqConstraintListValue {
  uint32_t list;
  SyntaqliteTextSpan pending_name;
  uint32_t last_node;
} SynqConstraintListValue;

// defer_subclause: DEFERRABLE / NOT DEFERRABLE plus the INITIALLY mode.
typedef struct SynqDeferValue {
  SyntaqliteDeferrable deferrable;
  SyntaqliteInitialDeferMode initial;
} SynqDeferValue;

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

// refarg / refargs: MATCH carries an identifier, so this cannot be a packed int.
#define SYNQ_REFARG_NONE   0
#define SYNQ_REFARG_DELETE 1
#define SYNQ_REFARG_UPDATE 2
#define SYNQ_REFARG_MATCH  3
#define SYNQ_REFARG_INSERT 4

typedef struct SynqRefArgValue {
  int kind;
  SyntaqliteForeignKeyAction action;
  SyntaqliteTextSpan match_name;
} SynqRefArgValue;

typedef struct SynqRefArgsValue {
  SyntaqliteForeignKeyAction on_delete;
  SyntaqliteForeignKeyAction on_update;
  SyntaqliteForeignKeyAction on_insert;
  SyntaqliteTextSpan match_name;
} SynqRefArgsValue;

// paren_exprlist: optional `LP exprlist RP` tail. Tracks whether the
// parens were present so callers can distinguish `foo` (has_parens=0)
// from `foo()` (has_parens=1, args=NULL_NODE) — relevant for table /
// table-valued function references where the two forms are distinct
// productions in the SQLite grammar.
typedef struct SynqParenExprlistValue {
  uint32_t args;
  SyntaqliteBool has_parens;
} SynqParenExprlistValue;
/* END GRAMMAR_TYPES */

#define YYPARSEFREENEVERNULL 1

// The ID fallback lets `GENERATED ALWAYS` be absorbed into a preceding type
// name. Trim it back off as sqlite3AddColumn does, reporting whether it was
// there so the keywords can still be emitted.
static inline int synq_trim_generated_always(SynqParseToken* t) {
  if (t->z == NULL || t->n < 16) {
    return 0;
  }
  if (SYNQ_STRNCASECMP(t->z + (t->n - 6), "always", 6) != 0) {
    return 0;
  }
  int n = t->n - 6;
  while (n > 0 && (t->z[n - 1] == ' ' || t->z[n - 1] == '\t' ||
                   t->z[n - 1] == '\n' || t->z[n - 1] == '\r')) {
    n--;
  }
  if (n < 9 || SYNQ_STRNCASECMP(t->z + (n - 9), "generated", 9) != 0) {
    return 0;
  }
  n -= 9;
  while (n > 0 && (t->z[n - 1] == ' ' || t->z[n - 1] == '\t' ||
                   t->z[n - 1] == '\n' || t->z[n - 1] == '\r')) {
    n--;
  }
  t->n = n;
  return 1;
}

// Join keywords are an unordered set, not a sequence; see sqlite3JoinType().
#define SYNQ_JT_INNER   0x01
#define SYNQ_JT_CROSS   0x02
#define SYNQ_JT_NATURAL 0x04
#define SYNQ_JT_LEFT    0x08
#define SYNQ_JT_RIGHT   0x10
#define SYNQ_JT_OUTER   0x20
#define SYNQ_JT_ERROR   0x40

static inline int synq_join_keyword_mask(const SynqParseToken* p) {
  static const struct {
    const char* text;
    unsigned char len;
    unsigned char code;
  } kw[] = {
      {"natural", 7, SYNQ_JT_NATURAL},
      {"left", 4, SYNQ_JT_LEFT | SYNQ_JT_OUTER},
      {"outer", 5, SYNQ_JT_OUTER},
      {"right", 5, SYNQ_JT_RIGHT | SYNQ_JT_OUTER},
      {"full", 4, SYNQ_JT_LEFT | SYNQ_JT_RIGHT | SYNQ_JT_OUTER},
      {"inner", 5, SYNQ_JT_INNER},
      {"cross", 5, SYNQ_JT_INNER | SYNQ_JT_CROSS},
  };
  if (p == NULL || p->z == NULL) {
    return 0;
  }
  for (unsigned j = 0; j < sizeof(kw) / sizeof(kw[0]); j++) {
    if (p->n == kw[j].len && SYNQ_STRNCASECMP(p->z, kw[j].text, p->n) == 0) {
      return kw[j].code;
    }
  }
  return SYNQ_JT_ERROR;
}

// Invalid combinations collapse to INNER, as sqlite3JoinType() does after erroring.
static inline SyntaqliteJoinType synq_join_type(const SynqParseToken* a,
                                                const SynqParseToken* b,
                                                const SynqParseToken* c) {
  int m = synq_join_keyword_mask(a) | synq_join_keyword_mask(b) |
          synq_join_keyword_mask(c);
  if ((m & (SYNQ_JT_INNER | SYNQ_JT_OUTER)) == (SYNQ_JT_INNER | SYNQ_JT_OUTER) ||
      (m & SYNQ_JT_ERROR) != 0 ||
      (m & (SYNQ_JT_OUTER | SYNQ_JT_LEFT | SYNQ_JT_RIGHT)) == SYNQ_JT_OUTER) {
    return SYNTAQLITE_JOIN_TYPE_INNER;
  }
  if (m & SYNQ_JT_NATURAL) {
    if (m & SYNQ_JT_CROSS) return SYNTAQLITE_JOIN_TYPE_NATURAL_CROSS;
    if ((m & SYNQ_JT_LEFT) && (m & SYNQ_JT_RIGHT))
      return SYNTAQLITE_JOIN_TYPE_NATURAL_FULL;
    if (m & SYNQ_JT_LEFT) return SYNTAQLITE_JOIN_TYPE_NATURAL_LEFT;
    if (m & SYNQ_JT_RIGHT) return SYNTAQLITE_JOIN_TYPE_NATURAL_RIGHT;
    return SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
  }
  if (m & SYNQ_JT_CROSS) return SYNTAQLITE_JOIN_TYPE_CROSS;
  if ((m & SYNQ_JT_LEFT) && (m & SYNQ_JT_RIGHT))
    return SYNTAQLITE_JOIN_TYPE_FULL;
  if (m & SYNQ_JT_LEFT) return SYNTAQLITE_JOIN_TYPE_LEFT;
  if (m & SYNQ_JT_RIGHT) return SYNTAQLITE_JOIN_TYPE_RIGHT;
  return SYNTAQLITE_JOIN_TYPE_INNER;
}

// Map parser error bookkeeping to a best-effort source span.
static inline SyntaqliteTextSpan synq_error_span(SynqParseCtx* pCtx) {
  if (pCtx->error_offset == 0xFFFFFFFF || pCtx->error_length == 0) {
    return SYNQ_NO_SPAN;
  }
  return (SyntaqliteTextSpan){
      .offset = pCtx->error_offset,
      .length = pCtx->error_length,
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
    A = synq_pass(pCtx, B);  // Just use the last command for now
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
    A = synq_pass(pCtx, B);
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
        // Widen the wrapper node's extents to cover the EXPLAIN /
        // EXPLAIN QUERY PLAN keyword, otherwise node_text(),
        // node_token_range() and node_expanded_text() of an explained
        // statement all drop it (e.g. "SELECT 1" for "EXPLAIN SELECT 1").
        //
        // Why this isn't automatic: the keyword lives in the `explain`
        // nonterminal, a sibling of `cmdx` in the parent rule
        // `ecmd ::= explain cmdx SEMI`. SQLite marks that rule {NEVER-REDUCE}
        // (and we mirror it: the statement is finished here, in cmdx), so the
        // reduction that would normally merge `explain`'s span into the node
        // never runs. on_reduce has already collapsed `cmd` into the shadow
        // stack top; the `explain` entry sits directly below it — fold it in
        // and re-record the widened extents on the wrapper node.
        synq_extent_fold_below_into_top(pCtx);
        synq_extent_record(pCtx, A);
    } else {
        A = B;
    }
    pCtx->root = A;
    synq_parse_list_flush(pCtx);
    pCtx->stmt_completed = 1;
}
