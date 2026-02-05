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
#define YYNRULE_WITH_ACTION  37
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
 /*     0 */  1586,    7,  574,  462,  522,  378, 1325,  130,  127,  234,
 /*    10 */   505, 1363,  574, 1342, 1378,  574,  372,  130,  127,  234,
 /*    20 */  1363,  462,  568,  462,  565,  574,  571,  464, 1257, 1293,
 /*    30 */    51, 1551,  295,  161,  290,  284, 1291,  413,  434, 1293,
 /*    40 */    51, 1259, 1293,   19,  534, 1328,  565,  299,  571, 1004,
 /*    50 */   264,  230, 1293,   60,  574,  499,  421, 1005,  130,  127,
 /*    60 */   234,  137,  139,   91,   48, 1228, 1228, 1072, 1075, 1062,
 /*    70 */  1062,  290, 1327, 1291,  290,  413, 1291,  290,  233, 1291,
 /*    80 */   539, 1293,   51,  565,  574,  571,  565,  535,  571,  565,
 /*    90 */  1035,  571,  568,  568,  516,  413,  499,  254,   44,  137,
 /*   100 */   139,   91,  267, 1228, 1228, 1072, 1075, 1062, 1062,  370,
 /*   110 */   434, 1293,   84,  303,  452,  130,  127,  234, 1631,  137,
 /*   120 */   139,   91,  574, 1228, 1228, 1072, 1075, 1062, 1062,  376,
 /*   130 */   376,  359,  301,  466,   48,  372, 1183,  377, 1183,  140,
 /*   140 */   377, 1550,  321,  575, 1313,  308,  556,  490,  361, 1293,
 /*   150 */    84,  420,  452,  521,  523,  135,  135,  136,  136,  136,
 /*   160 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   170 */   131,  128,  452,  304,  290,  290, 1291, 1291,  452,  547,
 /*   180 */  1293,   22,  547,  574,  556,  501,  565,  565,  571,  571,
 /*   190 */   452, 1156,  555,  135,  135,  136,  136,  136,  136,  134,
 /*   200 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   210 */  1293,   82,  452,  135,  135,  136,  136,  136,  136,  134,
 /*   220 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   230 */  1156,  413, 1225,   10,  110, 1366, 1225,  134,  134,  134,
 /*   240 */   134,  133,  133,  132,  132,  132,  131,  128,  452,  321,
 /*   250 */   575,  413, 1158, 1657, 1657,  137,  139,   91,  574, 1228,
 /*   260 */  1228, 1072, 1075, 1062, 1062,  132,  132,  132,  131,  128,
 /*   270 */   383,  499, 1135,   97,  212,  137,  139,   91,  379, 1228,
 /*   280 */  1228, 1072, 1075, 1062, 1062, 1293,  145, 1315,  157,  413,
 /*   290 */   326, 1158, 1658, 1658, 1136,  398, 1366,   45,  505,  133,
 /*   300 */   133,  132,  132,  132,  131,  128,    6, 1315,  452, 1137,
 /*   310 */  1178,  413, 1210,  137,  139,   91, 1210, 1228, 1228, 1072,
 /*   320 */  1075, 1062, 1062, 1178,  957, 1197, 1178,  573,  452, 1208,
 /*   330 */  1208,  130,  127,  234,  958,  137,  139,   91, 1368, 1228,
 /*   340 */  1228, 1072, 1075, 1062, 1062, 1208, 1208,  212,  307,  135,
 /*   350 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   360 */   133,  132,  132,  132,  131,  128,  452,  505,  399,  135,
 /*   370 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   380 */   133,  132,  132,  132,  131,  128, 1208,  293,  452,  323,
 /*   390 */   878, 1059, 1059, 1073, 1076, 1052,  975,  478,  183, 1368,
 /*   400 */   209,  358, 1208,  867, 1520,  489,  878,  135,  135,  136,
 /*   410 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*   420 */   132,  132,  131,  128,   44,  373,  413,  467,  435,  135,
 /*   430 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   440 */   133,  132,  132,  132,  131,  128, 1063,  413,  452,  921,
 /*   450 */   137,  139,   91,   98, 1228, 1228, 1072, 1075, 1062, 1062,
 /*   460 */   868,  481,  867,  867,  869,  481,  499,  574,  321,  575,
 /*   470 */   310,  137,  139,   91,  574, 1228, 1228, 1072, 1075, 1062,
 /*   480 */  1062, 1625,  293,  236,  452,  413,  461,  458,  457, 1625,
 /*   490 */   478,  477,  478,   46, 1293,   19,  456, 1520, 1522, 1520,
 /*   500 */   435, 1293,  147,  452,  131,  128,  321,  575,  294,  137,
 /*   510 */   139,   91,  349, 1228, 1228, 1072, 1075, 1062, 1062,  429,
 /*   520 */   530,  530,    7,  448,  452, 1208, 1208,  136,  136,  136,
 /*   530 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   540 */   131,  128,  351,  309,  135,  135,  136,  136,  136,  136,
 /*   550 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   560 */   128,  524,  452,  237,  914,  135,  135,  136,  136,  136,
 /*   570 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   580 */   131,  128, 1208, 1463, 1507,  183,  878, 1233, 1208, 1208,
 /*   590 */   346, 1235,  516,  448,  448,  527,  219,  413,  419, 1234,
 /*   600 */   348,  231,  350,  135,  135,  136,  136,  136,  136,  134,
 /*   610 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   620 */  1607,  137,  139,   91,  898, 1228, 1228, 1072, 1075, 1062,
 /*   630 */  1062,  380, 1463,  549,  413, 1208, 1208,  447,  446,  427,
 /*   640 */   525, 1208, 1208,  236,  574, 1208,  461,  458,  457,  878,
 /*   650 */   129, 1236,  548, 1236,  232,  413,  456, 1178,  137,  139,
 /*   660 */    91, 1189, 1228, 1228, 1072, 1075, 1062, 1062,  543,  212,
 /*   670 */  1178, 1293,   84, 1178,  452,  488,  510,  344,  991,  137,
 /*   680 */   139,   91,  507, 1228, 1228, 1072, 1075, 1062, 1062,  283,
 /*   690 */   407, 1291, 1208,  452, 1659,  402,  878,  382, 1208, 1189,
 /*   700 */   474,  565,  878,  571,  516, 1094,  556,  540,  540,    7,
 /*   710 */   322,  452,  542,  991,  444,  135,  135,  136,  136,  136,
 /*   720 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   730 */   131,  128,  452, 1390,  316, 1053,  136,  136,  136,  136,
 /*   740 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   750 */   128, 1514,  135,  135,  136,  136,  136,  136,  134,  134,
 /*   760 */   134,  134,  133,  133,  132,  132,  132,  131,  128, 1515,
 /*   770 */   413,  251,  866,  135,  135,  136,  136,  136,  136,  134,
 /*   780 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   790 */   413, 1359,  516, 1196,  137,  139,   91,  574, 1228, 1228,
 /*   800 */  1072, 1075, 1062, 1062,  409,   44, 1119, 1119,  513, 1208,
 /*   810 */  1208,  532, 1316,  574,  137,  139,   91,  107, 1228, 1228,
 /*   820 */  1072, 1075, 1062, 1062, 1293,   19,  370,  392,  413,  447,
 /*   830 */   446,  479,  557, 1462,  290, 1114, 1291,  516,  432, 1375,
 /*   840 */  1293,   84,  100, 1236,  526, 1236,  565,  452,  571,  321,
 /*   850 */   575,  302,  137,  139,   91,  302, 1228, 1228, 1072, 1075,
 /*   860 */  1062, 1062,  290,  497, 1291,  924, 1208,  452,  440,  924,
 /*   870 */   878,  436, 1208, 1208,  565,  556,  571, 1208, 1208, 1208,
 /*   880 */  1208,  380, 1462,  558, 1371,  368,  116,  390,  135,  135,
 /*   890 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   900 */   132,  132,  132,  131,  128,  452,  253, 1197,  135,  135,
 /*   910 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   920 */   132,  132,  132,  131,  128,   14, 1208, 1208, 1134, 1208,
 /*   930 */   198,  533,  533,  878, 1208,  199, 1208,    5,  878,  413,
 /*   940 */   878,  490,  361,  436,  120,  215,  135,  135,  136,  136,
 /*   950 */   136,  136,  134,  134,  134,  134,  133,  133,  132,  132,
 /*   960 */   132,  131,  128,  137,  139,   91,  413, 1228, 1228, 1072,
 /*   970 */  1075, 1062, 1062,  518,  574,  524,  516, 1052, 1116, 1346,
 /*   980 */   512,  917, 1116, 1208,   42,  867,  574,  878, 1584,    7,
 /*   990 */   137,  139,   91, 1178, 1228, 1228, 1072, 1075, 1062, 1062,
 /*  1000 */   413, 1293,   84,  290,  552, 1291, 1178,  559,  511, 1178,
 /*  1010 */  1345,  406,  405, 1293,   19,  565,  452,  571,  491,  532,
 /*  1020 */    95, 1208, 1208,  366,  137,  126,   91,  437, 1228, 1228,
 /*  1030 */  1072, 1075, 1062, 1062,  429,  267,  528,  290,  917, 1291,
 /*  1040 */   356,  372,  868,  452,  867,  867,  869,  574,  541,  565,
 /*  1050 */   851,  571,  512,  107,  525,  850,  155,  135,  135,  136,
 /*  1060 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*  1070 */   132,  132,  131,  128, 1293,   53,  574,  452, 1208,  493,
 /*  1080 */   526,  413,  878,  536,  135,  135,  136,  136,  136,  136,
 /*  1090 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*  1100 */   128,  305, 1461, 1293,   84,  213,  139,   91,  574, 1228,
 /*  1110 */  1228, 1072, 1075, 1062, 1062,  413, 1208, 1208,  135,  135,
 /*  1120 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*  1130 */   132,  132,  132,  131,  128, 1293,   19,  241,  317,  574,
 /*  1140 */  1580,   91,  372, 1228, 1228, 1072, 1075, 1062, 1062,  442,
 /*  1150 */   380, 1461,  429, 1374,  576,  441,  969,  969,  452, 1579,
 /*  1160 */   372, 1208, 1208, 1208, 1208, 1225, 1293,   19,  886, 1225,
 /*  1170 */   887, 1343,  888, 1208,    3, 1318,  418,  878,    6,  124,
 /*  1180 */   207,  566, 1344,  290, 1197, 1291,  993,  417,  992,  574,
 /*  1190 */    50,  331,  452, 1585,    7,  565,  569,  571, 1324,  135,
 /*  1200 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*  1210 */   133,  132,  132,  132,  131,  128, 1293,   19, 1208,  384,
 /*  1220 */  1208,  375,  878,  214,  878,  562, 1250,  441, 1208, 1208,
 /*  1230 */   208,  567,  418,  135,  135,  136,  136,  136,  136,  134,
 /*  1240 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*  1250 */   124, 1561,  566,  574, 1052, 1197,  574,  372,  450,  450,
 /*  1260 */    50,  372,  867,  108,  372,  910,  160,  569, 1563,  910,
 /*  1270 */   574, 1583,    7,  574,  124,  574,  566,  506,  158, 1197,
 /*  1280 */  1293,   19, 1004, 1293,   84, 1208,  475,  401, 1249,  878,
 /*  1290 */  1005,  569,  228,   34,  550, 1559,  562, 1293,   84, 1250,
 /*  1300 */  1293,   84, 1293,   84,    9,   47,   44,  122,  122,  855,
 /*  1310 */   288,  230,   49,  453,  123,  450,    4,  574,  318,  572,
 /*  1320 */   562,  867,  867,  869, 1043, 1052,  291,  247, 1291,  450,
 /*  1330 */   450,  851,  445,  867,  545,  449,  850,  451,  565,  544,
 /*  1340 */   571,  241,  232, 1178, 1293,   54, 1582,    7,  937, 1052,
 /*  1350 */   321,  575,  426,  450,  450,  336, 1178,  867,  327, 1178,
 /*  1360 */   401, 1157,  422,  185,   34,  335,  991,  124,  298,  566,
 /*  1370 */  1244,  574, 1197,  470,  928,   47,  372,  574,  122,  122,
 /*  1380 */   574,  938,   49,  372,  569,  123,  450,    4,   34,  338,
 /*  1390 */   572,  417,  867,  867,  869, 1043, 1630,  946, 1293,   55,
 /*  1400 */   286,  991,  122,  122, 1293,   56,  330, 1293,   66,  123,
 /*  1410 */   450,    4,  480,  562,  572,  551,  867,  867,  869, 1043,
 /*  1420 */  1035,  574,  469, 1388,  424,  113,  121,  545,  118,  423,
 /*  1430 */  1135,  113,  546,  243,  280,  329,  472,  332,  471,  242,
 /*  1440 */   482,  574, 1052,  503,  483,  330,  450,  450, 1293,   67,
 /*  1450 */   867,  412, 1136, 1101,  124,  412,  566, 1101,  574, 1197,
 /*  1460 */   339,  359,  485,  347,  574,  495,  492, 1137, 1293,   21,
 /*  1470 */   340,  569,  240,  239,  238,  343,  496,  412,  320,  454,
 /*  1480 */   263,   34,  561,  324,  113, 1293,   57,  412,  574,  412,
 /*  1490 */   574, 1293,   68,  574, 1402,  122,  122,  574,  221,  943,
 /*  1500 */   562,  944,  123,  450,    4,  101,  218,  572,  574,  867,
 /*  1510 */   867,  869, 1043,  574,  545, 1293,   58, 1293,   69,  544,
 /*  1520 */  1293,   70,  504,  574, 1293,   71,  574,  222,  574, 1052,
 /*  1530 */   574,  551,  574,  450,  450, 1293,   72,  867,  574,  222,
 /*  1540 */  1293,   73,  574,  430,  574,  205,  574, 1153,  403,   39,
 /*  1550 */  1293,   74,  574, 1293,   75, 1293,   76, 1293,   77, 1293,
 /*  1560 */    59,  936,  935,  228,  138, 1293,   61,   38,   34, 1293,
 /*  1570 */    20, 1293,  143, 1293,  144,  880,  537,  357,  113, 1293,
 /*  1580 */    79, 1401,  122,  122, 1285,  484,  476,  582,  574,  123,
 /*  1590 */   450,    4,  574,  292,  572,  574,  867,  867,  869, 1043,
 /*  1600 */  1113,  574, 1113,  396, 1112,  396, 1112,  395,  166,  277,
 /*  1610 */   520,  393, 1037,  266, 1566, 1293,   62,  895,  574, 1293,
 /*  1620 */    80,  574, 1293,   63, 1007, 1008,  574,  352, 1293,   81,
 /*  1630 */   498,  266,  248, 1539,  342,  124,  574,  566,  500,  266,
 /*  1640 */  1197, 1538,  341,  508,   25, 1293,   64,  363, 1293,  170,
 /*  1650 */  1598,  574,  569, 1293,  171,   90,  574,  566,  289,  574,
 /*  1660 */  1197, 1044, 1620, 1293,   88, 1044,  250,  362,  113,  574,
 /*  1670 */  1398,  560,  569,  224,  177,  517,  574,   43, 1293,   65,
 /*  1680 */   574,  562, 1162, 1293,  146,  574, 1293,   83,  367, 1097,
 /*  1690 */   263,  371,  574, 1411,  249, 1450, 1293,  168,  574, 1268,
 /*  1700 */   574,  562, 1454, 1293,  148,  574,  389, 1293,  142, 1396,
 /*  1710 */  1052,  391, 1293,  169,  450,  450,  414,  574,  867, 1293,
 /*  1720 */   162,  321,  575,  574, 1304, 1293,  152, 1293,  151, 1303,
 /*  1730 */  1052, 1305, 1293,  149,  450,  450, 1613,  554,  867,  996,
 /*  1740 */   266,  165,  113,  281, 1293,  150,  973,  138, 1266,   34,
 /*  1750 */  1293,   86,  908,  159,  574,   12,  124,  553,  566,  313,
 /*  1760 */   167, 1197,  397,  122,  122,  974,  138,  876,  314,   34,
 /*  1770 */   123,  450,    4,  569,  315,  572,  227,  867,  867,  869,
 /*  1780 */  1043, 1293,   78,  122,  122, 1616,  476,  582,  971,  138,
 /*  1790 */   123,  450,    4,  292,  459,  572,  246,  867,  867,  869,
 /*  1800 */  1043, 1342,  562,  396,  334,  396, 1437,  395, 1432,  277,
 /*  1810 */   574,  393,  354,  300, 1427, 1442,  574,  895,  502, 1441,
 /*  1820 */   574,  410,  355,  306, 1511, 1510,  203,  360,  223, 1393,
 /*  1830 */  1394, 1052,  248,  210,  342,  450,  450, 1293,   87,  867,
 /*  1840 */   211,  563,  341, 1293,   85,  387, 1392, 1293,   52, 1391,
 /*  1850 */   400,  473, 1362, 1361, 1360,   40,   93,  463, 1627,  425,
 /*  1860 */  1353, 1629, 1628,  404, 1332,  928,  250, 1331, 1352, 1244,
 /*  1870 */    34,  333, 1330,  274,  177, 1558, 1556,   43,  187, 1241,
 /*  1880 */   100,  428, 1516, 1438,  122,  122,  220,  196,  476,  244,
 /*  1890 */    96,  123,  450,    4,  249,  292,  572,  182,  867,  867,
 /*  1900 */   869, 1043,   99,  189,  486,  396,  191,  396,  487,  395,
 /*  1910 */   192,  277,   13,  393,  193,  579,  414,  194,  256,  895,
 /*  1920 */   108,  321,  575,   15, 1446,  408,    2,  465, 1257, 1444,
 /*  1930 */  1443,  494,  295,  161,  248,  411,  342,  200, 1505,  106,
 /*  1940 */   509, 1259,  260,  262,  341,  515, 1527,  365,  282,  438,
 /*  1950 */   311,  519,  439,  369,  312,    2,  465, 1257,  268,  141,
 /*  1960 */  1382,  295,  161,  551,  269,  374,  443,  876,  250, 1415,
 /*  1970 */  1259,  290, 1589, 1291,  233,   11,  177, 1588, 1414,   43,
 /*  1980 */   381, 1603, 1492,  565,  117,  571,  319, 1261,  109,  216,
 /*  1990 */   529,  385,  386,  577,  388,  273,  249,  254, 1202, 1306,
 /*  2000 */   290,  276, 1291,  275,  278,  279,  580, 1301,  154, 1296,
 /*  2010 */   186,  296,  565,  172,  571,  415,  225, 1543,  414,  173,
 /*  2020 */   174,  226,  335,  321,  575, 1544,  254,  416,  235, 1542,
 /*  2030 */  1541,   89,  184,  466,  455,  325,   92,   23,   24,  460,
 /*  2040 */  1198,  175,  153,  328,  217,  468,  926,  939,   94,  877,
 /*  2050 */   297,  176,  337,  245, 1255,    1, 1287,  156, 1111, 1109,
 /*  2060 */   345,  188,  466,  178,  880,  252,  190,  960,  353,  255,
 /*  2070 */  1125,  195,  179,  180,  433,  431,  102,  197,  181,  103,
 /*  2080 */   104,  105,  257, 1128, 1124, 1286,  258,  163,   26,  259,
 /*  2090 */   364, 1117,  266,  201, 1238,  514,  261,  202, 1163,  229,
 /*  2100 */  1000,  265,  994,   27, 1164,  285,   16,  531,  287,  204,
 /*  2110 */    28,  112,  138, 1169,   41,  206, 1078,   29,  111,   30,
 /*  2120 */   164,  538,   31, 1194,    8, 1180, 1184, 1182, 1188,  113,
 /*  2130 */   114,   32, 1187,   33, 1092,  115, 1082, 1079, 1077,  968,
 /*  2140 */  1081,   18,  897,  270,  119, 1133,   35,  578,  394,  883,
 /*  2150 */    17,  564, 1045,  909,  125,   36,  581, 1290, 1256,  570,
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
 /*   150 */    76,  134,   98,  519, 1276, 1112, 1112, 1112, 1112,  664,
 /*   160 */   664,  583,  334,   20,   70,   70,   70,  369,   64,   64,
 /*   170 */  2165, 2165, 1315, 1315, 1315,  519,  249,  693,  756,  756,
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
 /*   150 */  -253, -253, -253, -184, -210,   87,  405,  854,  624, -189,
 /*   160 */   312,  399, -253,  116,  314,  564,  833, -224, -253, -253,
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
 /*     0 */  1663, 1663, 1663, 1500, 1254, 1389, 1254, 1254, 1254, 1254,
 /*    10 */  1500, 1500, 1500, 1254, 1254, 1254, 1254, 1254, 1254, 1418,
 /*    20 */  1418, 1553, 1258, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*    30 */  1254, 1254, 1254, 1254, 1499, 1254, 1254, 1254, 1254, 1254,
 /*    40 */  1254, 1587, 1587, 1254, 1254, 1254, 1254, 1572, 1571, 1254,
 /*    50 */  1254, 1254, 1424, 1254, 1254, 1254, 1254, 1254, 1429, 1501,
 /*    60 */  1502, 1254, 1254, 1254, 1254, 1254, 1552, 1554, 1517, 1436,
 /*    70 */  1435, 1434, 1433, 1535, 1407, 1277, 1423, 1426, 1501, 1496,
 /*    80 */  1497, 1495, 1641, 1254, 1502, 1425, 1467, 1466, 1480, 1254,
 /*    90 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   100 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   110 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   120 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   130 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   140 */  1254, 1254, 1472, 1479, 1478, 1477, 1486, 1476, 1473, 1468,
 /*   150 */  1282, 1469, 1283, 1314, 1364, 1254, 1254, 1254, 1254, 1254,
 /*   160 */  1254, 1311, 1284, 1258, 1460, 1459, 1458, 1254, 1483, 1470,
 /*   170 */  1482, 1481, 1619, 1618, 1560, 1254, 1254, 1518, 1254, 1254,
 /*   180 */  1254, 1254, 1254, 1587, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   190 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   200 */  1254, 1254, 1254, 1254, 1254, 1409, 1254, 1587, 1587, 1258,
 /*   210 */  1587, 1587, 1319, 1410, 1410, 1319, 1272, 1567, 1265, 1265,
 /*   220 */  1265, 1265, 1389, 1265, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   230 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   240 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1557, 1555,
 /*   250 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   260 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1269, 1254,
 /*   270 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1612,
 /*   280 */  1323, 1254, 1530, 1377, 1269, 1269, 1269, 1269, 1271, 1264,
 /*   290 */  1260, 1376, 1297, 1633, 1339, 1254, 1334, 1418, 1655, 1439,
 /*   300 */  1428, 1270, 1428, 1652, 1276, 1439, 1439, 1276, 1439, 1270,
 /*   310 */  1652, 1418, 1418, 1409, 1409, 1409, 1409, 1272, 1272, 1498,
 /*   320 */  1270, 1264, 1508, 1365, 1365, 1365, 1357, 1254, 1254, 1365,
 /*   330 */  1354, 1508, 1365, 1339, 1365, 1354, 1365, 1655, 1655, 1395,
 /*   340 */  1395, 1654, 1654, 1395, 1518, 1639, 1448, 1367, 1373, 1373,
 /*   350 */  1373, 1373, 1395, 1308, 1276, 1639, 1639, 1276, 1448, 1367,
 /*   360 */  1276, 1367, 1276, 1395, 1308, 1534, 1649, 1395, 1308, 1254,
 /*   370 */  1512, 1512, 1508, 1395, 1275, 1272, 1597, 1597, 1421, 1421,
 /*   380 */  1605, 1503, 1395, 1254, 1275, 1274, 1273, 1276, 1508, 1395,
 /*   390 */  1308, 1395, 1308, 1615, 1615, 1611, 1611, 1611, 1626, 1626,
 /*   400 */  1341, 1660, 1660, 1567, 1341, 1323, 1323, 1626, 1258, 1258,
 /*   410 */  1258, 1258, 1258, 1254, 1254, 1254, 1254, 1254, 1621, 1254,
 /*   420 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1562, 1519, 1399,
 /*   430 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1573, 1254,
 /*   440 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   450 */  1452, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   460 */  1254, 1254, 1312, 1254, 1254, 1254, 1564, 1337, 1254, 1254,
 /*   470 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   480 */  1254, 1254, 1430, 1431, 1400, 1254, 1254, 1254, 1254, 1254,
 /*   490 */  1254, 1254, 1445, 1254, 1254, 1254, 1440, 1254, 1254, 1254,
 /*   500 */  1254, 1254, 1254, 1254, 1254, 1651, 1254, 1254, 1254, 1254,
 /*   510 */  1254, 1254, 1533, 1532, 1254, 1254, 1397, 1254, 1254, 1254,
 /*   520 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1422, 1254,
 /*   530 */  1254, 1254, 1254, 1602, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   540 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   550 */  1413, 1254, 1254, 1254, 1254, 1254, 1642, 1254, 1254, 1254,
 /*   560 */  1254, 1254, 1254, 1254, 1254, 1636, 1254, 1254, 1254, 1254,
 /*   570 */  1254, 1384, 1280, 1453, 1254, 1254, 1379, 1254, 1254, 1254,
 /*   580 */  1302, 1254, 1294,
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
 /*  22 */ "term ::= NULL|FLOAT|BLOB",
 /*  23 */ "term ::= STRING",
 /*  24 */ "term ::= INTEGER",
 /*  25 */ "expr ::= expr LT|GT|GE|LE expr",
 /*  26 */ "expr ::= expr PLUS|MINUS expr",
 /*  27 */ "expr ::= expr STAR|SLASH|REM expr",
 /*  28 */ "input ::= cmdlist",
 /*  29 */ "cmdlist ::= cmdlist ecmd",
 /*  30 */ "cmdlist ::= ecmd",
 /*  31 */ "ecmd ::= SEMI",
 /*  32 */ "ecmd ::= cmdx SEMI",
 /*  33 */ "nm ::= STRING",
 /*  34 */ "selectnowith ::= oneselect",
 /*  35 */ "sclp ::= selcollist COMMA",
 /*  36 */ "expr ::= term",
 /*  37 */ "explain ::= EXPLAIN",
 /*  38 */ "explain ::= EXPLAIN QUERY PLAN",
 /*  39 */ "cmd ::= BEGIN transtype trans_opt",
 /*  40 */ "transtype ::=",
 /*  41 */ "transtype ::= DEFERRED",
 /*  42 */ "transtype ::= IMMEDIATE",
 /*  43 */ "transtype ::= EXCLUSIVE",
 /*  44 */ "cmd ::= COMMIT|END trans_opt",
 /*  45 */ "cmd ::= ROLLBACK trans_opt",
 /*  46 */ "cmd ::= SAVEPOINT nm",
 /*  47 */ "cmd ::= RELEASE savepoint_opt nm",
 /*  48 */ "cmd ::= ROLLBACK trans_opt TO savepoint_opt nm",
 /*  49 */ "create_table ::= createkw temp TABLE ifnotexists nm dbnm",
 /*  50 */ "createkw ::= CREATE",
 /*  51 */ "ifnotexists ::=",
 /*  52 */ "ifnotexists ::= IF NOT EXISTS",
 /*  53 */ "temp ::= TEMP",
 /*  54 */ "temp ::=",
 /*  55 */ "create_table_args ::= LP columnlist conslist_opt RP table_option_set",
 /*  56 */ "create_table_args ::= AS select",
 /*  57 */ "table_option_set ::=",
 /*  58 */ "table_option_set ::= table_option_set COMMA table_option",
 /*  59 */ "table_option ::= WITHOUT nm",
 /*  60 */ "table_option ::= nm",
 /*  61 */ "columnname ::= nm typetoken",
 /*  62 */ "typetoken ::=",
 /*  63 */ "typetoken ::= typename LP signed RP",
 /*  64 */ "typetoken ::= typename LP signed COMMA signed RP",
 /*  65 */ "typename ::= typename ID|STRING",
 /*  66 */ "scantok ::=",
 /*  67 */ "ccons ::= CONSTRAINT nm",
 /*  68 */ "ccons ::= DEFAULT scantok term",
 /*  69 */ "ccons ::= DEFAULT LP expr RP",
 /*  70 */ "ccons ::= DEFAULT PLUS scantok term",
 /*  71 */ "ccons ::= DEFAULT MINUS scantok term",
 /*  72 */ "ccons ::= DEFAULT scantok ID|INDEXED",
 /*  73 */ "ccons ::= NOT NULL onconf",
 /*  74 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
 /*  75 */ "ccons ::= UNIQUE onconf",
 /*  76 */ "ccons ::= CHECK LP expr RP",
 /*  77 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
 /*  78 */ "ccons ::= defer_subclause",
 /*  79 */ "ccons ::= COLLATE ID|STRING",
 /*  80 */ "generated ::= LP expr RP",
 /*  81 */ "generated ::= LP expr RP ID",
 /*  82 */ "autoinc ::=",
 /*  83 */ "autoinc ::= AUTOINCR",
 /*  84 */ "refargs ::=",
 /*  85 */ "refargs ::= refargs refarg",
 /*  86 */ "refarg ::= MATCH nm",
 /*  87 */ "refarg ::= ON INSERT refact",
 /*  88 */ "refarg ::= ON DELETE refact",
 /*  89 */ "refarg ::= ON UPDATE refact",
 /*  90 */ "refact ::= SET NULL",
 /*  91 */ "refact ::= SET DEFAULT",
 /*  92 */ "refact ::= CASCADE",
 /*  93 */ "refact ::= RESTRICT",
 /*  94 */ "refact ::= NO ACTION",
 /*  95 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
 /*  96 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
 /*  97 */ "init_deferred_pred_opt ::=",
 /*  98 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
 /*  99 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
 /* 100 */ "conslist_opt ::=",
 /* 101 */ "tconscomma ::= COMMA",
 /* 102 */ "tcons ::= CONSTRAINT nm",
 /* 103 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf",
 /* 104 */ "tcons ::= UNIQUE LP sortlist RP onconf",
 /* 105 */ "tcons ::= CHECK LP expr RP onconf",
 /* 106 */ "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /* 107 */ "defer_subclause_opt ::=",
 /* 108 */ "onconf ::=",
 /* 109 */ "onconf ::= ON CONFLICT resolvetype",
 /* 110 */ "orconf ::=",
 /* 111 */ "orconf ::= OR resolvetype",
 /* 112 */ "resolvetype ::= IGNORE",
 /* 113 */ "resolvetype ::= REPLACE",
 /* 114 */ "cmd ::= DROP TABLE ifexists fullname",
 /* 115 */ "ifexists ::= IF EXISTS",
 /* 116 */ "ifexists ::=",
 /* 117 */ "cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select",
 /* 118 */ "cmd ::= DROP VIEW ifexists fullname",
 /* 119 */ "select ::= WITH wqlist selectnowith",
 /* 120 */ "select ::= WITH RECURSIVE wqlist selectnowith",
 /* 121 */ "selectnowith ::= selectnowith multiselect_op oneselect",
 /* 122 */ "multiselect_op ::= UNION",
 /* 123 */ "multiselect_op ::= UNION ALL",
 /* 124 */ "multiselect_op ::= EXCEPT|INTERSECT",
 /* 125 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt",
 /* 126 */ "values ::= VALUES LP nexprlist RP",
 /* 127 */ "oneselect ::= mvalues",
 /* 128 */ "mvalues ::= values COMMA LP nexprlist RP",
 /* 129 */ "mvalues ::= mvalues COMMA LP nexprlist RP",
 /* 130 */ "selcollist ::= sclp scanpt nm DOT STAR",
 /* 131 */ "stl_prefix ::= seltablist joinop",
 /* 132 */ "stl_prefix ::=",
 /* 133 */ "seltablist ::= stl_prefix nm dbnm as on_using",
 /* 134 */ "seltablist ::= stl_prefix nm dbnm as indexed_by on_using",
 /* 135 */ "seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using",
 /* 136 */ "seltablist ::= stl_prefix LP select RP as on_using",
 /* 137 */ "seltablist ::= stl_prefix LP seltablist RP as on_using",
 /* 138 */ "dbnm ::=",
 /* 139 */ "dbnm ::= DOT nm",
 /* 140 */ "fullname ::= nm",
 /* 141 */ "fullname ::= nm DOT nm",
 /* 142 */ "xfullname ::= nm",
 /* 143 */ "xfullname ::= nm DOT nm",
 /* 144 */ "xfullname ::= nm DOT nm AS nm",
 /* 145 */ "xfullname ::= nm AS nm",
 /* 146 */ "joinop ::= COMMA|JOIN",
 /* 147 */ "joinop ::= JOIN_KW JOIN",
 /* 148 */ "joinop ::= JOIN_KW nm JOIN",
 /* 149 */ "joinop ::= JOIN_KW nm nm JOIN",
 /* 150 */ "on_using ::= ON expr",
 /* 151 */ "on_using ::= USING LP idlist RP",
 /* 152 */ "on_using ::=",
 /* 153 */ "indexed_opt ::=",
 /* 154 */ "indexed_by ::= INDEXED BY nm",
 /* 155 */ "indexed_by ::= NOT INDEXED",
 /* 156 */ "orderby_opt ::= ORDER BY sortlist",
 /* 157 */ "sortlist ::= sortlist COMMA expr sortorder nulls",
 /* 158 */ "sortlist ::= expr sortorder nulls",
 /* 159 */ "sortorder ::= ASC",
 /* 160 */ "sortorder ::= DESC",
 /* 161 */ "sortorder ::=",
 /* 162 */ "nulls ::= NULLS FIRST",
 /* 163 */ "nulls ::= NULLS LAST",
 /* 164 */ "nulls ::=",
 /* 165 */ "groupby_opt ::= GROUP BY nexprlist",
 /* 166 */ "having_opt ::= HAVING expr",
 /* 167 */ "limit_opt ::= LIMIT expr",
 /* 168 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /* 169 */ "limit_opt ::= LIMIT expr COMMA expr",
 /* 170 */ "cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret",
 /* 171 */ "where_opt_ret ::=",
 /* 172 */ "where_opt_ret ::= WHERE expr",
 /* 173 */ "where_opt_ret ::= RETURNING selcollist",
 /* 174 */ "where_opt_ret ::= WHERE expr RETURNING selcollist",
 /* 175 */ "cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret",
 /* 176 */ "setlist ::= setlist COMMA nm EQ expr",
 /* 177 */ "setlist ::= setlist COMMA LP idlist RP EQ expr",
 /* 178 */ "setlist ::= nm EQ expr",
 /* 179 */ "setlist ::= LP idlist RP EQ expr",
 /* 180 */ "cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert",
 /* 181 */ "cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning",
 /* 182 */ "upsert ::=",
 /* 183 */ "upsert ::= RETURNING selcollist",
 /* 184 */ "upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert",
 /* 185 */ "upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert",
 /* 186 */ "upsert ::= ON CONFLICT DO NOTHING returning",
 /* 187 */ "upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning",
 /* 188 */ "returning ::= RETURNING selcollist",
 /* 189 */ "insert_cmd ::= INSERT orconf",
 /* 190 */ "insert_cmd ::= REPLACE",
 /* 191 */ "idlist_opt ::=",
 /* 192 */ "idlist_opt ::= LP idlist RP",
 /* 193 */ "idlist ::= idlist COMMA nm",
 /* 194 */ "idlist ::= nm",
 /* 195 */ "expr ::= ID|INDEXED|JOIN_KW",
 /* 196 */ "expr ::= nm DOT nm",
 /* 197 */ "expr ::= nm DOT nm DOT nm",
 /* 198 */ "expr ::= VARIABLE",
 /* 199 */ "expr ::= expr COLLATE ID|STRING",
 /* 200 */ "expr ::= CAST LP expr AS typetoken RP",
 /* 201 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP",
 /* 202 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP",
 /* 203 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP",
 /* 204 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over",
 /* 205 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over",
 /* 206 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over",
 /* 207 */ "term ::= CTIME_KW",
 /* 208 */ "expr ::= LP nexprlist COMMA expr RP",
 /* 209 */ "expr ::= expr AND expr",
 /* 210 */ "expr ::= expr OR expr",
 /* 211 */ "expr ::= expr EQ|NE expr",
 /* 212 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
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
   189,  /* (11) as ::= AS nm */
   189,  /* (12) as ::= */
   241,  /* (13) from ::= */
   241,  /* (14) from ::= FROM seltablist */
   245,  /* (15) orderby_opt ::= */
   243,  /* (16) groupby_opt ::= */
   244,  /* (17) having_opt ::= */
   246,  /* (18) limit_opt ::= */
   242,  /* (19) where_opt ::= */
   242,  /* (20) where_opt ::= WHERE expr */
   215,  /* (21) expr ::= LP expr RP */
   214,  /* (22) term ::= NULL|FLOAT|BLOB */
   214,  /* (23) term ::= STRING */
   214,  /* (24) term ::= INTEGER */
   215,  /* (25) expr ::= expr LT|GT|GE|LE expr */
   215,  /* (26) expr ::= expr PLUS|MINUS expr */
   215,  /* (27) expr ::= expr STAR|SLASH|REM expr */
   319,  /* (28) input ::= cmdlist */
   320,  /* (29) cmdlist ::= cmdlist ecmd */
   320,  /* (30) cmdlist ::= ecmd */
   321,  /* (31) ecmd ::= SEMI */
   321,  /* (32) ecmd ::= cmdx SEMI */
   187,  /* (33) nm ::= STRING */
   236,  /* (34) selectnowith ::= oneselect */
   251,  /* (35) sclp ::= selcollist COMMA */
   215,  /* (36) expr ::= term */
   191,  /* (37) explain ::= EXPLAIN */
   191,  /* (38) explain ::= EXPLAIN QUERY PLAN */
   193,  /* (39) cmd ::= BEGIN transtype trans_opt */
   194,  /* (40) transtype ::= */
   194,  /* (41) transtype ::= DEFERRED */
   194,  /* (42) transtype ::= IMMEDIATE */
   194,  /* (43) transtype ::= EXCLUSIVE */
   193,  /* (44) cmd ::= COMMIT|END trans_opt */
   193,  /* (45) cmd ::= ROLLBACK trans_opt */
   193,  /* (46) cmd ::= SAVEPOINT nm */
   193,  /* (47) cmd ::= RELEASE savepoint_opt nm */
   193,  /* (48) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
   197,  /* (49) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   198,  /* (50) createkw ::= CREATE */
   200,  /* (51) ifnotexists ::= */
   200,  /* (52) ifnotexists ::= IF NOT EXISTS */
   199,  /* (53) temp ::= TEMP */
   199,  /* (54) temp ::= */
   202,  /* (55) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   202,  /* (56) create_table_args ::= AS select */
   205,  /* (57) table_option_set ::= */
   205,  /* (58) table_option_set ::= table_option_set COMMA table_option */
   207,  /* (59) table_option ::= WITHOUT nm */
   207,  /* (60) table_option ::= nm */
   208,  /* (61) columnname ::= nm typetoken */
   209,  /* (62) typetoken ::= */
   209,  /* (63) typetoken ::= typename LP signed RP */
   209,  /* (64) typetoken ::= typename LP signed COMMA signed RP */
   210,  /* (65) typename ::= typename ID|STRING */
   212,  /* (66) scantok ::= */
   213,  /* (67) ccons ::= CONSTRAINT nm */
   213,  /* (68) ccons ::= DEFAULT scantok term */
   213,  /* (69) ccons ::= DEFAULT LP expr RP */
   213,  /* (70) ccons ::= DEFAULT PLUS scantok term */
   213,  /* (71) ccons ::= DEFAULT MINUS scantok term */
   213,  /* (72) ccons ::= DEFAULT scantok ID|INDEXED */
   213,  /* (73) ccons ::= NOT NULL onconf */
   213,  /* (74) ccons ::= PRIMARY KEY sortorder onconf autoinc */
   213,  /* (75) ccons ::= UNIQUE onconf */
   213,  /* (76) ccons ::= CHECK LP expr RP */
   213,  /* (77) ccons ::= REFERENCES nm eidlist_opt refargs */
   213,  /* (78) ccons ::= defer_subclause */
   213,  /* (79) ccons ::= COLLATE ID|STRING */
   222,  /* (80) generated ::= LP expr RP */
   222,  /* (81) generated ::= LP expr RP ID */
   218,  /* (82) autoinc ::= */
   218,  /* (83) autoinc ::= AUTOINCR */
   220,  /* (84) refargs ::= */
   220,  /* (85) refargs ::= refargs refarg */
   223,  /* (86) refarg ::= MATCH nm */
   223,  /* (87) refarg ::= ON INSERT refact */
   223,  /* (88) refarg ::= ON DELETE refact */
   223,  /* (89) refarg ::= ON UPDATE refact */
   224,  /* (90) refact ::= SET NULL */
   224,  /* (91) refact ::= SET DEFAULT */
   224,  /* (92) refact ::= CASCADE */
   224,  /* (93) refact ::= RESTRICT */
   224,  /* (94) refact ::= NO ACTION */
   221,  /* (95) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
   221,  /* (96) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
   225,  /* (97) init_deferred_pred_opt ::= */
   225,  /* (98) init_deferred_pred_opt ::= INITIALLY DEFERRED */
   225,  /* (99) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
   204,  /* (100) conslist_opt ::= */
   226,  /* (101) tconscomma ::= COMMA */
   227,  /* (102) tcons ::= CONSTRAINT nm */
   227,  /* (103) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
   227,  /* (104) tcons ::= UNIQUE LP sortlist RP onconf */
   227,  /* (105) tcons ::= CHECK LP expr RP onconf */
   227,  /* (106) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
   230,  /* (107) defer_subclause_opt ::= */
   216,  /* (108) onconf ::= */
   216,  /* (109) onconf ::= ON CONFLICT resolvetype */
   232,  /* (110) orconf ::= */
   232,  /* (111) orconf ::= OR resolvetype */
   231,  /* (112) resolvetype ::= IGNORE */
   231,  /* (113) resolvetype ::= REPLACE */
   193,  /* (114) cmd ::= DROP TABLE ifexists fullname */
   233,  /* (115) ifexists ::= IF EXISTS */
   233,  /* (116) ifexists ::= */
   193,  /* (117) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   193,  /* (118) cmd ::= DROP VIEW ifexists fullname */
   206,  /* (119) select ::= WITH wqlist selectnowith */
   206,  /* (120) select ::= WITH RECURSIVE wqlist selectnowith */
   236,  /* (121) selectnowith ::= selectnowith multiselect_op oneselect */
   237,  /* (122) multiselect_op ::= UNION */
   237,  /* (123) multiselect_op ::= UNION ALL */
   237,  /* (124) multiselect_op ::= EXCEPT|INTERSECT */
   238,  /* (125) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   248,  /* (126) values ::= VALUES LP nexprlist RP */
   238,  /* (127) oneselect ::= mvalues */
   250,  /* (128) mvalues ::= values COMMA LP nexprlist RP */
   250,  /* (129) mvalues ::= mvalues COMMA LP nexprlist RP */
   240,  /* (130) selcollist ::= sclp scanpt nm DOT STAR */
   253,  /* (131) stl_prefix ::= seltablist joinop */
   253,  /* (132) stl_prefix ::= */
   252,  /* (133) seltablist ::= stl_prefix nm dbnm as on_using */
   252,  /* (134) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   252,  /* (135) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   252,  /* (136) seltablist ::= stl_prefix LP select RP as on_using */
   252,  /* (137) seltablist ::= stl_prefix LP seltablist RP as on_using */
   201,  /* (138) dbnm ::= */
   201,  /* (139) dbnm ::= DOT nm */
   234,  /* (140) fullname ::= nm */
   234,  /* (141) fullname ::= nm DOT nm */
   258,  /* (142) xfullname ::= nm */
   258,  /* (143) xfullname ::= nm DOT nm */
   258,  /* (144) xfullname ::= nm DOT nm AS nm */
   258,  /* (145) xfullname ::= nm AS nm */
   254,  /* (146) joinop ::= COMMA|JOIN */
   254,  /* (147) joinop ::= JOIN_KW JOIN */
   254,  /* (148) joinop ::= JOIN_KW nm JOIN */
   254,  /* (149) joinop ::= JOIN_KW nm nm JOIN */
   255,  /* (150) on_using ::= ON expr */
   255,  /* (151) on_using ::= USING LP idlist RP */
   255,  /* (152) on_using ::= */
   260,  /* (153) indexed_opt ::= */
   256,  /* (154) indexed_by ::= INDEXED BY nm */
   256,  /* (155) indexed_by ::= NOT INDEXED */
   245,  /* (156) orderby_opt ::= ORDER BY sortlist */
   228,  /* (157) sortlist ::= sortlist COMMA expr sortorder nulls */
   228,  /* (158) sortlist ::= expr sortorder nulls */
   217,  /* (159) sortorder ::= ASC */
   217,  /* (160) sortorder ::= DESC */
   217,  /* (161) sortorder ::= */
   261,  /* (162) nulls ::= NULLS FIRST */
   261,  /* (163) nulls ::= NULLS LAST */
   261,  /* (164) nulls ::= */
   243,  /* (165) groupby_opt ::= GROUP BY nexprlist */
   244,  /* (166) having_opt ::= HAVING expr */
   246,  /* (167) limit_opt ::= LIMIT expr */
   246,  /* (168) limit_opt ::= LIMIT expr OFFSET expr */
   246,  /* (169) limit_opt ::= LIMIT expr COMMA expr */
   193,  /* (170) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
   263,  /* (171) where_opt_ret ::= */
   263,  /* (172) where_opt_ret ::= WHERE expr */
   263,  /* (173) where_opt_ret ::= RETURNING selcollist */
   263,  /* (174) where_opt_ret ::= WHERE expr RETURNING selcollist */
   193,  /* (175) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   264,  /* (176) setlist ::= setlist COMMA nm EQ expr */
   264,  /* (177) setlist ::= setlist COMMA LP idlist RP EQ expr */
   264,  /* (178) setlist ::= nm EQ expr */
   264,  /* (179) setlist ::= LP idlist RP EQ expr */
   193,  /* (180) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   193,  /* (181) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
   267,  /* (182) upsert ::= */
   267,  /* (183) upsert ::= RETURNING selcollist */
   267,  /* (184) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
   267,  /* (185) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
   267,  /* (186) upsert ::= ON CONFLICT DO NOTHING returning */
   267,  /* (187) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
   268,  /* (188) returning ::= RETURNING selcollist */
   265,  /* (189) insert_cmd ::= INSERT orconf */
   265,  /* (190) insert_cmd ::= REPLACE */
   266,  /* (191) idlist_opt ::= */
   266,  /* (192) idlist_opt ::= LP idlist RP */
   259,  /* (193) idlist ::= idlist COMMA nm */
   259,  /* (194) idlist ::= nm */
   215,  /* (195) expr ::= ID|INDEXED|JOIN_KW */
   215,  /* (196) expr ::= nm DOT nm */
   215,  /* (197) expr ::= nm DOT nm DOT nm */
   215,  /* (198) expr ::= VARIABLE */
   215,  /* (199) expr ::= expr COLLATE ID|STRING */
   215,  /* (200) expr ::= CAST LP expr AS typetoken RP */
   215,  /* (201) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   215,  /* (202) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   215,  /* (203) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   215,  /* (204) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   215,  /* (205) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   215,  /* (206) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   214,  /* (207) term ::= CTIME_KW */
   215,  /* (208) expr ::= LP nexprlist COMMA expr RP */
   215,  /* (209) expr ::= expr AND expr */
   215,  /* (210) expr ::= expr OR expr */
   215,  /* (211) expr ::= expr EQ|NE expr */
   215,  /* (212) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
   215,  /* (213) expr ::= expr CONCAT expr */
   270,  /* (214) likeop ::= NOT LIKE_KW|MATCH */
   215,  /* (215) expr ::= expr likeop expr */
   215,  /* (216) expr ::= expr likeop expr ESCAPE expr */
   215,  /* (217) expr ::= expr ISNULL|NOTNULL */
   215,  /* (218) expr ::= expr NOT NULL */
   215,  /* (219) expr ::= expr IS expr */
   215,  /* (220) expr ::= expr IS NOT expr */
   215,  /* (221) expr ::= expr IS NOT DISTINCT FROM expr */
   215,  /* (222) expr ::= expr IS DISTINCT FROM expr */
   215,  /* (223) expr ::= NOT expr */
   215,  /* (224) expr ::= BITNOT expr */
   215,  /* (225) expr ::= PLUS|MINUS expr */
   215,  /* (226) expr ::= expr PTR expr */
   271,  /* (227) between_op ::= BETWEEN */
   271,  /* (228) between_op ::= NOT BETWEEN */
   215,  /* (229) expr ::= expr between_op expr AND expr */
   272,  /* (230) in_op ::= IN */
   272,  /* (231) in_op ::= NOT IN */
   215,  /* (232) expr ::= expr in_op LP exprlist RP */
   215,  /* (233) expr ::= LP select RP */
   215,  /* (234) expr ::= expr in_op LP select RP */
   215,  /* (235) expr ::= expr in_op nm dbnm paren_exprlist */
   215,  /* (236) expr ::= EXISTS LP select RP */
   215,  /* (237) expr ::= CASE case_operand case_exprlist case_else END */
   275,  /* (238) case_exprlist ::= case_exprlist WHEN expr THEN expr */
   275,  /* (239) case_exprlist ::= WHEN expr THEN expr */
   276,  /* (240) case_else ::= ELSE expr */
   276,  /* (241) case_else ::= */
   274,  /* (242) case_operand ::= */
   257,  /* (243) exprlist ::= */
   249,  /* (244) nexprlist ::= nexprlist COMMA expr */
   249,  /* (245) nexprlist ::= expr */
   273,  /* (246) paren_exprlist ::= */
   273,  /* (247) paren_exprlist ::= LP exprlist RP */
   193,  /* (248) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
   277,  /* (249) uniqueflag ::= UNIQUE */
   277,  /* (250) uniqueflag ::= */
   219,  /* (251) eidlist_opt ::= */
   219,  /* (252) eidlist_opt ::= LP eidlist RP */
   229,  /* (253) eidlist ::= eidlist COMMA nm collate sortorder */
   229,  /* (254) eidlist ::= nm collate sortorder */
   278,  /* (255) collate ::= */
   278,  /* (256) collate ::= COLLATE ID|STRING */
   193,  /* (257) cmd ::= DROP INDEX ifexists fullname */
   193,  /* (258) cmd ::= VACUUM vinto */
   193,  /* (259) cmd ::= VACUUM nm vinto */
   279,  /* (260) vinto ::= INTO expr */
   279,  /* (261) vinto ::= */
   193,  /* (262) cmd ::= PRAGMA nm dbnm */
   193,  /* (263) cmd ::= PRAGMA nm dbnm EQ nmnum */
   193,  /* (264) cmd ::= PRAGMA nm dbnm LP nmnum RP */
   193,  /* (265) cmd ::= PRAGMA nm dbnm EQ minus_num */
   193,  /* (266) cmd ::= PRAGMA nm dbnm LP minus_num RP */
   282,  /* (267) plus_num ::= PLUS INTEGER|FLOAT */
   281,  /* (268) minus_num ::= MINUS INTEGER|FLOAT */
   193,  /* (269) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
   283,  /* (270) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
   285,  /* (271) trigger_time ::= BEFORE|AFTER */
   285,  /* (272) trigger_time ::= INSTEAD OF */
   285,  /* (273) trigger_time ::= */
   286,  /* (274) trigger_event ::= DELETE|INSERT */
   286,  /* (275) trigger_event ::= UPDATE */
   286,  /* (276) trigger_event ::= UPDATE OF idlist */
   288,  /* (277) when_clause ::= */
   288,  /* (278) when_clause ::= WHEN expr */
   284,  /* (279) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
   284,  /* (280) trigger_cmd_list ::= trigger_cmd SEMI */
   290,  /* (281) trnm ::= nm DOT nm */
   291,  /* (282) tridxby ::= INDEXED BY nm */
   291,  /* (283) tridxby ::= NOT INDEXED */
   289,  /* (284) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
   289,  /* (285) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
   289,  /* (286) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
   289,  /* (287) trigger_cmd ::= scanpt select scanpt */
   215,  /* (288) expr ::= RAISE LP IGNORE RP */
   215,  /* (289) expr ::= RAISE LP raisetype COMMA expr RP */
   292,  /* (290) raisetype ::= ROLLBACK */
   292,  /* (291) raisetype ::= ABORT */
   292,  /* (292) raisetype ::= FAIL */
   193,  /* (293) cmd ::= DROP TRIGGER ifexists fullname */
   193,  /* (294) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
   193,  /* (295) cmd ::= DETACH database_kw_opt expr */
   294,  /* (296) key_opt ::= */
   294,  /* (297) key_opt ::= KEY expr */
   193,  /* (298) cmd ::= REINDEX */
   193,  /* (299) cmd ::= REINDEX nm dbnm */
   193,  /* (300) cmd ::= ANALYZE */
   193,  /* (301) cmd ::= ANALYZE nm dbnm */
   193,  /* (302) cmd ::= ALTER TABLE fullname RENAME TO nm */
   193,  /* (303) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
   193,  /* (304) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
   295,  /* (305) add_column_fullname ::= fullname */
   193,  /* (306) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
   193,  /* (307) cmd ::= create_vtab */
   193,  /* (308) cmd ::= create_vtab LP vtabarglist RP */
   298,  /* (309) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
   300,  /* (310) vtabarg ::= */
   301,  /* (311) vtabargtoken ::= ANY */
   301,  /* (312) vtabargtoken ::= lp anylist RP */
   302,  /* (313) lp ::= LP */
   262,  /* (314) with ::= WITH wqlist */
   262,  /* (315) with ::= WITH RECURSIVE wqlist */
   304,  /* (316) wqas ::= AS */
   304,  /* (317) wqas ::= AS MATERIALIZED */
   304,  /* (318) wqas ::= AS NOT MATERIALIZED */
   305,  /* (319) wqitem ::= withnm eidlist_opt wqas LP select RP */
   306,  /* (320) withnm ::= nm */
   235,  /* (321) wqlist ::= wqitem */
   235,  /* (322) wqlist ::= wqlist COMMA wqitem */
   307,  /* (323) windowdefn_list ::= windowdefn_list COMMA windowdefn */
   308,  /* (324) windowdefn ::= nm AS LP window RP */
   309,  /* (325) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
   309,  /* (326) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
   309,  /* (327) window ::= ORDER BY sortlist frame_opt */
   309,  /* (328) window ::= nm ORDER BY sortlist frame_opt */
   309,  /* (329) window ::= nm frame_opt */
   310,  /* (330) frame_opt ::= */
   310,  /* (331) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
   310,  /* (332) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
   311,  /* (333) range_or_rows ::= RANGE|ROWS|GROUPS */
   312,  /* (334) frame_bound_s ::= frame_bound */
   312,  /* (335) frame_bound_s ::= UNBOUNDED PRECEDING */
   314,  /* (336) frame_bound_e ::= frame_bound */
   314,  /* (337) frame_bound_e ::= UNBOUNDED FOLLOWING */
   315,  /* (338) frame_bound ::= expr PRECEDING|FOLLOWING */
   315,  /* (339) frame_bound ::= CURRENT ROW */
   313,  /* (340) frame_exclude_opt ::= */
   313,  /* (341) frame_exclude_opt ::= EXCLUDE frame_exclude */
   316,  /* (342) frame_exclude ::= NO OTHERS */
   316,  /* (343) frame_exclude ::= CURRENT ROW */
   316,  /* (344) frame_exclude ::= GROUP|TIES */
   247,  /* (345) window_clause ::= WINDOW windowdefn_list */
   269,  /* (346) filter_over ::= filter_clause over_clause */
   269,  /* (347) filter_over ::= over_clause */
   269,  /* (348) filter_over ::= filter_clause */
   318,  /* (349) over_clause ::= OVER LP window RP */
   318,  /* (350) over_clause ::= OVER nm */
   317,  /* (351) filter_clause ::= FILTER LP WHERE expr RP */
   214,  /* (352) term ::= QNUMBER */
   321,  /* (353) ecmd ::= explain cmdx SEMI */
   195,  /* (354) trans_opt ::= */
   195,  /* (355) trans_opt ::= TRANSACTION */
   195,  /* (356) trans_opt ::= TRANSACTION nm */
   196,  /* (357) savepoint_opt ::= SAVEPOINT */
   196,  /* (358) savepoint_opt ::= */
   193,  /* (359) cmd ::= create_table create_table_args */
   205,  /* (360) table_option_set ::= table_option */
   203,  /* (361) columnlist ::= columnlist COMMA columnname carglist */
   203,  /* (362) columnlist ::= columnname carglist */
   187,  /* (363) nm ::= ID|INDEXED|JOIN_KW */
   209,  /* (364) typetoken ::= typename */
   210,  /* (365) typename ::= ID|STRING */
   211,  /* (366) signed ::= plus_num */
   211,  /* (367) signed ::= minus_num */
   297,  /* (368) carglist ::= carglist ccons */
   297,  /* (369) carglist ::= */
   213,  /* (370) ccons ::= NULL onconf */
   213,  /* (371) ccons ::= GENERATED ALWAYS AS generated */
   213,  /* (372) ccons ::= AS generated */
   204,  /* (373) conslist_opt ::= COMMA conslist */
   322,  /* (374) conslist ::= conslist tconscomma tcons */
   322,  /* (375) conslist ::= tcons */
   226,  /* (376) tconscomma ::= */
   230,  /* (377) defer_subclause_opt ::= defer_subclause */
   231,  /* (378) resolvetype ::= raisetype */
   238,  /* (379) oneselect ::= values */
   189,  /* (380) as ::= ID|STRING */
   260,  /* (381) indexed_opt ::= indexed_by */
   268,  /* (382) returning ::= */
   270,  /* (383) likeop ::= LIKE_KW|MATCH */
   274,  /* (384) case_operand ::= expr */
   257,  /* (385) exprlist ::= nexprlist */
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
   -1,  /* (22) term ::= NULL|FLOAT|BLOB */
   -1,  /* (23) term ::= STRING */
   -1,  /* (24) term ::= INTEGER */
   -3,  /* (25) expr ::= expr LT|GT|GE|LE expr */
   -3,  /* (26) expr ::= expr PLUS|MINUS expr */
   -3,  /* (27) expr ::= expr STAR|SLASH|REM expr */
   -1,  /* (28) input ::= cmdlist */
   -2,  /* (29) cmdlist ::= cmdlist ecmd */
   -1,  /* (30) cmdlist ::= ecmd */
   -1,  /* (31) ecmd ::= SEMI */
   -2,  /* (32) ecmd ::= cmdx SEMI */
   -1,  /* (33) nm ::= STRING */
   -1,  /* (34) selectnowith ::= oneselect */
   -2,  /* (35) sclp ::= selcollist COMMA */
   -1,  /* (36) expr ::= term */
   -1,  /* (37) explain ::= EXPLAIN */
   -3,  /* (38) explain ::= EXPLAIN QUERY PLAN */
   -3,  /* (39) cmd ::= BEGIN transtype trans_opt */
    0,  /* (40) transtype ::= */
   -1,  /* (41) transtype ::= DEFERRED */
   -1,  /* (42) transtype ::= IMMEDIATE */
   -1,  /* (43) transtype ::= EXCLUSIVE */
   -2,  /* (44) cmd ::= COMMIT|END trans_opt */
   -2,  /* (45) cmd ::= ROLLBACK trans_opt */
   -2,  /* (46) cmd ::= SAVEPOINT nm */
   -3,  /* (47) cmd ::= RELEASE savepoint_opt nm */
   -5,  /* (48) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
   -6,  /* (49) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   -1,  /* (50) createkw ::= CREATE */
    0,  /* (51) ifnotexists ::= */
   -3,  /* (52) ifnotexists ::= IF NOT EXISTS */
   -1,  /* (53) temp ::= TEMP */
    0,  /* (54) temp ::= */
   -5,  /* (55) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   -2,  /* (56) create_table_args ::= AS select */
    0,  /* (57) table_option_set ::= */
   -3,  /* (58) table_option_set ::= table_option_set COMMA table_option */
   -2,  /* (59) table_option ::= WITHOUT nm */
   -1,  /* (60) table_option ::= nm */
   -2,  /* (61) columnname ::= nm typetoken */
    0,  /* (62) typetoken ::= */
   -4,  /* (63) typetoken ::= typename LP signed RP */
   -6,  /* (64) typetoken ::= typename LP signed COMMA signed RP */
   -2,  /* (65) typename ::= typename ID|STRING */
    0,  /* (66) scantok ::= */
   -2,  /* (67) ccons ::= CONSTRAINT nm */
   -3,  /* (68) ccons ::= DEFAULT scantok term */
   -4,  /* (69) ccons ::= DEFAULT LP expr RP */
   -4,  /* (70) ccons ::= DEFAULT PLUS scantok term */
   -4,  /* (71) ccons ::= DEFAULT MINUS scantok term */
   -3,  /* (72) ccons ::= DEFAULT scantok ID|INDEXED */
   -3,  /* (73) ccons ::= NOT NULL onconf */
   -5,  /* (74) ccons ::= PRIMARY KEY sortorder onconf autoinc */
   -2,  /* (75) ccons ::= UNIQUE onconf */
   -4,  /* (76) ccons ::= CHECK LP expr RP */
   -4,  /* (77) ccons ::= REFERENCES nm eidlist_opt refargs */
   -1,  /* (78) ccons ::= defer_subclause */
   -2,  /* (79) ccons ::= COLLATE ID|STRING */
   -3,  /* (80) generated ::= LP expr RP */
   -4,  /* (81) generated ::= LP expr RP ID */
    0,  /* (82) autoinc ::= */
   -1,  /* (83) autoinc ::= AUTOINCR */
    0,  /* (84) refargs ::= */
   -2,  /* (85) refargs ::= refargs refarg */
   -2,  /* (86) refarg ::= MATCH nm */
   -3,  /* (87) refarg ::= ON INSERT refact */
   -3,  /* (88) refarg ::= ON DELETE refact */
   -3,  /* (89) refarg ::= ON UPDATE refact */
   -2,  /* (90) refact ::= SET NULL */
   -2,  /* (91) refact ::= SET DEFAULT */
   -1,  /* (92) refact ::= CASCADE */
   -1,  /* (93) refact ::= RESTRICT */
   -2,  /* (94) refact ::= NO ACTION */
   -3,  /* (95) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
   -2,  /* (96) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
    0,  /* (97) init_deferred_pred_opt ::= */
   -2,  /* (98) init_deferred_pred_opt ::= INITIALLY DEFERRED */
   -2,  /* (99) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
    0,  /* (100) conslist_opt ::= */
   -1,  /* (101) tconscomma ::= COMMA */
   -2,  /* (102) tcons ::= CONSTRAINT nm */
   -7,  /* (103) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
   -5,  /* (104) tcons ::= UNIQUE LP sortlist RP onconf */
   -5,  /* (105) tcons ::= CHECK LP expr RP onconf */
  -10,  /* (106) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
    0,  /* (107) defer_subclause_opt ::= */
    0,  /* (108) onconf ::= */
   -3,  /* (109) onconf ::= ON CONFLICT resolvetype */
    0,  /* (110) orconf ::= */
   -2,  /* (111) orconf ::= OR resolvetype */
   -1,  /* (112) resolvetype ::= IGNORE */
   -1,  /* (113) resolvetype ::= REPLACE */
   -4,  /* (114) cmd ::= DROP TABLE ifexists fullname */
   -2,  /* (115) ifexists ::= IF EXISTS */
    0,  /* (116) ifexists ::= */
   -9,  /* (117) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   -4,  /* (118) cmd ::= DROP VIEW ifexists fullname */
   -3,  /* (119) select ::= WITH wqlist selectnowith */
   -4,  /* (120) select ::= WITH RECURSIVE wqlist selectnowith */
   -3,  /* (121) selectnowith ::= selectnowith multiselect_op oneselect */
   -1,  /* (122) multiselect_op ::= UNION */
   -2,  /* (123) multiselect_op ::= UNION ALL */
   -1,  /* (124) multiselect_op ::= EXCEPT|INTERSECT */
  -10,  /* (125) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   -4,  /* (126) values ::= VALUES LP nexprlist RP */
   -1,  /* (127) oneselect ::= mvalues */
   -5,  /* (128) mvalues ::= values COMMA LP nexprlist RP */
   -5,  /* (129) mvalues ::= mvalues COMMA LP nexprlist RP */
   -5,  /* (130) selcollist ::= sclp scanpt nm DOT STAR */
   -2,  /* (131) stl_prefix ::= seltablist joinop */
    0,  /* (132) stl_prefix ::= */
   -5,  /* (133) seltablist ::= stl_prefix nm dbnm as on_using */
   -6,  /* (134) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   -8,  /* (135) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   -6,  /* (136) seltablist ::= stl_prefix LP select RP as on_using */
   -6,  /* (137) seltablist ::= stl_prefix LP seltablist RP as on_using */
    0,  /* (138) dbnm ::= */
   -2,  /* (139) dbnm ::= DOT nm */
   -1,  /* (140) fullname ::= nm */
   -3,  /* (141) fullname ::= nm DOT nm */
   -1,  /* (142) xfullname ::= nm */
   -3,  /* (143) xfullname ::= nm DOT nm */
   -5,  /* (144) xfullname ::= nm DOT nm AS nm */
   -3,  /* (145) xfullname ::= nm AS nm */
   -1,  /* (146) joinop ::= COMMA|JOIN */
   -2,  /* (147) joinop ::= JOIN_KW JOIN */
   -3,  /* (148) joinop ::= JOIN_KW nm JOIN */
   -4,  /* (149) joinop ::= JOIN_KW nm nm JOIN */
   -2,  /* (150) on_using ::= ON expr */
   -4,  /* (151) on_using ::= USING LP idlist RP */
    0,  /* (152) on_using ::= */
    0,  /* (153) indexed_opt ::= */
   -3,  /* (154) indexed_by ::= INDEXED BY nm */
   -2,  /* (155) indexed_by ::= NOT INDEXED */
   -3,  /* (156) orderby_opt ::= ORDER BY sortlist */
   -5,  /* (157) sortlist ::= sortlist COMMA expr sortorder nulls */
   -3,  /* (158) sortlist ::= expr sortorder nulls */
   -1,  /* (159) sortorder ::= ASC */
   -1,  /* (160) sortorder ::= DESC */
    0,  /* (161) sortorder ::= */
   -2,  /* (162) nulls ::= NULLS FIRST */
   -2,  /* (163) nulls ::= NULLS LAST */
    0,  /* (164) nulls ::= */
   -3,  /* (165) groupby_opt ::= GROUP BY nexprlist */
   -2,  /* (166) having_opt ::= HAVING expr */
   -2,  /* (167) limit_opt ::= LIMIT expr */
   -4,  /* (168) limit_opt ::= LIMIT expr OFFSET expr */
   -4,  /* (169) limit_opt ::= LIMIT expr COMMA expr */
   -6,  /* (170) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
    0,  /* (171) where_opt_ret ::= */
   -2,  /* (172) where_opt_ret ::= WHERE expr */
   -2,  /* (173) where_opt_ret ::= RETURNING selcollist */
   -4,  /* (174) where_opt_ret ::= WHERE expr RETURNING selcollist */
   -9,  /* (175) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   -5,  /* (176) setlist ::= setlist COMMA nm EQ expr */
   -7,  /* (177) setlist ::= setlist COMMA LP idlist RP EQ expr */
   -3,  /* (178) setlist ::= nm EQ expr */
   -5,  /* (179) setlist ::= LP idlist RP EQ expr */
   -7,  /* (180) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   -8,  /* (181) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
    0,  /* (182) upsert ::= */
   -2,  /* (183) upsert ::= RETURNING selcollist */
  -12,  /* (184) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
   -9,  /* (185) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
   -5,  /* (186) upsert ::= ON CONFLICT DO NOTHING returning */
   -8,  /* (187) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
   -2,  /* (188) returning ::= RETURNING selcollist */
   -2,  /* (189) insert_cmd ::= INSERT orconf */
   -1,  /* (190) insert_cmd ::= REPLACE */
    0,  /* (191) idlist_opt ::= */
   -3,  /* (192) idlist_opt ::= LP idlist RP */
   -3,  /* (193) idlist ::= idlist COMMA nm */
   -1,  /* (194) idlist ::= nm */
   -1,  /* (195) expr ::= ID|INDEXED|JOIN_KW */
   -3,  /* (196) expr ::= nm DOT nm */
   -5,  /* (197) expr ::= nm DOT nm DOT nm */
   -1,  /* (198) expr ::= VARIABLE */
   -3,  /* (199) expr ::= expr COLLATE ID|STRING */
   -6,  /* (200) expr ::= CAST LP expr AS typetoken RP */
   -5,  /* (201) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   -8,  /* (202) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   -4,  /* (203) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   -6,  /* (204) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   -9,  /* (205) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   -5,  /* (206) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   -1,  /* (207) term ::= CTIME_KW */
   -5,  /* (208) expr ::= LP nexprlist COMMA expr RP */
   -3,  /* (209) expr ::= expr AND expr */
   -3,  /* (210) expr ::= expr OR expr */
   -3,  /* (211) expr ::= expr EQ|NE expr */
   -3,  /* (212) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
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
      case 30: /* cmdlist ::= ecmd */ yytestcase(yyruleno==30);
      case 34: /* selectnowith ::= oneselect */ yytestcase(yyruleno==34);
      case 36: /* expr ::= term */ yytestcase(yyruleno==36);
{
    yylhsminor.yy9 = yymsp[0].minor.yy9;
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 1: /* scanpt ::= */
      case 12: /* as ::= */ yytestcase(yyruleno==12);
{
    yymsp[1].minor.yy0.z = NULL; yymsp[1].minor.yy0.n = 0;
}
        break;
      case 4: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
{
    yymsp[-8].minor.yy9 = ast_select_stmt(pCtx->astCtx, (uint8_t)yymsp[-7].minor.yy9, yymsp[-6].minor.yy9);
}
        break;
      case 5: /* distinct ::= DISTINCT */
{
    yymsp[0].minor.yy9 = 1;
}
        break;
      case 6: /* distinct ::= ALL */
{
    yymsp[0].minor.yy9 = 0;
}
        break;
      case 7: /* distinct ::= */
{
    yymsp[1].minor.yy9 = 0;
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
      case 11: /* as ::= AS nm */
{
    yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;
}
        break;
      case 14: /* from ::= FROM seltablist */
      case 20: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==20);
{
    yymsp[-1].minor.yy9 = yymsp[0].minor.yy9;
}
        break;
      case 21: /* expr ::= LP expr RP */
{
    yymsp[-2].minor.yy9 = yymsp[-1].minor.yy9;
}
        break;
      case 22: /* term ::= NULL|FLOAT|BLOB */
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
      case 23: /* term ::= STRING */
{
    yylhsminor.yy9 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_STRING, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 24: /* term ::= INTEGER */
{
    yylhsminor.yy9 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_INTEGER, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 25: /* expr ::= expr LT|GT|GE|LE expr */
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
      case 26: /* expr ::= expr PLUS|MINUS expr */
{
    SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == TK_PLUS) ? SYNTAQLITE_BINARY_OP_PLUS : SYNTAQLITE_BINARY_OP_MINUS;
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 27: /* expr ::= expr STAR|SLASH|REM expr */
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
      case 28: /* input ::= cmdlist */
{
    pCtx->root = yymsp[0].minor.yy9;
}
        break;
      case 29: /* cmdlist ::= cmdlist ecmd */
{
    yymsp[-1].minor.yy9 = yymsp[0].minor.yy9;  // Just use the last command for now
}
        break;
      case 31: /* ecmd ::= SEMI */
{
    yymsp[0].minor.yy9 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 32: /* ecmd ::= cmdx SEMI */
      case 35: /* sclp ::= selcollist COMMA */ yytestcase(yyruleno==35);
{
    yylhsminor.yy9 = yymsp[-1].minor.yy9;
}
  yymsp[-1].minor.yy9 = yylhsminor.yy9;
        break;
      case 33: /* nm ::= STRING */
{
    yylhsminor.yy0 = yymsp[0].minor.yy0;
}
  yymsp[0].minor.yy0 = yylhsminor.yy0;
        break;
      default:
      /* (37) explain ::= EXPLAIN */ yytestcase(yyruleno==37);
      /* (38) explain ::= EXPLAIN QUERY PLAN */ yytestcase(yyruleno==38);
      /* (39) cmd ::= BEGIN transtype trans_opt */ yytestcase(yyruleno==39);
      /* (40) transtype ::= */ yytestcase(yyruleno==40);
      /* (41) transtype ::= DEFERRED */ yytestcase(yyruleno==41);
      /* (42) transtype ::= IMMEDIATE */ yytestcase(yyruleno==42);
      /* (43) transtype ::= EXCLUSIVE */ yytestcase(yyruleno==43);
      /* (44) cmd ::= COMMIT|END trans_opt */ yytestcase(yyruleno==44);
      /* (45) cmd ::= ROLLBACK trans_opt */ yytestcase(yyruleno==45);
      /* (46) cmd ::= SAVEPOINT nm */ yytestcase(yyruleno==46);
      /* (47) cmd ::= RELEASE savepoint_opt nm */ yytestcase(yyruleno==47);
      /* (48) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */ yytestcase(yyruleno==48);
      /* (49) create_table ::= createkw temp TABLE ifnotexists nm dbnm */ yytestcase(yyruleno==49);
      /* (50) createkw ::= CREATE */ yytestcase(yyruleno==50);
      /* (51) ifnotexists ::= */ yytestcase(yyruleno==51);
      /* (52) ifnotexists ::= IF NOT EXISTS */ yytestcase(yyruleno==52);
      /* (53) temp ::= TEMP */ yytestcase(yyruleno==53);
      /* (54) temp ::= */ yytestcase(yyruleno==54);
      /* (55) create_table_args ::= LP columnlist conslist_opt RP table_option_set */ yytestcase(yyruleno==55);
      /* (56) create_table_args ::= AS select */ yytestcase(yyruleno==56);
      /* (57) table_option_set ::= */ yytestcase(yyruleno==57);
      /* (58) table_option_set ::= table_option_set COMMA table_option */ yytestcase(yyruleno==58);
      /* (59) table_option ::= WITHOUT nm */ yytestcase(yyruleno==59);
      /* (60) table_option ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=60);
      /* (61) columnname ::= nm typetoken */ yytestcase(yyruleno==61);
      /* (62) typetoken ::= */ yytestcase(yyruleno==62);
      /* (63) typetoken ::= typename LP signed RP */ yytestcase(yyruleno==63);
      /* (64) typetoken ::= typename LP signed COMMA signed RP */ yytestcase(yyruleno==64);
      /* (65) typename ::= typename ID|STRING */ yytestcase(yyruleno==65);
      /* (66) scantok ::= */ yytestcase(yyruleno==66);
      /* (67) ccons ::= CONSTRAINT nm */ yytestcase(yyruleno==67);
      /* (68) ccons ::= DEFAULT scantok term */ yytestcase(yyruleno==68);
      /* (69) ccons ::= DEFAULT LP expr RP */ yytestcase(yyruleno==69);
      /* (70) ccons ::= DEFAULT PLUS scantok term */ yytestcase(yyruleno==70);
      /* (71) ccons ::= DEFAULT MINUS scantok term */ yytestcase(yyruleno==71);
      /* (72) ccons ::= DEFAULT scantok ID|INDEXED */ yytestcase(yyruleno==72);
      /* (73) ccons ::= NOT NULL onconf */ yytestcase(yyruleno==73);
      /* (74) ccons ::= PRIMARY KEY sortorder onconf autoinc */ yytestcase(yyruleno==74);
      /* (75) ccons ::= UNIQUE onconf */ yytestcase(yyruleno==75);
      /* (76) ccons ::= CHECK LP expr RP */ yytestcase(yyruleno==76);
      /* (77) ccons ::= REFERENCES nm eidlist_opt refargs */ yytestcase(yyruleno==77);
      /* (78) ccons ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=78);
      /* (79) ccons ::= COLLATE ID|STRING */ yytestcase(yyruleno==79);
      /* (80) generated ::= LP expr RP */ yytestcase(yyruleno==80);
      /* (81) generated ::= LP expr RP ID */ yytestcase(yyruleno==81);
      /* (82) autoinc ::= */ yytestcase(yyruleno==82);
      /* (83) autoinc ::= AUTOINCR */ yytestcase(yyruleno==83);
      /* (84) refargs ::= */ yytestcase(yyruleno==84);
      /* (85) refargs ::= refargs refarg */ yytestcase(yyruleno==85);
      /* (86) refarg ::= MATCH nm */ yytestcase(yyruleno==86);
      /* (87) refarg ::= ON INSERT refact */ yytestcase(yyruleno==87);
      /* (88) refarg ::= ON DELETE refact */ yytestcase(yyruleno==88);
      /* (89) refarg ::= ON UPDATE refact */ yytestcase(yyruleno==89);
      /* (90) refact ::= SET NULL */ yytestcase(yyruleno==90);
      /* (91) refact ::= SET DEFAULT */ yytestcase(yyruleno==91);
      /* (92) refact ::= CASCADE */ yytestcase(yyruleno==92);
      /* (93) refact ::= RESTRICT */ yytestcase(yyruleno==93);
      /* (94) refact ::= NO ACTION */ yytestcase(yyruleno==94);
      /* (95) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */ yytestcase(yyruleno==95);
      /* (96) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */ yytestcase(yyruleno==96);
      /* (97) init_deferred_pred_opt ::= */ yytestcase(yyruleno==97);
      /* (98) init_deferred_pred_opt ::= INITIALLY DEFERRED */ yytestcase(yyruleno==98);
      /* (99) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */ yytestcase(yyruleno==99);
      /* (100) conslist_opt ::= */ yytestcase(yyruleno==100);
      /* (101) tconscomma ::= COMMA */ yytestcase(yyruleno==101);
      /* (102) tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==102);
      /* (103) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */ yytestcase(yyruleno==103);
      /* (104) tcons ::= UNIQUE LP sortlist RP onconf */ yytestcase(yyruleno==104);
      /* (105) tcons ::= CHECK LP expr RP onconf */ yytestcase(yyruleno==105);
      /* (106) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */ yytestcase(yyruleno==106);
      /* (107) defer_subclause_opt ::= */ yytestcase(yyruleno==107);
      /* (108) onconf ::= */ yytestcase(yyruleno==108);
      /* (109) onconf ::= ON CONFLICT resolvetype */ yytestcase(yyruleno==109);
      /* (110) orconf ::= */ yytestcase(yyruleno==110);
      /* (111) orconf ::= OR resolvetype */ yytestcase(yyruleno==111);
      /* (112) resolvetype ::= IGNORE */ yytestcase(yyruleno==112);
      /* (113) resolvetype ::= REPLACE */ yytestcase(yyruleno==113);
      /* (114) cmd ::= DROP TABLE ifexists fullname */ yytestcase(yyruleno==114);
      /* (115) ifexists ::= IF EXISTS */ yytestcase(yyruleno==115);
      /* (116) ifexists ::= */ yytestcase(yyruleno==116);
      /* (117) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */ yytestcase(yyruleno==117);
      /* (118) cmd ::= DROP VIEW ifexists fullname */ yytestcase(yyruleno==118);
      /* (119) select ::= WITH wqlist selectnowith */ yytestcase(yyruleno==119);
      /* (120) select ::= WITH RECURSIVE wqlist selectnowith */ yytestcase(yyruleno==120);
      /* (121) selectnowith ::= selectnowith multiselect_op oneselect */ yytestcase(yyruleno==121);
      /* (122) multiselect_op ::= UNION */ yytestcase(yyruleno==122);
      /* (123) multiselect_op ::= UNION ALL */ yytestcase(yyruleno==123);
      /* (124) multiselect_op ::= EXCEPT|INTERSECT */ yytestcase(yyruleno==124);
      /* (125) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */ yytestcase(yyruleno==125);
      /* (126) values ::= VALUES LP nexprlist RP */ yytestcase(yyruleno==126);
      /* (127) oneselect ::= mvalues */ yytestcase(yyruleno==127);
      /* (128) mvalues ::= values COMMA LP nexprlist RP */ yytestcase(yyruleno==128);
      /* (129) mvalues ::= mvalues COMMA LP nexprlist RP */ yytestcase(yyruleno==129);
      /* (130) selcollist ::= sclp scanpt nm DOT STAR */ yytestcase(yyruleno==130);
      /* (131) stl_prefix ::= seltablist joinop */ yytestcase(yyruleno==131);
      /* (132) stl_prefix ::= */ yytestcase(yyruleno==132);
      /* (133) seltablist ::= stl_prefix nm dbnm as on_using */ yytestcase(yyruleno==133);
      /* (134) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */ yytestcase(yyruleno==134);
      /* (135) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */ yytestcase(yyruleno==135);
      /* (136) seltablist ::= stl_prefix LP select RP as on_using */ yytestcase(yyruleno==136);
      /* (137) seltablist ::= stl_prefix LP seltablist RP as on_using */ yytestcase(yyruleno==137);
      /* (138) dbnm ::= */ yytestcase(yyruleno==138);
      /* (139) dbnm ::= DOT nm */ yytestcase(yyruleno==139);
      /* (140) fullname ::= nm */ yytestcase(yyruleno==140);
      /* (141) fullname ::= nm DOT nm */ yytestcase(yyruleno==141);
      /* (142) xfullname ::= nm */ yytestcase(yyruleno==142);
      /* (143) xfullname ::= nm DOT nm */ yytestcase(yyruleno==143);
      /* (144) xfullname ::= nm DOT nm AS nm */ yytestcase(yyruleno==144);
      /* (145) xfullname ::= nm AS nm */ yytestcase(yyruleno==145);
      /* (146) joinop ::= COMMA|JOIN */ yytestcase(yyruleno==146);
      /* (147) joinop ::= JOIN_KW JOIN */ yytestcase(yyruleno==147);
      /* (148) joinop ::= JOIN_KW nm JOIN */ yytestcase(yyruleno==148);
      /* (149) joinop ::= JOIN_KW nm nm JOIN */ yytestcase(yyruleno==149);
      /* (150) on_using ::= ON expr */ yytestcase(yyruleno==150);
      /* (151) on_using ::= USING LP idlist RP */ yytestcase(yyruleno==151);
      /* (152) on_using ::= */ yytestcase(yyruleno==152);
      /* (153) indexed_opt ::= */ yytestcase(yyruleno==153);
      /* (154) indexed_by ::= INDEXED BY nm */ yytestcase(yyruleno==154);
      /* (155) indexed_by ::= NOT INDEXED */ yytestcase(yyruleno==155);
      /* (156) orderby_opt ::= ORDER BY sortlist */ yytestcase(yyruleno==156);
      /* (157) sortlist ::= sortlist COMMA expr sortorder nulls */ yytestcase(yyruleno==157);
      /* (158) sortlist ::= expr sortorder nulls */ yytestcase(yyruleno==158);
      /* (159) sortorder ::= ASC */ yytestcase(yyruleno==159);
      /* (160) sortorder ::= DESC */ yytestcase(yyruleno==160);
      /* (161) sortorder ::= */ yytestcase(yyruleno==161);
      /* (162) nulls ::= NULLS FIRST */ yytestcase(yyruleno==162);
      /* (163) nulls ::= NULLS LAST */ yytestcase(yyruleno==163);
      /* (164) nulls ::= */ yytestcase(yyruleno==164);
      /* (165) groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==165);
      /* (166) having_opt ::= HAVING expr */ yytestcase(yyruleno==166);
      /* (167) limit_opt ::= LIMIT expr */ yytestcase(yyruleno==167);
      /* (168) limit_opt ::= LIMIT expr OFFSET expr */ yytestcase(yyruleno==168);
      /* (169) limit_opt ::= LIMIT expr COMMA expr */ yytestcase(yyruleno==169);
      /* (170) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */ yytestcase(yyruleno==170);
      /* (171) where_opt_ret ::= */ yytestcase(yyruleno==171);
      /* (172) where_opt_ret ::= WHERE expr */ yytestcase(yyruleno==172);
      /* (173) where_opt_ret ::= RETURNING selcollist */ yytestcase(yyruleno==173);
      /* (174) where_opt_ret ::= WHERE expr RETURNING selcollist */ yytestcase(yyruleno==174);
      /* (175) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */ yytestcase(yyruleno==175);
      /* (176) setlist ::= setlist COMMA nm EQ expr */ yytestcase(yyruleno==176);
      /* (177) setlist ::= setlist COMMA LP idlist RP EQ expr */ yytestcase(yyruleno==177);
      /* (178) setlist ::= nm EQ expr */ yytestcase(yyruleno==178);
      /* (179) setlist ::= LP idlist RP EQ expr */ yytestcase(yyruleno==179);
      /* (180) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */ yytestcase(yyruleno==180);
      /* (181) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */ yytestcase(yyruleno==181);
      /* (182) upsert ::= */ yytestcase(yyruleno==182);
      /* (183) upsert ::= RETURNING selcollist */ yytestcase(yyruleno==183);
      /* (184) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */ yytestcase(yyruleno==184);
      /* (185) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */ yytestcase(yyruleno==185);
      /* (186) upsert ::= ON CONFLICT DO NOTHING returning */ yytestcase(yyruleno==186);
      /* (187) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */ yytestcase(yyruleno==187);
      /* (188) returning ::= RETURNING selcollist */ yytestcase(yyruleno==188);
      /* (189) insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==189);
      /* (190) insert_cmd ::= REPLACE */ yytestcase(yyruleno==190);
      /* (191) idlist_opt ::= */ yytestcase(yyruleno==191);
      /* (192) idlist_opt ::= LP idlist RP */ yytestcase(yyruleno==192);
      /* (193) idlist ::= idlist COMMA nm */ yytestcase(yyruleno==193);
      /* (194) idlist ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=194);
      /* (195) expr ::= ID|INDEXED|JOIN_KW */ yytestcase(yyruleno==195);
      /* (196) expr ::= nm DOT nm */ yytestcase(yyruleno==196);
      /* (197) expr ::= nm DOT nm DOT nm */ yytestcase(yyruleno==197);
      /* (198) expr ::= VARIABLE */ yytestcase(yyruleno==198);
      /* (199) expr ::= expr COLLATE ID|STRING */ yytestcase(yyruleno==199);
      /* (200) expr ::= CAST LP expr AS typetoken RP */ yytestcase(yyruleno==200);
      /* (201) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */ yytestcase(yyruleno==201);
      /* (202) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */ yytestcase(yyruleno==202);
      /* (203) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */ yytestcase(yyruleno==203);
      /* (204) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */ yytestcase(yyruleno==204);
      /* (205) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */ yytestcase(yyruleno==205);
      /* (206) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */ yytestcase(yyruleno==206);
      /* (207) term ::= CTIME_KW */ yytestcase(yyruleno==207);
      /* (208) expr ::= LP nexprlist COMMA expr RP */ yytestcase(yyruleno==208);
      /* (209) expr ::= expr AND expr */ yytestcase(yyruleno==209);
      /* (210) expr ::= expr OR expr */ yytestcase(yyruleno==210);
      /* (211) expr ::= expr EQ|NE expr */ yytestcase(yyruleno==211);
      /* (212) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */ yytestcase(yyruleno==212);
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
