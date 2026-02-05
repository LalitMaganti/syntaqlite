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
#define YYNOCODE 322
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 113
#define syntaqlite_sqlite3ParserTOKENTYPE SyntaqliteToken
typedef union {
  int yyinit;
  syntaqlite_sqlite3ParserTOKENTYPE yy0;
  u32 yy391;
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
#define YYNRULE_WITH_ACTION  33
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
 /*     0 */   464, 1257,  130,  127,  234, 1625,  295,  161,  574,  462,
 /*    10 */   283,  574, 1287, 1625, 1321, 1259,  344,  429,  574,  462,
 /*    20 */   212,  462,  565,  574,  571,  378, 1551,  212, 1289,   19,
 /*    30 */   574, 1289,   51,  399,  574,  326,    6,  413, 1289,   51,
 /*    40 */   398,  557,  421, 1289,   51,  534,  290, 1114, 1287, 1000,
 /*    50 */  1289,   84, 1311,  293, 1289,   60,  379, 1001,  565, 1324,
 /*    60 */   571,  137,  139,   91, 1311, 1228, 1228, 1072, 1075, 1062,
 /*    70 */  1062,  521,  290,  254, 1287,  413,  290, 1323, 1287,  130,
 /*    80 */   127,  234,  346, 1116,  565,  556,  571, 1116,  565,  535,
 /*    90 */   571, 1289,   22,  478,  558,  413,  574, 1631,  522,  137,
 /*   100 */   139,   91, 1515, 1228, 1228, 1072, 1075, 1062, 1062,  466,
 /*   110 */   130,  127,  234,  511,  452,  506, 1289,   84,  532,  137,
 /*   120 */   139,   91,  452, 1228, 1228, 1072, 1075, 1062, 1062,  290,
 /*   130 */   452, 1287,  376,  376,  532,  452, 1183,  377, 1183,  140,
 /*   140 */   377,  565,  547,  571,   44,  547,  264,  230,  233,  293,
 /*   150 */   539,  528,  452,  490,  361,  135,  135,  136,  136,  136,
 /*   160 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   170 */   131,  128,  452,  133,  133,  132,  132,  132,  131,  128,
 /*   180 */   478,  477,  478,  132,  132,  132,  131,  128,  321,  575,
 /*   190 */   452,  321,  575,  135,  135,  136,  136,  136,  136,  134,
 /*   200 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   210 */   501, 1208, 1208,  135,  135,  136,  136,  136,  136,  134,
 /*   220 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   230 */   157,  413,  574, 1580,  110, 1362,  349,  134,  134,  134,
 /*   240 */   134,  133,  133,  132,  132,  132,  131,  128,  533,  533,
 /*   250 */  1031,  413, 1289,   84, 1364,  137,  139,   91,   44, 1228,
 /*   260 */  1228, 1072, 1075, 1062, 1062,  574,  351,  299, 1208,  198,
 /*   270 */   284,  573,  874,   97,  372,  137,  139,   91,  373, 1228,
 /*   280 */  1228, 1072, 1075, 1062, 1062, 1289,   19,  556,  372,  413,
 /*   290 */   882,  359,  883, 1178,  884,  237,  523, 1362,  290,  294,
 /*   300 */  1287,   45,  321,  575,  543,  495, 1178,  516,  452, 1178,
 /*   310 */   565,  413,  571,  137,  139,   91, 1364, 1228, 1228, 1072,
 /*   320 */  1075, 1062, 1062, 1210,  348, 1197,  350, 1210,  452,   48,
 /*   330 */   130,  127,  234, 1208, 1208,  137,  139,   91,  221, 1228,
 /*   340 */  1228, 1072, 1075, 1062, 1062, 1208, 1208, 1550,  542,  135,
 /*   350 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   360 */   133,  132,  132,  132,  131,  128,  452, 1208, 1208,  135,
 /*   370 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   380 */   133,  132,  132,  132,  131,  128, 1579,  372,  452,  323,
 /*   390 */  1208,  524,  490,  361,  874, 1049,  971,  130,  127,  234,
 /*   400 */    48,  372, 1208, 1039,  321,  575,  874,  135,  135,  136,
 /*   410 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*   420 */   132,  132,  131,  128, 1208,  527,  413,  467,  874,  135,
 /*   430 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   440 */   133,  132,  132,  132,  131,  128, 1135,  413,  568,  917,
 /*   450 */   137,  139,   91,   98, 1228, 1228, 1072, 1075, 1062, 1062,
 /*   460 */   867,  409, 1039, 1039,  868, 1359, 1342, 1338, 1136,  107,
 /*   470 */   525,  137,  139,   91, 1359, 1228, 1228, 1072, 1075, 1062,
 /*   480 */  1062,  447,  446, 1137,  452,  413,  236, 1586,    7,  461,
 /*   490 */   458,  457,  290,   46, 1287, 1236,  526, 1236,  953,  456,
 /*   500 */  1119, 1119,  513,  452,  565, 1520,  571,  222,  954,  137,
 /*   510 */   139,   91,  231, 1228, 1228, 1072, 1075, 1062, 1062,  499,
 /*   520 */   568,  568,  497,  505,  452, 1208, 1208,  136,  136,  136,
 /*   530 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   540 */   131,  128,  392,  228,  135,  135,  136,  136,  136,  136,
 /*   550 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   560 */   128,  267,  452,  574,  910,  135,  135,  136,  136,  136,
 /*   570 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   580 */   131,  128, 1208, 1289,   82, 1178,  874, 1233, 1208, 1208,
 /*   590 */   301, 1235, 1520, 1522, 1520,  372,  219,  413, 1178, 1234,
 /*   600 */   368, 1178,  390,  135,  135,  136,  136,  136,  136,  134,
 /*   610 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   620 */   303,  137,  139,   91, 1607, 1228, 1228, 1072, 1075, 1062,
 /*   630 */  1062,   44,  158,  370,  413,  434,   50,  447,  446, 1208,
 /*   640 */  1208, 1208, 1208,  236,  383, 1208,  461,  458,  457,  874,
 /*   650 */   129, 1236,  121, 1236,  118,  413,  456,  479,  137,  139,
 /*   660 */    91, 1309, 1228, 1228, 1072, 1075, 1062, 1062,  420,  574,
 /*   670 */   370, 1031,  100,  113,  452,  321,  575, 1659,  402,  137,
 /*   680 */   139,   91,  251, 1228, 1228, 1072, 1075, 1062, 1062, 1289,
 /*   690 */    19, 1225,  290,  452, 1287, 1225, 1208,  212, 1208,  382,
 /*   700 */   874,  434,  874,  432,  565,  440,  571,   50,  372,  267,
 /*   710 */   407,  452,  359,  485,  347,  135,  135,  136,  136,  136,
 /*   720 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   730 */   131,  128,  452,  183,  448, 1050,  136,  136,  136,  136,
 /*   740 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   750 */   128, 1507,  135,  135,  136,  136,  136,  136,  134,  134,
 /*   760 */   134,  134,  133,  133,  132,  132,  132,  131,  128,  551,
 /*   770 */   413,  298,  866,  135,  135,  136,  136,  136,  136,  134,
 /*   780 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   790 */   413,  894,  338, 1196,  137,  139,   91, 1178, 1228, 1228,
 /*   800 */  1072, 1075, 1062, 1062,  516,  516,  448,  448,  933,  574,
 /*   810 */  1178,  574,  247, 1178,  137,  139,   91,  253, 1228, 1228,
 /*   820 */  1072, 1075, 1062, 1062, 1059, 1059, 1073, 1076,  413, 1289,
 /*   830 */    84, 1289,   84,  470,  291,  505, 1287,  551,  308, 1225,
 /*   840 */    10,  934,  488, 1225, 1514,  427,  565,  452,  571,  530,
 /*   850 */   530,    7,  137,  139,   91, 1244, 1228, 1228, 1072, 1075,
 /*   860 */  1062, 1062, 1386,  316,  556,  499,  556,  452, 1374,  290,
 /*   870 */   138, 1287, 1094,  555,  435,  444, 1208, 1208,  565, 1063,
 /*   880 */   571,  565, 1460,  571,  424,  384,  116,  375,  135,  135,
 /*   890 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   900 */   132,  132,  132,  131,  128,  452,  574,  474,  135,  135,
 /*   910 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   920 */   132,  132,  132,  131,  128,  419, 1289,  145, 1134,  183,
 /*   930 */   380, 1460,  358, 1208,  199, 1459,  304,  874,  429,  413,
 /*   940 */   435,  372,  518,  559,  120,  215,  135,  135,  136,  136,
 /*   950 */   136,  136,  134,  134,  134,  134,  133,  133,  132,  132,
 /*   960 */   132,  131,  128,  137,  139,   91,  413, 1228, 1228, 1072,
 /*   970 */  1075, 1062, 1062,  290,  290, 1287, 1287,    6, 1208, 1208,
 /*   980 */  1250,  574,   47,  380, 1459,  565,  565,  571,  571,  549,
 /*   990 */   137,  139,   91, 1178, 1228, 1228, 1072, 1075, 1062, 1062,
 /*  1000 */   413, 1289,   19,  213,  552,  510, 1178, 1370,  548, 1178,
 /*  1010 */   576,  507,  965,  965,  322,  437,  452, 1189,  452, 1208,
 /*  1020 */  1208, 1208, 1208,  482,  137,  126,   91,  336, 1228, 1228,
 /*  1030 */  1072, 1075, 1062, 1062,  412, 1208,    5,  335,  290,  874,
 /*  1040 */  1287,  401, 1249,  452,  989,  540,  540,    7,  541, 1355,
 /*  1050 */   565,  209,  571,   47,  372, 1189,  489,  135,  135,  136,
 /*  1060 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*  1070 */   132,  132,  131,  128,  131,  128, 1208,  452, 1208,  499,
 /*  1080 */   874,  413,  874,  499,  135,  135,  136,  136,  136,  136,
 /*  1090 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*  1100 */   128,   14, 1208, 1208,  906,  160,  139,   91,  906, 1228,
 /*  1110 */  1228, 1072, 1075, 1062, 1062,  413, 1208, 1208,  135,  135,
 /*  1120 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*  1130 */   132,  132,  132,  131,  128,  574, 1208, 1208,  516,  222,
 /*  1140 */  1458,   91, 1312, 1228, 1228, 1072, 1075, 1062, 1062, 1197,
 /*  1150 */   307,  574,  436,  372,  309, 1289,   19,  429,  452, 1208,
 /*  1160 */    42,  988,  574,  874,  512,  851,  574, 1208, 1208,  442,
 /*  1170 */   850, 1289,   19, 1208,    3,  876,  516,  874, 1371,  124,
 /*  1180 */   516,  566, 1289,   84, 1197,  207, 1289,   84,  380, 1458,
 /*  1190 */   441, 1341,  452, 1208,   49,  505,  569,  874,  536,  135,
 /*  1200 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*  1210 */   133,  132,  132,  132,  131,  128, 1367,  317,  436, 1049,
 /*  1220 */   366,  318,  214,  913, 1208,  562,  524, 1039,  874, 1340,
 /*  1230 */   512, 1135,  155,  135,  135,  136,  136,  136,  136,  134,
 /*  1240 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*  1250 */   124,  574,  566, 1136, 1049, 1197,  441,  574,  450,  450,
 /*  1260 */  1339,  574, 1039, 1584,    7,   49,  372,  569, 1137, 1314,
 /*  1270 */   418, 1289,   19,  232,  124, 1156,  566, 1289,   84, 1197,
 /*  1280 */   913, 1289,   19,  561,  867,  208, 1039, 1039,  868,  232,
 /*  1290 */   574,  569,  310,   34,  331,  550,  562,  987, 1156,  240,
 /*  1300 */   239,  238, 1585,    7,    9,  525,   44,  122,  122,  855,
 /*  1310 */  1289,   84,  445,  987,  123,  450,    4, 1583,    7,  572,
 /*  1320 */   562, 1039, 1039,  868, 1040, 1049,  574, 1320,  453,  450,
 /*  1330 */   450,  286,  987, 1039,  545, 1250, 1158, 1657, 1657,  544,
 /*  1340 */   422,  185,  574, 1582,    7,  449, 1289,   84,  987, 1049,
 /*  1350 */   321,  575,  426,  450,  450,  574,  241, 1039,  327, 1158,
 /*  1360 */  1658, 1658, 1289,  147,   34,  406,  405,  124, 1561,  566,
 /*  1370 */   567,  418, 1197,  481,   95, 1289,   53,  481,  122,  122,
 /*  1380 */   574,  451,  288,  230,  569,  123,  450,    4,   34, 1563,
 /*  1390 */   572,  503, 1039, 1039,  868, 1040,  401, 1157, 1630,  942,
 /*  1400 */  1289,   54,  122,  122,  574,  939,  417,  940,  302,  123,
 /*  1410 */   450,    4,  302,  562,  572,  483, 1039, 1039,  868, 1040,
 /*  1420 */   574,  330,  469,  924, 1289,   55,  412,  545,  574,  423,
 /*  1430 */   475,  574,  546,  243,  280,  329,  472,  332,  471,  242,
 /*  1440 */  1289,   56, 1049,  574,  241,  330,  450,  450, 1289,   66,
 /*  1450 */  1039, 1289,   67, 1000,  124,  574,  566,  492,  574, 1197,
 /*  1460 */   491, 1001,  496, 1289,   21,  574, 1559,  574,  412,  574,
 /*  1470 */   504,  569,  574,  412,  113, 1289,   57,  574, 1289,   68,
 /*  1480 */   320,   34,  356,  101,  218, 1289,   58, 1289,   69, 1289,
 /*  1490 */    70,  412, 1289,   71,  417,  122,  122, 1289,   72,  108,
 /*  1500 */   562,  574,  123,  450,    4,  574,  339,  572,  851, 1039,
 /*  1510 */  1039,  868, 1040,  850,  545,  574,  454,  263,  574,  544,
 /*  1520 */   920, 1289,   73,  574,  920, 1289,   74,  574,  228, 1049,
 /*  1530 */   574,  107,  574,  450,  450, 1289,   75, 1039, 1289,   76,
 /*  1540 */   574,  324,  113, 1289,   77,  574,  480, 1289,   59,  574,
 /*  1550 */  1289,   61, 1289,   20,  340,    2,  465, 1257,  526,  343,
 /*  1560 */  1289,  143,  295,  161, 1398, 1289,  144,   39,   34, 1289,
 /*  1570 */    79, 1259,    2,  465, 1257, 1153,  403,  932,  931,  295,
 /*  1580 */   161, 1397,  122,  122, 1281,   38,  476,  582, 1259,  123,
 /*  1590 */   450,    4, 1384,  292,  572,  430, 1039, 1039,  868, 1040,
 /*  1600 */   493,  520,  290,  396, 1287,  396,  574,  395, 1113,  277,
 /*  1610 */  1113,  393,  357,  113,  565,  484,  571,  891,  574,  290,
 /*  1620 */  1566, 1287,  305, 1112,  574, 1112, 1289,   62,  574,  254,
 /*  1630 */   574,  565,  248,  571,  342,  124,  205,  566, 1289,   80,
 /*  1640 */  1197,  574,  341,  574, 1289,   63,  254,  574, 1289,   81,
 /*  1650 */  1289,   64,  569, 1033,  266,   90,  352,  566,  574, 1539,
 /*  1660 */  1197, 1289,  170, 1289,  171,  466,  250, 1289,   88,  574,
 /*  1670 */  1598,  574,  569, 1162,  177,  498,  266,   43, 1289,   65,
 /*  1680 */  1538,  562,  466,  574,  500,  266, 1255,    1, 1283, 1289,
 /*  1690 */   146, 1289,   83, 1101,  249,  508,   25, 1101,  362,  113,
 /*  1700 */   363,  562,  289, 1289,  168, 1282, 1620,  574, 1003, 1004,
 /*  1710 */  1049,  574, 1097,  263,  450,  450,  414, 1394, 1039,  574,
 /*  1720 */   537,  321,  575,  574,  367,  574,  371, 1289,  148,  281,
 /*  1730 */  1049, 1289,  142,  574,  450,  450, 1407,  554, 1039, 1289,
 /*  1740 */   169,  992,  266, 1289,  162, 1289,  152,  165,  113,   34,
 /*  1750 */   969,  138,  166, 1289,  151, 1446,  124,  553,  566, 1268,
 /*  1760 */   560, 1197, 1450,  122,  122,  904,  159,  872,  389,   34,
 /*  1770 */   123,  450,    4,  569, 1392,  572, 1266, 1039, 1039,  868,
 /*  1780 */  1040,  970,  138,  122,  122,  391,  476,  582, 1300, 1299,
 /*  1790 */   123,  450,    4,  292, 1301,  572, 1613, 1039, 1039,  868,
 /*  1800 */  1040,  224,  562,  396,  574,  396, 1041,  395,  167,  277,
 /*  1810 */  1041,  393,  313,  397,  967,  138,   12,  891,  314,  315,
 /*  1820 */   574,  517, 1338,  574, 1289,  149,  574,  227,  574, 1616,
 /*  1830 */   574, 1049,  248,  574,  342,  450,  450,  459,  246, 1039,
 /*  1840 */  1289,  150,  341, 1289,   86, 1428, 1289,   78, 1289,   87,
 /*  1850 */  1289,   85,  334, 1289,   52, 1433,  300, 1423, 1438,  355,
 /*  1860 */  1437,  354,  306,  360,  502,  410,  250, 1511, 1510, 1389,
 /*  1870 */    34,  203,  210,  400,  177,  211,  473,   43, 1358, 1390,
 /*  1880 */  1388, 1387,  563,  387,  122,  122,  223, 1357,  476, 1356,
 /*  1890 */    40,  123,  450,    4,  249,  292,  572,  463, 1039, 1039,
 /*  1900 */   868, 1040,   93, 1629, 1628,  396, 1627,  396,  425,  395,
 /*  1910 */  1349,  277,  404,  393, 1328,  924,  414, 1327,  333,  891,
 /*  1920 */  1326,  321,  575, 1244,  274, 1348, 1241, 1558, 1556,  428,
 /*  1930 */   187,  100, 1516,  220,  248,  244,  342,   96,   99, 1434,
 /*  1940 */   196,   13,  182,  189,  341,  486,  191,  192,  193,  194,
 /*  1950 */   487,  579,  256,  108, 1442, 1440, 1439,  200,   15,  260,
 /*  1960 */   408,  494,  411, 1505,  106,  509, 1527,  872,  250,  515,
 /*  1970 */   282,  365,  262,  438,  519,  439,  177,  369,  268,   43,
 /*  1980 */   311,  141,  312, 1378,  551,  374,  269,  443,  233, 1589,
 /*  1990 */  1588, 1603, 1411, 1410,   11,  381,  249, 1492,  117,  109,
 /*  2000 */   319, 1261,  529,  577,  386,  216,  385,  388,  273, 1202,
 /*  2010 */   276, 1302,  275,  279,  278,  580, 1297, 1292,  414,  154,
 /*  2020 */   296,  335,  235,  321,  575,  455,  225,  184,  226,   92,
 /*  2030 */   172,  173, 1543,   23,  460,  325,  174,  186,  415,  416,
 /*  2040 */  1544,   24, 1542, 1541,  175,   89,  153, 1198,  873,  217,
 /*  2050 */   922,  328,  468,  935,   94,  297,  176,  337,  245,  156,
 /*  2060 */  1111,  345, 1109,  188,  178,  876,  252,  190,  255,  956,
 /*  2070 */   353, 1125,  195,  431,  179,  433,  180,  197,  102,  181,
 /*  2080 */   103,  104, 1128,  257,  258,  105, 1124,  163,  259,   26,
 /*  2090 */   364, 1117,  201,  266,  514, 1164, 1238,  261,  202, 1163,
 /*  2100 */   265,  996,  990,   27,  531,   16,   28,  112,   29,  285,
 /*  2110 */   204,  229,  287,   41,  138, 1169,  538,  206, 1078,   30,
 /*  2120 */    31, 1180, 1182, 1184,  111,    8,  164, 1194, 1188,  113,
 /*  2130 */  1187,   32,   33,  114,  115,  119, 1082, 1092, 1079, 1077,
 /*  2140 */  1081,   18, 1133,  270,   35,  893,   17,  564,  964, 1042,
 /*  2150 */   905,  125,   36,  578,  394,  570,  879,  581, 1256, 1286,
 /*  2160 */   271,   37, 1256, 1256,  272,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */   188,  189,  269,  270,  271,  211,  194,  195,  192,  192,
 /*    10 */   234,  192,  236,  219,  212,  203,  192,  192,  192,  202,
 /*    20 */   192,  204,  246,  192,  248,  215,  293,  192,  212,  213,
 /*    30 */   192,  212,  213,  205,  192,  200,  209,   37,  212,  213,
 /*    40 */   205,  203,  226,  212,  213,  203,  234,   21,  236,   49,
 /*    50 */   212,  213,  192,  225,  212,  213,  215,   57,  246,  212,
 /*    60 */   248,   61,   62,   63,  204,   65,   66,   67,   68,   69,
 /*    70 */    70,  192,  234,  261,  236,   37,  234,  212,  236,  269,
 /*    80 */   270,  271,  257,   47,  246,  247,  248,   51,  246,  247,
 /*    90 */   248,  212,  213,  192,  256,   37,  192,  225,  203,   61,
 /*   100 */    62,   63,  278,   65,   66,   67,   68,   69,   70,  297,
 /*   110 */   269,  270,  271,   77,  114,  288,  212,  213,  192,   61,
 /*   120 */    62,   63,  114,   65,   66,   67,   68,   69,   70,  234,
 /*   130 */   114,  236,  313,  314,  192,  114,   98,  311,  100,   81,
 /*   140 */   314,  246,  311,  248,   93,  314,  251,  252,    8,  321,
 /*   150 */    10,  247,  114,  127,  128,  155,  156,  157,  158,  159,
 /*   160 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   170 */   170,  171,  114,  165,  166,  167,  168,  169,  170,  171,
 /*   180 */   279,  280,  281,  167,  168,  169,  170,  171,  137,  138,
 /*   190 */   114,  137,  138,  155,  156,  157,  158,  159,  160,  161,
 /*   200 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   210 */   192,  116,  117,  155,  156,  157,  158,  159,  160,  161,
 /*   220 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   230 */   176,   37,  192,  307,  176,  229,    1,  161,  162,  163,
 /*   240 */   164,  165,  166,  167,  168,  169,  170,  171,  306,  307,
 /*   250 */    85,   37,  212,  213,  229,   61,   62,   63,   93,   65,
 /*   260 */    66,   67,   68,   69,   70,  192,   31,  203,  173,  174,
 /*   270 */   209,  192,  177,   79,  192,   61,   62,   63,  192,   65,
 /*   280 */    66,   67,   68,   69,   70,  212,  213,  247,  192,   37,
 /*   290 */    13,  126,   15,   88,   17,   44,  256,  291,  234,  226,
 /*   300 */   236,   84,  137,  138,   99,  115,  101,  192,  114,  104,
 /*   310 */   246,   37,  248,   61,   62,   63,  291,   65,   66,   67,
 /*   320 */    68,   69,   70,  173,   89,   42,   91,  177,  114,  233,
 /*   330 */   269,  270,  271,  116,  117,   61,   62,   63,  148,   65,
 /*   340 */    66,   67,   68,   69,   70,  116,  117,  232,  143,  155,
 /*   350 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   360 */   166,  167,  168,  169,  170,  171,  114,  116,  117,  155,
 /*   370 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   380 */   166,  167,  168,  169,  170,  171,  304,  305,  114,  175,
 /*   390 */   173,   37,  127,  128,  177,  112,  167,  269,  270,  271,
 /*   400 */   304,  305,  173,  120,  137,  138,  177,  155,  156,  157,
 /*   410 */   158,  159,  160,  161,  162,  163,  164,  165,  166,  167,
 /*   420 */   168,  169,  170,  171,  173,  192,   37,  175,  177,  155,
 /*   430 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   440 */   166,  167,  168,  169,  170,  171,   23,   37,  208,  175,
 /*   450 */    61,   62,   63,   43,   65,   66,   67,   68,   69,   70,
 /*   460 */   177,  203,  179,  180,  181,  219,  222,  221,   45,  115,
 /*   470 */   116,   61,   62,   63,  228,   65,   66,   67,   68,   69,
 /*   480 */    70,  165,  166,   60,  114,   37,  118,  309,  310,  121,
 /*   490 */   122,  123,  234,   84,  236,  179,  142,  181,   75,  131,
 /*   500 */   126,  127,  128,  114,  246,  192,  248,  140,   85,   61,
 /*   510 */    62,   63,  192,   65,   66,   67,   68,   69,   70,  192,
 /*   520 */   280,  281,  264,  192,  114,  116,  117,  157,  158,  159,
 /*   530 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   540 */   170,  171,   31,  176,  155,  156,  157,  158,  159,  160,
 /*   550 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*   560 */   171,   43,  114,  192,  175,  155,  156,  157,  158,  159,
 /*   570 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   580 */   170,  171,  173,  212,  213,   88,  177,  115,  116,  117,
 /*   590 */   263,  119,  279,  280,  281,  192,  148,   37,  101,  127,
 /*   600 */    89,  104,   91,  155,  156,  157,  158,  159,  160,  161,
 /*   610 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   620 */   289,   61,   62,   63,  192,   65,   66,   67,   68,   69,
 /*   630 */    70,   93,    6,  192,   37,  192,  233,  165,  166,  116,
 /*   640 */   117,  116,  117,  118,  273,  173,  121,  122,  123,  177,
 /*   650 */    71,  179,  152,  181,  154,   37,  131,  119,   61,   62,
 /*   660 */    63,  203,   65,   66,   67,   68,   69,   70,  227,  192,
 /*   670 */   192,   85,  149,  176,  114,  137,  138,  300,  301,   61,
 /*   680 */    62,   63,   43,   65,   66,   67,   68,   69,   70,  212,
 /*   690 */   213,  173,  234,  114,  236,  177,  173,  192,  173,  192,
 /*   700 */   177,  258,  177,  226,  246,  227,  248,  304,  305,   43,
 /*   710 */   205,  114,  126,  127,  128,  155,  156,  157,  158,  159,
 /*   720 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   730 */   170,  171,  114,  192,  208,  175,  157,  158,  159,  160,
 /*   740 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*   750 */   171,    1,  155,  156,  157,  158,  159,  160,  161,  162,
 /*   760 */   163,  164,  165,  166,  167,  168,  169,  170,  171,  143,
 /*   770 */    37,  111,  175,  155,  156,  157,  158,  159,  160,  161,
 /*   780 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   790 */    37,   41,  132,  175,   61,   62,   63,   88,   65,   66,
 /*   800 */    67,   68,   69,   70,  192,  192,  280,  281,   53,  192,
 /*   810 */   101,  192,   29,  104,   61,   62,   63,  178,   65,   66,
 /*   820 */    67,   68,   69,   70,   65,   66,   67,   68,   37,  212,
 /*   830 */   213,  212,  213,   78,  234,  192,  236,  143,  203,  173,
 /*   840 */   174,   86,   92,  177,  232,  232,  246,  114,  248,  308,
 /*   850 */   309,  310,   61,   62,   63,   72,   65,   66,   67,   68,
 /*   860 */    69,   70,  254,  255,  247,  192,  247,  114,  236,  234,
 /*   870 */   176,  236,  122,  256,  192,  256,  116,  117,  246,  120,
 /*   880 */   248,  246,  268,  248,  129,  243,  153,  245,  155,  156,
 /*   890 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*   900 */   167,  168,  169,  170,  171,  114,  192,  294,  155,  156,
 /*   910 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*   920 */   167,  168,  169,  170,  171,  196,  212,  213,  175,  192,
 /*   930 */   316,  317,  289,  173,  174,  268,  263,  177,  192,   37,
 /*   940 */   258,  192,  203,  203,  153,   43,  155,  156,  157,  158,
 /*   950 */   159,  160,  161,  162,  163,  164,  165,  166,  167,  168,
 /*   960 */   169,  170,  171,   61,   62,   63,   37,   65,   66,   67,
 /*   970 */    68,   69,   70,  234,  234,  236,  236,  209,  116,  117,
 /*   980 */   113,  192,  233,  316,  317,  246,  246,  248,  248,   78,
 /*   990 */    61,   62,   63,   88,   65,   66,   67,   68,   69,   70,
 /*  1000 */    37,  212,  213,  257,   99,  276,  101,  203,   97,  104,
 /*  1010 */   133,  282,  135,  136,  192,  226,  114,  106,  114,  116,
 /*  1020 */   117,  116,  117,  238,   61,   62,   63,  120,   65,   66,
 /*  1030 */    67,   68,   69,   70,  249,  173,  174,  130,  234,  177,
 /*  1040 */   236,  174,  175,  114,  141,  308,  309,  310,  143,  192,
 /*  1050 */   246,  283,  248,  304,  305,  144,  288,  155,  156,  157,
 /*  1060 */   158,  159,  160,  161,  162,  163,  164,  165,  166,  167,
 /*  1070 */   168,  169,  170,  171,  170,  171,  173,  114,  173,  192,
 /*  1080 */   177,   37,  177,  192,  155,  156,  157,  158,  159,  160,
 /*  1090 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*  1100 */   171,   43,  116,  117,  173,  174,   62,   63,  177,   65,
 /*  1110 */    66,   67,   68,   69,   70,   37,  116,  117,  155,  156,
 /*  1120 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*  1130 */   167,  168,  169,  170,  171,  192,  116,  117,  192,  140,
 /*  1140 */   268,   63,  192,   65,   66,   67,   68,   69,   70,   42,
 /*  1150 */   263,  192,  192,  192,  263,  212,  213,  192,  114,  173,
 /*  1160 */   174,  141,  192,  177,  192,  134,  192,  116,  117,  226,
 /*  1170 */   139,  212,  213,  173,  174,  176,  192,  177,  232,   37,
 /*  1180 */   192,   39,  212,  213,   42,  226,  212,  213,  316,  317,
 /*  1190 */   192,  222,  114,  173,  233,  192,   54,  177,  167,  155,
 /*  1200 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*  1210 */   166,  167,  168,  169,  170,  171,  232,  247,  258,  112,
 /*  1220 */   232,  247,  257,  116,  173,   83,   37,  120,  177,  222,
 /*  1230 */   258,   23,  174,  155,  156,  157,  158,  159,  160,  161,
 /*  1240 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*  1250 */    37,  192,   39,   45,  112,   42,  258,  192,  116,  117,
 /*  1260 */   192,  192,  120,  309,  310,  304,  305,   54,   60,  206,
 /*  1270 */   207,  212,  213,  117,   37,  113,   39,  212,  213,   42,
 /*  1280 */   173,  212,  213,   75,  177,  226,  179,  180,  181,  117,
 /*  1290 */   192,   54,  289,  151,  192,  226,   83,  141,  113,  126,
 /*  1300 */   127,  128,  309,  310,   67,  116,   93,  165,  166,  167,
 /*  1310 */   212,  213,  247,  141,  172,  173,  174,  309,  310,  177,
 /*  1320 */    83,  179,  180,  181,  182,  112,  192,  192,   37,  116,
 /*  1330 */   117,  175,  176,  120,   97,  113,  174,  175,  176,  102,
 /*  1340 */   298,  299,  192,  309,  310,  247,  212,  213,  176,  112,
 /*  1350 */   137,  138,   37,  116,  117,  192,   65,  120,   43,  174,
 /*  1360 */   175,  176,  212,  213,  151,  165,  166,   37,  192,   39,
 /*  1370 */   206,  207,   42,  255,  174,  212,  213,  259,  165,  166,
 /*  1380 */   192,  247,  251,  252,   54,  172,  173,  174,  151,  192,
 /*  1390 */   177,   37,  179,  180,  181,  182,  174,  175,  175,  176,
 /*  1400 */   212,  213,  165,  166,  192,   13,  115,   15,  255,  172,
 /*  1410 */   173,  174,  259,   83,  177,  238,  179,  180,  181,  182,
 /*  1420 */   192,  130,  107,  125,  212,  213,  249,   97,  192,  114,
 /*  1430 */   192,  192,  102,  118,  119,  120,  121,  122,  123,  124,
 /*  1440 */   212,  213,  112,  192,   65,  130,  116,  117,  212,  213,
 /*  1450 */   120,  212,  213,   49,   37,  192,   39,  238,  192,   42,
 /*  1460 */   128,   57,  238,  212,  213,  192,  192,  192,  249,  192,
 /*  1470 */   116,   54,  192,  249,  176,  212,  213,  192,  212,  213,
 /*  1480 */   238,  151,  150,  147,  148,  212,  213,  212,  213,  212,
 /*  1490 */   213,  249,  212,  213,  115,  165,  166,  212,  213,  147,
 /*  1500 */    83,  192,  172,  173,  174,  192,  192,  177,  134,  179,
 /*  1510 */   180,  181,  182,  139,   97,  192,  175,  176,  192,  102,
 /*  1520 */   173,  212,  213,  192,  177,  212,  213,  192,  176,  112,
 /*  1530 */   192,  115,  192,  116,  117,  212,  213,  120,  212,  213,
 /*  1540 */   192,  175,  176,  212,  213,  192,  264,  212,  213,  192,
 /*  1550 */   212,  213,  212,  213,  192,  187,  188,  189,  142,  192,
 /*  1560 */   212,  213,  194,  195,  192,  212,  213,  156,  151,  212,
 /*  1570 */   213,  203,  187,  188,  189,  175,  176,  119,  120,  194,
 /*  1580 */   195,  192,  165,  166,    0,  174,    2,    3,  203,  172,
 /*  1590 */   173,  174,  253,    9,  177,   73,  179,  180,  181,  182,
 /*  1600 */   128,   37,  234,   19,  236,   21,  192,   23,  179,   25,
 /*  1610 */   181,   27,  175,  176,  246,  192,  248,   33,  192,  234,
 /*  1620 */   192,  236,  150,  179,  192,  181,  212,  213,  192,  261,
 /*  1630 */   192,  246,   48,  248,   50,   37,  250,   39,  212,  213,
 /*  1640 */    42,  192,   58,  192,  212,  213,  261,  192,  212,  213,
 /*  1650 */   212,  213,   54,  175,  176,   37,  192,   39,  192,  192,
 /*  1660 */    42,  212,  213,  212,  213,  297,   82,  212,  213,  192,
 /*  1670 */   315,  192,   54,  109,   90,  175,  176,   93,  212,  213,
 /*  1680 */   192,   83,  297,  192,  175,  176,  318,  319,  320,  212,
 /*  1690 */   213,  212,  213,  173,  110,  192,  174,  177,  175,  176,
 /*  1700 */   192,   83,  174,  212,  213,  320,  178,  192,   95,   96,
 /*  1710 */   112,  192,  175,  176,  116,  117,  132,  192,  120,  192,
 /*  1720 */   143,  137,  138,  192,  192,  192,  192,  212,  213,  284,
 /*  1730 */   112,  212,  213,  192,  116,  117,  192,  139,  120,  212,
 /*  1740 */   213,  175,  176,  212,  213,  212,  213,  175,  176,  151,
 /*  1750 */   175,  176,  175,  212,  213,  192,   37,  139,   39,  192,
 /*  1760 */   291,   42,  192,  165,  166,  175,  176,  183,  192,  151,
 /*  1770 */   172,  173,  174,   54,  192,  177,  250,  179,  180,  181,
 /*  1780 */   182,  175,  176,  165,  166,  192,    2,    3,  192,  192,
 /*  1790 */   172,  173,  174,    9,  192,  177,  192,  179,  180,  181,
 /*  1800 */   182,  210,   83,   19,  192,   21,  173,   23,  235,   25,
 /*  1810 */   177,   27,  250,  190,  175,  176,  237,   33,  250,  250,
 /*  1820 */   192,  285,  221,  192,  212,  213,  192,  224,  192,  199,
 /*  1830 */   192,  112,   48,  192,   50,  116,  117,  216,  295,  120,
 /*  1840 */   212,  213,   58,  212,  213,  262,  212,  213,  212,  213,
 /*  1850 */   212,  213,  215,  212,  213,  266,  239,  262,  266,  240,
 /*  1860 */   266,  290,  240,  239,  290,  266,   82,  215,  215,  254,
 /*  1870 */   151,  174,  243,  217,   90,  243,  115,   93,  214,  254,
 /*  1880 */   254,  254,  275,  239,  165,  166,  237,  214,    2,  214,
 /*  1890 */   176,  172,  173,  174,  110,    9,  177,  201,  179,  180,
 /*  1900 */   181,  182,  174,  220,  220,   19,  214,   21,   76,   23,
 /*  1910 */   223,   25,  217,   27,  214,  125,  132,  216,  214,   33,
 /*  1920 */   214,  137,  138,   72,  178,  223,   56,  198,  198,  198,
 /*  1930 */   295,  149,  278,  148,   48,  295,   50,  292,  292,  267,
 /*  1940 */   174,  265,   61,  230,   58,   35,  231,  231,  231,  231,
 /*  1950 */   198,   35,  197,  147,  230,  267,  267,  230,  265,  197,
 /*  1960 */   240,  240,  240,  240,  152,  198,  287,  183,   82,   74,
 /*  1970 */   198,  286,  197,   43,  303,  114,   90,  217,  198,   93,
 /*  1980 */   277,  146,  277,  244,  143,  243,  103,   94,    8,  312,
 /*  1990 */   312,  317,  260,  260,  174,  198,  110,  272,  152,  145,
 /*  2000 */   274,  244,  144,  217,  240,  242,  241,  198,  197,   25,
 /*  2010 */   193,  198,  197,   11,  193,  191,  191,  191,  132,  218,
 /*  2020 */   218,  130,  124,  137,  138,   79,  210,  174,  210,  174,
 /*  2030 */   296,  296,  209,  174,   79,  175,  296,  299,  302,  302,
 /*  2040 */   209,  174,  209,  209,  176,  209,  175,  183,  183,  174,
 /*  2050 */   173,   43,  108,   46,  174,   79,   55,    4,   29,   31,
 /*  2060 */   175,  138,  175,  149,  129,  176,   43,  140,  142,   39,
 /*  2070 */    31,  183,  140,   73,  129,   55,  129,  149,  156,  129,
 /*  2080 */   156,  156,  116,   52,  178,  156,  183,    9,  115,  174,
 /*  2090 */     1,   80,   80,  176,   59,  175,   87,  178,  115,  109,
 /*  2100 */    52,  116,  141,   52,   43,  174,   52,  147,   52,  175,
 /*  2110 */   174,  178,  175,  174,  176,  175,  174,  176,  175,   52,
 /*  2120 */    52,  100,   98,   87,  176,   62,  175,  175,   87,  176,
 /*  2130 */   105,   52,   52,  140,  140,  176,   21,  175,  175,  175,
 /*  2140 */   175,   43,  175,  174,  174,   39,  174,  176,  134,  175,
 /*  2150 */   175,  174,  174,   37,   29,  176,    7,    5,  322,  178,
 /*  2160 */   178,  174,  322,  322,  178,  322,  322,  322,  322,  322,
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
 /*  2350 */   322,  322,
};
#define YY_SHIFT_COUNT    (582)
#define YY_SHIFT_MIN      (0)
#define YY_SHIFT_MAX      (2152)
static const unsigned short int yy_shift_ofst[] = {
 /*     0 */  1784, 1584, 1886, 1213, 1213,   51,  165, 1237, 1330, 1417,
 /*    10 */  1719, 1719, 1719,  538,   51,   51,   51,   51,   51,    0,
 /*    20 */     0,  194,  929, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*    30 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,  472,  472,
 /*    40 */   525,  905,  905,  217,  409, 1051, 1051,   54,   54,   54,
 /*    50 */    54,   38,   58,  214,  252,  274,  389,  410,  448,  560,
 /*    60 */   597,  618,  733,  753,  791,  902,  929,  929,  929,  929,
 /*    70 */   929,  929,  929,  929,  929,  929,  929,  929,  929,  929,
 /*    80 */   929,  929,  929,  963,  929,  929, 1044, 1078, 1078, 1142,
 /*    90 */  1598, 1618, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   100 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   110 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   120 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   130 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   140 */  1719, 1719,  579,  370,  370,  370,  370,  370,  370,  370,
 /*   150 */    76,    8,   16,  251, 1291, 1051, 1051, 1051, 1051,  316,
 /*   160 */   316,  750,  904,  265,  140,  140,  140,  267,   21,   21,
 /*   170 */  2165, 2165, 1315, 1315, 1315,  251,  423,  523,   95,   95,
 /*   180 */    95,   95,  423,  205, 1051, 1162, 1185, 1051, 1051, 1051,
 /*   190 */  1051, 1051, 1051, 1051, 1051, 1051, 1051, 1051, 1051, 1051,
 /*   200 */  1051, 1051, 1051, 1051, 1051,  354, 1051,  497,  497,   26,
 /*   210 */   709,  709,  150, 1189, 1189,  150,  626, 2165, 2165, 2165,
 /*   220 */  2165, 2165, 2165, 2165, 1107,  283,  283,  368,  760,  229,
 /*   230 */   862,  903, 1020,  986, 1000, 1051, 1051, 1051,  755,  755,
 /*   240 */   755, 1051, 1051, 1051, 1051, 1051, 1051, 1051, 1051, 1051,
 /*   250 */  1051, 1051, 1051, 1051,  586, 1051, 1051, 1051, 1051, 1051,
 /*   260 */  1051, 1051, 1051, 1051, 1156, 1051, 1051, 1051,  666,  911,
 /*   270 */  1208, 1051, 1051, 1051, 1051, 1051, 1051, 1051, 1051, 1051,
 /*   280 */  1200,  374,   36,  877,  518,  518,  518,  518, 1172, 1031,
 /*   290 */   877,  877,  277, 1223, 1298, 1058, 1379, 1404,  783,  190,
 /*   300 */  1336,  367, 1336, 1354, 1352,  190,  190, 1352,  190,  367,
 /*   310 */  1354, 1404, 1404, 1416, 1416, 1416, 1416,  694,  694,  500,
 /*   320 */   999, 1374, 1697, 1761, 1761, 1761, 1714, 1728, 1728, 1761,
 /*   330 */  1832, 1697, 1761, 1790, 1761, 1832, 1761, 1851, 1851, 1746,
 /*   340 */  1746, 1870, 1870, 1746, 1782, 1785, 1766, 1881, 1910, 1910,
 /*   350 */  1910, 1910, 1746, 1916, 1806, 1785, 1785, 1806, 1766, 1881,
 /*   360 */  1806, 1881, 1806, 1746, 1916, 1812, 1895, 1746, 1916, 1930,
 /*   370 */  1861, 1861, 1697, 1746, 1835, 1841, 1883, 1883, 1893, 1893,
 /*   380 */  1980, 1820, 1746, 1846, 1835, 1854, 1858, 1806, 1697, 1746,
 /*   390 */  1916, 1746, 1916, 1984, 1984, 2002, 2002, 2002, 2165, 2165,
 /*   400 */  2165, 2165, 2165, 2165, 2165, 2165, 2165, 2165, 2165, 2165,
 /*   410 */  2165, 2165, 2165,  759,  235,  867, 1222, 1173,  931,  511,
 /*   420 */  1341, 1366, 1400, 1347, 1458, 1392,  907,  660, 1411,  639,
 /*   430 */  1332, 1472, 1437, 1522, 1478, 1500, 1509, 1523, 1564, 1520,
 /*   440 */  1537, 1566, 1572, 1613, 1577, 1575, 1429, 1444, 1590, 1606,
 /*   450 */  1528, 1639, 1633, 1891, 1898, 1853, 1946, 1855, 1859, 1860,
 /*   460 */  1867, 1955, 1868, 1871, 1864, 1865, 1875, 1877, 2008, 1944,
 /*   470 */  2007, 1880, 1976, 2001, 2053, 2029, 2028, 1885, 1887, 1923,
 /*   480 */  1914, 1935, 1889, 1889, 2023, 1927, 2030, 1926, 2039, 1888,
 /*   490 */  1932, 1945, 1889, 1947, 2000, 2020, 1889, 1928, 1922, 1924,
 /*   500 */  1925, 1929, 1950, 1966, 2031, 1906, 1903, 2078, 1915, 1973,
 /*   510 */  2089, 2011, 1917, 2012, 2009, 2035, 1919, 1983, 1920, 1931,
 /*   520 */  1990, 1933, 1934, 1937, 1985, 2048, 1936, 1961, 1938, 2051,
 /*   530 */  1940, 1939, 2061, 1941, 1943, 1948, 1951, 2054, 1960, 1942,
 /*   540 */  1952, 2056, 2067, 2068, 2021, 2036, 2024, 2063, 2041, 2025,
 /*   550 */  1953, 2079, 2080, 1993, 1994, 1962, 1938, 1963, 1964, 1965,
 /*   560 */  1959, 1967, 1969, 2115, 1970, 1971, 1972, 1974, 1975, 1977,
 /*   570 */  1978, 1979, 1981, 1982, 1986, 1987, 2014, 2098, 2106, 2116,
 /*   580 */  2125, 2149, 2152,
};
#define YY_REDUCE_COUNT (412)
#define YY_REDUCE_MIN   (-267)
#define YY_REDUCE_MAX   (1836)
static const short yy_reduce_ofst[] = {
 /*     0 */  1368, 1385, -188, -162, -158, -105,  258, -174, -181, -169,
 /*    10 */    40,  617,  619,   64,  458,  635,  739,  740,  804, -190,
 /*    20 */  -159, -267,   61, -184,   73,  477,  789,  -96,  943,  959,
 /*    30 */  1059,  970, 1069,  974,  371, 1065, 1098, 1134,  -99,  313,
 /*    40 */  -172,  541,  737,   96,  403,  749,  961, -224,  600, -224,
 /*    50 */   600,  128,  128,  128,  128,  128,  128,  128,  128,  128,
 /*    60 */   128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
 /*    70 */   128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
 /*    80 */   128,  128,  128,  128,  128,  128,  128,  128,  128, -121,
 /*    90 */   714, 1150, 1163, 1188, 1212, 1228, 1236, 1239, 1251, 1263,
 /*   100 */  1266, 1273, 1275, 1277, 1280, 1285, 1309, 1313, 1323, 1326,
 /*   110 */  1331, 1335, 1338, 1340, 1348, 1353, 1357, 1414, 1426, 1432,
 /*   120 */  1436, 1438, 1449, 1451, 1455, 1466, 1477, 1479, 1491, 1515,
 /*   130 */  1519, 1527, 1531, 1533, 1541, 1612, 1628, 1631, 1634, 1636,
 /*   140 */  1638, 1641,  128,  128,  128,  128,  128,  128,  128,  128,
 /*   150 */   128,  128,  128, -183,  246, -165,  613,   82,  -58,  240,
 /*   160 */   526,  729,  128,  768,  614,  667,  872,  632,  128,  128,
 /*   170 */   128,  128, -206, -206, -206, -140,    6, -176,  327,  673,
 /*   180 */   887,  891,   25,  178,  441,  377,  377,  505, -175,  746,
 /*   190 */   965,  115,  612,  946,  984,  331,  443,  643,  682,  960,
 /*   200 */  1003,  972,  988,  478,  998,  608,  -74,  954,  993, -173,
 /*   210 */  1008, 1034, 1063, 1118, 1153, 1164,  642, 1042,  785, 1177,
 /*   220 */  1219, 1224, 1131, 1242, -198, -153, -135, -128,   18,   79,
 /*   230 */    86,  233,  320,  432,  507,  822,  857,  950,  244,  969,
 /*   240 */  1007, 1068, 1102, 1135, 1176, 1197, 1238, 1274, 1314, 1362,
 /*   250 */  1367, 1372, 1389, 1423, 1282, 1428, 1464, 1467, 1488, 1503,
 /*   260 */  1508, 1525, 1532, 1534, 1339, 1544, 1563, 1567, 1386, 1355,
 /*   270 */  1469, 1570,   79, 1576, 1582, 1593, 1596, 1597, 1602, 1604,
 /*   280 */  1591, 1536, 1445, 1573, 1526, 1562, 1568, 1569, 1339, 1579,
 /*   290 */  1573, 1573, 1623, 1603, 1621, 1630, 1601, 1637, 1543, 1589,
 /*   300 */  1583, 1617, 1595, 1571, 1619, 1592, 1594, 1622, 1599, 1624,
 /*   310 */  1574, 1652, 1653, 1615, 1625, 1626, 1627, 1629, 1632, 1607,
 /*   320 */  1644, 1649, 1656, 1664, 1673, 1675, 1696, 1683, 1684, 1692,
 /*   330 */  1687, 1695, 1700, 1701, 1704, 1702, 1706, 1635, 1640, 1729,
 /*   340 */  1730, 1645, 1646, 1731, 1654, 1672, 1676, 1713, 1715, 1716,
 /*   350 */  1717, 1718, 1752, 1755, 1720, 1688, 1689, 1721, 1693, 1724,
 /*   360 */  1722, 1727, 1723, 1767, 1762, 1679, 1685, 1772, 1775, 1671,
 /*   370 */  1703, 1705, 1760, 1780, 1739, 1742, 1677, 1678, 1732, 1733,
 /*   380 */  1674, 1725, 1797, 1726, 1757, 1763, 1765, 1764, 1786, 1809,
 /*   390 */  1811, 1813, 1815, 1817, 1821, 1824, 1825, 1826, 1734, 1735,
 /*   400 */  1801, 1736, 1737, 1738, 1802, 1816, 1818, 1740, 1823, 1831,
 /*   410 */  1833, 1834, 1836,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */  1663, 1663, 1663, 1500, 1254, 1385, 1254, 1254, 1254, 1254,
 /*    10 */  1500, 1500, 1500, 1254, 1254, 1254, 1254, 1254, 1254, 1414,
 /*    20 */  1414, 1553, 1258, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*    30 */  1254, 1254, 1254, 1254, 1499, 1254, 1254, 1254, 1254, 1254,
 /*    40 */  1254, 1587, 1587, 1254, 1254, 1254, 1254, 1572, 1571, 1254,
 /*    50 */  1254, 1254, 1420, 1254, 1254, 1254, 1254, 1254, 1425, 1501,
 /*    60 */  1502, 1254, 1254, 1254, 1254, 1254, 1552, 1554, 1517, 1432,
 /*    70 */  1431, 1430, 1429, 1535, 1403, 1277, 1419, 1422, 1501, 1496,
 /*    80 */  1497, 1495, 1641, 1254, 1502, 1421, 1464, 1463, 1480, 1254,
 /*    90 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   100 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   110 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   120 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   130 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   140 */  1254, 1254, 1472, 1479, 1478, 1477, 1486, 1476, 1473, 1466,
 /*   150 */  1465, 1467, 1468, 1310, 1360, 1254, 1254, 1254, 1254, 1254,
 /*   160 */  1254, 1307, 1469, 1258, 1457, 1456, 1455, 1254, 1483, 1470,
 /*   170 */  1482, 1481, 1619, 1618, 1560, 1254, 1254, 1518, 1254, 1254,
 /*   180 */  1254, 1254, 1254, 1587, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   190 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   200 */  1254, 1254, 1254, 1254, 1254, 1405, 1254, 1587, 1587, 1258,
 /*   210 */  1587, 1587, 1315, 1406, 1406, 1315, 1272, 1567, 1265, 1265,
 /*   220 */  1265, 1265, 1385, 1265, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   230 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   240 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1557, 1555,
 /*   250 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   260 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1269, 1254,
 /*   270 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1612,
 /*   280 */  1319, 1254, 1530, 1373, 1269, 1269, 1269, 1269, 1271, 1264,
 /*   290 */  1260, 1372, 1293, 1633, 1335, 1254, 1330, 1414, 1655, 1435,
 /*   300 */  1424, 1270, 1424, 1652, 1276, 1435, 1435, 1276, 1435, 1270,
 /*   310 */  1652, 1414, 1414, 1405, 1405, 1405, 1405, 1272, 1272, 1498,
 /*   320 */  1270, 1264, 1508, 1361, 1361, 1361, 1353, 1254, 1254, 1361,
 /*   330 */  1350, 1508, 1361, 1335, 1361, 1350, 1361, 1655, 1655, 1391,
 /*   340 */  1391, 1654, 1654, 1391, 1518, 1639, 1444, 1363, 1369, 1369,
 /*   350 */  1369, 1369, 1391, 1304, 1276, 1639, 1639, 1276, 1444, 1363,
 /*   360 */  1276, 1363, 1276, 1391, 1304, 1534, 1649, 1391, 1304, 1254,
 /*   370 */  1512, 1512, 1508, 1391, 1275, 1272, 1597, 1597, 1417, 1417,
 /*   380 */  1605, 1503, 1391, 1254, 1275, 1274, 1273, 1276, 1508, 1391,
 /*   390 */  1304, 1391, 1304, 1615, 1615, 1611, 1611, 1611, 1626, 1626,
 /*   400 */  1337, 1660, 1660, 1567, 1337, 1319, 1319, 1626, 1258, 1258,
 /*   410 */  1258, 1258, 1258, 1254, 1254, 1254, 1254, 1254, 1621, 1254,
 /*   420 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1562, 1519, 1395,
 /*   430 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1573, 1254,
 /*   440 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   450 */  1448, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   460 */  1254, 1254, 1308, 1254, 1254, 1254, 1564, 1333, 1254, 1254,
 /*   470 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   480 */  1254, 1254, 1426, 1427, 1396, 1254, 1254, 1254, 1254, 1254,
 /*   490 */  1254, 1254, 1441, 1254, 1254, 1254, 1436, 1254, 1254, 1254,
 /*   500 */  1254, 1254, 1254, 1254, 1254, 1651, 1254, 1254, 1254, 1254,
 /*   510 */  1254, 1254, 1533, 1532, 1254, 1254, 1393, 1254, 1254, 1254,
 /*   520 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1418, 1254,
 /*   530 */  1254, 1254, 1254, 1602, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   540 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   550 */  1409, 1254, 1254, 1254, 1254, 1254, 1642, 1254, 1254, 1254,
 /*   560 */  1254, 1254, 1254, 1254, 1254, 1636, 1254, 1254, 1254, 1254,
 /*   570 */  1254, 1380, 1279, 1449, 1254, 1254, 1375, 1254, 1254, 1254,
 /*   580 */  1298, 1254, 1290,
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
  /*  187 */ "explain",
  /*  188 */ "cmdx",
  /*  189 */ "cmd",
  /*  190 */ "transtype",
  /*  191 */ "trans_opt",
  /*  192 */ "nm",
  /*  193 */ "savepoint_opt",
  /*  194 */ "create_table",
  /*  195 */ "createkw",
  /*  196 */ "temp",
  /*  197 */ "ifnotexists",
  /*  198 */ "dbnm",
  /*  199 */ "create_table_args",
  /*  200 */ "columnlist",
  /*  201 */ "conslist_opt",
  /*  202 */ "table_option_set",
  /*  203 */ "select",
  /*  204 */ "table_option",
  /*  205 */ "columnname",
  /*  206 */ "typetoken",
  /*  207 */ "typename",
  /*  208 */ "signed",
  /*  209 */ "scanpt",
  /*  210 */ "scantok",
  /*  211 */ "ccons",
  /*  212 */ "term",
  /*  213 */ "expr",
  /*  214 */ "onconf",
  /*  215 */ "sortorder",
  /*  216 */ "autoinc",
  /*  217 */ "eidlist_opt",
  /*  218 */ "refargs",
  /*  219 */ "defer_subclause",
  /*  220 */ "generated",
  /*  221 */ "refarg",
  /*  222 */ "refact",
  /*  223 */ "init_deferred_pred_opt",
  /*  224 */ "tconscomma",
  /*  225 */ "tcons",
  /*  226 */ "sortlist",
  /*  227 */ "eidlist",
  /*  228 */ "defer_subclause_opt",
  /*  229 */ "resolvetype",
  /*  230 */ "orconf",
  /*  231 */ "ifexists",
  /*  232 */ "fullname",
  /*  233 */ "wqlist",
  /*  234 */ "selectnowith",
  /*  235 */ "multiselect_op",
  /*  236 */ "oneselect",
  /*  237 */ "distinct",
  /*  238 */ "selcollist",
  /*  239 */ "from",
  /*  240 */ "where_opt",
  /*  241 */ "groupby_opt",
  /*  242 */ "having_opt",
  /*  243 */ "orderby_opt",
  /*  244 */ "limit_opt",
  /*  245 */ "window_clause",
  /*  246 */ "values",
  /*  247 */ "nexprlist",
  /*  248 */ "mvalues",
  /*  249 */ "sclp",
  /*  250 */ "as",
  /*  251 */ "seltablist",
  /*  252 */ "stl_prefix",
  /*  253 */ "joinop",
  /*  254 */ "on_using",
  /*  255 */ "indexed_by",
  /*  256 */ "exprlist",
  /*  257 */ "xfullname",
  /*  258 */ "idlist",
  /*  259 */ "indexed_opt",
  /*  260 */ "nulls",
  /*  261 */ "with",
  /*  262 */ "where_opt_ret",
  /*  263 */ "setlist",
  /*  264 */ "insert_cmd",
  /*  265 */ "idlist_opt",
  /*  266 */ "upsert",
  /*  267 */ "returning",
  /*  268 */ "filter_over",
  /*  269 */ "likeop",
  /*  270 */ "between_op",
  /*  271 */ "in_op",
  /*  272 */ "paren_exprlist",
  /*  273 */ "case_operand",
  /*  274 */ "case_exprlist",
  /*  275 */ "case_else",
  /*  276 */ "uniqueflag",
  /*  277 */ "collate",
  /*  278 */ "vinto",
  /*  279 */ "nmnum",
  /*  280 */ "minus_num",
  /*  281 */ "plus_num",
  /*  282 */ "trigger_decl",
  /*  283 */ "trigger_cmd_list",
  /*  284 */ "trigger_time",
  /*  285 */ "trigger_event",
  /*  286 */ "foreach_clause",
  /*  287 */ "when_clause",
  /*  288 */ "trigger_cmd",
  /*  289 */ "trnm",
  /*  290 */ "tridxby",
  /*  291 */ "raisetype",
  /*  292 */ "database_kw_opt",
  /*  293 */ "key_opt",
  /*  294 */ "add_column_fullname",
  /*  295 */ "kwcolumn_opt",
  /*  296 */ "carglist",
  /*  297 */ "create_vtab",
  /*  298 */ "vtabarglist",
  /*  299 */ "vtabarg",
  /*  300 */ "vtabargtoken",
  /*  301 */ "lp",
  /*  302 */ "anylist",
  /*  303 */ "wqas",
  /*  304 */ "wqitem",
  /*  305 */ "withnm",
  /*  306 */ "windowdefn_list",
  /*  307 */ "windowdefn",
  /*  308 */ "window",
  /*  309 */ "frame_opt",
  /*  310 */ "range_or_rows",
  /*  311 */ "frame_bound_s",
  /*  312 */ "frame_exclude_opt",
  /*  313 */ "frame_bound_e",
  /*  314 */ "frame_bound",
  /*  315 */ "frame_exclude",
  /*  316 */ "filter_clause",
  /*  317 */ "over_clause",
  /*  318 */ "input",
  /*  319 */ "cmdlist",
  /*  320 */ "ecmd",
  /*  321 */ "conslist",
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
 /*  11 */ "as ::= AS nm",
 /*  12 */ "as ::=",
 /*  13 */ "from ::=",
 /*  14 */ "from ::= FROM seltablist",
 /*  15 */ "orderby_opt ::=",
 /*  16 */ "groupby_opt ::=",
 /*  17 */ "having_opt ::=",
 /*  18 */ "limit_opt ::=",
 /*  19 */ "where_opt ::=",
 /*  20 */ "where_opt ::= WHERE expr",
 /*  21 */ "expr ::= LP expr RP",
 /*  22 */ "term ::= STRING",
 /*  23 */ "term ::= INTEGER",
 /*  24 */ "input ::= cmdlist",
 /*  25 */ "cmdlist ::= cmdlist ecmd",
 /*  26 */ "cmdlist ::= ecmd",
 /*  27 */ "ecmd ::= SEMI",
 /*  28 */ "ecmd ::= cmdx SEMI",
 /*  29 */ "nm ::= STRING",
 /*  30 */ "selectnowith ::= oneselect",
 /*  31 */ "sclp ::= selcollist COMMA",
 /*  32 */ "expr ::= term",
 /*  33 */ "explain ::= EXPLAIN",
 /*  34 */ "explain ::= EXPLAIN QUERY PLAN",
 /*  35 */ "cmd ::= BEGIN transtype trans_opt",
 /*  36 */ "transtype ::=",
 /*  37 */ "transtype ::= DEFERRED",
 /*  38 */ "transtype ::= IMMEDIATE",
 /*  39 */ "transtype ::= EXCLUSIVE",
 /*  40 */ "cmd ::= COMMIT|END trans_opt",
 /*  41 */ "cmd ::= ROLLBACK trans_opt",
 /*  42 */ "cmd ::= SAVEPOINT nm",
 /*  43 */ "cmd ::= RELEASE savepoint_opt nm",
 /*  44 */ "cmd ::= ROLLBACK trans_opt TO savepoint_opt nm",
 /*  45 */ "create_table ::= createkw temp TABLE ifnotexists nm dbnm",
 /*  46 */ "createkw ::= CREATE",
 /*  47 */ "ifnotexists ::=",
 /*  48 */ "ifnotexists ::= IF NOT EXISTS",
 /*  49 */ "temp ::= TEMP",
 /*  50 */ "temp ::=",
 /*  51 */ "create_table_args ::= LP columnlist conslist_opt RP table_option_set",
 /*  52 */ "create_table_args ::= AS select",
 /*  53 */ "table_option_set ::=",
 /*  54 */ "table_option_set ::= table_option_set COMMA table_option",
 /*  55 */ "table_option ::= WITHOUT nm",
 /*  56 */ "table_option ::= nm",
 /*  57 */ "columnname ::= nm typetoken",
 /*  58 */ "typetoken ::=",
 /*  59 */ "typetoken ::= typename LP signed RP",
 /*  60 */ "typetoken ::= typename LP signed COMMA signed RP",
 /*  61 */ "typename ::= typename ID|STRING",
 /*  62 */ "scantok ::=",
 /*  63 */ "ccons ::= CONSTRAINT nm",
 /*  64 */ "ccons ::= DEFAULT scantok term",
 /*  65 */ "ccons ::= DEFAULT LP expr RP",
 /*  66 */ "ccons ::= DEFAULT PLUS scantok term",
 /*  67 */ "ccons ::= DEFAULT MINUS scantok term",
 /*  68 */ "ccons ::= DEFAULT scantok ID|INDEXED",
 /*  69 */ "ccons ::= NOT NULL onconf",
 /*  70 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
 /*  71 */ "ccons ::= UNIQUE onconf",
 /*  72 */ "ccons ::= CHECK LP expr RP",
 /*  73 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
 /*  74 */ "ccons ::= defer_subclause",
 /*  75 */ "ccons ::= COLLATE ID|STRING",
 /*  76 */ "generated ::= LP expr RP",
 /*  77 */ "generated ::= LP expr RP ID",
 /*  78 */ "autoinc ::=",
 /*  79 */ "autoinc ::= AUTOINCR",
 /*  80 */ "refargs ::=",
 /*  81 */ "refargs ::= refargs refarg",
 /*  82 */ "refarg ::= MATCH nm",
 /*  83 */ "refarg ::= ON INSERT refact",
 /*  84 */ "refarg ::= ON DELETE refact",
 /*  85 */ "refarg ::= ON UPDATE refact",
 /*  86 */ "refact ::= SET NULL",
 /*  87 */ "refact ::= SET DEFAULT",
 /*  88 */ "refact ::= CASCADE",
 /*  89 */ "refact ::= RESTRICT",
 /*  90 */ "refact ::= NO ACTION",
 /*  91 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
 /*  92 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
 /*  93 */ "init_deferred_pred_opt ::=",
 /*  94 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
 /*  95 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
 /*  96 */ "conslist_opt ::=",
 /*  97 */ "tconscomma ::= COMMA",
 /*  98 */ "tcons ::= CONSTRAINT nm",
 /*  99 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf",
 /* 100 */ "tcons ::= UNIQUE LP sortlist RP onconf",
 /* 101 */ "tcons ::= CHECK LP expr RP onconf",
 /* 102 */ "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /* 103 */ "defer_subclause_opt ::=",
 /* 104 */ "onconf ::=",
 /* 105 */ "onconf ::= ON CONFLICT resolvetype",
 /* 106 */ "orconf ::=",
 /* 107 */ "orconf ::= OR resolvetype",
 /* 108 */ "resolvetype ::= IGNORE",
 /* 109 */ "resolvetype ::= REPLACE",
 /* 110 */ "cmd ::= DROP TABLE ifexists fullname",
 /* 111 */ "ifexists ::= IF EXISTS",
 /* 112 */ "ifexists ::=",
 /* 113 */ "cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select",
 /* 114 */ "cmd ::= DROP VIEW ifexists fullname",
 /* 115 */ "select ::= WITH wqlist selectnowith",
 /* 116 */ "select ::= WITH RECURSIVE wqlist selectnowith",
 /* 117 */ "selectnowith ::= selectnowith multiselect_op oneselect",
 /* 118 */ "multiselect_op ::= UNION",
 /* 119 */ "multiselect_op ::= UNION ALL",
 /* 120 */ "multiselect_op ::= EXCEPT|INTERSECT",
 /* 121 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt",
 /* 122 */ "values ::= VALUES LP nexprlist RP",
 /* 123 */ "oneselect ::= mvalues",
 /* 124 */ "mvalues ::= values COMMA LP nexprlist RP",
 /* 125 */ "mvalues ::= mvalues COMMA LP nexprlist RP",
 /* 126 */ "selcollist ::= sclp scanpt nm DOT STAR",
 /* 127 */ "stl_prefix ::= seltablist joinop",
 /* 128 */ "stl_prefix ::=",
 /* 129 */ "seltablist ::= stl_prefix nm dbnm as on_using",
 /* 130 */ "seltablist ::= stl_prefix nm dbnm as indexed_by on_using",
 /* 131 */ "seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using",
 /* 132 */ "seltablist ::= stl_prefix LP select RP as on_using",
 /* 133 */ "seltablist ::= stl_prefix LP seltablist RP as on_using",
 /* 134 */ "dbnm ::=",
 /* 135 */ "dbnm ::= DOT nm",
 /* 136 */ "fullname ::= nm",
 /* 137 */ "fullname ::= nm DOT nm",
 /* 138 */ "xfullname ::= nm",
 /* 139 */ "xfullname ::= nm DOT nm",
 /* 140 */ "xfullname ::= nm DOT nm AS nm",
 /* 141 */ "xfullname ::= nm AS nm",
 /* 142 */ "joinop ::= COMMA|JOIN",
 /* 143 */ "joinop ::= JOIN_KW JOIN",
 /* 144 */ "joinop ::= JOIN_KW nm JOIN",
 /* 145 */ "joinop ::= JOIN_KW nm nm JOIN",
 /* 146 */ "on_using ::= ON expr",
 /* 147 */ "on_using ::= USING LP idlist RP",
 /* 148 */ "on_using ::=",
 /* 149 */ "indexed_opt ::=",
 /* 150 */ "indexed_by ::= INDEXED BY nm",
 /* 151 */ "indexed_by ::= NOT INDEXED",
 /* 152 */ "orderby_opt ::= ORDER BY sortlist",
 /* 153 */ "sortlist ::= sortlist COMMA expr sortorder nulls",
 /* 154 */ "sortlist ::= expr sortorder nulls",
 /* 155 */ "sortorder ::= ASC",
 /* 156 */ "sortorder ::= DESC",
 /* 157 */ "sortorder ::=",
 /* 158 */ "nulls ::= NULLS FIRST",
 /* 159 */ "nulls ::= NULLS LAST",
 /* 160 */ "nulls ::=",
 /* 161 */ "groupby_opt ::= GROUP BY nexprlist",
 /* 162 */ "having_opt ::= HAVING expr",
 /* 163 */ "limit_opt ::= LIMIT expr",
 /* 164 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /* 165 */ "limit_opt ::= LIMIT expr COMMA expr",
 /* 166 */ "cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret",
 /* 167 */ "where_opt_ret ::=",
 /* 168 */ "where_opt_ret ::= WHERE expr",
 /* 169 */ "where_opt_ret ::= RETURNING selcollist",
 /* 170 */ "where_opt_ret ::= WHERE expr RETURNING selcollist",
 /* 171 */ "cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret",
 /* 172 */ "setlist ::= setlist COMMA nm EQ expr",
 /* 173 */ "setlist ::= setlist COMMA LP idlist RP EQ expr",
 /* 174 */ "setlist ::= nm EQ expr",
 /* 175 */ "setlist ::= LP idlist RP EQ expr",
 /* 176 */ "cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert",
 /* 177 */ "cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning",
 /* 178 */ "upsert ::=",
 /* 179 */ "upsert ::= RETURNING selcollist",
 /* 180 */ "upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert",
 /* 181 */ "upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert",
 /* 182 */ "upsert ::= ON CONFLICT DO NOTHING returning",
 /* 183 */ "upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning",
 /* 184 */ "returning ::= RETURNING selcollist",
 /* 185 */ "insert_cmd ::= INSERT orconf",
 /* 186 */ "insert_cmd ::= REPLACE",
 /* 187 */ "idlist_opt ::=",
 /* 188 */ "idlist_opt ::= LP idlist RP",
 /* 189 */ "idlist ::= idlist COMMA nm",
 /* 190 */ "idlist ::= nm",
 /* 191 */ "expr ::= ID|INDEXED|JOIN_KW",
 /* 192 */ "expr ::= nm DOT nm",
 /* 193 */ "expr ::= nm DOT nm DOT nm",
 /* 194 */ "term ::= NULL|FLOAT|BLOB",
 /* 195 */ "expr ::= VARIABLE",
 /* 196 */ "expr ::= expr COLLATE ID|STRING",
 /* 197 */ "expr ::= CAST LP expr AS typetoken RP",
 /* 198 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP",
 /* 199 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP",
 /* 200 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP",
 /* 201 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over",
 /* 202 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over",
 /* 203 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over",
 /* 204 */ "term ::= CTIME_KW",
 /* 205 */ "expr ::= LP nexprlist COMMA expr RP",
 /* 206 */ "expr ::= expr AND expr",
 /* 207 */ "expr ::= expr OR expr",
 /* 208 */ "expr ::= expr LT|GT|GE|LE expr",
 /* 209 */ "expr ::= expr EQ|NE expr",
 /* 210 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
 /* 211 */ "expr ::= expr PLUS|MINUS expr",
 /* 212 */ "expr ::= expr STAR|SLASH|REM expr",
 /* 213 */ "expr ::= expr CONCAT expr",
 /* 214 */ "likeop ::= NOT LIKE_KW|MATCH",
 /* 215 */ "expr ::= expr likeop expr",
 /* 216 */ "expr ::= expr likeop expr ESCAPE expr",
 /* 217 */ "expr ::= expr ISNULL|NOTNULL",
 /* 218 */ "expr ::= expr NOT NULL",
 /* 219 */ "expr ::= expr IS expr",
 /* 220 */ "expr ::= expr IS NOT expr",
 /* 221 */ "expr ::= expr IS NOT DISTINCT FROM expr",
 /* 222 */ "expr ::= expr IS DISTINCT FROM expr",
 /* 223 */ "expr ::= NOT expr",
 /* 224 */ "expr ::= BITNOT expr",
 /* 225 */ "expr ::= PLUS|MINUS expr",
 /* 226 */ "expr ::= expr PTR expr",
 /* 227 */ "between_op ::= BETWEEN",
 /* 228 */ "between_op ::= NOT BETWEEN",
 /* 229 */ "expr ::= expr between_op expr AND expr",
 /* 230 */ "in_op ::= IN",
 /* 231 */ "in_op ::= NOT IN",
 /* 232 */ "expr ::= expr in_op LP exprlist RP",
 /* 233 */ "expr ::= LP select RP",
 /* 234 */ "expr ::= expr in_op LP select RP",
 /* 235 */ "expr ::= expr in_op nm dbnm paren_exprlist",
 /* 236 */ "expr ::= EXISTS LP select RP",
 /* 237 */ "expr ::= CASE case_operand case_exprlist case_else END",
 /* 238 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
 /* 239 */ "case_exprlist ::= WHEN expr THEN expr",
 /* 240 */ "case_else ::= ELSE expr",
 /* 241 */ "case_else ::=",
 /* 242 */ "case_operand ::=",
 /* 243 */ "exprlist ::=",
 /* 244 */ "nexprlist ::= nexprlist COMMA expr",
 /* 245 */ "nexprlist ::= expr",
 /* 246 */ "paren_exprlist ::=",
 /* 247 */ "paren_exprlist ::= LP exprlist RP",
 /* 248 */ "cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt",
 /* 249 */ "uniqueflag ::= UNIQUE",
 /* 250 */ "uniqueflag ::=",
 /* 251 */ "eidlist_opt ::=",
 /* 252 */ "eidlist_opt ::= LP eidlist RP",
 /* 253 */ "eidlist ::= eidlist COMMA nm collate sortorder",
 /* 254 */ "eidlist ::= nm collate sortorder",
 /* 255 */ "collate ::=",
 /* 256 */ "collate ::= COLLATE ID|STRING",
 /* 257 */ "cmd ::= DROP INDEX ifexists fullname",
 /* 258 */ "cmd ::= VACUUM vinto",
 /* 259 */ "cmd ::= VACUUM nm vinto",
 /* 260 */ "vinto ::= INTO expr",
 /* 261 */ "vinto ::=",
 /* 262 */ "cmd ::= PRAGMA nm dbnm",
 /* 263 */ "cmd ::= PRAGMA nm dbnm EQ nmnum",
 /* 264 */ "cmd ::= PRAGMA nm dbnm LP nmnum RP",
 /* 265 */ "cmd ::= PRAGMA nm dbnm EQ minus_num",
 /* 266 */ "cmd ::= PRAGMA nm dbnm LP minus_num RP",
 /* 267 */ "plus_num ::= PLUS INTEGER|FLOAT",
 /* 268 */ "minus_num ::= MINUS INTEGER|FLOAT",
 /* 269 */ "cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END",
 /* 270 */ "trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause",
 /* 271 */ "trigger_time ::= BEFORE|AFTER",
 /* 272 */ "trigger_time ::= INSTEAD OF",
 /* 273 */ "trigger_time ::=",
 /* 274 */ "trigger_event ::= DELETE|INSERT",
 /* 275 */ "trigger_event ::= UPDATE",
 /* 276 */ "trigger_event ::= UPDATE OF idlist",
 /* 277 */ "when_clause ::=",
 /* 278 */ "when_clause ::= WHEN expr",
 /* 279 */ "trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI",
 /* 280 */ "trigger_cmd_list ::= trigger_cmd SEMI",
 /* 281 */ "trnm ::= nm DOT nm",
 /* 282 */ "tridxby ::= INDEXED BY nm",
 /* 283 */ "tridxby ::= NOT INDEXED",
 /* 284 */ "trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt",
 /* 285 */ "trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt",
 /* 286 */ "trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt",
 /* 287 */ "trigger_cmd ::= scanpt select scanpt",
 /* 288 */ "expr ::= RAISE LP IGNORE RP",
 /* 289 */ "expr ::= RAISE LP raisetype COMMA expr RP",
 /* 290 */ "raisetype ::= ROLLBACK",
 /* 291 */ "raisetype ::= ABORT",
 /* 292 */ "raisetype ::= FAIL",
 /* 293 */ "cmd ::= DROP TRIGGER ifexists fullname",
 /* 294 */ "cmd ::= ATTACH database_kw_opt expr AS expr key_opt",
 /* 295 */ "cmd ::= DETACH database_kw_opt expr",
 /* 296 */ "key_opt ::=",
 /* 297 */ "key_opt ::= KEY expr",
 /* 298 */ "cmd ::= REINDEX",
 /* 299 */ "cmd ::= REINDEX nm dbnm",
 /* 300 */ "cmd ::= ANALYZE",
 /* 301 */ "cmd ::= ANALYZE nm dbnm",
 /* 302 */ "cmd ::= ALTER TABLE fullname RENAME TO nm",
 /* 303 */ "cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist",
 /* 304 */ "cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm",
 /* 305 */ "add_column_fullname ::= fullname",
 /* 306 */ "cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm",
 /* 307 */ "cmd ::= create_vtab",
 /* 308 */ "cmd ::= create_vtab LP vtabarglist RP",
 /* 309 */ "create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm",
 /* 310 */ "vtabarg ::=",
 /* 311 */ "vtabargtoken ::= ANY",
 /* 312 */ "vtabargtoken ::= lp anylist RP",
 /* 313 */ "lp ::= LP",
 /* 314 */ "with ::= WITH wqlist",
 /* 315 */ "with ::= WITH RECURSIVE wqlist",
 /* 316 */ "wqas ::= AS",
 /* 317 */ "wqas ::= AS MATERIALIZED",
 /* 318 */ "wqas ::= AS NOT MATERIALIZED",
 /* 319 */ "wqitem ::= withnm eidlist_opt wqas LP select RP",
 /* 320 */ "withnm ::= nm",
 /* 321 */ "wqlist ::= wqitem",
 /* 322 */ "wqlist ::= wqlist COMMA wqitem",
 /* 323 */ "windowdefn_list ::= windowdefn_list COMMA windowdefn",
 /* 324 */ "windowdefn ::= nm AS LP window RP",
 /* 325 */ "window ::= PARTITION BY nexprlist orderby_opt frame_opt",
 /* 326 */ "window ::= nm PARTITION BY nexprlist orderby_opt frame_opt",
 /* 327 */ "window ::= ORDER BY sortlist frame_opt",
 /* 328 */ "window ::= nm ORDER BY sortlist frame_opt",
 /* 329 */ "window ::= nm frame_opt",
 /* 330 */ "frame_opt ::=",
 /* 331 */ "frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt",
 /* 332 */ "frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt",
 /* 333 */ "range_or_rows ::= RANGE|ROWS|GROUPS",
 /* 334 */ "frame_bound_s ::= frame_bound",
 /* 335 */ "frame_bound_s ::= UNBOUNDED PRECEDING",
 /* 336 */ "frame_bound_e ::= frame_bound",
 /* 337 */ "frame_bound_e ::= UNBOUNDED FOLLOWING",
 /* 338 */ "frame_bound ::= expr PRECEDING|FOLLOWING",
 /* 339 */ "frame_bound ::= CURRENT ROW",
 /* 340 */ "frame_exclude_opt ::=",
 /* 341 */ "frame_exclude_opt ::= EXCLUDE frame_exclude",
 /* 342 */ "frame_exclude ::= NO OTHERS",
 /* 343 */ "frame_exclude ::= CURRENT ROW",
 /* 344 */ "frame_exclude ::= GROUP|TIES",
 /* 345 */ "window_clause ::= WINDOW windowdefn_list",
 /* 346 */ "filter_over ::= filter_clause over_clause",
 /* 347 */ "filter_over ::= over_clause",
 /* 348 */ "filter_over ::= filter_clause",
 /* 349 */ "over_clause ::= OVER LP window RP",
 /* 350 */ "over_clause ::= OVER nm",
 /* 351 */ "filter_clause ::= FILTER LP WHERE expr RP",
 /* 352 */ "term ::= QNUMBER",
 /* 353 */ "ecmd ::= explain cmdx SEMI",
 /* 354 */ "trans_opt ::=",
 /* 355 */ "trans_opt ::= TRANSACTION",
 /* 356 */ "trans_opt ::= TRANSACTION nm",
 /* 357 */ "savepoint_opt ::= SAVEPOINT",
 /* 358 */ "savepoint_opt ::=",
 /* 359 */ "cmd ::= create_table create_table_args",
 /* 360 */ "table_option_set ::= table_option",
 /* 361 */ "columnlist ::= columnlist COMMA columnname carglist",
 /* 362 */ "columnlist ::= columnname carglist",
 /* 363 */ "nm ::= ID|INDEXED|JOIN_KW",
 /* 364 */ "typetoken ::= typename",
 /* 365 */ "typename ::= ID|STRING",
 /* 366 */ "signed ::= plus_num",
 /* 367 */ "signed ::= minus_num",
 /* 368 */ "carglist ::= carglist ccons",
 /* 369 */ "carglist ::=",
 /* 370 */ "ccons ::= NULL onconf",
 /* 371 */ "ccons ::= GENERATED ALWAYS AS generated",
 /* 372 */ "ccons ::= AS generated",
 /* 373 */ "conslist_opt ::= COMMA conslist",
 /* 374 */ "conslist ::= conslist tconscomma tcons",
 /* 375 */ "conslist ::= tcons",
 /* 376 */ "tconscomma ::=",
 /* 377 */ "defer_subclause_opt ::= defer_subclause",
 /* 378 */ "resolvetype ::= raisetype",
 /* 379 */ "oneselect ::= values",
 /* 380 */ "as ::= ID|STRING",
 /* 381 */ "indexed_opt ::= indexed_by",
 /* 382 */ "returning ::=",
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
   188,  /* (0) cmdx ::= cmd */
   209,  /* (1) scanpt ::= */
   189,  /* (2) cmd ::= select */
   203,  /* (3) select ::= selectnowith */
   236,  /* (4) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
   237,  /* (5) distinct ::= DISTINCT */
   237,  /* (6) distinct ::= ALL */
   237,  /* (7) distinct ::= */
   249,  /* (8) sclp ::= */
   238,  /* (9) selcollist ::= sclp scanpt expr scanpt as */
   238,  /* (10) selcollist ::= sclp scanpt STAR */
   250,  /* (11) as ::= AS nm */
   250,  /* (12) as ::= */
   239,  /* (13) from ::= */
   239,  /* (14) from ::= FROM seltablist */
   243,  /* (15) orderby_opt ::= */
   241,  /* (16) groupby_opt ::= */
   242,  /* (17) having_opt ::= */
   244,  /* (18) limit_opt ::= */
   240,  /* (19) where_opt ::= */
   240,  /* (20) where_opt ::= WHERE expr */
   213,  /* (21) expr ::= LP expr RP */
   212,  /* (22) term ::= STRING */
   212,  /* (23) term ::= INTEGER */
   318,  /* (24) input ::= cmdlist */
   319,  /* (25) cmdlist ::= cmdlist ecmd */
   319,  /* (26) cmdlist ::= ecmd */
   320,  /* (27) ecmd ::= SEMI */
   320,  /* (28) ecmd ::= cmdx SEMI */
   192,  /* (29) nm ::= STRING */
   234,  /* (30) selectnowith ::= oneselect */
   249,  /* (31) sclp ::= selcollist COMMA */
   213,  /* (32) expr ::= term */
   187,  /* (33) explain ::= EXPLAIN */
   187,  /* (34) explain ::= EXPLAIN QUERY PLAN */
   189,  /* (35) cmd ::= BEGIN transtype trans_opt */
   190,  /* (36) transtype ::= */
   190,  /* (37) transtype ::= DEFERRED */
   190,  /* (38) transtype ::= IMMEDIATE */
   190,  /* (39) transtype ::= EXCLUSIVE */
   189,  /* (40) cmd ::= COMMIT|END trans_opt */
   189,  /* (41) cmd ::= ROLLBACK trans_opt */
   189,  /* (42) cmd ::= SAVEPOINT nm */
   189,  /* (43) cmd ::= RELEASE savepoint_opt nm */
   189,  /* (44) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
   194,  /* (45) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   195,  /* (46) createkw ::= CREATE */
   197,  /* (47) ifnotexists ::= */
   197,  /* (48) ifnotexists ::= IF NOT EXISTS */
   196,  /* (49) temp ::= TEMP */
   196,  /* (50) temp ::= */
   199,  /* (51) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   199,  /* (52) create_table_args ::= AS select */
   202,  /* (53) table_option_set ::= */
   202,  /* (54) table_option_set ::= table_option_set COMMA table_option */
   204,  /* (55) table_option ::= WITHOUT nm */
   204,  /* (56) table_option ::= nm */
   205,  /* (57) columnname ::= nm typetoken */
   206,  /* (58) typetoken ::= */
   206,  /* (59) typetoken ::= typename LP signed RP */
   206,  /* (60) typetoken ::= typename LP signed COMMA signed RP */
   207,  /* (61) typename ::= typename ID|STRING */
   210,  /* (62) scantok ::= */
   211,  /* (63) ccons ::= CONSTRAINT nm */
   211,  /* (64) ccons ::= DEFAULT scantok term */
   211,  /* (65) ccons ::= DEFAULT LP expr RP */
   211,  /* (66) ccons ::= DEFAULT PLUS scantok term */
   211,  /* (67) ccons ::= DEFAULT MINUS scantok term */
   211,  /* (68) ccons ::= DEFAULT scantok ID|INDEXED */
   211,  /* (69) ccons ::= NOT NULL onconf */
   211,  /* (70) ccons ::= PRIMARY KEY sortorder onconf autoinc */
   211,  /* (71) ccons ::= UNIQUE onconf */
   211,  /* (72) ccons ::= CHECK LP expr RP */
   211,  /* (73) ccons ::= REFERENCES nm eidlist_opt refargs */
   211,  /* (74) ccons ::= defer_subclause */
   211,  /* (75) ccons ::= COLLATE ID|STRING */
   220,  /* (76) generated ::= LP expr RP */
   220,  /* (77) generated ::= LP expr RP ID */
   216,  /* (78) autoinc ::= */
   216,  /* (79) autoinc ::= AUTOINCR */
   218,  /* (80) refargs ::= */
   218,  /* (81) refargs ::= refargs refarg */
   221,  /* (82) refarg ::= MATCH nm */
   221,  /* (83) refarg ::= ON INSERT refact */
   221,  /* (84) refarg ::= ON DELETE refact */
   221,  /* (85) refarg ::= ON UPDATE refact */
   222,  /* (86) refact ::= SET NULL */
   222,  /* (87) refact ::= SET DEFAULT */
   222,  /* (88) refact ::= CASCADE */
   222,  /* (89) refact ::= RESTRICT */
   222,  /* (90) refact ::= NO ACTION */
   219,  /* (91) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
   219,  /* (92) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
   223,  /* (93) init_deferred_pred_opt ::= */
   223,  /* (94) init_deferred_pred_opt ::= INITIALLY DEFERRED */
   223,  /* (95) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
   201,  /* (96) conslist_opt ::= */
   224,  /* (97) tconscomma ::= COMMA */
   225,  /* (98) tcons ::= CONSTRAINT nm */
   225,  /* (99) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
   225,  /* (100) tcons ::= UNIQUE LP sortlist RP onconf */
   225,  /* (101) tcons ::= CHECK LP expr RP onconf */
   225,  /* (102) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
   228,  /* (103) defer_subclause_opt ::= */
   214,  /* (104) onconf ::= */
   214,  /* (105) onconf ::= ON CONFLICT resolvetype */
   230,  /* (106) orconf ::= */
   230,  /* (107) orconf ::= OR resolvetype */
   229,  /* (108) resolvetype ::= IGNORE */
   229,  /* (109) resolvetype ::= REPLACE */
   189,  /* (110) cmd ::= DROP TABLE ifexists fullname */
   231,  /* (111) ifexists ::= IF EXISTS */
   231,  /* (112) ifexists ::= */
   189,  /* (113) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   189,  /* (114) cmd ::= DROP VIEW ifexists fullname */
   203,  /* (115) select ::= WITH wqlist selectnowith */
   203,  /* (116) select ::= WITH RECURSIVE wqlist selectnowith */
   234,  /* (117) selectnowith ::= selectnowith multiselect_op oneselect */
   235,  /* (118) multiselect_op ::= UNION */
   235,  /* (119) multiselect_op ::= UNION ALL */
   235,  /* (120) multiselect_op ::= EXCEPT|INTERSECT */
   236,  /* (121) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   246,  /* (122) values ::= VALUES LP nexprlist RP */
   236,  /* (123) oneselect ::= mvalues */
   248,  /* (124) mvalues ::= values COMMA LP nexprlist RP */
   248,  /* (125) mvalues ::= mvalues COMMA LP nexprlist RP */
   238,  /* (126) selcollist ::= sclp scanpt nm DOT STAR */
   252,  /* (127) stl_prefix ::= seltablist joinop */
   252,  /* (128) stl_prefix ::= */
   251,  /* (129) seltablist ::= stl_prefix nm dbnm as on_using */
   251,  /* (130) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   251,  /* (131) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   251,  /* (132) seltablist ::= stl_prefix LP select RP as on_using */
   251,  /* (133) seltablist ::= stl_prefix LP seltablist RP as on_using */
   198,  /* (134) dbnm ::= */
   198,  /* (135) dbnm ::= DOT nm */
   232,  /* (136) fullname ::= nm */
   232,  /* (137) fullname ::= nm DOT nm */
   257,  /* (138) xfullname ::= nm */
   257,  /* (139) xfullname ::= nm DOT nm */
   257,  /* (140) xfullname ::= nm DOT nm AS nm */
   257,  /* (141) xfullname ::= nm AS nm */
   253,  /* (142) joinop ::= COMMA|JOIN */
   253,  /* (143) joinop ::= JOIN_KW JOIN */
   253,  /* (144) joinop ::= JOIN_KW nm JOIN */
   253,  /* (145) joinop ::= JOIN_KW nm nm JOIN */
   254,  /* (146) on_using ::= ON expr */
   254,  /* (147) on_using ::= USING LP idlist RP */
   254,  /* (148) on_using ::= */
   259,  /* (149) indexed_opt ::= */
   255,  /* (150) indexed_by ::= INDEXED BY nm */
   255,  /* (151) indexed_by ::= NOT INDEXED */
   243,  /* (152) orderby_opt ::= ORDER BY sortlist */
   226,  /* (153) sortlist ::= sortlist COMMA expr sortorder nulls */
   226,  /* (154) sortlist ::= expr sortorder nulls */
   215,  /* (155) sortorder ::= ASC */
   215,  /* (156) sortorder ::= DESC */
   215,  /* (157) sortorder ::= */
   260,  /* (158) nulls ::= NULLS FIRST */
   260,  /* (159) nulls ::= NULLS LAST */
   260,  /* (160) nulls ::= */
   241,  /* (161) groupby_opt ::= GROUP BY nexprlist */
   242,  /* (162) having_opt ::= HAVING expr */
   244,  /* (163) limit_opt ::= LIMIT expr */
   244,  /* (164) limit_opt ::= LIMIT expr OFFSET expr */
   244,  /* (165) limit_opt ::= LIMIT expr COMMA expr */
   189,  /* (166) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
   262,  /* (167) where_opt_ret ::= */
   262,  /* (168) where_opt_ret ::= WHERE expr */
   262,  /* (169) where_opt_ret ::= RETURNING selcollist */
   262,  /* (170) where_opt_ret ::= WHERE expr RETURNING selcollist */
   189,  /* (171) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   263,  /* (172) setlist ::= setlist COMMA nm EQ expr */
   263,  /* (173) setlist ::= setlist COMMA LP idlist RP EQ expr */
   263,  /* (174) setlist ::= nm EQ expr */
   263,  /* (175) setlist ::= LP idlist RP EQ expr */
   189,  /* (176) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   189,  /* (177) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
   266,  /* (178) upsert ::= */
   266,  /* (179) upsert ::= RETURNING selcollist */
   266,  /* (180) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
   266,  /* (181) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
   266,  /* (182) upsert ::= ON CONFLICT DO NOTHING returning */
   266,  /* (183) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
   267,  /* (184) returning ::= RETURNING selcollist */
   264,  /* (185) insert_cmd ::= INSERT orconf */
   264,  /* (186) insert_cmd ::= REPLACE */
   265,  /* (187) idlist_opt ::= */
   265,  /* (188) idlist_opt ::= LP idlist RP */
   258,  /* (189) idlist ::= idlist COMMA nm */
   258,  /* (190) idlist ::= nm */
   213,  /* (191) expr ::= ID|INDEXED|JOIN_KW */
   213,  /* (192) expr ::= nm DOT nm */
   213,  /* (193) expr ::= nm DOT nm DOT nm */
   212,  /* (194) term ::= NULL|FLOAT|BLOB */
   213,  /* (195) expr ::= VARIABLE */
   213,  /* (196) expr ::= expr COLLATE ID|STRING */
   213,  /* (197) expr ::= CAST LP expr AS typetoken RP */
   213,  /* (198) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   213,  /* (199) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   213,  /* (200) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   213,  /* (201) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   213,  /* (202) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   213,  /* (203) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   212,  /* (204) term ::= CTIME_KW */
   213,  /* (205) expr ::= LP nexprlist COMMA expr RP */
   213,  /* (206) expr ::= expr AND expr */
   213,  /* (207) expr ::= expr OR expr */
   213,  /* (208) expr ::= expr LT|GT|GE|LE expr */
   213,  /* (209) expr ::= expr EQ|NE expr */
   213,  /* (210) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
   213,  /* (211) expr ::= expr PLUS|MINUS expr */
   213,  /* (212) expr ::= expr STAR|SLASH|REM expr */
   213,  /* (213) expr ::= expr CONCAT expr */
   269,  /* (214) likeop ::= NOT LIKE_KW|MATCH */
   213,  /* (215) expr ::= expr likeop expr */
   213,  /* (216) expr ::= expr likeop expr ESCAPE expr */
   213,  /* (217) expr ::= expr ISNULL|NOTNULL */
   213,  /* (218) expr ::= expr NOT NULL */
   213,  /* (219) expr ::= expr IS expr */
   213,  /* (220) expr ::= expr IS NOT expr */
   213,  /* (221) expr ::= expr IS NOT DISTINCT FROM expr */
   213,  /* (222) expr ::= expr IS DISTINCT FROM expr */
   213,  /* (223) expr ::= NOT expr */
   213,  /* (224) expr ::= BITNOT expr */
   213,  /* (225) expr ::= PLUS|MINUS expr */
   213,  /* (226) expr ::= expr PTR expr */
   270,  /* (227) between_op ::= BETWEEN */
   270,  /* (228) between_op ::= NOT BETWEEN */
   213,  /* (229) expr ::= expr between_op expr AND expr */
   271,  /* (230) in_op ::= IN */
   271,  /* (231) in_op ::= NOT IN */
   213,  /* (232) expr ::= expr in_op LP exprlist RP */
   213,  /* (233) expr ::= LP select RP */
   213,  /* (234) expr ::= expr in_op LP select RP */
   213,  /* (235) expr ::= expr in_op nm dbnm paren_exprlist */
   213,  /* (236) expr ::= EXISTS LP select RP */
   213,  /* (237) expr ::= CASE case_operand case_exprlist case_else END */
   274,  /* (238) case_exprlist ::= case_exprlist WHEN expr THEN expr */
   274,  /* (239) case_exprlist ::= WHEN expr THEN expr */
   275,  /* (240) case_else ::= ELSE expr */
   275,  /* (241) case_else ::= */
   273,  /* (242) case_operand ::= */
   256,  /* (243) exprlist ::= */
   247,  /* (244) nexprlist ::= nexprlist COMMA expr */
   247,  /* (245) nexprlist ::= expr */
   272,  /* (246) paren_exprlist ::= */
   272,  /* (247) paren_exprlist ::= LP exprlist RP */
   189,  /* (248) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
   276,  /* (249) uniqueflag ::= UNIQUE */
   276,  /* (250) uniqueflag ::= */
   217,  /* (251) eidlist_opt ::= */
   217,  /* (252) eidlist_opt ::= LP eidlist RP */
   227,  /* (253) eidlist ::= eidlist COMMA nm collate sortorder */
   227,  /* (254) eidlist ::= nm collate sortorder */
   277,  /* (255) collate ::= */
   277,  /* (256) collate ::= COLLATE ID|STRING */
   189,  /* (257) cmd ::= DROP INDEX ifexists fullname */
   189,  /* (258) cmd ::= VACUUM vinto */
   189,  /* (259) cmd ::= VACUUM nm vinto */
   278,  /* (260) vinto ::= INTO expr */
   278,  /* (261) vinto ::= */
   189,  /* (262) cmd ::= PRAGMA nm dbnm */
   189,  /* (263) cmd ::= PRAGMA nm dbnm EQ nmnum */
   189,  /* (264) cmd ::= PRAGMA nm dbnm LP nmnum RP */
   189,  /* (265) cmd ::= PRAGMA nm dbnm EQ minus_num */
   189,  /* (266) cmd ::= PRAGMA nm dbnm LP minus_num RP */
   281,  /* (267) plus_num ::= PLUS INTEGER|FLOAT */
   280,  /* (268) minus_num ::= MINUS INTEGER|FLOAT */
   189,  /* (269) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
   282,  /* (270) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
   284,  /* (271) trigger_time ::= BEFORE|AFTER */
   284,  /* (272) trigger_time ::= INSTEAD OF */
   284,  /* (273) trigger_time ::= */
   285,  /* (274) trigger_event ::= DELETE|INSERT */
   285,  /* (275) trigger_event ::= UPDATE */
   285,  /* (276) trigger_event ::= UPDATE OF idlist */
   287,  /* (277) when_clause ::= */
   287,  /* (278) when_clause ::= WHEN expr */
   283,  /* (279) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
   283,  /* (280) trigger_cmd_list ::= trigger_cmd SEMI */
   289,  /* (281) trnm ::= nm DOT nm */
   290,  /* (282) tridxby ::= INDEXED BY nm */
   290,  /* (283) tridxby ::= NOT INDEXED */
   288,  /* (284) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
   288,  /* (285) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
   288,  /* (286) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
   288,  /* (287) trigger_cmd ::= scanpt select scanpt */
   213,  /* (288) expr ::= RAISE LP IGNORE RP */
   213,  /* (289) expr ::= RAISE LP raisetype COMMA expr RP */
   291,  /* (290) raisetype ::= ROLLBACK */
   291,  /* (291) raisetype ::= ABORT */
   291,  /* (292) raisetype ::= FAIL */
   189,  /* (293) cmd ::= DROP TRIGGER ifexists fullname */
   189,  /* (294) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
   189,  /* (295) cmd ::= DETACH database_kw_opt expr */
   293,  /* (296) key_opt ::= */
   293,  /* (297) key_opt ::= KEY expr */
   189,  /* (298) cmd ::= REINDEX */
   189,  /* (299) cmd ::= REINDEX nm dbnm */
   189,  /* (300) cmd ::= ANALYZE */
   189,  /* (301) cmd ::= ANALYZE nm dbnm */
   189,  /* (302) cmd ::= ALTER TABLE fullname RENAME TO nm */
   189,  /* (303) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
   189,  /* (304) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
   294,  /* (305) add_column_fullname ::= fullname */
   189,  /* (306) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
   189,  /* (307) cmd ::= create_vtab */
   189,  /* (308) cmd ::= create_vtab LP vtabarglist RP */
   297,  /* (309) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
   299,  /* (310) vtabarg ::= */
   300,  /* (311) vtabargtoken ::= ANY */
   300,  /* (312) vtabargtoken ::= lp anylist RP */
   301,  /* (313) lp ::= LP */
   261,  /* (314) with ::= WITH wqlist */
   261,  /* (315) with ::= WITH RECURSIVE wqlist */
   303,  /* (316) wqas ::= AS */
   303,  /* (317) wqas ::= AS MATERIALIZED */
   303,  /* (318) wqas ::= AS NOT MATERIALIZED */
   304,  /* (319) wqitem ::= withnm eidlist_opt wqas LP select RP */
   305,  /* (320) withnm ::= nm */
   233,  /* (321) wqlist ::= wqitem */
   233,  /* (322) wqlist ::= wqlist COMMA wqitem */
   306,  /* (323) windowdefn_list ::= windowdefn_list COMMA windowdefn */
   307,  /* (324) windowdefn ::= nm AS LP window RP */
   308,  /* (325) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
   308,  /* (326) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
   308,  /* (327) window ::= ORDER BY sortlist frame_opt */
   308,  /* (328) window ::= nm ORDER BY sortlist frame_opt */
   308,  /* (329) window ::= nm frame_opt */
   309,  /* (330) frame_opt ::= */
   309,  /* (331) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
   309,  /* (332) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
   310,  /* (333) range_or_rows ::= RANGE|ROWS|GROUPS */
   311,  /* (334) frame_bound_s ::= frame_bound */
   311,  /* (335) frame_bound_s ::= UNBOUNDED PRECEDING */
   313,  /* (336) frame_bound_e ::= frame_bound */
   313,  /* (337) frame_bound_e ::= UNBOUNDED FOLLOWING */
   314,  /* (338) frame_bound ::= expr PRECEDING|FOLLOWING */
   314,  /* (339) frame_bound ::= CURRENT ROW */
   312,  /* (340) frame_exclude_opt ::= */
   312,  /* (341) frame_exclude_opt ::= EXCLUDE frame_exclude */
   315,  /* (342) frame_exclude ::= NO OTHERS */
   315,  /* (343) frame_exclude ::= CURRENT ROW */
   315,  /* (344) frame_exclude ::= GROUP|TIES */
   245,  /* (345) window_clause ::= WINDOW windowdefn_list */
   268,  /* (346) filter_over ::= filter_clause over_clause */
   268,  /* (347) filter_over ::= over_clause */
   268,  /* (348) filter_over ::= filter_clause */
   317,  /* (349) over_clause ::= OVER LP window RP */
   317,  /* (350) over_clause ::= OVER nm */
   316,  /* (351) filter_clause ::= FILTER LP WHERE expr RP */
   212,  /* (352) term ::= QNUMBER */
   320,  /* (353) ecmd ::= explain cmdx SEMI */
   191,  /* (354) trans_opt ::= */
   191,  /* (355) trans_opt ::= TRANSACTION */
   191,  /* (356) trans_opt ::= TRANSACTION nm */
   193,  /* (357) savepoint_opt ::= SAVEPOINT */
   193,  /* (358) savepoint_opt ::= */
   189,  /* (359) cmd ::= create_table create_table_args */
   202,  /* (360) table_option_set ::= table_option */
   200,  /* (361) columnlist ::= columnlist COMMA columnname carglist */
   200,  /* (362) columnlist ::= columnname carglist */
   192,  /* (363) nm ::= ID|INDEXED|JOIN_KW */
   206,  /* (364) typetoken ::= typename */
   207,  /* (365) typename ::= ID|STRING */
   208,  /* (366) signed ::= plus_num */
   208,  /* (367) signed ::= minus_num */
   296,  /* (368) carglist ::= carglist ccons */
   296,  /* (369) carglist ::= */
   211,  /* (370) ccons ::= NULL onconf */
   211,  /* (371) ccons ::= GENERATED ALWAYS AS generated */
   211,  /* (372) ccons ::= AS generated */
   201,  /* (373) conslist_opt ::= COMMA conslist */
   321,  /* (374) conslist ::= conslist tconscomma tcons */
   321,  /* (375) conslist ::= tcons */
   224,  /* (376) tconscomma ::= */
   228,  /* (377) defer_subclause_opt ::= defer_subclause */
   229,  /* (378) resolvetype ::= raisetype */
   236,  /* (379) oneselect ::= values */
   250,  /* (380) as ::= ID|STRING */
   259,  /* (381) indexed_opt ::= indexed_by */
   267,  /* (382) returning ::= */
   269,  /* (383) likeop ::= LIKE_KW|MATCH */
   273,  /* (384) case_operand ::= expr */
   256,  /* (385) exprlist ::= nexprlist */
   279,  /* (386) nmnum ::= plus_num */
   279,  /* (387) nmnum ::= nm */
   279,  /* (388) nmnum ::= ON */
   279,  /* (389) nmnum ::= DELETE */
   279,  /* (390) nmnum ::= DEFAULT */
   281,  /* (391) plus_num ::= INTEGER|FLOAT */
   286,  /* (392) foreach_clause ::= */
   286,  /* (393) foreach_clause ::= FOR EACH ROW */
   289,  /* (394) trnm ::= nm */
   290,  /* (395) tridxby ::= */
   292,  /* (396) database_kw_opt ::= DATABASE */
   292,  /* (397) database_kw_opt ::= */
   295,  /* (398) kwcolumn_opt ::= */
   295,  /* (399) kwcolumn_opt ::= COLUMNKW */
   298,  /* (400) vtabarglist ::= vtabarg */
   298,  /* (401) vtabarglist ::= vtabarglist COMMA vtabarg */
   299,  /* (402) vtabarg ::= vtabarg vtabargtoken */
   302,  /* (403) anylist ::= */
   302,  /* (404) anylist ::= anylist LP anylist RP */
   302,  /* (405) anylist ::= anylist ANY */
   261,  /* (406) with ::= */
   306,  /* (407) windowdefn_list ::= windowdefn */
   308,  /* (408) window ::= frame_opt */
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
   -2,  /* (11) as ::= AS nm */
    0,  /* (12) as ::= */
    0,  /* (13) from ::= */
   -2,  /* (14) from ::= FROM seltablist */
    0,  /* (15) orderby_opt ::= */
    0,  /* (16) groupby_opt ::= */
    0,  /* (17) having_opt ::= */
    0,  /* (18) limit_opt ::= */
    0,  /* (19) where_opt ::= */
   -2,  /* (20) where_opt ::= WHERE expr */
   -3,  /* (21) expr ::= LP expr RP */
   -1,  /* (22) term ::= STRING */
   -1,  /* (23) term ::= INTEGER */
   -1,  /* (24) input ::= cmdlist */
   -2,  /* (25) cmdlist ::= cmdlist ecmd */
   -1,  /* (26) cmdlist ::= ecmd */
   -1,  /* (27) ecmd ::= SEMI */
   -2,  /* (28) ecmd ::= cmdx SEMI */
   -1,  /* (29) nm ::= STRING */
   -1,  /* (30) selectnowith ::= oneselect */
   -2,  /* (31) sclp ::= selcollist COMMA */
   -1,  /* (32) expr ::= term */
   -1,  /* (33) explain ::= EXPLAIN */
   -3,  /* (34) explain ::= EXPLAIN QUERY PLAN */
   -3,  /* (35) cmd ::= BEGIN transtype trans_opt */
    0,  /* (36) transtype ::= */
   -1,  /* (37) transtype ::= DEFERRED */
   -1,  /* (38) transtype ::= IMMEDIATE */
   -1,  /* (39) transtype ::= EXCLUSIVE */
   -2,  /* (40) cmd ::= COMMIT|END trans_opt */
   -2,  /* (41) cmd ::= ROLLBACK trans_opt */
   -2,  /* (42) cmd ::= SAVEPOINT nm */
   -3,  /* (43) cmd ::= RELEASE savepoint_opt nm */
   -5,  /* (44) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
   -6,  /* (45) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   -1,  /* (46) createkw ::= CREATE */
    0,  /* (47) ifnotexists ::= */
   -3,  /* (48) ifnotexists ::= IF NOT EXISTS */
   -1,  /* (49) temp ::= TEMP */
    0,  /* (50) temp ::= */
   -5,  /* (51) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   -2,  /* (52) create_table_args ::= AS select */
    0,  /* (53) table_option_set ::= */
   -3,  /* (54) table_option_set ::= table_option_set COMMA table_option */
   -2,  /* (55) table_option ::= WITHOUT nm */
   -1,  /* (56) table_option ::= nm */
   -2,  /* (57) columnname ::= nm typetoken */
    0,  /* (58) typetoken ::= */
   -4,  /* (59) typetoken ::= typename LP signed RP */
   -6,  /* (60) typetoken ::= typename LP signed COMMA signed RP */
   -2,  /* (61) typename ::= typename ID|STRING */
    0,  /* (62) scantok ::= */
   -2,  /* (63) ccons ::= CONSTRAINT nm */
   -3,  /* (64) ccons ::= DEFAULT scantok term */
   -4,  /* (65) ccons ::= DEFAULT LP expr RP */
   -4,  /* (66) ccons ::= DEFAULT PLUS scantok term */
   -4,  /* (67) ccons ::= DEFAULT MINUS scantok term */
   -3,  /* (68) ccons ::= DEFAULT scantok ID|INDEXED */
   -3,  /* (69) ccons ::= NOT NULL onconf */
   -5,  /* (70) ccons ::= PRIMARY KEY sortorder onconf autoinc */
   -2,  /* (71) ccons ::= UNIQUE onconf */
   -4,  /* (72) ccons ::= CHECK LP expr RP */
   -4,  /* (73) ccons ::= REFERENCES nm eidlist_opt refargs */
   -1,  /* (74) ccons ::= defer_subclause */
   -2,  /* (75) ccons ::= COLLATE ID|STRING */
   -3,  /* (76) generated ::= LP expr RP */
   -4,  /* (77) generated ::= LP expr RP ID */
    0,  /* (78) autoinc ::= */
   -1,  /* (79) autoinc ::= AUTOINCR */
    0,  /* (80) refargs ::= */
   -2,  /* (81) refargs ::= refargs refarg */
   -2,  /* (82) refarg ::= MATCH nm */
   -3,  /* (83) refarg ::= ON INSERT refact */
   -3,  /* (84) refarg ::= ON DELETE refact */
   -3,  /* (85) refarg ::= ON UPDATE refact */
   -2,  /* (86) refact ::= SET NULL */
   -2,  /* (87) refact ::= SET DEFAULT */
   -1,  /* (88) refact ::= CASCADE */
   -1,  /* (89) refact ::= RESTRICT */
   -2,  /* (90) refact ::= NO ACTION */
   -3,  /* (91) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
   -2,  /* (92) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
    0,  /* (93) init_deferred_pred_opt ::= */
   -2,  /* (94) init_deferred_pred_opt ::= INITIALLY DEFERRED */
   -2,  /* (95) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
    0,  /* (96) conslist_opt ::= */
   -1,  /* (97) tconscomma ::= COMMA */
   -2,  /* (98) tcons ::= CONSTRAINT nm */
   -7,  /* (99) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
   -5,  /* (100) tcons ::= UNIQUE LP sortlist RP onconf */
   -5,  /* (101) tcons ::= CHECK LP expr RP onconf */
  -10,  /* (102) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
    0,  /* (103) defer_subclause_opt ::= */
    0,  /* (104) onconf ::= */
   -3,  /* (105) onconf ::= ON CONFLICT resolvetype */
    0,  /* (106) orconf ::= */
   -2,  /* (107) orconf ::= OR resolvetype */
   -1,  /* (108) resolvetype ::= IGNORE */
   -1,  /* (109) resolvetype ::= REPLACE */
   -4,  /* (110) cmd ::= DROP TABLE ifexists fullname */
   -2,  /* (111) ifexists ::= IF EXISTS */
    0,  /* (112) ifexists ::= */
   -9,  /* (113) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   -4,  /* (114) cmd ::= DROP VIEW ifexists fullname */
   -3,  /* (115) select ::= WITH wqlist selectnowith */
   -4,  /* (116) select ::= WITH RECURSIVE wqlist selectnowith */
   -3,  /* (117) selectnowith ::= selectnowith multiselect_op oneselect */
   -1,  /* (118) multiselect_op ::= UNION */
   -2,  /* (119) multiselect_op ::= UNION ALL */
   -1,  /* (120) multiselect_op ::= EXCEPT|INTERSECT */
  -10,  /* (121) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   -4,  /* (122) values ::= VALUES LP nexprlist RP */
   -1,  /* (123) oneselect ::= mvalues */
   -5,  /* (124) mvalues ::= values COMMA LP nexprlist RP */
   -5,  /* (125) mvalues ::= mvalues COMMA LP nexprlist RP */
   -5,  /* (126) selcollist ::= sclp scanpt nm DOT STAR */
   -2,  /* (127) stl_prefix ::= seltablist joinop */
    0,  /* (128) stl_prefix ::= */
   -5,  /* (129) seltablist ::= stl_prefix nm dbnm as on_using */
   -6,  /* (130) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   -8,  /* (131) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   -6,  /* (132) seltablist ::= stl_prefix LP select RP as on_using */
   -6,  /* (133) seltablist ::= stl_prefix LP seltablist RP as on_using */
    0,  /* (134) dbnm ::= */
   -2,  /* (135) dbnm ::= DOT nm */
   -1,  /* (136) fullname ::= nm */
   -3,  /* (137) fullname ::= nm DOT nm */
   -1,  /* (138) xfullname ::= nm */
   -3,  /* (139) xfullname ::= nm DOT nm */
   -5,  /* (140) xfullname ::= nm DOT nm AS nm */
   -3,  /* (141) xfullname ::= nm AS nm */
   -1,  /* (142) joinop ::= COMMA|JOIN */
   -2,  /* (143) joinop ::= JOIN_KW JOIN */
   -3,  /* (144) joinop ::= JOIN_KW nm JOIN */
   -4,  /* (145) joinop ::= JOIN_KW nm nm JOIN */
   -2,  /* (146) on_using ::= ON expr */
   -4,  /* (147) on_using ::= USING LP idlist RP */
    0,  /* (148) on_using ::= */
    0,  /* (149) indexed_opt ::= */
   -3,  /* (150) indexed_by ::= INDEXED BY nm */
   -2,  /* (151) indexed_by ::= NOT INDEXED */
   -3,  /* (152) orderby_opt ::= ORDER BY sortlist */
   -5,  /* (153) sortlist ::= sortlist COMMA expr sortorder nulls */
   -3,  /* (154) sortlist ::= expr sortorder nulls */
   -1,  /* (155) sortorder ::= ASC */
   -1,  /* (156) sortorder ::= DESC */
    0,  /* (157) sortorder ::= */
   -2,  /* (158) nulls ::= NULLS FIRST */
   -2,  /* (159) nulls ::= NULLS LAST */
    0,  /* (160) nulls ::= */
   -3,  /* (161) groupby_opt ::= GROUP BY nexprlist */
   -2,  /* (162) having_opt ::= HAVING expr */
   -2,  /* (163) limit_opt ::= LIMIT expr */
   -4,  /* (164) limit_opt ::= LIMIT expr OFFSET expr */
   -4,  /* (165) limit_opt ::= LIMIT expr COMMA expr */
   -6,  /* (166) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
    0,  /* (167) where_opt_ret ::= */
   -2,  /* (168) where_opt_ret ::= WHERE expr */
   -2,  /* (169) where_opt_ret ::= RETURNING selcollist */
   -4,  /* (170) where_opt_ret ::= WHERE expr RETURNING selcollist */
   -9,  /* (171) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   -5,  /* (172) setlist ::= setlist COMMA nm EQ expr */
   -7,  /* (173) setlist ::= setlist COMMA LP idlist RP EQ expr */
   -3,  /* (174) setlist ::= nm EQ expr */
   -5,  /* (175) setlist ::= LP idlist RP EQ expr */
   -7,  /* (176) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   -8,  /* (177) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
    0,  /* (178) upsert ::= */
   -2,  /* (179) upsert ::= RETURNING selcollist */
  -12,  /* (180) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
   -9,  /* (181) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
   -5,  /* (182) upsert ::= ON CONFLICT DO NOTHING returning */
   -8,  /* (183) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
   -2,  /* (184) returning ::= RETURNING selcollist */
   -2,  /* (185) insert_cmd ::= INSERT orconf */
   -1,  /* (186) insert_cmd ::= REPLACE */
    0,  /* (187) idlist_opt ::= */
   -3,  /* (188) idlist_opt ::= LP idlist RP */
   -3,  /* (189) idlist ::= idlist COMMA nm */
   -1,  /* (190) idlist ::= nm */
   -1,  /* (191) expr ::= ID|INDEXED|JOIN_KW */
   -3,  /* (192) expr ::= nm DOT nm */
   -5,  /* (193) expr ::= nm DOT nm DOT nm */
   -1,  /* (194) term ::= NULL|FLOAT|BLOB */
   -1,  /* (195) expr ::= VARIABLE */
   -3,  /* (196) expr ::= expr COLLATE ID|STRING */
   -6,  /* (197) expr ::= CAST LP expr AS typetoken RP */
   -5,  /* (198) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   -8,  /* (199) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   -4,  /* (200) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   -6,  /* (201) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   -9,  /* (202) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   -5,  /* (203) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   -1,  /* (204) term ::= CTIME_KW */
   -5,  /* (205) expr ::= LP nexprlist COMMA expr RP */
   -3,  /* (206) expr ::= expr AND expr */
   -3,  /* (207) expr ::= expr OR expr */
   -3,  /* (208) expr ::= expr LT|GT|GE|LE expr */
   -3,  /* (209) expr ::= expr EQ|NE expr */
   -3,  /* (210) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
   -3,  /* (211) expr ::= expr PLUS|MINUS expr */
   -3,  /* (212) expr ::= expr STAR|SLASH|REM expr */
   -3,  /* (213) expr ::= expr CONCAT expr */
   -2,  /* (214) likeop ::= NOT LIKE_KW|MATCH */
   -3,  /* (215) expr ::= expr likeop expr */
   -5,  /* (216) expr ::= expr likeop expr ESCAPE expr */
   -2,  /* (217) expr ::= expr ISNULL|NOTNULL */
   -3,  /* (218) expr ::= expr NOT NULL */
   -3,  /* (219) expr ::= expr IS expr */
   -4,  /* (220) expr ::= expr IS NOT expr */
   -6,  /* (221) expr ::= expr IS NOT DISTINCT FROM expr */
   -5,  /* (222) expr ::= expr IS DISTINCT FROM expr */
   -2,  /* (223) expr ::= NOT expr */
   -2,  /* (224) expr ::= BITNOT expr */
   -2,  /* (225) expr ::= PLUS|MINUS expr */
   -3,  /* (226) expr ::= expr PTR expr */
   -1,  /* (227) between_op ::= BETWEEN */
   -2,  /* (228) between_op ::= NOT BETWEEN */
   -5,  /* (229) expr ::= expr between_op expr AND expr */
   -1,  /* (230) in_op ::= IN */
   -2,  /* (231) in_op ::= NOT IN */
   -5,  /* (232) expr ::= expr in_op LP exprlist RP */
   -3,  /* (233) expr ::= LP select RP */
   -5,  /* (234) expr ::= expr in_op LP select RP */
   -5,  /* (235) expr ::= expr in_op nm dbnm paren_exprlist */
   -4,  /* (236) expr ::= EXISTS LP select RP */
   -5,  /* (237) expr ::= CASE case_operand case_exprlist case_else END */
   -5,  /* (238) case_exprlist ::= case_exprlist WHEN expr THEN expr */
   -4,  /* (239) case_exprlist ::= WHEN expr THEN expr */
   -2,  /* (240) case_else ::= ELSE expr */
    0,  /* (241) case_else ::= */
    0,  /* (242) case_operand ::= */
    0,  /* (243) exprlist ::= */
   -3,  /* (244) nexprlist ::= nexprlist COMMA expr */
   -1,  /* (245) nexprlist ::= expr */
    0,  /* (246) paren_exprlist ::= */
   -3,  /* (247) paren_exprlist ::= LP exprlist RP */
  -12,  /* (248) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
   -1,  /* (249) uniqueflag ::= UNIQUE */
    0,  /* (250) uniqueflag ::= */
    0,  /* (251) eidlist_opt ::= */
   -3,  /* (252) eidlist_opt ::= LP eidlist RP */
   -5,  /* (253) eidlist ::= eidlist COMMA nm collate sortorder */
   -3,  /* (254) eidlist ::= nm collate sortorder */
    0,  /* (255) collate ::= */
   -2,  /* (256) collate ::= COLLATE ID|STRING */
   -4,  /* (257) cmd ::= DROP INDEX ifexists fullname */
   -2,  /* (258) cmd ::= VACUUM vinto */
   -3,  /* (259) cmd ::= VACUUM nm vinto */
   -2,  /* (260) vinto ::= INTO expr */
    0,  /* (261) vinto ::= */
   -3,  /* (262) cmd ::= PRAGMA nm dbnm */
   -5,  /* (263) cmd ::= PRAGMA nm dbnm EQ nmnum */
   -6,  /* (264) cmd ::= PRAGMA nm dbnm LP nmnum RP */
   -5,  /* (265) cmd ::= PRAGMA nm dbnm EQ minus_num */
   -6,  /* (266) cmd ::= PRAGMA nm dbnm LP minus_num RP */
   -2,  /* (267) plus_num ::= PLUS INTEGER|FLOAT */
   -2,  /* (268) minus_num ::= MINUS INTEGER|FLOAT */
   -5,  /* (269) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
  -11,  /* (270) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
   -1,  /* (271) trigger_time ::= BEFORE|AFTER */
   -2,  /* (272) trigger_time ::= INSTEAD OF */
    0,  /* (273) trigger_time ::= */
   -1,  /* (274) trigger_event ::= DELETE|INSERT */
   -1,  /* (275) trigger_event ::= UPDATE */
   -3,  /* (276) trigger_event ::= UPDATE OF idlist */
    0,  /* (277) when_clause ::= */
   -2,  /* (278) when_clause ::= WHEN expr */
   -3,  /* (279) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
   -2,  /* (280) trigger_cmd_list ::= trigger_cmd SEMI */
   -3,  /* (281) trnm ::= nm DOT nm */
   -3,  /* (282) tridxby ::= INDEXED BY nm */
   -2,  /* (283) tridxby ::= NOT INDEXED */
   -9,  /* (284) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
   -8,  /* (285) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
   -6,  /* (286) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
   -3,  /* (287) trigger_cmd ::= scanpt select scanpt */
   -4,  /* (288) expr ::= RAISE LP IGNORE RP */
   -6,  /* (289) expr ::= RAISE LP raisetype COMMA expr RP */
   -1,  /* (290) raisetype ::= ROLLBACK */
   -1,  /* (291) raisetype ::= ABORT */
   -1,  /* (292) raisetype ::= FAIL */
   -4,  /* (293) cmd ::= DROP TRIGGER ifexists fullname */
   -6,  /* (294) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
   -3,  /* (295) cmd ::= DETACH database_kw_opt expr */
    0,  /* (296) key_opt ::= */
   -2,  /* (297) key_opt ::= KEY expr */
   -1,  /* (298) cmd ::= REINDEX */
   -3,  /* (299) cmd ::= REINDEX nm dbnm */
   -1,  /* (300) cmd ::= ANALYZE */
   -3,  /* (301) cmd ::= ANALYZE nm dbnm */
   -6,  /* (302) cmd ::= ALTER TABLE fullname RENAME TO nm */
   -7,  /* (303) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
   -6,  /* (304) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
   -1,  /* (305) add_column_fullname ::= fullname */
   -8,  /* (306) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
   -1,  /* (307) cmd ::= create_vtab */
   -4,  /* (308) cmd ::= create_vtab LP vtabarglist RP */
   -8,  /* (309) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
    0,  /* (310) vtabarg ::= */
   -1,  /* (311) vtabargtoken ::= ANY */
   -3,  /* (312) vtabargtoken ::= lp anylist RP */
   -1,  /* (313) lp ::= LP */
   -2,  /* (314) with ::= WITH wqlist */
   -3,  /* (315) with ::= WITH RECURSIVE wqlist */
   -1,  /* (316) wqas ::= AS */
   -2,  /* (317) wqas ::= AS MATERIALIZED */
   -3,  /* (318) wqas ::= AS NOT MATERIALIZED */
   -6,  /* (319) wqitem ::= withnm eidlist_opt wqas LP select RP */
   -1,  /* (320) withnm ::= nm */
   -1,  /* (321) wqlist ::= wqitem */
   -3,  /* (322) wqlist ::= wqlist COMMA wqitem */
   -3,  /* (323) windowdefn_list ::= windowdefn_list COMMA windowdefn */
   -5,  /* (324) windowdefn ::= nm AS LP window RP */
   -5,  /* (325) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
   -6,  /* (326) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
   -4,  /* (327) window ::= ORDER BY sortlist frame_opt */
   -5,  /* (328) window ::= nm ORDER BY sortlist frame_opt */
   -2,  /* (329) window ::= nm frame_opt */
    0,  /* (330) frame_opt ::= */
   -3,  /* (331) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
   -6,  /* (332) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
   -1,  /* (333) range_or_rows ::= RANGE|ROWS|GROUPS */
   -1,  /* (334) frame_bound_s ::= frame_bound */
   -2,  /* (335) frame_bound_s ::= UNBOUNDED PRECEDING */
   -1,  /* (336) frame_bound_e ::= frame_bound */
   -2,  /* (337) frame_bound_e ::= UNBOUNDED FOLLOWING */
   -2,  /* (338) frame_bound ::= expr PRECEDING|FOLLOWING */
   -2,  /* (339) frame_bound ::= CURRENT ROW */
    0,  /* (340) frame_exclude_opt ::= */
   -2,  /* (341) frame_exclude_opt ::= EXCLUDE frame_exclude */
   -2,  /* (342) frame_exclude ::= NO OTHERS */
   -2,  /* (343) frame_exclude ::= CURRENT ROW */
   -1,  /* (344) frame_exclude ::= GROUP|TIES */
   -2,  /* (345) window_clause ::= WINDOW windowdefn_list */
   -2,  /* (346) filter_over ::= filter_clause over_clause */
   -1,  /* (347) filter_over ::= over_clause */
   -1,  /* (348) filter_over ::= filter_clause */
   -4,  /* (349) over_clause ::= OVER LP window RP */
   -2,  /* (350) over_clause ::= OVER nm */
   -5,  /* (351) filter_clause ::= FILTER LP WHERE expr RP */
   -1,  /* (352) term ::= QNUMBER */
   -3,  /* (353) ecmd ::= explain cmdx SEMI */
    0,  /* (354) trans_opt ::= */
   -1,  /* (355) trans_opt ::= TRANSACTION */
   -2,  /* (356) trans_opt ::= TRANSACTION nm */
   -1,  /* (357) savepoint_opt ::= SAVEPOINT */
    0,  /* (358) savepoint_opt ::= */
   -2,  /* (359) cmd ::= create_table create_table_args */
   -1,  /* (360) table_option_set ::= table_option */
   -4,  /* (361) columnlist ::= columnlist COMMA columnname carglist */
   -2,  /* (362) columnlist ::= columnname carglist */
   -1,  /* (363) nm ::= ID|INDEXED|JOIN_KW */
   -1,  /* (364) typetoken ::= typename */
   -1,  /* (365) typename ::= ID|STRING */
   -1,  /* (366) signed ::= plus_num */
   -1,  /* (367) signed ::= minus_num */
   -2,  /* (368) carglist ::= carglist ccons */
    0,  /* (369) carglist ::= */
   -2,  /* (370) ccons ::= NULL onconf */
   -4,  /* (371) ccons ::= GENERATED ALWAYS AS generated */
   -2,  /* (372) ccons ::= AS generated */
   -2,  /* (373) conslist_opt ::= COMMA conslist */
   -3,  /* (374) conslist ::= conslist tconscomma tcons */
   -1,  /* (375) conslist ::= tcons */
    0,  /* (376) tconscomma ::= */
   -1,  /* (377) defer_subclause_opt ::= defer_subclause */
   -1,  /* (378) resolvetype ::= raisetype */
   -1,  /* (379) oneselect ::= values */
   -1,  /* (380) as ::= ID|STRING */
   -1,  /* (381) indexed_opt ::= indexed_by */
    0,  /* (382) returning ::= */
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
      case 26: /* cmdlist ::= ecmd */ yytestcase(yyruleno==26);
      case 30: /* selectnowith ::= oneselect */ yytestcase(yyruleno==30);
      case 32: /* expr ::= term */ yytestcase(yyruleno==32);
{
    yylhsminor.yy391 = yymsp[0].minor.yy391;
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 1: /* scanpt ::= */
      case 12: /* as ::= */ yytestcase(yyruleno==12);
{
    yymsp[1].minor.yy391.z = NULL; yymsp[1].minor.yy391.n = 0;
}
        break;
      case 4: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
{
    yymsp[-8].minor.yy391 = ast_select_stmt(pCtx->astCtx, (u8)yymsp[-7].minor.yy391, yymsp[-6].minor.yy391);
}
        break;
      case 5: /* distinct ::= DISTINCT */
{
    yymsp[0].minor.yy391 = 1;
}
        break;
      case 6: /* distinct ::= ALL */
{
    yymsp[0].minor.yy391 = 0;
}
        break;
      case 7: /* distinct ::= */
{
    yymsp[1].minor.yy391 = 0;
}
        break;
      case 8: /* sclp ::= */
      case 13: /* from ::= */ yytestcase(yyruleno==13);
      case 15: /* orderby_opt ::= */ yytestcase(yyruleno==15);
      case 16: /* groupby_opt ::= */ yytestcase(yyruleno==16);
      case 17: /* having_opt ::= */ yytestcase(yyruleno==17);
      case 18: /* limit_opt ::= */ yytestcase(yyruleno==18);
      case 19: /* where_opt ::= */ yytestcase(yyruleno==19);
{
    yymsp[1].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 9: /* selcollist ::= sclp scanpt expr scanpt as */
{
    u32 col = ast_result_column(pCtx->astCtx, 0, 0, 0, yymsp[-2].minor.yy391);
    yylhsminor.yy391 = ast_result_column_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, col);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 10: /* selcollist ::= sclp scanpt STAR */
{
    u32 col = ast_result_column(pCtx->astCtx, 1, 0, 0, SYNTAQLITE_NULL_NODE);
    yylhsminor.yy391 = ast_result_column_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, col);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 11: /* as ::= AS nm */
      case 14: /* from ::= FROM seltablist */ yytestcase(yyruleno==14);
      case 20: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==20);
{
    yymsp[-1].minor.yy391 = yymsp[0].minor.yy391;
}
        break;
      case 21: /* expr ::= LP expr RP */
{
    yymsp[-2].minor.yy391 = yymsp[-1].minor.yy391;
}
        break;
      case 22: /* term ::= STRING */
{
    yylhsminor.yy391 = ast_literal(pCtx->astCtx, 2, (u32)(yymsp[0].minor.yy0.z - pCtx->zSql), (u16)yymsp[0].minor.yy0.n);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 23: /* term ::= INTEGER */
{
    yylhsminor.yy391 = ast_literal(pCtx->astCtx, 0, (u32)(yymsp[0].minor.yy0.z - pCtx->zSql), (u16)yymsp[0].minor.yy0.n);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 24: /* input ::= cmdlist */
{
    pCtx->root = yymsp[0].minor.yy391;
}
        break;
      case 25: /* cmdlist ::= cmdlist ecmd */
{
    yymsp[-1].minor.yy391 = yymsp[0].minor.yy391;  // Just use the last command for now
}
        break;
      case 27: /* ecmd ::= SEMI */
{
    yymsp[0].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 28: /* ecmd ::= cmdx SEMI */
      case 31: /* sclp ::= selcollist COMMA */ yytestcase(yyruleno==31);
{
    yylhsminor.yy391 = yymsp[-1].minor.yy391;
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 29: /* nm ::= STRING */
{
    yylhsminor.yy391 = yymsp[0].minor.yy0;
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      default:
      /* (33) explain ::= EXPLAIN */ yytestcase(yyruleno==33);
      /* (34) explain ::= EXPLAIN QUERY PLAN */ yytestcase(yyruleno==34);
      /* (35) cmd ::= BEGIN transtype trans_opt */ yytestcase(yyruleno==35);
      /* (36) transtype ::= */ yytestcase(yyruleno==36);
      /* (37) transtype ::= DEFERRED */ yytestcase(yyruleno==37);
      /* (38) transtype ::= IMMEDIATE */ yytestcase(yyruleno==38);
      /* (39) transtype ::= EXCLUSIVE */ yytestcase(yyruleno==39);
      /* (40) cmd ::= COMMIT|END trans_opt */ yytestcase(yyruleno==40);
      /* (41) cmd ::= ROLLBACK trans_opt */ yytestcase(yyruleno==41);
      /* (42) cmd ::= SAVEPOINT nm */ yytestcase(yyruleno==42);
      /* (43) cmd ::= RELEASE savepoint_opt nm */ yytestcase(yyruleno==43);
      /* (44) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */ yytestcase(yyruleno==44);
      /* (45) create_table ::= createkw temp TABLE ifnotexists nm dbnm */ yytestcase(yyruleno==45);
      /* (46) createkw ::= CREATE */ yytestcase(yyruleno==46);
      /* (47) ifnotexists ::= */ yytestcase(yyruleno==47);
      /* (48) ifnotexists ::= IF NOT EXISTS */ yytestcase(yyruleno==48);
      /* (49) temp ::= TEMP */ yytestcase(yyruleno==49);
      /* (50) temp ::= */ yytestcase(yyruleno==50);
      /* (51) create_table_args ::= LP columnlist conslist_opt RP table_option_set */ yytestcase(yyruleno==51);
      /* (52) create_table_args ::= AS select */ yytestcase(yyruleno==52);
      /* (53) table_option_set ::= */ yytestcase(yyruleno==53);
      /* (54) table_option_set ::= table_option_set COMMA table_option */ yytestcase(yyruleno==54);
      /* (55) table_option ::= WITHOUT nm */ yytestcase(yyruleno==55);
      /* (56) table_option ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=56);
      /* (57) columnname ::= nm typetoken */ yytestcase(yyruleno==57);
      /* (58) typetoken ::= */ yytestcase(yyruleno==58);
      /* (59) typetoken ::= typename LP signed RP */ yytestcase(yyruleno==59);
      /* (60) typetoken ::= typename LP signed COMMA signed RP */ yytestcase(yyruleno==60);
      /* (61) typename ::= typename ID|STRING */ yytestcase(yyruleno==61);
      /* (62) scantok ::= */ yytestcase(yyruleno==62);
      /* (63) ccons ::= CONSTRAINT nm */ yytestcase(yyruleno==63);
      /* (64) ccons ::= DEFAULT scantok term */ yytestcase(yyruleno==64);
      /* (65) ccons ::= DEFAULT LP expr RP */ yytestcase(yyruleno==65);
      /* (66) ccons ::= DEFAULT PLUS scantok term */ yytestcase(yyruleno==66);
      /* (67) ccons ::= DEFAULT MINUS scantok term */ yytestcase(yyruleno==67);
      /* (68) ccons ::= DEFAULT scantok ID|INDEXED */ yytestcase(yyruleno==68);
      /* (69) ccons ::= NOT NULL onconf */ yytestcase(yyruleno==69);
      /* (70) ccons ::= PRIMARY KEY sortorder onconf autoinc */ yytestcase(yyruleno==70);
      /* (71) ccons ::= UNIQUE onconf */ yytestcase(yyruleno==71);
      /* (72) ccons ::= CHECK LP expr RP */ yytestcase(yyruleno==72);
      /* (73) ccons ::= REFERENCES nm eidlist_opt refargs */ yytestcase(yyruleno==73);
      /* (74) ccons ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=74);
      /* (75) ccons ::= COLLATE ID|STRING */ yytestcase(yyruleno==75);
      /* (76) generated ::= LP expr RP */ yytestcase(yyruleno==76);
      /* (77) generated ::= LP expr RP ID */ yytestcase(yyruleno==77);
      /* (78) autoinc ::= */ yytestcase(yyruleno==78);
      /* (79) autoinc ::= AUTOINCR */ yytestcase(yyruleno==79);
      /* (80) refargs ::= */ yytestcase(yyruleno==80);
      /* (81) refargs ::= refargs refarg */ yytestcase(yyruleno==81);
      /* (82) refarg ::= MATCH nm */ yytestcase(yyruleno==82);
      /* (83) refarg ::= ON INSERT refact */ yytestcase(yyruleno==83);
      /* (84) refarg ::= ON DELETE refact */ yytestcase(yyruleno==84);
      /* (85) refarg ::= ON UPDATE refact */ yytestcase(yyruleno==85);
      /* (86) refact ::= SET NULL */ yytestcase(yyruleno==86);
      /* (87) refact ::= SET DEFAULT */ yytestcase(yyruleno==87);
      /* (88) refact ::= CASCADE */ yytestcase(yyruleno==88);
      /* (89) refact ::= RESTRICT */ yytestcase(yyruleno==89);
      /* (90) refact ::= NO ACTION */ yytestcase(yyruleno==90);
      /* (91) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */ yytestcase(yyruleno==91);
      /* (92) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */ yytestcase(yyruleno==92);
      /* (93) init_deferred_pred_opt ::= */ yytestcase(yyruleno==93);
      /* (94) init_deferred_pred_opt ::= INITIALLY DEFERRED */ yytestcase(yyruleno==94);
      /* (95) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */ yytestcase(yyruleno==95);
      /* (96) conslist_opt ::= */ yytestcase(yyruleno==96);
      /* (97) tconscomma ::= COMMA */ yytestcase(yyruleno==97);
      /* (98) tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==98);
      /* (99) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */ yytestcase(yyruleno==99);
      /* (100) tcons ::= UNIQUE LP sortlist RP onconf */ yytestcase(yyruleno==100);
      /* (101) tcons ::= CHECK LP expr RP onconf */ yytestcase(yyruleno==101);
      /* (102) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */ yytestcase(yyruleno==102);
      /* (103) defer_subclause_opt ::= */ yytestcase(yyruleno==103);
      /* (104) onconf ::= */ yytestcase(yyruleno==104);
      /* (105) onconf ::= ON CONFLICT resolvetype */ yytestcase(yyruleno==105);
      /* (106) orconf ::= */ yytestcase(yyruleno==106);
      /* (107) orconf ::= OR resolvetype */ yytestcase(yyruleno==107);
      /* (108) resolvetype ::= IGNORE */ yytestcase(yyruleno==108);
      /* (109) resolvetype ::= REPLACE */ yytestcase(yyruleno==109);
      /* (110) cmd ::= DROP TABLE ifexists fullname */ yytestcase(yyruleno==110);
      /* (111) ifexists ::= IF EXISTS */ yytestcase(yyruleno==111);
      /* (112) ifexists ::= */ yytestcase(yyruleno==112);
      /* (113) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */ yytestcase(yyruleno==113);
      /* (114) cmd ::= DROP VIEW ifexists fullname */ yytestcase(yyruleno==114);
      /* (115) select ::= WITH wqlist selectnowith */ yytestcase(yyruleno==115);
      /* (116) select ::= WITH RECURSIVE wqlist selectnowith */ yytestcase(yyruleno==116);
      /* (117) selectnowith ::= selectnowith multiselect_op oneselect */ yytestcase(yyruleno==117);
      /* (118) multiselect_op ::= UNION */ yytestcase(yyruleno==118);
      /* (119) multiselect_op ::= UNION ALL */ yytestcase(yyruleno==119);
      /* (120) multiselect_op ::= EXCEPT|INTERSECT */ yytestcase(yyruleno==120);
      /* (121) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */ yytestcase(yyruleno==121);
      /* (122) values ::= VALUES LP nexprlist RP */ yytestcase(yyruleno==122);
      /* (123) oneselect ::= mvalues */ yytestcase(yyruleno==123);
      /* (124) mvalues ::= values COMMA LP nexprlist RP */ yytestcase(yyruleno==124);
      /* (125) mvalues ::= mvalues COMMA LP nexprlist RP */ yytestcase(yyruleno==125);
      /* (126) selcollist ::= sclp scanpt nm DOT STAR */ yytestcase(yyruleno==126);
      /* (127) stl_prefix ::= seltablist joinop */ yytestcase(yyruleno==127);
      /* (128) stl_prefix ::= */ yytestcase(yyruleno==128);
      /* (129) seltablist ::= stl_prefix nm dbnm as on_using */ yytestcase(yyruleno==129);
      /* (130) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */ yytestcase(yyruleno==130);
      /* (131) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */ yytestcase(yyruleno==131);
      /* (132) seltablist ::= stl_prefix LP select RP as on_using */ yytestcase(yyruleno==132);
      /* (133) seltablist ::= stl_prefix LP seltablist RP as on_using */ yytestcase(yyruleno==133);
      /* (134) dbnm ::= */ yytestcase(yyruleno==134);
      /* (135) dbnm ::= DOT nm */ yytestcase(yyruleno==135);
      /* (136) fullname ::= nm */ yytestcase(yyruleno==136);
      /* (137) fullname ::= nm DOT nm */ yytestcase(yyruleno==137);
      /* (138) xfullname ::= nm */ yytestcase(yyruleno==138);
      /* (139) xfullname ::= nm DOT nm */ yytestcase(yyruleno==139);
      /* (140) xfullname ::= nm DOT nm AS nm */ yytestcase(yyruleno==140);
      /* (141) xfullname ::= nm AS nm */ yytestcase(yyruleno==141);
      /* (142) joinop ::= COMMA|JOIN */ yytestcase(yyruleno==142);
      /* (143) joinop ::= JOIN_KW JOIN */ yytestcase(yyruleno==143);
      /* (144) joinop ::= JOIN_KW nm JOIN */ yytestcase(yyruleno==144);
      /* (145) joinop ::= JOIN_KW nm nm JOIN */ yytestcase(yyruleno==145);
      /* (146) on_using ::= ON expr */ yytestcase(yyruleno==146);
      /* (147) on_using ::= USING LP idlist RP */ yytestcase(yyruleno==147);
      /* (148) on_using ::= */ yytestcase(yyruleno==148);
      /* (149) indexed_opt ::= */ yytestcase(yyruleno==149);
      /* (150) indexed_by ::= INDEXED BY nm */ yytestcase(yyruleno==150);
      /* (151) indexed_by ::= NOT INDEXED */ yytestcase(yyruleno==151);
      /* (152) orderby_opt ::= ORDER BY sortlist */ yytestcase(yyruleno==152);
      /* (153) sortlist ::= sortlist COMMA expr sortorder nulls */ yytestcase(yyruleno==153);
      /* (154) sortlist ::= expr sortorder nulls */ yytestcase(yyruleno==154);
      /* (155) sortorder ::= ASC */ yytestcase(yyruleno==155);
      /* (156) sortorder ::= DESC */ yytestcase(yyruleno==156);
      /* (157) sortorder ::= */ yytestcase(yyruleno==157);
      /* (158) nulls ::= NULLS FIRST */ yytestcase(yyruleno==158);
      /* (159) nulls ::= NULLS LAST */ yytestcase(yyruleno==159);
      /* (160) nulls ::= */ yytestcase(yyruleno==160);
      /* (161) groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==161);
      /* (162) having_opt ::= HAVING expr */ yytestcase(yyruleno==162);
      /* (163) limit_opt ::= LIMIT expr */ yytestcase(yyruleno==163);
      /* (164) limit_opt ::= LIMIT expr OFFSET expr */ yytestcase(yyruleno==164);
      /* (165) limit_opt ::= LIMIT expr COMMA expr */ yytestcase(yyruleno==165);
      /* (166) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */ yytestcase(yyruleno==166);
      /* (167) where_opt_ret ::= */ yytestcase(yyruleno==167);
      /* (168) where_opt_ret ::= WHERE expr */ yytestcase(yyruleno==168);
      /* (169) where_opt_ret ::= RETURNING selcollist */ yytestcase(yyruleno==169);
      /* (170) where_opt_ret ::= WHERE expr RETURNING selcollist */ yytestcase(yyruleno==170);
      /* (171) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */ yytestcase(yyruleno==171);
      /* (172) setlist ::= setlist COMMA nm EQ expr */ yytestcase(yyruleno==172);
      /* (173) setlist ::= setlist COMMA LP idlist RP EQ expr */ yytestcase(yyruleno==173);
      /* (174) setlist ::= nm EQ expr */ yytestcase(yyruleno==174);
      /* (175) setlist ::= LP idlist RP EQ expr */ yytestcase(yyruleno==175);
      /* (176) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */ yytestcase(yyruleno==176);
      /* (177) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */ yytestcase(yyruleno==177);
      /* (178) upsert ::= */ yytestcase(yyruleno==178);
      /* (179) upsert ::= RETURNING selcollist */ yytestcase(yyruleno==179);
      /* (180) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */ yytestcase(yyruleno==180);
      /* (181) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */ yytestcase(yyruleno==181);
      /* (182) upsert ::= ON CONFLICT DO NOTHING returning */ yytestcase(yyruleno==182);
      /* (183) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */ yytestcase(yyruleno==183);
      /* (184) returning ::= RETURNING selcollist */ yytestcase(yyruleno==184);
      /* (185) insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==185);
      /* (186) insert_cmd ::= REPLACE */ yytestcase(yyruleno==186);
      /* (187) idlist_opt ::= */ yytestcase(yyruleno==187);
      /* (188) idlist_opt ::= LP idlist RP */ yytestcase(yyruleno==188);
      /* (189) idlist ::= idlist COMMA nm */ yytestcase(yyruleno==189);
      /* (190) idlist ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=190);
      /* (191) expr ::= ID|INDEXED|JOIN_KW */ yytestcase(yyruleno==191);
      /* (192) expr ::= nm DOT nm */ yytestcase(yyruleno==192);
      /* (193) expr ::= nm DOT nm DOT nm */ yytestcase(yyruleno==193);
      /* (194) term ::= NULL|FLOAT|BLOB */ yytestcase(yyruleno==194);
      /* (195) expr ::= VARIABLE */ yytestcase(yyruleno==195);
      /* (196) expr ::= expr COLLATE ID|STRING */ yytestcase(yyruleno==196);
      /* (197) expr ::= CAST LP expr AS typetoken RP */ yytestcase(yyruleno==197);
      /* (198) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */ yytestcase(yyruleno==198);
      /* (199) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */ yytestcase(yyruleno==199);
      /* (200) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */ yytestcase(yyruleno==200);
      /* (201) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */ yytestcase(yyruleno==201);
      /* (202) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */ yytestcase(yyruleno==202);
      /* (203) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */ yytestcase(yyruleno==203);
      /* (204) term ::= CTIME_KW */ yytestcase(yyruleno==204);
      /* (205) expr ::= LP nexprlist COMMA expr RP */ yytestcase(yyruleno==205);
      /* (206) expr ::= expr AND expr */ yytestcase(yyruleno==206);
      /* (207) expr ::= expr OR expr */ yytestcase(yyruleno==207);
      /* (208) expr ::= expr LT|GT|GE|LE expr */ yytestcase(yyruleno==208);
      /* (209) expr ::= expr EQ|NE expr */ yytestcase(yyruleno==209);
      /* (210) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */ yytestcase(yyruleno==210);
      /* (211) expr ::= expr PLUS|MINUS expr */ yytestcase(yyruleno==211);
      /* (212) expr ::= expr STAR|SLASH|REM expr */ yytestcase(yyruleno==212);
      /* (213) expr ::= expr CONCAT expr */ yytestcase(yyruleno==213);
      /* (214) likeop ::= NOT LIKE_KW|MATCH */ yytestcase(yyruleno==214);
      /* (215) expr ::= expr likeop expr */ yytestcase(yyruleno==215);
      /* (216) expr ::= expr likeop expr ESCAPE expr */ yytestcase(yyruleno==216);
      /* (217) expr ::= expr ISNULL|NOTNULL */ yytestcase(yyruleno==217);
      /* (218) expr ::= expr NOT NULL */ yytestcase(yyruleno==218);
      /* (219) expr ::= expr IS expr */ yytestcase(yyruleno==219);
      /* (220) expr ::= expr IS NOT expr */ yytestcase(yyruleno==220);
      /* (221) expr ::= expr IS NOT DISTINCT FROM expr */ yytestcase(yyruleno==221);
      /* (222) expr ::= expr IS DISTINCT FROM expr */ yytestcase(yyruleno==222);
      /* (223) expr ::= NOT expr */ yytestcase(yyruleno==223);
      /* (224) expr ::= BITNOT expr */ yytestcase(yyruleno==224);
      /* (225) expr ::= PLUS|MINUS expr */ yytestcase(yyruleno==225);
      /* (226) expr ::= expr PTR expr */ yytestcase(yyruleno==226);
      /* (227) between_op ::= BETWEEN */ yytestcase(yyruleno==227);
      /* (228) between_op ::= NOT BETWEEN */ yytestcase(yyruleno==228);
      /* (229) expr ::= expr between_op expr AND expr */ yytestcase(yyruleno==229);
      /* (230) in_op ::= IN */ yytestcase(yyruleno==230);
      /* (231) in_op ::= NOT IN */ yytestcase(yyruleno==231);
      /* (232) expr ::= expr in_op LP exprlist RP */ yytestcase(yyruleno==232);
      /* (233) expr ::= LP select RP */ yytestcase(yyruleno==233);
      /* (234) expr ::= expr in_op LP select RP */ yytestcase(yyruleno==234);
      /* (235) expr ::= expr in_op nm dbnm paren_exprlist */ yytestcase(yyruleno==235);
      /* (236) expr ::= EXISTS LP select RP */ yytestcase(yyruleno==236);
      /* (237) expr ::= CASE case_operand case_exprlist case_else END */ yytestcase(yyruleno==237);
      /* (238) case_exprlist ::= case_exprlist WHEN expr THEN expr */ yytestcase(yyruleno==238);
      /* (239) case_exprlist ::= WHEN expr THEN expr */ yytestcase(yyruleno==239);
      /* (240) case_else ::= ELSE expr */ yytestcase(yyruleno==240);
      /* (241) case_else ::= */ yytestcase(yyruleno==241);
      /* (242) case_operand ::= */ yytestcase(yyruleno==242);
      /* (243) exprlist ::= */ yytestcase(yyruleno==243);
      /* (244) nexprlist ::= nexprlist COMMA expr */ yytestcase(yyruleno==244);
      /* (245) nexprlist ::= expr */ yytestcase(yyruleno==245);
      /* (246) paren_exprlist ::= */ yytestcase(yyruleno==246);
      /* (247) paren_exprlist ::= LP exprlist RP */ yytestcase(yyruleno==247);
      /* (248) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */ yytestcase(yyruleno==248);
      /* (249) uniqueflag ::= UNIQUE */ yytestcase(yyruleno==249);
      /* (250) uniqueflag ::= */ yytestcase(yyruleno==250);
      /* (251) eidlist_opt ::= */ yytestcase(yyruleno==251);
      /* (252) eidlist_opt ::= LP eidlist RP */ yytestcase(yyruleno==252);
      /* (253) eidlist ::= eidlist COMMA nm collate sortorder */ yytestcase(yyruleno==253);
      /* (254) eidlist ::= nm collate sortorder */ yytestcase(yyruleno==254);
      /* (255) collate ::= */ yytestcase(yyruleno==255);
      /* (256) collate ::= COLLATE ID|STRING */ yytestcase(yyruleno==256);
      /* (257) cmd ::= DROP INDEX ifexists fullname */ yytestcase(yyruleno==257);
      /* (258) cmd ::= VACUUM vinto */ yytestcase(yyruleno==258);
      /* (259) cmd ::= VACUUM nm vinto */ yytestcase(yyruleno==259);
      /* (260) vinto ::= INTO expr */ yytestcase(yyruleno==260);
      /* (261) vinto ::= */ yytestcase(yyruleno==261);
      /* (262) cmd ::= PRAGMA nm dbnm */ yytestcase(yyruleno==262);
      /* (263) cmd ::= PRAGMA nm dbnm EQ nmnum */ yytestcase(yyruleno==263);
      /* (264) cmd ::= PRAGMA nm dbnm LP nmnum RP */ yytestcase(yyruleno==264);
      /* (265) cmd ::= PRAGMA nm dbnm EQ minus_num */ yytestcase(yyruleno==265);
      /* (266) cmd ::= PRAGMA nm dbnm LP minus_num RP */ yytestcase(yyruleno==266);
      /* (267) plus_num ::= PLUS INTEGER|FLOAT */ yytestcase(yyruleno==267);
      /* (268) minus_num ::= MINUS INTEGER|FLOAT */ yytestcase(yyruleno==268);
      /* (269) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */ yytestcase(yyruleno==269);
      /* (270) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */ yytestcase(yyruleno==270);
      /* (271) trigger_time ::= BEFORE|AFTER */ yytestcase(yyruleno==271);
      /* (272) trigger_time ::= INSTEAD OF */ yytestcase(yyruleno==272);
      /* (273) trigger_time ::= */ yytestcase(yyruleno==273);
      /* (274) trigger_event ::= DELETE|INSERT */ yytestcase(yyruleno==274);
      /* (275) trigger_event ::= UPDATE */ yytestcase(yyruleno==275);
      /* (276) trigger_event ::= UPDATE OF idlist */ yytestcase(yyruleno==276);
      /* (277) when_clause ::= */ yytestcase(yyruleno==277);
      /* (278) when_clause ::= WHEN expr */ yytestcase(yyruleno==278);
      /* (279) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */ yytestcase(yyruleno==279);
      /* (280) trigger_cmd_list ::= trigger_cmd SEMI */ yytestcase(yyruleno==280);
      /* (281) trnm ::= nm DOT nm */ yytestcase(yyruleno==281);
      /* (282) tridxby ::= INDEXED BY nm */ yytestcase(yyruleno==282);
      /* (283) tridxby ::= NOT INDEXED */ yytestcase(yyruleno==283);
      /* (284) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */ yytestcase(yyruleno==284);
      /* (285) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */ yytestcase(yyruleno==285);
      /* (286) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */ yytestcase(yyruleno==286);
      /* (287) trigger_cmd ::= scanpt select scanpt */ yytestcase(yyruleno==287);
      /* (288) expr ::= RAISE LP IGNORE RP */ yytestcase(yyruleno==288);
      /* (289) expr ::= RAISE LP raisetype COMMA expr RP */ yytestcase(yyruleno==289);
      /* (290) raisetype ::= ROLLBACK */ yytestcase(yyruleno==290);
      /* (291) raisetype ::= ABORT */ yytestcase(yyruleno==291);
      /* (292) raisetype ::= FAIL */ yytestcase(yyruleno==292);
      /* (293) cmd ::= DROP TRIGGER ifexists fullname */ yytestcase(yyruleno==293);
      /* (294) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */ yytestcase(yyruleno==294);
      /* (295) cmd ::= DETACH database_kw_opt expr */ yytestcase(yyruleno==295);
      /* (296) key_opt ::= */ yytestcase(yyruleno==296);
      /* (297) key_opt ::= KEY expr */ yytestcase(yyruleno==297);
      /* (298) cmd ::= REINDEX */ yytestcase(yyruleno==298);
      /* (299) cmd ::= REINDEX nm dbnm */ yytestcase(yyruleno==299);
      /* (300) cmd ::= ANALYZE */ yytestcase(yyruleno==300);
      /* (301) cmd ::= ANALYZE nm dbnm */ yytestcase(yyruleno==301);
      /* (302) cmd ::= ALTER TABLE fullname RENAME TO nm */ yytestcase(yyruleno==302);
      /* (303) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */ yytestcase(yyruleno==303);
      /* (304) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */ yytestcase(yyruleno==304);
      /* (305) add_column_fullname ::= fullname */ yytestcase(yyruleno==305);
      /* (306) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */ yytestcase(yyruleno==306);
      /* (307) cmd ::= create_vtab */ yytestcase(yyruleno==307);
      /* (308) cmd ::= create_vtab LP vtabarglist RP */ yytestcase(yyruleno==308);
      /* (309) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */ yytestcase(yyruleno==309);
      /* (310) vtabarg ::= */ yytestcase(yyruleno==310);
      /* (311) vtabargtoken ::= ANY */ yytestcase(yyruleno==311);
      /* (312) vtabargtoken ::= lp anylist RP */ yytestcase(yyruleno==312);
      /* (313) lp ::= LP */ yytestcase(yyruleno==313);
      /* (314) with ::= WITH wqlist */ yytestcase(yyruleno==314);
      /* (315) with ::= WITH RECURSIVE wqlist */ yytestcase(yyruleno==315);
      /* (316) wqas ::= AS */ yytestcase(yyruleno==316);
      /* (317) wqas ::= AS MATERIALIZED */ yytestcase(yyruleno==317);
      /* (318) wqas ::= AS NOT MATERIALIZED */ yytestcase(yyruleno==318);
      /* (319) wqitem ::= withnm eidlist_opt wqas LP select RP */ yytestcase(yyruleno==319);
      /* (320) withnm ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=320);
      /* (321) wqlist ::= wqitem (OPTIMIZED OUT) */ assert(yyruleno!=321);
      /* (322) wqlist ::= wqlist COMMA wqitem */ yytestcase(yyruleno==322);
      /* (323) windowdefn_list ::= windowdefn_list COMMA windowdefn */ yytestcase(yyruleno==323);
      /* (324) windowdefn ::= nm AS LP window RP */ yytestcase(yyruleno==324);
      /* (325) window ::= PARTITION BY nexprlist orderby_opt frame_opt */ yytestcase(yyruleno==325);
      /* (326) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */ yytestcase(yyruleno==326);
      /* (327) window ::= ORDER BY sortlist frame_opt */ yytestcase(yyruleno==327);
      /* (328) window ::= nm ORDER BY sortlist frame_opt */ yytestcase(yyruleno==328);
      /* (329) window ::= nm frame_opt */ yytestcase(yyruleno==329);
      /* (330) frame_opt ::= */ yytestcase(yyruleno==330);
      /* (331) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */ yytestcase(yyruleno==331);
      /* (332) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */ yytestcase(yyruleno==332);
      /* (333) range_or_rows ::= RANGE|ROWS|GROUPS */ yytestcase(yyruleno==333);
      /* (334) frame_bound_s ::= frame_bound (OPTIMIZED OUT) */ assert(yyruleno!=334);
      /* (335) frame_bound_s ::= UNBOUNDED PRECEDING */ yytestcase(yyruleno==335);
      /* (336) frame_bound_e ::= frame_bound (OPTIMIZED OUT) */ assert(yyruleno!=336);
      /* (337) frame_bound_e ::= UNBOUNDED FOLLOWING */ yytestcase(yyruleno==337);
      /* (338) frame_bound ::= expr PRECEDING|FOLLOWING */ yytestcase(yyruleno==338);
      /* (339) frame_bound ::= CURRENT ROW */ yytestcase(yyruleno==339);
      /* (340) frame_exclude_opt ::= */ yytestcase(yyruleno==340);
      /* (341) frame_exclude_opt ::= EXCLUDE frame_exclude */ yytestcase(yyruleno==341);
      /* (342) frame_exclude ::= NO OTHERS */ yytestcase(yyruleno==342);
      /* (343) frame_exclude ::= CURRENT ROW */ yytestcase(yyruleno==343);
      /* (344) frame_exclude ::= GROUP|TIES */ yytestcase(yyruleno==344);
      /* (345) window_clause ::= WINDOW windowdefn_list */ yytestcase(yyruleno==345);
      /* (346) filter_over ::= filter_clause over_clause */ yytestcase(yyruleno==346);
      /* (347) filter_over ::= over_clause (OPTIMIZED OUT) */ assert(yyruleno!=347);
      /* (348) filter_over ::= filter_clause */ yytestcase(yyruleno==348);
      /* (349) over_clause ::= OVER LP window RP */ yytestcase(yyruleno==349);
      /* (350) over_clause ::= OVER nm */ yytestcase(yyruleno==350);
      /* (351) filter_clause ::= FILTER LP WHERE expr RP */ yytestcase(yyruleno==351);
      /* (352) term ::= QNUMBER */ yytestcase(yyruleno==352);
      /* (353) ecmd ::= explain cmdx SEMI */ yytestcase(yyruleno==353);
      /* (354) trans_opt ::= */ yytestcase(yyruleno==354);
      /* (355) trans_opt ::= TRANSACTION */ yytestcase(yyruleno==355);
      /* (356) trans_opt ::= TRANSACTION nm */ yytestcase(yyruleno==356);
      /* (357) savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==357);
      /* (358) savepoint_opt ::= */ yytestcase(yyruleno==358);
      /* (359) cmd ::= create_table create_table_args */ yytestcase(yyruleno==359);
      /* (360) table_option_set ::= table_option (OPTIMIZED OUT) */ assert(yyruleno!=360);
      /* (361) columnlist ::= columnlist COMMA columnname carglist */ yytestcase(yyruleno==361);
      /* (362) columnlist ::= columnname carglist */ yytestcase(yyruleno==362);
      /* (363) nm ::= ID|INDEXED|JOIN_KW */ yytestcase(yyruleno==363);
      /* (364) typetoken ::= typename */ yytestcase(yyruleno==364);
      /* (365) typename ::= ID|STRING */ yytestcase(yyruleno==365);
      /* (366) signed ::= plus_num (OPTIMIZED OUT) */ assert(yyruleno!=366);
      /* (367) signed ::= minus_num (OPTIMIZED OUT) */ assert(yyruleno!=367);
      /* (368) carglist ::= carglist ccons */ yytestcase(yyruleno==368);
      /* (369) carglist ::= */ yytestcase(yyruleno==369);
      /* (370) ccons ::= NULL onconf */ yytestcase(yyruleno==370);
      /* (371) ccons ::= GENERATED ALWAYS AS generated */ yytestcase(yyruleno==371);
      /* (372) ccons ::= AS generated */ yytestcase(yyruleno==372);
      /* (373) conslist_opt ::= COMMA conslist */ yytestcase(yyruleno==373);
      /* (374) conslist ::= conslist tconscomma tcons */ yytestcase(yyruleno==374);
      /* (375) conslist ::= tcons (OPTIMIZED OUT) */ assert(yyruleno!=375);
      /* (376) tconscomma ::= */ yytestcase(yyruleno==376);
      /* (377) defer_subclause_opt ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=377);
      /* (378) resolvetype ::= raisetype (OPTIMIZED OUT) */ assert(yyruleno!=378);
      /* (379) oneselect ::= values */ yytestcase(yyruleno==379);
      /* (380) as ::= ID|STRING */ yytestcase(yyruleno==380);
      /* (381) indexed_opt ::= indexed_by (OPTIMIZED OUT) */ assert(yyruleno!=381);
      /* (382) returning ::= */ yytestcase(yyruleno==382);
      /* (383) likeop ::= LIKE_KW|MATCH */ yytestcase(yyruleno==383);
      /* (384) case_operand ::= expr */ yytestcase(yyruleno==384);
      /* (385) exprlist ::= nexprlist */ yytestcase(yyruleno==385);
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
