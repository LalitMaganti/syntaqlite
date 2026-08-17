/*
** 2000-05-29
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Driver template for the LEMON parser generator.
**
** The "lemon" program processes an LALR(1) input grammar file, then uses
** this template to construct a parser.  The "lemon" program inserts text
** at each "%%" line.  Also, any "P-a-r-s-e" identifier prefix (without the
** interstitial "-" characters) contained in this template is changed into
** the value of the %name directive from the grammar.  Otherwise, the content
** of this template is copied straight through into the generate parser
** source file.
**
** The following is the concatenation of all %include directives from the
** input grammar file:
*/
/************ Begin %include sections from the grammar ************************/
#include <limits.h>
#include <string.h>

#include "csrc/sqlite/dialect_builder.h"
#include "syntaqlite/types.h"
#include "syntaqlite_dialect/ast_builder.h"
#include "syntaqlite_dialect/dialect_macros.h"
#include "syntaqlite_dialect/sqlite_compat.h"

// Parser stack realloc/free macros. These expand at the Lemon call site
// where the parser struct is in scope, routing through pCtx->mem.
// YYREALLOC is called in yyGrowStack (parser variable: p).
// YYFREE is called in ParseFinalize (parser variable: pParser).
#define synq_stack_realloc(ptr, sz) (p->pCtx->mem.xRealloc((ptr), (sz)))
#define synq_stack_free(ptr) (pParser->pCtx->mem.xFree((ptr)))

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

// refarg / refargs: MATCH carries an identifier, so this cannot be a packed
// int.
#define SYNQ_REFARG_NONE 0
#define SYNQ_REFARG_DELETE 1
#define SYNQ_REFARG_UPDATE 2
#define SYNQ_REFARG_MATCH 3
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
#define SYNQ_JT_INNER 0x01
#define SYNQ_JT_CROSS 0x02
#define SYNQ_JT_NATURAL 0x04
#define SYNQ_JT_LEFT 0x08
#define SYNQ_JT_RIGHT 0x10
#define SYNQ_JT_OUTER 0x20
#define SYNQ_JT_ERROR 0x40

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

// Invalid combinations collapse to INNER, as sqlite3JoinType() does after
// erroring.
static inline SyntaqliteJoinType synq_join_type(const SynqParseToken* a,
                                                const SynqParseToken* b,
                                                const SynqParseToken* c) {
  int m = synq_join_keyword_mask(a) | synq_join_keyword_mask(b) |
          synq_join_keyword_mask(c);
  if ((m & (SYNQ_JT_INNER | SYNQ_JT_OUTER)) ==
          (SYNQ_JT_INNER | SYNQ_JT_OUTER) ||
      (m & SYNQ_JT_ERROR) != 0 ||
      (m & (SYNQ_JT_OUTER | SYNQ_JT_LEFT | SYNQ_JT_RIGHT)) == SYNQ_JT_OUTER) {
    return SYNTAQLITE_JOIN_TYPE_INNER;
  }
  if (m & SYNQ_JT_NATURAL) {
    if (m & SYNQ_JT_CROSS)
      return SYNTAQLITE_JOIN_TYPE_NATURAL_CROSS;
    if ((m & SYNQ_JT_LEFT) && (m & SYNQ_JT_RIGHT))
      return SYNTAQLITE_JOIN_TYPE_NATURAL_FULL;
    if (m & SYNQ_JT_LEFT)
      return SYNTAQLITE_JOIN_TYPE_NATURAL_LEFT;
    if (m & SYNQ_JT_RIGHT)
      return SYNTAQLITE_JOIN_TYPE_NATURAL_RIGHT;
    return SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
  }
  if (m & SYNQ_JT_CROSS)
    return SYNTAQLITE_JOIN_TYPE_CROSS;
  if ((m & SYNQ_JT_LEFT) && (m & SYNQ_JT_RIGHT))
    return SYNTAQLITE_JOIN_TYPE_FULL;
  if (m & SYNQ_JT_LEFT)
    return SYNTAQLITE_JOIN_TYPE_LEFT;
  if (m & SYNQ_JT_RIGHT)
    return SYNTAQLITE_JOIN_TYPE_RIGHT;
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
/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols.
***************** Begin token definitions *************************************/
#ifndef SYNTAQLITE_TK_ABORT
#define SYNTAQLITE_TK_ABORT 1
#define SYNTAQLITE_TK_ACTION 2
#define SYNTAQLITE_TK_AFTER 3
#define SYNTAQLITE_TK_ANALYZE 4
#define SYNTAQLITE_TK_ASC 5
#define SYNTAQLITE_TK_ATTACH 6
#define SYNTAQLITE_TK_BEFORE 7
#define SYNTAQLITE_TK_BEGIN 8
#define SYNTAQLITE_TK_BY 9
#define SYNTAQLITE_TK_CASCADE 10
#define SYNTAQLITE_TK_CAST 11
#define SYNTAQLITE_TK_CONFLICT 12
#define SYNTAQLITE_TK_DATABASE 13
#define SYNTAQLITE_TK_DEFERRED 14
#define SYNTAQLITE_TK_DESC 15
#define SYNTAQLITE_TK_DETACH 16
#define SYNTAQLITE_TK_EACH 17
#define SYNTAQLITE_TK_END 18
#define SYNTAQLITE_TK_EXCLUSIVE 19
#define SYNTAQLITE_TK_EXPLAIN 20
#define SYNTAQLITE_TK_FAIL 21
#define SYNTAQLITE_TK_OR 22
#define SYNTAQLITE_TK_AND 23
#define SYNTAQLITE_TK_NOT 24
#define SYNTAQLITE_TK_IS 25
#define SYNTAQLITE_TK_ISNOT 26
#define SYNTAQLITE_TK_MATCH 27
#define SYNTAQLITE_TK_LIKE_KW 28
#define SYNTAQLITE_TK_BETWEEN 29
#define SYNTAQLITE_TK_IN 30
#define SYNTAQLITE_TK_ISNULL 31
#define SYNTAQLITE_TK_NOTNULL 32
#define SYNTAQLITE_TK_NE 33
#define SYNTAQLITE_TK_EQ 34
#define SYNTAQLITE_TK_GT 35
#define SYNTAQLITE_TK_LE 36
#define SYNTAQLITE_TK_LT 37
#define SYNTAQLITE_TK_GE 38
#define SYNTAQLITE_TK_ESCAPE 39
#define SYNTAQLITE_TK_ID 40
#define SYNTAQLITE_TK_COLUMNKW 41
#define SYNTAQLITE_TK_DO 42
#define SYNTAQLITE_TK_FOR 43
#define SYNTAQLITE_TK_IGNORE 44
#define SYNTAQLITE_TK_IMMEDIATE 45
#define SYNTAQLITE_TK_INITIALLY 46
#define SYNTAQLITE_TK_INSTEAD 47
#define SYNTAQLITE_TK_NO 48
#define SYNTAQLITE_TK_PLAN 49
#define SYNTAQLITE_TK_QUERY 50
#define SYNTAQLITE_TK_KEY 51
#define SYNTAQLITE_TK_OF 52
#define SYNTAQLITE_TK_OFFSET 53
#define SYNTAQLITE_TK_PRAGMA 54
#define SYNTAQLITE_TK_RAISE 55
#define SYNTAQLITE_TK_RECURSIVE 56
#define SYNTAQLITE_TK_RELEASE 57
#define SYNTAQLITE_TK_REPLACE 58
#define SYNTAQLITE_TK_RESTRICT 59
#define SYNTAQLITE_TK_ROW 60
#define SYNTAQLITE_TK_ROWS 61
#define SYNTAQLITE_TK_ROLLBACK 62
#define SYNTAQLITE_TK_SAVEPOINT 63
#define SYNTAQLITE_TK_TEMP 64
#define SYNTAQLITE_TK_TRIGGER 65
#define SYNTAQLITE_TK_VACUUM 66
#define SYNTAQLITE_TK_VIEW 67
#define SYNTAQLITE_TK_VIRTUAL 68
#define SYNTAQLITE_TK_WITH 69
#define SYNTAQLITE_TK_WITHOUT 70
#define SYNTAQLITE_TK_NULLS 71
#define SYNTAQLITE_TK_FIRST 72
#define SYNTAQLITE_TK_LAST 73
#define SYNTAQLITE_TK_CURRENT 74
#define SYNTAQLITE_TK_FOLLOWING 75
#define SYNTAQLITE_TK_PARTITION 76
#define SYNTAQLITE_TK_PRECEDING 77
#define SYNTAQLITE_TK_RANGE 78
#define SYNTAQLITE_TK_UNBOUNDED 79
#define SYNTAQLITE_TK_EXCLUDE 80
#define SYNTAQLITE_TK_GROUPS 81
#define SYNTAQLITE_TK_OTHERS 82
#define SYNTAQLITE_TK_TIES 83
#define SYNTAQLITE_TK_GENERATED 84
#define SYNTAQLITE_TK_ALWAYS 85
#define SYNTAQLITE_TK_WITHIN 86
#define SYNTAQLITE_TK_MATERIALIZED 87
#define SYNTAQLITE_TK_REINDEX 88
#define SYNTAQLITE_TK_RENAME 89
#define SYNTAQLITE_TK_CTIME_KW 90
#define SYNTAQLITE_TK_IF 91
#define SYNTAQLITE_TK_ANY 92
#define SYNTAQLITE_TK_BITAND 93
#define SYNTAQLITE_TK_BITOR 94
#define SYNTAQLITE_TK_LSHIFT 95
#define SYNTAQLITE_TK_RSHIFT 96
#define SYNTAQLITE_TK_PLUS 97
#define SYNTAQLITE_TK_MINUS 98
#define SYNTAQLITE_TK_STAR 99
#define SYNTAQLITE_TK_SLASH 100
#define SYNTAQLITE_TK_REM 101
#define SYNTAQLITE_TK_CONCAT 102
#define SYNTAQLITE_TK_PTR 103
#define SYNTAQLITE_TK_COLLATE 104
#define SYNTAQLITE_TK_BITNOT 105
#define SYNTAQLITE_TK_ON 106
#define SYNTAQLITE_TK_INDEXED 107
#define SYNTAQLITE_TK_STRING 108
#define SYNTAQLITE_TK_JOIN_KW 109
#define SYNTAQLITE_TK_INTEGER 110
#define SYNTAQLITE_TK_FLOAT 111
#define SYNTAQLITE_TK_SEMI 112
#define SYNTAQLITE_TK_LP 113
#define SYNTAQLITE_TK_ORDER 114
#define SYNTAQLITE_TK_RP 115
#define SYNTAQLITE_TK_GROUP 116
#define SYNTAQLITE_TK_AS 117
#define SYNTAQLITE_TK_COMMA 118
#define SYNTAQLITE_TK_DOT 119
#define SYNTAQLITE_TK_UNION 120
#define SYNTAQLITE_TK_ALL 121
#define SYNTAQLITE_TK_EXCEPT 122
#define SYNTAQLITE_TK_INTERSECT 123
#define SYNTAQLITE_TK_EXISTS 124
#define SYNTAQLITE_TK_NULL 125
#define SYNTAQLITE_TK_DISTINCT 126
#define SYNTAQLITE_TK_FROM 127
#define SYNTAQLITE_TK_CASE 128
#define SYNTAQLITE_TK_WHEN 129
#define SYNTAQLITE_TK_THEN 130
#define SYNTAQLITE_TK_ELSE 131
#define SYNTAQLITE_TK_TABLE 132
#define SYNTAQLITE_TK_CONSTRAINT 133
#define SYNTAQLITE_TK_DEFAULT 134
#define SYNTAQLITE_TK_PRIMARY 135
#define SYNTAQLITE_TK_UNIQUE 136
#define SYNTAQLITE_TK_CHECK 137
#define SYNTAQLITE_TK_REFERENCES 138
#define SYNTAQLITE_TK_AUTOINCR 139
#define SYNTAQLITE_TK_INSERT 140
#define SYNTAQLITE_TK_DELETE 141
#define SYNTAQLITE_TK_UPDATE 142
#define SYNTAQLITE_TK_SET 143
#define SYNTAQLITE_TK_DEFERRABLE 144
#define SYNTAQLITE_TK_FOREIGN 145
#define SYNTAQLITE_TK_INTO 146
#define SYNTAQLITE_TK_VALUES 147
#define SYNTAQLITE_TK_WHERE 148
#define SYNTAQLITE_TK_RETURNING 149
#define SYNTAQLITE_TK_NOTHING 150
#define SYNTAQLITE_TK_BLOB 151
#define SYNTAQLITE_TK_QNUMBER 152
#define SYNTAQLITE_TK_VARIABLE 153
#define SYNTAQLITE_TK_DROP 154
#define SYNTAQLITE_TK_INDEX 155
#define SYNTAQLITE_TK_ALTER 156
#define SYNTAQLITE_TK_TO 157
#define SYNTAQLITE_TK_ADD 158
#define SYNTAQLITE_TK_COMMIT 159
#define SYNTAQLITE_TK_TRANSACTION 160
#define SYNTAQLITE_TK_SELECT 161
#define SYNTAQLITE_TK_HAVING 162
#define SYNTAQLITE_TK_LIMIT 163
#define SYNTAQLITE_TK_JOIN 164
#define SYNTAQLITE_TK_USING 165
#define SYNTAQLITE_TK_CREATE 166
#define SYNTAQLITE_TK_WINDOW 167
#define SYNTAQLITE_TK_OVER 168
#define SYNTAQLITE_TK_FILTER 169
#define SYNTAQLITE_TK_COLUMN 170
#define SYNTAQLITE_TK_AGG_FUNCTION 171
#define SYNTAQLITE_TK_AGG_COLUMN 172
#define SYNTAQLITE_TK_TRUEFALSE 173
#define SYNTAQLITE_TK_FUNCTION 174
#define SYNTAQLITE_TK_UPLUS 175
#define SYNTAQLITE_TK_UMINUS 176
#define SYNTAQLITE_TK_TRUTH 177
#define SYNTAQLITE_TK_REGISTER 178
#define SYNTAQLITE_TK_VECTOR 179
#define SYNTAQLITE_TK_SELECT_COLUMN 180
#define SYNTAQLITE_TK_IF_NULL_ROW 181
#define SYNTAQLITE_TK_ASTERISK 182
#define SYNTAQLITE_TK_SPAN 183
#define SYNTAQLITE_TK_ERROR 184
#define SYNTAQLITE_TK_SPACE 185
#define SYNTAQLITE_TK_COMMENT 186
#define SYNTAQLITE_TK_ILLEGAL 187
#define SYNTAQLITE_TK_BANG 188
#endif
/**************** End token definitions ***************************************/

/* The next sections is a series of control #defines.
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used to store the integer codes
**                       that represent terminal and non-terminal symbols.
**                       "unsigned char" is used if there are fewer than
**                       256 symbols.  Larger types otherwise.
**    YYNOCODE           is a number of type YYCODETYPE that is not used for
**                       any terminal or nonterminal symbol.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       (also known as: "terminal symbols") have fall-back
**                       values which should be used if the original symbol
**                       would not parse.  This permits keywords to sometimes
**                       be used as identifiers, for example.
**    YYACTIONTYPE       is the data type used for "action codes" - numbers
**                       that indicate what to do in response to the next
**                       token.
**    SynqSqliteParseTOKENTYPE     is the data type used for minor type for
*terminal
**                       symbols.  Background: A "minor type" is a semantic
**                       value associated with a terminal or non-terminal
**                       symbols.  For example, for an "ID" terminal symbol,
**                       the minor type might be the name of the identifier.
**                       Each non-terminal can have a different minor type.
**                       Terminal symbols all have the same minor type, though.
**                       This macros defines the minor type for terminal
**                       symbols.
**    YYMINORTYPE        is the data type used for all minor types.
**                       This is typically a union of many types, one of
**                       which is SynqSqliteParseTOKENTYPE.  The entry in the
*union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    SynqSqliteParseARG_SDECL     A static variable declaration for the
*%extra_argument
**    SynqSqliteParseARG_PDECL     A parameter declaration for the
*%extra_argument
**    SynqSqliteParseARG_PARAM     Code to pass %extra_argument as a subroutine
*parameter
**    SynqSqliteParseARG_STORE     Code to store %extra_argument into yypParser
**    SynqSqliteParseARG_FETCH     Code to extract %extra_argument from
*yypParser
**    SynqSqliteParseCTX_*         As SynqSqliteParseARG_ except for
*%extra_context
**    YYREALLOC          Name of the realloc() function to use
**    YYFREE             Name of the free() function to use
**    YYDYNSTACK         True if stack space should be extended on heap
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYNTOKEN           Number of terminal symbols
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
**    YY_MIN_REDUCE      Minimum value for reduce actions
**    YY_MAX_REDUCE      Maximum value for reduce actions
**    YY_MIN_DSTRCTR     Minimum symbol value that has a destructor
**    YY_MAX_DSTRCTR     Maximum symbol value that has a destructor
*/
#ifndef INTERFACE
#define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned short int
#define YYNOCODE 326
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 92
#define SynqSqliteParseTOKENTYPE SynqParseToken
typedef union {
  int yyinit;
  SynqSqliteParseTOKENTYPE yy0;
  SynqRefArgValue yy76;
  SyntaqliteJoinType yy81;
  SynqWhereRetValue yy119;
  SynqConstraintValue yy150;
  uint32_t yy277;
  int yy320;
  SynqUpsertValue yy352;
  SynqConstraintListValue yy430;
  SynqDeferValue yy519;
  SynqWithValue yy541;
  SynqRefArgsValue yy603;
  SynqParenExprlistValue yy618;
  SynqOnUsingValue yy632;
  SynqColumnNameValue yy640;
  int yy653;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define SynqSqliteParseARG_SDECL
#define SynqSqliteParseARG_PDECL
#define SynqSqliteParseARG_PARAM
#define SynqSqliteParseARG_FETCH
#define SynqSqliteParseARG_STORE
#define YYREALLOC synq_stack_realloc
#define YYFREE synq_stack_free
#define YYDYNSTACK 1
#define SynqSqliteParseCTX_SDECL SynqParseCtx* pCtx;
#define SynqSqliteParseCTX_PDECL , SynqParseCtx* pCtx
#define SynqSqliteParseCTX_PARAM , pCtx
#define SynqSqliteParseCTX_FETCH SynqParseCtx* pCtx = yypParser->pCtx;
#define SynqSqliteParseCTX_STORE yypParser->pCtx = pCtx;
#define YYERRORSYMBOL 193
#define YYERRSYMDT yy653
#define YYFALLBACK 1
#define YYNSTATE 595
#define YYNRULE 415
#define YYNRULE_WITH_ACTION 415
#define YYNTOKEN 189
#define YY_MAX_SHIFT 594
#define YY_MIN_SHIFTREDUCE 862
#define YY_MAX_SHIFTREDUCE 1276
#define YY_ERROR_ACTION 1277
#define YY_ACCEPT_ACTION 1278
#define YY_NO_ACTION 1279
#define YY_MIN_REDUCE 1280
#define YY_MAX_REDUCE 1694
#define YY_MIN_DSTRCTR 0
#define YY_MAX_DSTRCTR 0
/************* End control #defines *******************************************/
#define YY_NLOOKAHEAD ((int)(sizeof(yy_lookahead) / sizeof(yy_lookahead[0])))

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
#define yytestcase(X)
#endif

/* Macro to determine if stack space has the ability to grow using
** heap memory.
*/
#if YYSTACKDEPTH <= 0 || YYDYNSTACK
#define YYGROWABLESTACK 1
#else
#define YYGROWABLESTACK 0
#endif

/* Guarantee a minimum number of initial stack slots.
 */
#if YYSTACKDEPTH <= 0
#undef YYSTACKDEPTH
#define YYSTACKDEPTH 2 /* Need a minimum stack size */
#endif

/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N <= YY_MAX_SHIFT             Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   N between YY_MIN_SHIFTREDUCE       Shift to an arbitrary state then
**     and YY_MAX_SHIFTREDUCE           reduce by rule N-YY_MIN_SHIFTREDUCE.
**
**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as either:
**
**    (A)   N = yy_action[ yy_shift_ofst[S] + X ]
**    (B)   N = yy_default[S]
**
** The (A) formula is preferred.  The B formula is used instead if
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X.
**
** The formulas above are for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
**
*********** Begin parsing tables **********************************************/
#define YY_ACTTAB_COUNT (2332)
static const YYACTIONTYPE yy_action[] = {
    /*     0 */ 1064,
    245,
    1351,
    1064,
    1064,
    1074,
    1362,
    91,
    93,
    295,
    /*    10 */ 1536,
    530,
    1348,
    1481,
    1354,
    1075,
    1481,
    1481,
    295,
    1536,
    /*    20 */ 410,
    1345,
    84,
    85,
    420,
    42,
    430,
    908,
    908,
    905,
    /*    30 */ 890,
    899,
    899,
    86,
    86,
    87,
    87,
    87,
    87,
    1389,
    /*    40 */ 310,
    1061,
    528,
    1342,
    1353,
    84,
    85,
    420,
    42,
    418,
    /*    50 */ 908,
    908,
    905,
    890,
    899,
    899,
    86,
    86,
    87,
    87,
    /*    60 */ 87,
    87,
    1037,
    491,
    114,
    295,
    1536,
    297,
    495,
    325,
    /*    70 */ 87,
    87,
    87,
    87,
    90,
    587,
    63,
    1657,
    406,
    226,
    /*    80 */ 966,
    343,
    508,
    1532,
    273,
    245,
    1477,
    241,
    493,
    429,
    /*    90 */ 467,
    91,
    93,
    83,
    83,
    83,
    83,
    89,
    89,
    88,
    /*   100 */ 88,
    88,
    82,
    81,
    453,
    571,
    569,
    499,
    1061,
    1062,
    /*   110 */ 1061,
    967,
    1456,
    453,
    571,
    569,
    83,
    83,
    83,
    83,
    /*   120 */ 89,
    89,
    88,
    88,
    88,
    82,
    81,
    453,
    83,
    83,
    /*   130 */ 83,
    83,
    89,
    89,
    88,
    88,
    88,
    82,
    81,
    453,
    /*   140 */ 1388,
    62,
    84,
    85,
    420,
    42,
    299,
    908,
    908,
    905,
    /*   150 */ 890,
    899,
    899,
    86,
    86,
    87,
    87,
    87,
    87,
    540,
    /*   160 */ 543,
    571,
    569,
    1240,
    84,
    85,
    420,
    42,
    1630,
    908,
    /*   170 */ 908,
    905,
    890,
    899,
    899,
    86,
    86,
    87,
    87,
    87,
    /*   180 */ 87,
    1624,
    1130,
    1037,
    1242,
    114,
    1654,
    1129,
    6,
    1654,
    /*   190 */ 394,
    301,
    1677,
    46,
    1672,
    1263,
    587,
    1263,
    83,
    83,
    /*   200 */ 83,
    83,
    89,
    89,
    88,
    88,
    88,
    82,
    81,
    453,
    /*   210 */ 1478,
    404,
    1690,
    83,
    83,
    83,
    83,
    89,
    89,
    88,
    /*   220 */ 88,
    88,
    82,
    81,
    453,
    89,
    89,
    88,
    88,
    88,
    /*   230 */ 82,
    81,
    453,
    1456,
    588,
    83,
    83,
    83,
    83,
    89,
    /*   240 */ 89,
    88,
    88,
    88,
    82,
    81,
    453,
    84,
    85,
    420,
    /*   250 */ 42,
    386,
    908,
    908,
    905,
    890,
    899,
    899,
    86,
    86,
    /*   260 */ 87,
    87,
    87,
    87,
    88,
    88,
    88,
    82,
    81,
    453,
    /*   270 */ 541,
    1664,
    84,
    85,
    420,
    42,
    399,
    908,
    908,
    905,
    /*   280 */ 890,
    899,
    899,
    86,
    86,
    87,
    87,
    87,
    87,
    1061,
    /*   290 */ 1371,
    1407,
    1398,
    1164,
    1396,
    84,
    85,
    420,
    42,
    499,
    /*   300 */ 908,
    908,
    905,
    890,
    899,
    899,
    86,
    86,
    87,
    87,
    /*   310 */ 87,
    87,
    393,
    1679,
    82,
    81,
    453,
    439,
    83,
    83,
    /*   320 */ 83,
    83,
    89,
    89,
    88,
    88,
    88,
    82,
    81,
    453,
    /*   330 */ 1618,
    1619,
    484,
    361,
    403,
    404,
    1690,
    110,
    1405,
    295,
    /*   340 */ 1536,
    306,
    47,
    83,
    83,
    83,
    83,
    89,
    89,
    88,
    /*   350 */ 88,
    88,
    82,
    81,
    453,
    535,
    1061,
    1062,
    1061,
    1169,
    /*   360 */ 1169,
    505,
    295,
    1536,
    1642,
    1345,
    83,
    83,
    83,
    83,
    /*   370 */ 89,
    89,
    88,
    88,
    88,
    82,
    81,
    453,
    547,
    455,
    /*   380 */ 454,
    1007,
    265,
    358,
    520,
    517,
    516,
    1343,
    372,
    295,
    /*   390 */ 1536,
    1340,
    1198,
    1198,
    515,
    84,
    85,
    420,
    42,
    231,
    /*   400 */ 908,
    908,
    905,
    890,
    899,
    899,
    86,
    86,
    87,
    87,
    /*   410 */ 87,
    87,
    295,
    1536,
    580,
    1061,
    484,
    361,
    84,
    85,
    /*   420 */ 420,
    42,
    269,
    908,
    908,
    905,
    890,
    899,
    899,
    86,
    /*   430 */ 86,
    87,
    87,
    87,
    87,
    571,
    569,
    1061,
    268,
    267,
    /*   440 */ 266,
    84,
    85,
    420,
    42,
    272,
    908,
    908,
    905,
    890,
    /*   450 */ 899,
    899,
    86,
    86,
    87,
    87,
    87,
    87,
    571,
    569,
    /*   460 */ 246,
    549,
    1061,
    359,
    477,
    346,
    83,
    83,
    83,
    83,
    /*   470 */ 89,
    89,
    88,
    88,
    88,
    82,
    81,
    453,
    106,
    295,
    /*   480 */ 1536,
    583,
    1061,
    1062,
    1061,
    571,
    569,
    1061,
    521,
    83,
    /*   490 */ 83,
    83,
    83,
    89,
    89,
    88,
    88,
    88,
    82,
    81,
    /*   500 */ 453,
    425,
    1105,
    108,
    1061,
    1062,
    1061,
    1107,
    571,
    569,
    /*   510 */ 210,
    942,
    83,
    83,
    83,
    83,
    89,
    89,
    88,
    88,
    /*   520 */ 88,
    82,
    81,
    453,
    242,
    909,
    909,
    906,
    891,
    1061,
    /*   530 */ 1062,
    1061,
    1061,
    1106,
    934,
    84,
    85,
    420,
    42,
    1064,
    /*   540 */ 908,
    908,
    905,
    890,
    899,
    899,
    86,
    86,
    87,
    87,
    /*   550 */ 87,
    87,
    1481,
    456,
    1061,
    1062,
    1061,
    1130,
    84,
    85,
    /*   560 */ 420,
    42,
    1129,
    908,
    908,
    905,
    890,
    899,
    899,
    86,
    /*   570 */ 86,
    87,
    87,
    87,
    87,
    571,
    569,
    582,
    399,
    887,
    /*   580 */ 887,
    84,
    85,
    420,
    42,
    1581,
    908,
    908,
    905,
    890,
    /*   590 */ 899,
    899,
    86,
    86,
    87,
    87,
    87,
    87,
    586,
    1061,
    /*   600 */ 1062,
    1061,
    463,
    13,
    1286,
    211,
    83,
    83,
    83,
    83,
    /*   610 */ 89,
    89,
    88,
    88,
    88,
    82,
    81,
    453,
    508,
    448,
    /*   620 */ 295,
    1536,
    1534,
    900,
    1302,
    1510,
    1258,
    1099,
    183,
    83,
    /*   630 */ 83,
    83,
    83,
    89,
    89,
    88,
    88,
    88,
    82,
    81,
    /*   640 */ 453,
    553,
    532,
    1258,
    1240,
    313,
    1258,
    172,
    436,
    1618,
    /*   650 */ 1619,
    1276,
    83,
    83,
    83,
    83,
    89,
    89,
    88,
    88,
    /*   660 */ 88,
    82,
    81,
    453,
    254,
    1242,
    245,
    1655,
    246,
    549,
    /*   670 */ 1655,
    1061,
    91,
    93,
    1056,
    84,
    85,
    420,
    42,
    552,
    /*   680 */ 908,
    908,
    905,
    890,
    899,
    899,
    86,
    86,
    87,
    87,
    /*   690 */ 87,
    87,
    1061,
    1611,
    500,
    1061,
    398,
    1611,
    84,
    85,
    /*   700 */ 420,
    42,
    395,
    908,
    908,
    905,
    890,
    899,
    899,
    86,
    /*   710 */ 86,
    87,
    87,
    87,
    87,
    1061,
    571,
    569,
    462,
    571,
    /*   720 */ 569,
    84,
    85,
    420,
    42,
    533,
    908,
    908,
    905,
    890,
    /*   730 */ 899,
    899,
    86,
    86,
    87,
    87,
    87,
    87,
    1061,
    1062,
    /*   740 */ 1061,
    1693,
    1061,
    170,
    5,
    1581,
    83,
    83,
    83,
    83,
    /*   750 */ 89,
    89,
    88,
    88,
    88,
    82,
    81,
    453,
    239,
    1061,
    /*   760 */ 1062,
    1061,
    1061,
    1062,
    1061,
    2,
    878,
    234,
    1039,
    83,
    /*   770 */ 83,
    83,
    83,
    89,
    89,
    88,
    88,
    88,
    82,
    81,
    /*   780 */ 453,
    328,
    1061,
    1062,
    1061,
    1610,
    464,
    465,
    98,
    1610,
    /*   790 */ 1608,
    1606,
    83,
    83,
    83,
    83,
    89,
    89,
    88,
    88,
    /*   800 */ 88,
    82,
    81,
    453,
    292,
    1536,
    72,
    100,
    437,
    1061,
    /*   810 */ 1062,
    1061,
    1407,
    201,
    1081,
    84,
    85,
    420,
    42,
    1155,
    /*   820 */ 908,
    908,
    905,
    890,
    899,
    899,
    86,
    86,
    87,
    87,
    /*   830 */ 87,
    87,
    329,
    1245,
    878,
    1581,
    1407,
    84,
    85,
    420,
    /*   840 */ 42,
    499,
    908,
    908,
    905,
    890,
    899,
    899,
    86,
    86,
    /*   850 */ 87,
    87,
    87,
    87,
    405,
    403,
    1244,
    296,
    112,
    1405,
    /*   860 */ 1407,
    1061,
    84,
    85,
    420,
    42,
    1154,
    908,
    908,
    905,
    /*   870 */ 890,
    899,
    899,
    86,
    86,
    87,
    87,
    87,
    87,
    403,
    /*   880 */ 1166,
    1375,
    109,
    1405,
    1166,
    579,
    83,
    83,
    83,
    83,
    /*   890 */ 89,
    89,
    88,
    88,
    88,
    82,
    81,
    453,
    438,
    324,
    /*   900 */ 571,
    569,
    176,
    403,
    293,
    1536,
    111,
    1405,
    83,
    83,
    /*   910 */ 83,
    83,
    89,
    89,
    88,
    88,
    88,
    82,
    81,
    453,
    /*   920 */ 880,
    6,
    337,
    79,
    503,
    308,
    538,
    1671,
    1061,
    1062,
    /*   930 */ 1061,
    579,
    228,
    83,
    83,
    83,
    83,
    89,
    89,
    88,
    /*   940 */ 88,
    88,
    82,
    81,
    453,
    324,
    84,
    92,
    420,
    42,
    /*   950 */ 567,
    908,
    908,
    905,
    890,
    899,
    899,
    86,
    86,
    87,
    /*   960 */ 87,
    87,
    87,
    85,
    420,
    42,
    330,
    908,
    908,
    905,
    /*   970 */ 890,
    899,
    899,
    86,
    86,
    87,
    87,
    87,
    87,
    420,
    /*   980 */ 42,
    540,
    908,
    908,
    905,
    890,
    899,
    899,
    86,
    86,
    /*   990 */ 87,
    87,
    87,
    87,
    532,
    1037,
    1064,
    114,
    317,
    1064,
    /*  1000 */ 571,
    569,
    590,
    1007,
    87,
    87,
    87,
    87,
    587,
    1481,
    /*  1010 */ 1518,
    426,
    1481,
    991,
    100,
    76,
    1566,
    83,
    83,
    83,
    /*  1020 */ 83,
    89,
    89,
    88,
    88,
    88,
    82,
    81,
    453,
    100,
    /*  1030 */ 368,
    459,
    364,
    83,
    83,
    83,
    83,
    89,
    89,
    88,
    /*  1040 */ 88,
    88,
    82,
    81,
    453,
    1456,
    576,
    497,
    83,
    83,
    /*  1050 */ 83,
    83,
    89,
    89,
    88,
    88,
    88,
    82,
    81,
    453,
    /*  1060 */ 100,
    590,
    83,
    83,
    83,
    83,
    89,
    89,
    88,
    88,
    /*  1070 */ 88,
    82,
    81,
    453,
    76,
    1531,
    59,
    533,
    1533,
    8,
    /*  1080 */ 1528,
    1069,
    1037,
    937,
    38,
    359,
    430,
    431,
    78,
    78,
    /*  1090 */ 459,
    1290,
    579,
    1665,
    471,
    587,
    77,
    370,
    459,
    577,
    /*  1100 */ 459,
    1065,
    1067,
    6,
    4,
    576,
    324,
    579,
    265,
    1669,
    /*  1110 */ 520,
    517,
    516,
    561,
    1064,
    584,
    1067,
    69,
    1427,
    25,
    /*  1120 */ 515,
    324,
    1061,
    557,
    555,
    1677,
    1037,
    1481,
    38,
    554,
    /*  1130 */ 498,
    434,
    1456,
    1069,
    493,
    534,
    590,
    1428,
    579,
    587,
    /*  1140 */ 1069,
    224,
    1067,
    1068,
    1070,
    1434,
    1069,
    78,
    78,
    76,
    /*  1150 */ 937,
    1066,
    324,
    1065,
    1067,
    77,
    1245,
    459,
    577,
    459,
    /*  1160 */ 1065,
    1067,
    1407,
    4,
    1066,
    459,
    1065,
    1067,
    1067,
    1037,
    /*  1170 */ 1037,
    148,
    148,
    565,
    584,
    1067,
    1456,
    405,
    25,
    1241,
    /*  1180 */ 576,
    1067,
    587,
    587,
    579,
    1433,
    295,
    1536,
    564,
    1061,
    /*  1190 */ 1062,
    1061,
    302,
    1513,
    1067,
    1068,
    417,
    416,
    324,
    555,
    /*  1200 */ 1061,
    1067,
    1068,
    1070,
    556,
    403,
    430,
    1067,
    1068,
    1406,
    /*  1210 */ 474,
    590,
    58,
    1064,
    1064,
    1069,
    404,
    1690,
    67,
    1456,
    /*  1220 */ 1456,
    1581,
    78,
    78,
    76,
    201,
    1481,
    1481,
    44,
    338,
    /*  1230 */ 77,
    340,
    459,
    577,
    459,
    1065,
    1067,
    1400,
    4,
    951,
    /*  1240 */ 459,
    1278,
    1,
    1282,
    594,
    593,
    1286,
    563,
    536,
    584,
    /*  1250 */ 1067,
    561,
    1037,
    25,
    146,
    576,
    1428,
    455,
    454,
    389,
    /*  1260 */ 1083,
    225,
    295,
    1536,
    1534,
    587,
    1194,
    1061,
    1062,
    1061,
    /*  1270 */ 1198,
    1198,
    1581,
    493,
    555,
    401,
    1067,
    1068,
    1070,
    554,
    /*  1280 */ 1084,
    388,
    571,
    569,
    504,
    472,
    1037,
    313,
    38,
    172,
    /*  1290 */ 1069,
    440,
    1512,
    508,
    1196,
    1258,
    341,
    78,
    78,
    587,
    /*  1300 */ 1509,
    1195,
    1456,
    1011,
    174,
    77,
    254,
    459,
    577,
    459,
    /*  1310 */ 1065,
    1067,
    1258,
    4,
    1280,
    1258,
    1653,
    1012,
    248,
    339,
    /*  1320 */ 327,
    1082,
    283,
    1127,
    584,
    1067,
    424,
    1064,
    25,
    349,
    /*  1330 */ 326,
    305,
    334,
    6,
    461,
    445,
    1456,
    590,
    550,
    1671,
    /*  1340 */ 1481,
    485,
    1037,
    1037,
    148,
    38,
    531,
    493,
    309,
    356,
    /*  1350 */ 76,
    1067,
    1068,
    1070,
    3,
    587,
    587,
    487,
    571,
    569,
    /*  1360 */ 462,
    1064,
    1037,
    245,
    38,
    303,
    459,
    218,
    249,
    91,
    /*  1370 */ 93,
    331,
    1288,
    480,
    1481,
    587,
    333,
    165,
    352,
    501,
    /*  1380 */ 193,
    576,
    1037,
    99,
    38,
    589,
    426,
    219,
    1037,
    482,
    /*  1390 */ 148,
    1127,
    1456,
    1456,
    1037,
    587,
    148,
    6,
    562,
    396,
    /*  1400 */ 175,
    587,
    247,
    1670,
    1599,
    307,
    508,
    587,
    385,
    243,
    /*  1410 */ 413,
    1597,
    1456,
    1508,
    590,
    288,
    1069,
    1037,
    1153,
    38,
    /*  1420 */ 563,
    1061,
    560,
    78,
    78,
    31,
    865,
    41,
    1064,
    34,
    /*  1430 */ 587,
    77,
    1456,
    459,
    577,
    459,
    1065,
    1067,
    1456,
    4,
    /*  1440 */ 226,
    1481,
    1258,
    459,
    1456,
    363,
    291,
    590,
    367,
    245,
    /*  1450 */ 584,
    1067,
    573,
    1074,
    25,
    91,
    93,
    591,
    576,
    1258,
    /*  1460 */ 76,
    579,
    1258,
    1075,
    1153,
    432,
    319,
    1456,
    421,
    1037,
    /*  1470 */ 469,
    38,
    563,
    334,
    457,
    324,
    459,
    1067,
    1068,
    1070,
    /*  1480 */ 1225,
    1226,
    587,
    1064,
    512,
    479,
    369,
    269,
    1061,
    1062,
    /*  1490 */ 1061,
    576,
    1037,
    1069,
    155,
    551,
    1481,
    404,
    1690,
    371,
    /*  1500 */ 78,
    78,
    1037,
    294,
    148,
    587,
    458,
    468,
    77,
    1479,
    /*  1510 */ 459,
    577,
    459,
    1065,
    1067,
    587,
    4,
    6,
    6,
    1456,
    /*  1520 */ 1374,
    427,
    202,
    1668,
    1667,
    1127,
    1069,
    584,
    1067,
    572,
    /*  1530 */ 243,
    25,
    408,
    78,
    78,
    1122,
    15,
    1064,
    1064,
    1153,
    /*  1540 */ 1392,
    77,
    1456,
    459,
    577,
    459,
    1065,
    1067,
    473,
    4,
    /*  1550 */ 1481,
    1481,
    1456,
    1220,
    1067,
    1068,
    1070,
    590,
    960,
    1436,
    /*  1560 */ 584,
    1067,
    226,
    1064,
    25,
    248,
    425,
    327,
    1387,
    283,
    /*  1570 */ 76,
    972,
    1639,
    1373,
    1258,
    59,
    1481,
    326,
    1372,
    334,
    /*  1580 */ 563,
    461,
    1037,
    239,
    148,
    1153,
    459,
    1067,
    1068,
    1070,
    /*  1590 */ 1037,
    1258,
    148,
    1127,
    1258,
    587,
    524,
    378,
    9,
    879,
    /*  1600 */ 300,
    576,
    175,
    587,
    375,
    1083,
    290,
    961,
    1037,
    241,
    /*  1610 */ 148,
    1436,
    1350,
    60,
    13,
    249,
    1511,
    1504,
    331,
    475,
    /*  1620 */ 419,
    587,
    1564,
    333,
    165,
    1084,
    1037,
    193,
    148,
    216,
    /*  1630 */ 99,
    67,
    1456,
    1037,
    534,
    159,
    1069,
    476,
    419,
    587,
    /*  1640 */ 1456,
    1344,
    1544,
    78,
    78,
    489,
    587,
    1578,
    575,
    247,
    /*  1650 */ 1037,
    77,
    40,
    459,
    577,
    459,
    1065,
    1067,
    1456,
    4,
    /*  1660 */ 450,
    444,
    1580,
    542,
    486,
    419,
    1082,
    879,
    451,
    1123,
    /*  1670 */ 584,
    1067,
    178,
    865,
    25,
    248,
    1456,
    327,
    234,
    283,
    /*  1680 */ 251,
    559,
    253,
    1456,
    490,
    419,
    452,
    326,
    233,
    334,
    /*  1690 */ 1064,
    442,
    1281,
    594,
    593,
    1286,
    1684,
    1067,
    1068,
    1070,
    /*  1700 */ 1456,
    221,
    1314,
    1481,
    320,
    483,
    400,
    558,
    579,
    318,
    /*  1710 */ 419,
    295,
    1536,
    1534,
    958,
    421,
    1269,
    469,
    49,
    230,
    /*  1720 */ 334,
    523,
    324,
    959,
    1301,
    249,
    1064,
    1225,
    331,
    1037,
    /*  1730 */ 80,
    129,
    74,
    333,
    165,
    96,
    313,
    193,
    172,
    1481,
    /*  1740 */ 99,
    441,
    587,
    1037,
    383,
    130,
    592,
    179,
    1037,
    1269,
    /*  1750 */ 124,
    1037,
    335,
    131,
    376,
    254,
    587,
    1077,
    1078,
    247,
    /*  1760 */ 574,
    587,
    384,
    382,
    587,
    295,
    1536,
    585,
    945,
    508,
    /*  1770 */ 271,
    286,
    526,
    379,
    525,
    270,
    1507,
    509,
    1037,
    1456,
    /*  1780 */ 39,
    375,
    998,
    1037,
    1037,
    115,
    116,
    1199,
    1199,
    1234,
    /*  1790 */ 1071,
    587,
    407,
    1456,
    1197,
    1197,
    587,
    587,
    1456,
    284,
    /*  1800 */ 236,
    1456,
    1037,
    3,
    132,
    508,
    314,
    571,
    569,
    462,
    /*  1810 */ 351,
    315,
    366,
    67,
    95,
    587,
    316,
    357,
    579,
    1539,
    /*  1820 */ 67,
    185,
    1037,
    11,
    133,
    421,
    566,
    469,
    1456,
    1037,
    /*  1830 */ 334,
    134,
    324,
    1456,
    1456,
    587,
    945,
    1225,
    1037,
    1422,
    /*  1840 */ 135,
    345,
    587,
    1037,
    1037,
    136,
    117,
    1037,
    355,
    118,
    /*  1850 */ 998,
    587,
    1456,
    1037,
    169,
    119,
    587,
    587,
    1071,
    298,
    /*  1860 */ 587,
    571,
    569,
    348,
    354,
    1037,
    587,
    120,
    1037,
    1037,
    /*  1870 */ 137,
    138,
    1456,
    1037,
    1450,
    139,
    1449,
    1028,
    587,
    1456,
    /*  1880 */ 275,
    587,
    587,
    1037,
    411,
    140,
    587,
    1037,
    1456,
    141,
    /*  1890 */ 304,
    360,
    492,
    1456,
    1456,
    275,
    587,
    1456,
    518,
    494,
    /*  1900 */ 587,
    513,
    275,
    1456,
    279,
    373,
    381,
    1037,
    67,
    113,
    /*  1910 */ 496,
    1037,
    240,
    121,
    1037,
    1456,
    122,
    1337,
    1456,
    1456,
    /*  1920 */ 587,
    1371,
    392,
    1456,
    587,
    1569,
    1158,
    587,
    1037,
    275,
    /*  1930 */ 37,
    222,
    1037,
    1456,
    123,
    1570,
    1037,
    1456,
    142,
    1568,
    /*  1940 */ 1413,
    587,
    1037,
    994,
    153,
    587,
    279,
    1567,
    1230,
    587,
    /*  1950 */ 1037,
    69,
    154,
    223,
    1414,
    587,
    1037,
    1456,
    143,
    578,
    /*  1960 */ 1037,
    1456,
    125,
    587,
    1456,
    235,
    1037,
    1229,
    144,
    587,
    /*  1970 */ 69,
    1208,
    1037,
    587,
    126,
    1037,
    44,
    150,
    1456,
    587,
    /*  1980 */ 1623,
    278,
    1456,
    1621,
    1228,
    587,
    1456,
    69,
    587,
    45,
    /*  1990 */ 48,
    1037,
    1456,
    188,
    1631,
    1037,
    1037,
    189,
    145,
    876,
    /*  2000 */ 1456,
    1037,
    177,
    127,
    587,
    428,
    1456,
    1111,
    587,
    587,
    /*  2010 */ 1456,
    164,
    184,
    166,
    587,
    67,
    1456,
    1099,
    1037,
    1037,
    /*  2020 */ 186,
    187,
    1456,
    250,
    1037,
    1456,
    161,
    1037,
    466,
    149,
    /*  2030 */ 1520,
    587,
    587,
    101,
    1037,
    1519,
    151,
    587,
    167,
    470,
    /*  2040 */ 587,
    1456,
    102,
    232,
    103,
    1456,
    1456,
    587,
    1423,
    104,
    /*  2050 */ 1037,
    1456,
    156,
    208,
    1037,
    105,
    160,
    30,
    1421,
    1037,
    /*  2060 */ 1037,
    180,
    162,
    587,
    64,
    561,
    344,
    587,
    1456,
    1456,
    /*  2070 */ 198,
    205,
    587,
    587,
    1456,
    1420,
    1037,
    1456,
    157,
    1037,
    /*  2080 */ 1037,
    152,
    158,
    1037,
    1456,
    147,
    347,
    478,
    1037,
    587,
    /*  2090 */ 128,
    511,
    587,
    587,
    256,
    60,
    587,
    1637,
    481,
    258,
    /*  2100 */ 1456,
    587,
    409,
    1452,
    1456,
    1451,
    488,
    1424,
    32,
    1456,
    /*  2110 */ 1456,
    220,
    212,
    502,
    412,
    362,
    261,
    54,
    1583,
    507,
    /*  2120 */ 285,
    262,
    1338,
    365,
    263,
    414,
    1456,
    527,
    1395,
    1456,
    /*  2130 */ 1456,
    1394,
    1393,
    1456,
    443,
    1382,
    56,
    415,
    1456,
    1365,
    /*  2140 */ 951,
    1364,
    1358,
    1359,
    380,
    1381,
    1357,
    1356,
    12,
    529,
    /*  2150 */ 276,
    1538,
    61,
    387,
    227,
    537,
    1537,
    390,
    277,
    10,
    /*  2160 */ 446,
    1675,
    1674,
    1312,
    397,
    447,
    391,
    321,
    449,
    73,
    /*  2170 */ 322,
    1490,
    402,
    1491,
    323,
    581,
    246,
    422,
    1689,
    1603,
    /*  2180 */ 190,
    173,
    203,
    423,
    1604,
    1602,
    1601,
    191,
    311,
    237,
    /*  2190 */ 238,
    43,
    192,
    1218,
    229,
    460,
    1216,
    1191,
    1189,
    332,
    /*  2200 */ 29,
    336,
    168,
    1087,
    342,
    204,
    194,
    252,
    14,
    1123,
    /*  2210 */ 257,
    353,
    206,
    1177,
    255,
    195,
    196,
    433,
    350,
    207,
    /*  2220 */ 435,
    209,
    50,
    51,
    52,
    53,
    1182,
    259,
    260,
    1176,
    /*  2230 */ 197,
    181,
    33,
    1167,
    275,
    213,
    1173,
    506,
    107,
    171,
    /*  2240 */ 510,
    382,
    1223,
    264,
    214,
    514,
    55,
    16,
    519,
    17,
    /*  2250 */ 374,
    949,
    522,
    962,
    377,
    57,
    312,
    199,
    200,
    1161,
    /*  2260 */ 1156,
    163,
    287,
    289,
    274,
    18,
    215,
    69,
    97,
    1248,
    /*  2270 */ 244,
    65,
    544,
    539,
    182,
    217,
    545,
    546,
    66,
    548,
    /*  2280 */ 1274,
    19,
    20,
    21,
    1260,
    1264,
    7,
    1262,
    1268,
    67,
    /*  2290 */ 22,
    1267,
    898,
    893,
    892,
    23,
    70,
    992,
    68,
    24,
    /*  2300 */ 71,
    912,
    568,
    570,
    27,
    75,
    280,
    1080,
    28,
    26,
    /*  2310 */ 1279,
    1279,
    35,
    986,
    1480,
    36,
    889,
    1279,
    888,
    1279,
    /*  2320 */ 1279,
    886,
    1279,
    281,
    1279,
    282,
    1279,
    867,
    877,
    873,
    /*  2330 */ 94,
    866,
};
static const YYCODETYPE yy_lookahead[] = {
    /*     0 */ 193, 208, 243, 193, 193, 5,   224, 214, 215, 210,
    /*    10 */ 211, 212, 230, 206, 243, 15,  206, 206, 210, 211,
    /*    20 */ 212, 206, 22,  23,  24,  25,  206, 27,  28,  29,
    /*    30 */ 30,  31,  32,  33,  34,  35,  36,  37,  38,  232,
    /*    40 */ 233, 40,  227, 228, 243, 22,  23,  24,  25,  242,
    /*    50 */ 27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    /*    60 */ 37,  38,  193, 255, 195, 210, 211, 212, 206, 206,
    /*    70 */ 35,  36,  37,  38,  39,  206, 53,  311, 312, 272,
    /*    80 */ 14,  261, 272, 272, 285, 208, 199, 288, 206, 279,
    /*    90 */ 280, 214, 215, 93,  94,  95,  96,  97,  98,  99,
    /*   100 */ 100, 101, 102, 103, 104, 306, 307, 206, 107, 108,
    /*   110 */ 109, 45,  243, 104, 306, 307, 93,  94,  95,  96,
    /*   120 */ 97,  98,  99,  100, 101, 102, 103, 104, 93,  94,
    /*   130 */ 95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    /*   140 */ 232, 118, 22,  23,  24,  25,  264, 27,  28,  29,
    /*   150 */ 30,  31,  32,  33,  34,  35,  36,  37,  38,  206,
    /*   160 */ 99,  306, 307, 92,  22,  23,  24,  25,  305, 27,
    /*   170 */ 28,  29,  30,  31,  32,  33,  34,  35,  36,  37,
    /*   180 */ 38,  304, 121, 193, 113, 195, 115, 126, 314, 118,
    /*   190 */ 321, 290, 323, 51,  320, 75,  206, 77,  93,  94,
    /*   200 */ 95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    /*   210 */ 199, 324, 325, 93,  94,  95,  96,  97,  98,  99,
    /*   220 */ 100, 101, 102, 103, 104, 97,  98,  99,  100, 101,
    /*   230 */ 102, 103, 104, 243, 202, 93,  94,  95,  96,  97,
    /*   240 */ 98,  99,  100, 101, 102, 103, 104, 22,  23,  24,
    /*   250 */ 25,  206, 27,  28,  29,  30,  31,  32,  33,  34,
    /*   260 */ 35,  36,  37,  38,  99,  100, 101, 102, 103, 104,
    /*   270 */ 317, 318, 22,  23,  24,  25,  206, 27,  28,  29,
    /*   280 */ 30,  31,  32,  33,  34,  35,  36,  37,  38,  40,
    /*   290 */ 222, 206, 224, 18,  226, 22,  23,  24,  25,  206,
    /*   300 */ 27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    /*   310 */ 37,  38,  322, 323, 102, 103, 104, 247, 93,  94,
    /*   320 */ 95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    /*   330 */ 298, 299, 141, 142, 249, 324, 325, 252, 253, 210,
    /*   340 */ 211, 212, 117, 93,  94,  95,  96,  97,  98,  99,
    /*   350 */ 100, 101, 102, 103, 104, 206, 107, 108, 109, 140,
    /*   360 */ 141, 142, 210, 211, 212, 206, 93,  94,  95,  96,
    /*   370 */ 97,  98,  99,  100, 101, 102, 103, 104, 86,  97,
    /*   380 */ 98,  58,  133, 290, 135, 136, 137, 228, 115, 210,
    /*   390 */ 211, 212, 110, 111, 145, 22,  23,  24,  25,  149,
    /*   400 */ 27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    /*   410 */ 37,  38,  210, 211, 212, 40,  141, 142, 22,  23,
    /*   420 */ 24,  25,  27,  27,  28,  29,  30,  31,  32,  33,
    /*   430 */ 34,  35,  36,  37,  38,  306, 307, 40,  140, 141,
    /*   440 */ 142, 22,  23,  24,  25,  70,  27,  28,  29,  30,
    /*   450 */ 31,  32,  33,  34,  35,  36,  37,  38,  306, 307,
    /*   460 */ 168, 169, 40,  140, 141, 142, 93,  94,  95,  96,
    /*   470 */ 97,  98,  99,  100, 101, 102, 103, 104, 56,  210,
    /*   480 */ 211, 212, 107, 108, 109, 306, 307, 40,  115, 93,
    /*   490 */ 94,  95,  96,  97,  98,  99,  100, 101, 102, 103,
    /*   500 */ 104, 106, 14,  56,  107, 108, 109, 19,  306, 307,
    /*   510 */ 113, 115, 93,  94,  95,  96,  97,  98,  99,  100,
    /*   520 */ 101, 102, 103, 104, 206, 27,  28,  29,  30,  107,
    /*   530 */ 108, 109, 40,  45,  115, 22,  23,  24,  25,  193,
    /*   540 */ 27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    /*   550 */ 37,  38,  206, 202, 107, 108, 109, 121, 22,  23,
    /*   560 */ 24,  25,  126, 27,  28,  29,  30,  31,  32,  33,
    /*   570 */ 34,  35,  36,  37,  38,  306, 307, 120, 206, 122,
    /*   580 */ 123, 22,  23,  24,  25,  206, 27,  28,  29,  30,
    /*   590 */ 31,  32,  33,  34,  35,  36,  37,  38,  206, 107,
    /*   600 */ 108, 109, 192, 205, 194, 113, 93,  94,  95,  96,
    /*   610 */ 97,  98,  99,  100, 101, 102, 103, 104, 272, 247,
    /*   620 */ 210, 211, 212, 125, 211, 279, 61,  41,  115, 93,
    /*   630 */ 94,  95,  96,  97,  98,  99,  100, 101, 102, 103,
    /*   640 */ 104, 76,  24,  78,  92,  235, 81,  237, 269, 298,
    /*   650 */ 299, 115, 93,  94,  95,  96,  97,  98,  99,  100,
    /*   660 */ 101, 102, 103, 104, 254, 113, 208, 115, 168, 169,
    /*   670 */ 118, 40,  214, 215, 115, 22,  23,  24,  25,  114,
    /*   680 */ 27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    /*   690 */ 37,  38,  40,  206, 296, 40,  206, 206, 22,  23,
    /*   700 */ 24,  25,  244, 27,  28,  29,  30,  31,  32,  33,
    /*   710 */ 34,  35,  36,  37,  38,  40,  306, 307, 308, 306,
    /*   720 */ 307, 22,  23,  24,  25,  107, 27,  28,  29,  30,
    /*   730 */ 31,  32,  33,  34,  35,  36,  37,  38,  107, 108,
    /*   740 */ 109, 206, 40,  157, 113, 206, 93,  94,  95,  96,
    /*   750 */ 97,  98,  99,  100, 101, 102, 103, 104, 118, 107,
    /*   760 */ 108, 109, 107, 108, 109, 113, 40,  127, 115, 93,
    /*   770 */ 94,  95,  96,  97,  98,  99,  100, 101, 102, 103,
    /*   780 */ 104, 206, 107, 108, 109, 298, 299, 300, 113, 298,
    /*   790 */ 299, 300, 93,  94,  95,  96,  97,  98,  99,  100,
    /*   800 */ 101, 102, 103, 104, 210, 211, 130, 69,  269, 107,
    /*   810 */ 108, 109, 206, 206, 115, 22,  23,  24,  25,  164,
    /*   820 */ 27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    /*   830 */ 37,  38,  206, 92,  108, 206, 206, 22,  23,  24,
    /*   840 */ 25,  206, 27,  28,  29,  30,  31,  32,  33,  34,
    /*   850 */ 35,  36,  37,  38,  113, 249, 115, 89,  252, 253,
    /*   860 */ 206, 40,  22,  23,  24,  25,  164, 27,  28,  29,
    /*   870 */ 30,  31,  32,  33,  34,  35,  36,  37,  38,  249,
    /*   880 */ 3,   223, 252, 253, 7,   147, 93,  94,  95,  96,
    /*   890 */ 97,  98,  99,  100, 101, 102, 103, 104, 269, 161,
    /*   900 */ 306, 307, 118, 249, 210, 211, 252, 253, 93,  94,
    /*   910 */ 95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    /*   920 */ 99,  314, 154, 130, 47,  290, 319, 320, 107, 108,
    /*   930 */ 109, 147, 117, 93,  94,  95,  96,  97,  98,  99,
    /*   940 */ 100, 101, 102, 103, 104, 161, 22,  23,  24,  25,
    /*   950 */ 24,  27,  28,  29,  30,  31,  32,  33,  34,  35,
    /*   960 */ 36,  37,  38,  23,  24,  25,  206, 27,  28,  29,
    /*   970 */ 30,  31,  32,  33,  34,  35,  36,  37,  38,  24,
    /*   980 */ 25,  206, 27,  28,  29,  30,  31,  32,  33,  34,
    /*   990 */ 35,  36,  37,  38,  24,  193, 193, 195, 268, 193,
    /*  1000 */ 306, 307, 11,  58,  35,  36,  37,  38,  206, 206,
    /*  1010 */ 200, 201, 206, 87,  69,  24,  286, 93,  94,  95,
    /*  1020 */ 96,  97,  98,  99,  100, 101, 102, 103, 104, 69,
    /*  1030 */ 65,  40,  67,  93,  94,  95,  96,  97,  98,  99,
    /*  1040 */ 100, 101, 102, 103, 104, 243, 55,  24,  93,  94,
    /*  1050 */ 95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    /*  1060 */ 69,  11,  93,  94,  95,  96,  97,  98,  99,  100,
    /*  1070 */ 101, 102, 103, 104, 24,  272, 106, 107, 272, 29,
    /*  1080 */ 206, 90,  193, 40,  195, 140, 206, 198, 97,  98,
    /*  1090 */ 40,  199, 147, 318, 134, 206, 105, 132, 107, 108,
    /*  1100 */ 109, 110, 111, 314, 113, 55,  161, 147, 133, 320,
    /*  1110 */ 135, 136, 137, 114, 193, 124, 125, 118, 248, 128,
    /*  1120 */ 145, 161, 40,  321, 74,  323, 193, 206, 195, 79,
    /*  1130 */ 107, 198, 243, 90,  206, 165, 11,  267, 147, 206,
    /*  1140 */ 90,  261, 151, 152, 153, 206, 90,  97,  98,  24,
    /*  1150 */ 107, 108, 161, 110, 111, 105, 92,  107, 108, 109,
    /*  1160 */ 110, 111, 206, 113, 108, 40,  110, 111, 125, 193,
    /*  1170 */ 193, 195, 195, 197, 124, 125, 243, 113, 128, 115,
    /*  1180 */ 55,  125, 206, 206, 147, 206, 210, 211, 212, 107,
    /*  1190 */ 108, 109, 264, 272, 151, 152, 97,  98,  161, 74,
    /*  1200 */ 40,  151, 152, 153, 79,  249, 206, 151, 152, 253,
    /*  1210 */ 206, 11,  113, 193, 193, 90,  324, 325, 118, 243,
    /*  1220 */ 243, 206, 97,  98,  24,  206, 206, 206, 146, 65,
    /*  1230 */ 105, 67,  107, 108, 109, 110, 111, 248, 113, 139,
    /*  1240 */ 40,  189, 190, 191, 192, 193, 194, 271, 271, 124,
    /*  1250 */ 125, 114, 193, 128, 195, 55,  267, 97,  98,  262,
    /*  1260 */ 1,   261, 210, 211, 212, 206, 106, 107, 108, 109,
    /*  1270 */ 110, 111, 206, 206, 74,  216, 151, 152, 153, 79,
    /*  1280 */ 21,  284, 306, 307, 269, 255, 193, 235, 195, 237,
    /*  1290 */ 90,  198, 272, 272, 134, 61,  132, 97,  98,  206,
    /*  1300 */ 279, 141, 243, 44,  167, 105, 254, 107, 108, 109,
    /*  1310 */ 110, 111, 78,  113, 0,   81,  206, 58,  4,   155,
    /*  1320 */ 6,   62,  8,   40,  124, 125, 238, 193, 128, 206,
    /*  1330 */ 16,  264, 18,  314, 20,  269, 243, 11,  319, 320,
    /*  1340 */ 206, 142, 193, 193, 195, 195, 197, 206, 198, 150,
    /*  1350 */ 24,  151, 152, 153, 302, 206, 206, 142, 306, 307,
    /*  1360 */ 308, 193, 193, 208, 195, 150, 40,  198, 54,  214,
    /*  1370 */ 215, 57,  199, 206, 206, 206, 62,  63,  206, 291,
    /*  1380 */ 66,  55,  193, 69,  195, 200, 201, 198, 193, 301,
    /*  1390 */ 195, 108, 243, 243, 193, 206, 195, 314, 197, 244,
    /*  1400 */ 117, 206, 88,  320, 206, 264, 272, 206, 240, 109,
    /*  1410 */ 242, 206, 243, 279, 11,  115, 90,  193, 118, 195,
    /*  1420 */ 271, 40,  198, 97,  98,  113, 112, 24,  193, 117,
    /*  1430 */ 206, 105, 243, 107, 108, 109, 110, 111, 243, 113,
    /*  1440 */ 272, 206, 61,  40,  243, 206, 205, 11,  206, 208,
    /*  1450 */ 124, 125, 126, 5,   128, 214, 215, 76,  55,  78,
    /*  1460 */ 24,  147, 81,  15,  164, 42,  271, 243, 154, 193,
    /*  1470 */ 156, 195, 271, 159, 198, 161, 40,  151, 152, 153,
    /*  1480 */ 166, 64,  206, 193, 24,  68,  206, 27,  107, 108,
    /*  1490 */ 109, 55,  193, 90,  195, 114, 206, 324, 325, 206,
    /*  1500 */ 97,  98,  193, 113, 195, 206, 197, 272, 105, 119,
    /*  1510 */ 107, 108, 109, 110, 111, 206, 113, 314, 314, 243,
    /*  1520 */ 223, 309, 310, 320, 320, 40,  90,  124, 125, 126,
    /*  1530 */ 109, 128, 242, 97,  98,  99,  113, 193, 193, 118,
    /*  1540 */ 206, 105, 243, 107, 108, 109, 110, 111, 257, 113,
    /*  1550 */ 206, 206, 243, 136, 151, 152, 153, 11,  10,  268,
    /*  1560 */ 124, 125, 272, 193, 128, 4,   106, 6,   115, 8,
    /*  1570 */ 24,  118, 155, 223, 61,  106, 206, 16,  206, 18,
    /*  1580 */ 271, 20,  193, 118, 195, 164, 40,  151, 152, 153,
    /*  1590 */ 193, 78,  195, 108, 81,  206, 48,  206, 113, 40,
    /*  1600 */ 257, 55,  117, 206, 144, 1,   285, 59,  193, 288,
    /*  1610 */ 195, 268, 206, 148, 205, 54,  272, 272, 57,  203,
    /*  1620 */ 204, 206, 287, 62,  63,  21,  193, 66,  195, 281,
    /*  1630 */ 69,  118, 243, 193, 165, 195, 90,  203, 204, 206,
    /*  1640 */ 243, 206, 272, 97,  98,  106, 206, 206, 44,  88,
    /*  1650 */ 193, 105, 195, 107, 108, 109, 110, 111, 243, 113,
    /*  1660 */ 271, 24,  206, 206, 203, 204, 62,  108, 271, 118,
    /*  1670 */ 124, 125, 113, 112, 128, 4,   243, 6,   127, 8,
    /*  1680 */ 117, 48,  119, 243, 203, 204, 271, 16,  149, 18,
    /*  1690 */ 193, 143, 191, 192, 193, 194, 316, 151, 152, 153,
    /*  1700 */ 243, 292, 206, 206, 271, 296, 206, 74,  147, 203,
    /*  1710 */ 204, 210, 211, 212, 125, 154, 83,  156, 148, 149,
    /*  1720 */ 159, 84,  161, 134, 206, 54,  193, 166, 57,  193,
    /*  1730 */ 129, 195, 131, 62,  63,  34,  235, 66,  237, 206,
    /*  1740 */ 69,  104, 206, 193, 125, 195, 114, 115, 193, 116,
    /*  1750 */ 195, 193, 275, 195, 117, 254, 206, 72,  73,  88,
    /*  1760 */ 267, 206, 125, 144, 206, 210, 211, 212, 40,  272,
    /*  1770 */ 133, 134, 135, 136, 137, 138, 279, 293, 193, 243,
    /*  1780 */ 195, 144, 40,  193, 193, 195, 195, 110, 111, 115,
    /*  1790 */ 40,  206, 118, 243, 110, 111, 206, 206, 243, 289,
    /*  1800 */ 219, 243, 193, 302, 195, 272, 281, 306, 307, 308,
    /*  1810 */ 115, 281, 279, 118, 113, 206, 281, 115, 147, 281,
    /*  1820 */ 118, 207, 193, 196, 195, 154, 271, 156, 243, 193,
    /*  1830 */ 159, 195, 161, 243, 243, 206, 108, 166, 193, 259,
    /*  1840 */ 195, 258, 206, 193, 193, 195, 195, 193, 270, 195,
    /*  1850 */ 108, 206, 243, 193, 278, 195, 206, 206, 108, 265,
    /*  1860 */ 206, 306, 307, 258, 297, 193, 206, 195, 193, 193,
    /*  1870 */ 195, 195, 243, 193, 259, 195, 259, 115, 206, 243,
    /*  1880 */ 118, 206, 206, 193, 259, 195, 206, 193, 243, 195,
    /*  1890 */ 270, 265, 115, 243, 243, 118, 206, 243, 220, 115,
    /*  1900 */ 206, 115, 118, 243, 118, 115, 244, 193, 118, 195,
    /*  1910 */ 297, 193, 246, 195, 193, 243, 195, 236, 243, 243,
    /*  1920 */ 206, 222, 265, 243, 206, 286, 115, 206, 193, 118,
    /*  1930 */ 195, 262, 193, 243, 195, 286, 193, 243, 195, 286,
    /*  1940 */ 244, 206, 193, 115, 195, 206, 118, 286, 115, 206,
    /*  1950 */ 193, 118, 195, 262, 244, 206, 193, 243, 195, 218,
    /*  1960 */ 193, 243, 195, 206, 243, 196, 193, 115, 195, 206,
    /*  1970 */ 118, 13,  193, 206, 195, 193, 146, 195, 243, 206,
    /*  1980 */ 209, 119, 243, 209, 115, 206, 243, 118, 206, 303,
    /*  1990 */ 303, 193, 243, 195, 305, 193, 193, 195, 195, 115,
    /*  2000 */ 243, 193, 118, 195, 206, 209, 243, 63,  206, 206,
    /*  2010 */ 243, 277, 115, 277, 206, 118, 243, 41,  193, 193,
    /*  2020 */ 195, 195, 243, 160, 193, 243, 195, 193, 276, 195,
    /*  2030 */ 276, 206, 206, 278, 193, 276, 195, 206, 278, 91,
    /*  2040 */ 206, 243, 274, 149, 274, 243, 243, 206, 260, 274,
    /*  2050 */ 193, 243, 195, 113, 193, 274, 195, 266, 263, 193,
    /*  2060 */ 193, 195, 195, 206, 163, 114, 262, 206, 243, 243,
    /*  2070 */ 22,  256, 206, 206, 243, 263, 193, 243, 195, 193,
    /*  2080 */ 193, 195, 195, 193, 243, 195, 262, 209, 193, 206,
    /*  2090 */ 195, 91,  206, 206, 239, 148, 206, 270, 209, 239,
    /*  2100 */ 243, 206, 270, 260, 243, 260, 270, 256, 266, 243,
    /*  2110 */ 243, 113, 256, 245, 270, 209, 239, 129, 295, 43,
    /*  2120 */ 209, 239, 209, 294, 239, 245, 243, 106, 229, 243,
    /*  2130 */ 243, 229, 229, 243, 46,  225, 113, 245, 243, 234,
    /*  2140 */ 139, 234, 220, 229, 229, 225, 229, 229, 118, 241,
    /*  2150 */ 209, 263, 162, 262, 283, 116, 263, 282, 80,  113,
    /*  2160 */ 71,  315, 315, 213, 209, 104, 270, 251, 117, 129,
    /*  2170 */ 251, 273, 245, 273, 217, 250, 168, 313, 325, 205,
    /*  2180 */ 231, 221, 310, 313, 205, 205, 205, 231, 221, 219,
    /*  2190 */ 219, 205, 231, 49,  113, 50,  112, 115, 115, 157,
    /*  2200 */ 132, 158, 157, 124, 147, 146, 143, 117, 113, 118,
    /*  2210 */ 106, 155, 127, 112, 165, 143, 143, 42,  132, 127,
    /*  2220 */ 12,  146, 34,  34,  34,  34,  107, 9,   119, 112,
    /*  2230 */ 143, 8,   117, 52,  118, 52,  60,  17,  106, 119,
    /*  2240 */ 24,  144, 124, 138, 113, 51,  113, 113, 51,  113,
    /*  2250 */ 115, 40,  85,  2,   117, 113, 51,  12,  118, 107,
    /*  2260 */ 164, 115, 115, 115, 9,   9,   113, 118, 113, 115,
    /*  2270 */ 119, 9,   114, 117, 115, 118, 113, 116, 148, 113,
    /*  2280 */ 115, 9,   9,   9,   77,  60,  23,  75,  60,  118,
    /*  2290 */ 9,   82,  115, 115, 115, 113, 127, 87,  118, 113,
    /*  2300 */ 127, 18,  118, 118, 9,   118, 113, 115, 9,   113,
    /*  2310 */ 326, 326, 113, 115, 119, 113, 115, 326, 115, 326,
    /*  2320 */ 326, 121, 326, 119, 326, 119, 326, 112, 115, 115,
    /*  2330 */ 113, 112, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2340 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2350 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2360 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2370 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2380 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2390 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2400 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2410 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2420 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2430 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2440 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2450 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2460 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2470 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2480 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2490 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2500 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2510 */ 326, 326, 326, 326, 326, 326, 326, 326, 326, 326,
    /*  2520 */ 326,
};
#define YY_SHIFT_COUNT (594)
#define YY_SHIFT_MIN (0)
#define YY_SHIFT_MAX (2299)
static const unsigned short int yy_shift_ofst[] = {
    /*     0 */ 1561,
    1314,
    991,
    1671,
    991,
    738,
    1050,
    1125,
    1200,
    1546,
    /*    10 */ 1546,
    1546,
    249,
    945,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    /*    20 */ 1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1,
    /*    30 */ 960,
    1,
    738,
    738,
    738,
    738,
    738,
    0,
    0,
    142,
    /*    40 */ 840,
    1326,
    1403,
    1436,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    /*    50 */ 1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    /*    60 */ 1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    /*    70 */ 1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    /*    80 */ 1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    1546,
    /*    90 */ 1546,
    1546,
    1546,
    1546,
    1546,
    1160,
    1160,
    1381,
    1381,
    422,
    /*   100 */ 447,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    784,
    /*   110 */ 784,
    784,
    784,
    23,
    120,
    225,
    250,
    273,
    373,
    396,
    /*   120 */ 419,
    513,
    536,
    559,
    653,
    676,
    699,
    793,
    815,
    840,
    /*   130 */ 840,
    840,
    840,
    840,
    840,
    840,
    840,
    840,
    840,
    840,
    /*   140 */ 840,
    840,
    840,
    840,
    840,
    840,
    840,
    924,
    840,
    940,
    /*   150 */ 955,
    955,
    35,
    969,
    969,
    969,
    969,
    969,
    969,
    969,
    /*   160 */ 105,
    128,
    165,
    375,
    1,
    1,
    1,
    1,
    1,
    1,
    /*   170 */ 1,
    1,
    1417,
    1460,
    1,
    1,
    1,
    282,
    282,
    292,
    /*   180 */ 212,
    191,
    500,
    500,
    500,
    1037,
    9,
    9,
    2332,
    2332,
    /*   190 */ 1637,
    1637,
    1637,
    1082,
    397,
    397,
    397,
    397,
    1259,
    1259,
    /*   200 */ 375,
    565,
    71,
    552,
    1,
    1,
    1,
    1,
    1,
    1,
    /*   210 */ 1,
    1,
    1,
    1,
    1,
    1,
    970,
    1,
    1513,
    1513,
    /*   220 */ 1,
    275,
    1234,
    1234,
    618,
    618,
    726,
    1137,
    726,
    2332,
    /*   230 */ 2332,
    2332,
    2332,
    2332,
    2332,
    2332,
    1043,
    1056,
    1056,
    492,
    /*   240 */ 975,
    631,
    655,
    702,
    821,
    652,
    675,
    1,
    1,
    1,
    /*   250 */ 1,
    1,
    1,
    1,
    323,
    1,
    1,
    1,
    1,
    1,
    /*   260 */ 1,
    1,
    1,
    1,
    1,
    1,
    1548,
    1548,
    1548,
    1,
    /*   270 */ 1,
    1,
    1,
    1300,
    1,
    1,
    1485,
    1633,
    1,
    1,
    /*   280 */ 1604,
    1,
    1,
    488,
    219,
    877,
    1099,
    1283,
    1283,
    1283,
    /*   290 */ 1421,
    1283,
    457,
    457,
    61,
    457,
    586,
    1539,
    1570,
    640,
    /*   300 */ 1570,
    1023,
    1465,
    1539,
    1539,
    1465,
    1539,
    640,
    1023,
    1100,
    /*   310 */ 1453,
    395,
    1448,
    1312,
    1469,
    1469,
    1469,
    1469,
    1551,
    999,
    /*   320 */ 999,
    1448,
    1448,
    1601,
    436,
    1830,
    1958,
    1958,
    1862,
    1862,
    /*   330 */ 1862,
    1944,
    1944,
    1863,
    1863,
    1863,
    1976,
    1976,
    1948,
    1948,
    /*   340 */ 1948,
    1948,
    1894,
    1940,
    1901,
    1951,
    2048,
    1901,
    1951,
    1862,
    /*   350 */ 2000,
    1947,
    1862,
    2000,
    1947,
    1894,
    1894,
    1947,
    1940,
    2048,
    /*   360 */ 1947,
    2048,
    1998,
    1862,
    2000,
    1988,
    2076,
    1862,
    2000,
    1862,
    /*   370 */ 2000,
    1998,
    2021,
    2021,
    2021,
    2088,
    2023,
    2023,
    1998,
    2021,
    /*   380 */ 2001,
    2021,
    2088,
    2021,
    2021,
    2030,
    1862,
    1901,
    1951,
    1901,
    /*   390 */ 1990,
    2039,
    1947,
    2078,
    2078,
    2089,
    2089,
    2046,
    1862,
    2061,
    /*   400 */ 2061,
    2040,
    2051,
    1998,
    2008,
    2332,
    2332,
    2332,
    2332,
    2332,
    /*   410 */ 2332,
    2332,
    2332,
    2332,
    2332,
    2332,
    2332,
    2332,
    2332,
    2332,
    /*   420 */ 498,
    1164,
    741,
    1064,
    965,
    298,
    1559,
    1674,
    1701,
    768,
    /*   430 */ 1563,
    1695,
    1199,
    1215,
    1702,
    1423,
    1762,
    1777,
    1784,
    1786,
    /*   440 */ 1790,
    1728,
    1589,
    66,
    1619,
    1811,
    1685,
    1742,
    1828,
    926,
    /*   450 */ 1833,
    1852,
    1869,
    1750,
    1677,
    1684,
    1884,
    1897,
    1632,
    1390,
    /*   460 */ 2144,
    2145,
    2081,
    2084,
    2082,
    2083,
    2042,
    2043,
    2045,
    2068,
    /*   470 */ 2079,
    2057,
    2059,
    2063,
    2090,
    2091,
    2091,
    2085,
    2049,
    2086,
    /*   480 */ 2095,
    2104,
    2056,
    2101,
    2092,
    2072,
    2091,
    2073,
    2175,
    2208,
    /*   490 */ 2091,
    2075,
    2188,
    2189,
    2190,
    2191,
    2087,
    2119,
    2218,
    2109,
    /*   500 */ 2117,
    2223,
    2115,
    2181,
    2116,
    2183,
    2176,
    2220,
    2120,
    2132,
    /*   510 */ 2118,
    2216,
    2097,
    2105,
    2131,
    2194,
    2133,
    2134,
    2135,
    2136,
    /*   520 */ 2197,
    2211,
    2137,
    2167,
    2251,
    2142,
    2205,
    2245,
    2140,
    2146,
    /*   530 */ 2147,
    2148,
    2152,
    2255,
    2153,
    2096,
    2149,
    2256,
    2154,
    2155,
    /*   540 */ 2156,
    2157,
    2151,
    2159,
    2262,
    2158,
    2163,
    2161,
    2130,
    2166,
    /*   550 */ 2165,
    2272,
    2273,
    2274,
    2207,
    2225,
    2212,
    2263,
    2228,
    2209,
    /*   560 */ 2171,
    2281,
    2177,
    2149,
    2178,
    2179,
    2180,
    2210,
    2182,
    2184,
    /*   570 */ 2186,
    2185,
    2169,
    2173,
    2187,
    2192,
    2193,
    2195,
    2283,
    2196,
    /*   580 */ 2198,
    2199,
    2200,
    2201,
    2202,
    2203,
    2204,
    2206,
    2213,
    2214,
    /*   590 */ 2217,
    2295,
    2299,
    2215,
    2219,
};
#define YY_REDUCE_COUNT (419)
#define YY_REDUCE_MIN (-241)
#define YY_REDUCE_MAX (1986)
static const short yy_reduce_ofst[] = {
    /*     0 */ 1052,
    1501,
    976,
    410,
    1555,
    -201,
    -131,
    -10,
    802,
    1149,
    /*    10 */ 1201,
    1309,
    -193,
    -192,
    889,
    933,
    1093,
    1150,
    977,
    1169,
    /*    20 */ 1189,
    1195,
    1224,
    1389,
    1397,
    1059,
    1415,
    1433,
    1276,
    -190,
    /*    30 */ -145,
    1168,
    129,
    152,
    179,
    202,
    269,
    458,
    1155,
    -123,
    /*    40 */ 1241,
    1299,
    1440,
    1457,
    1536,
    1550,
    1558,
    1585,
    1590,
    1591,
    /*    50 */ 1609,
    1629,
    1636,
    1645,
    1650,
    1651,
    1654,
    1660,
    1672,
    1675,
    /*    60 */ 1676,
    1680,
    1690,
    1694,
    1714,
    1718,
    1721,
    1735,
    1739,
    1743,
    /*    70 */ 1749,
    1757,
    1763,
    1767,
    1773,
    1779,
    1782,
    1798,
    1802,
    1803,
    /*    80 */ 1808,
    1825,
    1826,
    1831,
    1834,
    1841,
    1857,
    1861,
    1866,
    1867,
    /*    90 */ 1883,
    1886,
    1887,
    1890,
    1895,
    487,
    491,
    607,
    1019,
    85,
    /*   100 */ 606,
    1290,
    346,
    1021,
    1134,
    1497,
    630,
    1533,
    654,
    594,
    /*   110 */ 694,
    594,
    694,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    /*   120 */ -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    /*   130 */ -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    /*   140 */ -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    /*   150 */ -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    -207,
    /*   160 */ -207,
    -207,
    -207,
    -185,
    -189,
    803,
    806,
    921,
    1020,
    1235,
    /*   170 */ 1344,
    1345,
    1088,
    68,
    -47,
    1370,
    956,
    32,
    351,
    -113,
    /*   180 */ -207,
    1409,
    11,
    892,
    1173,
    413,
    -207,
    -207,
    -207,
    -207,
    /*   190 */ -218,
    -218,
    -218,
    -137,
    -118,
    928,
    1067,
    1141,
    870,
    989,
    /*   200 */ 159,
    -126,
    -234,
    -234,
    -180,
    880,
    1000,
    -99,
    379,
    93,
    /*   210 */ 539,
    629,
    635,
    1015,
    70,
    1066,
    730,
    775,
    789,
    1083,
    /*   220 */ 372,
    398,
    1203,
    1204,
    1291,
    1343,
    810,
    997,
    1185,
    1212,
    /*   230 */ 1416,
    1434,
    1461,
    1481,
    1321,
    1506,
    -241,
    -229,
    -199,
    -138,
    /*   240 */ -92,
    45,
    149,
    318,
    392,
    490,
    535,
    575,
    626,
    760,
    /*   250 */ 874,
    939,
    979,
    1004,
    1030,
    1110,
    1123,
    1167,
    1172,
    1198,
    /*   260 */ 1205,
    1239,
    1242,
    1280,
    1293,
    1334,
    658,
    1297,
    1350,
    1372,
    /*   270 */ 1391,
    1406,
    1435,
    1335,
    1441,
    1456,
    1348,
    1380,
    1496,
    1500,
    /*   280 */ 1493,
    1518,
    392,
    1477,
    1484,
    1510,
    1581,
    1525,
    1530,
    1535,
    /*   290 */ 1335,
    1538,
    1614,
    1614,
    1627,
    1614,
    1576,
    1580,
    1583,
    1594,
    /*   300 */ 1605,
    1567,
    1578,
    1615,
    1617,
    1620,
    1625,
    1626,
    1613,
    1678,
    /*   310 */ 1666,
    1699,
    1662,
    1681,
    1639,
    1649,
    1653,
    1661,
    1657,
    1669,
    /*   320 */ 1691,
    1696,
    1710,
    1741,
    1769,
    1689,
    1686,
    1687,
    1771,
    1774,
    /*   330 */ 1796,
    1734,
    1736,
    1752,
    1754,
    1759,
    1755,
    1760,
    1768,
    1770,
    /*   340 */ 1775,
    1781,
    1788,
    1791,
    1795,
    1804,
    1815,
    1812,
    1824,
    1878,
    /*   350 */ 1855,
    1827,
    1889,
    1860,
    1832,
    1843,
    1845,
    1836,
    1842,
    1851,
    /*   360 */ 1844,
    1856,
    1868,
    1906,
    1877,
    1823,
    1829,
    1911,
    1882,
    1913,
    /*   370 */ 1885,
    1880,
    1899,
    1902,
    1903,
    1910,
    1905,
    1907,
    1892,
    1914,
    /*   380 */ 1922,
    1915,
    1920,
    1917,
    1918,
    1908,
    1941,
    1888,
    1891,
    1893,
    /*   390 */ 1871,
    1875,
    1896,
    1846,
    1847,
    1898,
    1900,
    1950,
    1955,
    1916,
    /*   400 */ 1919,
    1957,
    1925,
    1927,
    1853,
    1864,
    1870,
    1872,
    1949,
    1974,
    /*   410 */ 1979,
    1980,
    1981,
    1956,
    1960,
    1967,
    1970,
    1971,
    1961,
    1986,
};
static const YYACTIONTYPE yy_default[] = {
    /*     0 */ 1417,
    1417,
    1471,
    1417,
    1277,
    1565,
    1277,
    1277,
    1277,
    1471,
    /*    10 */ 1471,
    1471,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*    20 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1336,
    1277,
    1277,
    1277,
    1277,
    /*    30 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1494,
    1494,
    1628,
    /*    40 */ 1543,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*    50 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*    60 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*    70 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*    80 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*    90 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1673,
    1673,
    1277,
    /*   100 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1419,
    /*   110 */ 1418,
    1277,
    1277,
    1561,
    1277,
    1277,
    1438,
    1277,
    1277,
    1277,
    /*   120 */ 1277,
    1277,
    1277,
    1472,
    1473,
    1277,
    1277,
    1277,
    1277,
    1632,
    /*   130 */ 1625,
    1629,
    1444,
    1443,
    1442,
    1441,
    1593,
    1575,
    1553,
    1557,
    /*   140 */ 1563,
    1562,
    1472,
    1332,
    1333,
    1331,
    1335,
    1277,
    1473,
    1463,
    /*   150 */ 1469,
    1462,
    1328,
    1322,
    1321,
    1320,
    1461,
    1329,
    1325,
    1319,
    /*   160 */ 1460,
    1464,
    1458,
    1341,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*   170 */ 1277,
    1277,
    1645,
    1397,
    1277,
    1277,
    1277,
    1277,
    1277,
    1475,
    /*   180 */ 1459,
    1543,
    1476,
    1289,
    1287,
    1277,
    1466,
    1465,
    1468,
    1467,
    /*   190 */ 1514,
    1347,
    1346,
    1633,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*   200 */ 1277,
    1673,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*   210 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1577,
    1277,
    1673,
    1673,
    /*   220 */ 1277,
    1543,
    1673,
    1673,
    1435,
    1435,
    1292,
    1558,
    1292,
    1656,
    /*   230 */ 1542,
    1542,
    1542,
    1542,
    1565,
    1542,
    1277,
    1277,
    1277,
    1277,
    /*   240 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1622,
    1620,
    1277,
    /*   250 */ 1527,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*   260 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*   270 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1546,
    1277,
    1277,
    1277,
    /*   280 */ 1277,
    1277,
    1277,
    1522,
    1277,
    1586,
    1401,
    1546,
    1546,
    1546,
    /*   290 */ 1551,
    1546,
    1403,
    1402,
    1549,
    1535,
    1516,
    1447,
    1437,
    1550,
    /*   300 */ 1437,
    1598,
    1552,
    1447,
    1447,
    1552,
    1447,
    1550,
    1598,
    1368,
    /*   310 */ 1391,
    1361,
    1494,
    1277,
    1577,
    1577,
    1577,
    1577,
    1550,
    1558,
    /*   320 */ 1558,
    1494,
    1494,
    1334,
    1549,
    1633,
    1627,
    1627,
    1313,
    1313,
    /*   330 */ 1313,
    1530,
    1530,
    1526,
    1526,
    1526,
    1516,
    1516,
    1506,
    1506,
    /*   340 */ 1506,
    1506,
    1454,
    1445,
    1560,
    1558,
    1426,
    1560,
    1558,
    1313,
    /*   350 */ 1640,
    1552,
    1313,
    1640,
    1552,
    1454,
    1454,
    1552,
    1445,
    1426,
    /*   360 */ 1552,
    1426,
    1411,
    1313,
    1640,
    1592,
    1590,
    1313,
    1640,
    1313,
    /*   370 */ 1640,
    1411,
    1399,
    1399,
    1399,
    1383,
    1277,
    1277,
    1411,
    1399,
    /*   380 */ 1368,
    1399,
    1383,
    1399,
    1399,
    1386,
    1313,
    1560,
    1558,
    1560,
    /*   390 */ 1556,
    1554,
    1552,
    1683,
    1683,
    1497,
    1497,
    1315,
    1313,
    1415,
    /*   400 */ 1415,
    1277,
    1277,
    1411,
    1691,
    1661,
    1661,
    1656,
    1349,
    1543,
    /*   410 */ 1543,
    1543,
    1543,
    1349,
    1370,
    1370,
    1401,
    1401,
    1349,
    1543,
    /*   420 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1293,
    1277,
    1605,
    1515,
    /*   430 */ 1431,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*   440 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1408,
    /*   450 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1299,
    /*   460 */ 1277,
    1635,
    1651,
    1277,
    1277,
    1277,
    1521,
    1277,
    1277,
    1277,
    /*   470 */ 1277,
    1277,
    1277,
    1277,
    1432,
    1439,
    1440,
    1277,
    1277,
    1277,
    /*   480 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1453,
    1277,
    1277,
    1277,
    /*   490 */ 1448,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1596,
    /*   500 */ 1277,
    1277,
    1277,
    1277,
    1589,
    1588,
    1277,
    1277,
    1503,
    1277,
    /*   510 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*   520 */ 1277,
    1366,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1339,
    1277,
    /*   530 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1555,
    1277,
    1277,
    1277,
    /*   540 */ 1277,
    1688,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*   550 */ 1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    1277,
    /*   560 */ 1559,
    1277,
    1277,
    1470,
    1277,
    1277,
    1277,
    1277,
    1277,
    1650,
    /*   570 */ 1277,
    1649,
    1277,
    1277,
    1277,
    1277,
    1277,
    1484,
    1277,
    1277,
    /*   580 */ 1277,
    1277,
    1303,
    1277,
    1277,
    1277,
    1300,
    1277,
    1277,
    1277,
    /*   590 */ 1277,
    1277,
    1277,
    1277,
    1277,
};
/********** End of lemon-generated parsing tables *****************************/

/* The next table maps tokens (terminal symbols) into fallback tokens.
** If a construct like the following:
**
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
**
** This feature can be used, for example, to cause some keywords in a language
** to revert to identifiers if they keyword does not apply in the context where
** it appears.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
    0,  /*          $ => nothing */
    40, /*      ABORT => ID */
    40, /*     ACTION => ID */
    40, /*      AFTER => ID */
    40, /*    ANALYZE => ID */
    40, /*        ASC => ID */
    40, /*     ATTACH => ID */
    40, /*     BEFORE => ID */
    40, /*      BEGIN => ID */
    40, /*         BY => ID */
    40, /*    CASCADE => ID */
    40, /*       CAST => ID */
    40, /*   CONFLICT => ID */
    40, /*   DATABASE => ID */
    40, /*   DEFERRED => ID */
    40, /*       DESC => ID */
    40, /*     DETACH => ID */
    40, /*       EACH => ID */
    40, /*        END => ID */
    40, /*  EXCLUSIVE => ID */
    40, /*    EXPLAIN => ID */
    40, /*       FAIL => ID */
    0,  /*         OR => nothing */
    0,  /*        AND => nothing */
    0,  /*        NOT => nothing */
    0,  /*         IS => nothing */
    0,  /*      ISNOT => nothing */
    40, /*      MATCH => ID */
    40, /*    LIKE_KW => ID */
    0,  /*    BETWEEN => nothing */
    0,  /*         IN => nothing */
    0,  /*     ISNULL => nothing */
    0,  /*    NOTNULL => nothing */
    0,  /*         NE => nothing */
    0,  /*         EQ => nothing */
    0,  /*         GT => nothing */
    0,  /*         LE => nothing */
    0,  /*         LT => nothing */
    0,  /*         GE => nothing */
    0,  /*     ESCAPE => nothing */
    0,  /*         ID => nothing */
    40, /*   COLUMNKW => ID */
    40, /*         DO => ID */
    40, /*        FOR => ID */
    40, /*     IGNORE => ID */
    40, /*  IMMEDIATE => ID */
    40, /*  INITIALLY => ID */
    40, /*    INSTEAD => ID */
    40, /*         NO => ID */
    40, /*       PLAN => ID */
    40, /*      QUERY => ID */
    40, /*        KEY => ID */
    40, /*         OF => ID */
    40, /*     OFFSET => ID */
    40, /*     PRAGMA => ID */
    40, /*      RAISE => ID */
    40, /*  RECURSIVE => ID */
    40, /*    RELEASE => ID */
    40, /*    REPLACE => ID */
    40, /*   RESTRICT => ID */
    40, /*        ROW => ID */
    40, /*       ROWS => ID */
    40, /*   ROLLBACK => ID */
    40, /*  SAVEPOINT => ID */
    40, /*       TEMP => ID */
    40, /*    TRIGGER => ID */
    40, /*     VACUUM => ID */
    40, /*       VIEW => ID */
    40, /*    VIRTUAL => ID */
    40, /*       WITH => ID */
    40, /*    WITHOUT => ID */
    40, /*      NULLS => ID */
    40, /*      FIRST => ID */
    40, /*       LAST => ID */
    40, /*    CURRENT => ID */
    40, /*  FOLLOWING => ID */
    40, /*  PARTITION => ID */
    40, /*  PRECEDING => ID */
    40, /*      RANGE => ID */
    40, /*  UNBOUNDED => ID */
    40, /*    EXCLUDE => ID */
    40, /*     GROUPS => ID */
    40, /*     OTHERS => ID */
    40, /*       TIES => ID */
    40, /*  GENERATED => ID */
    40, /*     ALWAYS => ID */
    40, /*     WITHIN => ID */
    40, /* MATERIALIZED => ID */
    40, /*    REINDEX => ID */
    40, /*     RENAME => ID */
    40, /*   CTIME_KW => ID */
    40, /*         IF => ID */
    0,  /*        ANY => nothing */
    0,  /*     BITAND => nothing */
    0,  /*      BITOR => nothing */
    0,  /*     LSHIFT => nothing */
    0,  /*     RSHIFT => nothing */
    0,  /*       PLUS => nothing */
    0,  /*      MINUS => nothing */
    0,  /*       STAR => nothing */
    0,  /*      SLASH => nothing */
    0,  /*        REM => nothing */
    0,  /*     CONCAT => nothing */
    0,  /*        PTR => nothing */
    0,  /*    COLLATE => nothing */
    0,  /*     BITNOT => nothing */
    0,  /*         ON => nothing */
    0,  /*    INDEXED => nothing */
    0,  /*     STRING => nothing */
    0,  /*    JOIN_KW => nothing */
    0,  /*    INTEGER => nothing */
    0,  /*      FLOAT => nothing */
    0,  /*       SEMI => nothing */
    0,  /*         LP => nothing */
    0,  /*      ORDER => nothing */
    0,  /*         RP => nothing */
    0,  /*      GROUP => nothing */
    0,  /*         AS => nothing */
    0,  /*      COMMA => nothing */
    0,  /*        DOT => nothing */
    0,  /*      UNION => nothing */
    0,  /*        ALL => nothing */
    0,  /*     EXCEPT => nothing */
    0,  /*  INTERSECT => nothing */
    0,  /*     EXISTS => nothing */
    0,  /*       NULL => nothing */
    0,  /*   DISTINCT => nothing */
    0,  /*       FROM => nothing */
    0,  /*       CASE => nothing */
    0,  /*       WHEN => nothing */
    0,  /*       THEN => nothing */
    0,  /*       ELSE => nothing */
    0,  /*      TABLE => nothing */
    0,  /* CONSTRAINT => nothing */
    0,  /*    DEFAULT => nothing */
    0,  /*    PRIMARY => nothing */
    0,  /*     UNIQUE => nothing */
    0,  /*      CHECK => nothing */
    0,  /* REFERENCES => nothing */
    0,  /*   AUTOINCR => nothing */
    0,  /*     INSERT => nothing */
    0,  /*     DELETE => nothing */
    0,  /*     UPDATE => nothing */
    0,  /*        SET => nothing */
    0,  /* DEFERRABLE => nothing */
    0,  /*    FOREIGN => nothing */
    0,  /*       INTO => nothing */
    0,  /*     VALUES => nothing */
    0,  /*      WHERE => nothing */
    0,  /*  RETURNING => nothing */
    0,  /*    NOTHING => nothing */
    0,  /*       BLOB => nothing */
    0,  /*    QNUMBER => nothing */
    0,  /*   VARIABLE => nothing */
    0,  /*       DROP => nothing */
    0,  /*      INDEX => nothing */
    0,  /*      ALTER => nothing */
    0,  /*         TO => nothing */
    0,  /*        ADD => nothing */
    0,  /*     COMMIT => nothing */
    0,  /* TRANSACTION => nothing */
    0,  /*     SELECT => nothing */
    0,  /*     HAVING => nothing */
    0,  /*      LIMIT => nothing */
    0,  /*       JOIN => nothing */
    0,  /*      USING => nothing */
    0,  /*     CREATE => nothing */
    0,  /*     WINDOW => nothing */
    0,  /*       OVER => nothing */
    0,  /*     FILTER => nothing */
    0,  /*     COLUMN => nothing */
    0,  /* AGG_FUNCTION => nothing */
    0,  /* AGG_COLUMN => nothing */
    0,  /*  TRUEFALSE => nothing */
    0,  /*   FUNCTION => nothing */
    0,  /*      UPLUS => nothing */
    0,  /*     UMINUS => nothing */
    0,  /*      TRUTH => nothing */
    0,  /*   REGISTER => nothing */
    0,  /*     VECTOR => nothing */
    0,  /* SELECT_COLUMN => nothing */
    0,  /* IF_NULL_ROW => nothing */
    0,  /*   ASTERISK => nothing */
    0,  /*       SPAN => nothing */
    0,  /*      ERROR => nothing */
    0,  /*      SPACE => nothing */
    0,  /*    COMMENT => nothing */
    0,  /*    ILLEGAL => nothing */
    0,  /*       BANG => nothing */
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
**
** After the "shift" half of a SHIFTREDUCE action, the stateno field
** actually contains the reduce action for the second half of the
** SHIFTREDUCE.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno; /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;     /* The major token value.  This is the code
                        ** number for the token at this stack level */
  YYMINORTYPE minor;    /* The user-supplied minor token value.  This
                        ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  yyStackEntry* yytos; /* Pointer to top element of the stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyhwm; /* High-water mark of the stack */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt; /* Shifts left before out of the error */
#endif
  SynqSqliteParseARG_SDECL           /* A place to hold %extra_argument */
      SynqSqliteParseCTX_SDECL       /* A place to hold %extra_context */
      yyStackEntry* yystackEnd;      /* Last entry in the stack */
  yyStackEntry* yystack;             /* The parser stack */
  yyStackEntry yystk0[YYSTACKDEPTH]; /* Initial stack space */
};
typedef struct yyParser yyParser;

#include <assert.h>
#ifndef NDEBUG
#include <stdio.h>

#include "syntaqlite_sqlite/sqlite_tokens.h"

#include "syntaqlite_dialect/dialect_macros.h"

#include "syntaqlite_dialect/extent_hooks.h"
static FILE* yyTraceFILE = 0;
static char* yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/*
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void SynqSqliteParseTrace(FILE* TraceFILE, char* zTracePrompt) {
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if (yyTraceFILE == 0)
    yyTracePrompt = 0;
  else if (yyTracePrompt == 0)
    yyTraceFILE = 0;
}
#endif /* NDEBUG */

#if defined(YYCOVERAGE) || !defined(NDEBUG)
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char* const yyTokenName[] = {
    /*    0 */ "$",
    /*    1 */ "ABORT",
    /*    2 */ "ACTION",
    /*    3 */ "AFTER",
    /*    4 */ "ANALYZE",
    /*    5 */ "ASC",
    /*    6 */ "ATTACH",
    /*    7 */ "BEFORE",
    /*    8 */ "BEGIN",
    /*    9 */ "BY",
    /*   10 */ "CASCADE",
    /*   11 */ "CAST",
    /*   12 */ "CONFLICT",
    /*   13 */ "DATABASE",
    /*   14 */ "DEFERRED",
    /*   15 */ "DESC",
    /*   16 */ "DETACH",
    /*   17 */ "EACH",
    /*   18 */ "END",
    /*   19 */ "EXCLUSIVE",
    /*   20 */ "EXPLAIN",
    /*   21 */ "FAIL",
    /*   22 */ "OR",
    /*   23 */ "AND",
    /*   24 */ "NOT",
    /*   25 */ "IS",
    /*   26 */ "ISNOT",
    /*   27 */ "MATCH",
    /*   28 */ "LIKE_KW",
    /*   29 */ "BETWEEN",
    /*   30 */ "IN",
    /*   31 */ "ISNULL",
    /*   32 */ "NOTNULL",
    /*   33 */ "NE",
    /*   34 */ "EQ",
    /*   35 */ "GT",
    /*   36 */ "LE",
    /*   37 */ "LT",
    /*   38 */ "GE",
    /*   39 */ "ESCAPE",
    /*   40 */ "ID",
    /*   41 */ "COLUMNKW",
    /*   42 */ "DO",
    /*   43 */ "FOR",
    /*   44 */ "IGNORE",
    /*   45 */ "IMMEDIATE",
    /*   46 */ "INITIALLY",
    /*   47 */ "INSTEAD",
    /*   48 */ "NO",
    /*   49 */ "PLAN",
    /*   50 */ "QUERY",
    /*   51 */ "KEY",
    /*   52 */ "OF",
    /*   53 */ "OFFSET",
    /*   54 */ "PRAGMA",
    /*   55 */ "RAISE",
    /*   56 */ "RECURSIVE",
    /*   57 */ "RELEASE",
    /*   58 */ "REPLACE",
    /*   59 */ "RESTRICT",
    /*   60 */ "ROW",
    /*   61 */ "ROWS",
    /*   62 */ "ROLLBACK",
    /*   63 */ "SAVEPOINT",
    /*   64 */ "TEMP",
    /*   65 */ "TRIGGER",
    /*   66 */ "VACUUM",
    /*   67 */ "VIEW",
    /*   68 */ "VIRTUAL",
    /*   69 */ "WITH",
    /*   70 */ "WITHOUT",
    /*   71 */ "NULLS",
    /*   72 */ "FIRST",
    /*   73 */ "LAST",
    /*   74 */ "CURRENT",
    /*   75 */ "FOLLOWING",
    /*   76 */ "PARTITION",
    /*   77 */ "PRECEDING",
    /*   78 */ "RANGE",
    /*   79 */ "UNBOUNDED",
    /*   80 */ "EXCLUDE",
    /*   81 */ "GROUPS",
    /*   82 */ "OTHERS",
    /*   83 */ "TIES",
    /*   84 */ "GENERATED",
    /*   85 */ "ALWAYS",
    /*   86 */ "WITHIN",
    /*   87 */ "MATERIALIZED",
    /*   88 */ "REINDEX",
    /*   89 */ "RENAME",
    /*   90 */ "CTIME_KW",
    /*   91 */ "IF",
    /*   92 */ "ANY",
    /*   93 */ "BITAND",
    /*   94 */ "BITOR",
    /*   95 */ "LSHIFT",
    /*   96 */ "RSHIFT",
    /*   97 */ "PLUS",
    /*   98 */ "MINUS",
    /*   99 */ "STAR",
    /*  100 */ "SLASH",
    /*  101 */ "REM",
    /*  102 */ "CONCAT",
    /*  103 */ "PTR",
    /*  104 */ "COLLATE",
    /*  105 */ "BITNOT",
    /*  106 */ "ON",
    /*  107 */ "INDEXED",
    /*  108 */ "STRING",
    /*  109 */ "JOIN_KW",
    /*  110 */ "INTEGER",
    /*  111 */ "FLOAT",
    /*  112 */ "SEMI",
    /*  113 */ "LP",
    /*  114 */ "ORDER",
    /*  115 */ "RP",
    /*  116 */ "GROUP",
    /*  117 */ "AS",
    /*  118 */ "COMMA",
    /*  119 */ "DOT",
    /*  120 */ "UNION",
    /*  121 */ "ALL",
    /*  122 */ "EXCEPT",
    /*  123 */ "INTERSECT",
    /*  124 */ "EXISTS",
    /*  125 */ "NULL",
    /*  126 */ "DISTINCT",
    /*  127 */ "FROM",
    /*  128 */ "CASE",
    /*  129 */ "WHEN",
    /*  130 */ "THEN",
    /*  131 */ "ELSE",
    /*  132 */ "TABLE",
    /*  133 */ "CONSTRAINT",
    /*  134 */ "DEFAULT",
    /*  135 */ "PRIMARY",
    /*  136 */ "UNIQUE",
    /*  137 */ "CHECK",
    /*  138 */ "REFERENCES",
    /*  139 */ "AUTOINCR",
    /*  140 */ "INSERT",
    /*  141 */ "DELETE",
    /*  142 */ "UPDATE",
    /*  143 */ "SET",
    /*  144 */ "DEFERRABLE",
    /*  145 */ "FOREIGN",
    /*  146 */ "INTO",
    /*  147 */ "VALUES",
    /*  148 */ "WHERE",
    /*  149 */ "RETURNING",
    /*  150 */ "NOTHING",
    /*  151 */ "BLOB",
    /*  152 */ "QNUMBER",
    /*  153 */ "VARIABLE",
    /*  154 */ "DROP",
    /*  155 */ "INDEX",
    /*  156 */ "ALTER",
    /*  157 */ "TO",
    /*  158 */ "ADD",
    /*  159 */ "COMMIT",
    /*  160 */ "TRANSACTION",
    /*  161 */ "SELECT",
    /*  162 */ "HAVING",
    /*  163 */ "LIMIT",
    /*  164 */ "JOIN",
    /*  165 */ "USING",
    /*  166 */ "CREATE",
    /*  167 */ "WINDOW",
    /*  168 */ "OVER",
    /*  169 */ "FILTER",
    /*  170 */ "COLUMN",
    /*  171 */ "AGG_FUNCTION",
    /*  172 */ "AGG_COLUMN",
    /*  173 */ "TRUEFALSE",
    /*  174 */ "FUNCTION",
    /*  175 */ "UPLUS",
    /*  176 */ "UMINUS",
    /*  177 */ "TRUTH",
    /*  178 */ "REGISTER",
    /*  179 */ "VECTOR",
    /*  180 */ "SELECT_COLUMN",
    /*  181 */ "IF_NULL_ROW",
    /*  182 */ "ASTERISK",
    /*  183 */ "SPAN",
    /*  184 */ "ERROR",
    /*  185 */ "SPACE",
    /*  186 */ "COMMENT",
    /*  187 */ "ILLEGAL",
    /*  188 */ "BANG",
    /*  189 */ "input",
    /*  190 */ "cmdlist",
    /*  191 */ "ecmd",
    /*  192 */ "cmdx",
    /*  193 */ "error",
    /*  194 */ "cmd",
    /*  195 */ "expr",
    /*  196 */ "distinct",
    /*  197 */ "exprlist",
    /*  198 */ "sortlist",
    /*  199 */ "filter_over",
    /*  200 */ "typetoken",
    /*  201 */ "typename",
    /*  202 */ "signed",
    /*  203 */ "selcollist",
    /*  204 */ "sclp",
    /*  205 */ "scanpt",
    /*  206 */ "nm",
    /*  207 */ "multiselect_op",
    /*  208 */ "in_op",
    /*  209 */ "dbnm",
    /*  210 */ "selectnowith",
    /*  211 */ "oneselect",
    /*  212 */ "select",
    /*  213 */ "paren_exprlist",
    /*  214 */ "likeop",
    /*  215 */ "between_op",
    /*  216 */ "case_operand",
    /*  217 */ "case_exprlist",
    /*  218 */ "case_else",
    /*  219 */ "scantok",
    /*  220 */ "autoinc",
    /*  221 */ "refargs",
    /*  222 */ "refarg",
    /*  223 */ "refact",
    /*  224 */ "defer_subclause",
    /*  225 */ "init_deferred_pred_opt",
    /*  226 */ "defer_subclause_opt",
    /*  227 */ "table_option_set",
    /*  228 */ "table_option",
    /*  229 */ "onconf",
    /*  230 */ "ccons",
    /*  231 */ "carglist",
    /*  232 */ "tcons",
    /*  233 */ "conslist",
    /*  234 */ "generated",
    /*  235 */ "create_table",
    /*  236 */ "create_table_args",
    /*  237 */ "createkw",
    /*  238 */ "temp",
    /*  239 */ "ifnotexists",
    /*  240 */ "columnlist",
    /*  241 */ "conslist_opt",
    /*  242 */ "columnname",
    /*  243 */ "term",
    /*  244 */ "sortorder",
    /*  245 */ "eidlist_opt",
    /*  246 */ "tconscomma",
    /*  247 */ "eidlist",
    /*  248 */ "resolvetype",
    /*  249 */ "withnm",
    /*  250 */ "wqas",
    /*  251 */ "collate",
    /*  252 */ "wqlist",
    /*  253 */ "wqitem",
    /*  254 */ "with",
    /*  255 */ "insert_cmd",
    /*  256 */ "orconf",
    /*  257 */ "indexed_opt",
    /*  258 */ "where_opt_ret",
    /*  259 */ "upsert",
    /*  260 */ "returning",
    /*  261 */ "xfullname",
    /*  262 */ "orderby_opt",
    /*  263 */ "limit_opt",
    /*  264 */ "setlist",
    /*  265 */ "from",
    /*  266 */ "idlist_opt",
    /*  267 */ "raisetype",
    /*  268 */ "indexed_by",
    /*  269 */ "idlist",
    /*  270 */ "where_opt",
    /*  271 */ "nexprlist",
    /*  272 */ "nmorerr",
    /*  273 */ "nulls",
    /*  274 */ "ifexists",
    /*  275 */ "transtype",
    /*  276 */ "trans_opt",
    /*  277 */ "savepoint_opt",
    /*  278 */ "kwcolumn_opt",
    /*  279 */ "fullname",
    /*  280 */ "add_column_fullname",
    /*  281 */ "as",
    /*  282 */ "groupby_opt",
    /*  283 */ "having_opt",
    /*  284 */ "window_clause",
    /*  285 */ "seltablist",
    /*  286 */ "on_using",
    /*  287 */ "joinop",
    /*  288 */ "stl_prefix",
    /*  289 */ "trigger_time",
    /*  290 */ "trnm",
    /*  291 */ "trigger_decl",
    /*  292 */ "trigger_cmd_list",
    /*  293 */ "trigger_event",
    /*  294 */ "foreach_clause",
    /*  295 */ "when_clause",
    /*  296 */ "trigger_cmd",
    /*  297 */ "tridxby",
    /*  298 */ "plus_num",
    /*  299 */ "minus_num",
    /*  300 */ "nmnum",
    /*  301 */ "uniqueflag",
    /*  302 */ "explain",
    /*  303 */ "database_kw_opt",
    /*  304 */ "key_opt",
    /*  305 */ "vinto",
    /*  306 */ "values",
    /*  307 */ "mvalues",
    /*  308 */ "create_vtab",
    /*  309 */ "vtabarglist",
    /*  310 */ "vtabarg",
    /*  311 */ "vtabargtoken",
    /*  312 */ "lp",
    /*  313 */ "anylist",
    /*  314 */ "range_or_rows",
    /*  315 */ "frame_exclude_opt",
    /*  316 */ "frame_exclude",
    /*  317 */ "windowdefn_list",
    /*  318 */ "windowdefn",
    /*  319 */ "window",
    /*  320 */ "frame_opt",
    /*  321 */ "frame_bound_s",
    /*  322 */ "frame_bound_e",
    /*  323 */ "frame_bound",
    /*  324 */ "filter_clause",
    /*  325 */ "over_clause",
};
#endif /* defined(YYCOVERAGE) || !defined(NDEBUG) */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
 */
static const char* const yyRuleName[] = {
    /*   0 */ "input ::= cmdlist",
    /*   1 */ "cmdlist ::= cmdlist ecmd",
    /*   2 */ "cmdlist ::= ecmd",
    /*   3 */ "ecmd ::= SEMI",
    /*   4 */ "ecmd ::= cmdx SEMI",
    /*   5 */ "ecmd ::= error SEMI",
    /*   6 */ "cmdx ::= cmd",
    /*   7 */
    "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP",
    /*   8 */
    "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP "
    "filter_over",
    /*   9 */
    "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP WITHIN GROUP LP ORDER "
    "BY expr RP",
    /*  10 */
    "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP WITHIN GROUP LP ORDER "
    "BY expr RP filter_over",
    /*  11 */ "expr ::= CAST LP expr AS typetoken RP",
    /*  12 */ "typetoken ::=",
    /*  13 */ "typetoken ::= typename",
    /*  14 */ "typetoken ::= typename LP signed RP",
    /*  15 */ "typetoken ::= typename LP signed COMMA signed RP",
    /*  16 */ "typename ::= ID|STRING",
    /*  17 */ "typename ::= typename ID|STRING",
    /*  18 */ "selcollist ::= sclp scanpt nm DOT STAR",
    /*  19 */ "expr ::= ID|INDEXED|JOIN_KW",
    /*  20 */ "expr ::= nm DOT nm",
    /*  21 */ "expr ::= nm DOT nm DOT nm",
    /*  22 */ "selectnowith ::= selectnowith multiselect_op oneselect",
    /*  23 */ "multiselect_op ::= UNION",
    /*  24 */ "multiselect_op ::= UNION ALL",
    /*  25 */ "multiselect_op ::= EXCEPT|INTERSECT",
    /*  26 */ "expr ::= LP select RP",
    /*  27 */ "expr ::= EXISTS LP select RP",
    /*  28 */ "in_op ::= IN",
    /*  29 */ "in_op ::= NOT IN",
    /*  30 */ "expr ::= expr in_op LP exprlist RP",
    /*  31 */ "expr ::= expr in_op LP select RP",
    /*  32 */ "expr ::= expr in_op nm dbnm paren_exprlist",
    /*  33 */ "dbnm ::=",
    /*  34 */ "dbnm ::= DOT nm",
    /*  35 */ "paren_exprlist ::=",
    /*  36 */ "paren_exprlist ::= LP exprlist RP",
    /*  37 */ "expr ::= expr ISNULL|NOTNULL",
    /*  38 */ "expr ::= expr NOT NULL",
    /*  39 */ "expr ::= expr IS expr",
    /*  40 */ "expr ::= expr IS NOT expr",
    /*  41 */ "expr ::= expr IS NOT DISTINCT FROM expr",
    /*  42 */ "expr ::= expr IS DISTINCT FROM expr",
    /*  43 */ "between_op ::= BETWEEN",
    /*  44 */ "between_op ::= NOT BETWEEN",
    /*  45 */ "expr ::= expr between_op expr AND expr",
    /*  46 */ "likeop ::= LIKE_KW|MATCH",
    /*  47 */ "likeop ::= NOT LIKE_KW|MATCH",
    /*  48 */ "expr ::= expr likeop expr",
    /*  49 */ "expr ::= expr likeop expr ESCAPE expr",
    /*  50 */ "expr ::= CASE case_operand case_exprlist case_else END",
    /*  51 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
    /*  52 */ "case_exprlist ::= WHEN expr THEN expr",
    /*  53 */ "case_else ::= ELSE expr",
    /*  54 */ "case_else ::=",
    /*  55 */ "case_operand ::= expr",
    /*  56 */ "case_operand ::=",
    /*  57 */ "cmd ::= create_table create_table_args",
    /*  58 */ "create_table ::= createkw temp TABLE ifnotexists nm dbnm",
    /*  59 */
    "create_table_args ::= LP columnlist conslist_opt RP table_option_set",
    /*  60 */ "create_table_args ::= AS select",
    /*  61 */ "table_option_set ::=",
    /*  62 */ "table_option_set ::= table_option",
    /*  63 */ "table_option_set ::= table_option_set COMMA table_option",
    /*  64 */ "table_option ::= WITHOUT nm",
    /*  65 */ "table_option ::= nm",
    /*  66 */ "columnlist ::= columnlist COMMA columnname carglist",
    /*  67 */ "columnlist ::= columnname carglist",
    /*  68 */ "carglist ::= carglist ccons",
    /*  69 */ "carglist ::=",
    /*  70 */ "ccons ::= CONSTRAINT nm",
    /*  71 */ "ccons ::= DEFAULT scantok term",
    /*  72 */ "ccons ::= DEFAULT LP expr RP",
    /*  73 */ "ccons ::= DEFAULT PLUS scantok term",
    /*  74 */ "ccons ::= DEFAULT MINUS scantok term",
    /*  75 */ "ccons ::= DEFAULT scantok ID|INDEXED",
    /*  76 */ "ccons ::= NULL onconf",
    /*  77 */ "ccons ::= NOT NULL onconf",
    /*  78 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
    /*  79 */ "ccons ::= UNIQUE onconf",
    /*  80 */ "ccons ::= CHECK LP expr RP",
    /*  81 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
    /*  82 */ "ccons ::= defer_subclause",
    /*  83 */ "ccons ::= COLLATE ID|STRING",
    /*  84 */ "ccons ::= GENERATED ALWAYS AS generated",
    /*  85 */ "ccons ::= AS generated",
    /*  86 */ "generated ::= LP expr RP",
    /*  87 */ "generated ::= LP expr RP ID",
    /*  88 */ "autoinc ::=",
    /*  89 */ "autoinc ::= AUTOINCR",
    /*  90 */ "refargs ::=",
    /*  91 */ "refargs ::= refargs refarg",
    /*  92 */ "refarg ::= MATCH nm",
    /*  93 */ "refarg ::= ON INSERT refact",
    /*  94 */ "refarg ::= ON DELETE refact",
    /*  95 */ "refarg ::= ON UPDATE refact",
    /*  96 */ "refact ::= SET NULL",
    /*  97 */ "refact ::= SET DEFAULT",
    /*  98 */ "refact ::= CASCADE",
    /*  99 */ "refact ::= RESTRICT",
    /* 100 */ "refact ::= NO ACTION",
    /* 101 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
    /* 102 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
    /* 103 */ "init_deferred_pred_opt ::=",
    /* 104 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
    /* 105 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
    /* 106 */ "conslist_opt ::=",
    /* 107 */ "conslist_opt ::= COMMA conslist",
    /* 108 */ "conslist ::= conslist tconscomma tcons",
    /* 109 */ "conslist ::= tcons",
    /* 110 */ "tconscomma ::= COMMA",
    /* 111 */ "tconscomma ::=",
    /* 112 */ "tcons ::= CONSTRAINT nm",
    /* 113 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf",
    /* 114 */ "tcons ::= UNIQUE LP sortlist RP onconf",
    /* 115 */ "tcons ::= CHECK LP expr RP onconf",
    /* 116 */
    "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs "
    "defer_subclause_opt",
    /* 117 */ "defer_subclause_opt ::=",
    /* 118 */ "defer_subclause_opt ::= defer_subclause",
    /* 119 */ "onconf ::=",
    /* 120 */ "onconf ::= ON CONFLICT resolvetype",
    /* 121 */ "scantok ::=",
    /* 122 */ "select ::= WITH wqlist selectnowith",
    /* 123 */ "select ::= WITH RECURSIVE wqlist selectnowith",
    /* 124 */ "wqitem ::= withnm eidlist_opt wqas LP select RP",
    /* 125 */ "wqlist ::= wqitem",
    /* 126 */ "wqlist ::= wqlist COMMA wqitem",
    /* 127 */ "withnm ::= nm",
    /* 128 */ "wqas ::= AS",
    /* 129 */ "wqas ::= AS MATERIALIZED",
    /* 130 */ "wqas ::= AS NOT MATERIALIZED",
    /* 131 */ "eidlist_opt ::=",
    /* 132 */ "eidlist_opt ::= LP eidlist RP",
    /* 133 */ "eidlist ::= nm collate sortorder",
    /* 134 */ "eidlist ::= eidlist COMMA nm collate sortorder",
    /* 135 */ "collate ::=",
    /* 136 */ "collate ::= COLLATE ID|STRING",
    /* 137 */ "with ::=",
    /* 138 */ "with ::= WITH wqlist",
    /* 139 */ "with ::= WITH RECURSIVE wqlist",
    /* 140 */
    "cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret orderby_opt "
    "limit_opt",
    /* 141 */
    "cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from "
    "where_opt_ret orderby_opt limit_opt",
    /* 142 */ "cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert",
    /* 143 */
    "cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES "
    "returning",
    /* 144 */ "insert_cmd ::= INSERT orconf",
    /* 145 */ "insert_cmd ::= REPLACE",
    /* 146 */ "orconf ::=",
    /* 147 */ "orconf ::= OR resolvetype",
    /* 148 */ "resolvetype ::= raisetype",
    /* 149 */ "resolvetype ::= IGNORE",
    /* 150 */ "resolvetype ::= REPLACE",
    /* 151 */ "xfullname ::= nm",
    /* 152 */ "xfullname ::= nm DOT nm",
    /* 153 */ "xfullname ::= nm DOT nm AS nm",
    /* 154 */ "xfullname ::= nm AS nm",
    /* 155 */ "indexed_opt ::=",
    /* 156 */ "indexed_opt ::= indexed_by",
    /* 157 */ "where_opt_ret ::=",
    /* 158 */ "where_opt_ret ::= WHERE expr",
    /* 159 */ "where_opt_ret ::= RETURNING selcollist",
    /* 160 */ "where_opt_ret ::= WHERE expr RETURNING selcollist",
    /* 161 */ "setlist ::= setlist COMMA nm EQ expr",
    /* 162 */ "setlist ::= setlist COMMA LP idlist RP EQ expr",
    /* 163 */ "setlist ::= nm EQ expr",
    /* 164 */ "setlist ::= LP idlist RP EQ expr",
    /* 165 */ "idlist_opt ::=",
    /* 166 */ "idlist_opt ::= LP idlist RP",
    /* 167 */ "upsert ::=",
    /* 168 */ "upsert ::= RETURNING selcollist",
    /* 169 */
    "upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist "
    "where_opt upsert",
    /* 170 */
    "upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert",
    /* 171 */ "upsert ::= ON CONFLICT DO NOTHING returning",
    /* 172 */
    "upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning",
    /* 173 */ "returning ::= RETURNING selcollist",
    /* 174 */ "returning ::=",
    /* 175 */ "expr ::= error",
    /* 176 */ "expr ::= term",
    /* 177 */ "expr ::= LP expr RP",
    /* 178 */ "expr ::= expr PLUS|MINUS expr",
    /* 179 */ "expr ::= expr STAR|SLASH|REM expr",
    /* 180 */ "expr ::= expr LT|GT|GE|LE expr",
    /* 181 */ "expr ::= expr EQ|NE expr",
    /* 182 */ "expr ::= expr AND expr",
    /* 183 */ "expr ::= expr OR expr",
    /* 184 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
    /* 185 */ "expr ::= expr CONCAT expr",
    /* 186 */ "expr ::= expr PTR expr",
    /* 187 */ "expr ::= PLUS|MINUS expr",
    /* 188 */ "expr ::= BITNOT expr",
    /* 189 */ "expr ::= NOT expr",
    /* 190 */ "exprlist ::= nexprlist",
    /* 191 */ "exprlist ::=",
    /* 192 */ "nexprlist ::= nexprlist COMMA expr",
    /* 193 */ "nexprlist ::= expr",
    /* 194 */ "expr ::= LP nexprlist COMMA expr RP",
    /* 195 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP",
    /* 196 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP",
    /* 197 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over",
    /* 198 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over",
    /* 199 */ "nm ::= ID|INDEXED|JOIN_KW",
    /* 200 */ "nm ::= STRING",
    /* 201 */ "nmorerr ::= nm",
    /* 202 */ "nmorerr ::= error",
    /* 203 */ "term ::= INTEGER",
    /* 204 */ "term ::= STRING",
    /* 205 */ "term ::= NULL|FLOAT|BLOB",
    /* 206 */ "term ::= QNUMBER",
    /* 207 */ "term ::= CTIME_KW",
    /* 208 */ "expr ::= VARIABLE",
    /* 209 */ "expr ::= expr COLLATE ID|STRING",
    /* 210 */ "sortlist ::= sortlist COMMA expr sortorder nulls",
    /* 211 */ "sortlist ::= expr sortorder nulls",
    /* 212 */ "sortorder ::= ASC",
    /* 213 */ "sortorder ::= DESC",
    /* 214 */ "sortorder ::=",
    /* 215 */ "nulls ::= NULLS FIRST",
    /* 216 */ "nulls ::= NULLS LAST",
    /* 217 */ "nulls ::=",
    /* 218 */ "expr ::= RAISE LP IGNORE RP",
    /* 219 */ "expr ::= RAISE LP raisetype COMMA expr RP",
    /* 220 */ "raisetype ::= ROLLBACK",
    /* 221 */ "raisetype ::= ABORT",
    /* 222 */ "raisetype ::= FAIL",
    /* 223 */ "fullname ::= nmorerr",
    /* 224 */ "fullname ::= nmorerr DOT nmorerr",
    /* 225 */ "ifexists ::= IF EXISTS",
    /* 226 */ "ifexists ::=",
    /* 227 */ "cmd ::= DROP TABLE ifexists fullname",
    /* 228 */ "cmd ::= DROP VIEW ifexists fullname",
    /* 229 */ "cmd ::= DROP INDEX ifexists fullname",
    /* 230 */ "cmd ::= DROP TRIGGER ifexists fullname",
    /* 231 */ "cmd ::= ALTER TABLE fullname RENAME TO nmorerr",
    /* 232 */
    "cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nmorerr TO nmorerr",
    /* 233 */ "cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nmorerr",
    /* 234 */
    "cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname "
    "carglist",
    /* 235 */ "add_column_fullname ::= fullname",
    /* 236 */ "kwcolumn_opt ::=",
    /* 237 */ "kwcolumn_opt ::= COLUMNKW",
    /* 238 */ "columnname ::= nmorerr typetoken",
    /* 239 */ "cmd ::= BEGIN transtype trans_opt",
    /* 240 */ "cmd ::= COMMIT|END trans_opt",
    /* 241 */ "cmd ::= ROLLBACK trans_opt",
    /* 242 */ "transtype ::=",
    /* 243 */ "transtype ::= DEFERRED",
    /* 244 */ "transtype ::= IMMEDIATE",
    /* 245 */ "transtype ::= EXCLUSIVE",
    /* 246 */ "trans_opt ::=",
    /* 247 */ "trans_opt ::= TRANSACTION",
    /* 248 */ "trans_opt ::= TRANSACTION nm",
    /* 249 */ "savepoint_opt ::= SAVEPOINT",
    /* 250 */ "savepoint_opt ::=",
    /* 251 */ "cmd ::= SAVEPOINT nmorerr",
    /* 252 */ "cmd ::= RELEASE savepoint_opt nmorerr",
    /* 253 */ "cmd ::= ROLLBACK trans_opt TO savepoint_opt nmorerr",
    /* 254 */ "cmd ::= select",
    /* 255 */ "select ::= selectnowith",
    /* 256 */ "selectnowith ::= oneselect",
    /* 257 */
    "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt "
    "having_opt orderby_opt limit_opt",
    /* 258 */
    "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt "
    "having_opt window_clause orderby_opt limit_opt",
    /* 259 */ "selcollist ::= sclp scanpt expr scanpt as",
    /* 260 */ "selcollist ::= sclp scanpt STAR",
    /* 261 */ "sclp ::= selcollist COMMA",
    /* 262 */ "sclp ::=",
    /* 263 */ "scanpt ::=",
    /* 264 */ "as ::= AS nmorerr",
    /* 265 */ "as ::= ID|STRING",
    /* 266 */ "as ::=",
    /* 267 */ "distinct ::= DISTINCT",
    /* 268 */ "distinct ::= ALL",
    /* 269 */ "distinct ::=",
    /* 270 */ "from ::=",
    /* 271 */ "from ::= FROM seltablist",
    /* 272 */ "where_opt ::=",
    /* 273 */ "where_opt ::= WHERE expr",
    /* 274 */ "groupby_opt ::=",
    /* 275 */ "groupby_opt ::= GROUP BY nexprlist",
    /* 276 */ "having_opt ::=",
    /* 277 */ "having_opt ::= HAVING expr",
    /* 278 */ "orderby_opt ::=",
    /* 279 */ "orderby_opt ::= ORDER BY sortlist",
    /* 280 */ "limit_opt ::=",
    /* 281 */ "limit_opt ::= LIMIT expr",
    /* 282 */ "limit_opt ::= LIMIT expr OFFSET expr",
    /* 283 */ "limit_opt ::= LIMIT expr COMMA expr",
    /* 284 */ "stl_prefix ::= seltablist joinop",
    /* 285 */ "stl_prefix ::=",
    /* 286 */ "seltablist ::= stl_prefix nm dbnm as on_using",
    /* 287 */ "seltablist ::= stl_prefix nm dbnm as indexed_by on_using",
    /* 288 */ "seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using",
    /* 289 */ "seltablist ::= stl_prefix LP select RP as on_using",
    /* 290 */ "seltablist ::= stl_prefix LP seltablist RP as on_using",
    /* 291 */ "joinop ::= COMMA|JOIN",
    /* 292 */ "joinop ::= JOIN_KW JOIN",
    /* 293 */ "joinop ::= JOIN_KW nm JOIN",
    /* 294 */ "joinop ::= JOIN_KW nm nm JOIN",
    /* 295 */ "on_using ::= ON expr",
    /* 296 */ "on_using ::= USING LP idlist RP",
    /* 297 */ "on_using ::=",
    /* 298 */ "indexed_by ::= INDEXED BY nm",
    /* 299 */ "indexed_by ::= NOT INDEXED",
    /* 300 */ "idlist ::= idlist COMMA nm",
    /* 301 */ "idlist ::= nm",
    /* 302 */ "cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END",
    /* 303 */
    "trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time "
    "trigger_event ON fullname foreach_clause when_clause",
    /* 304 */ "trigger_time ::= BEFORE|AFTER",
    /* 305 */ "trigger_time ::= INSTEAD OF",
    /* 306 */ "trigger_time ::=",
    /* 307 */ "trigger_event ::= DELETE|INSERT",
    /* 308 */ "trigger_event ::= UPDATE",
    /* 309 */ "trigger_event ::= UPDATE OF idlist",
    /* 310 */ "foreach_clause ::=",
    /* 311 */ "foreach_clause ::= FOR EACH ROW",
    /* 312 */ "when_clause ::=",
    /* 313 */ "when_clause ::= WHEN expr",
    /* 314 */ "trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI",
    /* 315 */ "trigger_cmd_list ::= trigger_cmd SEMI",
    /* 316 */ "trnm ::= nm",
    /* 317 */ "trnm ::= nm DOT nm",
    /* 318 */ "tridxby ::=",
    /* 319 */ "tridxby ::= INDEXED BY nm",
    /* 320 */ "tridxby ::= NOT INDEXED",
    /* 321 */
    "trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt "
    "scanpt",
    /* 322 */
    "trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert "
    "scanpt",
    /* 323 */ "trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt",
    /* 324 */ "trigger_cmd ::= scanpt select scanpt",
    /* 325 */ "cmd ::= PRAGMA nm dbnm",
    /* 326 */ "cmd ::= PRAGMA nm dbnm EQ nmnum",
    /* 327 */ "cmd ::= PRAGMA nm dbnm LP nmnum RP",
    /* 328 */ "cmd ::= PRAGMA nm dbnm EQ minus_num",
    /* 329 */ "cmd ::= PRAGMA nm dbnm LP minus_num RP",
    /* 330 */ "nmnum ::= plus_num",
    /* 331 */ "nmnum ::= nm",
    /* 332 */ "nmnum ::= ON",
    /* 333 */ "nmnum ::= DELETE",
    /* 334 */ "nmnum ::= DEFAULT",
    /* 335 */ "plus_num ::= PLUS INTEGER|FLOAT",
    /* 336 */ "plus_num ::= INTEGER|FLOAT",
    /* 337 */ "minus_num ::= MINUS INTEGER|FLOAT",
    /* 338 */ "signed ::= plus_num",
    /* 339 */ "signed ::= minus_num",
    /* 340 */ "cmd ::= ANALYZE",
    /* 341 */ "cmd ::= ANALYZE nm dbnm",
    /* 342 */ "cmd ::= REINDEX",
    /* 343 */ "cmd ::= REINDEX nm dbnm",
    /* 344 */ "cmd ::= ATTACH database_kw_opt expr AS expr key_opt",
    /* 345 */ "cmd ::= DETACH database_kw_opt expr",
    /* 346 */ "database_kw_opt ::= DATABASE",
    /* 347 */ "database_kw_opt ::=",
    /* 348 */ "key_opt ::=",
    /* 349 */ "key_opt ::= KEY expr",
    /* 350 */ "cmd ::= VACUUM vinto",
    /* 351 */ "cmd ::= VACUUM nm vinto",
    /* 352 */ "vinto ::= INTO expr",
    /* 353 */ "vinto ::=",
    /* 354 */ "ecmd ::= explain cmdx SEMI",
    /* 355 */ "explain ::= EXPLAIN",
    /* 356 */ "explain ::= EXPLAIN QUERY PLAN",
    /* 357 */
    "cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist "
    "RP where_opt",
    /* 358 */ "uniqueflag ::= UNIQUE",
    /* 359 */ "uniqueflag ::=",
    /* 360 */ "ifnotexists ::=",
    /* 361 */ "ifnotexists ::= IF NOT EXISTS",
    /* 362 */
    "cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select",
    /* 363 */ "createkw ::= CREATE",
    /* 364 */ "temp ::= TEMP",
    /* 365 */ "temp ::=",
    /* 366 */ "values ::= VALUES LP nexprlist RP",
    /* 367 */ "mvalues ::= values COMMA LP nexprlist RP",
    /* 368 */ "mvalues ::= mvalues COMMA LP nexprlist RP",
    /* 369 */ "oneselect ::= values",
    /* 370 */ "oneselect ::= mvalues",
    /* 371 */ "cmd ::= create_vtab",
    /* 372 */ "cmd ::= create_vtab LP vtabarglist RP",
    /* 373 */
    "create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm",
    /* 374 */ "vtabarglist ::= vtabarg",
    /* 375 */ "vtabarglist ::= vtabarglist COMMA vtabarg",
    /* 376 */ "vtabarg ::=",
    /* 377 */ "vtabarg ::= vtabarg vtabargtoken",
    /* 378 */ "vtabargtoken ::= ANY",
    /* 379 */ "vtabargtoken ::= lp anylist RP",
    /* 380 */ "lp ::= LP",
    /* 381 */ "anylist ::=",
    /* 382 */ "anylist ::= anylist LP anylist RP",
    /* 383 */ "anylist ::= anylist ANY",
    /* 384 */ "windowdefn_list ::= windowdefn",
    /* 385 */ "windowdefn_list ::= windowdefn_list COMMA windowdefn",
    /* 386 */ "windowdefn ::= nm AS LP window RP",
    /* 387 */ "window ::= PARTITION BY nexprlist orderby_opt frame_opt",
    /* 388 */ "window ::= nm PARTITION BY nexprlist orderby_opt frame_opt",
    /* 389 */ "window ::= ORDER BY sortlist frame_opt",
    /* 390 */ "window ::= nm ORDER BY sortlist frame_opt",
    /* 391 */ "window ::= frame_opt",
    /* 392 */ "window ::= nm frame_opt",
    /* 393 */ "frame_opt ::=",
    /* 394 */ "frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt",
    /* 395 */
    "frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e "
    "frame_exclude_opt",
    /* 396 */ "range_or_rows ::= RANGE|ROWS|GROUPS",
    /* 397 */ "frame_bound_s ::= frame_bound",
    /* 398 */ "frame_bound_s ::= UNBOUNDED PRECEDING",
    /* 399 */ "frame_bound_e ::= frame_bound",
    /* 400 */ "frame_bound_e ::= UNBOUNDED FOLLOWING",
    /* 401 */ "frame_bound ::= expr PRECEDING|FOLLOWING",
    /* 402 */ "frame_bound ::= CURRENT ROW",
    /* 403 */ "frame_exclude_opt ::=",
    /* 404 */ "frame_exclude_opt ::= EXCLUDE frame_exclude",
    /* 405 */ "frame_exclude ::= NO OTHERS",
    /* 406 */ "frame_exclude ::= CURRENT ROW",
    /* 407 */ "frame_exclude ::= GROUP|TIES",
    /* 408 */ "window_clause ::= WINDOW windowdefn_list",
    /* 409 */ "filter_over ::= filter_clause over_clause",
    /* 410 */ "filter_over ::= over_clause",
    /* 411 */ "filter_over ::= filter_clause",
    /* 412 */ "over_clause ::= OVER LP window RP",
    /* 413 */ "over_clause ::= OVER nm",
    /* 414 */ "filter_clause ::= FILTER LP WHERE expr RP",
};
#endif /* NDEBUG */

#if YYGROWABLESTACK
/*
** Try to increase the size of the parser stack.  Return the number
** of errors.  Return 0 on success.
*/
static int yyGrowStack(yyParser* p) {
  int oldSize = 1 + (int)(p->yystackEnd - p->yystack);
  int newSize;
  int idx;
  yyStackEntry* pNew;

  newSize = oldSize * 2 + 100;
  idx = (int)(p->yytos - p->yystack);
  if (p->yystack == p->yystk0) {
    pNew = YYREALLOC(0, newSize * sizeof(pNew[0]));
    if (pNew == 0)
      return 1;
    memcpy(pNew, p->yystack, oldSize * sizeof(pNew[0]));
  } else {
    pNew = YYREALLOC(p->yystack, newSize * sizeof(pNew[0]));
    if (pNew == 0)
      return 1;
  }
  p->yystack = pNew;
  p->yytos = &p->yystack[idx];
#ifndef NDEBUG
  if (yyTraceFILE) {
    fprintf(yyTraceFILE, "%sStack grows from %d to %d entries.\n",
            yyTracePrompt, oldSize, newSize);
  }
#endif
  p->yystackEnd = &p->yystack[newSize - 1];
  return 0;
}
#endif /* YYGROWABLESTACK */

#if !YYGROWABLESTACK
/* For builds that do no have a growable stack, yyGrowStack always
** returns an error.
*/
#define yyGrowStack(X) 1
#endif

/* Datatype of the argument to the memory allocated passed as the
** second argument to SynqSqliteParseAlloc() below.  This can be changed by
** putting an appropriate #define in the %include section of the input
** grammar.
*/
#ifndef YYMALLOCARGTYPE
#define YYMALLOCARGTYPE size_t
#endif

/* Initialize a new parser that has already been allocated.
 */
void SynqSqliteParseInit(void* yypRawParser SynqSqliteParseCTX_PDECL) {
  yyParser* yypParser = (yyParser*)yypRawParser;
  SynqSqliteParseCTX_STORE
#ifdef YYTRACKMAXSTACKDEPTH
      yypParser->yyhwm = 0;
#endif
  yypParser->yystack = yypParser->yystk0;
  yypParser->yystackEnd = &yypParser->yystack[YYSTACKDEPTH - 1];
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
  yypParser->yytos = yypParser->yystack;
  yypParser->yystack[0].stateno = 0;
  yypParser->yystack[0].major = 0;
}

#ifndef SynqSqliteParse_ENGINEALWAYSONSTACK
/*
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to SynqSqliteParse and SynqSqliteParseFree.
*/
void* SynqSqliteParseAlloc(void* (*mallocProc)(YYMALLOCARGTYPE)
                               SynqSqliteParseCTX_PDECL) {
  yyParser* yypParser;
  yypParser = (yyParser*)(*mallocProc)((YYMALLOCARGTYPE)sizeof(yyParser));
  if (yypParser) {
    SynqSqliteParseCTX_STORE SynqSqliteParseInit(
        yypParser SynqSqliteParseCTX_PARAM);
  }
  return (void*)yypParser;
}
#endif /* SynqSqliteParse_ENGINEALWAYSONSTACK */

/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
static void yy_destructor(
    yyParser* yypParser,  /* The parser */
    YYCODETYPE yymajor,   /* Type code for object to destroy */
    YYMINORTYPE* yypminor /* The object to be destroyed */
) {
  SynqSqliteParseARG_FETCH SynqSqliteParseCTX_FETCH switch (yymajor) {
      /* Here is inserted the actions which take place when a
      ** terminal or non-terminal is destroyed.  This can happen
      ** when the symbol is popped from the stack during a
      ** reduce or during error processing or when a parser is
      ** being destroyed before it is finished parsing.
      **
      ** Note: during a reduce, the only symbols destroyed are those
      ** which appear on the RHS of the rule, but which are *not* used
      ** inside the C code.
      */
      /********* Begin destructor definitions
       * ***************************************/
      /********* End destructor definitions
       * *****************************************/
    default:
      break; /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
static void yy_pop_parser_stack(yyParser* pParser) {
  yyStackEntry* yytos;
  assert(pParser->yytos != 0);
  assert(pParser->yytos > pParser->yystack);
  yytos = pParser->yytos--;
#ifndef NDEBUG
  if (yyTraceFILE) {
    fprintf(yyTraceFILE, "%sPopping %s\n", yyTracePrompt,
            yyTokenName[yytos->major]);
  }
#endif
  yy_destructor(pParser, yytos->major, &yytos->minor);
}

/*
** Clear all secondary memory allocations from the parser
*/
void SynqSqliteParseFinalize(void* p) {
  yyParser* pParser = (yyParser*)p;

  /* In-lined version of calling yy_pop_parser_stack() for each
  ** element left in the stack */
  yyStackEntry* yytos = pParser->yytos;
  while (yytos > pParser->yystack) {
#ifndef NDEBUG
    if (yyTraceFILE) {
      fprintf(yyTraceFILE, "%sPopping %s\n", yyTracePrompt,
              yyTokenName[yytos->major]);
    }
#endif
    if (yytos->major >= YY_MIN_DSTRCTR) {
      yy_destructor(pParser, yytos->major, &yytos->minor);
    }
    yytos--;
  }

#if YYGROWABLESTACK
  if (pParser->yystack != pParser->yystk0)
    YYFREE(pParser->yystack);
#endif
}

#ifndef SynqSqliteParse_ENGINEALWAYSONSTACK
/*
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/
void SynqSqliteParseFree(
    void* p,                /* The parser to be deleted */
    void (*freeProc)(void*) /* Function used to reclaim memory */
) {
#ifndef YYPARSEFREENEVERNULL
  if (p == 0)
    return;
#endif
  SynqSqliteParseFinalize(p);
  (*freeProc)(p);
}
#endif /* SynqSqliteParse_ENGINEALWAYSONSTACK */

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int SynqSqliteParseStackPeak(void* p) {
  yyParser* pParser = (yyParser*)p;
  return pParser->yyhwm;
}
#endif

/* This array of booleans keeps track of the parser statement
** coverage.  The element yycoverage[X][Y] is set when the parser
** is in state X and has a lookahead token Y.  In a well-tested
** systems, every element of this matrix should end up being set.
*/
#if defined(YYCOVERAGE)
static unsigned char yycoverage[YYNSTATE][YYNTOKEN];
#endif

/*
** Write into out a description of every state/lookahead combination that
**
**   (1)  has not been used by the parser, and
**   (2)  is not a syntax error.
**
** Return the number of missed state/lookahead combinations.
*/
#if defined(YYCOVERAGE)
int SynqSqliteParseCoverage(FILE* out) {
  int stateno, iLookAhead, i;
  int nMissed = 0;
  for (stateno = 0; stateno < YYNSTATE; stateno++) {
    i = yy_shift_ofst[stateno];
    for (iLookAhead = 0; iLookAhead < YYNTOKEN; iLookAhead++) {
      if (yy_lookahead[i + iLookAhead] != iLookAhead)
        continue;
      if (yycoverage[stateno][iLookAhead] == 0)
        nMissed++;
      if (out) {
        fprintf(out, "State %d lookahead %s %s\n", stateno,
                yyTokenName[iLookAhead],
                yycoverage[stateno][iLookAhead] ? "ok" : "missed");
      }
    }
  }
  return nMissed;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
static YYACTIONTYPE yy_find_shift_action(
    YYCODETYPE iLookAhead, /* The look-ahead token */
    YYACTIONTYPE stateno   /* Current state number */
) {
  int i;

  if (stateno > YY_MAX_SHIFT)
    return stateno;
  assert(stateno <= YY_SHIFT_COUNT);
#if defined(YYCOVERAGE)
  yycoverage[stateno][iLookAhead] = 1;
#endif
  do {
    i = yy_shift_ofst[stateno];
    assert(i >= 0);
    assert(i <= YY_ACTTAB_COUNT);
    assert(i + YYNTOKEN <= (int)YY_NLOOKAHEAD);
    assert(iLookAhead != YYNOCODE);
    assert(iLookAhead < YYNTOKEN);
    i += iLookAhead;
    assert(i < (int)YY_NLOOKAHEAD);
    if (yy_lookahead[i] != iLookAhead) {
#ifdef YYFALLBACK
      YYCODETYPE iFallback; /* Fallback token */
      assert(iLookAhead < sizeof(yyFallback) / sizeof(yyFallback[0]));
      iFallback = yyFallback[iLookAhead];
      if (iFallback != 0) {
#ifndef NDEBUG
        if (yyTraceFILE) {
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n", yyTracePrompt,
                  yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        assert(yyFallback[iFallback] == 0); /* Fallback loop must terminate */
        iLookAhead = iFallback;
        continue;
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        assert(j < (int)(sizeof(yy_lookahead) / sizeof(yy_lookahead[0])));
        if (yy_lookahead[j] == YYWILDCARD && iLookAhead > 0) {
#ifndef NDEBUG
          if (yyTraceFILE) {
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n", yyTracePrompt,
                    yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
      return yy_default[stateno];
    } else {
      assert(i >= 0 && i < (int)(sizeof(yy_action) / sizeof(yy_action[0])));
      return yy_action[i];
    }
  } while (1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
static YYACTIONTYPE yy_find_reduce_action(
    YYACTIONTYPE stateno, /* Current state number */
    YYCODETYPE iLookAhead /* The look-ahead token */
) {
  int i;
#ifdef YYERRORSYMBOL
  if (stateno > YY_REDUCE_COUNT) {
    return yy_default[stateno];
  }
#else
  assert(stateno <= YY_REDUCE_COUNT);
#endif
  i = yy_reduce_ofst[stateno];
  assert(iLookAhead != YYNOCODE);
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if (i < 0 || i >= YY_ACTTAB_COUNT || yy_lookahead[i] != iLookAhead) {
    return yy_default[stateno];
  }
#else
  assert(i >= 0 && i < YY_ACTTAB_COUNT);
  assert(yy_lookahead[i] == iLookAhead);
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser* yypParser) {
  SynqSqliteParseARG_FETCH SynqSqliteParseCTX_FETCH
#ifndef NDEBUG
      if (yyTraceFILE) {
    fprintf(yyTraceFILE, "%sStack Overflow!\n", yyTracePrompt);
  }
#endif
  while (yypParser->yytos > yypParser->yystack)
    yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will execute if the parser
  ** stack every overflows */
  /******** Begin %stack_overflow code
   * ******************************************/

  if (pCtx) {
    pCtx->error = 1;
  }
  /******** End %stack_overflow code
   * ********************************************/
  SynqSqliteParseARG_STORE /* Suppress warning about unused %extra_argument var
                            */
      SynqSqliteParseCTX_STORE
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(yyParser* yypParser,
                         int yyNewState,
                         const char* zTag) {
  if (yyTraceFILE) {
    if (yyNewState < YYNSTATE) {
      fprintf(yyTraceFILE, "%s%s '%s', go to state %d\n", yyTracePrompt, zTag,
              yyTokenName[yypParser->yytos->major], yyNewState);
    } else {
      fprintf(yyTraceFILE, "%s%s '%s', pending reduce %d\n", yyTracePrompt,
              zTag, yyTokenName[yypParser->yytos->major],
              yyNewState - YY_MIN_REDUCE);
    }
  }
}
#else
#define yyTraceShift(X, Y, Z)
#endif

/*
** Perform a shift action.
*/
static void yy_shift(
    yyParser* yypParser,             /* The parser to be shifted */
    YYACTIONTYPE yyNewState,         /* The new state to shift in */
    YYCODETYPE yyMajor,              /* The major token to shift in */
    SynqSqliteParseTOKENTYPE yyMinor /* The minor token to shift in */
) {
  yyStackEntry* yytos;
  yypParser->yytos++;
#ifdef YYTRACKMAXSTACKDEPTH
  if ((int)(yypParser->yytos - yypParser->yystack) > yypParser->yyhwm) {
    yypParser->yyhwm++;
    assert(yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack));
  }
#endif
  yytos = yypParser->yytos;
  if (yytos > yypParser->yystackEnd) {
    if (yyGrowStack(yypParser)) {
      yypParser->yytos--;
      yyStackOverflow(yypParser);
      return;
    }
    yytos = yypParser->yytos;
    assert(yytos <= yypParser->yystackEnd);
  }
  if (yyNewState > YY_MAX_SHIFT) {
    yyNewState += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
  }
  yytos->stateno = yyNewState;
  yytos->major = yyMajor;
  yytos->minor.yy0 = yyMinor;
  synq_on_shift(yypParser, yyMajor, &yyMinor);
  yyTraceShift(yypParser, yyNewState, "Shift");
}

/* For rule J, yyRuleInfoLhs[J] contains the symbol on the left-hand side
** of that rule */
static const YYCODETYPE yyRuleInfoLhs[] = {
    189, /* (0) input ::= cmdlist */
    190, /* (1) cmdlist ::= cmdlist ecmd */
    190, /* (2) cmdlist ::= ecmd */
    191, /* (3) ecmd ::= SEMI */
    191, /* (4) ecmd ::= cmdx SEMI */
    191, /* (5) ecmd ::= error SEMI */
    192, /* (6) cmdx ::= cmd */
    195, /* (7) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY
            sortlist RP */
    195, /* (8) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY
            sortlist RP filter_over */
    195, /* (9) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP WITHIN GROUP
            LP ORDER BY expr RP */
    195, /* (10) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP WITHIN
            GROUP LP ORDER BY expr RP filter_over */
    195, /* (11) expr ::= CAST LP expr AS typetoken RP */
    200, /* (12) typetoken ::= */
    200, /* (13) typetoken ::= typename */
    200, /* (14) typetoken ::= typename LP signed RP */
    200, /* (15) typetoken ::= typename LP signed COMMA signed RP */
    201, /* (16) typename ::= ID|STRING */
    201, /* (17) typename ::= typename ID|STRING */
    203, /* (18) selcollist ::= sclp scanpt nm DOT STAR */
    195, /* (19) expr ::= ID|INDEXED|JOIN_KW */
    195, /* (20) expr ::= nm DOT nm */
    195, /* (21) expr ::= nm DOT nm DOT nm */
    210, /* (22) selectnowith ::= selectnowith multiselect_op oneselect */
    207, /* (23) multiselect_op ::= UNION */
    207, /* (24) multiselect_op ::= UNION ALL */
    207, /* (25) multiselect_op ::= EXCEPT|INTERSECT */
    195, /* (26) expr ::= LP select RP */
    195, /* (27) expr ::= EXISTS LP select RP */
    208, /* (28) in_op ::= IN */
    208, /* (29) in_op ::= NOT IN */
    195, /* (30) expr ::= expr in_op LP exprlist RP */
    195, /* (31) expr ::= expr in_op LP select RP */
    195, /* (32) expr ::= expr in_op nm dbnm paren_exprlist */
    209, /* (33) dbnm ::= */
    209, /* (34) dbnm ::= DOT nm */
    213, /* (35) paren_exprlist ::= */
    213, /* (36) paren_exprlist ::= LP exprlist RP */
    195, /* (37) expr ::= expr ISNULL|NOTNULL */
    195, /* (38) expr ::= expr NOT NULL */
    195, /* (39) expr ::= expr IS expr */
    195, /* (40) expr ::= expr IS NOT expr */
    195, /* (41) expr ::= expr IS NOT DISTINCT FROM expr */
    195, /* (42) expr ::= expr IS DISTINCT FROM expr */
    215, /* (43) between_op ::= BETWEEN */
    215, /* (44) between_op ::= NOT BETWEEN */
    195, /* (45) expr ::= expr between_op expr AND expr */
    214, /* (46) likeop ::= LIKE_KW|MATCH */
    214, /* (47) likeop ::= NOT LIKE_KW|MATCH */
    195, /* (48) expr ::= expr likeop expr */
    195, /* (49) expr ::= expr likeop expr ESCAPE expr */
    195, /* (50) expr ::= CASE case_operand case_exprlist case_else END */
    217, /* (51) case_exprlist ::= case_exprlist WHEN expr THEN expr */
    217, /* (52) case_exprlist ::= WHEN expr THEN expr */
    218, /* (53) case_else ::= ELSE expr */
    218, /* (54) case_else ::= */
    216, /* (55) case_operand ::= expr */
    216, /* (56) case_operand ::= */
    194, /* (57) cmd ::= create_table create_table_args */
    235, /* (58) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
    236, /* (59) create_table_args ::= LP columnlist conslist_opt RP
            table_option_set */
    236, /* (60) create_table_args ::= AS select */
    227, /* (61) table_option_set ::= */
    227, /* (62) table_option_set ::= table_option */
    227, /* (63) table_option_set ::= table_option_set COMMA table_option */
    228, /* (64) table_option ::= WITHOUT nm */
    228, /* (65) table_option ::= nm */
    240, /* (66) columnlist ::= columnlist COMMA columnname carglist */
    240, /* (67) columnlist ::= columnname carglist */
    231, /* (68) carglist ::= carglist ccons */
    231, /* (69) carglist ::= */
    230, /* (70) ccons ::= CONSTRAINT nm */
    230, /* (71) ccons ::= DEFAULT scantok term */
    230, /* (72) ccons ::= DEFAULT LP expr RP */
    230, /* (73) ccons ::= DEFAULT PLUS scantok term */
    230, /* (74) ccons ::= DEFAULT MINUS scantok term */
    230, /* (75) ccons ::= DEFAULT scantok ID|INDEXED */
    230, /* (76) ccons ::= NULL onconf */
    230, /* (77) ccons ::= NOT NULL onconf */
    230, /* (78) ccons ::= PRIMARY KEY sortorder onconf autoinc */
    230, /* (79) ccons ::= UNIQUE onconf */
    230, /* (80) ccons ::= CHECK LP expr RP */
    230, /* (81) ccons ::= REFERENCES nm eidlist_opt refargs */
    230, /* (82) ccons ::= defer_subclause */
    230, /* (83) ccons ::= COLLATE ID|STRING */
    230, /* (84) ccons ::= GENERATED ALWAYS AS generated */
    230, /* (85) ccons ::= AS generated */
    234, /* (86) generated ::= LP expr RP */
    234, /* (87) generated ::= LP expr RP ID */
    220, /* (88) autoinc ::= */
    220, /* (89) autoinc ::= AUTOINCR */
    221, /* (90) refargs ::= */
    221, /* (91) refargs ::= refargs refarg */
    222, /* (92) refarg ::= MATCH nm */
    222, /* (93) refarg ::= ON INSERT refact */
    222, /* (94) refarg ::= ON DELETE refact */
    222, /* (95) refarg ::= ON UPDATE refact */
    223, /* (96) refact ::= SET NULL */
    223, /* (97) refact ::= SET DEFAULT */
    223, /* (98) refact ::= CASCADE */
    223, /* (99) refact ::= RESTRICT */
    223, /* (100) refact ::= NO ACTION */
    224, /* (101) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
    224, /* (102) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
    225, /* (103) init_deferred_pred_opt ::= */
    225, /* (104) init_deferred_pred_opt ::= INITIALLY DEFERRED */
    225, /* (105) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
    241, /* (106) conslist_opt ::= */
    241, /* (107) conslist_opt ::= COMMA conslist */
    233, /* (108) conslist ::= conslist tconscomma tcons */
    233, /* (109) conslist ::= tcons */
    246, /* (110) tconscomma ::= COMMA */
    246, /* (111) tconscomma ::= */
    232, /* (112) tcons ::= CONSTRAINT nm */
    232, /* (113) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
    232, /* (114) tcons ::= UNIQUE LP sortlist RP onconf */
    232, /* (115) tcons ::= CHECK LP expr RP onconf */
    232, /* (116) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt
            refargs defer_subclause_opt */
    226, /* (117) defer_subclause_opt ::= */
    226, /* (118) defer_subclause_opt ::= defer_subclause */
    229, /* (119) onconf ::= */
    229, /* (120) onconf ::= ON CONFLICT resolvetype */
    219, /* (121) scantok ::= */
    212, /* (122) select ::= WITH wqlist selectnowith */
    212, /* (123) select ::= WITH RECURSIVE wqlist selectnowith */
    253, /* (124) wqitem ::= withnm eidlist_opt wqas LP select RP */
    252, /* (125) wqlist ::= wqitem */
    252, /* (126) wqlist ::= wqlist COMMA wqitem */
    249, /* (127) withnm ::= nm */
    250, /* (128) wqas ::= AS */
    250, /* (129) wqas ::= AS MATERIALIZED */
    250, /* (130) wqas ::= AS NOT MATERIALIZED */
    245, /* (131) eidlist_opt ::= */
    245, /* (132) eidlist_opt ::= LP eidlist RP */
    247, /* (133) eidlist ::= nm collate sortorder */
    247, /* (134) eidlist ::= eidlist COMMA nm collate sortorder */
    251, /* (135) collate ::= */
    251, /* (136) collate ::= COLLATE ID|STRING */
    254, /* (137) with ::= */
    254, /* (138) with ::= WITH wqlist */
    254, /* (139) with ::= WITH RECURSIVE wqlist */
    194, /* (140) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret
            orderby_opt limit_opt */
    194, /* (141) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist
            from where_opt_ret orderby_opt limit_opt */
    194, /* (142) cmd ::= with insert_cmd INTO xfullname idlist_opt select
            upsert */
    194, /* (143) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT
            VALUES returning */
    255, /* (144) insert_cmd ::= INSERT orconf */
    255, /* (145) insert_cmd ::= REPLACE */
    256, /* (146) orconf ::= */
    256, /* (147) orconf ::= OR resolvetype */
    248, /* (148) resolvetype ::= raisetype */
    248, /* (149) resolvetype ::= IGNORE */
    248, /* (150) resolvetype ::= REPLACE */
    261, /* (151) xfullname ::= nm */
    261, /* (152) xfullname ::= nm DOT nm */
    261, /* (153) xfullname ::= nm DOT nm AS nm */
    261, /* (154) xfullname ::= nm AS nm */
    257, /* (155) indexed_opt ::= */
    257, /* (156) indexed_opt ::= indexed_by */
    258, /* (157) where_opt_ret ::= */
    258, /* (158) where_opt_ret ::= WHERE expr */
    258, /* (159) where_opt_ret ::= RETURNING selcollist */
    258, /* (160) where_opt_ret ::= WHERE expr RETURNING selcollist */
    264, /* (161) setlist ::= setlist COMMA nm EQ expr */
    264, /* (162) setlist ::= setlist COMMA LP idlist RP EQ expr */
    264, /* (163) setlist ::= nm EQ expr */
    264, /* (164) setlist ::= LP idlist RP EQ expr */
    266, /* (165) idlist_opt ::= */
    266, /* (166) idlist_opt ::= LP idlist RP */
    259, /* (167) upsert ::= */
    259, /* (168) upsert ::= RETURNING selcollist */
    259, /* (169) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET
            setlist where_opt upsert */
    259, /* (170) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING
            upsert */
    259, /* (171) upsert ::= ON CONFLICT DO NOTHING returning */
    259, /* (172) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt
            returning */
    260, /* (173) returning ::= RETURNING selcollist */
    260, /* (174) returning ::= */
    195, /* (175) expr ::= error */
    195, /* (176) expr ::= term */
    195, /* (177) expr ::= LP expr RP */
    195, /* (178) expr ::= expr PLUS|MINUS expr */
    195, /* (179) expr ::= expr STAR|SLASH|REM expr */
    195, /* (180) expr ::= expr LT|GT|GE|LE expr */
    195, /* (181) expr ::= expr EQ|NE expr */
    195, /* (182) expr ::= expr AND expr */
    195, /* (183) expr ::= expr OR expr */
    195, /* (184) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
    195, /* (185) expr ::= expr CONCAT expr */
    195, /* (186) expr ::= expr PTR expr */
    195, /* (187) expr ::= PLUS|MINUS expr */
    195, /* (188) expr ::= BITNOT expr */
    195, /* (189) expr ::= NOT expr */
    197, /* (190) exprlist ::= nexprlist */
    197, /* (191) exprlist ::= */
    271, /* (192) nexprlist ::= nexprlist COMMA expr */
    271, /* (193) nexprlist ::= expr */
    195, /* (194) expr ::= LP nexprlist COMMA expr RP */
    195, /* (195) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
    195, /* (196) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
    195, /* (197) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP
            filter_over */
    195, /* (198) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
    206, /* (199) nm ::= ID|INDEXED|JOIN_KW */
    206, /* (200) nm ::= STRING */
    272, /* (201) nmorerr ::= nm */
    272, /* (202) nmorerr ::= error */
    243, /* (203) term ::= INTEGER */
    243, /* (204) term ::= STRING */
    243, /* (205) term ::= NULL|FLOAT|BLOB */
    243, /* (206) term ::= QNUMBER */
    243, /* (207) term ::= CTIME_KW */
    195, /* (208) expr ::= VARIABLE */
    195, /* (209) expr ::= expr COLLATE ID|STRING */
    198, /* (210) sortlist ::= sortlist COMMA expr sortorder nulls */
    198, /* (211) sortlist ::= expr sortorder nulls */
    244, /* (212) sortorder ::= ASC */
    244, /* (213) sortorder ::= DESC */
    244, /* (214) sortorder ::= */
    273, /* (215) nulls ::= NULLS FIRST */
    273, /* (216) nulls ::= NULLS LAST */
    273, /* (217) nulls ::= */
    195, /* (218) expr ::= RAISE LP IGNORE RP */
    195, /* (219) expr ::= RAISE LP raisetype COMMA expr RP */
    267, /* (220) raisetype ::= ROLLBACK */
    267, /* (221) raisetype ::= ABORT */
    267, /* (222) raisetype ::= FAIL */
    279, /* (223) fullname ::= nmorerr */
    279, /* (224) fullname ::= nmorerr DOT nmorerr */
    274, /* (225) ifexists ::= IF EXISTS */
    274, /* (226) ifexists ::= */
    194, /* (227) cmd ::= DROP TABLE ifexists fullname */
    194, /* (228) cmd ::= DROP VIEW ifexists fullname */
    194, /* (229) cmd ::= DROP INDEX ifexists fullname */
    194, /* (230) cmd ::= DROP TRIGGER ifexists fullname */
    194, /* (231) cmd ::= ALTER TABLE fullname RENAME TO nmorerr */
    194, /* (232) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nmorerr TO
            nmorerr */
    194, /* (233) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nmorerr */
    194, /* (234) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt
            columnname carglist */
    280, /* (235) add_column_fullname ::= fullname */
    278, /* (236) kwcolumn_opt ::= */
    278, /* (237) kwcolumn_opt ::= COLUMNKW */
    242, /* (238) columnname ::= nmorerr typetoken */
    194, /* (239) cmd ::= BEGIN transtype trans_opt */
    194, /* (240) cmd ::= COMMIT|END trans_opt */
    194, /* (241) cmd ::= ROLLBACK trans_opt */
    275, /* (242) transtype ::= */
    275, /* (243) transtype ::= DEFERRED */
    275, /* (244) transtype ::= IMMEDIATE */
    275, /* (245) transtype ::= EXCLUSIVE */
    276, /* (246) trans_opt ::= */
    276, /* (247) trans_opt ::= TRANSACTION */
    276, /* (248) trans_opt ::= TRANSACTION nm */
    277, /* (249) savepoint_opt ::= SAVEPOINT */
    277, /* (250) savepoint_opt ::= */
    194, /* (251) cmd ::= SAVEPOINT nmorerr */
    194, /* (252) cmd ::= RELEASE savepoint_opt nmorerr */
    194, /* (253) cmd ::= ROLLBACK trans_opt TO savepoint_opt nmorerr */
    194, /* (254) cmd ::= select */
    212, /* (255) select ::= selectnowith */
    210, /* (256) selectnowith ::= oneselect */
    211, /* (257) oneselect ::= SELECT distinct selcollist from where_opt
            groupby_opt having_opt orderby_opt limit_opt */
    211, /* (258) oneselect ::= SELECT distinct selcollist from where_opt
            groupby_opt having_opt window_clause orderby_opt limit_opt */
    203, /* (259) selcollist ::= sclp scanpt expr scanpt as */
    203, /* (260) selcollist ::= sclp scanpt STAR */
    204, /* (261) sclp ::= selcollist COMMA */
    204, /* (262) sclp ::= */
    205, /* (263) scanpt ::= */
    281, /* (264) as ::= AS nmorerr */
    281, /* (265) as ::= ID|STRING */
    281, /* (266) as ::= */
    196, /* (267) distinct ::= DISTINCT */
    196, /* (268) distinct ::= ALL */
    196, /* (269) distinct ::= */
    265, /* (270) from ::= */
    265, /* (271) from ::= FROM seltablist */
    270, /* (272) where_opt ::= */
    270, /* (273) where_opt ::= WHERE expr */
    282, /* (274) groupby_opt ::= */
    282, /* (275) groupby_opt ::= GROUP BY nexprlist */
    283, /* (276) having_opt ::= */
    283, /* (277) having_opt ::= HAVING expr */
    262, /* (278) orderby_opt ::= */
    262, /* (279) orderby_opt ::= ORDER BY sortlist */
    263, /* (280) limit_opt ::= */
    263, /* (281) limit_opt ::= LIMIT expr */
    263, /* (282) limit_opt ::= LIMIT expr OFFSET expr */
    263, /* (283) limit_opt ::= LIMIT expr COMMA expr */
    288, /* (284) stl_prefix ::= seltablist joinop */
    288, /* (285) stl_prefix ::= */
    285, /* (286) seltablist ::= stl_prefix nm dbnm as on_using */
    285, /* (287) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
    285, /* (288) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using
          */
    285, /* (289) seltablist ::= stl_prefix LP select RP as on_using */
    285, /* (290) seltablist ::= stl_prefix LP seltablist RP as on_using */
    287, /* (291) joinop ::= COMMA|JOIN */
    287, /* (292) joinop ::= JOIN_KW JOIN */
    287, /* (293) joinop ::= JOIN_KW nm JOIN */
    287, /* (294) joinop ::= JOIN_KW nm nm JOIN */
    286, /* (295) on_using ::= ON expr */
    286, /* (296) on_using ::= USING LP idlist RP */
    286, /* (297) on_using ::= */
    268, /* (298) indexed_by ::= INDEXED BY nm */
    268, /* (299) indexed_by ::= NOT INDEXED */
    269, /* (300) idlist ::= idlist COMMA nm */
    269, /* (301) idlist ::= nm */
    194, /* (302) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
    291, /* (303) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time
            trigger_event ON fullname foreach_clause when_clause */
    289, /* (304) trigger_time ::= BEFORE|AFTER */
    289, /* (305) trigger_time ::= INSTEAD OF */
    289, /* (306) trigger_time ::= */
    293, /* (307) trigger_event ::= DELETE|INSERT */
    293, /* (308) trigger_event ::= UPDATE */
    293, /* (309) trigger_event ::= UPDATE OF idlist */
    294, /* (310) foreach_clause ::= */
    294, /* (311) foreach_clause ::= FOR EACH ROW */
    295, /* (312) when_clause ::= */
    295, /* (313) when_clause ::= WHEN expr */
    292, /* (314) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
    292, /* (315) trigger_cmd_list ::= trigger_cmd SEMI */
    290, /* (316) trnm ::= nm */
    290, /* (317) trnm ::= nm DOT nm */
    297, /* (318) tridxby ::= */
    297, /* (319) tridxby ::= INDEXED BY nm */
    297, /* (320) tridxby ::= NOT INDEXED */
    296, /* (321) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from
            where_opt scanpt */
    296, /* (322) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select
            upsert scanpt */
    296, /* (323) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
    296, /* (324) trigger_cmd ::= scanpt select scanpt */
    194, /* (325) cmd ::= PRAGMA nm dbnm */
    194, /* (326) cmd ::= PRAGMA nm dbnm EQ nmnum */
    194, /* (327) cmd ::= PRAGMA nm dbnm LP nmnum RP */
    194, /* (328) cmd ::= PRAGMA nm dbnm EQ minus_num */
    194, /* (329) cmd ::= PRAGMA nm dbnm LP minus_num RP */
    300, /* (330) nmnum ::= plus_num */
    300, /* (331) nmnum ::= nm */
    300, /* (332) nmnum ::= ON */
    300, /* (333) nmnum ::= DELETE */
    300, /* (334) nmnum ::= DEFAULT */
    298, /* (335) plus_num ::= PLUS INTEGER|FLOAT */
    298, /* (336) plus_num ::= INTEGER|FLOAT */
    299, /* (337) minus_num ::= MINUS INTEGER|FLOAT */
    202, /* (338) signed ::= plus_num */
    202, /* (339) signed ::= minus_num */
    194, /* (340) cmd ::= ANALYZE */
    194, /* (341) cmd ::= ANALYZE nm dbnm */
    194, /* (342) cmd ::= REINDEX */
    194, /* (343) cmd ::= REINDEX nm dbnm */
    194, /* (344) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
    194, /* (345) cmd ::= DETACH database_kw_opt expr */
    303, /* (346) database_kw_opt ::= DATABASE */
    303, /* (347) database_kw_opt ::= */
    304, /* (348) key_opt ::= */
    304, /* (349) key_opt ::= KEY expr */
    194, /* (350) cmd ::= VACUUM vinto */
    194, /* (351) cmd ::= VACUUM nm vinto */
    305, /* (352) vinto ::= INTO expr */
    305, /* (353) vinto ::= */
    191, /* (354) ecmd ::= explain cmdx SEMI */
    302, /* (355) explain ::= EXPLAIN */
    302, /* (356) explain ::= EXPLAIN QUERY PLAN */
    194, /* (357) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP
            sortlist RP where_opt */
    301, /* (358) uniqueflag ::= UNIQUE */
    301, /* (359) uniqueflag ::= */
    239, /* (360) ifnotexists ::= */
    239, /* (361) ifnotexists ::= IF NOT EXISTS */
    194, /* (362) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS
            select */
    237, /* (363) createkw ::= CREATE */
    238, /* (364) temp ::= TEMP */
    238, /* (365) temp ::= */
    306, /* (366) values ::= VALUES LP nexprlist RP */
    307, /* (367) mvalues ::= values COMMA LP nexprlist RP */
    307, /* (368) mvalues ::= mvalues COMMA LP nexprlist RP */
    211, /* (369) oneselect ::= values */
    211, /* (370) oneselect ::= mvalues */
    194, /* (371) cmd ::= create_vtab */
    194, /* (372) cmd ::= create_vtab LP vtabarglist RP */
    308, /* (373) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm
            USING nm */
    309, /* (374) vtabarglist ::= vtabarg */
    309, /* (375) vtabarglist ::= vtabarglist COMMA vtabarg */
    310, /* (376) vtabarg ::= */
    310, /* (377) vtabarg ::= vtabarg vtabargtoken */
    311, /* (378) vtabargtoken ::= ANY */
    311, /* (379) vtabargtoken ::= lp anylist RP */
    312, /* (380) lp ::= LP */
    313, /* (381) anylist ::= */
    313, /* (382) anylist ::= anylist LP anylist RP */
    313, /* (383) anylist ::= anylist ANY */
    317, /* (384) windowdefn_list ::= windowdefn */
    317, /* (385) windowdefn_list ::= windowdefn_list COMMA windowdefn */
    318, /* (386) windowdefn ::= nm AS LP window RP */
    319, /* (387) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
    319, /* (388) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
    319, /* (389) window ::= ORDER BY sortlist frame_opt */
    319, /* (390) window ::= nm ORDER BY sortlist frame_opt */
    319, /* (391) window ::= frame_opt */
    319, /* (392) window ::= nm frame_opt */
    320, /* (393) frame_opt ::= */
    320, /* (394) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
    320, /* (395) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND
            frame_bound_e frame_exclude_opt */
    314, /* (396) range_or_rows ::= RANGE|ROWS|GROUPS */
    321, /* (397) frame_bound_s ::= frame_bound */
    321, /* (398) frame_bound_s ::= UNBOUNDED PRECEDING */
    322, /* (399) frame_bound_e ::= frame_bound */
    322, /* (400) frame_bound_e ::= UNBOUNDED FOLLOWING */
    323, /* (401) frame_bound ::= expr PRECEDING|FOLLOWING */
    323, /* (402) frame_bound ::= CURRENT ROW */
    315, /* (403) frame_exclude_opt ::= */
    315, /* (404) frame_exclude_opt ::= EXCLUDE frame_exclude */
    316, /* (405) frame_exclude ::= NO OTHERS */
    316, /* (406) frame_exclude ::= CURRENT ROW */
    316, /* (407) frame_exclude ::= GROUP|TIES */
    284, /* (408) window_clause ::= WINDOW windowdefn_list */
    199, /* (409) filter_over ::= filter_clause over_clause */
    199, /* (410) filter_over ::= over_clause */
    199, /* (411) filter_over ::= filter_clause */
    325, /* (412) over_clause ::= OVER LP window RP */
    325, /* (413) over_clause ::= OVER nm */
    324, /* (414) filter_clause ::= FILTER LP WHERE expr RP */
};

/* For rule J, yyRuleInfoNRhs[J] contains the negative of the number
** of symbols on the right-hand side of that rule. */
static const signed char yyRuleInfoNRhs[] = {
    -1,  /* (0) input ::= cmdlist */
    -2,  /* (1) cmdlist ::= cmdlist ecmd */
    -1,  /* (2) cmdlist ::= ecmd */
    -1,  /* (3) ecmd ::= SEMI */
    -2,  /* (4) ecmd ::= cmdx SEMI */
    -2,  /* (5) ecmd ::= error SEMI */
    -1,  /* (6) cmdx ::= cmd */
    -8,  /* (7) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY
            sortlist RP */
    -9,  /* (8) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY
            sortlist RP filter_over */
    -12, /* (9) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP WITHIN GROUP
            LP ORDER BY expr RP */
    -13, /* (10) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP WITHIN
            GROUP LP ORDER BY expr RP filter_over */
    -6,  /* (11) expr ::= CAST LP expr AS typetoken RP */
    0,   /* (12) typetoken ::= */
    -1,  /* (13) typetoken ::= typename */
    -4,  /* (14) typetoken ::= typename LP signed RP */
    -6,  /* (15) typetoken ::= typename LP signed COMMA signed RP */
    -1,  /* (16) typename ::= ID|STRING */
    -2,  /* (17) typename ::= typename ID|STRING */
    -5,  /* (18) selcollist ::= sclp scanpt nm DOT STAR */
    -1,  /* (19) expr ::= ID|INDEXED|JOIN_KW */
    -3,  /* (20) expr ::= nm DOT nm */
    -5,  /* (21) expr ::= nm DOT nm DOT nm */
    -3,  /* (22) selectnowith ::= selectnowith multiselect_op oneselect */
    -1,  /* (23) multiselect_op ::= UNION */
    -2,  /* (24) multiselect_op ::= UNION ALL */
    -1,  /* (25) multiselect_op ::= EXCEPT|INTERSECT */
    -3,  /* (26) expr ::= LP select RP */
    -4,  /* (27) expr ::= EXISTS LP select RP */
    -1,  /* (28) in_op ::= IN */
    -2,  /* (29) in_op ::= NOT IN */
    -5,  /* (30) expr ::= expr in_op LP exprlist RP */
    -5,  /* (31) expr ::= expr in_op LP select RP */
    -5,  /* (32) expr ::= expr in_op nm dbnm paren_exprlist */
    0,   /* (33) dbnm ::= */
    -2,  /* (34) dbnm ::= DOT nm */
    0,   /* (35) paren_exprlist ::= */
    -3,  /* (36) paren_exprlist ::= LP exprlist RP */
    -2,  /* (37) expr ::= expr ISNULL|NOTNULL */
    -3,  /* (38) expr ::= expr NOT NULL */
    -3,  /* (39) expr ::= expr IS expr */
    -4,  /* (40) expr ::= expr IS NOT expr */
    -6,  /* (41) expr ::= expr IS NOT DISTINCT FROM expr */
    -5,  /* (42) expr ::= expr IS DISTINCT FROM expr */
    -1,  /* (43) between_op ::= BETWEEN */
    -2,  /* (44) between_op ::= NOT BETWEEN */
    -5,  /* (45) expr ::= expr between_op expr AND expr */
    -1,  /* (46) likeop ::= LIKE_KW|MATCH */
    -2,  /* (47) likeop ::= NOT LIKE_KW|MATCH */
    -3,  /* (48) expr ::= expr likeop expr */
    -5,  /* (49) expr ::= expr likeop expr ESCAPE expr */
    -5,  /* (50) expr ::= CASE case_operand case_exprlist case_else END */
    -5,  /* (51) case_exprlist ::= case_exprlist WHEN expr THEN expr */
    -4,  /* (52) case_exprlist ::= WHEN expr THEN expr */
    -2,  /* (53) case_else ::= ELSE expr */
    0,   /* (54) case_else ::= */
    -1,  /* (55) case_operand ::= expr */
    0,   /* (56) case_operand ::= */
    -2,  /* (57) cmd ::= create_table create_table_args */
    -6,  /* (58) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
    -5,  /* (59) create_table_args ::= LP columnlist conslist_opt RP
            table_option_set */
    -2,  /* (60) create_table_args ::= AS select */
    0,   /* (61) table_option_set ::= */
    -1,  /* (62) table_option_set ::= table_option */
    -3,  /* (63) table_option_set ::= table_option_set COMMA table_option */
    -2,  /* (64) table_option ::= WITHOUT nm */
    -1,  /* (65) table_option ::= nm */
    -4,  /* (66) columnlist ::= columnlist COMMA columnname carglist */
    -2,  /* (67) columnlist ::= columnname carglist */
    -2,  /* (68) carglist ::= carglist ccons */
    0,   /* (69) carglist ::= */
    -2,  /* (70) ccons ::= CONSTRAINT nm */
    -3,  /* (71) ccons ::= DEFAULT scantok term */
    -4,  /* (72) ccons ::= DEFAULT LP expr RP */
    -4,  /* (73) ccons ::= DEFAULT PLUS scantok term */
    -4,  /* (74) ccons ::= DEFAULT MINUS scantok term */
    -3,  /* (75) ccons ::= DEFAULT scantok ID|INDEXED */
    -2,  /* (76) ccons ::= NULL onconf */
    -3,  /* (77) ccons ::= NOT NULL onconf */
    -5,  /* (78) ccons ::= PRIMARY KEY sortorder onconf autoinc */
    -2,  /* (79) ccons ::= UNIQUE onconf */
    -4,  /* (80) ccons ::= CHECK LP expr RP */
    -4,  /* (81) ccons ::= REFERENCES nm eidlist_opt refargs */
    -1,  /* (82) ccons ::= defer_subclause */
    -2,  /* (83) ccons ::= COLLATE ID|STRING */
    -4,  /* (84) ccons ::= GENERATED ALWAYS AS generated */
    -2,  /* (85) ccons ::= AS generated */
    -3,  /* (86) generated ::= LP expr RP */
    -4,  /* (87) generated ::= LP expr RP ID */
    0,   /* (88) autoinc ::= */
    -1,  /* (89) autoinc ::= AUTOINCR */
    0,   /* (90) refargs ::= */
    -2,  /* (91) refargs ::= refargs refarg */
    -2,  /* (92) refarg ::= MATCH nm */
    -3,  /* (93) refarg ::= ON INSERT refact */
    -3,  /* (94) refarg ::= ON DELETE refact */
    -3,  /* (95) refarg ::= ON UPDATE refact */
    -2,  /* (96) refact ::= SET NULL */
    -2,  /* (97) refact ::= SET DEFAULT */
    -1,  /* (98) refact ::= CASCADE */
    -1,  /* (99) refact ::= RESTRICT */
    -2,  /* (100) refact ::= NO ACTION */
    -3,  /* (101) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
    -2,  /* (102) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
    0,   /* (103) init_deferred_pred_opt ::= */
    -2,  /* (104) init_deferred_pred_opt ::= INITIALLY DEFERRED */
    -2,  /* (105) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
    0,   /* (106) conslist_opt ::= */
    -2,  /* (107) conslist_opt ::= COMMA conslist */
    -3,  /* (108) conslist ::= conslist tconscomma tcons */
    -1,  /* (109) conslist ::= tcons */
    -1,  /* (110) tconscomma ::= COMMA */
    0,   /* (111) tconscomma ::= */
    -2,  /* (112) tcons ::= CONSTRAINT nm */
    -7,  /* (113) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
    -5,  /* (114) tcons ::= UNIQUE LP sortlist RP onconf */
    -5,  /* (115) tcons ::= CHECK LP expr RP onconf */
    -10, /* (116) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt
            refargs defer_subclause_opt */
    0,   /* (117) defer_subclause_opt ::= */
    -1,  /* (118) defer_subclause_opt ::= defer_subclause */
    0,   /* (119) onconf ::= */
    -3,  /* (120) onconf ::= ON CONFLICT resolvetype */
    0,   /* (121) scantok ::= */
    -3,  /* (122) select ::= WITH wqlist selectnowith */
    -4,  /* (123) select ::= WITH RECURSIVE wqlist selectnowith */
    -6,  /* (124) wqitem ::= withnm eidlist_opt wqas LP select RP */
    -1,  /* (125) wqlist ::= wqitem */
    -3,  /* (126) wqlist ::= wqlist COMMA wqitem */
    -1,  /* (127) withnm ::= nm */
    -1,  /* (128) wqas ::= AS */
    -2,  /* (129) wqas ::= AS MATERIALIZED */
    -3,  /* (130) wqas ::= AS NOT MATERIALIZED */
    0,   /* (131) eidlist_opt ::= */
    -3,  /* (132) eidlist_opt ::= LP eidlist RP */
    -3,  /* (133) eidlist ::= nm collate sortorder */
    -5,  /* (134) eidlist ::= eidlist COMMA nm collate sortorder */
    0,   /* (135) collate ::= */
    -2,  /* (136) collate ::= COLLATE ID|STRING */
    0,   /* (137) with ::= */
    -2,  /* (138) with ::= WITH wqlist */
    -3,  /* (139) with ::= WITH RECURSIVE wqlist */
    -8,  /* (140) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret
            orderby_opt limit_opt */
    -11, /* (141) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist
            from where_opt_ret orderby_opt limit_opt */
    -7, /* (142) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert
         */
    -8, /* (143) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT
           VALUES returning */
    -2, /* (144) insert_cmd ::= INSERT orconf */
    -1, /* (145) insert_cmd ::= REPLACE */
    0,  /* (146) orconf ::= */
    -2, /* (147) orconf ::= OR resolvetype */
    -1, /* (148) resolvetype ::= raisetype */
    -1, /* (149) resolvetype ::= IGNORE */
    -1, /* (150) resolvetype ::= REPLACE */
    -1, /* (151) xfullname ::= nm */
    -3, /* (152) xfullname ::= nm DOT nm */
    -5, /* (153) xfullname ::= nm DOT nm AS nm */
    -3, /* (154) xfullname ::= nm AS nm */
    0,  /* (155) indexed_opt ::= */
    -1, /* (156) indexed_opt ::= indexed_by */
    0,  /* (157) where_opt_ret ::= */
    -2, /* (158) where_opt_ret ::= WHERE expr */
    -2, /* (159) where_opt_ret ::= RETURNING selcollist */
    -4, /* (160) where_opt_ret ::= WHERE expr RETURNING selcollist */
    -5, /* (161) setlist ::= setlist COMMA nm EQ expr */
    -7, /* (162) setlist ::= setlist COMMA LP idlist RP EQ expr */
    -3, /* (163) setlist ::= nm EQ expr */
    -5, /* (164) setlist ::= LP idlist RP EQ expr */
    0,  /* (165) idlist_opt ::= */
    -3, /* (166) idlist_opt ::= LP idlist RP */
    0,  /* (167) upsert ::= */
    -2, /* (168) upsert ::= RETURNING selcollist */
    -12, /* (169) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET
            setlist where_opt upsert */
    -9,  /* (170) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING
            upsert */
    -5,  /* (171) upsert ::= ON CONFLICT DO NOTHING returning */
    -8,  /* (172) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt
            returning */
    -2,  /* (173) returning ::= RETURNING selcollist */
    0,   /* (174) returning ::= */
    -1,  /* (175) expr ::= error */
    -1,  /* (176) expr ::= term */
    -3,  /* (177) expr ::= LP expr RP */
    -3,  /* (178) expr ::= expr PLUS|MINUS expr */
    -3,  /* (179) expr ::= expr STAR|SLASH|REM expr */
    -3,  /* (180) expr ::= expr LT|GT|GE|LE expr */
    -3,  /* (181) expr ::= expr EQ|NE expr */
    -3,  /* (182) expr ::= expr AND expr */
    -3,  /* (183) expr ::= expr OR expr */
    -3,  /* (184) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
    -3,  /* (185) expr ::= expr CONCAT expr */
    -3,  /* (186) expr ::= expr PTR expr */
    -2,  /* (187) expr ::= PLUS|MINUS expr */
    -2,  /* (188) expr ::= BITNOT expr */
    -2,  /* (189) expr ::= NOT expr */
    -1,  /* (190) exprlist ::= nexprlist */
    0,   /* (191) exprlist ::= */
    -3,  /* (192) nexprlist ::= nexprlist COMMA expr */
    -1,  /* (193) nexprlist ::= expr */
    -5,  /* (194) expr ::= LP nexprlist COMMA expr RP */
    -5,  /* (195) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
    -4,  /* (196) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
    -6, /* (197) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over
         */
    -5, /* (198) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
    -1, /* (199) nm ::= ID|INDEXED|JOIN_KW */
    -1, /* (200) nm ::= STRING */
    -1, /* (201) nmorerr ::= nm */
    -1, /* (202) nmorerr ::= error */
    -1, /* (203) term ::= INTEGER */
    -1, /* (204) term ::= STRING */
    -1, /* (205) term ::= NULL|FLOAT|BLOB */
    -1, /* (206) term ::= QNUMBER */
    -1, /* (207) term ::= CTIME_KW */
    -1, /* (208) expr ::= VARIABLE */
    -3, /* (209) expr ::= expr COLLATE ID|STRING */
    -5, /* (210) sortlist ::= sortlist COMMA expr sortorder nulls */
    -3, /* (211) sortlist ::= expr sortorder nulls */
    -1, /* (212) sortorder ::= ASC */
    -1, /* (213) sortorder ::= DESC */
    0,  /* (214) sortorder ::= */
    -2, /* (215) nulls ::= NULLS FIRST */
    -2, /* (216) nulls ::= NULLS LAST */
    0,  /* (217) nulls ::= */
    -4, /* (218) expr ::= RAISE LP IGNORE RP */
    -6, /* (219) expr ::= RAISE LP raisetype COMMA expr RP */
    -1, /* (220) raisetype ::= ROLLBACK */
    -1, /* (221) raisetype ::= ABORT */
    -1, /* (222) raisetype ::= FAIL */
    -1, /* (223) fullname ::= nmorerr */
    -3, /* (224) fullname ::= nmorerr DOT nmorerr */
    -2, /* (225) ifexists ::= IF EXISTS */
    0,  /* (226) ifexists ::= */
    -4, /* (227) cmd ::= DROP TABLE ifexists fullname */
    -4, /* (228) cmd ::= DROP VIEW ifexists fullname */
    -4, /* (229) cmd ::= DROP INDEX ifexists fullname */
    -4, /* (230) cmd ::= DROP TRIGGER ifexists fullname */
    -6, /* (231) cmd ::= ALTER TABLE fullname RENAME TO nmorerr */
    -8, /* (232) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nmorerr TO
           nmorerr */
    -6, /* (233) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nmorerr */
    -7, /* (234) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt
           columnname carglist */
    -1, /* (235) add_column_fullname ::= fullname */
    0,  /* (236) kwcolumn_opt ::= */
    -1, /* (237) kwcolumn_opt ::= COLUMNKW */
    -2, /* (238) columnname ::= nmorerr typetoken */
    -3, /* (239) cmd ::= BEGIN transtype trans_opt */
    -2, /* (240) cmd ::= COMMIT|END trans_opt */
    -2, /* (241) cmd ::= ROLLBACK trans_opt */
    0,  /* (242) transtype ::= */
    -1, /* (243) transtype ::= DEFERRED */
    -1, /* (244) transtype ::= IMMEDIATE */
    -1, /* (245) transtype ::= EXCLUSIVE */
    0,  /* (246) trans_opt ::= */
    -1, /* (247) trans_opt ::= TRANSACTION */
    -2, /* (248) trans_opt ::= TRANSACTION nm */
    -1, /* (249) savepoint_opt ::= SAVEPOINT */
    0,  /* (250) savepoint_opt ::= */
    -2, /* (251) cmd ::= SAVEPOINT nmorerr */
    -3, /* (252) cmd ::= RELEASE savepoint_opt nmorerr */
    -5, /* (253) cmd ::= ROLLBACK trans_opt TO savepoint_opt nmorerr */
    -1, /* (254) cmd ::= select */
    -1, /* (255) select ::= selectnowith */
    -1, /* (256) selectnowith ::= oneselect */
    -9, /* (257) oneselect ::= SELECT distinct selcollist from where_opt
           groupby_opt having_opt orderby_opt limit_opt */
    -10, /* (258) oneselect ::= SELECT distinct selcollist from where_opt
            groupby_opt having_opt window_clause orderby_opt limit_opt */
    -5,  /* (259) selcollist ::= sclp scanpt expr scanpt as */
    -3,  /* (260) selcollist ::= sclp scanpt STAR */
    -2,  /* (261) sclp ::= selcollist COMMA */
    0,   /* (262) sclp ::= */
    0,   /* (263) scanpt ::= */
    -2,  /* (264) as ::= AS nmorerr */
    -1,  /* (265) as ::= ID|STRING */
    0,   /* (266) as ::= */
    -1,  /* (267) distinct ::= DISTINCT */
    -1,  /* (268) distinct ::= ALL */
    0,   /* (269) distinct ::= */
    0,   /* (270) from ::= */
    -2,  /* (271) from ::= FROM seltablist */
    0,   /* (272) where_opt ::= */
    -2,  /* (273) where_opt ::= WHERE expr */
    0,   /* (274) groupby_opt ::= */
    -3,  /* (275) groupby_opt ::= GROUP BY nexprlist */
    0,   /* (276) having_opt ::= */
    -2,  /* (277) having_opt ::= HAVING expr */
    0,   /* (278) orderby_opt ::= */
    -3,  /* (279) orderby_opt ::= ORDER BY sortlist */
    0,   /* (280) limit_opt ::= */
    -2,  /* (281) limit_opt ::= LIMIT expr */
    -4,  /* (282) limit_opt ::= LIMIT expr OFFSET expr */
    -4,  /* (283) limit_opt ::= LIMIT expr COMMA expr */
    -2,  /* (284) stl_prefix ::= seltablist joinop */
    0,   /* (285) stl_prefix ::= */
    -5,  /* (286) seltablist ::= stl_prefix nm dbnm as on_using */
    -6,  /* (287) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
    -8, /* (288) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
    -6, /* (289) seltablist ::= stl_prefix LP select RP as on_using */
    -6, /* (290) seltablist ::= stl_prefix LP seltablist RP as on_using */
    -1, /* (291) joinop ::= COMMA|JOIN */
    -2, /* (292) joinop ::= JOIN_KW JOIN */
    -3, /* (293) joinop ::= JOIN_KW nm JOIN */
    -4, /* (294) joinop ::= JOIN_KW nm nm JOIN */
    -2, /* (295) on_using ::= ON expr */
    -4, /* (296) on_using ::= USING LP idlist RP */
    0,  /* (297) on_using ::= */
    -3, /* (298) indexed_by ::= INDEXED BY nm */
    -2, /* (299) indexed_by ::= NOT INDEXED */
    -3, /* (300) idlist ::= idlist COMMA nm */
    -1, /* (301) idlist ::= nm */
    -5, /* (302) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
    -11, /* (303) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time
            trigger_event ON fullname foreach_clause when_clause */
    -1,  /* (304) trigger_time ::= BEFORE|AFTER */
    -2,  /* (305) trigger_time ::= INSTEAD OF */
    0,   /* (306) trigger_time ::= */
    -1,  /* (307) trigger_event ::= DELETE|INSERT */
    -1,  /* (308) trigger_event ::= UPDATE */
    -3,  /* (309) trigger_event ::= UPDATE OF idlist */
    0,   /* (310) foreach_clause ::= */
    -3,  /* (311) foreach_clause ::= FOR EACH ROW */
    0,   /* (312) when_clause ::= */
    -2,  /* (313) when_clause ::= WHEN expr */
    -3,  /* (314) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
    -2,  /* (315) trigger_cmd_list ::= trigger_cmd SEMI */
    -1,  /* (316) trnm ::= nm */
    -3,  /* (317) trnm ::= nm DOT nm */
    0,   /* (318) tridxby ::= */
    -3,  /* (319) tridxby ::= INDEXED BY nm */
    -2,  /* (320) tridxby ::= NOT INDEXED */
    -9,  /* (321) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from
            where_opt scanpt */
    -8,  /* (322) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select
            upsert scanpt */
    -6,  /* (323) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
    -3,  /* (324) trigger_cmd ::= scanpt select scanpt */
    -3,  /* (325) cmd ::= PRAGMA nm dbnm */
    -5,  /* (326) cmd ::= PRAGMA nm dbnm EQ nmnum */
    -6,  /* (327) cmd ::= PRAGMA nm dbnm LP nmnum RP */
    -5,  /* (328) cmd ::= PRAGMA nm dbnm EQ minus_num */
    -6,  /* (329) cmd ::= PRAGMA nm dbnm LP minus_num RP */
    -1,  /* (330) nmnum ::= plus_num */
    -1,  /* (331) nmnum ::= nm */
    -1,  /* (332) nmnum ::= ON */
    -1,  /* (333) nmnum ::= DELETE */
    -1,  /* (334) nmnum ::= DEFAULT */
    -2,  /* (335) plus_num ::= PLUS INTEGER|FLOAT */
    -1,  /* (336) plus_num ::= INTEGER|FLOAT */
    -2,  /* (337) minus_num ::= MINUS INTEGER|FLOAT */
    -1,  /* (338) signed ::= plus_num */
    -1,  /* (339) signed ::= minus_num */
    -1,  /* (340) cmd ::= ANALYZE */
    -3,  /* (341) cmd ::= ANALYZE nm dbnm */
    -1,  /* (342) cmd ::= REINDEX */
    -3,  /* (343) cmd ::= REINDEX nm dbnm */
    -6,  /* (344) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
    -3,  /* (345) cmd ::= DETACH database_kw_opt expr */
    -1,  /* (346) database_kw_opt ::= DATABASE */
    0,   /* (347) database_kw_opt ::= */
    0,   /* (348) key_opt ::= */
    -2,  /* (349) key_opt ::= KEY expr */
    -2,  /* (350) cmd ::= VACUUM vinto */
    -3,  /* (351) cmd ::= VACUUM nm vinto */
    -2,  /* (352) vinto ::= INTO expr */
    0,   /* (353) vinto ::= */
    -3,  /* (354) ecmd ::= explain cmdx SEMI */
    -1,  /* (355) explain ::= EXPLAIN */
    -3,  /* (356) explain ::= EXPLAIN QUERY PLAN */
    -12, /* (357) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP
            sortlist RP where_opt */
    -1,  /* (358) uniqueflag ::= UNIQUE */
    0,   /* (359) uniqueflag ::= */
    0,   /* (360) ifnotexists ::= */
    -3,  /* (361) ifnotexists ::= IF NOT EXISTS */
    -9,  /* (362) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS
            select */
    -1,  /* (363) createkw ::= CREATE */
    -1,  /* (364) temp ::= TEMP */
    0,   /* (365) temp ::= */
    -4,  /* (366) values ::= VALUES LP nexprlist RP */
    -5,  /* (367) mvalues ::= values COMMA LP nexprlist RP */
    -5,  /* (368) mvalues ::= mvalues COMMA LP nexprlist RP */
    -1,  /* (369) oneselect ::= values */
    -1,  /* (370) oneselect ::= mvalues */
    -1,  /* (371) cmd ::= create_vtab */
    -4,  /* (372) cmd ::= create_vtab LP vtabarglist RP */
    -8,  /* (373) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm
            USING nm */
    -1,  /* (374) vtabarglist ::= vtabarg */
    -3,  /* (375) vtabarglist ::= vtabarglist COMMA vtabarg */
    0,   /* (376) vtabarg ::= */
    -2,  /* (377) vtabarg ::= vtabarg vtabargtoken */
    -1,  /* (378) vtabargtoken ::= ANY */
    -3,  /* (379) vtabargtoken ::= lp anylist RP */
    -1,  /* (380) lp ::= LP */
    0,   /* (381) anylist ::= */
    -4,  /* (382) anylist ::= anylist LP anylist RP */
    -2,  /* (383) anylist ::= anylist ANY */
    -1,  /* (384) windowdefn_list ::= windowdefn */
    -3,  /* (385) windowdefn_list ::= windowdefn_list COMMA windowdefn */
    -5,  /* (386) windowdefn ::= nm AS LP window RP */
    -5,  /* (387) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
    -6,  /* (388) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
    -4,  /* (389) window ::= ORDER BY sortlist frame_opt */
    -5,  /* (390) window ::= nm ORDER BY sortlist frame_opt */
    -1,  /* (391) window ::= frame_opt */
    -2,  /* (392) window ::= nm frame_opt */
    0,   /* (393) frame_opt ::= */
    -3,  /* (394) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
    -6,  /* (395) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND
            frame_bound_e frame_exclude_opt */
    -1,  /* (396) range_or_rows ::= RANGE|ROWS|GROUPS */
    -1,  /* (397) frame_bound_s ::= frame_bound */
    -2,  /* (398) frame_bound_s ::= UNBOUNDED PRECEDING */
    -1,  /* (399) frame_bound_e ::= frame_bound */
    -2,  /* (400) frame_bound_e ::= UNBOUNDED FOLLOWING */
    -2,  /* (401) frame_bound ::= expr PRECEDING|FOLLOWING */
    -2,  /* (402) frame_bound ::= CURRENT ROW */
    0,   /* (403) frame_exclude_opt ::= */
    -2,  /* (404) frame_exclude_opt ::= EXCLUDE frame_exclude */
    -2,  /* (405) frame_exclude ::= NO OTHERS */
    -2,  /* (406) frame_exclude ::= CURRENT ROW */
    -1,  /* (407) frame_exclude ::= GROUP|TIES */
    -2,  /* (408) window_clause ::= WINDOW windowdefn_list */
    -2,  /* (409) filter_over ::= filter_clause over_clause */
    -1,  /* (410) filter_over ::= over_clause */
    -1,  /* (411) filter_over ::= filter_clause */
    -4,  /* (412) over_clause ::= OVER LP window RP */
    -2,  /* (413) over_clause ::= OVER nm */
    -5,  /* (414) filter_clause ::= FILTER LP WHERE expr RP */
};

static void yy_accept(yyParser*); /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
**
** The yyLookahead and yyLookaheadToken parameters provide reduce actions
** access to the lookahead token (if any).  The yyLookahead will be YYNOCODE
** if the lookahead token has already been consumed.  As this procedure is
** only called from one place, optimizing compilers will in-line it, which
** means that the extra parameters have no performance impact.
*/
static YYACTIONTYPE yy_reduce(
    yyParser* yypParser,   /* The parser */
    unsigned int yyruleno, /* Number of the rule by which to reduce */
    int yyLookahead,       /* Lookahead token, or YYNOCODE if none */
    SynqSqliteParseTOKENTYPE yyLookaheadToken /* Value of the lookahead token */
        SynqSqliteParseCTX_PDECL              /* %extra_context */
) {
  int yygoto;          /* The next state */
  YYACTIONTYPE yyact;  /* The next action */
  yyStackEntry* yymsp; /* The top of the parser's stack */
  int yysize;          /* Amount to pop the stack */
  SynqSqliteParseARG_FETCH(void) yyLookahead;
  (void)yyLookaheadToken;
  yymsp = yypParser->yytos;
  synq_on_reduce(yypParser, yyruleno);

  switch (yyruleno) {
    /* Beginning here are the reduction cases.  A typical example
    ** follows:
    **   case 0:
    **  #line <lineno> <grammarfile>
    **     { ... }           // User supplied code
    **  #line <lineno> <thisfile>
    **     break;
    */
    /********** Begin reduce actions
     * **********************************************/
    YYMINORTYPE yylhsminor;
    case 0: /* input ::= cmdlist */
    {
      pCtx->root = yymsp[0].minor.yy277;
    } break;
    case 1: /* cmdlist ::= cmdlist ecmd */
    {
      yymsp[-1].minor.yy277 = synq_pass(
          pCtx, yymsp[0].minor.yy277);  // Just use the last command for now
    } break;
    case 2:  /* cmdlist ::= ecmd */
    case 55: /* case_operand ::= expr */
      yytestcase(yyruleno == 55);
    case 176: /* expr ::= term */
      yytestcase(yyruleno == 176);
    case 190: /* exprlist ::= nexprlist */
      yytestcase(yyruleno == 190);
    case 235: /* add_column_fullname ::= fullname */
      yytestcase(yyruleno == 235);
    case 254: /* cmd ::= select */
      yytestcase(yyruleno == 254);
    case 255: /* select ::= selectnowith */
      yytestcase(yyruleno == 255);
    case 256: /* selectnowith ::= oneselect */
      yytestcase(yyruleno == 256);
    case 371: /* cmd ::= create_vtab */
      yytestcase(yyruleno == 371);
    case 397: /* frame_bound_s ::= frame_bound */
      yytestcase(yyruleno == 397);
    case 399: /* frame_bound_e ::= frame_bound */
      yytestcase(yyruleno == 399);
    case 410: /* filter_over ::= over_clause */
      yytestcase(yyruleno == 410);
      {
        yylhsminor.yy277 = yymsp[0].minor.yy277;
      }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 3: /* ecmd ::= SEMI */
    {
      yymsp[0].minor.yy277 = SYNTAQLITE_NULL_NODE;
      pCtx->stmt_completed = 1;
    } break;
    case 4:   /* ecmd ::= cmdx SEMI */
    case 261: /* sclp ::= selcollist COMMA */
      yytestcase(yyruleno == 261);
      {
        yylhsminor.yy277 = synq_pass(pCtx, yymsp[-1].minor.yy277);
      }
      yymsp[-1].minor.yy277 = yylhsminor.yy277;
      break;
    case 5: /* ecmd ::= error SEMI */
    {
      yymsp[-1].minor.yy277 = SYNTAQLITE_NULL_NODE;
      pCtx->root = SYNTAQLITE_NULL_NODE;
      pCtx->stmt_completed = 1;
    } break;
    case 6: /* cmdx ::= cmd */
    {
      if (pCtx->pending_explain_mode) {
        yylhsminor.yy277 = synq_parse_explain_stmt(
            pCtx, (SyntaqliteExplainMode)(pCtx->pending_explain_mode - 1),
            yymsp[0].minor.yy277);
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
        synq_extent_record(pCtx, yylhsminor.yy277);
      } else {
        yylhsminor.yy277 = yymsp[0].minor.yy277;
      }
      pCtx->root = yylhsminor.yy277;
      synq_parse_list_flush(pCtx);
      pCtx->stmt_completed = 1;
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 7: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY
               sortlist RP */
    {
      synq_mark_as_function(pCtx, yymsp[-7].minor.yy0);
      yylhsminor.yy277 = synq_parse_aggregate_function_call(
          pCtx, synq_span(pCtx, yymsp[-7].minor.yy0),
          (SyntaqliteAggregateFunctionCallFlags){
              .raw = (uint8_t)(yymsp[-5].minor.yy277 & 0xFF)},
          yymsp[-4].minor.yy277, yymsp[-1].minor.yy277, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE);
    }
      yymsp[-7].minor.yy277 = yylhsminor.yy277;
      break;
    case 8: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY
               sortlist RP filter_over */
    {
      SyntaqliteFilterOver* fo =
          AST_NODE_AS(SyntaqliteFilterOver, &pCtx->ast, yymsp[0].minor.yy277);
      synq_mark_as_function(pCtx, yymsp[-8].minor.yy0);
      yylhsminor.yy277 = synq_parse_aggregate_function_call(
          pCtx, synq_span(pCtx, yymsp[-8].minor.yy0),
          (SyntaqliteAggregateFunctionCallFlags){
              .raw = (uint8_t)(yymsp[-6].minor.yy277 & 0xFF)},
          yymsp[-5].minor.yy277, yymsp[-2].minor.yy277, fo->filter_expr,
          fo->over_def);
    }
      yymsp[-8].minor.yy277 = yylhsminor.yy277;
      break;
    case 9: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP WITHIN GROUP
               LP ORDER BY expr RP */
    {
      synq_mark_as_function(pCtx, yymsp[-11].minor.yy0);
      yylhsminor.yy277 = synq_parse_ordered_set_function_call(
          pCtx, synq_span(pCtx, yymsp[-11].minor.yy0),
          (SyntaqliteAggregateFunctionCallFlags){
              .raw = (uint8_t)(yymsp[-9].minor.yy277 & 0xFF)},
          yymsp[-8].minor.yy277, yymsp[-1].minor.yy277, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE);
    }
      yymsp[-11].minor.yy277 = yylhsminor.yy277;
      break;
    case 10: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP WITHIN GROUP
                LP ORDER BY expr RP filter_over */
    {
      SyntaqliteFilterOver* fo =
          AST_NODE_AS(SyntaqliteFilterOver, &pCtx->ast, yymsp[0].minor.yy277);
      synq_mark_as_function(pCtx, yymsp[-12].minor.yy0);
      yylhsminor.yy277 = synq_parse_ordered_set_function_call(
          pCtx, synq_span(pCtx, yymsp[-12].minor.yy0),
          (SyntaqliteAggregateFunctionCallFlags){
              .raw = (uint8_t)(yymsp[-10].minor.yy277 & 0xFF)},
          yymsp[-9].minor.yy277, yymsp[-2].minor.yy277, fo->filter_expr,
          fo->over_def);
    }
      yymsp[-12].minor.yy277 = yylhsminor.yy277;
      break;
    case 11: /* expr ::= CAST LP expr AS typetoken RP */
    {
      yymsp[-5].minor.yy277 = synq_parse_cast_expr(
          pCtx, yymsp[-3].minor.yy277, synq_span(pCtx, yymsp[-1].minor.yy0));
    } break;
    case 12: /* typetoken ::= */
    {
      yymsp[1].minor.yy0.n = 0;
      yymsp[1].minor.yy0.z = 0;
    } break;
    case 13: /* typetoken ::= typename */
    {
      (void)yymsp[0].minor.yy0;
    } break;
    case 14: /* typetoken ::= typename LP signed RP */
    {
      yymsp[-3].minor.yy0.n =
          (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] -
                yymsp[-3].minor.yy0.z);
    } break;
    case 15: /* typetoken ::= typename LP signed COMMA signed RP */
    {
      yymsp[-5].minor.yy0.n =
          (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] -
                yymsp[-5].minor.yy0.z);
    } break;
    case 16: /* typename ::= ID|STRING */
    {
      synq_mark_as_type(pCtx, yymsp[0].minor.yy0);
      yylhsminor.yy0 = yymsp[0].minor.yy0;
    }
      yymsp[0].minor.yy0 = yylhsminor.yy0;
      break;
    case 17: /* typename ::= typename ID|STRING */
    {
      synq_mark_as_type(pCtx, yymsp[0].minor.yy0);
      yymsp[-1].minor.yy0.n =
          yymsp[0].minor.yy0.n +
          (int)(yymsp[0].minor.yy0.z - yymsp[-1].minor.yy0.z);
    } break;
    case 18: /* selcollist ::= sclp scanpt nm DOT STAR */
    {
      uint32_t expr =
          synq_parse_ident_name(pCtx, synq_span(pCtx, yymsp[-2].minor.yy0));
      uint32_t col = synq_parse_result_column(
          pCtx, (SyntaqliteResultColumnFlags){.raw = 0x01},
          SYNTAQLITE_NULL_NODE, expr);
      yylhsminor.yy277 =
          synq_parse_result_column_list(pCtx, yymsp[-4].minor.yy277, col);
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 19: /* expr ::= ID|INDEXED|JOIN_KW */
    {
      synq_mark_as_id(pCtx, yymsp[0].minor.yy0);
      yylhsminor.yy277 = synq_parse_column_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0), SYNQ_NO_SPAN,
          SYNQ_NO_SPAN);
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 20: /* expr ::= nm DOT nm */
    {
      yylhsminor.yy277 = synq_parse_column_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0),
          synq_span_dequote(pCtx, yymsp[-2].minor.yy0), SYNQ_NO_SPAN);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 21: /* expr ::= nm DOT nm DOT nm */
    {
      yylhsminor.yy277 = synq_parse_column_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0),
          synq_span_dequote(pCtx, yymsp[-2].minor.yy0),
          synq_span_dequote(pCtx, yymsp[-4].minor.yy0));
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 22: /* selectnowith ::= selectnowith multiselect_op oneselect */
    {
      // ORDER BY / LIMIT parse inside the last arm (grammar shape) but apply
      // to the whole compound — hoist them onto the CompoundSelect node.
      // An ORDER BY on a non-last arm stays put (SQLite rejects it later).
      uint32_t orderby = SYNTAQLITE_NULL_NODE;
      uint32_t limit = SYNTAQLITE_NULL_NODE;
      SyntaqliteNode* arm = AST_NODE(&pCtx->ast, yymsp[0].minor.yy277);
      if (arm->tag == SYNTAQLITE_NODE_SELECT_STMT) {
        orderby = arm->select_stmt.orderby;
        limit = arm->select_stmt.limit_clause;
        arm->select_stmt.orderby = SYNTAQLITE_NULL_NODE;
        arm->select_stmt.limit_clause = SYNTAQLITE_NULL_NODE;
      }
      yymsp[-2].minor.yy277 = synq_parse_compound_select(
          pCtx, (SyntaqliteCompoundOp)yymsp[-1].minor.yy320,
          yymsp[-2].minor.yy277, yymsp[0].minor.yy277, orderby, limit);
    } break;
    case 23: /* multiselect_op ::= UNION */
    {
      yylhsminor.yy320 = 0;
      (void)yymsp[0].minor.yy0;
    }
      yymsp[0].minor.yy320 = yylhsminor.yy320;
      break;
    case 24: /* multiselect_op ::= UNION ALL */
    case 29: /* in_op ::= NOT IN */
      yytestcase(yyruleno == 29);
      {
        yymsp[-1].minor.yy320 = 1;
      }
      break;
    case 25: /* multiselect_op ::= EXCEPT|INTERSECT */
    {
      yylhsminor.yy320 =
          (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_INTERSECT) ? 2 : 3;
    }
      yymsp[0].minor.yy320 = yylhsminor.yy320;
      break;
    case 26: /* expr ::= LP select RP */
    {
      pCtx->saw_subquery = 1;
      yymsp[-2].minor.yy277 =
          synq_parse_subquery_expr(pCtx, yymsp[-1].minor.yy277);
    } break;
    case 27: /* expr ::= EXISTS LP select RP */
    {
      pCtx->saw_subquery = 1;
      yymsp[-3].minor.yy277 =
          synq_parse_exists_expr(pCtx, yymsp[-1].minor.yy277);
    } break;
    case 28: /* in_op ::= IN */
    {
      yymsp[0].minor.yy320 = 0;
    } break;
    case 30: /* expr ::= expr in_op LP exprlist RP */
    {
      yymsp[-4].minor.yy277 = synq_parse_in_expr(
          pCtx, (SyntaqliteBool)yymsp[-3].minor.yy320, SYNTAQLITE_BOOL_FALSE,
          yymsp[-4].minor.yy277, yymsp[-1].minor.yy277);
    } break;
    case 31: /* expr ::= expr in_op LP select RP */
    {
      pCtx->saw_subquery = 1;
      // Pass the raw select node directly — InExpr's fmt block already adds
      // the surrounding parens, so wrapping in SubqueryExpr would double them.
      yymsp[-4].minor.yy277 = synq_parse_in_expr(
          pCtx, (SyntaqliteBool)yymsp[-3].minor.yy320, SYNTAQLITE_BOOL_FALSE,
          yymsp[-4].minor.yy277, yymsp[-1].minor.yy277);
    } break;
    case 32: /* expr ::= expr in_op nm dbnm paren_exprlist */
    {
      SyntaqliteTextSpan table_name;
      SyntaqliteTextSpan schema;
      if (yymsp[-1].minor.yy0.z != NULL) {
        table_name = synq_span_dequote(pCtx, yymsp[-1].minor.yy0);
        schema = synq_span_dequote(pCtx, yymsp[-2].minor.yy0);
      } else {
        table_name = synq_span_dequote(pCtx, yymsp[-2].minor.yy0);
        schema = SYNQ_NO_SPAN;
      }
      uint32_t tref = synq_parse_table_ref(
          pCtx, table_name, schema, yymsp[0].minor.yy618.has_parens,
          SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy618.args,
          SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
      yymsp[-4].minor.yy277 =
          synq_parse_in_expr(pCtx, (SyntaqliteBool)yymsp[-3].minor.yy320,
                             SYNTAQLITE_BOOL_TRUE, yymsp[-4].minor.yy277, tref);
    } break;
    case 33: /* dbnm ::= */
    {
      yymsp[1].minor.yy0.z = NULL;
      yymsp[1].minor.yy0.n = 0;
    } break;
    case 34: /* dbnm ::= DOT nm */
    {
      yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;
    } break;
    case 35: /* paren_exprlist ::= */
    {
      yymsp[1].minor.yy618.args = SYNTAQLITE_NULL_NODE;
      yymsp[1].minor.yy618.has_parens = SYNTAQLITE_BOOL_FALSE;
    } break;
    case 36: /* paren_exprlist ::= LP exprlist RP */
    {
      yymsp[-2].minor.yy618.args = synq_pass(pCtx, yymsp[-1].minor.yy277);
      yymsp[-2].minor.yy618.has_parens = SYNTAQLITE_BOOL_TRUE;
    } break;
    case 37: /* expr ::= expr ISNULL|NOTNULL */
    {
      SyntaqliteIsOp op = (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_ISNULL)
                              ? SYNTAQLITE_IS_OP_IS_NULL
                              : SYNTAQLITE_IS_OP_NOT_NULL;
      yylhsminor.yy277 = synq_parse_is_expr(pCtx, op, yymsp[-1].minor.yy277,
                                            SYNTAQLITE_NULL_NODE);
    }
      yymsp[-1].minor.yy277 = yylhsminor.yy277;
      break;
    case 38: /* expr ::= expr NOT NULL */
    {
      yylhsminor.yy277 =
          synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_NOT_NULL,
                             yymsp[-2].minor.yy277, SYNTAQLITE_NULL_NODE);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 39: /* expr ::= expr IS expr */
    {
      yylhsminor.yy277 =
          synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_IS, yymsp[-2].minor.yy277,
                             yymsp[0].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 40: /* expr ::= expr IS NOT expr */
    {
      yylhsminor.yy277 =
          synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_IS_NOT,
                             yymsp[-3].minor.yy277, yymsp[0].minor.yy277);
    }
      yymsp[-3].minor.yy277 = yylhsminor.yy277;
      break;
    case 41: /* expr ::= expr IS NOT DISTINCT FROM expr */
    {
      yylhsminor.yy277 =
          synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_IS_NOT_DISTINCT,
                             yymsp[-5].minor.yy277, yymsp[0].minor.yy277);
    }
      yymsp[-5].minor.yy277 = yylhsminor.yy277;
      break;
    case 42: /* expr ::= expr IS DISTINCT FROM expr */
    {
      yylhsminor.yy277 =
          synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_IS_DISTINCT,
                             yymsp[-4].minor.yy277, yymsp[0].minor.yy277);
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 43:  /* between_op ::= BETWEEN */
    case 212: /* sortorder ::= ASC */
      yytestcase(yyruleno == 212);
      {
        yymsp[0].minor.yy277 = 0;
      }
      break;
    case 44:  /* between_op ::= NOT BETWEEN */
    case 215: /* nulls ::= NULLS FIRST */
      yytestcase(yyruleno == 215);
      {
        yymsp[-1].minor.yy277 = 1;
      }
      break;
    case 45: /* expr ::= expr between_op expr AND expr */
    {
      yylhsminor.yy277 = synq_parse_between_expr(
          pCtx, (SyntaqliteBool)yymsp[-3].minor.yy277, yymsp[-4].minor.yy277,
          yymsp[-2].minor.yy277, yymsp[0].minor.yy277);
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 46: /* likeop ::= LIKE_KW|MATCH */
    {
      yylhsminor.yy0 = yymsp[0].minor.yy0;
    }
      yymsp[0].minor.yy0 = yylhsminor.yy0;
      break;
    case 47: /* likeop ::= NOT LIKE_KW|MATCH */
    {
      yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;
      yymsp[-1].minor.yy0.n |= 0x80000000;
    } break;
    case 48: /* expr ::= expr likeop expr */
    {
      SyntaqliteBool negated = (yymsp[-1].minor.yy0.n & 0x80000000)
                                   ? SYNTAQLITE_BOOL_TRUE
                                   : SYNTAQLITE_BOOL_FALSE;
      uint32_t len = yymsp[-1].minor.yy0.n & 0x7FFFFFFF;
      SyntaqliteLikeKeyword kw =
          (len == 6)   ? SYNTAQLITE_LIKE_KEYWORD_REGEXP
          : (len == 5) ? SYNTAQLITE_LIKE_KEYWORD_MATCH
          : (yymsp[-1].minor.yy0.z[0] == 'g' || yymsp[-1].minor.yy0.z[0] == 'G')
              ? SYNTAQLITE_LIKE_KEYWORD_GLOB
              : SYNTAQLITE_LIKE_KEYWORD_LIKE;
      yylhsminor.yy277 =
          synq_parse_like_expr(pCtx, negated, kw, yymsp[-2].minor.yy277,
                               yymsp[0].minor.yy277, SYNTAQLITE_NULL_NODE);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 49: /* expr ::= expr likeop expr ESCAPE expr */
    {
      SyntaqliteBool negated = (yymsp[-3].minor.yy0.n & 0x80000000)
                                   ? SYNTAQLITE_BOOL_TRUE
                                   : SYNTAQLITE_BOOL_FALSE;
      uint32_t len = yymsp[-3].minor.yy0.n & 0x7FFFFFFF;
      SyntaqliteLikeKeyword kw =
          (len == 6)   ? SYNTAQLITE_LIKE_KEYWORD_REGEXP
          : (len == 5) ? SYNTAQLITE_LIKE_KEYWORD_MATCH
          : (yymsp[-3].minor.yy0.z[0] == 'g' || yymsp[-3].minor.yy0.z[0] == 'G')
              ? SYNTAQLITE_LIKE_KEYWORD_GLOB
              : SYNTAQLITE_LIKE_KEYWORD_LIKE;
      yylhsminor.yy277 =
          synq_parse_like_expr(pCtx, negated, kw, yymsp[-4].minor.yy277,
                               yymsp[-2].minor.yy277, yymsp[0].minor.yy277);
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 50: /* expr ::= CASE case_operand case_exprlist case_else END */
    {
      yymsp[-4].minor.yy277 =
          synq_parse_case_expr(pCtx, yymsp[-3].minor.yy277,
                               yymsp[-1].minor.yy277, yymsp[-2].minor.yy277);
    } break;
    case 51: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
    {
      uint32_t w = synq_parse_case_when(pCtx, yymsp[-2].minor.yy277,
                                        yymsp[0].minor.yy277);
      yylhsminor.yy277 =
          synq_parse_case_when_list(pCtx, yymsp[-4].minor.yy277, w);
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 52: /* case_exprlist ::= WHEN expr THEN expr */
    {
      uint32_t w = synq_parse_case_when(pCtx, yymsp[-2].minor.yy277,
                                        yymsp[0].minor.yy277);
      yymsp[-3].minor.yy277 =
          synq_parse_case_when_list(pCtx, SYNTAQLITE_NULL_NODE, w);
    } break;
    case 53:  /* case_else ::= ELSE expr */
    case 173: /* returning ::= RETURNING selcollist */
      yytestcase(yyruleno == 173);
    case 264: /* as ::= AS nmorerr */
      yytestcase(yyruleno == 264);
    case 271: /* from ::= FROM seltablist */
      yytestcase(yyruleno == 271);
    case 273: /* where_opt ::= WHERE expr */
      yytestcase(yyruleno == 273);
    case 277: /* having_opt ::= HAVING expr */
      yytestcase(yyruleno == 277);
    case 313: /* when_clause ::= WHEN expr */
      yytestcase(yyruleno == 313);
    case 349: /* key_opt ::= KEY expr */
      yytestcase(yyruleno == 349);
    case 352: /* vinto ::= INTO expr */
      yytestcase(yyruleno == 352);
    case 408: /* window_clause ::= WINDOW windowdefn_list */
      yytestcase(yyruleno == 408);
      {
        yymsp[-1].minor.yy277 = synq_pass(pCtx, yymsp[0].minor.yy277);
      }
      break;
    case 54: /* case_else ::= */
    case 56: /* case_operand ::= */
      yytestcase(yyruleno == 56);
    case 106: /* conslist_opt ::= */
      yytestcase(yyruleno == 106);
    case 131: /* eidlist_opt ::= */
      yytestcase(yyruleno == 131);
    case 165: /* idlist_opt ::= */
      yytestcase(yyruleno == 165);
    case 174: /* returning ::= */
      yytestcase(yyruleno == 174);
    case 191: /* exprlist ::= */
      yytestcase(yyruleno == 191);
    case 262: /* sclp ::= */
      yytestcase(yyruleno == 262);
    case 266: /* as ::= */
      yytestcase(yyruleno == 266);
    case 270: /* from ::= */
      yytestcase(yyruleno == 270);
    case 272: /* where_opt ::= */
      yytestcase(yyruleno == 272);
    case 274: /* groupby_opt ::= */
      yytestcase(yyruleno == 274);
    case 276: /* having_opt ::= */
      yytestcase(yyruleno == 276);
    case 278: /* orderby_opt ::= */
      yytestcase(yyruleno == 278);
    case 280: /* limit_opt ::= */
      yytestcase(yyruleno == 280);
    case 285: /* stl_prefix ::= */
      yytestcase(yyruleno == 285);
    case 312: /* when_clause ::= */
      yytestcase(yyruleno == 312);
    case 348: /* key_opt ::= */
      yytestcase(yyruleno == 348);
    case 353: /* vinto ::= */
      yytestcase(yyruleno == 353);
    case 393: /* frame_opt ::= */
      yytestcase(yyruleno == 393);
      {
        yymsp[1].minor.yy277 = SYNTAQLITE_NULL_NODE;
      }
      break;
    case 57: /* cmd ::= create_table create_table_args */
    {
      // yymsp[0].minor.yy277 is either: (1) a CreateTableStmt node with
      // columns/constraints filled in or: (2) a CreateTableStmt node with
      // as_select filled in yymsp[-1].minor.yy277 has the table
      // name/schema/temp/ifnotexists info packed as a node. We need to merge
      // yymsp[-1].minor.yy277 info into yymsp[0].minor.yy277.
      SyntaqliteNode* ct_node = AST_NODE(&pCtx->ast, yymsp[-1].minor.yy277);
      SyntaqliteNode* args_node = AST_NODE(&pCtx->ast, yymsp[0].minor.yy277);
      args_node->create_table_stmt.table_name =
          ct_node->create_table_stmt.table_name;
      args_node->create_table_stmt.schema = ct_node->create_table_stmt.schema;
      args_node->create_table_stmt.is_temp = ct_node->create_table_stmt.is_temp;
      args_node->create_table_stmt.if_not_exists =
          ct_node->create_table_stmt.if_not_exists;
      yylhsminor.yy277 = synq_pass(pCtx, yymsp[0].minor.yy277);
    }
      yymsp[-1].minor.yy277 = yylhsminor.yy277;
      break;
    case 58: /* create_table ::= createkw temp TABLE ifnotexists nm dbnm */
    {
      SyntaqliteTextSpan tbl_name =
          yymsp[0].minor.yy0.z ? synq_span_dequote(pCtx, yymsp[0].minor.yy0)
                               : synq_span_dequote(pCtx, yymsp[-1].minor.yy0);
      SyntaqliteTextSpan tbl_schema =
          yymsp[0].minor.yy0.z ? synq_span_dequote(pCtx, yymsp[-1].minor.yy0)
                               : SYNQ_NO_SPAN;
      yymsp[-5].minor.yy277 = synq_parse_create_table_stmt(
          pCtx, tbl_name, tbl_schema, (SyntaqliteBool)yymsp[-4].minor.yy320,
          (SyntaqliteBool)yymsp[-2].minor.yy320,
          (SyntaqliteCreateTableStmtFlags){.raw = 0}, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    } break;
    case 59: /* create_table_args ::= LP columnlist conslist_opt RP
                table_option_set */
    {
      yymsp[-4].minor.yy277 = synq_parse_create_table_stmt(
          pCtx, SYNQ_NO_SPAN, SYNQ_NO_SPAN, SYNTAQLITE_BOOL_FALSE,
          SYNTAQLITE_BOOL_FALSE,
          (SyntaqliteCreateTableStmtFlags){
              .raw = (uint8_t)(yymsp[0].minor.yy320 & 0xFF)},
          yymsp[-3].minor.yy277, yymsp[-2].minor.yy277, SYNTAQLITE_NULL_NODE);
    } break;
    case 60: /* create_table_args ::= AS select */
    {
      yymsp[-1].minor.yy277 = synq_parse_create_table_stmt(
          pCtx, SYNQ_NO_SPAN, SYNQ_NO_SPAN, SYNTAQLITE_BOOL_FALSE,
          SYNTAQLITE_BOOL_FALSE, (SyntaqliteCreateTableStmtFlags){.raw = 0},
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy277);
    } break;
    case 61: /* table_option_set ::= */
    case 88: /* autoinc ::= */
      yytestcase(yyruleno == 88);
    case 135: /* collate ::= */
      yytestcase(yyruleno == 135);
    case 226: /* ifexists ::= */
      yytestcase(yyruleno == 226);
    case 236: /* kwcolumn_opt ::= */
      yytestcase(yyruleno == 236);
    case 250: /* savepoint_opt ::= */
      yytestcase(yyruleno == 250);
    case 359: /* uniqueflag ::= */
      yytestcase(yyruleno == 359);
    case 360: /* ifnotexists ::= */
      yytestcase(yyruleno == 360);
    case 365: /* temp ::= */
      yytestcase(yyruleno == 365);
      {
        yymsp[1].minor.yy320 = 0;
      }
      break;
    case 62:  /* table_option_set ::= table_option */
    case 118: /* defer_subclause_opt ::= defer_subclause */
      yytestcase(yyruleno == 118);
      {
        // passthrough
      }
      break;
    case 63: /* table_option_set ::= table_option_set COMMA table_option */
    {
      yylhsminor.yy320 = yymsp[-2].minor.yy320 | yymsp[0].minor.yy320;
    }
      yymsp[-2].minor.yy320 = yylhsminor.yy320;
      break;
    case 64: /* table_option ::= WITHOUT nm */
    {
      // WITHOUT ROWID = bit 0
      if (yymsp[0].minor.yy0.n == 5 &&
          SYNQ_STRNCASECMP(yymsp[0].minor.yy0.z, "rowid", 5) == 0) {
        yymsp[-1].minor.yy320 = 1;
      } else {
        yymsp[-1].minor.yy320 = 0;
        pCtx->error = 1;
      }
    } break;
    case 65: /* table_option ::= nm */
    {
      // STRICT = bit 1
      if (yymsp[0].minor.yy0.n == 6 &&
          SYNQ_STRNCASECMP(yymsp[0].minor.yy0.z, "strict", 6) == 0) {
        yylhsminor.yy320 = 2;
      } else {
        yylhsminor.yy320 = 0;
        pCtx->error = 1;
      }
    }
      yymsp[0].minor.yy320 = yylhsminor.yy320;
      break;
    case 66: /* columnlist ::= columnlist COMMA columnname carglist */
    {
      uint32_t col = synq_parse_column_def(pCtx, yymsp[-1].minor.yy640.name,
                                           yymsp[-1].minor.yy640.typetoken,
                                           yymsp[0].minor.yy430.list);
      yylhsminor.yy277 =
          synq_parse_column_def_list(pCtx, yymsp[-3].minor.yy277, col);
    }
      yymsp[-3].minor.yy277 = yylhsminor.yy277;
      break;
    case 67: /* columnlist ::= columnname carglist */
    {
      uint32_t col = synq_parse_column_def(pCtx, yymsp[-1].minor.yy640.name,
                                           yymsp[-1].minor.yy640.typetoken,
                                           yymsp[0].minor.yy430.list);
      yylhsminor.yy277 =
          synq_parse_column_def_list(pCtx, SYNTAQLITE_NULL_NODE, col);
    }
      yymsp[-1].minor.yy277 = yylhsminor.yy277;
      break;
    case 68: /* carglist ::= carglist ccons */
    {
      if (yymsp[0].minor.yy150.node != SYNTAQLITE_NULL_NODE) {
        // The name stays pending: SQLite reads it without clearing, so it
        // names every constraint until a new column or a tconscomma.
        SyntaqliteNode* node = AST_NODE(&pCtx->ast, yymsp[0].minor.yy150.node);
        node->column_constraint.constraint_name = pCtx->constraint_name;
        if (yymsp[-1].minor.yy430.list == SYNTAQLITE_NULL_NODE) {
          yylhsminor.yy430.list = synq_parse_column_constraint_list(
              pCtx, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy150.node);
        } else {
          yylhsminor.yy430.list = synq_parse_column_constraint_list(
              pCtx, yymsp[-1].minor.yy430.list, yymsp[0].minor.yy150.node);
        }
        yylhsminor.yy430.pending_name = yymsp[-1].minor.yy430.pending_name;
        yylhsminor.yy430.last_node = yymsp[0].minor.yy150.node;
      } else if (yymsp[0].minor.yy150.pending_name.length > 0) {
        // CONSTRAINT nm — store pending name for next constraint
        yylhsminor.yy430.list = yymsp[-1].minor.yy430.list;
        yylhsminor.yy430.pending_name = yymsp[0].minor.yy150.pending_name;
        yylhsminor.yy430.last_node = yymsp[-1].minor.yy430.last_node;
      } else {
        yylhsminor.yy430 = yymsp[-1].minor.yy430;
      }
    }
      yymsp[-1].minor.yy430 = yylhsminor.yy430;
      break;
    case 69: /* carglist ::= */
    {
      yymsp[1].minor.yy430.list = SYNTAQLITE_NULL_NODE;
      yymsp[1].minor.yy430.pending_name = SYNQ_NO_SPAN;
      yymsp[1].minor.yy430.last_node = SYNTAQLITE_NULL_NODE;
    } break;
    case 70:  /* ccons ::= CONSTRAINT nm */
    case 112: /* tcons ::= CONSTRAINT nm */
      yytestcase(yyruleno == 112);
      {
        yymsp[-1].minor.yy150.node = SYNTAQLITE_NULL_NODE;
        yymsp[-1].minor.yy150.pending_name =
            synq_span(pCtx, yymsp[0].minor.yy0);
        pCtx->constraint_name = yymsp[-1].minor.yy150.pending_name;
      }
      break;
    case 71: /* ccons ::= DEFAULT scantok term */
    {
      yymsp[-2].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, yymsp[0].minor.yy277,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-2].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 72: /* ccons ::= DEFAULT LP expr RP */
    {
      yymsp[-3].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_TRUE, SYNTAQLITE_BOOL_FALSE, yymsp[-1].minor.yy277,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-3].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 73: /* ccons ::= DEFAULT PLUS scantok term */
    {
      uint32_t pos = synq_parse_unary_expr(pCtx, SYNTAQLITE_UNARY_OP_PLUS,
                                           yymsp[0].minor.yy277);
      yymsp[-3].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, pos,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-3].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 74: /* ccons ::= DEFAULT MINUS scantok term */
    {
      // Create a unary minus wrapping the term
      uint32_t neg = synq_parse_unary_expr(pCtx, SYNTAQLITE_UNARY_OP_MINUS,
                                           yymsp[0].minor.yy277);
      yymsp[-3].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, neg,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-3].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 75: /* ccons ::= DEFAULT scantok ID|INDEXED */
    {
      // `DEFAULT foo` is a string literal upstream (tokenExpr TK_STRING), not
      // a column reference: as an expression it would not be constant.
      uint32_t ref = synq_parse_literal(pCtx, SYNTAQLITE_LITERAL_TYPE_STRING,
                                        synq_span(pCtx, yymsp[0].minor.yy0));
      yymsp[-2].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, ref,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-2].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 76: /* ccons ::= NULL onconf */
    {
      yymsp[-1].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_NULL, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[0].minor.yy320,
          SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-1].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 77: /* ccons ::= NOT NULL onconf */
    {
      yymsp[-2].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_NOT_NULL, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[0].minor.yy320,
          SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-2].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 78: /* ccons ::= PRIMARY KEY sortorder onconf autoinc */
    {
      yymsp[-4].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_PRIMARY_KEY, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[-1].minor.yy320,
          (SyntaqliteSortOrder)yymsp[-2].minor.yy277,
          (SyntaqliteBool)yymsp[0].minor.yy320, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-4].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 79: /* ccons ::= UNIQUE onconf */
    {
      yymsp[-1].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_UNIQUE, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[0].minor.yy320,
          SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-1].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 80: /* ccons ::= CHECK LP expr RP */
    {
      yymsp[-3].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_CHECK, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE,
          yymsp[-1].minor.yy277, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-3].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 81: /* ccons ::= REFERENCES nm eidlist_opt refargs */
    {
      uint32_t fk = synq_parse_foreign_key_clause(
          pCtx, synq_span(pCtx, yymsp[-2].minor.yy0), yymsp[-1].minor.yy277,
          yymsp[0].minor.yy603.match_name, yymsp[0].minor.yy603.on_delete,
          yymsp[0].minor.yy603.on_update, yymsp[0].minor.yy603.on_insert,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET);
      yymsp[-3].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_REFERENCES, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, fk);
      yymsp[-3].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 82: /* ccons ::= defer_subclause */
    {
      yylhsminor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFERRABLE, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          yymsp[0].minor.yy519.deferrable, yymsp[0].minor.yy519.initial,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yylhsminor.yy150.pending_name = SYNQ_NO_SPAN;
    }
      yymsp[0].minor.yy150 = yylhsminor.yy150;
      break;
    case 83: /* ccons ::= COLLATE ID|STRING */
    {
      yymsp[-1].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_COLLATE, SYNQ_NO_SPAN, 0, 0,
          0, synq_span(pCtx, yymsp[0].minor.yy0),
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-1].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 84: /* ccons ::= GENERATED ALWAYS AS generated */
    {
      yymsp[-3].minor.yy150 = yymsp[0].minor.yy150;
      if (yymsp[-3].minor.yy150.node != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode* node = AST_NODE(&pCtx->ast, yymsp[-3].minor.yy150.node);
        node->column_constraint.generated_always = SYNTAQLITE_BOOL_TRUE;
      }
    } break;
    case 85: /* ccons ::= AS generated */
    {
      yymsp[-1].minor.yy150 = yymsp[0].minor.yy150;
      if (yymsp[-1].minor.yy150.node != SYNTAQLITE_NULL_NODE &&
          pCtx->generated_always) {
        SyntaqliteNode* node = AST_NODE(&pCtx->ast, yymsp[-1].minor.yy150.node);
        node->column_constraint.generated_always = SYNTAQLITE_BOOL_TRUE;
      }
    } break;
    case 86: /* generated ::= LP expr RP */
    {
      yymsp[-2].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_GENERATED, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, yymsp[-1].minor.yy277, SYNTAQLITE_NULL_NODE);
      yymsp[-2].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 87: /* generated ::= LP expr RP ID */
    {
      SyntaqliteGeneratedColumnStorage storage =
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL;
      if (yymsp[0].minor.yy0.n == 6 &&
          SYNQ_STRNCASECMP(yymsp[0].minor.yy0.z, "stored", 6) == 0) {
        storage = SYNTAQLITE_GENERATED_COLUMN_STORAGE_STORED;
      }
      yymsp[-3].minor.yy150.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_GENERATED, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN, storage,
          SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, yymsp[-2].minor.yy277, SYNTAQLITE_NULL_NODE);
      yymsp[-3].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 89:  /* autoinc ::= AUTOINCR */
    case 237: /* kwcolumn_opt ::= COLUMNKW */
      yytestcase(yyruleno == 237);
    case 358: /* uniqueflag ::= UNIQUE */
      yytestcase(yyruleno == 358);
    case 364: /* temp ::= TEMP */
      yytestcase(yyruleno == 364);
      {
        yymsp[0].minor.yy320 = 1;
      }
      break;
    case 90: /* refargs ::= */
    {
      yymsp[1].minor.yy603.on_delete = SYNTAQLITE_FOREIGN_KEY_ACTION_UNSET;
      yymsp[1].minor.yy603.on_update = SYNTAQLITE_FOREIGN_KEY_ACTION_UNSET;
      yymsp[1].minor.yy603.on_insert = SYNTAQLITE_FOREIGN_KEY_ACTION_UNSET;
      yymsp[1].minor.yy603.match_name = SYNQ_NO_SPAN;
    } break;
    case 91: /* refargs ::= refargs refarg */
    {
      switch (yymsp[0].minor.yy76.kind) {
        case SYNQ_REFARG_DELETE:
          yymsp[-1].minor.yy603.on_delete = yymsp[0].minor.yy76.action;
          break;
        case SYNQ_REFARG_UPDATE:
          yymsp[-1].minor.yy603.on_update = yymsp[0].minor.yy76.action;
          break;
        case SYNQ_REFARG_INSERT:
          yymsp[-1].minor.yy603.on_insert = yymsp[0].minor.yy76.action;
          break;
        case SYNQ_REFARG_MATCH:
          yymsp[-1].minor.yy603.match_name = yymsp[0].minor.yy76.match_name;
          break;
        default:
          break;
      }
    } break;
    case 92: /* refarg ::= MATCH nm */
    {
      yymsp[-1].minor.yy76.kind = SYNQ_REFARG_MATCH;
      yymsp[-1].minor.yy76.action = SYNTAQLITE_FOREIGN_KEY_ACTION_UNSET;
      yymsp[-1].minor.yy76.match_name = synq_span(pCtx, yymsp[0].minor.yy0);
    } break;
    case 93: /* refarg ::= ON INSERT refact */
    {
      yymsp[-2].minor.yy76.kind = SYNQ_REFARG_INSERT;
      yymsp[-2].minor.yy76.action =
          (SyntaqliteForeignKeyAction)yymsp[0].minor.yy320;
      yymsp[-2].minor.yy76.match_name = SYNQ_NO_SPAN;
    } break;
    case 94: /* refarg ::= ON DELETE refact */
    {
      yymsp[-2].minor.yy76.kind = SYNQ_REFARG_DELETE;
      yymsp[-2].minor.yy76.action =
          (SyntaqliteForeignKeyAction)yymsp[0].minor.yy320;
      yymsp[-2].minor.yy76.match_name = SYNQ_NO_SPAN;
    } break;
    case 95: /* refarg ::= ON UPDATE refact */
    {
      yymsp[-2].minor.yy76.kind = SYNQ_REFARG_UPDATE;
      yymsp[-2].minor.yy76.action =
          (SyntaqliteForeignKeyAction)yymsp[0].minor.yy320;
      yymsp[-2].minor.yy76.match_name = SYNQ_NO_SPAN;
    } break;
    case 96: /* refact ::= SET NULL */
    {
      yymsp[-1].minor.yy320 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_SET_NULL;
    } break;
    case 97: /* refact ::= SET DEFAULT */
    {
      yymsp[-1].minor.yy320 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_SET_DEFAULT;
    } break;
    case 98: /* refact ::= CASCADE */
    {
      yymsp[0].minor.yy320 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_CASCADE;
    } break;
    case 99: /* refact ::= RESTRICT */
    {
      yymsp[0].minor.yy320 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_RESTRICT;
    } break;
    case 100: /* refact ::= NO ACTION */
    {
      yymsp[-1].minor.yy320 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION;
    } break;
    case 101: /* defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
    {
      yymsp[-2].minor.yy519.deferrable = SYNTAQLITE_DEFERRABLE_NOT_DEFERRABLE;
      yymsp[-2].minor.yy519.initial =
          (SyntaqliteInitialDeferMode)yymsp[0].minor.yy320;
    } break;
    case 102: /* defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
    {
      yymsp[-1].minor.yy519.deferrable = SYNTAQLITE_DEFERRABLE_DEFERRABLE;
      yymsp[-1].minor.yy519.initial =
          (SyntaqliteInitialDeferMode)yymsp[0].minor.yy320;
    } break;
    case 103: /* init_deferred_pred_opt ::= */
    {
      yymsp[1].minor.yy320 = (int)SYNTAQLITE_INITIAL_DEFER_MODE_UNSET;
    } break;
    case 104: /* init_deferred_pred_opt ::= INITIALLY DEFERRED */
    {
      yymsp[-1].minor.yy320 = (int)SYNTAQLITE_INITIAL_DEFER_MODE_DEFERRED;
    } break;
    case 105: /* init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
    {
      yymsp[-1].minor.yy320 = (int)SYNTAQLITE_INITIAL_DEFER_MODE_IMMEDIATE;
    } break;
    case 107: /* conslist_opt ::= COMMA conslist */
    {
      yymsp[-1].minor.yy277 = yymsp[0].minor.yy430.list;
    } break;
    case 108: /* conslist ::= conslist tconscomma tcons */
    {
      // If comma separator was present, clear pending constraint name
      SyntaqliteTextSpan pending = pCtx->constraint_name;
      if (yymsp[0].minor.yy150.node != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode* node = AST_NODE(&pCtx->ast, yymsp[0].minor.yy150.node);
        node->table_constraint.constraint_name = pending;
        if (yymsp[-2].minor.yy430.list == SYNTAQLITE_NULL_NODE) {
          yylhsminor.yy430.list = synq_parse_table_constraint_list(
              pCtx, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy150.node);
        } else {
          yylhsminor.yy430.list = synq_parse_table_constraint_list(
              pCtx, yymsp[-2].minor.yy430.list, yymsp[0].minor.yy150.node);
        }
        yylhsminor.yy430.pending_name = pending;
        yylhsminor.yy430.last_node = yymsp[0].minor.yy150.node;
      } else if (yymsp[0].minor.yy150.pending_name.length > 0) {
        yylhsminor.yy430.list = yymsp[-2].minor.yy430.list;
        yylhsminor.yy430.pending_name = yymsp[0].minor.yy150.pending_name;
        yylhsminor.yy430.last_node = yymsp[-2].minor.yy430.last_node;
      } else {
        yylhsminor.yy430 = yymsp[-2].minor.yy430;
      }
    }
      yymsp[-2].minor.yy430 = yylhsminor.yy430;
      break;
    case 109: /* conslist ::= tcons */
    {
      if (yymsp[0].minor.yy150.node != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode* node = AST_NODE(&pCtx->ast, yymsp[0].minor.yy150.node);
        node->table_constraint.constraint_name = pCtx->constraint_name;
        yylhsminor.yy430.list = synq_parse_table_constraint_list(
            pCtx, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy150.node);
        yylhsminor.yy430.pending_name = pCtx->constraint_name;
        yylhsminor.yy430.last_node = yymsp[0].minor.yy150.node;
      } else {
        yylhsminor.yy430.list = SYNTAQLITE_NULL_NODE;
        yylhsminor.yy430.pending_name = yymsp[0].minor.yy150.pending_name;
        yylhsminor.yy430.last_node = SYNTAQLITE_NULL_NODE;
      }
    }
      yymsp[0].minor.yy430 = yylhsminor.yy430;
      break;
    case 110: /* tconscomma ::= COMMA */
    {
      pCtx->constraint_name = SYNQ_NO_SPAN;
    } break;
    case 111: /* tconscomma ::= */
    {
    } break;
    case 113: /* tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
    {
      yymsp[-6].minor.yy150.node = synq_parse_table_constraint(
          pCtx, SYNTAQLITE_TABLE_CONSTRAINT_TYPE_PRIMARY_KEY, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[0].minor.yy320,
          (SyntaqliteBool)yymsp[-2].minor.yy320, yymsp[-3].minor.yy277,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-6].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 114: /* tcons ::= UNIQUE LP sortlist RP onconf */
    {
      yymsp[-4].minor.yy150.node = synq_parse_table_constraint(
          pCtx, SYNTAQLITE_TABLE_CONSTRAINT_TYPE_UNIQUE, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[0].minor.yy320, SYNTAQLITE_BOOL_FALSE,
          yymsp[-2].minor.yy277, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE);
      yymsp[-4].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 115: /* tcons ::= CHECK LP expr RP onconf */
    {
      yymsp[-4].minor.yy150.node = synq_parse_table_constraint(
          pCtx, SYNTAQLITE_TABLE_CONSTRAINT_TYPE_CHECK, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[0].minor.yy320, SYNTAQLITE_BOOL_FALSE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, yymsp[-2].minor.yy277,
          SYNTAQLITE_NULL_NODE);
      yymsp[-4].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 116: /* tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt
                 refargs defer_subclause_opt */
    {
      uint32_t fk = synq_parse_foreign_key_clause(
          pCtx, synq_span(pCtx, yymsp[-3].minor.yy0), yymsp[-2].minor.yy277,
          yymsp[-1].minor.yy603.match_name, yymsp[-1].minor.yy603.on_delete,
          yymsp[-1].minor.yy603.on_update, yymsp[-1].minor.yy603.on_insert,
          yymsp[0].minor.yy519.deferrable, yymsp[0].minor.yy519.initial);
      yymsp[-9].minor.yy150.node = synq_parse_table_constraint(
          pCtx, SYNTAQLITE_TABLE_CONSTRAINT_TYPE_FOREIGN_KEY, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_BOOL_FALSE,
          SYNTAQLITE_NULL_NODE, yymsp[-6].minor.yy277, SYNTAQLITE_NULL_NODE,
          fk);
      yymsp[-9].minor.yy150.pending_name = SYNQ_NO_SPAN;
    } break;
    case 117: /* defer_subclause_opt ::= */
    {
      yymsp[1].minor.yy519.deferrable = SYNTAQLITE_DEFERRABLE_UNSET;
      yymsp[1].minor.yy519.initial = SYNTAQLITE_INITIAL_DEFER_MODE_UNSET;
    } break;
    case 119: /* onconf ::= */
    case 146: /* orconf ::= */
      yytestcase(yyruleno == 146);
      {
        yymsp[1].minor.yy320 = (int)SYNTAQLITE_CONFLICT_ACTION_DEFAULT;
      }
      break;
    case 120: /* onconf ::= ON CONFLICT resolvetype */
    {
      yymsp[-2].minor.yy320 = yymsp[0].minor.yy320;
    } break;
    case 121: /* scantok ::= */
    case 155: /* indexed_opt ::= */
      yytestcase(yyruleno == 155);
    case 246: /* trans_opt ::= */
      yytestcase(yyruleno == 246);
    case 263: /* scanpt ::= */
      yytestcase(yyruleno == 263);
      {
        yymsp[1].minor.yy0.z = NULL;
        yymsp[1].minor.yy0.n = 0;
      }
      break;
    case 122: /* select ::= WITH wqlist selectnowith */
    {
      yymsp[-2].minor.yy277 = synq_parse_with_clause(
          pCtx, 0, yymsp[-1].minor.yy277, yymsp[0].minor.yy277);
    } break;
    case 123: /* select ::= WITH RECURSIVE wqlist selectnowith */
    {
      yymsp[-3].minor.yy277 = synq_parse_with_clause(
          pCtx, 1, yymsp[-1].minor.yy277, yymsp[0].minor.yy277);
    } break;
    case 124: /* wqitem ::= withnm eidlist_opt wqas LP select RP */
    {
      yylhsminor.yy277 = synq_parse_cte_definition(
          pCtx, synq_span_dequote(pCtx, yymsp[-5].minor.yy0),
          (SyntaqliteMaterialized)yymsp[-3].minor.yy320, yymsp[-4].minor.yy277,
          yymsp[-1].minor.yy277);
    }
      yymsp[-5].minor.yy277 = yylhsminor.yy277;
      break;
    case 125: /* wqlist ::= wqitem */
    {
      yylhsminor.yy277 =
          synq_parse_cte_list(pCtx, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy277);
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 126: /* wqlist ::= wqlist COMMA wqitem */
    {
      yymsp[-2].minor.yy277 = synq_parse_cte_list(pCtx, yymsp[-2].minor.yy277,
                                                  yymsp[0].minor.yy277);
    } break;
    case 127: /* withnm ::= nm */
    {
      // Token passthrough - nm already produces SynqParseToken
    } break;
    case 128: /* wqas ::= AS */
    {
      yymsp[0].minor.yy320 = (int)SYNTAQLITE_MATERIALIZED_DEFAULT;
    } break;
    case 129: /* wqas ::= AS MATERIALIZED */
    {
      yymsp[-1].minor.yy320 = (int)SYNTAQLITE_MATERIALIZED_MATERIALIZED;
    } break;
    case 130: /* wqas ::= AS NOT MATERIALIZED */
    {
      yymsp[-2].minor.yy320 = (int)SYNTAQLITE_MATERIALIZED_NOT_MATERIALIZED;
    } break;
    case 132: /* eidlist_opt ::= LP eidlist RP */
    case 166: /* idlist_opt ::= LP idlist RP */
      yytestcase(yyruleno == 166);
    case 324: /* trigger_cmd ::= scanpt select scanpt */
      yytestcase(yyruleno == 324);
      {
        yymsp[-2].minor.yy277 = synq_pass(pCtx, yymsp[-1].minor.yy277);
      }
      break;
    case 133: /* eidlist ::= nm collate sortorder */
    {
      (void)yymsp[-1].minor.yy320;
      (void)yymsp[0].minor.yy277;
      uint32_t col = synq_parse_column_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[-2].minor.yy0), SYNQ_NO_SPAN,
          SYNQ_NO_SPAN);
      yylhsminor.yy277 = synq_parse_expr_list(pCtx, SYNTAQLITE_NULL_NODE, col);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 134: /* eidlist ::= eidlist COMMA nm collate sortorder */
    {
      (void)yymsp[-1].minor.yy320;
      (void)yymsp[0].minor.yy277;
      uint32_t col = synq_parse_column_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[-2].minor.yy0), SYNQ_NO_SPAN,
          SYNQ_NO_SPAN);
      yymsp[-4].minor.yy277 =
          synq_parse_expr_list(pCtx, yymsp[-4].minor.yy277, col);
    } break;
    case 136: /* collate ::= COLLATE ID|STRING */
    case 225: /* ifexists ::= IF EXISTS */
      yytestcase(yyruleno == 225);
      {
        yymsp[-1].minor.yy320 = 1;
      }
      break;
    case 137: /* with ::= */
    {
      yymsp[1].minor.yy541.cte_list = SYNTAQLITE_NULL_NODE;
      yymsp[1].minor.yy541.is_recursive = 0;
    } break;
    case 138: /* with ::= WITH wqlist */
    {
      yymsp[-1].minor.yy541.cte_list = yymsp[0].minor.yy277;
      yymsp[-1].minor.yy541.is_recursive = 0;
    } break;
    case 139: /* with ::= WITH RECURSIVE wqlist */
    {
      yymsp[-2].minor.yy541.cte_list = yymsp[0].minor.yy277;
      yymsp[-2].minor.yy541.is_recursive = 1;
    } break;
    case 140: /* cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret
                 orderby_opt limit_opt */
    {
      if (yymsp[-1].minor.yy277 != SYNTAQLITE_NULL_NODE ||
          yymsp[0].minor.yy277 != SYNTAQLITE_NULL_NODE) {
        pCtx->saw_update_delete_limit = 1;
        if (!SYNQ_HAS_CFLAG(pCtx->env,
                            SYNQ_CFLAG_IDX_ENABLE_UPDATE_DELETE_LIMIT)) {
          pCtx->error = 1;
        }
      }
      SyntaqliteIndexHint ih =
          (yymsp[-3].minor.yy0.z != NULL) ? SYNTAQLITE_INDEX_HINT_INDEXED
          : (yymsp[-3].minor.yy0.n == 1)  ? SYNTAQLITE_INDEX_HINT_NOT_INDEXED
                                          : SYNTAQLITE_INDEX_HINT_DEFAULT;
      yylhsminor.yy277 = synq_parse_delete_stmt(
          pCtx, yymsp[-7].minor.yy541.cte_list,
          yymsp[-7].minor.yy541.is_recursive ? SYNTAQLITE_BOOL_TRUE
                                             : SYNTAQLITE_BOOL_FALSE,
          yymsp[-4].minor.yy277, ih, synq_span(pCtx, yymsp[-3].minor.yy0),
          yymsp[-2].minor.yy119.where_expr, yymsp[-1].minor.yy277,
          yymsp[0].minor.yy277, yymsp[-2].minor.yy119.returning);
    }
      yymsp[-7].minor.yy277 = yylhsminor.yy277;
      break;
    case 141: /* cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist
                 from where_opt_ret orderby_opt limit_opt */
    {
      if (yymsp[-1].minor.yy277 != SYNTAQLITE_NULL_NODE ||
          yymsp[0].minor.yy277 != SYNTAQLITE_NULL_NODE) {
        pCtx->saw_update_delete_limit = 1;
        if (!SYNQ_HAS_CFLAG(pCtx->env,
                            SYNQ_CFLAG_IDX_ENABLE_UPDATE_DELETE_LIMIT)) {
          pCtx->error = 1;
        }
      }
      SyntaqliteIndexHint ih =
          (yymsp[-6].minor.yy0.z != NULL) ? SYNTAQLITE_INDEX_HINT_INDEXED
          : (yymsp[-6].minor.yy0.n == 1)  ? SYNTAQLITE_INDEX_HINT_NOT_INDEXED
                                          : SYNTAQLITE_INDEX_HINT_DEFAULT;
      yylhsminor.yy277 = synq_parse_update_stmt(
          pCtx, yymsp[-10].minor.yy541.cte_list,
          yymsp[-10].minor.yy541.is_recursive ? SYNTAQLITE_BOOL_TRUE
                                              : SYNTAQLITE_BOOL_FALSE,
          (SyntaqliteConflictAction)yymsp[-8].minor.yy320,
          yymsp[-7].minor.yy277, ih, synq_span(pCtx, yymsp[-6].minor.yy0),
          yymsp[-4].minor.yy277, yymsp[-3].minor.yy277,
          yymsp[-2].minor.yy119.where_expr, yymsp[-1].minor.yy277,
          yymsp[0].minor.yy277, yymsp[-2].minor.yy119.returning);
    }
      yymsp[-10].minor.yy277 = yylhsminor.yy277;
      break;
    case 142: /* cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert
               */
    {
      yylhsminor.yy277 = synq_parse_insert_stmt(
          pCtx, yymsp[-6].minor.yy541.cte_list,
          yymsp[-6].minor.yy541.is_recursive ? SYNTAQLITE_BOOL_TRUE
                                             : SYNTAQLITE_BOOL_FALSE,
          (SyntaqliteConflictAction)yymsp[-5].minor.yy320,
          yymsp[-3].minor.yy277, yymsp[-2].minor.yy277, yymsp[-1].minor.yy277,
          yymsp[0].minor.yy352.clauses, yymsp[0].minor.yy352.returning);
    }
      yymsp[-6].minor.yy277 = yylhsminor.yy277;
      break;
    case 143: /* cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT
                 VALUES returning */
    {
      yylhsminor.yy277 = synq_parse_insert_stmt(
          pCtx, yymsp[-7].minor.yy541.cte_list,
          yymsp[-7].minor.yy541.is_recursive ? SYNTAQLITE_BOOL_TRUE
                                             : SYNTAQLITE_BOOL_FALSE,
          (SyntaqliteConflictAction)yymsp[-6].minor.yy320,
          yymsp[-4].minor.yy277, yymsp[-3].minor.yy277, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy277);
    }
      yymsp[-7].minor.yy277 = yylhsminor.yy277;
      break;
    case 144: /* insert_cmd ::= INSERT orconf */
    case 147: /* orconf ::= OR resolvetype */
      yytestcase(yyruleno == 147);
    case 404: /* frame_exclude_opt ::= EXCLUDE frame_exclude */
      yytestcase(yyruleno == 404);
      {
        yymsp[-1].minor.yy320 = yymsp[0].minor.yy320;
      }
      break;
    case 145: /* insert_cmd ::= REPLACE */
    case 150: /* resolvetype ::= REPLACE */
      yytestcase(yyruleno == 150);
      {
        yymsp[0].minor.yy320 = (int)SYNTAQLITE_CONFLICT_ACTION_REPLACE;
      }
      break;
    case 148: /* resolvetype ::= raisetype */
    {
      // raisetype: ROLLBACK=1, ABORT=2, FAIL=3 (SynqRaiseType enum values)
      // ConflictAction: ROLLBACK=1, ABORT=2, FAIL=3 (same values, direct
      // passthrough)
      yylhsminor.yy320 = yymsp[0].minor.yy320;
    }
      yymsp[0].minor.yy320 = yylhsminor.yy320;
      break;
    case 149: /* resolvetype ::= IGNORE */
    {
      yymsp[0].minor.yy320 = (int)SYNTAQLITE_CONFLICT_ACTION_IGNORE;
    } break;
    case 151: /* xfullname ::= nm */
    {
      yylhsminor.yy277 = synq_parse_table_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0), SYNQ_NO_SPAN,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 152: /* xfullname ::= nm DOT nm */
    {
      yylhsminor.yy277 = synq_parse_table_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0),
          synq_span_dequote(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_BOOL_FALSE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 153: /* xfullname ::= nm DOT nm AS nm */
    {
      uint32_t alias = synq_parse_ident_name(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0));
      yylhsminor.yy277 = synq_parse_table_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[-2].minor.yy0),
          synq_span_dequote(pCtx, yymsp[-4].minor.yy0), SYNTAQLITE_BOOL_FALSE,
          alias, SYNTAQLITE_NULL_NODE, SYNTAQLITE_INDEX_HINT_DEFAULT,
          SYNQ_NO_SPAN);
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 154: /* xfullname ::= nm AS nm */
    {
      uint32_t alias = synq_parse_ident_name(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0));
      yylhsminor.yy277 = synq_parse_table_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[-2].minor.yy0), SYNQ_NO_SPAN,
          SYNTAQLITE_BOOL_FALSE, alias, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 156: /* indexed_opt ::= indexed_by */
    case 316: /* trnm ::= nm */
      yytestcase(yyruleno == 316);
    case 330: /* nmnum ::= plus_num */
      yytestcase(yyruleno == 330);
    case 331: /* nmnum ::= nm */
      yytestcase(yyruleno == 331);
    case 332: /* nmnum ::= ON */
      yytestcase(yyruleno == 332);
    case 333: /* nmnum ::= DELETE */
      yytestcase(yyruleno == 333);
    case 334: /* nmnum ::= DEFAULT */
      yytestcase(yyruleno == 334);
    case 336: /* plus_num ::= INTEGER|FLOAT */
      yytestcase(yyruleno == 336);
    case 338: /* signed ::= plus_num */
      yytestcase(yyruleno == 338);
    case 339: /* signed ::= minus_num */
      yytestcase(yyruleno == 339);
    case 363: /* createkw ::= CREATE */
      yytestcase(yyruleno == 363);
      {
        // Token passthrough
      }
      break;
    case 157: /* where_opt_ret ::= */
    {
      yymsp[1].minor.yy119.where_expr = SYNTAQLITE_NULL_NODE;
      yymsp[1].minor.yy119.returning = SYNTAQLITE_NULL_NODE;
    } break;
    case 158: /* where_opt_ret ::= WHERE expr */
    {
      yymsp[-1].minor.yy119.where_expr = yymsp[0].minor.yy277;
      yymsp[-1].minor.yy119.returning = SYNTAQLITE_NULL_NODE;
    } break;
    case 159: /* where_opt_ret ::= RETURNING selcollist */
    {
      yymsp[-1].minor.yy119.where_expr = SYNTAQLITE_NULL_NODE;
      yymsp[-1].minor.yy119.returning = yymsp[0].minor.yy277;
    } break;
    case 160: /* where_opt_ret ::= WHERE expr RETURNING selcollist */
    {
      yymsp[-3].minor.yy119.where_expr = yymsp[-2].minor.yy277;
      yymsp[-3].minor.yy119.returning = yymsp[0].minor.yy277;
    } break;
    case 161: /* setlist ::= setlist COMMA nm EQ expr */
    {
      uint32_t clause =
          synq_parse_set_clause(pCtx, synq_span(pCtx, yymsp[-2].minor.yy0),
                                SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy277);
      yylhsminor.yy277 =
          synq_parse_set_clause_list(pCtx, yymsp[-4].minor.yy277, clause);
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 162: /* setlist ::= setlist COMMA LP idlist RP EQ expr */
    {
      uint32_t clause = synq_parse_set_clause(
          pCtx, SYNQ_NO_SPAN, yymsp[-3].minor.yy277, yymsp[0].minor.yy277);
      yylhsminor.yy277 =
          synq_parse_set_clause_list(pCtx, yymsp[-6].minor.yy277, clause);
    }
      yymsp[-6].minor.yy277 = yylhsminor.yy277;
      break;
    case 163: /* setlist ::= nm EQ expr */
    {
      uint32_t clause =
          synq_parse_set_clause(pCtx, synq_span(pCtx, yymsp[-2].minor.yy0),
                                SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy277);
      yylhsminor.yy277 =
          synq_parse_set_clause_list(pCtx, SYNTAQLITE_NULL_NODE, clause);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 164: /* setlist ::= LP idlist RP EQ expr */
    {
      uint32_t clause = synq_parse_set_clause(
          pCtx, SYNQ_NO_SPAN, yymsp[-3].minor.yy277, yymsp[0].minor.yy277);
      yymsp[-4].minor.yy277 =
          synq_parse_set_clause_list(pCtx, SYNTAQLITE_NULL_NODE, clause);
    } break;
    case 167: /* upsert ::= */
    {
      yymsp[1].minor.yy352.clauses = SYNTAQLITE_NULL_NODE;
      yymsp[1].minor.yy352.returning = SYNTAQLITE_NULL_NODE;
    } break;
    case 168: /* upsert ::= RETURNING selcollist */
    {
      yymsp[-1].minor.yy352.clauses = SYNTAQLITE_NULL_NODE;
      yymsp[-1].minor.yy352.returning = yymsp[0].minor.yy277;
    } break;
    case 169: /* upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET
                 setlist where_opt upsert */
    {
      uint32_t clause = synq_parse_upsert_clause(
          pCtx, yymsp[-8].minor.yy277, yymsp[-6].minor.yy277,
          (SyntaqliteUpsertAction)SYNTAQLITE_UPSERT_ACTION_UPDATE,
          yymsp[-2].minor.yy277, yymsp[-1].minor.yy277);
      yymsp[-11].minor.yy352.clauses = synq_parse_upsert_clause_list(
          pCtx, yymsp[0].minor.yy352.clauses, clause);
      yymsp[-11].minor.yy352.returning = yymsp[0].minor.yy352.returning;
    } break;
    case 170: /* upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING
                 upsert */
    {
      uint32_t clause = synq_parse_upsert_clause(
          pCtx, yymsp[-5].minor.yy277, yymsp[-3].minor.yy277,
          (SyntaqliteUpsertAction)SYNTAQLITE_UPSERT_ACTION_NOTHING,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-8].minor.yy352.clauses = synq_parse_upsert_clause_list(
          pCtx, yymsp[0].minor.yy352.clauses, clause);
      yymsp[-8].minor.yy352.returning = yymsp[0].minor.yy352.returning;
    } break;
    case 171: /* upsert ::= ON CONFLICT DO NOTHING returning */
    {
      uint32_t clause = synq_parse_upsert_clause(
          pCtx, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
          (SyntaqliteUpsertAction)SYNTAQLITE_UPSERT_ACTION_NOTHING,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-4].minor.yy352.clauses =
          synq_parse_upsert_clause_list(pCtx, SYNTAQLITE_NULL_NODE, clause);
      yymsp[-4].minor.yy352.returning = yymsp[0].minor.yy277;
    } break;
    case 172: /* upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt
                 returning */
    {
      uint32_t clause = synq_parse_upsert_clause(
          pCtx, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
          (SyntaqliteUpsertAction)SYNTAQLITE_UPSERT_ACTION_UPDATE,
          yymsp[-2].minor.yy277, yymsp[-1].minor.yy277);
      yymsp[-7].minor.yy352.clauses =
          synq_parse_upsert_clause_list(pCtx, SYNTAQLITE_NULL_NODE, clause);
      yymsp[-7].minor.yy352.returning = yymsp[0].minor.yy277;
    } break;
    case 175: /* expr ::= error */
    case 202: /* nmorerr ::= error */
      yytestcase(yyruleno == 202);
      {
        yymsp[0].minor.yy277 = synq_parse_error(pCtx, synq_error_span(pCtx));
      }
      break;
    case 177: /* expr ::= LP expr RP */
    {
      yymsp[-2].minor.yy277 =
          synq_parse_paren_expr(pCtx, yymsp[-1].minor.yy277);
    } break;
    case 178: /* expr ::= expr PLUS|MINUS expr */
    {
      SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == SYNTAQLITE_TK_PLUS)
                                  ? SYNTAQLITE_BINARY_OP_PLUS
                                  : SYNTAQLITE_BINARY_OP_MINUS;
      yylhsminor.yy277 = synq_parse_binary_expr(pCtx, op, yymsp[-2].minor.yy277,
                                                yymsp[0].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 179: /* expr ::= expr STAR|SLASH|REM expr */
    {
      SyntaqliteBinaryOp op;
      switch (yymsp[-1].minor.yy0.type) {
        case SYNTAQLITE_TK_STAR:
          op = SYNTAQLITE_BINARY_OP_STAR;
          break;
        case SYNTAQLITE_TK_SLASH:
          op = SYNTAQLITE_BINARY_OP_SLASH;
          break;
        default:
          op = SYNTAQLITE_BINARY_OP_REM;
          break;
      }
      yylhsminor.yy277 = synq_parse_binary_expr(pCtx, op, yymsp[-2].minor.yy277,
                                                yymsp[0].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 180: /* expr ::= expr LT|GT|GE|LE expr */
    {
      SyntaqliteBinaryOp op;
      switch (yymsp[-1].minor.yy0.type) {
        case SYNTAQLITE_TK_LT:
          op = SYNTAQLITE_BINARY_OP_LT;
          break;
        case SYNTAQLITE_TK_GT:
          op = SYNTAQLITE_BINARY_OP_GT;
          break;
        case SYNTAQLITE_TK_LE:
          op = SYNTAQLITE_BINARY_OP_LE;
          break;
        default:
          op = SYNTAQLITE_BINARY_OP_GE;
          break;
      }
      yylhsminor.yy277 = synq_parse_binary_expr(pCtx, op, yymsp[-2].minor.yy277,
                                                yymsp[0].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 181: /* expr ::= expr EQ|NE expr */
    {
      SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == SYNTAQLITE_TK_EQ)
                                  ? SYNTAQLITE_BINARY_OP_EQ
                                  : SYNTAQLITE_BINARY_OP_NE;
      yylhsminor.yy277 = synq_parse_binary_expr(pCtx, op, yymsp[-2].minor.yy277,
                                                yymsp[0].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 182: /* expr ::= expr AND expr */
    {
      yylhsminor.yy277 =
          synq_parse_binary_expr(pCtx, SYNTAQLITE_BINARY_OP_AND,
                                 yymsp[-2].minor.yy277, yymsp[0].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 183: /* expr ::= expr OR expr */
    {
      yylhsminor.yy277 =
          synq_parse_binary_expr(pCtx, SYNTAQLITE_BINARY_OP_OR,
                                 yymsp[-2].minor.yy277, yymsp[0].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 184: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
    {
      SyntaqliteBinaryOp op;
      switch (yymsp[-1].minor.yy0.type) {
        case SYNTAQLITE_TK_BITAND:
          op = SYNTAQLITE_BINARY_OP_BIT_AND;
          break;
        case SYNTAQLITE_TK_BITOR:
          op = SYNTAQLITE_BINARY_OP_BIT_OR;
          break;
        case SYNTAQLITE_TK_LSHIFT:
          op = SYNTAQLITE_BINARY_OP_LSHIFT;
          break;
        default:
          op = SYNTAQLITE_BINARY_OP_RSHIFT;
          break;
      }
      yylhsminor.yy277 = synq_parse_binary_expr(pCtx, op, yymsp[-2].minor.yy277,
                                                yymsp[0].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 185: /* expr ::= expr CONCAT expr */
    {
      yylhsminor.yy277 =
          synq_parse_binary_expr(pCtx, SYNTAQLITE_BINARY_OP_CONCAT,
                                 yymsp[-2].minor.yy277, yymsp[0].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 186: /* expr ::= expr PTR expr */
    {
      SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.n == 3)
                                  ? SYNTAQLITE_BINARY_OP_PTR2
                                  : SYNTAQLITE_BINARY_OP_PTR;
      yylhsminor.yy277 = synq_parse_binary_expr(pCtx, op, yymsp[-2].minor.yy277,
                                                yymsp[0].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 187: /* expr ::= PLUS|MINUS expr */
    {
      SyntaqliteUnaryOp op = (yymsp[-1].minor.yy0.type == SYNTAQLITE_TK_MINUS)
                                 ? SYNTAQLITE_UNARY_OP_MINUS
                                 : SYNTAQLITE_UNARY_OP_PLUS;
      yylhsminor.yy277 = synq_parse_unary_expr(pCtx, op, yymsp[0].minor.yy277);
    }
      yymsp[-1].minor.yy277 = yylhsminor.yy277;
      break;
    case 188: /* expr ::= BITNOT expr */
    {
      yymsp[-1].minor.yy277 = synq_parse_unary_expr(
          pCtx, SYNTAQLITE_UNARY_OP_BIT_NOT, yymsp[0].minor.yy277);
    } break;
    case 189: /* expr ::= NOT expr */
    {
      yymsp[-1].minor.yy277 = synq_parse_unary_expr(
          pCtx, SYNTAQLITE_UNARY_OP_NOT, yymsp[0].minor.yy277);
    } break;
    case 192: /* nexprlist ::= nexprlist COMMA expr */
    {
      yylhsminor.yy277 = synq_parse_expr_list(pCtx, yymsp[-2].minor.yy277,
                                              yymsp[0].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 193: /* nexprlist ::= expr */
    {
      yylhsminor.yy277 = synq_parse_expr_list(pCtx, SYNTAQLITE_NULL_NODE,
                                              yymsp[0].minor.yy277);
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 194: /* expr ::= LP nexprlist COMMA expr RP */
    {
      yymsp[-4].minor.yy277 = synq_parse_row_value(
          pCtx, synq_parse_expr_list(pCtx, yymsp[-3].minor.yy277,
                                     yymsp[-1].minor.yy277));
    } break;
    case 195: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
    {
      synq_mark_as_function(pCtx, yymsp[-4].minor.yy0);
      yylhsminor.yy277 = synq_parse_function_call(
          pCtx, synq_span(pCtx, yymsp[-4].minor.yy0),
          (SyntaqliteFunctionCallFlags){
              .raw = (uint8_t)(yymsp[-2].minor.yy277 & 0xFF)},
          yymsp[-1].minor.yy277, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 196: /* expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
    {
      synq_mark_as_function(pCtx, yymsp[-3].minor.yy0);
      yylhsminor.yy277 = synq_parse_function_call(
          pCtx, synq_span(pCtx, yymsp[-3].minor.yy0),
          (SyntaqliteFunctionCallFlags){.raw = 0x02}, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    }
      yymsp[-3].minor.yy277 = yylhsminor.yy277;
      break;
    case 197: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over
               */
    {
      SyntaqliteFilterOver* fo =
          AST_NODE_AS(SyntaqliteFilterOver, &pCtx->ast, yymsp[0].minor.yy277);
      synq_mark_as_function(pCtx, yymsp[-5].minor.yy0);
      yylhsminor.yy277 = synq_parse_function_call(
          pCtx, synq_span(pCtx, yymsp[-5].minor.yy0),
          (SyntaqliteFunctionCallFlags){
              .raw = (uint8_t)(yymsp[-3].minor.yy277 & 0xFF)},
          yymsp[-2].minor.yy277, fo->filter_expr, fo->over_def);
    }
      yymsp[-5].minor.yy277 = yylhsminor.yy277;
      break;
    case 198: /* expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
    {
      SyntaqliteFilterOver* fo =
          AST_NODE_AS(SyntaqliteFilterOver, &pCtx->ast, yymsp[0].minor.yy277);
      synq_mark_as_function(pCtx, yymsp[-4].minor.yy0);
      yylhsminor.yy277 = synq_parse_function_call(
          pCtx, synq_span(pCtx, yymsp[-4].minor.yy0),
          (SyntaqliteFunctionCallFlags){.raw = 0x02}, SYNTAQLITE_NULL_NODE,
          fo->filter_expr, fo->over_def);
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 199: /* nm ::= ID|INDEXED|JOIN_KW */
    case 200: /* nm ::= STRING */
      yytestcase(yyruleno == 200);
      {
        synq_mark_as_id(pCtx, yymsp[0].minor.yy0);
        yylhsminor.yy0 = yymsp[0].minor.yy0;
      }
      yymsp[0].minor.yy0 = yylhsminor.yy0;
      break;
    case 201: /* nmorerr ::= nm */
    case 265: /* as ::= ID|STRING */
      yytestcase(yyruleno == 265);
      {
        yylhsminor.yy277 = synq_parse_ident_name(
            pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0));
      }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 203: /* term ::= INTEGER */
    {
      yylhsminor.yy277 =
          synq_parse_literal(pCtx, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                             synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 204: /* term ::= STRING */
    {
      yylhsminor.yy277 =
          synq_parse_literal(pCtx, SYNTAQLITE_LITERAL_TYPE_STRING,
                             synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 205: /* term ::= NULL|FLOAT|BLOB */
    {
      SyntaqliteLiteralType lit_type;
      switch (yymsp[0].minor.yy0.type) {
        case SYNTAQLITE_TK_NULL:
          lit_type = SYNTAQLITE_LITERAL_TYPE_NULL;
          break;
        case SYNTAQLITE_TK_FLOAT:
          lit_type = SYNTAQLITE_LITERAL_TYPE_FLOAT;
          break;
        case SYNTAQLITE_TK_BLOB:
          lit_type = SYNTAQLITE_LITERAL_TYPE_BLOB;
          break;
        default:
          lit_type = SYNTAQLITE_LITERAL_TYPE_NULL;
          break;
      }
      yylhsminor.yy277 = synq_parse_literal(
          pCtx, lit_type, synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 206: /* term ::= QNUMBER */
    {
      yylhsminor.yy277 =
          synq_parse_literal(pCtx, SYNTAQLITE_LITERAL_TYPE_QNUMBER,
                             synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 207: /* term ::= CTIME_KW */
    {
      yylhsminor.yy277 =
          synq_parse_literal(pCtx, SYNTAQLITE_LITERAL_TYPE_CURRENT,
                             synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 208: /* expr ::= VARIABLE */
    {
      yylhsminor.yy277 =
          synq_parse_variable(pCtx, synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 209: /* expr ::= expr COLLATE ID|STRING */
    {
      yylhsminor.yy277 = synq_parse_collate_expr(
          pCtx, yymsp[-2].minor.yy277, synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 210: /* sortlist ::= sortlist COMMA expr sortorder nulls */
    {
      uint32_t term =
          synq_parse_ordering_term(pCtx, yymsp[-2].minor.yy277,
                                   (SyntaqliteSortOrder)yymsp[-1].minor.yy277,
                                   (SyntaqliteNullsOrder)yymsp[0].minor.yy277);
      yylhsminor.yy277 =
          synq_parse_order_by_list(pCtx, yymsp[-4].minor.yy277, term);
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 211: /* sortlist ::= expr sortorder nulls */
    {
      uint32_t term =
          synq_parse_ordering_term(pCtx, yymsp[-2].minor.yy277,
                                   (SyntaqliteSortOrder)yymsp[-1].minor.yy277,
                                   (SyntaqliteNullsOrder)yymsp[0].minor.yy277);
      yylhsminor.yy277 =
          synq_parse_order_by_list(pCtx, SYNTAQLITE_NULL_NODE, term);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 213: /* sortorder ::= DESC */
    case 267: /* distinct ::= DISTINCT */
      yytestcase(yyruleno == 267);
      {
        yymsp[0].minor.yy277 = 1;
      }
      break;
    case 214: /* sortorder ::= */
    case 217: /* nulls ::= */
      yytestcase(yyruleno == 217);
    case 269: /* distinct ::= */
      yytestcase(yyruleno == 269);
      {
        yymsp[1].minor.yy277 = 0;
      }
      break;
    case 216: /* nulls ::= NULLS LAST */
    {
      yymsp[-1].minor.yy277 = 2;
    } break;
    case 218: /* expr ::= RAISE LP IGNORE RP */
    {
      yymsp[-3].minor.yy277 = synq_parse_raise_expr(
          pCtx, SYNTAQLITE_RAISE_TYPE_IGNORE, SYNTAQLITE_NULL_NODE);
    } break;
    case 219: /* expr ::= RAISE LP raisetype COMMA expr RP */
    {
      yymsp[-5].minor.yy277 = synq_parse_raise_expr(
          pCtx, (SyntaqliteRaiseType)yymsp[-3].minor.yy320,
          yymsp[-1].minor.yy277);
    } break;
    case 220: /* raisetype ::= ROLLBACK */
    {
      yymsp[0].minor.yy320 = SYNTAQLITE_RAISE_TYPE_ROLLBACK;
    } break;
    case 221: /* raisetype ::= ABORT */
    {
      yymsp[0].minor.yy320 = SYNTAQLITE_RAISE_TYPE_ABORT;
    } break;
    case 222: /* raisetype ::= FAIL */
    {
      yymsp[0].minor.yy320 = SYNTAQLITE_RAISE_TYPE_FAIL;
    } break;
    case 223: /* fullname ::= nmorerr */
    {
      yylhsminor.yy277 = synq_parse_qualified_name(pCtx, yymsp[0].minor.yy277,
                                                   SYNTAQLITE_NULL_NODE);
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 224: /* fullname ::= nmorerr DOT nmorerr */
    {
      yylhsminor.yy277 = synq_parse_qualified_name(pCtx, yymsp[0].minor.yy277,
                                                   yymsp[-2].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 227: /* cmd ::= DROP TABLE ifexists fullname */
    {
      yymsp[-3].minor.yy277 = synq_parse_drop_stmt(
          pCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TABLE,
          (SyntaqliteBool)yymsp[-1].minor.yy320, yymsp[0].minor.yy277);
    } break;
    case 228: /* cmd ::= DROP VIEW ifexists fullname */
    {
      yymsp[-3].minor.yy277 = synq_parse_drop_stmt(
          pCtx, SYNTAQLITE_DROP_OBJECT_TYPE_VIEW,
          (SyntaqliteBool)yymsp[-1].minor.yy320, yymsp[0].minor.yy277);
    } break;
    case 229: /* cmd ::= DROP INDEX ifexists fullname */
    {
      yymsp[-3].minor.yy277 = synq_parse_drop_stmt(
          pCtx, SYNTAQLITE_DROP_OBJECT_TYPE_INDEX,
          (SyntaqliteBool)yymsp[-1].minor.yy320, yymsp[0].minor.yy277);
    } break;
    case 230: /* cmd ::= DROP TRIGGER ifexists fullname */
    {
      yymsp[-3].minor.yy277 = synq_parse_drop_stmt(
          pCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TRIGGER,
          (SyntaqliteBool)yymsp[-1].minor.yy320, yymsp[0].minor.yy277);
    } break;
    case 231: /* cmd ::= ALTER TABLE fullname RENAME TO nmorerr */
    {
      yymsp[-5].minor.yy277 = synq_parse_alter_table_stmt(
          pCtx, SYNTAQLITE_ALTER_OP_RENAME_TABLE, yymsp[-3].minor.yy277,
          yymsp[0].minor.yy277, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    } break;
    case 232: /* cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nmorerr TO
                 nmorerr */
    {
      yymsp[-7].minor.yy277 = synq_parse_alter_table_stmt(
          pCtx, SYNTAQLITE_ALTER_OP_RENAME_COLUMN, yymsp[-5].minor.yy277,
          yymsp[0].minor.yy277, yymsp[-2].minor.yy277, SYNTAQLITE_NULL_NODE);
    } break;
    case 233: /* cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nmorerr */
    {
      yymsp[-5].minor.yy277 = synq_parse_alter_table_stmt(
          pCtx, SYNTAQLITE_ALTER_OP_DROP_COLUMN, yymsp[-3].minor.yy277,
          SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy277, SYNTAQLITE_NULL_NODE);
    } break;
    case 234: /* cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt
                 columnname carglist */
    {
      uint32_t col = synq_parse_column_def(pCtx, yymsp[-1].minor.yy640.name,
                                           yymsp[-1].minor.yy640.typetoken,
                                           yymsp[0].minor.yy430.list);
      yymsp[-6].minor.yy277 = synq_parse_alter_table_stmt(
          pCtx, SYNTAQLITE_ALTER_OP_ADD_COLUMN, yymsp[-4].minor.yy277,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, col);
    } break;
    case 238: /* columnname ::= nmorerr typetoken */
    {
      yylhsminor.yy640.name = yymsp[-1].minor.yy277;
      pCtx->generated_always = synq_trim_generated_always(&yymsp[0].minor.yy0);
      yylhsminor.yy640.typetoken =
          (yymsp[0].minor.yy0.z && yymsp[0].minor.yy0.n)
              ? synq_span(pCtx, yymsp[0].minor.yy0)
              : SYNQ_NO_SPAN;
      // Starting a column ends the previous one's pending CONSTRAINT name.
      pCtx->constraint_name = SYNQ_NO_SPAN;
    }
      yymsp[-1].minor.yy640 = yylhsminor.yy640;
      break;
    case 239: /* cmd ::= BEGIN transtype trans_opt */
    {
      yymsp[-2].minor.yy277 = synq_parse_transaction_stmt(
          pCtx, SYNTAQLITE_TRANSACTION_OP_BEGIN,
          (SyntaqliteTransactionType)yymsp[-1].minor.yy320,
          yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[0].minor.yy0)
                               : SYNQ_NO_SPAN);
    } break;
    case 240: /* cmd ::= COMMIT|END trans_opt */
    {
      yymsp[-1].minor.yy277 = synq_parse_transaction_stmt(
          pCtx, SYNTAQLITE_TRANSACTION_OP_COMMIT,
          SYNTAQLITE_TRANSACTION_TYPE_DEFERRED,
          yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[0].minor.yy0)
                               : SYNQ_NO_SPAN);
    } break;
    case 241: /* cmd ::= ROLLBACK trans_opt */
    {
      yymsp[-1].minor.yy277 = synq_parse_transaction_stmt(
          pCtx, SYNTAQLITE_TRANSACTION_OP_ROLLBACK,
          SYNTAQLITE_TRANSACTION_TYPE_DEFERRED,
          yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[0].minor.yy0)
                               : SYNQ_NO_SPAN);
    } break;
    case 242: /* transtype ::= */
    {
      yymsp[1].minor.yy320 = (int)SYNTAQLITE_TRANSACTION_TYPE_DEFERRED;
    } break;
    case 243: /* transtype ::= DEFERRED */
    {
      yymsp[0].minor.yy320 = (int)SYNTAQLITE_TRANSACTION_TYPE_DEFERRED;
    } break;
    case 244: /* transtype ::= IMMEDIATE */
    {
      yymsp[0].minor.yy320 = (int)SYNTAQLITE_TRANSACTION_TYPE_IMMEDIATE;
    } break;
    case 245: /* transtype ::= EXCLUSIVE */
    {
      yymsp[0].minor.yy320 = (int)SYNTAQLITE_TRANSACTION_TYPE_EXCLUSIVE;
    } break;
    case 247: /* trans_opt ::= TRANSACTION */
    {
      yymsp[0].minor.yy0.z = NULL;
      yymsp[0].minor.yy0.n = 0;
    } break;
    case 248: /* trans_opt ::= TRANSACTION nm */
    case 335: /* plus_num ::= PLUS INTEGER|FLOAT */
      yytestcase(yyruleno == 335);
      {
        yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;
      }
      break;
    case 249: /* savepoint_opt ::= SAVEPOINT */
    {
      yymsp[0].minor.yy320 = 0;
    } break;
    case 251: /* cmd ::= SAVEPOINT nmorerr */
    {
      yymsp[-1].minor.yy277 =
          synq_parse_savepoint_stmt(pCtx, SYNTAQLITE_SAVEPOINT_OP_SAVEPOINT,
                                    yymsp[0].minor.yy277, SYNQ_NO_SPAN);
    } break;
    case 252: /* cmd ::= RELEASE savepoint_opt nmorerr */
    {
      yymsp[-2].minor.yy277 =
          synq_parse_savepoint_stmt(pCtx, SYNTAQLITE_SAVEPOINT_OP_RELEASE,
                                    yymsp[0].minor.yy277, SYNQ_NO_SPAN);
    } break;
    case 253: /* cmd ::= ROLLBACK trans_opt TO savepoint_opt nmorerr */
    {
      yymsp[-4].minor.yy277 = synq_parse_savepoint_stmt(
          pCtx, SYNTAQLITE_SAVEPOINT_OP_ROLLBACK_TO, yymsp[0].minor.yy277,
          yymsp[-3].minor.yy0.z ? synq_span(pCtx, yymsp[-3].minor.yy0)
                                : SYNQ_NO_SPAN);
    } break;
    case 257: /* oneselect ::= SELECT distinct selcollist from where_opt
                 groupby_opt having_opt orderby_opt limit_opt */
    {
      yymsp[-8].minor.yy277 = synq_parse_select_stmt(
          pCtx,
          (SyntaqliteSelectStmtFlags){
              .raw = (uint8_t)(yymsp[-7].minor.yy277 & 0xFF)},
          yymsp[-6].minor.yy277, yymsp[-5].minor.yy277, yymsp[-4].minor.yy277,
          yymsp[-3].minor.yy277, yymsp[-2].minor.yy277, yymsp[-1].minor.yy277,
          yymsp[0].minor.yy277, SYNTAQLITE_NULL_NODE);
    } break;
    case 258: /* oneselect ::= SELECT distinct selcollist from where_opt
                 groupby_opt having_opt window_clause orderby_opt limit_opt */
    {
      yymsp[-9].minor.yy277 = synq_parse_select_stmt(
          pCtx,
          (SyntaqliteSelectStmtFlags){
              .raw = (uint8_t)(yymsp[-8].minor.yy277 & 0xFF)},
          yymsp[-7].minor.yy277, yymsp[-6].minor.yy277, yymsp[-5].minor.yy277,
          yymsp[-4].minor.yy277, yymsp[-3].minor.yy277, yymsp[-1].minor.yy277,
          yymsp[0].minor.yy277, yymsp[-2].minor.yy277);
    } break;
    case 259: /* selcollist ::= sclp scanpt expr scanpt as */
    {
      uint32_t col =
          synq_parse_result_column(pCtx, (SyntaqliteResultColumnFlags){0},
                                   yymsp[0].minor.yy277, yymsp[-2].minor.yy277);
      yylhsminor.yy277 =
          synq_parse_result_column_list(pCtx, yymsp[-4].minor.yy277, col);
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 260: /* selcollist ::= sclp scanpt STAR */
    {
      uint32_t col = synq_parse_result_column(
          pCtx, (SyntaqliteResultColumnFlags){.raw = 0x01},
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yylhsminor.yy277 =
          synq_parse_result_column_list(pCtx, yymsp[-2].minor.yy277, col);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 268: /* distinct ::= ALL */
    {
      // Bit 2 is STAR in FunctionCallFlags, so ALL takes bit 4 in every set
      // that this value is cast into.
      yymsp[0].minor.yy277 = 4;
    } break;
    case 275: /* groupby_opt ::= GROUP BY nexprlist */
    case 279: /* orderby_opt ::= ORDER BY sortlist */
      yytestcase(yyruleno == 279);
      {
        yymsp[-2].minor.yy277 = synq_pass(pCtx, yymsp[0].minor.yy277);
      }
      break;
    case 281: /* limit_opt ::= LIMIT expr */
    {
      yymsp[-1].minor.yy277 = synq_parse_limit_clause(
          pCtx, yymsp[0].minor.yy277, SYNTAQLITE_NULL_NODE);
    } break;
    case 282: /* limit_opt ::= LIMIT expr OFFSET expr */
    {
      yymsp[-3].minor.yy277 = synq_parse_limit_clause(
          pCtx, yymsp[-2].minor.yy277, yymsp[0].minor.yy277);
    } break;
    case 283: /* limit_opt ::= LIMIT expr COMMA expr */
    {
      yymsp[-3].minor.yy277 = synq_parse_limit_clause(
          pCtx, yymsp[0].minor.yy277, yymsp[-2].minor.yy277);
    } break;
    case 284: /* stl_prefix ::= seltablist joinop */
    {
      yymsp[-1].minor.yy277 = synq_parse_join_prefix(
          pCtx, yymsp[-1].minor.yy277, yymsp[0].minor.yy81);
    } break;
    case 286: /* seltablist ::= stl_prefix nm dbnm as on_using */
    {
      uint32_t alias = yymsp[-1].minor.yy277;
      SyntaqliteTextSpan table_name;
      SyntaqliteTextSpan schema;
      if (yymsp[-2].minor.yy0.z != NULL) {
        table_name = synq_span_dequote(pCtx, yymsp[-2].minor.yy0);
        schema = synq_span_dequote(pCtx, yymsp[-3].minor.yy0);
      } else {
        table_name = synq_span_dequote(pCtx, yymsp[-3].minor.yy0);
        schema = SYNQ_NO_SPAN;
      }
      uint32_t tref = synq_parse_table_ref(
          pCtx, table_name, schema, SYNTAQLITE_BOOL_FALSE, alias,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
      if (yymsp[-4].minor.yy277 == SYNTAQLITE_NULL_NODE) {
        yymsp[-4].minor.yy277 = tref;
      } else {
        SyntaqliteNode* pfx = AST_NODE(&pCtx->ast, yymsp[-4].minor.yy277);
        yymsp[-4].minor.yy277 = synq_parse_join_clause(
            pCtx, pfx->join_prefix.join_type, pfx->join_prefix.source, tref,
            yymsp[0].minor.yy632.on_expr, yymsp[0].minor.yy632.using_cols);
      }
    } break;
    case 287: /* seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
    {
      uint32_t alias = yymsp[-2].minor.yy277;
      SyntaqliteTextSpan table_name;
      SyntaqliteTextSpan schema;
      if (yymsp[-3].minor.yy0.z != NULL) {
        table_name = synq_span_dequote(pCtx, yymsp[-3].minor.yy0);
        schema = synq_span_dequote(pCtx, yymsp[-4].minor.yy0);
      } else {
        table_name = synq_span_dequote(pCtx, yymsp[-4].minor.yy0);
        schema = SYNQ_NO_SPAN;
      }
      SyntaqliteIndexHint ih =
          (yymsp[-1].minor.yy0.z != NULL) ? SYNTAQLITE_INDEX_HINT_INDEXED
          : (yymsp[-1].minor.yy0.n == 1)  ? SYNTAQLITE_INDEX_HINT_NOT_INDEXED
                                          : SYNTAQLITE_INDEX_HINT_DEFAULT;
      uint32_t tref = synq_parse_table_ref(
          pCtx, table_name, schema, SYNTAQLITE_BOOL_FALSE, alias,
          SYNTAQLITE_NULL_NODE, ih, synq_span(pCtx, yymsp[-1].minor.yy0));
      if (yymsp[-5].minor.yy277 == SYNTAQLITE_NULL_NODE) {
        yymsp[-5].minor.yy277 = tref;
      } else {
        SyntaqliteNode* pfx = AST_NODE(&pCtx->ast, yymsp[-5].minor.yy277);
        yymsp[-5].minor.yy277 = synq_parse_join_clause(
            pCtx, pfx->join_prefix.join_type, pfx->join_prefix.source, tref,
            yymsp[0].minor.yy632.on_expr, yymsp[0].minor.yy632.using_cols);
      }
    } break;
    case 288: /* seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
    {
      uint32_t alias = yymsp[-1].minor.yy277;
      SyntaqliteTextSpan table_name;
      SyntaqliteTextSpan schema;
      if (yymsp[-5].minor.yy0.z != NULL) {
        table_name = synq_span_dequote(pCtx, yymsp[-5].minor.yy0);
        schema = synq_span_dequote(pCtx, yymsp[-6].minor.yy0);
      } else {
        table_name = synq_span_dequote(pCtx, yymsp[-6].minor.yy0);
        schema = SYNQ_NO_SPAN;
      }
      uint32_t tref = synq_parse_table_ref(
          pCtx, table_name, schema, SYNTAQLITE_BOOL_TRUE, alias,
          yymsp[-3].minor.yy277, SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
      if (yymsp[-7].minor.yy277 == SYNTAQLITE_NULL_NODE) {
        yymsp[-7].minor.yy277 = tref;
      } else {
        SyntaqliteNode* pfx = AST_NODE(&pCtx->ast, yymsp[-7].minor.yy277);
        yymsp[-7].minor.yy277 = synq_parse_join_clause(
            pCtx, pfx->join_prefix.join_type, pfx->join_prefix.source, tref,
            yymsp[0].minor.yy632.on_expr, yymsp[0].minor.yy632.using_cols);
      }
    } break;
    case 289: /* seltablist ::= stl_prefix LP select RP as on_using */
    {
      pCtx->saw_subquery = 1;
      uint32_t alias = yymsp[-1].minor.yy277;
      uint32_t sub =
          synq_parse_subquery_table_source(pCtx, yymsp[-3].minor.yy277, alias);
      if (yymsp[-5].minor.yy277 == SYNTAQLITE_NULL_NODE) {
        yymsp[-5].minor.yy277 = sub;
      } else {
        SyntaqliteNode* pfx = AST_NODE(&pCtx->ast, yymsp[-5].minor.yy277);
        yymsp[-5].minor.yy277 = synq_parse_join_clause(
            pCtx, pfx->join_prefix.join_type, pfx->join_prefix.source, sub,
            yymsp[0].minor.yy632.on_expr, yymsp[0].minor.yy632.using_cols);
      }
    } break;
    case 290: /* seltablist ::= stl_prefix LP seltablist RP as on_using */
    {
      if (yymsp[-5].minor.yy277 == SYNTAQLITE_NULL_NODE &&
          yymsp[-1].minor.yy277 == SYNTAQLITE_NULL_NODE &&
          yymsp[0].minor.yy632.on_expr == SYNTAQLITE_NULL_NODE &&
          yymsp[0].minor.yy632.using_cols == SYNTAQLITE_NULL_NODE) {
        yymsp[-5].minor.yy277 = synq_pass(pCtx, yymsp[-3].minor.yy277);
      } else {
        uint32_t paren = synq_parse_paren_table_source(
            pCtx, yymsp[-3].minor.yy277, yymsp[-1].minor.yy277);
        if (yymsp[-5].minor.yy277 == SYNTAQLITE_NULL_NODE) {
          yymsp[-5].minor.yy277 = paren;
        } else {
          SyntaqliteNode* pfx = AST_NODE(&pCtx->ast, yymsp[-5].minor.yy277);
          yymsp[-5].minor.yy277 = synq_parse_join_clause(
              pCtx, pfx->join_prefix.join_type, pfx->join_prefix.source, paren,
              yymsp[0].minor.yy632.on_expr, yymsp[0].minor.yy632.using_cols);
        }
      }
    } break;
    case 291: /* joinop ::= COMMA|JOIN */
    {
      yylhsminor.yy81 = (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_COMMA)
                            ? SYNTAQLITE_JOIN_TYPE_COMMA
                            : SYNTAQLITE_JOIN_TYPE_INNER;
    }
      yymsp[0].minor.yy81 = yylhsminor.yy81;
      break;
    case 292: /* joinop ::= JOIN_KW JOIN */
    {
      yylhsminor.yy81 = synq_join_type(&yymsp[-1].minor.yy0, NULL, NULL);
    }
      yymsp[-1].minor.yy81 = yylhsminor.yy81;
      break;
    case 293: /* joinop ::= JOIN_KW nm JOIN */
    {
      yylhsminor.yy81 =
          synq_join_type(&yymsp[-2].minor.yy0, &yymsp[-1].minor.yy0, NULL);
    }
      yymsp[-2].minor.yy81 = yylhsminor.yy81;
      break;
    case 294: /* joinop ::= JOIN_KW nm nm JOIN */
    {
      yylhsminor.yy81 = synq_join_type(
          &yymsp[-3].minor.yy0, &yymsp[-2].minor.yy0, &yymsp[-1].minor.yy0);
    }
      yymsp[-3].minor.yy81 = yylhsminor.yy81;
      break;
    case 295: /* on_using ::= ON expr */
    {
      yymsp[-1].minor.yy632.on_expr = yymsp[0].minor.yy277;
      yymsp[-1].minor.yy632.using_cols = SYNTAQLITE_NULL_NODE;
    } break;
    case 296: /* on_using ::= USING LP idlist RP */
    {
      yymsp[-3].minor.yy632.on_expr = SYNTAQLITE_NULL_NODE;
      yymsp[-3].minor.yy632.using_cols = yymsp[-1].minor.yy277;
    } break;
    case 297: /* on_using ::= */
    {
      yymsp[1].minor.yy632.on_expr = SYNTAQLITE_NULL_NODE;
      yymsp[1].minor.yy632.using_cols = SYNTAQLITE_NULL_NODE;
    } break;
    case 298: /* indexed_by ::= INDEXED BY nm */
    {
      yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;
    } break;
    case 299: /* indexed_by ::= NOT INDEXED */
    {
      yymsp[-1].minor.yy0.z = NULL;
      yymsp[-1].minor.yy0.n = 1;
    } break;
    case 300: /* idlist ::= idlist COMMA nm */
    {
      uint32_t col = synq_parse_column_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0), SYNQ_NO_SPAN,
          SYNQ_NO_SPAN);
      yymsp[-2].minor.yy277 =
          synq_parse_expr_list(pCtx, yymsp[-2].minor.yy277, col);
    } break;
    case 301: /* idlist ::= nm */
    {
      uint32_t col = synq_parse_column_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0), SYNQ_NO_SPAN,
          SYNQ_NO_SPAN);
      yylhsminor.yy277 = synq_parse_expr_list(pCtx, SYNTAQLITE_NULL_NODE, col);
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 302: /* cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
    {
      // yymsp[-3].minor.yy277 is a partially-built CreateTriggerStmt, fill in
      // the body
      SyntaqliteNode* trig = AST_NODE(&pCtx->ast, yymsp[-3].minor.yy277);
      trig->create_trigger_stmt.body = yymsp[-1].minor.yy277;
      yymsp[-4].minor.yy277 = synq_pass(pCtx, yymsp[-3].minor.yy277);
    } break;
    case 303: /* trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time
                 trigger_event ON fullname foreach_clause when_clause */
    {
      SyntaqliteTextSpan trig_name = yymsp[-6].minor.yy0.z
                                         ? synq_span(pCtx, yymsp[-6].minor.yy0)
                                         : synq_span(pCtx, yymsp[-7].minor.yy0);
      SyntaqliteTextSpan trig_schema =
          yymsp[-6].minor.yy0.z ? synq_span(pCtx, yymsp[-7].minor.yy0)
                                : SYNQ_NO_SPAN;
      yylhsminor.yy277 = synq_parse_create_trigger_stmt(
          pCtx, trig_name, trig_schema, (SyntaqliteBool)yymsp[-10].minor.yy320,
          (SyntaqliteBool)yymsp[-8].minor.yy320,
          (SyntaqliteTriggerTiming)yymsp[-5].minor.yy320, yymsp[-4].minor.yy277,
          yymsp[-2].minor.yy277, yymsp[0].minor.yy277,
          SYNTAQLITE_NULL_NODE);  // body filled in by cmd rule
    }
      yymsp[-10].minor.yy277 = yylhsminor.yy277;
      break;
    case 304: /* trigger_time ::= BEFORE|AFTER */
    {
      yylhsminor.yy320 = (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_BEFORE)
                             ? (int)SYNTAQLITE_TRIGGER_TIMING_BEFORE
                             : (int)SYNTAQLITE_TRIGGER_TIMING_AFTER;
    }
      yymsp[0].minor.yy320 = yylhsminor.yy320;
      break;
    case 305: /* trigger_time ::= INSTEAD OF */
    {
      yymsp[-1].minor.yy320 = (int)SYNTAQLITE_TRIGGER_TIMING_INSTEAD_OF;
    } break;
    case 306: /* trigger_time ::= */
    {
      yymsp[1].minor.yy320 = (int)SYNTAQLITE_TRIGGER_TIMING_BEFORE;
    } break;
    case 307: /* trigger_event ::= DELETE|INSERT */
    {
      SyntaqliteTriggerEventType evt =
          (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_DELETE)
              ? SYNTAQLITE_TRIGGER_EVENT_TYPE_DELETE
              : SYNTAQLITE_TRIGGER_EVENT_TYPE_INSERT;
      yylhsminor.yy277 =
          synq_parse_trigger_event(pCtx, evt, SYNTAQLITE_NULL_NODE);
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 308: /* trigger_event ::= UPDATE */
    {
      yymsp[0].minor.yy277 = synq_parse_trigger_event(
          pCtx, SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE, SYNTAQLITE_NULL_NODE);
    } break;
    case 309: /* trigger_event ::= UPDATE OF idlist */
    {
      yymsp[-2].minor.yy277 = synq_parse_trigger_event(
          pCtx, SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE, yymsp[0].minor.yy277);
    } break;
    case 310: /* foreach_clause ::= */
    case 318: /* tridxby ::= */
      yytestcase(yyruleno == 318);
    case 376: /* vtabarg ::= */
      yytestcase(yyruleno == 376);
    case 381: /* anylist ::= */
      yytestcase(yyruleno == 381);
      {
        // empty
      }
      break;
    case 311: /* foreach_clause ::= FOR EACH ROW */
    case 374: /* vtabarglist ::= vtabarg */
      yytestcase(yyruleno == 374);
    case 375: /* vtabarglist ::= vtabarglist COMMA vtabarg */
      yytestcase(yyruleno == 375);
    case 377: /* vtabarg ::= vtabarg vtabargtoken */
      yytestcase(yyruleno == 377);
    case 378: /* vtabargtoken ::= ANY */
      yytestcase(yyruleno == 378);
    case 379: /* vtabargtoken ::= lp anylist RP */
      yytestcase(yyruleno == 379);
    case 380: /* lp ::= LP */
      yytestcase(yyruleno == 380);
    case 382: /* anylist ::= anylist LP anylist RP */
      yytestcase(yyruleno == 382);
    case 383: /* anylist ::= anylist ANY */
      yytestcase(yyruleno == 383);
      {
        // consumed
      }
      break;
    case 314: /* trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
    {
      yylhsminor.yy277 = synq_parse_trigger_cmd_list(
          pCtx, yymsp[-2].minor.yy277, yymsp[-1].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 315: /* trigger_cmd_list ::= trigger_cmd SEMI */
    {
      yylhsminor.yy277 = synq_parse_trigger_cmd_list(pCtx, SYNTAQLITE_NULL_NODE,
                                                     yymsp[-1].minor.yy277);
    }
      yymsp[-1].minor.yy277 = yylhsminor.yy277;
      break;
    case 317: /* trnm ::= nm DOT nm */
    {
      yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;
      pCtx->error = 1;
    } break;
    case 319: /* tridxby ::= INDEXED BY nm */
    case 320: /* tridxby ::= NOT INDEXED */
      yytestcase(yyruleno == 320);
      {
        pCtx->error = 1;
      }
      break;
    case 321: /* trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from
                 where_opt scanpt */
    {
      uint32_t tbl = synq_parse_table_ref(
          pCtx, synq_span(pCtx, yymsp[-6].minor.yy0), SYNQ_NO_SPAN,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
      yymsp[-8].minor.yy277 = synq_parse_update_stmt(
          pCtx, SYNTAQLITE_NULL_NODE, SYNTAQLITE_BOOL_FALSE,
          (SyntaqliteConflictAction)yymsp[-7].minor.yy320, tbl,
          SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN, yymsp[-3].minor.yy277,
          yymsp[-2].minor.yy277, yymsp[-1].minor.yy277, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    } break;
    case 322: /* trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select
                 upsert scanpt */
    {
      uint32_t tbl = synq_parse_table_ref(
          pCtx, synq_span(pCtx, yymsp[-4].minor.yy0), SYNQ_NO_SPAN,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
      yymsp[-7].minor.yy277 = synq_parse_insert_stmt(
          pCtx, SYNTAQLITE_NULL_NODE, SYNTAQLITE_BOOL_FALSE,
          (SyntaqliteConflictAction)yymsp[-6].minor.yy320, tbl,
          yymsp[-3].minor.yy277, yymsp[-2].minor.yy277,
          yymsp[-1].minor.yy352.clauses, yymsp[-1].minor.yy352.returning);
    } break;
    case 323: /* trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
    {
      uint32_t tbl = synq_parse_table_ref(
          pCtx, synq_span(pCtx, yymsp[-3].minor.yy0), SYNQ_NO_SPAN,
          SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
      yymsp[-5].minor.yy277 = synq_parse_delete_stmt(
          pCtx, SYNTAQLITE_NULL_NODE, SYNTAQLITE_BOOL_FALSE, tbl,
          SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN, yymsp[-1].minor.yy277,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    } break;
    case 325: /* cmd ::= PRAGMA nm dbnm */
    {
      SyntaqliteTextSpan name_span = yymsp[0].minor.yy0.z
                                         ? synq_span(pCtx, yymsp[0].minor.yy0)
                                         : synq_span(pCtx, yymsp[-1].minor.yy0);
      SyntaqliteTextSpan schema_span =
          yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[-1].minor.yy0)
                               : SYNQ_NO_SPAN;
      yymsp[-2].minor.yy277 =
          synq_parse_pragma_stmt(pCtx, name_span, schema_span, SYNQ_NO_SPAN,
                                 SYNTAQLITE_PRAGMA_FORM_BARE);
    } break;
    case 326: /* cmd ::= PRAGMA nm dbnm EQ nmnum */
    case 328: /* cmd ::= PRAGMA nm dbnm EQ minus_num */
      yytestcase(yyruleno == 328);
      {
        SyntaqliteTextSpan name_span =
            yymsp[-2].minor.yy0.z ? synq_span(pCtx, yymsp[-2].minor.yy0)
                                  : synq_span(pCtx, yymsp[-3].minor.yy0);
        SyntaqliteTextSpan schema_span =
            yymsp[-2].minor.yy0.z ? synq_span(pCtx, yymsp[-3].minor.yy0)
                                  : SYNQ_NO_SPAN;
        yymsp[-4].minor.yy277 = synq_parse_pragma_stmt(
            pCtx, name_span, schema_span, synq_span(pCtx, yymsp[0].minor.yy0),
            SYNTAQLITE_PRAGMA_FORM_EQ);
      }
      break;
    case 327: /* cmd ::= PRAGMA nm dbnm LP nmnum RP */
    case 329: /* cmd ::= PRAGMA nm dbnm LP minus_num RP */
      yytestcase(yyruleno == 329);
      {
        SyntaqliteTextSpan name_span =
            yymsp[-3].minor.yy0.z ? synq_span(pCtx, yymsp[-3].minor.yy0)
                                  : synq_span(pCtx, yymsp[-4].minor.yy0);
        SyntaqliteTextSpan schema_span =
            yymsp[-3].minor.yy0.z ? synq_span(pCtx, yymsp[-4].minor.yy0)
                                  : SYNQ_NO_SPAN;
        yymsp[-5].minor.yy277 = synq_parse_pragma_stmt(
            pCtx, name_span, schema_span, synq_span(pCtx, yymsp[-1].minor.yy0),
            SYNTAQLITE_PRAGMA_FORM_CALL);
      }
      break;
    case 337: /* minus_num ::= MINUS INTEGER|FLOAT */
    {
      // Build a token that spans from the MINUS sign through the number
      yylhsminor.yy0.z = yymsp[-1].minor.yy0.z;
      yylhsminor.yy0.n = (int)(yymsp[0].minor.yy0.z - yymsp[-1].minor.yy0.z) +
                         yymsp[0].minor.yy0.n;
      yylhsminor.yy0.offset = yymsp[-1].minor.yy0.offset;
      yylhsminor.yy0.layer_id = yymsp[-1].minor.yy0.layer_id;
    }
      yymsp[-1].minor.yy0 = yylhsminor.yy0;
      break;
    case 340: /* cmd ::= ANALYZE */
    {
      yymsp[0].minor.yy277 = synq_parse_analyze_or_reindex_stmt(
          pCtx, SYNQ_NO_SPAN, SYNQ_NO_SPAN,
          SYNTAQLITE_ANALYZE_OR_REINDEX_OP_ANALYZE);
    } break;
    case 341: /* cmd ::= ANALYZE nm dbnm */
    {
      SyntaqliteTextSpan name_span = yymsp[0].minor.yy0.z
                                         ? synq_span(pCtx, yymsp[0].minor.yy0)
                                         : synq_span(pCtx, yymsp[-1].minor.yy0);
      SyntaqliteTextSpan schema_span =
          yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[-1].minor.yy0)
                               : SYNQ_NO_SPAN;
      yymsp[-2].minor.yy277 = synq_parse_analyze_or_reindex_stmt(
          pCtx, name_span, schema_span,
          SYNTAQLITE_ANALYZE_OR_REINDEX_OP_ANALYZE);
    } break;
    case 342: /* cmd ::= REINDEX */
    {
      yymsp[0].minor.yy277 = synq_parse_analyze_or_reindex_stmt(
          pCtx, SYNQ_NO_SPAN, SYNQ_NO_SPAN,
          SYNTAQLITE_ANALYZE_OR_REINDEX_OP_REINDEX);
    } break;
    case 343: /* cmd ::= REINDEX nm dbnm */
    {
      SyntaqliteTextSpan name_span = yymsp[0].minor.yy0.z
                                         ? synq_span(pCtx, yymsp[0].minor.yy0)
                                         : synq_span(pCtx, yymsp[-1].minor.yy0);
      SyntaqliteTextSpan schema_span =
          yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[-1].minor.yy0)
                               : SYNQ_NO_SPAN;
      yymsp[-2].minor.yy277 =
          synq_parse_analyze_or_reindex_stmt(pCtx, name_span, schema_span, 1);
    } break;
    case 344: /* cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
    {
      yymsp[-5].minor.yy277 =
          synq_parse_attach_stmt(pCtx, yymsp[-3].minor.yy277,
                                 yymsp[-1].minor.yy277, yymsp[0].minor.yy277);
    } break;
    case 345: /* cmd ::= DETACH database_kw_opt expr */
    {
      yymsp[-2].minor.yy277 =
          synq_parse_detach_stmt(pCtx, yymsp[0].minor.yy277);
    } break;
    case 346: /* database_kw_opt ::= DATABASE */
    {
      // Keyword consumed, no value needed
    } break;
    case 347: /* database_kw_opt ::= */
    {
      // Empty
    } break;
    case 350: /* cmd ::= VACUUM vinto */
    {
      yymsp[-1].minor.yy277 =
          synq_parse_vacuum_stmt(pCtx, SYNQ_NO_SPAN, yymsp[0].minor.yy277);
    } break;
    case 351: /* cmd ::= VACUUM nm vinto */
    {
      yymsp[-2].minor.yy277 = synq_parse_vacuum_stmt(
          pCtx, synq_span(pCtx, yymsp[-1].minor.yy0), yymsp[0].minor.yy277);
    } break;
    case 354: /* ecmd ::= explain cmdx SEMI */
    {
      (void)yymsp[-2].minor.yy320;
      yylhsminor.yy277 = synq_pass(pCtx, yymsp[-1].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 355: /* explain ::= EXPLAIN */
    {
      yymsp[0].minor.yy320 = 1;
      pCtx->pending_explain_mode = 1;
    } break;
    case 356: /* explain ::= EXPLAIN QUERY PLAN */
    {
      yymsp[-2].minor.yy320 = 2;
      pCtx->pending_explain_mode = 2;
    } break;
    case 357: /* cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP
                 sortlist RP where_opt */
    {
      SyntaqliteTextSpan idx_name = yymsp[-6].minor.yy0.z
                                        ? synq_span(pCtx, yymsp[-6].minor.yy0)
                                        : synq_span(pCtx, yymsp[-7].minor.yy0);
      SyntaqliteTextSpan idx_schema = yymsp[-6].minor.yy0.z
                                          ? synq_span(pCtx, yymsp[-7].minor.yy0)
                                          : SYNQ_NO_SPAN;
      yymsp[-11].minor.yy277 = synq_parse_create_index_stmt(
          pCtx, idx_name, idx_schema, synq_span(pCtx, yymsp[-4].minor.yy0),
          (SyntaqliteBool)yymsp[-10].minor.yy320,
          (SyntaqliteBool)yymsp[-8].minor.yy320, yymsp[-2].minor.yy277,
          yymsp[0].minor.yy277);
    } break;
    case 361: /* ifnotexists ::= IF NOT EXISTS */
    {
      yymsp[-2].minor.yy320 = 1;
    } break;
    case 362: /* cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS
                 select */
    {
      SyntaqliteTextSpan view_name = yymsp[-3].minor.yy0.z
                                         ? synq_span(pCtx, yymsp[-3].minor.yy0)
                                         : synq_span(pCtx, yymsp[-4].minor.yy0);
      SyntaqliteTextSpan view_schema =
          yymsp[-3].minor.yy0.z ? synq_span(pCtx, yymsp[-4].minor.yy0)
                                : SYNQ_NO_SPAN;
      yymsp[-8].minor.yy277 = synq_parse_create_view_stmt(
          pCtx, view_name, view_schema, (SyntaqliteBool)yymsp[-7].minor.yy320,
          (SyntaqliteBool)yymsp[-5].minor.yy320, yymsp[-2].minor.yy277,
          yymsp[0].minor.yy277);
    } break;
    case 366: /* values ::= VALUES LP nexprlist RP */
    {
      yymsp[-3].minor.yy277 = synq_parse_values_row_list(
          pCtx, SYNTAQLITE_NULL_NODE, yymsp[-1].minor.yy277);
    } break;
    case 367: /* mvalues ::= values COMMA LP nexprlist RP */
    case 368: /* mvalues ::= mvalues COMMA LP nexprlist RP */
      yytestcase(yyruleno == 368);
      {
        yymsp[-4].minor.yy277 = synq_parse_values_row_list(
            pCtx, yymsp[-4].minor.yy277, yymsp[-1].minor.yy277);
      }
      break;
    case 369: /* oneselect ::= values */
    case 370: /* oneselect ::= mvalues */
      yytestcase(yyruleno == 370);
      {
        yylhsminor.yy277 = synq_parse_values_clause(pCtx, yymsp[0].minor.yy277);
      }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 372: /* cmd ::= create_vtab LP vtabarglist RP */
    {
      // Capture module arguments span (content between parens).
      // Use token offsets/layer_id so this works correctly when the statement
      // is produced by a macro expansion; LP and RP share a layer within the
      // same reduction.
      SyntaqliteNode* vtab = AST_NODE(&pCtx->ast, yymsp[-3].minor.yy277);
      uint32_t args_start = yymsp[-2].minor.yy0.offset + yymsp[-2].minor.yy0.n;
      uint32_t args_end = yymsp[0].minor.yy0.offset;
      vtab->create_virtual_table_stmt.module_args = (SyntaqliteTextSpan){
          .offset = args_start,
          .length = args_end - args_start,
          .flags = 0,
          ._layer_id = yymsp[-2].minor.yy0.layer_id,
      };
      yylhsminor.yy277 = synq_pass(pCtx, yymsp[-3].minor.yy277);
    }
      yymsp[-3].minor.yy277 = yylhsminor.yy277;
      break;
    case 373: /* create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm
                 USING nm */
    {
      SyntaqliteTextSpan tbl_name = yymsp[-2].minor.yy0.z
                                        ? synq_span(pCtx, yymsp[-2].minor.yy0)
                                        : synq_span(pCtx, yymsp[-3].minor.yy0);
      SyntaqliteTextSpan tbl_schema = yymsp[-2].minor.yy0.z
                                          ? synq_span(pCtx, yymsp[-3].minor.yy0)
                                          : SYNQ_NO_SPAN;
      yymsp[-7].minor.yy277 = synq_parse_create_virtual_table_stmt(
          pCtx, tbl_name, tbl_schema, synq_span(pCtx, yymsp[0].minor.yy0),
          (SyntaqliteBool)yymsp[-4].minor.yy320,
          SYNQ_NO_SPAN);  // module_args = none by default
    } break;
    case 384: /* windowdefn_list ::= windowdefn */
    {
      yylhsminor.yy277 = synq_parse_named_window_def_list(
          pCtx, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy277);
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 385: /* windowdefn_list ::= windowdefn_list COMMA windowdefn */
    {
      yylhsminor.yy277 = synq_parse_named_window_def_list(
          pCtx, yymsp[-2].minor.yy277, yymsp[0].minor.yy277);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 386: /* windowdefn ::= nm AS LP window RP */
    {
      yylhsminor.yy277 = synq_parse_named_window_def(
          pCtx, synq_span(pCtx, yymsp[-4].minor.yy0), yymsp[-1].minor.yy277);
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 387: /* window ::= PARTITION BY nexprlist orderby_opt frame_opt */
    {
      yymsp[-4].minor.yy277 = synq_parse_window_def(
          pCtx, SYNQ_NO_SPAN, SYNQ_NO_SPAN, yymsp[-2].minor.yy277,
          yymsp[-1].minor.yy277, yymsp[0].minor.yy277);
    } break;
    case 388: /* window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
    {
      yylhsminor.yy277 = synq_parse_window_def(
          pCtx, SYNQ_NO_SPAN, synq_span(pCtx, yymsp[-5].minor.yy0),
          yymsp[-2].minor.yy277, yymsp[-1].minor.yy277, yymsp[0].minor.yy277);
    }
      yymsp[-5].minor.yy277 = yylhsminor.yy277;
      break;
    case 389: /* window ::= ORDER BY sortlist frame_opt */
    {
      yymsp[-3].minor.yy277 = synq_parse_window_def(
          pCtx, SYNQ_NO_SPAN, SYNQ_NO_SPAN, SYNTAQLITE_NULL_NODE,
          yymsp[-1].minor.yy277, yymsp[0].minor.yy277);
    } break;
    case 390: /* window ::= nm ORDER BY sortlist frame_opt */
    {
      yylhsminor.yy277 = synq_parse_window_def(
          pCtx, SYNQ_NO_SPAN, synq_span(pCtx, yymsp[-4].minor.yy0),
          SYNTAQLITE_NULL_NODE, yymsp[-1].minor.yy277, yymsp[0].minor.yy277);
    }
      yymsp[-4].minor.yy277 = yylhsminor.yy277;
      break;
    case 391: /* window ::= frame_opt */
    {
      yylhsminor.yy277 = synq_parse_window_def(
          pCtx, SYNQ_NO_SPAN, SYNQ_NO_SPAN, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy277);
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 392: /* window ::= nm frame_opt */
    {
      yylhsminor.yy277 = synq_parse_window_def(
          pCtx, SYNQ_NO_SPAN, synq_span(pCtx, yymsp[-1].minor.yy0),
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy277);
    }
      yymsp[-1].minor.yy277 = yylhsminor.yy277;
      break;
    case 394: /* frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
    {
      // Single bound: start=yymsp[-1].minor.yy277, end=CURRENT ROW (implicit)
      uint32_t end_bound = synq_parse_frame_bound(
          pCtx, SYNTAQLITE_FRAME_BOUND_TYPE_CURRENT_ROW, SYNTAQLITE_NULL_NODE);
      yylhsminor.yy277 = synq_parse_frame_spec(
          pCtx, (SyntaqliteFrameType)yymsp[-2].minor.yy320,
          (SyntaqliteFrameExclude)yymsp[0].minor.yy320, yymsp[-1].minor.yy277,
          end_bound);
    }
      yymsp[-2].minor.yy277 = yylhsminor.yy277;
      break;
    case 395: /* frame_opt ::= range_or_rows BETWEEN frame_bound_s AND
                 frame_bound_e frame_exclude_opt */
    {
      yylhsminor.yy277 = synq_parse_frame_spec(
          pCtx, (SyntaqliteFrameType)yymsp[-5].minor.yy320,
          (SyntaqliteFrameExclude)yymsp[0].minor.yy320, yymsp[-3].minor.yy277,
          yymsp[-1].minor.yy277);
    }
      yymsp[-5].minor.yy277 = yylhsminor.yy277;
      break;
    case 396: /* range_or_rows ::= RANGE|ROWS|GROUPS */
    {
      switch (yymsp[0].minor.yy0.type) {
        case SYNTAQLITE_TK_RANGE:
          yylhsminor.yy320 = SYNTAQLITE_FRAME_TYPE_RANGE;
          break;
        case SYNTAQLITE_TK_ROWS:
          yylhsminor.yy320 = SYNTAQLITE_FRAME_TYPE_ROWS;
          break;
        default:
          yylhsminor.yy320 = SYNTAQLITE_FRAME_TYPE_GROUPS;
          break;
      }
    }
      yymsp[0].minor.yy320 = yylhsminor.yy320;
      break;
    case 398: /* frame_bound_s ::= UNBOUNDED PRECEDING */
    {
      yymsp[-1].minor.yy277 = synq_parse_frame_bound(
          pCtx, SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_PRECEDING,
          SYNTAQLITE_NULL_NODE);
    } break;
    case 400: /* frame_bound_e ::= UNBOUNDED FOLLOWING */
    {
      yymsp[-1].minor.yy277 = synq_parse_frame_bound(
          pCtx, SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_FOLLOWING,
          SYNTAQLITE_NULL_NODE);
    } break;
    case 401: /* frame_bound ::= expr PRECEDING|FOLLOWING */
    {
      SyntaqliteFrameBoundType bt =
          (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_PRECEDING)
              ? SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_PRECEDING
              : SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_FOLLOWING;
      yylhsminor.yy277 =
          synq_parse_frame_bound(pCtx, bt, yymsp[-1].minor.yy277);
    }
      yymsp[-1].minor.yy277 = yylhsminor.yy277;
      break;
    case 402: /* frame_bound ::= CURRENT ROW */
    {
      yymsp[-1].minor.yy277 = synq_parse_frame_bound(
          pCtx, SYNTAQLITE_FRAME_BOUND_TYPE_CURRENT_ROW, SYNTAQLITE_NULL_NODE);
    } break;
    case 403: /* frame_exclude_opt ::= */
    {
      yymsp[1].minor.yy320 = SYNTAQLITE_FRAME_EXCLUDE_NONE;
    } break;
    case 405: /* frame_exclude ::= NO OTHERS */
    {
      yymsp[-1].minor.yy320 = SYNTAQLITE_FRAME_EXCLUDE_NO_OTHERS;
    } break;
    case 406: /* frame_exclude ::= CURRENT ROW */
    {
      yymsp[-1].minor.yy320 = SYNTAQLITE_FRAME_EXCLUDE_CURRENT_ROW;
    } break;
    case 407: /* frame_exclude ::= GROUP|TIES */
    {
      yylhsminor.yy320 = (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_GROUP)
                             ? SYNTAQLITE_FRAME_EXCLUDE_GROUP
                             : SYNTAQLITE_FRAME_EXCLUDE_TIES;
    }
      yymsp[0].minor.yy320 = yylhsminor.yy320;
      break;
    case 409: /* filter_over ::= filter_clause over_clause */
    {
      // Unpack the over_clause FilterOver to combine with filter expr
      SyntaqliteFilterOver* fo_over =
          AST_NODE_AS(SyntaqliteFilterOver, &pCtx->ast, yymsp[0].minor.yy277);
      yylhsminor.yy277 = synq_parse_filter_over(
          pCtx, yymsp[-1].minor.yy277, fo_over->over_def, SYNQ_NO_SPAN);
    }
      yymsp[-1].minor.yy277 = yylhsminor.yy277;
      break;
    case 411: /* filter_over ::= filter_clause */
    {
      yylhsminor.yy277 = synq_parse_filter_over(
          pCtx, yymsp[0].minor.yy277, SYNTAQLITE_NULL_NODE, SYNQ_NO_SPAN);
    }
      yymsp[0].minor.yy277 = yylhsminor.yy277;
      break;
    case 412: /* over_clause ::= OVER LP window RP */
    {
      yymsp[-3].minor.yy277 = synq_parse_filter_over(
          pCtx, SYNTAQLITE_NULL_NODE, yymsp[-1].minor.yy277, SYNQ_NO_SPAN);
    } break;
    case 413: /* over_clause ::= OVER nm */
    {
      uint32_t wdef = synq_parse_window_def(
          pCtx, synq_span(pCtx, yymsp[0].minor.yy0), SYNQ_NO_SPAN,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-1].minor.yy277 = synq_parse_filter_over(pCtx, SYNTAQLITE_NULL_NODE,
                                                     wdef, SYNQ_NO_SPAN);
    } break;
    case 414: /* filter_clause ::= FILTER LP WHERE expr RP */
    {
      yymsp[-4].minor.yy277 = synq_pass(pCtx, yymsp[-1].minor.yy277);
    } break;
    default:
      break;
      /********** End reduce actions
       * ************************************************/
  };
  assert(yyruleno < sizeof(yyRuleInfoLhs) / sizeof(yyRuleInfoLhs[0]));
  yygoto = yyRuleInfoLhs[yyruleno];
  yysize = yyRuleInfoNRhs[yyruleno];
  yyact = yy_find_reduce_action(yymsp[yysize].stateno, (YYCODETYPE)yygoto);

  /* There are no SHIFTREDUCE actions on nonterminals because the table
  ** generator has simplified them to pure REDUCE actions. */
  assert(!(yyact > YY_MAX_SHIFT && yyact <= YY_MAX_SHIFTREDUCE));

  /* It is not possible for a REDUCE to be followed by an error */
  assert(yyact != YY_ERROR_ACTION);

  yymsp += yysize + 1;
  yypParser->yytos = yymsp;
  yymsp->stateno = (YYACTIONTYPE)yyact;
  yymsp->major = (YYCODETYPE)yygoto;
  yyTraceShift(yypParser, yyact, "... then shift");
  return yyact;
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(yyParser* yypParser /* The parser */
) {
  SynqSqliteParseARG_FETCH SynqSqliteParseCTX_FETCH
#ifndef NDEBUG
      if (yyTraceFILE) {
    fprintf(yyTraceFILE, "%sFail!\n", yyTracePrompt);
  }
#endif
  while (yypParser->yytos > yypParser->yystack)
    yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  /************ Begin %parse_failure code
   * ***************************************/

  if (pCtx) {
    pCtx->error = 1;
  }
  /************ End %parse_failure code
   * *****************************************/
  SynqSqliteParseARG_STORE /* Suppress warning about unused %extra_argument
                              variable */
      SynqSqliteParseCTX_STORE
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
    yyParser* yypParser,             /* The parser */
    int yymajor,                     /* The major type of the error token */
    SynqSqliteParseTOKENTYPE yyminor /* The minor type of the error token */
) {
  SynqSqliteParseARG_FETCH SynqSqliteParseCTX_FETCH
#define TOKEN yyminor
      /************ Begin %syntax_error code
       ****************************************/

      (void) yymajor;
  (void)TOKEN;
  if (pCtx) {
    pCtx->error = 1;
  }
  /************ End %syntax_error code
   * ******************************************/
  SynqSqliteParseARG_STORE /* Suppress warning about unused %extra_argument
                              variable */
      SynqSqliteParseCTX_STORE
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(yyParser* yypParser /* The parser */
) {
  SynqSqliteParseARG_FETCH SynqSqliteParseCTX_FETCH
#ifndef NDEBUG
      if (yyTraceFILE) {
    fprintf(yyTraceFILE, "%sAccept!\n", yyTracePrompt);
  }
#endif
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
  assert(yypParser->yytos == yypParser->yystack);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  /*********** Begin %parse_accept code
   * *****************************************/
  /*********** End %parse_accept code
   * *******************************************/
  SynqSqliteParseARG_STORE /* Suppress warning about unused %extra_argument
                              variable */
      SynqSqliteParseCTX_STORE
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "SynqSqliteParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void SynqSqliteParse(
    void* yyp,                       /* The parser */
    int yymajor,                     /* The major token code number */
    SynqSqliteParseTOKENTYPE yyminor /* The value for the token */
        SynqSqliteParseARG_PDECL     /* Optional %extra_argument parameter */
) {
  YYMINORTYPE yyminorunion;
  YYACTIONTYPE yyact; /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput; /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0; /* True if yymajor has invoked an error */
#endif
  yyParser* yypParser = (yyParser*)yyp; /* The parser */
  SynqSqliteParseCTX_FETCH SynqSqliteParseARG_STORE

      assert(yypParser->yytos != 0);
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor == 0);
#endif

  yyact = yypParser->yytos->stateno;
#ifndef NDEBUG
  if (yyTraceFILE) {
    if (yyact < YY_MIN_REDUCE) {
      fprintf(yyTraceFILE, "%sInput '%s' in state %d\n", yyTracePrompt,
              yyTokenName[yymajor], yyact);
    } else {
      fprintf(yyTraceFILE, "%sInput '%s' with pending reduce %d\n",
              yyTracePrompt, yyTokenName[yymajor], yyact - YY_MIN_REDUCE);
    }
  }
#endif

  while (1) { /* Exit by "break" */
    assert(yypParser->yytos >= yypParser->yystack);
    assert(yyact == yypParser->yytos->stateno);
    yyact = yy_find_shift_action((YYCODETYPE)yymajor, yyact);
    if (yyact >= YY_MIN_REDUCE) {
      unsigned int yyruleno = yyact - YY_MIN_REDUCE; /* Reduce by this rule */
#ifndef NDEBUG
      assert(yyruleno < (int)(sizeof(yyRuleName) / sizeof(yyRuleName[0])));
      if (yyTraceFILE) {
        int yysize = yyRuleInfoNRhs[yyruleno];
        if (yysize) {
          fprintf(
              yyTraceFILE, "%sReduce %d [%s]%s, pop back to state %d.\n",
              yyTracePrompt, yyruleno, yyRuleName[yyruleno],
              yyruleno < YYNRULE_WITH_ACTION ? "" : " without external action",
              yypParser->yytos[yysize].stateno);
        } else {
          fprintf(
              yyTraceFILE, "%sReduce %d [%s]%s.\n", yyTracePrompt, yyruleno,
              yyRuleName[yyruleno],
              yyruleno < YYNRULE_WITH_ACTION ? "" : " without external action");
        }
      }
#endif /* NDEBUG */

      /* Check that the stack is large enough to grow by a single entry
      ** if the RHS of the rule is empty.  This ensures that there is room
      ** enough on the stack to push the LHS value */
      if (yyRuleInfoNRhs[yyruleno] == 0) {
#ifdef YYTRACKMAXSTACKDEPTH
        if ((int)(yypParser->yytos - yypParser->yystack) > yypParser->yyhwm) {
          yypParser->yyhwm++;
          assert(yypParser->yyhwm ==
                 (int)(yypParser->yytos - yypParser->yystack));
        }
#endif
        if (yypParser->yytos >= yypParser->yystackEnd) {
          if (yyGrowStack(yypParser)) {
            yyStackOverflow(yypParser);
            break;
          }
        }
      }
      yyact = yy_reduce(yypParser, yyruleno, yymajor,
                        yyminor SynqSqliteParseCTX_PARAM);
    } else if (yyact <= YY_MAX_SHIFTREDUCE) {
      yy_shift(yypParser, yyact, (YYCODETYPE)yymajor, yyminor);
#ifndef YYNOERRORRECOVERY
      yypParser->yyerrcnt--;
#endif
      break;
    } else if (yyact == YY_ACCEPT_ACTION) {
      yypParser->yytos--;
      yy_accept(yypParser);
      return;
    } else {
      assert(yyact == YY_ERROR_ACTION);
      yyminorunion.yy0 = yyminor;
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if (yyTraceFILE) {
        fprintf(yyTraceFILE, "%sSyntax Error!\n", yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if (yypParser->yyerrcnt < 0) {
        yy_syntax_error(yypParser, yymajor, yyminor);
      }
      yymx = yypParser->yytos->major;
      if (yymx == YYERRORSYMBOL || yyerrorhit) {
#ifndef NDEBUG
        if (yyTraceFILE) {
          fprintf(yyTraceFILE, "%sDiscard input token %s\n", yyTracePrompt,
                  yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
        yymajor = YYNOCODE;
      } else {
        while (yypParser->yytos > yypParser->yystack) {
          yyact =
              yy_find_reduce_action(yypParser->yytos->stateno, YYERRORSYMBOL);
          if (yyact <= YY_MAX_SHIFTREDUCE)
            break;
          yy_pop_parser_stack(yypParser);
        }
        if (yypParser->yytos <= yypParser->yystack || yymajor == 0) {
          yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
          yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
          yypParser->yyerrcnt = -1;
#endif
          yymajor = YYNOCODE;
        } else if (yymx != YYERRORSYMBOL) {
          yy_shift(yypParser, yyact, YYERRORSYMBOL, yyminor);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
      if (yymajor == YYNOCODE)
        break;
      yyact = yypParser->yytos->stateno;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser, yymajor, yyminor);
      yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
      break;
#else /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if (yypParser->yyerrcnt <= 0) {
        yy_syntax_error(yypParser, yymajor, yyminor);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
      if (yyendofinput) {
        yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
        yypParser->yyerrcnt = -1;
#endif
      }
      break;
#endif
    }
  }
#ifndef NDEBUG
  if (yyTraceFILE) {
    yyStackEntry* i;
    char cDiv = '[';
    fprintf(yyTraceFILE, "%sReturn. Stack=", yyTracePrompt);
    for (i = &yypParser->yystack[1]; i <= yypParser->yytos; i++) {
      fprintf(yyTraceFILE, "%c%s", cDiv, yyTokenName[i->major]);
      cDiv = ' ';
    }
    fprintf(yyTraceFILE, "]\n");
  }
#endif
  return;
}

/*
** Return the fallback token corresponding to canonical token iToken, or
** 0 if iToken has no fallback.
*/
int SynqSqliteParseFallback(int iToken) {
#ifdef YYFALLBACK
  assert(iToken < (int)(sizeof(yyFallback) / sizeof(yyFallback[0])));
  return yyFallback[iToken];
#else
  (void)iToken;
  return 0;
#endif
}

/* syntaqlite extension: enumerate terminals that can be shifted/reduced from
** the parser's current state. Returns the total number of expected tokens,
** even when out_tokens/out_cap only request a prefix. */
static YYACTIONTYPE synq_find_reduce_action_safe(YYACTIONTYPE stateno,
                                                 YYCODETYPE iLookAhead) {
  int i;
  if (stateno > YY_REDUCE_COUNT)
    return yy_default[stateno];
  i = yy_reduce_ofst[stateno] + iLookAhead;
  if (i < 0 || i >= YY_ACTTAB_COUNT || yy_lookahead[i] != iLookAhead) {
    return yy_default[stateno];
  }
  return yy_action[i];
}

/* Like yy_find_shift_action but skips YYWILDCARD and YYFALLBACK paths.
** Wildcard matches are for error recovery (ANY token) and fallback matches
** accept keywords as identifiers — neither should appear as keyword
** autocompletion suggestions. */
static YYACTIONTYPE synq_find_shift_action_strict(YYCODETYPE iLookAhead,
                                                  YYACTIONTYPE stateno) {
  int i;
  if (stateno > YY_MAX_SHIFT)
    return stateno;
  i = yy_shift_ofst[stateno];
  assert(i >= 0);
  assert(i + YYNTOKEN <= (int)YY_NLOOKAHEAD);
  i += iLookAhead;
  if (yy_lookahead[i] != iLookAhead) {
    /* No specific entry — skip fallback and wildcard, use default. */
    return yy_default[stateno];
  }
  return yy_action[i];
}

static int synq_can_lookahead(yyParser* p, uint32_t token) {
  YYACTIONTYPE stack_states[YYSTACKDEPTH + 1];
  int top = 0;
  int i = 0;
  int steps = 0;

  if (p == 0 || p->yytos == 0)
    return 0;

  top = (int)(p->yytos - p->yystack);
  if (top < 0 || top > YYSTACKDEPTH)
    return 0;
  for (i = 0; i <= top; i++) {
    stack_states[i] = p->yystack[i].stateno;
  }

  while (steps++ < 10000) {
    YYACTIONTYPE action =
        synq_find_shift_action_strict((YYCODETYPE)token, stack_states[top]);

    if (action == YY_ERROR_ACTION || action == YY_NO_ACTION)
      return 0;
    if (action == YY_ACCEPT_ACTION)
      return token == 0;
    if (action <= YY_MAX_SHIFT)
      return 1;

    /* Shift-reduce: the token is consumed (shifted) then a reduce follows.
    ** This means the token IS accepted, same as a pure shift. */
    if (action >= YY_MIN_SHIFTREDUCE && action <= YY_MAX_SHIFTREDUCE)
      return 1;

    if (action >= YY_MIN_REDUCE && action <= YY_MAX_REDUCE) {
      int rule = (int)(action - YY_MIN_REDUCE);
      int yysize = yyRuleInfoNRhs[rule];
      YYACTIONTYPE goto_state;

      top += yysize; /* yyRuleInfoNRhs is negative rhs-size */
      if (top < 0)
        return 0;

      goto_state =
          synq_find_reduce_action_safe(stack_states[top], yyRuleInfoLhs[rule]);
      if (goto_state == YY_ERROR_ACTION || goto_state == YY_NO_ACTION)
        return 0;

      if (top >= YYSTACKDEPTH)
        return 0;
      top++;
      stack_states[top] = goto_state;
      continue;
    }

    return 0;
  }

  return 0;
}

uint32_t SynqSqliteParseExpectedTokens(void* parser,
                                       uint32_t* out_tokens,
                                       uint32_t out_cap) {
  uint32_t n = 0;
  uint32_t token = 0;
  yyParser* p = (yyParser*)parser;

  if (p == 0 || p->yytos == 0)
    return 0;

  for (token = 1; token < YYNTOKEN; token++) {
    if (!synq_can_lookahead(p, token))
      continue;
    if (out_tokens && n < out_cap)
      out_tokens[n] = token;
    n++;
  }

  return n;
}

/* syntaqlite extension: non-terminal IDs for completion context. */
#define SYNQ_NT_INPUT 189
#define SYNQ_NT_CMDLIST 190
#define SYNQ_NT_ECMD 191
#define SYNQ_NT_CMDX 192
#define SYNQ_NT_ERROR 193
#define SYNQ_NT_CMD 194
#define SYNQ_NT_EXPR 195
#define SYNQ_NT_DISTINCT 196
#define SYNQ_NT_EXPRLIST 197
#define SYNQ_NT_SORTLIST 198
#define SYNQ_NT_FILTER_OVER 199
#define SYNQ_NT_TYPETOKEN 200
#define SYNQ_NT_TYPENAME 201
#define SYNQ_NT_SIGNED 202
#define SYNQ_NT_SELCOLLIST 203
#define SYNQ_NT_SCLP 204
#define SYNQ_NT_SCANPT 205
#define SYNQ_NT_NM 206
#define SYNQ_NT_MULTISELECT_OP 207
#define SYNQ_NT_IN_OP 208
#define SYNQ_NT_DBNM 209
#define SYNQ_NT_SELECTNOWITH 210
#define SYNQ_NT_ONESELECT 211
#define SYNQ_NT_SELECT 212
#define SYNQ_NT_PAREN_EXPRLIST 213
#define SYNQ_NT_LIKEOP 214
#define SYNQ_NT_BETWEEN_OP 215
#define SYNQ_NT_CASE_OPERAND 216
#define SYNQ_NT_CASE_EXPRLIST 217
#define SYNQ_NT_CASE_ELSE 218
#define SYNQ_NT_SCANTOK 219
#define SYNQ_NT_AUTOINC 220
#define SYNQ_NT_REFARGS 221
#define SYNQ_NT_REFARG 222
#define SYNQ_NT_REFACT 223
#define SYNQ_NT_DEFER_SUBCLAUSE 224
#define SYNQ_NT_INIT_DEFERRED_PRED_OPT 225
#define SYNQ_NT_DEFER_SUBCLAUSE_OPT 226
#define SYNQ_NT_TABLE_OPTION_SET 227
#define SYNQ_NT_TABLE_OPTION 228
#define SYNQ_NT_ONCONF 229
#define SYNQ_NT_CCONS 230
#define SYNQ_NT_CARGLIST 231
#define SYNQ_NT_TCONS 232
#define SYNQ_NT_CONSLIST 233
#define SYNQ_NT_GENERATED 234
#define SYNQ_NT_CREATE_TABLE 235
#define SYNQ_NT_CREATE_TABLE_ARGS 236
#define SYNQ_NT_CREATEKW 237
#define SYNQ_NT_TEMP 238
#define SYNQ_NT_IFNOTEXISTS 239
#define SYNQ_NT_COLUMNLIST 240
#define SYNQ_NT_CONSLIST_OPT 241
#define SYNQ_NT_COLUMNNAME 242
#define SYNQ_NT_TERM 243
#define SYNQ_NT_SORTORDER 244
#define SYNQ_NT_EIDLIST_OPT 245
#define SYNQ_NT_TCONSCOMMA 246
#define SYNQ_NT_EIDLIST 247
#define SYNQ_NT_RESOLVETYPE 248
#define SYNQ_NT_WITHNM 249
#define SYNQ_NT_WQAS 250
#define SYNQ_NT_COLLATE 251
#define SYNQ_NT_WQLIST 252
#define SYNQ_NT_WQITEM 253
#define SYNQ_NT_WITH 254
#define SYNQ_NT_INSERT_CMD 255
#define SYNQ_NT_ORCONF 256
#define SYNQ_NT_INDEXED_OPT 257
#define SYNQ_NT_WHERE_OPT_RET 258
#define SYNQ_NT_UPSERT 259
#define SYNQ_NT_RETURNING 260
#define SYNQ_NT_XFULLNAME 261
#define SYNQ_NT_ORDERBY_OPT 262
#define SYNQ_NT_LIMIT_OPT 263
#define SYNQ_NT_SETLIST 264
#define SYNQ_NT_FROM 265
#define SYNQ_NT_IDLIST_OPT 266
#define SYNQ_NT_RAISETYPE 267
#define SYNQ_NT_INDEXED_BY 268
#define SYNQ_NT_IDLIST 269
#define SYNQ_NT_WHERE_OPT 270
#define SYNQ_NT_NEXPRLIST 271
#define SYNQ_NT_NMORERR 272
#define SYNQ_NT_NULLS 273
#define SYNQ_NT_IFEXISTS 274
#define SYNQ_NT_TRANSTYPE 275
#define SYNQ_NT_TRANS_OPT 276
#define SYNQ_NT_SAVEPOINT_OPT 277
#define SYNQ_NT_KWCOLUMN_OPT 278
#define SYNQ_NT_FULLNAME 279
#define SYNQ_NT_ADD_COLUMN_FULLNAME 280
#define SYNQ_NT_AS 281
#define SYNQ_NT_GROUPBY_OPT 282
#define SYNQ_NT_HAVING_OPT 283
#define SYNQ_NT_WINDOW_CLAUSE 284
#define SYNQ_NT_SELTABLIST 285
#define SYNQ_NT_ON_USING 286
#define SYNQ_NT_JOINOP 287
#define SYNQ_NT_STL_PREFIX 288
#define SYNQ_NT_TRIGGER_TIME 289
#define SYNQ_NT_TRNM 290
#define SYNQ_NT_TRIGGER_DECL 291
#define SYNQ_NT_TRIGGER_CMD_LIST 292
#define SYNQ_NT_TRIGGER_EVENT 293
#define SYNQ_NT_FOREACH_CLAUSE 294
#define SYNQ_NT_WHEN_CLAUSE 295
#define SYNQ_NT_TRIGGER_CMD 296
#define SYNQ_NT_TRIDXBY 297
#define SYNQ_NT_PLUS_NUM 298
#define SYNQ_NT_MINUS_NUM 299
#define SYNQ_NT_NMNUM 300
#define SYNQ_NT_UNIQUEFLAG 301
#define SYNQ_NT_EXPLAIN 302
#define SYNQ_NT_DATABASE_KW_OPT 303
#define SYNQ_NT_KEY_OPT 304
#define SYNQ_NT_VINTO 305
#define SYNQ_NT_VALUES 306
#define SYNQ_NT_MVALUES 307
#define SYNQ_NT_CREATE_VTAB 308
#define SYNQ_NT_VTABARGLIST 309
#define SYNQ_NT_VTABARG 310
#define SYNQ_NT_VTABARGTOKEN 311
#define SYNQ_NT_LP 312
#define SYNQ_NT_ANYLIST 313
#define SYNQ_NT_RANGE_OR_ROWS 314
#define SYNQ_NT_FRAME_EXCLUDE_OPT 315
#define SYNQ_NT_FRAME_EXCLUDE 316
#define SYNQ_NT_WINDOWDEFN_LIST 317
#define SYNQ_NT_WINDOWDEFN 318
#define SYNQ_NT_WINDOW 319
#define SYNQ_NT_FRAME_OPT 320
#define SYNQ_NT_FRAME_BOUND_S 321
#define SYNQ_NT_FRAME_BOUND_E 322
#define SYNQ_NT_FRAME_BOUND 323
#define SYNQ_NT_FILTER_CLAUSE 324
#define SYNQ_NT_OVER_CLAUSE 325

/* syntaqlite extension: probe the goto table to check if a state has
** an explicit goto entry for non-terminal `nt`. */
static int synq_has_goto(YYACTIONTYPE state, YYCODETYPE nt) {
  int i;
  if (state > YY_REDUCE_COUNT)
    return 0;
  i = yy_reduce_ofst[state] + nt;
  if (i < 0 || i >= YY_ACTTAB_COUNT)
    return 0;
  return yy_lookahead[i] == nt;
}

/* syntaqlite extension: determine the semantic completion context
** (Expression vs TableRef) by walking the parser stack. Returns one of
** SYNTAQLITE_COMPLETION_CONTEXT_*. */
uint32_t SynqSqliteParseCompletionContext(void* parser) {
  yyParser* p = (yyParser*)parser;
  if (p == 0 || p->yytos == 0)
    return SYNTAQLITE_COMPLETION_CONTEXT_UNKNOWN;

  for (yyStackEntry* e = p->yytos; e >= p->yystack; e--) {
    YYACTIONTYPE s = e->stateno;

    /* Check if this state has gotos for table-ref non-terminals. */
    if (synq_has_goto(s, SYNQ_NT_SELTABLIST) ||
        synq_has_goto(s, SYNQ_NT_FULLNAME) ||
        synq_has_goto(s, SYNQ_NT_XFULLNAME)) {
      return SYNTAQLITE_COMPLETION_CONTEXT_TABLE_REF;
    }

    /* Check if this state has gotos for expression non-terminals. */
    if (synq_has_goto(s, SYNQ_NT_EXPR)) {
      return SYNTAQLITE_COMPLETION_CONTEXT_EXPRESSION;
    }
  }
  return SYNTAQLITE_COMPLETION_CONTEXT_UNKNOWN;
}
