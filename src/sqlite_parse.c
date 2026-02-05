/*
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
** SQLite parser for syntaqlite.
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
** Syntaqlite parser injection support.
** When SYNTAQLITE_PARSER_DATA_FILE is defined, external grammar data is used.
*/
#ifdef SYNTAQLITE_PARSER_DATA_FILE
#include SYNTAQLITE_PARSER_DATA_FILE
#define _SYNTAQLITE_EXTERNAL_PARSER 1
#endif

/************ Begin %include sections from the grammar ************************/
#include "src/syntaqlite_sqlite_defs.h"
#include "src/sqlite_tokens.h"
#include "src/ast/ast_builder.h"

#define YYNOERRORRECOVERY 1
#define YYPARSEFREENEVERNULL 1
/**************** End of %include directives **********************************/
#ifndef _SYNTAQLITE_EXTERNAL_PARSER
/* These constants specify the various numeric values for terminal symbols.
***************** Begin token definitions *************************************/
#ifndef TK_INDEX
#define TK_INDEX                           1
#define TK_ALTER                           2
#define TK_EXPLAIN                         3
#define TK_ADD                             4
#define TK_QUERY                           5
#define TK_WINDOW                          6
#define TK_PLAN                            7
#define TK_OVER                            8
#define TK_BEGIN                           9
#define TK_FILTER                         10
#define TK_TRANSACTION                    11
#define TK_COLUMN                         12
#define TK_DEFERRED                       13
#define TK_AGG_FUNCTION                   14
#define TK_IMMEDIATE                      15
#define TK_AGG_COLUMN                     16
#define TK_EXCLUSIVE                      17
#define TK_TRUEFALSE                      18
#define TK_COMMIT                         19
#define TK_FUNCTION                       20
#define TK_END                            21
#define TK_UPLUS                          22
#define TK_ROLLBACK                       23
#define TK_UMINUS                         24
#define TK_SAVEPOINT                      25
#define TK_TRUTH                          26
#define TK_RELEASE                        27
#define TK_REGISTER                       28
#define TK_TO                             29
#define TK_VECTOR                         30
#define TK_TABLE                          31
#define TK_SELECT_COLUMN                  32
#define TK_CREATE                         33
#define TK_IF_NULL_ROW                    34
#define TK_IF                             35
#define TK_ASTERISK                       36
#define TK_NOT                            37
#define TK_SPAN                           38
#define TK_EXISTS                         39
#define TK_ERROR                          40
#define TK_TEMP                           41
#define TK_QNUMBER                        42
#define TK_AS                             43
#define TK_WITHOUT                        44
#define TK_ABORT                          45
#define TK_ACTION                         46
#define TK_AFTER                          47
#define TK_ANALYZE                        48
#define TK_ASC                            49
#define TK_ATTACH                         50
#define TK_BEFORE                         51
#define TK_BY                             52
#define TK_CASCADE                        53
#define TK_CAST                           54
#define TK_CONFLICT                       55
#define TK_DATABASE                       56
#define TK_DESC                           57
#define TK_DETACH                         58
#define TK_EACH                           59
#define TK_FAIL                           60
#define TK_OR                             61
#define TK_AND                            62
#define TK_IS                             63
#define TK_ISNOT                          64
#define TK_MATCH                          65
#define TK_LIKE_KW                        66
#define TK_BETWEEN                        67
#define TK_IN                             68
#define TK_ISNULL                         69
#define TK_NOTNULL                        70
#define TK_ESCAPE                         71
#define TK_COLUMNKW                       72
#define TK_DO                             73
#define TK_FOR                            74
#define TK_IGNORE                         75
#define TK_INITIALLY                      76
#define TK_INSTEAD                        77
#define TK_NO                             78
#define TK_KEY                            79
#define TK_OF                             80
#define TK_OFFSET                         81
#define TK_PRAGMA                         82
#define TK_RAISE                          83
#define TK_RECURSIVE                      84
#define TK_REPLACE                        85
#define TK_RESTRICT                       86
#define TK_ROW                            87
#define TK_ROWS                           88
#define TK_TRIGGER                        89
#define TK_VACUUM                         90
#define TK_VIEW                           91
#define TK_VIRTUAL                        92
#define TK_WITH                           93
#define TK_NULLS                          94
#define TK_FIRST                          95
#define TK_LAST                           96
#define TK_CURRENT                        97
#define TK_FOLLOWING                      98
#define TK_PARTITION                      99
#define TK_PRECEDING                      100
#define TK_RANGE                          101
#define TK_UNBOUNDED                      102
#define TK_EXCLUDE                        103
#define TK_GROUPS                         104
#define TK_OTHERS                         105
#define TK_TIES                           106
#define TK_GENERATED                      107
#define TK_ALWAYS                         108
#define TK_MATERIALIZED                   109
#define TK_REINDEX                        110
#define TK_RENAME                         111
#define TK_CTIME_KW                       112
#define TK_ANY                            113
#define TK_COLLATE                        114
#define TK_ON                             115
#define TK_INDEXED                        116
#define TK_JOIN_KW                        117
#define TK_CONSTRAINT                     118
#define TK_DEFAULT                        119
#define TK_NULL                           120
#define TK_PRIMARY                        121
#define TK_UNIQUE                         122
#define TK_CHECK                          123
#define TK_REFERENCES                     124
#define TK_AUTOINCR                       125
#define TK_INSERT                         126
#define TK_DELETE                         127
#define TK_UPDATE                         128
#define TK_SET                            129
#define TK_DEFERRABLE                     130
#define TK_FOREIGN                        131
#define TK_DROP                           132
#define TK_UNION                          133
#define TK_ALL                            134
#define TK_EXCEPT                         135
#define TK_INTERSECT                      136
#define TK_SELECT                         137
#define TK_VALUES                         138
#define TK_DISTINCT                       139
#define TK_FROM                           140
#define TK_JOIN                           141
#define TK_USING                          142
#define TK_ORDER                          143
#define TK_GROUP                          144
#define TK_HAVING                         145
#define TK_LIMIT                          146
#define TK_WHERE                          147
#define TK_RETURNING                      148
#define TK_INTO                           149
#define TK_NOTHING                        150
#define TK_CASE                           151
#define TK_WHEN                           152
#define TK_THEN                           153
#define TK_ELSE                           154
#define TK_NE                             155
#define TK_EQ                             156
#define TK_GT                             157
#define TK_LE                             158
#define TK_LT                             159
#define TK_GE                             160
#define TK_BITAND                         161
#define TK_BITOR                          162
#define TK_LSHIFT                         163
#define TK_RSHIFT                         164
#define TK_PLUS                           165
#define TK_MINUS                          166
#define TK_STAR                           167
#define TK_SLASH                          168
#define TK_REM                            169
#define TK_CONCAT                         170
#define TK_PTR                            171
#define TK_BITNOT                         172
#define TK_ID                             173
#define TK_LP                             174
#define TK_RP                             175
#define TK_COMMA                          176
#define TK_STRING                         177
#define TK_DOT                            178
#define TK_FLOAT                          179
#define TK_BLOB                           180
#define TK_INTEGER                        181
#define TK_VARIABLE                       182
#define TK_SEMI                           183
#define TK_SPACE                          184
#define TK_COMMENT                        185
#define TK_ILLEGAL                        186
#endif
/**************** End token definitions ***************************************/
#endif /* !_SYNTAQLITE_EXTERNAL_PARSER */

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
**    syntaqlite_sqlite3ParserTOKENTYPE     is the data type used for minor type for terminal
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
**                       which is syntaqlite_sqlite3ParserTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    syntaqlite_sqlite3ParserARG_SDECL     A static variable declaration for the %extra_argument
**    syntaqlite_sqlite3ParserARG_PDECL     A parameter declaration for the %extra_argument
**    syntaqlite_sqlite3ParserARG_PARAM     Code to pass %extra_argument as a subroutine parameter
**    syntaqlite_sqlite3ParserARG_STORE     Code to store %extra_argument into yypParser
**    syntaqlite_sqlite3ParserARG_FETCH     Code to extract %extra_argument from yypParser
**    syntaqlite_sqlite3ParserCTX_*         As syntaqlite_sqlite3ParserARG_ except for %extra_context
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
#ifndef _SYNTAQLITE_EXTERNAL_PARSER
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned short int
#define YYNOCODE 323
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 113
#define syntaqlite_sqlite3ParserTOKENTYPE SyntaqliteToken
typedef union {
  int yyinit;
  syntaqlite_sqlite3ParserTOKENTYPE yy0;
  u32 yy9;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define syntaqlite_sqlite3ParserARG_SDECL
#define syntaqlite_sqlite3ParserARG_PDECL
#define syntaqlite_sqlite3ParserARG_PARAM
#define syntaqlite_sqlite3ParserARG_FETCH
#define syntaqlite_sqlite3ParserARG_STORE
#define YYREALLOC realloc
#define YYFREE free
#define YYDYNSTACK 0
#define syntaqlite_sqlite3ParserCTX_SDECL SyntaqliteParseContext *pCtx;
#define syntaqlite_sqlite3ParserCTX_PDECL ,SyntaqliteParseContext *pCtx
#define syntaqlite_sqlite3ParserCTX_PARAM ,pCtx
#define syntaqlite_sqlite3ParserCTX_FETCH SyntaqliteParseContext *pCtx=yypParser->pCtx;
#define syntaqlite_sqlite3ParserCTX_STORE yypParser->pCtx=pCtx;
#define YYFALLBACK 1
#define YYNSTATE             583
#define YYNRULE              409
#define YYNRULE_WITH_ACTION  70
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
#endif /* !_SYNTAQLITE_EXTERNAL_PARSER */
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
#define YY_ACTTAB_COUNT (2165)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */  1588,    7,  574,  462,  522,  378, 1358,  130,  127,  234,
 /*    10 */   505, 1396,  574, 1375, 1411,  574,  372,  130,  127,  234,
 /*    20 */  1396,  462,  568,  462,  565,  574,  571,  464, 1257, 1325,
 /*    30 */    51, 1553,  295,  162,  290,  284, 1323,  413,  434, 1325,
 /*    40 */    51, 1259, 1325,   19,  534, 1361,  565,  299,  571,  865,
 /*    50 */   264,  230, 1325,   60,  574,  499,  421,  866,  130,  127,
 /*    60 */   234,  137,  139,   91,   48, 1229, 1229, 1077, 1080, 1070,
 /*    70 */  1070,  290, 1360, 1323,  290,  413, 1323,  290,  233, 1323,
 /*    80 */   539, 1325,   51,  565,  574,  571,  565,  535,  571,  565,
 /*    90 */  1053,  571,  568,  568,  516,  413,  499,  254,   44,  137,
 /*   100 */   139,   91,  267, 1229, 1229, 1077, 1080, 1070, 1070,  370,
 /*   110 */   434, 1325,   84,  303,  452,  130,  127,  234, 1632,  137,
 /*   120 */   139,   91,  574, 1229, 1229, 1077, 1080, 1070, 1070,  376,
 /*   130 */   376,  359,  301,  466,   48,  372, 1185,  377, 1185,  140,
 /*   140 */   377, 1552,  321,  575, 1346,  308,  556,  490,  361, 1325,
 /*   150 */    84,  420,  452,  521,  523,  135,  135,  136,  136,  136,
 /*   160 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   170 */   131,  128,  452,  304,  290,  290, 1323, 1323,  452,  547,
 /*   180 */  1325,   22,  547,  574,  556,  501,  565,  565,  571,  571,
 /*   190 */   452, 1158,  555,  135,  135,  136,  136,  136,  136,  134,
 /*   200 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   210 */  1325,   82,  452,  135,  135,  136,  136,  136,  136,  134,
 /*   220 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   230 */  1158,  413, 1226,   10,  110, 1399, 1226,  134,  134,  134,
 /*   240 */   134,  133,  133,  132,  132,  132,  131,  128,  452,  321,
 /*   250 */   575,  413, 1160, 1657, 1657,  137,  139,   91,  574, 1229,
 /*   260 */  1229, 1077, 1080, 1070, 1070,  132,  132,  132,  131,  128,
 /*   270 */   383,  499, 1137,   97,  212,  137,  139,   91,  379, 1229,
 /*   280 */  1229, 1077, 1080, 1070, 1070, 1325,  145, 1348,  158,  413,
 /*   290 */   326, 1160, 1658, 1658, 1138,  398, 1399,   45,  505,  133,
 /*   300 */   133,  132,  132,  132,  131,  128,    6, 1348,  452, 1139,
 /*   310 */  1180,  413, 1211,  137,  139,   91, 1211, 1229, 1229, 1077,
 /*   320 */  1080, 1070, 1070, 1180,  990, 1199, 1180,  573,  452,  909,
 /*   330 */   909,  130,  127,  234,  991,  137,  139,   91, 1401, 1229,
 /*   340 */  1229, 1077, 1080, 1070, 1070,  909,  909,  212,  307,  135,
 /*   350 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   360 */   133,  132,  132,  132,  131,  128,  452,  505,  399,  135,
 /*   370 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   380 */   133,  132,  132,  132,  131,  128,  909,  293,  452,  323,
 /*   390 */   910, 1067, 1067, 1078, 1081, 1065,  856,  478,  183, 1401,
 /*   400 */   209,  358,  909,  885, 1522,  489,  910,  135,  135,  136,
 /*   410 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*   420 */   132,  132,  131,  128,   44,  373,  413,  467,  435,  135,
 /*   430 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   440 */   133,  132,  132,  132,  131,  128, 1071,  413,  452,  954,
 /*   450 */   137,  139,   91,   98, 1229, 1229, 1077, 1080, 1070, 1070,
 /*   460 */   886,  481,  885,  885,  887,  481,  499,  574,  321,  575,
 /*   470 */   310,  137,  139,   91,  574, 1229, 1229, 1077, 1080, 1070,
 /*   480 */  1070, 1626,  293,  236,  452,  413,  461,  458,  457, 1626,
 /*   490 */   478,  477,  478,   46, 1325,   19,  456, 1522, 1524, 1522,
 /*   500 */   435, 1325,  147,  452,  131,  128,  321,  575,  294,  137,
 /*   510 */   139,   91,  349, 1229, 1229, 1077, 1080, 1070, 1070,  429,
 /*   520 */   530,  530,    7,  448,  452,  909,  909,  136,  136,  136,
 /*   530 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   540 */   131,  128,  351,  309,  135,  135,  136,  136,  136,  136,
 /*   550 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   560 */   128,  524,  452,  237,  947,  135,  135,  136,  136,  136,
 /*   570 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   580 */   131,  128,  909, 1476, 1509,  183,  910, 1233,  909,  909,
 /*   590 */   346, 1235,  516,  448,  448,  527,  219,  413,  419, 1234,
 /*   600 */   348,  231,  350,  135,  135,  136,  136,  136,  136,  134,
 /*   610 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   620 */  1609,  137,  139,   91,  931, 1229, 1229, 1077, 1080, 1070,
 /*   630 */  1070,  380, 1476,  549,  413,  909,  909,  447,  446,  427,
 /*   640 */   525,  909,  909,  236,  574,  909,  461,  458,  457,  910,
 /*   650 */   129, 1236,  548, 1236,  232,  413,  456, 1180,  137,  139,
 /*   660 */    91, 1191, 1229, 1229, 1077, 1080, 1070, 1070,  543,  212,
 /*   670 */  1180, 1325,   84, 1180,  452,  488,  510,  344, 1023,  137,
 /*   680 */   139,   91,  507, 1229, 1229, 1077, 1080, 1070, 1070,  283,
 /*   690 */   407, 1323,  909,  452, 1659,  402,  910,  382,  909, 1191,
 /*   700 */   474,  565,  910,  571,  516, 1096,  556,  540,  540,    7,
 /*   710 */   322,  452,  542, 1023,  444,  135,  135,  136,  136,  136,
 /*   720 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   730 */   131,  128,  452, 1422,  316, 1066,  136,  136,  136,  136,
 /*   740 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   750 */   128, 1516,  135,  135,  136,  136,  136,  136,  134,  134,
 /*   760 */   134,  134,  133,  133,  132,  132,  132,  131,  128, 1517,
 /*   770 */   413,  251,  881,  135,  135,  136,  136,  136,  136,  134,
 /*   780 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   790 */   413, 1392,  516, 1198,  137,  139,   91,  574, 1229, 1229,
 /*   800 */  1077, 1080, 1070, 1070,  409,   44, 1121, 1121,  513,  909,
 /*   810 */   909,  532, 1349,  574,  137,  139,   91,  107, 1229, 1229,
 /*   820 */  1077, 1080, 1070, 1070, 1325,   19,  370,  392,  413,  447,
 /*   830 */   446,  479,  557, 1475,  290, 1116, 1323,  516,  432, 1408,
 /*   840 */  1325,   84,  100, 1236,  526, 1236,  565,  452,  571,  321,
 /*   850 */   575,  302,  137,  139,   91,  302, 1229, 1229, 1077, 1080,
 /*   860 */  1070, 1070,  290,  497, 1323,  957,  909,  452,  440,  957,
 /*   870 */   910,  436,  909,  909,  565,  556,  571,  909,  909,  909,
 /*   880 */   909,  380, 1475,  558, 1404,  368,  116,  390,  135,  135,
 /*   890 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   900 */   132,  132,  132,  131,  128,  452,  253, 1199,  135,  135,
 /*   910 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   920 */   132,  132,  132,  131,  128,   14,  909,  909, 1136,  909,
 /*   930 */   198,  533,  533,  910,  909,  199,  909,    5,  910,  413,
 /*   940 */   910,  490,  361,  436,  120,  215,  135,  135,  136,  136,
 /*   950 */   136,  136,  134,  134,  134,  134,  133,  133,  132,  132,
 /*   960 */   132,  131,  128,  137,  139,   91,  413, 1229, 1229, 1077,
 /*   970 */  1080, 1070, 1070,  518,  574,  524,  516, 1065, 1118, 1379,
 /*   980 */   512,  950, 1118,  909,   42,  885,  574,  910, 1586,    7,
 /*   990 */   137,  139,   91, 1180, 1229, 1229, 1077, 1080, 1070, 1070,
 /*  1000 */   413, 1325,   84,  290,  552, 1323, 1180,  559,  511, 1180,
 /*  1010 */  1378,  406,  405, 1325,   19,  565,  452,  571,  491,  532,
 /*  1020 */    95,  909,  909,  366,  137,  126,   91,  437, 1229, 1229,
 /*  1030 */  1077, 1080, 1070, 1070,  429,  267,  528,  290,  950, 1323,
 /*  1040 */   356,  372,  886,  452,  885,  885,  887,  574,  541,  565,
 /*  1050 */   851,  571,  512,  107,  525,  850,  156,  135,  135,  136,
 /*  1060 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*  1070 */   132,  132,  131,  128, 1325,   53,  574,  452,  909,  493,
 /*  1080 */   526,  413,  910,  536,  135,  135,  136,  136,  136,  136,
 /*  1090 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*  1100 */   128,  305, 1474, 1325,   84,  213,  139,   91,  574, 1229,
 /*  1110 */  1229, 1077, 1080, 1070, 1070,  413,  909,  909,  135,  135,
 /*  1120 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*  1130 */   132,  132,  132,  131,  128, 1325,   19,  241,  317,  574,
 /*  1140 */  1582,   91,  372, 1229, 1229, 1077, 1080, 1070, 1070,  442,
 /*  1150 */   380, 1474,  429, 1407,  576,  441, 1002, 1002,  452, 1581,
 /*  1160 */   372,  909,  909,  909,  909, 1226, 1325,   19,  919, 1226,
 /*  1170 */   920, 1376,  921,  909,    3, 1351,  418,  910,    6,  124,
 /*  1180 */   207,  566, 1377,  290, 1199, 1323, 1025,  417, 1024,  574,
 /*  1190 */    50,  331,  452, 1587,    7,  565,  569,  571, 1357,  135,
 /*  1200 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*  1210 */   133,  132,  132,  132,  131,  128, 1325,   19,  909,  384,
 /*  1220 */   909,  375,  910,  214,  910,  562, 1250,  441,  909,  909,
 /*  1230 */   208,  567,  418,  135,  135,  136,  136,  136,  136,  134,
 /*  1240 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*  1250 */   124, 1563,  566,  574, 1065, 1199,  574,  372,  450,  450,
 /*  1260 */    50,  372,  885,  108,  372,  943,  161,  569, 1565,  943,
 /*  1270 */   574, 1585,    7,  574,  124,  574,  566,  506,  159, 1199,
 /*  1280 */  1325,   19,  865, 1325,   84,  909,  475,  401, 1249,  910,
 /*  1290 */   866,  569,  228,   34,  550, 1561,  562, 1325,   84, 1250,
 /*  1300 */  1325,   84, 1325,   84,    9,   47,   44,  122,  122,  855,
 /*  1310 */   288,  230,   49,  453,  123,  450,    4,  574,  318,  572,
 /*  1320 */   562,  885,  885,  887, 1058, 1065,  291,  247, 1323,  450,
 /*  1330 */   450,  851,  445,  885,  545,  449,  850,  451,  565,  544,
 /*  1340 */   571,  241,  232, 1180, 1325,   54, 1584,    7,  970, 1065,
 /*  1350 */   321,  575,  426,  450,  450,  336, 1180,  885,  327, 1180,
 /*  1360 */   401, 1159,  422,  185,   34,  335, 1023,  124,  298,  566,
 /*  1370 */  1244,  574, 1199,  470,  961,   47,  372,  574,  122,  122,
 /*  1380 */   574,  971,   49,  372,  569,  123,  450,    4,   34,  338,
 /*  1390 */   572,  417,  885,  885,  887, 1058, 1631,  979, 1325,   55,
 /*  1400 */   286, 1023,  122,  122, 1325,   56,  330, 1325,   66,  123,
 /*  1410 */   450,    4,  480,  562,  572,  551,  885,  885,  887, 1058,
 /*  1420 */  1053,  574,  469, 1420,  424,  113,  121,  545,  118,  423,
 /*  1430 */  1137,  113,  546,  243,  280,  329,  472,  332,  471,  242,
 /*  1440 */   482,  574, 1065,  503,  483,  330,  450,  450, 1325,   67,
 /*  1450 */   885,  412, 1138, 1103,  124,  412,  566, 1103,  574, 1199,
 /*  1460 */   339,  359,  485,  347,  574,  495,  492, 1139, 1325,   21,
 /*  1470 */   340,  569,  240,  239,  238,  343,  496,  412,  320,  454,
 /*  1480 */   263,   34,  561,  324,  113, 1325,   57,  412,  574,  412,
 /*  1490 */   574, 1325,   68,  574, 1434,  122,  122,  574,  221,  976,
 /*  1500 */   562,  977,  123,  450,    4,  101,  218,  572,  574,  885,
 /*  1510 */   885,  887, 1058,  574,  545, 1325,   58, 1325,   69,  544,
 /*  1520 */  1325,   70,  504,  574, 1325,   71,  574,  222,  574, 1065,
 /*  1530 */   574,  551,  574,  450,  450, 1325,   72,  885,  574,  222,
 /*  1540 */  1325,   73,  574,  430,  574,  205,  574, 1155,  403,   39,
 /*  1550 */  1325,   74,  574, 1325,   75, 1325,   76, 1325,   77, 1325,
 /*  1560 */    59,  969,  968,  228,  138, 1325,   61,   38,   34, 1325,
 /*  1570 */    20, 1325,  143, 1325,  144,  912,  537,  357,  113, 1325,
 /*  1580 */    79, 1433,  122,  122, 1316,  484,  476,  582,  574,  123,
 /*  1590 */   450,    4,  574,  292,  572,  574,  885,  885,  887, 1058,
 /*  1600 */  1115,  574, 1115,  396, 1114,  396, 1114,  395,  167,  277,
 /*  1610 */   520,  393, 1055,  266, 1568, 1325,   62,  928,  574, 1325,
 /*  1620 */    80,  574, 1325,   63,  868,  869,  574,  352, 1325,   81,
 /*  1630 */   498,  266,  248, 1541,  342,  124,  574,  566,  500,  266,
 /*  1640 */  1199, 1540,  341,  508,   25, 1325,   64,  363, 1325,  152,
 /*  1650 */  1600,  574,  569, 1325,  171,   90,  574,  566,  289,  574,
 /*  1660 */  1199, 1059, 1321, 1325,   88, 1059,  250,  362,  113,  574,
 /*  1670 */  1430,  560,  569,  224,  177,  517,  574,   43, 1325,   65,
 /*  1680 */   574,  562, 1164, 1325,  146,  574, 1325,   83,  367, 1099,
 /*  1690 */   263,  371,  574, 1443,  249, 1468, 1325,  169,  574, 1269,
 /*  1700 */   574,  562, 1296, 1325,  148,  574,  389, 1325,  142, 1428,
 /*  1710 */  1065,  391, 1325,  170,  450,  450,  414,  574,  885, 1325,
 /*  1720 */   163,  321,  575,  574, 1337, 1325,  153, 1325,  151, 1336,
 /*  1730 */  1065, 1338, 1325,  149,  450,  450, 1615,  554,  885, 1028,
 /*  1740 */   266,  166,  113,  281, 1325,  150, 1006,  138, 1266,   34,
 /*  1750 */  1325,   86,  941,  160,  574,   12,  124,  553,  566,  313,
 /*  1760 */   168, 1199,  397,  122,  122, 1007,  138,  907,  314,   34,
 /*  1770 */   123,  450,    4,  569,  315,  572,  227,  885,  885,  887,
 /*  1780 */  1058, 1325,   78,  122,  122, 1618,  476,  582, 1004,  138,
 /*  1790 */   123,  450,    4,  292,  459,  572,  246,  885,  885,  887,
 /*  1800 */  1058, 1375,  562,  396,  334,  396, 1455,  395, 1450,  277,
 /*  1810 */   574,  393,  354,  300, 1445, 1460,  574,  928,  502, 1459,
 /*  1820 */   574,  410,  355,  306, 1513, 1512,  203,  360,  223, 1425,
 /*  1830 */  1426, 1065,  248,  210,  342,  450,  450, 1325,   87,  885,
 /*  1840 */   211,  563,  341, 1325,   85,  387, 1424, 1325,   52, 1423,
 /*  1850 */   400,  473, 1395, 1394, 1393,   40,   93,  463, 1628,  425,
 /*  1860 */  1386, 1630, 1629,  404, 1365,  961,  250, 1364, 1385, 1244,
 /*  1870 */    34,  333, 1363,  274,  177, 1560, 1558,   43,  187, 1241,
 /*  1880 */   100,  428, 1518, 1456,  122,  122,  220,  196,  476,  244,
 /*  1890 */    96,  123,  450,    4,  249,  292,  572,  182,  885,  885,
 /*  1900 */   887, 1058,   99,  189,  486,  396,  191,  396,  487,  395,
 /*  1910 */   192,  277,   13,  393,  193,  579,  414,  194,  256,  928,
 /*  1920 */   108,  321,  575,   15, 1464,  408,    2,  465, 1257, 1462,
 /*  1930 */  1461,  494,  295,  162,  248,  411,  342,  200, 1507,  106,
 /*  1940 */   509, 1259,  260,  262,  341,  515, 1529,  365,  282,  438,
 /*  1950 */   311,  519,  439,  369,  312,    2,  465, 1257,  268,  141,
 /*  1960 */  1415,  295,  162,  551,  269,  374,  443,  907,  250, 1276,
 /*  1970 */  1259,  290, 1591, 1323,  233,   11,  177, 1590, 1275,   43,
 /*  1980 */   381, 1605, 1497,  565,  117,  571,  319, 1261,  109,  216,
 /*  1990 */   529,  385,  386,  577,  388,  273,  249,  254, 1204, 1339,
 /*  2000 */   290,  276, 1323,  275,  278,  279,  580, 1334,  155, 1329,
 /*  2010 */   186,  296,  565,  172,  571,  415,  225, 1545,  414,  173,
 /*  2020 */   174,  226,  335,  321,  575, 1546,  254,  416,  235, 1544,
 /*  2030 */  1543,   89,  184,  466,  455,  325,   92,   23,   24,  460,
 /*  2040 */  1200,  175,  154,  328,  217,  468,  959,  972,   94,  908,
 /*  2050 */   297,  176,  337,  245, 1255,    1, 1318,  157, 1113, 1111,
 /*  2060 */   345,  188,  466,  178,  912,  252,  190,  993,  353,  255,
 /*  2070 */  1127,  195,  179,  180,  433,  431,  102,  197,  181,  103,
 /*  2080 */   104,  105,  257, 1130, 1126, 1317,  258,  164,   26,  259,
 /*  2090 */   364, 1119,  266,  201, 1238,  514,  261,  202, 1165,  229,
 /*  2100 */  1032,  265, 1026,   27, 1166,  285,   16,  531,  287,  204,
 /*  2110 */    28,  112,  138, 1171,   41,  206, 1083,   29,  111,   30,
 /*  2120 */   165,  538,   31, 1196,    8, 1182, 1186, 1184, 1190,  113,
 /*  2130 */   114,   32, 1189,   33, 1094,  115, 1087, 1084, 1082, 1001,
 /*  2140 */  1086,   18,  930,  270,  119, 1135,   35,  578,  394,  916,
 /*  2150 */    17,  564, 1060,  942,  125,   36,  581, 1322, 1256,  570,
 /*  2160 */  1256,   37, 1256,  271,  272,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */   310,  311,  187,  187,  206,  217,  214,  270,  271,  272,
 /*    10 */   187,  221,  187,  223,  238,  187,  187,  270,  271,  272,
 /*    20 */   230,  205,  211,  207,  248,  187,  250,  192,  193,  214,
 /*    30 */   215,  294,  197,  198,  236,  190,  238,   37,  187,  214,
 /*    40 */   215,  206,  214,  215,  206,  214,  248,  206,  250,   49,
 /*    50 */   252,  253,  214,  215,  187,  187,  228,   57,  270,  271,
 /*    60 */   272,   61,   62,   63,  235,   65,   66,   67,   68,   69,
 /*    70 */    70,  236,  214,  238,  236,   37,  238,  236,    8,  238,
 /*    80 */    10,  214,  215,  248,  187,  250,  248,  249,  250,  248,
 /*    90 */    85,  250,  281,  282,  187,   37,  187,  262,   93,   61,
 /*   100 */    62,   63,   43,   65,   66,   67,   68,   69,   70,  187,
 /*   110 */   259,  214,  215,  290,  114,  270,  271,  272,  227,   61,
 /*   120 */    62,   63,  187,   65,   66,   67,   68,   69,   70,  314,
 /*   130 */   315,  126,  264,  298,  305,  306,   98,  312,  100,   81,
 /*   140 */   315,  234,  137,  138,  206,  206,  249,  127,  128,  214,
 /*   150 */   215,  229,  114,  187,  257,  155,  156,  157,  158,  159,
 /*   160 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   170 */   170,  171,  114,  264,  236,  236,  238,  238,  114,  312,
 /*   180 */   214,  215,  315,  187,  249,  187,  248,  248,  250,  250,
 /*   190 */   114,  113,  257,  155,  156,  157,  158,  159,  160,  161,
 /*   200 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   210 */   214,  215,  114,  155,  156,  157,  158,  159,  160,  161,
 /*   220 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   230 */   113,   37,  173,  174,  176,  231,  177,  161,  162,  163,
 /*   240 */   164,  165,  166,  167,  168,  169,  170,  171,  114,  137,
 /*   250 */   138,   37,  174,  175,  176,   61,   62,   63,  187,   65,
 /*   260 */    66,   67,   68,   69,   70,  167,  168,  169,  170,  171,
 /*   270 */   274,  187,   23,   79,  187,   61,   62,   63,  217,   65,
 /*   280 */    66,   67,   68,   69,   70,  214,  215,  187,  176,   37,
 /*   290 */   203,  174,  175,  176,   45,  208,  292,   84,  187,  165,
 /*   300 */   166,  167,  168,  169,  170,  171,  190,  207,  114,   60,
 /*   310 */    88,   37,  173,   61,   62,   63,  177,   65,   66,   67,
 /*   320 */    68,   69,   70,  101,   75,   42,  104,  187,  114,  116,
 /*   330 */   117,  270,  271,  272,   85,   61,   62,   63,  231,   65,
 /*   340 */    66,   67,   68,   69,   70,  116,  117,  187,  264,  155,
 /*   350 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   360 */   166,  167,  168,  169,  170,  171,  114,  187,  208,  155,
 /*   370 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   380 */   166,  167,  168,  169,  170,  171,  173,  227,  114,  175,
 /*   390 */   177,   65,   66,   67,   68,  112,  167,  187,  187,  292,
 /*   400 */   284,  290,  173,  120,  187,  289,  177,  155,  156,  157,
 /*   410 */   158,  159,  160,  161,  162,  163,  164,  165,  166,  167,
 /*   420 */   168,  169,  170,  171,   93,  187,   37,  175,  187,  155,
 /*   430 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   440 */   166,  167,  168,  169,  170,  171,  120,   37,  114,  175,
 /*   450 */    61,   62,   63,   43,   65,   66,   67,   68,   69,   70,
 /*   460 */   177,  256,  179,  180,  181,  260,  187,  187,  137,  138,
 /*   470 */   290,   61,   62,   63,  187,   65,   66,   67,   68,   69,
 /*   480 */    70,  213,  322,  118,  114,   37,  121,  122,  123,  221,
 /*   490 */   280,  281,  282,   84,  214,  215,  131,  280,  281,  282,
 /*   500 */   259,  214,  215,  114,  170,  171,  137,  138,  228,   61,
 /*   510 */    62,   63,    1,   65,   66,   67,   68,   69,   70,  187,
 /*   520 */   309,  310,  311,  211,  114,  116,  117,  157,  158,  159,
 /*   530 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   540 */   170,  171,   31,  264,  155,  156,  157,  158,  159,  160,
 /*   550 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*   560 */   171,   37,  114,   44,  175,  155,  156,  157,  158,  159,
 /*   570 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   580 */   170,  171,  173,  269,    1,  187,  177,  115,  116,  117,
 /*   590 */   258,  119,  187,  281,  282,  187,  148,   37,  199,  127,
 /*   600 */    89,  187,   91,  155,  156,  157,  158,  159,  160,  161,
 /*   610 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   620 */   187,   61,   62,   63,   41,   65,   66,   67,   68,   69,
 /*   630 */    70,  317,  318,   78,   37,  116,  117,  165,  166,  234,
 /*   640 */   116,  116,  117,  118,  187,  173,  121,  122,  123,  177,
 /*   650 */    71,  179,   97,  181,  117,   37,  131,   88,   61,   62,
 /*   660 */    63,  106,   65,   66,   67,   68,   69,   70,   99,  187,
 /*   670 */   101,  214,  215,  104,  114,   92,  277,  187,  141,   61,
 /*   680 */    62,   63,  283,   65,   66,   67,   68,   69,   70,  236,
 /*   690 */   208,  238,  173,  114,  301,  302,  177,  187,  173,  144,
 /*   700 */   295,  248,  177,  250,  187,  122,  249,  309,  310,  311,
 /*   710 */   187,  114,  143,  176,  257,  155,  156,  157,  158,  159,
 /*   720 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   730 */   170,  171,  114,  255,  256,  175,  157,  158,  159,  160,
 /*   740 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*   750 */   171,  234,  155,  156,  157,  158,  159,  160,  161,  162,
 /*   760 */   163,  164,  165,  166,  167,  168,  169,  170,  171,  279,
 /*   770 */    37,   43,  175,  155,  156,  157,  158,  159,  160,  161,
 /*   780 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   790 */    37,  187,  187,  175,   61,   62,   63,  187,   65,   66,
 /*   800 */    67,   68,   69,   70,  206,   93,  126,  127,  128,  116,
 /*   810 */   117,  187,  187,  187,   61,   62,   63,  115,   65,   66,
 /*   820 */    67,   68,   69,   70,  214,  215,  187,   31,   37,  165,
 /*   830 */   166,  119,  206,  269,  236,   21,  238,  187,  228,  234,
 /*   840 */   214,  215,  149,  179,  142,  181,  248,  114,  250,  137,
 /*   850 */   138,  256,   61,   62,   63,  260,   65,   66,   67,   68,
 /*   860 */    69,   70,  236,  265,  238,  173,  173,  114,  229,  177,
 /*   870 */   177,  187,  116,  117,  248,  249,  250,  116,  117,  116,
 /*   880 */   117,  317,  318,  257,  234,   89,  153,   91,  155,  156,
 /*   890 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*   900 */   167,  168,  169,  170,  171,  114,  178,   42,  155,  156,
 /*   910 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*   920 */   167,  168,  169,  170,  171,   43,  116,  117,  175,  173,
 /*   930 */   174,  307,  308,  177,  173,  174,  173,  174,  177,   37,
 /*   940 */   177,  127,  128,  259,  153,   43,  155,  156,  157,  158,
 /*   950 */   159,  160,  161,  162,  163,  164,  165,  166,  167,  168,
 /*   960 */   169,  170,  171,   61,   62,   63,   37,   65,   66,   67,
 /*   970 */    68,   69,   70,  206,  187,   37,  187,  112,   47,  224,
 /*   980 */   187,  116,   51,  173,  174,  120,  187,  177,  310,  311,
 /*   990 */    61,   62,   63,   88,   65,   66,   67,   68,   69,   70,
 /*  1000 */    37,  214,  215,  236,   99,  238,  101,  206,   77,  104,
 /*  1010 */   224,  165,  166,  214,  215,  248,  114,  250,  128,  187,
 /*  1020 */   174,  116,  117,  234,   61,   62,   63,  228,   65,   66,
 /*  1030 */    67,   68,   69,   70,  187,   43,  249,  236,  173,  238,
 /*  1040 */   150,  187,  177,  114,  179,  180,  181,  187,  143,  248,
 /*  1050 */   134,  250,  259,  115,  116,  139,  174,  155,  156,  157,
 /*  1060 */   158,  159,  160,  161,  162,  163,  164,  165,  166,  167,
 /*  1070 */   168,  169,  170,  171,  214,  215,  187,  114,  173,  128,
 /*  1080 */   142,   37,  177,  167,  155,  156,  157,  158,  159,  160,
 /*  1090 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*  1100 */   171,  150,  269,  214,  215,  258,   62,   63,  187,   65,
 /*  1110 */    66,   67,   68,   69,   70,   37,  116,  117,  155,  156,
 /*  1120 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*  1130 */   167,  168,  169,  170,  171,  214,  215,   65,  249,  187,
 /*  1140 */   308,   63,  187,   65,   66,   67,   68,   69,   70,  228,
 /*  1150 */   317,  318,  187,  206,  133,  187,  135,  136,  114,  305,
 /*  1160 */   306,  116,  117,  116,  117,  173,  214,  215,   13,  177,
 /*  1170 */    15,  187,   17,  173,  174,  209,  210,  177,  190,   37,
 /*  1180 */   228,   39,  224,  236,   42,  238,  141,  115,  141,  187,
 /*  1190 */   235,  187,  114,  310,  311,  248,   54,  250,  187,  155,
 /*  1200 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*  1210 */   166,  167,  168,  169,  170,  171,  214,  215,  173,  245,
 /*  1220 */   173,  247,  177,  258,  177,   83,  113,  259,  116,  117,
 /*  1230 */   228,  209,  210,  155,  156,  157,  158,  159,  160,  161,
 /*  1240 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*  1250 */    37,  187,   39,  187,  112,   42,  187,  187,  116,  117,
 /*  1260 */   305,  306,  120,  147,  187,  173,  174,   54,  187,  177,
 /*  1270 */   187,  310,  311,  187,   37,  187,   39,  289,    6,   42,
 /*  1280 */   214,  215,   49,  214,  215,  173,  187,  174,  175,  177,
 /*  1290 */    57,   54,  176,  151,  228,  187,   83,  214,  215,  113,
 /*  1300 */   214,  215,  214,  215,   67,  235,   93,  165,  166,  167,
 /*  1310 */   252,  253,  235,   37,  172,  173,  174,  187,  249,  177,
 /*  1320 */    83,  179,  180,  181,  182,  112,  236,   29,  238,  116,
 /*  1330 */   117,  134,  249,  120,   97,  249,  139,  249,  248,  102,
 /*  1340 */   250,   65,  117,   88,  214,  215,  310,  311,   53,  112,
 /*  1350 */   137,  138,   37,  116,  117,  120,  101,  120,   43,  104,
 /*  1360 */   174,  175,  299,  300,  151,  130,  141,   37,  111,   39,
 /*  1370 */    72,  187,   42,   78,  125,  305,  306,  187,  165,  166,
 /*  1380 */   187,   86,  305,  306,   54,  172,  173,  174,  151,  132,
 /*  1390 */   177,  115,  179,  180,  181,  182,  175,  176,  214,  215,
 /*  1400 */   175,  176,  165,  166,  214,  215,  130,  214,  215,  172,
 /*  1410 */   173,  174,  265,   83,  177,  143,  179,  180,  181,  182,
 /*  1420 */    85,  187,  107,  254,  129,  176,  152,   97,  154,  114,
 /*  1430 */    23,  176,  102,  118,  119,  120,  121,  122,  123,  124,
 /*  1440 */   240,  187,  112,   37,  240,  130,  116,  117,  214,  215,
 /*  1450 */   120,  251,   45,  173,   37,  251,   39,  177,  187,   42,
 /*  1460 */   187,  126,  127,  128,  187,  115,  240,   60,  214,  215,
 /*  1470 */   187,   54,  126,  127,  128,  187,  240,  251,  240,  175,
 /*  1480 */   176,  151,   75,  175,  176,  214,  215,  251,  187,  251,
 /*  1490 */   187,  214,  215,  187,  187,  165,  166,  187,  148,   13,
 /*  1500 */    83,   15,  172,  173,  174,  147,  148,  177,  187,  179,
 /*  1510 */   180,  181,  182,  187,   97,  214,  215,  214,  215,  102,
 /*  1520 */   214,  215,  116,  187,  214,  215,  187,  140,  187,  112,
 /*  1530 */   187,  143,  187,  116,  117,  214,  215,  120,  187,  140,
 /*  1540 */   214,  215,  187,   73,  187,  189,  187,  175,  176,  156,
 /*  1550 */   214,  215,  187,  214,  215,  214,  215,  214,  215,  214,
 /*  1560 */   215,  119,  120,  176,  176,  214,  215,  174,  151,  214,
 /*  1570 */   215,  214,  215,  214,  215,  176,  143,  175,  176,  214,
 /*  1580 */   215,  187,  165,  166,    0,  187,    2,    3,  187,  172,
 /*  1590 */   173,  174,  187,    9,  177,  187,  179,  180,  181,  182,
 /*  1600 */   179,  187,  181,   19,  179,   21,  181,   23,  175,   25,
 /*  1610 */    37,   27,  175,  176,  187,  214,  215,   33,  187,  214,
 /*  1620 */   215,  187,  214,  215,   95,   96,  187,  187,  214,  215,
 /*  1630 */   175,  176,   48,  187,   50,   37,  187,   39,  175,  176,
 /*  1640 */    42,  187,   58,  187,  174,  214,  215,  187,  214,  215,
 /*  1650 */   316,  187,   54,  214,  215,   37,  187,   39,  174,  187,
 /*  1660 */    42,  173,  178,  214,  215,  177,   82,  175,  176,  187,
 /*  1670 */   187,  292,   54,  212,   90,  286,  187,   93,  214,  215,
 /*  1680 */   187,   83,  109,  214,  215,  187,  214,  215,  187,  175,
 /*  1690 */   176,  187,  187,  187,  110,  187,  214,  215,  187,  187,
 /*  1700 */   187,   83,  187,  214,  215,  187,  187,  214,  215,  187,
 /*  1710 */   112,  187,  214,  215,  116,  117,  132,  187,  120,  214,
 /*  1720 */   215,  137,  138,  187,  187,  214,  215,  214,  215,  187,
 /*  1730 */   112,  187,  214,  215,  116,  117,  187,  139,  120,  175,
 /*  1740 */   176,  175,  176,  285,  214,  215,  175,  176,  189,  151,
 /*  1750 */   214,  215,  175,  176,  187,  239,   37,  139,   39,  189,
 /*  1760 */   237,   42,  194,  165,  166,  175,  176,  183,  189,  151,
 /*  1770 */   172,  173,  174,   54,  189,  177,  226,  179,  180,  181,
 /*  1780 */   182,  214,  215,  165,  166,  202,    2,    3,  175,  176,
 /*  1790 */   172,  173,  174,    9,  218,  177,  296,  179,  180,  181,
 /*  1800 */   182,  223,   83,   19,  217,   21,  267,   23,  263,   25,
 /*  1810 */   187,   27,  291,  241,  263,  267,  187,   33,  291,  267,
 /*  1820 */   187,  267,  242,  242,  217,  217,  174,  241,  239,  255,
 /*  1830 */   255,  112,   48,  245,   50,  116,  117,  214,  215,  120,
 /*  1840 */   245,  276,   58,  214,  215,  241,  255,  214,  215,  255,
 /*  1850 */   219,  115,  216,  216,  216,  176,  174,  204,  216,   76,
 /*  1860 */   225,  222,  222,  219,  216,  125,   82,  218,  225,   72,
 /*  1870 */   151,  216,  216,  178,   90,  201,  201,   93,  296,   56,
 /*  1880 */   149,  201,  279,  268,  165,  166,  148,  174,    2,  296,
 /*  1890 */   293,  172,  173,  174,  110,    9,  177,   61,  179,  180,
 /*  1900 */   181,  182,  293,  232,   35,   19,  233,   21,  201,   23,
 /*  1910 */   233,   25,  266,   27,  233,   35,  132,  233,  200,   33,
 /*  1920 */   147,  137,  138,  266,  232,  242,  191,  192,  193,  268,
 /*  1930 */   268,  242,  197,  198,   48,  242,   50,  232,  242,  152,
 /*  1940 */   201,  206,  200,  200,   58,   74,  288,  287,  201,   43,
 /*  1950 */   278,  304,  114,  219,  278,  191,  192,  193,  201,  146,
 /*  1960 */   246,  197,  198,  143,  103,  245,   94,  183,   82,  261,
 /*  1970 */   206,  236,  313,  238,    8,  174,   90,  313,  261,   93,
 /*  1980 */   201,  318,  273,  248,  152,  250,  275,  246,  145,  244,
 /*  1990 */   144,  243,  242,  219,  201,  200,  110,  262,   25,  201,
 /*  2000 */   236,  196,  238,  200,  196,   11,  195,  195,  220,  195,
 /*  2010 */   300,  220,  248,  297,  250,  303,  212,  190,  132,  297,
 /*  2020 */   297,  212,  130,  137,  138,  190,  262,  303,  124,  190,
 /*  2030 */   190,  190,  174,  298,   79,  175,  174,  174,  174,   79,
 /*  2040 */   183,  176,  175,   43,  174,  108,  173,   46,  174,  183,
 /*  2050 */    79,   55,    4,   29,  319,  320,  321,   31,  175,  175,
 /*  2060 */   138,  149,  298,  129,  176,   43,  140,   39,   31,  142,
 /*  2070 */   183,  140,  129,  129,   55,   73,  156,  149,  129,  156,
 /*  2080 */   156,  156,   52,  116,  183,  321,  178,    9,  174,  115,
 /*  2090 */     1,   80,  176,   80,   87,   59,  178,  115,  109,  178,
 /*  2100 */   116,   52,  141,   52,  175,  175,  174,   43,  175,  174,
 /*  2110 */    52,  147,  176,  175,  174,  176,  175,   52,  176,   52,
 /*  2120 */   175,  174,   52,  175,   62,  100,   87,   98,   87,  176,
 /*  2130 */   140,   52,  105,   52,  175,  140,   21,  175,  175,  134,
 /*  2140 */   175,   43,   39,  174,  176,  175,  174,   37,   29,    7,
 /*  2150 */   174,  176,  175,  175,  174,  174,    5,  178,  323,  176,
 /*  2160 */   323,  174,  323,  178,  178,  323,  323,  323,  323,  323,
 /*  2170 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2180 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2190 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2200 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2210 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2220 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2230 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2240 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2250 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2260 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2270 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2280 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2290 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2300 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2310 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2320 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2330 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2340 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2350 */   323,  323,
};
#define YY_SHIFT_COUNT    (582)
#define YY_SHIFT_MIN      (0)
#define YY_SHIFT_MAX      (2151)
static const unsigned short int yy_shift_ofst[] = {
 /*     0 */  1784, 1584, 1886, 1213, 1213,  331,    5, 1237, 1330, 1417,
 /*    10 */  1719, 1719, 1719,  712,  331,  331,  331,  331,  331,    0,
 /*    20 */     0,  194,  929, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*    30 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,  472,  472,
 /*    40 */   525,  905,  905,  213,  409, 1112, 1112,  112,  112,  112,
 /*    50 */   112,   38,   58,  214,  252,  274,  389,  410,  448,  560,
 /*    60 */   597,  618,  733,  753,  791,  902,  929,  929,  929,  929,
 /*    70 */   929,  929,  929,  929,  929,  929,  929,  929,  929,  929,
 /*    80 */   929,  929,  929,  963,  929,  929, 1044, 1078, 1078, 1142,
 /*    90 */  1598, 1618, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   100 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   110 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   120 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   130 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   140 */  1719, 1719,  579,  370,  370,  370,  370,  370,  370,  370,
 /*   150 */    76,  134,   98,   98,  519, 1276, 1112, 1112, 1112, 1112,
 /*   160 */   664,  664,  583,  334,   20,   70,   70,   70,  369,   64,
 /*   170 */    64, 2165, 1315, 1315, 1315,  519,  249,  693,  756,  756,
 /*   180 */   756,  756,  249,  569, 1112,   78,  117, 1112, 1112, 1112,
 /*   190 */  1112, 1112, 1112, 1112, 1112, 1112, 1112, 1112, 1112, 1112,
 /*   200 */  1112, 1112, 1112, 1112, 1112,  938, 1112, 1255, 1255,  814,
 /*   210 */   222,  222,  139,  524,  524,  139, 1272, 2165, 2165, 2165,
 /*   220 */  2165, 2165, 2165, 2165,  865,  283,  283,  365,  761,  229,
 /*   230 */   763, 1045, 1047,  810, 1000, 1112, 1112, 1112, 1295, 1295,
 /*   240 */  1295, 1112, 1112, 1112, 1112, 1112, 1112, 1112, 1112, 1112,
 /*   250 */  1112, 1112, 1112, 1112, 1335, 1112, 1112, 1112, 1112, 1112,
 /*   260 */  1112, 1112, 1112, 1112, 1225, 1112, 1112, 1112,   59,  555,
 /*   270 */  1407, 1112, 1112, 1112, 1112, 1112, 1112, 1112, 1112, 1112,
 /*   280 */   846,  680,  931, 1021,  992,  992,  992,  992,  537,  916,
 /*   290 */  1021, 1021, 1155, 1221, 1249,  882, 1072, 1233, 1298, 1350,
 /*   300 */  1358, 1387, 1358, 1406, 1116, 1350, 1350, 1116, 1350, 1387,
 /*   310 */  1406, 1233, 1233,  702,  702,  702,  702, 1388, 1388, 1274,
 /*   320 */  1399, 1197, 1652, 1736, 1736, 1736, 1679, 1682, 1682, 1736,
 /*   330 */  1783, 1652, 1736, 1740, 1736, 1783, 1736, 1797, 1797, 1695,
 /*   340 */  1695, 1823, 1823, 1695, 1731, 1738, 1713, 1836, 1869, 1869,
 /*   350 */  1869, 1869, 1695, 1880, 1773, 1738, 1738, 1773, 1713, 1836,
 /*   360 */  1773, 1836, 1773, 1695, 1880, 1787, 1871, 1695, 1880, 1906,
 /*   370 */  1838, 1838, 1652, 1695, 1813, 1820, 1861, 1861, 1872, 1872,
 /*   380 */  1966, 1801, 1695, 1832, 1813, 1843, 1846, 1773, 1652, 1695,
 /*   390 */  1880, 1695, 1880, 1973, 1973, 1994, 1994, 1994, 2165, 2165,
 /*   400 */  2165, 2165, 2165, 2165, 2165, 2165, 2165, 2165, 2165, 2165,
 /*   410 */  2165, 2165, 2165,  326,  511, 1113, 1186, 1346, 1092,  796,
 /*   420 */  1304, 1308, 1372,  692, 1442, 1486, 1235, 1257, 1393,  728,
 /*   430 */   890,  951, 1402, 1470, 1437, 1455, 1463, 1492, 1573, 1280,
 /*   440 */  1514, 1564, 1566, 1529, 1433, 1571, 1421, 1425, 1577, 1590,
 /*   450 */  1484, 1613, 1488, 1892, 1904, 1858, 1955, 1862, 1863, 1860,
 /*   460 */  1864, 1960, 1865, 1867, 1857, 1866, 1870, 1873, 2000, 1937,
 /*   470 */  2001, 1874, 1971, 1996, 2048, 2024, 2026, 1883, 1884, 1922,
 /*   480 */  1912, 1934, 1888, 1888, 2022, 1926, 2028, 1927, 2037, 1887,
 /*   490 */  1931, 1943, 1888, 1944, 2002, 2019, 1888, 1928, 1920, 1923,
 /*   500 */  1924, 1925, 1949, 1967, 2030, 1908, 1901, 2078, 1914, 1974,
 /*   510 */  2089, 2011, 1916, 2013, 2007, 2036, 1918, 1982, 1929, 1932,
 /*   520 */  1989, 1921, 1930, 1933, 1984, 2049, 1935, 1961, 1936, 2051,
 /*   530 */  1938, 1940, 2064, 1939, 1941, 1942, 1945, 2058, 1964, 1947,
 /*   540 */  1948, 2065, 2067, 2070, 2025, 2039, 2029, 2062, 2041, 2027,
 /*   550 */  1953, 2079, 2081, 1990, 1995, 1959, 1936, 1962, 1963, 1965,
 /*   560 */  1968, 1970, 1969, 2115, 1972, 1975, 1976, 1977, 1978, 1980,
 /*   570 */  1981, 1983, 1979, 1985, 1986, 1987, 2005, 2098, 2103, 2110,
 /*   580 */  2119, 2142, 2151,
};
#define YY_REDUCE_COUNT (412)
#define YY_REDUCE_MIN   (-310)
#define YY_REDUCE_MAX   (1841)
static const short yy_reduce_ofst[] = {
 /*     0 */  1735, 1764, -165,  626, -162, -202,  598, -175, -185, -133,
 /*    10 */  -103,  -65,  457, -159,  -62,  -61,  767,  801,  947, -212,
 /*    20 */    61, -263, -155, -172,  280,  610,  799,  787,  921,  952,
 /*    30 */  1002,  889, 1066, 1069,   -4, 1083, 1086, 1088,  210,  217,
 /*    40 */   160,  211,  398, -171,  955, 1070, 1077,  453, 1090,  453,
 /*    50 */  1090, -253, -253, -253, -253, -253, -253, -253, -253, -253,
 /*    60 */  -253, -253, -253, -253, -253, -253, -253, -253, -253, -253,
 /*    70 */  -253, -253, -253, -253, -253, -253, -253, -253, -253, -253,
 /*    80 */  -253, -253, -253, -253, -253, -253, -253, -253, -253,  -34,
 /*    90 */    71,  287,  860, 1130, 1184, 1190, 1193, 1234, 1254, 1271,
 /*   100 */  1277, 1301, 1303, 1306, 1310, 1321, 1326, 1336, 1339, 1341,
 /*   110 */  1343, 1345, 1351, 1355, 1357, 1359, 1365, 1401, 1405, 1408,
 /*   120 */  1414, 1431, 1434, 1439, 1449, 1464, 1469, 1472, 1482, 1489,
 /*   130 */  1493, 1498, 1505, 1511, 1513, 1518, 1530, 1536, 1567, 1623,
 /*   140 */  1629, 1633, -253, -253, -253, -253, -253, -253, -253, -253,
 /*   150 */  -253, -253, -253, -253, -184, -210,   87,  405,  854,  624,
 /*   160 */  -189,  312,  399, -253,  116,  314,  564,  833, -224, -253,
 /*   170 */  -253, -253,  268,  268,  268,  100,    4,  490, -132,  -91,
 /*   180 */    84,  279,  107, -310,  -78,  393,  393,  482,  332,  847,
 /*   190 */   965,  -93,  517,  605,  650, -177, -149,  111,  241,  684,
 /*   200 */   180,  793,  789,  639,  968,  478,  832,  678,  883,  988,
 /*   210 */   961, 1036,  966,  205,  595, 1022,  974, 1063, 1200, 1204,
 /*   220 */  1226, 1236, 1058, 1238, -208, -169, -142, -109,   -2,  140,
 /*   230 */   238,  408,  414,  433,  510,  523,  604,  625,  755,  786,
 /*   240 */   958,  984, 1004, 1011, 1064, 1081, 1099, 1108, 1273, 1283,
 /*   250 */  1288, 1307, 1394, 1398, 1147, 1427, 1440, 1446, 1454, 1456,
 /*   260 */  1460, 1483, 1501, 1504, 1169, 1506, 1508, 1512, 1356, 1334,
 /*   270 */  1379, 1515,  140, 1519, 1522, 1524, 1537, 1542, 1544, 1549,
 /*   280 */  1461, 1389, 1458, 1523, 1559, 1570, 1579, 1585, 1169, 1516,
 /*   290 */  1523, 1523, 1568, 1550, 1576, 1583, 1578, 1587, 1500, 1539,
 /*   300 */  1545, 1572, 1551, 1521, 1580, 1548, 1552, 1581, 1554, 1586,
 /*   310 */  1527, 1607, 1608, 1574, 1575, 1591, 1594, 1588, 1595, 1565,
 /*   320 */  1604, 1589, 1631, 1636, 1637, 1638, 1653, 1639, 1640, 1642,
 /*   330 */  1635, 1644, 1648, 1649, 1655, 1643, 1656, 1582, 1593, 1674,
 /*   340 */  1675, 1597, 1609, 1680, 1603, 1615, 1646, 1671, 1673, 1677,
 /*   350 */  1681, 1684, 1707, 1718, 1683, 1661, 1662, 1689, 1657, 1692,
 /*   360 */  1693, 1705, 1696, 1739, 1742, 1658, 1660, 1747, 1743, 1647,
 /*   370 */  1672, 1676, 1734, 1757, 1714, 1720, 1659, 1664, 1708, 1717,
 /*   380 */  1663, 1709, 1779, 1711, 1741, 1745, 1748, 1750, 1774, 1793,
 /*   390 */  1795, 1798, 1803, 1805, 1808, 1811, 1812, 1814, 1716, 1722,
 /*   400 */  1788, 1712, 1724, 1710, 1791, 1804, 1809, 1723, 1827, 1835,
 /*   410 */  1839, 1840, 1841,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */  1663, 1663, 1663, 1313, 1254, 1421, 1254, 1254, 1254, 1254,
 /*    10 */  1313, 1313, 1313, 1254, 1254, 1254, 1254, 1254, 1254, 1279,
 /*    20 */  1279, 1555, 1258, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*    30 */  1254, 1254, 1254, 1254, 1504, 1254, 1254, 1254, 1254, 1254,
 /*    40 */  1254, 1589, 1589, 1254, 1254, 1254, 1254, 1574, 1573, 1254,
 /*    50 */  1254, 1254, 1288, 1254, 1254, 1254, 1254, 1254, 1447, 1314,
 /*    60 */  1315, 1254, 1254, 1254, 1254, 1254, 1554, 1556, 1519, 1454,
 /*    70 */  1453, 1452, 1451, 1537, 1439, 1292, 1286, 1290, 1314, 1501,
 /*    80 */  1502, 1500, 1642, 1254, 1315, 1289, 1303, 1302, 1310, 1254,
 /*    90 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   100 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   110 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   120 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   130 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   140 */  1254, 1254, 1480, 1487, 1486, 1485, 1491, 1484, 1481, 1305,
 /*   150 */  1304, 1306, 1312, 1307, 1347, 1397, 1254, 1254, 1254, 1254,
 /*   160 */  1254, 1254, 1344, 1308, 1258, 1301, 1473, 1300, 1254, 1488,
 /*   170 */  1309, 1311, 1621, 1620, 1562, 1254, 1254, 1520, 1254, 1254,
 /*   180 */  1254, 1254, 1254, 1589, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   190 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   200 */  1254, 1254, 1254, 1254, 1254, 1441, 1254, 1589, 1589, 1258,
 /*   210 */  1589, 1589, 1352, 1442, 1442, 1352, 1273, 1569, 1265, 1265,
 /*   220 */  1265, 1265, 1421, 1265, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   230 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   240 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1559, 1557,
 /*   250 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   260 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1270, 1254,
 /*   270 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1614,
 /*   280 */  1356, 1254, 1532, 1410, 1270, 1270, 1270, 1270, 1272, 1264,
 /*   290 */  1260, 1409, 1330, 1634, 1372, 1254, 1367, 1279, 1655, 1457,
 /*   300 */  1446, 1271, 1446, 1652, 1291, 1457, 1457, 1291, 1457, 1271,
 /*   310 */  1652, 1279, 1279, 1441, 1441, 1441, 1441, 1273, 1273, 1503,
 /*   320 */  1271, 1264, 1510, 1398, 1398, 1398, 1390, 1254, 1254, 1398,
 /*   330 */  1387, 1510, 1398, 1372, 1398, 1387, 1398, 1655, 1655, 1427,
 /*   340 */  1427, 1654, 1654, 1427, 1520, 1640, 1466, 1400, 1406, 1406,
 /*   350 */  1406, 1406, 1427, 1341, 1291, 1640, 1640, 1291, 1466, 1400,
 /*   360 */  1291, 1400, 1291, 1427, 1341, 1536, 1649, 1427, 1341, 1254,
 /*   370 */  1514, 1514, 1510, 1427, 1287, 1273, 1599, 1599, 1282, 1282,
 /*   380 */  1607, 1505, 1427, 1254, 1287, 1285, 1283, 1291, 1510, 1427,
 /*   390 */  1341, 1427, 1341, 1617, 1617, 1613, 1613, 1613, 1627, 1627,
 /*   400 */  1374, 1660, 1660, 1569, 1374, 1356, 1356, 1627, 1258, 1258,
 /*   410 */  1258, 1258, 1258, 1254, 1254, 1254, 1254, 1254, 1622, 1254,
 /*   420 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1564, 1521, 1431,
 /*   430 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1575, 1254,
 /*   440 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   450 */  1294, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   460 */  1254, 1254, 1345, 1254, 1254, 1254, 1566, 1370, 1254, 1254,
 /*   470 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   480 */  1254, 1254, 1448, 1449, 1432, 1254, 1254, 1254, 1254, 1254,
 /*   490 */  1254, 1254, 1463, 1254, 1254, 1254, 1458, 1254, 1254, 1254,
 /*   500 */  1254, 1254, 1254, 1254, 1254, 1651, 1254, 1254, 1254, 1254,
 /*   510 */  1254, 1254, 1535, 1534, 1254, 1254, 1429, 1254, 1254, 1254,
 /*   520 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1284, 1254,
 /*   530 */  1254, 1254, 1254, 1604, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   540 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   550 */  1274, 1254, 1254, 1254, 1254, 1254, 1326, 1254, 1254, 1254,
 /*   560 */  1254, 1254, 1254, 1254, 1254, 1637, 1254, 1254, 1254, 1254,
 /*   570 */  1254, 1417, 1298, 1295, 1254, 1254, 1412, 1254, 1254, 1254,
 /*   580 */  1335, 1254, 1327,
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
#ifndef _SYNTAQLITE_EXTERNAL_PARSER
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
  173,  /*      UNION => ID */
    0,  /*        ALL => nothing */
  173,  /*     EXCEPT => ID */
  173,  /*  INTERSECT => ID */
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
#endif /* !_SYNTAQLITE_EXTERNAL_PARSER */
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
struct syntaqlite_yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct syntaqlite_yyStackEntry syntaqlite_yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct syntaqlite_yyParser {
  syntaqlite_yyStackEntry *yytos;          /* Pointer to top element of the stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyhwm;                    /* High-water mark of the stack */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt;                 /* Shifts left before out of the error */
#endif
  syntaqlite_sqlite3ParserARG_SDECL                /* A place to hold %extra_argument */
  syntaqlite_sqlite3ParserCTX_SDECL                /* A place to hold %extra_context */
  syntaqlite_yyStackEntry *yystackEnd;           /* Last entry in the stack */
  syntaqlite_yyStackEntry *yystack;              /* The parser stack */
  syntaqlite_yyStackEntry yystk0[YYSTACKDEPTH];  /* Initial stack space */
};
typedef struct syntaqlite_yyParser syntaqlite_yyParser;

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
void syntaqlite_sqlite3ParserTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#if defined(YYCOVERAGE) || !defined(NDEBUG)
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
#ifndef _SYNTAQLITE_EXTERNAL_PARSER
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
  /*  188 */ "ids",
  /*  189 */ "as",
  /*  190 */ "scanpt",
  /*  191 */ "explain",
  /*  192 */ "cmdx",
  /*  193 */ "cmd",
  /*  194 */ "transtype",
  /*  195 */ "trans_opt",
  /*  196 */ "savepoint_opt",
  /*  197 */ "create_table",
  /*  198 */ "createkw",
  /*  199 */ "temp",
  /*  200 */ "ifnotexists",
  /*  201 */ "dbnm",
  /*  202 */ "create_table_args",
  /*  203 */ "columnlist",
  /*  204 */ "conslist_opt",
  /*  205 */ "table_option_set",
  /*  206 */ "select",
  /*  207 */ "table_option",
  /*  208 */ "columnname",
  /*  209 */ "typetoken",
  /*  210 */ "typename",
  /*  211 */ "signed",
  /*  212 */ "scantok",
  /*  213 */ "ccons",
  /*  214 */ "term",
  /*  215 */ "expr",
  /*  216 */ "onconf",
  /*  217 */ "sortorder",
  /*  218 */ "autoinc",
  /*  219 */ "eidlist_opt",
  /*  220 */ "refargs",
  /*  221 */ "defer_subclause",
  /*  222 */ "generated",
  /*  223 */ "refarg",
  /*  224 */ "refact",
  /*  225 */ "init_deferred_pred_opt",
  /*  226 */ "tconscomma",
  /*  227 */ "tcons",
  /*  228 */ "sortlist",
  /*  229 */ "eidlist",
  /*  230 */ "defer_subclause_opt",
  /*  231 */ "resolvetype",
  /*  232 */ "orconf",
  /*  233 */ "ifexists",
  /*  234 */ "fullname",
  /*  235 */ "wqlist",
  /*  236 */ "selectnowith",
  /*  237 */ "multiselect_op",
  /*  238 */ "oneselect",
  /*  239 */ "distinct",
  /*  240 */ "selcollist",
  /*  241 */ "from",
  /*  242 */ "where_opt",
  /*  243 */ "groupby_opt",
  /*  244 */ "having_opt",
  /*  245 */ "orderby_opt",
  /*  246 */ "limit_opt",
  /*  247 */ "window_clause",
  /*  248 */ "values",
  /*  249 */ "nexprlist",
  /*  250 */ "mvalues",
  /*  251 */ "sclp",
  /*  252 */ "seltablist",
  /*  253 */ "stl_prefix",
  /*  254 */ "joinop",
  /*  255 */ "on_using",
  /*  256 */ "indexed_by",
  /*  257 */ "exprlist",
  /*  258 */ "xfullname",
  /*  259 */ "idlist",
  /*  260 */ "indexed_opt",
  /*  261 */ "nulls",
  /*  262 */ "with",
  /*  263 */ "where_opt_ret",
  /*  264 */ "setlist",
  /*  265 */ "insert_cmd",
  /*  266 */ "idlist_opt",
  /*  267 */ "upsert",
  /*  268 */ "returning",
  /*  269 */ "filter_over",
  /*  270 */ "likeop",
  /*  271 */ "between_op",
  /*  272 */ "in_op",
  /*  273 */ "paren_exprlist",
  /*  274 */ "case_operand",
  /*  275 */ "case_exprlist",
  /*  276 */ "case_else",
  /*  277 */ "uniqueflag",
  /*  278 */ "collate",
  /*  279 */ "vinto",
  /*  280 */ "nmnum",
  /*  281 */ "minus_num",
  /*  282 */ "plus_num",
  /*  283 */ "trigger_decl",
  /*  284 */ "trigger_cmd_list",
  /*  285 */ "trigger_time",
  /*  286 */ "trigger_event",
  /*  287 */ "foreach_clause",
  /*  288 */ "when_clause",
  /*  289 */ "trigger_cmd",
  /*  290 */ "trnm",
  /*  291 */ "tridxby",
  /*  292 */ "raisetype",
  /*  293 */ "database_kw_opt",
  /*  294 */ "key_opt",
  /*  295 */ "add_column_fullname",
  /*  296 */ "kwcolumn_opt",
  /*  297 */ "carglist",
  /*  298 */ "create_vtab",
  /*  299 */ "vtabarglist",
  /*  300 */ "vtabarg",
  /*  301 */ "vtabargtoken",
  /*  302 */ "lp",
  /*  303 */ "anylist",
  /*  304 */ "wqas",
  /*  305 */ "wqitem",
  /*  306 */ "withnm",
  /*  307 */ "windowdefn_list",
  /*  308 */ "windowdefn",
  /*  309 */ "window",
  /*  310 */ "frame_opt",
  /*  311 */ "range_or_rows",
  /*  312 */ "frame_bound_s",
  /*  313 */ "frame_exclude_opt",
  /*  314 */ "frame_bound_e",
  /*  315 */ "frame_bound",
  /*  316 */ "frame_exclude",
  /*  317 */ "filter_clause",
  /*  318 */ "over_clause",
  /*  319 */ "input",
  /*  320 */ "cmdlist",
  /*  321 */ "ecmd",
  /*  322 */ "conslist",
};
#endif /* !_SYNTAQLITE_EXTERNAL_PARSER */
#endif /* defined(YYCOVERAGE) || !defined(NDEBUG) */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
#ifndef _SYNTAQLITE_EXTERNAL_PARSER
static const char *const yyRuleName[] = {
 /*   0 */ "cmdx ::= cmd",
 /*   1 */ "scanpt ::=",
 /*   2 */ "cmd ::= select",
 /*   3 */ "select ::= selectnowith",
 /*   4 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt",
 /*   5 */ "distinct ::= DISTINCT",
 /*   6 */ "distinct ::= ALL",
 /*   7 */ "distinct ::=",
 /*   8 */ "sclp ::=",
 /*   9 */ "selcollist ::= sclp scanpt expr scanpt as",
 /*  10 */ "selcollist ::= sclp scanpt STAR",
 /*  11 */ "selcollist ::= sclp scanpt nm DOT STAR",
 /*  12 */ "as ::= AS nm",
 /*  13 */ "as ::=",
 /*  14 */ "from ::=",
 /*  15 */ "from ::= FROM seltablist",
 /*  16 */ "orderby_opt ::=",
 /*  17 */ "orderby_opt ::= ORDER BY sortlist",
 /*  18 */ "sortlist ::= sortlist COMMA expr sortorder nulls",
 /*  19 */ "sortlist ::= expr sortorder nulls",
 /*  20 */ "sortorder ::= ASC",
 /*  21 */ "sortorder ::= DESC",
 /*  22 */ "sortorder ::=",
 /*  23 */ "nulls ::= NULLS FIRST",
 /*  24 */ "nulls ::= NULLS LAST",
 /*  25 */ "nulls ::=",
 /*  26 */ "groupby_opt ::=",
 /*  27 */ "groupby_opt ::= GROUP BY nexprlist",
 /*  28 */ "having_opt ::=",
 /*  29 */ "having_opt ::= HAVING expr",
 /*  30 */ "limit_opt ::=",
 /*  31 */ "limit_opt ::= LIMIT expr",
 /*  32 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /*  33 */ "limit_opt ::= LIMIT expr COMMA expr",
 /*  34 */ "where_opt ::=",
 /*  35 */ "where_opt ::= WHERE expr",
 /*  36 */ "expr ::= LP expr RP",
 /*  37 */ "expr ::= ID|INDEXED|JOIN_KW",
 /*  38 */ "expr ::= nm DOT nm",
 /*  39 */ "expr ::= nm DOT nm DOT nm",
 /*  40 */ "term ::= NULL|FLOAT|BLOB",
 /*  41 */ "term ::= STRING",
 /*  42 */ "term ::= INTEGER",
 /*  43 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP",
 /*  44 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP",
 /*  45 */ "expr ::= expr AND expr",
 /*  46 */ "expr ::= expr OR expr",
 /*  47 */ "expr ::= expr LT|GT|GE|LE expr",
 /*  48 */ "expr ::= expr EQ|NE expr",
 /*  49 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
 /*  50 */ "expr ::= expr PLUS|MINUS expr",
 /*  51 */ "expr ::= expr STAR|SLASH|REM expr",
 /*  52 */ "expr ::= expr CONCAT expr",
 /*  53 */ "expr ::= NOT expr",
 /*  54 */ "expr ::= BITNOT expr",
 /*  55 */ "expr ::= PLUS|MINUS expr",
 /*  56 */ "exprlist ::=",
 /*  57 */ "nexprlist ::= nexprlist COMMA expr",
 /*  58 */ "nexprlist ::= expr",
 /*  59 */ "input ::= cmdlist",
 /*  60 */ "cmdlist ::= cmdlist ecmd",
 /*  61 */ "cmdlist ::= ecmd",
 /*  62 */ "ecmd ::= SEMI",
 /*  63 */ "ecmd ::= cmdx SEMI",
 /*  64 */ "nm ::= ID|INDEXED|JOIN_KW",
 /*  65 */ "nm ::= STRING",
 /*  66 */ "selectnowith ::= oneselect",
 /*  67 */ "sclp ::= selcollist COMMA",
 /*  68 */ "expr ::= term",
 /*  69 */ "exprlist ::= nexprlist",
 /*  70 */ "explain ::= EXPLAIN",
 /*  71 */ "explain ::= EXPLAIN QUERY PLAN",
 /*  72 */ "cmd ::= BEGIN transtype trans_opt",
 /*  73 */ "transtype ::=",
 /*  74 */ "transtype ::= DEFERRED",
 /*  75 */ "transtype ::= IMMEDIATE",
 /*  76 */ "transtype ::= EXCLUSIVE",
 /*  77 */ "cmd ::= COMMIT|END trans_opt",
 /*  78 */ "cmd ::= ROLLBACK trans_opt",
 /*  79 */ "cmd ::= SAVEPOINT nm",
 /*  80 */ "cmd ::= RELEASE savepoint_opt nm",
 /*  81 */ "cmd ::= ROLLBACK trans_opt TO savepoint_opt nm",
 /*  82 */ "create_table ::= createkw temp TABLE ifnotexists nm dbnm",
 /*  83 */ "createkw ::= CREATE",
 /*  84 */ "ifnotexists ::=",
 /*  85 */ "ifnotexists ::= IF NOT EXISTS",
 /*  86 */ "temp ::= TEMP",
 /*  87 */ "temp ::=",
 /*  88 */ "create_table_args ::= LP columnlist conslist_opt RP table_option_set",
 /*  89 */ "create_table_args ::= AS select",
 /*  90 */ "table_option_set ::=",
 /*  91 */ "table_option_set ::= table_option_set COMMA table_option",
 /*  92 */ "table_option ::= WITHOUT nm",
 /*  93 */ "table_option ::= nm",
 /*  94 */ "columnname ::= nm typetoken",
 /*  95 */ "typetoken ::=",
 /*  96 */ "typetoken ::= typename LP signed RP",
 /*  97 */ "typetoken ::= typename LP signed COMMA signed RP",
 /*  98 */ "typename ::= typename ID|STRING",
 /*  99 */ "scantok ::=",
 /* 100 */ "ccons ::= CONSTRAINT nm",
 /* 101 */ "ccons ::= DEFAULT scantok term",
 /* 102 */ "ccons ::= DEFAULT LP expr RP",
 /* 103 */ "ccons ::= DEFAULT PLUS scantok term",
 /* 104 */ "ccons ::= DEFAULT MINUS scantok term",
 /* 105 */ "ccons ::= DEFAULT scantok ID|INDEXED",
 /* 106 */ "ccons ::= NOT NULL onconf",
 /* 107 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
 /* 108 */ "ccons ::= UNIQUE onconf",
 /* 109 */ "ccons ::= CHECK LP expr RP",
 /* 110 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
 /* 111 */ "ccons ::= defer_subclause",
 /* 112 */ "ccons ::= COLLATE ID|STRING",
 /* 113 */ "generated ::= LP expr RP",
 /* 114 */ "generated ::= LP expr RP ID",
 /* 115 */ "autoinc ::=",
 /* 116 */ "autoinc ::= AUTOINCR",
 /* 117 */ "refargs ::=",
 /* 118 */ "refargs ::= refargs refarg",
 /* 119 */ "refarg ::= MATCH nm",
 /* 120 */ "refarg ::= ON INSERT refact",
 /* 121 */ "refarg ::= ON DELETE refact",
 /* 122 */ "refarg ::= ON UPDATE refact",
 /* 123 */ "refact ::= SET NULL",
 /* 124 */ "refact ::= SET DEFAULT",
 /* 125 */ "refact ::= CASCADE",
 /* 126 */ "refact ::= RESTRICT",
 /* 127 */ "refact ::= NO ACTION",
 /* 128 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
 /* 129 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
 /* 130 */ "init_deferred_pred_opt ::=",
 /* 131 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
 /* 132 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
 /* 133 */ "conslist_opt ::=",
 /* 134 */ "tconscomma ::= COMMA",
 /* 135 */ "tcons ::= CONSTRAINT nm",
 /* 136 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf",
 /* 137 */ "tcons ::= UNIQUE LP sortlist RP onconf",
 /* 138 */ "tcons ::= CHECK LP expr RP onconf",
 /* 139 */ "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /* 140 */ "defer_subclause_opt ::=",
 /* 141 */ "onconf ::=",
 /* 142 */ "onconf ::= ON CONFLICT resolvetype",
 /* 143 */ "orconf ::=",
 /* 144 */ "orconf ::= OR resolvetype",
 /* 145 */ "resolvetype ::= IGNORE",
 /* 146 */ "resolvetype ::= REPLACE",
 /* 147 */ "cmd ::= DROP TABLE ifexists fullname",
 /* 148 */ "ifexists ::= IF EXISTS",
 /* 149 */ "ifexists ::=",
 /* 150 */ "cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select",
 /* 151 */ "cmd ::= DROP VIEW ifexists fullname",
 /* 152 */ "select ::= WITH wqlist selectnowith",
 /* 153 */ "select ::= WITH RECURSIVE wqlist selectnowith",
 /* 154 */ "selectnowith ::= selectnowith multiselect_op oneselect",
 /* 155 */ "multiselect_op ::= UNION",
 /* 156 */ "multiselect_op ::= UNION ALL",
 /* 157 */ "multiselect_op ::= EXCEPT|INTERSECT",
 /* 158 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt",
 /* 159 */ "values ::= VALUES LP nexprlist RP",
 /* 160 */ "oneselect ::= mvalues",
 /* 161 */ "mvalues ::= values COMMA LP nexprlist RP",
 /* 162 */ "mvalues ::= mvalues COMMA LP nexprlist RP",
 /* 163 */ "stl_prefix ::= seltablist joinop",
 /* 164 */ "stl_prefix ::=",
 /* 165 */ "seltablist ::= stl_prefix nm dbnm as on_using",
 /* 166 */ "seltablist ::= stl_prefix nm dbnm as indexed_by on_using",
 /* 167 */ "seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using",
 /* 168 */ "seltablist ::= stl_prefix LP select RP as on_using",
 /* 169 */ "seltablist ::= stl_prefix LP seltablist RP as on_using",
 /* 170 */ "dbnm ::=",
 /* 171 */ "dbnm ::= DOT nm",
 /* 172 */ "fullname ::= nm",
 /* 173 */ "fullname ::= nm DOT nm",
 /* 174 */ "xfullname ::= nm",
 /* 175 */ "xfullname ::= nm DOT nm",
 /* 176 */ "xfullname ::= nm DOT nm AS nm",
 /* 177 */ "xfullname ::= nm AS nm",
 /* 178 */ "joinop ::= COMMA|JOIN",
 /* 179 */ "joinop ::= JOIN_KW JOIN",
 /* 180 */ "joinop ::= JOIN_KW nm JOIN",
 /* 181 */ "joinop ::= JOIN_KW nm nm JOIN",
 /* 182 */ "on_using ::= ON expr",
 /* 183 */ "on_using ::= USING LP idlist RP",
 /* 184 */ "on_using ::=",
 /* 185 */ "indexed_opt ::=",
 /* 186 */ "indexed_by ::= INDEXED BY nm",
 /* 187 */ "indexed_by ::= NOT INDEXED",
 /* 188 */ "cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret",
 /* 189 */ "where_opt_ret ::=",
 /* 190 */ "where_opt_ret ::= WHERE expr",
 /* 191 */ "where_opt_ret ::= RETURNING selcollist",
 /* 192 */ "where_opt_ret ::= WHERE expr RETURNING selcollist",
 /* 193 */ "cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret",
 /* 194 */ "setlist ::= setlist COMMA nm EQ expr",
 /* 195 */ "setlist ::= setlist COMMA LP idlist RP EQ expr",
 /* 196 */ "setlist ::= nm EQ expr",
 /* 197 */ "setlist ::= LP idlist RP EQ expr",
 /* 198 */ "cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert",
 /* 199 */ "cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning",
 /* 200 */ "upsert ::=",
 /* 201 */ "upsert ::= RETURNING selcollist",
 /* 202 */ "upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert",
 /* 203 */ "upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert",
 /* 204 */ "upsert ::= ON CONFLICT DO NOTHING returning",
 /* 205 */ "upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning",
 /* 206 */ "returning ::= RETURNING selcollist",
 /* 207 */ "insert_cmd ::= INSERT orconf",
 /* 208 */ "insert_cmd ::= REPLACE",
 /* 209 */ "idlist_opt ::=",
 /* 210 */ "idlist_opt ::= LP idlist RP",
 /* 211 */ "idlist ::= idlist COMMA nm",
 /* 212 */ "idlist ::= nm",
 /* 213 */ "expr ::= VARIABLE",
 /* 214 */ "expr ::= expr COLLATE ID|STRING",
 /* 215 */ "expr ::= CAST LP expr AS typetoken RP",
 /* 216 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP",
 /* 217 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over",
 /* 218 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over",
 /* 219 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over",
 /* 220 */ "term ::= CTIME_KW",
 /* 221 */ "expr ::= LP nexprlist COMMA expr RP",
 /* 222 */ "likeop ::= NOT LIKE_KW|MATCH",
 /* 223 */ "expr ::= expr likeop expr",
 /* 224 */ "expr ::= expr likeop expr ESCAPE expr",
 /* 225 */ "expr ::= expr ISNULL|NOTNULL",
 /* 226 */ "expr ::= expr NOT NULL",
 /* 227 */ "expr ::= expr IS expr",
 /* 228 */ "expr ::= expr IS NOT expr",
 /* 229 */ "expr ::= expr IS NOT DISTINCT FROM expr",
 /* 230 */ "expr ::= expr IS DISTINCT FROM expr",
 /* 231 */ "expr ::= expr PTR expr",
 /* 232 */ "between_op ::= BETWEEN",
 /* 233 */ "between_op ::= NOT BETWEEN",
 /* 234 */ "expr ::= expr between_op expr AND expr",
 /* 235 */ "in_op ::= IN",
 /* 236 */ "in_op ::= NOT IN",
 /* 237 */ "expr ::= expr in_op LP exprlist RP",
 /* 238 */ "expr ::= LP select RP",
 /* 239 */ "expr ::= expr in_op LP select RP",
 /* 240 */ "expr ::= expr in_op nm dbnm paren_exprlist",
 /* 241 */ "expr ::= EXISTS LP select RP",
 /* 242 */ "expr ::= CASE case_operand case_exprlist case_else END",
 /* 243 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
 /* 244 */ "case_exprlist ::= WHEN expr THEN expr",
 /* 245 */ "case_else ::= ELSE expr",
 /* 246 */ "case_else ::=",
 /* 247 */ "case_operand ::=",
 /* 248 */ "paren_exprlist ::=",
 /* 249 */ "paren_exprlist ::= LP exprlist RP",
 /* 250 */ "cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt",
 /* 251 */ "uniqueflag ::= UNIQUE",
 /* 252 */ "uniqueflag ::=",
 /* 253 */ "eidlist_opt ::=",
 /* 254 */ "eidlist_opt ::= LP eidlist RP",
 /* 255 */ "eidlist ::= eidlist COMMA nm collate sortorder",
 /* 256 */ "eidlist ::= nm collate sortorder",
 /* 257 */ "collate ::=",
 /* 258 */ "collate ::= COLLATE ID|STRING",
 /* 259 */ "cmd ::= DROP INDEX ifexists fullname",
 /* 260 */ "cmd ::= VACUUM vinto",
 /* 261 */ "cmd ::= VACUUM nm vinto",
 /* 262 */ "vinto ::= INTO expr",
 /* 263 */ "vinto ::=",
 /* 264 */ "cmd ::= PRAGMA nm dbnm",
 /* 265 */ "cmd ::= PRAGMA nm dbnm EQ nmnum",
 /* 266 */ "cmd ::= PRAGMA nm dbnm LP nmnum RP",
 /* 267 */ "cmd ::= PRAGMA nm dbnm EQ minus_num",
 /* 268 */ "cmd ::= PRAGMA nm dbnm LP minus_num RP",
 /* 269 */ "plus_num ::= PLUS INTEGER|FLOAT",
 /* 270 */ "minus_num ::= MINUS INTEGER|FLOAT",
 /* 271 */ "cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END",
 /* 272 */ "trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause",
 /* 273 */ "trigger_time ::= BEFORE|AFTER",
 /* 274 */ "trigger_time ::= INSTEAD OF",
 /* 275 */ "trigger_time ::=",
 /* 276 */ "trigger_event ::= DELETE|INSERT",
 /* 277 */ "trigger_event ::= UPDATE",
 /* 278 */ "trigger_event ::= UPDATE OF idlist",
 /* 279 */ "when_clause ::=",
 /* 280 */ "when_clause ::= WHEN expr",
 /* 281 */ "trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI",
 /* 282 */ "trigger_cmd_list ::= trigger_cmd SEMI",
 /* 283 */ "trnm ::= nm DOT nm",
 /* 284 */ "tridxby ::= INDEXED BY nm",
 /* 285 */ "tridxby ::= NOT INDEXED",
 /* 286 */ "trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt",
 /* 287 */ "trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt",
 /* 288 */ "trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt",
 /* 289 */ "trigger_cmd ::= scanpt select scanpt",
 /* 290 */ "expr ::= RAISE LP IGNORE RP",
 /* 291 */ "expr ::= RAISE LP raisetype COMMA expr RP",
 /* 292 */ "raisetype ::= ROLLBACK",
 /* 293 */ "raisetype ::= ABORT",
 /* 294 */ "raisetype ::= FAIL",
 /* 295 */ "cmd ::= DROP TRIGGER ifexists fullname",
 /* 296 */ "cmd ::= ATTACH database_kw_opt expr AS expr key_opt",
 /* 297 */ "cmd ::= DETACH database_kw_opt expr",
 /* 298 */ "key_opt ::=",
 /* 299 */ "key_opt ::= KEY expr",
 /* 300 */ "cmd ::= REINDEX",
 /* 301 */ "cmd ::= REINDEX nm dbnm",
 /* 302 */ "cmd ::= ANALYZE",
 /* 303 */ "cmd ::= ANALYZE nm dbnm",
 /* 304 */ "cmd ::= ALTER TABLE fullname RENAME TO nm",
 /* 305 */ "cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist",
 /* 306 */ "cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm",
 /* 307 */ "add_column_fullname ::= fullname",
 /* 308 */ "cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm",
 /* 309 */ "cmd ::= create_vtab",
 /* 310 */ "cmd ::= create_vtab LP vtabarglist RP",
 /* 311 */ "create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm",
 /* 312 */ "vtabarg ::=",
 /* 313 */ "vtabargtoken ::= ANY",
 /* 314 */ "vtabargtoken ::= lp anylist RP",
 /* 315 */ "lp ::= LP",
 /* 316 */ "with ::= WITH wqlist",
 /* 317 */ "with ::= WITH RECURSIVE wqlist",
 /* 318 */ "wqas ::= AS",
 /* 319 */ "wqas ::= AS MATERIALIZED",
 /* 320 */ "wqas ::= AS NOT MATERIALIZED",
 /* 321 */ "wqitem ::= withnm eidlist_opt wqas LP select RP",
 /* 322 */ "withnm ::= nm",
 /* 323 */ "wqlist ::= wqitem",
 /* 324 */ "wqlist ::= wqlist COMMA wqitem",
 /* 325 */ "windowdefn_list ::= windowdefn_list COMMA windowdefn",
 /* 326 */ "windowdefn ::= nm AS LP window RP",
 /* 327 */ "window ::= PARTITION BY nexprlist orderby_opt frame_opt",
 /* 328 */ "window ::= nm PARTITION BY nexprlist orderby_opt frame_opt",
 /* 329 */ "window ::= ORDER BY sortlist frame_opt",
 /* 330 */ "window ::= nm ORDER BY sortlist frame_opt",
 /* 331 */ "window ::= nm frame_opt",
 /* 332 */ "frame_opt ::=",
 /* 333 */ "frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt",
 /* 334 */ "frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt",
 /* 335 */ "range_or_rows ::= RANGE|ROWS|GROUPS",
 /* 336 */ "frame_bound_s ::= frame_bound",
 /* 337 */ "frame_bound_s ::= UNBOUNDED PRECEDING",
 /* 338 */ "frame_bound_e ::= frame_bound",
 /* 339 */ "frame_bound_e ::= UNBOUNDED FOLLOWING",
 /* 340 */ "frame_bound ::= expr PRECEDING|FOLLOWING",
 /* 341 */ "frame_bound ::= CURRENT ROW",
 /* 342 */ "frame_exclude_opt ::=",
 /* 343 */ "frame_exclude_opt ::= EXCLUDE frame_exclude",
 /* 344 */ "frame_exclude ::= NO OTHERS",
 /* 345 */ "frame_exclude ::= CURRENT ROW",
 /* 346 */ "frame_exclude ::= GROUP|TIES",
 /* 347 */ "window_clause ::= WINDOW windowdefn_list",
 /* 348 */ "filter_over ::= filter_clause over_clause",
 /* 349 */ "filter_over ::= over_clause",
 /* 350 */ "filter_over ::= filter_clause",
 /* 351 */ "over_clause ::= OVER LP window RP",
 /* 352 */ "over_clause ::= OVER nm",
 /* 353 */ "filter_clause ::= FILTER LP WHERE expr RP",
 /* 354 */ "term ::= QNUMBER",
 /* 355 */ "ecmd ::= explain cmdx SEMI",
 /* 356 */ "trans_opt ::=",
 /* 357 */ "trans_opt ::= TRANSACTION",
 /* 358 */ "trans_opt ::= TRANSACTION nm",
 /* 359 */ "savepoint_opt ::= SAVEPOINT",
 /* 360 */ "savepoint_opt ::=",
 /* 361 */ "cmd ::= create_table create_table_args",
 /* 362 */ "table_option_set ::= table_option",
 /* 363 */ "columnlist ::= columnlist COMMA columnname carglist",
 /* 364 */ "columnlist ::= columnname carglist",
 /* 365 */ "typetoken ::= typename",
 /* 366 */ "typename ::= ID|STRING",
 /* 367 */ "signed ::= plus_num",
 /* 368 */ "signed ::= minus_num",
 /* 369 */ "carglist ::= carglist ccons",
 /* 370 */ "carglist ::=",
 /* 371 */ "ccons ::= NULL onconf",
 /* 372 */ "ccons ::= GENERATED ALWAYS AS generated",
 /* 373 */ "ccons ::= AS generated",
 /* 374 */ "conslist_opt ::= COMMA conslist",
 /* 375 */ "conslist ::= conslist tconscomma tcons",
 /* 376 */ "conslist ::= tcons",
 /* 377 */ "tconscomma ::=",
 /* 378 */ "defer_subclause_opt ::= defer_subclause",
 /* 379 */ "resolvetype ::= raisetype",
 /* 380 */ "oneselect ::= values",
 /* 381 */ "as ::= ID|STRING",
 /* 382 */ "indexed_opt ::= indexed_by",
 /* 383 */ "returning ::=",
 /* 384 */ "likeop ::= LIKE_KW|MATCH",
 /* 385 */ "case_operand ::= expr",
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
#endif /* !_SYNTAQLITE_EXTERNAL_PARSER */
#endif /* NDEBUG */


#if YYGROWABLESTACK
/*
** Try to increase the size of the parser stack.  Return the number
** of errors.  Return 0 on success.
*/
static int yyGrowStack(syntaqlite_yyParser *p){
  int oldSize = 1 + (int)(p->yystackEnd - p->yystack);
  int newSize;
  int idx;
  syntaqlite_yyStackEntry *pNew;

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
** second argument to syntaqlite_sqlite3ParserAlloc() below.  This can be changed by
** putting an appropriate #define in the %include section of the input
** grammar.
*/
#ifndef YYMALLOCARGTYPE
# define YYMALLOCARGTYPE size_t
#endif

/* Initialize a new parser that has already been allocated.
*/
void syntaqlite_sqlite3ParserInit(void *yypRawParser syntaqlite_sqlite3ParserCTX_PDECL){
  syntaqlite_yyParser *yypParser = (syntaqlite_yyParser*)yypRawParser;
  syntaqlite_sqlite3ParserCTX_STORE
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

#ifndef syntaqlite_sqlite3Parser_ENGINEALWAYSONSTACK
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
** to syntaqlite_sqlite3Parser and syntaqlite_sqlite3ParserFree.
*/
void *syntaqlite_sqlite3ParserAlloc(void *(*mallocProc)(YYMALLOCARGTYPE) syntaqlite_sqlite3ParserCTX_PDECL){
  syntaqlite_yyParser *yypParser;
  yypParser = (syntaqlite_yyParser*)(*mallocProc)( (YYMALLOCARGTYPE)sizeof(syntaqlite_yyParser) );
  if( yypParser ){
    syntaqlite_sqlite3ParserCTX_STORE
    syntaqlite_sqlite3ParserInit(yypParser syntaqlite_sqlite3ParserCTX_PARAM);
  }
  return (void*)yypParser;
}
#endif /* syntaqlite_sqlite3Parser_ENGINEALWAYSONSTACK */


/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the 
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
static void yy_destructor(
  syntaqlite_yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  syntaqlite_sqlite3ParserARG_FETCH
  syntaqlite_sqlite3ParserCTX_FETCH
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
static void yy_pop_parser_stack(syntaqlite_yyParser *pParser){
  syntaqlite_yyStackEntry *yytos;
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
void syntaqlite_sqlite3ParserFinalize(void *p){
  syntaqlite_yyParser *pParser = (syntaqlite_yyParser*)p;

  /* In-lined version of calling yy_pop_parser_stack() for each
  ** element left in the stack */
  syntaqlite_yyStackEntry *yytos = pParser->yytos;
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

#ifndef syntaqlite_sqlite3Parser_ENGINEALWAYSONSTACK
/* 
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/
void syntaqlite_sqlite3ParserFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
#ifndef YYPARSEFREENEVERNULL
  if( p==0 ) return;
#endif
  syntaqlite_sqlite3ParserFinalize(p);
  (*freeProc)(p);
}
#endif /* syntaqlite_sqlite3Parser_ENGINEALWAYSONSTACK */

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int syntaqlite_sqlite3ParserStackPeak(void *p){
  syntaqlite_yyParser *pParser = (syntaqlite_yyParser*)p;
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
int syntaqlite_sqlite3ParserCoverage(FILE *out){
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
static void yyStackOverflow(syntaqlite_yyParser *yypParser){
   syntaqlite_sqlite3ParserARG_FETCH
   syntaqlite_sqlite3ParserCTX_FETCH
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
   syntaqlite_sqlite3ParserARG_STORE /* Suppress warning about unused %extra_argument var */
   syntaqlite_sqlite3ParserCTX_STORE
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(syntaqlite_yyParser *yypParser, int yyNewState, const char *zTag){
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
  syntaqlite_yyParser *yypParser,          /* The parser to be shifted */
  YYACTIONTYPE yyNewState,      /* The new state to shift in */
  YYCODETYPE yyMajor,           /* The major token to shift in */
  syntaqlite_sqlite3ParserTOKENTYPE yyMinor        /* The minor token to shift in */
){
  syntaqlite_yyStackEntry *yytos;
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
#ifndef _SYNTAQLITE_EXTERNAL_PARSER
static const YYCODETYPE yyRuleInfoLhs[] = {
   192,  /* (0) cmdx ::= cmd */
   190,  /* (1) scanpt ::= */
   193,  /* (2) cmd ::= select */
   206,  /* (3) select ::= selectnowith */
   238,  /* (4) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
   239,  /* (5) distinct ::= DISTINCT */
   239,  /* (6) distinct ::= ALL */
   239,  /* (7) distinct ::= */
   251,  /* (8) sclp ::= */
   240,  /* (9) selcollist ::= sclp scanpt expr scanpt as */
   240,  /* (10) selcollist ::= sclp scanpt STAR */
   240,  /* (11) selcollist ::= sclp scanpt nm DOT STAR */
   189,  /* (12) as ::= AS nm */
   189,  /* (13) as ::= */
   241,  /* (14) from ::= */
   241,  /* (15) from ::= FROM seltablist */
   245,  /* (16) orderby_opt ::= */
   245,  /* (17) orderby_opt ::= ORDER BY sortlist */
   228,  /* (18) sortlist ::= sortlist COMMA expr sortorder nulls */
   228,  /* (19) sortlist ::= expr sortorder nulls */
   217,  /* (20) sortorder ::= ASC */
   217,  /* (21) sortorder ::= DESC */
   217,  /* (22) sortorder ::= */
   261,  /* (23) nulls ::= NULLS FIRST */
   261,  /* (24) nulls ::= NULLS LAST */
   261,  /* (25) nulls ::= */
   243,  /* (26) groupby_opt ::= */
   243,  /* (27) groupby_opt ::= GROUP BY nexprlist */
   244,  /* (28) having_opt ::= */
   244,  /* (29) having_opt ::= HAVING expr */
   246,  /* (30) limit_opt ::= */
   246,  /* (31) limit_opt ::= LIMIT expr */
   246,  /* (32) limit_opt ::= LIMIT expr OFFSET expr */
   246,  /* (33) limit_opt ::= LIMIT expr COMMA expr */
   242,  /* (34) where_opt ::= */
   242,  /* (35) where_opt ::= WHERE expr */
   215,  /* (36) expr ::= LP expr RP */
   215,  /* (37) expr ::= ID|INDEXED|JOIN_KW */
   215,  /* (38) expr ::= nm DOT nm */
   215,  /* (39) expr ::= nm DOT nm DOT nm */
   214,  /* (40) term ::= NULL|FLOAT|BLOB */
   214,  /* (41) term ::= STRING */
   214,  /* (42) term ::= INTEGER */
   215,  /* (43) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   215,  /* (44) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   215,  /* (45) expr ::= expr AND expr */
   215,  /* (46) expr ::= expr OR expr */
   215,  /* (47) expr ::= expr LT|GT|GE|LE expr */
   215,  /* (48) expr ::= expr EQ|NE expr */
   215,  /* (49) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
   215,  /* (50) expr ::= expr PLUS|MINUS expr */
   215,  /* (51) expr ::= expr STAR|SLASH|REM expr */
   215,  /* (52) expr ::= expr CONCAT expr */
   215,  /* (53) expr ::= NOT expr */
   215,  /* (54) expr ::= BITNOT expr */
   215,  /* (55) expr ::= PLUS|MINUS expr */
   257,  /* (56) exprlist ::= */
   249,  /* (57) nexprlist ::= nexprlist COMMA expr */
   249,  /* (58) nexprlist ::= expr */
   319,  /* (59) input ::= cmdlist */
   320,  /* (60) cmdlist ::= cmdlist ecmd */
   320,  /* (61) cmdlist ::= ecmd */
   321,  /* (62) ecmd ::= SEMI */
   321,  /* (63) ecmd ::= cmdx SEMI */
   187,  /* (64) nm ::= ID|INDEXED|JOIN_KW */
   187,  /* (65) nm ::= STRING */
   236,  /* (66) selectnowith ::= oneselect */
   251,  /* (67) sclp ::= selcollist COMMA */
   215,  /* (68) expr ::= term */
   257,  /* (69) exprlist ::= nexprlist */
   191,  /* (70) explain ::= EXPLAIN */
   191,  /* (71) explain ::= EXPLAIN QUERY PLAN */
   193,  /* (72) cmd ::= BEGIN transtype trans_opt */
   194,  /* (73) transtype ::= */
   194,  /* (74) transtype ::= DEFERRED */
   194,  /* (75) transtype ::= IMMEDIATE */
   194,  /* (76) transtype ::= EXCLUSIVE */
   193,  /* (77) cmd ::= COMMIT|END trans_opt */
   193,  /* (78) cmd ::= ROLLBACK trans_opt */
   193,  /* (79) cmd ::= SAVEPOINT nm */
   193,  /* (80) cmd ::= RELEASE savepoint_opt nm */
   193,  /* (81) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
   197,  /* (82) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   198,  /* (83) createkw ::= CREATE */
   200,  /* (84) ifnotexists ::= */
   200,  /* (85) ifnotexists ::= IF NOT EXISTS */
   199,  /* (86) temp ::= TEMP */
   199,  /* (87) temp ::= */
   202,  /* (88) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   202,  /* (89) create_table_args ::= AS select */
   205,  /* (90) table_option_set ::= */
   205,  /* (91) table_option_set ::= table_option_set COMMA table_option */
   207,  /* (92) table_option ::= WITHOUT nm */
   207,  /* (93) table_option ::= nm */
   208,  /* (94) columnname ::= nm typetoken */
   209,  /* (95) typetoken ::= */
   209,  /* (96) typetoken ::= typename LP signed RP */
   209,  /* (97) typetoken ::= typename LP signed COMMA signed RP */
   210,  /* (98) typename ::= typename ID|STRING */
   212,  /* (99) scantok ::= */
   213,  /* (100) ccons ::= CONSTRAINT nm */
   213,  /* (101) ccons ::= DEFAULT scantok term */
   213,  /* (102) ccons ::= DEFAULT LP expr RP */
   213,  /* (103) ccons ::= DEFAULT PLUS scantok term */
   213,  /* (104) ccons ::= DEFAULT MINUS scantok term */
   213,  /* (105) ccons ::= DEFAULT scantok ID|INDEXED */
   213,  /* (106) ccons ::= NOT NULL onconf */
   213,  /* (107) ccons ::= PRIMARY KEY sortorder onconf autoinc */
   213,  /* (108) ccons ::= UNIQUE onconf */
   213,  /* (109) ccons ::= CHECK LP expr RP */
   213,  /* (110) ccons ::= REFERENCES nm eidlist_opt refargs */
   213,  /* (111) ccons ::= defer_subclause */
   213,  /* (112) ccons ::= COLLATE ID|STRING */
   222,  /* (113) generated ::= LP expr RP */
   222,  /* (114) generated ::= LP expr RP ID */
   218,  /* (115) autoinc ::= */
   218,  /* (116) autoinc ::= AUTOINCR */
   220,  /* (117) refargs ::= */
   220,  /* (118) refargs ::= refargs refarg */
   223,  /* (119) refarg ::= MATCH nm */
   223,  /* (120) refarg ::= ON INSERT refact */
   223,  /* (121) refarg ::= ON DELETE refact */
   223,  /* (122) refarg ::= ON UPDATE refact */
   224,  /* (123) refact ::= SET NULL */
   224,  /* (124) refact ::= SET DEFAULT */
   224,  /* (125) refact ::= CASCADE */
   224,  /* (126) refact ::= RESTRICT */
   224,  /* (127) refact ::= NO ACTION */
   221,  /* (128) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
   221,  /* (129) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
   225,  /* (130) init_deferred_pred_opt ::= */
   225,  /* (131) init_deferred_pred_opt ::= INITIALLY DEFERRED */
   225,  /* (132) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
   204,  /* (133) conslist_opt ::= */
   226,  /* (134) tconscomma ::= COMMA */
   227,  /* (135) tcons ::= CONSTRAINT nm */
   227,  /* (136) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
   227,  /* (137) tcons ::= UNIQUE LP sortlist RP onconf */
   227,  /* (138) tcons ::= CHECK LP expr RP onconf */
   227,  /* (139) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
   230,  /* (140) defer_subclause_opt ::= */
   216,  /* (141) onconf ::= */
   216,  /* (142) onconf ::= ON CONFLICT resolvetype */
   232,  /* (143) orconf ::= */
   232,  /* (144) orconf ::= OR resolvetype */
   231,  /* (145) resolvetype ::= IGNORE */
   231,  /* (146) resolvetype ::= REPLACE */
   193,  /* (147) cmd ::= DROP TABLE ifexists fullname */
   233,  /* (148) ifexists ::= IF EXISTS */
   233,  /* (149) ifexists ::= */
   193,  /* (150) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   193,  /* (151) cmd ::= DROP VIEW ifexists fullname */
   206,  /* (152) select ::= WITH wqlist selectnowith */
   206,  /* (153) select ::= WITH RECURSIVE wqlist selectnowith */
   236,  /* (154) selectnowith ::= selectnowith multiselect_op oneselect */
   237,  /* (155) multiselect_op ::= UNION */
   237,  /* (156) multiselect_op ::= UNION ALL */
   237,  /* (157) multiselect_op ::= EXCEPT|INTERSECT */
   238,  /* (158) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   248,  /* (159) values ::= VALUES LP nexprlist RP */
   238,  /* (160) oneselect ::= mvalues */
   250,  /* (161) mvalues ::= values COMMA LP nexprlist RP */
   250,  /* (162) mvalues ::= mvalues COMMA LP nexprlist RP */
   253,  /* (163) stl_prefix ::= seltablist joinop */
   253,  /* (164) stl_prefix ::= */
   252,  /* (165) seltablist ::= stl_prefix nm dbnm as on_using */
   252,  /* (166) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   252,  /* (167) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   252,  /* (168) seltablist ::= stl_prefix LP select RP as on_using */
   252,  /* (169) seltablist ::= stl_prefix LP seltablist RP as on_using */
   201,  /* (170) dbnm ::= */
   201,  /* (171) dbnm ::= DOT nm */
   234,  /* (172) fullname ::= nm */
   234,  /* (173) fullname ::= nm DOT nm */
   258,  /* (174) xfullname ::= nm */
   258,  /* (175) xfullname ::= nm DOT nm */
   258,  /* (176) xfullname ::= nm DOT nm AS nm */
   258,  /* (177) xfullname ::= nm AS nm */
   254,  /* (178) joinop ::= COMMA|JOIN */
   254,  /* (179) joinop ::= JOIN_KW JOIN */
   254,  /* (180) joinop ::= JOIN_KW nm JOIN */
   254,  /* (181) joinop ::= JOIN_KW nm nm JOIN */
   255,  /* (182) on_using ::= ON expr */
   255,  /* (183) on_using ::= USING LP idlist RP */
   255,  /* (184) on_using ::= */
   260,  /* (185) indexed_opt ::= */
   256,  /* (186) indexed_by ::= INDEXED BY nm */
   256,  /* (187) indexed_by ::= NOT INDEXED */
   193,  /* (188) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
   263,  /* (189) where_opt_ret ::= */
   263,  /* (190) where_opt_ret ::= WHERE expr */
   263,  /* (191) where_opt_ret ::= RETURNING selcollist */
   263,  /* (192) where_opt_ret ::= WHERE expr RETURNING selcollist */
   193,  /* (193) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   264,  /* (194) setlist ::= setlist COMMA nm EQ expr */
   264,  /* (195) setlist ::= setlist COMMA LP idlist RP EQ expr */
   264,  /* (196) setlist ::= nm EQ expr */
   264,  /* (197) setlist ::= LP idlist RP EQ expr */
   193,  /* (198) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   193,  /* (199) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
   267,  /* (200) upsert ::= */
   267,  /* (201) upsert ::= RETURNING selcollist */
   267,  /* (202) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
   267,  /* (203) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
   267,  /* (204) upsert ::= ON CONFLICT DO NOTHING returning */
   267,  /* (205) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
   268,  /* (206) returning ::= RETURNING selcollist */
   265,  /* (207) insert_cmd ::= INSERT orconf */
   265,  /* (208) insert_cmd ::= REPLACE */
   266,  /* (209) idlist_opt ::= */
   266,  /* (210) idlist_opt ::= LP idlist RP */
   259,  /* (211) idlist ::= idlist COMMA nm */
   259,  /* (212) idlist ::= nm */
   215,  /* (213) expr ::= VARIABLE */
   215,  /* (214) expr ::= expr COLLATE ID|STRING */
   215,  /* (215) expr ::= CAST LP expr AS typetoken RP */
   215,  /* (216) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   215,  /* (217) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   215,  /* (218) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   215,  /* (219) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   214,  /* (220) term ::= CTIME_KW */
   215,  /* (221) expr ::= LP nexprlist COMMA expr RP */
   270,  /* (222) likeop ::= NOT LIKE_KW|MATCH */
   215,  /* (223) expr ::= expr likeop expr */
   215,  /* (224) expr ::= expr likeop expr ESCAPE expr */
   215,  /* (225) expr ::= expr ISNULL|NOTNULL */
   215,  /* (226) expr ::= expr NOT NULL */
   215,  /* (227) expr ::= expr IS expr */
   215,  /* (228) expr ::= expr IS NOT expr */
   215,  /* (229) expr ::= expr IS NOT DISTINCT FROM expr */
   215,  /* (230) expr ::= expr IS DISTINCT FROM expr */
   215,  /* (231) expr ::= expr PTR expr */
   271,  /* (232) between_op ::= BETWEEN */
   271,  /* (233) between_op ::= NOT BETWEEN */
   215,  /* (234) expr ::= expr between_op expr AND expr */
   272,  /* (235) in_op ::= IN */
   272,  /* (236) in_op ::= NOT IN */
   215,  /* (237) expr ::= expr in_op LP exprlist RP */
   215,  /* (238) expr ::= LP select RP */
   215,  /* (239) expr ::= expr in_op LP select RP */
   215,  /* (240) expr ::= expr in_op nm dbnm paren_exprlist */
   215,  /* (241) expr ::= EXISTS LP select RP */
   215,  /* (242) expr ::= CASE case_operand case_exprlist case_else END */
   275,  /* (243) case_exprlist ::= case_exprlist WHEN expr THEN expr */
   275,  /* (244) case_exprlist ::= WHEN expr THEN expr */
   276,  /* (245) case_else ::= ELSE expr */
   276,  /* (246) case_else ::= */
   274,  /* (247) case_operand ::= */
   273,  /* (248) paren_exprlist ::= */
   273,  /* (249) paren_exprlist ::= LP exprlist RP */
   193,  /* (250) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
   277,  /* (251) uniqueflag ::= UNIQUE */
   277,  /* (252) uniqueflag ::= */
   219,  /* (253) eidlist_opt ::= */
   219,  /* (254) eidlist_opt ::= LP eidlist RP */
   229,  /* (255) eidlist ::= eidlist COMMA nm collate sortorder */
   229,  /* (256) eidlist ::= nm collate sortorder */
   278,  /* (257) collate ::= */
   278,  /* (258) collate ::= COLLATE ID|STRING */
   193,  /* (259) cmd ::= DROP INDEX ifexists fullname */
   193,  /* (260) cmd ::= VACUUM vinto */
   193,  /* (261) cmd ::= VACUUM nm vinto */
   279,  /* (262) vinto ::= INTO expr */
   279,  /* (263) vinto ::= */
   193,  /* (264) cmd ::= PRAGMA nm dbnm */
   193,  /* (265) cmd ::= PRAGMA nm dbnm EQ nmnum */
   193,  /* (266) cmd ::= PRAGMA nm dbnm LP nmnum RP */
   193,  /* (267) cmd ::= PRAGMA nm dbnm EQ minus_num */
   193,  /* (268) cmd ::= PRAGMA nm dbnm LP minus_num RP */
   282,  /* (269) plus_num ::= PLUS INTEGER|FLOAT */
   281,  /* (270) minus_num ::= MINUS INTEGER|FLOAT */
   193,  /* (271) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
   283,  /* (272) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
   285,  /* (273) trigger_time ::= BEFORE|AFTER */
   285,  /* (274) trigger_time ::= INSTEAD OF */
   285,  /* (275) trigger_time ::= */
   286,  /* (276) trigger_event ::= DELETE|INSERT */
   286,  /* (277) trigger_event ::= UPDATE */
   286,  /* (278) trigger_event ::= UPDATE OF idlist */
   288,  /* (279) when_clause ::= */
   288,  /* (280) when_clause ::= WHEN expr */
   284,  /* (281) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
   284,  /* (282) trigger_cmd_list ::= trigger_cmd SEMI */
   290,  /* (283) trnm ::= nm DOT nm */
   291,  /* (284) tridxby ::= INDEXED BY nm */
   291,  /* (285) tridxby ::= NOT INDEXED */
   289,  /* (286) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
   289,  /* (287) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
   289,  /* (288) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
   289,  /* (289) trigger_cmd ::= scanpt select scanpt */
   215,  /* (290) expr ::= RAISE LP IGNORE RP */
   215,  /* (291) expr ::= RAISE LP raisetype COMMA expr RP */
   292,  /* (292) raisetype ::= ROLLBACK */
   292,  /* (293) raisetype ::= ABORT */
   292,  /* (294) raisetype ::= FAIL */
   193,  /* (295) cmd ::= DROP TRIGGER ifexists fullname */
   193,  /* (296) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
   193,  /* (297) cmd ::= DETACH database_kw_opt expr */
   294,  /* (298) key_opt ::= */
   294,  /* (299) key_opt ::= KEY expr */
   193,  /* (300) cmd ::= REINDEX */
   193,  /* (301) cmd ::= REINDEX nm dbnm */
   193,  /* (302) cmd ::= ANALYZE */
   193,  /* (303) cmd ::= ANALYZE nm dbnm */
   193,  /* (304) cmd ::= ALTER TABLE fullname RENAME TO nm */
   193,  /* (305) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
   193,  /* (306) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
   295,  /* (307) add_column_fullname ::= fullname */
   193,  /* (308) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
   193,  /* (309) cmd ::= create_vtab */
   193,  /* (310) cmd ::= create_vtab LP vtabarglist RP */
   298,  /* (311) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
   300,  /* (312) vtabarg ::= */
   301,  /* (313) vtabargtoken ::= ANY */
   301,  /* (314) vtabargtoken ::= lp anylist RP */
   302,  /* (315) lp ::= LP */
   262,  /* (316) with ::= WITH wqlist */
   262,  /* (317) with ::= WITH RECURSIVE wqlist */
   304,  /* (318) wqas ::= AS */
   304,  /* (319) wqas ::= AS MATERIALIZED */
   304,  /* (320) wqas ::= AS NOT MATERIALIZED */
   305,  /* (321) wqitem ::= withnm eidlist_opt wqas LP select RP */
   306,  /* (322) withnm ::= nm */
   235,  /* (323) wqlist ::= wqitem */
   235,  /* (324) wqlist ::= wqlist COMMA wqitem */
   307,  /* (325) windowdefn_list ::= windowdefn_list COMMA windowdefn */
   308,  /* (326) windowdefn ::= nm AS LP window RP */
   309,  /* (327) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
   309,  /* (328) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
   309,  /* (329) window ::= ORDER BY sortlist frame_opt */
   309,  /* (330) window ::= nm ORDER BY sortlist frame_opt */
   309,  /* (331) window ::= nm frame_opt */
   310,  /* (332) frame_opt ::= */
   310,  /* (333) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
   310,  /* (334) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
   311,  /* (335) range_or_rows ::= RANGE|ROWS|GROUPS */
   312,  /* (336) frame_bound_s ::= frame_bound */
   312,  /* (337) frame_bound_s ::= UNBOUNDED PRECEDING */
   314,  /* (338) frame_bound_e ::= frame_bound */
   314,  /* (339) frame_bound_e ::= UNBOUNDED FOLLOWING */
   315,  /* (340) frame_bound ::= expr PRECEDING|FOLLOWING */
   315,  /* (341) frame_bound ::= CURRENT ROW */
   313,  /* (342) frame_exclude_opt ::= */
   313,  /* (343) frame_exclude_opt ::= EXCLUDE frame_exclude */
   316,  /* (344) frame_exclude ::= NO OTHERS */
   316,  /* (345) frame_exclude ::= CURRENT ROW */
   316,  /* (346) frame_exclude ::= GROUP|TIES */
   247,  /* (347) window_clause ::= WINDOW windowdefn_list */
   269,  /* (348) filter_over ::= filter_clause over_clause */
   269,  /* (349) filter_over ::= over_clause */
   269,  /* (350) filter_over ::= filter_clause */
   318,  /* (351) over_clause ::= OVER LP window RP */
   318,  /* (352) over_clause ::= OVER nm */
   317,  /* (353) filter_clause ::= FILTER LP WHERE expr RP */
   214,  /* (354) term ::= QNUMBER */
   321,  /* (355) ecmd ::= explain cmdx SEMI */
   195,  /* (356) trans_opt ::= */
   195,  /* (357) trans_opt ::= TRANSACTION */
   195,  /* (358) trans_opt ::= TRANSACTION nm */
   196,  /* (359) savepoint_opt ::= SAVEPOINT */
   196,  /* (360) savepoint_opt ::= */
   193,  /* (361) cmd ::= create_table create_table_args */
   205,  /* (362) table_option_set ::= table_option */
   203,  /* (363) columnlist ::= columnlist COMMA columnname carglist */
   203,  /* (364) columnlist ::= columnname carglist */
   209,  /* (365) typetoken ::= typename */
   210,  /* (366) typename ::= ID|STRING */
   211,  /* (367) signed ::= plus_num */
   211,  /* (368) signed ::= minus_num */
   297,  /* (369) carglist ::= carglist ccons */
   297,  /* (370) carglist ::= */
   213,  /* (371) ccons ::= NULL onconf */
   213,  /* (372) ccons ::= GENERATED ALWAYS AS generated */
   213,  /* (373) ccons ::= AS generated */
   204,  /* (374) conslist_opt ::= COMMA conslist */
   322,  /* (375) conslist ::= conslist tconscomma tcons */
   322,  /* (376) conslist ::= tcons */
   226,  /* (377) tconscomma ::= */
   230,  /* (378) defer_subclause_opt ::= defer_subclause */
   231,  /* (379) resolvetype ::= raisetype */
   238,  /* (380) oneselect ::= values */
   189,  /* (381) as ::= ID|STRING */
   260,  /* (382) indexed_opt ::= indexed_by */
   268,  /* (383) returning ::= */
   270,  /* (384) likeop ::= LIKE_KW|MATCH */
   274,  /* (385) case_operand ::= expr */
   280,  /* (386) nmnum ::= plus_num */
   280,  /* (387) nmnum ::= nm */
   280,  /* (388) nmnum ::= ON */
   280,  /* (389) nmnum ::= DELETE */
   280,  /* (390) nmnum ::= DEFAULT */
   282,  /* (391) plus_num ::= INTEGER|FLOAT */
   287,  /* (392) foreach_clause ::= */
   287,  /* (393) foreach_clause ::= FOR EACH ROW */
   290,  /* (394) trnm ::= nm */
   291,  /* (395) tridxby ::= */
   293,  /* (396) database_kw_opt ::= DATABASE */
   293,  /* (397) database_kw_opt ::= */
   296,  /* (398) kwcolumn_opt ::= */
   296,  /* (399) kwcolumn_opt ::= COLUMNKW */
   299,  /* (400) vtabarglist ::= vtabarg */
   299,  /* (401) vtabarglist ::= vtabarglist COMMA vtabarg */
   300,  /* (402) vtabarg ::= vtabarg vtabargtoken */
   303,  /* (403) anylist ::= */
   303,  /* (404) anylist ::= anylist LP anylist RP */
   303,  /* (405) anylist ::= anylist ANY */
   262,  /* (406) with ::= */
   307,  /* (407) windowdefn_list ::= windowdefn */
   309,  /* (408) window ::= frame_opt */
};
#endif /* !_SYNTAQLITE_EXTERNAL_PARSER */

/* For rule J, yyRuleInfoNRhs[J] contains the negative of the number
** of symbols on the right-hand side of that rule. */
#ifndef _SYNTAQLITE_EXTERNAL_PARSER
static const signed char yyRuleInfoNRhs[] = {
   -1,  /* (0) cmdx ::= cmd */
    0,  /* (1) scanpt ::= */
   -1,  /* (2) cmd ::= select */
   -1,  /* (3) select ::= selectnowith */
   -9,  /* (4) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
   -1,  /* (5) distinct ::= DISTINCT */
   -1,  /* (6) distinct ::= ALL */
    0,  /* (7) distinct ::= */
    0,  /* (8) sclp ::= */
   -5,  /* (9) selcollist ::= sclp scanpt expr scanpt as */
   -3,  /* (10) selcollist ::= sclp scanpt STAR */
   -5,  /* (11) selcollist ::= sclp scanpt nm DOT STAR */
   -2,  /* (12) as ::= AS nm */
    0,  /* (13) as ::= */
    0,  /* (14) from ::= */
   -2,  /* (15) from ::= FROM seltablist */
    0,  /* (16) orderby_opt ::= */
   -3,  /* (17) orderby_opt ::= ORDER BY sortlist */
   -5,  /* (18) sortlist ::= sortlist COMMA expr sortorder nulls */
   -3,  /* (19) sortlist ::= expr sortorder nulls */
   -1,  /* (20) sortorder ::= ASC */
   -1,  /* (21) sortorder ::= DESC */
    0,  /* (22) sortorder ::= */
   -2,  /* (23) nulls ::= NULLS FIRST */
   -2,  /* (24) nulls ::= NULLS LAST */
    0,  /* (25) nulls ::= */
    0,  /* (26) groupby_opt ::= */
   -3,  /* (27) groupby_opt ::= GROUP BY nexprlist */
    0,  /* (28) having_opt ::= */
   -2,  /* (29) having_opt ::= HAVING expr */
    0,  /* (30) limit_opt ::= */
   -2,  /* (31) limit_opt ::= LIMIT expr */
   -4,  /* (32) limit_opt ::= LIMIT expr OFFSET expr */
   -4,  /* (33) limit_opt ::= LIMIT expr COMMA expr */
    0,  /* (34) where_opt ::= */
   -2,  /* (35) where_opt ::= WHERE expr */
   -3,  /* (36) expr ::= LP expr RP */
   -1,  /* (37) expr ::= ID|INDEXED|JOIN_KW */
   -3,  /* (38) expr ::= nm DOT nm */
   -5,  /* (39) expr ::= nm DOT nm DOT nm */
   -1,  /* (40) term ::= NULL|FLOAT|BLOB */
   -1,  /* (41) term ::= STRING */
   -1,  /* (42) term ::= INTEGER */
   -5,  /* (43) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   -4,  /* (44) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   -3,  /* (45) expr ::= expr AND expr */
   -3,  /* (46) expr ::= expr OR expr */
   -3,  /* (47) expr ::= expr LT|GT|GE|LE expr */
   -3,  /* (48) expr ::= expr EQ|NE expr */
   -3,  /* (49) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
   -3,  /* (50) expr ::= expr PLUS|MINUS expr */
   -3,  /* (51) expr ::= expr STAR|SLASH|REM expr */
   -3,  /* (52) expr ::= expr CONCAT expr */
   -2,  /* (53) expr ::= NOT expr */
   -2,  /* (54) expr ::= BITNOT expr */
   -2,  /* (55) expr ::= PLUS|MINUS expr */
    0,  /* (56) exprlist ::= */
   -3,  /* (57) nexprlist ::= nexprlist COMMA expr */
   -1,  /* (58) nexprlist ::= expr */
   -1,  /* (59) input ::= cmdlist */
   -2,  /* (60) cmdlist ::= cmdlist ecmd */
   -1,  /* (61) cmdlist ::= ecmd */
   -1,  /* (62) ecmd ::= SEMI */
   -2,  /* (63) ecmd ::= cmdx SEMI */
   -1,  /* (64) nm ::= ID|INDEXED|JOIN_KW */
   -1,  /* (65) nm ::= STRING */
   -1,  /* (66) selectnowith ::= oneselect */
   -2,  /* (67) sclp ::= selcollist COMMA */
   -1,  /* (68) expr ::= term */
   -1,  /* (69) exprlist ::= nexprlist */
   -1,  /* (70) explain ::= EXPLAIN */
   -3,  /* (71) explain ::= EXPLAIN QUERY PLAN */
   -3,  /* (72) cmd ::= BEGIN transtype trans_opt */
    0,  /* (73) transtype ::= */
   -1,  /* (74) transtype ::= DEFERRED */
   -1,  /* (75) transtype ::= IMMEDIATE */
   -1,  /* (76) transtype ::= EXCLUSIVE */
   -2,  /* (77) cmd ::= COMMIT|END trans_opt */
   -2,  /* (78) cmd ::= ROLLBACK trans_opt */
   -2,  /* (79) cmd ::= SAVEPOINT nm */
   -3,  /* (80) cmd ::= RELEASE savepoint_opt nm */
   -5,  /* (81) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
   -6,  /* (82) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   -1,  /* (83) createkw ::= CREATE */
    0,  /* (84) ifnotexists ::= */
   -3,  /* (85) ifnotexists ::= IF NOT EXISTS */
   -1,  /* (86) temp ::= TEMP */
    0,  /* (87) temp ::= */
   -5,  /* (88) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   -2,  /* (89) create_table_args ::= AS select */
    0,  /* (90) table_option_set ::= */
   -3,  /* (91) table_option_set ::= table_option_set COMMA table_option */
   -2,  /* (92) table_option ::= WITHOUT nm */
   -1,  /* (93) table_option ::= nm */
   -2,  /* (94) columnname ::= nm typetoken */
    0,  /* (95) typetoken ::= */
   -4,  /* (96) typetoken ::= typename LP signed RP */
   -6,  /* (97) typetoken ::= typename LP signed COMMA signed RP */
   -2,  /* (98) typename ::= typename ID|STRING */
    0,  /* (99) scantok ::= */
   -2,  /* (100) ccons ::= CONSTRAINT nm */
   -3,  /* (101) ccons ::= DEFAULT scantok term */
   -4,  /* (102) ccons ::= DEFAULT LP expr RP */
   -4,  /* (103) ccons ::= DEFAULT PLUS scantok term */
   -4,  /* (104) ccons ::= DEFAULT MINUS scantok term */
   -3,  /* (105) ccons ::= DEFAULT scantok ID|INDEXED */
   -3,  /* (106) ccons ::= NOT NULL onconf */
   -5,  /* (107) ccons ::= PRIMARY KEY sortorder onconf autoinc */
   -2,  /* (108) ccons ::= UNIQUE onconf */
   -4,  /* (109) ccons ::= CHECK LP expr RP */
   -4,  /* (110) ccons ::= REFERENCES nm eidlist_opt refargs */
   -1,  /* (111) ccons ::= defer_subclause */
   -2,  /* (112) ccons ::= COLLATE ID|STRING */
   -3,  /* (113) generated ::= LP expr RP */
   -4,  /* (114) generated ::= LP expr RP ID */
    0,  /* (115) autoinc ::= */
   -1,  /* (116) autoinc ::= AUTOINCR */
    0,  /* (117) refargs ::= */
   -2,  /* (118) refargs ::= refargs refarg */
   -2,  /* (119) refarg ::= MATCH nm */
   -3,  /* (120) refarg ::= ON INSERT refact */
   -3,  /* (121) refarg ::= ON DELETE refact */
   -3,  /* (122) refarg ::= ON UPDATE refact */
   -2,  /* (123) refact ::= SET NULL */
   -2,  /* (124) refact ::= SET DEFAULT */
   -1,  /* (125) refact ::= CASCADE */
   -1,  /* (126) refact ::= RESTRICT */
   -2,  /* (127) refact ::= NO ACTION */
   -3,  /* (128) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
   -2,  /* (129) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
    0,  /* (130) init_deferred_pred_opt ::= */
   -2,  /* (131) init_deferred_pred_opt ::= INITIALLY DEFERRED */
   -2,  /* (132) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
    0,  /* (133) conslist_opt ::= */
   -1,  /* (134) tconscomma ::= COMMA */
   -2,  /* (135) tcons ::= CONSTRAINT nm */
   -7,  /* (136) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
   -5,  /* (137) tcons ::= UNIQUE LP sortlist RP onconf */
   -5,  /* (138) tcons ::= CHECK LP expr RP onconf */
  -10,  /* (139) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
    0,  /* (140) defer_subclause_opt ::= */
    0,  /* (141) onconf ::= */
   -3,  /* (142) onconf ::= ON CONFLICT resolvetype */
    0,  /* (143) orconf ::= */
   -2,  /* (144) orconf ::= OR resolvetype */
   -1,  /* (145) resolvetype ::= IGNORE */
   -1,  /* (146) resolvetype ::= REPLACE */
   -4,  /* (147) cmd ::= DROP TABLE ifexists fullname */
   -2,  /* (148) ifexists ::= IF EXISTS */
    0,  /* (149) ifexists ::= */
   -9,  /* (150) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   -4,  /* (151) cmd ::= DROP VIEW ifexists fullname */
   -3,  /* (152) select ::= WITH wqlist selectnowith */
   -4,  /* (153) select ::= WITH RECURSIVE wqlist selectnowith */
   -3,  /* (154) selectnowith ::= selectnowith multiselect_op oneselect */
   -1,  /* (155) multiselect_op ::= UNION */
   -2,  /* (156) multiselect_op ::= UNION ALL */
   -1,  /* (157) multiselect_op ::= EXCEPT|INTERSECT */
  -10,  /* (158) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   -4,  /* (159) values ::= VALUES LP nexprlist RP */
   -1,  /* (160) oneselect ::= mvalues */
   -5,  /* (161) mvalues ::= values COMMA LP nexprlist RP */
   -5,  /* (162) mvalues ::= mvalues COMMA LP nexprlist RP */
   -2,  /* (163) stl_prefix ::= seltablist joinop */
    0,  /* (164) stl_prefix ::= */
   -5,  /* (165) seltablist ::= stl_prefix nm dbnm as on_using */
   -6,  /* (166) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   -8,  /* (167) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   -6,  /* (168) seltablist ::= stl_prefix LP select RP as on_using */
   -6,  /* (169) seltablist ::= stl_prefix LP seltablist RP as on_using */
    0,  /* (170) dbnm ::= */
   -2,  /* (171) dbnm ::= DOT nm */
   -1,  /* (172) fullname ::= nm */
   -3,  /* (173) fullname ::= nm DOT nm */
   -1,  /* (174) xfullname ::= nm */
   -3,  /* (175) xfullname ::= nm DOT nm */
   -5,  /* (176) xfullname ::= nm DOT nm AS nm */
   -3,  /* (177) xfullname ::= nm AS nm */
   -1,  /* (178) joinop ::= COMMA|JOIN */
   -2,  /* (179) joinop ::= JOIN_KW JOIN */
   -3,  /* (180) joinop ::= JOIN_KW nm JOIN */
   -4,  /* (181) joinop ::= JOIN_KW nm nm JOIN */
   -2,  /* (182) on_using ::= ON expr */
   -4,  /* (183) on_using ::= USING LP idlist RP */
    0,  /* (184) on_using ::= */
    0,  /* (185) indexed_opt ::= */
   -3,  /* (186) indexed_by ::= INDEXED BY nm */
   -2,  /* (187) indexed_by ::= NOT INDEXED */
   -6,  /* (188) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
    0,  /* (189) where_opt_ret ::= */
   -2,  /* (190) where_opt_ret ::= WHERE expr */
   -2,  /* (191) where_opt_ret ::= RETURNING selcollist */
   -4,  /* (192) where_opt_ret ::= WHERE expr RETURNING selcollist */
   -9,  /* (193) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   -5,  /* (194) setlist ::= setlist COMMA nm EQ expr */
   -7,  /* (195) setlist ::= setlist COMMA LP idlist RP EQ expr */
   -3,  /* (196) setlist ::= nm EQ expr */
   -5,  /* (197) setlist ::= LP idlist RP EQ expr */
   -7,  /* (198) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   -8,  /* (199) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
    0,  /* (200) upsert ::= */
   -2,  /* (201) upsert ::= RETURNING selcollist */
  -12,  /* (202) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
   -9,  /* (203) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
   -5,  /* (204) upsert ::= ON CONFLICT DO NOTHING returning */
   -8,  /* (205) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
   -2,  /* (206) returning ::= RETURNING selcollist */
   -2,  /* (207) insert_cmd ::= INSERT orconf */
   -1,  /* (208) insert_cmd ::= REPLACE */
    0,  /* (209) idlist_opt ::= */
   -3,  /* (210) idlist_opt ::= LP idlist RP */
   -3,  /* (211) idlist ::= idlist COMMA nm */
   -1,  /* (212) idlist ::= nm */
   -1,  /* (213) expr ::= VARIABLE */
   -3,  /* (214) expr ::= expr COLLATE ID|STRING */
   -6,  /* (215) expr ::= CAST LP expr AS typetoken RP */
   -8,  /* (216) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   -6,  /* (217) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   -9,  /* (218) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   -5,  /* (219) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   -1,  /* (220) term ::= CTIME_KW */
   -5,  /* (221) expr ::= LP nexprlist COMMA expr RP */
   -2,  /* (222) likeop ::= NOT LIKE_KW|MATCH */
   -3,  /* (223) expr ::= expr likeop expr */
   -5,  /* (224) expr ::= expr likeop expr ESCAPE expr */
   -2,  /* (225) expr ::= expr ISNULL|NOTNULL */
   -3,  /* (226) expr ::= expr NOT NULL */
   -3,  /* (227) expr ::= expr IS expr */
   -4,  /* (228) expr ::= expr IS NOT expr */
   -6,  /* (229) expr ::= expr IS NOT DISTINCT FROM expr */
   -5,  /* (230) expr ::= expr IS DISTINCT FROM expr */
   -3,  /* (231) expr ::= expr PTR expr */
   -1,  /* (232) between_op ::= BETWEEN */
   -2,  /* (233) between_op ::= NOT BETWEEN */
   -5,  /* (234) expr ::= expr between_op expr AND expr */
   -1,  /* (235) in_op ::= IN */
   -2,  /* (236) in_op ::= NOT IN */
   -5,  /* (237) expr ::= expr in_op LP exprlist RP */
   -3,  /* (238) expr ::= LP select RP */
   -5,  /* (239) expr ::= expr in_op LP select RP */
   -5,  /* (240) expr ::= expr in_op nm dbnm paren_exprlist */
   -4,  /* (241) expr ::= EXISTS LP select RP */
   -5,  /* (242) expr ::= CASE case_operand case_exprlist case_else END */
   -5,  /* (243) case_exprlist ::= case_exprlist WHEN expr THEN expr */
   -4,  /* (244) case_exprlist ::= WHEN expr THEN expr */
   -2,  /* (245) case_else ::= ELSE expr */
    0,  /* (246) case_else ::= */
    0,  /* (247) case_operand ::= */
    0,  /* (248) paren_exprlist ::= */
   -3,  /* (249) paren_exprlist ::= LP exprlist RP */
  -12,  /* (250) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
   -1,  /* (251) uniqueflag ::= UNIQUE */
    0,  /* (252) uniqueflag ::= */
    0,  /* (253) eidlist_opt ::= */
   -3,  /* (254) eidlist_opt ::= LP eidlist RP */
   -5,  /* (255) eidlist ::= eidlist COMMA nm collate sortorder */
   -3,  /* (256) eidlist ::= nm collate sortorder */
    0,  /* (257) collate ::= */
   -2,  /* (258) collate ::= COLLATE ID|STRING */
   -4,  /* (259) cmd ::= DROP INDEX ifexists fullname */
   -2,  /* (260) cmd ::= VACUUM vinto */
   -3,  /* (261) cmd ::= VACUUM nm vinto */
   -2,  /* (262) vinto ::= INTO expr */
    0,  /* (263) vinto ::= */
   -3,  /* (264) cmd ::= PRAGMA nm dbnm */
   -5,  /* (265) cmd ::= PRAGMA nm dbnm EQ nmnum */
   -6,  /* (266) cmd ::= PRAGMA nm dbnm LP nmnum RP */
   -5,  /* (267) cmd ::= PRAGMA nm dbnm EQ minus_num */
   -6,  /* (268) cmd ::= PRAGMA nm dbnm LP minus_num RP */
   -2,  /* (269) plus_num ::= PLUS INTEGER|FLOAT */
   -2,  /* (270) minus_num ::= MINUS INTEGER|FLOAT */
   -5,  /* (271) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
  -11,  /* (272) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
   -1,  /* (273) trigger_time ::= BEFORE|AFTER */
   -2,  /* (274) trigger_time ::= INSTEAD OF */
    0,  /* (275) trigger_time ::= */
   -1,  /* (276) trigger_event ::= DELETE|INSERT */
   -1,  /* (277) trigger_event ::= UPDATE */
   -3,  /* (278) trigger_event ::= UPDATE OF idlist */
    0,  /* (279) when_clause ::= */
   -2,  /* (280) when_clause ::= WHEN expr */
   -3,  /* (281) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
   -2,  /* (282) trigger_cmd_list ::= trigger_cmd SEMI */
   -3,  /* (283) trnm ::= nm DOT nm */
   -3,  /* (284) tridxby ::= INDEXED BY nm */
   -2,  /* (285) tridxby ::= NOT INDEXED */
   -9,  /* (286) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
   -8,  /* (287) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
   -6,  /* (288) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
   -3,  /* (289) trigger_cmd ::= scanpt select scanpt */
   -4,  /* (290) expr ::= RAISE LP IGNORE RP */
   -6,  /* (291) expr ::= RAISE LP raisetype COMMA expr RP */
   -1,  /* (292) raisetype ::= ROLLBACK */
   -1,  /* (293) raisetype ::= ABORT */
   -1,  /* (294) raisetype ::= FAIL */
   -4,  /* (295) cmd ::= DROP TRIGGER ifexists fullname */
   -6,  /* (296) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
   -3,  /* (297) cmd ::= DETACH database_kw_opt expr */
    0,  /* (298) key_opt ::= */
   -2,  /* (299) key_opt ::= KEY expr */
   -1,  /* (300) cmd ::= REINDEX */
   -3,  /* (301) cmd ::= REINDEX nm dbnm */
   -1,  /* (302) cmd ::= ANALYZE */
   -3,  /* (303) cmd ::= ANALYZE nm dbnm */
   -6,  /* (304) cmd ::= ALTER TABLE fullname RENAME TO nm */
   -7,  /* (305) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
   -6,  /* (306) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
   -1,  /* (307) add_column_fullname ::= fullname */
   -8,  /* (308) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
   -1,  /* (309) cmd ::= create_vtab */
   -4,  /* (310) cmd ::= create_vtab LP vtabarglist RP */
   -8,  /* (311) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
    0,  /* (312) vtabarg ::= */
   -1,  /* (313) vtabargtoken ::= ANY */
   -3,  /* (314) vtabargtoken ::= lp anylist RP */
   -1,  /* (315) lp ::= LP */
   -2,  /* (316) with ::= WITH wqlist */
   -3,  /* (317) with ::= WITH RECURSIVE wqlist */
   -1,  /* (318) wqas ::= AS */
   -2,  /* (319) wqas ::= AS MATERIALIZED */
   -3,  /* (320) wqas ::= AS NOT MATERIALIZED */
   -6,  /* (321) wqitem ::= withnm eidlist_opt wqas LP select RP */
   -1,  /* (322) withnm ::= nm */
   -1,  /* (323) wqlist ::= wqitem */
   -3,  /* (324) wqlist ::= wqlist COMMA wqitem */
   -3,  /* (325) windowdefn_list ::= windowdefn_list COMMA windowdefn */
   -5,  /* (326) windowdefn ::= nm AS LP window RP */
   -5,  /* (327) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
   -6,  /* (328) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
   -4,  /* (329) window ::= ORDER BY sortlist frame_opt */
   -5,  /* (330) window ::= nm ORDER BY sortlist frame_opt */
   -2,  /* (331) window ::= nm frame_opt */
    0,  /* (332) frame_opt ::= */
   -3,  /* (333) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
   -6,  /* (334) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
   -1,  /* (335) range_or_rows ::= RANGE|ROWS|GROUPS */
   -1,  /* (336) frame_bound_s ::= frame_bound */
   -2,  /* (337) frame_bound_s ::= UNBOUNDED PRECEDING */
   -1,  /* (338) frame_bound_e ::= frame_bound */
   -2,  /* (339) frame_bound_e ::= UNBOUNDED FOLLOWING */
   -2,  /* (340) frame_bound ::= expr PRECEDING|FOLLOWING */
   -2,  /* (341) frame_bound ::= CURRENT ROW */
    0,  /* (342) frame_exclude_opt ::= */
   -2,  /* (343) frame_exclude_opt ::= EXCLUDE frame_exclude */
   -2,  /* (344) frame_exclude ::= NO OTHERS */
   -2,  /* (345) frame_exclude ::= CURRENT ROW */
   -1,  /* (346) frame_exclude ::= GROUP|TIES */
   -2,  /* (347) window_clause ::= WINDOW windowdefn_list */
   -2,  /* (348) filter_over ::= filter_clause over_clause */
   -1,  /* (349) filter_over ::= over_clause */
   -1,  /* (350) filter_over ::= filter_clause */
   -4,  /* (351) over_clause ::= OVER LP window RP */
   -2,  /* (352) over_clause ::= OVER nm */
   -5,  /* (353) filter_clause ::= FILTER LP WHERE expr RP */
   -1,  /* (354) term ::= QNUMBER */
   -3,  /* (355) ecmd ::= explain cmdx SEMI */
    0,  /* (356) trans_opt ::= */
   -1,  /* (357) trans_opt ::= TRANSACTION */
   -2,  /* (358) trans_opt ::= TRANSACTION nm */
   -1,  /* (359) savepoint_opt ::= SAVEPOINT */
    0,  /* (360) savepoint_opt ::= */
   -2,  /* (361) cmd ::= create_table create_table_args */
   -1,  /* (362) table_option_set ::= table_option */
   -4,  /* (363) columnlist ::= columnlist COMMA columnname carglist */
   -2,  /* (364) columnlist ::= columnname carglist */
   -1,  /* (365) typetoken ::= typename */
   -1,  /* (366) typename ::= ID|STRING */
   -1,  /* (367) signed ::= plus_num */
   -1,  /* (368) signed ::= minus_num */
   -2,  /* (369) carglist ::= carglist ccons */
    0,  /* (370) carglist ::= */
   -2,  /* (371) ccons ::= NULL onconf */
   -4,  /* (372) ccons ::= GENERATED ALWAYS AS generated */
   -2,  /* (373) ccons ::= AS generated */
   -2,  /* (374) conslist_opt ::= COMMA conslist */
   -3,  /* (375) conslist ::= conslist tconscomma tcons */
   -1,  /* (376) conslist ::= tcons */
    0,  /* (377) tconscomma ::= */
   -1,  /* (378) defer_subclause_opt ::= defer_subclause */
   -1,  /* (379) resolvetype ::= raisetype */
   -1,  /* (380) oneselect ::= values */
   -1,  /* (381) as ::= ID|STRING */
   -1,  /* (382) indexed_opt ::= indexed_by */
    0,  /* (383) returning ::= */
   -1,  /* (384) likeop ::= LIKE_KW|MATCH */
   -1,  /* (385) case_operand ::= expr */
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
#endif /* !_SYNTAQLITE_EXTERNAL_PARSER */

static void yy_accept(syntaqlite_yyParser*);  /* Forward Declaration */

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
  syntaqlite_yyParser *yypParser,         /* The parser */
  unsigned int yyruleno,       /* Number of the rule by which to reduce */
  int yyLookahead,             /* Lookahead token, or YYNOCODE if none */
  syntaqlite_sqlite3ParserTOKENTYPE yyLookaheadToken  /* Value of the lookahead token */
  syntaqlite_sqlite3ParserCTX_PDECL                   /* %extra_context */
){
  int yygoto;                     /* The next state */
  YYACTIONTYPE yyact;             /* The next action */
  syntaqlite_yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  syntaqlite_sqlite3ParserARG_FETCH
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
      case 0: /* cmdx ::= cmd */
      case 2: /* cmd ::= select */ yytestcase(yyruleno==2);
      case 3: /* select ::= selectnowith */ yytestcase(yyruleno==3);
      case 61: /* cmdlist ::= ecmd */ yytestcase(yyruleno==61);
      case 66: /* selectnowith ::= oneselect */ yytestcase(yyruleno==66);
      case 68: /* expr ::= term */ yytestcase(yyruleno==68);
      case 69: /* exprlist ::= nexprlist */ yytestcase(yyruleno==69);
{
    yylhsminor.yy9 = yymsp[0].minor.yy9;
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 1: /* scanpt ::= */
      case 13: /* as ::= */ yytestcase(yyruleno==13);
{
    yymsp[1].minor.yy0.z = NULL; yymsp[1].minor.yy0.n = 0;
}
        break;
      case 4: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
{
    yymsp[-8].minor.yy9 = ast_select_stmt(pCtx->astCtx, (uint8_t)yymsp[-7].minor.yy9, yymsp[-6].minor.yy9, yymsp[-4].minor.yy9, yymsp[-3].minor.yy9, yymsp[-2].minor.yy9, yymsp[-1].minor.yy9, yymsp[0].minor.yy9);
}
        break;
      case 5: /* distinct ::= DISTINCT */
      case 21: /* sortorder ::= DESC */ yytestcase(yyruleno==21);
{
    yymsp[0].minor.yy9 = 1;
}
        break;
      case 6: /* distinct ::= ALL */
      case 20: /* sortorder ::= ASC */ yytestcase(yyruleno==20);
{
    yymsp[0].minor.yy9 = 0;
}
        break;
      case 7: /* distinct ::= */
      case 22: /* sortorder ::= */ yytestcase(yyruleno==22);
      case 25: /* nulls ::= */ yytestcase(yyruleno==25);
{
    yymsp[1].minor.yy9 = 0;
}
        break;
      case 8: /* sclp ::= */
      case 14: /* from ::= */ yytestcase(yyruleno==14);
      case 16: /* orderby_opt ::= */ yytestcase(yyruleno==16);
      case 26: /* groupby_opt ::= */ yytestcase(yyruleno==26);
      case 28: /* having_opt ::= */ yytestcase(yyruleno==28);
      case 30: /* limit_opt ::= */ yytestcase(yyruleno==30);
      case 34: /* where_opt ::= */ yytestcase(yyruleno==34);
      case 56: /* exprlist ::= */ yytestcase(yyruleno==56);
{
    yymsp[1].minor.yy9 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 9: /* selcollist ::= sclp scanpt expr scanpt as */
{
    uint32_t col = ast_result_column(pCtx->astCtx, 0, SYNTAQLITE_NO_SPAN, yymsp[-2].minor.yy9);
    yylhsminor.yy9 = ast_result_column_list_append(pCtx->astCtx, yymsp[-4].minor.yy9, col);
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 10: /* selcollist ::= sclp scanpt STAR */
{
    uint32_t col = ast_result_column(pCtx->astCtx, 1, SYNTAQLITE_NO_SPAN, SYNTAQLITE_NULL_NODE);
    yylhsminor.yy9 = ast_result_column_list_append(pCtx->astCtx, yymsp[-2].minor.yy9, col);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 11: /* selcollist ::= sclp scanpt nm DOT STAR */
{
    uint32_t col = ast_result_column(pCtx->astCtx, 1, syntaqlite_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NULL_NODE);
    yylhsminor.yy9 = ast_result_column_list_append(pCtx->astCtx, yymsp[-4].minor.yy9, col);
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 12: /* as ::= AS nm */
{
    yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;
}
        break;
      case 15: /* from ::= FROM seltablist */
      case 29: /* having_opt ::= HAVING expr */ yytestcase(yyruleno==29);
      case 35: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==35);
{
    yymsp[-1].minor.yy9 = yymsp[0].minor.yy9;
}
        break;
      case 17: /* orderby_opt ::= ORDER BY sortlist */
      case 27: /* groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==27);
{
    yymsp[-2].minor.yy9 = yymsp[0].minor.yy9;
}
        break;
      case 18: /* sortlist ::= sortlist COMMA expr sortorder nulls */
{
    uint32_t term = ast_ordering_term(pCtx->astCtx, yymsp[-2].minor.yy9, (uint8_t)yymsp[-1].minor.yy9, (uint8_t)yymsp[0].minor.yy9);
    yylhsminor.yy9 = ast_order_by_list_append(pCtx->astCtx, yymsp[-4].minor.yy9, term);
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 19: /* sortlist ::= expr sortorder nulls */
{
    uint32_t term = ast_ordering_term(pCtx->astCtx, yymsp[-2].minor.yy9, (uint8_t)yymsp[-1].minor.yy9, (uint8_t)yymsp[0].minor.yy9);
    yylhsminor.yy9 = ast_order_by_list(pCtx->astCtx, term);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 23: /* nulls ::= NULLS FIRST */
{
    yymsp[-1].minor.yy9 = 1;
}
        break;
      case 24: /* nulls ::= NULLS LAST */
{
    yymsp[-1].minor.yy9 = 2;
}
        break;
      case 31: /* limit_opt ::= LIMIT expr */
{
    yymsp[-1].minor.yy9 = ast_limit_clause(pCtx->astCtx, yymsp[0].minor.yy9, SYNTAQLITE_NULL_NODE);
}
        break;
      case 32: /* limit_opt ::= LIMIT expr OFFSET expr */
{
    yymsp[-3].minor.yy9 = ast_limit_clause(pCtx->astCtx, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
        break;
      case 33: /* limit_opt ::= LIMIT expr COMMA expr */
{
    yymsp[-3].minor.yy9 = ast_limit_clause(pCtx->astCtx, yymsp[0].minor.yy9, yymsp[-2].minor.yy9);
}
        break;
      case 36: /* expr ::= LP expr RP */
{
    yymsp[-2].minor.yy9 = yymsp[-1].minor.yy9;
}
        break;
      case 37: /* expr ::= ID|INDEXED|JOIN_KW */
{
    yylhsminor.yy9 = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NO_SPAN);
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 38: /* expr ::= nm DOT nm */
{
    yylhsminor.yy9 = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0),
        SYNTAQLITE_NO_SPAN);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 39: /* expr ::= nm DOT nm DOT nm */
{
    yylhsminor.yy9 = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-4].minor.yy0));
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 40: /* term ::= NULL|FLOAT|BLOB */
{
    SyntaqliteLiteralType lit_type;
    switch (yymsp[0].minor.yy0.type) {
        case TK_NULL:  lit_type = SYNTAQLITE_LITERAL_TYPE_NULL; break;
        case TK_FLOAT: lit_type = SYNTAQLITE_LITERAL_TYPE_FLOAT; break;
        case TK_BLOB:  lit_type = SYNTAQLITE_LITERAL_TYPE_BLOB; break;
        default:       lit_type = SYNTAQLITE_LITERAL_TYPE_NULL; break;
    }
    yylhsminor.yy9 = ast_literal(pCtx->astCtx, lit_type, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 41: /* term ::= STRING */
{
    yylhsminor.yy9 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_STRING, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 42: /* term ::= INTEGER */
{
    yylhsminor.yy9 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_INTEGER, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 43: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
{
    yylhsminor.yy9 = ast_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-4].minor.yy0),
        (uint8_t)yymsp[-2].minor.yy9,
        yymsp[-1].minor.yy9);
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 44: /* expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
{
    yylhsminor.yy9 = ast_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-3].minor.yy0),
        2,
        SYNTAQLITE_NULL_NODE);
}
  yymsp[-3].minor.yy9 = yylhsminor.yy9;
        break;
      case 45: /* expr ::= expr AND expr */
{
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_AND, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 46: /* expr ::= expr OR expr */
{
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_OR, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 47: /* expr ::= expr LT|GT|GE|LE expr */
{
    SyntaqliteBinaryOp op;
    switch (yymsp[-1].minor.yy0.type) {
        case TK_LT: op = SYNTAQLITE_BINARY_OP_LT; break;
        case TK_GT: op = SYNTAQLITE_BINARY_OP_GT; break;
        case TK_LE: op = SYNTAQLITE_BINARY_OP_LE; break;
        default:    op = SYNTAQLITE_BINARY_OP_GE; break;
    }
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 48: /* expr ::= expr EQ|NE expr */
{
    SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == TK_EQ) ? SYNTAQLITE_BINARY_OP_EQ : SYNTAQLITE_BINARY_OP_NE;
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 49: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
{
    SyntaqliteBinaryOp op;
    switch (yymsp[-1].minor.yy0.type) {
        case TK_BITAND: op = SYNTAQLITE_BINARY_OP_BITAND; break;
        case TK_BITOR:  op = SYNTAQLITE_BINARY_OP_BITOR; break;
        case TK_LSHIFT: op = SYNTAQLITE_BINARY_OP_LSHIFT; break;
        default:        op = SYNTAQLITE_BINARY_OP_RSHIFT; break;
    }
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 50: /* expr ::= expr PLUS|MINUS expr */
{
    SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == TK_PLUS) ? SYNTAQLITE_BINARY_OP_PLUS : SYNTAQLITE_BINARY_OP_MINUS;
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 51: /* expr ::= expr STAR|SLASH|REM expr */
{
    SyntaqliteBinaryOp op;
    switch (yymsp[-1].minor.yy0.type) {
        case TK_STAR:  op = SYNTAQLITE_BINARY_OP_STAR; break;
        case TK_SLASH: op = SYNTAQLITE_BINARY_OP_SLASH; break;
        default:       op = SYNTAQLITE_BINARY_OP_REM; break;
    }
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 52: /* expr ::= expr CONCAT expr */
{
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_CONCAT, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 53: /* expr ::= NOT expr */
{
    yymsp[-1].minor.yy9 = ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_NOT, yymsp[0].minor.yy9);
}
        break;
      case 54: /* expr ::= BITNOT expr */
{
    yymsp[-1].minor.yy9 = ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_BITNOT, yymsp[0].minor.yy9);
}
        break;
      case 55: /* expr ::= PLUS|MINUS expr */
{
    SyntaqliteUnaryOp op = (yymsp[-1].minor.yy0.type == TK_MINUS) ? SYNTAQLITE_UNARY_OP_MINUS : SYNTAQLITE_UNARY_OP_PLUS;
    yylhsminor.yy9 = ast_unary_expr(pCtx->astCtx, op, yymsp[0].minor.yy9);
}
  yymsp[-1].minor.yy9 = yylhsminor.yy9;
        break;
      case 57: /* nexprlist ::= nexprlist COMMA expr */
{
    yylhsminor.yy9 = ast_expr_list_append(pCtx->astCtx, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 58: /* nexprlist ::= expr */
{
    yylhsminor.yy9 = ast_expr_list(pCtx->astCtx, yymsp[0].minor.yy9);
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 59: /* input ::= cmdlist */
{
    pCtx->root = yymsp[0].minor.yy9;
}
        break;
      case 60: /* cmdlist ::= cmdlist ecmd */
{
    yymsp[-1].minor.yy9 = yymsp[0].minor.yy9;  // Just use the last command for now
}
        break;
      case 62: /* ecmd ::= SEMI */
{
    yymsp[0].minor.yy9 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 63: /* ecmd ::= cmdx SEMI */
      case 67: /* sclp ::= selcollist COMMA */ yytestcase(yyruleno==67);
{
    yylhsminor.yy9 = yymsp[-1].minor.yy9;
}
  yymsp[-1].minor.yy9 = yylhsminor.yy9;
        break;
      case 64: /* nm ::= ID|INDEXED|JOIN_KW */
      case 65: /* nm ::= STRING */ yytestcase(yyruleno==65);
{
    yylhsminor.yy0 = yymsp[0].minor.yy0;
}
  yymsp[0].minor.yy0 = yylhsminor.yy0;
        break;
      default:
      /* (70) explain ::= EXPLAIN */ yytestcase(yyruleno==70);
      /* (71) explain ::= EXPLAIN QUERY PLAN */ yytestcase(yyruleno==71);
      /* (72) cmd ::= BEGIN transtype trans_opt */ yytestcase(yyruleno==72);
      /* (73) transtype ::= */ yytestcase(yyruleno==73);
      /* (74) transtype ::= DEFERRED */ yytestcase(yyruleno==74);
      /* (75) transtype ::= IMMEDIATE */ yytestcase(yyruleno==75);
      /* (76) transtype ::= EXCLUSIVE */ yytestcase(yyruleno==76);
      /* (77) cmd ::= COMMIT|END trans_opt */ yytestcase(yyruleno==77);
      /* (78) cmd ::= ROLLBACK trans_opt */ yytestcase(yyruleno==78);
      /* (79) cmd ::= SAVEPOINT nm */ yytestcase(yyruleno==79);
      /* (80) cmd ::= RELEASE savepoint_opt nm */ yytestcase(yyruleno==80);
      /* (81) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */ yytestcase(yyruleno==81);
      /* (82) create_table ::= createkw temp TABLE ifnotexists nm dbnm */ yytestcase(yyruleno==82);
      /* (83) createkw ::= CREATE */ yytestcase(yyruleno==83);
      /* (84) ifnotexists ::= */ yytestcase(yyruleno==84);
      /* (85) ifnotexists ::= IF NOT EXISTS */ yytestcase(yyruleno==85);
      /* (86) temp ::= TEMP */ yytestcase(yyruleno==86);
      /* (87) temp ::= */ yytestcase(yyruleno==87);
      /* (88) create_table_args ::= LP columnlist conslist_opt RP table_option_set */ yytestcase(yyruleno==88);
      /* (89) create_table_args ::= AS select */ yytestcase(yyruleno==89);
      /* (90) table_option_set ::= */ yytestcase(yyruleno==90);
      /* (91) table_option_set ::= table_option_set COMMA table_option */ yytestcase(yyruleno==91);
      /* (92) table_option ::= WITHOUT nm */ yytestcase(yyruleno==92);
      /* (93) table_option ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=93);
      /* (94) columnname ::= nm typetoken */ yytestcase(yyruleno==94);
      /* (95) typetoken ::= */ yytestcase(yyruleno==95);
      /* (96) typetoken ::= typename LP signed RP */ yytestcase(yyruleno==96);
      /* (97) typetoken ::= typename LP signed COMMA signed RP */ yytestcase(yyruleno==97);
      /* (98) typename ::= typename ID|STRING */ yytestcase(yyruleno==98);
      /* (99) scantok ::= */ yytestcase(yyruleno==99);
      /* (100) ccons ::= CONSTRAINT nm */ yytestcase(yyruleno==100);
      /* (101) ccons ::= DEFAULT scantok term */ yytestcase(yyruleno==101);
      /* (102) ccons ::= DEFAULT LP expr RP */ yytestcase(yyruleno==102);
      /* (103) ccons ::= DEFAULT PLUS scantok term */ yytestcase(yyruleno==103);
      /* (104) ccons ::= DEFAULT MINUS scantok term */ yytestcase(yyruleno==104);
      /* (105) ccons ::= DEFAULT scantok ID|INDEXED */ yytestcase(yyruleno==105);
      /* (106) ccons ::= NOT NULL onconf */ yytestcase(yyruleno==106);
      /* (107) ccons ::= PRIMARY KEY sortorder onconf autoinc */ yytestcase(yyruleno==107);
      /* (108) ccons ::= UNIQUE onconf */ yytestcase(yyruleno==108);
      /* (109) ccons ::= CHECK LP expr RP */ yytestcase(yyruleno==109);
      /* (110) ccons ::= REFERENCES nm eidlist_opt refargs */ yytestcase(yyruleno==110);
      /* (111) ccons ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=111);
      /* (112) ccons ::= COLLATE ID|STRING */ yytestcase(yyruleno==112);
      /* (113) generated ::= LP expr RP */ yytestcase(yyruleno==113);
      /* (114) generated ::= LP expr RP ID */ yytestcase(yyruleno==114);
      /* (115) autoinc ::= */ yytestcase(yyruleno==115);
      /* (116) autoinc ::= AUTOINCR */ yytestcase(yyruleno==116);
      /* (117) refargs ::= */ yytestcase(yyruleno==117);
      /* (118) refargs ::= refargs refarg */ yytestcase(yyruleno==118);
      /* (119) refarg ::= MATCH nm */ yytestcase(yyruleno==119);
      /* (120) refarg ::= ON INSERT refact */ yytestcase(yyruleno==120);
      /* (121) refarg ::= ON DELETE refact */ yytestcase(yyruleno==121);
      /* (122) refarg ::= ON UPDATE refact */ yytestcase(yyruleno==122);
      /* (123) refact ::= SET NULL */ yytestcase(yyruleno==123);
      /* (124) refact ::= SET DEFAULT */ yytestcase(yyruleno==124);
      /* (125) refact ::= CASCADE */ yytestcase(yyruleno==125);
      /* (126) refact ::= RESTRICT */ yytestcase(yyruleno==126);
      /* (127) refact ::= NO ACTION */ yytestcase(yyruleno==127);
      /* (128) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */ yytestcase(yyruleno==128);
      /* (129) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */ yytestcase(yyruleno==129);
      /* (130) init_deferred_pred_opt ::= */ yytestcase(yyruleno==130);
      /* (131) init_deferred_pred_opt ::= INITIALLY DEFERRED */ yytestcase(yyruleno==131);
      /* (132) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */ yytestcase(yyruleno==132);
      /* (133) conslist_opt ::= */ yytestcase(yyruleno==133);
      /* (134) tconscomma ::= COMMA */ yytestcase(yyruleno==134);
      /* (135) tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==135);
      /* (136) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */ yytestcase(yyruleno==136);
      /* (137) tcons ::= UNIQUE LP sortlist RP onconf */ yytestcase(yyruleno==137);
      /* (138) tcons ::= CHECK LP expr RP onconf */ yytestcase(yyruleno==138);
      /* (139) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */ yytestcase(yyruleno==139);
      /* (140) defer_subclause_opt ::= */ yytestcase(yyruleno==140);
      /* (141) onconf ::= */ yytestcase(yyruleno==141);
      /* (142) onconf ::= ON CONFLICT resolvetype */ yytestcase(yyruleno==142);
      /* (143) orconf ::= */ yytestcase(yyruleno==143);
      /* (144) orconf ::= OR resolvetype */ yytestcase(yyruleno==144);
      /* (145) resolvetype ::= IGNORE */ yytestcase(yyruleno==145);
      /* (146) resolvetype ::= REPLACE */ yytestcase(yyruleno==146);
      /* (147) cmd ::= DROP TABLE ifexists fullname */ yytestcase(yyruleno==147);
      /* (148) ifexists ::= IF EXISTS */ yytestcase(yyruleno==148);
      /* (149) ifexists ::= */ yytestcase(yyruleno==149);
      /* (150) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */ yytestcase(yyruleno==150);
      /* (151) cmd ::= DROP VIEW ifexists fullname */ yytestcase(yyruleno==151);
      /* (152) select ::= WITH wqlist selectnowith */ yytestcase(yyruleno==152);
      /* (153) select ::= WITH RECURSIVE wqlist selectnowith */ yytestcase(yyruleno==153);
      /* (154) selectnowith ::= selectnowith multiselect_op oneselect */ yytestcase(yyruleno==154);
      /* (155) multiselect_op ::= UNION */ yytestcase(yyruleno==155);
      /* (156) multiselect_op ::= UNION ALL */ yytestcase(yyruleno==156);
      /* (157) multiselect_op ::= EXCEPT|INTERSECT */ yytestcase(yyruleno==157);
      /* (158) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */ yytestcase(yyruleno==158);
      /* (159) values ::= VALUES LP nexprlist RP */ yytestcase(yyruleno==159);
      /* (160) oneselect ::= mvalues */ yytestcase(yyruleno==160);
      /* (161) mvalues ::= values COMMA LP nexprlist RP */ yytestcase(yyruleno==161);
      /* (162) mvalues ::= mvalues COMMA LP nexprlist RP */ yytestcase(yyruleno==162);
      /* (163) stl_prefix ::= seltablist joinop */ yytestcase(yyruleno==163);
      /* (164) stl_prefix ::= */ yytestcase(yyruleno==164);
      /* (165) seltablist ::= stl_prefix nm dbnm as on_using */ yytestcase(yyruleno==165);
      /* (166) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */ yytestcase(yyruleno==166);
      /* (167) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */ yytestcase(yyruleno==167);
      /* (168) seltablist ::= stl_prefix LP select RP as on_using */ yytestcase(yyruleno==168);
      /* (169) seltablist ::= stl_prefix LP seltablist RP as on_using */ yytestcase(yyruleno==169);
      /* (170) dbnm ::= */ yytestcase(yyruleno==170);
      /* (171) dbnm ::= DOT nm */ yytestcase(yyruleno==171);
      /* (172) fullname ::= nm */ yytestcase(yyruleno==172);
      /* (173) fullname ::= nm DOT nm */ yytestcase(yyruleno==173);
      /* (174) xfullname ::= nm */ yytestcase(yyruleno==174);
      /* (175) xfullname ::= nm DOT nm */ yytestcase(yyruleno==175);
      /* (176) xfullname ::= nm DOT nm AS nm */ yytestcase(yyruleno==176);
      /* (177) xfullname ::= nm AS nm */ yytestcase(yyruleno==177);
      /* (178) joinop ::= COMMA|JOIN */ yytestcase(yyruleno==178);
      /* (179) joinop ::= JOIN_KW JOIN */ yytestcase(yyruleno==179);
      /* (180) joinop ::= JOIN_KW nm JOIN */ yytestcase(yyruleno==180);
      /* (181) joinop ::= JOIN_KW nm nm JOIN */ yytestcase(yyruleno==181);
      /* (182) on_using ::= ON expr */ yytestcase(yyruleno==182);
      /* (183) on_using ::= USING LP idlist RP */ yytestcase(yyruleno==183);
      /* (184) on_using ::= */ yytestcase(yyruleno==184);
      /* (185) indexed_opt ::= */ yytestcase(yyruleno==185);
      /* (186) indexed_by ::= INDEXED BY nm */ yytestcase(yyruleno==186);
      /* (187) indexed_by ::= NOT INDEXED */ yytestcase(yyruleno==187);
      /* (188) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */ yytestcase(yyruleno==188);
      /* (189) where_opt_ret ::= */ yytestcase(yyruleno==189);
      /* (190) where_opt_ret ::= WHERE expr */ yytestcase(yyruleno==190);
      /* (191) where_opt_ret ::= RETURNING selcollist */ yytestcase(yyruleno==191);
      /* (192) where_opt_ret ::= WHERE expr RETURNING selcollist */ yytestcase(yyruleno==192);
      /* (193) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */ yytestcase(yyruleno==193);
      /* (194) setlist ::= setlist COMMA nm EQ expr */ yytestcase(yyruleno==194);
      /* (195) setlist ::= setlist COMMA LP idlist RP EQ expr */ yytestcase(yyruleno==195);
      /* (196) setlist ::= nm EQ expr */ yytestcase(yyruleno==196);
      /* (197) setlist ::= LP idlist RP EQ expr */ yytestcase(yyruleno==197);
      /* (198) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */ yytestcase(yyruleno==198);
      /* (199) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */ yytestcase(yyruleno==199);
      /* (200) upsert ::= */ yytestcase(yyruleno==200);
      /* (201) upsert ::= RETURNING selcollist */ yytestcase(yyruleno==201);
      /* (202) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */ yytestcase(yyruleno==202);
      /* (203) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */ yytestcase(yyruleno==203);
      /* (204) upsert ::= ON CONFLICT DO NOTHING returning */ yytestcase(yyruleno==204);
      /* (205) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */ yytestcase(yyruleno==205);
      /* (206) returning ::= RETURNING selcollist */ yytestcase(yyruleno==206);
      /* (207) insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==207);
      /* (208) insert_cmd ::= REPLACE */ yytestcase(yyruleno==208);
      /* (209) idlist_opt ::= */ yytestcase(yyruleno==209);
      /* (210) idlist_opt ::= LP idlist RP */ yytestcase(yyruleno==210);
      /* (211) idlist ::= idlist COMMA nm */ yytestcase(yyruleno==211);
      /* (212) idlist ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=212);
      /* (213) expr ::= VARIABLE */ yytestcase(yyruleno==213);
      /* (214) expr ::= expr COLLATE ID|STRING */ yytestcase(yyruleno==214);
      /* (215) expr ::= CAST LP expr AS typetoken RP */ yytestcase(yyruleno==215);
      /* (216) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */ yytestcase(yyruleno==216);
      /* (217) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */ yytestcase(yyruleno==217);
      /* (218) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */ yytestcase(yyruleno==218);
      /* (219) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */ yytestcase(yyruleno==219);
      /* (220) term ::= CTIME_KW */ yytestcase(yyruleno==220);
      /* (221) expr ::= LP nexprlist COMMA expr RP */ yytestcase(yyruleno==221);
      /* (222) likeop ::= NOT LIKE_KW|MATCH */ yytestcase(yyruleno==222);
      /* (223) expr ::= expr likeop expr */ yytestcase(yyruleno==223);
      /* (224) expr ::= expr likeop expr ESCAPE expr */ yytestcase(yyruleno==224);
      /* (225) expr ::= expr ISNULL|NOTNULL */ yytestcase(yyruleno==225);
      /* (226) expr ::= expr NOT NULL */ yytestcase(yyruleno==226);
      /* (227) expr ::= expr IS expr */ yytestcase(yyruleno==227);
      /* (228) expr ::= expr IS NOT expr */ yytestcase(yyruleno==228);
      /* (229) expr ::= expr IS NOT DISTINCT FROM expr */ yytestcase(yyruleno==229);
      /* (230) expr ::= expr IS DISTINCT FROM expr */ yytestcase(yyruleno==230);
      /* (231) expr ::= expr PTR expr */ yytestcase(yyruleno==231);
      /* (232) between_op ::= BETWEEN */ yytestcase(yyruleno==232);
      /* (233) between_op ::= NOT BETWEEN */ yytestcase(yyruleno==233);
      /* (234) expr ::= expr between_op expr AND expr */ yytestcase(yyruleno==234);
      /* (235) in_op ::= IN */ yytestcase(yyruleno==235);
      /* (236) in_op ::= NOT IN */ yytestcase(yyruleno==236);
      /* (237) expr ::= expr in_op LP exprlist RP */ yytestcase(yyruleno==237);
      /* (238) expr ::= LP select RP */ yytestcase(yyruleno==238);
      /* (239) expr ::= expr in_op LP select RP */ yytestcase(yyruleno==239);
      /* (240) expr ::= expr in_op nm dbnm paren_exprlist */ yytestcase(yyruleno==240);
      /* (241) expr ::= EXISTS LP select RP */ yytestcase(yyruleno==241);
      /* (242) expr ::= CASE case_operand case_exprlist case_else END */ yytestcase(yyruleno==242);
      /* (243) case_exprlist ::= case_exprlist WHEN expr THEN expr */ yytestcase(yyruleno==243);
      /* (244) case_exprlist ::= WHEN expr THEN expr */ yytestcase(yyruleno==244);
      /* (245) case_else ::= ELSE expr */ yytestcase(yyruleno==245);
      /* (246) case_else ::= */ yytestcase(yyruleno==246);
      /* (247) case_operand ::= */ yytestcase(yyruleno==247);
      /* (248) paren_exprlist ::= */ yytestcase(yyruleno==248);
      /* (249) paren_exprlist ::= LP exprlist RP */ yytestcase(yyruleno==249);
      /* (250) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */ yytestcase(yyruleno==250);
      /* (251) uniqueflag ::= UNIQUE */ yytestcase(yyruleno==251);
      /* (252) uniqueflag ::= */ yytestcase(yyruleno==252);
      /* (253) eidlist_opt ::= */ yytestcase(yyruleno==253);
      /* (254) eidlist_opt ::= LP eidlist RP */ yytestcase(yyruleno==254);
      /* (255) eidlist ::= eidlist COMMA nm collate sortorder */ yytestcase(yyruleno==255);
      /* (256) eidlist ::= nm collate sortorder */ yytestcase(yyruleno==256);
      /* (257) collate ::= */ yytestcase(yyruleno==257);
      /* (258) collate ::= COLLATE ID|STRING */ yytestcase(yyruleno==258);
      /* (259) cmd ::= DROP INDEX ifexists fullname */ yytestcase(yyruleno==259);
      /* (260) cmd ::= VACUUM vinto */ yytestcase(yyruleno==260);
      /* (261) cmd ::= VACUUM nm vinto */ yytestcase(yyruleno==261);
      /* (262) vinto ::= INTO expr */ yytestcase(yyruleno==262);
      /* (263) vinto ::= */ yytestcase(yyruleno==263);
      /* (264) cmd ::= PRAGMA nm dbnm */ yytestcase(yyruleno==264);
      /* (265) cmd ::= PRAGMA nm dbnm EQ nmnum */ yytestcase(yyruleno==265);
      /* (266) cmd ::= PRAGMA nm dbnm LP nmnum RP */ yytestcase(yyruleno==266);
      /* (267) cmd ::= PRAGMA nm dbnm EQ minus_num */ yytestcase(yyruleno==267);
      /* (268) cmd ::= PRAGMA nm dbnm LP minus_num RP */ yytestcase(yyruleno==268);
      /* (269) plus_num ::= PLUS INTEGER|FLOAT */ yytestcase(yyruleno==269);
      /* (270) minus_num ::= MINUS INTEGER|FLOAT */ yytestcase(yyruleno==270);
      /* (271) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */ yytestcase(yyruleno==271);
      /* (272) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */ yytestcase(yyruleno==272);
      /* (273) trigger_time ::= BEFORE|AFTER */ yytestcase(yyruleno==273);
      /* (274) trigger_time ::= INSTEAD OF */ yytestcase(yyruleno==274);
      /* (275) trigger_time ::= */ yytestcase(yyruleno==275);
      /* (276) trigger_event ::= DELETE|INSERT */ yytestcase(yyruleno==276);
      /* (277) trigger_event ::= UPDATE */ yytestcase(yyruleno==277);
      /* (278) trigger_event ::= UPDATE OF idlist */ yytestcase(yyruleno==278);
      /* (279) when_clause ::= */ yytestcase(yyruleno==279);
      /* (280) when_clause ::= WHEN expr */ yytestcase(yyruleno==280);
      /* (281) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */ yytestcase(yyruleno==281);
      /* (282) trigger_cmd_list ::= trigger_cmd SEMI */ yytestcase(yyruleno==282);
      /* (283) trnm ::= nm DOT nm */ yytestcase(yyruleno==283);
      /* (284) tridxby ::= INDEXED BY nm */ yytestcase(yyruleno==284);
      /* (285) tridxby ::= NOT INDEXED */ yytestcase(yyruleno==285);
      /* (286) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */ yytestcase(yyruleno==286);
      /* (287) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */ yytestcase(yyruleno==287);
      /* (288) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */ yytestcase(yyruleno==288);
      /* (289) trigger_cmd ::= scanpt select scanpt */ yytestcase(yyruleno==289);
      /* (290) expr ::= RAISE LP IGNORE RP */ yytestcase(yyruleno==290);
      /* (291) expr ::= RAISE LP raisetype COMMA expr RP */ yytestcase(yyruleno==291);
      /* (292) raisetype ::= ROLLBACK */ yytestcase(yyruleno==292);
      /* (293) raisetype ::= ABORT */ yytestcase(yyruleno==293);
      /* (294) raisetype ::= FAIL */ yytestcase(yyruleno==294);
      /* (295) cmd ::= DROP TRIGGER ifexists fullname */ yytestcase(yyruleno==295);
      /* (296) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */ yytestcase(yyruleno==296);
      /* (297) cmd ::= DETACH database_kw_opt expr */ yytestcase(yyruleno==297);
      /* (298) key_opt ::= */ yytestcase(yyruleno==298);
      /* (299) key_opt ::= KEY expr */ yytestcase(yyruleno==299);
      /* (300) cmd ::= REINDEX */ yytestcase(yyruleno==300);
      /* (301) cmd ::= REINDEX nm dbnm */ yytestcase(yyruleno==301);
      /* (302) cmd ::= ANALYZE */ yytestcase(yyruleno==302);
      /* (303) cmd ::= ANALYZE nm dbnm */ yytestcase(yyruleno==303);
      /* (304) cmd ::= ALTER TABLE fullname RENAME TO nm */ yytestcase(yyruleno==304);
      /* (305) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */ yytestcase(yyruleno==305);
      /* (306) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */ yytestcase(yyruleno==306);
      /* (307) add_column_fullname ::= fullname */ yytestcase(yyruleno==307);
      /* (308) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */ yytestcase(yyruleno==308);
      /* (309) cmd ::= create_vtab */ yytestcase(yyruleno==309);
      /* (310) cmd ::= create_vtab LP vtabarglist RP */ yytestcase(yyruleno==310);
      /* (311) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */ yytestcase(yyruleno==311);
      /* (312) vtabarg ::= */ yytestcase(yyruleno==312);
      /* (313) vtabargtoken ::= ANY */ yytestcase(yyruleno==313);
      /* (314) vtabargtoken ::= lp anylist RP */ yytestcase(yyruleno==314);
      /* (315) lp ::= LP */ yytestcase(yyruleno==315);
      /* (316) with ::= WITH wqlist */ yytestcase(yyruleno==316);
      /* (317) with ::= WITH RECURSIVE wqlist */ yytestcase(yyruleno==317);
      /* (318) wqas ::= AS */ yytestcase(yyruleno==318);
      /* (319) wqas ::= AS MATERIALIZED */ yytestcase(yyruleno==319);
      /* (320) wqas ::= AS NOT MATERIALIZED */ yytestcase(yyruleno==320);
      /* (321) wqitem ::= withnm eidlist_opt wqas LP select RP */ yytestcase(yyruleno==321);
      /* (322) withnm ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=322);
      /* (323) wqlist ::= wqitem (OPTIMIZED OUT) */ assert(yyruleno!=323);
      /* (324) wqlist ::= wqlist COMMA wqitem */ yytestcase(yyruleno==324);
      /* (325) windowdefn_list ::= windowdefn_list COMMA windowdefn */ yytestcase(yyruleno==325);
      /* (326) windowdefn ::= nm AS LP window RP */ yytestcase(yyruleno==326);
      /* (327) window ::= PARTITION BY nexprlist orderby_opt frame_opt */ yytestcase(yyruleno==327);
      /* (328) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */ yytestcase(yyruleno==328);
      /* (329) window ::= ORDER BY sortlist frame_opt */ yytestcase(yyruleno==329);
      /* (330) window ::= nm ORDER BY sortlist frame_opt */ yytestcase(yyruleno==330);
      /* (331) window ::= nm frame_opt */ yytestcase(yyruleno==331);
      /* (332) frame_opt ::= */ yytestcase(yyruleno==332);
      /* (333) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */ yytestcase(yyruleno==333);
      /* (334) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */ yytestcase(yyruleno==334);
      /* (335) range_or_rows ::= RANGE|ROWS|GROUPS */ yytestcase(yyruleno==335);
      /* (336) frame_bound_s ::= frame_bound (OPTIMIZED OUT) */ assert(yyruleno!=336);
      /* (337) frame_bound_s ::= UNBOUNDED PRECEDING */ yytestcase(yyruleno==337);
      /* (338) frame_bound_e ::= frame_bound (OPTIMIZED OUT) */ assert(yyruleno!=338);
      /* (339) frame_bound_e ::= UNBOUNDED FOLLOWING */ yytestcase(yyruleno==339);
      /* (340) frame_bound ::= expr PRECEDING|FOLLOWING */ yytestcase(yyruleno==340);
      /* (341) frame_bound ::= CURRENT ROW */ yytestcase(yyruleno==341);
      /* (342) frame_exclude_opt ::= */ yytestcase(yyruleno==342);
      /* (343) frame_exclude_opt ::= EXCLUDE frame_exclude */ yytestcase(yyruleno==343);
      /* (344) frame_exclude ::= NO OTHERS */ yytestcase(yyruleno==344);
      /* (345) frame_exclude ::= CURRENT ROW */ yytestcase(yyruleno==345);
      /* (346) frame_exclude ::= GROUP|TIES */ yytestcase(yyruleno==346);
      /* (347) window_clause ::= WINDOW windowdefn_list */ yytestcase(yyruleno==347);
      /* (348) filter_over ::= filter_clause over_clause */ yytestcase(yyruleno==348);
      /* (349) filter_over ::= over_clause (OPTIMIZED OUT) */ assert(yyruleno!=349);
      /* (350) filter_over ::= filter_clause */ yytestcase(yyruleno==350);
      /* (351) over_clause ::= OVER LP window RP */ yytestcase(yyruleno==351);
      /* (352) over_clause ::= OVER nm */ yytestcase(yyruleno==352);
      /* (353) filter_clause ::= FILTER LP WHERE expr RP */ yytestcase(yyruleno==353);
      /* (354) term ::= QNUMBER */ yytestcase(yyruleno==354);
      /* (355) ecmd ::= explain cmdx SEMI */ yytestcase(yyruleno==355);
      /* (356) trans_opt ::= */ yytestcase(yyruleno==356);
      /* (357) trans_opt ::= TRANSACTION */ yytestcase(yyruleno==357);
      /* (358) trans_opt ::= TRANSACTION nm */ yytestcase(yyruleno==358);
      /* (359) savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==359);
      /* (360) savepoint_opt ::= */ yytestcase(yyruleno==360);
      /* (361) cmd ::= create_table create_table_args */ yytestcase(yyruleno==361);
      /* (362) table_option_set ::= table_option (OPTIMIZED OUT) */ assert(yyruleno!=362);
      /* (363) columnlist ::= columnlist COMMA columnname carglist */ yytestcase(yyruleno==363);
      /* (364) columnlist ::= columnname carglist */ yytestcase(yyruleno==364);
      /* (365) typetoken ::= typename */ yytestcase(yyruleno==365);
      /* (366) typename ::= ID|STRING */ yytestcase(yyruleno==366);
      /* (367) signed ::= plus_num (OPTIMIZED OUT) */ assert(yyruleno!=367);
      /* (368) signed ::= minus_num (OPTIMIZED OUT) */ assert(yyruleno!=368);
      /* (369) carglist ::= carglist ccons */ yytestcase(yyruleno==369);
      /* (370) carglist ::= */ yytestcase(yyruleno==370);
      /* (371) ccons ::= NULL onconf */ yytestcase(yyruleno==371);
      /* (372) ccons ::= GENERATED ALWAYS AS generated */ yytestcase(yyruleno==372);
      /* (373) ccons ::= AS generated */ yytestcase(yyruleno==373);
      /* (374) conslist_opt ::= COMMA conslist */ yytestcase(yyruleno==374);
      /* (375) conslist ::= conslist tconscomma tcons */ yytestcase(yyruleno==375);
      /* (376) conslist ::= tcons (OPTIMIZED OUT) */ assert(yyruleno!=376);
      /* (377) tconscomma ::= */ yytestcase(yyruleno==377);
      /* (378) defer_subclause_opt ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=378);
      /* (379) resolvetype ::= raisetype (OPTIMIZED OUT) */ assert(yyruleno!=379);
      /* (380) oneselect ::= values */ yytestcase(yyruleno==380);
      /* (381) as ::= ID|STRING */ yytestcase(yyruleno==381);
      /* (382) indexed_opt ::= indexed_by (OPTIMIZED OUT) */ assert(yyruleno!=382);
      /* (383) returning ::= */ yytestcase(yyruleno==383);
      /* (384) likeop ::= LIKE_KW|MATCH */ yytestcase(yyruleno==384);
      /* (385) case_operand ::= expr */ yytestcase(yyruleno==385);
      /* (386) nmnum ::= plus_num (OPTIMIZED OUT) */ assert(yyruleno!=386);
      /* (387) nmnum ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=387);
      /* (388) nmnum ::= ON */ yytestcase(yyruleno==388);
      /* (389) nmnum ::= DELETE */ yytestcase(yyruleno==389);
      /* (390) nmnum ::= DEFAULT */ yytestcase(yyruleno==390);
      /* (391) plus_num ::= INTEGER|FLOAT */ yytestcase(yyruleno==391);
      /* (392) foreach_clause ::= */ yytestcase(yyruleno==392);
      /* (393) foreach_clause ::= FOR EACH ROW */ yytestcase(yyruleno==393);
      /* (394) trnm ::= nm */ yytestcase(yyruleno==394);
      /* (395) tridxby ::= */ yytestcase(yyruleno==395);
      /* (396) database_kw_opt ::= DATABASE */ yytestcase(yyruleno==396);
      /* (397) database_kw_opt ::= */ yytestcase(yyruleno==397);
      /* (398) kwcolumn_opt ::= */ yytestcase(yyruleno==398);
      /* (399) kwcolumn_opt ::= COLUMNKW */ yytestcase(yyruleno==399);
      /* (400) vtabarglist ::= vtabarg */ yytestcase(yyruleno==400);
      /* (401) vtabarglist ::= vtabarglist COMMA vtabarg */ yytestcase(yyruleno==401);
      /* (402) vtabarg ::= vtabarg vtabargtoken */ yytestcase(yyruleno==402);
      /* (403) anylist ::= */ yytestcase(yyruleno==403);
      /* (404) anylist ::= anylist LP anylist RP */ yytestcase(yyruleno==404);
      /* (405) anylist ::= anylist ANY */ yytestcase(yyruleno==405);
      /* (406) with ::= */ yytestcase(yyruleno==406);
      /* (407) windowdefn_list ::= windowdefn (OPTIMIZED OUT) */ assert(yyruleno!=407);
      /* (408) window ::= frame_opt (OPTIMIZED OUT) */ assert(yyruleno!=408);
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
  syntaqlite_yyParser *yypParser           /* The parser */
){
  syntaqlite_sqlite3ParserARG_FETCH
  syntaqlite_sqlite3ParserCTX_FETCH
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
  syntaqlite_sqlite3ParserARG_STORE /* Suppress warning about unused %extra_argument variable */
  syntaqlite_sqlite3ParserCTX_STORE
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  syntaqlite_yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  syntaqlite_sqlite3ParserTOKENTYPE yyminor         /* The minor type of the error token */
){
  syntaqlite_sqlite3ParserARG_FETCH
  syntaqlite_sqlite3ParserCTX_FETCH
#define TOKEN yyminor
/************ Begin %syntax_error code ****************************************/

  (void)yymajor;
  (void)TOKEN;
  if( pCtx && pCtx->onSyntaxError ) {
    pCtx->onSyntaxError(pCtx);
  }
/************ End %syntax_error code ******************************************/
  syntaqlite_sqlite3ParserARG_STORE /* Suppress warning about unused %extra_argument variable */
  syntaqlite_sqlite3ParserCTX_STORE
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  syntaqlite_yyParser *yypParser           /* The parser */
){
  syntaqlite_sqlite3ParserARG_FETCH
  syntaqlite_sqlite3ParserCTX_FETCH
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
  syntaqlite_sqlite3ParserARG_STORE /* Suppress warning about unused %extra_argument variable */
  syntaqlite_sqlite3ParserCTX_STORE
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "syntaqlite_sqlite3ParserAlloc" which describes the current state of the parser.
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
void syntaqlite_sqlite3Parser(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  syntaqlite_sqlite3ParserTOKENTYPE yyminor       /* The value for the token */
  syntaqlite_sqlite3ParserARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  YYACTIONTYPE yyact;   /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  syntaqlite_yyParser *yypParser = (syntaqlite_yyParser*)yyp;  /* The parser */
  syntaqlite_sqlite3ParserCTX_FETCH
  syntaqlite_sqlite3ParserARG_STORE

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
      yyact = yy_reduce(yypParser,yyruleno,yymajor,yyminor syntaqlite_sqlite3ParserCTX_PARAM);
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
    syntaqlite_yyStackEntry *i;
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
int syntaqlite_sqlite3ParserFallback(int iToken){
#ifdef YYFALLBACK
  assert( iToken<(int)(sizeof(yyFallback)/sizeof(yyFallback[0])) );
  return yyFallback[iToken];
#else
  (void)iToken;
  return 0;
#endif
}
