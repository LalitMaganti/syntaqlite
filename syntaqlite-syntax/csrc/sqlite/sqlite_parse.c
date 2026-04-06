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
#define YYNOCODE 325
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 92
#define SynqSqliteParseTOKENTYPE SynqParseToken
typedef union {
  int yyinit;
  SynqSqliteParseTOKENTYPE yy0;
  SynqWhereRetValue yy5;
  SynqConstraintListValue yy94;
  SynqWithValue yy95;
  uint32_t yy141;
  SynqOnUsingValue yy216;
  SynqUpsertValue yy336;
  SynqConstraintValue yy356;
  SynqColumnNameValue yy452;
  int yy592;
  int yy651;
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
#define YYERRORSYMBOL 192
#define YYERRSYMDT yy651
#define YYFALLBACK 1
#define YYNSTATE 595
#define YYNRULE 415
#define YYNRULE_WITH_ACTION 415
#define YYNTOKEN 188
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
#define YY_ACTTAB_COUNT (2340)
static const YYACTIONTYPE yy_action[] = {
    /*     0 */ 245,
    1362,
    1581,
    1064,
    1064,
    1074,
    91,
    93,
    1348,
    295,
    /*    10 */ 1536,
    530,
    295,
    1536,
    297,
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
    424,
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
    1657,
    /*    40 */ 406,
    325,
    528,
    1342,
    1351,
    84,
    85,
    420,
    42,
    493,
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
    436,
    295,
    1536,
    306,
    473,
    /*    70 */ 87,
    87,
    87,
    87,
    90,
    587,
    63,
    540,
    501,
    1007,
    /*    80 */ 1436,
    1354,
    508,
    1532,
    273,
    245,
    1477,
    241,
    482,
    1510,
    /*    90 */ 100,
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
    299,
    571,
    569,
    /*   110 */ 82,
    81,
    453,
    1456,
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
    /*   140 */ 1630,
    62,
    84,
    85,
    420,
    42,
    399,
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
    484,
    /*   160 */ 361,
    359,
    571,
    569,
    84,
    85,
    420,
    42,
    579,
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
    324,
    1037,
    1371,
    114,
    1398,
    439,
    1396,
    1665,
    /*   190 */ 394,
    290,
    1677,
    46,
    241,
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
    /*   210 */ 1007,
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
    493,
    1456,
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
    1427,
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
    /*   270 */ 1428,
    1061,
    84,
    85,
    420,
    42,
    453,
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
    /*   290 */ 1407,
    302,
    359,
    477,
    346,
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
    909,
    909,
    906,
    891,
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
    /*   330 */ 880,
    246,
    549,
    403,
    547,
    489,
    110,
    1405,
    1061,
    1062,
    /*   340 */ 1061,
    1164,
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
    1074,
    1061,
    1062,
    1061,
    1169,
    /*   360 */ 1169,
    505,
    295,
    1536,
    1642,
    1075,
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
    233,
    455,
    /*   380 */ 454,
    493,
    265,
    301,
    520,
    517,
    516,
    1611,
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
    900,
    1611,
    1353,
    1061,
    246,
    549,
    84,
    85,
    /*   420 */ 420,
    42,
    1064,
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
    1481,
    295,
    1536,
    580,
    305,
    /*   440 */ 495,
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
    /*   460 */ 295,
    1536,
    583,
    13,
    484,
    361,
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
    239,
    1610,
    /*   480 */ 464,
    465,
    1061,
    1062,
    1061,
    571,
    569,
    234,
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
    508,
    532,
    588,
    493,
    1610,
    1608,
    1606,
    429,
    467,
    /*   510 */ 1061,
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
    1105,
    430,
    106,
    292,
    1536,
    1107,
    /*   530 */ 293,
    1536,
    571,
    569,
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
    386,
    500,
    1106,
    571,
    569,
    84,
    85,
    /*   560 */ 420,
    42,
    307,
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
    1061,
    176,
    1061,
    1062,
    1061,
    /*   580 */ 343,
    84,
    85,
    420,
    42,
    533,
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
    1061,
    1618,
    /*   600 */ 1619,
    1061,
    463,
    1388,
    1286,
    579,
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
    1531,
    324,
    /*   620 */ 295,
    1536,
    1534,
    571,
    569,
    1061,
    571,
    569,
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
    108,
    1061,
    1062,
    1061,
    535,
    313,
    456,
    172,
    540,
    /*   650 */ 201,
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
    1061,
    1062,
    1061,
    1061,
    1062,
    /*   670 */ 1061,
    210,
    1061,
    242,
    1056,
    84,
    85,
    420,
    42,
    1478,
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
    1062,
    1061,
    338,
    6,
    340,
    84,
    85,
    /*   700 */ 420,
    42,
    1672,
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
    1166,
    571,
    569,
    462,
    1166,
    /*   720 */ 317,
    84,
    85,
    420,
    42,
    1155,
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
    1566,
    1061,
    /*   740 */ 1062,
    1061,
    1061,
    1618,
    1619,
    211,
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
    6,
    503,
    /*   760 */ 541,
    1664,
    341,
    538,
    1671,
    1400,
    1258,
    399,
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
    553,
    586,
    1258,
    1428,
    339,
    1258,
    1061,
    582,
    1407,
    /*   790 */ 887,
    887,
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
    404,
    1690,
    72,
    6,
    448,
    1061,
    /*   810 */ 1062,
    1061,
    1061,
    1669,
    1081,
    84,
    85,
    420,
    42,
    552,
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
    403,
    398,
    1061,
    112,
    1405,
    84,
    85,
    420,
    /*   840 */ 42,
    1258,
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
    1061,
    1062,
    1061,
    80,
    1258,
    74,
    /*   860 */ 5,
    1258,
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
    1061,
    /*   880 */ 1062,
    1061,
    1407,
    1345,
    1083,
    2,
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
    67,
    1518,
    /*   900 */ 426,
    1061,
    1062,
    1061,
    1084,
    1343,
    579,
    98,
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
    /*   920 */ 324,
    291,
    100,
    79,
    245,
    403,
    1127,
    575,
    109,
    1405,
    /*   930 */ 91,
    93,
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
    1082,
    84,
    92,
    420,
    42,
    /*   950 */ 1581,
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
    269,
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
    1693,
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
    1127,
    67,
    1037,
    328,
    38,
    9,
    /*  1000 */ 579,
    431,
    590,
    175,
    87,
    87,
    87,
    87,
    1061,
    587,
    /*  1010 */ 1127,
    499,
    201,
    437,
    324,
    76,
    951,
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
    /*  1030 */ 532,
    459,
    432,
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
    425,
    576,
    1456,
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
    1061,
    1062,
    1061,
    1127,
    8,
    /*  1080 */ 1037,
    1069,
    114,
    937,
    1037,
    245,
    148,
    175,
    78,
    78,
    /*  1090 */ 459,
    91,
    93,
    587,
    471,
    358,
    77,
    587,
    459,
    577,
    /*  1100 */ 459,
    1065,
    1067,
    15,
    4,
    576,
    265,
    579,
    520,
    517,
    /*  1110 */ 516,
    6,
    59,
    533,
    44,
    584,
    1067,
    1670,
    515,
    25,
    /*  1120 */ 6,
    324,
    395,
    300,
    555,
    550,
    1671,
    329,
    1037,
    554,
    /*  1130 */ 148,
    1456,
    531,
    1069,
    1436,
    1456,
    590,
    59,
    579,
    330,
    /*  1140 */ 1069,
    587,
    1067,
    1068,
    1070,
    1061,
    1407,
    78,
    78,
    76,
    /*  1150 */ 937,
    1066,
    324,
    1065,
    1067,
    77,
    13,
    459,
    577,
    459,
    /*  1160 */ 1065,
    1067,
    536,
    4,
    1123,
    459,
    1258,
    1245,
    1067,
    878,
    /*  1170 */ 245,
    534,
    1064,
    234,
    584,
    1067,
    91,
    93,
    25,
    1456,
    /*  1180 */ 576,
    591,
    444,
    1258,
    1064,
    1481,
    1258,
    1064,
    405,
    403,
    /*  1190 */ 1244,
    1290,
    111,
    1405,
    1067,
    1068,
    534,
    1481,
    472,
    555,
    /*  1200 */ 1481,
    1067,
    1068,
    1070,
    556,
    430,
    563,
    396,
    557,
    430,
    /*  1210 */ 1677,
    590,
    1061,
    1062,
    1061,
    1069,
    1037,
    1581,
    38,
    551,
    /*  1220 */ 561,
    434,
    78,
    78,
    76,
    1281,
    594,
    593,
    1286,
    587,
    /*  1230 */ 77,
    499,
    459,
    577,
    459,
    1065,
    1067,
    878,
    4,
    1407,
    /*  1240 */ 459,
    239,
    523,
    221,
    295,
    1536,
    1534,
    483,
    1064,
    584,
    /*  1250 */ 1067,
    508,
    1037,
    25,
    38,
    576,
    1064,
    440,
    1509,
    1288,
    /*  1260 */ 224,
    1481,
    441,
    1533,
    225,
    587,
    1513,
    1456,
    1581,
    1481,
    /*  1270 */ 313,
    60,
    172,
    174,
    555,
    376,
    1067,
    1068,
    1070,
    554,
    /*  1280 */ 438,
    512,
    403,
    384,
    269,
    1528,
    1406,
    1037,
    254,
    155,
    /*  1290 */ 1069,
    271,
    286,
    526,
    379,
    525,
    270,
    78,
    78,
    1064,
    /*  1300 */ 587,
    1581,
    375,
    1456,
    385,
    77,
    413,
    459,
    577,
    459,
    /*  1310 */ 1065,
    1067,
    1481,
    4,
    1280,
    308,
    404,
    1690,
    248,
    1226,
    /*  1320 */ 327,
    485,
    283,
    479,
    584,
    1067,
    1069,
    1512,
    25,
    356,
    /*  1330 */ 326,
    504,
    334,
    1099,
    461,
    226,
    3,
    590,
    1456,
    1240,
    /*  1340 */ 571,
    569,
    462,
    251,
    1066,
    253,
    1065,
    1067,
    243,
    408,
    /*  1350 */ 76,
    1067,
    1068,
    1070,
    1037,
    960,
    148,
    1153,
    562,
    1064,
    /*  1360 */ 1242,
    1067,
    1654,
    425,
    445,
    1654,
    459,
    587,
    249,
    1302,
    /*  1370 */ 543,
    331,
    1481,
    417,
    416,
    1434,
    333,
    165,
    226,
    1387,
    /*  1380 */ 193,
    576,
    972,
    99,
    404,
    1690,
    31,
    1067,
    1068,
    58,
    /*  1390 */ 34,
    1220,
    1130,
    524,
    1037,
    1433,
    148,
    1129,
    458,
    1389,
    /*  1400 */ 310,
    375,
    247,
    1153,
    961,
    1456,
    1037,
    587,
    38,
    418,
    /*  1410 */ 1639,
    309,
    243,
    96,
    590,
    1037,
    1069,
    38,
    288,
    587,
    /*  1420 */ 218,
    1153,
    879,
    78,
    78,
    474,
    865,
    41,
    587,
    589,
    /*  1430 */ 426,
    77,
    563,
    459,
    577,
    459,
    1065,
    1067,
    226,
    4,
    /*  1440 */ 1653,
    389,
    1064,
    459,
    958,
    1456,
    1037,
    590,
    148,
    170,
    /*  1450 */ 584,
    1067,
    573,
    959,
    25,
    1481,
    6,
    1456,
    576,
    587,
    /*  1460 */ 76,
    579,
    1668,
    388,
    571,
    569,
    1456,
    1153,
    421,
    1037,
    /*  1470 */ 469,
    38,
    563,
    334,
    219,
    324,
    459,
    1067,
    1068,
    1070,
    /*  1480 */ 1225,
    1037,
    587,
    38,
    349,
    1037,
    560,
    148,
    442,
    565,
    /*  1490 */ 879,
    576,
    95,
    1069,
    587,
    178,
    1064,
    1456,
    587,
    480,
    /*  1500 */ 78,
    78,
    295,
    1536,
    564,
    1083,
    427,
    202,
    77,
    1481,
    /*  1510 */ 459,
    577,
    459,
    1065,
    1067,
    1234,
    4,
    1037,
    407,
    148,
    /*  1520 */ 1456,
    468,
    475,
    419,
    319,
    1084,
    1069,
    584,
    1067,
    572,
    /*  1530 */ 587,
    25,
    1456,
    78,
    78,
    1122,
    1456,
    476,
    419,
    1061,
    /*  1540 */ 1037,
    77,
    159,
    459,
    577,
    459,
    1065,
    1067,
    1011,
    4,
    /*  1550 */ 1240,
    1245,
    497,
    587,
    1067,
    1068,
    1070,
    590,
    486,
    419,
    /*  1560 */ 584,
    1067,
    1012,
    563,
    25,
    248,
    1082,
    327,
    1456,
    283,
    /*  1570 */ 76,
    1242,
    405,
    1655,
    1241,
    1511,
    1655,
    326,
    559,
    334,
    /*  1580 */ 1037,
    461,
    148,
    1258,
    487,
    296,
    459,
    1067,
    1068,
    1070,
    /*  1590 */ 561,
    1456,
    303,
    587,
    69,
    450,
    455,
    454,
    571,
    569,
    /*  1600 */ 1258,
    576,
    352,
    1258,
    558,
    1194,
    1061,
    1062,
    1061,
    1198,
    /*  1610 */ 1198,
    490,
    419,
    1269,
    6,
    249,
    1064,
    368,
    331,
    364,
    /*  1620 */ 1667,
    318,
    419,
    333,
    165,
    1064,
    945,
    193,
    998,
    1481,
    /*  1630 */ 99,
    1456,
    1037,
    1196,
    148,
    498,
    1069,
    1599,
    1481,
    1037,
    /*  1640 */ 1195,
    146,
    383,
    78,
    78,
    587,
    1269,
    1064,
    1597,
    247,
    /*  1650 */ 337,
    77,
    587,
    459,
    577,
    459,
    1065,
    1067,
    451,
    4,
    /*  1660 */ 1481,
    382,
    401,
    1037,
    1564,
    124,
    268,
    267,
    266,
    966,
    /*  1670 */ 584,
    1067,
    1064,
    865,
    25,
    248,
    587,
    327,
    363,
    283,
    /*  1680 */ 295,
    1536,
    585,
    1456,
    370,
    1481,
    367,
    326,
    1130,
    334,
    /*  1690 */ 1456,
    49,
    230,
    1129,
    945,
    508,
    998,
    1067,
    1068,
    1070,
    /*  1700 */ 967,
    1375,
    1508,
    1037,
    508,
    148,
    351,
    369,
    579,
    67,
    /*  1710 */ 452,
    1507,
    1077,
    1078,
    1456,
    421,
    587,
    469,
    371,
    1037,
    /*  1720 */ 334,
    38,
    324,
    1392,
    457,
    249,
    1504,
    1225,
    331,
    1037,
    /*  1730 */ 1374,
    40,
    587,
    333,
    165,
    567,
    1037,
    193,
    129,
    1372,
    /*  1740 */ 99,
    566,
    542,
    1278,
    1,
    1282,
    594,
    593,
    1286,
    587,
    /*  1750 */ 1037,
    1544,
    130,
    1373,
    1456,
    1037,
    1037,
    131,
    39,
    247,
    /*  1760 */ 1037,
    294,
    115,
    587,
    295,
    1536,
    1534,
    1479,
    587,
    587,
    /*  1770 */ 1456,
    378,
    1037,
    587,
    116,
    357,
    571,
    569,
    67,
    1350,
    /*  1780 */ 1456,
    320,
    1037,
    1028,
    132,
    587,
    275,
    1456,
    1344,
    1037,
    /*  1790 */ 313,
    133,
    172,
    1199,
    1199,
    587,
    216,
    1071,
    991,
    1197,
    /*  1800 */ 1197,
    1456,
    587,
    1037,
    1578,
    134,
    1456,
    1456,
    254,
    492,
    /*  1810 */ 1158,
    1456,
    275,
    275,
    592,
    179,
    587,
    1580,
    579,
    1314,
    /*  1820 */ 1684,
    494,
    400,
    1456,
    275,
    421,
    1037,
    469,
    135,
    1037,
    /*  1830 */ 334,
    136,
    324,
    1456,
    1037,
    513,
    117,
    1225,
    279,
    587,
    /*  1840 */ 1456,
    574,
    587,
    1301,
    1037,
    373,
    118,
    587,
    67,
    509,
    /*  1850 */ 1037,
    1037,
    119,
    120,
    1456,
    335,
    3,
    587,
    284,
    236,
    /*  1860 */ 571,
    569,
    462,
    587,
    587,
    1071,
    1037,
    314,
    137,
    1037,
    /*  1870 */ 1037,
    138,
    139,
    1037,
    1037,
    140,
    141,
    1456,
    994,
    587,
    /*  1880 */ 1456,
    279,
    587,
    587,
    315,
    1456,
    587,
    587,
    316,
    1539,
    /*  1890 */ 1037,
    1037,
    113,
    121,
    1230,
    1456,
    1037,
    69,
    122,
    185,
    /*  1900 */ 11,
    1456,
    1456,
    587,
    587,
    1229,
    169,
    1422,
    69,
    587,
    /*  1910 */ 1037,
    345,
    37,
    1037,
    1037,
    123,
    142,
    1456,
    348,
    1228,
    /*  1920 */ 1456,
    1456,
    69,
    587,
    1456,
    1456,
    587,
    587,
    298,
    354,
    /*  1930 */ 1037,
    1037,
    153,
    154,
    355,
    1037,
    1037,
    143,
    125,
    304,
    /*  1940 */ 360,
    1456,
    1456,
    587,
    587,
    1450,
    1449,
    1456,
    587,
    587,
    /*  1950 */ 1037,
    876,
    144,
    1037,
    177,
    126,
    1037,
    1037,
    150,
    188,
    /*  1960 */ 411,
    1456,
    496,
    587,
    1456,
    1456,
    587,
    518,
    184,
    587,
    /*  1970 */ 587,
    67,
    1371,
    240,
    222,
    1037,
    381,
    189,
    1037,
    392,
    /*  1980 */ 145,
    1456,
    1456,
    1037,
    223,
    127,
    1456,
    1456,
    587,
    1337,
    /*  1990 */ 1413,
    587,
    1569,
    1570,
    1568,
    1567,
    587,
    578,
    1037,
    1064,
    /*  2000 */ 186,
    1456,
    44,
    1414,
    1456,
    235,
    1208,
    1456,
    1456,
    278,
    /*  2010 */ 1623,
    587,
    1481,
    1621,
    1111,
    250,
    1631,
    428,
    1037,
    1037,
    /*  2020 */ 187,
    161,
    1037,
    1037,
    149,
    151,
    1456,
    45,
    48,
    1456,
    /*  2030 */ 164,
    587,
    587,
    1099,
    1456,
    587,
    587,
    1037,
    166,
    156,
    /*  2040 */ 1037,
    466,
    160,
    1037,
    1037,
    180,
    162,
    1520,
    1519,
    1456,
    /*  2050 */ 587,
    101,
    167,
    587,
    470,
    102,
    587,
    587,
    103,
    1037,
    /*  2060 */ 104,
    157,
    1037,
    1037,
    152,
    158,
    1037,
    105,
    147,
    1456,
    /*  2070 */ 1456,
    232,
    587,
    1456,
    1456,
    587,
    587,
    1423,
    508,
    587,
    /*  2080 */ 1037,
    208,
    128,
    64,
    30,
    366,
    1421,
    561,
    1456,
    344,
    /*  2090 */ 198,
    1456,
    205,
    587,
    1456,
    1456,
    1420,
    478,
    511,
    347,
    /*  2100 */ 256,
    60,
    1637,
    481,
    258,
    1452,
    1451,
    32,
    409,
    488,
    /*  2110 */ 1456,
    220,
    1424,
    1456,
    1456,
    212,
    362,
    1456,
    412,
    261,
    /*  2120 */ 502,
    507,
    365,
    54,
    285,
    262,
    1583,
    1338,
    414,
    527,
    /*  2130 */ 263,
    1456,
    1395,
    1394,
    1393,
    443,
    1382,
    56,
    415,
    1359,
    /*  2140 */ 1365,
    1364,
    951,
    1358,
    380,
    1381,
    1357,
    1356,
    12,
    276,
    /*  2150 */ 529,
    61,
    537,
    390,
    1538,
    387,
    391,
    277,
    1537,
    227,
    /*  2160 */ 10,
    446,
    1312,
    397,
    1675,
    447,
    73,
    449,
    1674,
    1490,
    /*  2170 */ 1491,
    402,
    323,
    321,
    246,
    322,
    581,
    190,
    422,
    1603,
    /*  2180 */ 423,
    1604,
    1689,
    1602,
    1601,
    203,
    173,
    311,
    191,
    237,
    /*  2190 */ 238,
    192,
    43,
    1218,
    229,
    460,
    1216,
    1191,
    1189,
    332,
    /*  2200 */ 1087,
    336,
    168,
    29,
    342,
    204,
    194,
    206,
    252,
    255,
    /*  2210 */ 1123,
    350,
    14,
    257,
    1177,
    353,
    195,
    196,
    433,
    435,
    /*  2220 */ 207,
    209,
    50,
    51,
    52,
    53,
    1182,
    259,
    197,
    260,
    /*  2230 */ 1176,
    181,
    33,
    1167,
    275,
    213,
    506,
    1173,
    107,
    171,
    /*  2240 */ 510,
    1223,
    382,
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
    1264,
    1260,
    7,
    1262,
    1268,
    67,
    /*  2290 */ 22,
    1267,
    68,
    898,
    893,
    892,
    70,
    992,
    23,
    24,
    /*  2300 */ 568,
    912,
    570,
    27,
    28,
    1279,
    1279,
    867,
    1080,
    280,
    /*  2310 */ 75,
    1279,
    26,
    1279,
    71,
    986,
    35,
    1480,
    1279,
    889,
    /*  2320 */ 1279,
    888,
    886,
    1279,
    1279,
    1279,
    36,
    1279,
    1279,
    1279,
    /*  2330 */ 1279,
    1279,
    877,
    281,
    282,
    873,
    1279,
    1279,
    94,
    866,
};
static const YYCODETYPE yy_lookahead[] = {
    /*     0 */ 207, 223, 205, 192, 192, 5,   213, 214, 230, 209,
    /*    10 */ 210, 211, 209, 210, 211, 15,  205, 205, 209, 210,
    /*    20 */ 211, 205, 22,  23,  24,  25,  238, 27,  28,  29,
    /*    30 */ 30,  31,  32,  33,  34,  35,  36,  37,  38,  310,
    /*    40 */ 311, 205, 226, 227, 243, 22,  23,  24,  25,  205,
    /*    50 */ 27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    /*    60 */ 37,  38,  192, 254, 194, 268, 209, 210, 211, 256,
    /*    70 */ 35,  36,  37,  38,  39,  205, 53,  205, 290, 58,
    /*    80 */ 267, 243, 271, 271, 284, 207, 198, 287, 300, 278,
    /*    90 */ 69,  213, 214, 93,  94,  95,  96,  97,  98,  99,
    /*   100 */ 100, 101, 102, 103, 104, 305, 306, 263, 305, 306,
    /*   110 */ 102, 103, 104, 243, 305, 306, 93,  94,  95,  96,
    /*   120 */ 97,  98,  99,  100, 101, 102, 103, 104, 93,  94,
    /*   130 */ 95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    /*   140 */ 304, 118, 22,  23,  24,  25,  205, 27,  28,  29,
    /*   150 */ 30,  31,  32,  33,  34,  35,  36,  37,  38,  141,
    /*   160 */ 142, 140, 305, 306, 22,  23,  24,  25,  147, 27,
    /*   170 */ 28,  29,  30,  31,  32,  33,  34,  35,  36,  37,
    /*   180 */ 38,  303, 161, 192, 221, 194, 223, 246, 225, 317,
    /*   190 */ 320, 284, 322, 51,  287, 75,  205, 77,  93,  94,
    /*   200 */ 95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    /*   210 */ 58,  323, 324, 93,  94,  95,  96,  97,  98,  99,
    /*   220 */ 100, 101, 102, 103, 104, 97,  98,  99,  100, 101,
    /*   230 */ 102, 103, 104, 205, 243, 93,  94,  95,  96,  97,
    /*   240 */ 98,  99,  100, 101, 102, 103, 104, 22,  23,  24,
    /*   250 */ 25,  247, 27,  28,  29,  30,  31,  32,  33,  34,
    /*   260 */ 35,  36,  37,  38,  99,  100, 101, 102, 103, 104,
    /*   270 */ 266, 40,  22,  23,  24,  25,  104, 27,  28,  29,
    /*   280 */ 30,  31,  32,  33,  34,  35,  36,  37,  38,  40,
    /*   290 */ 205, 263, 140, 141, 142, 22,  23,  24,  25,  205,
    /*   300 */ 27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    /*   310 */ 37,  38,  321, 322, 27,  28,  29,  30,  93,  94,
    /*   320 */ 95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    /*   330 */ 99,  168, 169, 248, 86,  106, 251, 252, 107, 108,
    /*   340 */ 109, 18,  117, 93,  94,  95,  96,  97,  98,  99,
    /*   350 */ 100, 101, 102, 103, 104, 5,   107, 108, 109, 140,
    /*   360 */ 141, 142, 209, 210, 211, 15,  93,  94,  95,  96,
    /*   370 */ 97,  98,  99,  100, 101, 102, 103, 104, 149, 97,
    /*   380 */ 98,  205, 133, 289, 135, 136, 137, 205, 115, 209,
    /*   390 */ 210, 211, 110, 111, 145, 22,  23,  24,  25,  149,
    /*   400 */ 27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    /*   410 */ 37,  38,  125, 205, 243, 40,  168, 169, 22,  23,
    /*   420 */ 24,  25,  192, 27,  28,  29,  30,  31,  32,  33,
    /*   430 */ 34,  35,  36,  37,  38,  205, 209, 210, 211, 263,
    /*   440 */ 205, 22,  23,  24,  25,  70,  27,  28,  29,  30,
    /*   450 */ 31,  32,  33,  34,  35,  36,  37,  38,  305, 306,
    /*   460 */ 209, 210, 211, 204, 141, 142, 93,  94,  95,  96,
    /*   470 */ 97,  98,  99,  100, 101, 102, 103, 104, 118, 297,
    /*   480 */ 298, 299, 107, 108, 109, 305, 306, 127, 115, 93,
    /*   490 */ 94,  95,  96,  97,  98,  99,  100, 101, 102, 103,
    /*   500 */ 104, 271, 24,  201, 205, 297, 298, 299, 278, 279,
    /*   510 */ 40,  115, 93,  94,  95,  96,  97,  98,  99,  100,
    /*   520 */ 101, 102, 103, 104, 14,  205, 56,  209, 210, 19,
    /*   530 */ 209, 210, 305, 306, 115, 22,  23,  24,  25,  192,
    /*   540 */ 27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    /*   550 */ 37,  38,  205, 205, 295, 45,  305, 306, 22,  23,
    /*   560 */ 24,  25,  263, 27,  28,  29,  30,  31,  32,  33,
    /*   570 */ 34,  35,  36,  37,  38,  40,  118, 107, 108, 109,
    /*   580 */ 260, 22,  23,  24,  25,  107, 27,  28,  29,  30,
    /*   590 */ 31,  32,  33,  34,  35,  36,  37,  38,  40,  297,
    /*   600 */ 298, 40,  191, 232, 193, 147, 93,  94,  95,  96,
    /*   610 */ 97,  98,  99,  100, 101, 102, 103, 104, 271, 161,
    /*   620 */ 209, 210, 211, 305, 306, 40,  305, 306, 115, 93,
    /*   630 */ 94,  95,  96,  97,  98,  99,  100, 101, 102, 103,
    /*   640 */ 104, 56,  107, 108, 109, 205, 235, 201, 237, 205,
    /*   650 */ 205, 115, 93,  94,  95,  96,  97,  98,  99,  100,
    /*   660 */ 101, 102, 103, 104, 253, 107, 108, 109, 107, 108,
    /*   670 */ 109, 113, 40,  205, 115, 22,  23,  24,  25,  198,
    /*   680 */ 27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    /*   690 */ 37,  38,  107, 108, 109, 65,  313, 67,  22,  23,
    /*   700 */ 24,  25,  319, 27,  28,  29,  30,  31,  32,  33,
    /*   710 */ 34,  35,  36,  37,  38,  3,   305, 306, 307, 7,
    /*   720 */ 267, 22,  23,  24,  25,  164, 27,  28,  29,  30,
    /*   730 */ 31,  32,  33,  34,  35,  36,  37,  38,  285, 107,
    /*   740 */ 108, 109, 40,  297, 298, 113, 93,  94,  95,  96,
    /*   750 */ 97,  98,  99,  100, 101, 102, 103, 104, 313, 47,
    /*   760 */ 316, 317, 132, 318, 319, 247, 61,  205, 115, 93,
    /*   770 */ 94,  95,  96,  97,  98,  99,  100, 101, 102, 103,
    /*   780 */ 104, 76,  205, 78,  266, 155, 81,  40,  120, 205,
    /*   790 */ 122, 123, 93,  94,  95,  96,  97,  98,  99,  100,
    /*   800 */ 101, 102, 103, 104, 323, 324, 130, 313, 246, 107,
    /*   810 */ 108, 109, 40,  319, 115, 22,  23,  24,  25,  114,
    /*   820 */ 27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    /*   830 */ 37,  38,  248, 205, 40,  251, 252, 22,  23,  24,
    /*   840 */ 25,  61,  27,  28,  29,  30,  31,  32,  33,  34,
    /*   850 */ 35,  36,  37,  38,  107, 108, 109, 129, 78,  131,
    /*   860 */ 113, 81,  22,  23,  24,  25,  164, 27,  28,  29,
    /*   870 */ 30,  31,  32,  33,  34,  35,  36,  37,  38,  107,
    /*   880 */ 108, 109, 205, 205, 1,   113, 93,  94,  95,  96,
    /*   890 */ 97,  98,  99,  100, 101, 102, 103, 104, 118, 199,
    /*   900 */ 200, 107, 108, 109, 21,  227, 147, 113, 93,  94,
    /*   910 */ 95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    /*   920 */ 161, 204, 69,  130, 207, 248, 40,  44,  251, 252,
    /*   930 */ 213, 214, 117, 93,  94,  95,  96,  97,  98,  99,
    /*   940 */ 100, 101, 102, 103, 104, 62,  22,  23,  24,  25,
    /*   950 */ 205, 27,  28,  29,  30,  31,  32,  33,  34,  35,
    /*   960 */ 36,  37,  38,  23,  24,  25,  27,  27,  28,  29,
    /*   970 */ 30,  31,  32,  33,  34,  35,  36,  37,  38,  24,
    /*   980 */ 25,  205, 27,  28,  29,  30,  31,  32,  33,  34,
    /*   990 */ 35,  36,  37,  38,  108, 118, 192, 205, 194, 113,
    /*  1000 */ 147, 197, 11,  117, 35,  36,  37,  38,  40,  205,
    /*  1010 */ 40,  205, 205, 268, 161, 24,  139, 93,  94,  95,
    /*  1020 */ 96,  97,  98,  99,  100, 101, 102, 103, 104, 69,
    /*  1030 */ 24,  40,  42,  93,  94,  95,  96,  97,  98,  99,
    /*  1040 */ 100, 101, 102, 103, 104, 106, 55,  243, 93,  94,
    /*  1050 */ 95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    /*  1060 */ 69,  11,  93,  94,  95,  96,  97,  98,  99,  100,
    /*  1070 */ 101, 102, 103, 104, 24,  107, 108, 109, 108, 29,
    /*  1080 */ 192, 90,  194, 40,  192, 207, 194, 117, 97,  98,
    /*  1090 */ 40,  213, 214, 205, 134, 289, 105, 205, 107, 108,
    /*  1100 */ 109, 110, 111, 113, 113, 55,  133, 147, 135, 136,
    /*  1110 */ 137, 313, 106, 107, 146, 124, 125, 319, 145, 128,
    /*  1120 */ 313, 161, 244, 256, 74,  318, 319, 205, 192, 79,
    /*  1130 */ 194, 243, 196, 90,  267, 243, 11,  106, 147, 205,
    /*  1140 */ 90,  205, 151, 152, 153, 40,  205, 97,  98,  24,
    /*  1150 */ 107, 108, 161, 110, 111, 105, 204, 107, 108, 109,
    /*  1160 */ 110, 111, 270, 113, 118, 40,  61,  92,  125, 40,
    /*  1170 */ 207, 165, 192, 127, 124, 125, 213, 214, 128, 243,
    /*  1180 */ 55,  76,  24,  78,  192, 205, 81,  192, 113, 248,
    /*  1190 */ 115, 198, 251, 252, 151, 152, 165, 205, 254, 74,
    /*  1200 */ 205, 151, 152, 153, 79,  205, 270, 244, 320, 205,
    /*  1210 */ 322, 11,  107, 108, 109, 90,  192, 205, 194, 114,
    /*  1220 */ 114, 197, 97,  98,  24,  190, 191, 192, 193, 205,
    /*  1230 */ 105, 205, 107, 108, 109, 110, 111, 108, 113, 205,
    /*  1240 */ 40,  118, 84,  291, 209, 210, 211, 295, 192, 124,
    /*  1250 */ 125, 271, 192, 128, 194, 55,  192, 197, 278, 198,
    /*  1260 */ 260, 205, 104, 271, 260, 205, 271, 243, 205, 205,
    /*  1270 */ 235, 148, 237, 167, 74,  117, 151, 152, 153, 79,
    /*  1280 */ 268, 24,  248, 125, 27,  205, 252, 192, 253, 194,
    /*  1290 */ 90,  133, 134, 135, 136, 137, 138, 97,  98,  192,
    /*  1300 */ 205, 205, 144, 243, 240, 105, 242, 107, 108, 109,
    /*  1310 */ 110, 111, 205, 113, 0,   289, 323, 324, 4,   64,
    /*  1320 */ 6,   142, 8,   68,  124, 125, 90,  271, 128, 150,
    /*  1330 */ 16,  268, 18,  41,  20,  271, 301, 11,  243, 92,
    /*  1340 */ 305, 306, 307, 117, 108, 119, 110, 111, 109, 242,
    /*  1350 */ 24,  151, 152, 153, 192, 10,  194, 118, 196, 192,
    /*  1360 */ 113, 125, 115, 106, 268, 118, 40,  205, 54,  210,
    /*  1370 */ 99,  57,  205, 97,  98,  205, 62,  63,  271, 115,
    /*  1380 */ 66,  55,  118, 69,  323, 324, 113, 151, 152, 113,
    /*  1390 */ 117, 136, 121, 48,  192, 205, 194, 126, 196, 232,
    /*  1400 */ 233, 144, 88,  164, 59,  243, 192, 205, 194, 242,
    /*  1410 */ 155, 197, 109, 34,  11,  192, 90,  194, 115, 205,
    /*  1420 */ 197, 118, 40,  97,  98,  205, 112, 24,  205, 199,
    /*  1430 */ 200, 105, 270, 107, 108, 109, 110, 111, 271, 113,
    /*  1440 */ 205, 261, 192, 40,  125, 243, 192, 11,  194, 157,
    /*  1450 */ 124, 125, 126, 134, 128, 205, 313, 243, 55,  205,
    /*  1460 */ 24,  147, 319, 283, 305, 306, 243, 164, 154, 192,
    /*  1470 */ 156, 194, 270, 159, 197, 161, 40,  151, 152, 153,
    /*  1480 */ 166, 192, 205, 194, 205, 192, 197, 194, 143, 196,
    /*  1490 */ 108, 55,  113, 90,  205, 113, 192, 243, 205, 205,
    /*  1500 */ 97,  98,  209, 210, 211, 1,   308, 309, 105, 205,
    /*  1510 */ 107, 108, 109, 110, 111, 115, 113, 192, 118, 194,
    /*  1520 */ 243, 271, 202, 203, 270, 21,  90,  124, 125, 126,
    /*  1530 */ 205, 128, 243, 97,  98,  99,  243, 202, 203, 40,
    /*  1540 */ 192, 105, 194, 107, 108, 109, 110, 111, 44,  113,
    /*  1550 */ 92,  92,  24,  205, 151, 152, 153, 11,  202, 203,
    /*  1560 */ 124, 125, 58,  270, 128, 4,   62,  6,   243, 8,
    /*  1570 */ 24,  113, 113, 115, 115, 271, 118, 16,  48,  18,
    /*  1580 */ 192, 20,  194, 61,  142, 89,  40,  151, 152, 153,
    /*  1590 */ 114, 243, 150, 205, 118, 270, 97,  98,  305, 306,
    /*  1600 */ 78,  55,  205, 81,  74,  106, 107, 108, 109, 110,
    /*  1610 */ 111, 202, 203, 83,  313, 54,  192, 65,  57,  67,
    /*  1620 */ 319, 202, 203, 62,  63,  192, 40,  66,  40,  205,
    /*  1630 */ 69,  243, 192, 134, 194, 107, 90,  205, 205, 192,
    /*  1640 */ 141, 194, 125, 97,  98,  205, 116, 192, 205, 88,
    /*  1650 */ 154, 105, 205, 107, 108, 109, 110, 111, 270, 113,
    /*  1660 */ 205, 144, 215, 192, 286, 194, 140, 141, 142, 14,
    /*  1670 */ 124, 125, 192, 112, 128, 4,   205, 6,   205, 8,
    /*  1680 */ 209, 210, 211, 243, 132, 205, 205, 16,  121, 18,
    /*  1690 */ 243, 148, 149, 126, 108, 271, 108, 151, 152, 153,
    /*  1700 */ 45,  222, 278, 192, 271, 194, 115, 205, 147, 118,
    /*  1710 */ 270, 278, 72,  73,  243, 154, 205, 156, 205, 192,
    /*  1720 */ 159, 194, 161, 205, 197, 54,  271, 166, 57,  192,
    /*  1730 */ 222, 194, 205, 62,  63,  24,  192, 66,  194, 205,
    /*  1740 */ 69,  270, 205, 188, 189, 190, 191, 192, 193, 205,
    /*  1750 */ 192, 271, 194, 222, 243, 192, 192, 194, 194, 88,
    /*  1760 */ 192, 113, 194, 205, 209, 210, 211, 119, 205, 205,
    /*  1770 */ 243, 205, 192, 205, 194, 115, 305, 306, 118, 205,
    /*  1780 */ 243, 270, 192, 115, 194, 205, 118, 243, 205, 192,
    /*  1790 */ 235, 194, 237, 110, 111, 205, 280, 40,  87,  110,
    /*  1800 */ 111, 243, 205, 192, 205, 194, 243, 243, 253, 115,
    /*  1810 */ 115, 243, 118, 118, 114, 115, 205, 205, 147, 205,
    /*  1820 */ 315, 115, 205, 243, 118, 154, 192, 156, 194, 192,
    /*  1830 */ 159, 194, 161, 243, 192, 115, 194, 166, 118, 205,
    /*  1840 */ 243, 266, 205, 205, 192, 115, 194, 205, 118, 292,
    /*  1850 */ 192, 192, 194, 194, 243, 274, 301, 205, 288, 218,
    /*  1860 */ 305, 306, 307, 205, 205, 108, 192, 280, 194, 192,
    /*  1870 */ 192, 194, 194, 192, 192, 194, 194, 243, 115, 205,
    /*  1880 */ 243, 118, 205, 205, 280, 243, 205, 205, 280, 280,
    /*  1890 */ 192, 192, 194, 194, 115, 243, 192, 118, 194, 206,
    /*  1900 */ 195, 243, 243, 205, 205, 115, 277, 258, 118, 205,
    /*  1910 */ 192, 257, 194, 192, 192, 194, 194, 243, 257, 115,
    /*  1920 */ 243, 243, 118, 205, 243, 243, 205, 205, 264, 296,
    /*  1930 */ 192, 192, 194, 194, 269, 192, 192, 194, 194, 269,
    /*  1940 */ 264, 243, 243, 205, 205, 258, 258, 243, 205, 205,
    /*  1950 */ 192, 115, 194, 192, 118, 194, 192, 192, 194, 194,
    /*  1960 */ 258, 243, 296, 205, 243, 243, 205, 219, 115, 205,
    /*  1970 */ 205, 118, 221, 228, 261, 192, 244, 194, 192, 264,
    /*  1980 */ 194, 243, 243, 192, 261, 194, 243, 243, 205, 236,
    /*  1990 */ 244, 205, 285, 285, 285, 285, 205, 217, 192, 192,
    /*  2000 */ 194, 243, 146, 244, 243, 195, 13,  243, 243, 119,
    /*  2010 */ 208, 205, 205, 208, 63,  160, 304, 208, 192, 192,
    /*  2020 */ 194, 194, 192, 192, 194, 194, 243, 302, 302, 243,
    /*  2030 */ 276, 205, 205, 41,  243, 205, 205, 192, 276, 194,
    /*  2040 */ 192, 275, 194, 192, 192, 194, 194, 275, 275, 243,
    /*  2050 */ 205, 277, 277, 205, 91,  273, 205, 205, 273, 192,
    /*  2060 */ 273, 194, 192, 192, 194, 194, 192, 273, 194, 243,
    /*  2070 */ 243, 149, 205, 243, 243, 205, 205, 259, 271, 205,
    /*  2080 */ 192, 113, 194, 163, 265, 278, 262, 114, 243, 261,
    /*  2090 */ 22,  243, 255, 205, 243, 243, 262, 208, 91,  261,
    /*  2100 */ 239, 148, 269, 208, 239, 259, 259, 265, 269, 269,
    /*  2110 */ 243, 113, 255, 243, 243, 255, 208, 243, 269, 239,
    /*  2120 */ 245, 43,  293, 129, 208, 239, 294, 208, 245, 106,
    /*  2130 */ 239, 243, 229, 229, 229, 46,  224, 113, 245, 229,
    /*  2140 */ 234, 234, 139, 219, 229, 224, 229, 229, 118, 208,
    /*  2150 */ 241, 162, 116, 281, 262, 261, 269, 80,  262, 282,
    /*  2160 */ 113, 71,  212, 208, 314, 104, 129, 117, 314, 272,
    /*  2170 */ 272, 245, 216, 250, 168, 250, 249, 231, 312, 204,
    /*  2180 */ 312, 204, 324, 204, 204, 309, 220, 220, 231, 218,
    /*  2190 */ 218, 231, 204, 49,  113, 50,  112, 115, 115, 157,
    /*  2200 */ 124, 158, 157, 132, 147, 146, 143, 127, 117, 165,
    /*  2210 */ 118, 132, 113, 106, 112, 155, 143, 143, 42,  12,
    /*  2220 */ 127, 146, 34,  34,  34,  34,  107, 9,   143, 119,
    /*  2230 */ 112, 8,   117, 52,  118, 52,  17,  60,  106, 119,
    /*  2240 */ 24,  124, 144, 138, 113, 51,  113, 113, 51,  113,
    /*  2250 */ 115, 40,  85,  2,   117, 113, 51,  12,  118, 107,
    /*  2260 */ 164, 115, 115, 115, 9,   9,   113, 118, 113, 115,
    /*  2270 */ 119, 9,   114, 117, 115, 118, 113, 116, 148, 113,
    /*  2280 */ 115, 9,   9,   9,   60,  77,  23,  75,  60,  118,
    /*  2290 */ 9,   82,  118, 115, 115, 115, 127, 87,  113, 113,
    /*  2300 */ 118, 18,  118, 9,   9,   325, 325, 112, 115, 113,
    /*  2310 */ 118, 325, 113, 325, 127, 115, 113, 119, 325, 115,
    /*  2320 */ 325, 115, 121, 325, 325, 325, 113, 325, 325, 325,
    /*  2330 */ 325, 325, 115, 119, 119, 115, 325, 325, 113, 112,
    /*  2340 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2350 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2360 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2370 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2380 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2390 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2400 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2410 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2420 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2430 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2440 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2450 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2460 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2470 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2480 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2490 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2500 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2510 */ 325, 325, 325, 325, 325, 325, 325, 325, 325, 325,
    /*  2520 */ 325, 325, 325, 325, 325, 325, 325, 325,
};
#define YY_SHIFT_COUNT (594)
#define YY_SHIFT_MIN (0)
#define YY_SHIFT_MAX (2295)
static const unsigned short int yy_shift_ofst[] = {
    /*     0 */ 1561,
    1314,
    991,
    1671,
    991,
    853,
    1050,
    1125,
    1200,
    1546,
    /*    10 */ 1546,
    1546,
    249,
    21,
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
    535,
    /*    30 */ 960,
    535,
    853,
    853,
    853,
    853,
    853,
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
    1499,
    1499,
    1105,
    1105,
    470,
    /*   100 */ 585,
    535,
    535,
    535,
    535,
    535,
    535,
    535,
    535,
    458,
    /*   110 */ 458,
    458,
    458,
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
    535,
    535,
    535,
    535,
    535,
    535,
    /*   170 */ 535,
    535,
    1255,
    1257,
    535,
    535,
    535,
    282,
    282,
    248,
    /*   180 */ 8,
    18,
    163,
    163,
    163,
    759,
    172,
    172,
    2340,
    2340,
    /*   190 */ 1158,
    1158,
    1158,
    968,
    558,
    558,
    558,
    558,
    1504,
    1504,
    /*   200 */ 375,
    705,
    1247,
    1458,
    535,
    535,
    535,
    535,
    535,
    535,
    /*   210 */ 535,
    535,
    535,
    535,
    535,
    535,
    1006,
    535,
    780,
    780,
    /*   220 */ 535,
    323,
    1522,
    1522,
    478,
    478,
    1129,
    1106,
    1129,
    2340,
    /*   230 */ 2340,
    2340,
    2340,
    2340,
    2340,
    2340,
    1043,
    1236,
    1236,
    632,
    /*   240 */ 973,
    747,
    561,
    702,
    231,
    772,
    794,
    535,
    535,
    535,
    /*   250 */ 535,
    535,
    535,
    535,
    152,
    535,
    535,
    535,
    535,
    535,
    /*   260 */ 535,
    535,
    535,
    535,
    535,
    535,
    1345,
    1345,
    1345,
    535,
    /*   270 */ 535,
    535,
    535,
    1303,
    535,
    535,
    886,
    1530,
    535,
    535,
    /*   280 */ 883,
    535,
    535,
    510,
    219,
    712,
    1276,
    970,
    970,
    970,
    /*   290 */ 1239,
    970,
    668,
    668,
    1271,
    668,
    1292,
    229,
    1543,
    360,
    /*   300 */ 1543,
    1528,
    1123,
    229,
    229,
    1123,
    229,
    360,
    1528,
    877,
    /*   310 */ 1264,
    939,
    350,
    1273,
    1031,
    1031,
    1031,
    1031,
    1046,
    1476,
    /*   320 */ 1476,
    350,
    350,
    728,
    1567,
    1856,
    1993,
    1993,
    1890,
    1890,
    /*   330 */ 1890,
    1951,
    1951,
    1855,
    1855,
    1855,
    1992,
    1992,
    1963,
    1963,
    /*   340 */ 1963,
    1963,
    1922,
    1968,
    1920,
    1973,
    2068,
    1920,
    1973,
    1890,
    /*   350 */ 2007,
    1953,
    1890,
    2007,
    1953,
    1922,
    1922,
    1953,
    1968,
    2068,
    /*   360 */ 1953,
    2068,
    1998,
    1890,
    2007,
    1994,
    2078,
    1890,
    2007,
    1890,
    /*   370 */ 2007,
    1998,
    2023,
    2023,
    2023,
    2089,
    2024,
    2024,
    1998,
    2023,
    /*   380 */ 2003,
    2023,
    2089,
    2023,
    2023,
    2030,
    1890,
    1920,
    1973,
    1920,
    /*   390 */ 1989,
    2036,
    1953,
    2077,
    2077,
    2090,
    2090,
    2047,
    1890,
    2061,
    /*   400 */ 2061,
    2037,
    2050,
    1998,
    2006,
    2340,
    2340,
    2340,
    2340,
    2340,
    /*   410 */ 2340,
    2340,
    2340,
    2340,
    2340,
    2340,
    2340,
    2340,
    2340,
    2340,
    /*   420 */ 287,
    630,
    1075,
    1459,
    1552,
    1526,
    1382,
    1400,
    1379,
    1496,
    /*   430 */ 1226,
    1591,
    1179,
    1442,
    1660,
    990,
    1668,
    1694,
    1706,
    1720,
    /*   440 */ 1730,
    1586,
    1319,
    1655,
    1517,
    1695,
    1640,
    1588,
    1763,
    1711,
    /*   450 */ 1779,
    1790,
    1804,
    1757,
    1683,
    1689,
    1836,
    1853,
    1700,
    1648,
    /*   460 */ 2144,
    2145,
    2081,
    2084,
    2082,
    2083,
    2042,
    2043,
    2045,
    2071,
    /*   470 */ 2076,
    2057,
    2059,
    2063,
    2091,
    2092,
    2092,
    2080,
    2044,
    2079,
    /*   480 */ 2099,
    2107,
    2060,
    2102,
    2093,
    2073,
    2092,
    2074,
    2176,
    2207,
    /*   490 */ 2092,
    2075,
    2188,
    2189,
    2190,
    2191,
    2085,
    2119,
    2218,
    2110,
    /*   500 */ 2118,
    2223,
    2115,
    2181,
    2116,
    2183,
    2177,
    2219,
    2120,
    2132,
    /*   510 */ 2117,
    2216,
    2098,
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
    2208,
    2224,
    2212,
    2263,
    2228,
    2209,
    /*   560 */ 2171,
    2281,
    2178,
    2149,
    2179,
    2180,
    2174,
    2210,
    2185,
    2182,
    /*   570 */ 2186,
    2184,
    2169,
    2187,
    2192,
    2193,
    2196,
    2198,
    2283,
    2199,
    /*   580 */ 2200,
    2203,
    2201,
    2204,
    2213,
    2206,
    2214,
    2215,
    2217,
    2220,
    /*   590 */ 2225,
    2294,
    2295,
    2195,
    2227,
};
#define YY_REDUCE_COUNT (419)
#define YY_REDUCE_MIN (-271)
#define YY_REDUCE_MAX (1988)
static const short yy_reduce_ofst[] = {
    /*     0 */ 1555,
    1035,
    1293,
    411,
    1471,
    -200,
    -130,
    -9,
    888,
    936,
    /*    10 */ 1162,
    1202,
    1167,
    -191,
    804,
    1024,
    1060,
    1214,
    892,
    1223,
    /*    20 */ 1277,
    1254,
    1289,
    1325,
    1388,
    1447,
    1440,
    1511,
    1527,
    230,
    /*    30 */ -197,
    1064,
    -143,
    153,
    180,
    227,
    251,
    878,
    963,
    -122,
    /*    40 */ 717,
    1095,
    1348,
    1537,
    1544,
    1558,
    1563,
    1564,
    1568,
    1580,
    /*    50 */ 1590,
    1597,
    1611,
    1634,
    1637,
    1642,
    1652,
    1658,
    1659,
    1674,
    /*    60 */ 1677,
    1678,
    1681,
    1682,
    1698,
    1699,
    1704,
    1718,
    1721,
    1722,
    /*    70 */ 1738,
    1739,
    1743,
    1744,
    1758,
    1761,
    1764,
    1765,
    1783,
    1786,
    /*    80 */ 1791,
    1806,
    1826,
    1827,
    1830,
    1831,
    1845,
    1848,
    1851,
    1852,
    /*    90 */ 1867,
    1870,
    1871,
    1874,
    1888,
    182,
    208,
    445,
    807,
    85,
    /*   100 */ 584,
    1107,
    -189,
    980,
    1424,
    1433,
    677,
    1807,
    941,
    318,
    /*   110 */ 321,
    318,
    321,
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
    -184,
    -188,
    347,
    992,
    995,
    1056,
    1250,
    /*   170 */ 1304,
    1455,
    -212,
    -37,
    444,
    1480,
    1034,
    302,
    446,
    -112,
    /*   180 */ -207,
    952,
    481,
    993,
    1061,
    1159,
    -207,
    -207,
    -207,
    -207,
    /*   190 */ -222,
    -222,
    -222,
    -164,
    -156,
    28,
    176,
    299,
    4,
    518,
    /*   200 */ 678,
    383,
    -271,
    -271,
    320,
    1000,
    1004,
    94,
    -203,
    806,
    /*   210 */ 745,
    1012,
    1026,
    1063,
    -59,
    1096,
    453,
    -128,
    494,
    798,
    /*   220 */ 562,
    259,
    1143,
    1301,
    -187,
    867,
    700,
    1180,
    1230,
    1198,
    /*   230 */ 1320,
    1335,
    1356,
    1409,
    -93,
    1419,
    -199,
    -162,
    171,
    235,
    /*   240 */ 371,
    348,
    440,
    468,
    577,
    628,
    776,
    792,
    922,
    934,
    /*   250 */ 1080,
    1170,
    1190,
    1220,
    944,
    1235,
    1279,
    1294,
    1397,
    1432,
    /*   260 */ 1443,
    1473,
    1481,
    1502,
    1513,
    1518,
    1479,
    1508,
    1531,
    1534,
    /*   270 */ 1566,
    1574,
    1583,
    1378,
    1599,
    1612,
    1516,
    1505,
    1614,
    1617,
    /*   280 */ 1575,
    1638,
    577,
    1581,
    1557,
    1570,
    1641,
    1587,
    1604,
    1608,
    /*   290 */ 1378,
    1609,
    1693,
    1693,
    1705,
    1693,
    1629,
    1649,
    1654,
    1664,
    /*   300 */ 1661,
    1633,
    1665,
    1687,
    1688,
    1670,
    1702,
    1676,
    1666,
    1748,
    /*   310 */ 1745,
    1751,
    1732,
    1753,
    1707,
    1708,
    1709,
    1710,
    1715,
    1713,
    /*   320 */ 1723,
    1746,
    1759,
    1780,
    1810,
    1712,
    1725,
    1726,
    1802,
    1805,
    /*   330 */ 1809,
    1754,
    1762,
    1766,
    1772,
    1773,
    1774,
    1775,
    1782,
    1785,
    /*   340 */ 1787,
    1794,
    1818,
    1819,
    1824,
    1828,
    1837,
    1834,
    1838,
    1889,
    /*   350 */ 1861,
    1833,
    1895,
    1865,
    1839,
    1846,
    1847,
    1840,
    1842,
    1857,
    /*   360 */ 1849,
    1860,
    1875,
    1908,
    1880,
    1832,
    1829,
    1916,
    1886,
    1919,
    /*   370 */ 1891,
    1883,
    1903,
    1904,
    1905,
    1912,
    1906,
    1907,
    1893,
    1910,
    /*   380 */ 1924,
    1915,
    1921,
    1917,
    1918,
    1909,
    1941,
    1892,
    1894,
    1896,
    /*   390 */ 1877,
    1872,
    1887,
    1850,
    1854,
    1897,
    1898,
    1950,
    1955,
    1923,
    /*   400 */ 1925,
    1956,
    1927,
    1926,
    1858,
    1866,
    1868,
    1876,
    1946,
    1975,
    /*   410 */ 1977,
    1979,
    1980,
    1957,
    1966,
    1967,
    1971,
    1972,
    1960,
    1988,
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
    /*  188 */ "input",
    /*  189 */ "cmdlist",
    /*  190 */ "ecmd",
    /*  191 */ "cmdx",
    /*  192 */ "error",
    /*  193 */ "cmd",
    /*  194 */ "expr",
    /*  195 */ "distinct",
    /*  196 */ "exprlist",
    /*  197 */ "sortlist",
    /*  198 */ "filter_over",
    /*  199 */ "typetoken",
    /*  200 */ "typename",
    /*  201 */ "signed",
    /*  202 */ "selcollist",
    /*  203 */ "sclp",
    /*  204 */ "scanpt",
    /*  205 */ "nm",
    /*  206 */ "multiselect_op",
    /*  207 */ "in_op",
    /*  208 */ "dbnm",
    /*  209 */ "selectnowith",
    /*  210 */ "oneselect",
    /*  211 */ "select",
    /*  212 */ "paren_exprlist",
    /*  213 */ "likeop",
    /*  214 */ "between_op",
    /*  215 */ "case_operand",
    /*  216 */ "case_exprlist",
    /*  217 */ "case_else",
    /*  218 */ "scantok",
    /*  219 */ "autoinc",
    /*  220 */ "refargs",
    /*  221 */ "refarg",
    /*  222 */ "refact",
    /*  223 */ "defer_subclause",
    /*  224 */ "init_deferred_pred_opt",
    /*  225 */ "defer_subclause_opt",
    /*  226 */ "table_option_set",
    /*  227 */ "table_option",
    /*  228 */ "tconscomma",
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
    /*  246 */ "eidlist",
    /*  247 */ "resolvetype",
    /*  248 */ "withnm",
    /*  249 */ "wqas",
    /*  250 */ "collate",
    /*  251 */ "wqlist",
    /*  252 */ "wqitem",
    /*  253 */ "with",
    /*  254 */ "insert_cmd",
    /*  255 */ "orconf",
    /*  256 */ "indexed_opt",
    /*  257 */ "where_opt_ret",
    /*  258 */ "upsert",
    /*  259 */ "returning",
    /*  260 */ "xfullname",
    /*  261 */ "orderby_opt",
    /*  262 */ "limit_opt",
    /*  263 */ "setlist",
    /*  264 */ "from",
    /*  265 */ "idlist_opt",
    /*  266 */ "raisetype",
    /*  267 */ "indexed_by",
    /*  268 */ "idlist",
    /*  269 */ "where_opt",
    /*  270 */ "nexprlist",
    /*  271 */ "nmorerr",
    /*  272 */ "nulls",
    /*  273 */ "ifexists",
    /*  274 */ "transtype",
    /*  275 */ "trans_opt",
    /*  276 */ "savepoint_opt",
    /*  277 */ "kwcolumn_opt",
    /*  278 */ "fullname",
    /*  279 */ "add_column_fullname",
    /*  280 */ "as",
    /*  281 */ "groupby_opt",
    /*  282 */ "having_opt",
    /*  283 */ "window_clause",
    /*  284 */ "seltablist",
    /*  285 */ "on_using",
    /*  286 */ "joinop",
    /*  287 */ "stl_prefix",
    /*  288 */ "trigger_time",
    /*  289 */ "trnm",
    /*  290 */ "trigger_decl",
    /*  291 */ "trigger_cmd_list",
    /*  292 */ "trigger_event",
    /*  293 */ "foreach_clause",
    /*  294 */ "when_clause",
    /*  295 */ "trigger_cmd",
    /*  296 */ "tridxby",
    /*  297 */ "plus_num",
    /*  298 */ "minus_num",
    /*  299 */ "nmnum",
    /*  300 */ "uniqueflag",
    /*  301 */ "explain",
    /*  302 */ "database_kw_opt",
    /*  303 */ "key_opt",
    /*  304 */ "vinto",
    /*  305 */ "values",
    /*  306 */ "mvalues",
    /*  307 */ "create_vtab",
    /*  308 */ "vtabarglist",
    /*  309 */ "vtabarg",
    /*  310 */ "vtabargtoken",
    /*  311 */ "lp",
    /*  312 */ "anylist",
    /*  313 */ "range_or_rows",
    /*  314 */ "frame_exclude_opt",
    /*  315 */ "frame_exclude",
    /*  316 */ "windowdefn_list",
    /*  317 */ "windowdefn",
    /*  318 */ "window",
    /*  319 */ "frame_opt",
    /*  320 */ "frame_bound_s",
    /*  321 */ "frame_bound_e",
    /*  322 */ "frame_bound",
    /*  323 */ "filter_clause",
    /*  324 */ "over_clause",
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
  yyTraceShift(yypParser, yyNewState, "Shift");
}

/* For rule J, yyRuleInfoLhs[J] contains the symbol on the left-hand side
** of that rule */
static const YYCODETYPE yyRuleInfoLhs[] = {
    188, /* (0) input ::= cmdlist */
    189, /* (1) cmdlist ::= cmdlist ecmd */
    189, /* (2) cmdlist ::= ecmd */
    190, /* (3) ecmd ::= SEMI */
    190, /* (4) ecmd ::= cmdx SEMI */
    190, /* (5) ecmd ::= error SEMI */
    191, /* (6) cmdx ::= cmd */
    194, /* (7) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY
            sortlist RP */
    194, /* (8) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY
            sortlist RP filter_over */
    194, /* (9) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP WITHIN GROUP
            LP ORDER BY expr RP */
    194, /* (10) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP WITHIN
            GROUP LP ORDER BY expr RP filter_over */
    194, /* (11) expr ::= CAST LP expr AS typetoken RP */
    199, /* (12) typetoken ::= */
    199, /* (13) typetoken ::= typename */
    199, /* (14) typetoken ::= typename LP signed RP */
    199, /* (15) typetoken ::= typename LP signed COMMA signed RP */
    200, /* (16) typename ::= ID|STRING */
    200, /* (17) typename ::= typename ID|STRING */
    202, /* (18) selcollist ::= sclp scanpt nm DOT STAR */
    194, /* (19) expr ::= ID|INDEXED|JOIN_KW */
    194, /* (20) expr ::= nm DOT nm */
    194, /* (21) expr ::= nm DOT nm DOT nm */
    209, /* (22) selectnowith ::= selectnowith multiselect_op oneselect */
    206, /* (23) multiselect_op ::= UNION */
    206, /* (24) multiselect_op ::= UNION ALL */
    206, /* (25) multiselect_op ::= EXCEPT|INTERSECT */
    194, /* (26) expr ::= LP select RP */
    194, /* (27) expr ::= EXISTS LP select RP */
    207, /* (28) in_op ::= IN */
    207, /* (29) in_op ::= NOT IN */
    194, /* (30) expr ::= expr in_op LP exprlist RP */
    194, /* (31) expr ::= expr in_op LP select RP */
    194, /* (32) expr ::= expr in_op nm dbnm paren_exprlist */
    208, /* (33) dbnm ::= */
    208, /* (34) dbnm ::= DOT nm */
    212, /* (35) paren_exprlist ::= */
    212, /* (36) paren_exprlist ::= LP exprlist RP */
    194, /* (37) expr ::= expr ISNULL|NOTNULL */
    194, /* (38) expr ::= expr NOT NULL */
    194, /* (39) expr ::= expr IS expr */
    194, /* (40) expr ::= expr IS NOT expr */
    194, /* (41) expr ::= expr IS NOT DISTINCT FROM expr */
    194, /* (42) expr ::= expr IS DISTINCT FROM expr */
    214, /* (43) between_op ::= BETWEEN */
    214, /* (44) between_op ::= NOT BETWEEN */
    194, /* (45) expr ::= expr between_op expr AND expr */
    213, /* (46) likeop ::= LIKE_KW|MATCH */
    213, /* (47) likeop ::= NOT LIKE_KW|MATCH */
    194, /* (48) expr ::= expr likeop expr */
    194, /* (49) expr ::= expr likeop expr ESCAPE expr */
    194, /* (50) expr ::= CASE case_operand case_exprlist case_else END */
    216, /* (51) case_exprlist ::= case_exprlist WHEN expr THEN expr */
    216, /* (52) case_exprlist ::= WHEN expr THEN expr */
    217, /* (53) case_else ::= ELSE expr */
    217, /* (54) case_else ::= */
    215, /* (55) case_operand ::= expr */
    215, /* (56) case_operand ::= */
    193, /* (57) cmd ::= create_table create_table_args */
    235, /* (58) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
    236, /* (59) create_table_args ::= LP columnlist conslist_opt RP
            table_option_set */
    236, /* (60) create_table_args ::= AS select */
    226, /* (61) table_option_set ::= */
    226, /* (62) table_option_set ::= table_option */
    226, /* (63) table_option_set ::= table_option_set COMMA table_option */
    227, /* (64) table_option ::= WITHOUT nm */
    227, /* (65) table_option ::= nm */
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
    219, /* (88) autoinc ::= */
    219, /* (89) autoinc ::= AUTOINCR */
    220, /* (90) refargs ::= */
    220, /* (91) refargs ::= refargs refarg */
    221, /* (92) refarg ::= MATCH nm */
    221, /* (93) refarg ::= ON INSERT refact */
    221, /* (94) refarg ::= ON DELETE refact */
    221, /* (95) refarg ::= ON UPDATE refact */
    222, /* (96) refact ::= SET NULL */
    222, /* (97) refact ::= SET DEFAULT */
    222, /* (98) refact ::= CASCADE */
    222, /* (99) refact ::= RESTRICT */
    222, /* (100) refact ::= NO ACTION */
    223, /* (101) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
    223, /* (102) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
    224, /* (103) init_deferred_pred_opt ::= */
    224, /* (104) init_deferred_pred_opt ::= INITIALLY DEFERRED */
    224, /* (105) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
    241, /* (106) conslist_opt ::= */
    241, /* (107) conslist_opt ::= COMMA conslist */
    233, /* (108) conslist ::= conslist tconscomma tcons */
    233, /* (109) conslist ::= tcons */
    228, /* (110) tconscomma ::= COMMA */
    228, /* (111) tconscomma ::= */
    232, /* (112) tcons ::= CONSTRAINT nm */
    232, /* (113) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
    232, /* (114) tcons ::= UNIQUE LP sortlist RP onconf */
    232, /* (115) tcons ::= CHECK LP expr RP onconf */
    232, /* (116) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt
            refargs defer_subclause_opt */
    225, /* (117) defer_subclause_opt ::= */
    225, /* (118) defer_subclause_opt ::= defer_subclause */
    229, /* (119) onconf ::= */
    229, /* (120) onconf ::= ON CONFLICT resolvetype */
    218, /* (121) scantok ::= */
    211, /* (122) select ::= WITH wqlist selectnowith */
    211, /* (123) select ::= WITH RECURSIVE wqlist selectnowith */
    252, /* (124) wqitem ::= withnm eidlist_opt wqas LP select RP */
    251, /* (125) wqlist ::= wqitem */
    251, /* (126) wqlist ::= wqlist COMMA wqitem */
    248, /* (127) withnm ::= nm */
    249, /* (128) wqas ::= AS */
    249, /* (129) wqas ::= AS MATERIALIZED */
    249, /* (130) wqas ::= AS NOT MATERIALIZED */
    245, /* (131) eidlist_opt ::= */
    245, /* (132) eidlist_opt ::= LP eidlist RP */
    246, /* (133) eidlist ::= nm collate sortorder */
    246, /* (134) eidlist ::= eidlist COMMA nm collate sortorder */
    250, /* (135) collate ::= */
    250, /* (136) collate ::= COLLATE ID|STRING */
    253, /* (137) with ::= */
    253, /* (138) with ::= WITH wqlist */
    253, /* (139) with ::= WITH RECURSIVE wqlist */
    193, /* (140) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret
            orderby_opt limit_opt */
    193, /* (141) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist
            from where_opt_ret orderby_opt limit_opt */
    193, /* (142) cmd ::= with insert_cmd INTO xfullname idlist_opt select
            upsert */
    193, /* (143) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT
            VALUES returning */
    254, /* (144) insert_cmd ::= INSERT orconf */
    254, /* (145) insert_cmd ::= REPLACE */
    255, /* (146) orconf ::= */
    255, /* (147) orconf ::= OR resolvetype */
    247, /* (148) resolvetype ::= raisetype */
    247, /* (149) resolvetype ::= IGNORE */
    247, /* (150) resolvetype ::= REPLACE */
    260, /* (151) xfullname ::= nm */
    260, /* (152) xfullname ::= nm DOT nm */
    260, /* (153) xfullname ::= nm DOT nm AS nm */
    260, /* (154) xfullname ::= nm AS nm */
    256, /* (155) indexed_opt ::= */
    256, /* (156) indexed_opt ::= indexed_by */
    257, /* (157) where_opt_ret ::= */
    257, /* (158) where_opt_ret ::= WHERE expr */
    257, /* (159) where_opt_ret ::= RETURNING selcollist */
    257, /* (160) where_opt_ret ::= WHERE expr RETURNING selcollist */
    263, /* (161) setlist ::= setlist COMMA nm EQ expr */
    263, /* (162) setlist ::= setlist COMMA LP idlist RP EQ expr */
    263, /* (163) setlist ::= nm EQ expr */
    263, /* (164) setlist ::= LP idlist RP EQ expr */
    265, /* (165) idlist_opt ::= */
    265, /* (166) idlist_opt ::= LP idlist RP */
    258, /* (167) upsert ::= */
    258, /* (168) upsert ::= RETURNING selcollist */
    258, /* (169) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET
            setlist where_opt upsert */
    258, /* (170) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING
            upsert */
    258, /* (171) upsert ::= ON CONFLICT DO NOTHING returning */
    258, /* (172) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt
            returning */
    259, /* (173) returning ::= RETURNING selcollist */
    259, /* (174) returning ::= */
    194, /* (175) expr ::= error */
    194, /* (176) expr ::= term */
    194, /* (177) expr ::= LP expr RP */
    194, /* (178) expr ::= expr PLUS|MINUS expr */
    194, /* (179) expr ::= expr STAR|SLASH|REM expr */
    194, /* (180) expr ::= expr LT|GT|GE|LE expr */
    194, /* (181) expr ::= expr EQ|NE expr */
    194, /* (182) expr ::= expr AND expr */
    194, /* (183) expr ::= expr OR expr */
    194, /* (184) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
    194, /* (185) expr ::= expr CONCAT expr */
    194, /* (186) expr ::= expr PTR expr */
    194, /* (187) expr ::= PLUS|MINUS expr */
    194, /* (188) expr ::= BITNOT expr */
    194, /* (189) expr ::= NOT expr */
    196, /* (190) exprlist ::= nexprlist */
    196, /* (191) exprlist ::= */
    270, /* (192) nexprlist ::= nexprlist COMMA expr */
    270, /* (193) nexprlist ::= expr */
    194, /* (194) expr ::= LP nexprlist COMMA expr RP */
    194, /* (195) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
    194, /* (196) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
    194, /* (197) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP
            filter_over */
    194, /* (198) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
    205, /* (199) nm ::= ID|INDEXED|JOIN_KW */
    205, /* (200) nm ::= STRING */
    271, /* (201) nmorerr ::= nm */
    271, /* (202) nmorerr ::= error */
    243, /* (203) term ::= INTEGER */
    243, /* (204) term ::= STRING */
    243, /* (205) term ::= NULL|FLOAT|BLOB */
    243, /* (206) term ::= QNUMBER */
    243, /* (207) term ::= CTIME_KW */
    194, /* (208) expr ::= VARIABLE */
    194, /* (209) expr ::= expr COLLATE ID|STRING */
    197, /* (210) sortlist ::= sortlist COMMA expr sortorder nulls */
    197, /* (211) sortlist ::= expr sortorder nulls */
    244, /* (212) sortorder ::= ASC */
    244, /* (213) sortorder ::= DESC */
    244, /* (214) sortorder ::= */
    272, /* (215) nulls ::= NULLS FIRST */
    272, /* (216) nulls ::= NULLS LAST */
    272, /* (217) nulls ::= */
    194, /* (218) expr ::= RAISE LP IGNORE RP */
    194, /* (219) expr ::= RAISE LP raisetype COMMA expr RP */
    266, /* (220) raisetype ::= ROLLBACK */
    266, /* (221) raisetype ::= ABORT */
    266, /* (222) raisetype ::= FAIL */
    278, /* (223) fullname ::= nmorerr */
    278, /* (224) fullname ::= nmorerr DOT nmorerr */
    273, /* (225) ifexists ::= IF EXISTS */
    273, /* (226) ifexists ::= */
    193, /* (227) cmd ::= DROP TABLE ifexists fullname */
    193, /* (228) cmd ::= DROP VIEW ifexists fullname */
    193, /* (229) cmd ::= DROP INDEX ifexists fullname */
    193, /* (230) cmd ::= DROP TRIGGER ifexists fullname */
    193, /* (231) cmd ::= ALTER TABLE fullname RENAME TO nmorerr */
    193, /* (232) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nmorerr TO
            nmorerr */
    193, /* (233) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nmorerr */
    193, /* (234) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt
            columnname carglist */
    279, /* (235) add_column_fullname ::= fullname */
    277, /* (236) kwcolumn_opt ::= */
    277, /* (237) kwcolumn_opt ::= COLUMNKW */
    242, /* (238) columnname ::= nmorerr typetoken */
    193, /* (239) cmd ::= BEGIN transtype trans_opt */
    193, /* (240) cmd ::= COMMIT|END trans_opt */
    193, /* (241) cmd ::= ROLLBACK trans_opt */
    274, /* (242) transtype ::= */
    274, /* (243) transtype ::= DEFERRED */
    274, /* (244) transtype ::= IMMEDIATE */
    274, /* (245) transtype ::= EXCLUSIVE */
    275, /* (246) trans_opt ::= */
    275, /* (247) trans_opt ::= TRANSACTION */
    275, /* (248) trans_opt ::= TRANSACTION nm */
    276, /* (249) savepoint_opt ::= SAVEPOINT */
    276, /* (250) savepoint_opt ::= */
    193, /* (251) cmd ::= SAVEPOINT nmorerr */
    193, /* (252) cmd ::= RELEASE savepoint_opt nmorerr */
    193, /* (253) cmd ::= ROLLBACK trans_opt TO savepoint_opt nmorerr */
    193, /* (254) cmd ::= select */
    211, /* (255) select ::= selectnowith */
    209, /* (256) selectnowith ::= oneselect */
    210, /* (257) oneselect ::= SELECT distinct selcollist from where_opt
            groupby_opt having_opt orderby_opt limit_opt */
    210, /* (258) oneselect ::= SELECT distinct selcollist from where_opt
            groupby_opt having_opt window_clause orderby_opt limit_opt */
    202, /* (259) selcollist ::= sclp scanpt expr scanpt as */
    202, /* (260) selcollist ::= sclp scanpt STAR */
    203, /* (261) sclp ::= selcollist COMMA */
    203, /* (262) sclp ::= */
    204, /* (263) scanpt ::= */
    280, /* (264) as ::= AS nmorerr */
    280, /* (265) as ::= ID|STRING */
    280, /* (266) as ::= */
    195, /* (267) distinct ::= DISTINCT */
    195, /* (268) distinct ::= ALL */
    195, /* (269) distinct ::= */
    264, /* (270) from ::= */
    264, /* (271) from ::= FROM seltablist */
    269, /* (272) where_opt ::= */
    269, /* (273) where_opt ::= WHERE expr */
    281, /* (274) groupby_opt ::= */
    281, /* (275) groupby_opt ::= GROUP BY nexprlist */
    282, /* (276) having_opt ::= */
    282, /* (277) having_opt ::= HAVING expr */
    261, /* (278) orderby_opt ::= */
    261, /* (279) orderby_opt ::= ORDER BY sortlist */
    262, /* (280) limit_opt ::= */
    262, /* (281) limit_opt ::= LIMIT expr */
    262, /* (282) limit_opt ::= LIMIT expr OFFSET expr */
    262, /* (283) limit_opt ::= LIMIT expr COMMA expr */
    287, /* (284) stl_prefix ::= seltablist joinop */
    287, /* (285) stl_prefix ::= */
    284, /* (286) seltablist ::= stl_prefix nm dbnm as on_using */
    284, /* (287) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
    284, /* (288) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using
          */
    284, /* (289) seltablist ::= stl_prefix LP select RP as on_using */
    284, /* (290) seltablist ::= stl_prefix LP seltablist RP as on_using */
    286, /* (291) joinop ::= COMMA|JOIN */
    286, /* (292) joinop ::= JOIN_KW JOIN */
    286, /* (293) joinop ::= JOIN_KW nm JOIN */
    286, /* (294) joinop ::= JOIN_KW nm nm JOIN */
    285, /* (295) on_using ::= ON expr */
    285, /* (296) on_using ::= USING LP idlist RP */
    285, /* (297) on_using ::= */
    267, /* (298) indexed_by ::= INDEXED BY nm */
    267, /* (299) indexed_by ::= NOT INDEXED */
    268, /* (300) idlist ::= idlist COMMA nm */
    268, /* (301) idlist ::= nm */
    193, /* (302) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
    290, /* (303) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time
            trigger_event ON fullname foreach_clause when_clause */
    288, /* (304) trigger_time ::= BEFORE|AFTER */
    288, /* (305) trigger_time ::= INSTEAD OF */
    288, /* (306) trigger_time ::= */
    292, /* (307) trigger_event ::= DELETE|INSERT */
    292, /* (308) trigger_event ::= UPDATE */
    292, /* (309) trigger_event ::= UPDATE OF idlist */
    293, /* (310) foreach_clause ::= */
    293, /* (311) foreach_clause ::= FOR EACH ROW */
    294, /* (312) when_clause ::= */
    294, /* (313) when_clause ::= WHEN expr */
    291, /* (314) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
    291, /* (315) trigger_cmd_list ::= trigger_cmd SEMI */
    289, /* (316) trnm ::= nm */
    289, /* (317) trnm ::= nm DOT nm */
    296, /* (318) tridxby ::= */
    296, /* (319) tridxby ::= INDEXED BY nm */
    296, /* (320) tridxby ::= NOT INDEXED */
    295, /* (321) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from
            where_opt scanpt */
    295, /* (322) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select
            upsert scanpt */
    295, /* (323) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
    295, /* (324) trigger_cmd ::= scanpt select scanpt */
    193, /* (325) cmd ::= PRAGMA nm dbnm */
    193, /* (326) cmd ::= PRAGMA nm dbnm EQ nmnum */
    193, /* (327) cmd ::= PRAGMA nm dbnm LP nmnum RP */
    193, /* (328) cmd ::= PRAGMA nm dbnm EQ minus_num */
    193, /* (329) cmd ::= PRAGMA nm dbnm LP minus_num RP */
    299, /* (330) nmnum ::= plus_num */
    299, /* (331) nmnum ::= nm */
    299, /* (332) nmnum ::= ON */
    299, /* (333) nmnum ::= DELETE */
    299, /* (334) nmnum ::= DEFAULT */
    297, /* (335) plus_num ::= PLUS INTEGER|FLOAT */
    297, /* (336) plus_num ::= INTEGER|FLOAT */
    298, /* (337) minus_num ::= MINUS INTEGER|FLOAT */
    201, /* (338) signed ::= plus_num */
    201, /* (339) signed ::= minus_num */
    193, /* (340) cmd ::= ANALYZE */
    193, /* (341) cmd ::= ANALYZE nm dbnm */
    193, /* (342) cmd ::= REINDEX */
    193, /* (343) cmd ::= REINDEX nm dbnm */
    193, /* (344) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
    193, /* (345) cmd ::= DETACH database_kw_opt expr */
    302, /* (346) database_kw_opt ::= DATABASE */
    302, /* (347) database_kw_opt ::= */
    303, /* (348) key_opt ::= */
    303, /* (349) key_opt ::= KEY expr */
    193, /* (350) cmd ::= VACUUM vinto */
    193, /* (351) cmd ::= VACUUM nm vinto */
    304, /* (352) vinto ::= INTO expr */
    304, /* (353) vinto ::= */
    190, /* (354) ecmd ::= explain cmdx SEMI */
    301, /* (355) explain ::= EXPLAIN */
    301, /* (356) explain ::= EXPLAIN QUERY PLAN */
    193, /* (357) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP
            sortlist RP where_opt */
    300, /* (358) uniqueflag ::= UNIQUE */
    300, /* (359) uniqueflag ::= */
    239, /* (360) ifnotexists ::= */
    239, /* (361) ifnotexists ::= IF NOT EXISTS */
    193, /* (362) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS
            select */
    237, /* (363) createkw ::= CREATE */
    238, /* (364) temp ::= TEMP */
    238, /* (365) temp ::= */
    305, /* (366) values ::= VALUES LP nexprlist RP */
    306, /* (367) mvalues ::= values COMMA LP nexprlist RP */
    306, /* (368) mvalues ::= mvalues COMMA LP nexprlist RP */
    210, /* (369) oneselect ::= values */
    210, /* (370) oneselect ::= mvalues */
    193, /* (371) cmd ::= create_vtab */
    193, /* (372) cmd ::= create_vtab LP vtabarglist RP */
    307, /* (373) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm
            USING nm */
    308, /* (374) vtabarglist ::= vtabarg */
    308, /* (375) vtabarglist ::= vtabarglist COMMA vtabarg */
    309, /* (376) vtabarg ::= */
    309, /* (377) vtabarg ::= vtabarg vtabargtoken */
    310, /* (378) vtabargtoken ::= ANY */
    310, /* (379) vtabargtoken ::= lp anylist RP */
    311, /* (380) lp ::= LP */
    312, /* (381) anylist ::= */
    312, /* (382) anylist ::= anylist LP anylist RP */
    312, /* (383) anylist ::= anylist ANY */
    316, /* (384) windowdefn_list ::= windowdefn */
    316, /* (385) windowdefn_list ::= windowdefn_list COMMA windowdefn */
    317, /* (386) windowdefn ::= nm AS LP window RP */
    318, /* (387) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
    318, /* (388) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
    318, /* (389) window ::= ORDER BY sortlist frame_opt */
    318, /* (390) window ::= nm ORDER BY sortlist frame_opt */
    318, /* (391) window ::= frame_opt */
    318, /* (392) window ::= nm frame_opt */
    319, /* (393) frame_opt ::= */
    319, /* (394) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
    319, /* (395) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND
            frame_bound_e frame_exclude_opt */
    313, /* (396) range_or_rows ::= RANGE|ROWS|GROUPS */
    320, /* (397) frame_bound_s ::= frame_bound */
    320, /* (398) frame_bound_s ::= UNBOUNDED PRECEDING */
    321, /* (399) frame_bound_e ::= frame_bound */
    321, /* (400) frame_bound_e ::= UNBOUNDED FOLLOWING */
    322, /* (401) frame_bound ::= expr PRECEDING|FOLLOWING */
    322, /* (402) frame_bound ::= CURRENT ROW */
    314, /* (403) frame_exclude_opt ::= */
    314, /* (404) frame_exclude_opt ::= EXCLUDE frame_exclude */
    315, /* (405) frame_exclude ::= NO OTHERS */
    315, /* (406) frame_exclude ::= CURRENT ROW */
    315, /* (407) frame_exclude ::= GROUP|TIES */
    283, /* (408) window_clause ::= WINDOW windowdefn_list */
    198, /* (409) filter_over ::= filter_clause over_clause */
    198, /* (410) filter_over ::= over_clause */
    198, /* (411) filter_over ::= filter_clause */
    324, /* (412) over_clause ::= OVER LP window RP */
    324, /* (413) over_clause ::= OVER nm */
    323, /* (414) filter_clause ::= FILTER LP WHERE expr RP */
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
      pCtx->root = yymsp[0].minor.yy141;
    } break;
    case 1: /* cmdlist ::= cmdlist ecmd */
    {
      yymsp[-1].minor.yy141 =
          yymsp[0].minor.yy141;  // Just use the last command for now
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
        yylhsminor.yy141 = yymsp[0].minor.yy141;
      }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 3: /* ecmd ::= SEMI */
    {
      yymsp[0].minor.yy141 = SYNTAQLITE_NULL_NODE;
      pCtx->stmt_completed = 1;
    } break;
    case 4:   /* ecmd ::= cmdx SEMI */
    case 261: /* sclp ::= selcollist COMMA */
      yytestcase(yyruleno == 261);
      {
        yylhsminor.yy141 = yymsp[-1].minor.yy141;
      }
      yymsp[-1].minor.yy141 = yylhsminor.yy141;
      break;
    case 5: /* ecmd ::= error SEMI */
    {
      yymsp[-1].minor.yy141 = SYNTAQLITE_NULL_NODE;
      pCtx->root = SYNTAQLITE_NULL_NODE;
      pCtx->stmt_completed = 1;
    } break;
    case 6: /* cmdx ::= cmd */
    {
      if (pCtx->pending_explain_mode) {
        yylhsminor.yy141 = synq_parse_explain_stmt(
            pCtx, (SyntaqliteExplainMode)(pCtx->pending_explain_mode - 1),
            yymsp[0].minor.yy141);
        pCtx->pending_explain_mode = 0;
      } else {
        yylhsminor.yy141 = yymsp[0].minor.yy141;
      }
      pCtx->root = yylhsminor.yy141;
      synq_parse_list_flush(pCtx);
      pCtx->stmt_completed = 1;
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 7: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY
               sortlist RP */
    {
      synq_mark_as_function(pCtx, yymsp[-7].minor.yy0);
      yylhsminor.yy141 = synq_parse_aggregate_function_call(
          pCtx, synq_span(pCtx, yymsp[-7].minor.yy0),
          (SyntaqliteAggregateFunctionCallFlags){
              .raw = (uint8_t)yymsp[-5].minor.yy141},
          yymsp[-4].minor.yy141, yymsp[-1].minor.yy141, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE);
    }
      yymsp[-7].minor.yy141 = yylhsminor.yy141;
      break;
    case 8: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY
               sortlist RP filter_over */
    {
      SyntaqliteFilterOver* fo = (SyntaqliteFilterOver*)synq_arena_ptr(
          &pCtx->ast, yymsp[0].minor.yy141);
      synq_mark_as_function(pCtx, yymsp[-8].minor.yy0);
      yylhsminor.yy141 = synq_parse_aggregate_function_call(
          pCtx, synq_span(pCtx, yymsp[-8].minor.yy0),
          (SyntaqliteAggregateFunctionCallFlags){
              .raw = (uint8_t)yymsp[-6].minor.yy141},
          yymsp[-5].minor.yy141, yymsp[-2].minor.yy141, fo->filter_expr,
          fo->over_def);
    }
      yymsp[-8].minor.yy141 = yylhsminor.yy141;
      break;
    case 9: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP WITHIN GROUP
               LP ORDER BY expr RP */
    {
      synq_mark_as_function(pCtx, yymsp[-11].minor.yy0);
      yylhsminor.yy141 = synq_parse_ordered_set_function_call(
          pCtx, synq_span(pCtx, yymsp[-11].minor.yy0),
          (SyntaqliteAggregateFunctionCallFlags){
              .raw = (uint8_t)yymsp[-9].minor.yy141},
          yymsp[-8].minor.yy141, yymsp[-1].minor.yy141, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE);
    }
      yymsp[-11].minor.yy141 = yylhsminor.yy141;
      break;
    case 10: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP WITHIN GROUP
                LP ORDER BY expr RP filter_over */
    {
      SyntaqliteFilterOver* fo = (SyntaqliteFilterOver*)synq_arena_ptr(
          &pCtx->ast, yymsp[0].minor.yy141);
      synq_mark_as_function(pCtx, yymsp[-12].minor.yy0);
      yylhsminor.yy141 = synq_parse_ordered_set_function_call(
          pCtx, synq_span(pCtx, yymsp[-12].minor.yy0),
          (SyntaqliteAggregateFunctionCallFlags){
              .raw = (uint8_t)yymsp[-10].minor.yy141},
          yymsp[-9].minor.yy141, yymsp[-2].minor.yy141, fo->filter_expr,
          fo->over_def);
    }
      yymsp[-12].minor.yy141 = yylhsminor.yy141;
      break;
    case 11: /* expr ::= CAST LP expr AS typetoken RP */
    {
      yymsp[-5].minor.yy141 = synq_parse_cast_expr(
          pCtx, yymsp[-3].minor.yy141, synq_span(pCtx, yymsp[-1].minor.yy0));
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
          pCtx, (SyntaqliteResultColumnFlags){.bits = {.star = 1}},
          SYNTAQLITE_NULL_NODE, expr);
      yylhsminor.yy141 =
          synq_parse_result_column_list(pCtx, yymsp[-4].minor.yy141, col);
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 19: /* expr ::= ID|INDEXED|JOIN_KW */
    {
      synq_mark_as_id(pCtx, yymsp[0].minor.yy0);
      yylhsminor.yy141 = synq_parse_column_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0), SYNQ_NO_SPAN,
          SYNQ_NO_SPAN);
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 20: /* expr ::= nm DOT nm */
    {
      yylhsminor.yy141 = synq_parse_column_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0),
          synq_span_dequote(pCtx, yymsp[-2].minor.yy0), SYNQ_NO_SPAN);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 21: /* expr ::= nm DOT nm DOT nm */
    {
      yylhsminor.yy141 = synq_parse_column_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0),
          synq_span_dequote(pCtx, yymsp[-2].minor.yy0),
          synq_span_dequote(pCtx, yymsp[-4].minor.yy0));
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 22: /* selectnowith ::= selectnowith multiselect_op oneselect */
    {
      yymsp[-2].minor.yy141 = synq_parse_compound_select(
          pCtx, (SyntaqliteCompoundOp)yymsp[-1].minor.yy592,
          yymsp[-2].minor.yy141, yymsp[0].minor.yy141);
    } break;
    case 23: /* multiselect_op ::= UNION */
    {
      yylhsminor.yy592 = 0;
      (void)yymsp[0].minor.yy0;
    }
      yymsp[0].minor.yy592 = yylhsminor.yy592;
      break;
    case 24: /* multiselect_op ::= UNION ALL */
    case 29: /* in_op ::= NOT IN */
      yytestcase(yyruleno == 29);
      {
        yymsp[-1].minor.yy592 = 1;
      }
      break;
    case 25: /* multiselect_op ::= EXCEPT|INTERSECT */
    {
      yylhsminor.yy592 =
          (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_INTERSECT) ? 2 : 3;
    }
      yymsp[0].minor.yy592 = yylhsminor.yy592;
      break;
    case 26: /* expr ::= LP select RP */
    {
      pCtx->saw_subquery = 1;
      yymsp[-2].minor.yy141 =
          synq_parse_subquery_expr(pCtx, yymsp[-1].minor.yy141);
    } break;
    case 27: /* expr ::= EXISTS LP select RP */
    {
      pCtx->saw_subquery = 1;
      yymsp[-3].minor.yy141 =
          synq_parse_exists_expr(pCtx, yymsp[-1].minor.yy141);
    } break;
    case 28: /* in_op ::= IN */
    {
      yymsp[0].minor.yy592 = 0;
    } break;
    case 30: /* expr ::= expr in_op LP exprlist RP */
    {
      yymsp[-4].minor.yy141 =
          synq_parse_in_expr(pCtx, (SyntaqliteBool)yymsp[-3].minor.yy592,
                             yymsp[-4].minor.yy141, yymsp[-1].minor.yy141);
    } break;
    case 31: /* expr ::= expr in_op LP select RP */
    {
      pCtx->saw_subquery = 1;
      // Pass the raw select node directly — InExpr's fmt block already adds
      // the surrounding parens, so wrapping in SubqueryExpr would double them.
      yymsp[-4].minor.yy141 =
          synq_parse_in_expr(pCtx, (SyntaqliteBool)yymsp[-3].minor.yy592,
                             yymsp[-4].minor.yy141, yymsp[-1].minor.yy141);
    } break;
    case 32: /* expr ::= expr in_op nm dbnm paren_exprlist */
    {
      // Table-valued function IN expression - stub for now
      (void)yymsp[-2].minor.yy0;
      (void)yymsp[-1].minor.yy0;
      (void)yymsp[0].minor.yy141;
      yymsp[-4].minor.yy141 =
          synq_parse_in_expr(pCtx, (SyntaqliteBool)yymsp[-3].minor.yy592,
                             yymsp[-4].minor.yy141, SYNTAQLITE_NULL_NODE);
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
      yymsp[1].minor.yy141 = SYNTAQLITE_NULL_NODE;
    } break;
    case 36: /* paren_exprlist ::= LP exprlist RP */
    {
      yymsp[-2].minor.yy141 = yymsp[-1].minor.yy141;
    } break;
    case 37: /* expr ::= expr ISNULL|NOTNULL */
    {
      SyntaqliteIsOp op = (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_ISNULL)
                              ? SYNTAQLITE_IS_OP_IS_NULL
                              : SYNTAQLITE_IS_OP_NOT_NULL;
      yylhsminor.yy141 = synq_parse_is_expr(pCtx, op, yymsp[-1].minor.yy141,
                                            SYNTAQLITE_NULL_NODE);
    }
      yymsp[-1].minor.yy141 = yylhsminor.yy141;
      break;
    case 38: /* expr ::= expr NOT NULL */
    {
      yylhsminor.yy141 =
          synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_NOT_NULL,
                             yymsp[-2].minor.yy141, SYNTAQLITE_NULL_NODE);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 39: /* expr ::= expr IS expr */
    {
      yylhsminor.yy141 =
          synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_IS, yymsp[-2].minor.yy141,
                             yymsp[0].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 40: /* expr ::= expr IS NOT expr */
    {
      yylhsminor.yy141 =
          synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_IS_NOT,
                             yymsp[-3].minor.yy141, yymsp[0].minor.yy141);
    }
      yymsp[-3].minor.yy141 = yylhsminor.yy141;
      break;
    case 41: /* expr ::= expr IS NOT DISTINCT FROM expr */
    {
      yylhsminor.yy141 =
          synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_IS_NOT_DISTINCT,
                             yymsp[-5].minor.yy141, yymsp[0].minor.yy141);
    }
      yymsp[-5].minor.yy141 = yylhsminor.yy141;
      break;
    case 42: /* expr ::= expr IS DISTINCT FROM expr */
    {
      yylhsminor.yy141 =
          synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_IS_DISTINCT,
                             yymsp[-4].minor.yy141, yymsp[0].minor.yy141);
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 43:  /* between_op ::= BETWEEN */
    case 212: /* sortorder ::= ASC */
      yytestcase(yyruleno == 212);
    case 268: /* distinct ::= ALL */
      yytestcase(yyruleno == 268);
      {
        yymsp[0].minor.yy141 = 0;
      }
      break;
    case 44:  /* between_op ::= NOT BETWEEN */
    case 215: /* nulls ::= NULLS FIRST */
      yytestcase(yyruleno == 215);
      {
        yymsp[-1].minor.yy141 = 1;
      }
      break;
    case 45: /* expr ::= expr between_op expr AND expr */
    {
      yylhsminor.yy141 = synq_parse_between_expr(
          pCtx, (SyntaqliteBool)yymsp[-3].minor.yy141, yymsp[-4].minor.yy141,
          yymsp[-2].minor.yy141, yymsp[0].minor.yy141);
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 46:  /* likeop ::= LIKE_KW|MATCH */
    case 200: /* nm ::= STRING */
      yytestcase(yyruleno == 200);
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
      yylhsminor.yy141 =
          synq_parse_like_expr(pCtx, negated, kw, yymsp[-2].minor.yy141,
                               yymsp[0].minor.yy141, SYNTAQLITE_NULL_NODE);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
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
      yylhsminor.yy141 =
          synq_parse_like_expr(pCtx, negated, kw, yymsp[-4].minor.yy141,
                               yymsp[-2].minor.yy141, yymsp[0].minor.yy141);
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 50: /* expr ::= CASE case_operand case_exprlist case_else END */
    {
      yymsp[-4].minor.yy141 =
          synq_parse_case_expr(pCtx, yymsp[-3].minor.yy141,
                               yymsp[-1].minor.yy141, yymsp[-2].minor.yy141);
    } break;
    case 51: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
    {
      uint32_t w = synq_parse_case_when(pCtx, yymsp[-2].minor.yy141,
                                        yymsp[0].minor.yy141);
      yylhsminor.yy141 =
          synq_parse_case_when_list(pCtx, yymsp[-4].minor.yy141, w);
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 52: /* case_exprlist ::= WHEN expr THEN expr */
    {
      uint32_t w = synq_parse_case_when(pCtx, yymsp[-2].minor.yy141,
                                        yymsp[0].minor.yy141);
      yymsp[-3].minor.yy141 =
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
        yymsp[-1].minor.yy141 = yymsp[0].minor.yy141;
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
        yymsp[1].minor.yy141 = SYNTAQLITE_NULL_NODE;
      }
      break;
    case 57: /* cmd ::= create_table create_table_args */
    {
      // yymsp[0].minor.yy141 is either: (1) a CreateTableStmt node with
      // columns/constraints filled in or: (2) a CreateTableStmt node with
      // as_select filled in yymsp[-1].minor.yy141 has the table
      // name/schema/temp/ifnotexists info packed as a node. We need to merge
      // yymsp[-1].minor.yy141 info into yymsp[0].minor.yy141.
      SyntaqliteNode* ct_node = AST_NODE(&pCtx->ast, yymsp[-1].minor.yy141);
      SyntaqliteNode* args_node = AST_NODE(&pCtx->ast, yymsp[0].minor.yy141);
      args_node->create_table_stmt.table_name =
          ct_node->create_table_stmt.table_name;
      args_node->create_table_stmt.schema = ct_node->create_table_stmt.schema;
      args_node->create_table_stmt.is_temp = ct_node->create_table_stmt.is_temp;
      args_node->create_table_stmt.if_not_exists =
          ct_node->create_table_stmt.if_not_exists;
      yylhsminor.yy141 = yymsp[0].minor.yy141;
    }
      yymsp[-1].minor.yy141 = yylhsminor.yy141;
      break;
    case 58: /* create_table ::= createkw temp TABLE ifnotexists nm dbnm */
    {
      SyntaqliteSourceSpan tbl_name =
          yymsp[0].minor.yy0.z ? synq_span_dequote(pCtx, yymsp[0].minor.yy0)
                               : synq_span_dequote(pCtx, yymsp[-1].minor.yy0);
      SyntaqliteSourceSpan tbl_schema =
          yymsp[0].minor.yy0.z ? synq_span_dequote(pCtx, yymsp[-1].minor.yy0)
                               : SYNQ_NO_SPAN;
      yymsp[-5].minor.yy141 = synq_parse_create_table_stmt(
          pCtx, tbl_name, tbl_schema, (SyntaqliteBool)yymsp[-4].minor.yy592,
          (SyntaqliteBool)yymsp[-2].minor.yy592,
          (SyntaqliteCreateTableStmtFlags){.raw = 0}, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    } break;
    case 59: /* create_table_args ::= LP columnlist conslist_opt RP
                table_option_set */
    {
      yymsp[-4].minor.yy141 = synq_parse_create_table_stmt(
          pCtx, SYNQ_NO_SPAN, SYNQ_NO_SPAN, SYNTAQLITE_BOOL_FALSE,
          SYNTAQLITE_BOOL_FALSE,
          (SyntaqliteCreateTableStmtFlags){.raw =
                                               (uint8_t)yymsp[0].minor.yy592},
          yymsp[-3].minor.yy141, yymsp[-2].minor.yy141, SYNTAQLITE_NULL_NODE);
    } break;
    case 60: /* create_table_args ::= AS select */
    {
      yymsp[-1].minor.yy141 = synq_parse_create_table_stmt(
          pCtx, SYNQ_NO_SPAN, SYNQ_NO_SPAN, SYNTAQLITE_BOOL_FALSE,
          SYNTAQLITE_BOOL_FALSE, (SyntaqliteCreateTableStmtFlags){.raw = 0},
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy141);
    } break;
    case 61: /* table_option_set ::= */
    case 88: /* autoinc ::= */
      yytestcase(yyruleno == 88);
    case 103: /* init_deferred_pred_opt ::= */
      yytestcase(yyruleno == 103);
    case 117: /* defer_subclause_opt ::= */
      yytestcase(yyruleno == 117);
    case 135: /* collate ::= */
      yytestcase(yyruleno == 135);
    case 226: /* ifexists ::= */
      yytestcase(yyruleno == 226);
    case 236: /* kwcolumn_opt ::= */
      yytestcase(yyruleno == 236);
    case 246: /* trans_opt ::= */
      yytestcase(yyruleno == 246);
    case 250: /* savepoint_opt ::= */
      yytestcase(yyruleno == 250);
    case 359: /* uniqueflag ::= */
      yytestcase(yyruleno == 359);
    case 360: /* ifnotexists ::= */
      yytestcase(yyruleno == 360);
    case 365: /* temp ::= */
      yytestcase(yyruleno == 365);
      {
        yymsp[1].minor.yy592 = 0;
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
      yylhsminor.yy592 = yymsp[-2].minor.yy592 | yymsp[0].minor.yy592;
    }
      yymsp[-2].minor.yy592 = yylhsminor.yy592;
      break;
    case 64: /* table_option ::= WITHOUT nm */
    {
      // WITHOUT ROWID = bit 0
      if (yymsp[0].minor.yy0.n == 5 &&
          SYNQ_STRNCASECMP(yymsp[0].minor.yy0.z, "rowid", 5) == 0) {
        yymsp[-1].minor.yy592 = 1;
      } else {
        yymsp[-1].minor.yy592 = 0;
      }
    } break;
    case 65: /* table_option ::= nm */
    {
      // STRICT = bit 1
      if (yymsp[0].minor.yy0.n == 6 &&
          SYNQ_STRNCASECMP(yymsp[0].minor.yy0.z, "strict", 6) == 0) {
        yylhsminor.yy592 = 2;
      } else {
        yylhsminor.yy592 = 0;
      }
    }
      yymsp[0].minor.yy592 = yylhsminor.yy592;
      break;
    case 66: /* columnlist ::= columnlist COMMA columnname carglist */
    {
      uint32_t col = synq_parse_column_def(pCtx, yymsp[-1].minor.yy452.name,
                                           yymsp[-1].minor.yy452.typetoken,
                                           yymsp[0].minor.yy94.list);
      yylhsminor.yy141 =
          synq_parse_column_def_list(pCtx, yymsp[-3].minor.yy141, col);
    }
      yymsp[-3].minor.yy141 = yylhsminor.yy141;
      break;
    case 67: /* columnlist ::= columnname carglist */
    {
      uint32_t col = synq_parse_column_def(pCtx, yymsp[-1].minor.yy452.name,
                                           yymsp[-1].minor.yy452.typetoken,
                                           yymsp[0].minor.yy94.list);
      yylhsminor.yy141 =
          synq_parse_column_def_list(pCtx, SYNTAQLITE_NULL_NODE, col);
    }
      yymsp[-1].minor.yy141 = yylhsminor.yy141;
      break;
    case 68: /* carglist ::= carglist ccons */
    {
      if (yymsp[0].minor.yy356.node != SYNTAQLITE_NULL_NODE) {
        // Apply pending constraint name from the list to this node
        SyntaqliteNode* node = AST_NODE(&pCtx->ast, yymsp[0].minor.yy356.node);
        node->column_constraint.constraint_name =
            yymsp[-1].minor.yy94.pending_name;
        if (yymsp[-1].minor.yy94.list == SYNTAQLITE_NULL_NODE) {
          yylhsminor.yy94.list = synq_parse_column_constraint_list(
              pCtx, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy356.node);
        } else {
          yylhsminor.yy94.list = synq_parse_column_constraint_list(
              pCtx, yymsp[-1].minor.yy94.list, yymsp[0].minor.yy356.node);
        }
        yylhsminor.yy94.pending_name = SYNQ_NO_SPAN;
      } else if (yymsp[0].minor.yy356.pending_name.length > 0) {
        // CONSTRAINT nm — store pending name for next constraint
        yylhsminor.yy94.list = yymsp[-1].minor.yy94.list;
        yylhsminor.yy94.pending_name = yymsp[0].minor.yy356.pending_name;
      } else {
        yylhsminor.yy94 = yymsp[-1].minor.yy94;
      }
    }
      yymsp[-1].minor.yy94 = yylhsminor.yy94;
      break;
    case 69: /* carglist ::= */
    {
      yymsp[1].minor.yy94.list = SYNTAQLITE_NULL_NODE;
      yymsp[1].minor.yy94.pending_name = SYNQ_NO_SPAN;
    } break;
    case 70:  /* ccons ::= CONSTRAINT nm */
    case 112: /* tcons ::= CONSTRAINT nm */
      yytestcase(yyruleno == 112);
      {
        yymsp[-1].minor.yy356.node = SYNTAQLITE_NULL_NODE;
        yymsp[-1].minor.yy356.pending_name =
            synq_span(pCtx, yymsp[0].minor.yy0);
      }
      break;
    case 71: /* ccons ::= DEFAULT scantok term */
    {
      yymsp[-2].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, yymsp[0].minor.yy141,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-2].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 72: /* ccons ::= DEFAULT LP expr RP */
    {
      yymsp[-3].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, yymsp[-1].minor.yy141,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-3].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 73: /* ccons ::= DEFAULT PLUS scantok term */
    {
      yymsp[-3].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, yymsp[0].minor.yy141,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-3].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 74: /* ccons ::= DEFAULT MINUS scantok term */
    {
      // Create a unary minus wrapping the term
      uint32_t neg = synq_parse_unary_expr(pCtx, SYNTAQLITE_UNARY_OP_MINUS,
                                           yymsp[0].minor.yy141);
      yymsp[-3].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, neg,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-3].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 75: /* ccons ::= DEFAULT scantok ID|INDEXED */
    {
      uint32_t ref =
          synq_parse_column_ref(pCtx, synq_span(pCtx, yymsp[0].minor.yy0),
                                SYNQ_NO_SPAN, SYNQ_NO_SPAN);
      yymsp[-2].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, ref,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-2].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 76: /* ccons ::= NULL onconf */
    {
      yymsp[-1].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_NULL, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[0].minor.yy592,
          SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-1].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 77: /* ccons ::= NOT NULL onconf */
    {
      yymsp[-2].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_NOT_NULL, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[0].minor.yy592,
          SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-2].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 78: /* ccons ::= PRIMARY KEY sortorder onconf autoinc */
    {
      yymsp[-4].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_PRIMARY_KEY, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[-1].minor.yy592,
          (SyntaqliteSortOrder)yymsp[-2].minor.yy141,
          (SyntaqliteBool)yymsp[0].minor.yy592, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-4].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 79: /* ccons ::= UNIQUE onconf */
    {
      yymsp[-1].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_UNIQUE, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[0].minor.yy592,
          SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-1].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 80: /* ccons ::= CHECK LP expr RP */
    {
      yymsp[-3].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_CHECK, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, SYNTAQLITE_NULL_NODE,
          yymsp[-1].minor.yy141, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-3].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 81: /* ccons ::= REFERENCES nm eidlist_opt refargs */
    {
      // Decode refargs: low byte = on_delete, next byte = on_update
      SyntaqliteForeignKeyAction on_del =
          (SyntaqliteForeignKeyAction)(yymsp[0].minor.yy592 & 0xff);
      SyntaqliteForeignKeyAction on_upd =
          (SyntaqliteForeignKeyAction)((yymsp[0].minor.yy592 >> 8) & 0xff);
      uint32_t fk = synq_parse_foreign_key_clause(
          pCtx, synq_span(pCtx, yymsp[-2].minor.yy0), yymsp[-1].minor.yy141,
          on_del, on_upd, SYNTAQLITE_BOOL_FALSE);
      yymsp[-3].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_REFERENCES, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, fk);
      yymsp[-3].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 82: /* ccons ::= defer_subclause */
    {
      // Create a minimal constraint that just marks deferral.
      // In practice, this follows a REFERENCES ccons. We'll handle it
      // by updating the last constraint in the list if possible.
      // For simplicity, we create a separate REFERENCES constraint with just
      // deferral info. The printer will show it as a separate constraint entry.
      uint32_t fk = synq_parse_foreign_key_clause(
          pCtx, SYNQ_NO_SPAN, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION,
          SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION,
          (SyntaqliteBool)yymsp[0].minor.yy592);
      yylhsminor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_REFERENCES, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, fk);
      yylhsminor.yy356.pending_name = SYNQ_NO_SPAN;
    }
      yymsp[0].minor.yy356 = yylhsminor.yy356;
      break;
    case 83: /* ccons ::= COLLATE ID|STRING */
    {
      yymsp[-1].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_COLLATE, SYNQ_NO_SPAN, 0, 0,
          0, synq_span(pCtx, yymsp[0].minor.yy0),
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-1].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 84: /* ccons ::= GENERATED ALWAYS AS generated */
    {
      yymsp[-3].minor.yy356 = yymsp[0].minor.yy356;
    } break;
    case 85: /* ccons ::= AS generated */
    {
      yymsp[-1].minor.yy356 = yymsp[0].minor.yy356;
    } break;
    case 86: /* generated ::= LP expr RP */
    {
      yymsp[-2].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_GENERATED, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN,
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, yymsp[-1].minor.yy141, SYNTAQLITE_NULL_NODE);
      yymsp[-2].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 87: /* generated ::= LP expr RP ID */
    {
      SyntaqliteGeneratedColumnStorage storage =
          SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL;
      if (yymsp[0].minor.yy0.n == 6 &&
          SYNQ_STRNCASECMP(yymsp[0].minor.yy0.z, "stored", 6) == 0) {
        storage = SYNTAQLITE_GENERATED_COLUMN_STORAGE_STORED;
      }
      yymsp[-3].minor.yy356.node = synq_parse_column_constraint(
          pCtx, SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_GENERATED, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC,
          SYNTAQLITE_BOOL_FALSE, SYNQ_NO_SPAN, storage, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, yymsp[-2].minor.yy141, SYNTAQLITE_NULL_NODE);
      yymsp[-3].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 89:  /* autoinc ::= AUTOINCR */
    case 237: /* kwcolumn_opt ::= COLUMNKW */
      yytestcase(yyruleno == 237);
    case 358: /* uniqueflag ::= UNIQUE */
      yytestcase(yyruleno == 358);
    case 364: /* temp ::= TEMP */
      yytestcase(yyruleno == 364);
      {
        yymsp[0].minor.yy592 = 1;
      }
      break;
    case 90: /* refargs ::= */
    {
      yymsp[1].minor.yy592 = 0;  // NO_ACTION for both
    } break;
    case 91: /* refargs ::= refargs refarg */
    {
      // refarg encodes: low byte = value, byte 1 = shift amount (0 or 8)
      int val = yymsp[0].minor.yy592 & 0xff;
      int shift = (yymsp[0].minor.yy592 >> 8) & 0xff;
      // Clear the target byte in yymsp[-1].minor.yy592 and set new value
      yymsp[-1].minor.yy592 =
          (yymsp[-1].minor.yy592 & ~(0xff << shift)) | (val << shift);
    } break;
    case 92: /* refarg ::= MATCH nm */
    {
      yymsp[-1].minor.yy592 = 0;  // MATCH is ignored
    } break;
    case 93: /* refarg ::= ON INSERT refact */
    {
      yymsp[-2].minor.yy592 = 0;  // ON INSERT is ignored
    } break;
    case 94: /* refarg ::= ON DELETE refact */
    {
      yymsp[-2].minor.yy592 = yymsp[0].minor.yy592;  // shift=0 for DELETE
    } break;
    case 95: /* refarg ::= ON UPDATE refact */
    {
      yymsp[-2].minor.yy592 =
          yymsp[0].minor.yy592 | (8 << 8);  // shift=8 for UPDATE
    } break;
    case 96: /* refact ::= SET NULL */
    {
      yymsp[-1].minor.yy592 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_SET_NULL;
    } break;
    case 97: /* refact ::= SET DEFAULT */
    {
      yymsp[-1].minor.yy592 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_SET_DEFAULT;
    } break;
    case 98: /* refact ::= CASCADE */
    {
      yymsp[0].minor.yy592 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_CASCADE;
    } break;
    case 99: /* refact ::= RESTRICT */
    {
      yymsp[0].minor.yy592 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_RESTRICT;
    } break;
    case 100: /* refact ::= NO ACTION */
    {
      yymsp[-1].minor.yy592 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION;
    } break;
    case 101: /* defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
    {
      yymsp[-2].minor.yy592 = 0;
    } break;
    case 102: /* defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
    case 144: /* insert_cmd ::= INSERT orconf */
      yytestcase(yyruleno == 144);
    case 147: /* orconf ::= OR resolvetype */
      yytestcase(yyruleno == 147);
    case 404: /* frame_exclude_opt ::= EXCLUDE frame_exclude */
      yytestcase(yyruleno == 404);
      {
        yymsp[-1].minor.yy592 = yymsp[0].minor.yy592;
      }
      break;
    case 104: /* init_deferred_pred_opt ::= INITIALLY DEFERRED */
    case 136: /* collate ::= COLLATE ID|STRING */
      yytestcase(yyruleno == 136);
    case 225: /* ifexists ::= IF EXISTS */
      yytestcase(yyruleno == 225);
      {
        yymsp[-1].minor.yy592 = 1;
      }
      break;
    case 105: /* init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
    case 248: /* trans_opt ::= TRANSACTION nm */
      yytestcase(yyruleno == 248);
      {
        yymsp[-1].minor.yy592 = 0;
      }
      break;
    case 107: /* conslist_opt ::= COMMA conslist */
    {
      yymsp[-1].minor.yy141 = yymsp[0].minor.yy94.list;
    } break;
    case 108: /* conslist ::= conslist tconscomma tcons */
    {
      // If comma separator was present, clear pending constraint name
      SyntaqliteSourceSpan pending = yymsp[-1].minor.yy592
                                         ? SYNQ_NO_SPAN
                                         : yymsp[-2].minor.yy94.pending_name;
      if (yymsp[0].minor.yy356.node != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode* node = AST_NODE(&pCtx->ast, yymsp[0].minor.yy356.node);
        node->table_constraint.constraint_name = pending;
        if (yymsp[-2].minor.yy94.list == SYNTAQLITE_NULL_NODE) {
          yylhsminor.yy94.list = synq_parse_table_constraint_list(
              pCtx, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy356.node);
        } else {
          yylhsminor.yy94.list = synq_parse_table_constraint_list(
              pCtx, yymsp[-2].minor.yy94.list, yymsp[0].minor.yy356.node);
        }
        yylhsminor.yy94.pending_name = SYNQ_NO_SPAN;
      } else if (yymsp[0].minor.yy356.pending_name.length > 0) {
        yylhsminor.yy94.list = yymsp[-2].minor.yy94.list;
        yylhsminor.yy94.pending_name = yymsp[0].minor.yy356.pending_name;
      } else {
        yylhsminor.yy94 = yymsp[-2].minor.yy94;
      }
    }
      yymsp[-2].minor.yy94 = yylhsminor.yy94;
      break;
    case 109: /* conslist ::= tcons */
    {
      if (yymsp[0].minor.yy356.node != SYNTAQLITE_NULL_NODE) {
        yylhsminor.yy94.list = synq_parse_table_constraint_list(
            pCtx, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy356.node);
        yylhsminor.yy94.pending_name = SYNQ_NO_SPAN;
      } else {
        yylhsminor.yy94.list = SYNTAQLITE_NULL_NODE;
        yylhsminor.yy94.pending_name = yymsp[0].minor.yy356.pending_name;
      }
    }
      yymsp[0].minor.yy94 = yylhsminor.yy94;
      break;
    case 110: /* tconscomma ::= COMMA */
    {
      yymsp[0].minor.yy592 = 1;
    } break;
    case 111: /* tconscomma ::= */
    {
      yymsp[1].minor.yy592 = 0;
    } break;
    case 113: /* tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
    {
      yymsp[-6].minor.yy356.node = synq_parse_table_constraint(
          pCtx, SYNTAQLITE_TABLE_CONSTRAINT_TYPE_PRIMARY_KEY, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[0].minor.yy592,
          (SyntaqliteBool)yymsp[-2].minor.yy592, yymsp[-3].minor.yy141,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-6].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 114: /* tcons ::= UNIQUE LP sortlist RP onconf */
    {
      yymsp[-4].minor.yy356.node = synq_parse_table_constraint(
          pCtx, SYNTAQLITE_TABLE_CONSTRAINT_TYPE_UNIQUE, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[0].minor.yy592, SYNTAQLITE_BOOL_FALSE,
          yymsp[-2].minor.yy141, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE);
      yymsp[-4].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 115: /* tcons ::= CHECK LP expr RP onconf */
    {
      yymsp[-4].minor.yy356.node = synq_parse_table_constraint(
          pCtx, SYNTAQLITE_TABLE_CONSTRAINT_TYPE_CHECK, SYNQ_NO_SPAN,
          (SyntaqliteConflictAction)yymsp[0].minor.yy592, SYNTAQLITE_BOOL_FALSE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, yymsp[-2].minor.yy141,
          SYNTAQLITE_NULL_NODE);
      yymsp[-4].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 116: /* tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt
                 refargs defer_subclause_opt */
    {
      SyntaqliteForeignKeyAction on_del =
          (SyntaqliteForeignKeyAction)(yymsp[-1].minor.yy592 & 0xff);
      SyntaqliteForeignKeyAction on_upd =
          (SyntaqliteForeignKeyAction)((yymsp[-1].minor.yy592 >> 8) & 0xff);
      uint32_t fk = synq_parse_foreign_key_clause(
          pCtx, synq_span(pCtx, yymsp[-3].minor.yy0), yymsp[-2].minor.yy141,
          on_del, on_upd, (SyntaqliteBool)yymsp[0].minor.yy592);
      yymsp[-9].minor.yy356.node = synq_parse_table_constraint(
          pCtx, SYNTAQLITE_TABLE_CONSTRAINT_TYPE_FOREIGN_KEY, SYNQ_NO_SPAN,
          SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_BOOL_FALSE,
          SYNTAQLITE_NULL_NODE, yymsp[-6].minor.yy141, SYNTAQLITE_NULL_NODE,
          fk);
      yymsp[-9].minor.yy356.pending_name = SYNQ_NO_SPAN;
    } break;
    case 119: /* onconf ::= */
    case 146: /* orconf ::= */
      yytestcase(yyruleno == 146);
      {
        yymsp[1].minor.yy592 = (int)SYNTAQLITE_CONFLICT_ACTION_DEFAULT;
      }
      break;
    case 120: /* onconf ::= ON CONFLICT resolvetype */
    {
      yymsp[-2].minor.yy592 = yymsp[0].minor.yy592;
    } break;
    case 121: /* scantok ::= */
    case 155: /* indexed_opt ::= */
      yytestcase(yyruleno == 155);
    case 263: /* scanpt ::= */
      yytestcase(yyruleno == 263);
      {
        yymsp[1].minor.yy0.z = NULL;
        yymsp[1].minor.yy0.n = 0;
      }
      break;
    case 122: /* select ::= WITH wqlist selectnowith */
    {
      yymsp[-2].minor.yy141 = synq_parse_with_clause(
          pCtx, 0, yymsp[-1].minor.yy141, yymsp[0].minor.yy141);
    } break;
    case 123: /* select ::= WITH RECURSIVE wqlist selectnowith */
    {
      yymsp[-3].minor.yy141 = synq_parse_with_clause(
          pCtx, 1, yymsp[-1].minor.yy141, yymsp[0].minor.yy141);
    } break;
    case 124: /* wqitem ::= withnm eidlist_opt wqas LP select RP */
    {
      yylhsminor.yy141 = synq_parse_cte_definition(
          pCtx, synq_span_dequote(pCtx, yymsp[-5].minor.yy0),
          (SyntaqliteMaterialized)yymsp[-3].minor.yy592, yymsp[-4].minor.yy141,
          yymsp[-1].minor.yy141);
    }
      yymsp[-5].minor.yy141 = yylhsminor.yy141;
      break;
    case 125: /* wqlist ::= wqitem */
    {
      yylhsminor.yy141 =
          synq_parse_cte_list(pCtx, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy141);
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 126: /* wqlist ::= wqlist COMMA wqitem */
    {
      yymsp[-2].minor.yy141 = synq_parse_cte_list(pCtx, yymsp[-2].minor.yy141,
                                                  yymsp[0].minor.yy141);
    } break;
    case 127: /* withnm ::= nm */
    {
      // Token passthrough - nm already produces SynqParseToken
    } break;
    case 128: /* wqas ::= AS */
    {
      yymsp[0].minor.yy592 = (int)SYNTAQLITE_MATERIALIZED_DEFAULT;
    } break;
    case 129: /* wqas ::= AS MATERIALIZED */
    {
      yymsp[-1].minor.yy592 = (int)SYNTAQLITE_MATERIALIZED_MATERIALIZED;
    } break;
    case 130: /* wqas ::= AS NOT MATERIALIZED */
    {
      yymsp[-2].minor.yy592 = (int)SYNTAQLITE_MATERIALIZED_NOT_MATERIALIZED;
    } break;
    case 132: /* eidlist_opt ::= LP eidlist RP */
    case 166: /* idlist_opt ::= LP idlist RP */
      yytestcase(yyruleno == 166);
    case 177: /* expr ::= LP expr RP */
      yytestcase(yyruleno == 177);
    case 324: /* trigger_cmd ::= scanpt select scanpt */
      yytestcase(yyruleno == 324);
      {
        yymsp[-2].minor.yy141 = yymsp[-1].minor.yy141;
      }
      break;
    case 133: /* eidlist ::= nm collate sortorder */
    {
      (void)yymsp[-1].minor.yy592;
      (void)yymsp[0].minor.yy141;
      uint32_t col = synq_parse_column_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[-2].minor.yy0), SYNQ_NO_SPAN,
          SYNQ_NO_SPAN);
      yylhsminor.yy141 = synq_parse_expr_list(pCtx, SYNTAQLITE_NULL_NODE, col);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 134: /* eidlist ::= eidlist COMMA nm collate sortorder */
    {
      (void)yymsp[-1].minor.yy592;
      (void)yymsp[0].minor.yy141;
      uint32_t col = synq_parse_column_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[-2].minor.yy0), SYNQ_NO_SPAN,
          SYNQ_NO_SPAN);
      yymsp[-4].minor.yy141 =
          synq_parse_expr_list(pCtx, yymsp[-4].minor.yy141, col);
    } break;
    case 137: /* with ::= */
    {
      yymsp[1].minor.yy95.cte_list = SYNTAQLITE_NULL_NODE;
      yymsp[1].minor.yy95.is_recursive = 0;
    } break;
    case 138: /* with ::= WITH wqlist */
    {
      yymsp[-1].minor.yy95.cte_list = yymsp[0].minor.yy141;
      yymsp[-1].minor.yy95.is_recursive = 0;
    } break;
    case 139: /* with ::= WITH RECURSIVE wqlist */
    {
      yymsp[-2].minor.yy95.cte_list = yymsp[0].minor.yy141;
      yymsp[-2].minor.yy95.is_recursive = 1;
    } break;
    case 140: /* cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret
                 orderby_opt limit_opt */
    {
      if (yymsp[-1].minor.yy141 != SYNTAQLITE_NULL_NODE ||
          yymsp[0].minor.yy141 != SYNTAQLITE_NULL_NODE) {
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
      uint32_t del = synq_parse_delete_stmt(
          pCtx, yymsp[-4].minor.yy141, ih, synq_span(pCtx, yymsp[-3].minor.yy0),
          yymsp[-2].minor.yy5.where_expr, yymsp[-1].minor.yy141,
          yymsp[0].minor.yy141, yymsp[-2].minor.yy5.returning);
      if (yymsp[-7].minor.yy95.cte_list != SYNTAQLITE_NULL_NODE) {
        yylhsminor.yy141 =
            synq_parse_with_clause(pCtx, yymsp[-7].minor.yy95.is_recursive,
                                   yymsp[-7].minor.yy95.cte_list, del);
      } else {
        yylhsminor.yy141 = del;
      }
    }
      yymsp[-7].minor.yy141 = yylhsminor.yy141;
      break;
    case 141: /* cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist
                 from where_opt_ret orderby_opt limit_opt */
    {
      if (yymsp[-1].minor.yy141 != SYNTAQLITE_NULL_NODE ||
          yymsp[0].minor.yy141 != SYNTAQLITE_NULL_NODE) {
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
      uint32_t upd = synq_parse_update_stmt(
          pCtx, (SyntaqliteConflictAction)yymsp[-8].minor.yy592,
          yymsp[-7].minor.yy141, ih, synq_span(pCtx, yymsp[-6].minor.yy0),
          yymsp[-4].minor.yy141, yymsp[-3].minor.yy141,
          yymsp[-2].minor.yy5.where_expr, yymsp[-1].minor.yy141,
          yymsp[0].minor.yy141, yymsp[-2].minor.yy5.returning);
      if (yymsp[-10].minor.yy95.cte_list != SYNTAQLITE_NULL_NODE) {
        yylhsminor.yy141 =
            synq_parse_with_clause(pCtx, yymsp[-10].minor.yy95.is_recursive,
                                   yymsp[-10].minor.yy95.cte_list, upd);
      } else {
        yylhsminor.yy141 = upd;
      }
    }
      yymsp[-10].minor.yy141 = yylhsminor.yy141;
      break;
    case 142: /* cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert
               */
    {
      uint32_t ins = synq_parse_insert_stmt(
          pCtx, (SyntaqliteConflictAction)yymsp[-5].minor.yy592,
          yymsp[-3].minor.yy141, yymsp[-2].minor.yy141, yymsp[-1].minor.yy141,
          yymsp[0].minor.yy336.clauses, yymsp[0].minor.yy336.returning);
      if (yymsp[-6].minor.yy95.cte_list != SYNTAQLITE_NULL_NODE) {
        yylhsminor.yy141 =
            synq_parse_with_clause(pCtx, yymsp[-6].minor.yy95.is_recursive,
                                   yymsp[-6].minor.yy95.cte_list, ins);
      } else {
        yylhsminor.yy141 = ins;
      }
    }
      yymsp[-6].minor.yy141 = yylhsminor.yy141;
      break;
    case 143: /* cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT
                 VALUES returning */
    {
      uint32_t ins = synq_parse_insert_stmt(
          pCtx, (SyntaqliteConflictAction)yymsp[-6].minor.yy592,
          yymsp[-4].minor.yy141, yymsp[-3].minor.yy141, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy141);
      if (yymsp[-7].minor.yy95.cte_list != SYNTAQLITE_NULL_NODE) {
        yylhsminor.yy141 =
            synq_parse_with_clause(pCtx, yymsp[-7].minor.yy95.is_recursive,
                                   yymsp[-7].minor.yy95.cte_list, ins);
      } else {
        yylhsminor.yy141 = ins;
      }
    }
      yymsp[-7].minor.yy141 = yylhsminor.yy141;
      break;
    case 145: /* insert_cmd ::= REPLACE */
    case 150: /* resolvetype ::= REPLACE */
      yytestcase(yyruleno == 150);
      {
        yymsp[0].minor.yy592 = (int)SYNTAQLITE_CONFLICT_ACTION_REPLACE;
      }
      break;
    case 148: /* resolvetype ::= raisetype */
    {
      // raisetype: ROLLBACK=1, ABORT=2, FAIL=3 (SynqRaiseType enum values)
      // ConflictAction: ROLLBACK=1, ABORT=2, FAIL=3 (same values, direct
      // passthrough)
      yylhsminor.yy592 = yymsp[0].minor.yy592;
    }
      yymsp[0].minor.yy592 = yylhsminor.yy592;
      break;
    case 149: /* resolvetype ::= IGNORE */
    {
      yymsp[0].minor.yy592 = (int)SYNTAQLITE_CONFLICT_ACTION_IGNORE;
    } break;
    case 151: /* xfullname ::= nm */
    {
      yylhsminor.yy141 = synq_parse_table_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0), SYNQ_NO_SPAN,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 152: /* xfullname ::= nm DOT nm */
    {
      yylhsminor.yy141 = synq_parse_table_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0),
          synq_span_dequote(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 153: /* xfullname ::= nm DOT nm AS nm */
    {
      uint32_t alias = synq_parse_ident_name(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0));
      yylhsminor.yy141 = synq_parse_table_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[-2].minor.yy0),
          synq_span_dequote(pCtx, yymsp[-4].minor.yy0), alias,
          SYNTAQLITE_NULL_NODE);
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 154: /* xfullname ::= nm AS nm */
    {
      uint32_t alias = synq_parse_ident_name(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0));
      yylhsminor.yy141 = synq_parse_table_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[-2].minor.yy0), SYNQ_NO_SPAN,
          alias, SYNTAQLITE_NULL_NODE);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
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
      yymsp[1].minor.yy5.where_expr = SYNTAQLITE_NULL_NODE;
      yymsp[1].minor.yy5.returning = SYNTAQLITE_NULL_NODE;
    } break;
    case 158: /* where_opt_ret ::= WHERE expr */
    {
      yymsp[-1].minor.yy5.where_expr = yymsp[0].minor.yy141;
      yymsp[-1].minor.yy5.returning = SYNTAQLITE_NULL_NODE;
    } break;
    case 159: /* where_opt_ret ::= RETURNING selcollist */
    {
      yymsp[-1].minor.yy5.where_expr = SYNTAQLITE_NULL_NODE;
      yymsp[-1].minor.yy5.returning = yymsp[0].minor.yy141;
    } break;
    case 160: /* where_opt_ret ::= WHERE expr RETURNING selcollist */
    {
      yymsp[-3].minor.yy5.where_expr = yymsp[-2].minor.yy141;
      yymsp[-3].minor.yy5.returning = yymsp[0].minor.yy141;
    } break;
    case 161: /* setlist ::= setlist COMMA nm EQ expr */
    {
      uint32_t clause =
          synq_parse_set_clause(pCtx, synq_span(pCtx, yymsp[-2].minor.yy0),
                                SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy141);
      yylhsminor.yy141 =
          synq_parse_set_clause_list(pCtx, yymsp[-4].minor.yy141, clause);
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 162: /* setlist ::= setlist COMMA LP idlist RP EQ expr */
    {
      uint32_t clause = synq_parse_set_clause(
          pCtx, SYNQ_NO_SPAN, yymsp[-3].minor.yy141, yymsp[0].minor.yy141);
      yylhsminor.yy141 =
          synq_parse_set_clause_list(pCtx, yymsp[-6].minor.yy141, clause);
    }
      yymsp[-6].minor.yy141 = yylhsminor.yy141;
      break;
    case 163: /* setlist ::= nm EQ expr */
    {
      uint32_t clause =
          synq_parse_set_clause(pCtx, synq_span(pCtx, yymsp[-2].minor.yy0),
                                SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy141);
      yylhsminor.yy141 =
          synq_parse_set_clause_list(pCtx, SYNTAQLITE_NULL_NODE, clause);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 164: /* setlist ::= LP idlist RP EQ expr */
    {
      uint32_t clause = synq_parse_set_clause(
          pCtx, SYNQ_NO_SPAN, yymsp[-3].minor.yy141, yymsp[0].minor.yy141);
      yymsp[-4].minor.yy141 =
          synq_parse_set_clause_list(pCtx, SYNTAQLITE_NULL_NODE, clause);
    } break;
    case 167: /* upsert ::= */
    {
      yymsp[1].minor.yy336.clauses = SYNTAQLITE_NULL_NODE;
      yymsp[1].minor.yy336.returning = SYNTAQLITE_NULL_NODE;
    } break;
    case 168: /* upsert ::= RETURNING selcollist */
    {
      yymsp[-1].minor.yy336.clauses = SYNTAQLITE_NULL_NODE;
      yymsp[-1].minor.yy336.returning = yymsp[0].minor.yy141;
    } break;
    case 169: /* upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET
                 setlist where_opt upsert */
    {
      uint32_t clause = synq_parse_upsert_clause(
          pCtx, yymsp[-8].minor.yy141, yymsp[-6].minor.yy141,
          (SyntaqliteUpsertAction)SYNTAQLITE_UPSERT_ACTION_UPDATE,
          yymsp[-2].minor.yy141, yymsp[-1].minor.yy141);
      yymsp[-11].minor.yy336.clauses = synq_parse_upsert_clause_list(
          pCtx, yymsp[0].minor.yy336.clauses, clause);
      yymsp[-11].minor.yy336.returning = yymsp[0].minor.yy336.returning;
    } break;
    case 170: /* upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING
                 upsert */
    {
      uint32_t clause = synq_parse_upsert_clause(
          pCtx, yymsp[-5].minor.yy141, yymsp[-3].minor.yy141,
          (SyntaqliteUpsertAction)SYNTAQLITE_UPSERT_ACTION_NOTHING,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-8].minor.yy336.clauses = synq_parse_upsert_clause_list(
          pCtx, yymsp[0].minor.yy336.clauses, clause);
      yymsp[-8].minor.yy336.returning = yymsp[0].minor.yy336.returning;
    } break;
    case 171: /* upsert ::= ON CONFLICT DO NOTHING returning */
    {
      uint32_t clause = synq_parse_upsert_clause(
          pCtx, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
          (SyntaqliteUpsertAction)SYNTAQLITE_UPSERT_ACTION_NOTHING,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-4].minor.yy336.clauses =
          synq_parse_upsert_clause_list(pCtx, SYNTAQLITE_NULL_NODE, clause);
      yymsp[-4].minor.yy336.returning = yymsp[0].minor.yy141;
    } break;
    case 172: /* upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt
                 returning */
    {
      uint32_t clause = synq_parse_upsert_clause(
          pCtx, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
          (SyntaqliteUpsertAction)SYNTAQLITE_UPSERT_ACTION_UPDATE,
          yymsp[-2].minor.yy141, yymsp[-1].minor.yy141);
      yymsp[-7].minor.yy336.clauses =
          synq_parse_upsert_clause_list(pCtx, SYNTAQLITE_NULL_NODE, clause);
      yymsp[-7].minor.yy336.returning = yymsp[0].minor.yy141;
    } break;
    case 175: /* expr ::= error */
    case 202: /* nmorerr ::= error */
      yytestcase(yyruleno == 202);
      {
        yymsp[0].minor.yy141 = synq_parse_error(pCtx, synq_error_span(pCtx));
      }
      break;
    case 178: /* expr ::= expr PLUS|MINUS expr */
    {
      SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == SYNTAQLITE_TK_PLUS)
                                  ? SYNTAQLITE_BINARY_OP_PLUS
                                  : SYNTAQLITE_BINARY_OP_MINUS;
      yylhsminor.yy141 = synq_parse_binary_expr(pCtx, op, yymsp[-2].minor.yy141,
                                                yymsp[0].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
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
      yylhsminor.yy141 = synq_parse_binary_expr(pCtx, op, yymsp[-2].minor.yy141,
                                                yymsp[0].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
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
      yylhsminor.yy141 = synq_parse_binary_expr(pCtx, op, yymsp[-2].minor.yy141,
                                                yymsp[0].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 181: /* expr ::= expr EQ|NE expr */
    {
      SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == SYNTAQLITE_TK_EQ)
                                  ? SYNTAQLITE_BINARY_OP_EQ
                                  : SYNTAQLITE_BINARY_OP_NE;
      yylhsminor.yy141 = synq_parse_binary_expr(pCtx, op, yymsp[-2].minor.yy141,
                                                yymsp[0].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 182: /* expr ::= expr AND expr */
    {
      yylhsminor.yy141 =
          synq_parse_binary_expr(pCtx, SYNTAQLITE_BINARY_OP_AND,
                                 yymsp[-2].minor.yy141, yymsp[0].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 183: /* expr ::= expr OR expr */
    {
      yylhsminor.yy141 =
          synq_parse_binary_expr(pCtx, SYNTAQLITE_BINARY_OP_OR,
                                 yymsp[-2].minor.yy141, yymsp[0].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
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
      yylhsminor.yy141 = synq_parse_binary_expr(pCtx, op, yymsp[-2].minor.yy141,
                                                yymsp[0].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 185: /* expr ::= expr CONCAT expr */
    {
      yylhsminor.yy141 =
          synq_parse_binary_expr(pCtx, SYNTAQLITE_BINARY_OP_CONCAT,
                                 yymsp[-2].minor.yy141, yymsp[0].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 186: /* expr ::= expr PTR expr */
    {
      SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.n == 3)
                                  ? SYNTAQLITE_BINARY_OP_PTR2
                                  : SYNTAQLITE_BINARY_OP_PTR;
      yylhsminor.yy141 = synq_parse_binary_expr(pCtx, op, yymsp[-2].minor.yy141,
                                                yymsp[0].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 187: /* expr ::= PLUS|MINUS expr */
    {
      SyntaqliteUnaryOp op = (yymsp[-1].minor.yy0.type == SYNTAQLITE_TK_MINUS)
                                 ? SYNTAQLITE_UNARY_OP_MINUS
                                 : SYNTAQLITE_UNARY_OP_PLUS;
      yylhsminor.yy141 = synq_parse_unary_expr(pCtx, op, yymsp[0].minor.yy141);
    }
      yymsp[-1].minor.yy141 = yylhsminor.yy141;
      break;
    case 188: /* expr ::= BITNOT expr */
    {
      yymsp[-1].minor.yy141 = synq_parse_unary_expr(
          pCtx, SYNTAQLITE_UNARY_OP_BIT_NOT, yymsp[0].minor.yy141);
    } break;
    case 189: /* expr ::= NOT expr */
    {
      yymsp[-1].minor.yy141 = synq_parse_unary_expr(
          pCtx, SYNTAQLITE_UNARY_OP_NOT, yymsp[0].minor.yy141);
    } break;
    case 192: /* nexprlist ::= nexprlist COMMA expr */
    {
      yylhsminor.yy141 = synq_parse_expr_list(pCtx, yymsp[-2].minor.yy141,
                                              yymsp[0].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 193: /* nexprlist ::= expr */
    {
      yylhsminor.yy141 = synq_parse_expr_list(pCtx, SYNTAQLITE_NULL_NODE,
                                              yymsp[0].minor.yy141);
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 194: /* expr ::= LP nexprlist COMMA expr RP */
    {
      yymsp[-4].minor.yy141 = synq_parse_expr_list(pCtx, yymsp[-3].minor.yy141,
                                                   yymsp[-1].minor.yy141);
    } break;
    case 195: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
    {
      synq_mark_as_function(pCtx, yymsp[-4].minor.yy0);
      yylhsminor.yy141 = synq_parse_function_call(
          pCtx, synq_span(pCtx, yymsp[-4].minor.yy0),
          (SyntaqliteFunctionCallFlags){.raw = (uint8_t)yymsp[-2].minor.yy141},
          yymsp[-1].minor.yy141, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 196: /* expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
    {
      synq_mark_as_function(pCtx, yymsp[-3].minor.yy0);
      yylhsminor.yy141 = synq_parse_function_call(
          pCtx, synq_span(pCtx, yymsp[-3].minor.yy0),
          (SyntaqliteFunctionCallFlags){.bits = {.star = 1}},
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    }
      yymsp[-3].minor.yy141 = yylhsminor.yy141;
      break;
    case 197: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over
               */
    {
      SyntaqliteFilterOver* fo = (SyntaqliteFilterOver*)synq_arena_ptr(
          &pCtx->ast, yymsp[0].minor.yy141);
      synq_mark_as_function(pCtx, yymsp[-5].minor.yy0);
      yylhsminor.yy141 = synq_parse_function_call(
          pCtx, synq_span(pCtx, yymsp[-5].minor.yy0),
          (SyntaqliteFunctionCallFlags){.raw = (uint8_t)yymsp[-3].minor.yy141},
          yymsp[-2].minor.yy141, fo->filter_expr, fo->over_def);
    }
      yymsp[-5].minor.yy141 = yylhsminor.yy141;
      break;
    case 198: /* expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
    {
      SyntaqliteFilterOver* fo = (SyntaqliteFilterOver*)synq_arena_ptr(
          &pCtx->ast, yymsp[0].minor.yy141);
      synq_mark_as_function(pCtx, yymsp[-4].minor.yy0);
      yylhsminor.yy141 = synq_parse_function_call(
          pCtx, synq_span(pCtx, yymsp[-4].minor.yy0),
          (SyntaqliteFunctionCallFlags){.bits = {.star = 1}},
          SYNTAQLITE_NULL_NODE, fo->filter_expr, fo->over_def);
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 199: /* nm ::= ID|INDEXED|JOIN_KW */
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
        yylhsminor.yy141 = synq_parse_ident_name(
            pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0));
      }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 203: /* term ::= INTEGER */
    {
      yylhsminor.yy141 =
          synq_parse_literal(pCtx, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                             synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 204: /* term ::= STRING */
    {
      yylhsminor.yy141 =
          synq_parse_literal(pCtx, SYNTAQLITE_LITERAL_TYPE_STRING,
                             synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
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
      yylhsminor.yy141 = synq_parse_literal(
          pCtx, lit_type, synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 206: /* term ::= QNUMBER */
    {
      yylhsminor.yy141 =
          synq_parse_literal(pCtx, SYNTAQLITE_LITERAL_TYPE_QNUMBER,
                             synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 207: /* term ::= CTIME_KW */
    {
      yylhsminor.yy141 =
          synq_parse_literal(pCtx, SYNTAQLITE_LITERAL_TYPE_CURRENT,
                             synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 208: /* expr ::= VARIABLE */
    {
      yylhsminor.yy141 =
          synq_parse_variable(pCtx, synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 209: /* expr ::= expr COLLATE ID|STRING */
    {
      yylhsminor.yy141 = synq_parse_collate_expr(
          pCtx, yymsp[-2].minor.yy141, synq_span(pCtx, yymsp[0].minor.yy0));
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 210: /* sortlist ::= sortlist COMMA expr sortorder nulls */
    {
      uint32_t term =
          synq_parse_ordering_term(pCtx, yymsp[-2].minor.yy141,
                                   (SyntaqliteSortOrder)yymsp[-1].minor.yy141,
                                   (SyntaqliteNullsOrder)yymsp[0].minor.yy141);
      yylhsminor.yy141 =
          synq_parse_order_by_list(pCtx, yymsp[-4].minor.yy141, term);
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 211: /* sortlist ::= expr sortorder nulls */
    {
      uint32_t term =
          synq_parse_ordering_term(pCtx, yymsp[-2].minor.yy141,
                                   (SyntaqliteSortOrder)yymsp[-1].minor.yy141,
                                   (SyntaqliteNullsOrder)yymsp[0].minor.yy141);
      yylhsminor.yy141 =
          synq_parse_order_by_list(pCtx, SYNTAQLITE_NULL_NODE, term);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 213: /* sortorder ::= DESC */
    case 267: /* distinct ::= DISTINCT */
      yytestcase(yyruleno == 267);
      {
        yymsp[0].minor.yy141 = 1;
      }
      break;
    case 214: /* sortorder ::= */
    case 217: /* nulls ::= */
      yytestcase(yyruleno == 217);
    case 269: /* distinct ::= */
      yytestcase(yyruleno == 269);
      {
        yymsp[1].minor.yy141 = 0;
      }
      break;
    case 216: /* nulls ::= NULLS LAST */
    {
      yymsp[-1].minor.yy141 = 2;
    } break;
    case 218: /* expr ::= RAISE LP IGNORE RP */
    {
      yymsp[-3].minor.yy141 = synq_parse_raise_expr(
          pCtx, SYNTAQLITE_RAISE_TYPE_IGNORE, SYNTAQLITE_NULL_NODE);
    } break;
    case 219: /* expr ::= RAISE LP raisetype COMMA expr RP */
    {
      yymsp[-5].minor.yy141 = synq_parse_raise_expr(
          pCtx, (SyntaqliteRaiseType)yymsp[-3].minor.yy592,
          yymsp[-1].minor.yy141);
    } break;
    case 220: /* raisetype ::= ROLLBACK */
    {
      yymsp[0].minor.yy592 = SYNTAQLITE_RAISE_TYPE_ROLLBACK;
    } break;
    case 221: /* raisetype ::= ABORT */
    {
      yymsp[0].minor.yy592 = SYNTAQLITE_RAISE_TYPE_ABORT;
    } break;
    case 222: /* raisetype ::= FAIL */
    {
      yymsp[0].minor.yy592 = SYNTAQLITE_RAISE_TYPE_FAIL;
    } break;
    case 223: /* fullname ::= nmorerr */
    {
      yylhsminor.yy141 = synq_parse_qualified_name(pCtx, yymsp[0].minor.yy141,
                                                   SYNTAQLITE_NULL_NODE);
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 224: /* fullname ::= nmorerr DOT nmorerr */
    {
      yylhsminor.yy141 = synq_parse_qualified_name(pCtx, yymsp[0].minor.yy141,
                                                   yymsp[-2].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 227: /* cmd ::= DROP TABLE ifexists fullname */
    {
      yymsp[-3].minor.yy141 = synq_parse_drop_stmt(
          pCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TABLE,
          (SyntaqliteBool)yymsp[-1].minor.yy592, yymsp[0].minor.yy141);
    } break;
    case 228: /* cmd ::= DROP VIEW ifexists fullname */
    {
      yymsp[-3].minor.yy141 = synq_parse_drop_stmt(
          pCtx, SYNTAQLITE_DROP_OBJECT_TYPE_VIEW,
          (SyntaqliteBool)yymsp[-1].minor.yy592, yymsp[0].minor.yy141);
    } break;
    case 229: /* cmd ::= DROP INDEX ifexists fullname */
    {
      yymsp[-3].minor.yy141 = synq_parse_drop_stmt(
          pCtx, SYNTAQLITE_DROP_OBJECT_TYPE_INDEX,
          (SyntaqliteBool)yymsp[-1].minor.yy592, yymsp[0].minor.yy141);
    } break;
    case 230: /* cmd ::= DROP TRIGGER ifexists fullname */
    {
      yymsp[-3].minor.yy141 = synq_parse_drop_stmt(
          pCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TRIGGER,
          (SyntaqliteBool)yymsp[-1].minor.yy592, yymsp[0].minor.yy141);
    } break;
    case 231: /* cmd ::= ALTER TABLE fullname RENAME TO nmorerr */
    {
      yymsp[-5].minor.yy141 = synq_parse_alter_table_stmt(
          pCtx, SYNTAQLITE_ALTER_OP_RENAME_TABLE, yymsp[-3].minor.yy141,
          yymsp[0].minor.yy141, SYNTAQLITE_NULL_NODE);
    } break;
    case 232: /* cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nmorerr TO
                 nmorerr */
    {
      yymsp[-7].minor.yy141 = synq_parse_alter_table_stmt(
          pCtx, SYNTAQLITE_ALTER_OP_RENAME_COLUMN, yymsp[-5].minor.yy141,
          yymsp[0].minor.yy141, yymsp[-2].minor.yy141);
    } break;
    case 233: /* cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nmorerr */
    {
      yymsp[-5].minor.yy141 = synq_parse_alter_table_stmt(
          pCtx, SYNTAQLITE_ALTER_OP_DROP_COLUMN, yymsp[-3].minor.yy141,
          SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy141);
    } break;
    case 234: /* cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt
                 columnname carglist */
    {
      yymsp[-6].minor.yy141 = synq_parse_alter_table_stmt(
          pCtx, SYNTAQLITE_ALTER_OP_ADD_COLUMN, yymsp[-4].minor.yy141,
          SYNTAQLITE_NULL_NODE, yymsp[-1].minor.yy452.name);
    } break;
    case 238: /* columnname ::= nmorerr typetoken */
    {
      yylhsminor.yy452.name = yymsp[-1].minor.yy141;
      yylhsminor.yy452.typetoken = yymsp[0].minor.yy0.z
                                       ? synq_span(pCtx, yymsp[0].minor.yy0)
                                       : SYNQ_NO_SPAN;
    }
      yymsp[-1].minor.yy452 = yylhsminor.yy452;
      break;
    case 239: /* cmd ::= BEGIN transtype trans_opt */
    {
      yymsp[-2].minor.yy141 = synq_parse_transaction_stmt(
          pCtx, SYNTAQLITE_TRANSACTION_OP_BEGIN,
          (SyntaqliteTransactionType)yymsp[-1].minor.yy592);
    } break;
    case 240: /* cmd ::= COMMIT|END trans_opt */
    {
      yymsp[-1].minor.yy141 =
          synq_parse_transaction_stmt(pCtx, SYNTAQLITE_TRANSACTION_OP_COMMIT,
                                      SYNTAQLITE_TRANSACTION_TYPE_DEFERRED);
    } break;
    case 241: /* cmd ::= ROLLBACK trans_opt */
    {
      yymsp[-1].minor.yy141 =
          synq_parse_transaction_stmt(pCtx, SYNTAQLITE_TRANSACTION_OP_ROLLBACK,
                                      SYNTAQLITE_TRANSACTION_TYPE_DEFERRED);
    } break;
    case 242: /* transtype ::= */
    {
      yymsp[1].minor.yy592 = (int)SYNTAQLITE_TRANSACTION_TYPE_DEFERRED;
    } break;
    case 243: /* transtype ::= DEFERRED */
    {
      yymsp[0].minor.yy592 = (int)SYNTAQLITE_TRANSACTION_TYPE_DEFERRED;
    } break;
    case 244: /* transtype ::= IMMEDIATE */
    {
      yymsp[0].minor.yy592 = (int)SYNTAQLITE_TRANSACTION_TYPE_IMMEDIATE;
    } break;
    case 245: /* transtype ::= EXCLUSIVE */
    {
      yymsp[0].minor.yy592 = (int)SYNTAQLITE_TRANSACTION_TYPE_EXCLUSIVE;
    } break;
    case 247: /* trans_opt ::= TRANSACTION */
    case 249: /* savepoint_opt ::= SAVEPOINT */
      yytestcase(yyruleno == 249);
      {
        yymsp[0].minor.yy592 = 0;
      }
      break;
    case 251: /* cmd ::= SAVEPOINT nmorerr */
    {
      yymsp[-1].minor.yy141 = synq_parse_savepoint_stmt(
          pCtx, SYNTAQLITE_SAVEPOINT_OP_SAVEPOINT, yymsp[0].minor.yy141);
    } break;
    case 252: /* cmd ::= RELEASE savepoint_opt nmorerr */
    {
      yymsp[-2].minor.yy141 = synq_parse_savepoint_stmt(
          pCtx, SYNTAQLITE_SAVEPOINT_OP_RELEASE, yymsp[0].minor.yy141);
    } break;
    case 253: /* cmd ::= ROLLBACK trans_opt TO savepoint_opt nmorerr */
    {
      yymsp[-4].minor.yy141 = synq_parse_savepoint_stmt(
          pCtx, SYNTAQLITE_SAVEPOINT_OP_ROLLBACK_TO, yymsp[0].minor.yy141);
    } break;
    case 257: /* oneselect ::= SELECT distinct selcollist from where_opt
                 groupby_opt having_opt orderby_opt limit_opt */
    {
      yymsp[-8].minor.yy141 = synq_parse_select_stmt(
          pCtx,
          (SyntaqliteSelectStmtFlags){.raw = (uint8_t)yymsp[-7].minor.yy141},
          yymsp[-6].minor.yy141, yymsp[-5].minor.yy141, yymsp[-4].minor.yy141,
          yymsp[-3].minor.yy141, yymsp[-2].minor.yy141, yymsp[-1].minor.yy141,
          yymsp[0].minor.yy141, SYNTAQLITE_NULL_NODE);
    } break;
    case 258: /* oneselect ::= SELECT distinct selcollist from where_opt
                 groupby_opt having_opt window_clause orderby_opt limit_opt */
    {
      yymsp[-9].minor.yy141 = synq_parse_select_stmt(
          pCtx,
          (SyntaqliteSelectStmtFlags){.raw = (uint8_t)yymsp[-8].minor.yy141},
          yymsp[-7].minor.yy141, yymsp[-6].minor.yy141, yymsp[-5].minor.yy141,
          yymsp[-4].minor.yy141, yymsp[-3].minor.yy141, yymsp[-1].minor.yy141,
          yymsp[0].minor.yy141, yymsp[-2].minor.yy141);
    } break;
    case 259: /* selcollist ::= sclp scanpt expr scanpt as */
    {
      uint32_t col =
          synq_parse_result_column(pCtx, (SyntaqliteResultColumnFlags){0},
                                   yymsp[0].minor.yy141, yymsp[-2].minor.yy141);
      yylhsminor.yy141 =
          synq_parse_result_column_list(pCtx, yymsp[-4].minor.yy141, col);
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 260: /* selcollist ::= sclp scanpt STAR */
    {
      uint32_t col = synq_parse_result_column(
          pCtx, (SyntaqliteResultColumnFlags){.bits = {.star = 1}},
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yylhsminor.yy141 =
          synq_parse_result_column_list(pCtx, yymsp[-2].minor.yy141, col);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 275: /* groupby_opt ::= GROUP BY nexprlist */
    case 279: /* orderby_opt ::= ORDER BY sortlist */
      yytestcase(yyruleno == 279);
      {
        yymsp[-2].minor.yy141 = yymsp[0].minor.yy141;
      }
      break;
    case 281: /* limit_opt ::= LIMIT expr */
    {
      yymsp[-1].minor.yy141 = synq_parse_limit_clause(
          pCtx, yymsp[0].minor.yy141, SYNTAQLITE_NULL_NODE);
    } break;
    case 282: /* limit_opt ::= LIMIT expr OFFSET expr */
    {
      yymsp[-3].minor.yy141 = synq_parse_limit_clause(
          pCtx, yymsp[-2].minor.yy141, yymsp[0].minor.yy141);
    } break;
    case 283: /* limit_opt ::= LIMIT expr COMMA expr */
    {
      yymsp[-3].minor.yy141 = synq_parse_limit_clause(
          pCtx, yymsp[0].minor.yy141, yymsp[-2].minor.yy141);
    } break;
    case 284: /* stl_prefix ::= seltablist joinop */
    {
      yymsp[-1].minor.yy141 =
          synq_parse_join_prefix(pCtx, yymsp[-1].minor.yy141,
                                 (SyntaqliteJoinType)yymsp[0].minor.yy592);
    } break;
    case 286: /* seltablist ::= stl_prefix nm dbnm as on_using */
    {
      uint32_t alias = yymsp[-1].minor.yy141;
      SyntaqliteSourceSpan table_name;
      SyntaqliteSourceSpan schema;
      if (yymsp[-2].minor.yy0.z != NULL) {
        table_name = synq_span_dequote(pCtx, yymsp[-2].minor.yy0);
        schema = synq_span_dequote(pCtx, yymsp[-3].minor.yy0);
      } else {
        table_name = synq_span_dequote(pCtx, yymsp[-3].minor.yy0);
        schema = SYNQ_NO_SPAN;
      }
      uint32_t tref = synq_parse_table_ref(pCtx, table_name, schema, alias,
                                           SYNTAQLITE_NULL_NODE);
      if (yymsp[-4].minor.yy141 == SYNTAQLITE_NULL_NODE) {
        yymsp[-4].minor.yy141 = tref;
      } else {
        SyntaqliteNode* pfx = AST_NODE(&pCtx->ast, yymsp[-4].minor.yy141);
        yymsp[-4].minor.yy141 = synq_parse_join_clause(
            pCtx, pfx->join_prefix.join_type, pfx->join_prefix.source, tref,
            yymsp[0].minor.yy216.on_expr, yymsp[0].minor.yy216.using_cols);
      }
    } break;
    case 287: /* seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
    {
      (void)yymsp[-1].minor.yy0;
      uint32_t alias = yymsp[-2].minor.yy141;
      SyntaqliteSourceSpan table_name;
      SyntaqliteSourceSpan schema;
      if (yymsp[-3].minor.yy0.z != NULL) {
        table_name = synq_span_dequote(pCtx, yymsp[-3].minor.yy0);
        schema = synq_span_dequote(pCtx, yymsp[-4].minor.yy0);
      } else {
        table_name = synq_span_dequote(pCtx, yymsp[-4].minor.yy0);
        schema = SYNQ_NO_SPAN;
      }
      uint32_t tref = synq_parse_table_ref(pCtx, table_name, schema, alias,
                                           SYNTAQLITE_NULL_NODE);
      if (yymsp[-5].minor.yy141 == SYNTAQLITE_NULL_NODE) {
        yymsp[-5].minor.yy141 = tref;
      } else {
        SyntaqliteNode* pfx = AST_NODE(&pCtx->ast, yymsp[-5].minor.yy141);
        yymsp[-5].minor.yy141 = synq_parse_join_clause(
            pCtx, pfx->join_prefix.join_type, pfx->join_prefix.source, tref,
            yymsp[0].minor.yy216.on_expr, yymsp[0].minor.yy216.using_cols);
      }
    } break;
    case 288: /* seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
    {
      uint32_t alias = yymsp[-1].minor.yy141;
      SyntaqliteSourceSpan table_name;
      SyntaqliteSourceSpan schema;
      if (yymsp[-5].minor.yy0.z != NULL) {
        table_name = synq_span_dequote(pCtx, yymsp[-5].minor.yy0);
        schema = synq_span_dequote(pCtx, yymsp[-6].minor.yy0);
      } else {
        table_name = synq_span_dequote(pCtx, yymsp[-6].minor.yy0);
        schema = SYNQ_NO_SPAN;
      }
      uint32_t tref = synq_parse_table_ref(pCtx, table_name, schema, alias,
                                           yymsp[-3].minor.yy141);
      if (yymsp[-7].minor.yy141 == SYNTAQLITE_NULL_NODE) {
        yymsp[-7].minor.yy141 = tref;
      } else {
        SyntaqliteNode* pfx = AST_NODE(&pCtx->ast, yymsp[-7].minor.yy141);
        yymsp[-7].minor.yy141 = synq_parse_join_clause(
            pCtx, pfx->join_prefix.join_type, pfx->join_prefix.source, tref,
            yymsp[0].minor.yy216.on_expr, yymsp[0].minor.yy216.using_cols);
      }
    } break;
    case 289: /* seltablist ::= stl_prefix LP select RP as on_using */
    {
      pCtx->saw_subquery = 1;
      uint32_t alias = yymsp[-1].minor.yy141;
      uint32_t sub =
          synq_parse_subquery_table_source(pCtx, yymsp[-3].minor.yy141, alias);
      if (yymsp[-5].minor.yy141 == SYNTAQLITE_NULL_NODE) {
        yymsp[-5].minor.yy141 = sub;
      } else {
        SyntaqliteNode* pfx = AST_NODE(&pCtx->ast, yymsp[-5].minor.yy141);
        yymsp[-5].minor.yy141 = synq_parse_join_clause(
            pCtx, pfx->join_prefix.join_type, pfx->join_prefix.source, sub,
            yymsp[0].minor.yy216.on_expr, yymsp[0].minor.yy216.using_cols);
      }
    } break;
    case 290: /* seltablist ::= stl_prefix LP seltablist RP as on_using */
    {
      (void)yymsp[-1].minor.yy141;
      (void)yymsp[0].minor.yy216;
      if (yymsp[-5].minor.yy141 == SYNTAQLITE_NULL_NODE) {
        yymsp[-5].minor.yy141 = yymsp[-3].minor.yy141;
      } else {
        SyntaqliteNode* pfx = AST_NODE(&pCtx->ast, yymsp[-5].minor.yy141);
        yymsp[-5].minor.yy141 = synq_parse_join_clause(
            pCtx, pfx->join_prefix.join_type, pfx->join_prefix.source,
            yymsp[-3].minor.yy141, yymsp[0].minor.yy216.on_expr,
            yymsp[0].minor.yy216.using_cols);
      }
    } break;
    case 291: /* joinop ::= COMMA|JOIN */
    {
      yylhsminor.yy592 = (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_COMMA)
                             ? (int)SYNTAQLITE_JOIN_TYPE_COMMA
                             : (int)SYNTAQLITE_JOIN_TYPE_INNER;
    }
      yymsp[0].minor.yy592 = yylhsminor.yy592;
      break;
    case 292: /* joinop ::= JOIN_KW JOIN */
    {
      // Single keyword: LEFT, RIGHT, INNER, OUTER, CROSS, NATURAL, FULL
      if (yymsp[-1].minor.yy0.n == 4 && (yymsp[-1].minor.yy0.z[0] == 'L' ||
                                         yymsp[-1].minor.yy0.z[0] == 'l')) {
        yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_LEFT;
      } else if (yymsp[-1].minor.yy0.n == 5 &&
                 (yymsp[-1].minor.yy0.z[0] == 'R' ||
                  yymsp[-1].minor.yy0.z[0] == 'r')) {
        yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_RIGHT;
      } else if (yymsp[-1].minor.yy0.n == 5 &&
                 (yymsp[-1].minor.yy0.z[0] == 'I' ||
                  yymsp[-1].minor.yy0.z[0] == 'i')) {
        yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_INNER;
      } else if (yymsp[-1].minor.yy0.n == 5 &&
                 (yymsp[-1].minor.yy0.z[0] == 'O' ||
                  yymsp[-1].minor.yy0.z[0] == 'o')) {
        // OUTER alone is not valid but treat as INNER
        yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_INNER;
      } else if (yymsp[-1].minor.yy0.n == 5 &&
                 (yymsp[-1].minor.yy0.z[0] == 'C' ||
                  yymsp[-1].minor.yy0.z[0] == 'c')) {
        yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_CROSS;
      } else if (yymsp[-1].minor.yy0.n == 7 &&
                 (yymsp[-1].minor.yy0.z[0] == 'N' ||
                  yymsp[-1].minor.yy0.z[0] == 'n')) {
        yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
      } else if (yymsp[-1].minor.yy0.n == 4 &&
                 (yymsp[-1].minor.yy0.z[0] == 'F' ||
                  yymsp[-1].minor.yy0.z[0] == 'f')) {
        yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_FULL;
      } else {
        yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_INNER;
      }
    }
      yymsp[-1].minor.yy592 = yylhsminor.yy592;
      break;
    case 293: /* joinop ::= JOIN_KW nm JOIN */
    {
      // Two keywords: LEFT OUTER, NATURAL LEFT, NATURAL RIGHT, etc.
      (void)yymsp[-1].minor.yy0;
      if (yymsp[-2].minor.yy0.n == 7 && (yymsp[-2].minor.yy0.z[0] == 'N' ||
                                         yymsp[-2].minor.yy0.z[0] == 'n')) {
        // NATURAL + something
        if (yymsp[-1].minor.yy0.n == 4 && (yymsp[-1].minor.yy0.z[0] == 'L' ||
                                           yymsp[-1].minor.yy0.z[0] == 'l')) {
          yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_LEFT;
        } else if (yymsp[-1].minor.yy0.n == 5 &&
                   (yymsp[-1].minor.yy0.z[0] == 'R' ||
                    yymsp[-1].minor.yy0.z[0] == 'r')) {
          yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_RIGHT;
        } else if (yymsp[-1].minor.yy0.n == 5 &&
                   (yymsp[-1].minor.yy0.z[0] == 'I' ||
                    yymsp[-1].minor.yy0.z[0] == 'i')) {
          yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
        } else if (yymsp[-1].minor.yy0.n == 4 &&
                   (yymsp[-1].minor.yy0.z[0] == 'F' ||
                    yymsp[-1].minor.yy0.z[0] == 'f')) {
          yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_FULL;
        } else if (yymsp[-1].minor.yy0.n == 5 &&
                   (yymsp[-1].minor.yy0.z[0] == 'C' ||
                    yymsp[-1].minor.yy0.z[0] == 'c')) {
          // NATURAL CROSS -> just CROSS
          yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_CROSS;
        } else {
          yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
        }
      } else if (yymsp[-2].minor.yy0.n == 4 &&
                 (yymsp[-2].minor.yy0.z[0] == 'L' ||
                  yymsp[-2].minor.yy0.z[0] == 'l')) {
        // LEFT OUTER
        yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_LEFT;
      } else if (yymsp[-2].minor.yy0.n == 5 &&
                 (yymsp[-2].minor.yy0.z[0] == 'R' ||
                  yymsp[-2].minor.yy0.z[0] == 'r')) {
        // RIGHT OUTER
        yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_RIGHT;
      } else if (yymsp[-2].minor.yy0.n == 4 &&
                 (yymsp[-2].minor.yy0.z[0] == 'F' ||
                  yymsp[-2].minor.yy0.z[0] == 'f')) {
        // FULL OUTER
        yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_FULL;
      } else {
        yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_INNER;
      }
    }
      yymsp[-2].minor.yy592 = yylhsminor.yy592;
      break;
    case 294: /* joinop ::= JOIN_KW nm nm JOIN */
    {
      // Three keywords: NATURAL LEFT OUTER, NATURAL RIGHT OUTER, etc.
      (void)yymsp[-2].minor.yy0;
      (void)yymsp[-1].minor.yy0;
      if (yymsp[-3].minor.yy0.n == 7 && (yymsp[-3].minor.yy0.z[0] == 'N' ||
                                         yymsp[-3].minor.yy0.z[0] == 'n')) {
        // NATURAL yylhsminor.yy592 OUTER
        if (yymsp[-2].minor.yy0.n == 4 && (yymsp[-2].minor.yy0.z[0] == 'L' ||
                                           yymsp[-2].minor.yy0.z[0] == 'l')) {
          yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_LEFT;
        } else if (yymsp[-2].minor.yy0.n == 5 &&
                   (yymsp[-2].minor.yy0.z[0] == 'R' ||
                    yymsp[-2].minor.yy0.z[0] == 'r')) {
          yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_RIGHT;
        } else if (yymsp[-2].minor.yy0.n == 4 &&
                   (yymsp[-2].minor.yy0.z[0] == 'F' ||
                    yymsp[-2].minor.yy0.z[0] == 'f')) {
          yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_FULL;
        } else {
          yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
        }
      } else {
        yylhsminor.yy592 = (int)SYNTAQLITE_JOIN_TYPE_INNER;
      }
    }
      yymsp[-3].minor.yy592 = yylhsminor.yy592;
      break;
    case 295: /* on_using ::= ON expr */
    {
      yymsp[-1].minor.yy216.on_expr = yymsp[0].minor.yy141;
      yymsp[-1].minor.yy216.using_cols = SYNTAQLITE_NULL_NODE;
    } break;
    case 296: /* on_using ::= USING LP idlist RP */
    {
      yymsp[-3].minor.yy216.on_expr = SYNTAQLITE_NULL_NODE;
      yymsp[-3].minor.yy216.using_cols = yymsp[-1].minor.yy141;
    } break;
    case 297: /* on_using ::= */
    {
      yymsp[1].minor.yy216.on_expr = SYNTAQLITE_NULL_NODE;
      yymsp[1].minor.yy216.using_cols = SYNTAQLITE_NULL_NODE;
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
      yymsp[-2].minor.yy141 =
          synq_parse_expr_list(pCtx, yymsp[-2].minor.yy141, col);
    } break;
    case 301: /* idlist ::= nm */
    {
      uint32_t col = synq_parse_column_ref(
          pCtx, synq_span_dequote(pCtx, yymsp[0].minor.yy0), SYNQ_NO_SPAN,
          SYNQ_NO_SPAN);
      yylhsminor.yy141 = synq_parse_expr_list(pCtx, SYNTAQLITE_NULL_NODE, col);
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 302: /* cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
    {
      // yymsp[-3].minor.yy141 is a partially-built CreateTriggerStmt, fill in
      // the body
      SyntaqliteNode* trig = AST_NODE(&pCtx->ast, yymsp[-3].minor.yy141);
      trig->create_trigger_stmt.body = yymsp[-1].minor.yy141;
      yymsp[-4].minor.yy141 = yymsp[-3].minor.yy141;
    } break;
    case 303: /* trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time
                 trigger_event ON fullname foreach_clause when_clause */
    {
      SyntaqliteSourceSpan trig_name =
          yymsp[-6].minor.yy0.z ? synq_span(pCtx, yymsp[-6].minor.yy0)
                                : synq_span(pCtx, yymsp[-7].minor.yy0);
      SyntaqliteSourceSpan trig_schema =
          yymsp[-6].minor.yy0.z ? synq_span(pCtx, yymsp[-7].minor.yy0)
                                : SYNQ_NO_SPAN;
      yylhsminor.yy141 = synq_parse_create_trigger_stmt(
          pCtx, trig_name, trig_schema, (SyntaqliteBool)yymsp[-10].minor.yy592,
          (SyntaqliteBool)yymsp[-8].minor.yy592,
          (SyntaqliteTriggerTiming)yymsp[-5].minor.yy592, yymsp[-4].minor.yy141,
          yymsp[-2].minor.yy141, yymsp[0].minor.yy141,
          SYNTAQLITE_NULL_NODE);  // body filled in by cmd rule
    }
      yymsp[-10].minor.yy141 = yylhsminor.yy141;
      break;
    case 304: /* trigger_time ::= BEFORE|AFTER */
    {
      yylhsminor.yy592 = (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_BEFORE)
                             ? (int)SYNTAQLITE_TRIGGER_TIMING_BEFORE
                             : (int)SYNTAQLITE_TRIGGER_TIMING_AFTER;
    }
      yymsp[0].minor.yy592 = yylhsminor.yy592;
      break;
    case 305: /* trigger_time ::= INSTEAD OF */
    {
      yymsp[-1].minor.yy592 = (int)SYNTAQLITE_TRIGGER_TIMING_INSTEAD_OF;
    } break;
    case 306: /* trigger_time ::= */
    {
      yymsp[1].minor.yy592 = (int)SYNTAQLITE_TRIGGER_TIMING_BEFORE;
    } break;
    case 307: /* trigger_event ::= DELETE|INSERT */
    {
      SyntaqliteTriggerEventType evt =
          (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_DELETE)
              ? SYNTAQLITE_TRIGGER_EVENT_TYPE_DELETE
              : SYNTAQLITE_TRIGGER_EVENT_TYPE_INSERT;
      yylhsminor.yy141 =
          synq_parse_trigger_event(pCtx, evt, SYNTAQLITE_NULL_NODE);
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 308: /* trigger_event ::= UPDATE */
    {
      yymsp[0].minor.yy141 = synq_parse_trigger_event(
          pCtx, SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE, SYNTAQLITE_NULL_NODE);
    } break;
    case 309: /* trigger_event ::= UPDATE OF idlist */
    {
      yymsp[-2].minor.yy141 = synq_parse_trigger_event(
          pCtx, SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE, yymsp[0].minor.yy141);
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
      yylhsminor.yy141 = synq_parse_trigger_cmd_list(
          pCtx, yymsp[-2].minor.yy141, yymsp[-1].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 315: /* trigger_cmd_list ::= trigger_cmd SEMI */
    {
      yylhsminor.yy141 = synq_parse_trigger_cmd_list(pCtx, SYNTAQLITE_NULL_NODE,
                                                     yymsp[-1].minor.yy141);
    }
      yymsp[-1].minor.yy141 = yylhsminor.yy141;
      break;
    case 317: /* trnm ::= nm DOT nm */
    {
      yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;
      // Qualified names not allowed in triggers, but grammar accepts them
    } break;
    case 319: /* tridxby ::= INDEXED BY nm */
    case 320: /* tridxby ::= NOT INDEXED */
      yytestcase(yyruleno == 320);
      {
        // Not allowed in triggers, but grammar accepts
      }
      break;
    case 321: /* trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from
                 where_opt scanpt */
    {
      uint32_t tbl = synq_parse_table_ref(
          pCtx, synq_span(pCtx, yymsp[-6].minor.yy0), SYNQ_NO_SPAN,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-8].minor.yy141 = synq_parse_update_stmt(
          pCtx, (SyntaqliteConflictAction)yymsp[-7].minor.yy592, tbl,
          SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN, yymsp[-3].minor.yy141,
          yymsp[-2].minor.yy141, yymsp[-1].minor.yy141, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    } break;
    case 322: /* trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select
                 upsert scanpt */
    {
      uint32_t tbl = synq_parse_table_ref(
          pCtx, synq_span(pCtx, yymsp[-4].minor.yy0), SYNQ_NO_SPAN,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-7].minor.yy141 = synq_parse_insert_stmt(
          pCtx, (SyntaqliteConflictAction)yymsp[-6].minor.yy592, tbl,
          yymsp[-3].minor.yy141, yymsp[-2].minor.yy141, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE);
    } break;
    case 323: /* trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
    {
      uint32_t tbl = synq_parse_table_ref(
          pCtx, synq_span(pCtx, yymsp[-3].minor.yy0), SYNQ_NO_SPAN,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-5].minor.yy141 = synq_parse_delete_stmt(
          pCtx, tbl, SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN,
          yymsp[-1].minor.yy141, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE);
    } break;
    case 325: /* cmd ::= PRAGMA nm dbnm */
    {
      SyntaqliteSourceSpan name_span =
          yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[0].minor.yy0)
                               : synq_span(pCtx, yymsp[-1].minor.yy0);
      SyntaqliteSourceSpan schema_span =
          yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[-1].minor.yy0)
                               : SYNQ_NO_SPAN;
      yymsp[-2].minor.yy141 =
          synq_parse_pragma_stmt(pCtx, name_span, schema_span, SYNQ_NO_SPAN,
                                 SYNTAQLITE_PRAGMA_FORM_BARE);
    } break;
    case 326: /* cmd ::= PRAGMA nm dbnm EQ nmnum */
    case 328: /* cmd ::= PRAGMA nm dbnm EQ minus_num */
      yytestcase(yyruleno == 328);
      {
        SyntaqliteSourceSpan name_span =
            yymsp[-2].minor.yy0.z ? synq_span(pCtx, yymsp[-2].minor.yy0)
                                  : synq_span(pCtx, yymsp[-3].minor.yy0);
        SyntaqliteSourceSpan schema_span =
            yymsp[-2].minor.yy0.z ? synq_span(pCtx, yymsp[-3].minor.yy0)
                                  : SYNQ_NO_SPAN;
        yymsp[-4].minor.yy141 = synq_parse_pragma_stmt(
            pCtx, name_span, schema_span, synq_span(pCtx, yymsp[0].minor.yy0),
            SYNTAQLITE_PRAGMA_FORM_EQ);
      }
      break;
    case 327: /* cmd ::= PRAGMA nm dbnm LP nmnum RP */
    case 329: /* cmd ::= PRAGMA nm dbnm LP minus_num RP */
      yytestcase(yyruleno == 329);
      {
        SyntaqliteSourceSpan name_span =
            yymsp[-3].minor.yy0.z ? synq_span(pCtx, yymsp[-3].minor.yy0)
                                  : synq_span(pCtx, yymsp[-4].minor.yy0);
        SyntaqliteSourceSpan schema_span =
            yymsp[-3].minor.yy0.z ? synq_span(pCtx, yymsp[-4].minor.yy0)
                                  : SYNQ_NO_SPAN;
        yymsp[-5].minor.yy141 = synq_parse_pragma_stmt(
            pCtx, name_span, schema_span, synq_span(pCtx, yymsp[-1].minor.yy0),
            SYNTAQLITE_PRAGMA_FORM_CALL);
      }
      break;
    case 335: /* plus_num ::= PLUS INTEGER|FLOAT */
    {
      yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;
    } break;
    case 337: /* minus_num ::= MINUS INTEGER|FLOAT */
    {
      // Build a token that spans from the MINUS sign through the number
      yylhsminor.yy0.z = yymsp[-1].minor.yy0.z;
      yylhsminor.yy0.n = (int)(yymsp[0].minor.yy0.z - yymsp[-1].minor.yy0.z) +
                         yymsp[0].minor.yy0.n;
    }
      yymsp[-1].minor.yy0 = yylhsminor.yy0;
      break;
    case 340: /* cmd ::= ANALYZE */
    {
      yymsp[0].minor.yy141 = synq_parse_analyze_or_reindex_stmt(
          pCtx, SYNQ_NO_SPAN, SYNQ_NO_SPAN,
          SYNTAQLITE_ANALYZE_OR_REINDEX_OP_ANALYZE);
    } break;
    case 341: /* cmd ::= ANALYZE nm dbnm */
    {
      SyntaqliteSourceSpan name_span =
          yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[0].minor.yy0)
                               : synq_span(pCtx, yymsp[-1].minor.yy0);
      SyntaqliteSourceSpan schema_span =
          yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[-1].minor.yy0)
                               : SYNQ_NO_SPAN;
      yymsp[-2].minor.yy141 = synq_parse_analyze_or_reindex_stmt(
          pCtx, name_span, schema_span,
          SYNTAQLITE_ANALYZE_OR_REINDEX_OP_ANALYZE);
    } break;
    case 342: /* cmd ::= REINDEX */
    {
      yymsp[0].minor.yy141 = synq_parse_analyze_or_reindex_stmt(
          pCtx, SYNQ_NO_SPAN, SYNQ_NO_SPAN,
          SYNTAQLITE_ANALYZE_OR_REINDEX_OP_REINDEX);
    } break;
    case 343: /* cmd ::= REINDEX nm dbnm */
    {
      SyntaqliteSourceSpan name_span =
          yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[0].minor.yy0)
                               : synq_span(pCtx, yymsp[-1].minor.yy0);
      SyntaqliteSourceSpan schema_span =
          yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[-1].minor.yy0)
                               : SYNQ_NO_SPAN;
      yymsp[-2].minor.yy141 =
          synq_parse_analyze_or_reindex_stmt(pCtx, name_span, schema_span, 1);
    } break;
    case 344: /* cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
    {
      yymsp[-5].minor.yy141 =
          synq_parse_attach_stmt(pCtx, yymsp[-3].minor.yy141,
                                 yymsp[-1].minor.yy141, yymsp[0].minor.yy141);
    } break;
    case 345: /* cmd ::= DETACH database_kw_opt expr */
    {
      yymsp[-2].minor.yy141 =
          synq_parse_detach_stmt(pCtx, yymsp[0].minor.yy141);
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
      yymsp[-1].minor.yy141 =
          synq_parse_vacuum_stmt(pCtx, SYNQ_NO_SPAN, yymsp[0].minor.yy141);
    } break;
    case 351: /* cmd ::= VACUUM nm vinto */
    {
      yymsp[-2].minor.yy141 = synq_parse_vacuum_stmt(
          pCtx, synq_span(pCtx, yymsp[-1].minor.yy0), yymsp[0].minor.yy141);
    } break;
    case 354: /* ecmd ::= explain cmdx SEMI */
    {
      (void)yymsp[-2].minor.yy592;
      yylhsminor.yy141 = yymsp[-1].minor.yy141;
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 355: /* explain ::= EXPLAIN */
    {
      yymsp[0].minor.yy592 = 1;
      pCtx->pending_explain_mode = 1;
    } break;
    case 356: /* explain ::= EXPLAIN QUERY PLAN */
    {
      yymsp[-2].minor.yy592 = 2;
      pCtx->pending_explain_mode = 2;
    } break;
    case 357: /* cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP
                 sortlist RP where_opt */
    {
      SyntaqliteSourceSpan idx_name =
          yymsp[-6].minor.yy0.z ? synq_span(pCtx, yymsp[-6].minor.yy0)
                                : synq_span(pCtx, yymsp[-7].minor.yy0);
      SyntaqliteSourceSpan idx_schema =
          yymsp[-6].minor.yy0.z ? synq_span(pCtx, yymsp[-7].minor.yy0)
                                : SYNQ_NO_SPAN;
      yymsp[-11].minor.yy141 = synq_parse_create_index_stmt(
          pCtx, idx_name, idx_schema, synq_span(pCtx, yymsp[-4].minor.yy0),
          (SyntaqliteBool)yymsp[-10].minor.yy592,
          (SyntaqliteBool)yymsp[-8].minor.yy592, yymsp[-2].minor.yy141,
          yymsp[0].minor.yy141);
    } break;
    case 361: /* ifnotexists ::= IF NOT EXISTS */
    {
      yymsp[-2].minor.yy592 = 1;
    } break;
    case 362: /* cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS
                 select */
    {
      SyntaqliteSourceSpan view_name =
          yymsp[-3].minor.yy0.z ? synq_span(pCtx, yymsp[-3].minor.yy0)
                                : synq_span(pCtx, yymsp[-4].minor.yy0);
      SyntaqliteSourceSpan view_schema =
          yymsp[-3].minor.yy0.z ? synq_span(pCtx, yymsp[-4].minor.yy0)
                                : SYNQ_NO_SPAN;
      yymsp[-8].minor.yy141 = synq_parse_create_view_stmt(
          pCtx, view_name, view_schema, (SyntaqliteBool)yymsp[-7].minor.yy592,
          (SyntaqliteBool)yymsp[-5].minor.yy592, yymsp[-2].minor.yy141,
          yymsp[0].minor.yy141);
    } break;
    case 366: /* values ::= VALUES LP nexprlist RP */
    {
      yymsp[-3].minor.yy141 = synq_parse_values_row_list(
          pCtx, SYNTAQLITE_NULL_NODE, yymsp[-1].minor.yy141);
    } break;
    case 367: /* mvalues ::= values COMMA LP nexprlist RP */
    case 368: /* mvalues ::= mvalues COMMA LP nexprlist RP */
      yytestcase(yyruleno == 368);
      {
        yymsp[-4].minor.yy141 = synq_parse_values_row_list(
            pCtx, yymsp[-4].minor.yy141, yymsp[-1].minor.yy141);
      }
      break;
    case 369: /* oneselect ::= values */
    case 370: /* oneselect ::= mvalues */
      yytestcase(yyruleno == 370);
      {
        yylhsminor.yy141 = synq_parse_values_clause(pCtx, yymsp[0].minor.yy141);
      }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 372: /* cmd ::= create_vtab LP vtabarglist RP */
    {
      // Capture module arguments span (content between parens)
      SyntaqliteNode* vtab = AST_NODE(&pCtx->ast, yymsp[-3].minor.yy141);
      const char* args_start = yymsp[-2].minor.yy0.z + yymsp[-2].minor.yy0.n;
      const char* args_end = yymsp[0].minor.yy0.z;
      vtab->create_virtual_table_stmt.module_args =
          (SyntaqliteSourceSpan){(uint32_t)(args_start - pCtx->source),
                                 (uint16_t)(args_end - args_start), 0};
      yylhsminor.yy141 = yymsp[-3].minor.yy141;
    }
      yymsp[-3].minor.yy141 = yylhsminor.yy141;
      break;
    case 373: /* create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm
                 USING nm */
    {
      SyntaqliteSourceSpan tbl_name =
          yymsp[-2].minor.yy0.z ? synq_span(pCtx, yymsp[-2].minor.yy0)
                                : synq_span(pCtx, yymsp[-3].minor.yy0);
      SyntaqliteSourceSpan tbl_schema =
          yymsp[-2].minor.yy0.z ? synq_span(pCtx, yymsp[-3].minor.yy0)
                                : SYNQ_NO_SPAN;
      yymsp[-7].minor.yy141 = synq_parse_create_virtual_table_stmt(
          pCtx, tbl_name, tbl_schema, synq_span(pCtx, yymsp[0].minor.yy0),
          (SyntaqliteBool)yymsp[-4].minor.yy592,
          SYNQ_NO_SPAN);  // module_args = none by default
    } break;
    case 384: /* windowdefn_list ::= windowdefn */
    {
      yylhsminor.yy141 = synq_parse_named_window_def_list(
          pCtx, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy141);
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 385: /* windowdefn_list ::= windowdefn_list COMMA windowdefn */
    {
      yylhsminor.yy141 = synq_parse_named_window_def_list(
          pCtx, yymsp[-2].minor.yy141, yymsp[0].minor.yy141);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 386: /* windowdefn ::= nm AS LP window RP */
    {
      yylhsminor.yy141 = synq_parse_named_window_def(
          pCtx, synq_span(pCtx, yymsp[-4].minor.yy0), yymsp[-1].minor.yy141);
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 387: /* window ::= PARTITION BY nexprlist orderby_opt frame_opt */
    {
      yymsp[-4].minor.yy141 =
          synq_parse_window_def(pCtx, SYNQ_NO_SPAN, yymsp[-2].minor.yy141,
                                yymsp[-1].minor.yy141, yymsp[0].minor.yy141);
    } break;
    case 388: /* window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
    {
      yylhsminor.yy141 = synq_parse_window_def(
          pCtx, synq_span(pCtx, yymsp[-5].minor.yy0), yymsp[-2].minor.yy141,
          yymsp[-1].minor.yy141, yymsp[0].minor.yy141);
    }
      yymsp[-5].minor.yy141 = yylhsminor.yy141;
      break;
    case 389: /* window ::= ORDER BY sortlist frame_opt */
    {
      yymsp[-3].minor.yy141 =
          synq_parse_window_def(pCtx, SYNQ_NO_SPAN, SYNTAQLITE_NULL_NODE,
                                yymsp[-1].minor.yy141, yymsp[0].minor.yy141);
    } break;
    case 390: /* window ::= nm ORDER BY sortlist frame_opt */
    {
      yylhsminor.yy141 = synq_parse_window_def(
          pCtx, synq_span(pCtx, yymsp[-4].minor.yy0), SYNTAQLITE_NULL_NODE,
          yymsp[-1].minor.yy141, yymsp[0].minor.yy141);
    }
      yymsp[-4].minor.yy141 = yylhsminor.yy141;
      break;
    case 391: /* window ::= frame_opt */
    {
      yylhsminor.yy141 =
          synq_parse_window_def(pCtx, SYNQ_NO_SPAN, SYNTAQLITE_NULL_NODE,
                                SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy141);
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 392: /* window ::= nm frame_opt */
    {
      yylhsminor.yy141 = synq_parse_window_def(
          pCtx, synq_span(pCtx, yymsp[-1].minor.yy0), SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy141);
    }
      yymsp[-1].minor.yy141 = yylhsminor.yy141;
      break;
    case 394: /* frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
    {
      // Single bound: start=yymsp[-1].minor.yy141, end=CURRENT ROW (implicit)
      uint32_t end_bound = synq_parse_frame_bound(
          pCtx, SYNTAQLITE_FRAME_BOUND_TYPE_CURRENT_ROW, SYNTAQLITE_NULL_NODE);
      yylhsminor.yy141 = synq_parse_frame_spec(
          pCtx, (SyntaqliteFrameType)yymsp[-2].minor.yy592,
          (SyntaqliteFrameExclude)yymsp[0].minor.yy592, yymsp[-1].minor.yy141,
          end_bound);
    }
      yymsp[-2].minor.yy141 = yylhsminor.yy141;
      break;
    case 395: /* frame_opt ::= range_or_rows BETWEEN frame_bound_s AND
                 frame_bound_e frame_exclude_opt */
    {
      yylhsminor.yy141 = synq_parse_frame_spec(
          pCtx, (SyntaqliteFrameType)yymsp[-5].minor.yy592,
          (SyntaqliteFrameExclude)yymsp[0].minor.yy592, yymsp[-3].minor.yy141,
          yymsp[-1].minor.yy141);
    }
      yymsp[-5].minor.yy141 = yylhsminor.yy141;
      break;
    case 396: /* range_or_rows ::= RANGE|ROWS|GROUPS */
    {
      switch (yymsp[0].minor.yy0.type) {
        case SYNTAQLITE_TK_RANGE:
          yylhsminor.yy592 = SYNTAQLITE_FRAME_TYPE_RANGE;
          break;
        case SYNTAQLITE_TK_ROWS:
          yylhsminor.yy592 = SYNTAQLITE_FRAME_TYPE_ROWS;
          break;
        default:
          yylhsminor.yy592 = SYNTAQLITE_FRAME_TYPE_GROUPS;
          break;
      }
    }
      yymsp[0].minor.yy592 = yylhsminor.yy592;
      break;
    case 398: /* frame_bound_s ::= UNBOUNDED PRECEDING */
    {
      yymsp[-1].minor.yy141 = synq_parse_frame_bound(
          pCtx, SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_PRECEDING,
          SYNTAQLITE_NULL_NODE);
    } break;
    case 400: /* frame_bound_e ::= UNBOUNDED FOLLOWING */
    {
      yymsp[-1].minor.yy141 = synq_parse_frame_bound(
          pCtx, SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_FOLLOWING,
          SYNTAQLITE_NULL_NODE);
    } break;
    case 401: /* frame_bound ::= expr PRECEDING|FOLLOWING */
    {
      SyntaqliteFrameBoundType bt =
          (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_PRECEDING)
              ? SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_PRECEDING
              : SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_FOLLOWING;
      yylhsminor.yy141 =
          synq_parse_frame_bound(pCtx, bt, yymsp[-1].minor.yy141);
    }
      yymsp[-1].minor.yy141 = yylhsminor.yy141;
      break;
    case 402: /* frame_bound ::= CURRENT ROW */
    {
      yymsp[-1].minor.yy141 = synq_parse_frame_bound(
          pCtx, SYNTAQLITE_FRAME_BOUND_TYPE_CURRENT_ROW, SYNTAQLITE_NULL_NODE);
    } break;
    case 403: /* frame_exclude_opt ::= */
    {
      yymsp[1].minor.yy592 = SYNTAQLITE_FRAME_EXCLUDE_NONE;
    } break;
    case 405: /* frame_exclude ::= NO OTHERS */
    {
      yymsp[-1].minor.yy592 = SYNTAQLITE_FRAME_EXCLUDE_NO_OTHERS;
    } break;
    case 406: /* frame_exclude ::= CURRENT ROW */
    {
      yymsp[-1].minor.yy592 = SYNTAQLITE_FRAME_EXCLUDE_CURRENT_ROW;
    } break;
    case 407: /* frame_exclude ::= GROUP|TIES */
    {
      yylhsminor.yy592 = (yymsp[0].minor.yy0.type == SYNTAQLITE_TK_GROUP)
                             ? SYNTAQLITE_FRAME_EXCLUDE_GROUP
                             : SYNTAQLITE_FRAME_EXCLUDE_TIES;
    }
      yymsp[0].minor.yy592 = yylhsminor.yy592;
      break;
    case 409: /* filter_over ::= filter_clause over_clause */
    {
      // Unpack the over_clause FilterOver to combine with filter expr
      SyntaqliteFilterOver* fo_over = (SyntaqliteFilterOver*)synq_arena_ptr(
          &pCtx->ast, yymsp[0].minor.yy141);
      yylhsminor.yy141 = synq_parse_filter_over(
          pCtx, yymsp[-1].minor.yy141, fo_over->over_def, SYNQ_NO_SPAN);
    }
      yymsp[-1].minor.yy141 = yylhsminor.yy141;
      break;
    case 411: /* filter_over ::= filter_clause */
    {
      yylhsminor.yy141 = synq_parse_filter_over(
          pCtx, yymsp[0].minor.yy141, SYNTAQLITE_NULL_NODE, SYNQ_NO_SPAN);
    }
      yymsp[0].minor.yy141 = yylhsminor.yy141;
      break;
    case 412: /* over_clause ::= OVER LP window RP */
    {
      yymsp[-3].minor.yy141 = synq_parse_filter_over(
          pCtx, SYNTAQLITE_NULL_NODE, yymsp[-1].minor.yy141, SYNQ_NO_SPAN);
    } break;
    case 413: /* over_clause ::= OVER nm */
    {
      // Create a WindowDef with just base_window_name to represent a named
      // window ref
      uint32_t wdef = synq_parse_window_def(
          pCtx, synq_span(pCtx, yymsp[0].minor.yy0), SYNTAQLITE_NULL_NODE,
          SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
      yymsp[-1].minor.yy141 = synq_parse_filter_over(pCtx, SYNTAQLITE_NULL_NODE,
                                                     wdef, SYNQ_NO_SPAN);
    } break;
    case 414: /* filter_clause ::= FILTER LP WHERE expr RP */
    {
      yymsp[-4].minor.yy141 = yymsp[-1].minor.yy141;
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
#define SYNQ_NT_INPUT 188
#define SYNQ_NT_CMDLIST 189
#define SYNQ_NT_ECMD 190
#define SYNQ_NT_CMDX 191
#define SYNQ_NT_ERROR 192
#define SYNQ_NT_CMD 193
#define SYNQ_NT_EXPR 194
#define SYNQ_NT_DISTINCT 195
#define SYNQ_NT_EXPRLIST 196
#define SYNQ_NT_SORTLIST 197
#define SYNQ_NT_FILTER_OVER 198
#define SYNQ_NT_TYPETOKEN 199
#define SYNQ_NT_TYPENAME 200
#define SYNQ_NT_SIGNED 201
#define SYNQ_NT_SELCOLLIST 202
#define SYNQ_NT_SCLP 203
#define SYNQ_NT_SCANPT 204
#define SYNQ_NT_NM 205
#define SYNQ_NT_MULTISELECT_OP 206
#define SYNQ_NT_IN_OP 207
#define SYNQ_NT_DBNM 208
#define SYNQ_NT_SELECTNOWITH 209
#define SYNQ_NT_ONESELECT 210
#define SYNQ_NT_SELECT 211
#define SYNQ_NT_PAREN_EXPRLIST 212
#define SYNQ_NT_LIKEOP 213
#define SYNQ_NT_BETWEEN_OP 214
#define SYNQ_NT_CASE_OPERAND 215
#define SYNQ_NT_CASE_EXPRLIST 216
#define SYNQ_NT_CASE_ELSE 217
#define SYNQ_NT_SCANTOK 218
#define SYNQ_NT_AUTOINC 219
#define SYNQ_NT_REFARGS 220
#define SYNQ_NT_REFARG 221
#define SYNQ_NT_REFACT 222
#define SYNQ_NT_DEFER_SUBCLAUSE 223
#define SYNQ_NT_INIT_DEFERRED_PRED_OPT 224
#define SYNQ_NT_DEFER_SUBCLAUSE_OPT 225
#define SYNQ_NT_TABLE_OPTION_SET 226
#define SYNQ_NT_TABLE_OPTION 227
#define SYNQ_NT_TCONSCOMMA 228
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
#define SYNQ_NT_EIDLIST 246
#define SYNQ_NT_RESOLVETYPE 247
#define SYNQ_NT_WITHNM 248
#define SYNQ_NT_WQAS 249
#define SYNQ_NT_COLLATE 250
#define SYNQ_NT_WQLIST 251
#define SYNQ_NT_WQITEM 252
#define SYNQ_NT_WITH 253
#define SYNQ_NT_INSERT_CMD 254
#define SYNQ_NT_ORCONF 255
#define SYNQ_NT_INDEXED_OPT 256
#define SYNQ_NT_WHERE_OPT_RET 257
#define SYNQ_NT_UPSERT 258
#define SYNQ_NT_RETURNING 259
#define SYNQ_NT_XFULLNAME 260
#define SYNQ_NT_ORDERBY_OPT 261
#define SYNQ_NT_LIMIT_OPT 262
#define SYNQ_NT_SETLIST 263
#define SYNQ_NT_FROM 264
#define SYNQ_NT_IDLIST_OPT 265
#define SYNQ_NT_RAISETYPE 266
#define SYNQ_NT_INDEXED_BY 267
#define SYNQ_NT_IDLIST 268
#define SYNQ_NT_WHERE_OPT 269
#define SYNQ_NT_NEXPRLIST 270
#define SYNQ_NT_NMORERR 271
#define SYNQ_NT_NULLS 272
#define SYNQ_NT_IFEXISTS 273
#define SYNQ_NT_TRANSTYPE 274
#define SYNQ_NT_TRANS_OPT 275
#define SYNQ_NT_SAVEPOINT_OPT 276
#define SYNQ_NT_KWCOLUMN_OPT 277
#define SYNQ_NT_FULLNAME 278
#define SYNQ_NT_ADD_COLUMN_FULLNAME 279
#define SYNQ_NT_AS 280
#define SYNQ_NT_GROUPBY_OPT 281
#define SYNQ_NT_HAVING_OPT 282
#define SYNQ_NT_WINDOW_CLAUSE 283
#define SYNQ_NT_SELTABLIST 284
#define SYNQ_NT_ON_USING 285
#define SYNQ_NT_JOINOP 286
#define SYNQ_NT_STL_PREFIX 287
#define SYNQ_NT_TRIGGER_TIME 288
#define SYNQ_NT_TRNM 289
#define SYNQ_NT_TRIGGER_DECL 290
#define SYNQ_NT_TRIGGER_CMD_LIST 291
#define SYNQ_NT_TRIGGER_EVENT 292
#define SYNQ_NT_FOREACH_CLAUSE 293
#define SYNQ_NT_WHEN_CLAUSE 294
#define SYNQ_NT_TRIGGER_CMD 295
#define SYNQ_NT_TRIDXBY 296
#define SYNQ_NT_PLUS_NUM 297
#define SYNQ_NT_MINUS_NUM 298
#define SYNQ_NT_NMNUM 299
#define SYNQ_NT_UNIQUEFLAG 300
#define SYNQ_NT_EXPLAIN 301
#define SYNQ_NT_DATABASE_KW_OPT 302
#define SYNQ_NT_KEY_OPT 303
#define SYNQ_NT_VINTO 304
#define SYNQ_NT_VALUES 305
#define SYNQ_NT_MVALUES 306
#define SYNQ_NT_CREATE_VTAB 307
#define SYNQ_NT_VTABARGLIST 308
#define SYNQ_NT_VTABARG 309
#define SYNQ_NT_VTABARGTOKEN 310
#define SYNQ_NT_LP 311
#define SYNQ_NT_ANYLIST 312
#define SYNQ_NT_RANGE_OR_ROWS 313
#define SYNQ_NT_FRAME_EXCLUDE_OPT 314
#define SYNQ_NT_FRAME_EXCLUDE 315
#define SYNQ_NT_WINDOWDEFN_LIST 316
#define SYNQ_NT_WINDOWDEFN 317
#define SYNQ_NT_WINDOW 318
#define SYNQ_NT_FRAME_OPT 319
#define SYNQ_NT_FRAME_BOUND_S 320
#define SYNQ_NT_FRAME_BOUND_E 321
#define SYNQ_NT_FRAME_BOUND 322
#define SYNQ_NT_FILTER_CLAUSE 323
#define SYNQ_NT_OVER_CLAUSE 324

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
