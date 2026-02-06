/*
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
** SQLite parser for synq.
** Generated from SQLite's parse.y via Lemon with injection points.
** DO NOT EDIT - regenerate with: python3 python/tools/extract_sqlite.py
*/

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

/*
** Synq parser injection support.
** When SYNQ_PARSER_DATA_FILE is defined, external grammar data is used.
*/
#ifdef SYNQ_PARSER_DATA_FILE
#include SYNQ_PARSER_DATA_FILE
#define _SYNQ_EXTERNAL_PARSER 1
#endif

/************ Begin %include sections from the grammar ************************/
#include "src/common/synq_sqlite_defs.h"
#include "syntaqlite/sqlite_tokens_gen.h"
#include "src/parser/ast_base.h"
#include "src/parser/ast_builder_gen.h"

#define YYNOERRORRECOVERY 1
#define YYPARSEFREENEVERNULL 1
/**************** End of %include directives **********************************/
#ifndef _SYNQ_EXTERNAL_PARSER
/* These constants specify the various numeric values for terminal symbols.
***************** Begin token definitions *************************************/
#ifndef SYNTAQLITE_TOKEN_INDEX
#define SYNTAQLITE_TOKEN_INDEX                           1
#define SYNTAQLITE_TOKEN_ALTER                           2
#define SYNTAQLITE_TOKEN_EXPLAIN                         3
#define SYNTAQLITE_TOKEN_ADD                             4
#define SYNTAQLITE_TOKEN_QUERY                           5
#define SYNTAQLITE_TOKEN_WINDOW                          6
#define SYNTAQLITE_TOKEN_PLAN                            7
#define SYNTAQLITE_TOKEN_OVER                            8
#define SYNTAQLITE_TOKEN_BEGIN                           9
#define SYNTAQLITE_TOKEN_FILTER                         10
#define SYNTAQLITE_TOKEN_TRANSACTION                    11
#define SYNTAQLITE_TOKEN_COLUMN                         12
#define SYNTAQLITE_TOKEN_DEFERRED                       13
#define SYNTAQLITE_TOKEN_AGG_FUNCTION                   14
#define SYNTAQLITE_TOKEN_IMMEDIATE                      15
#define SYNTAQLITE_TOKEN_AGG_COLUMN                     16
#define SYNTAQLITE_TOKEN_EXCLUSIVE                      17
#define SYNTAQLITE_TOKEN_TRUEFALSE                      18
#define SYNTAQLITE_TOKEN_COMMIT                         19
#define SYNTAQLITE_TOKEN_FUNCTION                       20
#define SYNTAQLITE_TOKEN_END                            21
#define SYNTAQLITE_TOKEN_UPLUS                          22
#define SYNTAQLITE_TOKEN_ROLLBACK                       23
#define SYNTAQLITE_TOKEN_UMINUS                         24
#define SYNTAQLITE_TOKEN_SAVEPOINT                      25
#define SYNTAQLITE_TOKEN_TRUTH                          26
#define SYNTAQLITE_TOKEN_RELEASE                        27
#define SYNTAQLITE_TOKEN_REGISTER                       28
#define SYNTAQLITE_TOKEN_TO                             29
#define SYNTAQLITE_TOKEN_VECTOR                         30
#define SYNTAQLITE_TOKEN_TABLE                          31
#define SYNTAQLITE_TOKEN_SELECT_COLUMN                  32
#define SYNTAQLITE_TOKEN_CREATE                         33
#define SYNTAQLITE_TOKEN_IF_NULL_ROW                    34
#define SYNTAQLITE_TOKEN_IF                             35
#define SYNTAQLITE_TOKEN_ASTERISK                       36
#define SYNTAQLITE_TOKEN_NOT                            37
#define SYNTAQLITE_TOKEN_SPAN                           38
#define SYNTAQLITE_TOKEN_EXISTS                         39
#define SYNTAQLITE_TOKEN_ERROR                          40
#define SYNTAQLITE_TOKEN_TEMP                           41
#define SYNTAQLITE_TOKEN_QNUMBER                        42
#define SYNTAQLITE_TOKEN_AS                             43
#define SYNTAQLITE_TOKEN_WITHOUT                        44
#define SYNTAQLITE_TOKEN_ABORT                          45
#define SYNTAQLITE_TOKEN_ACTION                         46
#define SYNTAQLITE_TOKEN_AFTER                          47
#define SYNTAQLITE_TOKEN_ANALYZE                        48
#define SYNTAQLITE_TOKEN_ASC                            49
#define SYNTAQLITE_TOKEN_ATTACH                         50
#define SYNTAQLITE_TOKEN_BEFORE                         51
#define SYNTAQLITE_TOKEN_BY                             52
#define SYNTAQLITE_TOKEN_CASCADE                        53
#define SYNTAQLITE_TOKEN_CAST                           54
#define SYNTAQLITE_TOKEN_CONFLICT                       55
#define SYNTAQLITE_TOKEN_DATABASE                       56
#define SYNTAQLITE_TOKEN_DESC                           57
#define SYNTAQLITE_TOKEN_DETACH                         58
#define SYNTAQLITE_TOKEN_EACH                           59
#define SYNTAQLITE_TOKEN_FAIL                           60
#define SYNTAQLITE_TOKEN_OR                             61
#define SYNTAQLITE_TOKEN_AND                            62
#define SYNTAQLITE_TOKEN_IS                             63
#define SYNTAQLITE_TOKEN_ISNOT                          64
#define SYNTAQLITE_TOKEN_MATCH                          65
#define SYNTAQLITE_TOKEN_LIKE_KW                        66
#define SYNTAQLITE_TOKEN_BETWEEN                        67
#define SYNTAQLITE_TOKEN_IN                             68
#define SYNTAQLITE_TOKEN_ISNULL                         69
#define SYNTAQLITE_TOKEN_NOTNULL                        70
#define SYNTAQLITE_TOKEN_ESCAPE                         71
#define SYNTAQLITE_TOKEN_COLUMNKW                       72
#define SYNTAQLITE_TOKEN_DO                             73
#define SYNTAQLITE_TOKEN_FOR                            74
#define SYNTAQLITE_TOKEN_IGNORE                         75
#define SYNTAQLITE_TOKEN_INITIALLY                      76
#define SYNTAQLITE_TOKEN_INSTEAD                        77
#define SYNTAQLITE_TOKEN_NO                             78
#define SYNTAQLITE_TOKEN_KEY                            79
#define SYNTAQLITE_TOKEN_OF                             80
#define SYNTAQLITE_TOKEN_OFFSET                         81
#define SYNTAQLITE_TOKEN_PRAGMA                         82
#define SYNTAQLITE_TOKEN_RAISE                          83
#define SYNTAQLITE_TOKEN_RECURSIVE                      84
#define SYNTAQLITE_TOKEN_REPLACE                        85
#define SYNTAQLITE_TOKEN_RESTRICT                       86
#define SYNTAQLITE_TOKEN_ROW                            87
#define SYNTAQLITE_TOKEN_ROWS                           88
#define SYNTAQLITE_TOKEN_TRIGGER                        89
#define SYNTAQLITE_TOKEN_VACUUM                         90
#define SYNTAQLITE_TOKEN_VIEW                           91
#define SYNTAQLITE_TOKEN_VIRTUAL                        92
#define SYNTAQLITE_TOKEN_WITH                           93
#define SYNTAQLITE_TOKEN_NULLS                          94
#define SYNTAQLITE_TOKEN_FIRST                          95
#define SYNTAQLITE_TOKEN_LAST                           96
#define SYNTAQLITE_TOKEN_CURRENT                        97
#define SYNTAQLITE_TOKEN_FOLLOWING                      98
#define SYNTAQLITE_TOKEN_PARTITION                      99
#define SYNTAQLITE_TOKEN_PRECEDING                      100
#define SYNTAQLITE_TOKEN_RANGE                          101
#define SYNTAQLITE_TOKEN_UNBOUNDED                      102
#define SYNTAQLITE_TOKEN_EXCLUDE                        103
#define SYNTAQLITE_TOKEN_GROUPS                         104
#define SYNTAQLITE_TOKEN_OTHERS                         105
#define SYNTAQLITE_TOKEN_TIES                           106
#define SYNTAQLITE_TOKEN_GENERATED                      107
#define SYNTAQLITE_TOKEN_ALWAYS                         108
#define SYNTAQLITE_TOKEN_MATERIALIZED                   109
#define SYNTAQLITE_TOKEN_REINDEX                        110
#define SYNTAQLITE_TOKEN_RENAME                         111
#define SYNTAQLITE_TOKEN_CTIME_KW                       112
#define SYNTAQLITE_TOKEN_ANY                            113
#define SYNTAQLITE_TOKEN_COLLATE                        114
#define SYNTAQLITE_TOKEN_ON                             115
#define SYNTAQLITE_TOKEN_INDEXED                        116
#define SYNTAQLITE_TOKEN_JOIN_KW                        117
#define SYNTAQLITE_TOKEN_CONSTRAINT                     118
#define SYNTAQLITE_TOKEN_DEFAULT                        119
#define SYNTAQLITE_TOKEN_NULL                           120
#define SYNTAQLITE_TOKEN_PRIMARY                        121
#define SYNTAQLITE_TOKEN_UNIQUE                         122
#define SYNTAQLITE_TOKEN_CHECK                          123
#define SYNTAQLITE_TOKEN_REFERENCES                     124
#define SYNTAQLITE_TOKEN_AUTOINCR                       125
#define SYNTAQLITE_TOKEN_INSERT                         126
#define SYNTAQLITE_TOKEN_DELETE                         127
#define SYNTAQLITE_TOKEN_UPDATE                         128
#define SYNTAQLITE_TOKEN_SET                            129
#define SYNTAQLITE_TOKEN_DEFERRABLE                     130
#define SYNTAQLITE_TOKEN_FOREIGN                        131
#define SYNTAQLITE_TOKEN_DROP                           132
#define SYNTAQLITE_TOKEN_UNION                          133
#define SYNTAQLITE_TOKEN_ALL                            134
#define SYNTAQLITE_TOKEN_EXCEPT                         135
#define SYNTAQLITE_TOKEN_INTERSECT                      136
#define SYNTAQLITE_TOKEN_SELECT                         137
#define SYNTAQLITE_TOKEN_VALUES                         138
#define SYNTAQLITE_TOKEN_DISTINCT                       139
#define SYNTAQLITE_TOKEN_FROM                           140
#define SYNTAQLITE_TOKEN_JOIN                           141
#define SYNTAQLITE_TOKEN_USING                          142
#define SYNTAQLITE_TOKEN_ORDER                          143
#define SYNTAQLITE_TOKEN_GROUP                          144
#define SYNTAQLITE_TOKEN_HAVING                         145
#define SYNTAQLITE_TOKEN_LIMIT                          146
#define SYNTAQLITE_TOKEN_WHERE                          147
#define SYNTAQLITE_TOKEN_RETURNING                      148
#define SYNTAQLITE_TOKEN_INTO                           149
#define SYNTAQLITE_TOKEN_NOTHING                        150
#define SYNTAQLITE_TOKEN_CASE                           151
#define SYNTAQLITE_TOKEN_WHEN                           152
#define SYNTAQLITE_TOKEN_THEN                           153
#define SYNTAQLITE_TOKEN_ELSE                           154
#define SYNTAQLITE_TOKEN_NE                             155
#define SYNTAQLITE_TOKEN_EQ                             156
#define SYNTAQLITE_TOKEN_GT                             157
#define SYNTAQLITE_TOKEN_LE                             158
#define SYNTAQLITE_TOKEN_LT                             159
#define SYNTAQLITE_TOKEN_GE                             160
#define SYNTAQLITE_TOKEN_BITAND                         161
#define SYNTAQLITE_TOKEN_BITOR                          162
#define SYNTAQLITE_TOKEN_LSHIFT                         163
#define SYNTAQLITE_TOKEN_RSHIFT                         164
#define SYNTAQLITE_TOKEN_PLUS                           165
#define SYNTAQLITE_TOKEN_MINUS                          166
#define SYNTAQLITE_TOKEN_STAR                           167
#define SYNTAQLITE_TOKEN_SLASH                          168
#define SYNTAQLITE_TOKEN_REM                            169
#define SYNTAQLITE_TOKEN_CONCAT                         170
#define SYNTAQLITE_TOKEN_PTR                            171
#define SYNTAQLITE_TOKEN_BITNOT                         172
#define SYNTAQLITE_TOKEN_ID                             173
#define SYNTAQLITE_TOKEN_LP                             174
#define SYNTAQLITE_TOKEN_RP                             175
#define SYNTAQLITE_TOKEN_COMMA                          176
#define SYNTAQLITE_TOKEN_STRING                         177
#define SYNTAQLITE_TOKEN_DOT                            178
#define SYNTAQLITE_TOKEN_FLOAT                          179
#define SYNTAQLITE_TOKEN_BLOB                           180
#define SYNTAQLITE_TOKEN_INTEGER                        181
#define SYNTAQLITE_TOKEN_VARIABLE                       182
#define SYNTAQLITE_TOKEN_SEMI                           183
#define SYNTAQLITE_TOKEN_SPACE                          184
#define SYNTAQLITE_TOKEN_COMMENT                        185
#define SYNTAQLITE_TOKEN_ILLEGAL                        186
#endif
/**************** End token definitions ***************************************/
#endif /* !_SYNQ_EXTERNAL_PARSER */

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
**    synq_sqlite3ParserTOKENTYPE     is the data type used for minor type for terminal
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
**                       which is synq_sqlite3ParserTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    synq_sqlite3ParserARG_SDECL     A static variable declaration for the %extra_argument
**    synq_sqlite3ParserARG_PDECL     A parameter declaration for the %extra_argument
**    synq_sqlite3ParserARG_PARAM     Code to pass %extra_argument as a subroutine parameter
**    synq_sqlite3ParserARG_STORE     Code to store %extra_argument into yypParser
**    synq_sqlite3ParserARG_FETCH     Code to extract %extra_argument from yypParser
**    synq_sqlite3ParserCTX_*         As synq_sqlite3ParserARG_ except for %extra_context
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
# define INTERFACE 1
#endif
#ifndef _SYNQ_EXTERNAL_PARSER
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned short int
#define YYNOCODE 322
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 113
#define synq_sqlite3ParserTOKENTYPE SynqToken
typedef union {
  int yyinit;
  synq_sqlite3ParserTOKENTYPE yy0;
  SynqConstraintListValue yy106;
  SynqOnUsingValue yy136;
  int yy144;
  SynqColumnNameValue yy188;
  u32 yy391;
  SynqConstraintValue yy462;
  SynqWithValue yy581;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define synq_sqlite3ParserARG_SDECL
#define synq_sqlite3ParserARG_PDECL
#define synq_sqlite3ParserARG_PARAM
#define synq_sqlite3ParserARG_FETCH
#define synq_sqlite3ParserARG_STORE
#define YYREALLOC realloc
#define YYFREE free
#define YYDYNSTACK 0
#define synq_sqlite3ParserCTX_SDECL SynqParseContext *pCtx;
#define synq_sqlite3ParserCTX_PDECL ,SynqParseContext *pCtx
#define synq_sqlite3ParserCTX_PARAM ,pCtx
#define synq_sqlite3ParserCTX_FETCH SynqParseContext *pCtx=yypParser->pCtx;
#define synq_sqlite3ParserCTX_STORE yypParser->pCtx=pCtx;
#define YYFALLBACK 1
#define YYNSTATE             583
#define YYNRULE              409
#define YYNRULE_WITH_ACTION  409
#define YYNTOKEN             187
#define YY_MAX_SHIFT         582
#define YY_MIN_SHIFTREDUCE   845
#define YY_MAX_SHIFTREDUCE   1253
#define YY_ERROR_ACTION      1254
#define YY_ACCEPT_ACTION     1255
#define YY_NO_ACTION         1256
#define YY_MIN_REDUCE        1257
#define YY_MAX_REDUCE        1665
#define YY_MIN_DSTRCTR       0
#define YY_MAX_DSTRCTR       0
/************* End control #defines *******************************************/
#endif /* !_SYNQ_EXTERNAL_PARSER */
#define YY_NLOOKAHEAD ((int)(sizeof(yy_lookahead)/sizeof(yy_lookahead[0])))

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif

/* Macro to determine if stack space has the ability to grow using
** heap memory.
*/
#if YYSTACKDEPTH<=0 || YYDYNSTACK
# define YYGROWABLESTACK 1
#else
# define YYGROWABLESTACK 0
#endif

/* Guarantee a minimum number of initial stack slots.
*/
#if YYSTACKDEPTH<=0
# undef YYSTACKDEPTH
# define YYSTACKDEPTH 2  /* Need a minimum stack size */
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
#define YY_ACTTAB_COUNT (2166)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   130,  161,  522,  234,  130, 1281,  574,  234, 1659,  402,
 /*    10 */   574,  290, 1633,  233,  130,  539,  574,  234,  574,  299,
 /*    20 */  1644, 1281,  565,  183,  571,  254,  264,  230,  290, 1633,
 /*    30 */  1345,  464, 1259,  295,  284,  130, 1341,  413,  234,  565,
 /*    40 */   565,  571,  571, 1290,  574,  290, 1633,  462, 1613,  983,
 /*    50 */   574, 1568, 1643,  477,  478, 1637,  565,  984,  571,  481,
 /*    60 */   372,  137,  139,   91, 1279, 1228, 1228, 1063, 1066, 1053,
 /*    70 */  1053,  557, 1639,   84, 1293,  413, 1639,   82,    7,  378,
 /*    80 */   290, 1633, 1639,   51, 1639,   51,  130,  466, 1019,  234,
 /*    90 */  1292,  565,  556,  571,  212,  413,   44,  558,  127,  137,
 /*   100 */   139,   91,  127, 1228, 1228, 1063, 1066, 1053, 1053,  516,
 /*   110 */  1639,   84,  127,  383,  452, 1542, 1639,   51,  407,  137,
 /*   120 */   139,   91, 1277, 1228, 1228, 1063, 1066, 1053, 1053,  359,
 /*   130 */   556,  290, 1633,  127,  532,  523, 1174,  158, 1174,  140,
 /*   140 */   321,  575,  565,  377,  571, 1582,  376, 1584,  530, 1665,
 /*   150 */   452,  379,  452, 1644,  452,  135,  135,  136,  136,  136,
 /*   160 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   170 */   131,  128,  452, 1570, 1628, 1204, 1204,  547, 1300, 1582,
 /*   180 */  1568, 1541,  574, 1622,  127, 1643, 1513, 1511,  501,  372,
 /*   190 */   452,  419,  510,  135,  135,  136,  136,  136,  136,  134,
 /*   200 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   210 */   131,  128,  452,  135,  135,  136,  136,  136,  136,  134,
 /*   220 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   230 */  1147,  413, 1204,  198,  110,  497, 1205,  134,  134,  134,
 /*   240 */   134,  133,  133,  132,  132,  132,  131,  128, 1639,   19,
 /*   250 */  1207,  413,  421,   48, 1207,  137,  139,   91, 1571, 1228,
 /*   260 */  1228, 1063, 1066, 1053, 1053,  132,  132,  132,  131,  128,
 /*   270 */  1204, 1204,  507,   97,  551,  137,  139,   91,  308, 1228,
 /*   280 */  1228, 1063, 1066, 1053, 1053,  409, 1250,  290, 1633,  413,
 /*   290 */  1169, 1149, 1657, 1657,  290, 1633,  499,   45,  565, 1632,
 /*   300 */   571,  543, 1569, 1169, 1331,  565, 1169,  571,  452,  447,
 /*   310 */   446,  413,  573,  137,  139,   91,    7, 1228, 1228, 1063,
 /*   320 */  1066, 1053, 1053, 1236,  373, 1236, 1188, 1204,  452, 1204,
 /*   330 */  1204, 1205, 1147,  237, 1637,  137,  139,   91,  302, 1228,
 /*   340 */  1228, 1063, 1066, 1053, 1053,  542,  452,  401, 1249,  135,
 /*   350 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   360 */   133,  132,  132,  132,  131,  128,  452,  267,  344,  135,
 /*   370 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   380 */   133,  132,  132,  132,  131,  128, 1204, 1577,  452,  323,
 /*   390 */  1205,  524,  301, 1149, 1658, 1658, 1040,  133,  133,  132,
 /*   400 */   132,  132,  131,  128, 1028, 1204, 1204,  135,  135,  136,
 /*   410 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*   420 */   132,  132,  131,  128,  321,  575,  413,  467,  516,  135,
 /*   430 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   440 */   133,  132,  132,  132,  131,  128, 1126,  413,  516,  886,
 /*   450 */   137,  139,   91,   98, 1228, 1228, 1063, 1066, 1053, 1053,
 /*   460 */   527, 1029, 1204, 1028, 1028, 1030, 1205,  183, 1127,  107,
 /*   470 */   525,  137,  139,   91, 1506, 1228, 1228, 1063, 1066, 1053,
 /*   480 */  1053, 1632,  518, 1128,  452,  413, 1333, 1050, 1050, 1064,
 /*   490 */  1067,  290, 1633,   46,  283, 1633,  526, 1224,  922,  499,
 /*   500 */   427, 1224,  565,  452,  571,  565,  429,  571,  923,  137,
 /*   510 */   139,   91,  893, 1228, 1228, 1063, 1066, 1053, 1053, 1307,
 /*   520 */  1505, 1631,    7, 1328,  452, 1204, 1204,  136,  136,  136,
 /*   530 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   540 */   131,  128, 1054,  474,  135,  135,  136,  136,  136,  136,
 /*   550 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   560 */   128,  524,  452,  113,  879,  135,  135,  136,  136,  136,
 /*   570 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   580 */   131,  128, 1204,  370,  532,  505, 1205, 1233, 1204, 1204,
 /*   590 */  1107, 1235,  540, 1665, 1107,  304,  219,  413,  346, 1234,
 /*   600 */  1204, 1204,  574,  135,  135,  136,  136,  136,  136,  134,
 /*   610 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   620 */   511,  137,  139,   91,  231, 1228, 1228, 1063, 1066, 1053,
 /*   630 */  1053,  321,  575,  100,  413, 1019,  251,  447,  446,  574,
 /*   640 */   525, 1204, 1204,  236,  303, 1204,  461,  458,  457, 1205,
 /*   650 */   129, 1236, 1250, 1236,  420,  413,  456, 1204,  137,  139,
 /*   660 */    91, 1205, 1228, 1228, 1063, 1066, 1053, 1053, 1639,   19,
 /*   670 */   157,  850,  294,  851,  452,  852,  359,  485,  347,  137,
 /*   680 */   139,   91, 1598, 1228, 1228, 1063, 1066, 1053, 1053,  382,
 /*   690 */   559,  236,  505,  452,  461,  458,  457,    6, 1204,  290,
 /*   700 */  1633,  322, 1205, 1451,  456, 1639,   19,  533, 1664,  432,
 /*   710 */   565,  452,  571,  401, 1148,  135,  135,  136,  136,  136,
 /*   720 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   730 */   131,  128,  452,  380, 1595, 1041,  136,  136,  136,  136,
 /*   740 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   750 */   128,  358,  135,  135,  136,  136,  136,  136,  134,  134,
 /*   760 */   134,  134,  133,  133,  132,  132,  132,  131,  128,  574,
 /*   770 */   413,  253, 1024,  135,  135,  136,  136,  136,  136,  134,
 /*   780 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   790 */   413,  490,  361, 1187,  137,  139,   91,  574, 1228, 1228,
 /*   800 */  1063, 1066, 1053, 1053,   44, 1311,   44,  506, 1204, 1204,
 /*   810 */   568, 1620, 1621,  521,  137,  139,   91, 1339, 1228, 1228,
 /*   820 */  1063, 1066, 1053, 1053, 1324, 1169,  290, 1633,  413,  316,
 /*   830 */   479,  291, 1633, 1282,  418, 1639,   84,  565, 1169,  571,
 /*   840 */   499, 1169,  565,  576,  571,  936,  936,  452,  321,  575,
 /*   850 */   321,  575,  137,  139,   91,  528, 1228, 1228, 1063, 1066,
 /*   860 */  1053, 1053,  534, 1639,   60, 1204,  199,  452, 1368, 1205,
 /*   870 */     6,  290, 1633,  944, 1204, 1204, 1204, 1204,  943, 1639,
 /*   880 */    22,  505,  565,  535,  571,  384,  116,  375,  135,  135,
 /*   890 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   900 */   132,  132,  132,  131,  128,  452,  536, 1188,  135,  135,
 /*   910 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   920 */   132,  132,  132,  131,  128,  949, 1204, 1204, 1125, 1568,
 /*   930 */  1105, 1204, 1280, 1204,    5, 1205,  307, 1205,  372,  413,
 /*   940 */   310,  574,  567,  418,  120,  215,  135,  135,  136,  136,
 /*   950 */   136,  136,  134,  134,  134,  134,  133,  133,  132,  132,
 /*   960 */   132,  131,  128,  137,  139,   91,  413, 1228, 1228, 1063,
 /*   970 */  1066, 1053, 1053,  448, 1620, 1621,  209, 1040, 1204, 1204,
 /*   980 */   489,  882, 1450, 1204,   42, 1028,  574, 1205,  574,  392,
 /*   990 */   137,  139,   91, 1169, 1228, 1228, 1063, 1066, 1053, 1053,
 /*  1000 */   413,  495,   50,  971,  552,  944, 1169, 1639,   19, 1169,
 /*  1010 */   943,  437,  380, 1595,  406,  405,  452,  232, 1110, 1110,
 /*  1020 */   513, 1204, 1204,   95,  137,  126,   91, 1310, 1228, 1228,
 /*  1030 */  1063, 1066, 1053, 1053,  221, 1204,  490,  361,  882, 1205,
 /*  1040 */  1309,  969, 1029,  452, 1028, 1028, 1030,  368,  541,  390,
 /*  1050 */   574, 1569, 1639,   19, 1639,   84,  442,  135,  135,  136,
 /*  1060 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*  1070 */   132,  132,  131,  128,  556,  286,  969,  452, 1204,  555,
 /*  1080 */   516,  413, 1205,  574,  135,  135,  136,  136,  136,  136,
 /*  1090 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*  1100 */   128,  574, 1204, 1204,  503,  429,  139,   91, 1449, 1228,
 /*  1110 */  1228, 1063, 1066, 1053, 1053,  413, 1639,   84,  135,  135,
 /*  1120 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*  1130 */   132,  132,  132,  131,  128, 1498,  317,  429,  380, 1595,
 /*  1140 */   482,   91,  212, 1228, 1228, 1063, 1066, 1053, 1053, 1639,
 /*  1150 */    84,  412, 1340,  574,  267, 1568,  370,  499,  452, 1204,
 /*  1160 */     3, 1204, 1204, 1205,  372,  574,  399, 1639,   19,  556,
 /*  1170 */  1435,  207,  874,  160,  444,  862,  874, 1169, 1308,  124,
 /*  1180 */  1435,  566, 1435,  504, 1188,  516,  970, 1435, 1629,  293,
 /*  1190 */  1169,  551,  452, 1169,  574,    7,  569,  213,  331,  135,
 /*  1200 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*  1210 */   133,  132,  132,  132,  131,  128,  574,  516, 1204, 1639,
 /*  1220 */    19,    7, 1205,  208,  138,  562,  488,  440,   47,  214,
 /*  1230 */     7, 1639,   84,  135,  135,  136,  136,  136,  136,  134,
 /*  1240 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*  1250 */   124,  318,  566,  309, 1040, 1188, 1085, 1336,  450,  450,
 /*  1260 */  1639,   19, 1028,  434,  550,  113, 1575,  569,  241,  453,
 /*  1270 */  1435,  549, 1289,  435,  124,  436,  566, 1569,  574, 1188,
 /*  1280 */   512,  232, 1639,   84, 1224,   10, 1568,  349, 1224,  366,
 /*  1290 */   548,  569, 1576,   34,  574,  372,  562,  241,  480, 1180,
 /*  1300 */     7, 1574,  445,  336,    9,  969,   44,  122,  122,  948,
 /*  1310 */     2,  161,  212,  335,  123,  450,    4,  351,  417,  572,
 /*  1320 */   562, 1028, 1028, 1030, 1031, 1040,  240,  239,  238,  450,
 /*  1330 */   450,  422,  185, 1028,  545,  254,  398, 1180,  574,  544,
 /*  1340 */   969,  465, 1259,  295, 1639,  145, 1341,  417,  247, 1040,
 /*  1350 */   321,  575,  426,  450,  450,  290, 1633, 1028,  327,   49,
 /*  1360 */  1639,   84,  330,  441,   34, 1552,  565,  124,  571,  566,
 /*  1370 */   483, 1573, 1188,  902,  574,  348,  326,  350,  122,  122,
 /*  1380 */   449,  412, 1627,  911,  569,  123,  450,    4,   34,   14,
 /*  1390 */   572, 1244, 1028, 1028, 1030, 1031,  574,  466,  470,  574,
 /*  1400 */   430,  574,  122,  122, 1639,   84,  903, 1554, 1569,  123,
 /*  1410 */   450,    4,  222,  562,  572, 1602, 1028, 1028, 1030, 1031,
 /*  1420 */   574, 1126,  469,  574,  451,  574,  121,  545,  118,  423,
 /*  1430 */   475,  574,  546,  243,  280,  329,  472,  332,  471,  242,
 /*  1440 */  1639,  147, 1040, 1127,  492,  330,  450,  450,  228,  424,
 /*  1450 */  1028,  288,  230,  983,  124,  412,  566,  574, 1128, 1188,
 /*  1460 */  1550,  984, 1639,   53,  574, 1639,   54, 1639,   55,  496,
 /*  1470 */   339,  569,  574,  561,  574,  340,  574,  222,  574,  107,
 /*  1480 */   412,   34,  574,  908,  574,  909, 1639,   56,  320, 1639,
 /*  1490 */    66, 1639,   67,  491,  574,  122,  122, 1639,   21,  412,
 /*  1500 */   562,   25,  123,  450,    4,  574,  526,  572,  574, 1028,
 /*  1510 */  1028, 1030, 1031, 1223,  545,  356,  101,  218,   39,  544,
 /*  1520 */   155,    2,  161, 1639,   57,  454,  263,  108,  889, 1040,
 /*  1530 */  1639,   68,  889,  450,  450,  574,   38, 1028, 1639,   58,
 /*  1540 */  1639,   69, 1639,   70, 1639,   71,  254,  574, 1639,   72,
 /*  1550 */  1639,   73,  465, 1259,  295,  574,  228, 1341,  324,  113,
 /*  1560 */  1639,   74,  574, 1144,  403,  343,  290, 1633,   34,  901,
 /*  1570 */   900, 1639,   75, 1380, 1639,   76, 1379,  565,  574,  571,
 /*  1580 */   574,  289,  122,  122, 1601, 1616,  476,  582,  493,  123,
 /*  1590 */   450,    4,  574,  292,  572,  298, 1028, 1028, 1030, 1031,
 /*  1600 */   484, 1639,   77,  396,  520,  396, 1557,  395,  466,  277,
 /*  1610 */   305,  393,  574, 1639,   59,  352,  338,  859, 1104,  574,
 /*  1620 */  1104, 1639,   61,  574, 1255,    1, 1603, 1092, 1639,   20,
 /*  1630 */   574, 1092,  248,  574,  342,  124, 1530,  566,  574,  537,
 /*  1640 */  1188, 1103,  341, 1103, 1639,  143, 1639,  144,  357,  113,
 /*  1650 */  1021,  266,  569,  574, 1529,   90,  508,  566, 1639,   79,
 /*  1660 */  1188,  574,  498,  266,  500,  266,  250,  362,  113, 1088,
 /*  1670 */   263,  166,  569,  363,  177,  574, 1153,   43, 1639,   62,
 /*  1680 */   574,  562,  974,  266,  574, 1639,   80, 1376,  574, 1639,
 /*  1690 */    63,  165,  113,  574,  249,  367, 1639,   81,  371, 1639,
 /*  1700 */    64,  562,  941,  138, 1639,  170,  986,  987,  872,  159,
 /*  1710 */  1040,  942,  138, 1032,  450,  450,  414, 1032, 1028, 1639,
 /*  1720 */   171,  321,  575,  939,  138, 1366, 1389, 1639,   88, 1434,
 /*  1730 */  1040, 1362,  459,  205,  450,  450,  560,  554, 1028, 1589,
 /*  1740 */  1439, 1639,   65,  389, 1374,  391, 1639,  146,  574,   34,
 /*  1750 */  1639,   83, 1268, 1267, 1639,  168,  124,  553,  566, 1639,
 /*  1760 */   148, 1188,  574,  122,  122, 1269, 1609, 1192,  224,   34,
 /*  1770 */   123,  450,    4,  569,  574,  572,  517, 1028, 1028, 1030,
 /*  1780 */  1031,  574,  167,  122,  122, 1359,  476,  582,  281,  574,
 /*  1790 */   123,  450,    4,  292,  313,  572,  574, 1028, 1028, 1030,
 /*  1800 */  1031,  574,  562,  396,  314,  396,  397,  395,  315,  277,
 /*  1810 */    12,  393,  574,  227, 1639,  142,  574,  859, 1612, 1307,
 /*  1820 */   574,  354,  246,  574,  334,  300, 1421,  360, 1639,  169,
 /*  1830 */   355, 1040,  248, 1416,  342,  450,  450, 1409,  502, 1028,
 /*  1840 */  1639,  162,  341, 1502, 1426, 1371,  306, 1639,  152,  223,
 /*  1850 */  1372, 1501,  210, 1370,  211, 1639,  151, 1425,  410, 1369,
 /*  1860 */   574,  203, 1639,  149,  400,  387,  250, 1639,  150,  473,
 /*  1870 */    34, 1327, 1326,  563,  177, 1325,   40,   43, 1639,   86,
 /*  1880 */   463,   93, 1639,   78,  122,  122, 1639,   87,  476, 1639,
 /*  1890 */    85,  123,  450,    4,  249,  292,  572, 1626, 1028, 1028,
 /*  1900 */  1030, 1031, 1625, 1624,  425,  396, 1318,  396,  404,  395,
 /*  1910 */   893,  277, 1297,  393, 1296,  333,  414, 1317, 1295,  859,
 /*  1920 */  1244,  321,  575,  187,  244,  274, 1639,   52, 1549, 1547,
 /*  1930 */  1241,   96,   99,  428,  248,  100,  342, 1507,  220, 1422,
 /*  1940 */   196,   13,  182,  189,  341,  486,  487,  191,  579,  192,
 /*  1950 */   193,  194,  108, 1428,  408, 1427,   15,  494,  256, 1430,
 /*  1960 */   411,  200, 1496,  509,  260,  106, 1518, 1192,  250,  515,
 /*  1970 */   282,  365,  262,  438,  519,  439,  177,  369,  311,   43,
 /*  1980 */   312,  141,  268,  551,  443, 1350,  269,  374,  233, 1394,
 /*  1990 */  1393, 1580, 1579,   11, 1594, 1483,  249,  381,  117, 1349,
 /*  2000 */   109,  319,  216,  529,  273,  388,  385,  577, 1270,  275,
 /*  2010 */  1198,  276,  278,  279,  225,  580,  386, 1265,  414, 1260,
 /*  2020 */   172,  173,  154,  321,  575,  226,  174,  296,  415,  186,
 /*  2030 */   416, 1534, 1535, 1533, 1532,  335,   89,  184,  235,  455,
 /*  2040 */    92,   23,  460,  325,   24,  175,  153, 1194, 1193,  217,
 /*  2050 */   891,  328,  468,  904,   94,  297,  176,  337,  245,  156,
 /*  2060 */  1102,  345, 1100,  178,  188, 1223,  252,  190,  255,  925,
 /*  2070 */   353, 1116,  195,  431,  179,  180,  433,  197,  102,  181,
 /*  2080 */   103,  104, 1119,  257,  258,  105, 1115,  163,  259,  364,
 /*  2090 */    26, 1108,  201,  266,  514, 1155, 1238,  261,  202, 1154,
 /*  2100 */   978,  265,  972,   27,  531,   16,   28,   29,   30,  112,
 /*  2110 */   285,  229,  287,  204,  138, 1160,   41,  206, 1069,   31,
 /*  2120 */  1171, 1175,  538,  111,  164, 1173, 1185,    8, 1179, 1178,
 /*  2130 */   113,   32,   33,  114,  115,  119, 1073,  935,   18, 1083,
 /*  2140 */  1070,  861, 1068, 1072, 1124,  270,   35,  578, 1617,   17,
 /*  2150 */   564,  394, 1033,  873,  846,  125,   36,  271,  581, 1256,
 /*  2160 */  1256, 1256,  570,   37, 1256,  272,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */   190,  217,  252,  193,  190,  187,  187,  193,  306,  307,
 /*    10 */   187,  261,  262,    8,  190,   10,  187,  193,  187,  252,
 /*    20 */   187,  187,  272,  187,  274,  241,  276,  277,  261,  262,
 /*    30 */   262,  247,  248,  249,  189,  190,  252,   37,  193,  272,
 /*    40 */   272,  274,  274,  253,  187,  261,  262,  229,  230,   49,
 /*    50 */   187,  187,  219,  220,  221,  201,  272,   57,  274,  205,
 /*    60 */   196,   61,   62,   63,  230,   65,   66,   67,   68,   69,
 /*    70 */    70,  252,  253,  254,  253,   37,  253,  254,  242,  255,
 /*    80 */   261,  262,  253,  254,  253,  254,  190,  303,   85,  193,
 /*    90 */   253,  272,  273,  274,  187,   37,   93,  278,  288,   61,
 /*   100 */    62,   63,  288,   65,   66,   67,   68,   69,   70,  187,
 /*   110 */   253,  254,  288,  290,  114,  301,  253,  254,  211,   61,
 /*   120 */    62,   63,  252,   65,   66,   67,   68,   69,   70,  126,
 /*   130 */   273,  261,  262,  288,  187,  278,   98,    6,  100,   81,
 /*   140 */   137,  138,  272,  314,  274,  316,  315,  316,  312,  313,
 /*   150 */   114,  255,  114,  187,  114,  155,  156,  157,  158,  159,
 /*   160 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   170 */   170,  171,  114,  309,  233,  116,  117,  314,  226,  316,
 /*   180 */   187,  259,  187,  231,  288,  219,  220,  221,  187,  196,
 /*   190 */   114,  213,  214,  155,  156,  157,  158,  159,  160,  161,
 /*   200 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   210 */   170,  171,  114,  155,  156,  157,  158,  159,  160,  161,
 /*   220 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   230 */   113,   37,  173,  174,  176,  202,  177,  161,  162,  163,
 /*   240 */   164,  165,  166,  167,  168,  169,  170,  171,  253,  254,
 /*   250 */   173,   37,  257,  260,  177,   61,   62,   63,  311,   65,
 /*   260 */    66,   67,   68,   69,   70,  167,  168,  169,  170,  171,
 /*   270 */   116,  117,  294,   79,  143,   61,   62,   63,  252,   65,
 /*   280 */    66,   67,   68,   69,   70,  252,  113,  261,  262,   37,
 /*   290 */    88,  174,  175,  176,  261,  262,  187,   84,  272,  199,
 /*   300 */   274,   99,  309,  101,  204,  272,  104,  274,  114,  165,
 /*   310 */   166,   37,  187,   61,   62,   63,  242,   65,   66,   67,
 /*   320 */    68,   69,   70,  179,  187,  181,   42,  173,  114,  116,
 /*   330 */   117,  177,  113,   44,  201,   61,   62,   63,  205,   65,
 /*   340 */    66,   67,   68,   69,   70,  143,  114,  174,  175,  155,
 /*   350 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   360 */   166,  167,  168,  169,  170,  171,  114,   43,  187,  155,
 /*   370 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   380 */   166,  167,  168,  169,  170,  171,  173,  313,  114,  175,
 /*   390 */   177,   37,  283,  174,  175,  176,  112,  165,  166,  167,
 /*   400 */   168,  169,  170,  171,  120,  116,  117,  155,  156,  157,
 /*   410 */   158,  159,  160,  161,  162,  163,  164,  165,  166,  167,
 /*   420 */   168,  169,  170,  171,  137,  138,   37,  175,  187,  155,
 /*   430 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   440 */   166,  167,  168,  169,  170,  171,   23,   37,  187,  175,
 /*   450 */    61,   62,   63,   43,   65,   66,   67,   68,   69,   70,
 /*   460 */   187,  177,  173,  179,  180,  181,  177,  187,   45,  115,
 /*   470 */   116,   61,   62,   63,  293,   65,   66,   67,   68,   69,
 /*   480 */    70,  199,  252,   60,  114,   37,  204,   65,   66,   67,
 /*   490 */    68,  261,  262,   84,  261,  262,  142,  173,   75,  187,
 /*   500 */   259,  177,  272,  114,  274,  272,  187,  274,   85,   61,
 /*   510 */    62,   63,  125,   65,   66,   67,   68,   69,   70,  224,
 /*   520 */   259,  226,  242,  228,  114,  116,  117,  157,  158,  159,
 /*   530 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   540 */   170,  171,  120,  302,  155,  156,  157,  158,  159,  160,
 /*   550 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*   560 */   171,   37,  114,  176,  175,  155,  156,  157,  158,  159,
 /*   570 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   580 */   170,  171,  173,  187,  187,  187,  177,  115,  116,  117,
 /*   590 */    47,  119,  312,  313,   51,  283,  148,   37,  279,  127,
 /*   600 */   116,  117,  187,  155,  156,  157,  158,  159,  160,  161,
 /*   610 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   620 */    77,   61,   62,   63,  187,   65,   66,   67,   68,   69,
 /*   630 */    70,  137,  138,  149,   37,   85,   43,  165,  166,  187,
 /*   640 */   116,  116,  117,  118,  246,  173,  121,  122,  123,  177,
 /*   650 */    71,  179,  113,  181,  258,   37,  131,  173,   61,   62,
 /*   660 */    63,  177,   65,   66,   67,   68,   69,   70,  253,  254,
 /*   670 */   176,   13,  257,   15,  114,   17,  126,  127,  128,   61,
 /*   680 */    62,   63,  187,   65,   66,   67,   68,   69,   70,  187,
 /*   690 */   252,  118,  187,  114,  121,  122,  123,  189,  173,  261,
 /*   700 */   262,  187,  177,  287,  131,  253,  254,  310,  311,  257,
 /*   710 */   272,  114,  274,  174,  175,  155,  156,  157,  158,  159,
 /*   720 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   730 */   170,  171,  114,  317,  318,  175,  157,  158,  159,  160,
 /*   740 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*   750 */   171,  246,  155,  156,  157,  158,  159,  160,  161,  162,
 /*   760 */   163,  164,  165,  166,  167,  168,  169,  170,  171,  187,
 /*   770 */    37,  178,  175,  155,  156,  157,  158,  159,  160,  161,
 /*   780 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   790 */    37,  127,  128,  175,   61,   62,   63,  187,   65,   66,
 /*   800 */    67,   68,   69,   70,   93,  225,   93,  299,  116,  117,
 /*   810 */   218,  219,  220,  187,   61,   62,   63,  252,   65,   66,
 /*   820 */    67,   68,   69,   70,  187,   88,  261,  262,   37,  201,
 /*   830 */   119,  261,  262,  194,  195,  253,  254,  272,  101,  274,
 /*   840 */   187,  104,  272,  133,  274,  135,  136,  114,  137,  138,
 /*   850 */   137,  138,   61,   62,   63,  273,   65,   66,   67,   68,
 /*   860 */    69,   70,  252,  253,  254,  173,  174,  114,  240,  177,
 /*   870 */   189,  261,  262,  134,  116,  117,  116,  117,  139,  253,
 /*   880 */   254,  187,  272,  273,  274,  269,  153,  271,  155,  156,
 /*   890 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*   900 */   167,  168,  169,  170,  171,  114,  167,   42,  155,  156,
 /*   910 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*   920 */   167,  168,  169,  170,  171,  167,  116,  117,  175,  187,
 /*   930 */    21,  173,  187,  173,  174,  177,  283,  177,  196,   37,
 /*   940 */   246,  187,  194,  195,  153,   43,  155,  156,  157,  158,
 /*   950 */   159,  160,  161,  162,  163,  164,  165,  166,  167,  168,
 /*   960 */   169,  170,  171,   61,   62,   63,   37,   65,   66,   67,
 /*   970 */    68,   69,   70,  218,  219,  220,  295,  112,  116,  117,
 /*   980 */   299,  116,  287,  173,  174,  120,  187,  177,  187,   31,
 /*   990 */    61,   62,   63,   88,   65,   66,   67,   68,   69,   70,
 /*  1000 */    37,  115,  260,  141,   99,  134,  101,  253,  254,  104,
 /*  1010 */   139,  257,  317,  318,  165,  166,  114,  117,  126,  127,
 /*  1020 */   128,  116,  117,  174,   61,   62,   63,  225,   65,   66,
 /*  1030 */    67,   68,   69,   70,  148,  173,  127,  128,  173,  177,
 /*  1040 */   225,  141,  177,  114,  179,  180,  181,   89,  143,   91,
 /*  1050 */   187,  309,  253,  254,  253,  254,  257,  155,  156,  157,
 /*  1060 */   158,  159,  160,  161,  162,  163,  164,  165,  166,  167,
 /*  1070 */   168,  169,  170,  171,  273,  175,  176,  114,  173,  278,
 /*  1080 */   187,   37,  177,  187,  155,  156,  157,  158,  159,  160,
 /*  1090 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*  1100 */   171,  187,  116,  117,   37,  187,   62,   63,  287,   65,
 /*  1110 */    66,   67,   68,   69,   70,   37,  253,  254,  155,  156,
 /*  1120 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*  1130 */   167,  168,  169,  170,  171,    1,  273,  187,  317,  318,
 /*  1140 */   264,   63,  187,   65,   66,   67,   68,   69,   70,  253,
 /*  1150 */   254,  275,  259,  187,   43,  187,  187,  187,  114,  173,
 /*  1160 */   174,  116,  117,  177,  196,  187,  211,  253,  254,  273,
 /*  1170 */   187,  257,  173,  174,  278,   41,  177,   88,  187,   37,
 /*  1180 */   187,   39,  187,  116,   42,  187,  141,  187,  233,  234,
 /*  1190 */   101,  143,  114,  104,  187,  242,   54,  279,  187,  155,
 /*  1200 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*  1210 */   166,  167,  168,  169,  170,  171,  187,  187,  173,  253,
 /*  1220 */   254,  242,  177,  257,  176,   83,   92,  258,  260,  279,
 /*  1230 */   242,  253,  254,  155,  156,  157,  158,  159,  160,  161,
 /*  1240 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*  1250 */    37,  273,   39,  283,  112,   42,  122,  259,  116,  117,
 /*  1260 */   253,  254,  120,  280,  257,  176,  313,   54,   65,   37,
 /*  1270 */   187,   78,  187,  280,   37,  280,   39,  309,  187,   42,
 /*  1280 */   280,  117,  253,  254,  173,  174,  187,    1,  177,  259,
 /*  1290 */    97,   54,  313,  151,  187,  196,   83,   65,  202,  106,
 /*  1300 */   242,  313,  273,  120,   67,  141,   93,  165,  166,  167,
 /*  1310 */   216,  217,  187,  130,  172,  173,  174,   31,  115,  177,
 /*  1320 */    83,  179,  180,  181,  182,  112,  126,  127,  128,  116,
 /*  1330 */   117,  304,  305,  120,   97,  241,  211,  144,  187,  102,
 /*  1340 */   176,  247,  248,  249,  253,  254,  252,  115,   29,  112,
 /*  1350 */   137,  138,   37,  116,  117,  261,  262,  120,   43,  260,
 /*  1360 */   253,  254,  130,  280,  151,  187,  272,   37,  274,   39,
 /*  1370 */   264,  313,   42,   53,  187,   89,  251,   91,  165,  166,
 /*  1380 */   273,  275,  175,  176,   54,  172,  173,  174,  151,   43,
 /*  1390 */   177,   72,  179,  180,  181,  182,  187,  303,   78,  187,
 /*  1400 */    73,  187,  165,  166,  253,  254,   86,  187,  309,  172,
 /*  1410 */   173,  174,  140,   83,  177,  321,  179,  180,  181,  182,
 /*  1420 */   187,   23,  107,  187,  273,  187,  152,   97,  154,  114,
 /*  1430 */   187,  187,  102,  118,  119,  120,  121,  122,  123,  124,
 /*  1440 */   253,  254,  112,   45,  264,  130,  116,  117,  176,  129,
 /*  1450 */   120,  276,  277,   49,   37,  275,   39,  187,   60,   42,
 /*  1460 */   187,   57,  253,  254,  187,  253,  254,  253,  254,  264,
 /*  1470 */   187,   54,  187,   75,  187,  187,  187,  140,  187,  115,
 /*  1480 */   275,  151,  187,   13,  187,   15,  253,  254,  264,  253,
 /*  1490 */   254,  253,  254,  128,  187,  165,  166,  253,  254,  275,
 /*  1500 */    83,  174,  172,  173,  174,  187,  142,  177,  187,  179,
 /*  1510 */   180,  181,  182,  176,   97,  150,  147,  148,  156,  102,
 /*  1520 */   174,  216,  217,  253,  254,  175,  176,  147,  173,  112,
 /*  1530 */   253,  254,  177,  116,  117,  187,  174,  120,  253,  254,
 /*  1540 */   253,  254,  253,  254,  253,  254,  241,  187,  253,  254,
 /*  1550 */   253,  254,  247,  248,  249,  187,  176,  252,  175,  176,
 /*  1560 */   253,  254,  187,  175,  176,  187,  261,  262,  151,  119,
 /*  1570 */   120,  253,  254,  187,  253,  254,  187,  272,  187,  274,
 /*  1580 */   187,  174,  165,  166,    0,  178,    2,    3,  128,  172,
 /*  1590 */   173,  174,  187,    9,  177,  111,  179,  180,  181,  182,
 /*  1600 */   187,  253,  254,   19,   37,   21,  187,   23,  303,   25,
 /*  1610 */   150,   27,  187,  253,  254,  187,  132,   33,  179,  187,
 /*  1620 */   181,  253,  254,  187,  319,  320,  321,  173,  253,  254,
 /*  1630 */   187,  177,   48,  187,   50,   37,  187,   39,  187,  143,
 /*  1640 */    42,  179,   58,  181,  253,  254,  253,  254,  175,  176,
 /*  1650 */   175,  176,   54,  187,  187,   37,  187,   39,  253,  254,
 /*  1660 */    42,  187,  175,  176,  175,  176,   82,  175,  176,  175,
 /*  1670 */   176,  175,   54,  187,   90,  187,  109,   93,  253,  254,
 /*  1680 */   187,   83,  175,  176,  187,  253,  254,  187,  187,  253,
 /*  1690 */   254,  175,  176,  187,  110,  187,  253,  254,  187,  253,
 /*  1700 */   254,   83,  175,  176,  253,  254,   95,   96,  175,  176,
 /*  1710 */   112,  175,  176,  173,  116,  117,  132,  177,  120,  253,
 /*  1720 */   254,  137,  138,  175,  176,  200,  187,  253,  254,  187,
 /*  1730 */   112,  187,  222,  188,  116,  117,  199,  139,  120,  244,
 /*  1740 */   187,  253,  254,  187,  187,  187,  253,  254,  187,  151,
 /*  1750 */   253,  254,  187,  187,  253,  254,   37,  139,   39,  253,
 /*  1760 */   254,   42,  187,  165,  166,  187,  187,  183,  238,  151,
 /*  1770 */   172,  173,  174,   54,  187,  177,  296,  179,  180,  181,
 /*  1780 */   182,  187,  192,  165,  166,  188,    2,    3,  245,  187,
 /*  1790 */   172,  173,  174,    9,  188,  177,  187,  179,  180,  181,
 /*  1800 */   182,  187,   83,   19,  188,   21,  207,   23,  188,   25,
 /*  1810 */   263,   27,  187,  236,  253,  254,  187,   33,  250,  224,
 /*  1820 */   187,  300,  210,  187,  255,  265,  285,  265,  253,  254,
 /*  1830 */   266,  112,   48,  282,   50,  116,  117,  282,  300,  120,
 /*  1840 */   253,  254,   58,  255,  285,  240,  266,  253,  254,  263,
 /*  1850 */   240,  255,  269,  240,  269,  253,  254,  285,  285,  240,
 /*  1860 */   187,  174,  253,  254,  256,  265,   82,  253,  254,  115,
 /*  1870 */   151,  237,  237,  292,   90,  237,  176,   93,  253,  254,
 /*  1880 */   235,  174,  253,  254,  165,  166,  253,  254,    2,  253,
 /*  1890 */   254,  172,  173,  174,  110,    9,  177,  239,  179,  180,
 /*  1900 */   181,  182,  239,  237,   76,   19,  227,   21,  256,   23,
 /*  1910 */   125,   25,  237,   27,  222,  237,  132,  227,  237,   33,
 /*  1920 */    72,  137,  138,  210,  210,  178,  253,  254,  191,  191,
 /*  1930 */    56,  215,  215,  191,   48,  149,   50,  293,  148,  286,
 /*  1940 */   174,  284,   61,  203,   58,   35,  191,  206,   35,  206,
 /*  1950 */   206,  206,  147,  286,  266,  286,  284,  266,  212,  203,
 /*  1960 */   266,  203,  266,  191,  212,  152,  298,  183,   82,   74,
 /*  1970 */   191,  297,  212,   43,  197,  114,   90,  256,  198,   93,
 /*  1980 */   198,  146,  191,  143,   94,  270,  103,  269,    8,  281,
 /*  1990 */   281,  243,  243,  174,  318,  289,  110,  191,  152,  270,
 /*  2000 */   145,  291,  268,  144,  212,  191,  267,  256,  191,  212,
 /*  2010 */    25,  209,  209,   11,  238,  208,  266,  208,  132,  208,
 /*  2020 */   232,  232,  223,  137,  138,  238,  232,  223,  308,  305,
 /*  2030 */   308,  189,  189,  189,  189,  130,  189,  174,  124,   79,
 /*  2040 */   174,  174,   79,  175,  174,  176,  175,  183,  183,  174,
 /*  2050 */   173,   43,  108,   46,  174,   79,   55,    4,   29,   31,
 /*  2060 */   175,  138,  175,  129,  149,  176,   43,  140,  142,   39,
 /*  2070 */    31,  183,  140,   73,  129,  129,   55,  149,  156,  129,
 /*  2080 */   156,  156,  116,   52,  178,  156,  183,    9,  115,    1,
 /*  2090 */   174,   80,   80,  176,   59,  175,   87,  178,  115,  109,
 /*  2100 */   116,   52,  141,   52,   43,  174,   52,   52,   52,  147,
 /*  2110 */   175,  178,  175,  174,  176,  175,  174,  176,  175,   52,
 /*  2120 */   100,   87,  174,  176,  175,   98,  175,   62,   87,  105,
 /*  2130 */   176,   52,   52,  140,  140,  176,   21,  134,   43,  175,
 /*  2140 */   175,   39,  175,  175,  175,  174,  174,   37,  178,  174,
 /*  2150 */   176,   29,  175,  175,    7,  174,  174,  178,    5,  322,
 /*  2160 */   322,  322,  176,  174,  322,  178,  322,  322,  322,  322,
 /*  2170 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2180 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2190 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2200 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2210 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2220 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2230 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2240 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2250 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2260 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2270 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2280 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2290 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2300 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2310 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2320 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2330 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2340 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2350 */   322,  322,  322,
};
#define YY_SHIFT_COUNT    (582)
#define YY_SHIFT_MIN      (0)
#define YY_SHIFT_MAX      (2153)
static const unsigned short int yy_shift_ofst[] = {
 /*     0 */  1784, 1584, 1886, 1213, 1213,  713,    3, 1237, 1330, 1417,
 /*    10 */  1719, 1719, 1719,  711,  713,  713,  713,  713,  713,    0,
 /*    20 */     0,  194,  929, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*    30 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,  472,  472,
 /*    40 */   525,  905,  905,  213,  409,  154,  154,  494,  494,  494,
 /*    50 */   494,   38,   58,  214,  252,  274,  389,  410,  448,  560,
 /*    60 */   597,  618,  733,  753,  791,  902,  929,  929,  929,  929,
 /*    70 */   929,  929,  929,  929,  929,  929,  929,  929,  929,  929,
 /*    80 */   929,  929,  929,  963,  929,  929, 1044, 1078, 1078, 1142,
 /*    90 */  1598, 1618, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   100 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   110 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   120 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   130 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   140 */  1719, 1719,  579,  370,  370,  370,  370,  370,  370,  370,
 /*   150 */    76,  232,   98,  289, 1232,  154,  154,  154,  154,  144,
 /*   160 */   144, 1134,   40,  664,    5,    5,    5,  287,   36,   36,
 /*   170 */  2166, 2166, 1315, 1315, 1315,  289,  423,  484,   59,   59,
 /*   180 */    59,   59,  423,  202,  154,  117,  219,  154,  154,  154,
 /*   190 */   154,  154,  154,  154,  154,  154,  154,  154,  154,  154,
 /*   200 */   154,  154,  154,  154,  154,  354,  154, 1089, 1089,  909,
 /*   210 */   737,  737,   77,  524,  524,   77,  131, 2166, 2166, 2166,
 /*   220 */  2166, 2166, 2166, 2166,  865,  284,  284,  573,  692,  758,
 /*   230 */   760,  862, 1045,  810,  986,  154,  154,  154, 1320, 1320,
 /*   240 */  1320,  154,  154,  154,  154,  154,  154,  154,  154,  154,
 /*   250 */   154,  154,  154,  154,  550,  154,  154,  154,  154,  154,
 /*   260 */   154,  154,  154,  154,  900,  154,  154,  154, 1111, 1193,
 /*   270 */  1398,  154,  154,  154,  154,  154,  154,  154,  154,  154,
 /*   280 */   849,  892,  543,  710,  324,  324,  324,  324, 1164,  739,
 /*   290 */   710,  710,  658, 1207,  387, 1346, 1203, 1404, 1319,  886,
 /*   300 */  1369, 1272, 1369, 1067, 1380,  886,  886, 1380,  886, 1272,
 /*   310 */  1067, 1404, 1404, 1364, 1364, 1364, 1364, 1048, 1048, 1274,
 /*   320 */  1337,  871, 1687, 1754, 1754, 1754, 1700, 1707, 1707, 1754,
 /*   330 */  1828, 1687, 1754, 1785, 1754, 1828, 1754, 1848, 1848, 1747,
 /*   340 */  1747, 1874, 1874, 1747, 1786, 1790, 1766, 1881, 1910, 1910,
 /*   350 */  1910, 1910, 1747, 1913, 1805, 1790, 1790, 1805, 1766, 1881,
 /*   360 */  1805, 1881, 1805, 1747, 1913, 1813, 1895, 1747, 1913, 1930,
 /*   370 */  1861, 1861, 1687, 1747, 1835, 1840, 1883, 1883, 1890, 1890,
 /*   380 */  1980, 1819, 1747, 1846, 1835, 1855, 1859, 1805, 1687, 1747,
 /*   390 */  1913, 1747, 1913, 1985, 1985, 2002, 2002, 2002, 2166, 2166,
 /*   400 */  2166, 2166, 2166, 2166, 2166, 2166, 2166, 2166, 2166, 2166,
 /*   410 */  2166, 2166, 2166,  422, 1286,  173,  539, 1200,  999,  958,
 /*   420 */  1350, 1383, 1388, 1355, 1450, 1470, 1183, 1484, 1362,  593,
 /*   430 */  1365, 1460, 1473, 1327, 1475, 1487, 1489, 1492, 1567, 1454,
 /*   440 */  1494, 1507, 1516, 1611, 1496, 1527, 1439, 1462, 1533, 1536,
 /*   450 */  1407, 1548, 1540, 1905, 1914, 1863, 1960, 1866, 1867, 1868,
 /*   460 */  1870, 1963, 1869, 1871, 1864, 1865, 1875, 1877, 2008, 1944,
 /*   470 */  2007, 1880, 1976, 2001, 2053, 2029, 2028, 1885, 1887, 1923,
 /*   480 */  1915, 1934, 1889, 1889, 2023, 1927, 2030, 1926, 2039, 1888,
 /*   490 */  1932, 1945, 1889, 1946, 2000, 2021, 1889, 1928, 1922, 1924,
 /*   500 */  1925, 1929, 1950, 1966, 2031, 1906, 1903, 2078, 1916, 1973,
 /*   510 */  2088, 2011, 1917, 2012, 2009, 2035, 1919, 1983, 1920, 1931,
 /*   520 */  1990, 1933, 1935, 1937, 1984, 2049, 1939, 1961, 1938, 2051,
 /*   530 */  1940, 1942, 2061, 1941, 1943, 1947, 1949, 2054, 1962, 1948,
 /*   540 */  1951, 2055, 2056, 2067, 2020, 2034, 2027, 2065, 2041, 2024,
 /*   550 */  1954, 2079, 2080, 1993, 1994, 1964, 1938, 1965, 1967, 1968,
 /*   560 */  1959, 1969, 1971, 2115, 1972, 1974, 1975, 1977, 1978, 1981,
 /*   570 */  1982, 1986, 1970, 1979, 1987, 1989, 2003, 2095, 2102, 2110,
 /*   580 */  2122, 2147, 2153,
};
#define YY_REDUCE_COUNT (412)
#define YY_REDUCE_MIN   (-298)
#define YY_REDUCE_MAX   (1847)
static const short yy_reduce_ofst[] = {
 /*     0 */  1305, 1094, -216, -181,  610, -250,   33, -171, -169, -137,
 /*    10 */  -143,  801,  896, -233, -130,   26,  230,  438,  565, -176,
 /*    20 */  -104, -186, -155,   -5,  415,  452,  754,  582,  799,  914,
 /*    30 */   966,  863, 1007,  978, -177, 1029, 1107, 1151, -167,  -34,
 /*    40 */   955, -164,  280,   -7,  742,  968, 1099,  233,  570,  233,
 /*    50 */   570, -190, -190, -190, -190, -190, -190, -190, -190, -190,
 /*    60 */  -190, -190, -190, -190, -190, -190, -190, -190, -190, -190,
 /*    70 */  -190, -190, -190, -190, -190, -190, -190, -190, -190, -190,
 /*    80 */  -190, -190, -190, -190, -190, -190, -190, -190, -190,  626,
 /*    90 */  1091, 1187, 1209, 1212, 1214, 1233, 1236, 1238, 1244, 1270,
 /*   100 */  1277, 1285, 1287, 1289, 1291, 1295, 1297, 1307, 1318, 1321,
 /*   110 */  1348, 1360, 1368, 1375, 1391, 1393, 1405, 1425, 1432, 1436,
 /*   120 */  1443, 1446, 1451, 1466, 1474, 1488, 1493, 1497, 1501, 1506,
 /*   130 */  1561, 1575, 1587, 1594, 1602, 1609, 1614, 1625, 1629, 1633,
 /*   140 */  1636, 1673, -190, -190, -190, -190, -190, -190, -190, -190,
 /*   150 */  -190, -190, -190, -182,  295, 1125,  241, -136,  397,  592,
 /*   160 */   755,  -22, -190,  681,  416,  695,  821, -232, -190, -190,
 /*   170 */  -190, -190,  -48,  -48,  -48, -166,  100,  181,  109,  312,
 /*   180 */   653,  970,  282,   74,  396, -298, -298,  -93,  319,  918,
 /*   190 */   950,  -78,  261,  893,  998,  398,  983,  505,  993,  995,
 /*   200 */   694, 1000, 1030,  969, 1083,  628,  -53,  953,  979,  508,
 /*   210 */   988, 1058,  639, -146,  133,  748,  616, 1027,  876, 1106,
 /*   220 */  1180, 1205, 1175, 1224, -210, -179, -163,  -59,    1,  125,
 /*   230 */   137,  273,  437,  495,  502,  514,  637,  745,  580,  802,
 /*   240 */   815,  991, 1011, 1085, 1178, 1220, 1243, 1273, 1283, 1288,
 /*   250 */  1378, 1386, 1389, 1413, 1096, 1419, 1428, 1449, 1467, 1469,
 /*   260 */  1486, 1500, 1508, 1511, 1525, 1539, 1542, 1544, 1545, 1495,
 /*   270 */  1537, 1553,  125, 1556, 1557, 1558, 1565, 1566, 1578, 1579,
 /*   280 */  1530, 1480, 1543, 1590, 1597, 1606, 1616, 1620, 1525, 1547,
 /*   290 */  1590, 1590, 1599, 1577, 1510, 1568, 1595, 1569, 1612, 1541,
 /*   300 */  1551, 1560, 1555, 1521, 1564, 1559, 1572, 1580, 1573, 1562,
 /*   310 */  1538, 1588, 1596, 1605, 1610, 1613, 1619, 1583, 1585, 1581,
 /*   320 */  1600, 1586, 1608, 1634, 1635, 1638, 1645, 1658, 1663, 1666,
 /*   330 */  1679, 1652, 1675, 1692, 1678, 1690, 1681, 1713, 1714, 1737,
 /*   340 */  1738, 1716, 1717, 1742, 1644, 1653, 1657, 1740, 1741, 1743,
 /*   350 */  1744, 1745, 1755, 1746, 1688, 1667, 1669, 1691, 1672, 1756,
 /*   360 */  1694, 1758, 1696, 1772, 1752, 1668, 1674, 1779, 1760, 1777,
 /*   370 */  1780, 1782, 1721, 1791, 1715, 1718, 1748, 1749, 1708, 1709,
 /*   380 */  1676, 1706, 1806, 1710, 1729, 1734, 1739, 1750, 1751, 1814,
 /*   390 */  1792, 1817, 1797, 1802, 1803, 1807, 1809, 1811, 1788, 1789,
 /*   400 */  1799, 1720, 1722, 1724, 1804, 1776, 1787, 1794, 1842, 1843,
 /*   410 */  1844, 1845, 1847,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */  1663, 1663, 1663, 1491, 1254, 1367, 1254, 1254, 1254, 1254,
 /*    10 */  1491, 1491, 1491, 1254, 1254, 1254, 1254, 1254, 1254, 1397,
 /*    20 */  1397, 1544, 1287, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*    30 */  1254, 1254, 1254, 1254, 1490, 1254, 1254, 1254, 1254, 1254,
 /*    40 */  1254, 1578, 1578, 1254, 1254, 1254, 1254, 1563, 1562, 1254,
 /*    50 */  1254, 1254, 1406, 1254, 1254, 1254, 1254, 1254, 1413, 1492,
 /*    60 */  1493, 1254, 1254, 1254, 1254, 1254, 1543, 1545, 1508, 1420,
 /*    70 */  1419, 1418, 1417, 1526, 1385, 1411, 1404, 1408, 1492, 1487,
 /*    80 */  1488, 1486, 1641, 1254, 1493, 1407, 1455, 1454, 1471, 1254,
 /*    90 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   100 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   110 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   120 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   130 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   140 */  1254, 1254, 1463, 1470, 1469, 1468, 1477, 1467, 1464, 1457,
 /*   150 */  1456, 1458, 1459, 1278, 1329, 1254, 1254, 1254, 1254, 1254,
 /*   160 */  1254, 1275, 1460, 1287, 1448, 1447, 1446, 1254, 1474, 1461,
 /*   170 */  1473, 1472, 1615, 1614, 1551, 1254, 1254, 1509, 1254, 1254,
 /*   180 */  1254, 1254, 1254, 1578, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   190 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   200 */  1254, 1254, 1254, 1254, 1254, 1387, 1254, 1578, 1578, 1287,
 /*   210 */  1578, 1578, 1283, 1388, 1388, 1283, 1391, 1558, 1358, 1358,
 /*   220 */  1358, 1358, 1367, 1358, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   230 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   240 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1548, 1546,
 /*   250 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   260 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1363, 1254,
 /*   270 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1608,
 /*   280 */  1288, 1254, 1521, 1343, 1363, 1363, 1363, 1363, 1365, 1357,
 /*   290 */  1344, 1342, 1261, 1630, 1304, 1254, 1299, 1397, 1655, 1423,
 /*   300 */  1412, 1364, 1412, 1652, 1410, 1423, 1423, 1410, 1423, 1364,
 /*   310 */  1652, 1397, 1397, 1387, 1387, 1387, 1387, 1391, 1391, 1489,
 /*   320 */  1364, 1357, 1499, 1330, 1330, 1330, 1322, 1254, 1254, 1330,
 /*   330 */  1319, 1499, 1330, 1304, 1330, 1319, 1330, 1655, 1655, 1373,
 /*   340 */  1373, 1654, 1654, 1373, 1509, 1638, 1432, 1332, 1338, 1338,
 /*   350 */  1338, 1338, 1373, 1272, 1410, 1638, 1638, 1410, 1432, 1332,
 /*   360 */  1410, 1332, 1410, 1373, 1272, 1525, 1649, 1373, 1272, 1254,
 /*   370 */  1503, 1503, 1499, 1373, 1405, 1391, 1588, 1588, 1400, 1400,
 /*   380 */  1596, 1494, 1373, 1254, 1405, 1403, 1401, 1410, 1499, 1373,
 /*   390 */  1272, 1373, 1272, 1611, 1611, 1607, 1607, 1607, 1623, 1623,
 /*   400 */  1306, 1660, 1660, 1558, 1306, 1288, 1288, 1623, 1287, 1287,
 /*   410 */  1287, 1287, 1287, 1254, 1254, 1254, 1254, 1254, 1618, 1254,
 /*   420 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1553, 1510, 1377,
 /*   430 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1564, 1254,
 /*   440 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   450 */  1437, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   460 */  1254, 1254, 1276, 1254, 1254, 1254, 1555, 1302, 1254, 1254,
 /*   470 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   480 */  1254, 1254, 1414, 1415, 1378, 1254, 1254, 1254, 1254, 1254,
 /*   490 */  1254, 1254, 1429, 1254, 1254, 1254, 1424, 1254, 1254, 1254,
 /*   500 */  1254, 1254, 1254, 1254, 1254, 1651, 1254, 1254, 1254, 1254,
 /*   510 */  1254, 1254, 1524, 1523, 1254, 1254, 1375, 1254, 1254, 1254,
 /*   520 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1402, 1254,
 /*   530 */  1254, 1254, 1254, 1593, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   540 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   550 */  1392, 1254, 1254, 1254, 1254, 1254, 1642, 1254, 1254, 1254,
 /*   560 */  1254, 1254, 1254, 1254, 1254, 1634, 1254, 1254, 1254, 1254,
 /*   570 */  1254, 1352, 1441, 1438, 1254, 1254, 1346, 1254, 1254, 1254,
 /*   580 */  1266, 1254, 1257,
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
#ifndef _SYNQ_EXTERNAL_PARSER
static const YYCODETYPE yyFallback[] = {
    0,  /*          $ => nothing */
    0,  /*      INDEX => nothing */
    0,  /*      ALTER => nothing */
  173,  /*    EXPLAIN => ID */
    0,  /*        ADD => nothing */
  173,  /*      QUERY => ID */
    0,  /*     WINDOW => nothing */
  173,  /*       PLAN => ID */
    0,  /*       OVER => nothing */
  173,  /*      BEGIN => ID */
    0,  /*     FILTER => nothing */
    0,  /* TRANSACTION => nothing */
    0,  /*     COLUMN => nothing */
  173,  /*   DEFERRED => ID */
    0,  /* AGG_FUNCTION => nothing */
  173,  /*  IMMEDIATE => ID */
    0,  /* AGG_COLUMN => nothing */
  173,  /*  EXCLUSIVE => ID */
    0,  /*  TRUEFALSE => nothing */
    0,  /*     COMMIT => nothing */
    0,  /*   FUNCTION => nothing */
  173,  /*        END => ID */
    0,  /*      UPLUS => nothing */
  173,  /*   ROLLBACK => ID */
    0,  /*     UMINUS => nothing */
  173,  /*  SAVEPOINT => ID */
    0,  /*      TRUTH => nothing */
  173,  /*    RELEASE => ID */
    0,  /*   REGISTER => nothing */
    0,  /*         TO => nothing */
    0,  /*     VECTOR => nothing */
    0,  /*      TABLE => nothing */
    0,  /* SELECT_COLUMN => nothing */
    0,  /*     CREATE => nothing */
    0,  /* IF_NULL_ROW => nothing */
  173,  /*         IF => ID */
    0,  /*   ASTERISK => nothing */
    0,  /*        NOT => nothing */
    0,  /*       SPAN => nothing */
    0,  /*     EXISTS => nothing */
    0,  /*      ERROR => nothing */
  173,  /*       TEMP => ID */
    0,  /*    QNUMBER => nothing */
    0,  /*         AS => nothing */
  173,  /*    WITHOUT => ID */
  173,  /*      ABORT => ID */
  173,  /*     ACTION => ID */
  173,  /*      AFTER => ID */
  173,  /*    ANALYZE => ID */
  173,  /*        ASC => ID */
  173,  /*     ATTACH => ID */
  173,  /*     BEFORE => ID */
  173,  /*         BY => ID */
  173,  /*    CASCADE => ID */
  173,  /*       CAST => ID */
  173,  /*   CONFLICT => ID */
  173,  /*   DATABASE => ID */
  173,  /*       DESC => ID */
  173,  /*     DETACH => ID */
  173,  /*       EACH => ID */
  173,  /*       FAIL => ID */
    0,  /*         OR => nothing */
    0,  /*        AND => nothing */
    0,  /*         IS => nothing */
    0,  /*      ISNOT => nothing */
  173,  /*      MATCH => ID */
  173,  /*    LIKE_KW => ID */
    0,  /*    BETWEEN => nothing */
    0,  /*         IN => nothing */
    0,  /*     ISNULL => nothing */
    0,  /*    NOTNULL => nothing */
    0,  /*     ESCAPE => nothing */
  173,  /*   COLUMNKW => ID */
  173,  /*         DO => ID */
  173,  /*        FOR => ID */
  173,  /*     IGNORE => ID */
  173,  /*  INITIALLY => ID */
  173,  /*    INSTEAD => ID */
  173,  /*         NO => ID */
  173,  /*        KEY => ID */
  173,  /*         OF => ID */
  173,  /*     OFFSET => ID */
  173,  /*     PRAGMA => ID */
  173,  /*      RAISE => ID */
  173,  /*  RECURSIVE => ID */
  173,  /*    REPLACE => ID */
  173,  /*   RESTRICT => ID */
  173,  /*        ROW => ID */
  173,  /*       ROWS => ID */
  173,  /*    TRIGGER => ID */
  173,  /*     VACUUM => ID */
  173,  /*       VIEW => ID */
  173,  /*    VIRTUAL => ID */
  173,  /*       WITH => ID */
  173,  /*      NULLS => ID */
  173,  /*      FIRST => ID */
  173,  /*       LAST => ID */
  173,  /*    CURRENT => ID */
  173,  /*  FOLLOWING => ID */
  173,  /*  PARTITION => ID */
  173,  /*  PRECEDING => ID */
  173,  /*      RANGE => ID */
  173,  /*  UNBOUNDED => ID */
  173,  /*    EXCLUDE => ID */
  173,  /*     GROUPS => ID */
  173,  /*     OTHERS => ID */
  173,  /*       TIES => ID */
  173,  /*  GENERATED => ID */
  173,  /*     ALWAYS => ID */
    0,  /* MATERIALIZED => nothing */
  173,  /*    REINDEX => ID */
  173,  /*     RENAME => ID */
  173,  /*   CTIME_KW => ID */
    0,  /*        ANY => nothing */
    0,  /*    COLLATE => nothing */
    0,  /*         ON => nothing */
    0,  /*    INDEXED => nothing */
    0,  /*    JOIN_KW => nothing */
    0,  /* CONSTRAINT => nothing */
    0,  /*    DEFAULT => nothing */
    0,  /*       NULL => nothing */
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
    0,  /*       DROP => nothing */
    0,  /*      UNION => nothing */
    0,  /*        ALL => nothing */
    0,  /*     EXCEPT => nothing */
    0,  /*  INTERSECT => nothing */
    0,  /*     SELECT => nothing */
    0,  /*     VALUES => nothing */
    0,  /*   DISTINCT => nothing */
    0,  /*       FROM => nothing */
    0,  /*       JOIN => nothing */
    0,  /*      USING => nothing */
    0,  /*      ORDER => nothing */
    0,  /*      GROUP => nothing */
    0,  /*     HAVING => nothing */
    0,  /*      LIMIT => nothing */
    0,  /*      WHERE => nothing */
    0,  /*  RETURNING => nothing */
    0,  /*       INTO => nothing */
    0,  /*    NOTHING => nothing */
    0,  /*       CASE => nothing */
    0,  /*       WHEN => nothing */
    0,  /*       THEN => nothing */
    0,  /*       ELSE => nothing */
    0,  /*         NE => nothing */
    0,  /*         EQ => nothing */
    0,  /*         GT => nothing */
    0,  /*         LE => nothing */
    0,  /*         LT => nothing */
    0,  /*         GE => nothing */
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
    0,  /*     BITNOT => nothing */
    0,  /*         ID => nothing */
    0,  /*         LP => nothing */
    0,  /*         RP => nothing */
    0,  /*      COMMA => nothing */
    0,  /*     STRING => nothing */
    0,  /*        DOT => nothing */
    0,  /*      FLOAT => nothing */
    0,  /*       BLOB => nothing */
    0,  /*    INTEGER => nothing */
    0,  /*   VARIABLE => nothing */
    0,  /*       SEMI => nothing */
    0,  /*      SPACE => nothing */
    0,  /*    COMMENT => nothing */
    0,  /*    ILLEGAL => nothing */
};
#endif /* !_SYNQ_EXTERNAL_PARSER */
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
struct synq_yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct synq_yyStackEntry synq_yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct synq_yyParser {
  synq_yyStackEntry *yytos;          /* Pointer to top element of the stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyhwm;                    /* High-water mark of the stack */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt;                 /* Shifts left before out of the error */
#endif
  synq_sqlite3ParserARG_SDECL                /* A place to hold %extra_argument */
  synq_sqlite3ParserCTX_SDECL                /* A place to hold %extra_context */
  synq_yyStackEntry *yystackEnd;           /* Last entry in the stack */
  synq_yyStackEntry *yystack;              /* The parser stack */
  synq_yyStackEntry yystk0[YYSTACKDEPTH];  /* Initial stack space */
};
typedef struct synq_yyParser synq_yyParser;

#include <assert.h>
#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
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
void synq_sqlite3ParserTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#if defined(YYCOVERAGE) || !defined(NDEBUG)
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
#ifndef _SYNQ_EXTERNAL_PARSER
static const char *const yyTokenName[] = { 
  /*    0 */ "$",
  /*    1 */ "INDEX",
  /*    2 */ "ALTER",
  /*    3 */ "EXPLAIN",
  /*    4 */ "ADD",
  /*    5 */ "QUERY",
  /*    6 */ "WINDOW",
  /*    7 */ "PLAN",
  /*    8 */ "OVER",
  /*    9 */ "BEGIN",
  /*   10 */ "FILTER",
  /*   11 */ "TRANSACTION",
  /*   12 */ "COLUMN",
  /*   13 */ "DEFERRED",
  /*   14 */ "AGG_FUNCTION",
  /*   15 */ "IMMEDIATE",
  /*   16 */ "AGG_COLUMN",
  /*   17 */ "EXCLUSIVE",
  /*   18 */ "TRUEFALSE",
  /*   19 */ "COMMIT",
  /*   20 */ "FUNCTION",
  /*   21 */ "END",
  /*   22 */ "UPLUS",
  /*   23 */ "ROLLBACK",
  /*   24 */ "UMINUS",
  /*   25 */ "SAVEPOINT",
  /*   26 */ "TRUTH",
  /*   27 */ "RELEASE",
  /*   28 */ "REGISTER",
  /*   29 */ "TO",
  /*   30 */ "VECTOR",
  /*   31 */ "TABLE",
  /*   32 */ "SELECT_COLUMN",
  /*   33 */ "CREATE",
  /*   34 */ "IF_NULL_ROW",
  /*   35 */ "IF",
  /*   36 */ "ASTERISK",
  /*   37 */ "NOT",
  /*   38 */ "SPAN",
  /*   39 */ "EXISTS",
  /*   40 */ "ERROR",
  /*   41 */ "TEMP",
  /*   42 */ "QNUMBER",
  /*   43 */ "AS",
  /*   44 */ "WITHOUT",
  /*   45 */ "ABORT",
  /*   46 */ "ACTION",
  /*   47 */ "AFTER",
  /*   48 */ "ANALYZE",
  /*   49 */ "ASC",
  /*   50 */ "ATTACH",
  /*   51 */ "BEFORE",
  /*   52 */ "BY",
  /*   53 */ "CASCADE",
  /*   54 */ "CAST",
  /*   55 */ "CONFLICT",
  /*   56 */ "DATABASE",
  /*   57 */ "DESC",
  /*   58 */ "DETACH",
  /*   59 */ "EACH",
  /*   60 */ "FAIL",
  /*   61 */ "OR",
  /*   62 */ "AND",
  /*   63 */ "IS",
  /*   64 */ "ISNOT",
  /*   65 */ "MATCH",
  /*   66 */ "LIKE_KW",
  /*   67 */ "BETWEEN",
  /*   68 */ "IN",
  /*   69 */ "ISNULL",
  /*   70 */ "NOTNULL",
  /*   71 */ "ESCAPE",
  /*   72 */ "COLUMNKW",
  /*   73 */ "DO",
  /*   74 */ "FOR",
  /*   75 */ "IGNORE",
  /*   76 */ "INITIALLY",
  /*   77 */ "INSTEAD",
  /*   78 */ "NO",
  /*   79 */ "KEY",
  /*   80 */ "OF",
  /*   81 */ "OFFSET",
  /*   82 */ "PRAGMA",
  /*   83 */ "RAISE",
  /*   84 */ "RECURSIVE",
  /*   85 */ "REPLACE",
  /*   86 */ "RESTRICT",
  /*   87 */ "ROW",
  /*   88 */ "ROWS",
  /*   89 */ "TRIGGER",
  /*   90 */ "VACUUM",
  /*   91 */ "VIEW",
  /*   92 */ "VIRTUAL",
  /*   93 */ "WITH",
  /*   94 */ "NULLS",
  /*   95 */ "FIRST",
  /*   96 */ "LAST",
  /*   97 */ "CURRENT",
  /*   98 */ "FOLLOWING",
  /*   99 */ "PARTITION",
  /*  100 */ "PRECEDING",
  /*  101 */ "RANGE",
  /*  102 */ "UNBOUNDED",
  /*  103 */ "EXCLUDE",
  /*  104 */ "GROUPS",
  /*  105 */ "OTHERS",
  /*  106 */ "TIES",
  /*  107 */ "GENERATED",
  /*  108 */ "ALWAYS",
  /*  109 */ "MATERIALIZED",
  /*  110 */ "REINDEX",
  /*  111 */ "RENAME",
  /*  112 */ "CTIME_KW",
  /*  113 */ "ANY",
  /*  114 */ "COLLATE",
  /*  115 */ "ON",
  /*  116 */ "INDEXED",
  /*  117 */ "JOIN_KW",
  /*  118 */ "CONSTRAINT",
  /*  119 */ "DEFAULT",
  /*  120 */ "NULL",
  /*  121 */ "PRIMARY",
  /*  122 */ "UNIQUE",
  /*  123 */ "CHECK",
  /*  124 */ "REFERENCES",
  /*  125 */ "AUTOINCR",
  /*  126 */ "INSERT",
  /*  127 */ "DELETE",
  /*  128 */ "UPDATE",
  /*  129 */ "SET",
  /*  130 */ "DEFERRABLE",
  /*  131 */ "FOREIGN",
  /*  132 */ "DROP",
  /*  133 */ "UNION",
  /*  134 */ "ALL",
  /*  135 */ "EXCEPT",
  /*  136 */ "INTERSECT",
  /*  137 */ "SELECT",
  /*  138 */ "VALUES",
  /*  139 */ "DISTINCT",
  /*  140 */ "FROM",
  /*  141 */ "JOIN",
  /*  142 */ "USING",
  /*  143 */ "ORDER",
  /*  144 */ "GROUP",
  /*  145 */ "HAVING",
  /*  146 */ "LIMIT",
  /*  147 */ "WHERE",
  /*  148 */ "RETURNING",
  /*  149 */ "INTO",
  /*  150 */ "NOTHING",
  /*  151 */ "CASE",
  /*  152 */ "WHEN",
  /*  153 */ "THEN",
  /*  154 */ "ELSE",
  /*  155 */ "NE",
  /*  156 */ "EQ",
  /*  157 */ "GT",
  /*  158 */ "LE",
  /*  159 */ "LT",
  /*  160 */ "GE",
  /*  161 */ "BITAND",
  /*  162 */ "BITOR",
  /*  163 */ "LSHIFT",
  /*  164 */ "RSHIFT",
  /*  165 */ "PLUS",
  /*  166 */ "MINUS",
  /*  167 */ "STAR",
  /*  168 */ "SLASH",
  /*  169 */ "REM",
  /*  170 */ "CONCAT",
  /*  171 */ "PTR",
  /*  172 */ "BITNOT",
  /*  173 */ "ID",
  /*  174 */ "LP",
  /*  175 */ "RP",
  /*  176 */ "COMMA",
  /*  177 */ "STRING",
  /*  178 */ "DOT",
  /*  179 */ "FLOAT",
  /*  180 */ "BLOB",
  /*  181 */ "INTEGER",
  /*  182 */ "VARIABLE",
  /*  183 */ "SEMI",
  /*  184 */ "SPACE",
  /*  185 */ "COMMENT",
  /*  186 */ "ILLEGAL",
  /*  187 */ "nm",
  /*  188 */ "as",
  /*  189 */ "scanpt",
  /*  190 */ "likeop",
  /*  191 */ "dbnm",
  /*  192 */ "multiselect_op",
  /*  193 */ "in_op",
  /*  194 */ "typetoken",
  /*  195 */ "typename",
  /*  196 */ "withnm",
  /*  197 */ "wqas",
  /*  198 */ "collate",
  /*  199 */ "raisetype",
  /*  200 */ "joinop",
  /*  201 */ "indexed_by",
  /*  202 */ "insert_cmd",
  /*  203 */ "orconf",
  /*  204 */ "resolvetype",
  /*  205 */ "indexed_opt",
  /*  206 */ "ifexists",
  /*  207 */ "transtype",
  /*  208 */ "trans_opt",
  /*  209 */ "savepoint_opt",
  /*  210 */ "kwcolumn_opt",
  /*  211 */ "columnname",
  /*  212 */ "ifnotexists",
  /*  213 */ "temp",
  /*  214 */ "uniqueflag",
  /*  215 */ "database_kw_opt",
  /*  216 */ "explain",
  /*  217 */ "createkw",
  /*  218 */ "signed",
  /*  219 */ "plus_num",
  /*  220 */ "minus_num",
  /*  221 */ "nmnum",
  /*  222 */ "autoinc",
  /*  223 */ "refargs",
  /*  224 */ "refarg",
  /*  225 */ "refact",
  /*  226 */ "defer_subclause",
  /*  227 */ "init_deferred_pred_opt",
  /*  228 */ "defer_subclause_opt",
  /*  229 */ "table_option_set",
  /*  230 */ "table_option",
  /*  231 */ "ccons",
  /*  232 */ "carglist",
  /*  233 */ "tcons",
  /*  234 */ "conslist",
  /*  235 */ "conslist_opt",
  /*  236 */ "tconscomma",
  /*  237 */ "onconf",
  /*  238 */ "scantok",
  /*  239 */ "generated",
  /*  240 */ "on_using",
  /*  241 */ "with",
  /*  242 */ "range_or_rows",
  /*  243 */ "frame_exclude_opt",
  /*  244 */ "frame_exclude",
  /*  245 */ "trigger_time",
  /*  246 */ "trnm",
  /*  247 */ "cmdx",
  /*  248 */ "cmd",
  /*  249 */ "create_table",
  /*  250 */ "create_table_args",
  /*  251 */ "columnlist",
  /*  252 */ "select",
  /*  253 */ "term",
  /*  254 */ "expr",
  /*  255 */ "sortorder",
  /*  256 */ "eidlist_opt",
  /*  257 */ "sortlist",
  /*  258 */ "eidlist",
  /*  259 */ "fullname",
  /*  260 */ "wqlist",
  /*  261 */ "selectnowith",
  /*  262 */ "oneselect",
  /*  263 */ "distinct",
  /*  264 */ "selcollist",
  /*  265 */ "from",
  /*  266 */ "where_opt",
  /*  267 */ "groupby_opt",
  /*  268 */ "having_opt",
  /*  269 */ "orderby_opt",
  /*  270 */ "limit_opt",
  /*  271 */ "window_clause",
  /*  272 */ "values",
  /*  273 */ "nexprlist",
  /*  274 */ "mvalues",
  /*  275 */ "sclp",
  /*  276 */ "seltablist",
  /*  277 */ "stl_prefix",
  /*  278 */ "exprlist",
  /*  279 */ "xfullname",
  /*  280 */ "idlist",
  /*  281 */ "nulls",
  /*  282 */ "where_opt_ret",
  /*  283 */ "setlist",
  /*  284 */ "idlist_opt",
  /*  285 */ "upsert",
  /*  286 */ "returning",
  /*  287 */ "filter_over",
  /*  288 */ "between_op",
  /*  289 */ "paren_exprlist",
  /*  290 */ "case_operand",
  /*  291 */ "case_exprlist",
  /*  292 */ "case_else",
  /*  293 */ "vinto",
  /*  294 */ "trigger_decl",
  /*  295 */ "trigger_cmd_list",
  /*  296 */ "trigger_event",
  /*  297 */ "foreach_clause",
  /*  298 */ "when_clause",
  /*  299 */ "trigger_cmd",
  /*  300 */ "tridxby",
  /*  301 */ "key_opt",
  /*  302 */ "add_column_fullname",
  /*  303 */ "create_vtab",
  /*  304 */ "vtabarglist",
  /*  305 */ "vtabarg",
  /*  306 */ "vtabargtoken",
  /*  307 */ "lp",
  /*  308 */ "anylist",
  /*  309 */ "wqitem",
  /*  310 */ "windowdefn_list",
  /*  311 */ "windowdefn",
  /*  312 */ "window",
  /*  313 */ "frame_opt",
  /*  314 */ "frame_bound_s",
  /*  315 */ "frame_bound_e",
  /*  316 */ "frame_bound",
  /*  317 */ "filter_clause",
  /*  318 */ "over_clause",
  /*  319 */ "input",
  /*  320 */ "cmdlist",
  /*  321 */ "ecmd",
};
#endif /* !_SYNQ_EXTERNAL_PARSER */
#endif /* defined(YYCOVERAGE) || !defined(NDEBUG) */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
#ifndef _SYNQ_EXTERNAL_PARSER
static const char *const yyRuleName[] = {
 /*   0 */ "explain ::= EXPLAIN",
 /*   1 */ "explain ::= EXPLAIN QUERY PLAN",
 /*   2 */ "cmdx ::= cmd",
 /*   3 */ "cmd ::= BEGIN transtype trans_opt",
 /*   4 */ "transtype ::=",
 /*   5 */ "transtype ::= DEFERRED",
 /*   6 */ "transtype ::= IMMEDIATE",
 /*   7 */ "transtype ::= EXCLUSIVE",
 /*   8 */ "cmd ::= COMMIT|END trans_opt",
 /*   9 */ "cmd ::= ROLLBACK trans_opt",
 /*  10 */ "cmd ::= SAVEPOINT nm",
 /*  11 */ "cmd ::= RELEASE savepoint_opt nm",
 /*  12 */ "cmd ::= ROLLBACK trans_opt TO savepoint_opt nm",
 /*  13 */ "create_table ::= createkw temp TABLE ifnotexists nm dbnm",
 /*  14 */ "createkw ::= CREATE",
 /*  15 */ "ifnotexists ::=",
 /*  16 */ "ifnotexists ::= IF NOT EXISTS",
 /*  17 */ "temp ::= TEMP",
 /*  18 */ "temp ::=",
 /*  19 */ "create_table_args ::= LP columnlist conslist_opt RP table_option_set",
 /*  20 */ "create_table_args ::= AS select",
 /*  21 */ "table_option_set ::=",
 /*  22 */ "table_option_set ::= table_option_set COMMA table_option",
 /*  23 */ "table_option ::= WITHOUT nm",
 /*  24 */ "table_option ::= nm",
 /*  25 */ "columnname ::= nm typetoken",
 /*  26 */ "typetoken ::=",
 /*  27 */ "typetoken ::= typename LP signed RP",
 /*  28 */ "typetoken ::= typename LP signed COMMA signed RP",
 /*  29 */ "typename ::= typename ID|STRING",
 /*  30 */ "scanpt ::=",
 /*  31 */ "scantok ::=",
 /*  32 */ "ccons ::= CONSTRAINT nm",
 /*  33 */ "ccons ::= DEFAULT scantok term",
 /*  34 */ "ccons ::= DEFAULT LP expr RP",
 /*  35 */ "ccons ::= DEFAULT PLUS scantok term",
 /*  36 */ "ccons ::= DEFAULT MINUS scantok term",
 /*  37 */ "ccons ::= DEFAULT scantok ID|INDEXED",
 /*  38 */ "ccons ::= NOT NULL onconf",
 /*  39 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
 /*  40 */ "ccons ::= UNIQUE onconf",
 /*  41 */ "ccons ::= CHECK LP expr RP",
 /*  42 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
 /*  43 */ "ccons ::= defer_subclause",
 /*  44 */ "ccons ::= COLLATE ID|STRING",
 /*  45 */ "generated ::= LP expr RP",
 /*  46 */ "generated ::= LP expr RP ID",
 /*  47 */ "autoinc ::=",
 /*  48 */ "autoinc ::= AUTOINCR",
 /*  49 */ "refargs ::=",
 /*  50 */ "refargs ::= refargs refarg",
 /*  51 */ "refarg ::= MATCH nm",
 /*  52 */ "refarg ::= ON INSERT refact",
 /*  53 */ "refarg ::= ON DELETE refact",
 /*  54 */ "refarg ::= ON UPDATE refact",
 /*  55 */ "refact ::= SET NULL",
 /*  56 */ "refact ::= SET DEFAULT",
 /*  57 */ "refact ::= CASCADE",
 /*  58 */ "refact ::= RESTRICT",
 /*  59 */ "refact ::= NO ACTION",
 /*  60 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
 /*  61 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
 /*  62 */ "init_deferred_pred_opt ::=",
 /*  63 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
 /*  64 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
 /*  65 */ "conslist_opt ::=",
 /*  66 */ "tconscomma ::= COMMA",
 /*  67 */ "tcons ::= CONSTRAINT nm",
 /*  68 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf",
 /*  69 */ "tcons ::= UNIQUE LP sortlist RP onconf",
 /*  70 */ "tcons ::= CHECK LP expr RP onconf",
 /*  71 */ "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /*  72 */ "defer_subclause_opt ::=",
 /*  73 */ "onconf ::=",
 /*  74 */ "onconf ::= ON CONFLICT resolvetype",
 /*  75 */ "orconf ::=",
 /*  76 */ "orconf ::= OR resolvetype",
 /*  77 */ "resolvetype ::= IGNORE",
 /*  78 */ "resolvetype ::= REPLACE",
 /*  79 */ "cmd ::= DROP TABLE ifexists fullname",
 /*  80 */ "ifexists ::= IF EXISTS",
 /*  81 */ "ifexists ::=",
 /*  82 */ "cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select",
 /*  83 */ "cmd ::= DROP VIEW ifexists fullname",
 /*  84 */ "cmd ::= select",
 /*  85 */ "select ::= WITH wqlist selectnowith",
 /*  86 */ "select ::= WITH RECURSIVE wqlist selectnowith",
 /*  87 */ "select ::= selectnowith",
 /*  88 */ "selectnowith ::= selectnowith multiselect_op oneselect",
 /*  89 */ "multiselect_op ::= UNION",
 /*  90 */ "multiselect_op ::= UNION ALL",
 /*  91 */ "multiselect_op ::= EXCEPT|INTERSECT",
 /*  92 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt",
 /*  93 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt",
 /*  94 */ "values ::= VALUES LP nexprlist RP",
 /*  95 */ "oneselect ::= mvalues",
 /*  96 */ "mvalues ::= values COMMA LP nexprlist RP",
 /*  97 */ "mvalues ::= mvalues COMMA LP nexprlist RP",
 /*  98 */ "distinct ::= DISTINCT",
 /*  99 */ "distinct ::= ALL",
 /* 100 */ "distinct ::=",
 /* 101 */ "sclp ::=",
 /* 102 */ "selcollist ::= sclp scanpt expr scanpt as",
 /* 103 */ "selcollist ::= sclp scanpt STAR",
 /* 104 */ "selcollist ::= sclp scanpt nm DOT STAR",
 /* 105 */ "as ::= AS nm",
 /* 106 */ "as ::=",
 /* 107 */ "from ::=",
 /* 108 */ "from ::= FROM seltablist",
 /* 109 */ "stl_prefix ::= seltablist joinop",
 /* 110 */ "stl_prefix ::=",
 /* 111 */ "seltablist ::= stl_prefix nm dbnm as on_using",
 /* 112 */ "seltablist ::= stl_prefix nm dbnm as indexed_by on_using",
 /* 113 */ "seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using",
 /* 114 */ "seltablist ::= stl_prefix LP select RP as on_using",
 /* 115 */ "seltablist ::= stl_prefix LP seltablist RP as on_using",
 /* 116 */ "dbnm ::=",
 /* 117 */ "dbnm ::= DOT nm",
 /* 118 */ "fullname ::= nm",
 /* 119 */ "fullname ::= nm DOT nm",
 /* 120 */ "xfullname ::= nm",
 /* 121 */ "xfullname ::= nm DOT nm",
 /* 122 */ "xfullname ::= nm DOT nm AS nm",
 /* 123 */ "xfullname ::= nm AS nm",
 /* 124 */ "joinop ::= COMMA|JOIN",
 /* 125 */ "joinop ::= JOIN_KW JOIN",
 /* 126 */ "joinop ::= JOIN_KW nm JOIN",
 /* 127 */ "joinop ::= JOIN_KW nm nm JOIN",
 /* 128 */ "on_using ::= ON expr",
 /* 129 */ "on_using ::= USING LP idlist RP",
 /* 130 */ "on_using ::=",
 /* 131 */ "indexed_opt ::=",
 /* 132 */ "indexed_by ::= INDEXED BY nm",
 /* 133 */ "indexed_by ::= NOT INDEXED",
 /* 134 */ "orderby_opt ::=",
 /* 135 */ "orderby_opt ::= ORDER BY sortlist",
 /* 136 */ "sortlist ::= sortlist COMMA expr sortorder nulls",
 /* 137 */ "sortlist ::= expr sortorder nulls",
 /* 138 */ "sortorder ::= ASC",
 /* 139 */ "sortorder ::= DESC",
 /* 140 */ "sortorder ::=",
 /* 141 */ "nulls ::= NULLS FIRST",
 /* 142 */ "nulls ::= NULLS LAST",
 /* 143 */ "nulls ::=",
 /* 144 */ "groupby_opt ::=",
 /* 145 */ "groupby_opt ::= GROUP BY nexprlist",
 /* 146 */ "having_opt ::=",
 /* 147 */ "having_opt ::= HAVING expr",
 /* 148 */ "limit_opt ::=",
 /* 149 */ "limit_opt ::= LIMIT expr",
 /* 150 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /* 151 */ "limit_opt ::= LIMIT expr COMMA expr",
 /* 152 */ "cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret",
 /* 153 */ "where_opt ::=",
 /* 154 */ "where_opt ::= WHERE expr",
 /* 155 */ "where_opt_ret ::=",
 /* 156 */ "where_opt_ret ::= WHERE expr",
 /* 157 */ "where_opt_ret ::= RETURNING selcollist",
 /* 158 */ "where_opt_ret ::= WHERE expr RETURNING selcollist",
 /* 159 */ "cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret",
 /* 160 */ "setlist ::= setlist COMMA nm EQ expr",
 /* 161 */ "setlist ::= setlist COMMA LP idlist RP EQ expr",
 /* 162 */ "setlist ::= nm EQ expr",
 /* 163 */ "setlist ::= LP idlist RP EQ expr",
 /* 164 */ "cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert",
 /* 165 */ "cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning",
 /* 166 */ "upsert ::=",
 /* 167 */ "upsert ::= RETURNING selcollist",
 /* 168 */ "upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert",
 /* 169 */ "upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert",
 /* 170 */ "upsert ::= ON CONFLICT DO NOTHING returning",
 /* 171 */ "upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning",
 /* 172 */ "returning ::= RETURNING selcollist",
 /* 173 */ "insert_cmd ::= INSERT orconf",
 /* 174 */ "insert_cmd ::= REPLACE",
 /* 175 */ "idlist_opt ::=",
 /* 176 */ "idlist_opt ::= LP idlist RP",
 /* 177 */ "idlist ::= idlist COMMA nm",
 /* 178 */ "idlist ::= nm",
 /* 179 */ "expr ::= LP expr RP",
 /* 180 */ "expr ::= ID|INDEXED|JOIN_KW",
 /* 181 */ "expr ::= nm DOT nm",
 /* 182 */ "expr ::= nm DOT nm DOT nm",
 /* 183 */ "term ::= NULL|FLOAT|BLOB",
 /* 184 */ "term ::= STRING",
 /* 185 */ "term ::= INTEGER",
 /* 186 */ "expr ::= VARIABLE",
 /* 187 */ "expr ::= expr COLLATE ID|STRING",
 /* 188 */ "expr ::= CAST LP expr AS typetoken RP",
 /* 189 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP",
 /* 190 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP",
 /* 191 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP",
 /* 192 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over",
 /* 193 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over",
 /* 194 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over",
 /* 195 */ "term ::= CTIME_KW",
 /* 196 */ "expr ::= LP nexprlist COMMA expr RP",
 /* 197 */ "expr ::= expr AND expr",
 /* 198 */ "expr ::= expr OR expr",
 /* 199 */ "expr ::= expr LT|GT|GE|LE expr",
 /* 200 */ "expr ::= expr EQ|NE expr",
 /* 201 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
 /* 202 */ "expr ::= expr PLUS|MINUS expr",
 /* 203 */ "expr ::= expr STAR|SLASH|REM expr",
 /* 204 */ "expr ::= expr CONCAT expr",
 /* 205 */ "likeop ::= NOT LIKE_KW|MATCH",
 /* 206 */ "expr ::= expr likeop expr",
 /* 207 */ "expr ::= expr likeop expr ESCAPE expr",
 /* 208 */ "expr ::= expr ISNULL|NOTNULL",
 /* 209 */ "expr ::= expr NOT NULL",
 /* 210 */ "expr ::= expr IS expr",
 /* 211 */ "expr ::= expr IS NOT expr",
 /* 212 */ "expr ::= expr IS NOT DISTINCT FROM expr",
 /* 213 */ "expr ::= expr IS DISTINCT FROM expr",
 /* 214 */ "expr ::= NOT expr",
 /* 215 */ "expr ::= BITNOT expr",
 /* 216 */ "expr ::= PLUS|MINUS expr",
 /* 217 */ "expr ::= expr PTR expr",
 /* 218 */ "between_op ::= BETWEEN",
 /* 219 */ "between_op ::= NOT BETWEEN",
 /* 220 */ "expr ::= expr between_op expr AND expr",
 /* 221 */ "in_op ::= IN",
 /* 222 */ "in_op ::= NOT IN",
 /* 223 */ "expr ::= expr in_op LP exprlist RP",
 /* 224 */ "expr ::= LP select RP",
 /* 225 */ "expr ::= expr in_op LP select RP",
 /* 226 */ "expr ::= expr in_op nm dbnm paren_exprlist",
 /* 227 */ "expr ::= EXISTS LP select RP",
 /* 228 */ "expr ::= CASE case_operand case_exprlist case_else END",
 /* 229 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
 /* 230 */ "case_exprlist ::= WHEN expr THEN expr",
 /* 231 */ "case_else ::= ELSE expr",
 /* 232 */ "case_else ::=",
 /* 233 */ "case_operand ::=",
 /* 234 */ "exprlist ::=",
 /* 235 */ "nexprlist ::= nexprlist COMMA expr",
 /* 236 */ "nexprlist ::= expr",
 /* 237 */ "paren_exprlist ::=",
 /* 238 */ "paren_exprlist ::= LP exprlist RP",
 /* 239 */ "cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt",
 /* 240 */ "uniqueflag ::= UNIQUE",
 /* 241 */ "uniqueflag ::=",
 /* 242 */ "eidlist_opt ::=",
 /* 243 */ "eidlist_opt ::= LP eidlist RP",
 /* 244 */ "eidlist ::= eidlist COMMA nm collate sortorder",
 /* 245 */ "eidlist ::= nm collate sortorder",
 /* 246 */ "collate ::=",
 /* 247 */ "collate ::= COLLATE ID|STRING",
 /* 248 */ "cmd ::= DROP INDEX ifexists fullname",
 /* 249 */ "cmd ::= VACUUM vinto",
 /* 250 */ "cmd ::= VACUUM nm vinto",
 /* 251 */ "vinto ::= INTO expr",
 /* 252 */ "vinto ::=",
 /* 253 */ "cmd ::= PRAGMA nm dbnm",
 /* 254 */ "cmd ::= PRAGMA nm dbnm EQ nmnum",
 /* 255 */ "cmd ::= PRAGMA nm dbnm LP nmnum RP",
 /* 256 */ "cmd ::= PRAGMA nm dbnm EQ minus_num",
 /* 257 */ "cmd ::= PRAGMA nm dbnm LP minus_num RP",
 /* 258 */ "plus_num ::= PLUS INTEGER|FLOAT",
 /* 259 */ "minus_num ::= MINUS INTEGER|FLOAT",
 /* 260 */ "cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END",
 /* 261 */ "trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause",
 /* 262 */ "trigger_time ::= BEFORE|AFTER",
 /* 263 */ "trigger_time ::= INSTEAD OF",
 /* 264 */ "trigger_time ::=",
 /* 265 */ "trigger_event ::= DELETE|INSERT",
 /* 266 */ "trigger_event ::= UPDATE",
 /* 267 */ "trigger_event ::= UPDATE OF idlist",
 /* 268 */ "when_clause ::=",
 /* 269 */ "when_clause ::= WHEN expr",
 /* 270 */ "trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI",
 /* 271 */ "trigger_cmd_list ::= trigger_cmd SEMI",
 /* 272 */ "trnm ::= nm DOT nm",
 /* 273 */ "tridxby ::= INDEXED BY nm",
 /* 274 */ "tridxby ::= NOT INDEXED",
 /* 275 */ "trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt",
 /* 276 */ "trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt",
 /* 277 */ "trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt",
 /* 278 */ "trigger_cmd ::= scanpt select scanpt",
 /* 279 */ "expr ::= RAISE LP IGNORE RP",
 /* 280 */ "expr ::= RAISE LP raisetype COMMA expr RP",
 /* 281 */ "raisetype ::= ROLLBACK",
 /* 282 */ "raisetype ::= ABORT",
 /* 283 */ "raisetype ::= FAIL",
 /* 284 */ "cmd ::= DROP TRIGGER ifexists fullname",
 /* 285 */ "cmd ::= ATTACH database_kw_opt expr AS expr key_opt",
 /* 286 */ "cmd ::= DETACH database_kw_opt expr",
 /* 287 */ "key_opt ::=",
 /* 288 */ "key_opt ::= KEY expr",
 /* 289 */ "cmd ::= REINDEX",
 /* 290 */ "cmd ::= REINDEX nm dbnm",
 /* 291 */ "cmd ::= ANALYZE",
 /* 292 */ "cmd ::= ANALYZE nm dbnm",
 /* 293 */ "cmd ::= ALTER TABLE fullname RENAME TO nm",
 /* 294 */ "cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist",
 /* 295 */ "cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm",
 /* 296 */ "add_column_fullname ::= fullname",
 /* 297 */ "cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm",
 /* 298 */ "cmd ::= create_vtab",
 /* 299 */ "cmd ::= create_vtab LP vtabarglist RP",
 /* 300 */ "create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm",
 /* 301 */ "vtabarg ::=",
 /* 302 */ "vtabargtoken ::= ANY",
 /* 303 */ "vtabargtoken ::= lp anylist RP",
 /* 304 */ "lp ::= LP",
 /* 305 */ "with ::= WITH wqlist",
 /* 306 */ "with ::= WITH RECURSIVE wqlist",
 /* 307 */ "wqas ::= AS",
 /* 308 */ "wqas ::= AS MATERIALIZED",
 /* 309 */ "wqas ::= AS NOT MATERIALIZED",
 /* 310 */ "wqitem ::= withnm eidlist_opt wqas LP select RP",
 /* 311 */ "withnm ::= nm",
 /* 312 */ "wqlist ::= wqitem",
 /* 313 */ "wqlist ::= wqlist COMMA wqitem",
 /* 314 */ "windowdefn_list ::= windowdefn_list COMMA windowdefn",
 /* 315 */ "windowdefn ::= nm AS LP window RP",
 /* 316 */ "window ::= PARTITION BY nexprlist orderby_opt frame_opt",
 /* 317 */ "window ::= nm PARTITION BY nexprlist orderby_opt frame_opt",
 /* 318 */ "window ::= ORDER BY sortlist frame_opt",
 /* 319 */ "window ::= nm ORDER BY sortlist frame_opt",
 /* 320 */ "window ::= nm frame_opt",
 /* 321 */ "frame_opt ::=",
 /* 322 */ "frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt",
 /* 323 */ "frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt",
 /* 324 */ "range_or_rows ::= RANGE|ROWS|GROUPS",
 /* 325 */ "frame_bound_s ::= frame_bound",
 /* 326 */ "frame_bound_s ::= UNBOUNDED PRECEDING",
 /* 327 */ "frame_bound_e ::= frame_bound",
 /* 328 */ "frame_bound_e ::= UNBOUNDED FOLLOWING",
 /* 329 */ "frame_bound ::= expr PRECEDING|FOLLOWING",
 /* 330 */ "frame_bound ::= CURRENT ROW",
 /* 331 */ "frame_exclude_opt ::=",
 /* 332 */ "frame_exclude_opt ::= EXCLUDE frame_exclude",
 /* 333 */ "frame_exclude ::= NO OTHERS",
 /* 334 */ "frame_exclude ::= CURRENT ROW",
 /* 335 */ "frame_exclude ::= GROUP|TIES",
 /* 336 */ "window_clause ::= WINDOW windowdefn_list",
 /* 337 */ "filter_over ::= filter_clause over_clause",
 /* 338 */ "filter_over ::= over_clause",
 /* 339 */ "filter_over ::= filter_clause",
 /* 340 */ "over_clause ::= OVER LP window RP",
 /* 341 */ "over_clause ::= OVER nm",
 /* 342 */ "filter_clause ::= FILTER LP WHERE expr RP",
 /* 343 */ "term ::= QNUMBER",
 /* 344 */ "input ::= cmdlist",
 /* 345 */ "cmdlist ::= cmdlist ecmd",
 /* 346 */ "cmdlist ::= ecmd",
 /* 347 */ "ecmd ::= SEMI",
 /* 348 */ "ecmd ::= cmdx SEMI",
 /* 349 */ "ecmd ::= explain cmdx SEMI",
 /* 350 */ "trans_opt ::=",
 /* 351 */ "trans_opt ::= TRANSACTION",
 /* 352 */ "trans_opt ::= TRANSACTION nm",
 /* 353 */ "savepoint_opt ::= SAVEPOINT",
 /* 354 */ "savepoint_opt ::=",
 /* 355 */ "cmd ::= create_table create_table_args",
 /* 356 */ "table_option_set ::= table_option",
 /* 357 */ "columnlist ::= columnlist COMMA columnname carglist",
 /* 358 */ "columnlist ::= columnname carglist",
 /* 359 */ "nm ::= ID|INDEXED|JOIN_KW",
 /* 360 */ "nm ::= STRING",
 /* 361 */ "typetoken ::= typename",
 /* 362 */ "typename ::= ID|STRING",
 /* 363 */ "signed ::= plus_num",
 /* 364 */ "signed ::= minus_num",
 /* 365 */ "carglist ::= carglist ccons",
 /* 366 */ "carglist ::=",
 /* 367 */ "ccons ::= NULL onconf",
 /* 368 */ "ccons ::= GENERATED ALWAYS AS generated",
 /* 369 */ "ccons ::= AS generated",
 /* 370 */ "conslist_opt ::= COMMA conslist",
 /* 371 */ "conslist ::= conslist tconscomma tcons",
 /* 372 */ "conslist ::= tcons",
 /* 373 */ "tconscomma ::=",
 /* 374 */ "defer_subclause_opt ::= defer_subclause",
 /* 375 */ "resolvetype ::= raisetype",
 /* 376 */ "selectnowith ::= oneselect",
 /* 377 */ "oneselect ::= values",
 /* 378 */ "sclp ::= selcollist COMMA",
 /* 379 */ "as ::= ID|STRING",
 /* 380 */ "indexed_opt ::= indexed_by",
 /* 381 */ "returning ::=",
 /* 382 */ "expr ::= term",
 /* 383 */ "likeop ::= LIKE_KW|MATCH",
 /* 384 */ "case_operand ::= expr",
 /* 385 */ "exprlist ::= nexprlist",
 /* 386 */ "nmnum ::= plus_num",
 /* 387 */ "nmnum ::= nm",
 /* 388 */ "nmnum ::= ON",
 /* 389 */ "nmnum ::= DELETE",
 /* 390 */ "nmnum ::= DEFAULT",
 /* 391 */ "plus_num ::= INTEGER|FLOAT",
 /* 392 */ "foreach_clause ::=",
 /* 393 */ "foreach_clause ::= FOR EACH ROW",
 /* 394 */ "trnm ::= nm",
 /* 395 */ "tridxby ::=",
 /* 396 */ "database_kw_opt ::= DATABASE",
 /* 397 */ "database_kw_opt ::=",
 /* 398 */ "kwcolumn_opt ::=",
 /* 399 */ "kwcolumn_opt ::= COLUMNKW",
 /* 400 */ "vtabarglist ::= vtabarg",
 /* 401 */ "vtabarglist ::= vtabarglist COMMA vtabarg",
 /* 402 */ "vtabarg ::= vtabarg vtabargtoken",
 /* 403 */ "anylist ::=",
 /* 404 */ "anylist ::= anylist LP anylist RP",
 /* 405 */ "anylist ::= anylist ANY",
 /* 406 */ "with ::=",
 /* 407 */ "windowdefn_list ::= windowdefn",
 /* 408 */ "window ::= frame_opt",
};
#endif /* !_SYNQ_EXTERNAL_PARSER */
#endif /* NDEBUG */


#if YYGROWABLESTACK
/*
** Try to increase the size of the parser stack.  Return the number
** of errors.  Return 0 on success.
*/
static int yyGrowStack(synq_yyParser *p){
  int oldSize = 1 + (int)(p->yystackEnd - p->yystack);
  int newSize;
  int idx;
  synq_yyStackEntry *pNew;

  newSize = oldSize*2 + 100;
  idx = (int)(p->yytos - p->yystack);
  if( p->yystack==p->yystk0 ){
    pNew = YYREALLOC(0, newSize*sizeof(pNew[0]));
    if( pNew==0 ) return 1;
    memcpy(pNew, p->yystack, oldSize*sizeof(pNew[0]));
  }else{
    pNew = YYREALLOC(p->yystack, newSize*sizeof(pNew[0]));
    if( pNew==0 ) return 1;
  }
  p->yystack = pNew;
  p->yytos = &p->yystack[idx];
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sStack grows from %d to %d entries.\n",
            yyTracePrompt, oldSize, newSize);
  }
