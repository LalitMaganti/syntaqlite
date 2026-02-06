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
#define YYNOCODE 322
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 113
#define syntaqlite_sqlite3ParserTOKENTYPE SyntaqliteToken
typedef union {
  int yyinit;
  syntaqlite_sqlite3ParserTOKENTYPE yy0;
  int yy144;
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
#define YYNRULE_WITH_ACTION  273
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
 /*     0 */   522,  212, 1644, 1650,  574,  130,    2,  161,  234, 1577,
 /*    10 */  1644, 1556,  465, 1259,  295,  283, 1509,  344, 1577, 1290,
 /*    20 */   212,  497,  299,  290, 1509,  399,  565,  574,  571,  850,
 /*    30 */   521,  851,  574,  852,  565, 1539,  571,  413,  264,  230,
 /*    40 */   462,  574,  290, 1509,  398,  290, 1509,  501,  409,  931,
 /*    50 */  1515,   51,  183,  565, 1534,  571,  565,  932,  571,  326,
 /*    60 */   293,  137,  139,   91,  254, 1104, 1104, 1008, 1011,  998,
 /*    70 */   998,  290, 1509, 1515,   51,  413, 1515,   22, 1515,   84,
 /*    80 */  1052,  462,  565,  462,  571, 1617,    7, 1515,   84,  466,
 /*    90 */   995,  995, 1009, 1012, 1542,  413,  127, 1534,  212,  137,
 /*   100 */   139,   91, 1053, 1104, 1104, 1008, 1011,  998,  998,  556,
 /*   110 */  1492, 1468, 1660,  402,  452,  523, 1451, 1054,  528,  137,
 /*   120 */   139,   91,  407, 1104, 1104, 1008, 1011,  998,  998,  452,
 /*   130 */   376,  376,  871, 1209,  452,  293, 1214, 1209, 1214,  140,
 /*   140 */   284,  130,  872,  452,  234,  999, 1209,  183,  543, 1209,
 /*   150 */  1209,  377,  452, 1209,  377,  135,  135,  136,  136,  136,
 /*   160 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   170 */   131,  128,  452,  530,  530,    7,  134,  134,  134,  134,
 /*   180 */   133,  133,  132,  132,  132,  131,  128,  132,  132,  132,
 /*   190 */   131,  128,  542,  135,  135,  136,  136,  136,  136,  134,
 /*   200 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   210 */   453,  967,    6,  135,  135,  136,  136,  136,  136,  134,
 /*   220 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   230 */   524,  413,  127, 1520,  110, 1109, 1090, 1090,  241, 1111,
 /*   240 */  1541, 1090, 1090, 1090, 1090,  236,   44, 1110,  461,  458,
 /*   250 */   457,  413,  359,  485,  347,  137,  139,   91,  456, 1104,
 /*   260 */  1104, 1008, 1011,  998,  998, 1519,  477,  478,  540,  540,
 /*   270 */     7,  573,  479,   97,  100,  137,  139,   91,  574, 1104,
 /*   280 */  1104, 1008, 1011,  998,  998,  447,  446, 1487,  417,  413,
 /*   290 */   321,  575,  384, 1090,  375,  237,  372, 1091, 1090, 1112,
 /*   300 */  1090, 1112, 1091,  330, 1091, 1178, 1178,  513,  452,  525,
 /*   310 */   233,  413,  539,  137,  139,   91,  506, 1104, 1104, 1008,
 /*   320 */  1011,  998,  998, 1093, 1515,   19, 1078, 1093,  452,  490,
 /*   330 */   361,  447,  446, 1090, 1090,  137,  139,   91,  421, 1104,
 /*   340 */  1104, 1008, 1011,  998,  998, 1112,  452, 1112,   44,  135,
 /*   350 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   360 */   133,  132,  132,  132,  131,  128,  452, 1090, 1090,  135,
 /*   370 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   380 */   133,  132,  132,  132,  131,  128,  321,  575,  452,  323,
 /*   390 */  1090,  198,  321,  575, 1091,  373,  985,  133,  133,  132,
 /*   400 */   132,  132,  131,  128,  976, 1489,  158,  135,  135,  136,
 /*   410 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*   420 */   132,  132,  131,  128, 1090,  157,  413,  467, 1091,  135,
 /*   430 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   440 */   133,  132,  132,  132,  131,  128,  232,  413,  574, 1135,
 /*   450 */   137,  139,   91,   98, 1104, 1104, 1008, 1011,  998,  998,
 /*   460 */  1052,  977, 1584,  976,  976,  978, 1532,  240,  239,  238,
 /*   470 */   917,  137,  139,   91,  574, 1104, 1104, 1008, 1011,  998,
 /*   480 */   998,  527, 1053,  236,  452,  413,  461,  458,  457,  290,
 /*   490 */  1509,  321,  575,   45, 1515,   19,  456, 1054,  380, 1584,
 /*   500 */   565,  231,  571,  452,  286,  917, 1615,    7,  294,  137,
 /*   510 */   139,   91,  561, 1104, 1104, 1008, 1011,  998,  998,  316,
 /*   520 */  1515,   51,  516,  267,  452, 1090, 1090,  136,  136,  136,
 /*   530 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   540 */   131,  128,  574,  551,  135,  135,  136,  136,  136,  136,
 /*   550 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   560 */   128, 1520,  452,  370, 1128,  135,  135,  136,  136,  136,
 /*   570 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   580 */   131,  128, 1090, 1090, 1090,  427, 1091, 1316, 1515,   19,
 /*   590 */  1090, 1090,  574, 1519, 1458, 1456,  219,  413,  547,  574,
 /*   600 */   370,  547,  432,  135,  135,  136,  136,  136,  136,  134,
 /*   610 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   620 */   491,  137,  139,   91,  420, 1104, 1104, 1008, 1011,  998,
 /*   630 */   998,  574,  474, 1638,  413,  568, 1506, 1507, 1515,   84,
 /*   640 */  1090,  199,  356,  516, 1091, 1515,   84, 1090,  967,  429,
 /*   650 */   129, 1091,  574, 1100,   10,  413,   44, 1100,  137,  139,
 /*   660 */    91,  440, 1104, 1104, 1008, 1011,  998,  998,  308,  556,
 /*   670 */   448, 1506, 1507,  557,  452,  555,  556, 1515,   84,  137,
 /*   680 */   139,   91,  444, 1104, 1104, 1008, 1011,  998,  998,  359,
 /*   690 */   382,  290, 1509,  452,  406,  405,  290, 1509, 1515,   19,
 /*   700 */   321,  575,  565,   95,  571,  532, 1467,  565,  556,  571,
 /*   710 */   232,  452,  437,    6,  558,  135,  135,  136,  136,  136,
 /*   720 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   730 */   131,  128,  452,  346,  917,  986,  136,  136,  136,  136,
 /*   740 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   750 */   128,  452,  135,  135,  136,  136,  136,  136,  134,  134,
 /*   760 */   134,  134,  133,  133,  132,  132,  132,  131,  128,  917,
 /*   770 */   413,  429,  972,  135,  135,  136,  136,  136,  136,  134,
 /*   780 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   790 */   413,  574,  322, 1227,  137,  139,   91,  574, 1104, 1104,
 /*   800 */  1008, 1011,  998,  998,  419,  510,  518,  131,  128, 1090,
 /*   810 */  1090, 1573,  209, 1583,  137,  139,   91,  489, 1104, 1104,
 /*   820 */  1008, 1011,  998,  998,  533,  533, 1090, 1090,  413,  290,
 /*   830 */  1509, 1616,    7,  576,  919,  885,  885, 1515,   19,  534,
 /*   840 */   565,  429,  571, 1515,   60,  559,  349,  452, 1288,  380,
 /*   850 */  1583,  442,  137,  139,   91,  213, 1104, 1104, 1008, 1011,
 /*   860 */   998,  998,  290, 1509, 1614,    7, 1090,  452,  290, 1509,
 /*   870 */  1091,  290, 1509,  565,  535,  571,  351,  897,  507,  565,
 /*   880 */  1196,  571,  565, 1090,  571,  574,  116, 1091,  135,  135,
 /*   890 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   900 */   132,  132,  132,  131,  128,  452,  108,  524,  135,  135,
 /*   910 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   920 */   132,  132,  132,  131,  128,  214,  130,  130, 1051,  234,
 /*   930 */   234, 1515,  145, 1535,  348,  228,  350,  574, 1582,  413,
 /*   940 */  1196, 1198, 1658, 1658,  120,  215,  135,  135,  136,  136,
 /*   950 */   136,  136,  134,  134,  134,  134,  133,  133,  132,  132,
 /*   960 */   132,  131,  128,  137,  139,   91,  413, 1104, 1104, 1008,
 /*   970 */  1011,  998,  998,  378,  380, 1582,  505, 1090, 1090, 1557,
 /*   980 */  1090, 1090,  495, 1515,   84,  107,  525,   46, 1090, 1090,
 /*   990 */   137,  139,   91, 1209, 1104, 1104, 1008, 1011,  998,  998,
 /*  1000 */   413, 1198, 1659, 1659,  552,  918, 1209, 1513,  574, 1209,
 /*  1010 */   130,  481,  526,  234,  317,  221,  452,  127,  127, 1090,
 /*  1020 */  1090, 1090, 1090, 1175,  137,  126,   91, 1175, 1104, 1104,
 /*  1030 */  1008, 1011,  998,  998, 1090,    5,  532, 1090, 1091,  291,
 /*  1040 */  1509, 1091,  267,  452, 1560, 1090,   42, 1513,  541, 1091,
 /*  1050 */   565,  302,  571,  511, 1515,  147,  379,  135,  135,  136,
 /*  1060 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*  1070 */   132,  132,  131,  128, 1508,  516, 1090,  452, 1090, 1580,
 /*  1080 */  1091,  413, 1091,  303,  135,  135,  136,  136,  136,  136,
 /*  1090 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*  1100 */   128,  127,  499, 1559, 1090, 1090,  139,   91,  574, 1104,
 /*  1110 */  1104, 1008, 1011,  998,  998,  413, 1142,  574,  135,  135,
 /*  1120 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*  1130 */   132,  132,  132,  131,  128, 1383,  574,  241, 1450,  574,
 /*  1140 */   499,   91,  574, 1104, 1104, 1008, 1011,  998,  998, 1078,
 /*  1150 */  1173,  574,  549,  505, 1515,   19, 1611, 1487,  452,  482,
 /*  1160 */  1487, 1090,    3, 1515,   53, 1091,  372,  113,  207,  372,
 /*  1170 */   412,  548, 1100,  331,  867,  160, 1100, 1209,  867,  124,
 /*  1180 */  1220,  566, 1515,   82, 1078, 1515,   84,  417, 1515,   54,
 /*  1190 */  1209,  301,  452, 1209,  505, 1538,  569, 1515,   84,  135,
 /*  1200 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*  1210 */   133,  132,  132,  132,  131,  128,  318,  574, 1220,  985,
 /*  1220 */   434,   48,  574, 1131,   50,  562,  574,  976,  445,  304,
 /*  1230 */  1275,  418,  383,  135,  135,  136,  136,  136,  136,  134,
 /*  1240 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*  1250 */   124,  574,  566,  574,  985, 1078,  490,  361,  450,  450,
 /*  1260 */   358,  892,  976, 1515,   19,  113,  891,  569, 1515,   19,
 /*  1270 */  1443, 1383, 1515,   84,  124, 1488,  566,  208, 1488, 1078,
 /*  1280 */  1131, 1487,  550, 1487,  977,  574,  976,  976,  978,  574,
 /*  1290 */   372,  569,  372,   34,  536,  574,  562, 1515,   84, 1515,
 /*  1300 */    55,  310,  392,  449,    9,  247,   44,  122,  122,  896,
 /*  1310 */   861,  499, 1508, 1151,  123,  450,    4, 1282,  499,  572,
 /*  1320 */   562,  976,  976,  978,  979,  985,  483,  574,  451,  450,
 /*  1330 */   450, 1515,   56,  976,  545, 1515,   66,  412,  470,  544,
 /*  1340 */  1383, 1515,   67, 1613,    7,   47, 1152,   49, 1116,  985,
 /*  1350 */   321,  575,  426,  450,  450,  492,  435,  976,  327,  574,
 /*  1360 */   368,  488,  390, 1383,   34,  516,  412,  124,  516,  566,
 /*  1370 */  1383, 1294, 1078, 1515,   21,  422,  185,  574,  122,  122,
 /*  1380 */  1157,  565, 1158,  571,  569,  123,  450,    4,   34,  424,
 /*  1390 */   572, 1030,  976,  976,  978,  979, 1251,  574,  336, 1488,
 /*  1400 */   307, 1488,  122,  122,   14, 1515,   57,  309,  335,  123,
 /*  1410 */   450,    4,  503,  562,  572,  574,  976,  976,  978,  979,
 /*  1420 */   574, 1478,  469, 1515,   68,  436,  574,  545, 1289,  423,
 /*  1430 */   516, 1285,  546,  243,  280,  329,  472,  332,  471,  242,
 /*  1440 */   480,  574,  985, 1515,   58,  330,  450,  450,  512,  496,
 /*  1450 */   976,  574,  567,  418,  124,  441,  566,  401, 1250, 1078,
 /*  1460 */   412, 1515,   69,  222,  288,  230, 1515,   70,  298,    2,
 /*  1470 */   161,  569, 1515,   71,  574,  465, 1259,  295,  574,  320,
 /*  1480 */  1480,   34, 1290, 1251,  574, 1649, 1160, 1515,   72,  338,
 /*  1490 */   412,  504,  574,  366,  574,  122,  122, 1515,   73,  228,
 /*  1500 */   562,  574,  123,  450,    4,  290, 1509,  572,  574,  976,
 /*  1510 */   976,  978,  979,  574,  545,  574,  565,  551,  571,  544,
 /*  1520 */  1515,   74,  107,  574, 1515,   75,  475,  254,  931,  985,
 /*  1530 */  1515,   76,  251,  450,  450,  155,  932,  976, 1515,   77,
 /*  1540 */  1515,   59,  101,  218,  401, 1197,  892, 1515,   61,  526,
 /*  1550 */   138,  891,  466, 1138, 1515,   20,  121, 1138,  118, 1515,
 /*  1560 */   143, 1515,  144,  493,  222, 1558, 1150, 1149,   34, 1515,
 /*  1570 */    79, 1255,    1, 1493,   39,  574, 1037,  574, 1314,  574,
 /*  1580 */  1037,  574,  122,  122, 1491,  305,  476,  582,  430,  123,
 /*  1590 */   450,    4,   38,  292,  572, 1476,  976,  976,  978,  979,
 /*  1600 */  1099,  574,  339,  396,  537,  396,  520,  395,  161,  277,
 /*  1610 */   340,  393,  574,  464, 1259,  295,  343,  858,  454,  263,
 /*  1620 */  1290, 1515,   62, 1515,   80, 1515,   63, 1515,   81,  574,
 /*  1630 */   324,  113,  248,  574,  342,  124,  166,  566,  574, 1049,
 /*  1640 */  1078, 1049,  341,  290, 1509, 1193,  403, 1515,   64,  574,
 /*  1650 */   357,  113,  569, 1328,  565,   90,  571,  566, 1515,  170,
 /*  1660 */  1078,  574,  969,  266, 1327,  254,  250,  253,  498,  266,
 /*  1670 */   934,  935,  569,  484,  177, 1515,  171,   43, 1072, 1515,
 /*  1680 */    88,  562,  574,  289, 1515,   65,  574, 1502, 1606,   25,
 /*  1690 */   466,  500,  266,  574,  249, 1515,  146,  574,  352,  574,
 /*  1700 */  1048,  562, 1048,  560,  574, 1598,  574, 1515,   83, 1597,
 /*  1710 */   985,  362,  113,  574,  450,  450,  414,  508,  976,  574,
 /*  1720 */   363,  321,  575, 1033,  263,  922,  266,  980, 1515,  168,
 /*  1730 */   985,  980, 1515,  148,  450,  450, 1324,  554,  976, 1515,
 /*  1740 */   142,  205,  574, 1515,  169, 1515,  162,  165,  113,   34,
 /*  1750 */  1515,  152, 1515,  151,  367,  371,  124,  553,  566, 1515,
 /*  1760 */   149, 1078,  224,  122,  122, 1515,  150, 1082, 1337,   34,
 /*  1770 */   123,  450,    4,  569,  574,  572, 1382,  976,  976,  978,
 /*  1780 */   979,  574, 1310,  122,  122,  281,  476,  582, 1515,   86,
 /*  1790 */   123,  450,    4,  292, 1387,  572, 1629,  976,  976,  978,
 /*  1800 */   979,  574,  562,  396,  389,  396, 1322,  395,  574,  277,
 /*  1810 */   391,  393,  889,  138,  865,  159, 1268,  858,  890,  138,
 /*  1820 */  1515,   78,  887,  138,  517, 1267,  167, 1515,   87, 1269,
 /*  1830 */    12,  985,  248, 1499,  342,  450,  450, 1307,  313,  976,
 /*  1840 */   314,  315,  341,  397,  227,  459, 1556, 1515,   85, 1640,
 /*  1850 */   246,  334, 1369, 1364, 1515,   52,  300, 1357,  354,  355,
 /*  1860 */   306, 1374,  360, 1373,  410,  502,  250, 1447, 1446,  203,
 /*  1870 */    34,  473,   40,  210,  177,  563,  211,   43, 1319, 1320,
 /*  1880 */  1318, 1317,  387,  400,  122,  122, 1576,  223,  476, 1575,
 /*  1890 */  1574,  123,  450,    4,  249,  292,  572,  463,  976,  976,
 /*  1900 */   978,  979,   93, 1648, 1647,  396, 1646,  396,  425,  395,
 /*  1910 */  1567,  277,  404,  393, 1546, 1142,  414, 1545,  333,  858,
 /*  1920 */  1566,  321,  575, 1544, 1116,  187,  244,  274, 1475, 1473,
 /*  1930 */  1113,  428,   96,   99,  248,  100,  342,  220, 1370,  196,
 /*  1940 */   182,   13, 1452,  486,  341,  189,  487,  191,  579,  192,
 /*  1950 */   193,  194,  108, 1378,  408,  494, 1376, 1375,  256,   15,
 /*  1960 */   411,  200, 1441,  509,  260,  106,  515, 1082,  250,  282,
 /*  1970 */   438,  262,  519, 1586,  439,  365,  177,  311,  312,   43,
 /*  1980 */   141, 1581,  268,  551,  374,  369,  269, 1620,  443,  233,
 /*  1990 */  1634,   11, 1428,  381, 1342, 1341,  249, 1619,  117,  109,
 /*  2000 */  1298,  529,  577,  388,  273, 1530,  385,  319,  275, 1088,
 /*  2010 */   216,  386,  276,  278,  279,  580, 1265, 1260,  414,  225,
 /*  2020 */   154,  296, 1602,  321,  575,  172,  226,  173, 1603,  415,
 /*  2030 */   416,  186,  174, 1601, 1600,  335,   89,  184,  235,  455,
 /*  2040 */    92,   23,  460,  325,   24,  175,  153, 1084, 1083,  217,
 /*  2050 */  1140,  328,  468, 1153,   94,  297,  176,  337,  245,  156,
 /*  2060 */  1047,  345, 1045,  178,  188, 1099,  252,  255,  190,  874,
 /*  2070 */   353, 1184,  195,  431,  179,  180,  433,  102,  197,  181,
 /*  2080 */   103, 1187,  104,  257,  258, 1183,  105,  163,  259,  364,
 /*  2090 */    26, 1176,  201,  266,  514,  202, 1243,  261, 1073,  926,
 /*  2100 */   229,  265,  920,   27,  531,  112, 1074,   16,   28,   29,
 /*  2110 */   285,  287,  204,  138, 1200,   41,  206, 1014,   30,  111,
 /*  2120 */    31, 1215,  164,  538, 1211, 1213, 1225,    8, 1219,  113,
 /*  2130 */  1218,   32,   33,  114,  115,  119, 1018, 1028,  884, 1015,
 /*  2140 */    18, 1013, 1017, 1050,  270,   35,  860, 1503,  564,   17,
 /*  2150 */   570,  578,  981,  866,  394,  846,  125,  271,  581,   36,
 /*  2160 */  1256,   37, 1256, 1256,  272,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */   229,  187,  232,  246,  187,  190,  216,  217,  193,  240,
 /*    10 */   240,  242,  222,  223,  224,  252,  253,  187,  249,  229,
 /*    20 */   187,  202,  229,  252,  253,  211,  263,  187,  265,   13,
 /*    30 */   187,   15,  187,   17,  263,  233,  265,   37,  267,  268,
 /*    40 */   187,  187,  252,  253,  211,  252,  253,  187,  229,   49,
 /*    50 */   233,  234,  187,  263,  187,  265,  263,   57,  265,  226,
 /*    60 */   246,   61,   62,   63,  274,   65,   66,   67,   68,   69,
 /*    70 */    70,  252,  253,  233,  234,   37,  233,  234,  233,  234,
 /*    80 */    23,  228,  263,  230,  265,  309,  310,  233,  234,  299,
 /*    90 */    65,   66,   67,   68,  233,   37,  281,  230,  187,   61,
 /*   100 */    62,   63,   45,   65,   66,   67,   68,   69,   70,  264,
 /*   110 */   320,  296,  302,  303,  114,  270,  286,   60,  264,   61,
 /*   120 */    62,   63,  211,   65,   66,   67,   68,   69,   70,  114,
 /*   130 */   313,  314,   75,   88,  114,  321,   98,   88,  100,   81,
 /*   140 */   189,  190,   85,  114,  193,  120,  101,  187,   99,  104,
 /*   150 */   101,  311,  114,  104,  314,  155,  156,  157,  158,  159,
 /*   160 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   170 */   170,  171,  114,  308,  309,  310,  161,  162,  163,  164,
 /*   180 */   165,  166,  167,  168,  169,  170,  171,  167,  168,  169,
 /*   190 */   170,  171,  143,  155,  156,  157,  158,  159,  160,  161,
 /*   200 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   210 */    37,   85,  189,  155,  156,  157,  158,  159,  160,  161,
 /*   220 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   230 */    37,   37,  281,  187,  176,  115,  116,  117,   65,  119,
 /*   240 */   233,  116,  117,  116,  117,  118,   93,  127,  121,  122,
 /*   250 */   123,   37,  126,  127,  128,   61,   62,   63,  131,   65,
 /*   260 */    66,   67,   68,   69,   70,  219,  220,  221,  308,  309,
 /*   270 */   310,  187,  119,   79,  149,   61,   62,   63,  187,   65,
 /*   280 */    66,   67,   68,   69,   70,  165,  166,  187,  115,   37,
 /*   290 */   137,  138,  260,  173,  262,   44,  196,  177,  173,  179,
 /*   300 */   173,  181,  177,  130,  177,  126,  127,  128,  114,  116,
 /*   310 */     8,   37,   10,   61,   62,   63,  293,   65,   66,   67,
 /*   320 */    68,   69,   70,  173,  233,  234,   42,  177,  114,  127,
 /*   330 */   128,  165,  166,  116,  117,   61,   62,   63,  247,   65,
 /*   340 */    66,   67,   68,   69,   70,  179,  114,  181,   93,  155,
 /*   350 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   360 */   166,  167,  168,  169,  170,  171,  114,  116,  117,  155,
 /*   370 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   380 */   166,  167,  168,  169,  170,  171,  137,  138,  114,  175,
 /*   390 */   173,  174,  137,  138,  177,  187,  112,  165,  166,  167,
 /*   400 */   168,  169,  170,  171,  120,  305,    6,  155,  156,  157,
 /*   410 */   158,  159,  160,  161,  162,  163,  164,  165,  166,  167,
 /*   420 */   168,  169,  170,  171,  173,  176,   37,  175,  177,  155,
 /*   430 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   440 */   166,  167,  168,  169,  170,  171,  117,   37,  187,  175,
 /*   450 */    61,   62,   63,   43,   65,   66,   67,   68,   69,   70,
 /*   460 */    23,  177,  280,  179,  180,  181,  229,  126,  127,  128,
 /*   470 */   141,   61,   62,   63,  187,   65,   66,   67,   68,   69,
 /*   480 */    70,  187,   45,  118,  114,   37,  121,  122,  123,  252,
 /*   490 */   253,  137,  138,   84,  233,  234,  131,   60,  316,  317,
 /*   500 */   263,  187,  265,  114,  175,  176,  309,  310,  247,   61,
 /*   510 */    62,   63,   75,   65,   66,   67,   68,   69,   70,  201,
 /*   520 */   233,  234,  187,   43,  114,  116,  117,  157,  158,  159,
 /*   530 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   540 */   170,  171,  187,  143,  155,  156,  157,  158,  159,  160,
 /*   550 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*   560 */   171,  187,  114,  187,  175,  155,  156,  157,  158,  159,
 /*   570 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   580 */   170,  171,  173,  116,  117,  250,  177,  269,  233,  234,
 /*   590 */   116,  117,  187,  219,  220,  221,  148,   37,  311,  187,
 /*   600 */   187,  314,  247,  155,  156,  157,  158,  159,  160,  161,
 /*   610 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   620 */   128,   61,   62,   63,  248,   65,   66,   67,   68,   69,
 /*   630 */    70,  187,  297,  187,   37,  218,  219,  220,  233,  234,
 /*   640 */   173,  174,  150,  187,  177,  233,  234,  173,   85,  187,
 /*   650 */    71,  177,  187,  173,  174,   37,   93,  177,   61,   62,
 /*   660 */    63,  248,   65,   66,   67,   68,   69,   70,  229,  264,
 /*   670 */   218,  219,  220,  229,  114,  270,  264,  233,  234,   61,
 /*   680 */    62,   63,  270,   65,   66,   67,   68,   69,   70,  126,
 /*   690 */   187,  252,  253,  114,  165,  166,  252,  253,  233,  234,
 /*   700 */   137,  138,  263,  174,  265,  187,  250,  263,  264,  265,
 /*   710 */   117,  114,  247,  189,  270,  155,  156,  157,  158,  159,
 /*   720 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   730 */   170,  171,  114,  271,  141,  175,  157,  158,  159,  160,
 /*   740 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*   750 */   171,  114,  155,  156,  157,  158,  159,  160,  161,  162,
 /*   760 */   163,  164,  165,  166,  167,  168,  169,  170,  171,  176,
 /*   770 */    37,  187,  175,  155,  156,  157,  158,  159,  160,  161,
 /*   780 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   790 */    37,  187,  187,  175,   61,   62,   63,  187,   65,   66,
 /*   800 */    67,   68,   69,   70,  213,  214,  229,  170,  171,  116,
 /*   810 */   117,  187,  288,  280,   61,   62,   63,  293,   65,   66,
 /*   820 */    67,   68,   69,   70,  306,  307,  116,  117,   37,  252,
 /*   830 */   253,  309,  310,  133,  141,  135,  136,  233,  234,  229,
 /*   840 */   263,  187,  265,  233,  234,  229,    1,  114,  229,  316,
 /*   850 */   317,  247,   61,   62,   63,  271,   65,   66,   67,   68,
 /*   860 */    69,   70,  252,  253,  309,  310,  173,  114,  252,  253,
 /*   870 */   177,  252,  253,  263,  264,  265,   31,  167,  287,  263,
 /*   880 */   113,  265,  263,  173,  265,  187,  153,  177,  155,  156,
 /*   890 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*   900 */   167,  168,  169,  170,  171,  114,  147,   37,  155,  156,
 /*   910 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*   920 */   167,  168,  169,  170,  171,  271,  190,  190,  175,  193,
 /*   930 */   193,  233,  234,  187,   89,  176,   91,  187,  280,   37,
 /*   940 */   113,  174,  175,  176,  153,   43,  155,  156,  157,  158,
 /*   950 */   159,  160,  161,  162,  163,  164,  165,  166,  167,  168,
 /*   960 */   169,  170,  171,   61,   62,   63,   37,   65,   66,   67,
 /*   970 */    68,   69,   70,  236,  316,  317,  187,  116,  117,  187,
 /*   980 */   116,  117,  115,  233,  234,  115,  116,   84,  116,  117,
 /*   990 */    61,   62,   63,   88,   65,   66,   67,   68,   69,   70,
 /*  1000 */    37,  174,  175,  176,   99,  141,  101,  201,  187,  104,
 /*  1010 */   190,  205,  142,  193,  264,  148,  114,  281,  281,  116,
 /*  1020 */   117,  116,  117,   47,   61,   62,   63,   51,   65,   66,
 /*  1030 */    67,   68,   69,   70,  173,  174,  187,  173,  177,  252,
 /*  1040 */   253,  177,   43,  114,  243,  173,  174,  201,  143,  177,
 /*  1050 */   263,  205,  265,   77,  233,  234,  236,  155,  156,  157,
 /*  1060 */   158,  159,  160,  161,  162,  163,  164,  165,  166,  167,
 /*  1070 */   168,  169,  170,  171,  199,  187,  173,  114,  173,  204,
 /*  1080 */   177,   37,  177,  294,  155,  156,  157,  158,  159,  160,
 /*  1090 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*  1100 */   171,  281,  187,  243,  116,  117,   62,   63,  187,   65,
 /*  1110 */    66,   67,   68,   69,   70,   37,  125,  187,  155,  156,
 /*  1120 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*  1130 */   167,  168,  169,  170,  171,  187,  187,   65,  250,  187,
 /*  1140 */   187,   63,  187,   65,   66,   67,   68,   69,   70,   42,
 /*  1150 */    21,  187,   78,  187,  233,  234,  307,  187,  114,  255,
 /*  1160 */   187,  173,  174,  233,  234,  177,  196,  176,  247,  196,
 /*  1170 */   266,   97,  173,  187,  173,  174,  177,   88,  177,   37,
 /*  1180 */   106,   39,  233,  234,   42,  233,  234,  115,  233,  234,
 /*  1190 */   101,  276,  114,  104,  187,  187,   54,  233,  234,  155,
 /*  1200 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*  1210 */   166,  167,  168,  169,  170,  171,  264,  187,  144,  112,
 /*  1220 */   272,  251,  187,  116,  251,   83,  187,  120,  264,  276,
 /*  1230 */   194,  195,  283,  155,  156,  157,  158,  159,  160,  161,
 /*  1240 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*  1250 */    37,  187,   39,  187,  112,   42,  127,  128,  116,  117,
 /*  1260 */   294,  134,  120,  233,  234,  176,  139,   54,  233,  234,
 /*  1270 */     1,  187,  233,  234,   37,  305,   39,  247,  305,   42,
 /*  1280 */   173,  187,  247,  187,  177,  187,  179,  180,  181,  187,
 /*  1290 */   196,   54,  196,  151,  167,  187,   83,  233,  234,  233,
 /*  1300 */   234,  294,   31,  264,   67,   29,   93,  165,  166,  167,
 /*  1310 */    41,  187,  199,   53,  172,  173,  174,  204,  187,  177,
 /*  1320 */    83,  179,  180,  181,  182,  112,  255,  187,  264,  116,
 /*  1330 */   117,  233,  234,  120,   97,  233,  234,  266,   78,  102,
 /*  1340 */   187,  233,  234,  309,  310,  251,   86,  251,   72,  112,
 /*  1350 */   137,  138,   37,  116,  117,  255,  272,  120,   43,  187,
 /*  1360 */    89,   92,   91,  187,  151,  187,  266,   37,  187,   39,
 /*  1370 */   187,  253,   42,  233,  234,  300,  301,  187,  165,  166,
 /*  1380 */    13,  263,   15,  265,   54,  172,  173,  174,  151,  129,
 /*  1390 */   177,  122,  179,  180,  181,  182,  113,  187,  120,  305,
 /*  1400 */   276,  305,  165,  166,   43,  233,  234,  276,  130,  172,
 /*  1410 */   173,  174,   37,   83,  177,  187,  179,  180,  181,  182,
 /*  1420 */   187,  187,  107,  233,  234,  272,  187,   97,  250,  114,
 /*  1430 */   187,  250,  102,  118,  119,  120,  121,  122,  123,  124,
 /*  1440 */   202,  187,  112,  233,  234,  130,  116,  117,  272,  255,
 /*  1450 */   120,  187,  194,  195,   37,  272,   39,  174,  175,   42,
 /*  1460 */   266,  233,  234,  140,  267,  268,  233,  234,  111,  216,
 /*  1470 */   217,   54,  233,  234,  187,  222,  223,  224,  187,  255,
 /*  1480 */   187,  151,  229,  113,  187,  175,  176,  233,  234,  132,
 /*  1490 */   266,  116,  187,  250,  187,  165,  166,  233,  234,  176,
 /*  1500 */    83,  187,  172,  173,  174,  252,  253,  177,  187,  179,
 /*  1510 */   180,  181,  182,  187,   97,  187,  263,  143,  265,  102,
 /*  1520 */   233,  234,  115,  187,  233,  234,  187,  274,   49,  112,
 /*  1530 */   233,  234,   43,  116,  117,  174,   57,  120,  233,  234,
 /*  1540 */   233,  234,  147,  148,  174,  175,  134,  233,  234,  142,
 /*  1550 */   176,  139,  299,  173,  233,  234,  152,  177,  154,  233,
 /*  1560 */   234,  233,  234,  128,  140,  243,  119,  120,  151,  233,
 /*  1570 */   234,  318,  319,  320,  156,  187,  173,  187,  200,  187,
 /*  1580 */   177,  187,  165,  166,    0,  150,    2,    3,   73,  172,
 /*  1590 */   173,  174,  174,    9,  177,  187,  179,  180,  181,  182,
 /*  1600 */   176,  187,  187,   19,  143,   21,   37,   23,  217,   25,
 /*  1610 */   187,   27,  187,  222,  223,  224,  187,   33,  175,  176,
 /*  1620 */   229,  233,  234,  233,  234,  233,  234,  233,  234,  187,
 /*  1630 */   175,  176,   48,  187,   50,   37,  175,   39,  187,  179,
 /*  1640 */    42,  181,   58,  252,  253,  175,  176,  233,  234,  187,
 /*  1650 */   175,  176,   54,  187,  263,   37,  265,   39,  233,  234,
 /*  1660 */    42,  187,  175,  176,  187,  274,   82,  178,  175,  176,
 /*  1670 */    95,   96,   54,  187,   90,  233,  234,   93,  109,  233,
 /*  1680 */   234,   83,  187,  174,  233,  234,  187,  178,  187,  174,
 /*  1690 */   299,  175,  176,  187,  110,  233,  234,  187,  187,  187,
 /*  1700 */   179,   83,  181,  199,  187,  187,  187,  233,  234,  187,
 /*  1710 */   112,  175,  176,  187,  116,  117,  132,  187,  120,  187,
 /*  1720 */   187,  137,  138,  175,  176,  175,  176,  173,  233,  234,
 /*  1730 */   112,  177,  233,  234,  116,  117,  187,  139,  120,  233,
 /*  1740 */   234,  188,  187,  233,  234,  233,  234,  175,  176,  151,
 /*  1750 */   233,  234,  233,  234,  187,  187,   37,  139,   39,  233,
 /*  1760 */   234,   42,  231,  165,  166,  233,  234,  183,  187,  151,
 /*  1770 */   172,  173,  174,   54,  187,  177,  187,  179,  180,  181,
 /*  1780 */   182,  187,  187,  165,  166,  289,    2,    3,  233,  234,
 /*  1790 */   172,  173,  174,    9,  187,  177,  315,  179,  180,  181,
 /*  1800 */   182,  187,   83,   19,  187,   21,  187,   23,  187,   25,
 /*  1810 */   187,   27,  175,  176,  175,  176,  187,   33,  175,  176,
 /*  1820 */   233,  234,  175,  176,  290,  187,  192,  233,  234,  187,
 /*  1830 */   254,  112,   48,  187,   50,  116,  117,  188,  188,  120,
 /*  1840 */   188,  188,   58,  207,  245,  237,  242,  233,  234,  225,
 /*  1850 */   210,  236,  278,  275,  233,  234,  256,  275,  295,  257,
 /*  1860 */   257,  278,  256,  278,  278,  295,   82,  236,  236,  174,
 /*  1870 */   151,  115,  176,  260,   90,  285,  260,   93,  269,  269,
 /*  1880 */   269,  269,  256,  238,  165,  166,  235,  254,    2,  235,
 /*  1890 */   235,  172,  173,  174,  110,    9,  177,  227,  179,  180,
 /*  1900 */   181,  182,  174,  241,  241,   19,  235,   21,   76,   23,
 /*  1910 */   244,   25,  238,   27,  235,  125,  132,  237,  235,   33,
 /*  1920 */   244,  137,  138,  235,   72,  210,  210,  178,  191,  191,
 /*  1930 */    56,  191,  215,  215,   48,  149,   50,  148,  279,  174,
 /*  1940 */    61,  277,  286,   35,   58,  203,  191,  206,   35,  206,
 /*  1950 */   206,  206,  147,  203,  257,  257,  279,  279,  212,  277,
 /*  1960 */   257,  203,  257,  191,  212,  152,   74,  183,   82,  191,
 /*  1970 */    43,  212,  197,  292,  114,  291,   90,  198,  198,   93,
 /*  1980 */   146,  261,  191,  143,  260,  238,  103,  312,   94,    8,
 /*  1990 */   317,  174,  282,  191,  273,  273,  110,  312,  152,  145,
 /*  2000 */   261,  144,  238,  191,  212,  191,  258,  284,  212,   25,
 /*  2010 */   259,  257,  209,  209,   11,  208,  208,  208,  132,  231,
 /*  2020 */   239,  239,  189,  137,  138,  298,  231,  298,  189,  304,
 /*  2030 */   304,  301,  298,  189,  189,  130,  189,  174,  124,   79,
 /*  2040 */   174,  174,   79,  175,  174,  176,  175,  183,  183,  174,
 /*  2050 */   173,   43,  108,   46,  174,   79,   55,    4,   29,   31,
 /*  2060 */   175,  138,  175,  129,  149,  176,   43,  142,  140,   39,
 /*  2070 */    31,  183,  140,   73,  129,  129,   55,  156,  149,  129,
 /*  2080 */   156,  116,  156,   52,  178,  183,  156,    9,  115,    1,
 /*  2090 */   174,   80,   80,  176,   59,  115,   87,  178,  109,  116,
 /*  2100 */   178,   52,  141,   52,   43,  147,  175,  174,   52,   52,
 /*  2110 */   175,  175,  174,  176,  175,  174,  176,  175,   52,  176,
 /*  2120 */    52,   87,  175,  174,  100,   98,  175,   62,   87,  176,
 /*  2130 */   105,   52,   52,  140,  140,  176,   21,  175,  134,  175,
 /*  2140 */    43,  175,  175,  175,  174,  174,   39,  178,  176,  174,
 /*  2150 */   176,   37,  175,  175,   29,    7,  174,  178,    5,  174,
 /*  2160 */   322,  174,  322,  322,  178,  322,  322,  322,  322,  322,
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
#define YY_SHIFT_MAX      (2153)
static const unsigned short int yy_shift_ofst[] = {
 /*     0 */  1784, 1584, 1886, 1213, 1213,  255,  563, 1237, 1330, 1417,
 /*    10 */  1719, 1719, 1719,  153,  255,  255,  255,  255,  255,    0,
 /*    20 */     0,  194,  929, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*    30 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,  120,  120,
 /*    40 */   127,  905,  905,  409,  903,  474,  474,  249,  249,  249,
 /*    50 */   249,   38,   58,  214,  252,  274,  389,  410,  448,  560,
 /*    60 */   597,  618,  733,  753,  791,  902,  929,  929,  929,  929,
 /*    70 */   929,  929,  929,  929,  929,  929,  929,  929,  929,  929,
 /*    80 */   929,  929,  929,  963,  929,  929, 1044, 1078, 1078, 1142,
 /*    90 */  1598, 1618, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   100 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   110 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   120 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   130 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   140 */  1719, 1719,  579,  370,  370,  370,  370,  370,  370,  370,
 /*   150 */    15,  232,   20,  251,  173,  474,  474,  474,  474,  166,
 /*   160 */   166, 1269,  637,  202,  302,  302,  302,  354,   29,   29,
 /*   170 */  2165, 2165, 1315, 1315, 1315,  251,   57,  125,  217,  217,
 /*   180 */   217,  217,   57,   49,  474,  767,  827,  474,  474,  474,
 /*   190 */   474,  474,  474,  474,  474,  474,  474,  474,  474,  474,
 /*   200 */   474,  474,  474,  474,  474,  870,  474, 1089, 1089, 1129,
 /*   210 */    45,   45,  150,  193,  193,  150,  400, 2165, 2165, 2165,
 /*   220 */  2165, 2165, 2165, 2165, 1107,  284,  284,  365,  467,  710,
 /*   230 */   861,  693,  864,  872,  988,  474,  474,  474, 1260, 1260,
 /*   240 */  1260,  474,  474,  474,  474,  474,  474,  474,  474,  474,
 /*   250 */   474,  474,  474,  474,  126,  474,  474,  474,  474,  474,
 /*   260 */   474,  474,  474,  474,  329,  474,  474,  474,  480, 1074,
 /*   270 */   437,  474,  474,  474,  474,  474,  474,  474,  474,  474,
 /*   280 */   529,  179,  976,  700,  999,  999,  999,  999,  593, 1127,
 /*   290 */   700,  700,   16, 1310,  991, 1361, 1072, 1479, 1276,  867,
 /*   300 */  1395, 1323, 1395, 1375,  759,  867,  867,  759,  867, 1323,
 /*   310 */  1375, 1479, 1479, 1407, 1407, 1407, 1407, 1374, 1374, 1404,
 /*   320 */  1424, 1412, 1695, 1756, 1756, 1756, 1696, 1728, 1728, 1756,
 /*   330 */  1832, 1695, 1756, 1790, 1756, 1832, 1756, 1852, 1852, 1749,
 /*   340 */  1749, 1874, 1874, 1749, 1786, 1789, 1765, 1879, 1908, 1908,
 /*   350 */  1908, 1908, 1749, 1913, 1805, 1789, 1789, 1805, 1765, 1879,
 /*   360 */  1805, 1879, 1805, 1749, 1913, 1813, 1892, 1749, 1913, 1927,
 /*   370 */  1860, 1860, 1695, 1749, 1834, 1840, 1883, 1883, 1894, 1894,
 /*   380 */  1981, 1817, 1749, 1846, 1834, 1854, 1857, 1805, 1695, 1749,
 /*   390 */  1913, 1749, 1913, 1984, 1984, 2003, 2003, 2003, 2165, 2165,
 /*   400 */  2165, 2165, 2165, 2165, 2165, 2165, 2165, 2165, 2165, 2165,
 /*   410 */  2165, 2165, 2165,   25,  845, 1283, 1370,  341, 1001, 1271,
 /*   420 */  1443, 1455, 1470, 1380, 1447, 1367, 1278, 1357, 1418, 1489,
 /*   430 */   492, 1435, 1475, 1515, 1487, 1493, 1516, 1536, 1569, 1403,
 /*   440 */  1548, 1550, 1572, 1575, 1461, 1637, 1460, 1521, 1639, 1643,
 /*   450 */  1509, 1647, 1554, 1905, 1914, 1863, 1960, 1866, 1867, 1868,
 /*   460 */  1870, 1963, 1869, 1871, 1864, 1865, 1875, 1877, 2008, 1944,
 /*   470 */  2007, 1880, 1976, 2001, 2053, 2029, 2028, 1885, 1887, 1923,
 /*   480 */  1915, 1934, 1889, 1889, 2023, 1928, 2030, 1925, 2039, 1888,
 /*   490 */  1932, 1945, 1889, 1946, 2000, 2021, 1889, 1929, 1921, 1924,
 /*   500 */  1926, 1930, 1950, 1965, 2031, 1906, 1902, 2078, 1916, 1973,
 /*   510 */  2088, 2011, 1917, 2012, 2009, 2035, 1919, 1980, 1931, 1933,
 /*   520 */  1989, 1922, 1935, 1936, 1983, 2049, 1938, 1961, 1937, 2051,
 /*   530 */  1939, 1941, 2061, 1940, 1942, 1943, 1947, 2056, 1958, 1949,
 /*   540 */  1951, 2057, 2066, 2068, 2024, 2034, 2027, 2065, 2041, 2025,
 /*   550 */  1953, 2079, 2080, 1993, 1994, 1962, 1937, 1964, 1966, 1967,
 /*   560 */  1959, 1968, 1970, 2115, 1971, 1972, 1975, 1977, 1978, 1982,
 /*   570 */  1985, 1974, 1969, 1979, 1986, 1987, 2004, 2097, 2107, 2114,
 /*   580 */  2125, 2148, 2153,
};
#define YY_REDUCE_COUNT (412)
#define YY_REDUCE_MIN   (-243)
#define YY_REDUCE_MAX   (1847)
static const short yy_reduce_ofst[] = {
 /*     0 */  1253, -210, 1391,  444,  610, -229, -181, -160, -183,  287,
 /*    10 */  -155,  405,  412, -207,  237,  439,  577,  616,  619,  737,
 /*    20 */   820, -185,  -49,   91,  261,  355,  465, -146,  604,  921,
 /*    30 */  1030,  750, 1035,  952,  949,  964, 1039, 1064,   46,  374,
 /*    40 */  -186, -135,  -40,  970,  973, 1094, 1096, -237,  787, -237,
 /*    50 */   787,  736,  736,  736,  736,  736,  736,  736,  736,  736,
 /*    60 */   736,  736,  736,  736,  736,  736,  736,  736,  736,  736,
 /*    70 */   736,  736,  736,  736,  736,  736,  736,  736,  736,  736,
 /*    80 */   736,  736,  736,  736,  736,  736,  736,  736,  736, -157,
 /*    90 */   698,  821,  930,  955, 1066, 1098, 1102, 1108, 1140, 1172,
 /*   100 */  1190, 1210, 1228, 1233, 1239, 1254, 1264, 1287, 1291, 1297,
 /*   110 */  1305, 1307, 1314, 1321, 1326, 1328, 1336, 1388, 1390, 1392,
 /*   120 */  1394, 1414, 1425, 1442, 1446, 1451, 1462, 1474, 1495, 1499,
 /*   130 */  1506, 1510, 1512, 1517, 1519, 1526, 1532, 1555, 1587, 1594,
 /*   140 */  1614, 1621,  736,  736,  736,  736,  736,  736,  736,  736,
 /*   150 */   736,  736,  736, -147, -231, -167,  335,  100,  518,  417,
 /*   160 */   452,  591,  736,  524,  182,  533,  658, 1118,  736,  736,
 /*   170 */   736,  736, -230, -230, -230, -133,  875, -170,  915,  953,
 /*   180 */  1124, 1131, 1113, -224,  376, -190, -190,  -89,  462,  584,
 /*   190 */   654,  456,  888, 1178, 1181,  789,  948,  966, 1084, 1153,
 /*   200 */  1007, 1176, 1243,  413, 1183,  318,  849,  197,  522,   23,
 /*   210 */   555, 1034, 1036,  806,  846, 1258,   32, 1075,  904, 1071,
 /*   220 */  1100, 1194, 1197, 1224, -198, -139,    7, -243, -140,   84,
 /*   230 */   208,  294,  314,  446,  503,  605,  624,  746,  801,  860,
 /*   240 */  1322,  792,  986, 1008, 1234, 1293, 1339, 1408, 1415, 1423,
 /*   250 */  1429, 1466, 1477, 1486, 1238, 1501, 1511, 1518, 1522, 1530,
 /*   260 */  1533, 1549, 1567, 1568, 1378, 1581, 1589, 1595, 1553, 1481,
 /*   270 */  1504, 1607,   84, 1617, 1619, 1623, 1629, 1638, 1642, 1646,
 /*   280 */  1531, 1534, 1496, 1634, 1649, 1650, 1652, 1653, 1378, 1576,
 /*   290 */  1634, 1634, 1636, 1599, 1608, 1624, 1604, 1615, 1640, 1574,
 /*   300 */  1578, 1600, 1582, 1563, 1602, 1583, 1585, 1603, 1586, 1606,
 /*   310 */  1570, 1631, 1632, 1609, 1610, 1611, 1612, 1613, 1616, 1590,
 /*   320 */  1626, 1633, 1645, 1651, 1654, 1655, 1670, 1662, 1663, 1671,
 /*   330 */  1666, 1674, 1679, 1680, 1683, 1676, 1688, 1715, 1716, 1737,
 /*   340 */  1738, 1717, 1718, 1740, 1656, 1659, 1664, 1742, 1741, 1743,
 /*   350 */  1744, 1745, 1755, 1746, 1697, 1677, 1678, 1698, 1682, 1750,
 /*   360 */  1703, 1758, 1705, 1772, 1752, 1681, 1684, 1778, 1759, 1775,
 /*   370 */  1779, 1780, 1747, 1791, 1720, 1724, 1675, 1685, 1721, 1722,
 /*   380 */  1673, 1710, 1802, 1723, 1739, 1751, 1748, 1754, 1764, 1812,
 /*   390 */  1792, 1814, 1796, 1803, 1804, 1807, 1808, 1809, 1727, 1729,
 /*   400 */  1781, 1725, 1726, 1730, 1782, 1788, 1795, 1734, 1833, 1839,
 /*   410 */  1844, 1845, 1847,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */  1529, 1529, 1529, 1436, 1254, 1315, 1254, 1254, 1254, 1254,
 /*    10 */  1436, 1436, 1436, 1254, 1254, 1254, 1254, 1254, 1254, 1345,
 /*    20 */  1345, 1470, 1280, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*    30 */  1254, 1254, 1254, 1254, 1435, 1254, 1254, 1254, 1254, 1254,
 /*    40 */  1254, 1618, 1618, 1254, 1254, 1254, 1254, 1482, 1481, 1254,
 /*    50 */  1254, 1254, 1354, 1254, 1254, 1254, 1254, 1254, 1361, 1437,
 /*    60 */  1438, 1254, 1254, 1254, 1254, 1254, 1469, 1471, 1453, 1368,
 /*    70 */  1367, 1366, 1365, 1594, 1333, 1359, 1352, 1356, 1437, 1432,
 /*    80 */  1433, 1431, 1517, 1254, 1438, 1355, 1400, 1399, 1416, 1254,
 /*    90 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   100 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   110 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   120 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   130 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   140 */  1254, 1254, 1408, 1415, 1414, 1413, 1422, 1412, 1409, 1402,
 /*   150 */  1401, 1403, 1404, 1533, 1578, 1254, 1254, 1254, 1254, 1254,
 /*   160 */  1254, 1274, 1405, 1280, 1396, 1395, 1394, 1254, 1419, 1406,
 /*   170 */  1418, 1417, 1643, 1642, 1477, 1254, 1254, 1454, 1254, 1254,
 /*   180 */  1254, 1254, 1254, 1618, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   190 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   200 */  1254, 1254, 1254, 1254, 1254, 1335, 1254, 1618, 1618, 1280,
 /*   210 */  1618, 1618, 1276, 1336, 1336, 1276, 1339, 1607, 1306, 1306,
 /*   220 */  1306, 1306, 1315, 1306, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   230 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   240 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1474, 1472,
 /*   250 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   260 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1311, 1254,
 /*   270 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1498,
 /*   280 */  1537, 1254, 1589, 1292, 1311, 1311, 1311, 1311, 1313, 1305,
 /*   290 */  1293, 1291, 1261, 1652, 1553, 1254, 1548, 1345, 1527, 1371,
 /*   300 */  1360, 1312, 1360, 1657, 1358, 1371, 1371, 1358, 1371, 1312,
 /*   310 */  1657, 1345, 1345, 1335, 1335, 1335, 1335, 1339, 1339, 1434,
 /*   320 */  1312, 1305, 1444, 1579, 1579, 1579, 1571, 1254, 1254, 1579,
 /*   330 */  1568, 1444, 1579, 1553, 1579, 1568, 1579, 1527, 1527, 1321,
 /*   340 */  1321, 1526, 1526, 1321, 1454, 1514, 1380, 1281, 1287, 1287,
 /*   350 */  1287, 1287, 1321, 1271, 1358, 1514, 1514, 1358, 1380, 1281,
 /*   360 */  1358, 1281, 1358, 1321, 1271, 1593, 1654, 1321, 1271, 1254,
 /*   370 */  1448, 1448, 1444, 1321, 1353, 1339, 1628, 1628, 1348, 1348,
 /*   380 */  1636, 1439, 1321, 1254, 1353, 1351, 1349, 1358, 1444, 1321,
 /*   390 */  1271, 1321, 1271, 1501, 1501, 1497, 1497, 1497, 1645, 1645,
 /*   400 */  1555, 1661, 1661, 1607, 1555, 1537, 1537, 1645, 1280, 1280,
 /*   410 */  1280, 1280, 1280, 1254, 1254, 1254, 1254, 1254, 1504, 1254,
 /*   420 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1479, 1455, 1325,
 /*   430 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1483, 1254,
 /*   440 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   450 */  1385, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   460 */  1254, 1254, 1531, 1254, 1254, 1254, 1604, 1551, 1254, 1254,
 /*   470 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   480 */  1254, 1254, 1362, 1363, 1326, 1254, 1254, 1254, 1254, 1254,
 /*   490 */  1254, 1254, 1377, 1254, 1254, 1254, 1372, 1254, 1254, 1254,
 /*   500 */  1254, 1254, 1254, 1254, 1254, 1656, 1254, 1254, 1254, 1254,
 /*   510 */  1254, 1254, 1592, 1591, 1254, 1254, 1323, 1254, 1254, 1254,
 /*   520 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1350, 1254,
 /*   530 */  1254, 1254, 1254, 1633, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   540 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   550 */  1340, 1254, 1254, 1254, 1254, 1254, 1518, 1254, 1254, 1254,
 /*   560 */  1254, 1254, 1254, 1254, 1254, 1510, 1254, 1254, 1254, 1254,
 /*   570 */  1254, 1300, 1389, 1386, 1254, 1254, 1295, 1254, 1254, 1254,
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
  /*  222 */ "cmdx",
  /*  223 */ "cmd",
  /*  224 */ "create_table",
  /*  225 */ "create_table_args",
  /*  226 */ "columnlist",
  /*  227 */ "conslist_opt",
  /*  228 */ "table_option_set",
  /*  229 */ "select",
  /*  230 */ "table_option",
  /*  231 */ "scantok",
  /*  232 */ "ccons",
  /*  233 */ "term",
  /*  234 */ "expr",
  /*  235 */ "onconf",
  /*  236 */ "sortorder",
  /*  237 */ "autoinc",
  /*  238 */ "eidlist_opt",
  /*  239 */ "refargs",
  /*  240 */ "defer_subclause",
  /*  241 */ "generated",
  /*  242 */ "refarg",
  /*  243 */ "refact",
  /*  244 */ "init_deferred_pred_opt",
  /*  245 */ "tconscomma",
  /*  246 */ "tcons",
  /*  247 */ "sortlist",
  /*  248 */ "eidlist",
  /*  249 */ "defer_subclause_opt",
  /*  250 */ "fullname",
  /*  251 */ "wqlist",
  /*  252 */ "selectnowith",
  /*  253 */ "oneselect",
  /*  254 */ "distinct",
  /*  255 */ "selcollist",
  /*  256 */ "from",
  /*  257 */ "where_opt",
  /*  258 */ "groupby_opt",
  /*  259 */ "having_opt",
  /*  260 */ "orderby_opt",
  /*  261 */ "limit_opt",
  /*  262 */ "window_clause",
  /*  263 */ "values",
  /*  264 */ "nexprlist",
  /*  265 */ "mvalues",
  /*  266 */ "sclp",
  /*  267 */ "seltablist",
  /*  268 */ "stl_prefix",
  /*  269 */ "on_using",
  /*  270 */ "exprlist",
  /*  271 */ "xfullname",
  /*  272 */ "idlist",
  /*  273 */ "nulls",
  /*  274 */ "with",
  /*  275 */ "where_opt_ret",
  /*  276 */ "setlist",
  /*  277 */ "idlist_opt",
  /*  278 */ "upsert",
  /*  279 */ "returning",
  /*  280 */ "filter_over",
  /*  281 */ "between_op",
  /*  282 */ "paren_exprlist",
  /*  283 */ "case_operand",
  /*  284 */ "case_exprlist",
  /*  285 */ "case_else",
  /*  286 */ "vinto",
  /*  287 */ "trigger_decl",
  /*  288 */ "trigger_cmd_list",
  /*  289 */ "trigger_time",
  /*  290 */ "trigger_event",
  /*  291 */ "foreach_clause",
  /*  292 */ "when_clause",
  /*  293 */ "trigger_cmd",
  /*  294 */ "trnm",
  /*  295 */ "tridxby",
  /*  296 */ "key_opt",
  /*  297 */ "add_column_fullname",
  /*  298 */ "carglist",
  /*  299 */ "create_vtab",
  /*  300 */ "vtabarglist",
  /*  301 */ "vtabarg",
  /*  302 */ "vtabargtoken",
  /*  303 */ "lp",
  /*  304 */ "anylist",
  /*  305 */ "wqitem",
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
 /*  13 */ "createkw ::= CREATE",
 /*  14 */ "ifnotexists ::=",
 /*  15 */ "ifnotexists ::= IF NOT EXISTS",
 /*  16 */ "temp ::= TEMP",
 /*  17 */ "temp ::=",
 /*  18 */ "columnname ::= nm typetoken",
 /*  19 */ "typetoken ::=",
 /*  20 */ "typetoken ::= typename LP signed RP",
 /*  21 */ "typetoken ::= typename LP signed COMMA signed RP",
 /*  22 */ "typename ::= typename ID|STRING",
 /*  23 */ "scanpt ::=",
 /*  24 */ "orconf ::=",
 /*  25 */ "orconf ::= OR resolvetype",
 /*  26 */ "resolvetype ::= IGNORE",
 /*  27 */ "resolvetype ::= REPLACE",
 /*  28 */ "cmd ::= DROP TABLE ifexists fullname",
 /*  29 */ "ifexists ::= IF EXISTS",
 /*  30 */ "ifexists ::=",
 /*  31 */ "cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select",
 /*  32 */ "cmd ::= DROP VIEW ifexists fullname",
 /*  33 */ "cmd ::= select",
 /*  34 */ "select ::= WITH wqlist selectnowith",
 /*  35 */ "select ::= WITH RECURSIVE wqlist selectnowith",
 /*  36 */ "select ::= selectnowith",
 /*  37 */ "selectnowith ::= selectnowith multiselect_op oneselect",
 /*  38 */ "multiselect_op ::= UNION",
 /*  39 */ "multiselect_op ::= UNION ALL",
 /*  40 */ "multiselect_op ::= EXCEPT|INTERSECT",
 /*  41 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt",
 /*  42 */ "values ::= VALUES LP nexprlist RP",
 /*  43 */ "oneselect ::= mvalues",
 /*  44 */ "mvalues ::= values COMMA LP nexprlist RP",
 /*  45 */ "mvalues ::= mvalues COMMA LP nexprlist RP",
 /*  46 */ "distinct ::= DISTINCT",
 /*  47 */ "distinct ::= ALL",
 /*  48 */ "distinct ::=",
 /*  49 */ "sclp ::=",
 /*  50 */ "selcollist ::= sclp scanpt expr scanpt as",
 /*  51 */ "selcollist ::= sclp scanpt STAR",
 /*  52 */ "selcollist ::= sclp scanpt nm DOT STAR",
 /*  53 */ "as ::= AS nm",
 /*  54 */ "as ::=",
 /*  55 */ "from ::=",
 /*  56 */ "from ::= FROM seltablist",
 /*  57 */ "stl_prefix ::= seltablist joinop",
 /*  58 */ "stl_prefix ::=",
 /*  59 */ "seltablist ::= stl_prefix nm dbnm as on_using",
 /*  60 */ "seltablist ::= stl_prefix nm dbnm as indexed_by on_using",
 /*  61 */ "seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using",
 /*  62 */ "seltablist ::= stl_prefix LP select RP as on_using",
 /*  63 */ "seltablist ::= stl_prefix LP seltablist RP as on_using",
 /*  64 */ "dbnm ::=",
 /*  65 */ "dbnm ::= DOT nm",
 /*  66 */ "fullname ::= nm",
 /*  67 */ "fullname ::= nm DOT nm",
 /*  68 */ "xfullname ::= nm",
 /*  69 */ "xfullname ::= nm DOT nm",
 /*  70 */ "xfullname ::= nm DOT nm AS nm",
 /*  71 */ "xfullname ::= nm AS nm",
 /*  72 */ "joinop ::= COMMA|JOIN",
 /*  73 */ "joinop ::= JOIN_KW JOIN",
 /*  74 */ "joinop ::= JOIN_KW nm JOIN",
 /*  75 */ "joinop ::= JOIN_KW nm nm JOIN",
 /*  76 */ "on_using ::= ON expr",
 /*  77 */ "on_using ::= USING LP idlist RP",
 /*  78 */ "on_using ::=",
 /*  79 */ "indexed_opt ::=",
 /*  80 */ "indexed_by ::= INDEXED BY nm",
 /*  81 */ "indexed_by ::= NOT INDEXED",
 /*  82 */ "orderby_opt ::=",
 /*  83 */ "orderby_opt ::= ORDER BY sortlist",
 /*  84 */ "sortlist ::= sortlist COMMA expr sortorder nulls",
 /*  85 */ "sortlist ::= expr sortorder nulls",
 /*  86 */ "sortorder ::= ASC",
 /*  87 */ "sortorder ::= DESC",
 /*  88 */ "sortorder ::=",
 /*  89 */ "nulls ::= NULLS FIRST",
 /*  90 */ "nulls ::= NULLS LAST",
 /*  91 */ "nulls ::=",
 /*  92 */ "groupby_opt ::=",
 /*  93 */ "groupby_opt ::= GROUP BY nexprlist",
 /*  94 */ "having_opt ::=",
 /*  95 */ "having_opt ::= HAVING expr",
 /*  96 */ "limit_opt ::=",
 /*  97 */ "limit_opt ::= LIMIT expr",
 /*  98 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /*  99 */ "limit_opt ::= LIMIT expr COMMA expr",
 /* 100 */ "cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret",
 /* 101 */ "where_opt ::=",
 /* 102 */ "where_opt ::= WHERE expr",
 /* 103 */ "where_opt_ret ::=",
 /* 104 */ "where_opt_ret ::= WHERE expr",
 /* 105 */ "where_opt_ret ::= RETURNING selcollist",
 /* 106 */ "where_opt_ret ::= WHERE expr RETURNING selcollist",
 /* 107 */ "cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret",
 /* 108 */ "setlist ::= setlist COMMA nm EQ expr",
 /* 109 */ "setlist ::= setlist COMMA LP idlist RP EQ expr",
 /* 110 */ "setlist ::= nm EQ expr",
 /* 111 */ "setlist ::= LP idlist RP EQ expr",
 /* 112 */ "cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert",
 /* 113 */ "cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning",
 /* 114 */ "upsert ::=",
 /* 115 */ "upsert ::= RETURNING selcollist",
 /* 116 */ "upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert",
 /* 117 */ "upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert",
 /* 118 */ "upsert ::= ON CONFLICT DO NOTHING returning",
 /* 119 */ "upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning",
 /* 120 */ "returning ::= RETURNING selcollist",
 /* 121 */ "insert_cmd ::= INSERT orconf",
 /* 122 */ "insert_cmd ::= REPLACE",
 /* 123 */ "idlist_opt ::=",
 /* 124 */ "idlist_opt ::= LP idlist RP",
 /* 125 */ "idlist ::= idlist COMMA nm",
 /* 126 */ "idlist ::= nm",
 /* 127 */ "expr ::= LP expr RP",
 /* 128 */ "expr ::= ID|INDEXED|JOIN_KW",
 /* 129 */ "expr ::= nm DOT nm",
 /* 130 */ "expr ::= nm DOT nm DOT nm",
 /* 131 */ "term ::= NULL|FLOAT|BLOB",
 /* 132 */ "term ::= STRING",
 /* 133 */ "term ::= INTEGER",
 /* 134 */ "expr ::= VARIABLE",
 /* 135 */ "expr ::= expr COLLATE ID|STRING",
 /* 136 */ "expr ::= CAST LP expr AS typetoken RP",
 /* 137 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP",
 /* 138 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP",
 /* 139 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP",
 /* 140 */ "term ::= CTIME_KW",
 /* 141 */ "expr ::= LP nexprlist COMMA expr RP",
 /* 142 */ "expr ::= expr AND expr",
 /* 143 */ "expr ::= expr OR expr",
 /* 144 */ "expr ::= expr LT|GT|GE|LE expr",
 /* 145 */ "expr ::= expr EQ|NE expr",
 /* 146 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
 /* 147 */ "expr ::= expr PLUS|MINUS expr",
 /* 148 */ "expr ::= expr STAR|SLASH|REM expr",
 /* 149 */ "expr ::= expr CONCAT expr",
 /* 150 */ "likeop ::= NOT LIKE_KW|MATCH",
 /* 151 */ "expr ::= expr likeop expr",
 /* 152 */ "expr ::= expr likeop expr ESCAPE expr",
 /* 153 */ "expr ::= expr ISNULL|NOTNULL",
 /* 154 */ "expr ::= expr NOT NULL",
 /* 155 */ "expr ::= expr IS expr",
 /* 156 */ "expr ::= expr IS NOT expr",
 /* 157 */ "expr ::= expr IS NOT DISTINCT FROM expr",
 /* 158 */ "expr ::= expr IS DISTINCT FROM expr",
 /* 159 */ "expr ::= NOT expr",
 /* 160 */ "expr ::= BITNOT expr",
 /* 161 */ "expr ::= PLUS|MINUS expr",
 /* 162 */ "expr ::= expr PTR expr",
 /* 163 */ "between_op ::= BETWEEN",
 /* 164 */ "between_op ::= NOT BETWEEN",
 /* 165 */ "expr ::= expr between_op expr AND expr",
 /* 166 */ "in_op ::= IN",
 /* 167 */ "in_op ::= NOT IN",
 /* 168 */ "expr ::= expr in_op LP exprlist RP",
 /* 169 */ "expr ::= LP select RP",
 /* 170 */ "expr ::= expr in_op LP select RP",
 /* 171 */ "expr ::= expr in_op nm dbnm paren_exprlist",
 /* 172 */ "expr ::= EXISTS LP select RP",
 /* 173 */ "expr ::= CASE case_operand case_exprlist case_else END",
 /* 174 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
 /* 175 */ "case_exprlist ::= WHEN expr THEN expr",
 /* 176 */ "case_else ::= ELSE expr",
 /* 177 */ "case_else ::=",
 /* 178 */ "case_operand ::=",
 /* 179 */ "exprlist ::=",
 /* 180 */ "nexprlist ::= nexprlist COMMA expr",
 /* 181 */ "nexprlist ::= expr",
 /* 182 */ "paren_exprlist ::=",
 /* 183 */ "paren_exprlist ::= LP exprlist RP",
 /* 184 */ "cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt",
 /* 185 */ "uniqueflag ::= UNIQUE",
 /* 186 */ "uniqueflag ::=",
 /* 187 */ "eidlist_opt ::=",
 /* 188 */ "eidlist_opt ::= LP eidlist RP",
 /* 189 */ "eidlist ::= eidlist COMMA nm collate sortorder",
 /* 190 */ "eidlist ::= nm collate sortorder",
 /* 191 */ "collate ::=",
 /* 192 */ "collate ::= COLLATE ID|STRING",
 /* 193 */ "cmd ::= DROP INDEX ifexists fullname",
 /* 194 */ "cmd ::= VACUUM vinto",
 /* 195 */ "cmd ::= VACUUM nm vinto",
 /* 196 */ "vinto ::= INTO expr",
 /* 197 */ "vinto ::=",
 /* 198 */ "cmd ::= PRAGMA nm dbnm",
 /* 199 */ "cmd ::= PRAGMA nm dbnm EQ nmnum",
 /* 200 */ "cmd ::= PRAGMA nm dbnm LP nmnum RP",
 /* 201 */ "cmd ::= PRAGMA nm dbnm EQ minus_num",
 /* 202 */ "cmd ::= PRAGMA nm dbnm LP minus_num RP",
 /* 203 */ "plus_num ::= PLUS INTEGER|FLOAT",
 /* 204 */ "minus_num ::= MINUS INTEGER|FLOAT",
 /* 205 */ "expr ::= RAISE LP IGNORE RP",
 /* 206 */ "expr ::= RAISE LP raisetype COMMA expr RP",
 /* 207 */ "raisetype ::= ROLLBACK",
 /* 208 */ "raisetype ::= ABORT",
 /* 209 */ "raisetype ::= FAIL",
 /* 210 */ "cmd ::= DROP TRIGGER ifexists fullname",
 /* 211 */ "cmd ::= ATTACH database_kw_opt expr AS expr key_opt",
 /* 212 */ "cmd ::= DETACH database_kw_opt expr",
 /* 213 */ "key_opt ::=",
 /* 214 */ "key_opt ::= KEY expr",
 /* 215 */ "cmd ::= REINDEX",
 /* 216 */ "cmd ::= REINDEX nm dbnm",
 /* 217 */ "cmd ::= ANALYZE",
 /* 218 */ "cmd ::= ANALYZE nm dbnm",
 /* 219 */ "cmd ::= ALTER TABLE fullname RENAME TO nm",
 /* 220 */ "cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist",
 /* 221 */ "cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm",
 /* 222 */ "add_column_fullname ::= fullname",
 /* 223 */ "cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm",
 /* 224 */ "with ::= WITH wqlist",
 /* 225 */ "with ::= WITH RECURSIVE wqlist",
 /* 226 */ "wqas ::= AS",
 /* 227 */ "wqas ::= AS MATERIALIZED",
 /* 228 */ "wqas ::= AS NOT MATERIALIZED",
 /* 229 */ "wqitem ::= withnm eidlist_opt wqas LP select RP",
 /* 230 */ "withnm ::= nm",
 /* 231 */ "wqlist ::= wqitem",
 /* 232 */ "wqlist ::= wqlist COMMA wqitem",
 /* 233 */ "term ::= QNUMBER",
 /* 234 */ "input ::= cmdlist",
 /* 235 */ "cmdlist ::= cmdlist ecmd",
 /* 236 */ "cmdlist ::= ecmd",
 /* 237 */ "ecmd ::= SEMI",
 /* 238 */ "ecmd ::= cmdx SEMI",
 /* 239 */ "ecmd ::= explain cmdx SEMI",
 /* 240 */ "trans_opt ::=",
 /* 241 */ "trans_opt ::= TRANSACTION",
 /* 242 */ "trans_opt ::= TRANSACTION nm",
 /* 243 */ "savepoint_opt ::= SAVEPOINT",
 /* 244 */ "savepoint_opt ::=",
 /* 245 */ "nm ::= ID|INDEXED|JOIN_KW",
 /* 246 */ "nm ::= STRING",
 /* 247 */ "typetoken ::= typename",
 /* 248 */ "typename ::= ID|STRING",
 /* 249 */ "signed ::= plus_num",
 /* 250 */ "signed ::= minus_num",
 /* 251 */ "resolvetype ::= raisetype",
 /* 252 */ "selectnowith ::= oneselect",
 /* 253 */ "oneselect ::= values",
 /* 254 */ "sclp ::= selcollist COMMA",
 /* 255 */ "as ::= ID|STRING",
 /* 256 */ "indexed_opt ::= indexed_by",
 /* 257 */ "returning ::=",
 /* 258 */ "expr ::= term",
 /* 259 */ "likeop ::= LIKE_KW|MATCH",
 /* 260 */ "case_operand ::= expr",
 /* 261 */ "exprlist ::= nexprlist",
 /* 262 */ "nmnum ::= plus_num",
 /* 263 */ "nmnum ::= nm",
 /* 264 */ "nmnum ::= ON",
 /* 265 */ "nmnum ::= DELETE",
 /* 266 */ "nmnum ::= DEFAULT",
 /* 267 */ "plus_num ::= INTEGER|FLOAT",
 /* 268 */ "database_kw_opt ::= DATABASE",
 /* 269 */ "database_kw_opt ::=",
 /* 270 */ "kwcolumn_opt ::=",
 /* 271 */ "kwcolumn_opt ::= COLUMNKW",
 /* 272 */ "with ::=",
 /* 273 */ "create_table ::= createkw temp TABLE ifnotexists nm dbnm",
 /* 274 */ "create_table_args ::= LP columnlist conslist_opt RP table_option_set",
 /* 275 */ "create_table_args ::= AS select",
 /* 276 */ "table_option_set ::=",
 /* 277 */ "table_option_set ::= table_option_set COMMA table_option",
 /* 278 */ "table_option ::= WITHOUT nm",
 /* 279 */ "table_option ::= nm",
 /* 280 */ "scantok ::=",
 /* 281 */ "ccons ::= CONSTRAINT nm",
 /* 282 */ "ccons ::= DEFAULT scantok term",
 /* 283 */ "ccons ::= DEFAULT LP expr RP",
 /* 284 */ "ccons ::= DEFAULT PLUS scantok term",
 /* 285 */ "ccons ::= DEFAULT MINUS scantok term",
 /* 286 */ "ccons ::= DEFAULT scantok ID|INDEXED",
 /* 287 */ "ccons ::= NOT NULL onconf",
 /* 288 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
 /* 289 */ "ccons ::= UNIQUE onconf",
 /* 290 */ "ccons ::= CHECK LP expr RP",
 /* 291 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
 /* 292 */ "ccons ::= defer_subclause",
 /* 293 */ "ccons ::= COLLATE ID|STRING",
 /* 294 */ "generated ::= LP expr RP",
 /* 295 */ "generated ::= LP expr RP ID",
 /* 296 */ "autoinc ::=",
 /* 297 */ "autoinc ::= AUTOINCR",
 /* 298 */ "refargs ::=",
 /* 299 */ "refargs ::= refargs refarg",
 /* 300 */ "refarg ::= MATCH nm",
 /* 301 */ "refarg ::= ON INSERT refact",
 /* 302 */ "refarg ::= ON DELETE refact",
 /* 303 */ "refarg ::= ON UPDATE refact",
 /* 304 */ "refact ::= SET NULL",
 /* 305 */ "refact ::= SET DEFAULT",
 /* 306 */ "refact ::= CASCADE",
 /* 307 */ "refact ::= RESTRICT",
 /* 308 */ "refact ::= NO ACTION",
 /* 309 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
 /* 310 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
 /* 311 */ "init_deferred_pred_opt ::=",
 /* 312 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
 /* 313 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
 /* 314 */ "conslist_opt ::=",
 /* 315 */ "tconscomma ::= COMMA",
 /* 316 */ "tcons ::= CONSTRAINT nm",
 /* 317 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf",
 /* 318 */ "tcons ::= UNIQUE LP sortlist RP onconf",
 /* 319 */ "tcons ::= CHECK LP expr RP onconf",
 /* 320 */ "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /* 321 */ "defer_subclause_opt ::=",
 /* 322 */ "onconf ::=",
 /* 323 */ "onconf ::= ON CONFLICT resolvetype",
 /* 324 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt",
 /* 325 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over",
 /* 326 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over",
 /* 327 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over",
 /* 328 */ "cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END",
 /* 329 */ "trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause",
 /* 330 */ "trigger_time ::= BEFORE|AFTER",
 /* 331 */ "trigger_time ::= INSTEAD OF",
 /* 332 */ "trigger_time ::=",
 /* 333 */ "trigger_event ::= DELETE|INSERT",
 /* 334 */ "trigger_event ::= UPDATE",
 /* 335 */ "trigger_event ::= UPDATE OF idlist",
 /* 336 */ "when_clause ::=",
 /* 337 */ "when_clause ::= WHEN expr",
 /* 338 */ "trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI",
 /* 339 */ "trigger_cmd_list ::= trigger_cmd SEMI",
 /* 340 */ "trnm ::= nm DOT nm",
 /* 341 */ "tridxby ::= INDEXED BY nm",
 /* 342 */ "tridxby ::= NOT INDEXED",
 /* 343 */ "trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt",
 /* 344 */ "trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt",
 /* 345 */ "trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt",
 /* 346 */ "trigger_cmd ::= scanpt select scanpt",
 /* 347 */ "cmd ::= create_vtab",
 /* 348 */ "cmd ::= create_vtab LP vtabarglist RP",
 /* 349 */ "create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm",
 /* 350 */ "vtabarg ::=",
 /* 351 */ "vtabargtoken ::= ANY",
 /* 352 */ "vtabargtoken ::= lp anylist RP",
 /* 353 */ "lp ::= LP",
 /* 354 */ "windowdefn_list ::= windowdefn_list COMMA windowdefn",
 /* 355 */ "windowdefn ::= nm AS LP window RP",
 /* 356 */ "window ::= PARTITION BY nexprlist orderby_opt frame_opt",
 /* 357 */ "window ::= nm PARTITION BY nexprlist orderby_opt frame_opt",
 /* 358 */ "window ::= ORDER BY sortlist frame_opt",
 /* 359 */ "window ::= nm ORDER BY sortlist frame_opt",
 /* 360 */ "window ::= nm frame_opt",
 /* 361 */ "frame_opt ::=",
 /* 362 */ "frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt",
 /* 363 */ "frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt",
 /* 364 */ "range_or_rows ::= RANGE|ROWS|GROUPS",
 /* 365 */ "frame_bound_s ::= frame_bound",
 /* 366 */ "frame_bound_s ::= UNBOUNDED PRECEDING",
 /* 367 */ "frame_bound_e ::= frame_bound",
 /* 368 */ "frame_bound_e ::= UNBOUNDED FOLLOWING",
 /* 369 */ "frame_bound ::= expr PRECEDING|FOLLOWING",
 /* 370 */ "frame_bound ::= CURRENT ROW",
 /* 371 */ "frame_exclude_opt ::=",
 /* 372 */ "frame_exclude_opt ::= EXCLUDE frame_exclude",
 /* 373 */ "frame_exclude ::= NO OTHERS",
 /* 374 */ "frame_exclude ::= CURRENT ROW",
 /* 375 */ "frame_exclude ::= GROUP|TIES",
 /* 376 */ "window_clause ::= WINDOW windowdefn_list",
 /* 377 */ "filter_over ::= filter_clause over_clause",
 /* 378 */ "filter_over ::= over_clause",
 /* 379 */ "filter_over ::= filter_clause",
 /* 380 */ "over_clause ::= OVER LP window RP",
 /* 381 */ "over_clause ::= OVER nm",
 /* 382 */ "filter_clause ::= FILTER LP WHERE expr RP",
 /* 383 */ "cmd ::= create_table create_table_args",
 /* 384 */ "table_option_set ::= table_option",
 /* 385 */ "columnlist ::= columnlist COMMA columnname carglist",
 /* 386 */ "columnlist ::= columnname carglist",
 /* 387 */ "carglist ::= carglist ccons",
 /* 388 */ "carglist ::=",
 /* 389 */ "ccons ::= NULL onconf",
 /* 390 */ "ccons ::= GENERATED ALWAYS AS generated",
 /* 391 */ "ccons ::= AS generated",
 /* 392 */ "conslist_opt ::= COMMA conslist",
 /* 393 */ "conslist ::= conslist tconscomma tcons",
 /* 394 */ "conslist ::= tcons",
 /* 395 */ "tconscomma ::=",
 /* 396 */ "defer_subclause_opt ::= defer_subclause",
 /* 397 */ "foreach_clause ::=",
 /* 398 */ "foreach_clause ::= FOR EACH ROW",
 /* 399 */ "trnm ::= nm",
 /* 400 */ "tridxby ::=",
 /* 401 */ "vtabarglist ::= vtabarg",
 /* 402 */ "vtabarglist ::= vtabarglist COMMA vtabarg",
 /* 403 */ "vtabarg ::= vtabarg vtabargtoken",
 /* 404 */ "anylist ::=",
 /* 405 */ "anylist ::= anylist LP anylist RP",
 /* 406 */ "anylist ::= anylist ANY",
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
   216,  /* (0) explain ::= EXPLAIN */
   216,  /* (1) explain ::= EXPLAIN QUERY PLAN */
   222,  /* (2) cmdx ::= cmd */
   223,  /* (3) cmd ::= BEGIN transtype trans_opt */
   207,  /* (4) transtype ::= */
   207,  /* (5) transtype ::= DEFERRED */
   207,  /* (6) transtype ::= IMMEDIATE */
   207,  /* (7) transtype ::= EXCLUSIVE */
   223,  /* (8) cmd ::= COMMIT|END trans_opt */
   223,  /* (9) cmd ::= ROLLBACK trans_opt */
   223,  /* (10) cmd ::= SAVEPOINT nm */
   223,  /* (11) cmd ::= RELEASE savepoint_opt nm */
   223,  /* (12) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
   217,  /* (13) createkw ::= CREATE */
   212,  /* (14) ifnotexists ::= */
   212,  /* (15) ifnotexists ::= IF NOT EXISTS */
   213,  /* (16) temp ::= TEMP */
   213,  /* (17) temp ::= */
   211,  /* (18) columnname ::= nm typetoken */
   194,  /* (19) typetoken ::= */
   194,  /* (20) typetoken ::= typename LP signed RP */
   194,  /* (21) typetoken ::= typename LP signed COMMA signed RP */
   195,  /* (22) typename ::= typename ID|STRING */
   189,  /* (23) scanpt ::= */
   203,  /* (24) orconf ::= */
   203,  /* (25) orconf ::= OR resolvetype */
   204,  /* (26) resolvetype ::= IGNORE */
   204,  /* (27) resolvetype ::= REPLACE */
   223,  /* (28) cmd ::= DROP TABLE ifexists fullname */
   206,  /* (29) ifexists ::= IF EXISTS */
   206,  /* (30) ifexists ::= */
   223,  /* (31) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   223,  /* (32) cmd ::= DROP VIEW ifexists fullname */
   223,  /* (33) cmd ::= select */
   229,  /* (34) select ::= WITH wqlist selectnowith */
   229,  /* (35) select ::= WITH RECURSIVE wqlist selectnowith */
   229,  /* (36) select ::= selectnowith */
   252,  /* (37) selectnowith ::= selectnowith multiselect_op oneselect */
   192,  /* (38) multiselect_op ::= UNION */
   192,  /* (39) multiselect_op ::= UNION ALL */
   192,  /* (40) multiselect_op ::= EXCEPT|INTERSECT */
   253,  /* (41) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
   263,  /* (42) values ::= VALUES LP nexprlist RP */
   253,  /* (43) oneselect ::= mvalues */
   265,  /* (44) mvalues ::= values COMMA LP nexprlist RP */
   265,  /* (45) mvalues ::= mvalues COMMA LP nexprlist RP */
   254,  /* (46) distinct ::= DISTINCT */
   254,  /* (47) distinct ::= ALL */
   254,  /* (48) distinct ::= */
   266,  /* (49) sclp ::= */
   255,  /* (50) selcollist ::= sclp scanpt expr scanpt as */
   255,  /* (51) selcollist ::= sclp scanpt STAR */
   255,  /* (52) selcollist ::= sclp scanpt nm DOT STAR */
   188,  /* (53) as ::= AS nm */
   188,  /* (54) as ::= */
   256,  /* (55) from ::= */
   256,  /* (56) from ::= FROM seltablist */
   268,  /* (57) stl_prefix ::= seltablist joinop */
   268,  /* (58) stl_prefix ::= */
   267,  /* (59) seltablist ::= stl_prefix nm dbnm as on_using */
   267,  /* (60) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   267,  /* (61) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   267,  /* (62) seltablist ::= stl_prefix LP select RP as on_using */
   267,  /* (63) seltablist ::= stl_prefix LP seltablist RP as on_using */
   191,  /* (64) dbnm ::= */
   191,  /* (65) dbnm ::= DOT nm */
   250,  /* (66) fullname ::= nm */
   250,  /* (67) fullname ::= nm DOT nm */
   271,  /* (68) xfullname ::= nm */
   271,  /* (69) xfullname ::= nm DOT nm */
   271,  /* (70) xfullname ::= nm DOT nm AS nm */
   271,  /* (71) xfullname ::= nm AS nm */
   200,  /* (72) joinop ::= COMMA|JOIN */
   200,  /* (73) joinop ::= JOIN_KW JOIN */
   200,  /* (74) joinop ::= JOIN_KW nm JOIN */
   200,  /* (75) joinop ::= JOIN_KW nm nm JOIN */
   269,  /* (76) on_using ::= ON expr */
   269,  /* (77) on_using ::= USING LP idlist RP */
   269,  /* (78) on_using ::= */
   205,  /* (79) indexed_opt ::= */
   201,  /* (80) indexed_by ::= INDEXED BY nm */
   201,  /* (81) indexed_by ::= NOT INDEXED */
   260,  /* (82) orderby_opt ::= */
   260,  /* (83) orderby_opt ::= ORDER BY sortlist */
   247,  /* (84) sortlist ::= sortlist COMMA expr sortorder nulls */
   247,  /* (85) sortlist ::= expr sortorder nulls */
   236,  /* (86) sortorder ::= ASC */
   236,  /* (87) sortorder ::= DESC */
   236,  /* (88) sortorder ::= */
   273,  /* (89) nulls ::= NULLS FIRST */
   273,  /* (90) nulls ::= NULLS LAST */
   273,  /* (91) nulls ::= */
   258,  /* (92) groupby_opt ::= */
   258,  /* (93) groupby_opt ::= GROUP BY nexprlist */
   259,  /* (94) having_opt ::= */
   259,  /* (95) having_opt ::= HAVING expr */
   261,  /* (96) limit_opt ::= */
   261,  /* (97) limit_opt ::= LIMIT expr */
   261,  /* (98) limit_opt ::= LIMIT expr OFFSET expr */
   261,  /* (99) limit_opt ::= LIMIT expr COMMA expr */
   223,  /* (100) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
   257,  /* (101) where_opt ::= */
   257,  /* (102) where_opt ::= WHERE expr */
   275,  /* (103) where_opt_ret ::= */
   275,  /* (104) where_opt_ret ::= WHERE expr */
   275,  /* (105) where_opt_ret ::= RETURNING selcollist */
   275,  /* (106) where_opt_ret ::= WHERE expr RETURNING selcollist */
   223,  /* (107) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   276,  /* (108) setlist ::= setlist COMMA nm EQ expr */
   276,  /* (109) setlist ::= setlist COMMA LP idlist RP EQ expr */
   276,  /* (110) setlist ::= nm EQ expr */
   276,  /* (111) setlist ::= LP idlist RP EQ expr */
   223,  /* (112) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   223,  /* (113) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
   278,  /* (114) upsert ::= */
   278,  /* (115) upsert ::= RETURNING selcollist */
   278,  /* (116) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
   278,  /* (117) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
   278,  /* (118) upsert ::= ON CONFLICT DO NOTHING returning */
   278,  /* (119) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
   279,  /* (120) returning ::= RETURNING selcollist */
   202,  /* (121) insert_cmd ::= INSERT orconf */
   202,  /* (122) insert_cmd ::= REPLACE */
   277,  /* (123) idlist_opt ::= */
   277,  /* (124) idlist_opt ::= LP idlist RP */
   272,  /* (125) idlist ::= idlist COMMA nm */
   272,  /* (126) idlist ::= nm */
   234,  /* (127) expr ::= LP expr RP */
   234,  /* (128) expr ::= ID|INDEXED|JOIN_KW */
   234,  /* (129) expr ::= nm DOT nm */
   234,  /* (130) expr ::= nm DOT nm DOT nm */
   233,  /* (131) term ::= NULL|FLOAT|BLOB */
   233,  /* (132) term ::= STRING */
   233,  /* (133) term ::= INTEGER */
   234,  /* (134) expr ::= VARIABLE */
   234,  /* (135) expr ::= expr COLLATE ID|STRING */
   234,  /* (136) expr ::= CAST LP expr AS typetoken RP */
   234,  /* (137) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   234,  /* (138) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   234,  /* (139) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   233,  /* (140) term ::= CTIME_KW */
   234,  /* (141) expr ::= LP nexprlist COMMA expr RP */
   234,  /* (142) expr ::= expr AND expr */
   234,  /* (143) expr ::= expr OR expr */
   234,  /* (144) expr ::= expr LT|GT|GE|LE expr */
   234,  /* (145) expr ::= expr EQ|NE expr */
   234,  /* (146) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
   234,  /* (147) expr ::= expr PLUS|MINUS expr */
   234,  /* (148) expr ::= expr STAR|SLASH|REM expr */
   234,  /* (149) expr ::= expr CONCAT expr */
   190,  /* (150) likeop ::= NOT LIKE_KW|MATCH */
   234,  /* (151) expr ::= expr likeop expr */
   234,  /* (152) expr ::= expr likeop expr ESCAPE expr */
   234,  /* (153) expr ::= expr ISNULL|NOTNULL */
   234,  /* (154) expr ::= expr NOT NULL */
   234,  /* (155) expr ::= expr IS expr */
   234,  /* (156) expr ::= expr IS NOT expr */
   234,  /* (157) expr ::= expr IS NOT DISTINCT FROM expr */
   234,  /* (158) expr ::= expr IS DISTINCT FROM expr */
   234,  /* (159) expr ::= NOT expr */
   234,  /* (160) expr ::= BITNOT expr */
   234,  /* (161) expr ::= PLUS|MINUS expr */
   234,  /* (162) expr ::= expr PTR expr */
   281,  /* (163) between_op ::= BETWEEN */
   281,  /* (164) between_op ::= NOT BETWEEN */
   234,  /* (165) expr ::= expr between_op expr AND expr */
   193,  /* (166) in_op ::= IN */
   193,  /* (167) in_op ::= NOT IN */
   234,  /* (168) expr ::= expr in_op LP exprlist RP */
   234,  /* (169) expr ::= LP select RP */
   234,  /* (170) expr ::= expr in_op LP select RP */
   234,  /* (171) expr ::= expr in_op nm dbnm paren_exprlist */
   234,  /* (172) expr ::= EXISTS LP select RP */
   234,  /* (173) expr ::= CASE case_operand case_exprlist case_else END */
   284,  /* (174) case_exprlist ::= case_exprlist WHEN expr THEN expr */
   284,  /* (175) case_exprlist ::= WHEN expr THEN expr */
   285,  /* (176) case_else ::= ELSE expr */
   285,  /* (177) case_else ::= */
   283,  /* (178) case_operand ::= */
   270,  /* (179) exprlist ::= */
   264,  /* (180) nexprlist ::= nexprlist COMMA expr */
   264,  /* (181) nexprlist ::= expr */
   282,  /* (182) paren_exprlist ::= */
   282,  /* (183) paren_exprlist ::= LP exprlist RP */
   223,  /* (184) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
   214,  /* (185) uniqueflag ::= UNIQUE */
   214,  /* (186) uniqueflag ::= */
   238,  /* (187) eidlist_opt ::= */
   238,  /* (188) eidlist_opt ::= LP eidlist RP */
   248,  /* (189) eidlist ::= eidlist COMMA nm collate sortorder */
   248,  /* (190) eidlist ::= nm collate sortorder */
   198,  /* (191) collate ::= */
   198,  /* (192) collate ::= COLLATE ID|STRING */
   223,  /* (193) cmd ::= DROP INDEX ifexists fullname */
   223,  /* (194) cmd ::= VACUUM vinto */
   223,  /* (195) cmd ::= VACUUM nm vinto */
   286,  /* (196) vinto ::= INTO expr */
   286,  /* (197) vinto ::= */
   223,  /* (198) cmd ::= PRAGMA nm dbnm */
   223,  /* (199) cmd ::= PRAGMA nm dbnm EQ nmnum */
   223,  /* (200) cmd ::= PRAGMA nm dbnm LP nmnum RP */
   223,  /* (201) cmd ::= PRAGMA nm dbnm EQ minus_num */
   223,  /* (202) cmd ::= PRAGMA nm dbnm LP minus_num RP */
   219,  /* (203) plus_num ::= PLUS INTEGER|FLOAT */
   220,  /* (204) minus_num ::= MINUS INTEGER|FLOAT */
   234,  /* (205) expr ::= RAISE LP IGNORE RP */
   234,  /* (206) expr ::= RAISE LP raisetype COMMA expr RP */
   199,  /* (207) raisetype ::= ROLLBACK */
   199,  /* (208) raisetype ::= ABORT */
   199,  /* (209) raisetype ::= FAIL */
   223,  /* (210) cmd ::= DROP TRIGGER ifexists fullname */
   223,  /* (211) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
   223,  /* (212) cmd ::= DETACH database_kw_opt expr */
   296,  /* (213) key_opt ::= */
   296,  /* (214) key_opt ::= KEY expr */
   223,  /* (215) cmd ::= REINDEX */
   223,  /* (216) cmd ::= REINDEX nm dbnm */
   223,  /* (217) cmd ::= ANALYZE */
   223,  /* (218) cmd ::= ANALYZE nm dbnm */
   223,  /* (219) cmd ::= ALTER TABLE fullname RENAME TO nm */
   223,  /* (220) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
   223,  /* (221) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
   297,  /* (222) add_column_fullname ::= fullname */
   223,  /* (223) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
   274,  /* (224) with ::= WITH wqlist */
   274,  /* (225) with ::= WITH RECURSIVE wqlist */
   197,  /* (226) wqas ::= AS */
   197,  /* (227) wqas ::= AS MATERIALIZED */
   197,  /* (228) wqas ::= AS NOT MATERIALIZED */
   305,  /* (229) wqitem ::= withnm eidlist_opt wqas LP select RP */
   196,  /* (230) withnm ::= nm */
   251,  /* (231) wqlist ::= wqitem */
   251,  /* (232) wqlist ::= wqlist COMMA wqitem */
   233,  /* (233) term ::= QNUMBER */
   318,  /* (234) input ::= cmdlist */
   319,  /* (235) cmdlist ::= cmdlist ecmd */
   319,  /* (236) cmdlist ::= ecmd */
   320,  /* (237) ecmd ::= SEMI */
   320,  /* (238) ecmd ::= cmdx SEMI */
   320,  /* (239) ecmd ::= explain cmdx SEMI */
   208,  /* (240) trans_opt ::= */
   208,  /* (241) trans_opt ::= TRANSACTION */
   208,  /* (242) trans_opt ::= TRANSACTION nm */
   209,  /* (243) savepoint_opt ::= SAVEPOINT */
   209,  /* (244) savepoint_opt ::= */
   187,  /* (245) nm ::= ID|INDEXED|JOIN_KW */
   187,  /* (246) nm ::= STRING */
   194,  /* (247) typetoken ::= typename */
   195,  /* (248) typename ::= ID|STRING */
   218,  /* (249) signed ::= plus_num */
   218,  /* (250) signed ::= minus_num */
   204,  /* (251) resolvetype ::= raisetype */
   252,  /* (252) selectnowith ::= oneselect */
   253,  /* (253) oneselect ::= values */
   266,  /* (254) sclp ::= selcollist COMMA */
   188,  /* (255) as ::= ID|STRING */
   205,  /* (256) indexed_opt ::= indexed_by */
   279,  /* (257) returning ::= */
   234,  /* (258) expr ::= term */
   190,  /* (259) likeop ::= LIKE_KW|MATCH */
   283,  /* (260) case_operand ::= expr */
   270,  /* (261) exprlist ::= nexprlist */
   221,  /* (262) nmnum ::= plus_num */
   221,  /* (263) nmnum ::= nm */
   221,  /* (264) nmnum ::= ON */
   221,  /* (265) nmnum ::= DELETE */
   221,  /* (266) nmnum ::= DEFAULT */
   219,  /* (267) plus_num ::= INTEGER|FLOAT */
   215,  /* (268) database_kw_opt ::= DATABASE */
   215,  /* (269) database_kw_opt ::= */
   210,  /* (270) kwcolumn_opt ::= */
   210,  /* (271) kwcolumn_opt ::= COLUMNKW */
   274,  /* (272) with ::= */
   224,  /* (273) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   225,  /* (274) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   225,  /* (275) create_table_args ::= AS select */
   228,  /* (276) table_option_set ::= */
   228,  /* (277) table_option_set ::= table_option_set COMMA table_option */
   230,  /* (278) table_option ::= WITHOUT nm */
   230,  /* (279) table_option ::= nm */
   231,  /* (280) scantok ::= */
   232,  /* (281) ccons ::= CONSTRAINT nm */
   232,  /* (282) ccons ::= DEFAULT scantok term */
   232,  /* (283) ccons ::= DEFAULT LP expr RP */
   232,  /* (284) ccons ::= DEFAULT PLUS scantok term */
   232,  /* (285) ccons ::= DEFAULT MINUS scantok term */
   232,  /* (286) ccons ::= DEFAULT scantok ID|INDEXED */
   232,  /* (287) ccons ::= NOT NULL onconf */
   232,  /* (288) ccons ::= PRIMARY KEY sortorder onconf autoinc */
   232,  /* (289) ccons ::= UNIQUE onconf */
   232,  /* (290) ccons ::= CHECK LP expr RP */
   232,  /* (291) ccons ::= REFERENCES nm eidlist_opt refargs */
   232,  /* (292) ccons ::= defer_subclause */
   232,  /* (293) ccons ::= COLLATE ID|STRING */
   241,  /* (294) generated ::= LP expr RP */
   241,  /* (295) generated ::= LP expr RP ID */
   237,  /* (296) autoinc ::= */
   237,  /* (297) autoinc ::= AUTOINCR */
   239,  /* (298) refargs ::= */
   239,  /* (299) refargs ::= refargs refarg */
   242,  /* (300) refarg ::= MATCH nm */
   242,  /* (301) refarg ::= ON INSERT refact */
   242,  /* (302) refarg ::= ON DELETE refact */
   242,  /* (303) refarg ::= ON UPDATE refact */
   243,  /* (304) refact ::= SET NULL */
   243,  /* (305) refact ::= SET DEFAULT */
   243,  /* (306) refact ::= CASCADE */
   243,  /* (307) refact ::= RESTRICT */
   243,  /* (308) refact ::= NO ACTION */
   240,  /* (309) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
   240,  /* (310) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
   244,  /* (311) init_deferred_pred_opt ::= */
   244,  /* (312) init_deferred_pred_opt ::= INITIALLY DEFERRED */
   244,  /* (313) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
   227,  /* (314) conslist_opt ::= */
   245,  /* (315) tconscomma ::= COMMA */
   246,  /* (316) tcons ::= CONSTRAINT nm */
   246,  /* (317) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
   246,  /* (318) tcons ::= UNIQUE LP sortlist RP onconf */
   246,  /* (319) tcons ::= CHECK LP expr RP onconf */
   246,  /* (320) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
   249,  /* (321) defer_subclause_opt ::= */
   235,  /* (322) onconf ::= */
   235,  /* (323) onconf ::= ON CONFLICT resolvetype */
   253,  /* (324) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   234,  /* (325) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   234,  /* (326) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   234,  /* (327) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   223,  /* (328) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
   287,  /* (329) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
   289,  /* (330) trigger_time ::= BEFORE|AFTER */
   289,  /* (331) trigger_time ::= INSTEAD OF */
   289,  /* (332) trigger_time ::= */
   290,  /* (333) trigger_event ::= DELETE|INSERT */
   290,  /* (334) trigger_event ::= UPDATE */
   290,  /* (335) trigger_event ::= UPDATE OF idlist */
   292,  /* (336) when_clause ::= */
   292,  /* (337) when_clause ::= WHEN expr */
   288,  /* (338) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
   288,  /* (339) trigger_cmd_list ::= trigger_cmd SEMI */
   294,  /* (340) trnm ::= nm DOT nm */
   295,  /* (341) tridxby ::= INDEXED BY nm */
   295,  /* (342) tridxby ::= NOT INDEXED */
   293,  /* (343) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
   293,  /* (344) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
   293,  /* (345) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
   293,  /* (346) trigger_cmd ::= scanpt select scanpt */
   223,  /* (347) cmd ::= create_vtab */
   223,  /* (348) cmd ::= create_vtab LP vtabarglist RP */
   299,  /* (349) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
   301,  /* (350) vtabarg ::= */
   302,  /* (351) vtabargtoken ::= ANY */
   302,  /* (352) vtabargtoken ::= lp anylist RP */
   303,  /* (353) lp ::= LP */
   306,  /* (354) windowdefn_list ::= windowdefn_list COMMA windowdefn */
   307,  /* (355) windowdefn ::= nm AS LP window RP */
   308,  /* (356) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
   308,  /* (357) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
   308,  /* (358) window ::= ORDER BY sortlist frame_opt */
   308,  /* (359) window ::= nm ORDER BY sortlist frame_opt */
   308,  /* (360) window ::= nm frame_opt */
   309,  /* (361) frame_opt ::= */
   309,  /* (362) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
   309,  /* (363) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
   310,  /* (364) range_or_rows ::= RANGE|ROWS|GROUPS */
   311,  /* (365) frame_bound_s ::= frame_bound */
   311,  /* (366) frame_bound_s ::= UNBOUNDED PRECEDING */
   313,  /* (367) frame_bound_e ::= frame_bound */
   313,  /* (368) frame_bound_e ::= UNBOUNDED FOLLOWING */
   314,  /* (369) frame_bound ::= expr PRECEDING|FOLLOWING */
   314,  /* (370) frame_bound ::= CURRENT ROW */
   312,  /* (371) frame_exclude_opt ::= */
   312,  /* (372) frame_exclude_opt ::= EXCLUDE frame_exclude */
   315,  /* (373) frame_exclude ::= NO OTHERS */
   315,  /* (374) frame_exclude ::= CURRENT ROW */
   315,  /* (375) frame_exclude ::= GROUP|TIES */
   262,  /* (376) window_clause ::= WINDOW windowdefn_list */
   280,  /* (377) filter_over ::= filter_clause over_clause */
   280,  /* (378) filter_over ::= over_clause */
   280,  /* (379) filter_over ::= filter_clause */
   317,  /* (380) over_clause ::= OVER LP window RP */
   317,  /* (381) over_clause ::= OVER nm */
   316,  /* (382) filter_clause ::= FILTER LP WHERE expr RP */
   223,  /* (383) cmd ::= create_table create_table_args */
   228,  /* (384) table_option_set ::= table_option */
   226,  /* (385) columnlist ::= columnlist COMMA columnname carglist */
   226,  /* (386) columnlist ::= columnname carglist */
   298,  /* (387) carglist ::= carglist ccons */
   298,  /* (388) carglist ::= */
   232,  /* (389) ccons ::= NULL onconf */
   232,  /* (390) ccons ::= GENERATED ALWAYS AS generated */
   232,  /* (391) ccons ::= AS generated */
   227,  /* (392) conslist_opt ::= COMMA conslist */
   321,  /* (393) conslist ::= conslist tconscomma tcons */
   321,  /* (394) conslist ::= tcons */
   245,  /* (395) tconscomma ::= */
   249,  /* (396) defer_subclause_opt ::= defer_subclause */
   291,  /* (397) foreach_clause ::= */
   291,  /* (398) foreach_clause ::= FOR EACH ROW */
   294,  /* (399) trnm ::= nm */
   295,  /* (400) tridxby ::= */
   300,  /* (401) vtabarglist ::= vtabarg */
   300,  /* (402) vtabarglist ::= vtabarglist COMMA vtabarg */
   301,  /* (403) vtabarg ::= vtabarg vtabargtoken */
   304,  /* (404) anylist ::= */
   304,  /* (405) anylist ::= anylist LP anylist RP */
   304,  /* (406) anylist ::= anylist ANY */
   306,  /* (407) windowdefn_list ::= windowdefn */
   308,  /* (408) window ::= frame_opt */
};
#endif /* !_SYNTAQLITE_EXTERNAL_PARSER */

/* For rule J, yyRuleInfoNRhs[J] contains the negative of the number
** of symbols on the right-hand side of that rule. */
#ifndef _SYNTAQLITE_EXTERNAL_PARSER
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
   -1,  /* (13) createkw ::= CREATE */
    0,  /* (14) ifnotexists ::= */
   -3,  /* (15) ifnotexists ::= IF NOT EXISTS */
   -1,  /* (16) temp ::= TEMP */
    0,  /* (17) temp ::= */
   -2,  /* (18) columnname ::= nm typetoken */
    0,  /* (19) typetoken ::= */
   -4,  /* (20) typetoken ::= typename LP signed RP */
   -6,  /* (21) typetoken ::= typename LP signed COMMA signed RP */
   -2,  /* (22) typename ::= typename ID|STRING */
    0,  /* (23) scanpt ::= */
    0,  /* (24) orconf ::= */
   -2,  /* (25) orconf ::= OR resolvetype */
   -1,  /* (26) resolvetype ::= IGNORE */
   -1,  /* (27) resolvetype ::= REPLACE */
   -4,  /* (28) cmd ::= DROP TABLE ifexists fullname */
   -2,  /* (29) ifexists ::= IF EXISTS */
    0,  /* (30) ifexists ::= */
   -9,  /* (31) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   -4,  /* (32) cmd ::= DROP VIEW ifexists fullname */
   -1,  /* (33) cmd ::= select */
   -3,  /* (34) select ::= WITH wqlist selectnowith */
   -4,  /* (35) select ::= WITH RECURSIVE wqlist selectnowith */
   -1,  /* (36) select ::= selectnowith */
   -3,  /* (37) selectnowith ::= selectnowith multiselect_op oneselect */
   -1,  /* (38) multiselect_op ::= UNION */
   -2,  /* (39) multiselect_op ::= UNION ALL */
   -1,  /* (40) multiselect_op ::= EXCEPT|INTERSECT */
   -9,  /* (41) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
   -4,  /* (42) values ::= VALUES LP nexprlist RP */
   -1,  /* (43) oneselect ::= mvalues */
   -5,  /* (44) mvalues ::= values COMMA LP nexprlist RP */
   -5,  /* (45) mvalues ::= mvalues COMMA LP nexprlist RP */
   -1,  /* (46) distinct ::= DISTINCT */
   -1,  /* (47) distinct ::= ALL */
    0,  /* (48) distinct ::= */
    0,  /* (49) sclp ::= */
   -5,  /* (50) selcollist ::= sclp scanpt expr scanpt as */
   -3,  /* (51) selcollist ::= sclp scanpt STAR */
   -5,  /* (52) selcollist ::= sclp scanpt nm DOT STAR */
   -2,  /* (53) as ::= AS nm */
    0,  /* (54) as ::= */
    0,  /* (55) from ::= */
   -2,  /* (56) from ::= FROM seltablist */
   -2,  /* (57) stl_prefix ::= seltablist joinop */
    0,  /* (58) stl_prefix ::= */
   -5,  /* (59) seltablist ::= stl_prefix nm dbnm as on_using */
   -6,  /* (60) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   -8,  /* (61) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   -6,  /* (62) seltablist ::= stl_prefix LP select RP as on_using */
   -6,  /* (63) seltablist ::= stl_prefix LP seltablist RP as on_using */
    0,  /* (64) dbnm ::= */
   -2,  /* (65) dbnm ::= DOT nm */
   -1,  /* (66) fullname ::= nm */
   -3,  /* (67) fullname ::= nm DOT nm */
   -1,  /* (68) xfullname ::= nm */
   -3,  /* (69) xfullname ::= nm DOT nm */
   -5,  /* (70) xfullname ::= nm DOT nm AS nm */
   -3,  /* (71) xfullname ::= nm AS nm */
   -1,  /* (72) joinop ::= COMMA|JOIN */
   -2,  /* (73) joinop ::= JOIN_KW JOIN */
   -3,  /* (74) joinop ::= JOIN_KW nm JOIN */
   -4,  /* (75) joinop ::= JOIN_KW nm nm JOIN */
   -2,  /* (76) on_using ::= ON expr */
   -4,  /* (77) on_using ::= USING LP idlist RP */
    0,  /* (78) on_using ::= */
    0,  /* (79) indexed_opt ::= */
   -3,  /* (80) indexed_by ::= INDEXED BY nm */
   -2,  /* (81) indexed_by ::= NOT INDEXED */
    0,  /* (82) orderby_opt ::= */
   -3,  /* (83) orderby_opt ::= ORDER BY sortlist */
   -5,  /* (84) sortlist ::= sortlist COMMA expr sortorder nulls */
   -3,  /* (85) sortlist ::= expr sortorder nulls */
   -1,  /* (86) sortorder ::= ASC */
   -1,  /* (87) sortorder ::= DESC */
    0,  /* (88) sortorder ::= */
   -2,  /* (89) nulls ::= NULLS FIRST */
   -2,  /* (90) nulls ::= NULLS LAST */
    0,  /* (91) nulls ::= */
    0,  /* (92) groupby_opt ::= */
   -3,  /* (93) groupby_opt ::= GROUP BY nexprlist */
    0,  /* (94) having_opt ::= */
   -2,  /* (95) having_opt ::= HAVING expr */
    0,  /* (96) limit_opt ::= */
   -2,  /* (97) limit_opt ::= LIMIT expr */
   -4,  /* (98) limit_opt ::= LIMIT expr OFFSET expr */
   -4,  /* (99) limit_opt ::= LIMIT expr COMMA expr */
   -6,  /* (100) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
    0,  /* (101) where_opt ::= */
   -2,  /* (102) where_opt ::= WHERE expr */
    0,  /* (103) where_opt_ret ::= */
   -2,  /* (104) where_opt_ret ::= WHERE expr */
   -2,  /* (105) where_opt_ret ::= RETURNING selcollist */
   -4,  /* (106) where_opt_ret ::= WHERE expr RETURNING selcollist */
   -9,  /* (107) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   -5,  /* (108) setlist ::= setlist COMMA nm EQ expr */
   -7,  /* (109) setlist ::= setlist COMMA LP idlist RP EQ expr */
   -3,  /* (110) setlist ::= nm EQ expr */
   -5,  /* (111) setlist ::= LP idlist RP EQ expr */
   -7,  /* (112) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   -8,  /* (113) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
    0,  /* (114) upsert ::= */
   -2,  /* (115) upsert ::= RETURNING selcollist */
  -12,  /* (116) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
   -9,  /* (117) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
   -5,  /* (118) upsert ::= ON CONFLICT DO NOTHING returning */
   -8,  /* (119) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
   -2,  /* (120) returning ::= RETURNING selcollist */
   -2,  /* (121) insert_cmd ::= INSERT orconf */
   -1,  /* (122) insert_cmd ::= REPLACE */
    0,  /* (123) idlist_opt ::= */
   -3,  /* (124) idlist_opt ::= LP idlist RP */
   -3,  /* (125) idlist ::= idlist COMMA nm */
   -1,  /* (126) idlist ::= nm */
   -3,  /* (127) expr ::= LP expr RP */
   -1,  /* (128) expr ::= ID|INDEXED|JOIN_KW */
   -3,  /* (129) expr ::= nm DOT nm */
   -5,  /* (130) expr ::= nm DOT nm DOT nm */
   -1,  /* (131) term ::= NULL|FLOAT|BLOB */
   -1,  /* (132) term ::= STRING */
   -1,  /* (133) term ::= INTEGER */
   -1,  /* (134) expr ::= VARIABLE */
   -3,  /* (135) expr ::= expr COLLATE ID|STRING */
   -6,  /* (136) expr ::= CAST LP expr AS typetoken RP */
   -5,  /* (137) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   -8,  /* (138) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   -4,  /* (139) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   -1,  /* (140) term ::= CTIME_KW */
   -5,  /* (141) expr ::= LP nexprlist COMMA expr RP */
   -3,  /* (142) expr ::= expr AND expr */
   -3,  /* (143) expr ::= expr OR expr */
   -3,  /* (144) expr ::= expr LT|GT|GE|LE expr */
   -3,  /* (145) expr ::= expr EQ|NE expr */
   -3,  /* (146) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
   -3,  /* (147) expr ::= expr PLUS|MINUS expr */
   -3,  /* (148) expr ::= expr STAR|SLASH|REM expr */
   -3,  /* (149) expr ::= expr CONCAT expr */
   -2,  /* (150) likeop ::= NOT LIKE_KW|MATCH */
   -3,  /* (151) expr ::= expr likeop expr */
   -5,  /* (152) expr ::= expr likeop expr ESCAPE expr */
   -2,  /* (153) expr ::= expr ISNULL|NOTNULL */
   -3,  /* (154) expr ::= expr NOT NULL */
   -3,  /* (155) expr ::= expr IS expr */
   -4,  /* (156) expr ::= expr IS NOT expr */
   -6,  /* (157) expr ::= expr IS NOT DISTINCT FROM expr */
   -5,  /* (158) expr ::= expr IS DISTINCT FROM expr */
   -2,  /* (159) expr ::= NOT expr */
   -2,  /* (160) expr ::= BITNOT expr */
   -2,  /* (161) expr ::= PLUS|MINUS expr */
   -3,  /* (162) expr ::= expr PTR expr */
   -1,  /* (163) between_op ::= BETWEEN */
   -2,  /* (164) between_op ::= NOT BETWEEN */
   -5,  /* (165) expr ::= expr between_op expr AND expr */
   -1,  /* (166) in_op ::= IN */
   -2,  /* (167) in_op ::= NOT IN */
   -5,  /* (168) expr ::= expr in_op LP exprlist RP */
   -3,  /* (169) expr ::= LP select RP */
   -5,  /* (170) expr ::= expr in_op LP select RP */
   -5,  /* (171) expr ::= expr in_op nm dbnm paren_exprlist */
   -4,  /* (172) expr ::= EXISTS LP select RP */
   -5,  /* (173) expr ::= CASE case_operand case_exprlist case_else END */
   -5,  /* (174) case_exprlist ::= case_exprlist WHEN expr THEN expr */
   -4,  /* (175) case_exprlist ::= WHEN expr THEN expr */
   -2,  /* (176) case_else ::= ELSE expr */
    0,  /* (177) case_else ::= */
    0,  /* (178) case_operand ::= */
    0,  /* (179) exprlist ::= */
   -3,  /* (180) nexprlist ::= nexprlist COMMA expr */
   -1,  /* (181) nexprlist ::= expr */
    0,  /* (182) paren_exprlist ::= */
   -3,  /* (183) paren_exprlist ::= LP exprlist RP */
  -12,  /* (184) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
   -1,  /* (185) uniqueflag ::= UNIQUE */
    0,  /* (186) uniqueflag ::= */
    0,  /* (187) eidlist_opt ::= */
   -3,  /* (188) eidlist_opt ::= LP eidlist RP */
   -5,  /* (189) eidlist ::= eidlist COMMA nm collate sortorder */
   -3,  /* (190) eidlist ::= nm collate sortorder */
    0,  /* (191) collate ::= */
   -2,  /* (192) collate ::= COLLATE ID|STRING */
   -4,  /* (193) cmd ::= DROP INDEX ifexists fullname */
   -2,  /* (194) cmd ::= VACUUM vinto */
   -3,  /* (195) cmd ::= VACUUM nm vinto */
   -2,  /* (196) vinto ::= INTO expr */
    0,  /* (197) vinto ::= */
   -3,  /* (198) cmd ::= PRAGMA nm dbnm */
   -5,  /* (199) cmd ::= PRAGMA nm dbnm EQ nmnum */
   -6,  /* (200) cmd ::= PRAGMA nm dbnm LP nmnum RP */
   -5,  /* (201) cmd ::= PRAGMA nm dbnm EQ minus_num */
   -6,  /* (202) cmd ::= PRAGMA nm dbnm LP minus_num RP */
   -2,  /* (203) plus_num ::= PLUS INTEGER|FLOAT */
   -2,  /* (204) minus_num ::= MINUS INTEGER|FLOAT */
   -4,  /* (205) expr ::= RAISE LP IGNORE RP */
   -6,  /* (206) expr ::= RAISE LP raisetype COMMA expr RP */
   -1,  /* (207) raisetype ::= ROLLBACK */
   -1,  /* (208) raisetype ::= ABORT */
   -1,  /* (209) raisetype ::= FAIL */
   -4,  /* (210) cmd ::= DROP TRIGGER ifexists fullname */
   -6,  /* (211) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
   -3,  /* (212) cmd ::= DETACH database_kw_opt expr */
    0,  /* (213) key_opt ::= */
   -2,  /* (214) key_opt ::= KEY expr */
   -1,  /* (215) cmd ::= REINDEX */
   -3,  /* (216) cmd ::= REINDEX nm dbnm */
   -1,  /* (217) cmd ::= ANALYZE */
   -3,  /* (218) cmd ::= ANALYZE nm dbnm */
   -6,  /* (219) cmd ::= ALTER TABLE fullname RENAME TO nm */
   -7,  /* (220) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
   -6,  /* (221) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
   -1,  /* (222) add_column_fullname ::= fullname */
   -8,  /* (223) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
   -2,  /* (224) with ::= WITH wqlist */
   -3,  /* (225) with ::= WITH RECURSIVE wqlist */
   -1,  /* (226) wqas ::= AS */
   -2,  /* (227) wqas ::= AS MATERIALIZED */
   -3,  /* (228) wqas ::= AS NOT MATERIALIZED */
   -6,  /* (229) wqitem ::= withnm eidlist_opt wqas LP select RP */
   -1,  /* (230) withnm ::= nm */
   -1,  /* (231) wqlist ::= wqitem */
   -3,  /* (232) wqlist ::= wqlist COMMA wqitem */
   -1,  /* (233) term ::= QNUMBER */
   -1,  /* (234) input ::= cmdlist */
   -2,  /* (235) cmdlist ::= cmdlist ecmd */
   -1,  /* (236) cmdlist ::= ecmd */
   -1,  /* (237) ecmd ::= SEMI */
   -2,  /* (238) ecmd ::= cmdx SEMI */
   -3,  /* (239) ecmd ::= explain cmdx SEMI */
    0,  /* (240) trans_opt ::= */
   -1,  /* (241) trans_opt ::= TRANSACTION */
   -2,  /* (242) trans_opt ::= TRANSACTION nm */
   -1,  /* (243) savepoint_opt ::= SAVEPOINT */
    0,  /* (244) savepoint_opt ::= */
   -1,  /* (245) nm ::= ID|INDEXED|JOIN_KW */
   -1,  /* (246) nm ::= STRING */
   -1,  /* (247) typetoken ::= typename */
   -1,  /* (248) typename ::= ID|STRING */
   -1,  /* (249) signed ::= plus_num */
   -1,  /* (250) signed ::= minus_num */
   -1,  /* (251) resolvetype ::= raisetype */
   -1,  /* (252) selectnowith ::= oneselect */
   -1,  /* (253) oneselect ::= values */
   -2,  /* (254) sclp ::= selcollist COMMA */
   -1,  /* (255) as ::= ID|STRING */
   -1,  /* (256) indexed_opt ::= indexed_by */
    0,  /* (257) returning ::= */
   -1,  /* (258) expr ::= term */
   -1,  /* (259) likeop ::= LIKE_KW|MATCH */
   -1,  /* (260) case_operand ::= expr */
   -1,  /* (261) exprlist ::= nexprlist */
   -1,  /* (262) nmnum ::= plus_num */
   -1,  /* (263) nmnum ::= nm */
   -1,  /* (264) nmnum ::= ON */
   -1,  /* (265) nmnum ::= DELETE */
   -1,  /* (266) nmnum ::= DEFAULT */
   -1,  /* (267) plus_num ::= INTEGER|FLOAT */
   -1,  /* (268) database_kw_opt ::= DATABASE */
    0,  /* (269) database_kw_opt ::= */
    0,  /* (270) kwcolumn_opt ::= */
   -1,  /* (271) kwcolumn_opt ::= COLUMNKW */
    0,  /* (272) with ::= */
   -6,  /* (273) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   -5,  /* (274) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   -2,  /* (275) create_table_args ::= AS select */
    0,  /* (276) table_option_set ::= */
   -3,  /* (277) table_option_set ::= table_option_set COMMA table_option */
   -2,  /* (278) table_option ::= WITHOUT nm */
   -1,  /* (279) table_option ::= nm */
    0,  /* (280) scantok ::= */
   -2,  /* (281) ccons ::= CONSTRAINT nm */
   -3,  /* (282) ccons ::= DEFAULT scantok term */
   -4,  /* (283) ccons ::= DEFAULT LP expr RP */
   -4,  /* (284) ccons ::= DEFAULT PLUS scantok term */
   -4,  /* (285) ccons ::= DEFAULT MINUS scantok term */
   -3,  /* (286) ccons ::= DEFAULT scantok ID|INDEXED */
   -3,  /* (287) ccons ::= NOT NULL onconf */
   -5,  /* (288) ccons ::= PRIMARY KEY sortorder onconf autoinc */
   -2,  /* (289) ccons ::= UNIQUE onconf */
   -4,  /* (290) ccons ::= CHECK LP expr RP */
   -4,  /* (291) ccons ::= REFERENCES nm eidlist_opt refargs */
   -1,  /* (292) ccons ::= defer_subclause */
   -2,  /* (293) ccons ::= COLLATE ID|STRING */
   -3,  /* (294) generated ::= LP expr RP */
   -4,  /* (295) generated ::= LP expr RP ID */
    0,  /* (296) autoinc ::= */
   -1,  /* (297) autoinc ::= AUTOINCR */
    0,  /* (298) refargs ::= */
   -2,  /* (299) refargs ::= refargs refarg */
   -2,  /* (300) refarg ::= MATCH nm */
   -3,  /* (301) refarg ::= ON INSERT refact */
   -3,  /* (302) refarg ::= ON DELETE refact */
   -3,  /* (303) refarg ::= ON UPDATE refact */
   -2,  /* (304) refact ::= SET NULL */
   -2,  /* (305) refact ::= SET DEFAULT */
   -1,  /* (306) refact ::= CASCADE */
   -1,  /* (307) refact ::= RESTRICT */
   -2,  /* (308) refact ::= NO ACTION */
   -3,  /* (309) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
   -2,  /* (310) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
    0,  /* (311) init_deferred_pred_opt ::= */
   -2,  /* (312) init_deferred_pred_opt ::= INITIALLY DEFERRED */
   -2,  /* (313) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
    0,  /* (314) conslist_opt ::= */
   -1,  /* (315) tconscomma ::= COMMA */
   -2,  /* (316) tcons ::= CONSTRAINT nm */
   -7,  /* (317) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
   -5,  /* (318) tcons ::= UNIQUE LP sortlist RP onconf */
   -5,  /* (319) tcons ::= CHECK LP expr RP onconf */
  -10,  /* (320) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
    0,  /* (321) defer_subclause_opt ::= */
    0,  /* (322) onconf ::= */
   -3,  /* (323) onconf ::= ON CONFLICT resolvetype */
  -10,  /* (324) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   -6,  /* (325) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   -9,  /* (326) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   -5,  /* (327) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   -5,  /* (328) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
  -11,  /* (329) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
   -1,  /* (330) trigger_time ::= BEFORE|AFTER */
   -2,  /* (331) trigger_time ::= INSTEAD OF */
    0,  /* (332) trigger_time ::= */
   -1,  /* (333) trigger_event ::= DELETE|INSERT */
   -1,  /* (334) trigger_event ::= UPDATE */
   -3,  /* (335) trigger_event ::= UPDATE OF idlist */
    0,  /* (336) when_clause ::= */
   -2,  /* (337) when_clause ::= WHEN expr */
   -3,  /* (338) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
   -2,  /* (339) trigger_cmd_list ::= trigger_cmd SEMI */
   -3,  /* (340) trnm ::= nm DOT nm */
   -3,  /* (341) tridxby ::= INDEXED BY nm */
   -2,  /* (342) tridxby ::= NOT INDEXED */
   -9,  /* (343) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
   -8,  /* (344) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
   -6,  /* (345) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
   -3,  /* (346) trigger_cmd ::= scanpt select scanpt */
   -1,  /* (347) cmd ::= create_vtab */
   -4,  /* (348) cmd ::= create_vtab LP vtabarglist RP */
   -8,  /* (349) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
    0,  /* (350) vtabarg ::= */
   -1,  /* (351) vtabargtoken ::= ANY */
   -3,  /* (352) vtabargtoken ::= lp anylist RP */
   -1,  /* (353) lp ::= LP */
   -3,  /* (354) windowdefn_list ::= windowdefn_list COMMA windowdefn */
   -5,  /* (355) windowdefn ::= nm AS LP window RP */
   -5,  /* (356) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
   -6,  /* (357) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
   -4,  /* (358) window ::= ORDER BY sortlist frame_opt */
   -5,  /* (359) window ::= nm ORDER BY sortlist frame_opt */
   -2,  /* (360) window ::= nm frame_opt */
    0,  /* (361) frame_opt ::= */
   -3,  /* (362) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
   -6,  /* (363) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
   -1,  /* (364) range_or_rows ::= RANGE|ROWS|GROUPS */
   -1,  /* (365) frame_bound_s ::= frame_bound */
   -2,  /* (366) frame_bound_s ::= UNBOUNDED PRECEDING */
   -1,  /* (367) frame_bound_e ::= frame_bound */
   -2,  /* (368) frame_bound_e ::= UNBOUNDED FOLLOWING */
   -2,  /* (369) frame_bound ::= expr PRECEDING|FOLLOWING */
   -2,  /* (370) frame_bound ::= CURRENT ROW */
    0,  /* (371) frame_exclude_opt ::= */
   -2,  /* (372) frame_exclude_opt ::= EXCLUDE frame_exclude */
   -2,  /* (373) frame_exclude ::= NO OTHERS */
   -2,  /* (374) frame_exclude ::= CURRENT ROW */
   -1,  /* (375) frame_exclude ::= GROUP|TIES */
   -2,  /* (376) window_clause ::= WINDOW windowdefn_list */
   -2,  /* (377) filter_over ::= filter_clause over_clause */
   -1,  /* (378) filter_over ::= over_clause */
   -1,  /* (379) filter_over ::= filter_clause */
   -4,  /* (380) over_clause ::= OVER LP window RP */
   -2,  /* (381) over_clause ::= OVER nm */
   -5,  /* (382) filter_clause ::= FILTER LP WHERE expr RP */
   -2,  /* (383) cmd ::= create_table create_table_args */
   -1,  /* (384) table_option_set ::= table_option */
   -4,  /* (385) columnlist ::= columnlist COMMA columnname carglist */
   -2,  /* (386) columnlist ::= columnname carglist */
   -2,  /* (387) carglist ::= carglist ccons */
    0,  /* (388) carglist ::= */
   -2,  /* (389) ccons ::= NULL onconf */
   -4,  /* (390) ccons ::= GENERATED ALWAYS AS generated */
   -2,  /* (391) ccons ::= AS generated */
   -2,  /* (392) conslist_opt ::= COMMA conslist */
   -3,  /* (393) conslist ::= conslist tconscomma tcons */
   -1,  /* (394) conslist ::= tcons */
    0,  /* (395) tconscomma ::= */
   -1,  /* (396) defer_subclause_opt ::= defer_subclause */
    0,  /* (397) foreach_clause ::= */
   -3,  /* (398) foreach_clause ::= FOR EACH ROW */
   -1,  /* (399) trnm ::= nm */
    0,  /* (400) tridxby ::= */
   -1,  /* (401) vtabarglist ::= vtabarg */
   -3,  /* (402) vtabarglist ::= vtabarglist COMMA vtabarg */
   -2,  /* (403) vtabarg ::= vtabarg vtabargtoken */
    0,  /* (404) anylist ::= */
   -4,  /* (405) anylist ::= anylist LP anylist RP */
   -2,  /* (406) anylist ::= anylist ANY */
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
      case 0: /* explain ::= EXPLAIN */
      case 16: /* temp ::= TEMP */ yytestcase(yyruleno==16);
      case 185: /* uniqueflag ::= UNIQUE */ yytestcase(yyruleno==185);
      case 271: /* kwcolumn_opt ::= COLUMNKW */ yytestcase(yyruleno==271);
{
    yymsp[0].minor.yy144 = 1;
}
        break;
      case 1: /* explain ::= EXPLAIN QUERY PLAN */
      case 228: /* wqas ::= AS NOT MATERIALIZED */ yytestcase(yyruleno==228);
{
    yymsp[-2].minor.yy144 = 2;
}
        break;
      case 2: /* cmdx ::= cmd */
      case 33: /* cmd ::= select */ yytestcase(yyruleno==33);
      case 36: /* select ::= selectnowith */ yytestcase(yyruleno==36);
      case 236: /* cmdlist ::= ecmd */ yytestcase(yyruleno==236);
      case 252: /* selectnowith ::= oneselect */ yytestcase(yyruleno==252);
      case 258: /* expr ::= term */ yytestcase(yyruleno==258);
      case 260: /* case_operand ::= expr */ yytestcase(yyruleno==260);
      case 261: /* exprlist ::= nexprlist */ yytestcase(yyruleno==261);
{
    yylhsminor.yy391 = yymsp[0].minor.yy391;
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 3: /* cmd ::= BEGIN transtype trans_opt */
{
    yymsp[-2].minor.yy391 = ast_transaction_stmt(pCtx->astCtx,
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
    yymsp[-1].minor.yy391 = ast_transaction_stmt(pCtx->astCtx,
        SYNTAQLITE_TRANSACTION_OP_COMMIT,
        SYNTAQLITE_TRANSACTION_TYPE_DEFERRED);
}
        break;
      case 9: /* cmd ::= ROLLBACK trans_opt */
{
    yymsp[-1].minor.yy391 = ast_transaction_stmt(pCtx->astCtx,
        SYNTAQLITE_TRANSACTION_OP_ROLLBACK,
        SYNTAQLITE_TRANSACTION_TYPE_DEFERRED);
}
        break;
      case 10: /* cmd ::= SAVEPOINT nm */
{
    yymsp[-1].minor.yy391 = ast_savepoint_stmt(pCtx->astCtx,
        SYNTAQLITE_SAVEPOINT_OP_SAVEPOINT,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
        break;
      case 11: /* cmd ::= RELEASE savepoint_opt nm */
{
    yymsp[-2].minor.yy391 = ast_savepoint_stmt(pCtx->astCtx,
        SYNTAQLITE_SAVEPOINT_OP_RELEASE,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
        break;
      case 12: /* cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
{
    yymsp[-4].minor.yy391 = ast_savepoint_stmt(pCtx->astCtx,
        SYNTAQLITE_SAVEPOINT_OP_ROLLBACK_TO,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
        break;
      case 13: /* createkw ::= CREATE */
      case 249: /* signed ::= plus_num */ yytestcase(yyruleno==249);
      case 250: /* signed ::= minus_num */ yytestcase(yyruleno==250);
      case 256: /* indexed_opt ::= indexed_by */ yytestcase(yyruleno==256);
      case 262: /* nmnum ::= plus_num */ yytestcase(yyruleno==262);
      case 263: /* nmnum ::= nm */ yytestcase(yyruleno==263);
      case 264: /* nmnum ::= ON */ yytestcase(yyruleno==264);
      case 265: /* nmnum ::= DELETE */ yytestcase(yyruleno==265);
      case 266: /* nmnum ::= DEFAULT */ yytestcase(yyruleno==266);
      case 267: /* plus_num ::= INTEGER|FLOAT */ yytestcase(yyruleno==267);
{
    // Token passthrough
}
        break;
      case 14: /* ifnotexists ::= */
      case 17: /* temp ::= */ yytestcase(yyruleno==17);
      case 30: /* ifexists ::= */ yytestcase(yyruleno==30);
      case 186: /* uniqueflag ::= */ yytestcase(yyruleno==186);
      case 191: /* collate ::= */ yytestcase(yyruleno==191);
      case 240: /* trans_opt ::= */ yytestcase(yyruleno==240);
      case 244: /* savepoint_opt ::= */ yytestcase(yyruleno==244);
      case 270: /* kwcolumn_opt ::= */ yytestcase(yyruleno==270);
{
    yymsp[1].minor.yy144 = 0;
}
        break;
      case 15: /* ifnotexists ::= IF NOT EXISTS */
{
    yymsp[-2].minor.yy144 = 1;
}
        break;
      case 18: /* columnname ::= nm typetoken */
      case 230: /* withnm ::= nm */ yytestcase(yyruleno==230);
{
    // Token passthrough - nm already produces SyntaqliteToken
}
        break;
      case 19: /* typetoken ::= */
{
    yymsp[1].minor.yy0.n = 0; yymsp[1].minor.yy0.z = 0;
}
        break;
      case 20: /* typetoken ::= typename LP signed RP */
{
    yymsp[-3].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-3].minor.yy0.z);
}
        break;
      case 21: /* typetoken ::= typename LP signed COMMA signed RP */
{
    yymsp[-5].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-5].minor.yy0.z);
}
        break;
      case 22: /* typename ::= typename ID|STRING */
{
    yymsp[-1].minor.yy0.n = yymsp[0].minor.yy0.n + (int)(yymsp[0].minor.yy0.z - yymsp[-1].minor.yy0.z);
}
        break;
      case 23: /* scanpt ::= */
      case 54: /* as ::= */ yytestcase(yyruleno==54);
      case 79: /* indexed_opt ::= */ yytestcase(yyruleno==79);
{
    yymsp[1].minor.yy0.z = NULL; yymsp[1].minor.yy0.n = 0;
}
        break;
      case 24: /* orconf ::= */
{
    yymsp[1].minor.yy144 = (int)SYNTAQLITE_CONFLICT_ACTION_DEFAULT;
}
        break;
      case 25: /* orconf ::= OR resolvetype */
      case 121: /* insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==121);
{
    yymsp[-1].minor.yy144 = yymsp[0].minor.yy144;
}
        break;
      case 26: /* resolvetype ::= IGNORE */
{
    yymsp[0].minor.yy144 = (int)SYNTAQLITE_CONFLICT_ACTION_IGNORE;
}
        break;
      case 27: /* resolvetype ::= REPLACE */
      case 122: /* insert_cmd ::= REPLACE */ yytestcase(yyruleno==122);
{
    yymsp[0].minor.yy144 = (int)SYNTAQLITE_CONFLICT_ACTION_REPLACE;
}
        break;
      case 28: /* cmd ::= DROP TABLE ifexists fullname */
{
    yymsp[-3].minor.yy391 = ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TABLE, (uint8_t)yymsp[-1].minor.yy144, yymsp[0].minor.yy391);
}
        break;
      case 29: /* ifexists ::= IF EXISTS */
      case 192: /* collate ::= COLLATE ID|STRING */ yytestcase(yyruleno==192);
      case 227: /* wqas ::= AS MATERIALIZED */ yytestcase(yyruleno==227);
{
    yymsp[-1].minor.yy144 = 1;
}
        break;
      case 31: /* cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
{
    SyntaqliteSourceSpan view_name = yymsp[-3].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-3].minor.yy0) : syntaqlite_span(pCtx, yymsp[-4].minor.yy0);
    SyntaqliteSourceSpan view_schema = yymsp[-3].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-4].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-8].minor.yy391 = ast_create_view_stmt(pCtx->astCtx,
        view_name,
        view_schema,
        (uint8_t)yymsp[-7].minor.yy144,
        (uint8_t)yymsp[-5].minor.yy144,
        yymsp[-2].minor.yy391,
        yymsp[0].minor.yy391);
}
        break;
      case 32: /* cmd ::= DROP VIEW ifexists fullname */
{
    yymsp[-3].minor.yy391 = ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_VIEW, (uint8_t)yymsp[-1].minor.yy144, yymsp[0].minor.yy391);
}
        break;
      case 34: /* select ::= WITH wqlist selectnowith */
{
    yymsp[-2].minor.yy391 = ast_with_clause(pCtx->astCtx, 0, yymsp[-1].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 35: /* select ::= WITH RECURSIVE wqlist selectnowith */
{
    yymsp[-3].minor.yy391 = ast_with_clause(pCtx->astCtx, 1, yymsp[-1].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 37: /* selectnowith ::= selectnowith multiselect_op oneselect */
{
    yymsp[-2].minor.yy391 = ast_compound_select(pCtx->astCtx, (uint8_t)yymsp[-1].minor.yy144, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 38: /* multiselect_op ::= UNION */
{ yylhsminor.yy144 = 0; UNUSED_PARAMETER(yymsp[0].minor.yy0); }
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 39: /* multiselect_op ::= UNION ALL */
      case 167: /* in_op ::= NOT IN */ yytestcase(yyruleno==167);
{ yymsp[-1].minor.yy144 = 1; }
        break;
      case 40: /* multiselect_op ::= EXCEPT|INTERSECT */
{
    yylhsminor.yy144 = (yymsp[0].minor.yy0.type == TK_INTERSECT) ? 2 : 3;
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 41: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
{
    yymsp[-8].minor.yy391 = ast_select_stmt(pCtx->astCtx, (uint8_t)yymsp[-7].minor.yy391, yymsp[-6].minor.yy391, yymsp[-5].minor.yy391, yymsp[-4].minor.yy391, yymsp[-3].minor.yy391, yymsp[-2].minor.yy391, yymsp[-1].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 42: /* values ::= VALUES LP nexprlist RP */
{
    yymsp[-3].minor.yy391 = ast_values_row_list(pCtx->astCtx, yymsp[-1].minor.yy391);
}
        break;
      case 43: /* oneselect ::= mvalues */
      case 253: /* oneselect ::= values */ yytestcase(yyruleno==253);
{
    yylhsminor.yy391 = ast_values_clause(pCtx->astCtx, yymsp[0].minor.yy391);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 44: /* mvalues ::= values COMMA LP nexprlist RP */
      case 45: /* mvalues ::= mvalues COMMA LP nexprlist RP */ yytestcase(yyruleno==45);
{
    yymsp[-4].minor.yy391 = ast_values_row_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, yymsp[-1].minor.yy391);
}
        break;
      case 46: /* distinct ::= DISTINCT */
      case 87: /* sortorder ::= DESC */ yytestcase(yyruleno==87);
{
    yymsp[0].minor.yy391 = 1;
}
        break;
      case 47: /* distinct ::= ALL */
      case 86: /* sortorder ::= ASC */ yytestcase(yyruleno==86);
      case 163: /* between_op ::= BETWEEN */ yytestcase(yyruleno==163);
{
    yymsp[0].minor.yy391 = 0;
}
        break;
      case 48: /* distinct ::= */
      case 88: /* sortorder ::= */ yytestcase(yyruleno==88);
      case 91: /* nulls ::= */ yytestcase(yyruleno==91);
{
    yymsp[1].minor.yy391 = 0;
}
        break;
      case 49: /* sclp ::= */
      case 55: /* from ::= */ yytestcase(yyruleno==55);
      case 58: /* stl_prefix ::= */ yytestcase(yyruleno==58);
      case 78: /* on_using ::= */ yytestcase(yyruleno==78);
      case 82: /* orderby_opt ::= */ yytestcase(yyruleno==82);
      case 92: /* groupby_opt ::= */ yytestcase(yyruleno==92);
      case 94: /* having_opt ::= */ yytestcase(yyruleno==94);
      case 96: /* limit_opt ::= */ yytestcase(yyruleno==96);
      case 101: /* where_opt ::= */ yytestcase(yyruleno==101);
      case 103: /* where_opt_ret ::= */ yytestcase(yyruleno==103);
      case 114: /* upsert ::= */ yytestcase(yyruleno==114);
      case 123: /* idlist_opt ::= */ yytestcase(yyruleno==123);
      case 177: /* case_else ::= */ yytestcase(yyruleno==177);
      case 178: /* case_operand ::= */ yytestcase(yyruleno==178);
      case 179: /* exprlist ::= */ yytestcase(yyruleno==179);
      case 187: /* eidlist_opt ::= */ yytestcase(yyruleno==187);
      case 197: /* vinto ::= */ yytestcase(yyruleno==197);
      case 213: /* key_opt ::= */ yytestcase(yyruleno==213);
      case 272: /* with ::= */ yytestcase(yyruleno==272);
{
    yymsp[1].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 50: /* selcollist ::= sclp scanpt expr scanpt as */
{
    uint32_t col = ast_result_column(pCtx->astCtx, 0, SYNTAQLITE_NO_SPAN, yymsp[-2].minor.yy391);
    yylhsminor.yy391 = ast_result_column_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, col);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 51: /* selcollist ::= sclp scanpt STAR */
{
    uint32_t col = ast_result_column(pCtx->astCtx, 1, SYNTAQLITE_NO_SPAN, SYNTAQLITE_NULL_NODE);
    yylhsminor.yy391 = ast_result_column_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, col);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 52: /* selcollist ::= sclp scanpt nm DOT STAR */
{
    uint32_t col = ast_result_column(pCtx->astCtx, 1, syntaqlite_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NULL_NODE);
    yylhsminor.yy391 = ast_result_column_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, col);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 53: /* as ::= AS nm */
      case 203: /* plus_num ::= PLUS INTEGER|FLOAT */ yytestcase(yyruleno==203);
{
    yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;
}
        break;
      case 56: /* from ::= FROM seltablist */
      case 76: /* on_using ::= ON expr */ yytestcase(yyruleno==76);
      case 95: /* having_opt ::= HAVING expr */ yytestcase(yyruleno==95);
      case 102: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==102);
      case 104: /* where_opt_ret ::= WHERE expr */ yytestcase(yyruleno==104);
      case 176: /* case_else ::= ELSE expr */ yytestcase(yyruleno==176);
      case 196: /* vinto ::= INTO expr */ yytestcase(yyruleno==196);
      case 214: /* key_opt ::= KEY expr */ yytestcase(yyruleno==214);
      case 224: /* with ::= WITH wqlist */ yytestcase(yyruleno==224);
{
    yymsp[-1].minor.yy391 = yymsp[0].minor.yy391;
}
        break;
      case 57: /* stl_prefix ::= seltablist joinop */
{
    yymsp[-1].minor.yy391 = ast_join_prefix(pCtx->astCtx, yymsp[-1].minor.yy391, (SyntaqliteJoinType)yymsp[0].minor.yy144);
}
        break;
      case 59: /* seltablist ::= stl_prefix nm dbnm as on_using */
{
    SyntaqliteSourceSpan alias = (yymsp[-1].minor.yy0.z != NULL) ? syntaqlite_span(pCtx, yymsp[-1].minor.yy0) : SYNTAQLITE_NO_SPAN;
    SyntaqliteSourceSpan table_name;
    SyntaqliteSourceSpan schema;
    if (yymsp[-2].minor.yy0.z != NULL) {
        table_name = syntaqlite_span(pCtx, yymsp[-2].minor.yy0);
        schema = syntaqlite_span(pCtx, yymsp[-3].minor.yy0);
    } else {
        table_name = syntaqlite_span(pCtx, yymsp[-3].minor.yy0);
        schema = SYNTAQLITE_NO_SPAN;
    }
    uint32_t tref = ast_table_ref(pCtx->astCtx, table_name, schema, alias);
    if (yymsp[-4].minor.yy391 == SYNTAQLITE_NULL_NODE) {
        yymsp[-4].minor.yy391 = tref;
    } else {
        SyntaqliteNode *pfx = AST_NODE(pCtx->astCtx->ast, yymsp[-4].minor.yy391);
        uint32_t on_expr = SYNTAQLITE_NULL_NODE;
        uint32_t using_cols = SYNTAQLITE_NULL_NODE;
        if (yymsp[0].minor.yy391 != SYNTAQLITE_NULL_NODE) {
            SyntaqliteNode *n = AST_NODE(pCtx->astCtx->ast, yymsp[0].minor.yy391);
            if (n->tag == SYNTAQLITE_NODE_EXPR_LIST) {
                using_cols = yymsp[0].minor.yy391;
            } else {
                on_expr = yymsp[0].minor.yy391;
            }
        }
        yymsp[-4].minor.yy391 = ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            tref, on_expr, using_cols);
    }
}
        break;
      case 60: /* seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
{
    (void)yymsp[-1].minor.yy0;
    SyntaqliteSourceSpan alias = (yymsp[-2].minor.yy0.z != NULL) ? syntaqlite_span(pCtx, yymsp[-2].minor.yy0) : SYNTAQLITE_NO_SPAN;
    SyntaqliteSourceSpan table_name;
    SyntaqliteSourceSpan schema;
    if (yymsp[-3].minor.yy0.z != NULL) {
        table_name = syntaqlite_span(pCtx, yymsp[-3].minor.yy0);
        schema = syntaqlite_span(pCtx, yymsp[-4].minor.yy0);
    } else {
        table_name = syntaqlite_span(pCtx, yymsp[-4].minor.yy0);
        schema = SYNTAQLITE_NO_SPAN;
    }
    uint32_t tref = ast_table_ref(pCtx->astCtx, table_name, schema, alias);
    if (yymsp[-5].minor.yy391 == SYNTAQLITE_NULL_NODE) {
        yymsp[-5].minor.yy391 = tref;
    } else {
        SyntaqliteNode *pfx = AST_NODE(pCtx->astCtx->ast, yymsp[-5].minor.yy391);
        uint32_t on_expr = SYNTAQLITE_NULL_NODE;
        uint32_t using_cols = SYNTAQLITE_NULL_NODE;
        if (yymsp[0].minor.yy391 != SYNTAQLITE_NULL_NODE) {
            SyntaqliteNode *n = AST_NODE(pCtx->astCtx->ast, yymsp[0].minor.yy391);
            if (n->tag == SYNTAQLITE_NODE_EXPR_LIST) {
                using_cols = yymsp[0].minor.yy391;
            } else {
                on_expr = yymsp[0].minor.yy391;
            }
        }
        yymsp[-5].minor.yy391 = ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            tref, on_expr, using_cols);
    }
}
        break;
      case 61: /* seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
{
    (void)yymsp[-3].minor.yy391;
    SyntaqliteSourceSpan alias = (yymsp[-1].minor.yy0.z != NULL) ? syntaqlite_span(pCtx, yymsp[-1].minor.yy0) : SYNTAQLITE_NO_SPAN;
    SyntaqliteSourceSpan table_name;
    SyntaqliteSourceSpan schema;
    if (yymsp[-5].minor.yy0.z != NULL) {
        table_name = syntaqlite_span(pCtx, yymsp[-5].minor.yy0);
        schema = syntaqlite_span(pCtx, yymsp[-6].minor.yy0);
    } else {
        table_name = syntaqlite_span(pCtx, yymsp[-6].minor.yy0);
        schema = SYNTAQLITE_NO_SPAN;
    }
    uint32_t tref = ast_table_ref(pCtx->astCtx, table_name, schema, alias);
    if (yymsp[-7].minor.yy391 == SYNTAQLITE_NULL_NODE) {
        yymsp[-7].minor.yy391 = tref;
    } else {
        SyntaqliteNode *pfx = AST_NODE(pCtx->astCtx->ast, yymsp[-7].minor.yy391);
        uint32_t on_expr = SYNTAQLITE_NULL_NODE;
        uint32_t using_cols = SYNTAQLITE_NULL_NODE;
        if (yymsp[0].minor.yy391 != SYNTAQLITE_NULL_NODE) {
            SyntaqliteNode *n = AST_NODE(pCtx->astCtx->ast, yymsp[0].minor.yy391);
            if (n->tag == SYNTAQLITE_NODE_EXPR_LIST) {
                using_cols = yymsp[0].minor.yy391;
            } else {
                on_expr = yymsp[0].minor.yy391;
            }
        }
        yymsp[-7].minor.yy391 = ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            tref, on_expr, using_cols);
    }
}
        break;
      case 62: /* seltablist ::= stl_prefix LP select RP as on_using */
{
    SyntaqliteSourceSpan alias = (yymsp[-1].minor.yy0.z != NULL) ? syntaqlite_span(pCtx, yymsp[-1].minor.yy0) : SYNTAQLITE_NO_SPAN;
    uint32_t sub = ast_subquery_table_source(pCtx->astCtx, yymsp[-3].minor.yy391, alias);
    if (yymsp[-5].minor.yy391 == SYNTAQLITE_NULL_NODE) {
        yymsp[-5].minor.yy391 = sub;
    } else {
        SyntaqliteNode *pfx = AST_NODE(pCtx->astCtx->ast, yymsp[-5].minor.yy391);
        uint32_t on_expr = SYNTAQLITE_NULL_NODE;
        uint32_t using_cols = SYNTAQLITE_NULL_NODE;
        if (yymsp[0].minor.yy391 != SYNTAQLITE_NULL_NODE) {
            SyntaqliteNode *n = AST_NODE(pCtx->astCtx->ast, yymsp[0].minor.yy391);
            if (n->tag == SYNTAQLITE_NODE_EXPR_LIST) {
                using_cols = yymsp[0].minor.yy391;
            } else {
                on_expr = yymsp[0].minor.yy391;
            }
        }
        yymsp[-5].minor.yy391 = ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            sub, on_expr, using_cols);
    }
}
        break;
      case 63: /* seltablist ::= stl_prefix LP seltablist RP as on_using */
{
    (void)yymsp[-1].minor.yy0;
    if (yymsp[-5].minor.yy391 == SYNTAQLITE_NULL_NODE && yymsp[0].minor.yy391 == SYNTAQLITE_NULL_NODE) {
        yymsp[-5].minor.yy391 = yymsp[-3].minor.yy391;
    } else if (yymsp[-5].minor.yy391 == SYNTAQLITE_NULL_NODE) {
        yymsp[-5].minor.yy391 = yymsp[-3].minor.yy391;
    } else {
        SyntaqliteNode *pfx = AST_NODE(pCtx->astCtx->ast, yymsp[-5].minor.yy391);
        uint32_t on_expr = SYNTAQLITE_NULL_NODE;
        uint32_t using_cols = SYNTAQLITE_NULL_NODE;
        if (yymsp[0].minor.yy391 != SYNTAQLITE_NULL_NODE) {
            SyntaqliteNode *n = AST_NODE(pCtx->astCtx->ast, yymsp[0].minor.yy391);
            if (n->tag == SYNTAQLITE_NODE_EXPR_LIST) {
                using_cols = yymsp[0].minor.yy391;
            } else {
                on_expr = yymsp[0].minor.yy391;
            }
        }
        yymsp[-5].minor.yy391 = ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            yymsp[-3].minor.yy391, on_expr, using_cols);
    }
}
        break;
      case 64: /* dbnm ::= */
{ yymsp[1].minor.yy0.z = NULL; yymsp[1].minor.yy0.n = 0; }
        break;
      case 65: /* dbnm ::= DOT nm */
{ yymsp[-1].minor.yy0 = yymsp[0].minor.yy0; }
        break;
      case 66: /* fullname ::= nm */
{
    yylhsminor.yy391 = ast_qualified_name(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        SYNTAQLITE_NO_SPAN);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 67: /* fullname ::= nm DOT nm */
{
    yylhsminor.yy391 = ast_qualified_name(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0));
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 68: /* xfullname ::= nm */
{
    yylhsminor.yy391 = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 69: /* xfullname ::= nm DOT nm */
{
    yylhsminor.yy391 = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0), syntaqlite_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NO_SPAN);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 70: /* xfullname ::= nm DOT nm AS nm */
{
    yylhsminor.yy391 = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0), syntaqlite_span(pCtx, yymsp[-4].minor.yy0), syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 71: /* xfullname ::= nm AS nm */
{
    yylhsminor.yy391 = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NO_SPAN, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 72: /* joinop ::= COMMA|JOIN */
{
    yylhsminor.yy144 = (yymsp[0].minor.yy0.type == TK_COMMA)
        ? (int)SYNTAQLITE_JOIN_TYPE_COMMA
        : (int)SYNTAQLITE_JOIN_TYPE_INNER;
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 73: /* joinop ::= JOIN_KW JOIN */
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
      case 74: /* joinop ::= JOIN_KW nm JOIN */
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
      case 75: /* joinop ::= JOIN_KW nm nm JOIN */
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
      case 77: /* on_using ::= USING LP idlist RP */
{
    yymsp[-3].minor.yy391 = yymsp[-1].minor.yy391;
}
        break;
      case 80: /* indexed_by ::= INDEXED BY nm */
{
    yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;
}
        break;
      case 81: /* indexed_by ::= NOT INDEXED */
{
    yymsp[-1].minor.yy0.z = NULL; yymsp[-1].minor.yy0.n = 1;
}
        break;
      case 83: /* orderby_opt ::= ORDER BY sortlist */
      case 93: /* groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==93);
{
    yymsp[-2].minor.yy391 = yymsp[0].minor.yy391;
}
        break;
      case 84: /* sortlist ::= sortlist COMMA expr sortorder nulls */
{
    uint32_t term = ast_ordering_term(pCtx->astCtx, yymsp[-2].minor.yy391, (uint8_t)yymsp[-1].minor.yy391, (uint8_t)yymsp[0].minor.yy391);
    yylhsminor.yy391 = ast_order_by_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, term);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 85: /* sortlist ::= expr sortorder nulls */
{
    uint32_t term = ast_ordering_term(pCtx->astCtx, yymsp[-2].minor.yy391, (uint8_t)yymsp[-1].minor.yy391, (uint8_t)yymsp[0].minor.yy391);
    yylhsminor.yy391 = ast_order_by_list(pCtx->astCtx, term);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 89: /* nulls ::= NULLS FIRST */
      case 164: /* between_op ::= NOT BETWEEN */ yytestcase(yyruleno==164);
{
    yymsp[-1].minor.yy391 = 1;
}
        break;
      case 90: /* nulls ::= NULLS LAST */
{
    yymsp[-1].minor.yy391 = 2;
}
        break;
      case 97: /* limit_opt ::= LIMIT expr */
{
    yymsp[-1].minor.yy391 = ast_limit_clause(pCtx->astCtx, yymsp[0].minor.yy391, SYNTAQLITE_NULL_NODE);
}
        break;
      case 98: /* limit_opt ::= LIMIT expr OFFSET expr */
{
    yymsp[-3].minor.yy391 = ast_limit_clause(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 99: /* limit_opt ::= LIMIT expr COMMA expr */
{
    yymsp[-3].minor.yy391 = ast_limit_clause(pCtx->astCtx, yymsp[0].minor.yy391, yymsp[-2].minor.yy391);
}
        break;
      case 100: /* cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
{
    (void)yymsp[-1].minor.yy0;
    uint32_t del = ast_delete_stmt(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
    if (yymsp[-5].minor.yy391 != SYNTAQLITE_NULL_NODE) {
        // Check if yymsp[-5].minor.yy391 is a WithClause (recursive WITH) or a CteList (non-recursive WITH)
        SyntaqliteNode *wn = AST_NODE(pCtx->astCtx->ast, yymsp[-5].minor.yy391);
        if (wn->tag == SYNTAQLITE_NODE_WITH_CLAUSE) {
            // Recursive WITH - extract ctes from the wrapper
            yylhsminor.yy391 = ast_with_clause(pCtx->astCtx, 1, wn->with_clause.ctes, del);
        } else {
            // Non-recursive WITH - yymsp[-5].minor.yy391 is a CteList directly
            yylhsminor.yy391 = ast_with_clause(pCtx->astCtx, 0, yymsp[-5].minor.yy391, del);
        }
    } else {
        yylhsminor.yy391 = del;
    }
}
  yymsp[-5].minor.yy391 = yylhsminor.yy391;
        break;
      case 105: /* where_opt_ret ::= RETURNING selcollist */
{
    // Ignore RETURNING clause for now (just discard the column list)
    (void)yymsp[0].minor.yy391;
    yymsp[-1].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 106: /* where_opt_ret ::= WHERE expr RETURNING selcollist */
{
    // Keep WHERE, ignore RETURNING
    (void)yymsp[0].minor.yy391;
    yymsp[-3].minor.yy391 = yymsp[-2].minor.yy391;
}
        break;
      case 107: /* cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
{
    (void)yymsp[-4].minor.yy0;
    uint32_t upd = ast_update_stmt(pCtx->astCtx, (SyntaqliteConflictAction)yymsp[-6].minor.yy144, yymsp[-5].minor.yy391, yymsp[-2].minor.yy391, yymsp[-1].minor.yy391, yymsp[0].minor.yy391);
    if (yymsp[-8].minor.yy391 != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *wn = AST_NODE(pCtx->astCtx->ast, yymsp[-8].minor.yy391);
        if (wn->tag == SYNTAQLITE_NODE_WITH_CLAUSE) {
            yylhsminor.yy391 = ast_with_clause(pCtx->astCtx, 1, wn->with_clause.ctes, upd);
        } else {
            yylhsminor.yy391 = ast_with_clause(pCtx->astCtx, 0, yymsp[-8].minor.yy391, upd);
        }
    } else {
        yylhsminor.yy391 = upd;
    }
}
  yymsp[-8].minor.yy391 = yylhsminor.yy391;
        break;
      case 108: /* setlist ::= setlist COMMA nm EQ expr */
{
    uint32_t clause = ast_set_clause(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy391);
    yylhsminor.yy391 = ast_set_clause_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, clause);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 109: /* setlist ::= setlist COMMA LP idlist RP EQ expr */
{
    uint32_t clause = ast_set_clause(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, yymsp[-3].minor.yy391, yymsp[0].minor.yy391);
    yylhsminor.yy391 = ast_set_clause_list_append(pCtx->astCtx, yymsp[-6].minor.yy391, clause);
}
  yymsp[-6].minor.yy391 = yylhsminor.yy391;
        break;
      case 110: /* setlist ::= nm EQ expr */
{
    uint32_t clause = ast_set_clause(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy391);
    yylhsminor.yy391 = ast_set_clause_list(pCtx->astCtx, clause);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 111: /* setlist ::= LP idlist RP EQ expr */
{
    uint32_t clause = ast_set_clause(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, yymsp[-3].minor.yy391, yymsp[0].minor.yy391);
    yymsp[-4].minor.yy391 = ast_set_clause_list(pCtx->astCtx, clause);
}
        break;
      case 112: /* cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
{
    (void)yymsp[0].minor.yy391;
    uint32_t ins = ast_insert_stmt(pCtx->astCtx, (SyntaqliteConflictAction)yymsp[-5].minor.yy144, yymsp[-3].minor.yy391, yymsp[-2].minor.yy391, yymsp[-1].minor.yy391);
    if (yymsp[-6].minor.yy391 != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *wn = AST_NODE(pCtx->astCtx->ast, yymsp[-6].minor.yy391);
        if (wn->tag == SYNTAQLITE_NODE_WITH_CLAUSE) {
            yylhsminor.yy391 = ast_with_clause(pCtx->astCtx, 1, wn->with_clause.ctes, ins);
        } else {
            yylhsminor.yy391 = ast_with_clause(pCtx->astCtx, 0, yymsp[-6].minor.yy391, ins);
        }
    } else {
        yylhsminor.yy391 = ins;
    }
}
  yymsp[-6].minor.yy391 = yylhsminor.yy391;
        break;
      case 113: /* cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
{
    uint32_t ins = ast_insert_stmt(pCtx->astCtx, (SyntaqliteConflictAction)yymsp[-6].minor.yy144, yymsp[-4].minor.yy391, yymsp[-3].minor.yy391, SYNTAQLITE_NULL_NODE);
    if (yymsp[-7].minor.yy391 != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *wn = AST_NODE(pCtx->astCtx->ast, yymsp[-7].minor.yy391);
        if (wn->tag == SYNTAQLITE_NODE_WITH_CLAUSE) {
            yylhsminor.yy391 = ast_with_clause(pCtx->astCtx, 1, wn->with_clause.ctes, ins);
        } else {
            yylhsminor.yy391 = ast_with_clause(pCtx->astCtx, 0, yymsp[-7].minor.yy391, ins);
        }
    } else {
        yylhsminor.yy391 = ins;
    }
}
  yymsp[-7].minor.yy391 = yylhsminor.yy391;
        break;
      case 115: /* upsert ::= RETURNING selcollist */
{
    (void)yymsp[0].minor.yy391;
    yymsp[-1].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 116: /* upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
{
    (void)yymsp[-8].minor.yy391; (void)yymsp[-6].minor.yy391; (void)yymsp[-2].minor.yy391; (void)yymsp[-1].minor.yy391; (void)yymsp[0].minor.yy391;
    yymsp[-11].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 117: /* upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
{
    (void)yymsp[-5].minor.yy391; (void)yymsp[-3].minor.yy391; (void)yymsp[0].minor.yy391;
    yymsp[-8].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 118: /* upsert ::= ON CONFLICT DO NOTHING returning */
{
    yymsp[-4].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 119: /* upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
{
    (void)yymsp[-2].minor.yy391; (void)yymsp[-1].minor.yy391;
    yymsp[-7].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 120: /* returning ::= RETURNING selcollist */
{
    (void)yymsp[0].minor.yy391;
}
        break;
      case 124: /* idlist_opt ::= LP idlist RP */
      case 127: /* expr ::= LP expr RP */ yytestcase(yyruleno==127);
      case 188: /* eidlist_opt ::= LP eidlist RP */ yytestcase(yyruleno==188);
{
    yymsp[-2].minor.yy391 = yymsp[-1].minor.yy391;
}
        break;
      case 125: /* idlist ::= idlist COMMA nm */
{
    uint32_t col = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
    yymsp[-2].minor.yy391 = ast_expr_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, col);
}
        break;
      case 126: /* idlist ::= nm */
{
    uint32_t col = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
    yylhsminor.yy391 = ast_expr_list(pCtx->astCtx, col);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 128: /* expr ::= ID|INDEXED|JOIN_KW */
{
    yylhsminor.yy391 = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NO_SPAN);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 129: /* expr ::= nm DOT nm */
{
    yylhsminor.yy391 = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0),
        SYNTAQLITE_NO_SPAN);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 130: /* expr ::= nm DOT nm DOT nm */
{
    yylhsminor.yy391 = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-4].minor.yy0));
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 131: /* term ::= NULL|FLOAT|BLOB */
{
    SyntaqliteLiteralType lit_type;
    switch (yymsp[0].minor.yy0.type) {
        case TK_NULL:  lit_type = SYNTAQLITE_LITERAL_TYPE_NULL; break;
        case TK_FLOAT: lit_type = SYNTAQLITE_LITERAL_TYPE_FLOAT; break;
        case TK_BLOB:  lit_type = SYNTAQLITE_LITERAL_TYPE_BLOB; break;
        default:       lit_type = SYNTAQLITE_LITERAL_TYPE_NULL; break;
    }
    yylhsminor.yy391 = ast_literal(pCtx->astCtx, lit_type, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 132: /* term ::= STRING */
{
    yylhsminor.yy391 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_STRING, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 133: /* term ::= INTEGER */
{
    yylhsminor.yy391 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_INTEGER, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 134: /* expr ::= VARIABLE */
{
    yylhsminor.yy391 = ast_variable(pCtx->astCtx, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 135: /* expr ::= expr COLLATE ID|STRING */
{
    yylhsminor.yy391 = ast_collate_expr(pCtx->astCtx, yymsp[-2].minor.yy391, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 136: /* expr ::= CAST LP expr AS typetoken RP */
{
    yymsp[-5].minor.yy391 = ast_cast_expr(pCtx->astCtx, yymsp[-3].minor.yy391, syntaqlite_span(pCtx, yymsp[-1].minor.yy0));
}
        break;
      case 137: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
{
    yylhsminor.yy391 = ast_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-4].minor.yy0),
        (uint8_t)yymsp[-2].minor.yy391,
        yymsp[-1].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 138: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
{
    yylhsminor.yy391 = ast_aggregate_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-7].minor.yy0),
        (uint8_t)yymsp[-5].minor.yy391,
        yymsp[-4].minor.yy391,
        yymsp[-1].minor.yy391);
}
  yymsp[-7].minor.yy391 = yylhsminor.yy391;
        break;
      case 139: /* expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
{
    yylhsminor.yy391 = ast_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-3].minor.yy0),
        2,
        SYNTAQLITE_NULL_NODE);
}
  yymsp[-3].minor.yy391 = yylhsminor.yy391;
        break;
      case 140: /* term ::= CTIME_KW */
{
    yylhsminor.yy391 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_CURRENT, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 141: /* expr ::= LP nexprlist COMMA expr RP */
{
    yymsp[-4].minor.yy391 = ast_expr_list_append(pCtx->astCtx, yymsp[-3].minor.yy391, yymsp[-1].minor.yy391);
}
        break;
      case 142: /* expr ::= expr AND expr */
{
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_AND, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 143: /* expr ::= expr OR expr */
{
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_OR, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 144: /* expr ::= expr LT|GT|GE|LE expr */
{
    SyntaqliteBinaryOp op;
    switch (yymsp[-1].minor.yy0.type) {
        case TK_LT: op = SYNTAQLITE_BINARY_OP_LT; break;
        case TK_GT: op = SYNTAQLITE_BINARY_OP_GT; break;
        case TK_LE: op = SYNTAQLITE_BINARY_OP_LE; break;
        default:    op = SYNTAQLITE_BINARY_OP_GE; break;
    }
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 145: /* expr ::= expr EQ|NE expr */
{
    SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == TK_EQ) ? SYNTAQLITE_BINARY_OP_EQ : SYNTAQLITE_BINARY_OP_NE;
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 146: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
{
    SyntaqliteBinaryOp op;
    switch (yymsp[-1].minor.yy0.type) {
        case TK_BITAND: op = SYNTAQLITE_BINARY_OP_BITAND; break;
        case TK_BITOR:  op = SYNTAQLITE_BINARY_OP_BITOR; break;
        case TK_LSHIFT: op = SYNTAQLITE_BINARY_OP_LSHIFT; break;
        default:        op = SYNTAQLITE_BINARY_OP_RSHIFT; break;
    }
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 147: /* expr ::= expr PLUS|MINUS expr */
{
    SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == TK_PLUS) ? SYNTAQLITE_BINARY_OP_PLUS : SYNTAQLITE_BINARY_OP_MINUS;
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 148: /* expr ::= expr STAR|SLASH|REM expr */
{
    SyntaqliteBinaryOp op;
    switch (yymsp[-1].minor.yy0.type) {
        case TK_STAR:  op = SYNTAQLITE_BINARY_OP_STAR; break;
        case TK_SLASH: op = SYNTAQLITE_BINARY_OP_SLASH; break;
        default:       op = SYNTAQLITE_BINARY_OP_REM; break;
    }
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 149: /* expr ::= expr CONCAT expr */
{
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_CONCAT, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 150: /* likeop ::= NOT LIKE_KW|MATCH */
{
    yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;
    yymsp[-1].minor.yy0.n |= 0x80000000;
}
        break;
      case 151: /* expr ::= expr likeop expr */
{
    uint8_t negated = (yymsp[-1].minor.yy0.n & 0x80000000) ? 1 : 0;
    yylhsminor.yy391 = ast_like_expr(pCtx->astCtx, negated, yymsp[-2].minor.yy391, yymsp[0].minor.yy391, SYNTAQLITE_NULL_NODE);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 152: /* expr ::= expr likeop expr ESCAPE expr */
{
    uint8_t negated = (yymsp[-3].minor.yy0.n & 0x80000000) ? 1 : 0;
    yylhsminor.yy391 = ast_like_expr(pCtx->astCtx, negated, yymsp[-4].minor.yy391, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 153: /* expr ::= expr ISNULL|NOTNULL */
{
    SyntaqliteIsOp op = (yymsp[0].minor.yy0.type == TK_ISNULL) ? SYNTAQLITE_IS_OP_ISNULL : SYNTAQLITE_IS_OP_NOTNULL;
    yylhsminor.yy391 = ast_is_expr(pCtx->astCtx, op, yymsp[-1].minor.yy391, SYNTAQLITE_NULL_NODE);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 154: /* expr ::= expr NOT NULL */
{
    yylhsminor.yy391 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_NOTNULL, yymsp[-2].minor.yy391, SYNTAQLITE_NULL_NODE);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 155: /* expr ::= expr IS expr */
{
    yylhsminor.yy391 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 156: /* expr ::= expr IS NOT expr */
{
    yylhsminor.yy391 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_NOT, yymsp[-3].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-3].minor.yy391 = yylhsminor.yy391;
        break;
      case 157: /* expr ::= expr IS NOT DISTINCT FROM expr */
{
    yylhsminor.yy391 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_NOT_DISTINCT, yymsp[-5].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-5].minor.yy391 = yylhsminor.yy391;
        break;
      case 158: /* expr ::= expr IS DISTINCT FROM expr */
{
    yylhsminor.yy391 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_DISTINCT, yymsp[-4].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 159: /* expr ::= NOT expr */
{
    yymsp[-1].minor.yy391 = ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_NOT, yymsp[0].minor.yy391);
}
        break;
      case 160: /* expr ::= BITNOT expr */
{
    yymsp[-1].minor.yy391 = ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_BITNOT, yymsp[0].minor.yy391);
}
        break;
      case 161: /* expr ::= PLUS|MINUS expr */
{
    SyntaqliteUnaryOp op = (yymsp[-1].minor.yy0.type == TK_MINUS) ? SYNTAQLITE_UNARY_OP_MINUS : SYNTAQLITE_UNARY_OP_PLUS;
    yylhsminor.yy391 = ast_unary_expr(pCtx->astCtx, op, yymsp[0].minor.yy391);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 162: /* expr ::= expr PTR expr */
{
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_PTR, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 165: /* expr ::= expr between_op expr AND expr */
{
    yylhsminor.yy391 = ast_between_expr(pCtx->astCtx, (uint8_t)yymsp[-3].minor.yy391, yymsp[-4].minor.yy391, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 166: /* in_op ::= IN */
{ yymsp[0].minor.yy144 = 0; }
        break;
      case 168: /* expr ::= expr in_op LP exprlist RP */
{
    yymsp[-4].minor.yy391 = ast_in_expr(pCtx->astCtx, (uint8_t)yymsp[-3].minor.yy144, yymsp[-4].minor.yy391, yymsp[-1].minor.yy391);
}
        break;
      case 169: /* expr ::= LP select RP */
{
    yymsp[-2].minor.yy391 = ast_subquery_expr(pCtx->astCtx, yymsp[-1].minor.yy391);
}
        break;
      case 170: /* expr ::= expr in_op LP select RP */
{
    uint32_t sub = ast_subquery_expr(pCtx->astCtx, yymsp[-1].minor.yy391);
    yymsp[-4].minor.yy391 = ast_in_expr(pCtx->astCtx, (uint8_t)yymsp[-3].minor.yy144, yymsp[-4].minor.yy391, sub);
}
        break;
      case 171: /* expr ::= expr in_op nm dbnm paren_exprlist */
{
    // Table-valued function IN expression - stub for now
    (void)yymsp[-2].minor.yy0; (void)yymsp[-1].minor.yy0; (void)yymsp[0].minor.yy391;
    yymsp[-4].minor.yy391 = ast_in_expr(pCtx->astCtx, (uint8_t)yymsp[-3].minor.yy144, yymsp[-4].minor.yy391, SYNTAQLITE_NULL_NODE);
}
        break;
      case 172: /* expr ::= EXISTS LP select RP */
{
    yymsp[-3].minor.yy391 = ast_exists_expr(pCtx->astCtx, yymsp[-1].minor.yy391);
}
        break;
      case 173: /* expr ::= CASE case_operand case_exprlist case_else END */
{
    yymsp[-4].minor.yy391 = ast_case_expr(pCtx->astCtx, yymsp[-3].minor.yy391, yymsp[-1].minor.yy391, yymsp[-2].minor.yy391);
}
        break;
      case 174: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
{
    uint32_t w = ast_case_when(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
    yylhsminor.yy391 = ast_case_when_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, w);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 175: /* case_exprlist ::= WHEN expr THEN expr */
{
    uint32_t w = ast_case_when(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
    yymsp[-3].minor.yy391 = ast_case_when_list(pCtx->astCtx, w);
}
        break;
      case 180: /* nexprlist ::= nexprlist COMMA expr */
{
    yylhsminor.yy391 = ast_expr_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 181: /* nexprlist ::= expr */
{
    yylhsminor.yy391 = ast_expr_list(pCtx->astCtx, yymsp[0].minor.yy391);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 182: /* paren_exprlist ::= */
{ yymsp[1].minor.yy391 = SYNTAQLITE_NULL_NODE; }
        break;
      case 183: /* paren_exprlist ::= LP exprlist RP */
{ yymsp[-2].minor.yy391 = yymsp[-1].minor.yy391; }
        break;
      case 184: /* cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
{
    SyntaqliteSourceSpan idx_name = yymsp[-6].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-6].minor.yy0) : syntaqlite_span(pCtx, yymsp[-7].minor.yy0);
    SyntaqliteSourceSpan idx_schema = yymsp[-6].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-7].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-11].minor.yy391 = ast_create_index_stmt(pCtx->astCtx,
        idx_name,
        idx_schema,
        syntaqlite_span(pCtx, yymsp[-4].minor.yy0),
        (uint8_t)yymsp[-10].minor.yy144,
        (uint8_t)yymsp[-8].minor.yy144,
        yymsp[-2].minor.yy391,
        yymsp[0].minor.yy391);
}
        break;
      case 189: /* eidlist ::= eidlist COMMA nm collate sortorder */
{
    (void)yymsp[-1].minor.yy144; (void)yymsp[0].minor.yy391;
    uint32_t col = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0),
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NO_SPAN);
    yymsp[-4].minor.yy391 = ast_expr_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, col);
}
        break;
      case 190: /* eidlist ::= nm collate sortorder */
{
    (void)yymsp[-1].minor.yy144; (void)yymsp[0].minor.yy391;
    uint32_t col = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0),
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NO_SPAN);
    yylhsminor.yy391 = ast_expr_list(pCtx->astCtx, col);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 193: /* cmd ::= DROP INDEX ifexists fullname */
{
    yymsp[-3].minor.yy391 = ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_INDEX, (uint8_t)yymsp[-1].minor.yy144, yymsp[0].minor.yy391);
}
        break;
      case 194: /* cmd ::= VACUUM vinto */
{
    yymsp[-1].minor.yy391 = ast_vacuum_stmt(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN,
        yymsp[0].minor.yy391);
}
        break;
      case 195: /* cmd ::= VACUUM nm vinto */
{
    yymsp[-2].minor.yy391 = ast_vacuum_stmt(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-1].minor.yy0),
        yymsp[0].minor.yy391);
}
        break;
      case 198: /* cmd ::= PRAGMA nm dbnm */
{
    SyntaqliteSourceSpan name_span = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[0].minor.yy0) : syntaqlite_span(pCtx, yymsp[-1].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-1].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-2].minor.yy391 = ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, SYNTAQLITE_NO_SPAN, 0);
}
        break;
      case 199: /* cmd ::= PRAGMA nm dbnm EQ nmnum */
      case 201: /* cmd ::= PRAGMA nm dbnm EQ minus_num */ yytestcase(yyruleno==201);
{
    SyntaqliteSourceSpan name_span = yymsp[-2].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-2].minor.yy0) : syntaqlite_span(pCtx, yymsp[-3].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[-2].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-3].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-4].minor.yy391 = ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, syntaqlite_span(pCtx, yymsp[0].minor.yy0), 1);
}
        break;
      case 200: /* cmd ::= PRAGMA nm dbnm LP nmnum RP */
      case 202: /* cmd ::= PRAGMA nm dbnm LP minus_num RP */ yytestcase(yyruleno==202);
{
    SyntaqliteSourceSpan name_span = yymsp[-3].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-3].minor.yy0) : syntaqlite_span(pCtx, yymsp[-4].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[-3].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-4].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-5].minor.yy391 = ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, syntaqlite_span(pCtx, yymsp[-1].minor.yy0), 2);
}
        break;
      case 204: /* minus_num ::= MINUS INTEGER|FLOAT */
{
    // Build a token that spans from the MINUS sign through the number
    yylhsminor.yy0.z = yymsp[-1].minor.yy0.z;
    yylhsminor.yy0.n = (int)(yymsp[0].minor.yy0.z - yymsp[-1].minor.yy0.z) + yymsp[0].minor.yy0.n;
}
  yymsp[-1].minor.yy0 = yylhsminor.yy0;
        break;
      case 205: /* expr ::= RAISE LP IGNORE RP */
{
    yymsp[-3].minor.yy391 = ast_raise_expr(pCtx->astCtx, SYNTAQLITE_RAISE_TYPE_IGNORE, SYNTAQLITE_NULL_NODE);
}
        break;
      case 206: /* expr ::= RAISE LP raisetype COMMA expr RP */
{
    yymsp[-5].minor.yy391 = ast_raise_expr(pCtx->astCtx, (SyntaqliteRaiseType)yymsp[-3].minor.yy144, yymsp[-1].minor.yy391);
}
        break;
      case 207: /* raisetype ::= ROLLBACK */
{ yymsp[0].minor.yy144 = SYNTAQLITE_RAISE_TYPE_ROLLBACK; }
        break;
      case 208: /* raisetype ::= ABORT */
{ yymsp[0].minor.yy144 = SYNTAQLITE_RAISE_TYPE_ABORT; }
        break;
      case 209: /* raisetype ::= FAIL */
{ yymsp[0].minor.yy144 = SYNTAQLITE_RAISE_TYPE_FAIL; }
        break;
      case 210: /* cmd ::= DROP TRIGGER ifexists fullname */
{
    yymsp[-3].minor.yy391 = ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TRIGGER, (uint8_t)yymsp[-1].minor.yy144, yymsp[0].minor.yy391);
}
        break;
      case 211: /* cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
{
    yymsp[-5].minor.yy391 = ast_attach_stmt(pCtx->astCtx, yymsp[-3].minor.yy391, yymsp[-1].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 212: /* cmd ::= DETACH database_kw_opt expr */
{
    yymsp[-2].minor.yy391 = ast_detach_stmt(pCtx->astCtx, yymsp[0].minor.yy391);
}
        break;
      case 215: /* cmd ::= REINDEX */
{
    yymsp[0].minor.yy391 = ast_analyze_stmt(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NO_SPAN,
        1);
}
        break;
      case 216: /* cmd ::= REINDEX nm dbnm */
{
    SyntaqliteSourceSpan name_span = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[0].minor.yy0) : syntaqlite_span(pCtx, yymsp[-1].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-1].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-2].minor.yy391 = ast_analyze_stmt(pCtx->astCtx, name_span, schema_span, 1);
}
        break;
      case 217: /* cmd ::= ANALYZE */
{
    yymsp[0].minor.yy391 = ast_analyze_stmt(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NO_SPAN,
        0);
}
        break;
      case 218: /* cmd ::= ANALYZE nm dbnm */
{
    SyntaqliteSourceSpan name_span = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[0].minor.yy0) : syntaqlite_span(pCtx, yymsp[-1].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-1].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-2].minor.yy391 = ast_analyze_stmt(pCtx->astCtx, name_span, schema_span, 0);
}
        break;
      case 219: /* cmd ::= ALTER TABLE fullname RENAME TO nm */
{
    yymsp[-5].minor.yy391 = ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_RENAME_TABLE, yymsp[-3].minor.yy391,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        SYNTAQLITE_NO_SPAN);
}
        break;
      case 220: /* cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
{
    yymsp[-6].minor.yy391 = ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_ADD_COLUMN, SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NO_SPAN,
        syntaqlite_span(pCtx, yymsp[-1].minor.yy0));
}
        break;
      case 221: /* cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
{
    yymsp[-5].minor.yy391 = ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_DROP_COLUMN, yymsp[-3].minor.yy391,
        SYNTAQLITE_NO_SPAN,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
        break;
      case 222: /* add_column_fullname ::= fullname */
{
    // Passthrough - fullname already produces a node ID but we don't need it
    // for the ADD COLUMN action since add_column_fullname is consumed by cmd
}
        break;
      case 223: /* cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
{
    yymsp[-7].minor.yy391 = ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_RENAME_COLUMN, yymsp[-5].minor.yy391,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0));
}
        break;
      case 225: /* with ::= WITH RECURSIVE wqlist */
{
    // Tag the CTE list as recursive by wrapping in a marker.
    // We'll handle this in the cmd rules when building the final statement.
    // Use high bit to signal recursive: set bit 31.
    // Actually, we need a cleaner approach. Store the recursive flag
    // by creating a CteList node and using a convention.
    // For now, negate the list ID... no, IDs are u32.
    // Simplest approach: store a sentinel node that wraps (recursive, cte_list).
    // Use the WithClause node with a null select, then extract in cmd rules.
    yymsp[-2].minor.yy391 = ast_with_clause(pCtx->astCtx, 1, yymsp[0].minor.yy391, SYNTAQLITE_NULL_NODE);
}
        break;
      case 226: /* wqas ::= AS */
      case 241: /* trans_opt ::= TRANSACTION */ yytestcase(yyruleno==241);
      case 243: /* savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==243);
{
    yymsp[0].minor.yy144 = 0;
}
        break;
      case 229: /* wqitem ::= withnm eidlist_opt wqas LP select RP */
{
    yylhsminor.yy391 = ast_cte_definition(pCtx->astCtx, syntaqlite_span(pCtx, yymsp[-5].minor.yy0), (uint8_t)yymsp[-3].minor.yy144, yymsp[-4].minor.yy391, yymsp[-1].minor.yy391);
}
  yymsp[-5].minor.yy391 = yylhsminor.yy391;
        break;
      case 231: /* wqlist ::= wqitem */
{
    yylhsminor.yy391 = ast_cte_list(pCtx->astCtx, yymsp[0].minor.yy391);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 232: /* wqlist ::= wqlist COMMA wqitem */
{
    yymsp[-2].minor.yy391 = ast_cte_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 233: /* term ::= QNUMBER */
{
    yylhsminor.yy391 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_QNUMBER, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 234: /* input ::= cmdlist */
{
    pCtx->root = yymsp[0].minor.yy391;
}
        break;
      case 235: /* cmdlist ::= cmdlist ecmd */
{
    yymsp[-1].minor.yy391 = yymsp[0].minor.yy391;  // Just use the last command for now
}
        break;
      case 237: /* ecmd ::= SEMI */
{
    yymsp[0].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 238: /* ecmd ::= cmdx SEMI */
      case 254: /* sclp ::= selcollist COMMA */ yytestcase(yyruleno==254);
{
    yylhsminor.yy391 = yymsp[-1].minor.yy391;
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 239: /* ecmd ::= explain cmdx SEMI */
{
    yylhsminor.yy391 = ast_explain_stmt(pCtx->astCtx, (uint8_t)(yymsp[-2].minor.yy144 - 1), yymsp[-1].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 242: /* trans_opt ::= TRANSACTION nm */
{
    yymsp[-1].minor.yy144 = 0;
}
        break;
      case 245: /* nm ::= ID|INDEXED|JOIN_KW */
      case 246: /* nm ::= STRING */ yytestcase(yyruleno==246);
      case 248: /* typename ::= ID|STRING */ yytestcase(yyruleno==248);
      case 255: /* as ::= ID|STRING */ yytestcase(yyruleno==255);
      case 259: /* likeop ::= LIKE_KW|MATCH */ yytestcase(yyruleno==259);
{
    yylhsminor.yy0 = yymsp[0].minor.yy0;
}
  yymsp[0].minor.yy0 = yylhsminor.yy0;
        break;
      case 247: /* typetoken ::= typename */
{
    (void)yymsp[0].minor.yy0;
}
        break;
      case 251: /* resolvetype ::= raisetype */
{
    // raisetype: ROLLBACK=1, ABORT=2, FAIL=3 (SyntaqliteRaiseType enum values)
    // ConflictAction: ROLLBACK=1, ABORT=2, FAIL=3 (same values, direct passthrough)
    yylhsminor.yy144 = yymsp[0].minor.yy144;
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 257: /* returning ::= */
{
    // empty
}
        break;
      case 268: /* database_kw_opt ::= DATABASE */
{
    // Keyword consumed, no value needed
}
        break;
      case 269: /* database_kw_opt ::= */
{
    // Empty
}
        break;
      default:
      /* (273) create_table ::= createkw temp TABLE ifnotexists nm dbnm */ yytestcase(yyruleno==273);
      /* (274) create_table_args ::= LP columnlist conslist_opt RP table_option_set */ yytestcase(yyruleno==274);
      /* (275) create_table_args ::= AS select */ yytestcase(yyruleno==275);
      /* (276) table_option_set ::= */ yytestcase(yyruleno==276);
      /* (277) table_option_set ::= table_option_set COMMA table_option */ yytestcase(yyruleno==277);
      /* (278) table_option ::= WITHOUT nm */ yytestcase(yyruleno==278);
      /* (279) table_option ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=279);
      /* (280) scantok ::= */ yytestcase(yyruleno==280);
      /* (281) ccons ::= CONSTRAINT nm */ yytestcase(yyruleno==281);
      /* (282) ccons ::= DEFAULT scantok term */ yytestcase(yyruleno==282);
      /* (283) ccons ::= DEFAULT LP expr RP */ yytestcase(yyruleno==283);
      /* (284) ccons ::= DEFAULT PLUS scantok term */ yytestcase(yyruleno==284);
      /* (285) ccons ::= DEFAULT MINUS scantok term */ yytestcase(yyruleno==285);
      /* (286) ccons ::= DEFAULT scantok ID|INDEXED */ yytestcase(yyruleno==286);
      /* (287) ccons ::= NOT NULL onconf */ yytestcase(yyruleno==287);
      /* (288) ccons ::= PRIMARY KEY sortorder onconf autoinc */ yytestcase(yyruleno==288);
      /* (289) ccons ::= UNIQUE onconf */ yytestcase(yyruleno==289);
      /* (290) ccons ::= CHECK LP expr RP */ yytestcase(yyruleno==290);
      /* (291) ccons ::= REFERENCES nm eidlist_opt refargs */ yytestcase(yyruleno==291);
      /* (292) ccons ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=292);
      /* (293) ccons ::= COLLATE ID|STRING */ yytestcase(yyruleno==293);
      /* (294) generated ::= LP expr RP */ yytestcase(yyruleno==294);
      /* (295) generated ::= LP expr RP ID */ yytestcase(yyruleno==295);
      /* (296) autoinc ::= */ yytestcase(yyruleno==296);
      /* (297) autoinc ::= AUTOINCR */ yytestcase(yyruleno==297);
      /* (298) refargs ::= */ yytestcase(yyruleno==298);
      /* (299) refargs ::= refargs refarg */ yytestcase(yyruleno==299);
      /* (300) refarg ::= MATCH nm */ yytestcase(yyruleno==300);
      /* (301) refarg ::= ON INSERT refact */ yytestcase(yyruleno==301);
      /* (302) refarg ::= ON DELETE refact */ yytestcase(yyruleno==302);
      /* (303) refarg ::= ON UPDATE refact */ yytestcase(yyruleno==303);
      /* (304) refact ::= SET NULL */ yytestcase(yyruleno==304);
      /* (305) refact ::= SET DEFAULT */ yytestcase(yyruleno==305);
      /* (306) refact ::= CASCADE */ yytestcase(yyruleno==306);
      /* (307) refact ::= RESTRICT */ yytestcase(yyruleno==307);
      /* (308) refact ::= NO ACTION */ yytestcase(yyruleno==308);
      /* (309) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */ yytestcase(yyruleno==309);
      /* (310) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */ yytestcase(yyruleno==310);
      /* (311) init_deferred_pred_opt ::= */ yytestcase(yyruleno==311);
      /* (312) init_deferred_pred_opt ::= INITIALLY DEFERRED */ yytestcase(yyruleno==312);
      /* (313) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */ yytestcase(yyruleno==313);
      /* (314) conslist_opt ::= */ yytestcase(yyruleno==314);
      /* (315) tconscomma ::= COMMA */ yytestcase(yyruleno==315);
      /* (316) tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==316);
      /* (317) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */ yytestcase(yyruleno==317);
      /* (318) tcons ::= UNIQUE LP sortlist RP onconf */ yytestcase(yyruleno==318);
      /* (319) tcons ::= CHECK LP expr RP onconf */ yytestcase(yyruleno==319);
      /* (320) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */ yytestcase(yyruleno==320);
      /* (321) defer_subclause_opt ::= */ yytestcase(yyruleno==321);
      /* (322) onconf ::= */ yytestcase(yyruleno==322);
      /* (323) onconf ::= ON CONFLICT resolvetype */ yytestcase(yyruleno==323);
      /* (324) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */ yytestcase(yyruleno==324);
      /* (325) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */ yytestcase(yyruleno==325);
      /* (326) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */ yytestcase(yyruleno==326);
      /* (327) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */ yytestcase(yyruleno==327);
      /* (328) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */ yytestcase(yyruleno==328);
      /* (329) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */ yytestcase(yyruleno==329);
      /* (330) trigger_time ::= BEFORE|AFTER */ yytestcase(yyruleno==330);
      /* (331) trigger_time ::= INSTEAD OF */ yytestcase(yyruleno==331);
      /* (332) trigger_time ::= */ yytestcase(yyruleno==332);
      /* (333) trigger_event ::= DELETE|INSERT */ yytestcase(yyruleno==333);
      /* (334) trigger_event ::= UPDATE */ yytestcase(yyruleno==334);
      /* (335) trigger_event ::= UPDATE OF idlist */ yytestcase(yyruleno==335);
      /* (336) when_clause ::= */ yytestcase(yyruleno==336);
      /* (337) when_clause ::= WHEN expr */ yytestcase(yyruleno==337);
      /* (338) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */ yytestcase(yyruleno==338);
      /* (339) trigger_cmd_list ::= trigger_cmd SEMI */ yytestcase(yyruleno==339);
      /* (340) trnm ::= nm DOT nm */ yytestcase(yyruleno==340);
      /* (341) tridxby ::= INDEXED BY nm */ yytestcase(yyruleno==341);
      /* (342) tridxby ::= NOT INDEXED */ yytestcase(yyruleno==342);
      /* (343) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */ yytestcase(yyruleno==343);
      /* (344) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */ yytestcase(yyruleno==344);
      /* (345) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */ yytestcase(yyruleno==345);
      /* (346) trigger_cmd ::= scanpt select scanpt */ yytestcase(yyruleno==346);
      /* (347) cmd ::= create_vtab */ yytestcase(yyruleno==347);
      /* (348) cmd ::= create_vtab LP vtabarglist RP */ yytestcase(yyruleno==348);
      /* (349) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */ yytestcase(yyruleno==349);
      /* (350) vtabarg ::= */ yytestcase(yyruleno==350);
      /* (351) vtabargtoken ::= ANY */ yytestcase(yyruleno==351);
      /* (352) vtabargtoken ::= lp anylist RP */ yytestcase(yyruleno==352);
      /* (353) lp ::= LP */ yytestcase(yyruleno==353);
      /* (354) windowdefn_list ::= windowdefn_list COMMA windowdefn */ yytestcase(yyruleno==354);
      /* (355) windowdefn ::= nm AS LP window RP */ yytestcase(yyruleno==355);
      /* (356) window ::= PARTITION BY nexprlist orderby_opt frame_opt */ yytestcase(yyruleno==356);
      /* (357) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */ yytestcase(yyruleno==357);
      /* (358) window ::= ORDER BY sortlist frame_opt */ yytestcase(yyruleno==358);
      /* (359) window ::= nm ORDER BY sortlist frame_opt */ yytestcase(yyruleno==359);
      /* (360) window ::= nm frame_opt */ yytestcase(yyruleno==360);
      /* (361) frame_opt ::= */ yytestcase(yyruleno==361);
      /* (362) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */ yytestcase(yyruleno==362);
      /* (363) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */ yytestcase(yyruleno==363);
      /* (364) range_or_rows ::= RANGE|ROWS|GROUPS */ yytestcase(yyruleno==364);
      /* (365) frame_bound_s ::= frame_bound (OPTIMIZED OUT) */ assert(yyruleno!=365);
      /* (366) frame_bound_s ::= UNBOUNDED PRECEDING */ yytestcase(yyruleno==366);
      /* (367) frame_bound_e ::= frame_bound (OPTIMIZED OUT) */ assert(yyruleno!=367);
      /* (368) frame_bound_e ::= UNBOUNDED FOLLOWING */ yytestcase(yyruleno==368);
      /* (369) frame_bound ::= expr PRECEDING|FOLLOWING */ yytestcase(yyruleno==369);
      /* (370) frame_bound ::= CURRENT ROW */ yytestcase(yyruleno==370);
      /* (371) frame_exclude_opt ::= */ yytestcase(yyruleno==371);
      /* (372) frame_exclude_opt ::= EXCLUDE frame_exclude */ yytestcase(yyruleno==372);
      /* (373) frame_exclude ::= NO OTHERS */ yytestcase(yyruleno==373);
      /* (374) frame_exclude ::= CURRENT ROW */ yytestcase(yyruleno==374);
      /* (375) frame_exclude ::= GROUP|TIES */ yytestcase(yyruleno==375);
      /* (376) window_clause ::= WINDOW windowdefn_list */ yytestcase(yyruleno==376);
      /* (377) filter_over ::= filter_clause over_clause */ yytestcase(yyruleno==377);
      /* (378) filter_over ::= over_clause (OPTIMIZED OUT) */ assert(yyruleno!=378);
      /* (379) filter_over ::= filter_clause */ yytestcase(yyruleno==379);
      /* (380) over_clause ::= OVER LP window RP */ yytestcase(yyruleno==380);
      /* (381) over_clause ::= OVER nm */ yytestcase(yyruleno==381);
      /* (382) filter_clause ::= FILTER LP WHERE expr RP */ yytestcase(yyruleno==382);
      /* (383) cmd ::= create_table create_table_args */ yytestcase(yyruleno==383);
      /* (384) table_option_set ::= table_option (OPTIMIZED OUT) */ assert(yyruleno!=384);
      /* (385) columnlist ::= columnlist COMMA columnname carglist */ yytestcase(yyruleno==385);
      /* (386) columnlist ::= columnname carglist */ yytestcase(yyruleno==386);
      /* (387) carglist ::= carglist ccons */ yytestcase(yyruleno==387);
      /* (388) carglist ::= */ yytestcase(yyruleno==388);
      /* (389) ccons ::= NULL onconf */ yytestcase(yyruleno==389);
      /* (390) ccons ::= GENERATED ALWAYS AS generated */ yytestcase(yyruleno==390);
      /* (391) ccons ::= AS generated */ yytestcase(yyruleno==391);
      /* (392) conslist_opt ::= COMMA conslist */ yytestcase(yyruleno==392);
      /* (393) conslist ::= conslist tconscomma tcons */ yytestcase(yyruleno==393);
      /* (394) conslist ::= tcons (OPTIMIZED OUT) */ assert(yyruleno!=394);
      /* (395) tconscomma ::= */ yytestcase(yyruleno==395);
      /* (396) defer_subclause_opt ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=396);
      /* (397) foreach_clause ::= */ yytestcase(yyruleno==397);
      /* (398) foreach_clause ::= FOR EACH ROW */ yytestcase(yyruleno==398);
      /* (399) trnm ::= nm */ yytestcase(yyruleno==399);
      /* (400) tridxby ::= */ yytestcase(yyruleno==400);
      /* (401) vtabarglist ::= vtabarg */ yytestcase(yyruleno==401);
      /* (402) vtabarglist ::= vtabarglist COMMA vtabarg */ yytestcase(yyruleno==402);
      /* (403) vtabarg ::= vtabarg vtabargtoken */ yytestcase(yyruleno==403);
      /* (404) anylist ::= */ yytestcase(yyruleno==404);
      /* (405) anylist ::= anylist LP anylist RP */ yytestcase(yyruleno==405);
      /* (406) anylist ::= anylist ANY */ yytestcase(yyruleno==406);
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
