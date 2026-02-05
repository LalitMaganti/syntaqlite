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
  int yy502;
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
#define YYNRULE_WITH_ACTION  108
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
#define YY_ACTTAB_COUNT (2202)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */  1437,  409,  212,  284,  132,  522, 1434,  234, 1413,  574,
 /*    10 */   132,  132,  434,  234,  234, 1434,  957,  233,  958,  539,
 /*    20 */   959,  429,  574,  344, 1396,  574,  399,    2,  465, 1257,
 /*    30 */  1628,  290, 1359,  295,  162,  290, 1359,  413, 1628, 1361,
 /*    40 */    86, 1259,  565,  499,  571,  293,  565,  557,  571,  871,
 /*    50 */   264,  230, 1361,   51, 1399, 1361,   86,  872, 1437,  497,
 /*    60 */   481,  139,  141,   93,  481,  950,  950,  919,  922,  910,
 /*    70 */   910,  290, 1359,  556,  212,  413, 1398,  290, 1359, 1120,
 /*    80 */   574,  523,  565, 1120,  571,  129,  434,  574,  565,  556,
 /*    90 */   571,  129,  129,  326,  346,  413,  254,  558,  398,  139,
 /*   100 */   141,   93,  534,  950,  950,  919,  922,  910,  910,  511,
 /*   110 */  1361,   60,  532, 1555,  452, 1519,  452, 1361,   19,  139,
 /*   120 */   141,   93,  301,  950,  950,  919,  922,  910,  910,  430,
 /*   130 */   466,  421,  290, 1359, 1085,  372, 1187,  293, 1187,  142,
 /*   140 */    44, 1634,   44,  565,  535,  571,  574,  377,  299, 1160,
 /*   150 */   377, 1386,  452, 1353,  501,  137,  137,  138,  138,  138,
 /*   160 */   138,  136,  136,  136,  136,  135,  135,  134,  134,  134,
 /*   170 */   133,  130,  452,  183, 1386,  359, 1361,   51,  290, 1359,
 /*   180 */   907,  907,  920,  923,  321,  575,  321,  575, 1201,  565,
 /*   190 */   452,  571,  478,  137,  137,  138,  138,  138,  138,  136,
 /*   200 */   136,  136,  136,  135,  135,  134,  134,  134,  133,  130,
 /*   210 */  1162, 1657, 1657,  137,  137,  138,  138,  138,  138,  136,
 /*   220 */   136,  136,  136,  135,  135,  134,  134,  134,  133,  130,
 /*   230 */    25,  413,  533,  533,  112,  911,  480,  136,  136,  136,
 /*   240 */   136,  135,  135,  134,  134,  134,  133,  130,  452,  945,
 /*   250 */   945,  413,  524, 1583,  372,  139,  141,   93,  898,  950,
 /*   260 */   950,  919,  922,  910,  910,  236,  891,  573,  461,  458,
 /*   270 */   457,  547, 1139,   99,  547,  139,  141,   93,  456,  950,
 /*   280 */   950,  919,  922,  910,  910,  478,  477,  478,  232,  413,
 /*   290 */  1118,  321,  575,  855, 1140,  530,  530,    7,  854,  135,
 /*   300 */   135,  134,  134,  134,  133,  130,  945,  198,  452, 1141,
 /*   310 */   946,  413, 1055,  139,  141,   93, 1182,  950,  950,  919,
 /*   320 */   922,  910,  910,  892, 1028,  891,  891,  893,  452, 1182,
 /*   330 */   158,  525, 1182,  568, 1029,  139,  141,   93,  452,  950,
 /*   340 */   950,  919,  922,  910,  910,  945,  945, 1055,  372,  137,
 /*   350 */   137,  138,  138,  138,  138,  136,  136,  136,  136,  135,
 /*   360 */   135,  134,  134,  134,  133,  130,  452,  945,  945,  137,
 /*   370 */   137,  138,  138,  138,  138,  136,  136,  136,  136,  135,
 /*   380 */   135,  134,  134,  134,  133,  130,  981,  161,  452,  323,
 /*   390 */   981,  134,  134,  134,  133,  130,  490,  361,  373,   48,
 /*   400 */   568,  568,  945,  199,  115,  505,  946,  137,  137,  138,
 /*   410 */   138,  138,  138,  136,  136,  136,  136,  135,  135,  134,
 /*   420 */   134,  134,  133,  130,  945,    5,  413,  467,  946,  137,
 /*   430 */   137,  138,  138,  138,  138,  136,  136,  136,  136,  135,
 /*   440 */   135,  134,  134,  134,  133,  130,  370,  413,  429,  992,
 /*   450 */   139,  141,   93,  100,  950,  950,  919,  922,  910,  910,
 /*   460 */    44,  132,  945,  945,  234,  574,   48,  372,  574,  855,
 /*   470 */   521,  139,  141,   93,  854,  950,  950,  919,  922,  910,
 /*   480 */   910,  283, 1359,  527,  452,  413,  479, 1057,  406,  405,
 /*   490 */   378,  420,  565,   45,  571, 1361,   51,   97, 1361,   19,
 /*   500 */  1361,   22,  536,  452,  321,  575,  447,  446,  303,  139,
 /*   510 */   141,   93,  294,  950,  950,  919,  922,  910,  910,  945,
 /*   520 */  1236,  213, 1236,  946,  452,  945,  945,  138,  138,  138,
 /*   530 */   138,  136,  136,  136,  136,  135,  135,  134,  134,  134,
 /*   540 */   133,  130,  129,   39,  137,  137,  138,  138,  138,  138,
 /*   550 */   136,  136,  136,  136,  135,  135,  134,  134,  134,  133,
 /*   560 */   130,   38,  452,  237,  985,  137,  137,  138,  138,  138,
 /*   570 */   138,  136,  136,  136,  136,  135,  135,  134,  134,  134,
 /*   580 */   133,  130,  945, 1123, 1123,  513,  946, 1233,  945,  945,
 /*   590 */   524, 1235,  376,  376,  392,  132,  219,  413,  234, 1234,
 /*   600 */    46,  490,  361,  137,  137,  138,  138,  138,  138,  136,
 /*   610 */   136,  136,  136,  135,  135,  134,  134,  134,  133,  130,
 /*   620 */   419,  139,  141,   93,  379,  950,  950,  919,  922,  910,
 /*   630 */   910,  372,  945,  945,  413,  945,  945,  447,  446, 1439,
 /*   640 */   574,  945,  945,  236, 1454,  945,  461,  458,  457,  946,
 /*   650 */   532, 1236,  368, 1236,  390,  413,  456,  231,  139,  141,
 /*   660 */    93, 1506,  950,  950,  919,  922,  910,  910,  109,  525,
 /*   670 */  1361,   86,  109, 1201,  452,  574,  129,  945,  945,  139,
 /*   680 */   141,   93,   50,  950,  950,  919,  922,  910,  910,  945,
 /*   690 */   291, 1359,  945,  946,  510,  526,  946, 1439,  945,  526,
 /*   700 */   507,  565,  946,  571,  556, 1361,   86,  380, 1506,  267,
 /*   710 */   102,  452,  555,  499,  574,  137,  137,  138,  138,  138,
 /*   720 */   138,  136,  136,  136,  136,  135,  135,  134,  134,  134,
 /*   730 */   133,  130,  452,  302,  945, 1095, 1524,  302,  946,  528,
 /*   740 */   321,  575,  183,  898, 1361,  146,  499,  988,  247,   50,
 /*   750 */   372,  891,  137,  137,  138,  138,  138,  138,  136,  136,
 /*   760 */   136,  136,  135,  135,  134,  134,  134,  133,  130,  505,
 /*   770 */   413, 1584,  887,  137,  137,  138,  138,  138,  138,  136,
 /*   780 */   136,  136,  136,  135,  135,  134,  134,  134,  133,  130,
 /*   790 */   413, 1244,  304, 1200,  139,  141,   93, 1611,  950,  950,
 /*   800 */   919,  922,  910,  910,  988, 1417,  574, 1085,  892, 1008,
 /*   810 */   891,  891,  893,  491,  139,  141,   93,  212,  950,  950,
 /*   820 */   919,  922,  910,  910,  576,  307,  852,  852,  413, 1524,
 /*   830 */  1526, 1524, 1384,  505,  470,  356, 1361,   86,  308, 1228,
 /*   840 */    10,  407, 1009, 1228,  349, 1250,  499,  452,  359,  485,
 /*   850 */   347, 1213,  139,  141,   93, 1213,  950,  950,  919,  922,
 /*   860 */   910,  910,  290, 1359,  540,  540,    7,  452,  290, 1359,
 /*   870 */   556, 1182,  358,  565,  351,  571,  945,  945,  444,  565,
 /*   880 */  1160,  571,  543, 1505, 1182,  424,  118, 1182,  137,  137,
 /*   890 */   138,  138,  138,  138,  136,  136,  136,  136,  135,  135,
 /*   900 */   134,  134,  134,  133,  130,  452,  401, 1249,  137,  137,
 /*   910 */   138,  138,  138,  138,  136,  136,  136,  136,  135,  135,
 /*   920 */   134,  134,  134,  133,  130,  309,  542,  860, 1138,  380,
 /*   930 */  1505,  516,  348,  945,  350,  372,  310,  946,  370,  413,
 /*   940 */   159, 1162, 1658, 1658,  122,  215,  137,  137,  138,  138,
 /*   950 */   138,  138,  136,  136,  136,  136,  135,  135,  134,  134,
 /*   960 */   134,  133,  130,  139,  141,   93,  413,  950,  950,  919,
 /*   970 */   922,  910,  910,  222,  518, 1590,    7,  559,  945,  945,
 /*   980 */  1445, 1554,  574,  440,  945,  945,   47,  945,  945,  999,
 /*   990 */   139,  141,   93, 1416,  950,  950,  919,  922,  910,  910,
 /*  1000 */   131,  413,  382, 1056,  290, 1359,  574,  290, 1359,  228,
 /*  1010 */   290, 1359, 1361,   86,  871,  565,  452,  571,  565,  452,
 /*  1020 */   571,  565,  872,  571,  516,  139,  141,   93,  462,  950,
 /*  1030 */   950,  919,  922,  910,  910,  945, 1361,   19,  322,  946,
 /*  1040 */   115,  945,   42,  452,  945,  946,  317,  267,  946,  462,
 /*  1050 */   432,  462,  574,   47,  372, 1415,    6,  137,  137,  138,
 /*  1060 */   138,  138,  138,  136,  136,  136,  136,  135,  135,  134,
 /*  1070 */   134,  134,  133,  130, 1518,  133,  130,  551,  452, 1659,
 /*  1080 */   402,  413, 1361,  147,  137,  137,  138,  138,  138,  138,
 /*  1090 */   136,  136,  136,  136,  135,  135,  134,  134,  134,  133,
 /*  1100 */   130,  516,  945,  945,  435, 1311, 1311,   93,  574,  950,
 /*  1110 */   950,  919,  922,  910,  910, 1504,  413, 1456,  316,  137,
 /*  1120 */   137,  138,  138,  138,  138,  136,  136,  136,  136,  135,
 /*  1130 */   135,  134,  134,  134,  133,  130,  574,  516, 1361,   19,
 /*  1140 */   139,  128,   93,  413,  950,  950,  919,  922,  910,  910,
 /*  1150 */   209, 1446,  437,  574,  429,  489, 1588,    7,  452,  945,
 /*  1160 */     3,  380, 1504,  946,  298,  574, 1361,   19,  141,   93,
 /*  1170 */  1430,  950,  950,  919,  922,  910,  910, 1228,  435,  436,
 /*  1180 */   442, 1228, 1387, 1361,   19,  338,  574, 1442,  574,  240,
 /*  1190 */   239,  238,  574,  452,  574, 1361,   86,  207, 1414,  137,
 /*  1200 */   137,  138,  138,  138,  138,  136,  136,  136,  136,  135,
 /*  1210 */   135,  134,  134,  134,  133,  130, 1361,   19, 1361,   19,
 /*  1220 */   452, 1261, 1361,   83, 1361,   86,  453,  214,    6,  318,
 /*  1230 */   208,  565,  550,  571,  137,  137,  138,  138,  138,  138,
 /*  1240 */   136,  136,  136,  136,  135,  135,  134,  134,  134,  133,
 /*  1250 */   130,  426,  413,  436,  241,  516,  574,  327,  445,  574,
 /*  1260 */   516,  137,  137,  138,  138,  138,  138,  136,  136,  136,
 /*  1270 */   136,  135,  135,  134,  134,  134,  133,  130,   93,  383,
 /*  1280 */   950,  950,  919,  922,  910,  910, 1361,   86,  549, 1361,
 /*  1290 */    86,  372,  512, 1602,  574, 1182,  464, 1257,  232, 1589,
 /*  1300 */     7,  295,  162, 1511,  417,  427,  331,  548, 1182, 1259,
 /*  1310 */   366, 1182,  384,  126,  375,  566, 1193,  574, 1201,  330,
 /*  1320 */   449,  469, 1055,  451, 1361,   53,  441,  506,  423,  452,
 /*  1330 */   569,  574,  243,  280,  329,  472,  332,  471,  242,  290,
 /*  1340 */  1359, 1395,   49,  969,  330, 1587,    7, 1361,   54,  503,
 /*  1350 */   565,  995,  571,  241, 1193,  995,  286, 1055,  110,  562,
 /*  1360 */   574, 1361,   55,  474,  254,   14,  512, 1139,  205,   44,
 /*  1370 */   137,  137,  138,  138,  138,  138,  136,  136,  136,  136,
 /*  1380 */   135,  135,  134,  134,  134,  133,  130,  228,  898, 1140,
 /*  1390 */  1361,   56,  450,  450,  488,  574,  891,  482,  466,  574,
 /*  1400 */   441, 1586,    7,  417, 1141, 1389,  418,  574,  412,   49,
 /*  1410 */   372,  422,  185,  321,  575,  126, 1565,  566,  448,  561,
 /*  1420 */  1201, 1250,  483, 1182, 1098, 1361,   67,   34,  504, 1361,
 /*  1430 */    68,  222,  569,  412,  552,  574, 1182, 1361,   21, 1182,
 /*  1440 */   574,  124,  124,  567,  418,    9,  288,  230,  125,  450,
 /*  1450 */     4,  945,  945,  572,  495,  891,  891,  893,  894, 1633,
 /*  1460 */  1017,  562,    2,  465, 1257, 1361,   57,  948,  295,  162,
 /*  1470 */  1361,   69,  574, 1567,  574,  545, 1259,  492,  541,  574,
 /*  1480 */   544,  496,  401, 1161,  574,  448,  448,  221,  412,  320,
 /*  1490 */   898,  574,  412,  551,  450,  450,  156,  574,  891,  123,
 /*  1500 */   412,  120, 1361,   58, 1361,   70,  290, 1359,  945, 1361,
 /*  1510 */    71,  574,  946,  520, 1361,   72,  574,  565,  126,  571,
 /*  1520 */   566, 1361,   73, 1201,  103,  218,  140, 1361,   74,   34,
 /*  1530 */   251,  254,  574, 1007, 1006,  569,  574, 1014,  574, 1015,
 /*  1540 */   574, 1361,   75,  124,  124,  574, 1361,   76,  475,  574,
 /*  1550 */   125,  450,    4, 1563, 1117,  572, 1117,  891,  891,  893,
 /*  1560 */   894,  339, 1361,   77,  562,  466, 1361,   78, 1361,   59,
 /*  1570 */  1361,   61,  574,  340,  574, 1361,   20,  574,  545, 1361,
 /*  1580 */   144,  574,  336,  546,  574, 1166, 1255,    1, 1354,  126,
 /*  1590 */   537,  566,  335,  898, 1201,  454,  263,  450,  450,  324,
 /*  1600 */   115,  891, 1361,  145, 1361,   80,  569, 1361,   62, 1157,
 /*  1610 */   403, 1361,   81,  574, 1361,   63,  343,  493,  574, 1466,
 /*  1620 */   574,  126,  167,  566,  357,  115, 1201, 1087,  266,  498,
 /*  1630 */   266, 1465,   34,  484,  560,  562,  500,  266,  569,  305,
 /*  1640 */   574,  362,  115, 1361,   82,  574,  124,  124, 1361,   64,
 /*  1650 */  1361,  152,  574,  125,  450,    4, 1570,  574,  572,  574,
 /*  1660 */   891,  891,  893,  894,  898,  253,  352,  562,  450,  450,
 /*  1670 */  1361,  171,  891, 1105,  224, 1361,   90, 1105,  574, 1543,
 /*  1680 */   289,  545, 1361,   65, 1357,  574,  544, 1361,   84, 1361,
 /*  1690 */    85,  574, 1101,  263,  874,  875,  898,  574,  895, 1542,
 /*  1700 */   450,  450,  895,   34,  891, 1116,  574, 1116, 1361,  169,
 /*  1710 */   574, 1060,  266,  166,  115, 1361,  149,  124,  124,  859,
 /*  1720 */   508, 1361,   66,  517,  125,  450,    4, 1361,  170,  572,
 /*  1730 */   363,  891,  891,  893,  894,   34, 1361,  163,  574,  168,
 /*  1740 */  1361,  153, 1040,  140,  979,  160, 1041,  140, 1462,  124,
 /*  1750 */   124, 1352,  574,  476,  582,  574,  125,  450,    4,  574,
 /*  1760 */   292,  572,  574,  891,  891,  893,  894,  574, 1361,  151,
 /*  1770 */   396,  281,  396,  367,  395,  574,  277,  371,  393, 1038,
 /*  1780 */   140,  574, 1361,  148,  966, 1361,  150, 1475, 1500, 1361,
 /*  1790 */    88, 1270, 1361,   79, 1273, 1302,  389, 1361,   89,  248,
 /*  1800 */  1278,  342,  126,  391,  566, 1361,   87, 1201, 1375,  341,
 /*  1810 */  1374, 1361,   52, 1376, 1617,   12,  397,  313,  314,  569,
 /*  1820 */   315,  227,   92, 1413,  566, 1620,  459, 1201,  246, 1487,
 /*  1830 */   334, 1482,  300,  250,  360, 1477,  354,  355,  502,  569,
 /*  1840 */   306,  177, 1492,  203,   43, 1491,  410,  400,  562,  473,
 /*  1850 */  1515,   40, 1514, 1433,  563, 1459, 1432,  387,  210, 1431,
 /*  1860 */  1460,  249,  211, 1458,  463,  223,   95, 1632,  562, 1457,
 /*  1870 */  1631, 1630,  425,  404, 1424, 1403, 1402,  898,  999,  333,
 /*  1880 */  1401,  450,  450,  414, 1244,  891, 1423,  274,  321,  575,
 /*  1890 */  1562,  187, 1560, 1241,  428,  102,  220,  898,  196,  182,
 /*  1900 */   189,  450,  450, 1488,  554,  891,   13,  486,  191,  192,
 /*  1910 */   193,  487,   98,  244, 1520,  101,   34,  194,  579,  256,
 /*  1920 */   110,  408,  494,  126,  553,  566, 1496,  411, 1201, 1494,
 /*  1930 */   124,  124, 1493, 1509,  943,   15,   34,  125,  450,    4,
 /*  1940 */   569,  200,  572,  260,  891,  891,  893,  894,  509,  108,
 /*  1950 */   124,  124, 1531,  476,  582,  515,  282,  125,  450,    4,
 /*  1960 */   292,  262,  572,  365,  891,  891,  893,  894,  438,  562,
 /*  1970 */   396,  439,  396,  311,  395,  519,  277,  312,  393,  268,
 /*  1980 */   369,  143,  551, 1449,  966,  374,  443,  269, 1282, 1281,
 /*  1990 */   233,   11, 1593, 1592, 1339,  381,  119,  319,  898,  248,
 /*  2000 */  1607,  342,  450,  450,  111,  529,  891, 1265,  216,  341,
 /*  2010 */   577,  385,  386,  388,  273, 1377, 1206,  275,  276,  278,
 /*  2020 */   279,  580,  155, 1372, 1367,  172,  173,  296,  335,  225,
 /*  2030 */   415,  226,  186,  250,  416, 1547, 1548,   34,  174, 1546,
 /*  2040 */  1545,  177,   91,  235,   43,  184,  455,   94,   23,  460,
 /*  2050 */   325,  124,  124,   24,  175,  476,  328,  154,  125,  450,
 /*  2060 */     4,  249,  292,  572,  217,  891,  891,  893,  894,  997,
 /*  2070 */  1202,  944,  396,  468,  396, 1010,  395,   96,  277,  297,
 /*  2080 */   393,  176,  337,  414,  245,  157,  966, 1115,  321,  575,
 /*  2090 */  1113,  345,  188,  178,  948,  252,  190, 1031,  353,  255,
 /*  2100 */  1129,  248,  195,  342,  179,  180,  431,  197,  104,  181,
 /*  2110 */   105,  341,  433, 1132,  257,  106,  107,  258, 1128,  164,
 /*  2120 */    26,  259,  364, 1121,  266,  201, 1238,  514,  261,  202,
 /*  2130 */  1168, 1167, 1064,  265,  943,  250, 1058,   16,   27,  229,
 /*  2140 */   531,  285,  204,  177,  287,   28,   43,  140, 1173,  114,
 /*  2150 */   206,   41,   29,  925,   30,  113,  165,  538,   31, 1198,
 /*  2160 */  1184, 1188, 1186,  249, 1192,    8, 1191,   32,  115,   33,
 /*  2170 */   116,  117,  121,  939,  926,  929,  924,   18,  928, 1137,
 /*  2180 */  1090,  270,   35,  851,  968,  414,  564,  980,  578,   17,
 /*  2190 */   321,  575,  127,   36,  394, 1358,  570,  954,   37,  271,
 /*  2200 */   581,  272,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */   234,  209,  187,  190,  191,  209,  224,  194,  226,  187,
 /*    10 */   191,  191,  187,  194,  194,  233,   13,    8,   15,   10,
 /*    20 */    17,  187,  187,  187,  217,  187,  211,  195,  196,  197,
 /*    30 */   216,  239,  240,  201,  202,  239,  240,   37,  224,  217,
 /*    40 */   218,  209,  250,  187,  252,  230,  250,  209,  252,   49,
 /*    50 */   254,  255,  217,  218,  217,  217,  218,   57,  292,  267,
 /*    60 */   258,   61,   62,   63,  262,   65,   66,   67,   68,   69,
 /*    70 */    70,  239,  240,  251,  187,   37,  217,  239,  240,   47,
 /*    80 */   187,  259,  250,   51,  252,  272,  261,  187,  250,  251,
 /*    90 */   252,  272,  272,  206,  260,   37,  264,  259,  211,   61,
 /*   100 */    62,   63,  209,   65,   66,   67,   68,   69,   70,   77,
 /*   110 */   217,  218,  187,  294,  114,  279,  114,  217,  218,   61,
 /*   120 */    62,   63,  266,   65,   66,   67,   68,   69,   70,   73,
 /*   130 */   298,  231,  239,  240,   85,  187,   98,  322,  100,   81,
 /*   140 */    93,  230,   93,  250,  251,  252,  187,  312,  209,  113,
 /*   150 */   315,  187,  114,  321,  187,  155,  156,  157,  158,  159,
 /*   160 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   170 */   170,  171,  114,  187,  210,  126,  217,  218,  239,  240,
 /*   180 */    65,   66,   67,   68,  137,  138,  137,  138,   42,  250,
 /*   190 */   114,  252,  187,  155,  156,  157,  158,  159,  160,  161,
 /*   200 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   210 */   174,  175,  176,  155,  156,  157,  158,  159,  160,  161,
 /*   220 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   230 */   174,   37,  307,  308,  176,  120,  267,  161,  162,  163,
 /*   240 */   164,  165,  166,  167,  168,  169,  170,  171,  114,  116,
 /*   250 */   117,   37,   37,  305,  306,   61,   62,   63,  112,   65,
 /*   260 */    66,   67,   68,   69,   70,  118,  120,  187,  121,  122,
 /*   270 */   123,  312,   23,   79,  315,   61,   62,   63,  131,   65,
 /*   280 */    66,   67,   68,   69,   70,  280,  281,  282,  117,   37,
 /*   290 */    21,  137,  138,  134,   45,  309,  310,  311,  139,  165,
 /*   300 */   166,  167,  168,  169,  170,  171,  173,  174,  114,   60,
 /*   310 */   177,   37,  141,   61,   62,   63,   88,   65,   66,   67,
 /*   320 */    68,   69,   70,  177,   75,  179,  180,  181,  114,  101,
 /*   330 */   176,  116,  104,  214,   85,   61,   62,   63,  114,   65,
 /*   340 */    66,   67,   68,   69,   70,  116,  117,  176,  187,  155,
 /*   350 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   360 */   166,  167,  168,  169,  170,  171,  114,  116,  117,  155,
 /*   370 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   380 */   166,  167,  168,  169,  170,  171,  173,  174,  114,  175,
 /*   390 */   177,  167,  168,  169,  170,  171,  127,  128,  187,  238,
 /*   400 */   281,  282,  173,  174,  176,  187,  177,  155,  156,  157,
 /*   410 */   158,  159,  160,  161,  162,  163,  164,  165,  166,  167,
 /*   420 */   168,  169,  170,  171,  173,  174,   37,  175,  177,  155,
 /*   430 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   440 */   166,  167,  168,  169,  170,  171,  187,   37,  187,  175,
 /*   450 */    61,   62,   63,   43,   65,   66,   67,   68,   69,   70,
 /*   460 */    93,  191,  116,  117,  194,  187,  305,  306,  187,  134,
 /*   470 */   187,   61,   62,   63,  139,   65,   66,   67,   68,   69,
 /*   480 */    70,  239,  240,  187,  114,   37,  119,  141,  165,  166,
 /*   490 */   220,  232,  250,   84,  252,  217,  218,  174,  217,  218,
 /*   500 */   217,  218,  167,  114,  137,  138,  165,  166,  290,   61,
 /*   510 */    62,   63,  231,   65,   66,   67,   68,   69,   70,  173,
 /*   520 */   179,  260,  181,  177,  114,  116,  117,  157,  158,  159,
 /*   530 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   540 */   170,  171,  272,  156,  155,  156,  157,  158,  159,  160,
 /*   550 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*   560 */   171,  174,  114,   44,  175,  155,  156,  157,  158,  159,
 /*   570 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   580 */   170,  171,  173,  126,  127,  128,  177,  115,  116,  117,
 /*   590 */    37,  119,  314,  315,   31,  191,  148,   37,  194,  127,
 /*   600 */    84,  127,  128,  155,  156,  157,  158,  159,  160,  161,
 /*   610 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   620 */   203,   61,   62,   63,  220,   65,   66,   67,   68,   69,
 /*   630 */    70,  187,  116,  117,   37,  116,  117,  165,  166,  234,
 /*   640 */   187,  116,  117,  118,  256,  173,  121,  122,  123,  177,
 /*   650 */   187,  179,   89,  181,   91,   37,  131,  187,   61,   62,
 /*   660 */    63,  271,   65,   66,   67,   68,   69,   70,  115,  116,
 /*   670 */   217,  218,  115,   42,  114,  187,  272,  116,  117,   61,
 /*   680 */    62,   63,  238,   65,   66,   67,   68,   69,   70,  173,
 /*   690 */   239,  240,  173,  177,  277,  142,  177,  292,  173,  142,
 /*   700 */   283,  250,  177,  252,  251,  217,  218,  317,  318,   43,
 /*   710 */   149,  114,  259,  187,  187,  155,  156,  157,  158,  159,
 /*   720 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   730 */   170,  171,  114,  258,  173,  175,  187,  262,  177,  251,
 /*   740 */   137,  138,  187,  112,  217,  218,  187,  116,   29,  305,
 /*   750 */   306,  120,  155,  156,  157,  158,  159,  160,  161,  162,
 /*   760 */   163,  164,  165,  166,  167,  168,  169,  170,  171,  187,
 /*   770 */    37,  308,  175,  155,  156,  157,  158,  159,  160,  161,
 /*   780 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   790 */    37,   72,  266,  175,   61,   62,   63,  187,   65,   66,
 /*   800 */    67,   68,   69,   70,  173,  227,  187,   85,  177,   53,
 /*   810 */   179,  180,  181,  128,   61,   62,   63,  187,   65,   66,
 /*   820 */    67,   68,   69,   70,  133,  266,  135,  136,   37,  280,
 /*   830 */   281,  282,  209,  187,   78,  150,  217,  218,  209,  173,
 /*   840 */   174,  211,   86,  177,    1,  113,  187,  114,  126,  127,
 /*   850 */   128,  173,   61,   62,   63,  177,   65,   66,   67,   68,
 /*   860 */    69,   70,  239,  240,  309,  310,  311,  114,  239,  240,
 /*   870 */   251,   88,  290,  250,   31,  252,  116,  117,  259,  250,
 /*   880 */   113,  252,   99,  271,  101,  129,  153,  104,  155,  156,
 /*   890 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*   900 */   167,  168,  169,  170,  171,  114,  174,  175,  155,  156,
 /*   910 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*   920 */   167,  168,  169,  170,  171,  266,  143,  167,  175,  317,
 /*   930 */   318,  187,   89,  173,   91,  187,  290,  177,  187,   37,
 /*   940 */     6,  174,  175,  176,  153,   43,  155,  156,  157,  158,
 /*   950 */   159,  160,  161,  162,  163,  164,  165,  166,  167,  168,
 /*   960 */   169,  170,  171,   61,   62,   63,   37,   65,   66,   67,
 /*   970 */    68,   69,   70,  140,  209,  310,  311,  209,  116,  117,
 /*   980 */   209,  237,  187,  232,  116,  117,  238,  116,  117,  125,
 /*   990 */    61,   62,   63,  227,   65,   66,   67,   68,   69,   70,
 /*  1000 */    71,   37,  187,  141,  239,  240,  187,  239,  240,  176,
 /*  1010 */   239,  240,  217,  218,   49,  250,  114,  252,  250,  114,
 /*  1020 */   252,  250,   57,  252,  187,   61,   62,   63,  187,   65,
 /*  1030 */    66,   67,   68,   69,   70,  173,  217,  218,  187,  177,
 /*  1040 */   176,  173,  174,  114,  173,  177,  251,   43,  177,  208,
 /*  1050 */   231,  210,  187,  305,  306,  227,  190,  155,  156,  157,
 /*  1060 */   158,  159,  160,  161,  162,  163,  164,  165,  166,  167,
 /*  1070 */   168,  169,  170,  171,  237,  170,  171,  143,  114,  301,
 /*  1080 */   302,   37,  217,  218,  155,  156,  157,  158,  159,  160,
 /*  1090 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*  1100 */   171,  187,  116,  117,  187,   61,   62,   63,  187,   65,
 /*  1110 */    66,   67,   68,   69,   70,  271,   37,  257,  258,  155,
 /*  1120 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*  1130 */   166,  167,  168,  169,  170,  171,  187,  187,  217,  218,
 /*  1140 */    61,   62,   63,   37,   65,   66,   67,   68,   69,   70,
 /*  1150 */   284,  237,  231,  187,  187,  289,  310,  311,  114,  173,
 /*  1160 */   174,  317,  318,  177,  111,  187,  217,  218,   62,   63,
 /*  1170 */   187,   65,   66,   67,   68,   69,   70,  173,  261,  187,
 /*  1180 */   231,  177,  187,  217,  218,  132,  187,  237,  187,  126,
 /*  1190 */   127,  128,  187,  114,  187,  217,  218,  231,  187,  155,
 /*  1200 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*  1210 */   166,  167,  168,  169,  170,  171,  217,  218,  217,  218,
 /*  1220 */   114,  240,  217,  218,  217,  218,   37,  260,  190,  251,
 /*  1230 */   231,  250,  231,  252,  155,  156,  157,  158,  159,  160,
 /*  1240 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*  1250 */   171,   37,   37,  261,   65,  187,  187,   43,  251,  187,
 /*  1260 */   187,  155,  156,  157,  158,  159,  160,  161,  162,  163,
 /*  1270 */   164,  165,  166,  167,  168,  169,  170,  171,   63,  274,
 /*  1280 */    65,   66,   67,   68,   69,   70,  217,  218,   78,  217,
 /*  1290 */   218,  187,  187,  316,  187,   88,  196,  197,  117,  310,
 /*  1300 */   311,  201,  202,    1,  115,  237,  187,   97,  101,  209,
 /*  1310 */   237,  104,  247,   37,  249,   39,  106,  187,   42,  130,
 /*  1320 */   251,  107,  141,  251,  217,  218,  187,  289,  114,  114,
 /*  1330 */    54,  187,  118,  119,  120,  121,  122,  123,  124,  239,
 /*  1340 */   240,  187,  238,   41,  130,  310,  311,  217,  218,   37,
 /*  1350 */   250,  173,  252,   65,  144,  177,  175,  176,  147,   83,
 /*  1360 */   187,  217,  218,  295,  264,   43,  261,   23,  189,   93,
 /*  1370 */   155,  156,  157,  158,  159,  160,  161,  162,  163,  164,
 /*  1380 */   165,  166,  167,  168,  169,  170,  171,  176,  112,   45,
 /*  1390 */   217,  218,  116,  117,   92,  187,  120,  242,  298,  187,
 /*  1400 */   261,  310,  311,  115,   60,  212,  213,  187,  253,  305,
 /*  1410 */   306,  299,  300,  137,  138,   37,  187,   39,  214,   75,
 /*  1420 */    42,  113,  242,   88,  122,  217,  218,  151,  116,  217,
 /*  1430 */   218,  140,   54,  253,   99,  187,  101,  217,  218,  104,
 /*  1440 */   187,  165,  166,  212,  213,   67,  254,  255,  172,  173,
 /*  1450 */   174,  116,  117,  177,  115,  179,  180,  181,  182,  175,
 /*  1460 */   176,   83,  195,  196,  197,  217,  218,  176,  201,  202,
 /*  1470 */   217,  218,  187,  187,  187,   97,  209,  242,  143,  187,
 /*  1480 */   102,  242,  174,  175,  187,  281,  282,  148,  253,  242,
 /*  1490 */   112,  187,  253,  143,  116,  117,  174,  187,  120,  152,
 /*  1500 */   253,  154,  217,  218,  217,  218,  239,  240,  173,  217,
 /*  1510 */   218,  187,  177,   37,  217,  218,  187,  250,   37,  252,
 /*  1520 */    39,  217,  218,   42,  147,  148,  176,  217,  218,  151,
 /*  1530 */    43,  264,  187,  119,  120,   54,  187,   13,  187,   15,
 /*  1540 */   187,  217,  218,  165,  166,  187,  217,  218,  187,  187,
 /*  1550 */   172,  173,  174,  187,  179,  177,  181,  179,  180,  181,
 /*  1560 */   182,  187,  217,  218,   83,  298,  217,  218,  217,  218,
 /*  1570 */   217,  218,  187,  187,  187,  217,  218,  187,   97,  217,
 /*  1580 */   218,  187,  120,  102,  187,  109,  319,  320,  321,   37,
 /*  1590 */   143,   39,  130,  112,   42,  175,  176,  116,  117,  175,
 /*  1600 */   176,  120,  217,  218,  217,  218,   54,  217,  218,  175,
 /*  1610 */   176,  217,  218,  187,  217,  218,  187,  128,  187,  187,
 /*  1620 */   187,   37,  175,   39,  175,  176,   42,  175,  176,  175,
 /*  1630 */   176,  187,  151,  187,  292,   83,  175,  176,   54,  150,
 /*  1640 */   187,  175,  176,  217,  218,  187,  165,  166,  217,  218,
 /*  1650 */   217,  218,  187,  172,  173,  174,  187,  187,  177,  187,
 /*  1660 */   179,  180,  181,  182,  112,  178,  187,   83,  116,  117,
 /*  1670 */   217,  218,  120,  173,  215,  217,  218,  177,  187,  187,
 /*  1680 */   174,   97,  217,  218,  178,  187,  102,  217,  218,  217,
 /*  1690 */   218,  187,  175,  176,   95,   96,  112,  187,  173,  187,
 /*  1700 */   116,  117,  177,  151,  120,  179,  187,  181,  217,  218,
 /*  1710 */   187,  175,  176,  175,  176,  217,  218,  165,  166,  167,
 /*  1720 */   187,  217,  218,  286,  172,  173,  174,  217,  218,  177,
 /*  1730 */   187,  179,  180,  181,  182,  151,  217,  218,  187,  193,
 /*  1740 */   217,  218,  175,  176,  175,  176,  175,  176,  187,  165,
 /*  1750 */   166,    0,  187,    2,    3,  187,  172,  173,  174,  187,
 /*  1760 */     9,  177,  187,  179,  180,  181,  182,  187,  217,  218,
 /*  1770 */    19,  285,   21,  187,   23,  187,   25,  187,   27,  175,
 /*  1780 */   176,  187,  217,  218,   33,  217,  218,  187,  187,  217,
 /*  1790 */   218,  189,  217,  218,  187,  187,  187,  217,  218,   48,
 /*  1800 */   187,   50,   37,  187,   39,  217,  218,   42,  187,   58,
 /*  1810 */   187,  217,  218,  187,  187,  241,  198,  189,  189,   54,
 /*  1820 */   189,  229,   37,  226,   39,  205,  221,   42,  296,  269,
 /*  1830 */   220,  265,  243,   82,  243,  265,  291,  244,  291,   54,
 /*  1840 */   244,   90,  269,  174,   93,  269,  269,  222,   83,  115,
 /*  1850 */   220,  176,  220,  219,  276,  257,  219,  243,  247,  219,
 /*  1860 */   257,  110,  247,  257,  207,  241,  174,  225,   83,  257,
 /*  1870 */   225,  219,   76,  222,  228,  219,  221,  112,  125,  219,
 /*  1880 */   219,  116,  117,  132,   72,  120,  228,  178,  137,  138,
 /*  1890 */   192,  296,  192,   56,  192,  149,  148,  112,  174,   61,
 /*  1900 */   235,  116,  117,  270,  139,  120,  268,   35,  236,  236,
 /*  1910 */   236,  192,  293,  296,  279,  293,  151,  236,   35,  204,
 /*  1920 */   147,  244,  244,   37,  139,   39,  235,  244,   42,  270,
 /*  1930 */   165,  166,  270,  244,  183,  268,  151,  172,  173,  174,
 /*  1940 */    54,  235,  177,  204,  179,  180,  181,  182,  192,  152,
 /*  1950 */   165,  166,  288,    2,    3,   74,  192,  172,  173,  174,
 /*  1960 */     9,  204,  177,  287,  179,  180,  181,  182,   43,   83,
 /*  1970 */    19,  114,   21,  278,   23,  304,   25,  278,   27,  192,
 /*  1980 */   222,  146,  143,  248,   33,  247,   94,  103,  263,  263,
 /*  1990 */     8,  174,  313,  313,  273,  192,  152,  275,  112,   48,
 /*  2000 */   318,   50,  116,  117,  145,  144,  120,  248,  246,   58,
 /*  2010 */   222,  245,  244,  192,  204,  192,   25,  204,  200,  200,
 /*  2020 */    11,  199,  223,  199,  199,  297,  297,  223,  130,  215,
 /*  2030 */   303,  215,  300,   82,  303,  190,  190,  151,  297,  190,
 /*  2040 */   190,   90,  190,  124,   93,  174,   79,  174,  174,   79,
 /*  2050 */   175,  165,  166,  174,  176,    2,   43,  175,  172,  173,
 /*  2060 */   174,  110,    9,  177,  174,  179,  180,  181,  182,  173,
 /*  2070 */   183,  183,   19,  108,   21,   46,   23,  174,   25,   79,
 /*  2080 */    27,   55,    4,  132,   29,   31,   33,  175,  137,  138,
 /*  2090 */   175,  138,  149,  129,  176,   43,  140,   39,   31,  142,
 /*  2100 */   183,   48,  140,   50,  129,  129,   73,  149,  156,  129,
 /*  2110 */   156,   58,   55,  116,   52,  156,  156,  178,  183,    9,
 /*  2120 */   174,  115,    1,   80,  176,   80,   87,   59,  178,  115,
 /*  2130 */   175,  109,  116,   52,  183,   82,  141,  174,   52,  178,
 /*  2140 */    43,  175,  174,   90,  175,   52,   93,  176,  175,  147,
 /*  2150 */   176,  174,   52,  175,   52,  176,  175,  174,   52,  175,
 /*  2160 */   100,   87,   98,  110,   87,   62,  105,   52,  176,   52,
 /*  2170 */   140,  140,  176,  175,  175,   21,  175,   43,  175,  175,
 /*  2180 */   175,  174,  174,  134,   39,  132,  176,  175,   37,  174,
 /*  2190 */   137,  138,  174,  174,   29,  178,  176,    7,  174,  178,
 /*  2200 */     5,  178,  323,  323,  323,  323,  323,  323,  323,  323,
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
 /*  2350 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2360 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2370 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  2380 */   323,  323,  323,  323,  323,  323,  323,  323,  323,
};
#define YY_SHIFT_COUNT    (582)
#define YY_SHIFT_MIN      (0)
#define YY_SHIFT_MAX      (2195)
static const unsigned short int yy_shift_ofst[] = {
 /*     0 */  1951, 1751, 2053, 1276, 1276,   47,   49, 1378, 1481, 1584,
 /*    10 */  1886, 1886, 1886,  367,   47,   47,   47,   47,   47,    0,
 /*    20 */     0,  194,  964, 1886, 1886, 1886, 1886, 1886, 1886, 1886,
 /*    30 */  1886, 1886, 1886, 1886, 1886, 1886, 1886, 1886,  472,  472,
 /*    40 */   525, 1335, 1335,  409,  516,  871,  871,  154,  154,  154,
 /*    50 */   154,   38,   58,  214,  252,  274,  389,  410,  448,  560,
 /*    60 */   597,  618,  733,  753,  791,  902,  929,  964,  964,  964,
 /*    70 */   964,  964,  964,  964,  964,  964,  964,  964,  964,  964,
 /*    80 */   964,  964,  964,  964, 1044, 1079,  964,  964, 1106, 1215,
 /*    90 */  1215, 1552, 1765, 1785, 1886, 1886, 1886, 1886, 1886, 1886,
 /*   100 */  1886, 1886, 1886, 1886, 1886, 1886, 1886, 1886, 1886, 1886,
 /*   110 */  1886, 1886, 1886, 1886, 1886, 1886, 1886, 1886, 1886, 1886,
 /*   120 */  1886, 1886, 1886, 1886, 1886, 1886, 1886, 1886, 1886, 1886,
 /*   130 */  1886, 1886, 1886, 1886, 1886, 1886, 1886, 1886, 1886, 1886,
 /*   140 */  1886, 1886, 1886, 1886,  370,  370,  370,  370,  370,   76,
 /*   150 */    76,  134,  224,  224,  519, 1189,  871,  871,  871,  871,
 /*   160 */   341,  341, 1302,  905,  474,    9,    9,    9,  603,    2,
 /*   170 */     2, 2202, 1214, 1214, 1214,  519,  249,  561,  133,  133,
 /*   180 */   133,  133,  249,  783,  871,   36,  767,  871,  871,  871,
 /*   190 */   871,  871,  871,  871,  871,  871,  871,  871,  871,  871,
 /*   200 */   871,  871,  871,  871,  871,  553,  871,  228,  228,  269,
 /*   210 */  1207, 1207,  678,  215,  215,  678,  934, 2202, 2202, 2202,
 /*   220 */  2202, 2202, 2202, 2202,  631,  146,  146,  147,  229,  760,
 /*   230 */   251,  346,  862,  868,  986,  871,  871,  871,  756,  756,
 /*   240 */   756,  871,  871,  871,  871,  871,  871,  871,  871,  871,
 /*   250 */   871,  871,  871,  871,  722,  871,  871,  871,  871,  871,
 /*   260 */   871,  871,  871,  871, 1181,  871,  871,  871,  666, 1210,
 /*   270 */  1344,  871,  871,  871,  871,  871,  871,  871,  871,  871,
 /*   280 */   323,  457,   32,  691, 1004, 1004, 1004, 1004,  171,  335,
 /*   290 */   691,  691,    3, 1284,  864, 1322, 1288,  965,  719, 1339,
 /*   300 */  1377,  833, 1377, 1312, 1211, 1339, 1339, 1211, 1339,  833,
 /*   310 */  1312,  965,  965,  557,  557,  557,  557, 1350, 1350, 1347,
 /*   320 */  1291,  159, 1669, 1734, 1734, 1734, 1675, 1692, 1692, 1734,
 /*   330 */  1796, 1669, 1734, 1753, 1734, 1796, 1734, 1812, 1812, 1709,
 /*   340 */  1709, 1837, 1837, 1709, 1746, 1748, 1724, 1838, 1872, 1872,
 /*   350 */  1872, 1872, 1709, 1883, 1773, 1748, 1748, 1773, 1724, 1838,
 /*   360 */  1773, 1838, 1773, 1709, 1883, 1797, 1881, 1709, 1883, 1925,
 /*   370 */  1857, 1857, 1669, 1709, 1835, 1839, 1884, 1884, 1892, 1892,
 /*   380 */  1982, 1817, 1709, 1844, 1835, 1859, 1861, 1773, 1669, 1709,
 /*   390 */  1883, 1709, 1883, 1991, 1991, 2009, 2009, 2009, 2202, 2202,
 /*   400 */  2202, 2202, 2202, 2202, 2202, 2202, 2202, 2202, 2202, 2202,
 /*   410 */  2202, 2202, 2202,  115,  843,  732, 1308, 1063,  213,  563,
 /*   420 */  1420, 1424, 1434, 1178, 1414, 1524, 1462, 1053,  387, 1487,
 /*   430 */   685, 1489, 1449,   56, 1452, 1454, 1461, 1466, 1476, 1500,
 /*   440 */  1517, 1536, 1538, 1599, 1447, 1567, 1375, 1526, 1569, 1571,
 /*   450 */  1506, 1604, 1525, 1898, 1919, 1871, 1967, 1873, 1874, 1875,
 /*   460 */  1879, 1970, 1878, 1882, 1887, 1888, 1890, 1896, 2013, 1965,
 /*   470 */  2029, 1903, 2000, 2026, 2078, 2055, 2054, 1912, 1915, 1953,
 /*   480 */  1943, 1964, 1918, 1918, 2052, 1956, 2058, 1957, 2067, 1917,
 /*   490 */  1962, 1975, 1918, 1976, 2033, 2057, 1918, 1958, 1952, 1954,
 /*   500 */  1959, 1960, 1980, 1997, 2062, 1939, 1935, 2110, 1946, 2006,
 /*   510 */  2121, 2043, 1948, 2045, 2039, 2068, 1950, 2014, 1955, 1963,
 /*   520 */  2022, 1961, 1966, 1969, 2016, 2081, 1968, 1995, 1971, 2086,
 /*   530 */  1973, 1977, 2097, 1974, 1978, 1979, 1981, 2093, 2002, 1983,
 /*   540 */  1984, 2100, 2102, 2106, 2060, 2074, 2064, 2103, 2077, 2061,
 /*   550 */  1992, 2115, 2117, 2030, 2031, 1998, 1971, 1999, 2001, 2003,
 /*   560 */  1996, 2004, 2007, 2154, 2008, 2010, 2015, 2005, 2012, 2018,
 /*   570 */  2019, 2020, 2017, 2021, 2023, 2024, 2049, 2134, 2145, 2151,
 /*   580 */  2165, 2190, 2195,
};
#define YY_REDUCE_COUNT (412)
#define YY_REDUCE_MIN   (-234)
#define YY_REDUCE_MAX   (1852)
static const short yy_reduce_ofst[] = {
 /*     0 */  1267, -168, 1100, -162, -107, -204, -208, -165,  278,  -41,
 /*    10 */  -178,  453,  619,  -61,  623,  629,  765,  768,  771,  270,
 /*    20 */   404, -181, -187, -100,  281,  819,  921,  488,  949,  966,
 /*    30 */   999,  795, 1001,  978, 1005, 1007, 1069, 1072,    5,  549,
 /*    40 */  -185,  -14,  555,  161,  444,  748, 1104,  242,  451,  242,
 /*    50 */   451, -180, -180, -180, -180, -180, -180, -180, -180, -180,
 /*    60 */  -180, -180, -180, -180, -180, -180, -180, -180, -180, -180,
 /*    70 */  -180, -180, -180, -180, -180, -180, -180, -180, -180, -180,
 /*    80 */  -180, -180, -180, -180, -180, -180, -180, -180, -180, -180,
 /*    90 */  -180,  283,  527,  865, 1107, 1130, 1144, 1173, 1208, 1212,
 /*   100 */  1220, 1248, 1253, 1285, 1287, 1292, 1297, 1304, 1310, 1324,
 /*   110 */  1329, 1345, 1349, 1351, 1353, 1358, 1362, 1385, 1387, 1390,
 /*   120 */  1394, 1397, 1426, 1431, 1433, 1453, 1458, 1465, 1470, 1472,
 /*   130 */  1491, 1498, 1504, 1510, 1519, 1523, 1551, 1565, 1568, 1572,
 /*   140 */  1575, 1580, 1588, 1594, -180, -180, -180, -180, -180, -180,
 /*   150 */  -180, -180, -180, -180,  841, -218, -113, 1068,  -52,  -75,
 /*   160 */   119, 1204,  417, -180,  866,  390,  612,  844,  981, -180,
 /*   170 */  -180, -180, -186, -186, -186,  -36, -234, -164, -144,  526,
 /*   180 */   559,  659,  405,  665,  259,  778,  778,  630, -166,  261,
 /*   190 */   967,  744,  837,  914,  950,  218, -175,  582,  917,  992,
 /*   200 */   646, 1105, 1073,  751, 1139,  860,  463,  846,  989, 1038,
 /*   210 */  1035, 1091, 1193, -198,  475, 1231, 1065, 1112, 1155, 1180,
 /*   220 */  1235, 1239, 1192, 1247, -193, -163, -141,  -89,  -33,   80,
 /*   230 */   211,  296,  470,  610,  815,  851,  983,  995,  578,  766,
 /*   240 */   828, 1011, 1119, 1154, 1229, 1286, 1361, 1366, 1374, 1386,
 /*   250 */  1429, 1432, 1444, 1446,  -31, 1469, 1479, 1492, 1512, 1533,
 /*   260 */  1543, 1561, 1586, 1590,  388, 1600, 1601, 1607, 1179,  977,
 /*   270 */  1342, 1608,   80, 1609, 1613, 1616, 1621, 1623, 1626, 1627,
 /*   280 */  1459, 1437, 1486, 1546, 1602, 1628, 1629, 1631,  388, 1574,
 /*   290 */  1546, 1546, 1618, 1592, 1605, 1620, 1597, 1610, 1532, 1560,
 /*   300 */  1566, 1589, 1570, 1545, 1593, 1573, 1576, 1596, 1577, 1591,
 /*   310 */  1547, 1630, 1632, 1598, 1603, 1606, 1612, 1611, 1615, 1578,
 /*   320 */  1614, 1624, 1625, 1634, 1637, 1640, 1657, 1642, 1645, 1652,
 /*   330 */  1646, 1651, 1656, 1655, 1660, 1658, 1661, 1595, 1617, 1698,
 /*   340 */  1700, 1619, 1622, 1702, 1635, 1633, 1638, 1665, 1672, 1673,
 /*   350 */  1674, 1681, 1719, 1715, 1677, 1659, 1662, 1678, 1667, 1691,
 /*   360 */  1683, 1706, 1689, 1756, 1739, 1664, 1676, 1764, 1757, 1671,
 /*   370 */  1695, 1699, 1758, 1787, 1735, 1738, 1679, 1680, 1725, 1726,
 /*   380 */  1682, 1721, 1803, 1722, 1759, 1762, 1766, 1768, 1788, 1821,
 /*   390 */  1810, 1823, 1813, 1818, 1819, 1822, 1824, 1825, 1728, 1729,
 /*   400 */  1799, 1727, 1731, 1732, 1804, 1814, 1816, 1741, 1845, 1846,
 /*   410 */  1849, 1850, 1852,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */  1663, 1663, 1663, 1347, 1254, 1455, 1254, 1254, 1254, 1254,
 /*    10 */  1347, 1347, 1347, 1254, 1254, 1254, 1254, 1254, 1254, 1285,
 /*    20 */  1285, 1557, 1258, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*    30 */  1254, 1254, 1254, 1254, 1346, 1254, 1254, 1254, 1254, 1254,
 /*    40 */  1254, 1591, 1591, 1254, 1254, 1254, 1254, 1576, 1575, 1254,
 /*    50 */  1254, 1254, 1294, 1254, 1254, 1254, 1254, 1254, 1479, 1348,
 /*    60 */  1349, 1254, 1254, 1254, 1254, 1254, 1320, 1556, 1558, 1521,
 /*    70 */  1486, 1485, 1484, 1483, 1539, 1471, 1298, 1292, 1296, 1348,
 /*    80 */  1343, 1344, 1342, 1363, 1333, 1254, 1349, 1295, 1312, 1311,
 /*    90 */  1328, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   100 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   110 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   120 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   130 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   140 */  1254, 1254, 1254, 1254, 1327, 1326, 1325, 1324, 1314, 1321,
 /*   150 */  1313, 1315, 1330, 1316, 1385, 1435, 1254, 1254, 1254, 1254,
 /*   160 */  1254, 1254, 1382, 1317, 1258, 1309, 1503, 1308, 1254, 1508,
 /*   170 */  1318, 1329, 1623, 1622, 1564, 1254, 1254, 1522, 1254, 1254,
 /*   180 */  1254, 1254, 1254, 1591, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   190 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   200 */  1254, 1254, 1254, 1254, 1254, 1473, 1254, 1591, 1591, 1258,
 /*   210 */  1591, 1591, 1390, 1474, 1474, 1390, 1279, 1571, 1269, 1269,
 /*   220 */  1269, 1269, 1455, 1269, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   230 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   240 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1561, 1559,
 /*   250 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   260 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1274, 1254,
 /*   270 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1616,
 /*   280 */  1394, 1254, 1534, 1448, 1274, 1274, 1274, 1274, 1276, 1268,
 /*   290 */  1260, 1447, 1368, 1636, 1410, 1254, 1405, 1285, 1655, 1489,
 /*   300 */  1478, 1275, 1478, 1652, 1297, 1489, 1489, 1297, 1489, 1275,
 /*   310 */  1652, 1285, 1285, 1473, 1473, 1473, 1473, 1279, 1279, 1345,
 /*   320 */  1275, 1268, 1512, 1436, 1436, 1436, 1428, 1254, 1254, 1436,
 /*   330 */  1425, 1512, 1436, 1410, 1436, 1425, 1436, 1655, 1655, 1277,
 /*   340 */  1277, 1654, 1654, 1277, 1522, 1642, 1498, 1438, 1444, 1444,
 /*   350 */  1444, 1444, 1277, 1379, 1297, 1642, 1642, 1297, 1498, 1438,
 /*   360 */  1297, 1438, 1297, 1277, 1379, 1538, 1649, 1277, 1379, 1254,
 /*   370 */  1516, 1516, 1512, 1277, 1293, 1279, 1601, 1601, 1288, 1288,
 /*   380 */  1609, 1350, 1277, 1254, 1293, 1291, 1289, 1297, 1512, 1277,
 /*   390 */  1379, 1277, 1379, 1619, 1619, 1615, 1615, 1615, 1629, 1629,
 /*   400 */  1412, 1660, 1660, 1571, 1412, 1394, 1394, 1629, 1258, 1258,
 /*   410 */  1258, 1258, 1258, 1254, 1254, 1254, 1254, 1254, 1624, 1254,
 /*   420 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1566, 1523, 1463,
 /*   430 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1577, 1254,
 /*   440 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   450 */  1300, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   460 */  1254, 1254, 1383, 1254, 1254, 1254, 1568, 1408, 1254, 1254,
 /*   470 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   480 */  1254, 1254, 1480, 1481, 1464, 1254, 1254, 1254, 1254, 1254,
 /*   490 */  1254, 1254, 1495, 1254, 1254, 1254, 1490, 1254, 1254, 1254,
 /*   500 */  1254, 1254, 1254, 1254, 1254, 1651, 1254, 1254, 1254, 1254,
 /*   510 */  1254, 1254, 1537, 1536, 1254, 1254, 1461, 1254, 1254, 1254,
 /*   520 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1290, 1254,
 /*   530 */  1254, 1254, 1254, 1606, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   540 */  1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
 /*   550 */  1280, 1254, 1254, 1254, 1254, 1254, 1364, 1254, 1254, 1254,
 /*   560 */  1254, 1254, 1254, 1254, 1254, 1639, 1254, 1254, 1254, 1254,
 /*   570 */  1254, 1451, 1304, 1301, 1254, 1254, 1262, 1254, 1254, 1254,
 /*   580 */  1373, 1254, 1365,
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
  /*  188 */ "ids",
  /*  189 */ "as",
  /*  190 */ "scanpt",
  /*  191 */ "likeop",
  /*  192 */ "dbnm",
  /*  193 */ "multiselect_op",
  /*  194 */ "in_op",
  /*  195 */ "explain",
  /*  196 */ "cmdx",
  /*  197 */ "cmd",
  /*  198 */ "transtype",
  /*  199 */ "trans_opt",
  /*  200 */ "savepoint_opt",
  /*  201 */ "create_table",
  /*  202 */ "createkw",
  /*  203 */ "temp",
  /*  204 */ "ifnotexists",
  /*  205 */ "create_table_args",
  /*  206 */ "columnlist",
  /*  207 */ "conslist_opt",
  /*  208 */ "table_option_set",
  /*  209 */ "select",
  /*  210 */ "table_option",
  /*  211 */ "columnname",
  /*  212 */ "typetoken",
  /*  213 */ "typename",
  /*  214 */ "signed",
  /*  215 */ "scantok",
  /*  216 */ "ccons",
  /*  217 */ "term",
  /*  218 */ "expr",
  /*  219 */ "onconf",
  /*  220 */ "sortorder",
  /*  221 */ "autoinc",
  /*  222 */ "eidlist_opt",
  /*  223 */ "refargs",
  /*  224 */ "defer_subclause",
  /*  225 */ "generated",
  /*  226 */ "refarg",
  /*  227 */ "refact",
  /*  228 */ "init_deferred_pred_opt",
  /*  229 */ "tconscomma",
  /*  230 */ "tcons",
  /*  231 */ "sortlist",
  /*  232 */ "eidlist",
  /*  233 */ "defer_subclause_opt",
  /*  234 */ "resolvetype",
  /*  235 */ "orconf",
  /*  236 */ "ifexists",
  /*  237 */ "fullname",
  /*  238 */ "wqlist",
  /*  239 */ "selectnowith",
  /*  240 */ "oneselect",
  /*  241 */ "distinct",
  /*  242 */ "selcollist",
  /*  243 */ "from",
  /*  244 */ "where_opt",
  /*  245 */ "groupby_opt",
  /*  246 */ "having_opt",
  /*  247 */ "orderby_opt",
  /*  248 */ "limit_opt",
  /*  249 */ "window_clause",
  /*  250 */ "values",
  /*  251 */ "nexprlist",
  /*  252 */ "mvalues",
  /*  253 */ "sclp",
  /*  254 */ "seltablist",
  /*  255 */ "stl_prefix",
  /*  256 */ "joinop",
  /*  257 */ "on_using",
  /*  258 */ "indexed_by",
  /*  259 */ "exprlist",
  /*  260 */ "xfullname",
  /*  261 */ "idlist",
  /*  262 */ "indexed_opt",
  /*  263 */ "nulls",
  /*  264 */ "with",
  /*  265 */ "where_opt_ret",
  /*  266 */ "setlist",
  /*  267 */ "insert_cmd",
  /*  268 */ "idlist_opt",
  /*  269 */ "upsert",
  /*  270 */ "returning",
  /*  271 */ "filter_over",
  /*  272 */ "between_op",
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
 /*   4 */ "selectnowith ::= selectnowith multiselect_op oneselect",
 /*   5 */ "multiselect_op ::= UNION",
 /*   6 */ "multiselect_op ::= UNION ALL",
 /*   7 */ "multiselect_op ::= EXCEPT|INTERSECT",
 /*   8 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt",
 /*   9 */ "distinct ::= DISTINCT",
 /*  10 */ "distinct ::= ALL",
 /*  11 */ "distinct ::=",
 /*  12 */ "sclp ::=",
 /*  13 */ "selcollist ::= sclp scanpt expr scanpt as",
 /*  14 */ "selcollist ::= sclp scanpt STAR",
 /*  15 */ "selcollist ::= sclp scanpt nm DOT STAR",
 /*  16 */ "as ::= AS nm",
 /*  17 */ "as ::=",
 /*  18 */ "from ::=",
 /*  19 */ "from ::= FROM seltablist",
 /*  20 */ "dbnm ::=",
 /*  21 */ "dbnm ::= DOT nm",
 /*  22 */ "orderby_opt ::=",
 /*  23 */ "orderby_opt ::= ORDER BY sortlist",
 /*  24 */ "sortlist ::= sortlist COMMA expr sortorder nulls",
 /*  25 */ "sortlist ::= expr sortorder nulls",
 /*  26 */ "sortorder ::= ASC",
 /*  27 */ "sortorder ::= DESC",
 /*  28 */ "sortorder ::=",
 /*  29 */ "nulls ::= NULLS FIRST",
 /*  30 */ "nulls ::= NULLS LAST",
 /*  31 */ "nulls ::=",
 /*  32 */ "groupby_opt ::=",
 /*  33 */ "groupby_opt ::= GROUP BY nexprlist",
 /*  34 */ "having_opt ::=",
 /*  35 */ "having_opt ::= HAVING expr",
 /*  36 */ "limit_opt ::=",
 /*  37 */ "limit_opt ::= LIMIT expr",
 /*  38 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /*  39 */ "limit_opt ::= LIMIT expr COMMA expr",
 /*  40 */ "where_opt ::=",
 /*  41 */ "where_opt ::= WHERE expr",
 /*  42 */ "expr ::= LP expr RP",
 /*  43 */ "expr ::= ID|INDEXED|JOIN_KW",
 /*  44 */ "expr ::= nm DOT nm",
 /*  45 */ "expr ::= nm DOT nm DOT nm",
 /*  46 */ "term ::= NULL|FLOAT|BLOB",
 /*  47 */ "term ::= STRING",
 /*  48 */ "term ::= INTEGER",
 /*  49 */ "expr ::= VARIABLE",
 /*  50 */ "expr ::= expr COLLATE ID|STRING",
 /*  51 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP",
 /*  52 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP",
 /*  53 */ "term ::= CTIME_KW",
 /*  54 */ "expr ::= expr AND expr",
 /*  55 */ "expr ::= expr OR expr",
 /*  56 */ "expr ::= expr LT|GT|GE|LE expr",
 /*  57 */ "expr ::= expr EQ|NE expr",
 /*  58 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
 /*  59 */ "expr ::= expr PLUS|MINUS expr",
 /*  60 */ "expr ::= expr STAR|SLASH|REM expr",
 /*  61 */ "expr ::= expr CONCAT expr",
 /*  62 */ "likeop ::= NOT LIKE_KW|MATCH",
 /*  63 */ "expr ::= expr likeop expr",
 /*  64 */ "expr ::= expr likeop expr ESCAPE expr",
 /*  65 */ "expr ::= expr ISNULL|NOTNULL",
 /*  66 */ "expr ::= expr NOT NULL",
 /*  67 */ "expr ::= expr IS expr",
 /*  68 */ "expr ::= expr IS NOT expr",
 /*  69 */ "expr ::= expr IS NOT DISTINCT FROM expr",
 /*  70 */ "expr ::= expr IS DISTINCT FROM expr",
 /*  71 */ "expr ::= NOT expr",
 /*  72 */ "expr ::= BITNOT expr",
 /*  73 */ "expr ::= PLUS|MINUS expr",
 /*  74 */ "between_op ::= BETWEEN",
 /*  75 */ "between_op ::= NOT BETWEEN",
 /*  76 */ "expr ::= expr between_op expr AND expr",
 /*  77 */ "in_op ::= IN",
 /*  78 */ "in_op ::= NOT IN",
 /*  79 */ "expr ::= expr in_op LP exprlist RP",
 /*  80 */ "expr ::= LP select RP",
 /*  81 */ "expr ::= expr in_op LP select RP",
 /*  82 */ "expr ::= expr in_op nm dbnm paren_exprlist",
 /*  83 */ "expr ::= EXISTS LP select RP",
 /*  84 */ "expr ::= CASE case_operand case_exprlist case_else END",
 /*  85 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
 /*  86 */ "case_exprlist ::= WHEN expr THEN expr",
 /*  87 */ "case_else ::= ELSE expr",
 /*  88 */ "case_else ::=",
 /*  89 */ "case_operand ::=",
 /*  90 */ "exprlist ::=",
 /*  91 */ "nexprlist ::= nexprlist COMMA expr",
 /*  92 */ "nexprlist ::= expr",
 /*  93 */ "paren_exprlist ::=",
 /*  94 */ "paren_exprlist ::= LP exprlist RP",
 /*  95 */ "input ::= cmdlist",
 /*  96 */ "cmdlist ::= cmdlist ecmd",
 /*  97 */ "cmdlist ::= ecmd",
 /*  98 */ "ecmd ::= SEMI",
 /*  99 */ "ecmd ::= cmdx SEMI",
 /* 100 */ "nm ::= ID|INDEXED|JOIN_KW",
 /* 101 */ "nm ::= STRING",
 /* 102 */ "selectnowith ::= oneselect",
 /* 103 */ "sclp ::= selcollist COMMA",
 /* 104 */ "expr ::= term",
 /* 105 */ "likeop ::= LIKE_KW|MATCH",
 /* 106 */ "case_operand ::= expr",
 /* 107 */ "exprlist ::= nexprlist",
 /* 108 */ "explain ::= EXPLAIN",
 /* 109 */ "explain ::= EXPLAIN QUERY PLAN",
 /* 110 */ "cmd ::= BEGIN transtype trans_opt",
 /* 111 */ "transtype ::=",
 /* 112 */ "transtype ::= DEFERRED",
 /* 113 */ "transtype ::= IMMEDIATE",
 /* 114 */ "transtype ::= EXCLUSIVE",
 /* 115 */ "cmd ::= COMMIT|END trans_opt",
 /* 116 */ "cmd ::= ROLLBACK trans_opt",
 /* 117 */ "cmd ::= SAVEPOINT nm",
 /* 118 */ "cmd ::= RELEASE savepoint_opt nm",
 /* 119 */ "cmd ::= ROLLBACK trans_opt TO savepoint_opt nm",
 /* 120 */ "create_table ::= createkw temp TABLE ifnotexists nm dbnm",
 /* 121 */ "createkw ::= CREATE",
 /* 122 */ "ifnotexists ::=",
 /* 123 */ "ifnotexists ::= IF NOT EXISTS",
 /* 124 */ "temp ::= TEMP",
 /* 125 */ "temp ::=",
 /* 126 */ "create_table_args ::= LP columnlist conslist_opt RP table_option_set",
 /* 127 */ "create_table_args ::= AS select",
 /* 128 */ "table_option_set ::=",
 /* 129 */ "table_option_set ::= table_option_set COMMA table_option",
 /* 130 */ "table_option ::= WITHOUT nm",
 /* 131 */ "table_option ::= nm",
 /* 132 */ "columnname ::= nm typetoken",
 /* 133 */ "typetoken ::=",
 /* 134 */ "typetoken ::= typename LP signed RP",
 /* 135 */ "typetoken ::= typename LP signed COMMA signed RP",
 /* 136 */ "typename ::= typename ID|STRING",
 /* 137 */ "scantok ::=",
 /* 138 */ "ccons ::= CONSTRAINT nm",
 /* 139 */ "ccons ::= DEFAULT scantok term",
 /* 140 */ "ccons ::= DEFAULT LP expr RP",
 /* 141 */ "ccons ::= DEFAULT PLUS scantok term",
 /* 142 */ "ccons ::= DEFAULT MINUS scantok term",
 /* 143 */ "ccons ::= DEFAULT scantok ID|INDEXED",
 /* 144 */ "ccons ::= NOT NULL onconf",
 /* 145 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
 /* 146 */ "ccons ::= UNIQUE onconf",
 /* 147 */ "ccons ::= CHECK LP expr RP",
 /* 148 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
 /* 149 */ "ccons ::= defer_subclause",
 /* 150 */ "ccons ::= COLLATE ID|STRING",
 /* 151 */ "generated ::= LP expr RP",
 /* 152 */ "generated ::= LP expr RP ID",
 /* 153 */ "autoinc ::=",
 /* 154 */ "autoinc ::= AUTOINCR",
 /* 155 */ "refargs ::=",
 /* 156 */ "refargs ::= refargs refarg",
 /* 157 */ "refarg ::= MATCH nm",
 /* 158 */ "refarg ::= ON INSERT refact",
 /* 159 */ "refarg ::= ON DELETE refact",
 /* 160 */ "refarg ::= ON UPDATE refact",
 /* 161 */ "refact ::= SET NULL",
 /* 162 */ "refact ::= SET DEFAULT",
 /* 163 */ "refact ::= CASCADE",
 /* 164 */ "refact ::= RESTRICT",
 /* 165 */ "refact ::= NO ACTION",
 /* 166 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
 /* 167 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
 /* 168 */ "init_deferred_pred_opt ::=",
 /* 169 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
 /* 170 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
 /* 171 */ "conslist_opt ::=",
 /* 172 */ "tconscomma ::= COMMA",
 /* 173 */ "tcons ::= CONSTRAINT nm",
 /* 174 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf",
 /* 175 */ "tcons ::= UNIQUE LP sortlist RP onconf",
 /* 176 */ "tcons ::= CHECK LP expr RP onconf",
 /* 177 */ "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /* 178 */ "defer_subclause_opt ::=",
 /* 179 */ "onconf ::=",
 /* 180 */ "onconf ::= ON CONFLICT resolvetype",
 /* 181 */ "orconf ::=",
 /* 182 */ "orconf ::= OR resolvetype",
 /* 183 */ "resolvetype ::= IGNORE",
 /* 184 */ "resolvetype ::= REPLACE",
 /* 185 */ "cmd ::= DROP TABLE ifexists fullname",
 /* 186 */ "ifexists ::= IF EXISTS",
 /* 187 */ "ifexists ::=",
 /* 188 */ "cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select",
 /* 189 */ "cmd ::= DROP VIEW ifexists fullname",
 /* 190 */ "select ::= WITH wqlist selectnowith",
 /* 191 */ "select ::= WITH RECURSIVE wqlist selectnowith",
 /* 192 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt",
 /* 193 */ "values ::= VALUES LP nexprlist RP",
 /* 194 */ "oneselect ::= mvalues",
 /* 195 */ "mvalues ::= values COMMA LP nexprlist RP",
 /* 196 */ "mvalues ::= mvalues COMMA LP nexprlist RP",
 /* 197 */ "stl_prefix ::= seltablist joinop",
 /* 198 */ "stl_prefix ::=",
 /* 199 */ "seltablist ::= stl_prefix nm dbnm as on_using",
 /* 200 */ "seltablist ::= stl_prefix nm dbnm as indexed_by on_using",
 /* 201 */ "seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using",
 /* 202 */ "seltablist ::= stl_prefix LP select RP as on_using",
 /* 203 */ "seltablist ::= stl_prefix LP seltablist RP as on_using",
 /* 204 */ "fullname ::= nm",
 /* 205 */ "fullname ::= nm DOT nm",
 /* 206 */ "xfullname ::= nm",
 /* 207 */ "xfullname ::= nm DOT nm",
 /* 208 */ "xfullname ::= nm DOT nm AS nm",
 /* 209 */ "xfullname ::= nm AS nm",
 /* 210 */ "joinop ::= COMMA|JOIN",
 /* 211 */ "joinop ::= JOIN_KW JOIN",
 /* 212 */ "joinop ::= JOIN_KW nm JOIN",
 /* 213 */ "joinop ::= JOIN_KW nm nm JOIN",
 /* 214 */ "on_using ::= ON expr",
 /* 215 */ "on_using ::= USING LP idlist RP",
 /* 216 */ "on_using ::=",
 /* 217 */ "indexed_opt ::=",
 /* 218 */ "indexed_by ::= INDEXED BY nm",
 /* 219 */ "indexed_by ::= NOT INDEXED",
 /* 220 */ "cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret",
 /* 221 */ "where_opt_ret ::=",
 /* 222 */ "where_opt_ret ::= WHERE expr",
 /* 223 */ "where_opt_ret ::= RETURNING selcollist",
 /* 224 */ "where_opt_ret ::= WHERE expr RETURNING selcollist",
 /* 225 */ "cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret",
 /* 226 */ "setlist ::= setlist COMMA nm EQ expr",
 /* 227 */ "setlist ::= setlist COMMA LP idlist RP EQ expr",
 /* 228 */ "setlist ::= nm EQ expr",
 /* 229 */ "setlist ::= LP idlist RP EQ expr",
 /* 230 */ "cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert",
 /* 231 */ "cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning",
 /* 232 */ "upsert ::=",
 /* 233 */ "upsert ::= RETURNING selcollist",
 /* 234 */ "upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert",
 /* 235 */ "upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert",
 /* 236 */ "upsert ::= ON CONFLICT DO NOTHING returning",
 /* 237 */ "upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning",
 /* 238 */ "returning ::= RETURNING selcollist",
 /* 239 */ "insert_cmd ::= INSERT orconf",
 /* 240 */ "insert_cmd ::= REPLACE",
 /* 241 */ "idlist_opt ::=",
 /* 242 */ "idlist_opt ::= LP idlist RP",
 /* 243 */ "idlist ::= idlist COMMA nm",
 /* 244 */ "idlist ::= nm",
 /* 245 */ "expr ::= CAST LP expr AS typetoken RP",
 /* 246 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP",
 /* 247 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over",
 /* 248 */ "expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over",
 /* 249 */ "expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over",
 /* 250 */ "expr ::= LP nexprlist COMMA expr RP",
 /* 251 */ "expr ::= expr PTR expr",
 /* 252 */ "cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt",
 /* 253 */ "uniqueflag ::= UNIQUE",
 /* 254 */ "uniqueflag ::=",
 /* 255 */ "eidlist_opt ::=",
 /* 256 */ "eidlist_opt ::= LP eidlist RP",
 /* 257 */ "eidlist ::= eidlist COMMA nm collate sortorder",
 /* 258 */ "eidlist ::= nm collate sortorder",
 /* 259 */ "collate ::=",
 /* 260 */ "collate ::= COLLATE ID|STRING",
 /* 261 */ "cmd ::= DROP INDEX ifexists fullname",
 /* 262 */ "cmd ::= VACUUM vinto",
 /* 263 */ "cmd ::= VACUUM nm vinto",
 /* 264 */ "vinto ::= INTO expr",
 /* 265 */ "vinto ::=",
 /* 266 */ "cmd ::= PRAGMA nm dbnm",
 /* 267 */ "cmd ::= PRAGMA nm dbnm EQ nmnum",
 /* 268 */ "cmd ::= PRAGMA nm dbnm LP nmnum RP",
 /* 269 */ "cmd ::= PRAGMA nm dbnm EQ minus_num",
 /* 270 */ "cmd ::= PRAGMA nm dbnm LP minus_num RP",
 /* 271 */ "plus_num ::= PLUS INTEGER|FLOAT",
 /* 272 */ "minus_num ::= MINUS INTEGER|FLOAT",
 /* 273 */ "cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END",
 /* 274 */ "trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause",
 /* 275 */ "trigger_time ::= BEFORE|AFTER",
 /* 276 */ "trigger_time ::= INSTEAD OF",
 /* 277 */ "trigger_time ::=",
 /* 278 */ "trigger_event ::= DELETE|INSERT",
 /* 279 */ "trigger_event ::= UPDATE",
 /* 280 */ "trigger_event ::= UPDATE OF idlist",
 /* 281 */ "when_clause ::=",
 /* 282 */ "when_clause ::= WHEN expr",
 /* 283 */ "trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI",
 /* 284 */ "trigger_cmd_list ::= trigger_cmd SEMI",
 /* 285 */ "trnm ::= nm DOT nm",
 /* 286 */ "tridxby ::= INDEXED BY nm",
 /* 287 */ "tridxby ::= NOT INDEXED",
 /* 288 */ "trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt",
 /* 289 */ "trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt",
 /* 290 */ "trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt",
 /* 291 */ "trigger_cmd ::= scanpt select scanpt",
 /* 292 */ "expr ::= RAISE LP IGNORE RP",
 /* 293 */ "expr ::= RAISE LP raisetype COMMA expr RP",
 /* 294 */ "raisetype ::= ROLLBACK",
 /* 295 */ "raisetype ::= ABORT",
 /* 296 */ "raisetype ::= FAIL",
 /* 297 */ "cmd ::= DROP TRIGGER ifexists fullname",
 /* 298 */ "cmd ::= ATTACH database_kw_opt expr AS expr key_opt",
 /* 299 */ "cmd ::= DETACH database_kw_opt expr",
 /* 300 */ "key_opt ::=",
 /* 301 */ "key_opt ::= KEY expr",
 /* 302 */ "cmd ::= REINDEX",
 /* 303 */ "cmd ::= REINDEX nm dbnm",
 /* 304 */ "cmd ::= ANALYZE",
 /* 305 */ "cmd ::= ANALYZE nm dbnm",
 /* 306 */ "cmd ::= ALTER TABLE fullname RENAME TO nm",
 /* 307 */ "cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist",
 /* 308 */ "cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm",
 /* 309 */ "add_column_fullname ::= fullname",
 /* 310 */ "cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm",
 /* 311 */ "cmd ::= create_vtab",
 /* 312 */ "cmd ::= create_vtab LP vtabarglist RP",
 /* 313 */ "create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm",
 /* 314 */ "vtabarg ::=",
 /* 315 */ "vtabargtoken ::= ANY",
 /* 316 */ "vtabargtoken ::= lp anylist RP",
 /* 317 */ "lp ::= LP",
 /* 318 */ "with ::= WITH wqlist",
 /* 319 */ "with ::= WITH RECURSIVE wqlist",
 /* 320 */ "wqas ::= AS",
 /* 321 */ "wqas ::= AS MATERIALIZED",
 /* 322 */ "wqas ::= AS NOT MATERIALIZED",
 /* 323 */ "wqitem ::= withnm eidlist_opt wqas LP select RP",
 /* 324 */ "withnm ::= nm",
 /* 325 */ "wqlist ::= wqitem",
 /* 326 */ "wqlist ::= wqlist COMMA wqitem",
 /* 327 */ "windowdefn_list ::= windowdefn_list COMMA windowdefn",
 /* 328 */ "windowdefn ::= nm AS LP window RP",
 /* 329 */ "window ::= PARTITION BY nexprlist orderby_opt frame_opt",
 /* 330 */ "window ::= nm PARTITION BY nexprlist orderby_opt frame_opt",
 /* 331 */ "window ::= ORDER BY sortlist frame_opt",
 /* 332 */ "window ::= nm ORDER BY sortlist frame_opt",
 /* 333 */ "window ::= nm frame_opt",
 /* 334 */ "frame_opt ::=",
 /* 335 */ "frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt",
 /* 336 */ "frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt",
 /* 337 */ "range_or_rows ::= RANGE|ROWS|GROUPS",
 /* 338 */ "frame_bound_s ::= frame_bound",
 /* 339 */ "frame_bound_s ::= UNBOUNDED PRECEDING",
 /* 340 */ "frame_bound_e ::= frame_bound",
 /* 341 */ "frame_bound_e ::= UNBOUNDED FOLLOWING",
 /* 342 */ "frame_bound ::= expr PRECEDING|FOLLOWING",
 /* 343 */ "frame_bound ::= CURRENT ROW",
 /* 344 */ "frame_exclude_opt ::=",
 /* 345 */ "frame_exclude_opt ::= EXCLUDE frame_exclude",
 /* 346 */ "frame_exclude ::= NO OTHERS",
 /* 347 */ "frame_exclude ::= CURRENT ROW",
 /* 348 */ "frame_exclude ::= GROUP|TIES",
 /* 349 */ "window_clause ::= WINDOW windowdefn_list",
 /* 350 */ "filter_over ::= filter_clause over_clause",
 /* 351 */ "filter_over ::= over_clause",
 /* 352 */ "filter_over ::= filter_clause",
 /* 353 */ "over_clause ::= OVER LP window RP",
 /* 354 */ "over_clause ::= OVER nm",
 /* 355 */ "filter_clause ::= FILTER LP WHERE expr RP",
 /* 356 */ "term ::= QNUMBER",
 /* 357 */ "ecmd ::= explain cmdx SEMI",
 /* 358 */ "trans_opt ::=",
 /* 359 */ "trans_opt ::= TRANSACTION",
 /* 360 */ "trans_opt ::= TRANSACTION nm",
 /* 361 */ "savepoint_opt ::= SAVEPOINT",
 /* 362 */ "savepoint_opt ::=",
 /* 363 */ "cmd ::= create_table create_table_args",
 /* 364 */ "table_option_set ::= table_option",
 /* 365 */ "columnlist ::= columnlist COMMA columnname carglist",
 /* 366 */ "columnlist ::= columnname carglist",
 /* 367 */ "typetoken ::= typename",
 /* 368 */ "typename ::= ID|STRING",
 /* 369 */ "signed ::= plus_num",
 /* 370 */ "signed ::= minus_num",
 /* 371 */ "carglist ::= carglist ccons",
 /* 372 */ "carglist ::=",
 /* 373 */ "ccons ::= NULL onconf",
 /* 374 */ "ccons ::= GENERATED ALWAYS AS generated",
 /* 375 */ "ccons ::= AS generated",
 /* 376 */ "conslist_opt ::= COMMA conslist",
 /* 377 */ "conslist ::= conslist tconscomma tcons",
 /* 378 */ "conslist ::= tcons",
 /* 379 */ "tconscomma ::=",
 /* 380 */ "defer_subclause_opt ::= defer_subclause",
 /* 381 */ "resolvetype ::= raisetype",
 /* 382 */ "oneselect ::= values",
 /* 383 */ "as ::= ID|STRING",
 /* 384 */ "indexed_opt ::= indexed_by",
 /* 385 */ "returning ::=",
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
   196,  /* (0) cmdx ::= cmd */
   190,  /* (1) scanpt ::= */
   197,  /* (2) cmd ::= select */
   209,  /* (3) select ::= selectnowith */
   239,  /* (4) selectnowith ::= selectnowith multiselect_op oneselect */
   193,  /* (5) multiselect_op ::= UNION */
   193,  /* (6) multiselect_op ::= UNION ALL */
   193,  /* (7) multiselect_op ::= EXCEPT|INTERSECT */
   240,  /* (8) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
   241,  /* (9) distinct ::= DISTINCT */
   241,  /* (10) distinct ::= ALL */
   241,  /* (11) distinct ::= */
   253,  /* (12) sclp ::= */
   242,  /* (13) selcollist ::= sclp scanpt expr scanpt as */
   242,  /* (14) selcollist ::= sclp scanpt STAR */
   242,  /* (15) selcollist ::= sclp scanpt nm DOT STAR */
   189,  /* (16) as ::= AS nm */
   189,  /* (17) as ::= */
   243,  /* (18) from ::= */
   243,  /* (19) from ::= FROM seltablist */
   192,  /* (20) dbnm ::= */
   192,  /* (21) dbnm ::= DOT nm */
   247,  /* (22) orderby_opt ::= */
   247,  /* (23) orderby_opt ::= ORDER BY sortlist */
   231,  /* (24) sortlist ::= sortlist COMMA expr sortorder nulls */
   231,  /* (25) sortlist ::= expr sortorder nulls */
   220,  /* (26) sortorder ::= ASC */
   220,  /* (27) sortorder ::= DESC */
   220,  /* (28) sortorder ::= */
   263,  /* (29) nulls ::= NULLS FIRST */
   263,  /* (30) nulls ::= NULLS LAST */
   263,  /* (31) nulls ::= */
   245,  /* (32) groupby_opt ::= */
   245,  /* (33) groupby_opt ::= GROUP BY nexprlist */
   246,  /* (34) having_opt ::= */
   246,  /* (35) having_opt ::= HAVING expr */
   248,  /* (36) limit_opt ::= */
   248,  /* (37) limit_opt ::= LIMIT expr */
   248,  /* (38) limit_opt ::= LIMIT expr OFFSET expr */
   248,  /* (39) limit_opt ::= LIMIT expr COMMA expr */
   244,  /* (40) where_opt ::= */
   244,  /* (41) where_opt ::= WHERE expr */
   218,  /* (42) expr ::= LP expr RP */
   218,  /* (43) expr ::= ID|INDEXED|JOIN_KW */
   218,  /* (44) expr ::= nm DOT nm */
   218,  /* (45) expr ::= nm DOT nm DOT nm */
   217,  /* (46) term ::= NULL|FLOAT|BLOB */
   217,  /* (47) term ::= STRING */
   217,  /* (48) term ::= INTEGER */
   218,  /* (49) expr ::= VARIABLE */
   218,  /* (50) expr ::= expr COLLATE ID|STRING */
   218,  /* (51) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   218,  /* (52) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   217,  /* (53) term ::= CTIME_KW */
   218,  /* (54) expr ::= expr AND expr */
   218,  /* (55) expr ::= expr OR expr */
   218,  /* (56) expr ::= expr LT|GT|GE|LE expr */
   218,  /* (57) expr ::= expr EQ|NE expr */
   218,  /* (58) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
   218,  /* (59) expr ::= expr PLUS|MINUS expr */
   218,  /* (60) expr ::= expr STAR|SLASH|REM expr */
   218,  /* (61) expr ::= expr CONCAT expr */
   191,  /* (62) likeop ::= NOT LIKE_KW|MATCH */
   218,  /* (63) expr ::= expr likeop expr */
   218,  /* (64) expr ::= expr likeop expr ESCAPE expr */
   218,  /* (65) expr ::= expr ISNULL|NOTNULL */
   218,  /* (66) expr ::= expr NOT NULL */
   218,  /* (67) expr ::= expr IS expr */
   218,  /* (68) expr ::= expr IS NOT expr */
   218,  /* (69) expr ::= expr IS NOT DISTINCT FROM expr */
   218,  /* (70) expr ::= expr IS DISTINCT FROM expr */
   218,  /* (71) expr ::= NOT expr */
   218,  /* (72) expr ::= BITNOT expr */
   218,  /* (73) expr ::= PLUS|MINUS expr */
   272,  /* (74) between_op ::= BETWEEN */
   272,  /* (75) between_op ::= NOT BETWEEN */
   218,  /* (76) expr ::= expr between_op expr AND expr */
   194,  /* (77) in_op ::= IN */
   194,  /* (78) in_op ::= NOT IN */
   218,  /* (79) expr ::= expr in_op LP exprlist RP */
   218,  /* (80) expr ::= LP select RP */
   218,  /* (81) expr ::= expr in_op LP select RP */
   218,  /* (82) expr ::= expr in_op nm dbnm paren_exprlist */
   218,  /* (83) expr ::= EXISTS LP select RP */
   218,  /* (84) expr ::= CASE case_operand case_exprlist case_else END */
   275,  /* (85) case_exprlist ::= case_exprlist WHEN expr THEN expr */
   275,  /* (86) case_exprlist ::= WHEN expr THEN expr */
   276,  /* (87) case_else ::= ELSE expr */
   276,  /* (88) case_else ::= */
   274,  /* (89) case_operand ::= */
   259,  /* (90) exprlist ::= */
   251,  /* (91) nexprlist ::= nexprlist COMMA expr */
   251,  /* (92) nexprlist ::= expr */
   273,  /* (93) paren_exprlist ::= */
   273,  /* (94) paren_exprlist ::= LP exprlist RP */
   319,  /* (95) input ::= cmdlist */
   320,  /* (96) cmdlist ::= cmdlist ecmd */
   320,  /* (97) cmdlist ::= ecmd */
   321,  /* (98) ecmd ::= SEMI */
   321,  /* (99) ecmd ::= cmdx SEMI */
   187,  /* (100) nm ::= ID|INDEXED|JOIN_KW */
   187,  /* (101) nm ::= STRING */
   239,  /* (102) selectnowith ::= oneselect */
   253,  /* (103) sclp ::= selcollist COMMA */
   218,  /* (104) expr ::= term */
   191,  /* (105) likeop ::= LIKE_KW|MATCH */
   274,  /* (106) case_operand ::= expr */
   259,  /* (107) exprlist ::= nexprlist */
   195,  /* (108) explain ::= EXPLAIN */
   195,  /* (109) explain ::= EXPLAIN QUERY PLAN */
   197,  /* (110) cmd ::= BEGIN transtype trans_opt */
   198,  /* (111) transtype ::= */
   198,  /* (112) transtype ::= DEFERRED */
   198,  /* (113) transtype ::= IMMEDIATE */
   198,  /* (114) transtype ::= EXCLUSIVE */
   197,  /* (115) cmd ::= COMMIT|END trans_opt */
   197,  /* (116) cmd ::= ROLLBACK trans_opt */
   197,  /* (117) cmd ::= SAVEPOINT nm */
   197,  /* (118) cmd ::= RELEASE savepoint_opt nm */
   197,  /* (119) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
   201,  /* (120) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   202,  /* (121) createkw ::= CREATE */
   204,  /* (122) ifnotexists ::= */
   204,  /* (123) ifnotexists ::= IF NOT EXISTS */
   203,  /* (124) temp ::= TEMP */
   203,  /* (125) temp ::= */
   205,  /* (126) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   205,  /* (127) create_table_args ::= AS select */
   208,  /* (128) table_option_set ::= */
   208,  /* (129) table_option_set ::= table_option_set COMMA table_option */
   210,  /* (130) table_option ::= WITHOUT nm */
   210,  /* (131) table_option ::= nm */
   211,  /* (132) columnname ::= nm typetoken */
   212,  /* (133) typetoken ::= */
   212,  /* (134) typetoken ::= typename LP signed RP */
   212,  /* (135) typetoken ::= typename LP signed COMMA signed RP */
   213,  /* (136) typename ::= typename ID|STRING */
   215,  /* (137) scantok ::= */
   216,  /* (138) ccons ::= CONSTRAINT nm */
   216,  /* (139) ccons ::= DEFAULT scantok term */
   216,  /* (140) ccons ::= DEFAULT LP expr RP */
   216,  /* (141) ccons ::= DEFAULT PLUS scantok term */
   216,  /* (142) ccons ::= DEFAULT MINUS scantok term */
   216,  /* (143) ccons ::= DEFAULT scantok ID|INDEXED */
   216,  /* (144) ccons ::= NOT NULL onconf */
   216,  /* (145) ccons ::= PRIMARY KEY sortorder onconf autoinc */
   216,  /* (146) ccons ::= UNIQUE onconf */
   216,  /* (147) ccons ::= CHECK LP expr RP */
   216,  /* (148) ccons ::= REFERENCES nm eidlist_opt refargs */
   216,  /* (149) ccons ::= defer_subclause */
   216,  /* (150) ccons ::= COLLATE ID|STRING */
   225,  /* (151) generated ::= LP expr RP */
   225,  /* (152) generated ::= LP expr RP ID */
   221,  /* (153) autoinc ::= */
   221,  /* (154) autoinc ::= AUTOINCR */
   223,  /* (155) refargs ::= */
   223,  /* (156) refargs ::= refargs refarg */
   226,  /* (157) refarg ::= MATCH nm */
   226,  /* (158) refarg ::= ON INSERT refact */
   226,  /* (159) refarg ::= ON DELETE refact */
   226,  /* (160) refarg ::= ON UPDATE refact */
   227,  /* (161) refact ::= SET NULL */
   227,  /* (162) refact ::= SET DEFAULT */
   227,  /* (163) refact ::= CASCADE */
   227,  /* (164) refact ::= RESTRICT */
   227,  /* (165) refact ::= NO ACTION */
   224,  /* (166) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
   224,  /* (167) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
   228,  /* (168) init_deferred_pred_opt ::= */
   228,  /* (169) init_deferred_pred_opt ::= INITIALLY DEFERRED */
   228,  /* (170) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
   207,  /* (171) conslist_opt ::= */
   229,  /* (172) tconscomma ::= COMMA */
   230,  /* (173) tcons ::= CONSTRAINT nm */
   230,  /* (174) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
   230,  /* (175) tcons ::= UNIQUE LP sortlist RP onconf */
   230,  /* (176) tcons ::= CHECK LP expr RP onconf */
   230,  /* (177) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
   233,  /* (178) defer_subclause_opt ::= */
   219,  /* (179) onconf ::= */
   219,  /* (180) onconf ::= ON CONFLICT resolvetype */
   235,  /* (181) orconf ::= */
   235,  /* (182) orconf ::= OR resolvetype */
   234,  /* (183) resolvetype ::= IGNORE */
   234,  /* (184) resolvetype ::= REPLACE */
   197,  /* (185) cmd ::= DROP TABLE ifexists fullname */
   236,  /* (186) ifexists ::= IF EXISTS */
   236,  /* (187) ifexists ::= */
   197,  /* (188) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   197,  /* (189) cmd ::= DROP VIEW ifexists fullname */
   209,  /* (190) select ::= WITH wqlist selectnowith */
   209,  /* (191) select ::= WITH RECURSIVE wqlist selectnowith */
   240,  /* (192) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   250,  /* (193) values ::= VALUES LP nexprlist RP */
   240,  /* (194) oneselect ::= mvalues */
   252,  /* (195) mvalues ::= values COMMA LP nexprlist RP */
   252,  /* (196) mvalues ::= mvalues COMMA LP nexprlist RP */
   255,  /* (197) stl_prefix ::= seltablist joinop */
   255,  /* (198) stl_prefix ::= */
   254,  /* (199) seltablist ::= stl_prefix nm dbnm as on_using */
   254,  /* (200) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   254,  /* (201) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   254,  /* (202) seltablist ::= stl_prefix LP select RP as on_using */
   254,  /* (203) seltablist ::= stl_prefix LP seltablist RP as on_using */
   237,  /* (204) fullname ::= nm */
   237,  /* (205) fullname ::= nm DOT nm */
   260,  /* (206) xfullname ::= nm */
   260,  /* (207) xfullname ::= nm DOT nm */
   260,  /* (208) xfullname ::= nm DOT nm AS nm */
   260,  /* (209) xfullname ::= nm AS nm */
   256,  /* (210) joinop ::= COMMA|JOIN */
   256,  /* (211) joinop ::= JOIN_KW JOIN */
   256,  /* (212) joinop ::= JOIN_KW nm JOIN */
   256,  /* (213) joinop ::= JOIN_KW nm nm JOIN */
   257,  /* (214) on_using ::= ON expr */
   257,  /* (215) on_using ::= USING LP idlist RP */
   257,  /* (216) on_using ::= */
   262,  /* (217) indexed_opt ::= */
   258,  /* (218) indexed_by ::= INDEXED BY nm */
   258,  /* (219) indexed_by ::= NOT INDEXED */
   197,  /* (220) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
   265,  /* (221) where_opt_ret ::= */
   265,  /* (222) where_opt_ret ::= WHERE expr */
   265,  /* (223) where_opt_ret ::= RETURNING selcollist */
   265,  /* (224) where_opt_ret ::= WHERE expr RETURNING selcollist */
   197,  /* (225) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   266,  /* (226) setlist ::= setlist COMMA nm EQ expr */
   266,  /* (227) setlist ::= setlist COMMA LP idlist RP EQ expr */
   266,  /* (228) setlist ::= nm EQ expr */
   266,  /* (229) setlist ::= LP idlist RP EQ expr */
   197,  /* (230) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   197,  /* (231) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
   269,  /* (232) upsert ::= */
   269,  /* (233) upsert ::= RETURNING selcollist */
   269,  /* (234) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
   269,  /* (235) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
   269,  /* (236) upsert ::= ON CONFLICT DO NOTHING returning */
   269,  /* (237) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
   270,  /* (238) returning ::= RETURNING selcollist */
   267,  /* (239) insert_cmd ::= INSERT orconf */
   267,  /* (240) insert_cmd ::= REPLACE */
   268,  /* (241) idlist_opt ::= */
   268,  /* (242) idlist_opt ::= LP idlist RP */
   261,  /* (243) idlist ::= idlist COMMA nm */
   261,  /* (244) idlist ::= nm */
   218,  /* (245) expr ::= CAST LP expr AS typetoken RP */
   218,  /* (246) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   218,  /* (247) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   218,  /* (248) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   218,  /* (249) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   218,  /* (250) expr ::= LP nexprlist COMMA expr RP */
   218,  /* (251) expr ::= expr PTR expr */
   197,  /* (252) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
   277,  /* (253) uniqueflag ::= UNIQUE */
   277,  /* (254) uniqueflag ::= */
   222,  /* (255) eidlist_opt ::= */
   222,  /* (256) eidlist_opt ::= LP eidlist RP */
   232,  /* (257) eidlist ::= eidlist COMMA nm collate sortorder */
   232,  /* (258) eidlist ::= nm collate sortorder */
   278,  /* (259) collate ::= */
   278,  /* (260) collate ::= COLLATE ID|STRING */
   197,  /* (261) cmd ::= DROP INDEX ifexists fullname */
   197,  /* (262) cmd ::= VACUUM vinto */
   197,  /* (263) cmd ::= VACUUM nm vinto */
   279,  /* (264) vinto ::= INTO expr */
   279,  /* (265) vinto ::= */
   197,  /* (266) cmd ::= PRAGMA nm dbnm */
   197,  /* (267) cmd ::= PRAGMA nm dbnm EQ nmnum */
   197,  /* (268) cmd ::= PRAGMA nm dbnm LP nmnum RP */
   197,  /* (269) cmd ::= PRAGMA nm dbnm EQ minus_num */
   197,  /* (270) cmd ::= PRAGMA nm dbnm LP minus_num RP */
   282,  /* (271) plus_num ::= PLUS INTEGER|FLOAT */
   281,  /* (272) minus_num ::= MINUS INTEGER|FLOAT */
   197,  /* (273) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
   283,  /* (274) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
   285,  /* (275) trigger_time ::= BEFORE|AFTER */
   285,  /* (276) trigger_time ::= INSTEAD OF */
   285,  /* (277) trigger_time ::= */
   286,  /* (278) trigger_event ::= DELETE|INSERT */
   286,  /* (279) trigger_event ::= UPDATE */
   286,  /* (280) trigger_event ::= UPDATE OF idlist */
   288,  /* (281) when_clause ::= */
   288,  /* (282) when_clause ::= WHEN expr */
   284,  /* (283) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
   284,  /* (284) trigger_cmd_list ::= trigger_cmd SEMI */
   290,  /* (285) trnm ::= nm DOT nm */
   291,  /* (286) tridxby ::= INDEXED BY nm */
   291,  /* (287) tridxby ::= NOT INDEXED */
   289,  /* (288) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
   289,  /* (289) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
   289,  /* (290) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
   289,  /* (291) trigger_cmd ::= scanpt select scanpt */
   218,  /* (292) expr ::= RAISE LP IGNORE RP */
   218,  /* (293) expr ::= RAISE LP raisetype COMMA expr RP */
   292,  /* (294) raisetype ::= ROLLBACK */
   292,  /* (295) raisetype ::= ABORT */
   292,  /* (296) raisetype ::= FAIL */
   197,  /* (297) cmd ::= DROP TRIGGER ifexists fullname */
   197,  /* (298) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
   197,  /* (299) cmd ::= DETACH database_kw_opt expr */
   294,  /* (300) key_opt ::= */
   294,  /* (301) key_opt ::= KEY expr */
   197,  /* (302) cmd ::= REINDEX */
   197,  /* (303) cmd ::= REINDEX nm dbnm */
   197,  /* (304) cmd ::= ANALYZE */
   197,  /* (305) cmd ::= ANALYZE nm dbnm */
   197,  /* (306) cmd ::= ALTER TABLE fullname RENAME TO nm */
   197,  /* (307) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
   197,  /* (308) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
   295,  /* (309) add_column_fullname ::= fullname */
   197,  /* (310) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
   197,  /* (311) cmd ::= create_vtab */
   197,  /* (312) cmd ::= create_vtab LP vtabarglist RP */
   298,  /* (313) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
   300,  /* (314) vtabarg ::= */
   301,  /* (315) vtabargtoken ::= ANY */
   301,  /* (316) vtabargtoken ::= lp anylist RP */
   302,  /* (317) lp ::= LP */
   264,  /* (318) with ::= WITH wqlist */
   264,  /* (319) with ::= WITH RECURSIVE wqlist */
   304,  /* (320) wqas ::= AS */
   304,  /* (321) wqas ::= AS MATERIALIZED */
   304,  /* (322) wqas ::= AS NOT MATERIALIZED */
   305,  /* (323) wqitem ::= withnm eidlist_opt wqas LP select RP */
   306,  /* (324) withnm ::= nm */
   238,  /* (325) wqlist ::= wqitem */
   238,  /* (326) wqlist ::= wqlist COMMA wqitem */
   307,  /* (327) windowdefn_list ::= windowdefn_list COMMA windowdefn */
   308,  /* (328) windowdefn ::= nm AS LP window RP */
   309,  /* (329) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
   309,  /* (330) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
   309,  /* (331) window ::= ORDER BY sortlist frame_opt */
   309,  /* (332) window ::= nm ORDER BY sortlist frame_opt */
   309,  /* (333) window ::= nm frame_opt */
   310,  /* (334) frame_opt ::= */
   310,  /* (335) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
   310,  /* (336) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
   311,  /* (337) range_or_rows ::= RANGE|ROWS|GROUPS */
   312,  /* (338) frame_bound_s ::= frame_bound */
   312,  /* (339) frame_bound_s ::= UNBOUNDED PRECEDING */
   314,  /* (340) frame_bound_e ::= frame_bound */
   314,  /* (341) frame_bound_e ::= UNBOUNDED FOLLOWING */
   315,  /* (342) frame_bound ::= expr PRECEDING|FOLLOWING */
   315,  /* (343) frame_bound ::= CURRENT ROW */
   313,  /* (344) frame_exclude_opt ::= */
   313,  /* (345) frame_exclude_opt ::= EXCLUDE frame_exclude */
   316,  /* (346) frame_exclude ::= NO OTHERS */
   316,  /* (347) frame_exclude ::= CURRENT ROW */
   316,  /* (348) frame_exclude ::= GROUP|TIES */
   249,  /* (349) window_clause ::= WINDOW windowdefn_list */
   271,  /* (350) filter_over ::= filter_clause over_clause */
   271,  /* (351) filter_over ::= over_clause */
   271,  /* (352) filter_over ::= filter_clause */
   318,  /* (353) over_clause ::= OVER LP window RP */
   318,  /* (354) over_clause ::= OVER nm */
   317,  /* (355) filter_clause ::= FILTER LP WHERE expr RP */
   217,  /* (356) term ::= QNUMBER */
   321,  /* (357) ecmd ::= explain cmdx SEMI */
   199,  /* (358) trans_opt ::= */
   199,  /* (359) trans_opt ::= TRANSACTION */
   199,  /* (360) trans_opt ::= TRANSACTION nm */
   200,  /* (361) savepoint_opt ::= SAVEPOINT */
   200,  /* (362) savepoint_opt ::= */
   197,  /* (363) cmd ::= create_table create_table_args */
   208,  /* (364) table_option_set ::= table_option */
   206,  /* (365) columnlist ::= columnlist COMMA columnname carglist */
   206,  /* (366) columnlist ::= columnname carglist */
   212,  /* (367) typetoken ::= typename */
   213,  /* (368) typename ::= ID|STRING */
   214,  /* (369) signed ::= plus_num */
   214,  /* (370) signed ::= minus_num */
   297,  /* (371) carglist ::= carglist ccons */
   297,  /* (372) carglist ::= */
   216,  /* (373) ccons ::= NULL onconf */
   216,  /* (374) ccons ::= GENERATED ALWAYS AS generated */
   216,  /* (375) ccons ::= AS generated */
   207,  /* (376) conslist_opt ::= COMMA conslist */
   322,  /* (377) conslist ::= conslist tconscomma tcons */
   322,  /* (378) conslist ::= tcons */
   229,  /* (379) tconscomma ::= */
   233,  /* (380) defer_subclause_opt ::= defer_subclause */
   234,  /* (381) resolvetype ::= raisetype */
   240,  /* (382) oneselect ::= values */
   189,  /* (383) as ::= ID|STRING */
   262,  /* (384) indexed_opt ::= indexed_by */
   270,  /* (385) returning ::= */
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
   264,  /* (406) with ::= */
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
   -3,  /* (4) selectnowith ::= selectnowith multiselect_op oneselect */
   -1,  /* (5) multiselect_op ::= UNION */
   -2,  /* (6) multiselect_op ::= UNION ALL */
   -1,  /* (7) multiselect_op ::= EXCEPT|INTERSECT */
   -9,  /* (8) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
   -1,  /* (9) distinct ::= DISTINCT */
   -1,  /* (10) distinct ::= ALL */
    0,  /* (11) distinct ::= */
    0,  /* (12) sclp ::= */
   -5,  /* (13) selcollist ::= sclp scanpt expr scanpt as */
   -3,  /* (14) selcollist ::= sclp scanpt STAR */
   -5,  /* (15) selcollist ::= sclp scanpt nm DOT STAR */
   -2,  /* (16) as ::= AS nm */
    0,  /* (17) as ::= */
    0,  /* (18) from ::= */
   -2,  /* (19) from ::= FROM seltablist */
    0,  /* (20) dbnm ::= */
   -2,  /* (21) dbnm ::= DOT nm */
    0,  /* (22) orderby_opt ::= */
   -3,  /* (23) orderby_opt ::= ORDER BY sortlist */
   -5,  /* (24) sortlist ::= sortlist COMMA expr sortorder nulls */
   -3,  /* (25) sortlist ::= expr sortorder nulls */
   -1,  /* (26) sortorder ::= ASC */
   -1,  /* (27) sortorder ::= DESC */
    0,  /* (28) sortorder ::= */
   -2,  /* (29) nulls ::= NULLS FIRST */
   -2,  /* (30) nulls ::= NULLS LAST */
    0,  /* (31) nulls ::= */
    0,  /* (32) groupby_opt ::= */
   -3,  /* (33) groupby_opt ::= GROUP BY nexprlist */
    0,  /* (34) having_opt ::= */
   -2,  /* (35) having_opt ::= HAVING expr */
    0,  /* (36) limit_opt ::= */
   -2,  /* (37) limit_opt ::= LIMIT expr */
   -4,  /* (38) limit_opt ::= LIMIT expr OFFSET expr */
   -4,  /* (39) limit_opt ::= LIMIT expr COMMA expr */
    0,  /* (40) where_opt ::= */
   -2,  /* (41) where_opt ::= WHERE expr */
   -3,  /* (42) expr ::= LP expr RP */
   -1,  /* (43) expr ::= ID|INDEXED|JOIN_KW */
   -3,  /* (44) expr ::= nm DOT nm */
   -5,  /* (45) expr ::= nm DOT nm DOT nm */
   -1,  /* (46) term ::= NULL|FLOAT|BLOB */
   -1,  /* (47) term ::= STRING */
   -1,  /* (48) term ::= INTEGER */
   -1,  /* (49) expr ::= VARIABLE */
   -3,  /* (50) expr ::= expr COLLATE ID|STRING */
   -5,  /* (51) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   -4,  /* (52) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   -1,  /* (53) term ::= CTIME_KW */
   -3,  /* (54) expr ::= expr AND expr */
   -3,  /* (55) expr ::= expr OR expr */
   -3,  /* (56) expr ::= expr LT|GT|GE|LE expr */
   -3,  /* (57) expr ::= expr EQ|NE expr */
   -3,  /* (58) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
   -3,  /* (59) expr ::= expr PLUS|MINUS expr */
   -3,  /* (60) expr ::= expr STAR|SLASH|REM expr */
   -3,  /* (61) expr ::= expr CONCAT expr */
   -2,  /* (62) likeop ::= NOT LIKE_KW|MATCH */
   -3,  /* (63) expr ::= expr likeop expr */
   -5,  /* (64) expr ::= expr likeop expr ESCAPE expr */
   -2,  /* (65) expr ::= expr ISNULL|NOTNULL */
   -3,  /* (66) expr ::= expr NOT NULL */
   -3,  /* (67) expr ::= expr IS expr */
   -4,  /* (68) expr ::= expr IS NOT expr */
   -6,  /* (69) expr ::= expr IS NOT DISTINCT FROM expr */
   -5,  /* (70) expr ::= expr IS DISTINCT FROM expr */
   -2,  /* (71) expr ::= NOT expr */
   -2,  /* (72) expr ::= BITNOT expr */
   -2,  /* (73) expr ::= PLUS|MINUS expr */
   -1,  /* (74) between_op ::= BETWEEN */
   -2,  /* (75) between_op ::= NOT BETWEEN */
   -5,  /* (76) expr ::= expr between_op expr AND expr */
   -1,  /* (77) in_op ::= IN */
   -2,  /* (78) in_op ::= NOT IN */
   -5,  /* (79) expr ::= expr in_op LP exprlist RP */
   -3,  /* (80) expr ::= LP select RP */
   -5,  /* (81) expr ::= expr in_op LP select RP */
   -5,  /* (82) expr ::= expr in_op nm dbnm paren_exprlist */
   -4,  /* (83) expr ::= EXISTS LP select RP */
   -5,  /* (84) expr ::= CASE case_operand case_exprlist case_else END */
   -5,  /* (85) case_exprlist ::= case_exprlist WHEN expr THEN expr */
   -4,  /* (86) case_exprlist ::= WHEN expr THEN expr */
   -2,  /* (87) case_else ::= ELSE expr */
    0,  /* (88) case_else ::= */
    0,  /* (89) case_operand ::= */
    0,  /* (90) exprlist ::= */
   -3,  /* (91) nexprlist ::= nexprlist COMMA expr */
   -1,  /* (92) nexprlist ::= expr */
    0,  /* (93) paren_exprlist ::= */
   -3,  /* (94) paren_exprlist ::= LP exprlist RP */
   -1,  /* (95) input ::= cmdlist */
   -2,  /* (96) cmdlist ::= cmdlist ecmd */
   -1,  /* (97) cmdlist ::= ecmd */
   -1,  /* (98) ecmd ::= SEMI */
   -2,  /* (99) ecmd ::= cmdx SEMI */
   -1,  /* (100) nm ::= ID|INDEXED|JOIN_KW */
   -1,  /* (101) nm ::= STRING */
   -1,  /* (102) selectnowith ::= oneselect */
   -2,  /* (103) sclp ::= selcollist COMMA */
   -1,  /* (104) expr ::= term */
   -1,  /* (105) likeop ::= LIKE_KW|MATCH */
   -1,  /* (106) case_operand ::= expr */
   -1,  /* (107) exprlist ::= nexprlist */
   -1,  /* (108) explain ::= EXPLAIN */
   -3,  /* (109) explain ::= EXPLAIN QUERY PLAN */
   -3,  /* (110) cmd ::= BEGIN transtype trans_opt */
    0,  /* (111) transtype ::= */
   -1,  /* (112) transtype ::= DEFERRED */
   -1,  /* (113) transtype ::= IMMEDIATE */
   -1,  /* (114) transtype ::= EXCLUSIVE */
   -2,  /* (115) cmd ::= COMMIT|END trans_opt */
   -2,  /* (116) cmd ::= ROLLBACK trans_opt */
   -2,  /* (117) cmd ::= SAVEPOINT nm */
   -3,  /* (118) cmd ::= RELEASE savepoint_opt nm */
   -5,  /* (119) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
   -6,  /* (120) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   -1,  /* (121) createkw ::= CREATE */
    0,  /* (122) ifnotexists ::= */
   -3,  /* (123) ifnotexists ::= IF NOT EXISTS */
   -1,  /* (124) temp ::= TEMP */
    0,  /* (125) temp ::= */
   -5,  /* (126) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   -2,  /* (127) create_table_args ::= AS select */
    0,  /* (128) table_option_set ::= */
   -3,  /* (129) table_option_set ::= table_option_set COMMA table_option */
   -2,  /* (130) table_option ::= WITHOUT nm */
   -1,  /* (131) table_option ::= nm */
   -2,  /* (132) columnname ::= nm typetoken */
    0,  /* (133) typetoken ::= */
   -4,  /* (134) typetoken ::= typename LP signed RP */
   -6,  /* (135) typetoken ::= typename LP signed COMMA signed RP */
   -2,  /* (136) typename ::= typename ID|STRING */
    0,  /* (137) scantok ::= */
   -2,  /* (138) ccons ::= CONSTRAINT nm */
   -3,  /* (139) ccons ::= DEFAULT scantok term */
   -4,  /* (140) ccons ::= DEFAULT LP expr RP */
   -4,  /* (141) ccons ::= DEFAULT PLUS scantok term */
   -4,  /* (142) ccons ::= DEFAULT MINUS scantok term */
   -3,  /* (143) ccons ::= DEFAULT scantok ID|INDEXED */
   -3,  /* (144) ccons ::= NOT NULL onconf */
   -5,  /* (145) ccons ::= PRIMARY KEY sortorder onconf autoinc */
   -2,  /* (146) ccons ::= UNIQUE onconf */
   -4,  /* (147) ccons ::= CHECK LP expr RP */
   -4,  /* (148) ccons ::= REFERENCES nm eidlist_opt refargs */
   -1,  /* (149) ccons ::= defer_subclause */
   -2,  /* (150) ccons ::= COLLATE ID|STRING */
   -3,  /* (151) generated ::= LP expr RP */
   -4,  /* (152) generated ::= LP expr RP ID */
    0,  /* (153) autoinc ::= */
   -1,  /* (154) autoinc ::= AUTOINCR */
    0,  /* (155) refargs ::= */
   -2,  /* (156) refargs ::= refargs refarg */
   -2,  /* (157) refarg ::= MATCH nm */
   -3,  /* (158) refarg ::= ON INSERT refact */
   -3,  /* (159) refarg ::= ON DELETE refact */
   -3,  /* (160) refarg ::= ON UPDATE refact */
   -2,  /* (161) refact ::= SET NULL */
   -2,  /* (162) refact ::= SET DEFAULT */
   -1,  /* (163) refact ::= CASCADE */
   -1,  /* (164) refact ::= RESTRICT */
   -2,  /* (165) refact ::= NO ACTION */
   -3,  /* (166) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
   -2,  /* (167) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
    0,  /* (168) init_deferred_pred_opt ::= */
   -2,  /* (169) init_deferred_pred_opt ::= INITIALLY DEFERRED */
   -2,  /* (170) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
    0,  /* (171) conslist_opt ::= */
   -1,  /* (172) tconscomma ::= COMMA */
   -2,  /* (173) tcons ::= CONSTRAINT nm */
   -7,  /* (174) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
   -5,  /* (175) tcons ::= UNIQUE LP sortlist RP onconf */
   -5,  /* (176) tcons ::= CHECK LP expr RP onconf */
  -10,  /* (177) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
    0,  /* (178) defer_subclause_opt ::= */
    0,  /* (179) onconf ::= */
   -3,  /* (180) onconf ::= ON CONFLICT resolvetype */
    0,  /* (181) orconf ::= */
   -2,  /* (182) orconf ::= OR resolvetype */
   -1,  /* (183) resolvetype ::= IGNORE */
   -1,  /* (184) resolvetype ::= REPLACE */
   -4,  /* (185) cmd ::= DROP TABLE ifexists fullname */
   -2,  /* (186) ifexists ::= IF EXISTS */
    0,  /* (187) ifexists ::= */
   -9,  /* (188) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   -4,  /* (189) cmd ::= DROP VIEW ifexists fullname */
   -3,  /* (190) select ::= WITH wqlist selectnowith */
   -4,  /* (191) select ::= WITH RECURSIVE wqlist selectnowith */
  -10,  /* (192) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   -4,  /* (193) values ::= VALUES LP nexprlist RP */
   -1,  /* (194) oneselect ::= mvalues */
   -5,  /* (195) mvalues ::= values COMMA LP nexprlist RP */
   -5,  /* (196) mvalues ::= mvalues COMMA LP nexprlist RP */
   -2,  /* (197) stl_prefix ::= seltablist joinop */
    0,  /* (198) stl_prefix ::= */
   -5,  /* (199) seltablist ::= stl_prefix nm dbnm as on_using */
   -6,  /* (200) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   -8,  /* (201) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   -6,  /* (202) seltablist ::= stl_prefix LP select RP as on_using */
   -6,  /* (203) seltablist ::= stl_prefix LP seltablist RP as on_using */
   -1,  /* (204) fullname ::= nm */
   -3,  /* (205) fullname ::= nm DOT nm */
   -1,  /* (206) xfullname ::= nm */
   -3,  /* (207) xfullname ::= nm DOT nm */
   -5,  /* (208) xfullname ::= nm DOT nm AS nm */
   -3,  /* (209) xfullname ::= nm AS nm */
   -1,  /* (210) joinop ::= COMMA|JOIN */
   -2,  /* (211) joinop ::= JOIN_KW JOIN */
   -3,  /* (212) joinop ::= JOIN_KW nm JOIN */
   -4,  /* (213) joinop ::= JOIN_KW nm nm JOIN */
   -2,  /* (214) on_using ::= ON expr */
   -4,  /* (215) on_using ::= USING LP idlist RP */
    0,  /* (216) on_using ::= */
    0,  /* (217) indexed_opt ::= */
   -3,  /* (218) indexed_by ::= INDEXED BY nm */
   -2,  /* (219) indexed_by ::= NOT INDEXED */
   -6,  /* (220) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
    0,  /* (221) where_opt_ret ::= */
   -2,  /* (222) where_opt_ret ::= WHERE expr */
   -2,  /* (223) where_opt_ret ::= RETURNING selcollist */
   -4,  /* (224) where_opt_ret ::= WHERE expr RETURNING selcollist */
   -9,  /* (225) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   -5,  /* (226) setlist ::= setlist COMMA nm EQ expr */
   -7,  /* (227) setlist ::= setlist COMMA LP idlist RP EQ expr */
   -3,  /* (228) setlist ::= nm EQ expr */
   -5,  /* (229) setlist ::= LP idlist RP EQ expr */
   -7,  /* (230) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   -8,  /* (231) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
    0,  /* (232) upsert ::= */
   -2,  /* (233) upsert ::= RETURNING selcollist */
  -12,  /* (234) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
   -9,  /* (235) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
   -5,  /* (236) upsert ::= ON CONFLICT DO NOTHING returning */
   -8,  /* (237) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
   -2,  /* (238) returning ::= RETURNING selcollist */
   -2,  /* (239) insert_cmd ::= INSERT orconf */
   -1,  /* (240) insert_cmd ::= REPLACE */
    0,  /* (241) idlist_opt ::= */
   -3,  /* (242) idlist_opt ::= LP idlist RP */
   -3,  /* (243) idlist ::= idlist COMMA nm */
   -1,  /* (244) idlist ::= nm */
   -6,  /* (245) expr ::= CAST LP expr AS typetoken RP */
   -8,  /* (246) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   -6,  /* (247) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   -9,  /* (248) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   -5,  /* (249) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   -5,  /* (250) expr ::= LP nexprlist COMMA expr RP */
   -3,  /* (251) expr ::= expr PTR expr */
  -12,  /* (252) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
   -1,  /* (253) uniqueflag ::= UNIQUE */
    0,  /* (254) uniqueflag ::= */
    0,  /* (255) eidlist_opt ::= */
   -3,  /* (256) eidlist_opt ::= LP eidlist RP */
   -5,  /* (257) eidlist ::= eidlist COMMA nm collate sortorder */
   -3,  /* (258) eidlist ::= nm collate sortorder */
    0,  /* (259) collate ::= */
   -2,  /* (260) collate ::= COLLATE ID|STRING */
   -4,  /* (261) cmd ::= DROP INDEX ifexists fullname */
   -2,  /* (262) cmd ::= VACUUM vinto */
   -3,  /* (263) cmd ::= VACUUM nm vinto */
   -2,  /* (264) vinto ::= INTO expr */
    0,  /* (265) vinto ::= */
   -3,  /* (266) cmd ::= PRAGMA nm dbnm */
   -5,  /* (267) cmd ::= PRAGMA nm dbnm EQ nmnum */
   -6,  /* (268) cmd ::= PRAGMA nm dbnm LP nmnum RP */
   -5,  /* (269) cmd ::= PRAGMA nm dbnm EQ minus_num */
   -6,  /* (270) cmd ::= PRAGMA nm dbnm LP minus_num RP */
   -2,  /* (271) plus_num ::= PLUS INTEGER|FLOAT */
   -2,  /* (272) minus_num ::= MINUS INTEGER|FLOAT */
   -5,  /* (273) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
  -11,  /* (274) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
   -1,  /* (275) trigger_time ::= BEFORE|AFTER */
   -2,  /* (276) trigger_time ::= INSTEAD OF */
    0,  /* (277) trigger_time ::= */
   -1,  /* (278) trigger_event ::= DELETE|INSERT */
   -1,  /* (279) trigger_event ::= UPDATE */
   -3,  /* (280) trigger_event ::= UPDATE OF idlist */
    0,  /* (281) when_clause ::= */
   -2,  /* (282) when_clause ::= WHEN expr */
   -3,  /* (283) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
   -2,  /* (284) trigger_cmd_list ::= trigger_cmd SEMI */
   -3,  /* (285) trnm ::= nm DOT nm */
   -3,  /* (286) tridxby ::= INDEXED BY nm */
   -2,  /* (287) tridxby ::= NOT INDEXED */
   -9,  /* (288) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
   -8,  /* (289) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
   -6,  /* (290) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
   -3,  /* (291) trigger_cmd ::= scanpt select scanpt */
   -4,  /* (292) expr ::= RAISE LP IGNORE RP */
   -6,  /* (293) expr ::= RAISE LP raisetype COMMA expr RP */
   -1,  /* (294) raisetype ::= ROLLBACK */
   -1,  /* (295) raisetype ::= ABORT */
   -1,  /* (296) raisetype ::= FAIL */
   -4,  /* (297) cmd ::= DROP TRIGGER ifexists fullname */
   -6,  /* (298) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
   -3,  /* (299) cmd ::= DETACH database_kw_opt expr */
    0,  /* (300) key_opt ::= */
   -2,  /* (301) key_opt ::= KEY expr */
   -1,  /* (302) cmd ::= REINDEX */
   -3,  /* (303) cmd ::= REINDEX nm dbnm */
   -1,  /* (304) cmd ::= ANALYZE */
   -3,  /* (305) cmd ::= ANALYZE nm dbnm */
   -6,  /* (306) cmd ::= ALTER TABLE fullname RENAME TO nm */
   -7,  /* (307) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
   -6,  /* (308) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
   -1,  /* (309) add_column_fullname ::= fullname */
   -8,  /* (310) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
   -1,  /* (311) cmd ::= create_vtab */
   -4,  /* (312) cmd ::= create_vtab LP vtabarglist RP */
   -8,  /* (313) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
    0,  /* (314) vtabarg ::= */
   -1,  /* (315) vtabargtoken ::= ANY */
   -3,  /* (316) vtabargtoken ::= lp anylist RP */
   -1,  /* (317) lp ::= LP */
   -2,  /* (318) with ::= WITH wqlist */
   -3,  /* (319) with ::= WITH RECURSIVE wqlist */
   -1,  /* (320) wqas ::= AS */
   -2,  /* (321) wqas ::= AS MATERIALIZED */
   -3,  /* (322) wqas ::= AS NOT MATERIALIZED */
   -6,  /* (323) wqitem ::= withnm eidlist_opt wqas LP select RP */
   -1,  /* (324) withnm ::= nm */
   -1,  /* (325) wqlist ::= wqitem */
   -3,  /* (326) wqlist ::= wqlist COMMA wqitem */
   -3,  /* (327) windowdefn_list ::= windowdefn_list COMMA windowdefn */
   -5,  /* (328) windowdefn ::= nm AS LP window RP */
   -5,  /* (329) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
   -6,  /* (330) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
   -4,  /* (331) window ::= ORDER BY sortlist frame_opt */
   -5,  /* (332) window ::= nm ORDER BY sortlist frame_opt */
   -2,  /* (333) window ::= nm frame_opt */
    0,  /* (334) frame_opt ::= */
   -3,  /* (335) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
   -6,  /* (336) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
   -1,  /* (337) range_or_rows ::= RANGE|ROWS|GROUPS */
   -1,  /* (338) frame_bound_s ::= frame_bound */
   -2,  /* (339) frame_bound_s ::= UNBOUNDED PRECEDING */
   -1,  /* (340) frame_bound_e ::= frame_bound */
   -2,  /* (341) frame_bound_e ::= UNBOUNDED FOLLOWING */
   -2,  /* (342) frame_bound ::= expr PRECEDING|FOLLOWING */
   -2,  /* (343) frame_bound ::= CURRENT ROW */
    0,  /* (344) frame_exclude_opt ::= */
   -2,  /* (345) frame_exclude_opt ::= EXCLUDE frame_exclude */
   -2,  /* (346) frame_exclude ::= NO OTHERS */
   -2,  /* (347) frame_exclude ::= CURRENT ROW */
   -1,  /* (348) frame_exclude ::= GROUP|TIES */
   -2,  /* (349) window_clause ::= WINDOW windowdefn_list */
   -2,  /* (350) filter_over ::= filter_clause over_clause */
   -1,  /* (351) filter_over ::= over_clause */
   -1,  /* (352) filter_over ::= filter_clause */
   -4,  /* (353) over_clause ::= OVER LP window RP */
   -2,  /* (354) over_clause ::= OVER nm */
   -5,  /* (355) filter_clause ::= FILTER LP WHERE expr RP */
   -1,  /* (356) term ::= QNUMBER */
   -3,  /* (357) ecmd ::= explain cmdx SEMI */
    0,  /* (358) trans_opt ::= */
   -1,  /* (359) trans_opt ::= TRANSACTION */
   -2,  /* (360) trans_opt ::= TRANSACTION nm */
   -1,  /* (361) savepoint_opt ::= SAVEPOINT */
    0,  /* (362) savepoint_opt ::= */
   -2,  /* (363) cmd ::= create_table create_table_args */
   -1,  /* (364) table_option_set ::= table_option */
   -4,  /* (365) columnlist ::= columnlist COMMA columnname carglist */
   -2,  /* (366) columnlist ::= columnname carglist */
   -1,  /* (367) typetoken ::= typename */
   -1,  /* (368) typename ::= ID|STRING */
   -1,  /* (369) signed ::= plus_num */
   -1,  /* (370) signed ::= minus_num */
   -2,  /* (371) carglist ::= carglist ccons */
    0,  /* (372) carglist ::= */
   -2,  /* (373) ccons ::= NULL onconf */
   -4,  /* (374) ccons ::= GENERATED ALWAYS AS generated */
   -2,  /* (375) ccons ::= AS generated */
   -2,  /* (376) conslist_opt ::= COMMA conslist */
   -3,  /* (377) conslist ::= conslist tconscomma tcons */
   -1,  /* (378) conslist ::= tcons */
    0,  /* (379) tconscomma ::= */
   -1,  /* (380) defer_subclause_opt ::= defer_subclause */
   -1,  /* (381) resolvetype ::= raisetype */
   -1,  /* (382) oneselect ::= values */
   -1,  /* (383) as ::= ID|STRING */
   -1,  /* (384) indexed_opt ::= indexed_by */
    0,  /* (385) returning ::= */
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
      case 97: /* cmdlist ::= ecmd */ yytestcase(yyruleno==97);
      case 102: /* selectnowith ::= oneselect */ yytestcase(yyruleno==102);
      case 104: /* expr ::= term */ yytestcase(yyruleno==104);
      case 106: /* case_operand ::= expr */ yytestcase(yyruleno==106);
      case 107: /* exprlist ::= nexprlist */ yytestcase(yyruleno==107);
{
    yylhsminor.yy9 = yymsp[0].minor.yy9;
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 1: /* scanpt ::= */
      case 17: /* as ::= */ yytestcase(yyruleno==17);
{
    yymsp[1].minor.yy0.z = NULL; yymsp[1].minor.yy0.n = 0;
}
        break;
      case 4: /* selectnowith ::= selectnowith multiselect_op oneselect */
{
    yymsp[-2].minor.yy9 = ast_compound_select(pCtx->astCtx, (uint8_t)yymsp[-1].minor.yy502, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
        break;
      case 5: /* multiselect_op ::= UNION */
{ yylhsminor.yy502 = 0; UNUSED_PARAMETER(yymsp[0].minor.yy0); }
  yymsp[0].minor.yy502 = yylhsminor.yy502;
        break;
      case 6: /* multiselect_op ::= UNION ALL */
      case 78: /* in_op ::= NOT IN */ yytestcase(yyruleno==78);
{ yymsp[-1].minor.yy502 = 1; }
        break;
      case 7: /* multiselect_op ::= EXCEPT|INTERSECT */
{
    yylhsminor.yy502 = (yymsp[0].minor.yy0.type == TK_INTERSECT) ? 2 : 3;
}
  yymsp[0].minor.yy502 = yylhsminor.yy502;
        break;
      case 8: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
{
    yymsp[-8].minor.yy9 = ast_select_stmt(pCtx->astCtx, (uint8_t)yymsp[-7].minor.yy9, yymsp[-6].minor.yy9, yymsp[-4].minor.yy9, yymsp[-3].minor.yy9, yymsp[-2].minor.yy9, yymsp[-1].minor.yy9, yymsp[0].minor.yy9);
}
        break;
      case 9: /* distinct ::= DISTINCT */
      case 27: /* sortorder ::= DESC */ yytestcase(yyruleno==27);
{
    yymsp[0].minor.yy9 = 1;
}
        break;
      case 10: /* distinct ::= ALL */
      case 26: /* sortorder ::= ASC */ yytestcase(yyruleno==26);
      case 74: /* between_op ::= BETWEEN */ yytestcase(yyruleno==74);
{
    yymsp[0].minor.yy9 = 0;
}
        break;
      case 11: /* distinct ::= */
      case 28: /* sortorder ::= */ yytestcase(yyruleno==28);
      case 31: /* nulls ::= */ yytestcase(yyruleno==31);
{
    yymsp[1].minor.yy9 = 0;
}
        break;
      case 12: /* sclp ::= */
      case 18: /* from ::= */ yytestcase(yyruleno==18);
      case 22: /* orderby_opt ::= */ yytestcase(yyruleno==22);
      case 32: /* groupby_opt ::= */ yytestcase(yyruleno==32);
      case 34: /* having_opt ::= */ yytestcase(yyruleno==34);
      case 36: /* limit_opt ::= */ yytestcase(yyruleno==36);
      case 40: /* where_opt ::= */ yytestcase(yyruleno==40);
      case 88: /* case_else ::= */ yytestcase(yyruleno==88);
      case 89: /* case_operand ::= */ yytestcase(yyruleno==89);
      case 90: /* exprlist ::= */ yytestcase(yyruleno==90);
{
    yymsp[1].minor.yy9 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 13: /* selcollist ::= sclp scanpt expr scanpt as */
{
    uint32_t col = ast_result_column(pCtx->astCtx, 0, SYNTAQLITE_NO_SPAN, yymsp[-2].minor.yy9);
    yylhsminor.yy9 = ast_result_column_list_append(pCtx->astCtx, yymsp[-4].minor.yy9, col);
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 14: /* selcollist ::= sclp scanpt STAR */
{
    uint32_t col = ast_result_column(pCtx->astCtx, 1, SYNTAQLITE_NO_SPAN, SYNTAQLITE_NULL_NODE);
    yylhsminor.yy9 = ast_result_column_list_append(pCtx->astCtx, yymsp[-2].minor.yy9, col);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 15: /* selcollist ::= sclp scanpt nm DOT STAR */
{
    uint32_t col = ast_result_column(pCtx->astCtx, 1, syntaqlite_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NULL_NODE);
    yylhsminor.yy9 = ast_result_column_list_append(pCtx->astCtx, yymsp[-4].minor.yy9, col);
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 16: /* as ::= AS nm */
{
    yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;
}
        break;
      case 19: /* from ::= FROM seltablist */
      case 35: /* having_opt ::= HAVING expr */ yytestcase(yyruleno==35);
      case 41: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==41);
      case 87: /* case_else ::= ELSE expr */ yytestcase(yyruleno==87);
{
    yymsp[-1].minor.yy9 = yymsp[0].minor.yy9;
}
        break;
      case 20: /* dbnm ::= */
{ yymsp[1].minor.yy0.z = NULL; yymsp[1].minor.yy0.n = 0; }
        break;
      case 21: /* dbnm ::= DOT nm */
{ yymsp[-1].minor.yy0 = yymsp[0].minor.yy0; }
        break;
      case 23: /* orderby_opt ::= ORDER BY sortlist */
      case 33: /* groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==33);
{
    yymsp[-2].minor.yy9 = yymsp[0].minor.yy9;
}
        break;
      case 24: /* sortlist ::= sortlist COMMA expr sortorder nulls */
{
    uint32_t term = ast_ordering_term(pCtx->astCtx, yymsp[-2].minor.yy9, (uint8_t)yymsp[-1].minor.yy9, (uint8_t)yymsp[0].minor.yy9);
    yylhsminor.yy9 = ast_order_by_list_append(pCtx->astCtx, yymsp[-4].minor.yy9, term);
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 25: /* sortlist ::= expr sortorder nulls */
{
    uint32_t term = ast_ordering_term(pCtx->astCtx, yymsp[-2].minor.yy9, (uint8_t)yymsp[-1].minor.yy9, (uint8_t)yymsp[0].minor.yy9);
    yylhsminor.yy9 = ast_order_by_list(pCtx->astCtx, term);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 29: /* nulls ::= NULLS FIRST */
      case 75: /* between_op ::= NOT BETWEEN */ yytestcase(yyruleno==75);
{
    yymsp[-1].minor.yy9 = 1;
}
        break;
      case 30: /* nulls ::= NULLS LAST */
{
    yymsp[-1].minor.yy9 = 2;
}
        break;
      case 37: /* limit_opt ::= LIMIT expr */
{
    yymsp[-1].minor.yy9 = ast_limit_clause(pCtx->astCtx, yymsp[0].minor.yy9, SYNTAQLITE_NULL_NODE);
}
        break;
      case 38: /* limit_opt ::= LIMIT expr OFFSET expr */
{
    yymsp[-3].minor.yy9 = ast_limit_clause(pCtx->astCtx, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
        break;
      case 39: /* limit_opt ::= LIMIT expr COMMA expr */
{
    yymsp[-3].minor.yy9 = ast_limit_clause(pCtx->astCtx, yymsp[0].minor.yy9, yymsp[-2].minor.yy9);
}
        break;
      case 42: /* expr ::= LP expr RP */
{
    yymsp[-2].minor.yy9 = yymsp[-1].minor.yy9;
}
        break;
      case 43: /* expr ::= ID|INDEXED|JOIN_KW */
{
    yylhsminor.yy9 = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NO_SPAN);
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 44: /* expr ::= nm DOT nm */
{
    yylhsminor.yy9 = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0),
        SYNTAQLITE_NO_SPAN);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 45: /* expr ::= nm DOT nm DOT nm */
{
    yylhsminor.yy9 = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-4].minor.yy0));
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 46: /* term ::= NULL|FLOAT|BLOB */
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
      case 47: /* term ::= STRING */
{
    yylhsminor.yy9 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_STRING, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 48: /* term ::= INTEGER */
{
    yylhsminor.yy9 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_INTEGER, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 49: /* expr ::= VARIABLE */
{
    yylhsminor.yy9 = ast_variable(pCtx->astCtx, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 50: /* expr ::= expr COLLATE ID|STRING */
{
    yylhsminor.yy9 = ast_collate_expr(pCtx->astCtx, yymsp[-2].minor.yy9, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 51: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
{
    yylhsminor.yy9 = ast_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-4].minor.yy0),
        (uint8_t)yymsp[-2].minor.yy9,
        yymsp[-1].minor.yy9);
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 52: /* expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
{
    yylhsminor.yy9 = ast_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-3].minor.yy0),
        2,
        SYNTAQLITE_NULL_NODE);
}
  yymsp[-3].minor.yy9 = yylhsminor.yy9;
        break;
      case 53: /* term ::= CTIME_KW */
{
    yylhsminor.yy9 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_CURRENT, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 54: /* expr ::= expr AND expr */
{
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_AND, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 55: /* expr ::= expr OR expr */
{
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_OR, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 56: /* expr ::= expr LT|GT|GE|LE expr */
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
      case 57: /* expr ::= expr EQ|NE expr */
{
    SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == TK_EQ) ? SYNTAQLITE_BINARY_OP_EQ : SYNTAQLITE_BINARY_OP_NE;
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 58: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
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
      case 59: /* expr ::= expr PLUS|MINUS expr */
{
    SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == TK_PLUS) ? SYNTAQLITE_BINARY_OP_PLUS : SYNTAQLITE_BINARY_OP_MINUS;
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 60: /* expr ::= expr STAR|SLASH|REM expr */
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
      case 61: /* expr ::= expr CONCAT expr */
{
    yylhsminor.yy9 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_CONCAT, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 62: /* likeop ::= NOT LIKE_KW|MATCH */
{
    yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;
    yymsp[-1].minor.yy0.n |= 0x80000000;
}
        break;
      case 63: /* expr ::= expr likeop expr */
{
    uint8_t negated = (yymsp[-1].minor.yy0.n & 0x80000000) ? 1 : 0;
    yylhsminor.yy9 = ast_like_expr(pCtx->astCtx, negated, yymsp[-2].minor.yy9, yymsp[0].minor.yy9, SYNTAQLITE_NULL_NODE);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 64: /* expr ::= expr likeop expr ESCAPE expr */
{
    uint8_t negated = (yymsp[-3].minor.yy0.n & 0x80000000) ? 1 : 0;
    yylhsminor.yy9 = ast_like_expr(pCtx->astCtx, negated, yymsp[-4].minor.yy9, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 65: /* expr ::= expr ISNULL|NOTNULL */
{
    SyntaqliteIsOp op = (yymsp[0].minor.yy0.type == TK_ISNULL) ? SYNTAQLITE_IS_OP_ISNULL : SYNTAQLITE_IS_OP_NOTNULL;
    yylhsminor.yy9 = ast_is_expr(pCtx->astCtx, op, yymsp[-1].minor.yy9, SYNTAQLITE_NULL_NODE);
}
  yymsp[-1].minor.yy9 = yylhsminor.yy9;
        break;
      case 66: /* expr ::= expr NOT NULL */
{
    yylhsminor.yy9 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_NOTNULL, yymsp[-2].minor.yy9, SYNTAQLITE_NULL_NODE);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 67: /* expr ::= expr IS expr */
{
    yylhsminor.yy9 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 68: /* expr ::= expr IS NOT expr */
{
    yylhsminor.yy9 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_NOT, yymsp[-3].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-3].minor.yy9 = yylhsminor.yy9;
        break;
      case 69: /* expr ::= expr IS NOT DISTINCT FROM expr */
{
    yylhsminor.yy9 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_NOT_DISTINCT, yymsp[-5].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-5].minor.yy9 = yylhsminor.yy9;
        break;
      case 70: /* expr ::= expr IS DISTINCT FROM expr */
{
    yylhsminor.yy9 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_DISTINCT, yymsp[-4].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 71: /* expr ::= NOT expr */
{
    yymsp[-1].minor.yy9 = ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_NOT, yymsp[0].minor.yy9);
}
        break;
      case 72: /* expr ::= BITNOT expr */
{
    yymsp[-1].minor.yy9 = ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_BITNOT, yymsp[0].minor.yy9);
}
        break;
      case 73: /* expr ::= PLUS|MINUS expr */
{
    SyntaqliteUnaryOp op = (yymsp[-1].minor.yy0.type == TK_MINUS) ? SYNTAQLITE_UNARY_OP_MINUS : SYNTAQLITE_UNARY_OP_PLUS;
    yylhsminor.yy9 = ast_unary_expr(pCtx->astCtx, op, yymsp[0].minor.yy9);
}
  yymsp[-1].minor.yy9 = yylhsminor.yy9;
        break;
      case 76: /* expr ::= expr between_op expr AND expr */
{
    yylhsminor.yy9 = ast_between_expr(pCtx->astCtx, (uint8_t)yymsp[-3].minor.yy9, yymsp[-4].minor.yy9, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 77: /* in_op ::= IN */
{ yymsp[0].minor.yy502 = 0; }
        break;
      case 79: /* expr ::= expr in_op LP exprlist RP */
{
    yymsp[-4].minor.yy9 = ast_in_expr(pCtx->astCtx, (uint8_t)yymsp[-3].minor.yy502, yymsp[-4].minor.yy9, yymsp[-1].minor.yy9);
}
        break;
      case 80: /* expr ::= LP select RP */
{
    yymsp[-2].minor.yy9 = ast_subquery_expr(pCtx->astCtx, yymsp[-1].minor.yy9);
}
        break;
      case 81: /* expr ::= expr in_op LP select RP */
{
    uint32_t sub = ast_subquery_expr(pCtx->astCtx, yymsp[-1].minor.yy9);
    yymsp[-4].minor.yy9 = ast_in_expr(pCtx->astCtx, (uint8_t)yymsp[-3].minor.yy502, yymsp[-4].minor.yy9, sub);
}
        break;
      case 82: /* expr ::= expr in_op nm dbnm paren_exprlist */
{
    // Table-valued function IN expression - stub for now
    (void)yymsp[-2].minor.yy0; (void)yymsp[-1].minor.yy0; (void)yymsp[0].minor.yy9;
    yymsp[-4].minor.yy9 = ast_in_expr(pCtx->astCtx, (uint8_t)yymsp[-3].minor.yy502, yymsp[-4].minor.yy9, SYNTAQLITE_NULL_NODE);
}
        break;
      case 83: /* expr ::= EXISTS LP select RP */
{
    yymsp[-3].minor.yy9 = ast_exists_expr(pCtx->astCtx, yymsp[-1].minor.yy9);
}
        break;
      case 84: /* expr ::= CASE case_operand case_exprlist case_else END */
{
    yymsp[-4].minor.yy9 = ast_case_expr(pCtx->astCtx, yymsp[-3].minor.yy9, yymsp[-1].minor.yy9, yymsp[-2].minor.yy9);
}
        break;
      case 85: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
{
    uint32_t w = ast_case_when(pCtx->astCtx, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
    yylhsminor.yy9 = ast_case_when_list_append(pCtx->astCtx, yymsp[-4].minor.yy9, w);
}
  yymsp[-4].minor.yy9 = yylhsminor.yy9;
        break;
      case 86: /* case_exprlist ::= WHEN expr THEN expr */
{
    uint32_t w = ast_case_when(pCtx->astCtx, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
    yymsp[-3].minor.yy9 = ast_case_when_list(pCtx->astCtx, w);
}
        break;
      case 91: /* nexprlist ::= nexprlist COMMA expr */
{
    yylhsminor.yy9 = ast_expr_list_append(pCtx->astCtx, yymsp[-2].minor.yy9, yymsp[0].minor.yy9);
}
  yymsp[-2].minor.yy9 = yylhsminor.yy9;
        break;
      case 92: /* nexprlist ::= expr */
{
    yylhsminor.yy9 = ast_expr_list(pCtx->astCtx, yymsp[0].minor.yy9);
}
  yymsp[0].minor.yy9 = yylhsminor.yy9;
        break;
      case 93: /* paren_exprlist ::= */
{ yymsp[1].minor.yy9 = SYNTAQLITE_NULL_NODE; }
        break;
      case 94: /* paren_exprlist ::= LP exprlist RP */
{ yymsp[-2].minor.yy9 = yymsp[-1].minor.yy9; }
        break;
      case 95: /* input ::= cmdlist */
{
    pCtx->root = yymsp[0].minor.yy9;
}
        break;
      case 96: /* cmdlist ::= cmdlist ecmd */
{
    yymsp[-1].minor.yy9 = yymsp[0].minor.yy9;  // Just use the last command for now
}
        break;
      case 98: /* ecmd ::= SEMI */
{
    yymsp[0].minor.yy9 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 99: /* ecmd ::= cmdx SEMI */
      case 103: /* sclp ::= selcollist COMMA */ yytestcase(yyruleno==103);
{
    yylhsminor.yy9 = yymsp[-1].minor.yy9;
}
  yymsp[-1].minor.yy9 = yylhsminor.yy9;
        break;
      case 100: /* nm ::= ID|INDEXED|JOIN_KW */
      case 101: /* nm ::= STRING */ yytestcase(yyruleno==101);
      case 105: /* likeop ::= LIKE_KW|MATCH */ yytestcase(yyruleno==105);
{
    yylhsminor.yy0 = yymsp[0].minor.yy0;
}
  yymsp[0].minor.yy0 = yylhsminor.yy0;
        break;
      default:
      /* (108) explain ::= EXPLAIN */ yytestcase(yyruleno==108);
      /* (109) explain ::= EXPLAIN QUERY PLAN */ yytestcase(yyruleno==109);
      /* (110) cmd ::= BEGIN transtype trans_opt */ yytestcase(yyruleno==110);
      /* (111) transtype ::= */ yytestcase(yyruleno==111);
      /* (112) transtype ::= DEFERRED */ yytestcase(yyruleno==112);
      /* (113) transtype ::= IMMEDIATE */ yytestcase(yyruleno==113);
      /* (114) transtype ::= EXCLUSIVE */ yytestcase(yyruleno==114);
      /* (115) cmd ::= COMMIT|END trans_opt */ yytestcase(yyruleno==115);
      /* (116) cmd ::= ROLLBACK trans_opt */ yytestcase(yyruleno==116);
      /* (117) cmd ::= SAVEPOINT nm */ yytestcase(yyruleno==117);
      /* (118) cmd ::= RELEASE savepoint_opt nm */ yytestcase(yyruleno==118);
      /* (119) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */ yytestcase(yyruleno==119);
      /* (120) create_table ::= createkw temp TABLE ifnotexists nm dbnm */ yytestcase(yyruleno==120);
      /* (121) createkw ::= CREATE */ yytestcase(yyruleno==121);
      /* (122) ifnotexists ::= */ yytestcase(yyruleno==122);
      /* (123) ifnotexists ::= IF NOT EXISTS */ yytestcase(yyruleno==123);
      /* (124) temp ::= TEMP */ yytestcase(yyruleno==124);
      /* (125) temp ::= */ yytestcase(yyruleno==125);
      /* (126) create_table_args ::= LP columnlist conslist_opt RP table_option_set */ yytestcase(yyruleno==126);
      /* (127) create_table_args ::= AS select */ yytestcase(yyruleno==127);
      /* (128) table_option_set ::= */ yytestcase(yyruleno==128);
      /* (129) table_option_set ::= table_option_set COMMA table_option */ yytestcase(yyruleno==129);
      /* (130) table_option ::= WITHOUT nm */ yytestcase(yyruleno==130);
      /* (131) table_option ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=131);
      /* (132) columnname ::= nm typetoken */ yytestcase(yyruleno==132);
      /* (133) typetoken ::= */ yytestcase(yyruleno==133);
      /* (134) typetoken ::= typename LP signed RP */ yytestcase(yyruleno==134);
      /* (135) typetoken ::= typename LP signed COMMA signed RP */ yytestcase(yyruleno==135);
      /* (136) typename ::= typename ID|STRING */ yytestcase(yyruleno==136);
      /* (137) scantok ::= */ yytestcase(yyruleno==137);
      /* (138) ccons ::= CONSTRAINT nm */ yytestcase(yyruleno==138);
      /* (139) ccons ::= DEFAULT scantok term */ yytestcase(yyruleno==139);
      /* (140) ccons ::= DEFAULT LP expr RP */ yytestcase(yyruleno==140);
      /* (141) ccons ::= DEFAULT PLUS scantok term */ yytestcase(yyruleno==141);
      /* (142) ccons ::= DEFAULT MINUS scantok term */ yytestcase(yyruleno==142);
      /* (143) ccons ::= DEFAULT scantok ID|INDEXED */ yytestcase(yyruleno==143);
      /* (144) ccons ::= NOT NULL onconf */ yytestcase(yyruleno==144);
      /* (145) ccons ::= PRIMARY KEY sortorder onconf autoinc */ yytestcase(yyruleno==145);
      /* (146) ccons ::= UNIQUE onconf */ yytestcase(yyruleno==146);
      /* (147) ccons ::= CHECK LP expr RP */ yytestcase(yyruleno==147);
      /* (148) ccons ::= REFERENCES nm eidlist_opt refargs */ yytestcase(yyruleno==148);
      /* (149) ccons ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=149);
      /* (150) ccons ::= COLLATE ID|STRING */ yytestcase(yyruleno==150);
      /* (151) generated ::= LP expr RP */ yytestcase(yyruleno==151);
      /* (152) generated ::= LP expr RP ID */ yytestcase(yyruleno==152);
      /* (153) autoinc ::= */ yytestcase(yyruleno==153);
      /* (154) autoinc ::= AUTOINCR */ yytestcase(yyruleno==154);
      /* (155) refargs ::= */ yytestcase(yyruleno==155);
      /* (156) refargs ::= refargs refarg */ yytestcase(yyruleno==156);
      /* (157) refarg ::= MATCH nm */ yytestcase(yyruleno==157);
      /* (158) refarg ::= ON INSERT refact */ yytestcase(yyruleno==158);
      /* (159) refarg ::= ON DELETE refact */ yytestcase(yyruleno==159);
      /* (160) refarg ::= ON UPDATE refact */ yytestcase(yyruleno==160);
      /* (161) refact ::= SET NULL */ yytestcase(yyruleno==161);
      /* (162) refact ::= SET DEFAULT */ yytestcase(yyruleno==162);
      /* (163) refact ::= CASCADE */ yytestcase(yyruleno==163);
      /* (164) refact ::= RESTRICT */ yytestcase(yyruleno==164);
      /* (165) refact ::= NO ACTION */ yytestcase(yyruleno==165);
      /* (166) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */ yytestcase(yyruleno==166);
      /* (167) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */ yytestcase(yyruleno==167);
      /* (168) init_deferred_pred_opt ::= */ yytestcase(yyruleno==168);
      /* (169) init_deferred_pred_opt ::= INITIALLY DEFERRED */ yytestcase(yyruleno==169);
      /* (170) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */ yytestcase(yyruleno==170);
      /* (171) conslist_opt ::= */ yytestcase(yyruleno==171);
      /* (172) tconscomma ::= COMMA */ yytestcase(yyruleno==172);
      /* (173) tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==173);
      /* (174) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */ yytestcase(yyruleno==174);
      /* (175) tcons ::= UNIQUE LP sortlist RP onconf */ yytestcase(yyruleno==175);
      /* (176) tcons ::= CHECK LP expr RP onconf */ yytestcase(yyruleno==176);
      /* (177) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */ yytestcase(yyruleno==177);
      /* (178) defer_subclause_opt ::= */ yytestcase(yyruleno==178);
      /* (179) onconf ::= */ yytestcase(yyruleno==179);
      /* (180) onconf ::= ON CONFLICT resolvetype */ yytestcase(yyruleno==180);
      /* (181) orconf ::= */ yytestcase(yyruleno==181);
      /* (182) orconf ::= OR resolvetype */ yytestcase(yyruleno==182);
      /* (183) resolvetype ::= IGNORE */ yytestcase(yyruleno==183);
      /* (184) resolvetype ::= REPLACE */ yytestcase(yyruleno==184);
      /* (185) cmd ::= DROP TABLE ifexists fullname */ yytestcase(yyruleno==185);
      /* (186) ifexists ::= IF EXISTS */ yytestcase(yyruleno==186);
      /* (187) ifexists ::= */ yytestcase(yyruleno==187);
      /* (188) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */ yytestcase(yyruleno==188);
      /* (189) cmd ::= DROP VIEW ifexists fullname */ yytestcase(yyruleno==189);
      /* (190) select ::= WITH wqlist selectnowith */ yytestcase(yyruleno==190);
      /* (191) select ::= WITH RECURSIVE wqlist selectnowith */ yytestcase(yyruleno==191);
      /* (192) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */ yytestcase(yyruleno==192);
      /* (193) values ::= VALUES LP nexprlist RP */ yytestcase(yyruleno==193);
      /* (194) oneselect ::= mvalues */ yytestcase(yyruleno==194);
      /* (195) mvalues ::= values COMMA LP nexprlist RP */ yytestcase(yyruleno==195);
      /* (196) mvalues ::= mvalues COMMA LP nexprlist RP */ yytestcase(yyruleno==196);
      /* (197) stl_prefix ::= seltablist joinop */ yytestcase(yyruleno==197);
      /* (198) stl_prefix ::= */ yytestcase(yyruleno==198);
      /* (199) seltablist ::= stl_prefix nm dbnm as on_using */ yytestcase(yyruleno==199);
      /* (200) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */ yytestcase(yyruleno==200);
      /* (201) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */ yytestcase(yyruleno==201);
      /* (202) seltablist ::= stl_prefix LP select RP as on_using */ yytestcase(yyruleno==202);
      /* (203) seltablist ::= stl_prefix LP seltablist RP as on_using */ yytestcase(yyruleno==203);
      /* (204) fullname ::= nm */ yytestcase(yyruleno==204);
      /* (205) fullname ::= nm DOT nm */ yytestcase(yyruleno==205);
      /* (206) xfullname ::= nm */ yytestcase(yyruleno==206);
      /* (207) xfullname ::= nm DOT nm */ yytestcase(yyruleno==207);
      /* (208) xfullname ::= nm DOT nm AS nm */ yytestcase(yyruleno==208);
      /* (209) xfullname ::= nm AS nm */ yytestcase(yyruleno==209);
      /* (210) joinop ::= COMMA|JOIN */ yytestcase(yyruleno==210);
      /* (211) joinop ::= JOIN_KW JOIN */ yytestcase(yyruleno==211);
      /* (212) joinop ::= JOIN_KW nm JOIN */ yytestcase(yyruleno==212);
      /* (213) joinop ::= JOIN_KW nm nm JOIN */ yytestcase(yyruleno==213);
      /* (214) on_using ::= ON expr */ yytestcase(yyruleno==214);
      /* (215) on_using ::= USING LP idlist RP */ yytestcase(yyruleno==215);
      /* (216) on_using ::= */ yytestcase(yyruleno==216);
      /* (217) indexed_opt ::= */ yytestcase(yyruleno==217);
      /* (218) indexed_by ::= INDEXED BY nm */ yytestcase(yyruleno==218);
      /* (219) indexed_by ::= NOT INDEXED */ yytestcase(yyruleno==219);
      /* (220) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */ yytestcase(yyruleno==220);
      /* (221) where_opt_ret ::= */ yytestcase(yyruleno==221);
      /* (222) where_opt_ret ::= WHERE expr */ yytestcase(yyruleno==222);
      /* (223) where_opt_ret ::= RETURNING selcollist */ yytestcase(yyruleno==223);
      /* (224) where_opt_ret ::= WHERE expr RETURNING selcollist */ yytestcase(yyruleno==224);
      /* (225) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */ yytestcase(yyruleno==225);
      /* (226) setlist ::= setlist COMMA nm EQ expr */ yytestcase(yyruleno==226);
      /* (227) setlist ::= setlist COMMA LP idlist RP EQ expr */ yytestcase(yyruleno==227);
      /* (228) setlist ::= nm EQ expr */ yytestcase(yyruleno==228);
      /* (229) setlist ::= LP idlist RP EQ expr */ yytestcase(yyruleno==229);
      /* (230) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */ yytestcase(yyruleno==230);
      /* (231) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */ yytestcase(yyruleno==231);
      /* (232) upsert ::= */ yytestcase(yyruleno==232);
      /* (233) upsert ::= RETURNING selcollist */ yytestcase(yyruleno==233);
      /* (234) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */ yytestcase(yyruleno==234);
      /* (235) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */ yytestcase(yyruleno==235);
      /* (236) upsert ::= ON CONFLICT DO NOTHING returning */ yytestcase(yyruleno==236);
      /* (237) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */ yytestcase(yyruleno==237);
      /* (238) returning ::= RETURNING selcollist */ yytestcase(yyruleno==238);
      /* (239) insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==239);
      /* (240) insert_cmd ::= REPLACE */ yytestcase(yyruleno==240);
      /* (241) idlist_opt ::= */ yytestcase(yyruleno==241);
      /* (242) idlist_opt ::= LP idlist RP */ yytestcase(yyruleno==242);
      /* (243) idlist ::= idlist COMMA nm */ yytestcase(yyruleno==243);
      /* (244) idlist ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=244);
      /* (245) expr ::= CAST LP expr AS typetoken RP */ yytestcase(yyruleno==245);
      /* (246) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */ yytestcase(yyruleno==246);
      /* (247) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */ yytestcase(yyruleno==247);
      /* (248) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */ yytestcase(yyruleno==248);
      /* (249) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */ yytestcase(yyruleno==249);
      /* (250) expr ::= LP nexprlist COMMA expr RP */ yytestcase(yyruleno==250);
      /* (251) expr ::= expr PTR expr */ yytestcase(yyruleno==251);
      /* (252) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */ yytestcase(yyruleno==252);
      /* (253) uniqueflag ::= UNIQUE */ yytestcase(yyruleno==253);
      /* (254) uniqueflag ::= */ yytestcase(yyruleno==254);
      /* (255) eidlist_opt ::= */ yytestcase(yyruleno==255);
      /* (256) eidlist_opt ::= LP eidlist RP */ yytestcase(yyruleno==256);
      /* (257) eidlist ::= eidlist COMMA nm collate sortorder */ yytestcase(yyruleno==257);
      /* (258) eidlist ::= nm collate sortorder */ yytestcase(yyruleno==258);
      /* (259) collate ::= */ yytestcase(yyruleno==259);
      /* (260) collate ::= COLLATE ID|STRING */ yytestcase(yyruleno==260);
      /* (261) cmd ::= DROP INDEX ifexists fullname */ yytestcase(yyruleno==261);
      /* (262) cmd ::= VACUUM vinto */ yytestcase(yyruleno==262);
      /* (263) cmd ::= VACUUM nm vinto */ yytestcase(yyruleno==263);
      /* (264) vinto ::= INTO expr */ yytestcase(yyruleno==264);
      /* (265) vinto ::= */ yytestcase(yyruleno==265);
      /* (266) cmd ::= PRAGMA nm dbnm */ yytestcase(yyruleno==266);
      /* (267) cmd ::= PRAGMA nm dbnm EQ nmnum */ yytestcase(yyruleno==267);
      /* (268) cmd ::= PRAGMA nm dbnm LP nmnum RP */ yytestcase(yyruleno==268);
      /* (269) cmd ::= PRAGMA nm dbnm EQ minus_num */ yytestcase(yyruleno==269);
      /* (270) cmd ::= PRAGMA nm dbnm LP minus_num RP */ yytestcase(yyruleno==270);
      /* (271) plus_num ::= PLUS INTEGER|FLOAT */ yytestcase(yyruleno==271);
      /* (272) minus_num ::= MINUS INTEGER|FLOAT */ yytestcase(yyruleno==272);
      /* (273) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */ yytestcase(yyruleno==273);
      /* (274) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */ yytestcase(yyruleno==274);
      /* (275) trigger_time ::= BEFORE|AFTER */ yytestcase(yyruleno==275);
      /* (276) trigger_time ::= INSTEAD OF */ yytestcase(yyruleno==276);
      /* (277) trigger_time ::= */ yytestcase(yyruleno==277);
      /* (278) trigger_event ::= DELETE|INSERT */ yytestcase(yyruleno==278);
      /* (279) trigger_event ::= UPDATE */ yytestcase(yyruleno==279);
      /* (280) trigger_event ::= UPDATE OF idlist */ yytestcase(yyruleno==280);
      /* (281) when_clause ::= */ yytestcase(yyruleno==281);
      /* (282) when_clause ::= WHEN expr */ yytestcase(yyruleno==282);
      /* (283) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */ yytestcase(yyruleno==283);
      /* (284) trigger_cmd_list ::= trigger_cmd SEMI */ yytestcase(yyruleno==284);
      /* (285) trnm ::= nm DOT nm */ yytestcase(yyruleno==285);
      /* (286) tridxby ::= INDEXED BY nm */ yytestcase(yyruleno==286);
      /* (287) tridxby ::= NOT INDEXED */ yytestcase(yyruleno==287);
      /* (288) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */ yytestcase(yyruleno==288);
      /* (289) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */ yytestcase(yyruleno==289);
      /* (290) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */ yytestcase(yyruleno==290);
      /* (291) trigger_cmd ::= scanpt select scanpt */ yytestcase(yyruleno==291);
      /* (292) expr ::= RAISE LP IGNORE RP */ yytestcase(yyruleno==292);
      /* (293) expr ::= RAISE LP raisetype COMMA expr RP */ yytestcase(yyruleno==293);
      /* (294) raisetype ::= ROLLBACK */ yytestcase(yyruleno==294);
      /* (295) raisetype ::= ABORT */ yytestcase(yyruleno==295);
      /* (296) raisetype ::= FAIL */ yytestcase(yyruleno==296);
      /* (297) cmd ::= DROP TRIGGER ifexists fullname */ yytestcase(yyruleno==297);
      /* (298) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */ yytestcase(yyruleno==298);
      /* (299) cmd ::= DETACH database_kw_opt expr */ yytestcase(yyruleno==299);
      /* (300) key_opt ::= */ yytestcase(yyruleno==300);
      /* (301) key_opt ::= KEY expr */ yytestcase(yyruleno==301);
      /* (302) cmd ::= REINDEX */ yytestcase(yyruleno==302);
      /* (303) cmd ::= REINDEX nm dbnm */ yytestcase(yyruleno==303);
      /* (304) cmd ::= ANALYZE */ yytestcase(yyruleno==304);
      /* (305) cmd ::= ANALYZE nm dbnm */ yytestcase(yyruleno==305);
      /* (306) cmd ::= ALTER TABLE fullname RENAME TO nm */ yytestcase(yyruleno==306);
      /* (307) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */ yytestcase(yyruleno==307);
      /* (308) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */ yytestcase(yyruleno==308);
      /* (309) add_column_fullname ::= fullname */ yytestcase(yyruleno==309);
      /* (310) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */ yytestcase(yyruleno==310);
      /* (311) cmd ::= create_vtab */ yytestcase(yyruleno==311);
      /* (312) cmd ::= create_vtab LP vtabarglist RP */ yytestcase(yyruleno==312);
      /* (313) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */ yytestcase(yyruleno==313);
      /* (314) vtabarg ::= */ yytestcase(yyruleno==314);
      /* (315) vtabargtoken ::= ANY */ yytestcase(yyruleno==315);
      /* (316) vtabargtoken ::= lp anylist RP */ yytestcase(yyruleno==316);
      /* (317) lp ::= LP */ yytestcase(yyruleno==317);
      /* (318) with ::= WITH wqlist */ yytestcase(yyruleno==318);
      /* (319) with ::= WITH RECURSIVE wqlist */ yytestcase(yyruleno==319);
      /* (320) wqas ::= AS */ yytestcase(yyruleno==320);
      /* (321) wqas ::= AS MATERIALIZED */ yytestcase(yyruleno==321);
      /* (322) wqas ::= AS NOT MATERIALIZED */ yytestcase(yyruleno==322);
      /* (323) wqitem ::= withnm eidlist_opt wqas LP select RP */ yytestcase(yyruleno==323);
      /* (324) withnm ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=324);
      /* (325) wqlist ::= wqitem (OPTIMIZED OUT) */ assert(yyruleno!=325);
      /* (326) wqlist ::= wqlist COMMA wqitem */ yytestcase(yyruleno==326);
      /* (327) windowdefn_list ::= windowdefn_list COMMA windowdefn */ yytestcase(yyruleno==327);
      /* (328) windowdefn ::= nm AS LP window RP */ yytestcase(yyruleno==328);
      /* (329) window ::= PARTITION BY nexprlist orderby_opt frame_opt */ yytestcase(yyruleno==329);
      /* (330) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */ yytestcase(yyruleno==330);
      /* (331) window ::= ORDER BY sortlist frame_opt */ yytestcase(yyruleno==331);
      /* (332) window ::= nm ORDER BY sortlist frame_opt */ yytestcase(yyruleno==332);
      /* (333) window ::= nm frame_opt */ yytestcase(yyruleno==333);
      /* (334) frame_opt ::= */ yytestcase(yyruleno==334);
      /* (335) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */ yytestcase(yyruleno==335);
      /* (336) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */ yytestcase(yyruleno==336);
      /* (337) range_or_rows ::= RANGE|ROWS|GROUPS */ yytestcase(yyruleno==337);
      /* (338) frame_bound_s ::= frame_bound (OPTIMIZED OUT) */ assert(yyruleno!=338);
      /* (339) frame_bound_s ::= UNBOUNDED PRECEDING */ yytestcase(yyruleno==339);
      /* (340) frame_bound_e ::= frame_bound (OPTIMIZED OUT) */ assert(yyruleno!=340);
      /* (341) frame_bound_e ::= UNBOUNDED FOLLOWING */ yytestcase(yyruleno==341);
      /* (342) frame_bound ::= expr PRECEDING|FOLLOWING */ yytestcase(yyruleno==342);
      /* (343) frame_bound ::= CURRENT ROW */ yytestcase(yyruleno==343);
      /* (344) frame_exclude_opt ::= */ yytestcase(yyruleno==344);
      /* (345) frame_exclude_opt ::= EXCLUDE frame_exclude */ yytestcase(yyruleno==345);
      /* (346) frame_exclude ::= NO OTHERS */ yytestcase(yyruleno==346);
      /* (347) frame_exclude ::= CURRENT ROW */ yytestcase(yyruleno==347);
      /* (348) frame_exclude ::= GROUP|TIES */ yytestcase(yyruleno==348);
      /* (349) window_clause ::= WINDOW windowdefn_list */ yytestcase(yyruleno==349);
      /* (350) filter_over ::= filter_clause over_clause */ yytestcase(yyruleno==350);
      /* (351) filter_over ::= over_clause (OPTIMIZED OUT) */ assert(yyruleno!=351);
      /* (352) filter_over ::= filter_clause */ yytestcase(yyruleno==352);
      /* (353) over_clause ::= OVER LP window RP */ yytestcase(yyruleno==353);
      /* (354) over_clause ::= OVER nm */ yytestcase(yyruleno==354);
      /* (355) filter_clause ::= FILTER LP WHERE expr RP */ yytestcase(yyruleno==355);
      /* (356) term ::= QNUMBER */ yytestcase(yyruleno==356);
      /* (357) ecmd ::= explain cmdx SEMI */ yytestcase(yyruleno==357);
      /* (358) trans_opt ::= */ yytestcase(yyruleno==358);
      /* (359) trans_opt ::= TRANSACTION */ yytestcase(yyruleno==359);
      /* (360) trans_opt ::= TRANSACTION nm */ yytestcase(yyruleno==360);
      /* (361) savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==361);
      /* (362) savepoint_opt ::= */ yytestcase(yyruleno==362);
      /* (363) cmd ::= create_table create_table_args */ yytestcase(yyruleno==363);
      /* (364) table_option_set ::= table_option (OPTIMIZED OUT) */ assert(yyruleno!=364);
      /* (365) columnlist ::= columnlist COMMA columnname carglist */ yytestcase(yyruleno==365);
      /* (366) columnlist ::= columnname carglist */ yytestcase(yyruleno==366);
      /* (367) typetoken ::= typename */ yytestcase(yyruleno==367);
      /* (368) typename ::= ID|STRING */ yytestcase(yyruleno==368);
      /* (369) signed ::= plus_num (OPTIMIZED OUT) */ assert(yyruleno!=369);
      /* (370) signed ::= minus_num (OPTIMIZED OUT) */ assert(yyruleno!=370);
      /* (371) carglist ::= carglist ccons */ yytestcase(yyruleno==371);
      /* (372) carglist ::= */ yytestcase(yyruleno==372);
      /* (373) ccons ::= NULL onconf */ yytestcase(yyruleno==373);
      /* (374) ccons ::= GENERATED ALWAYS AS generated */ yytestcase(yyruleno==374);
      /* (375) ccons ::= AS generated */ yytestcase(yyruleno==375);
      /* (376) conslist_opt ::= COMMA conslist */ yytestcase(yyruleno==376);
      /* (377) conslist ::= conslist tconscomma tcons */ yytestcase(yyruleno==377);
      /* (378) conslist ::= tcons (OPTIMIZED OUT) */ assert(yyruleno!=378);
      /* (379) tconscomma ::= */ yytestcase(yyruleno==379);
      /* (380) defer_subclause_opt ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=380);
      /* (381) resolvetype ::= raisetype (OPTIMIZED OUT) */ assert(yyruleno!=381);
      /* (382) oneselect ::= values */ yytestcase(yyruleno==382);
      /* (383) as ::= ID|STRING */ yytestcase(yyruleno==383);
      /* (384) indexed_opt ::= indexed_by (OPTIMIZED OUT) */ assert(yyruleno!=384);
      /* (385) returning ::= */ yytestcase(yyruleno==385);
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