#endif
  p->yystackEnd = &p->yystack[newSize-1];
  return 0;
}
#endif /* YYGROWABLESTACK */

#if !YYGROWABLESTACK
/* For builds that do no have a growable stack, yyGrowStack always
** returns an error.
*/
# define yyGrowStack(X) 1
#endif

/* Datatype of the argument to the memory allocated passed as the
** second argument to synq_sqlite3ParserAlloc() below.  This can be changed by
** putting an appropriate #define in the %include section of the input
** grammar.
*/
#ifndef YYMALLOCARGTYPE
# define YYMALLOCARGTYPE size_t
#endif

/* Initialize a new parser that has already been allocated.
*/
void synq_sqlite3ParserInit(void *yypRawParser synq_sqlite3ParserCTX_PDECL){
  synq_yyParser *yypParser = (synq_yyParser*)yypRawParser;
  synq_sqlite3ParserCTX_STORE
#ifdef YYTRACKMAXSTACKDEPTH
  yypParser->yyhwm = 0;
#endif
  yypParser->yystack = yypParser->yystk0;
  yypParser->yystackEnd = &yypParser->yystack[YYSTACKDEPTH-1];
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
  yypParser->yytos = yypParser->yystack;
  yypParser->yystack[0].stateno = 0;
  yypParser->yystack[0].major = 0;
}

#ifndef synq_sqlite3Parser_ENGINEALWAYSONSTACK
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
** to synq_sqlite3Parser and synq_sqlite3ParserFree.
*/
void *synq_sqlite3ParserAlloc(void *(*mallocProc)(YYMALLOCARGTYPE) synq_sqlite3ParserCTX_PDECL){
  synq_yyParser *yypParser;
  yypParser = (synq_yyParser*)(*mallocProc)( (YYMALLOCARGTYPE)sizeof(synq_yyParser) );
  if( yypParser ){
    synq_sqlite3ParserCTX_STORE
    synq_sqlite3ParserInit(yypParser synq_sqlite3ParserCTX_PARAM);
  }
  return (void*)yypParser;
}
#endif /* synq_sqlite3Parser_ENGINEALWAYSONSTACK */


/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the 
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
static void yy_destructor(
  synq_yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  synq_sqlite3ParserARG_FETCH
  synq_sqlite3ParserCTX_FETCH
  switch( yymajor ){
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
/********* Begin destructor definitions ***************************************/
/********* End destructor definitions *****************************************/
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
static void yy_pop_parser_stack(synq_yyParser *pParser){
  synq_yyStackEntry *yytos;
  assert( pParser->yytos!=0 );
  assert( pParser->yytos > pParser->yystack );
  yytos = pParser->yytos--;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yy_destructor(pParser, yytos->major, &yytos->minor);
}

/*
** Clear all secondary memory allocations from the parser
*/
void synq_sqlite3ParserFinalize(void *p){
  synq_yyParser *pParser = (synq_yyParser*)p;

  /* In-lined version of calling yy_pop_parser_stack() for each
  ** element left in the stack */
  synq_yyStackEntry *yytos = pParser->yytos;
  while( yytos>pParser->yystack ){
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sPopping %s\n",
        yyTracePrompt,
        yyTokenName[yytos->major]);
    }
#endif
    if( yytos->major>=YY_MIN_DSTRCTR ){
      yy_destructor(pParser, yytos->major, &yytos->minor);
    }
    yytos--;
  }

#if YYGROWABLESTACK
  if( pParser->yystack!=pParser->yystk0 ) YYFREE(pParser->yystack);
#endif
}

#ifndef synq_sqlite3Parser_ENGINEALWAYSONSTACK
/* 
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/
void synq_sqlite3ParserFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
#ifndef YYPARSEFREENEVERNULL
  if( p==0 ) return;
#endif
  synq_sqlite3ParserFinalize(p);
  (*freeProc)(p);
}
#endif /* synq_sqlite3Parser_ENGINEALWAYSONSTACK */

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int synq_sqlite3ParserStackPeak(void *p){
  synq_yyParser *pParser = (synq_yyParser*)p;
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
int synq_sqlite3ParserCoverage(FILE *out){
  int stateno, iLookAhead, i;
  int nMissed = 0;
  for(stateno=0; stateno<YYNSTATE; stateno++){
    i = yy_shift_ofst[stateno];
    for(iLookAhead=0; iLookAhead<YYNTOKEN; iLookAhead++){
      if( yy_lookahead[i+iLookAhead]!=iLookAhead ) continue;
      if( yycoverage[stateno][iLookAhead]==0 ) nMissed++;
      if( out ){
        fprintf(out,"State %d lookahead %s %s\n", stateno,
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
  YYCODETYPE iLookAhead,    /* The look-ahead token */
  YYACTIONTYPE stateno      /* Current state number */
){
  int i;

  if( stateno>YY_MAX_SHIFT ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
#if defined(YYCOVERAGE)
  yycoverage[stateno][iLookAhead] = 1;
#endif
  do{
    i = yy_shift_ofst[stateno];
    assert( i>=0 );
    assert( i<=YY_ACTTAB_COUNT );
    assert( i+YYNTOKEN<=(int)YY_NLOOKAHEAD );
    assert( iLookAhead!=YYNOCODE );
    assert( iLookAhead < YYNTOKEN );
    i += iLookAhead;
    assert( i<(int)YY_NLOOKAHEAD );
    if( yy_lookahead[i]!=iLookAhead ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      assert( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0]) );
      iFallback = yyFallback[iLookAhead];
      if( iFallback!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        assert( yyFallback[iFallback]==0 ); /* Fallback loop must terminate */
        iLookAhead = iFallback;
        continue;
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        assert( j<(int)(sizeof(yy_lookahead)/sizeof(yy_lookahead[0])) );
        if( yy_lookahead[j]==YYWILDCARD && iLookAhead>0 ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead],
               yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
      return yy_default[stateno];
    }else{
      assert( i>=0 && i<(int)(sizeof(yy_action)/sizeof(yy_action[0])) );
      return yy_action[i];
    }
  }while(1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
static YYACTIONTYPE yy_find_reduce_action(
  YYACTIONTYPE stateno,     /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(synq_yyParser *yypParser){
   synq_sqlite3ParserARG_FETCH
   synq_sqlite3ParserCTX_FETCH
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/

  if( pCtx && pCtx->onStackOverflow ) {
    pCtx->onStackOverflow(pCtx);
  }
/******** End %stack_overflow code ********************************************/
   synq_sqlite3ParserARG_STORE /* Suppress warning about unused %extra_argument var */
   synq_sqlite3ParserCTX_STORE
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(synq_yyParser *yypParser, int yyNewState, const char *zTag){
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%s%s '%s', go to state %d\n",
         yyTracePrompt, zTag, yyTokenName[yypParser->yytos->major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%s%s '%s', pending reduce %d\n",
         yyTracePrompt, zTag, yyTokenName[yypParser->yytos->major],
         yyNewState - YY_MIN_REDUCE);
    }
  }
}
#else
# define yyTraceShift(X,Y,Z)
#endif

/*
** Perform a shift action.
*/
static void yy_shift(
  synq_yyParser *yypParser,          /* The parser to be shifted */
  YYACTIONTYPE yyNewState,      /* The new state to shift in */
  YYCODETYPE yyMajor,           /* The major token to shift in */
  synq_sqlite3ParserTOKENTYPE yyMinor        /* The minor token to shift in */
){
  synq_yyStackEntry *yytos;
  yypParser->yytos++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
    yypParser->yyhwm++;
    assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack) );
  }
#endif
  yytos = yypParser->yytos;
  if( yytos>yypParser->yystackEnd ){
    if( yyGrowStack(yypParser) ){
      yypParser->yytos--;
      yyStackOverflow(yypParser);
      return;
    }
    yytos = yypParser->yytos;
    assert( yytos <= yypParser->yystackEnd );
  }
  if( yyNewState > YY_MAX_SHIFT ){
    yyNewState += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
  }
  yytos->stateno = yyNewState;
  yytos->major = yyMajor;
  yytos->minor.yy0 = yyMinor;
  yyTraceShift(yypParser, yyNewState, "Shift");
}

/* For rule J, yyRuleInfoLhs[J] contains the symbol on the left-hand side
** of that rule */
#ifndef _SYNQ_EXTERNAL_PARSER
static const YYCODETYPE yyRuleInfoLhs[] = {
   216,  /* (0) explain ::= EXPLAIN */
   216,  /* (1) explain ::= EXPLAIN QUERY PLAN */
   247,  /* (2) cmdx ::= cmd */
   248,  /* (3) cmd ::= BEGIN transtype trans_opt */
   207,  /* (4) transtype ::= */
   207,  /* (5) transtype ::= DEFERRED */
   207,  /* (6) transtype ::= IMMEDIATE */
   207,  /* (7) transtype ::= EXCLUSIVE */
   248,  /* (8) cmd ::= COMMIT|END trans_opt */
   248,  /* (9) cmd ::= ROLLBACK trans_opt */
   248,  /* (10) cmd ::= SAVEPOINT nm */
   248,  /* (11) cmd ::= RELEASE savepoint_opt nm */
   248,  /* (12) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
   249,  /* (13) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   217,  /* (14) createkw ::= CREATE */
   212,  /* (15) ifnotexists ::= */
   212,  /* (16) ifnotexists ::= IF NOT EXISTS */
   213,  /* (17) temp ::= TEMP */
   213,  /* (18) temp ::= */
   250,  /* (19) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   250,  /* (20) create_table_args ::= AS select */
   229,  /* (21) table_option_set ::= */
   229,  /* (22) table_option_set ::= table_option_set COMMA table_option */
   230,  /* (23) table_option ::= WITHOUT nm */
   230,  /* (24) table_option ::= nm */
   211,  /* (25) columnname ::= nm typetoken */
   194,  /* (26) typetoken ::= */
   194,  /* (27) typetoken ::= typename LP signed RP */
   194,  /* (28) typetoken ::= typename LP signed COMMA signed RP */
   195,  /* (29) typename ::= typename ID|STRING */
   189,  /* (30) scanpt ::= */
   238,  /* (31) scantok ::= */
   231,  /* (32) ccons ::= CONSTRAINT nm */
   231,  /* (33) ccons ::= DEFAULT scantok term */
   231,  /* (34) ccons ::= DEFAULT LP expr RP */
   231,  /* (35) ccons ::= DEFAULT PLUS scantok term */
   231,  /* (36) ccons ::= DEFAULT MINUS scantok term */
   231,  /* (37) ccons ::= DEFAULT scantok ID|INDEXED */
   231,  /* (38) ccons ::= NOT NULL onconf */
   231,  /* (39) ccons ::= PRIMARY KEY sortorder onconf autoinc */
   231,  /* (40) ccons ::= UNIQUE onconf */
   231,  /* (41) ccons ::= CHECK LP expr RP */
   231,  /* (42) ccons ::= REFERENCES nm eidlist_opt refargs */
   231,  /* (43) ccons ::= defer_subclause */
   231,  /* (44) ccons ::= COLLATE ID|STRING */
   239,  /* (45) generated ::= LP expr RP */
   239,  /* (46) generated ::= LP expr RP ID */
   222,  /* (47) autoinc ::= */
   222,  /* (48) autoinc ::= AUTOINCR */
   223,  /* (49) refargs ::= */
   223,  /* (50) refargs ::= refargs refarg */
   224,  /* (51) refarg ::= MATCH nm */
   224,  /* (52) refarg ::= ON INSERT refact */
   224,  /* (53) refarg ::= ON DELETE refact */
   224,  /* (54) refarg ::= ON UPDATE refact */
   225,  /* (55) refact ::= SET NULL */
   225,  /* (56) refact ::= SET DEFAULT */
   225,  /* (57) refact ::= CASCADE */
   225,  /* (58) refact ::= RESTRICT */
   225,  /* (59) refact ::= NO ACTION */
   226,  /* (60) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
   226,  /* (61) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
   227,  /* (62) init_deferred_pred_opt ::= */
   227,  /* (63) init_deferred_pred_opt ::= INITIALLY DEFERRED */
   227,  /* (64) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
   235,  /* (65) conslist_opt ::= */
   236,  /* (66) tconscomma ::= COMMA */
   233,  /* (67) tcons ::= CONSTRAINT nm */
   233,  /* (68) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
   233,  /* (69) tcons ::= UNIQUE LP sortlist RP onconf */
   233,  /* (70) tcons ::= CHECK LP expr RP onconf */
   233,  /* (71) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
   228,  /* (72) defer_subclause_opt ::= */
   237,  /* (73) onconf ::= */
   237,  /* (74) onconf ::= ON CONFLICT resolvetype */
   203,  /* (75) orconf ::= */
   203,  /* (76) orconf ::= OR resolvetype */
   204,  /* (77) resolvetype ::= IGNORE */
   204,  /* (78) resolvetype ::= REPLACE */
   248,  /* (79) cmd ::= DROP TABLE ifexists fullname */
   206,  /* (80) ifexists ::= IF EXISTS */
   206,  /* (81) ifexists ::= */
   248,  /* (82) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   248,  /* (83) cmd ::= DROP VIEW ifexists fullname */
   248,  /* (84) cmd ::= select */
   252,  /* (85) select ::= WITH wqlist selectnowith */
   252,  /* (86) select ::= WITH RECURSIVE wqlist selectnowith */
   252,  /* (87) select ::= selectnowith */
   261,  /* (88) selectnowith ::= selectnowith multiselect_op oneselect */
   192,  /* (89) multiselect_op ::= UNION */
   192,  /* (90) multiselect_op ::= UNION ALL */
   192,  /* (91) multiselect_op ::= EXCEPT|INTERSECT */
   262,  /* (92) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
   262,  /* (93) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   272,  /* (94) values ::= VALUES LP nexprlist RP */
   262,  /* (95) oneselect ::= mvalues */
   274,  /* (96) mvalues ::= values COMMA LP nexprlist RP */
   274,  /* (97) mvalues ::= mvalues COMMA LP nexprlist RP */
   263,  /* (98) distinct ::= DISTINCT */
   263,  /* (99) distinct ::= ALL */
   263,  /* (100) distinct ::= */
   275,  /* (101) sclp ::= */
   264,  /* (102) selcollist ::= sclp scanpt expr scanpt as */
   264,  /* (103) selcollist ::= sclp scanpt STAR */
   264,  /* (104) selcollist ::= sclp scanpt nm DOT STAR */
   188,  /* (105) as ::= AS nm */
   188,  /* (106) as ::= */
   265,  /* (107) from ::= */
   265,  /* (108) from ::= FROM seltablist */
   277,  /* (109) stl_prefix ::= seltablist joinop */
   277,  /* (110) stl_prefix ::= */
   276,  /* (111) seltablist ::= stl_prefix nm dbnm as on_using */
   276,  /* (112) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   276,  /* (113) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   276,  /* (114) seltablist ::= stl_prefix LP select RP as on_using */
   276,  /* (115) seltablist ::= stl_prefix LP seltablist RP as on_using */
   191,  /* (116) dbnm ::= */
   191,  /* (117) dbnm ::= DOT nm */
   259,  /* (118) fullname ::= nm */
   259,  /* (119) fullname ::= nm DOT nm */
   279,  /* (120) xfullname ::= nm */
   279,  /* (121) xfullname ::= nm DOT nm */
   279,  /* (122) xfullname ::= nm DOT nm AS nm */
   279,  /* (123) xfullname ::= nm AS nm */
   200,  /* (124) joinop ::= COMMA|JOIN */
   200,  /* (125) joinop ::= JOIN_KW JOIN */
   200,  /* (126) joinop ::= JOIN_KW nm JOIN */
   200,  /* (127) joinop ::= JOIN_KW nm nm JOIN */
   240,  /* (128) on_using ::= ON expr */
   240,  /* (129) on_using ::= USING LP idlist RP */
   240,  /* (130) on_using ::= */
   205,  /* (131) indexed_opt ::= */
   201,  /* (132) indexed_by ::= INDEXED BY nm */
   201,  /* (133) indexed_by ::= NOT INDEXED */
   269,  /* (134) orderby_opt ::= */
   269,  /* (135) orderby_opt ::= ORDER BY sortlist */
   257,  /* (136) sortlist ::= sortlist COMMA expr sortorder nulls */
   257,  /* (137) sortlist ::= expr sortorder nulls */
   255,  /* (138) sortorder ::= ASC */
   255,  /* (139) sortorder ::= DESC */
   255,  /* (140) sortorder ::= */
   281,  /* (141) nulls ::= NULLS FIRST */
   281,  /* (142) nulls ::= NULLS LAST */
   281,  /* (143) nulls ::= */
   267,  /* (144) groupby_opt ::= */
   267,  /* (145) groupby_opt ::= GROUP BY nexprlist */
   268,  /* (146) having_opt ::= */
   268,  /* (147) having_opt ::= HAVING expr */
   270,  /* (148) limit_opt ::= */
   270,  /* (149) limit_opt ::= LIMIT expr */
   270,  /* (150) limit_opt ::= LIMIT expr OFFSET expr */
   270,  /* (151) limit_opt ::= LIMIT expr COMMA expr */
   248,  /* (152) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
   266,  /* (153) where_opt ::= */
   266,  /* (154) where_opt ::= WHERE expr */
   282,  /* (155) where_opt_ret ::= */
   282,  /* (156) where_opt_ret ::= WHERE expr */
   282,  /* (157) where_opt_ret ::= RETURNING selcollist */
   282,  /* (158) where_opt_ret ::= WHERE expr RETURNING selcollist */
   248,  /* (159) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   283,  /* (160) setlist ::= setlist COMMA nm EQ expr */
   283,  /* (161) setlist ::= setlist COMMA LP idlist RP EQ expr */
   283,  /* (162) setlist ::= nm EQ expr */
   283,  /* (163) setlist ::= LP idlist RP EQ expr */
   248,  /* (164) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   248,  /* (165) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
   285,  /* (166) upsert ::= */
   285,  /* (167) upsert ::= RETURNING selcollist */
   285,  /* (168) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
   285,  /* (169) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
   285,  /* (170) upsert ::= ON CONFLICT DO NOTHING returning */
   285,  /* (171) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
   286,  /* (172) returning ::= RETURNING selcollist */
   202,  /* (173) insert_cmd ::= INSERT orconf */
   202,  /* (174) insert_cmd ::= REPLACE */
   284,  /* (175) idlist_opt ::= */
   284,  /* (176) idlist_opt ::= LP idlist RP */
   280,  /* (177) idlist ::= idlist COMMA nm */
   280,  /* (178) idlist ::= nm */
   254,  /* (179) expr ::= LP expr RP */
   254,  /* (180) expr ::= ID|INDEXED|JOIN_KW */
   254,  /* (181) expr ::= nm DOT nm */
   254,  /* (182) expr ::= nm DOT nm DOT nm */
   253,  /* (183) term ::= NULL|FLOAT|BLOB */
   253,  /* (184) term ::= STRING */
   253,  /* (185) term ::= INTEGER */
   254,  /* (186) expr ::= VARIABLE */
   254,  /* (187) expr ::= expr COLLATE ID|STRING */
   254,  /* (188) expr ::= CAST LP expr AS typetoken RP */
   254,  /* (189) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   254,  /* (190) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   254,  /* (191) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   254,  /* (192) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   254,  /* (193) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   254,  /* (194) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   253,  /* (195) term ::= CTIME_KW */
   254,  /* (196) expr ::= LP nexprlist COMMA expr RP */
   254,  /* (197) expr ::= expr AND expr */
   254,  /* (198) expr ::= expr OR expr */
   254,  /* (199) expr ::= expr LT|GT|GE|LE expr */
   254,  /* (200) expr ::= expr EQ|NE expr */
   254,  /* (201) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
   254,  /* (202) expr ::= expr PLUS|MINUS expr */
   254,  /* (203) expr ::= expr STAR|SLASH|REM expr */
   254,  /* (204) expr ::= expr CONCAT expr */
   190,  /* (205) likeop ::= NOT LIKE_KW|MATCH */
   254,  /* (206) expr ::= expr likeop expr */
   254,  /* (207) expr ::= expr likeop expr ESCAPE expr */
   254,  /* (208) expr ::= expr ISNULL|NOTNULL */
   254,  /* (209) expr ::= expr NOT NULL */
   254,  /* (210) expr ::= expr IS expr */
   254,  /* (211) expr ::= expr IS NOT expr */
   254,  /* (212) expr ::= expr IS NOT DISTINCT FROM expr */
   254,  /* (213) expr ::= expr IS DISTINCT FROM expr */
   254,  /* (214) expr ::= NOT expr */
   254,  /* (215) expr ::= BITNOT expr */
   254,  /* (216) expr ::= PLUS|MINUS expr */
   254,  /* (217) expr ::= expr PTR expr */
   288,  /* (218) between_op ::= BETWEEN */
   288,  /* (219) between_op ::= NOT BETWEEN */
   254,  /* (220) expr ::= expr between_op expr AND expr */
   193,  /* (221) in_op ::= IN */
   193,  /* (222) in_op ::= NOT IN */
   254,  /* (223) expr ::= expr in_op LP exprlist RP */
   254,  /* (224) expr ::= LP select RP */
   254,  /* (225) expr ::= expr in_op LP select RP */
   254,  /* (226) expr ::= expr in_op nm dbnm paren_exprlist */
   254,  /* (227) expr ::= EXISTS LP select RP */
   254,  /* (228) expr ::= CASE case_operand case_exprlist case_else END */
   291,  /* (229) case_exprlist ::= case_exprlist WHEN expr THEN expr */
   291,  /* (230) case_exprlist ::= WHEN expr THEN expr */
   292,  /* (231) case_else ::= ELSE expr */
   292,  /* (232) case_else ::= */
   290,  /* (233) case_operand ::= */
   278,  /* (234) exprlist ::= */
   273,  /* (235) nexprlist ::= nexprlist COMMA expr */
   273,  /* (236) nexprlist ::= expr */
   289,  /* (237) paren_exprlist ::= */
   289,  /* (238) paren_exprlist ::= LP exprlist RP */
   248,  /* (239) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
   214,  /* (240) uniqueflag ::= UNIQUE */
   214,  /* (241) uniqueflag ::= */
   256,  /* (242) eidlist_opt ::= */
   256,  /* (243) eidlist_opt ::= LP eidlist RP */
   258,  /* (244) eidlist ::= eidlist COMMA nm collate sortorder */
   258,  /* (245) eidlist ::= nm collate sortorder */
   198,  /* (246) collate ::= */
   198,  /* (247) collate ::= COLLATE ID|STRING */
   248,  /* (248) cmd ::= DROP INDEX ifexists fullname */
   248,  /* (249) cmd ::= VACUUM vinto */
   248,  /* (250) cmd ::= VACUUM nm vinto */
   293,  /* (251) vinto ::= INTO expr */
   293,  /* (252) vinto ::= */
   248,  /* (253) cmd ::= PRAGMA nm dbnm */
   248,  /* (254) cmd ::= PRAGMA nm dbnm EQ nmnum */
   248,  /* (255) cmd ::= PRAGMA nm dbnm LP nmnum RP */
   248,  /* (256) cmd ::= PRAGMA nm dbnm EQ minus_num */
   248,  /* (257) cmd ::= PRAGMA nm dbnm LP minus_num RP */
   219,  /* (258) plus_num ::= PLUS INTEGER|FLOAT */
   220,  /* (259) minus_num ::= MINUS INTEGER|FLOAT */
   248,  /* (260) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
   294,  /* (261) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
   245,  /* (262) trigger_time ::= BEFORE|AFTER */
   245,  /* (263) trigger_time ::= INSTEAD OF */
   245,  /* (264) trigger_time ::= */
   296,  /* (265) trigger_event ::= DELETE|INSERT */
   296,  /* (266) trigger_event ::= UPDATE */
   296,  /* (267) trigger_event ::= UPDATE OF idlist */
   298,  /* (268) when_clause ::= */
   298,  /* (269) when_clause ::= WHEN expr */
   295,  /* (270) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
   295,  /* (271) trigger_cmd_list ::= trigger_cmd SEMI */
   246,  /* (272) trnm ::= nm DOT nm */
   300,  /* (273) tridxby ::= INDEXED BY nm */
   300,  /* (274) tridxby ::= NOT INDEXED */
   299,  /* (275) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
   299,  /* (276) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
   299,  /* (277) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
   299,  /* (278) trigger_cmd ::= scanpt select scanpt */
   254,  /* (279) expr ::= RAISE LP IGNORE RP */
   254,  /* (280) expr ::= RAISE LP raisetype COMMA expr RP */
   199,  /* (281) raisetype ::= ROLLBACK */
   199,  /* (282) raisetype ::= ABORT */
   199,  /* (283) raisetype ::= FAIL */
   248,  /* (284) cmd ::= DROP TRIGGER ifexists fullname */
   248,  /* (285) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
   248,  /* (286) cmd ::= DETACH database_kw_opt expr */
   301,  /* (287) key_opt ::= */
   301,  /* (288) key_opt ::= KEY expr */
   248,  /* (289) cmd ::= REINDEX */
   248,  /* (290) cmd ::= REINDEX nm dbnm */
   248,  /* (291) cmd ::= ANALYZE */
   248,  /* (292) cmd ::= ANALYZE nm dbnm */
   248,  /* (293) cmd ::= ALTER TABLE fullname RENAME TO nm */
   248,  /* (294) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
   248,  /* (295) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
   302,  /* (296) add_column_fullname ::= fullname */
   248,  /* (297) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
   248,  /* (298) cmd ::= create_vtab */
   248,  /* (299) cmd ::= create_vtab LP vtabarglist RP */
   303,  /* (300) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
   305,  /* (301) vtabarg ::= */
   306,  /* (302) vtabargtoken ::= ANY */
   306,  /* (303) vtabargtoken ::= lp anylist RP */
   307,  /* (304) lp ::= LP */
   241,  /* (305) with ::= WITH wqlist */
   241,  /* (306) with ::= WITH RECURSIVE wqlist */
   197,  /* (307) wqas ::= AS */
   197,  /* (308) wqas ::= AS MATERIALIZED */
   197,  /* (309) wqas ::= AS NOT MATERIALIZED */
   309,  /* (310) wqitem ::= withnm eidlist_opt wqas LP select RP */
   196,  /* (311) withnm ::= nm */
   260,  /* (312) wqlist ::= wqitem */
   260,  /* (313) wqlist ::= wqlist COMMA wqitem */
   310,  /* (314) windowdefn_list ::= windowdefn_list COMMA windowdefn */
   311,  /* (315) windowdefn ::= nm AS LP window RP */
   312,  /* (316) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
   312,  /* (317) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
   312,  /* (318) window ::= ORDER BY sortlist frame_opt */
   312,  /* (319) window ::= nm ORDER BY sortlist frame_opt */
   312,  /* (320) window ::= nm frame_opt */
   313,  /* (321) frame_opt ::= */
   313,  /* (322) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
   313,  /* (323) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
   242,  /* (324) range_or_rows ::= RANGE|ROWS|GROUPS */
   314,  /* (325) frame_bound_s ::= frame_bound */
   314,  /* (326) frame_bound_s ::= UNBOUNDED PRECEDING */
   315,  /* (327) frame_bound_e ::= frame_bound */
   315,  /* (328) frame_bound_e ::= UNBOUNDED FOLLOWING */
   316,  /* (329) frame_bound ::= expr PRECEDING|FOLLOWING */
   316,  /* (330) frame_bound ::= CURRENT ROW */
   243,  /* (331) frame_exclude_opt ::= */
   243,  /* (332) frame_exclude_opt ::= EXCLUDE frame_exclude */
   244,  /* (333) frame_exclude ::= NO OTHERS */
   244,  /* (334) frame_exclude ::= CURRENT ROW */
   244,  /* (335) frame_exclude ::= GROUP|TIES */
   271,  /* (336) window_clause ::= WINDOW windowdefn_list */
   287,  /* (337) filter_over ::= filter_clause over_clause */
   287,  /* (338) filter_over ::= over_clause */
   287,  /* (339) filter_over ::= filter_clause */
   318,  /* (340) over_clause ::= OVER LP window RP */
   318,  /* (341) over_clause ::= OVER nm */
   317,  /* (342) filter_clause ::= FILTER LP WHERE expr RP */
   253,  /* (343) term ::= QNUMBER */
   319,  /* (344) input ::= cmdlist */
   320,  /* (345) cmdlist ::= cmdlist ecmd */
   320,  /* (346) cmdlist ::= ecmd */
   321,  /* (347) ecmd ::= SEMI */
   321,  /* (348) ecmd ::= cmdx SEMI */
   321,  /* (349) ecmd ::= explain cmdx SEMI */
   208,  /* (350) trans_opt ::= */
   208,  /* (351) trans_opt ::= TRANSACTION */
   208,  /* (352) trans_opt ::= TRANSACTION nm */
   209,  /* (353) savepoint_opt ::= SAVEPOINT */
   209,  /* (354) savepoint_opt ::= */
   248,  /* (355) cmd ::= create_table create_table_args */
   229,  /* (356) table_option_set ::= table_option */
   251,  /* (357) columnlist ::= columnlist COMMA columnname carglist */
   251,  /* (358) columnlist ::= columnname carglist */
   187,  /* (359) nm ::= ID|INDEXED|JOIN_KW */
   187,  /* (360) nm ::= STRING */
   194,  /* (361) typetoken ::= typename */
   195,  /* (362) typename ::= ID|STRING */
   218,  /* (363) signed ::= plus_num */
   218,  /* (364) signed ::= minus_num */
   232,  /* (365) carglist ::= carglist ccons */
   232,  /* (366) carglist ::= */
   231,  /* (367) ccons ::= NULL onconf */
   231,  /* (368) ccons ::= GENERATED ALWAYS AS generated */
   231,  /* (369) ccons ::= AS generated */
   235,  /* (370) conslist_opt ::= COMMA conslist */
   234,  /* (371) conslist ::= conslist tconscomma tcons */
   234,  /* (372) conslist ::= tcons */
   236,  /* (373) tconscomma ::= */
   228,  /* (374) defer_subclause_opt ::= defer_subclause */
   204,  /* (375) resolvetype ::= raisetype */
   261,  /* (376) selectnowith ::= oneselect */
   262,  /* (377) oneselect ::= values */
   275,  /* (378) sclp ::= selcollist COMMA */
   188,  /* (379) as ::= ID|STRING */
   205,  /* (380) indexed_opt ::= indexed_by */
   286,  /* (381) returning ::= */
   254,  /* (382) expr ::= term */
   190,  /* (383) likeop ::= LIKE_KW|MATCH */
   290,  /* (384) case_operand ::= expr */
   278,  /* (385) exprlist ::= nexprlist */
   221,  /* (386) nmnum ::= plus_num */
   221,  /* (387) nmnum ::= nm */
   221,  /* (388) nmnum ::= ON */
   221,  /* (389) nmnum ::= DELETE */
   221,  /* (390) nmnum ::= DEFAULT */
   219,  /* (391) plus_num ::= INTEGER|FLOAT */
   297,  /* (392) foreach_clause ::= */
   297,  /* (393) foreach_clause ::= FOR EACH ROW */
   246,  /* (394) trnm ::= nm */
   300,  /* (395) tridxby ::= */
   215,  /* (396) database_kw_opt ::= DATABASE */
   215,  /* (397) database_kw_opt ::= */
   210,  /* (398) kwcolumn_opt ::= */
   210,  /* (399) kwcolumn_opt ::= COLUMNKW */
   304,  /* (400) vtabarglist ::= vtabarg */
   304,  /* (401) vtabarglist ::= vtabarglist COMMA vtabarg */
   305,  /* (402) vtabarg ::= vtabarg vtabargtoken */
   308,  /* (403) anylist ::= */
   308,  /* (404) anylist ::= anylist LP anylist RP */
   308,  /* (405) anylist ::= anylist ANY */
   241,  /* (406) with ::= */
   310,  /* (407) windowdefn_list ::= windowdefn */
   312,  /* (408) window ::= frame_opt */
};
#endif /* !_SYNQ_EXTERNAL_PARSER */

/* For rule J, yyRuleInfoNRhs[J] contains the negative of the number
** of symbols on the right-hand side of that rule. */
#ifndef _SYNQ_EXTERNAL_PARSER
static const signed char yyRuleInfoNRhs[] = {
   -1,  /* (0) explain ::= EXPLAIN */
   -3,  /* (1) explain ::= EXPLAIN QUERY PLAN */
   -1,  /* (2) cmdx ::= cmd */
   -3,  /* (3) cmd ::= BEGIN transtype trans_opt */
    0,  /* (4) transtype ::= */
   -1,  /* (5) transtype ::= DEFERRED */
   -1,  /* (6) transtype ::= IMMEDIATE */
   -1,  /* (7) transtype ::= EXCLUSIVE */
   -2,  /* (8) cmd ::= COMMIT|END trans_opt */
   -2,  /* (9) cmd ::= ROLLBACK trans_opt */
   -2,  /* (10) cmd ::= SAVEPOINT nm */
   -3,  /* (11) cmd ::= RELEASE savepoint_opt nm */
   -5,  /* (12) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
   -6,  /* (13) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   -1,  /* (14) createkw ::= CREATE */
    0,  /* (15) ifnotexists ::= */
   -3,  /* (16) ifnotexists ::= IF NOT EXISTS */
   -1,  /* (17) temp ::= TEMP */
    0,  /* (18) temp ::= */
   -5,  /* (19) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   -2,  /* (20) create_table_args ::= AS select */
    0,  /* (21) table_option_set ::= */
   -3,  /* (22) table_option_set ::= table_option_set COMMA table_option */
   -2,  /* (23) table_option ::= WITHOUT nm */
   -1,  /* (24) table_option ::= nm */
   -2,  /* (25) columnname ::= nm typetoken */
    0,  /* (26) typetoken ::= */
   -4,  /* (27) typetoken ::= typename LP signed RP */
   -6,  /* (28) typetoken ::= typename LP signed COMMA signed RP */
   -2,  /* (29) typename ::= typename ID|STRING */
    0,  /* (30) scanpt ::= */
    0,  /* (31) scantok ::= */
   -2,  /* (32) ccons ::= CONSTRAINT nm */
   -3,  /* (33) ccons ::= DEFAULT scantok term */
   -4,  /* (34) ccons ::= DEFAULT LP expr RP */
   -4,  /* (35) ccons ::= DEFAULT PLUS scantok term */
   -4,  /* (36) ccons ::= DEFAULT MINUS scantok term */
   -3,  /* (37) ccons ::= DEFAULT scantok ID|INDEXED */
   -3,  /* (38) ccons ::= NOT NULL onconf */
   -5,  /* (39) ccons ::= PRIMARY KEY sortorder onconf autoinc */
   -2,  /* (40) ccons ::= UNIQUE onconf */
   -4,  /* (41) ccons ::= CHECK LP expr RP */
   -4,  /* (42) ccons ::= REFERENCES nm eidlist_opt refargs */
   -1,  /* (43) ccons ::= defer_subclause */
   -2,  /* (44) ccons ::= COLLATE ID|STRING */
   -3,  /* (45) generated ::= LP expr RP */
   -4,  /* (46) generated ::= LP expr RP ID */
    0,  /* (47) autoinc ::= */
   -1,  /* (48) autoinc ::= AUTOINCR */
    0,  /* (49) refargs ::= */
   -2,  /* (50) refargs ::= refargs refarg */
   -2,  /* (51) refarg ::= MATCH nm */
   -3,  /* (52) refarg ::= ON INSERT refact */
   -3,  /* (53) refarg ::= ON DELETE refact */
   -3,  /* (54) refarg ::= ON UPDATE refact */
   -2,  /* (55) refact ::= SET NULL */
   -2,  /* (56) refact ::= SET DEFAULT */
   -1,  /* (57) refact ::= CASCADE */
   -1,  /* (58) refact ::= RESTRICT */
   -2,  /* (59) refact ::= NO ACTION */
   -3,  /* (60) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
   -2,  /* (61) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
    0,  /* (62) init_deferred_pred_opt ::= */
   -2,  /* (63) init_deferred_pred_opt ::= INITIALLY DEFERRED */
   -2,  /* (64) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
    0,  /* (65) conslist_opt ::= */
   -1,  /* (66) tconscomma ::= COMMA */
   -2,  /* (67) tcons ::= CONSTRAINT nm */
   -7,  /* (68) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
   -5,  /* (69) tcons ::= UNIQUE LP sortlist RP onconf */
   -5,  /* (70) tcons ::= CHECK LP expr RP onconf */
  -10,  /* (71) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
    0,  /* (72) defer_subclause_opt ::= */
    0,  /* (73) onconf ::= */
   -3,  /* (74) onconf ::= ON CONFLICT resolvetype */
    0,  /* (75) orconf ::= */
   -2,  /* (76) orconf ::= OR resolvetype */
   -1,  /* (77) resolvetype ::= IGNORE */
   -1,  /* (78) resolvetype ::= REPLACE */
   -4,  /* (79) cmd ::= DROP TABLE ifexists fullname */
   -2,  /* (80) ifexists ::= IF EXISTS */
    0,  /* (81) ifexists ::= */
   -9,  /* (82) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   -4,  /* (83) cmd ::= DROP VIEW ifexists fullname */
   -1,  /* (84) cmd ::= select */
   -3,  /* (85) select ::= WITH wqlist selectnowith */
   -4,  /* (86) select ::= WITH RECURSIVE wqlist selectnowith */
   -1,  /* (87) select ::= selectnowith */
   -3,  /* (88) selectnowith ::= selectnowith multiselect_op oneselect */
   -1,  /* (89) multiselect_op ::= UNION */
   -2,  /* (90) multiselect_op ::= UNION ALL */
   -1,  /* (91) multiselect_op ::= EXCEPT|INTERSECT */
   -9,  /* (92) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
  -10,  /* (93) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   -4,  /* (94) values ::= VALUES LP nexprlist RP */
   -1,  /* (95) oneselect ::= mvalues */
   -5,  /* (96) mvalues ::= values COMMA LP nexprlist RP */
   -5,  /* (97) mvalues ::= mvalues COMMA LP nexprlist RP */
   -1,  /* (98) distinct ::= DISTINCT */
   -1,  /* (99) distinct ::= ALL */
    0,  /* (100) distinct ::= */
    0,  /* (101) sclp ::= */
   -5,  /* (102) selcollist ::= sclp scanpt expr scanpt as */
   -3,  /* (103) selcollist ::= sclp scanpt STAR */
   -5,  /* (104) selcollist ::= sclp scanpt nm DOT STAR */
   -2,  /* (105) as ::= AS nm */
    0,  /* (106) as ::= */
    0,  /* (107) from ::= */
   -2,  /* (108) from ::= FROM seltablist */
   -2,  /* (109) stl_prefix ::= seltablist joinop */
    0,  /* (110) stl_prefix ::= */
   -5,  /* (111) seltablist ::= stl_prefix nm dbnm as on_using */
   -6,  /* (112) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   -8,  /* (113) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   -6,  /* (114) seltablist ::= stl_prefix LP select RP as on_using */
   -6,  /* (115) seltablist ::= stl_prefix LP seltablist RP as on_using */
    0,  /* (116) dbnm ::= */
   -2,  /* (117) dbnm ::= DOT nm */
   -1,  /* (118) fullname ::= nm */
   -3,  /* (119) fullname ::= nm DOT nm */
   -1,  /* (120) xfullname ::= nm */
   -3,  /* (121) xfullname ::= nm DOT nm */
   -5,  /* (122) xfullname ::= nm DOT nm AS nm */
   -3,  /* (123) xfullname ::= nm AS nm */
   -1,  /* (124) joinop ::= COMMA|JOIN */
   -2,  /* (125) joinop ::= JOIN_KW JOIN */
   -3,  /* (126) joinop ::= JOIN_KW nm JOIN */
   -4,  /* (127) joinop ::= JOIN_KW nm nm JOIN */
   -2,  /* (128) on_using ::= ON expr */
   -4,  /* (129) on_using ::= USING LP idlist RP */
    0,  /* (130) on_using ::= */
    0,  /* (131) indexed_opt ::= */
   -3,  /* (132) indexed_by ::= INDEXED BY nm */
   -2,  /* (133) indexed_by ::= NOT INDEXED */
    0,  /* (134) orderby_opt ::= */
   -3,  /* (135) orderby_opt ::= ORDER BY sortlist */
   -5,  /* (136) sortlist ::= sortlist COMMA expr sortorder nulls */
   -3,  /* (137) sortlist ::= expr sortorder nulls */
   -1,  /* (138) sortorder ::= ASC */
   -1,  /* (139) sortorder ::= DESC */
    0,  /* (140) sortorder ::= */
   -2,  /* (141) nulls ::= NULLS FIRST */
   -2,  /* (142) nulls ::= NULLS LAST */
    0,  /* (143) nulls ::= */
    0,  /* (144) groupby_opt ::= */
   -3,  /* (145) groupby_opt ::= GROUP BY nexprlist */
    0,  /* (146) having_opt ::= */
   -2,  /* (147) having_opt ::= HAVING expr */
    0,  /* (148) limit_opt ::= */
   -2,  /* (149) limit_opt ::= LIMIT expr */
   -4,  /* (150) limit_opt ::= LIMIT expr OFFSET expr */
   -4,  /* (151) limit_opt ::= LIMIT expr COMMA expr */
   -6,  /* (152) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
    0,  /* (153) where_opt ::= */
   -2,  /* (154) where_opt ::= WHERE expr */
    0,  /* (155) where_opt_ret ::= */
   -2,  /* (156) where_opt_ret ::= WHERE expr */
   -2,  /* (157) where_opt_ret ::= RETURNING selcollist */
   -4,  /* (158) where_opt_ret ::= WHERE expr RETURNING selcollist */
   -9,  /* (159) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   -5,  /* (160) setlist ::= setlist COMMA nm EQ expr */
   -7,  /* (161) setlist ::= setlist COMMA LP idlist RP EQ expr */
   -3,  /* (162) setlist ::= nm EQ expr */
   -5,  /* (163) setlist ::= LP idlist RP EQ expr */
   -7,  /* (164) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   -8,  /* (165) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
    0,  /* (166) upsert ::= */
   -2,  /* (167) upsert ::= RETURNING selcollist */
  -12,  /* (168) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
   -9,  /* (169) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
   -5,  /* (170) upsert ::= ON CONFLICT DO NOTHING returning */
   -8,  /* (171) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
   -2,  /* (172) returning ::= RETURNING selcollist */
   -2,  /* (173) insert_cmd ::= INSERT orconf */
   -1,  /* (174) insert_cmd ::= REPLACE */
    0,  /* (175) idlist_opt ::= */
   -3,  /* (176) idlist_opt ::= LP idlist RP */
   -3,  /* (177) idlist ::= idlist COMMA nm */
   -1,  /* (178) idlist ::= nm */
   -3,  /* (179) expr ::= LP expr RP */
   -1,  /* (180) expr ::= ID|INDEXED|JOIN_KW */
   -3,  /* (181) expr ::= nm DOT nm */
   -5,  /* (182) expr ::= nm DOT nm DOT nm */
   -1,  /* (183) term ::= NULL|FLOAT|BLOB */
   -1,  /* (184) term ::= STRING */
   -1,  /* (185) term ::= INTEGER */
   -1,  /* (186) expr ::= VARIABLE */
   -3,  /* (187) expr ::= expr COLLATE ID|STRING */
   -6,  /* (188) expr ::= CAST LP expr AS typetoken RP */
   -5,  /* (189) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   -8,  /* (190) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   -4,  /* (191) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   -6,  /* (192) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   -9,  /* (193) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   -5,  /* (194) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   -1,  /* (195) term ::= CTIME_KW */
   -5,  /* (196) expr ::= LP nexprlist COMMA expr RP */
   -3,  /* (197) expr ::= expr AND expr */
   -3,  /* (198) expr ::= expr OR expr */
   -3,  /* (199) expr ::= expr LT|GT|GE|LE expr */
   -3,  /* (200) expr ::= expr EQ|NE expr */
   -3,  /* (201) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
   -3,  /* (202) expr ::= expr PLUS|MINUS expr */
   -3,  /* (203) expr ::= expr STAR|SLASH|REM expr */
   -3,  /* (204) expr ::= expr CONCAT expr */
   -2,  /* (205) likeop ::= NOT LIKE_KW|MATCH */
   -3,  /* (206) expr ::= expr likeop expr */
   -5,  /* (207) expr ::= expr likeop expr ESCAPE expr */
   -2,  /* (208) expr ::= expr ISNULL|NOTNULL */
   -3,  /* (209) expr ::= expr NOT NULL */
   -3,  /* (210) expr ::= expr IS expr */
   -4,  /* (211) expr ::= expr IS NOT expr */
   -6,  /* (212) expr ::= expr IS NOT DISTINCT FROM expr */
   -5,  /* (213) expr ::= expr IS DISTINCT FROM expr */
   -2,  /* (214) expr ::= NOT expr */
   -2,  /* (215) expr ::= BITNOT expr */
   -2,  /* (216) expr ::= PLUS|MINUS expr */
   -3,  /* (217) expr ::= expr PTR expr */
   -1,  /* (218) between_op ::= BETWEEN */
   -2,  /* (219) between_op ::= NOT BETWEEN */
   -5,  /* (220) expr ::= expr between_op expr AND expr */
   -1,  /* (221) in_op ::= IN */
   -2,  /* (222) in_op ::= NOT IN */
   -5,  /* (223) expr ::= expr in_op LP exprlist RP */
   -3,  /* (224) expr ::= LP select RP */
   -5,  /* (225) expr ::= expr in_op LP select RP */
   -5,  /* (226) expr ::= expr in_op nm dbnm paren_exprlist */
   -4,  /* (227) expr ::= EXISTS LP select RP */
   -5,  /* (228) expr ::= CASE case_operand case_exprlist case_else END */
   -5,  /* (229) case_exprlist ::= case_exprlist WHEN expr THEN expr */
   -4,  /* (230) case_exprlist ::= WHEN expr THEN expr */
   -2,  /* (231) case_else ::= ELSE expr */
    0,  /* (232) case_else ::= */
    0,  /* (233) case_operand ::= */
    0,  /* (234) exprlist ::= */
   -3,  /* (235) nexprlist ::= nexprlist COMMA expr */
   -1,  /* (236) nexprlist ::= expr */
    0,  /* (237) paren_exprlist ::= */
   -3,  /* (238) paren_exprlist ::= LP exprlist RP */
  -12,  /* (239) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
   -1,  /* (240) uniqueflag ::= UNIQUE */
    0,  /* (241) uniqueflag ::= */
    0,  /* (242) eidlist_opt ::= */
   -3,  /* (243) eidlist_opt ::= LP eidlist RP */
   -5,  /* (244) eidlist ::= eidlist COMMA nm collate sortorder */
   -3,  /* (245) eidlist ::= nm collate sortorder */
    0,  /* (246) collate ::= */
   -2,  /* (247) collate ::= COLLATE ID|STRING */
   -4,  /* (248) cmd ::= DROP INDEX ifexists fullname */
   -2,  /* (249) cmd ::= VACUUM vinto */
   -3,  /* (250) cmd ::= VACUUM nm vinto */
   -2,  /* (251) vinto ::= INTO expr */
    0,  /* (252) vinto ::= */
   -3,  /* (253) cmd ::= PRAGMA nm dbnm */
   -5,  /* (254) cmd ::= PRAGMA nm dbnm EQ nmnum */
   -6,  /* (255) cmd ::= PRAGMA nm dbnm LP nmnum RP */
   -5,  /* (256) cmd ::= PRAGMA nm dbnm EQ minus_num */
   -6,  /* (257) cmd ::= PRAGMA nm dbnm LP minus_num RP */
   -2,  /* (258) plus_num ::= PLUS INTEGER|FLOAT */
   -2,  /* (259) minus_num ::= MINUS INTEGER|FLOAT */
   -5,  /* (260) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
  -11,  /* (261) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
   -1,  /* (262) trigger_time ::= BEFORE|AFTER */
   -2,  /* (263) trigger_time ::= INSTEAD OF */
    0,  /* (264) trigger_time ::= */
   -1,  /* (265) trigger_event ::= DELETE|INSERT */
   -1,  /* (266) trigger_event ::= UPDATE */
   -3,  /* (267) trigger_event ::= UPDATE OF idlist */
    0,  /* (268) when_clause ::= */
   -2,  /* (269) when_clause ::= WHEN expr */
   -3,  /* (270) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
   -2,  /* (271) trigger_cmd_list ::= trigger_cmd SEMI */
   -3,  /* (272) trnm ::= nm DOT nm */
   -3,  /* (273) tridxby ::= INDEXED BY nm */
   -2,  /* (274) tridxby ::= NOT INDEXED */
   -9,  /* (275) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
   -8,  /* (276) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
   -6,  /* (277) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
   -3,  /* (278) trigger_cmd ::= scanpt select scanpt */
   -4,  /* (279) expr ::= RAISE LP IGNORE RP */
   -6,  /* (280) expr ::= RAISE LP raisetype COMMA expr RP */
   -1,  /* (281) raisetype ::= ROLLBACK */
   -1,  /* (282) raisetype ::= ABORT */
   -1,  /* (283) raisetype ::= FAIL */
   -4,  /* (284) cmd ::= DROP TRIGGER ifexists fullname */
   -6,  /* (285) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
   -3,  /* (286) cmd ::= DETACH database_kw_opt expr */
    0,  /* (287) key_opt ::= */
   -2,  /* (288) key_opt ::= KEY expr */
   -1,  /* (289) cmd ::= REINDEX */
   -3,  /* (290) cmd ::= REINDEX nm dbnm */
   -1,  /* (291) cmd ::= ANALYZE */
   -3,  /* (292) cmd ::= ANALYZE nm dbnm */
   -6,  /* (293) cmd ::= ALTER TABLE fullname RENAME TO nm */
   -7,  /* (294) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
   -6,  /* (295) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
   -1,  /* (296) add_column_fullname ::= fullname */
   -8,  /* (297) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
   -1,  /* (298) cmd ::= create_vtab */
   -4,  /* (299) cmd ::= create_vtab LP vtabarglist RP */
   -8,  /* (300) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
    0,  /* (301) vtabarg ::= */
   -1,  /* (302) vtabargtoken ::= ANY */
   -3,  /* (303) vtabargtoken ::= lp anylist RP */
   -1,  /* (304) lp ::= LP */
   -2,  /* (305) with ::= WITH wqlist */
   -3,  /* (306) with ::= WITH RECURSIVE wqlist */
   -1,  /* (307) wqas ::= AS */
   -2,  /* (308) wqas ::= AS MATERIALIZED */
   -3,  /* (309) wqas ::= AS NOT MATERIALIZED */
   -6,  /* (310) wqitem ::= withnm eidlist_opt wqas LP select RP */
   -1,  /* (311) withnm ::= nm */
   -1,  /* (312) wqlist ::= wqitem */
   -3,  /* (313) wqlist ::= wqlist COMMA wqitem */
   -3,  /* (314) windowdefn_list ::= windowdefn_list COMMA windowdefn */
   -5,  /* (315) windowdefn ::= nm AS LP window RP */
   -5,  /* (316) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
   -6,  /* (317) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
   -4,  /* (318) window ::= ORDER BY sortlist frame_opt */
   -5,  /* (319) window ::= nm ORDER BY sortlist frame_opt */
   -2,  /* (320) window ::= nm frame_opt */
    0,  /* (321) frame_opt ::= */
   -3,  /* (322) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
   -6,  /* (323) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
   -1,  /* (324) range_or_rows ::= RANGE|ROWS|GROUPS */
   -1,  /* (325) frame_bound_s ::= frame_bound */
   -2,  /* (326) frame_bound_s ::= UNBOUNDED PRECEDING */
   -1,  /* (327) frame_bound_e ::= frame_bound */
   -2,  /* (328) frame_bound_e ::= UNBOUNDED FOLLOWING */
   -2,  /* (329) frame_bound ::= expr PRECEDING|FOLLOWING */
   -2,  /* (330) frame_bound ::= CURRENT ROW */
    0,  /* (331) frame_exclude_opt ::= */
   -2,  /* (332) frame_exclude_opt ::= EXCLUDE frame_exclude */
   -2,  /* (333) frame_exclude ::= NO OTHERS */
   -2,  /* (334) frame_exclude ::= CURRENT ROW */
   -1,  /* (335) frame_exclude ::= GROUP|TIES */
   -2,  /* (336) window_clause ::= WINDOW windowdefn_list */
   -2,  /* (337) filter_over ::= filter_clause over_clause */
   -1,  /* (338) filter_over ::= over_clause */
   -1,  /* (339) filter_over ::= filter_clause */
   -4,  /* (340) over_clause ::= OVER LP window RP */
   -2,  /* (341) over_clause ::= OVER nm */
   -5,  /* (342) filter_clause ::= FILTER LP WHERE expr RP */
   -1,  /* (343) term ::= QNUMBER */
   -1,  /* (344) input ::= cmdlist */
   -2,  /* (345) cmdlist ::= cmdlist ecmd */
   -1,  /* (346) cmdlist ::= ecmd */
   -1,  /* (347) ecmd ::= SEMI */
   -2,  /* (348) ecmd ::= cmdx SEMI */
   -3,  /* (349) ecmd ::= explain cmdx SEMI */
    0,  /* (350) trans_opt ::= */
   -1,  /* (351) trans_opt ::= TRANSACTION */
   -2,  /* (352) trans_opt ::= TRANSACTION nm */
   -1,  /* (353) savepoint_opt ::= SAVEPOINT */
    0,  /* (354) savepoint_opt ::= */
   -2,  /* (355) cmd ::= create_table create_table_args */
   -1,  /* (356) table_option_set ::= table_option */
   -4,  /* (357) columnlist ::= columnlist COMMA columnname carglist */
   -2,  /* (358) columnlist ::= columnname carglist */
   -1,  /* (359) nm ::= ID|INDEXED|JOIN_KW */
   -1,  /* (360) nm ::= STRING */
   -1,  /* (361) typetoken ::= typename */
   -1,  /* (362) typename ::= ID|STRING */
   -1,  /* (363) signed ::= plus_num */
   -1,  /* (364) signed ::= minus_num */
   -2,  /* (365) carglist ::= carglist ccons */
    0,  /* (366) carglist ::= */
   -2,  /* (367) ccons ::= NULL onconf */
   -4,  /* (368) ccons ::= GENERATED ALWAYS AS generated */
   -2,  /* (369) ccons ::= AS generated */
   -2,  /* (370) conslist_opt ::= COMMA conslist */
   -3,  /* (371) conslist ::= conslist tconscomma tcons */
   -1,  /* (372) conslist ::= tcons */
    0,  /* (373) tconscomma ::= */
   -1,  /* (374) defer_subclause_opt ::= defer_subclause */
   -1,  /* (375) resolvetype ::= raisetype */
   -1,  /* (376) selectnowith ::= oneselect */
   -1,  /* (377) oneselect ::= values */
   -2,  /* (378) sclp ::= selcollist COMMA */
   -1,  /* (379) as ::= ID|STRING */
   -1,  /* (380) indexed_opt ::= indexed_by */
    0,  /* (381) returning ::= */
   -1,  /* (382) expr ::= term */
   -1,  /* (383) likeop ::= LIKE_KW|MATCH */
   -1,  /* (384) case_operand ::= expr */
   -1,  /* (385) exprlist ::= nexprlist */
   -1,  /* (386) nmnum ::= plus_num */
   -1,  /* (387) nmnum ::= nm */
   -1,  /* (388) nmnum ::= ON */
   -1,  /* (389) nmnum ::= DELETE */
   -1,  /* (390) nmnum ::= DEFAULT */
   -1,  /* (391) plus_num ::= INTEGER|FLOAT */
    0,  /* (392) foreach_clause ::= */
   -3,  /* (393) foreach_clause ::= FOR EACH ROW */
   -1,  /* (394) trnm ::= nm */
    0,  /* (395) tridxby ::= */
   -1,  /* (396) database_kw_opt ::= DATABASE */
    0,  /* (397) database_kw_opt ::= */
    0,  /* (398) kwcolumn_opt ::= */
   -1,  /* (399) kwcolumn_opt ::= COLUMNKW */
   -1,  /* (400) vtabarglist ::= vtabarg */
   -3,  /* (401) vtabarglist ::= vtabarglist COMMA vtabarg */
   -2,  /* (402) vtabarg ::= vtabarg vtabargtoken */
    0,  /* (403) anylist ::= */
   -4,  /* (404) anylist ::= anylist LP anylist RP */
   -2,  /* (405) anylist ::= anylist ANY */
    0,  /* (406) with ::= */
   -1,  /* (407) windowdefn_list ::= windowdefn */
   -1,  /* (408) window ::= frame_opt */
};
#endif /* !_SYNQ_EXTERNAL_PARSER */

static void yy_accept(synq_yyParser*);  /* Forward Declaration */

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
  synq_yyParser *yypParser,         /* The parser */
  unsigned int yyruleno,       /* Number of the rule by which to reduce */
  int yyLookahead,             /* Lookahead token, or YYNOCODE if none */
  synq_sqlite3ParserTOKENTYPE yyLookaheadToken  /* Value of the lookahead token */
  synq_sqlite3ParserCTX_PDECL                   /* %extra_context */
){
  int yygoto;                     /* The next state */
  YYACTIONTYPE yyact;             /* The next action */
  synq_yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  synq_sqlite3ParserARG_FETCH
  (void)yyLookahead;
  (void)yyLookaheadToken;
  yymsp = yypParser->yytos;

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
/********** Begin reduce actions **********************************************/
        YYMINORTYPE yylhsminor;
      case 0: /* explain ::= EXPLAIN */
      case 17: /* temp ::= TEMP */ yytestcase(yyruleno==17);
      case 48: /* autoinc ::= AUTOINCR */ yytestcase(yyruleno==48);
      case 240: /* uniqueflag ::= UNIQUE */ yytestcase(yyruleno==240);
      case 399: /* kwcolumn_opt ::= COLUMNKW */ yytestcase(yyruleno==399);
{
    yymsp[0].minor.yy144 = 1;
}
        break;
      case 1: /* explain ::= EXPLAIN QUERY PLAN */
{
    yymsp[-2].minor.yy144 = 2;
}
        break;
      case 2: /* cmdx ::= cmd */
      case 84: /* cmd ::= select */ yytestcase(yyruleno==84);
      case 87: /* select ::= selectnowith */ yytestcase(yyruleno==87);
      case 298: /* cmd ::= create_vtab */ yytestcase(yyruleno==298);
      case 325: /* frame_bound_s ::= frame_bound */ yytestcase(yyruleno==325);
      case 327: /* frame_bound_e ::= frame_bound */ yytestcase(yyruleno==327);
      case 338: /* filter_over ::= over_clause */ yytestcase(yyruleno==338);
      case 346: /* cmdlist ::= ecmd */ yytestcase(yyruleno==346);
      case 376: /* selectnowith ::= oneselect */ yytestcase(yyruleno==376);
      case 382: /* expr ::= term */ yytestcase(yyruleno==382);
      case 384: /* case_operand ::= expr */ yytestcase(yyruleno==384);
      case 385: /* exprlist ::= nexprlist */ yytestcase(yyruleno==385);
{
    yylhsminor.yy391 = yymsp[0].minor.yy391;
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 3: /* cmd ::= BEGIN transtype trans_opt */
{
    yymsp[-2].minor.yy391 = synq_ast_transaction_stmt(pCtx->astCtx,
        SYNTAQLITE_TRANSACTION_OP_BEGIN,
        (SyntaqliteTransactionType)yymsp[-1].minor.yy144);
}
        break;
      case 4: /* transtype ::= */
{
    yymsp[1].minor.yy144 = (int)SYNTAQLITE_TRANSACTION_TYPE_DEFERRED;
}
        break;
      case 5: /* transtype ::= DEFERRED */
{
    yymsp[0].minor.yy144 = (int)SYNTAQLITE_TRANSACTION_TYPE_DEFERRED;
}
        break;
      case 6: /* transtype ::= IMMEDIATE */
{
    yymsp[0].minor.yy144 = (int)SYNTAQLITE_TRANSACTION_TYPE_IMMEDIATE;
}
        break;
      case 7: /* transtype ::= EXCLUSIVE */
{
    yymsp[0].minor.yy144 = (int)SYNTAQLITE_TRANSACTION_TYPE_EXCLUSIVE;
}
        break;
      case 8: /* cmd ::= COMMIT|END trans_opt */
{
    yymsp[-1].minor.yy391 = synq_ast_transaction_stmt(pCtx->astCtx,
        SYNTAQLITE_TRANSACTION_OP_COMMIT,
        SYNTAQLITE_TRANSACTION_TYPE_DEFERRED);
}
        break;
      case 9: /* cmd ::= ROLLBACK trans_opt */
{
    yymsp[-1].minor.yy391 = synq_ast_transaction_stmt(pCtx->astCtx,
        SYNTAQLITE_TRANSACTION_OP_ROLLBACK,
        SYNTAQLITE_TRANSACTION_TYPE_DEFERRED);
}
        break;
      case 10: /* cmd ::= SAVEPOINT nm */
{
    yymsp[-1].minor.yy391 = synq_ast_savepoint_stmt(pCtx->astCtx,
        SYNTAQLITE_SAVEPOINT_OP_SAVEPOINT,
        synq_span(pCtx, yymsp[0].minor.yy0));
}
        break;
      case 11: /* cmd ::= RELEASE savepoint_opt nm */
{
    yymsp[-2].minor.yy391 = synq_ast_savepoint_stmt(pCtx->astCtx,
        SYNTAQLITE_SAVEPOINT_OP_RELEASE,
        synq_span(pCtx, yymsp[0].minor.yy0));
}
        break;
      case 12: /* cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
{
    yymsp[-4].minor.yy391 = synq_ast_savepoint_stmt(pCtx->astCtx,
        SYNTAQLITE_SAVEPOINT_OP_ROLLBACK_TO,
        synq_span(pCtx, yymsp[0].minor.yy0));
}
        break;
      case 13: /* create_table ::= createkw temp TABLE ifnotexists nm dbnm */
{
    SyntaqliteSourceSpan tbl_name = yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[0].minor.yy0) : synq_span(pCtx, yymsp[-1].minor.yy0);
    SyntaqliteSourceSpan tbl_schema = yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[-1].minor.yy0) : SYNQ_NO_SPAN;
    yymsp[-5].minor.yy391 = synq_ast_create_table_stmt(pCtx->astCtx,
        tbl_name, tbl_schema, (SyntaqliteBool)yymsp[-4].minor.yy144, (SyntaqliteBool)yymsp[-2].minor.yy144,
        (SyntaqliteCreateTableStmtFlags){.raw = 0}, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
}
        break;
      case 14: /* createkw ::= CREATE */
      case 363: /* signed ::= plus_num */ yytestcase(yyruleno==363);
      case 364: /* signed ::= minus_num */ yytestcase(yyruleno==364);
      case 380: /* indexed_opt ::= indexed_by */ yytestcase(yyruleno==380);
      case 386: /* nmnum ::= plus_num */ yytestcase(yyruleno==386);
      case 387: /* nmnum ::= nm */ yytestcase(yyruleno==387);
      case 388: /* nmnum ::= ON */ yytestcase(yyruleno==388);
      case 389: /* nmnum ::= DELETE */ yytestcase(yyruleno==389);
      case 390: /* nmnum ::= DEFAULT */ yytestcase(yyruleno==390);
      case 391: /* plus_num ::= INTEGER|FLOAT */ yytestcase(yyruleno==391);
      case 394: /* trnm ::= nm */ yytestcase(yyruleno==394);
{
    // Token passthrough
}
        break;
      case 15: /* ifnotexists ::= */
      case 18: /* temp ::= */ yytestcase(yyruleno==18);
      case 21: /* table_option_set ::= */ yytestcase(yyruleno==21);
      case 47: /* autoinc ::= */ yytestcase(yyruleno==47);
      case 62: /* init_deferred_pred_opt ::= */ yytestcase(yyruleno==62);
      case 72: /* defer_subclause_opt ::= */ yytestcase(yyruleno==72);
      case 81: /* ifexists ::= */ yytestcase(yyruleno==81);
      case 241: /* uniqueflag ::= */ yytestcase(yyruleno==241);
      case 246: /* collate ::= */ yytestcase(yyruleno==246);
      case 350: /* trans_opt ::= */ yytestcase(yyruleno==350);
      case 354: /* savepoint_opt ::= */ yytestcase(yyruleno==354);
      case 398: /* kwcolumn_opt ::= */ yytestcase(yyruleno==398);
{
    yymsp[1].minor.yy144 = 0;
}
        break;
      case 16: /* ifnotexists ::= IF NOT EXISTS */
{
    yymsp[-2].minor.yy144 = 1;
}
        break;
      case 19: /* create_table_args ::= LP columnlist conslist_opt RP table_option_set */
{
    yymsp[-4].minor.yy391 = synq_ast_create_table_stmt(pCtx->astCtx,
        SYNQ_NO_SPAN, SYNQ_NO_SPAN, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE,
        (SyntaqliteCreateTableStmtFlags){.raw = (uint8_t)yymsp[0].minor.yy144}, yymsp[-3].minor.yy391, yymsp[-2].minor.yy391, SYNTAQLITE_NULL_NODE);
}
        break;
      case 20: /* create_table_args ::= AS select */
{
    yymsp[-1].minor.yy391 = synq_ast_create_table_stmt(pCtx->astCtx,
        SYNQ_NO_SPAN, SYNQ_NO_SPAN, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE,
        (SyntaqliteCreateTableStmtFlags){.raw = 0}, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy391);
}
        break;
      case 22: /* table_option_set ::= table_option_set COMMA table_option */
{
    yylhsminor.yy144 = yymsp[-2].minor.yy144 | yymsp[0].minor.yy144;
}
  yymsp[-2].minor.yy144 = yylhsminor.yy144;
        break;
      case 23: /* table_option ::= WITHOUT nm */
{
    // WITHOUT ROWID = bit 0
    if (yymsp[0].minor.yy0.n == 5 && strncasecmp(yymsp[0].minor.yy0.z, "rowid", 5) == 0) {
        yymsp[-1].minor.yy144 = 1;
    } else {
        yymsp[-1].minor.yy144 = 0;
    }
}
        break;
      case 24: /* table_option ::= nm */
{
    // STRICT = bit 1
    if (yymsp[0].minor.yy0.n == 6 && strncasecmp(yymsp[0].minor.yy0.z, "strict", 6) == 0) {
        yylhsminor.yy144 = 2;
    } else {
        yylhsminor.yy144 = 0;
    }
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 25: /* columnname ::= nm typetoken */
{
    yylhsminor.yy188.name = synq_span(pCtx, yymsp[-1].minor.yy0);
    yylhsminor.yy188.typetoken = yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[0].minor.yy0) : SYNQ_NO_SPAN;
}
  yymsp[-1].minor.yy188 = yylhsminor.yy188;
        break;
      case 26: /* typetoken ::= */
{
    yymsp[1].minor.yy0.n = 0; yymsp[1].minor.yy0.z = 0;
}
        break;
      case 27: /* typetoken ::= typename LP signed RP */
{
    yymsp[-3].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-3].minor.yy0.z);
}
        break;
      case 28: /* typetoken ::= typename LP signed COMMA signed RP */
{
    yymsp[-5].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-5].minor.yy0.z);
}
        break;
      case 29: /* typename ::= typename ID|STRING */
{
    yymsp[-1].minor.yy0.n = yymsp[0].minor.yy0.n + (int)(yymsp[0].minor.yy0.z - yymsp[-1].minor.yy0.z);
}
        break;
      case 30: /* scanpt ::= */
      case 31: /* scantok ::= */ yytestcase(yyruleno==31);
      case 106: /* as ::= */ yytestcase(yyruleno==106);
      case 131: /* indexed_opt ::= */ yytestcase(yyruleno==131);
{
    yymsp[1].minor.yy0.z = NULL; yymsp[1].minor.yy0.n = 0;
}
        break;
      case 32: /* ccons ::= CONSTRAINT nm */
      case 67: /* tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==67);
{
    yymsp[-1].minor.yy462.node = SYNTAQLITE_NULL_NODE;
    yymsp[-1].minor.yy462.pending_name = synq_span(pCtx, yymsp[0].minor.yy0);
}
        break;
      case 33: /* ccons ::= DEFAULT scantok term */
{
    yymsp[-2].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        yymsp[0].minor.yy391, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-2].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 34: /* ccons ::= DEFAULT LP expr RP */
{
    yymsp[-3].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        yymsp[-1].minor.yy391, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-3].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 35: /* ccons ::= DEFAULT PLUS scantok term */
{
    yymsp[-3].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        yymsp[0].minor.yy391, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-3].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 36: /* ccons ::= DEFAULT MINUS scantok term */
{
    // Create a unary minus wrapping the term
    uint32_t neg = synq_ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_MINUS, yymsp[0].minor.yy391);
    yymsp[-3].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        neg, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-3].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 37: /* ccons ::= DEFAULT scantok ID|INDEXED */
{
    // Treat the identifier as a literal expression
    uint32_t lit = synq_ast_literal(pCtx->astCtx,
        SYNTAQLITE_LITERAL_TYPE_STRING, synq_span(pCtx, yymsp[0].minor.yy0));
    yymsp[-2].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        lit, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-2].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 38: /* ccons ::= NOT NULL onconf */
{
    yymsp[-2].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NOT_NULL,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)yymsp[0].minor.yy144, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-2].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 39: /* ccons ::= PRIMARY KEY sortorder onconf autoinc */
{
    yymsp[-4].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_PRIMARY_KEY,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)yymsp[-1].minor.yy144, (SyntaqliteSortOrder)yymsp[-2].minor.yy391, (SyntaqliteBool)yymsp[0].minor.yy144,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-4].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 40: /* ccons ::= UNIQUE onconf */
{
    yymsp[-1].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_UNIQUE,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)yymsp[0].minor.yy144, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-1].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 41: /* ccons ::= CHECK LP expr RP */
{
    yymsp[-3].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_CHECK,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, yymsp[-1].minor.yy391, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-3].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 42: /* ccons ::= REFERENCES nm eidlist_opt refargs */
{
    // Decode refargs: low byte = on_delete, next byte = on_update
    SyntaqliteForeignKeyAction on_del = (SyntaqliteForeignKeyAction)(yymsp[0].minor.yy144 & 0xff);
    SyntaqliteForeignKeyAction on_upd = (SyntaqliteForeignKeyAction)((yymsp[0].minor.yy144 >> 8) & 0xff);
    uint32_t fk = synq_ast_foreign_key_clause(pCtx->astCtx,
        synq_span(pCtx, yymsp[-2].minor.yy0), yymsp[-1].minor.yy391, on_del, on_upd, SYNTAQLITE_BOOL_FALSE);
    yymsp[-3].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_REFERENCES,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, fk);
    yymsp[-3].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 43: /* ccons ::= defer_subclause */
{
    // Create a minimal constraint that just marks deferral.
    // In practice, this follows a REFERENCES ccons. We'll handle it
    // by updating the last constraint in the list if possible.
    // For simplicity, we create a separate REFERENCES constraint with just deferral info.
    // The printer will show it as a separate constraint entry.
    uint32_t fk = synq_ast_foreign_key_clause(pCtx->astCtx,
        SYNQ_NO_SPAN, SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION,
        SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION,
        (SyntaqliteBool)yymsp[0].minor.yy144);
    yylhsminor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_REFERENCES,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, fk);
    yylhsminor.yy462.pending_name = SYNQ_NO_SPAN;
}
  yymsp[0].minor.yy462 = yylhsminor.yy462;
        break;
      case 44: /* ccons ::= COLLATE ID|STRING */
{
    yymsp[-1].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_COLLATE,
        SYNQ_NO_SPAN,
        0, 0, 0,
        synq_span(pCtx, yymsp[0].minor.yy0),
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-1].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 45: /* generated ::= LP expr RP */
{
    yymsp[-2].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_GENERATED,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, yymsp[-1].minor.yy391, SYNTAQLITE_NULL_NODE);
    yymsp[-2].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 46: /* generated ::= LP expr RP ID */
{
    SyntaqliteGeneratedColumnStorage storage = SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL;
    if (yymsp[0].minor.yy0.n == 6 && strncasecmp(yymsp[0].minor.yy0.z, "stored", 6) == 0) {
        storage = SYNTAQLITE_GENERATED_COLUMN_STORAGE_STORED;
    }
    yymsp[-3].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_GENERATED,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        storage,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, yymsp[-2].minor.yy391, SYNTAQLITE_NULL_NODE);
    yymsp[-3].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 49: /* refargs ::= */
{
    yymsp[1].minor.yy144 = 0; // NO_ACTION for both
}
        break;
      case 50: /* refargs ::= refargs refarg */
{
    // refarg encodes: low byte = value, byte 1 = shift amount (0 or 8)
    int val = yymsp[0].minor.yy144 & 0xff;
    int shift = (yymsp[0].minor.yy144 >> 8) & 0xff;
    // Clear the target byte in yymsp[-1].minor.yy144 and set new value
    yymsp[-1].minor.yy144 = (yymsp[-1].minor.yy144 & ~(0xff << shift)) | (val << shift);
}
        break;
      case 51: /* refarg ::= MATCH nm */
{
    yymsp[-1].minor.yy144 = 0; // MATCH is ignored
}
        break;
      case 52: /* refarg ::= ON INSERT refact */
{
    yymsp[-2].minor.yy144 = 0; // ON INSERT is ignored
}
        break;
      case 53: /* refarg ::= ON DELETE refact */
{
    yymsp[-2].minor.yy144 = yymsp[0].minor.yy144; // shift=0 for DELETE
}
        break;
      case 54: /* refarg ::= ON UPDATE refact */
{
    yymsp[-2].minor.yy144 = yymsp[0].minor.yy144 | (8 << 8); // shift=8 for UPDATE
}
        break;
      case 55: /* refact ::= SET NULL */
{
    yymsp[-1].minor.yy144 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_SET_NULL;
}
        break;
      case 56: /* refact ::= SET DEFAULT */
{
    yymsp[-1].minor.yy144 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_SET_DEFAULT;
}
        break;
      case 57: /* refact ::= CASCADE */
{
    yymsp[0].minor.yy144 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_CASCADE;
}
        break;
      case 58: /* refact ::= RESTRICT */
{
    yymsp[0].minor.yy144 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_RESTRICT;
}
        break;
      case 59: /* refact ::= NO ACTION */
{
    yymsp[-1].minor.yy144 = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION;
}
        break;
      case 60: /* defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
{
    yymsp[-2].minor.yy144 = 0;
}
        break;
      case 61: /* defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
      case 76: /* orconf ::= OR resolvetype */ yytestcase(yyruleno==76);
      case 173: /* insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==173);
      case 332: /* frame_exclude_opt ::= EXCLUDE frame_exclude */ yytestcase(yyruleno==332);
{
    yymsp[-1].minor.yy144 = yymsp[0].minor.yy144;
}
        break;
      case 63: /* init_deferred_pred_opt ::= INITIALLY DEFERRED */
      case 80: /* ifexists ::= IF EXISTS */ yytestcase(yyruleno==80);
      case 247: /* collate ::= COLLATE ID|STRING */ yytestcase(yyruleno==247);
{
    yymsp[-1].minor.yy144 = 1;
}
        break;
      case 64: /* init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
      case 352: /* trans_opt ::= TRANSACTION nm */ yytestcase(yyruleno==352);
{
    yymsp[-1].minor.yy144 = 0;
}
        break;
      case 65: /* conslist_opt ::= */
      case 101: /* sclp ::= */ yytestcase(yyruleno==101);
      case 107: /* from ::= */ yytestcase(yyruleno==107);
      case 110: /* stl_prefix ::= */ yytestcase(yyruleno==110);
      case 134: /* orderby_opt ::= */ yytestcase(yyruleno==134);
      case 144: /* groupby_opt ::= */ yytestcase(yyruleno==144);
      case 146: /* having_opt ::= */ yytestcase(yyruleno==146);
      case 148: /* limit_opt ::= */ yytestcase(yyruleno==148);
      case 153: /* where_opt ::= */ yytestcase(yyruleno==153);
      case 155: /* where_opt_ret ::= */ yytestcase(yyruleno==155);
      case 166: /* upsert ::= */ yytestcase(yyruleno==166);
      case 175: /* idlist_opt ::= */ yytestcase(yyruleno==175);
      case 232: /* case_else ::= */ yytestcase(yyruleno==232);
      case 233: /* case_operand ::= */ yytestcase(yyruleno==233);
      case 234: /* exprlist ::= */ yytestcase(yyruleno==234);
      case 242: /* eidlist_opt ::= */ yytestcase(yyruleno==242);
      case 252: /* vinto ::= */ yytestcase(yyruleno==252);
      case 268: /* when_clause ::= */ yytestcase(yyruleno==268);
      case 287: /* key_opt ::= */ yytestcase(yyruleno==287);
      case 321: /* frame_opt ::= */ yytestcase(yyruleno==321);
{
    yymsp[1].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 66: /* tconscomma ::= COMMA */
{ yymsp[0].minor.yy144 = 1; }
        break;
      case 68: /* tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
{
    yymsp[-6].minor.yy462.node = synq_ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_PRIMARY_KEY,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)yymsp[0].minor.yy144, (SyntaqliteBool)yymsp[-2].minor.yy144,
        yymsp[-3].minor.yy391, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-6].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 69: /* tcons ::= UNIQUE LP sortlist RP onconf */
{
    yymsp[-4].minor.yy462.node = synq_ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_UNIQUE,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)yymsp[0].minor.yy144, SYNTAQLITE_BOOL_FALSE,
        yymsp[-2].minor.yy391, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-4].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 70: /* tcons ::= CHECK LP expr RP onconf */
{
    yymsp[-4].minor.yy462.node = synq_ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_CHECK,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)yymsp[0].minor.yy144, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NULL_NODE, yymsp[-2].minor.yy391, SYNTAQLITE_NULL_NODE);
    yymsp[-4].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 71: /* tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
{
    SyntaqliteForeignKeyAction on_del = (SyntaqliteForeignKeyAction)(yymsp[-1].minor.yy144 & 0xff);
    SyntaqliteForeignKeyAction on_upd = (SyntaqliteForeignKeyAction)((yymsp[-1].minor.yy144 >> 8) & 0xff);
    uint32_t fk = synq_ast_foreign_key_clause(pCtx->astCtx,
        synq_span(pCtx, yymsp[-3].minor.yy0), yymsp[-2].minor.yy391, on_del, on_upd, (SyntaqliteBool)yymsp[0].minor.yy144);
    yymsp[-9].minor.yy462.node = synq_ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_FOREIGN_KEY,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_BOOL_FALSE,
        yymsp[-6].minor.yy391, SYNTAQLITE_NULL_NODE, fk);
    yymsp[-9].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 73: /* onconf ::= */
      case 75: /* orconf ::= */ yytestcase(yyruleno==75);
{
    yymsp[1].minor.yy144 = (int)SYNTAQLITE_CONFLICT_ACTION_DEFAULT;
}
        break;
      case 74: /* onconf ::= ON CONFLICT resolvetype */
{
    yymsp[-2].minor.yy144 = yymsp[0].minor.yy144;
}
        break;
      case 77: /* resolvetype ::= IGNORE */
{
    yymsp[0].minor.yy144 = (int)SYNTAQLITE_CONFLICT_ACTION_IGNORE;
}
        break;
      case 78: /* resolvetype ::= REPLACE */
      case 174: /* insert_cmd ::= REPLACE */ yytestcase(yyruleno==174);
{
    yymsp[0].minor.yy144 = (int)SYNTAQLITE_CONFLICT_ACTION_REPLACE;
}
        break;
      case 79: /* cmd ::= DROP TABLE ifexists fullname */
{
    yymsp[-3].minor.yy391 = synq_ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TABLE, (SyntaqliteBool)yymsp[-1].minor.yy144, yymsp[0].minor.yy391);
}
        break;
      case 82: /* cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
{
    SyntaqliteSourceSpan view_name = yymsp[-3].minor.yy0.z ? synq_span(pCtx, yymsp[-3].minor.yy0) : synq_span(pCtx, yymsp[-4].minor.yy0);
    SyntaqliteSourceSpan view_schema = yymsp[-3].minor.yy0.z ? synq_span(pCtx, yymsp[-4].minor.yy0) : SYNQ_NO_SPAN;
    yymsp[-8].minor.yy391 = synq_ast_create_view_stmt(pCtx->astCtx,
        view_name,
        view_schema,
        (SyntaqliteBool)yymsp[-7].minor.yy144,
        (SyntaqliteBool)yymsp[-5].minor.yy144,
        yymsp[-2].minor.yy391,
        yymsp[0].minor.yy391);
}
        break;
      case 83: /* cmd ::= DROP VIEW ifexists fullname */
{
    yymsp[-3].minor.yy391 = synq_ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_VIEW, (SyntaqliteBool)yymsp[-1].minor.yy144, yymsp[0].minor.yy391);
}
        break;
      case 85: /* select ::= WITH wqlist selectnowith */
{
    yymsp[-2].minor.yy391 = synq_ast_with_clause(pCtx->astCtx, 0, yymsp[-1].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 86: /* select ::= WITH RECURSIVE wqlist selectnowith */
{
    yymsp[-3].minor.yy391 = synq_ast_with_clause(pCtx->astCtx, 1, yymsp[-1].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 88: /* selectnowith ::= selectnowith multiselect_op oneselect */
{
    yymsp[-2].minor.yy391 = synq_ast_compound_select(pCtx->astCtx, (SyntaqliteCompoundOp)yymsp[-1].minor.yy144, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 89: /* multiselect_op ::= UNION */
{ yylhsminor.yy144 = 0; UNUSED_PARAMETER(yymsp[0].minor.yy0); }
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 90: /* multiselect_op ::= UNION ALL */
      case 222: /* in_op ::= NOT IN */ yytestcase(yyruleno==222);
{ yymsp[-1].minor.yy144 = 1; }
        break;
      case 91: /* multiselect_op ::= EXCEPT|INTERSECT */
{
    yylhsminor.yy144 = (yymsp[0].minor.yy0.type == SYNTAQLITE_TOKEN_INTERSECT) ? 2 : 3;
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 92: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
{
    yymsp[-8].minor.yy391 = synq_ast_select_stmt(pCtx->astCtx, (SyntaqliteSelectStmtFlags){.raw = (uint8_t)yymsp[-7].minor.yy391}, yymsp[-6].minor.yy391, yymsp[-5].minor.yy391, yymsp[-4].minor.yy391, yymsp[-3].minor.yy391, yymsp[-2].minor.yy391, yymsp[-1].minor.yy391, yymsp[0].minor.yy391, SYNTAQLITE_NULL_NODE);
}
        break;
      case 93: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
{
    yymsp[-9].minor.yy391 = synq_ast_select_stmt(pCtx->astCtx, (SyntaqliteSelectStmtFlags){.raw = (uint8_t)yymsp[-8].minor.yy391}, yymsp[-7].minor.yy391, yymsp[-6].minor.yy391, yymsp[-5].minor.yy391, yymsp[-4].minor.yy391, yymsp[-3].minor.yy391, yymsp[-1].minor.yy391, yymsp[0].minor.yy391, yymsp[-2].minor.yy391);
}
        break;
      case 94: /* values ::= VALUES LP nexprlist RP */
{
    yymsp[-3].minor.yy391 = synq_ast_values_row_list(pCtx->astCtx, yymsp[-1].minor.yy391);
}
        break;
      case 95: /* oneselect ::= mvalues */
      case 377: /* oneselect ::= values */ yytestcase(yyruleno==377);
{
    yylhsminor.yy391 = synq_ast_values_clause(pCtx->astCtx, yymsp[0].minor.yy391);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 96: /* mvalues ::= values COMMA LP nexprlist RP */
      case 97: /* mvalues ::= mvalues COMMA LP nexprlist RP */ yytestcase(yyruleno==97);
{
    yymsp[-4].minor.yy391 = synq_ast_values_row_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, yymsp[-1].minor.yy391);
}
        break;
      case 98: /* distinct ::= DISTINCT */
      case 139: /* sortorder ::= DESC */ yytestcase(yyruleno==139);
{
    yymsp[0].minor.yy391 = 1;
}
        break;
      case 99: /* distinct ::= ALL */
      case 138: /* sortorder ::= ASC */ yytestcase(yyruleno==138);
      case 218: /* between_op ::= BETWEEN */ yytestcase(yyruleno==218);
{
    yymsp[0].minor.yy391 = 0;
}
        break;
      case 100: /* distinct ::= */
      case 140: /* sortorder ::= */ yytestcase(yyruleno==140);
      case 143: /* nulls ::= */ yytestcase(yyruleno==143);
{
    yymsp[1].minor.yy391 = 0;
}
        break;
      case 102: /* selcollist ::= sclp scanpt expr scanpt as */
{
    SyntaqliteSourceSpan alias = (yymsp[0].minor.yy0.z) ? synq_span(pCtx, yymsp[0].minor.yy0) : SYNQ_NO_SPAN;
    uint32_t col = synq_ast_result_column(pCtx->astCtx, (SyntaqliteResultColumnFlags){0}, alias, yymsp[-2].minor.yy391);
    yylhsminor.yy391 = synq_ast_result_column_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, col);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 103: /* selcollist ::= sclp scanpt STAR */
{
    uint32_t col = synq_ast_result_column(pCtx->astCtx, (SyntaqliteResultColumnFlags){.star = 1}, SYNQ_NO_SPAN, SYNTAQLITE_NULL_NODE);
    yylhsminor.yy391 = synq_ast_result_column_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, col);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 104: /* selcollist ::= sclp scanpt nm DOT STAR */
{
    uint32_t col = synq_ast_result_column(pCtx->astCtx, (SyntaqliteResultColumnFlags){.star = 1}, synq_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NULL_NODE);
    yylhsminor.yy391 = synq_ast_result_column_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, col);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 105: /* as ::= AS nm */
      case 258: /* plus_num ::= PLUS INTEGER|FLOAT */ yytestcase(yyruleno==258);
{
    yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;
}
        break;
      case 108: /* from ::= FROM seltablist */
      case 147: /* having_opt ::= HAVING expr */ yytestcase(yyruleno==147);
      case 154: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==154);
      case 156: /* where_opt_ret ::= WHERE expr */ yytestcase(yyruleno==156);
      case 231: /* case_else ::= ELSE expr */ yytestcase(yyruleno==231);
      case 251: /* vinto ::= INTO expr */ yytestcase(yyruleno==251);
      case 269: /* when_clause ::= WHEN expr */ yytestcase(yyruleno==269);
      case 288: /* key_opt ::= KEY expr */ yytestcase(yyruleno==288);
      case 336: /* window_clause ::= WINDOW windowdefn_list */ yytestcase(yyruleno==336);
{
    yymsp[-1].minor.yy391 = yymsp[0].minor.yy391;
}
        break;
      case 109: /* stl_prefix ::= seltablist joinop */
{
    yymsp[-1].minor.yy391 = synq_ast_join_prefix(pCtx->astCtx, yymsp[-1].minor.yy391, (SyntaqliteJoinType)yymsp[0].minor.yy144);
}
        break;
      case 111: /* seltablist ::= stl_prefix nm dbnm as on_using */
{
    SyntaqliteSourceSpan alias = (yymsp[-1].minor.yy0.z != NULL) ? synq_span(pCtx, yymsp[-1].minor.yy0) : SYNQ_NO_SPAN;
    SyntaqliteSourceSpan table_name;
    SyntaqliteSourceSpan schema;
    if (yymsp[-2].minor.yy0.z != NULL) {
        table_name = synq_span(pCtx, yymsp[-2].minor.yy0);
        schema = synq_span(pCtx, yymsp[-3].minor.yy0);
    } else {
        table_name = synq_span(pCtx, yymsp[-3].minor.yy0);
        schema = SYNQ_NO_SPAN;
    }
    uint32_t tref = synq_ast_table_ref(pCtx->astCtx, table_name, schema, alias);
    if (yymsp[-4].minor.yy391 == SYNTAQLITE_NULL_NODE) {
        yymsp[-4].minor.yy391 = tref;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->astCtx->ast, yymsp[-4].minor.yy391);
        yymsp[-4].minor.yy391 = synq_ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            tref, yymsp[0].minor.yy136.on_expr, yymsp[0].minor.yy136.using_cols);
    }
}
        break;
      case 112: /* seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
{
    (void)yymsp[-1].minor.yy0;
    SyntaqliteSourceSpan alias = (yymsp[-2].minor.yy0.z != NULL) ? synq_span(pCtx, yymsp[-2].minor.yy0) : SYNQ_NO_SPAN;
    SyntaqliteSourceSpan table_name;
    SyntaqliteSourceSpan schema;
    if (yymsp[-3].minor.yy0.z != NULL) {
        table_name = synq_span(pCtx, yymsp[-3].minor.yy0);
        schema = synq_span(pCtx, yymsp[-4].minor.yy0);
    } else {
        table_name = synq_span(pCtx, yymsp[-4].minor.yy0);
        schema = SYNQ_NO_SPAN;
    }
    uint32_t tref = synq_ast_table_ref(pCtx->astCtx, table_name, schema, alias);
    if (yymsp[-5].minor.yy391 == SYNTAQLITE_NULL_NODE) {
        yymsp[-5].minor.yy391 = tref;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->astCtx->ast, yymsp[-5].minor.yy391);
        yymsp[-5].minor.yy391 = synq_ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            tref, yymsp[0].minor.yy136.on_expr, yymsp[0].minor.yy136.using_cols);
    }
}
        break;
      case 113: /* seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
{
    (void)yymsp[-3].minor.yy391;
    SyntaqliteSourceSpan alias = (yymsp[-1].minor.yy0.z != NULL) ? synq_span(pCtx, yymsp[-1].minor.yy0) : SYNQ_NO_SPAN;
    SyntaqliteSourceSpan table_name;
    SyntaqliteSourceSpan schema;
    if (yymsp[-5].minor.yy0.z != NULL) {
        table_name = synq_span(pCtx, yymsp[-5].minor.yy0);
        schema = synq_span(pCtx, yymsp[-6].minor.yy0);
    } else {
        table_name = synq_span(pCtx, yymsp[-6].minor.yy0);
        schema = SYNQ_NO_SPAN;
    }
    uint32_t tref = synq_ast_table_ref(pCtx->astCtx, table_name, schema, alias);
    if (yymsp[-7].minor.yy391 == SYNTAQLITE_NULL_NODE) {
        yymsp[-7].minor.yy391 = tref;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->astCtx->ast, yymsp[-7].minor.yy391);
        yymsp[-7].minor.yy391 = synq_ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            tref, yymsp[0].minor.yy136.on_expr, yymsp[0].minor.yy136.using_cols);
    }
}
        break;
      case 114: /* seltablist ::= stl_prefix LP select RP as on_using */
{
    SyntaqliteSourceSpan alias = (yymsp[-1].minor.yy0.z != NULL) ? synq_span(pCtx, yymsp[-1].minor.yy0) : SYNQ_NO_SPAN;
    uint32_t sub = synq_ast_subquery_table_source(pCtx->astCtx, yymsp[-3].minor.yy391, alias);
    if (yymsp[-5].minor.yy391 == SYNTAQLITE_NULL_NODE) {
        yymsp[-5].minor.yy391 = sub;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->astCtx->ast, yymsp[-5].minor.yy391);
        yymsp[-5].minor.yy391 = synq_ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            sub, yymsp[0].minor.yy136.on_expr, yymsp[0].minor.yy136.using_cols);
    }
}
        break;
      case 115: /* seltablist ::= stl_prefix LP seltablist RP as on_using */
{
    (void)yymsp[-1].minor.yy0; (void)yymsp[0].minor.yy136;
    if (yymsp[-5].minor.yy391 == SYNTAQLITE_NULL_NODE) {
        yymsp[-5].minor.yy391 = yymsp[-3].minor.yy391;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->astCtx->ast, yymsp[-5].minor.yy391);
        yymsp[-5].minor.yy391 = synq_ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            yymsp[-3].minor.yy391, yymsp[0].minor.yy136.on_expr, yymsp[0].minor.yy136.using_cols);
    }
}
        break;
      case 116: /* dbnm ::= */
{ yymsp[1].minor.yy0.z = NULL; yymsp[1].minor.yy0.n = 0; }
        break;
      case 117: /* dbnm ::= DOT nm */
{ yymsp[-1].minor.yy0 = yymsp[0].minor.yy0; }
        break;
      case 118: /* fullname ::= nm */
{
    yylhsminor.yy391 = synq_ast_qualified_name(pCtx->astCtx,
        synq_span(pCtx, yymsp[0].minor.yy0),
        SYNQ_NO_SPAN);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 119: /* fullname ::= nm DOT nm */
{
    yylhsminor.yy391 = synq_ast_qualified_name(pCtx->astCtx,
        synq_span(pCtx, yymsp[0].minor.yy0),
        synq_span(pCtx, yymsp[-2].minor.yy0));
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 120: /* xfullname ::= nm */
{
    yylhsminor.yy391 = synq_ast_table_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[0].minor.yy0), SYNQ_NO_SPAN, SYNQ_NO_SPAN);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 121: /* xfullname ::= nm DOT nm */
{
    yylhsminor.yy391 = synq_ast_table_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[0].minor.yy0), synq_span(pCtx, yymsp[-2].minor.yy0), SYNQ_NO_SPAN);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 122: /* xfullname ::= nm DOT nm AS nm */
{
    yylhsminor.yy391 = synq_ast_table_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[-2].minor.yy0), synq_span(pCtx, yymsp[-4].minor.yy0), synq_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 123: /* xfullname ::= nm AS nm */
{
    yylhsminor.yy391 = synq_ast_table_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[-2].minor.yy0), SYNQ_NO_SPAN, synq_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 124: /* joinop ::= COMMA|JOIN */
{
    yylhsminor.yy144 = (yymsp[0].minor.yy0.type == SYNTAQLITE_TOKEN_COMMA)
        ? (int)SYNTAQLITE_JOIN_TYPE_COMMA
        : (int)SYNTAQLITE_JOIN_TYPE_INNER;
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 125: /* joinop ::= JOIN_KW JOIN */
{
    // Single keyword: LEFT, RIGHT, INNER, OUTER, CROSS, NATURAL, FULL
    if (yymsp[-1].minor.yy0.n == 4 && (yymsp[-1].minor.yy0.z[0] == 'L' || yymsp[-1].minor.yy0.z[0] == 'l')) {
        yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_LEFT;
    } else if (yymsp[-1].minor.yy0.n == 5 && (yymsp[-1].minor.yy0.z[0] == 'R' || yymsp[-1].minor.yy0.z[0] == 'r')) {
        yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_RIGHT;
    } else if (yymsp[-1].minor.yy0.n == 5 && (yymsp[-1].minor.yy0.z[0] == 'I' || yymsp[-1].minor.yy0.z[0] == 'i')) {
        yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_INNER;
    } else if (yymsp[-1].minor.yy0.n == 5 && (yymsp[-1].minor.yy0.z[0] == 'O' || yymsp[-1].minor.yy0.z[0] == 'o')) {
        // OUTER alone is not valid but treat as INNER
        yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_INNER;
    } else if (yymsp[-1].minor.yy0.n == 5 && (yymsp[-1].minor.yy0.z[0] == 'C' || yymsp[-1].minor.yy0.z[0] == 'c')) {
        yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_CROSS;
    } else if (yymsp[-1].minor.yy0.n == 7 && (yymsp[-1].minor.yy0.z[0] == 'N' || yymsp[-1].minor.yy0.z[0] == 'n')) {
        yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
    } else if (yymsp[-1].minor.yy0.n == 4 && (yymsp[-1].minor.yy0.z[0] == 'F' || yymsp[-1].minor.yy0.z[0] == 'f')) {
        yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_FULL;
    } else {
        yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_INNER;
    }
}
  yymsp[-1].minor.yy144 = yylhsminor.yy144;
        break;
      case 126: /* joinop ::= JOIN_KW nm JOIN */
{
    // Two keywords: LEFT OUTER, NATURAL LEFT, NATURAL RIGHT, etc.
    (void)yymsp[-1].minor.yy0;
    if (yymsp[-2].minor.yy0.n == 7 && (yymsp[-2].minor.yy0.z[0] == 'N' || yymsp[-2].minor.yy0.z[0] == 'n')) {
        // NATURAL + something
        if (yymsp[-1].minor.yy0.n == 4 && (yymsp[-1].minor.yy0.z[0] == 'L' || yymsp[-1].minor.yy0.z[0] == 'l')) {
            yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_LEFT;
        } else if (yymsp[-1].minor.yy0.n == 5 && (yymsp[-1].minor.yy0.z[0] == 'R' || yymsp[-1].minor.yy0.z[0] == 'r')) {
            yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_RIGHT;
        } else if (yymsp[-1].minor.yy0.n == 5 && (yymsp[-1].minor.yy0.z[0] == 'I' || yymsp[-1].minor.yy0.z[0] == 'i')) {
            yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
        } else if (yymsp[-1].minor.yy0.n == 4 && (yymsp[-1].minor.yy0.z[0] == 'F' || yymsp[-1].minor.yy0.z[0] == 'f')) {
            yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_FULL;
        } else if (yymsp[-1].minor.yy0.n == 5 && (yymsp[-1].minor.yy0.z[0] == 'C' || yymsp[-1].minor.yy0.z[0] == 'c')) {
            // NATURAL CROSS -> just CROSS
            yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_CROSS;
        } else {
            yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
        }
    } else if (yymsp[-2].minor.yy0.n == 4 && (yymsp[-2].minor.yy0.z[0] == 'L' || yymsp[-2].minor.yy0.z[0] == 'l')) {
        // LEFT OUTER
        yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_LEFT;
    } else if (yymsp[-2].minor.yy0.n == 5 && (yymsp[-2].minor.yy0.z[0] == 'R' || yymsp[-2].minor.yy0.z[0] == 'r')) {
        // RIGHT OUTER
        yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_RIGHT;
    } else if (yymsp[-2].minor.yy0.n == 4 && (yymsp[-2].minor.yy0.z[0] == 'F' || yymsp[-2].minor.yy0.z[0] == 'f')) {
        // FULL OUTER
        yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_FULL;
    } else {
        yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_INNER;
    }
}
  yymsp[-2].minor.yy144 = yylhsminor.yy144;
        break;
      case 127: /* joinop ::= JOIN_KW nm nm JOIN */
{
    // Three keywords: NATURAL LEFT OUTER, NATURAL RIGHT OUTER, etc.
    (void)yymsp[-2].minor.yy0; (void)yymsp[-1].minor.yy0;
    if (yymsp[-3].minor.yy0.n == 7 && (yymsp[-3].minor.yy0.z[0] == 'N' || yymsp[-3].minor.yy0.z[0] == 'n')) {
        // NATURAL yylhsminor.yy144 OUTER
        if (yymsp[-2].minor.yy0.n == 4 && (yymsp[-2].minor.yy0.z[0] == 'L' || yymsp[-2].minor.yy0.z[0] == 'l')) {
            yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_LEFT;
        } else if (yymsp[-2].minor.yy0.n == 5 && (yymsp[-2].minor.yy0.z[0] == 'R' || yymsp[-2].minor.yy0.z[0] == 'r')) {
            yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_RIGHT;
        } else if (yymsp[-2].minor.yy0.n == 4 && (yymsp[-2].minor.yy0.z[0] == 'F' || yymsp[-2].minor.yy0.z[0] == 'f')) {
            yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_FULL;
        } else {
            yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
        }
    } else {
        yylhsminor.yy144 = (int)SYNTAQLITE_JOIN_TYPE_INNER;
    }
}
  yymsp[-3].minor.yy144 = yylhsminor.yy144;
        break;
      case 128: /* on_using ::= ON expr */
{
    yymsp[-1].minor.yy136.on_expr = yymsp[0].minor.yy391;
    yymsp[-1].minor.yy136.using_cols = SYNTAQLITE_NULL_NODE;
}
        break;
      case 129: /* on_using ::= USING LP idlist RP */
{
    yymsp[-3].minor.yy136.on_expr = SYNTAQLITE_NULL_NODE;
    yymsp[-3].minor.yy136.using_cols = yymsp[-1].minor.yy391;
}
        break;
      case 130: /* on_using ::= */
{
    yymsp[1].minor.yy136.on_expr = SYNTAQLITE_NULL_NODE;
    yymsp[1].minor.yy136.using_cols = SYNTAQLITE_NULL_NODE;
}
        break;
      case 132: /* indexed_by ::= INDEXED BY nm */
{
    yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;
}
        break;
      case 133: /* indexed_by ::= NOT INDEXED */
{
    yymsp[-1].minor.yy0.z = NULL; yymsp[-1].minor.yy0.n = 1;
}
        break;
      case 135: /* orderby_opt ::= ORDER BY sortlist */
      case 145: /* groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==145);
{
    yymsp[-2].minor.yy391 = yymsp[0].minor.yy391;
}
        break;
      case 136: /* sortlist ::= sortlist COMMA expr sortorder nulls */
{
    uint32_t term = synq_ast_ordering_term(pCtx->astCtx, yymsp[-2].minor.yy391, (SyntaqliteSortOrder)yymsp[-1].minor.yy391, (SyntaqliteNullsOrder)yymsp[0].minor.yy391);
    yylhsminor.yy391 = synq_ast_order_by_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, term);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 137: /* sortlist ::= expr sortorder nulls */
{
    uint32_t term = synq_ast_ordering_term(pCtx->astCtx, yymsp[-2].minor.yy391, (SyntaqliteSortOrder)yymsp[-1].minor.yy391, (SyntaqliteNullsOrder)yymsp[0].minor.yy391);
    yylhsminor.yy391 = synq_ast_order_by_list(pCtx->astCtx, term);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 141: /* nulls ::= NULLS FIRST */
      case 219: /* between_op ::= NOT BETWEEN */ yytestcase(yyruleno==219);
{
    yymsp[-1].minor.yy391 = 1;
}
        break;
      case 142: /* nulls ::= NULLS LAST */
{
    yymsp[-1].minor.yy391 = 2;
}
        break;
      case 149: /* limit_opt ::= LIMIT expr */
{
    yymsp[-1].minor.yy391 = synq_ast_limit_clause(pCtx->astCtx, yymsp[0].minor.yy391, SYNTAQLITE_NULL_NODE);
}
        break;
      case 150: /* limit_opt ::= LIMIT expr OFFSET expr */
{
    yymsp[-3].minor.yy391 = synq_ast_limit_clause(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 151: /* limit_opt ::= LIMIT expr COMMA expr */
{
    yymsp[-3].minor.yy391 = synq_ast_limit_clause(pCtx->astCtx, yymsp[0].minor.yy391, yymsp[-2].minor.yy391);
}
        break;
      case 152: /* cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
{
    (void)yymsp[-1].minor.yy0;
    uint32_t del = synq_ast_delete_stmt(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
    if (yymsp[-5].minor.yy581.cte_list != SYNTAQLITE_NULL_NODE) {
        yylhsminor.yy391 = synq_ast_with_clause(pCtx->astCtx, yymsp[-5].minor.yy581.is_recursive, yymsp[-5].minor.yy581.cte_list, del);
    } else {
        yylhsminor.yy391 = del;
    }
}
  yymsp[-5].minor.yy391 = yylhsminor.yy391;
        break;
      case 157: /* where_opt_ret ::= RETURNING selcollist */
{
    // Ignore RETURNING clause for now (just discard the column list)
    (void)yymsp[0].minor.yy391;
    yymsp[-1].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 158: /* where_opt_ret ::= WHERE expr RETURNING selcollist */
{
    // Keep WHERE, ignore RETURNING
    (void)yymsp[0].minor.yy391;
    yymsp[-3].minor.yy391 = yymsp[-2].minor.yy391;
}
        break;
      case 159: /* cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
{
    (void)yymsp[-4].minor.yy0;
    uint32_t upd = synq_ast_update_stmt(pCtx->astCtx, (SyntaqliteConflictAction)yymsp[-6].minor.yy144, yymsp[-5].minor.yy391, yymsp[-2].minor.yy391, yymsp[-1].minor.yy391, yymsp[0].minor.yy391);
    if (yymsp[-8].minor.yy581.cte_list != SYNTAQLITE_NULL_NODE) {
        yylhsminor.yy391 = synq_ast_with_clause(pCtx->astCtx, yymsp[-8].minor.yy581.is_recursive, yymsp[-8].minor.yy581.cte_list, upd);
    } else {
        yylhsminor.yy391 = upd;
    }
}
  yymsp[-8].minor.yy391 = yylhsminor.yy391;
        break;
      case 160: /* setlist ::= setlist COMMA nm EQ expr */
{
    uint32_t clause = synq_ast_set_clause(pCtx->astCtx,
        synq_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy391);
    yylhsminor.yy391 = synq_ast_set_clause_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, clause);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 161: /* setlist ::= setlist COMMA LP idlist RP EQ expr */
{
    uint32_t clause = synq_ast_set_clause(pCtx->astCtx,
        SYNQ_NO_SPAN, yymsp[-3].minor.yy391, yymsp[0].minor.yy391);
    yylhsminor.yy391 = synq_ast_set_clause_list_append(pCtx->astCtx, yymsp[-6].minor.yy391, clause);
}
  yymsp[-6].minor.yy391 = yylhsminor.yy391;
        break;
      case 162: /* setlist ::= nm EQ expr */
{
    uint32_t clause = synq_ast_set_clause(pCtx->astCtx,
        synq_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy391);
    yylhsminor.yy391 = synq_ast_set_clause_list(pCtx->astCtx, clause);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 163: /* setlist ::= LP idlist RP EQ expr */
{
    uint32_t clause = synq_ast_set_clause(pCtx->astCtx,
        SYNQ_NO_SPAN, yymsp[-3].minor.yy391, yymsp[0].minor.yy391);
    yymsp[-4].minor.yy391 = synq_ast_set_clause_list(pCtx->astCtx, clause);
}
        break;
      case 164: /* cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
{
    (void)yymsp[0].minor.yy391;
    uint32_t ins = synq_ast_insert_stmt(pCtx->astCtx, (SyntaqliteConflictAction)yymsp[-5].minor.yy144, yymsp[-3].minor.yy391, yymsp[-2].minor.yy391, yymsp[-1].minor.yy391);
    if (yymsp[-6].minor.yy581.cte_list != SYNTAQLITE_NULL_NODE) {
        yylhsminor.yy391 = synq_ast_with_clause(pCtx->astCtx, yymsp[-6].minor.yy581.is_recursive, yymsp[-6].minor.yy581.cte_list, ins);
    } else {
        yylhsminor.yy391 = ins;
    }
}
  yymsp[-6].minor.yy391 = yylhsminor.yy391;
        break;
      case 165: /* cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
{
    uint32_t ins = synq_ast_insert_stmt(pCtx->astCtx, (SyntaqliteConflictAction)yymsp[-6].minor.yy144, yymsp[-4].minor.yy391, yymsp[-3].minor.yy391, SYNTAQLITE_NULL_NODE);
    if (yymsp[-7].minor.yy581.cte_list != SYNTAQLITE_NULL_NODE) {
        yylhsminor.yy391 = synq_ast_with_clause(pCtx->astCtx, yymsp[-7].minor.yy581.is_recursive, yymsp[-7].minor.yy581.cte_list, ins);
    } else {
        yylhsminor.yy391 = ins;
    }
}
  yymsp[-7].minor.yy391 = yylhsminor.yy391;
        break;
      case 167: /* upsert ::= RETURNING selcollist */
{
    (void)yymsp[0].minor.yy391;
    yymsp[-1].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 168: /* upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
{
    (void)yymsp[-8].minor.yy391; (void)yymsp[-6].minor.yy391; (void)yymsp[-2].minor.yy391; (void)yymsp[-1].minor.yy391; (void)yymsp[0].minor.yy391;
    yymsp[-11].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 169: /* upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
{
    (void)yymsp[-5].minor.yy391; (void)yymsp[-3].minor.yy391; (void)yymsp[0].minor.yy391;
    yymsp[-8].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 170: /* upsert ::= ON CONFLICT DO NOTHING returning */
{
    yymsp[-4].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 171: /* upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
{
    (void)yymsp[-2].minor.yy391; (void)yymsp[-1].minor.yy391;
    yymsp[-7].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 172: /* returning ::= RETURNING selcollist */
{
    (void)yymsp[0].minor.yy391;
}
        break;
      case 176: /* idlist_opt ::= LP idlist RP */
      case 179: /* expr ::= LP expr RP */ yytestcase(yyruleno==179);
      case 243: /* eidlist_opt ::= LP eidlist RP */ yytestcase(yyruleno==243);
      case 278: /* trigger_cmd ::= scanpt select scanpt */ yytestcase(yyruleno==278);
{
    yymsp[-2].minor.yy391 = yymsp[-1].minor.yy391;
}
        break;
      case 177: /* idlist ::= idlist COMMA nm */
{
    uint32_t col = synq_ast_column_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[0].minor.yy0), SYNQ_NO_SPAN, SYNQ_NO_SPAN);
    yymsp[-2].minor.yy391 = synq_ast_expr_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, col);
}
        break;
      case 178: /* idlist ::= nm */
{
    uint32_t col = synq_ast_column_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[0].minor.yy0), SYNQ_NO_SPAN, SYNQ_NO_SPAN);
    yylhsminor.yy391 = synq_ast_expr_list(pCtx->astCtx, col);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 180: /* expr ::= ID|INDEXED|JOIN_KW */
{
    yylhsminor.yy391 = synq_ast_column_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[0].minor.yy0),
        SYNQ_NO_SPAN,
        SYNQ_NO_SPAN);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 181: /* expr ::= nm DOT nm */
{
    yylhsminor.yy391 = synq_ast_column_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[0].minor.yy0),
        synq_span(pCtx, yymsp[-2].minor.yy0),
        SYNQ_NO_SPAN);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 182: /* expr ::= nm DOT nm DOT nm */
{
    yylhsminor.yy391 = synq_ast_column_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[0].minor.yy0),
        synq_span(pCtx, yymsp[-2].minor.yy0),
        synq_span(pCtx, yymsp[-4].minor.yy0));
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 183: /* term ::= NULL|FLOAT|BLOB */
{
    SyntaqliteLiteralType lit_type;
    switch (yymsp[0].minor.yy0.type) {
        case SYNTAQLITE_TOKEN_NULL:  lit_type = SYNTAQLITE_LITERAL_TYPE_NULL; break;
        case SYNTAQLITE_TOKEN_FLOAT: lit_type = SYNTAQLITE_LITERAL_TYPE_FLOAT; break;
        case SYNTAQLITE_TOKEN_BLOB:  lit_type = SYNTAQLITE_LITERAL_TYPE_BLOB; break;
        default:       lit_type = SYNTAQLITE_LITERAL_TYPE_NULL; break;
    }
    yylhsminor.yy391 = synq_ast_literal(pCtx->astCtx, lit_type, synq_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 184: /* term ::= STRING */
{
    yylhsminor.yy391 = synq_ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_STRING, synq_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 185: /* term ::= INTEGER */
{
    yylhsminor.yy391 = synq_ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_INTEGER, synq_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 186: /* expr ::= VARIABLE */
{
    yylhsminor.yy391 = synq_ast_variable(pCtx->astCtx, synq_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 187: /* expr ::= expr COLLATE ID|STRING */
{
    yylhsminor.yy391 = synq_ast_collate_expr(pCtx->astCtx, yymsp[-2].minor.yy391, synq_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 188: /* expr ::= CAST LP expr AS typetoken RP */
{
    yymsp[-5].minor.yy391 = synq_ast_cast_expr(pCtx->astCtx, yymsp[-3].minor.yy391, synq_span(pCtx, yymsp[-1].minor.yy0));
}
        break;
      case 189: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
{
    yylhsminor.yy391 = synq_ast_function_call(pCtx->astCtx,
        synq_span(pCtx, yymsp[-4].minor.yy0),
        (SyntaqliteFunctionCallFlags){.raw = (uint8_t)yymsp[-2].minor.yy391},
        yymsp[-1].minor.yy391,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 190: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
{
    yylhsminor.yy391 = synq_ast_aggregate_function_call(pCtx->astCtx,
        synq_span(pCtx, yymsp[-7].minor.yy0),
        (SyntaqliteAggregateFunctionCallFlags){.raw = (uint8_t)yymsp[-5].minor.yy391},
        yymsp[-4].minor.yy391,
        yymsp[-1].minor.yy391,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE);
}
  yymsp[-7].minor.yy391 = yylhsminor.yy391;
        break;
      case 191: /* expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
{
    yylhsminor.yy391 = synq_ast_function_call(pCtx->astCtx,
        synq_span(pCtx, yymsp[-3].minor.yy0),
        (SyntaqliteFunctionCallFlags){.star = 1},
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE);
}
  yymsp[-3].minor.yy391 = yylhsminor.yy391;
        break;
      case 192: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
{
    SyntaqliteFilterOver *fo = (SyntaqliteFilterOver*)
        (pCtx->astCtx->ast.data + pCtx->astCtx->ast.offsets[yymsp[0].minor.yy391]);
    yylhsminor.yy391 = synq_ast_function_call(pCtx->astCtx,
        synq_span(pCtx, yymsp[-5].minor.yy0),
        (SyntaqliteFunctionCallFlags){.raw = (uint8_t)yymsp[-3].minor.yy391},
        yymsp[-2].minor.yy391,
        fo->filter_expr,
        fo->over_def);
}
  yymsp[-5].minor.yy391 = yylhsminor.yy391;
        break;
      case 193: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
{
    SyntaqliteFilterOver *fo = (SyntaqliteFilterOver*)
        (pCtx->astCtx->ast.data + pCtx->astCtx->ast.offsets[yymsp[0].minor.yy391]);
    yylhsminor.yy391 = synq_ast_aggregate_function_call(pCtx->astCtx,
        synq_span(pCtx, yymsp[-8].minor.yy0),
        (SyntaqliteAggregateFunctionCallFlags){.raw = (uint8_t)yymsp[-6].minor.yy391},
        yymsp[-5].minor.yy391,
        yymsp[-2].minor.yy391,
        fo->filter_expr,
        fo->over_def);
}
  yymsp[-8].minor.yy391 = yylhsminor.yy391;
        break;
      case 194: /* expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
{
    SyntaqliteFilterOver *fo = (SyntaqliteFilterOver*)
        (pCtx->astCtx->ast.data + pCtx->astCtx->ast.offsets[yymsp[0].minor.yy391]);
    yylhsminor.yy391 = synq_ast_function_call(pCtx->astCtx,
        synq_span(pCtx, yymsp[-4].minor.yy0),
        (SyntaqliteFunctionCallFlags){.star = 1},
        SYNTAQLITE_NULL_NODE,
        fo->filter_expr,
        fo->over_def);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 195: /* term ::= CTIME_KW */
{
    yylhsminor.yy391 = synq_ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_CURRENT, synq_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 196: /* expr ::= LP nexprlist COMMA expr RP */
{
    yymsp[-4].minor.yy391 = synq_ast_expr_list_append(pCtx->astCtx, yymsp[-3].minor.yy391, yymsp[-1].minor.yy391);
}
        break;
      case 197: /* expr ::= expr AND expr */
{
    yylhsminor.yy391 = synq_ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_AND, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 198: /* expr ::= expr OR expr */
{
    yylhsminor.yy391 = synq_ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_OR, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 199: /* expr ::= expr LT|GT|GE|LE expr */
{
    SyntaqliteBinaryOp op;
    switch (yymsp[-1].minor.yy0.type) {
        case SYNTAQLITE_TOKEN_LT: op = SYNTAQLITE_BINARY_OP_LT; break;
        case SYNTAQLITE_TOKEN_GT: op = SYNTAQLITE_BINARY_OP_GT; break;
        case SYNTAQLITE_TOKEN_LE: op = SYNTAQLITE_BINARY_OP_LE; break;
        default:    op = SYNTAQLITE_BINARY_OP_GE; break;
    }
    yylhsminor.yy391 = synq_ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 200: /* expr ::= expr EQ|NE expr */
{
    SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == SYNTAQLITE_TOKEN_EQ) ? SYNTAQLITE_BINARY_OP_EQ : SYNTAQLITE_BINARY_OP_NE;
    yylhsminor.yy391 = synq_ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 201: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
{
    SyntaqliteBinaryOp op;
    switch (yymsp[-1].minor.yy0.type) {
        case SYNTAQLITE_TOKEN_BITAND: op = SYNTAQLITE_BINARY_OP_BITAND; break;
        case SYNTAQLITE_TOKEN_BITOR:  op = SYNTAQLITE_BINARY_OP_BITOR; break;
        case SYNTAQLITE_TOKEN_LSHIFT: op = SYNTAQLITE_BINARY_OP_LSHIFT; break;
        default:        op = SYNTAQLITE_BINARY_OP_RSHIFT; break;
    }
    yylhsminor.yy391 = synq_ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 202: /* expr ::= expr PLUS|MINUS expr */
{
    SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == SYNTAQLITE_TOKEN_PLUS) ? SYNTAQLITE_BINARY_OP_PLUS : SYNTAQLITE_BINARY_OP_MINUS;
    yylhsminor.yy391 = synq_ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 203: /* expr ::= expr STAR|SLASH|REM expr */
{
    SyntaqliteBinaryOp op;
    switch (yymsp[-1].minor.yy0.type) {
        case SYNTAQLITE_TOKEN_STAR:  op = SYNTAQLITE_BINARY_OP_STAR; break;
        case SYNTAQLITE_TOKEN_SLASH: op = SYNTAQLITE_BINARY_OP_SLASH; break;
        default:       op = SYNTAQLITE_BINARY_OP_REM; break;
    }
    yylhsminor.yy391 = synq_ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 204: /* expr ::= expr CONCAT expr */
{
    yylhsminor.yy391 = synq_ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_CONCAT, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 205: /* likeop ::= NOT LIKE_KW|MATCH */
{
    yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;
    yymsp[-1].minor.yy0.n |= 0x80000000;
}
        break;
      case 206: /* expr ::= expr likeop expr */
{
    SyntaqliteBool negated = (yymsp[-1].minor.yy0.n & 0x80000000) ? SYNTAQLITE_BOOL_TRUE : SYNTAQLITE_BOOL_FALSE;
    yylhsminor.yy391 = synq_ast_like_expr(pCtx->astCtx, negated, yymsp[-2].minor.yy391, yymsp[0].minor.yy391, SYNTAQLITE_NULL_NODE);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 207: /* expr ::= expr likeop expr ESCAPE expr */
{
    SyntaqliteBool negated = (yymsp[-3].minor.yy0.n & 0x80000000) ? SYNTAQLITE_BOOL_TRUE : SYNTAQLITE_BOOL_FALSE;
    yylhsminor.yy391 = synq_ast_like_expr(pCtx->astCtx, negated, yymsp[-4].minor.yy391, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 208: /* expr ::= expr ISNULL|NOTNULL */
{
    SyntaqliteIsOp op = (yymsp[0].minor.yy0.type == SYNTAQLITE_TOKEN_ISNULL) ? SYNTAQLITE_IS_OP_ISNULL : SYNTAQLITE_IS_OP_NOTNULL;
    yylhsminor.yy391 = synq_ast_is_expr(pCtx->astCtx, op, yymsp[-1].minor.yy391, SYNTAQLITE_NULL_NODE);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 209: /* expr ::= expr NOT NULL */
{
    yylhsminor.yy391 = synq_ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_NOTNULL, yymsp[-2].minor.yy391, SYNTAQLITE_NULL_NODE);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 210: /* expr ::= expr IS expr */
{
    yylhsminor.yy391 = synq_ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 211: /* expr ::= expr IS NOT expr */
{
    yylhsminor.yy391 = synq_ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_NOT, yymsp[-3].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-3].minor.yy391 = yylhsminor.yy391;
        break;
      case 212: /* expr ::= expr IS NOT DISTINCT FROM expr */
{
    yylhsminor.yy391 = synq_ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_NOT_DISTINCT, yymsp[-5].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-5].minor.yy391 = yylhsminor.yy391;
        break;
      case 213: /* expr ::= expr IS DISTINCT FROM expr */
{
    yylhsminor.yy391 = synq_ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_DISTINCT, yymsp[-4].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 214: /* expr ::= NOT expr */
{
    yymsp[-1].minor.yy391 = synq_ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_NOT, yymsp[0].minor.yy391);
}
        break;
      case 215: /* expr ::= BITNOT expr */
{
    yymsp[-1].minor.yy391 = synq_ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_BITNOT, yymsp[0].minor.yy391);
}
        break;
      case 216: /* expr ::= PLUS|MINUS expr */
{
    SyntaqliteUnaryOp op = (yymsp[-1].minor.yy0.type == SYNTAQLITE_TOKEN_MINUS) ? SYNTAQLITE_UNARY_OP_MINUS : SYNTAQLITE_UNARY_OP_PLUS;
    yylhsminor.yy391 = synq_ast_unary_expr(pCtx->astCtx, op, yymsp[0].minor.yy391);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 217: /* expr ::= expr PTR expr */
{
    yylhsminor.yy391 = synq_ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_PTR, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 220: /* expr ::= expr between_op expr AND expr */
{
    yylhsminor.yy391 = synq_ast_between_expr(pCtx->astCtx, (SyntaqliteBool)yymsp[-3].minor.yy391, yymsp[-4].minor.yy391, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 221: /* in_op ::= IN */
{ yymsp[0].minor.yy144 = 0; }
        break;
      case 223: /* expr ::= expr in_op LP exprlist RP */
{
    yymsp[-4].minor.yy391 = synq_ast_in_expr(pCtx->astCtx, (SyntaqliteBool)yymsp[-3].minor.yy144, yymsp[-4].minor.yy391, yymsp[-1].minor.yy391);
}
        break;
      case 224: /* expr ::= LP select RP */
{
    yymsp[-2].minor.yy391 = synq_ast_subquery_expr(pCtx->astCtx, yymsp[-1].minor.yy391);
}
        break;
      case 225: /* expr ::= expr in_op LP select RP */
{
    uint32_t sub = synq_ast_subquery_expr(pCtx->astCtx, yymsp[-1].minor.yy391);
    yymsp[-4].minor.yy391 = synq_ast_in_expr(pCtx->astCtx, (SyntaqliteBool)yymsp[-3].minor.yy144, yymsp[-4].minor.yy391, sub);
}
        break;
      case 226: /* expr ::= expr in_op nm dbnm paren_exprlist */
{
    // Table-valued function IN expression - stub for now
    (void)yymsp[-2].minor.yy0; (void)yymsp[-1].minor.yy0; (void)yymsp[0].minor.yy391;
    yymsp[-4].minor.yy391 = synq_ast_in_expr(pCtx->astCtx, (SyntaqliteBool)yymsp[-3].minor.yy144, yymsp[-4].minor.yy391, SYNTAQLITE_NULL_NODE);
}
        break;
      case 227: /* expr ::= EXISTS LP select RP */
{
    yymsp[-3].minor.yy391 = synq_ast_exists_expr(pCtx->astCtx, yymsp[-1].minor.yy391);
}
        break;
      case 228: /* expr ::= CASE case_operand case_exprlist case_else END */
{
    yymsp[-4].minor.yy391 = synq_ast_case_expr(pCtx->astCtx, yymsp[-3].minor.yy391, yymsp[-1].minor.yy391, yymsp[-2].minor.yy391);
}
        break;
      case 229: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
{
    uint32_t w = synq_ast_case_when(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
    yylhsminor.yy391 = synq_ast_case_when_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, w);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 230: /* case_exprlist ::= WHEN expr THEN expr */
{
    uint32_t w = synq_ast_case_when(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
    yymsp[-3].minor.yy391 = synq_ast_case_when_list(pCtx->astCtx, w);
}
        break;
      case 235: /* nexprlist ::= nexprlist COMMA expr */
{
    yylhsminor.yy391 = synq_ast_expr_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 236: /* nexprlist ::= expr */
{
    yylhsminor.yy391 = synq_ast_expr_list(pCtx->astCtx, yymsp[0].minor.yy391);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 237: /* paren_exprlist ::= */
{ yymsp[1].minor.yy391 = SYNTAQLITE_NULL_NODE; }
        break;
      case 238: /* paren_exprlist ::= LP exprlist RP */
{ yymsp[-2].minor.yy391 = yymsp[-1].minor.yy391; }
        break;
      case 239: /* cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
{
    SyntaqliteSourceSpan idx_name = yymsp[-6].minor.yy0.z ? synq_span(pCtx, yymsp[-6].minor.yy0) : synq_span(pCtx, yymsp[-7].minor.yy0);
    SyntaqliteSourceSpan idx_schema = yymsp[-6].minor.yy0.z ? synq_span(pCtx, yymsp[-7].minor.yy0) : SYNQ_NO_SPAN;
    yymsp[-11].minor.yy391 = synq_ast_create_index_stmt(pCtx->astCtx,
        idx_name,
        idx_schema,
        synq_span(pCtx, yymsp[-4].minor.yy0),
        (SyntaqliteBool)yymsp[-10].minor.yy144,
        (SyntaqliteBool)yymsp[-8].minor.yy144,
        yymsp[-2].minor.yy391,
        yymsp[0].minor.yy391);
}
        break;
      case 244: /* eidlist ::= eidlist COMMA nm collate sortorder */
{
    (void)yymsp[-1].minor.yy144; (void)yymsp[0].minor.yy391;
    uint32_t col = synq_ast_column_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[-2].minor.yy0),
        SYNQ_NO_SPAN,
        SYNQ_NO_SPAN);
    yymsp[-4].minor.yy391 = synq_ast_expr_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, col);
}
        break;
      case 245: /* eidlist ::= nm collate sortorder */
{
    (void)yymsp[-1].minor.yy144; (void)yymsp[0].minor.yy391;
    uint32_t col = synq_ast_column_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[-2].minor.yy0),
        SYNQ_NO_SPAN,
        SYNQ_NO_SPAN);
    yylhsminor.yy391 = synq_ast_expr_list(pCtx->astCtx, col);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 248: /* cmd ::= DROP INDEX ifexists fullname */
{
    yymsp[-3].minor.yy391 = synq_ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_INDEX, (SyntaqliteBool)yymsp[-1].minor.yy144, yymsp[0].minor.yy391);
}
        break;
      case 249: /* cmd ::= VACUUM vinto */
{
    yymsp[-1].minor.yy391 = synq_ast_vacuum_stmt(pCtx->astCtx,
        SYNQ_NO_SPAN,
        yymsp[0].minor.yy391);
}
        break;
      case 250: /* cmd ::= VACUUM nm vinto */
{
    yymsp[-2].minor.yy391 = synq_ast_vacuum_stmt(pCtx->astCtx,
        synq_span(pCtx, yymsp[-1].minor.yy0),
        yymsp[0].minor.yy391);
}
        break;
      case 253: /* cmd ::= PRAGMA nm dbnm */
{
    SyntaqliteSourceSpan name_span = yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[0].minor.yy0) : synq_span(pCtx, yymsp[-1].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[-1].minor.yy0) : SYNQ_NO_SPAN;
    yymsp[-2].minor.yy391 = synq_ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, SYNQ_NO_SPAN, SYNTAQLITE_PRAGMA_FORM_BARE);
}
        break;
      case 254: /* cmd ::= PRAGMA nm dbnm EQ nmnum */
      case 256: /* cmd ::= PRAGMA nm dbnm EQ minus_num */ yytestcase(yyruleno==256);
{
    SyntaqliteSourceSpan name_span = yymsp[-2].minor.yy0.z ? synq_span(pCtx, yymsp[-2].minor.yy0) : synq_span(pCtx, yymsp[-3].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[-2].minor.yy0.z ? synq_span(pCtx, yymsp[-3].minor.yy0) : SYNQ_NO_SPAN;
    yymsp[-4].minor.yy391 = synq_ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, synq_span(pCtx, yymsp[0].minor.yy0), SYNTAQLITE_PRAGMA_FORM_EQ);
}
        break;
      case 255: /* cmd ::= PRAGMA nm dbnm LP nmnum RP */
      case 257: /* cmd ::= PRAGMA nm dbnm LP minus_num RP */ yytestcase(yyruleno==257);
{
    SyntaqliteSourceSpan name_span = yymsp[-3].minor.yy0.z ? synq_span(pCtx, yymsp[-3].minor.yy0) : synq_span(pCtx, yymsp[-4].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[-3].minor.yy0.z ? synq_span(pCtx, yymsp[-4].minor.yy0) : SYNQ_NO_SPAN;
    yymsp[-5].minor.yy391 = synq_ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, synq_span(pCtx, yymsp[-1].minor.yy0), SYNTAQLITE_PRAGMA_FORM_CALL);
}
        break;
      case 259: /* minus_num ::= MINUS INTEGER|FLOAT */
{
    // Build a token that spans from the MINUS sign through the number
    yylhsminor.yy0.z = yymsp[-1].minor.yy0.z;
    yylhsminor.yy0.n = (int)(yymsp[0].minor.yy0.z - yymsp[-1].minor.yy0.z) + yymsp[0].minor.yy0.n;
}
  yymsp[-1].minor.yy0 = yylhsminor.yy0;
        break;
      case 260: /* cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
{
    // yymsp[-3].minor.yy391 is a partially-built CreateTriggerStmt, fill in the body
    SyntaqliteNode *trig = AST_NODE(&pCtx->astCtx->ast, yymsp[-3].minor.yy391);
    trig->create_trigger_stmt.body = yymsp[-1].minor.yy391;
    yymsp[-4].minor.yy391 = yymsp[-3].minor.yy391;
}
        break;
      case 261: /* trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
{
    SyntaqliteSourceSpan trig_name = yymsp[-6].minor.yy0.z ? synq_span(pCtx, yymsp[-6].minor.yy0) : synq_span(pCtx, yymsp[-7].minor.yy0);
    SyntaqliteSourceSpan trig_schema = yymsp[-6].minor.yy0.z ? synq_span(pCtx, yymsp[-7].minor.yy0) : SYNQ_NO_SPAN;
    yylhsminor.yy391 = synq_ast_create_trigger_stmt(pCtx->astCtx,
        trig_name,
        trig_schema,
        (SyntaqliteBool)yymsp[-10].minor.yy144,
        (SyntaqliteBool)yymsp[-8].minor.yy144,
        (SyntaqliteTriggerTiming)yymsp[-5].minor.yy144,
        yymsp[-4].minor.yy391,
        yymsp[-2].minor.yy391,
        yymsp[0].minor.yy391,
        SYNTAQLITE_NULL_NODE);  // body filled in by cmd rule
}
  yymsp[-10].minor.yy391 = yylhsminor.yy391;
        break;
      case 262: /* trigger_time ::= BEFORE|AFTER */
{
    yylhsminor.yy144 = (yymsp[0].minor.yy0.type == SYNTAQLITE_TOKEN_BEFORE) ? (int)SYNTAQLITE_TRIGGER_TIMING_BEFORE
                               : (int)SYNTAQLITE_TRIGGER_TIMING_AFTER;
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 263: /* trigger_time ::= INSTEAD OF */
{
    yymsp[-1].minor.yy144 = (int)SYNTAQLITE_TRIGGER_TIMING_INSTEAD_OF;
}
        break;
      case 264: /* trigger_time ::= */
{
    yymsp[1].minor.yy144 = (int)SYNTAQLITE_TRIGGER_TIMING_BEFORE;
}
        break;
      case 265: /* trigger_event ::= DELETE|INSERT */
{
    SyntaqliteTriggerEventType evt = (yymsp[0].minor.yy0.type == SYNTAQLITE_TOKEN_DELETE)
        ? SYNTAQLITE_TRIGGER_EVENT_TYPE_DELETE
        : SYNTAQLITE_TRIGGER_EVENT_TYPE_INSERT;
    yylhsminor.yy391 = synq_ast_trigger_event(pCtx->astCtx, evt, SYNTAQLITE_NULL_NODE);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 266: /* trigger_event ::= UPDATE */
{
    yymsp[0].minor.yy391 = synq_ast_trigger_event(pCtx->astCtx,
        SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE, SYNTAQLITE_NULL_NODE);
}
        break;
      case 267: /* trigger_event ::= UPDATE OF idlist */
{
    yymsp[-2].minor.yy391 = synq_ast_trigger_event(pCtx->astCtx,
        SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE, yymsp[0].minor.yy391);
}
        break;
      case 270: /* trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
{
    yylhsminor.yy391 = synq_ast_trigger_cmd_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[-1].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 271: /* trigger_cmd_list ::= trigger_cmd SEMI */
{
    yylhsminor.yy391 = synq_ast_trigger_cmd_list(pCtx->astCtx, yymsp[-1].minor.yy391);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 272: /* trnm ::= nm DOT nm */
{
    yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;
    // Qualified names not allowed in triggers, but grammar accepts them
}
        break;
      case 273: /* tridxby ::= INDEXED BY nm */
      case 274: /* tridxby ::= NOT INDEXED */ yytestcase(yyruleno==274);
{
    // Not allowed in triggers, but grammar accepts
}
        break;
      case 275: /* trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
{
    uint32_t tbl = synq_ast_table_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[-6].minor.yy0), SYNQ_NO_SPAN, SYNQ_NO_SPAN);
    yymsp[-8].minor.yy391 = synq_ast_update_stmt(pCtx->astCtx, (SyntaqliteConflictAction)yymsp[-7].minor.yy144, tbl, yymsp[-3].minor.yy391, yymsp[-2].minor.yy391, yymsp[-1].minor.yy391);
}
        break;
      case 276: /* trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
{
    uint32_t tbl = synq_ast_table_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[-4].minor.yy0), SYNQ_NO_SPAN, SYNQ_NO_SPAN);
    yymsp[-7].minor.yy391 = synq_ast_insert_stmt(pCtx->astCtx, (SyntaqliteConflictAction)yymsp[-6].minor.yy144, tbl, yymsp[-3].minor.yy391, yymsp[-2].minor.yy391);
}
        break;
      case 277: /* trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
{
    uint32_t tbl = synq_ast_table_ref(pCtx->astCtx,
        synq_span(pCtx, yymsp[-3].minor.yy0), SYNQ_NO_SPAN, SYNQ_NO_SPAN);
    yymsp[-5].minor.yy391 = synq_ast_delete_stmt(pCtx->astCtx, tbl, yymsp[-1].minor.yy391);
}
        break;
      case 279: /* expr ::= RAISE LP IGNORE RP */
{
    yymsp[-3].minor.yy391 = synq_ast_raise_expr(pCtx->astCtx, SYNTAQLITE_RAISE_TYPE_IGNORE, SYNTAQLITE_NULL_NODE);
}
        break;
      case 280: /* expr ::= RAISE LP raisetype COMMA expr RP */
{
    yymsp[-5].minor.yy391 = synq_ast_raise_expr(pCtx->astCtx, (SyntaqliteRaiseType)yymsp[-3].minor.yy144, yymsp[-1].minor.yy391);
}
        break;
      case 281: /* raisetype ::= ROLLBACK */
{ yymsp[0].minor.yy144 = SYNTAQLITE_RAISE_TYPE_ROLLBACK; }
        break;
      case 282: /* raisetype ::= ABORT */
{ yymsp[0].minor.yy144 = SYNTAQLITE_RAISE_TYPE_ABORT; }
        break;
      case 283: /* raisetype ::= FAIL */
{ yymsp[0].minor.yy144 = SYNTAQLITE_RAISE_TYPE_FAIL; }
        break;
      case 284: /* cmd ::= DROP TRIGGER ifexists fullname */
{
    yymsp[-3].minor.yy391 = synq_ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TRIGGER, (SyntaqliteBool)yymsp[-1].minor.yy144, yymsp[0].minor.yy391);
}
        break;
      case 285: /* cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
{
    yymsp[-5].minor.yy391 = synq_ast_attach_stmt(pCtx->astCtx, yymsp[-3].minor.yy391, yymsp[-1].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 286: /* cmd ::= DETACH database_kw_opt expr */
{
    yymsp[-2].minor.yy391 = synq_ast_detach_stmt(pCtx->astCtx, yymsp[0].minor.yy391);
}
        break;
      case 289: /* cmd ::= REINDEX */
{
    yymsp[0].minor.yy391 = synq_ast_analyze_stmt(pCtx->astCtx,
        SYNQ_NO_SPAN,
        SYNQ_NO_SPAN,
        SYNTAQLITE_ANALYZE_KIND_REINDEX);
}
        break;
      case 290: /* cmd ::= REINDEX nm dbnm */
{
    SyntaqliteSourceSpan name_span = yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[0].minor.yy0) : synq_span(pCtx, yymsp[-1].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[-1].minor.yy0) : SYNQ_NO_SPAN;
    yymsp[-2].minor.yy391 = synq_ast_analyze_stmt(pCtx->astCtx, name_span, schema_span, 1);
}
        break;
      case 291: /* cmd ::= ANALYZE */
{
    yymsp[0].minor.yy391 = synq_ast_analyze_stmt(pCtx->astCtx,
        SYNQ_NO_SPAN,
        SYNQ_NO_SPAN,
        SYNTAQLITE_ANALYZE_KIND_ANALYZE);
}
        break;
      case 292: /* cmd ::= ANALYZE nm dbnm */
{
    SyntaqliteSourceSpan name_span = yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[0].minor.yy0) : synq_span(pCtx, yymsp[-1].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[0].minor.yy0.z ? synq_span(pCtx, yymsp[-1].minor.yy0) : SYNQ_NO_SPAN;
    yymsp[-2].minor.yy391 = synq_ast_analyze_stmt(pCtx->astCtx, name_span, schema_span, SYNTAQLITE_ANALYZE_KIND_ANALYZE);
}
        break;
      case 293: /* cmd ::= ALTER TABLE fullname RENAME TO nm */
{
    yymsp[-5].minor.yy391 = synq_ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_RENAME_TABLE, yymsp[-3].minor.yy391,
        synq_span(pCtx, yymsp[0].minor.yy0),
        SYNQ_NO_SPAN);
}
        break;
      case 294: /* cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
{
    yymsp[-6].minor.yy391 = synq_ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_ADD_COLUMN, SYNTAQLITE_NULL_NODE,
        SYNQ_NO_SPAN,
        yymsp[-1].minor.yy188.name);
}
        break;
      case 295: /* cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
{
    yymsp[-5].minor.yy391 = synq_ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_DROP_COLUMN, yymsp[-3].minor.yy391,
        SYNQ_NO_SPAN,
        synq_span(pCtx, yymsp[0].minor.yy0));
}
        break;
      case 296: /* add_column_fullname ::= fullname */
{
    // Passthrough - fullname already produces a node ID but we don't need it
    // for the ADD COLUMN action since add_column_fullname is consumed by cmd
}
        break;
      case 297: /* cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
{
    yymsp[-7].minor.yy391 = synq_ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_RENAME_COLUMN, yymsp[-5].minor.yy391,
        synq_span(pCtx, yymsp[0].minor.yy0),
        synq_span(pCtx, yymsp[-2].minor.yy0));
}
        break;
      case 299: /* cmd ::= create_vtab LP vtabarglist RP */
{
    // Capture module arguments span (content between parens)
    SyntaqliteNode *vtab = AST_NODE(&pCtx->astCtx->ast, yymsp[-3].minor.yy391);
    const char *args_start = yymsp[-2].minor.yy0.z + yymsp[-2].minor.yy0.n;
    const char *args_end = yymsp[0].minor.yy0.z;
    vtab->create_virtual_table_stmt.module_args = (SyntaqliteSourceSpan){
        (uint32_t)(args_start - pCtx->zSql),
        (uint16_t)(args_end - args_start)
    };
    yylhsminor.yy391 = yymsp[-3].minor.yy391;
}
  yymsp[-3].minor.yy391 = yylhsminor.yy391;
        break;
      case 300: /* create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
{
    SyntaqliteSourceSpan tbl_name = yymsp[-2].minor.yy0.z ? synq_span(pCtx, yymsp[-2].minor.yy0) : synq_span(pCtx, yymsp[-3].minor.yy0);
    SyntaqliteSourceSpan tbl_schema = yymsp[-2].minor.yy0.z ? synq_span(pCtx, yymsp[-3].minor.yy0) : SYNQ_NO_SPAN;
    yymsp[-7].minor.yy391 = synq_ast_create_virtual_table_stmt(pCtx->astCtx,
        tbl_name,
        tbl_schema,
        synq_span(pCtx, yymsp[0].minor.yy0),
        (SyntaqliteBool)yymsp[-4].minor.yy144,
        SYNQ_NO_SPAN);  // module_args = none by default
}
        break;
      case 301: /* vtabarg ::= */
      case 381: /* returning ::= */ yytestcase(yyruleno==381);
      case 392: /* foreach_clause ::= */ yytestcase(yyruleno==392);
      case 395: /* tridxby ::= */ yytestcase(yyruleno==395);
      case 403: /* anylist ::= */ yytestcase(yyruleno==403);
{
    // empty
}
        break;
      case 302: /* vtabargtoken ::= ANY */
      case 303: /* vtabargtoken ::= lp anylist RP */ yytestcase(yyruleno==303);
      case 304: /* lp ::= LP */ yytestcase(yyruleno==304);
      case 393: /* foreach_clause ::= FOR EACH ROW */ yytestcase(yyruleno==393);
      case 400: /* vtabarglist ::= vtabarg */ yytestcase(yyruleno==400);
      case 401: /* vtabarglist ::= vtabarglist COMMA vtabarg */ yytestcase(yyruleno==401);
      case 402: /* vtabarg ::= vtabarg vtabargtoken */ yytestcase(yyruleno==402);
      case 404: /* anylist ::= anylist LP anylist RP */ yytestcase(yyruleno==404);
      case 405: /* anylist ::= anylist ANY */ yytestcase(yyruleno==405);
{
    // consumed
}
        break;
      case 305: /* with ::= WITH wqlist */
{
    yymsp[-1].minor.yy581.cte_list = yymsp[0].minor.yy391;
    yymsp[-1].minor.yy581.is_recursive = 0;
}
        break;
      case 306: /* with ::= WITH RECURSIVE wqlist */
{
    yymsp[-2].minor.yy581.cte_list = yymsp[0].minor.yy391;
    yymsp[-2].minor.yy581.is_recursive = 1;
}
        break;
      case 307: /* wqas ::= AS */
{
    yymsp[0].minor.yy144 = (int)SYNTAQLITE_MATERIALIZED_DEFAULT;
}
        break;
      case 308: /* wqas ::= AS MATERIALIZED */
{
    yymsp[-1].minor.yy144 = (int)SYNTAQLITE_MATERIALIZED_MATERIALIZED;
}
        break;
      case 309: /* wqas ::= AS NOT MATERIALIZED */
{
    yymsp[-2].minor.yy144 = (int)SYNTAQLITE_MATERIALIZED_NOT_MATERIALIZED;
}
        break;
      case 310: /* wqitem ::= withnm eidlist_opt wqas LP select RP */
{
    yylhsminor.yy391 = synq_ast_cte_definition(pCtx->astCtx, synq_span(pCtx, yymsp[-5].minor.yy0), (SyntaqliteMaterialized)yymsp[-3].minor.yy144, yymsp[-4].minor.yy391, yymsp[-1].minor.yy391);
}
  yymsp[-5].minor.yy391 = yylhsminor.yy391;
        break;
      case 311: /* withnm ::= nm */
{
    // Token passthrough - nm already produces SynqToken
}
        break;
      case 312: /* wqlist ::= wqitem */
{
    yylhsminor.yy391 = synq_ast_cte_list(pCtx->astCtx, yymsp[0].minor.yy391);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 313: /* wqlist ::= wqlist COMMA wqitem */
{
    yymsp[-2].minor.yy391 = synq_ast_cte_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 314: /* windowdefn_list ::= windowdefn_list COMMA windowdefn */
{
    yylhsminor.yy391 = synq_ast_named_window_def_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 315: /* windowdefn ::= nm AS LP window RP */
{
    yylhsminor.yy391 = synq_ast_named_window_def(pCtx->astCtx,
        synq_span(pCtx, yymsp[-4].minor.yy0),
        yymsp[-1].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 316: /* window ::= PARTITION BY nexprlist orderby_opt frame_opt */
{
    yymsp[-4].minor.yy391 = synq_ast_window_def(pCtx->astCtx,
        SYNQ_NO_SPAN,
        yymsp[-2].minor.yy391,
        yymsp[-1].minor.yy391,
        yymsp[0].minor.yy391);
}
        break;
      case 317: /* window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
{
    yylhsminor.yy391 = synq_ast_window_def(pCtx->astCtx,
        synq_span(pCtx, yymsp[-5].minor.yy0),
        yymsp[-2].minor.yy391,
        yymsp[-1].minor.yy391,
        yymsp[0].minor.yy391);
}
  yymsp[-5].minor.yy391 = yylhsminor.yy391;
        break;
      case 318: /* window ::= ORDER BY sortlist frame_opt */
{
    yymsp[-3].minor.yy391 = synq_ast_window_def(pCtx->astCtx,
        SYNQ_NO_SPAN,
        SYNTAQLITE_NULL_NODE,
        yymsp[-1].minor.yy391,
        yymsp[0].minor.yy391);
}
        break;
      case 319: /* window ::= nm ORDER BY sortlist frame_opt */
{
    yylhsminor.yy391 = synq_ast_window_def(pCtx->astCtx,
        synq_span(pCtx, yymsp[-4].minor.yy0),
        SYNTAQLITE_NULL_NODE,
        yymsp[-1].minor.yy391,
        yymsp[0].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 320: /* window ::= nm frame_opt */
{
    yylhsminor.yy391 = synq_ast_window_def(pCtx->astCtx,
        synq_span(pCtx, yymsp[-1].minor.yy0),
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE,
        yymsp[0].minor.yy391);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 322: /* frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
{
    // Single bound: start=yymsp[-1].minor.yy391, end=CURRENT ROW (implicit)
    uint32_t end_bound = synq_ast_frame_bound(pCtx->astCtx,
        SYNTAQLITE_FRAME_BOUND_TYPE_CURRENT_ROW,
        SYNTAQLITE_NULL_NODE);
    yylhsminor.yy391 = synq_ast_frame_spec(pCtx->astCtx,
        (SyntaqliteFrameType)yymsp[-2].minor.yy144,
        (SyntaqliteFrameExclude)yymsp[0].minor.yy144,
        yymsp[-1].minor.yy391,
        end_bound);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 323: /* frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
{
    yylhsminor.yy391 = synq_ast_frame_spec(pCtx->astCtx,
        (SyntaqliteFrameType)yymsp[-5].minor.yy144,
        (SyntaqliteFrameExclude)yymsp[0].minor.yy144,
        yymsp[-3].minor.yy391,
        yymsp[-1].minor.yy391);
}
  yymsp[-5].minor.yy391 = yylhsminor.yy391;
        break;
      case 324: /* range_or_rows ::= RANGE|ROWS|GROUPS */
{
    switch (yymsp[0].minor.yy0.type) {
        case SYNTAQLITE_TOKEN_RANGE:  yylhsminor.yy144 = SYNTAQLITE_FRAME_TYPE_RANGE; break;
        case SYNTAQLITE_TOKEN_ROWS:   yylhsminor.yy144 = SYNTAQLITE_FRAME_TYPE_ROWS; break;
        default:        yylhsminor.yy144 = SYNTAQLITE_FRAME_TYPE_GROUPS; break;
    }
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 326: /* frame_bound_s ::= UNBOUNDED PRECEDING */
{
    yymsp[-1].minor.yy391 = synq_ast_frame_bound(pCtx->astCtx,
        SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_PRECEDING,
        SYNTAQLITE_NULL_NODE);
}
        break;
      case 328: /* frame_bound_e ::= UNBOUNDED FOLLOWING */
{
    yymsp[-1].minor.yy391 = synq_ast_frame_bound(pCtx->astCtx,
        SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_FOLLOWING,
        SYNTAQLITE_NULL_NODE);
}
        break;
      case 329: /* frame_bound ::= expr PRECEDING|FOLLOWING */
{
    SyntaqliteFrameBoundType bt = (yymsp[0].minor.yy0.type == SYNTAQLITE_TOKEN_PRECEDING)
        ? SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_PRECEDING
        : SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_FOLLOWING;
    yylhsminor.yy391 = synq_ast_frame_bound(pCtx->astCtx, bt, yymsp[-1].minor.yy391);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 330: /* frame_bound ::= CURRENT ROW */
{
    yymsp[-1].minor.yy391 = synq_ast_frame_bound(pCtx->astCtx,
        SYNTAQLITE_FRAME_BOUND_TYPE_CURRENT_ROW,
        SYNTAQLITE_NULL_NODE);
}
        break;
      case 331: /* frame_exclude_opt ::= */
{
    yymsp[1].minor.yy144 = SYNTAQLITE_FRAME_EXCLUDE_NONE;
}
        break;
      case 333: /* frame_exclude ::= NO OTHERS */
{
    yymsp[-1].minor.yy144 = SYNTAQLITE_FRAME_EXCLUDE_NO_OTHERS;
}
        break;
      case 334: /* frame_exclude ::= CURRENT ROW */
{
    yymsp[-1].minor.yy144 = SYNTAQLITE_FRAME_EXCLUDE_CURRENT_ROW;
}
        break;
      case 335: /* frame_exclude ::= GROUP|TIES */
{
    yylhsminor.yy144 = (yymsp[0].minor.yy0.type == SYNTAQLITE_TOKEN_GROUP)
        ? SYNTAQLITE_FRAME_EXCLUDE_GROUP
        : SYNTAQLITE_FRAME_EXCLUDE_TIES;
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 337: /* filter_over ::= filter_clause over_clause */
{
    // Unpack the over_clause FilterOver to combine with filter expr
    SyntaqliteFilterOver *fo_over = (SyntaqliteFilterOver*)
        (pCtx->astCtx->ast.data + pCtx->astCtx->ast.offsets[yymsp[0].minor.yy391]);
    yylhsminor.yy391 = synq_ast_filter_over(pCtx->astCtx,
        yymsp[-1].minor.yy391,
        fo_over->over_def,
        SYNQ_NO_SPAN);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 339: /* filter_over ::= filter_clause */
{
    yylhsminor.yy391 = synq_ast_filter_over(pCtx->astCtx,
        yymsp[0].minor.yy391,
        SYNTAQLITE_NULL_NODE,
        SYNQ_NO_SPAN);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 340: /* over_clause ::= OVER LP window RP */
{
    yymsp[-3].minor.yy391 = synq_ast_filter_over(pCtx->astCtx,
        SYNTAQLITE_NULL_NODE,
        yymsp[-1].minor.yy391,
        SYNQ_NO_SPAN);
}
        break;
      case 341: /* over_clause ::= OVER nm */
{
    // Create a WindowDef with just base_window_name to represent a named window ref
    uint32_t wdef = synq_ast_window_def(pCtx->astCtx,
        synq_span(pCtx, yymsp[0].minor.yy0),
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE);
    yymsp[-1].minor.yy391 = synq_ast_filter_over(pCtx->astCtx,
        SYNTAQLITE_NULL_NODE,
        wdef,
        SYNQ_NO_SPAN);
}
        break;
      case 342: /* filter_clause ::= FILTER LP WHERE expr RP */
{
    yymsp[-4].minor.yy391 = yymsp[-1].minor.yy391;
}
        break;
      case 343: /* term ::= QNUMBER */
{
    yylhsminor.yy391 = synq_ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_QNUMBER, synq_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 344: /* input ::= cmdlist */
{
    pCtx->root = yymsp[0].minor.yy391;
}
        break;
      case 345: /* cmdlist ::= cmdlist ecmd */
{
    yymsp[-1].minor.yy391 = yymsp[0].minor.yy391;  // Just use the last command for now
}
        break;
      case 347: /* ecmd ::= SEMI */
{
    yymsp[0].minor.yy391 = SYNTAQLITE_NULL_NODE;
    pCtx->stmt_completed = 1;
}
        break;
      case 348: /* ecmd ::= cmdx SEMI */
{
    yylhsminor.yy391 = yymsp[-1].minor.yy391;
    pCtx->root = yymsp[-1].minor.yy391;
    synq_ast_list_flush(pCtx->astCtx);
    pCtx->stmt_completed = 1;
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 349: /* ecmd ::= explain cmdx SEMI */
{
    yylhsminor.yy391 = synq_ast_explain_stmt(pCtx->astCtx, (SyntaqliteExplainMode)(yymsp[-2].minor.yy144 - 1), yymsp[-1].minor.yy391);
    pCtx->root = yylhsminor.yy391;
    synq_ast_list_flush(pCtx->astCtx);
    pCtx->stmt_completed = 1;
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 351: /* trans_opt ::= TRANSACTION */
      case 353: /* savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==353);
{
    yymsp[0].minor.yy144 = 0;
}
        break;
      case 355: /* cmd ::= create_table create_table_args */
{
    // yymsp[0].minor.yy391 is either: (1) a CreateTableStmt node with columns/constraints filled in
    // or: (2) a CreateTableStmt node with as_select filled in
    // yymsp[-1].minor.yy391 has the table name/schema/temp/ifnotexists info packed as a node.
    // We need to merge yymsp[-1].minor.yy391 info into yymsp[0].minor.yy391.
    SyntaqliteNode *ct_node = AST_NODE(&pCtx->astCtx->ast, yymsp[-1].minor.yy391);
    SyntaqliteNode *args_node = AST_NODE(&pCtx->astCtx->ast, yymsp[0].minor.yy391);
    args_node->create_table_stmt.table_name = ct_node->create_table_stmt.table_name;
    args_node->create_table_stmt.schema = ct_node->create_table_stmt.schema;
    args_node->create_table_stmt.is_temp = ct_node->create_table_stmt.is_temp;
    args_node->create_table_stmt.if_not_exists = ct_node->create_table_stmt.if_not_exists;
    yylhsminor.yy391 = yymsp[0].minor.yy391;
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 356: /* table_option_set ::= table_option */
      case 374: /* defer_subclause_opt ::= defer_subclause */ yytestcase(yyruleno==374);
{
    // passthrough
}
        break;
      case 357: /* columnlist ::= columnlist COMMA columnname carglist */
{
    uint32_t col = synq_ast_column_def(pCtx->astCtx, yymsp[-1].minor.yy188.name, yymsp[-1].minor.yy188.typetoken, yymsp[0].minor.yy106.list);
    yylhsminor.yy391 = synq_ast_column_def_list_append(pCtx->astCtx, yymsp[-3].minor.yy391, col);
}
  yymsp[-3].minor.yy391 = yylhsminor.yy391;
        break;
      case 358: /* columnlist ::= columnname carglist */
{
    uint32_t col = synq_ast_column_def(pCtx->astCtx, yymsp[-1].minor.yy188.name, yymsp[-1].minor.yy188.typetoken, yymsp[0].minor.yy106.list);
    yylhsminor.yy391 = synq_ast_column_def_list(pCtx->astCtx, col);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 359: /* nm ::= ID|INDEXED|JOIN_KW */
      case 360: /* nm ::= STRING */ yytestcase(yyruleno==360);
      case 362: /* typename ::= ID|STRING */ yytestcase(yyruleno==362);
      case 379: /* as ::= ID|STRING */ yytestcase(yyruleno==379);
      case 383: /* likeop ::= LIKE_KW|MATCH */ yytestcase(yyruleno==383);
{
    yylhsminor.yy0 = yymsp[0].minor.yy0;
}
  yymsp[0].minor.yy0 = yylhsminor.yy0;
        break;
      case 361: /* typetoken ::= typename */
{
    (void)yymsp[0].minor.yy0;
}
        break;
      case 365: /* carglist ::= carglist ccons */
{
    if (yymsp[0].minor.yy462.node != SYNTAQLITE_NULL_NODE) {
        // Apply pending constraint name from the list to this node
        SyntaqliteNode *node = AST_NODE(&pCtx->astCtx->ast, yymsp[0].minor.yy462.node);
        node->column_constraint.constraint_name = yymsp[-1].minor.yy106.pending_name;
        if (yymsp[-1].minor.yy106.list == SYNTAQLITE_NULL_NODE) {
            yylhsminor.yy106.list = synq_ast_column_constraint_list(pCtx->astCtx, yymsp[0].minor.yy462.node);
        } else {
            yylhsminor.yy106.list = synq_ast_column_constraint_list_append(pCtx->astCtx, yymsp[-1].minor.yy106.list, yymsp[0].minor.yy462.node);
        }
        yylhsminor.yy106.pending_name = SYNQ_NO_SPAN;
    } else if (yymsp[0].minor.yy462.pending_name.length > 0) {
        // CONSTRAINT nm — store pending name for next constraint
        yylhsminor.yy106.list = yymsp[-1].minor.yy106.list;
        yylhsminor.yy106.pending_name = yymsp[0].minor.yy462.pending_name;
    } else {
        yylhsminor.yy106 = yymsp[-1].minor.yy106;
    }
}
  yymsp[-1].minor.yy106 = yylhsminor.yy106;
        break;
      case 366: /* carglist ::= */
{
    yymsp[1].minor.yy106.list = SYNTAQLITE_NULL_NODE;
    yymsp[1].minor.yy106.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 367: /* ccons ::= NULL onconf */
{
    yymsp[-1].minor.yy462.node = synq_ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NULL,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)yymsp[0].minor.yy144, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-1].minor.yy462.pending_name = SYNQ_NO_SPAN;
}
        break;
      case 368: /* ccons ::= GENERATED ALWAYS AS generated */
{
    yymsp[-3].minor.yy462 = yymsp[0].minor.yy462;
}
        break;
      case 369: /* ccons ::= AS generated */
{
    yymsp[-1].minor.yy462 = yymsp[0].minor.yy462;
}
        break;
      case 370: /* conslist_opt ::= COMMA conslist */
{
    yymsp[-1].minor.yy391 = yymsp[0].minor.yy106.list;
}
        break;
      case 371: /* conslist ::= conslist tconscomma tcons */
{
    // If comma separator was present, clear pending constraint name
    SyntaqliteSourceSpan pending = yymsp[-1].minor.yy144 ? SYNQ_NO_SPAN : yymsp[-2].minor.yy106.pending_name;
    if (yymsp[0].minor.yy462.node != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *node = AST_NODE(&pCtx->astCtx->ast, yymsp[0].minor.yy462.node);
        node->table_constraint.constraint_name = pending;
        if (yymsp[-2].minor.yy106.list == SYNTAQLITE_NULL_NODE) {
            yylhsminor.yy106.list = synq_ast_table_constraint_list(pCtx->astCtx, yymsp[0].minor.yy462.node);
        } else {
            yylhsminor.yy106.list = synq_ast_table_constraint_list_append(pCtx->astCtx, yymsp[-2].minor.yy106.list, yymsp[0].minor.yy462.node);
        }
        yylhsminor.yy106.pending_name = SYNQ_NO_SPAN;
    } else if (yymsp[0].minor.yy462.pending_name.length > 0) {
        yylhsminor.yy106.list = yymsp[-2].minor.yy106.list;
        yylhsminor.yy106.pending_name = yymsp[0].minor.yy462.pending_name;
    } else {
        yylhsminor.yy106 = yymsp[-2].minor.yy106;
    }
}
  yymsp[-2].minor.yy106 = yylhsminor.yy106;
        break;
      case 372: /* conslist ::= tcons */
{
    if (yymsp[0].minor.yy462.node != SYNTAQLITE_NULL_NODE) {
        yylhsminor.yy106.list = synq_ast_table_constraint_list(pCtx->astCtx, yymsp[0].minor.yy462.node);
        yylhsminor.yy106.pending_name = SYNQ_NO_SPAN;
    } else {
        yylhsminor.yy106.list = SYNTAQLITE_NULL_NODE;
        yylhsminor.yy106.pending_name = yymsp[0].minor.yy462.pending_name;
    }
}
  yymsp[0].minor.yy106 = yylhsminor.yy106;
        break;
      case 373: /* tconscomma ::= */
{ yymsp[1].minor.yy144 = 0; }
        break;
      case 375: /* resolvetype ::= raisetype */
{
    // raisetype: ROLLBACK=1, ABORT=2, FAIL=3 (SynqRaiseType enum values)
    // ConflictAction: ROLLBACK=1, ABORT=2, FAIL=3 (same values, direct passthrough)
    yylhsminor.yy144 = yymsp[0].minor.yy144;
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 378: /* sclp ::= selcollist COMMA */
{
    yylhsminor.yy391 = yymsp[-1].minor.yy391;
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 396: /* database_kw_opt ::= DATABASE */
{
    // Keyword consumed, no value needed
}
        break;
      case 397: /* database_kw_opt ::= */
{
    // Empty
}
        break;
      case 406: /* with ::= */
{
    yymsp[1].minor.yy581.cte_list = SYNTAQLITE_NULL_NODE;
    yymsp[1].minor.yy581.is_recursive = 0;
}
        break;
      case 407: /* windowdefn_list ::= windowdefn */
{
    yylhsminor.yy391 = synq_ast_named_window_def_list(pCtx->astCtx, yymsp[0].minor.yy391);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 408: /* window ::= frame_opt */
{
    yylhsminor.yy391 = synq_ast_window_def(pCtx->astCtx,
        SYNQ_NO_SPAN,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE,
        yymsp[0].minor.yy391);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      default:
        break;
/********** End reduce actions ************************************************/
  };
  assert( yyruleno<sizeof(yyRuleInfoLhs)/sizeof(yyRuleInfoLhs[0]) );
  yygoto = yyRuleInfoLhs[yyruleno];
  yysize = yyRuleInfoNRhs[yyruleno];
  yyact = yy_find_reduce_action(yymsp[yysize].stateno,(YYCODETYPE)yygoto);

  /* There are no SHIFTREDUCE actions on nonterminals because the table
  ** generator has simplified them to pure REDUCE actions. */
  assert( !(yyact>YY_MAX_SHIFT && yyact<=YY_MAX_SHIFTREDUCE) );

  /* It is not possible for a REDUCE to be followed by an error */
  assert( yyact!=YY_ERROR_ACTION );

  yymsp += yysize+1;
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
static void yy_parse_failed(
  synq_yyParser *yypParser           /* The parser */
){
  synq_sqlite3ParserARG_FETCH
  synq_sqlite3ParserCTX_FETCH
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
/************ End %parse_failure code *****************************************/
  synq_sqlite3ParserARG_STORE /* Suppress warning about unused %extra_argument variable */
  synq_sqlite3ParserCTX_STORE
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  synq_yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  synq_sqlite3ParserTOKENTYPE yyminor         /* The minor type of the error token */
){
  synq_sqlite3ParserARG_FETCH
  synq_sqlite3ParserCTX_FETCH
#define TOKEN yyminor
/************ Begin %syntax_error code ****************************************/

  (void)yymajor;
  (void)TOKEN;
  if( pCtx && pCtx->onSyntaxError ) {
    pCtx->onSyntaxError(pCtx);
  }
/************ End %syntax_error code ******************************************/
  synq_sqlite3ParserARG_STORE /* Suppress warning about unused %extra_argument variable */
  synq_sqlite3ParserCTX_STORE
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  synq_yyParser *yypParser           /* The parser */
){
  synq_sqlite3ParserARG_FETCH
  synq_sqlite3ParserCTX_FETCH
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
  assert( yypParser->yytos==yypParser->yystack );
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
/*********** End %parse_accept code *******************************************/
  synq_sqlite3ParserARG_STORE /* Suppress warning about unused %extra_argument variable */
  synq_sqlite3ParserCTX_STORE
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "synq_sqlite3ParserAlloc" which describes the current state of the parser.
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
void synq_sqlite3Parser(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  synq_sqlite3ParserTOKENTYPE yyminor       /* The value for the token */
  synq_sqlite3ParserARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  YYACTIONTYPE yyact;   /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  synq_yyParser *yypParser = (synq_yyParser*)yyp;  /* The parser */
  synq_sqlite3ParserCTX_FETCH
  synq_sqlite3ParserARG_STORE

  assert( yypParser->yytos!=0 );
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif

  yyact = yypParser->yytos->stateno;
#ifndef NDEBUG
  if( yyTraceFILE ){
    if( yyact < YY_MIN_REDUCE ){
      fprintf(yyTraceFILE,"%sInput '%s' in state %d\n",
              yyTracePrompt,yyTokenName[yymajor],yyact);
    }else{
      fprintf(yyTraceFILE,"%sInput '%s' with pending reduce %d\n",
              yyTracePrompt,yyTokenName[yymajor],yyact-YY_MIN_REDUCE);
    }
  }
#endif

  while(1){ /* Exit by "break" */
    assert( yypParser->yytos>=yypParser->yystack );
    assert( yyact==yypParser->yytos->stateno );
    yyact = yy_find_shift_action((YYCODETYPE)yymajor,yyact);
    if( yyact >= YY_MIN_REDUCE ){
      unsigned int yyruleno = yyact - YY_MIN_REDUCE; /* Reduce by this rule */
#ifndef NDEBUG
      assert( yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) );
      if( yyTraceFILE ){
        int yysize = yyRuleInfoNRhs[yyruleno];
        if( yysize ){
          fprintf(yyTraceFILE, "%sReduce %d [%s]%s, pop back to state %d.\n",
            yyTracePrompt,
            yyruleno, yyRuleName[yyruleno],
            yyruleno<YYNRULE_WITH_ACTION ? "" : " without external action",
            yypParser->yytos[yysize].stateno);
        }else{
          fprintf(yyTraceFILE, "%sReduce %d [%s]%s.\n",
            yyTracePrompt, yyruleno, yyRuleName[yyruleno],
            yyruleno<YYNRULE_WITH_ACTION ? "" : " without external action");
        }
      }
#endif /* NDEBUG */

      /* Check that the stack is large enough to grow by a single entry
      ** if the RHS of the rule is empty.  This ensures that there is room
      ** enough on the stack to push the LHS value */
      if( yyRuleInfoNRhs[yyruleno]==0 ){
#ifdef YYTRACKMAXSTACKDEPTH
        if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
          yypParser->yyhwm++;
          assert( yypParser->yyhwm ==
                  (int)(yypParser->yytos - yypParser->yystack));
        }
#endif
        if( yypParser->yytos>=yypParser->yystackEnd ){
          if( yyGrowStack(yypParser) ){
            yyStackOverflow(yypParser);
            break;
          }
        }
      }
      yyact = yy_reduce(yypParser,yyruleno,yymajor,yyminor synq_sqlite3ParserCTX_PARAM);
    }else if( yyact <= YY_MAX_SHIFTREDUCE ){
      yy_shift(yypParser,yyact,(YYCODETYPE)yymajor,yyminor);
#ifndef YYNOERRORRECOVERY
      yypParser->yyerrcnt--;
#endif
      break;
    }else if( yyact==YY_ACCEPT_ACTION ){
      yypParser->yytos--;
      yy_accept(yypParser);
      return;
    }else{
      assert( yyact == YY_ERROR_ACTION );
      yyminorunion.yy0 = yyminor;
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
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
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminor);
      }
      yymx = yypParser->yytos->major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
        yymajor = YYNOCODE;
      }else{
        while( yypParser->yytos > yypParser->yystack ){
          yyact = yy_find_reduce_action(yypParser->yytos->stateno,
                                        YYERRORSYMBOL);
          if( yyact<=YY_MAX_SHIFTREDUCE ) break;
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yytos <= yypParser->yystack || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
          yypParser->yyerrcnt = -1;
#endif
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          yy_shift(yypParser,yyact,YYERRORSYMBOL,yyminor);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
      if( yymajor==YYNOCODE ) break;
      yyact = yypParser->yytos->stateno;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor, yyminor);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      break;
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor, yyminor);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
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
  if( yyTraceFILE ){
    synq_yyStackEntry *i;
    char cDiv = '[';
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=&yypParser->yystack[1]; i<=yypParser->yytos; i++){
      fprintf(yyTraceFILE,"%c%s", cDiv, yyTokenName[i->major]);
      cDiv = ' ';
    }
    fprintf(yyTraceFILE,"]\n");
  }
#endif
  return;
}

/*
** Return the fallback token corresponding to canonical token iToken, or
** 0 if iToken has no fallback.
*/
int synq_sqlite3ParserFallback(int iToken){
#ifdef YYFALLBACK
  assert( iToken<(int)(sizeof(yyFallback)/sizeof(yyFallback[0])) );
  return yyFallback[iToken];
#else
  (void)iToken;
  return 0;
#endif
}
