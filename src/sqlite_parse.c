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
#include "src/ast/ast_base.h"
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
  SyntaqliteConstraintValue yy117;
  int yy144;
  u32 yy391;
  SyntaqliteColumnNameValue yy487;
  SyntaqliteConstraintListValue yy637;
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
 /*     0 */   161,  522,  419,  510,  283, 1633, 1644, 1300,  574, 1290,
 /*    10 */   290, 1633, 1622,  130,  574,  565,  234,  571, 1644,  130,
 /*    20 */   574,  565,  234,  571,  574,  264,  230,    6,  464, 1259,
 /*    30 */   295,  284,  130, 1341, 1281,  234,  299,  413, 1643,  477,
 /*    40 */   478, 1293,  290, 1633,  344,  290, 1633, 1292,  574,  983,
 /*    50 */  1643, 1513, 1511,  565, 1281,  571,  565,  984,  571, 1659,
 /*    60 */   402,  137,  139,   91,  254, 1228, 1228, 1063, 1066, 1053,
 /*    70 */  1053,  557, 1639,   84,  497,  413,  378, 1279, 1639,   51,
 /*    80 */   290, 1633,  379,  507, 1639,   51,  466,  130, 1639,   51,
 /*    90 */   234,  565,  556,  571, 1019,  413,  462, 1613,  558,  137,
 /*   100 */   139,   91,   44, 1228, 1228, 1063, 1066, 1053, 1053,  574,
 /*   110 */  1632,  127, 1639,   19,  452, 1331,  421,  127,  107,  137,
 /*   120 */   139,   91,  409, 1228, 1228, 1063, 1066, 1053, 1053, 1632,
 /*   130 */   127,  290, 1633,  209, 1333,  359, 1174,  489, 1174,  140,
 /*   140 */   452,  377,  565, 1582,  571,  526,  321,  575,  376, 1584,
 /*   150 */  1506,  547,  452, 1582,  499,  135,  135,  136,  136,  136,
 /*   160 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   170 */   131,  128,  452, 1639,   19, 1204, 1204,  294,   45,  429,
 /*   180 */  1050, 1050, 1064, 1067,  574,  127,  524, 1307,  574, 1631,
 /*   190 */   452, 1328,  499,  135,  135,  136,  136,  136,  136,  134,
 /*   200 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   210 */  1204, 1204,    7,  135,  135,  136,  136,  136,  136,  134,
 /*   220 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   230 */   574,  413, 1204,  198,  110, 1054, 1205,  134,  134,  134,
 /*   240 */   134,  133,  133,  132,  132,  132,  131,  128, 1639,   84,
 /*   250 */   301,  413, 1639,   84,  516,  137,  139,   91, 1628, 1228,
 /*   260 */  1228, 1063, 1066, 1053, 1053,  525,  574, 1204,  556, 1451,
 /*   270 */   346, 1205,  556,   97,  523,  137,  139,   91,  555, 1228,
 /*   280 */  1228, 1063, 1066, 1053, 1053, 1577,  212, 1277,  304,  413,
 /*   290 */  1169,  291, 1633,  251, 1639,   19,  290, 1633,  432,  380,
 /*   300 */  1595,  543,  565, 1169,  571,  501, 1169,  565,  452,  571,
 /*   310 */   399,  413,  316,  137,  139,   91,  573, 1228, 1228, 1063,
 /*   320 */  1066, 1053, 1053,   44, 1541,  850, 1188,  851,  452,  852,
 /*   330 */  1639,   19, 1629,  293,  437,  137,  139,   91,  452, 1228,
 /*   340 */  1228, 1063, 1066, 1053, 1053,  542,  452,  321,  575,  135,
 /*   350 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   360 */   133,  132,  132,  132,  131,  128,  452,  321,  575,  135,
 /*   370 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   380 */   133,  132,  132,  132,  131,  128,  157, 1368,  452,  323,
 /*   390 */  1105,  132,  132,  132,  131,  128, 1040,  133,  133,  132,
 /*   400 */   132,  132,  131,  128, 1028,  574,  373,  135,  135,  136,
 /*   410 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*   420 */   132,  132,  131,  128,  212,  452,  413,  467,  253,  135,
 /*   430 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   440 */   133,  132,  132,  132,  131,  128, 1126,  413,  407,  886,
 /*   450 */   137,  139,   91,   98, 1228, 1228, 1063, 1066, 1053, 1053,
 /*   460 */    44, 1029,  241, 1028, 1028, 1030,  902,  183, 1127, 1639,
 /*   470 */    84,  137,  139,   91,  495, 1228, 1228, 1063, 1066, 1053,
 /*   480 */  1053,  131,  128, 1128,  452,  413,  479,  130,  527,  556,
 /*   490 */   234,  470,  370,   46,  430,  444,  490,  361,  922,  903,
 /*   500 */   568, 1620, 1621,  452,  321,  575, 1169,  221,  923,  137,
 /*   510 */   139,   91,  417, 1228, 1228, 1063, 1066, 1053, 1053, 1169,
 /*   520 */     7,  384, 1169,  375,  452, 1204, 1204,  136,  136,  136,
 /*   530 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   540 */   131,  128,  424,  267,  135,  135,  136,  136,  136,  136,
 /*   550 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   560 */   128,  420,  452,  237,  879,  135,  135,  136,  136,  136,
 /*   570 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   580 */   131,  128, 1204,  490,  361,  127, 1205, 1233, 1204, 1204,
 /*   590 */   524, 1235,  530, 1665,  113,   25,  219,  413, 1542, 1234,
 /*   600 */  1204, 1204,  231,  135,  135,  136,  136,  136,  136,  134,
 /*   610 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   620 */    14,  137,  139,   91,  516, 1228, 1228, 1063, 1066, 1053,
 /*   630 */  1053,  406,  405,  100,  413, 1204, 1204,  447,  446,  247,
 /*   640 */    95, 1204, 1204,  236,  521, 1204,  461,  458,  457, 1205,
 /*   650 */   129, 1236,  233, 1236,  539,  413,  456, 1204,  137,  139,
 /*   660 */    91, 1205, 1228, 1228, 1063, 1066, 1053, 1053,  107,  525,
 /*   670 */   448, 1620, 1621, 1224,  452,  447,  446, 1224,  267,  137,
 /*   680 */   139,   91, 1244, 1228, 1228, 1063, 1066, 1053, 1053, 1236,
 /*   690 */   308, 1236, 1204,  452, 1505,  526, 1205,  480, 1204,  290,
 /*   700 */  1633,  236, 1205, 1450,  461,  458,  457, 1598, 1639,   22,
 /*   710 */   565,  452,  571,  382,  456,  135,  135,  136,  136,  136,
 /*   720 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   730 */   131,  128,  452,  380, 1595, 1041,  136,  136,  136,  136,
 /*   740 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   750 */   128,  155,  135,  135,  136,  136,  136,  136,  134,  134,
 /*   760 */   134,  134,  133,  133,  132,  132,  132,  131,  128, 1147,
 /*   770 */   413,  322, 1024,  135,  135,  136,  136,  136,  136,  134,
 /*   780 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   790 */   413,  499,  944, 1187,  137,  139,   91,  943, 1228, 1228,
 /*   800 */  1063, 1066, 1053, 1053,  574, 1204, 1204, 1019, 1224,   10,
 /*   810 */   549,  212, 1224,  505,  137,  139,   91,  518, 1228, 1228,
 /*   820 */  1063, 1066, 1053, 1053,  559,  536,  290, 1633,  413,  548,
 /*   830 */  1149, 1657, 1657,  290, 1633,  398,  429,  565, 1180,  571,
 /*   840 */   576,  349,  936,  936,  565, 1311,  571,  452,  359,  485,
 /*   850 */   347,  532,  137,  139,   91, 1339, 1228, 1228, 1063, 1066,
 /*   860 */  1053, 1053, 1204,  199,  290, 1633, 1205,  452, 1639,   19,
 /*   870 */   303,  351,  442,  326, 1207,  565, 1180,  571, 1207, 1204,
 /*   880 */  1204, 1110, 1110,  513,  321,  575,  116,  307,  135,  135,
 /*   890 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   900 */   132,  132,  132,  131,  128,  452,  516, 1324,  135,  135,
 /*   910 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   920 */   132,  132,  132,  131,  128, 1345,  983,  213, 1125,  348,
 /*   930 */   949,  350, 1204, 1204,  984,  565, 1204,  571,  574,  413,
 /*   940 */  1205,  574, 1310,  574,  120,  215,  135,  135,  136,  136,
 /*   950 */   136,  136,  134,  134,  134,  134,  133,  133,  132,  132,
 /*   960 */   132,  131,  128,  137,  139,   91,  413, 1228, 1228, 1063,
 /*   970 */  1066, 1053, 1053, 1280,  533, 1664, 1340, 1204, 1204, 1204,
 /*   980 */  1204, 1308,  505, 1449, 1204, 1204,  574,  551,  574, 1204,
 /*   990 */   137,  139,   91, 1205, 1228, 1228, 1063, 1066, 1053, 1053,
 /*  1000 */   413,  534, 1639,   60,  971, 1639,   82, 1639,   84,  970,
 /*  1010 */   290, 1633, 1309,  380, 1595,  574,  452, 1204, 1204,  505,
 /*  1020 */   138,  565,  535,  571,  137,  126,   91,  528, 1228, 1228,
 /*  1030 */  1063, 1066, 1053, 1053, 1204,    5, 1204,   39, 1205,  358,
 /*  1040 */  1205, 1204,  499,  452,  383, 1205,  516, 1204, 1204,  183,
 /*  1050 */  1639,   19, 1639,   19,  207,   38,  208,  135,  135,  136,
 /*  1060 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*  1070 */   132,  132,  131,  128, 1204,   42,  310,  452, 1205, 1639,
 /*  1080 */    19,  413,  392,  550,  135,  135,  136,  136,  136,  136,
 /*  1090 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*  1100 */   128,  574,    7,  574, 1204,    3,  139,   91, 1205, 1228,
 /*  1110 */  1228, 1063, 1066, 1053, 1053,  413,  427, 1126,  135,  135,
 /*  1120 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*  1130 */   132,  132,  132,  131,  128,  232, 1250, 1435,  309, 1127,
 /*  1140 */   368,   91,  390, 1228, 1228, 1063, 1066, 1053, 1053, 1188,
 /*  1150 */   205, 1568, 1107, 1568, 1128, 1568, 1107, 1169,  452,  969,
 /*  1160 */   372,  474,  372,  370,  372, 1639,   84, 1639,   84,  561,
 /*  1170 */  1169,  232, 1637, 1169,  540, 1665,  481, 1568,  429,  124,
 /*  1180 */  1568,  566,  511,  944, 1188,  317,  372,  318,  943,  372,
 /*  1190 */   516,  331,  452,  286,  969,  969,  569,  401, 1249,  135,
 /*  1200 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*  1210 */   133,  132,  132,  132,  131,  128,  532,  574,  908, 1040,
 /*  1220 */   909, 1498,   48,  882,   50,  562,   47, 1028, 1250,  434,
 /*  1230 */   969,  158,  440,  135,  135,  136,  136,  136,  136,  134,
 /*  1240 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*  1250 */   124,   49,  566,  574, 1040, 1188,    6,  298,  450,  450,
 /*  1260 */  1336,  862, 1028,  574,  874,  160,  520,  569,  874,  214,
 /*  1270 */   240,  239,  238, 1569,  124, 1569,  566, 1569,  338, 1188,
 /*  1280 */   882, 1639,   84,  453, 1029, 1435, 1028, 1028, 1030,  401,
 /*  1290 */  1148,  569, 1366,   34,  482,  574,  562, 1435,  574, 1570,
 /*  1300 */   516,  445, 1569,  336,    9,  412,   44,  122,  122,  948,
 /*  1310 */   574,  241,  488,  335,  123,  450,    4, 1639,   84,  572,
 /*  1320 */   562, 1028, 1028, 1030, 1031, 1040,  491, 1639,   84,  450,
 /*  1330 */   450, 1589, 1435, 1028,  545, 1147,  574,  449, 1153,  544,
 /*  1340 */  1571,  574, 1085,  574, 1282,  418, 1289,  451,  356, 1040,
 /*  1350 */   321,  575,  426,  450,  450,  222,  574, 1028,  327, 1639,
 /*  1360 */   145,  417, 1639,  147,   34, 1435,  506,  124,  551,  566,
 /*  1370 */   366,    7, 1188,  503, 1639,   53,  330,  435,  122,  122,
 /*  1380 */     7,  567,  418, 1552,  569,  123,  450,    4,   34,  436,
 /*  1390 */   572,  228, 1028, 1028, 1030, 1031, 1149, 1658, 1658,  574,
 /*  1400 */  1639,   54,  122,  122,    7, 1639,   55, 1639,   56,  123,
 /*  1410 */   450,    4, 1169,  562,  572,  574, 1028, 1028, 1030, 1031,
 /*  1420 */  1639,   66,  469,  552,  512, 1169,  574,  545, 1169,  423,
 /*  1430 */   422,  185,  546,  243,  280,  329,  472,  332,  471,  242,
 /*  1440 */  1204, 1204, 1040,  537, 1575,  330,  450,  450,    7, 1554,
 /*  1450 */  1028,  574,  504, 1576,  124,  483,  566,  441, 1637, 1188,
 /*  1460 */   475,  574,  302, 1639,   67,  493,  412,  541,  288,  230,
 /*  1470 */  1104,  569, 1104,  492,  574,  166,  574, 1574,  496, 1639,
 /*  1480 */    21,   34, 1550,  574,  412,  893,  574,  305,  320,  412,
 /*  1490 */  1639,   57, 1627,  911,  222,  122,  122, 1204,  574,  412,
 /*  1500 */   562, 1205,  123,  450,    4,  574,  108,  572,  574, 1028,
 /*  1510 */  1028, 1030, 1031,  339,  545, 1639,   68,  101,  218,  544,
 /*  1520 */   121, 1573,  118,    2,  161, 1639,   58,  454,  263, 1040,
 /*  1530 */  1223,  324,  113,  450,  450,  228,  113, 1028, 1639,   69,
 /*  1540 */  1639,   70, 1144,  403,  901,  900,  560, 1639,   71,  574,
 /*  1550 */  1639,   72,  465, 1259,  295,  889, 1103, 1341, 1103,  889,
 /*  1560 */   574,  340, 1639,   73,  357,  113,  290, 1633,   34, 1639,
 /*  1570 */    74,  343, 1639,   75,  574, 1021,  266,  565, 1380,  571,
 /*  1580 */   574, 1092,  122,  122, 1601, 1092,  476,  582,  254,  123,
 /*  1590 */   450,    4, 1379,  292,  572,  224, 1028, 1028, 1030, 1031,
 /*  1600 */   574,  498,  266,  396,  289,  396,  574,  395, 1616,  277,
 /*  1610 */   466,  393, 1032, 1639,   76,  574, 1032,  859,  484,    2,
 /*  1620 */   161,  574, 1557,  574, 1639,   77, 1255,    1, 1603,  500,
 /*  1630 */   266,  352,  248,  574,  342,  124, 1530,  566, 1639,   59,
 /*  1640 */  1188,  517,  341, 1529, 1639,   61,  508,  574,  465, 1259,
 /*  1650 */   295,  363,  569, 1341, 1376,   90,  367,  566,  371, 1389,
 /*  1660 */  1188, 1434,  290, 1633, 1639,   20,  250,  574,  362,  113,
 /*  1670 */  1639,  143,  569,  565,  177,  571, 1362,   43,  574, 1639,
 /*  1680 */   144,  562, 1439,  574,  254, 1639,   79, 1639,   62,  574,
 /*  1690 */  1088,  263,  974,  266,  249,  574,  389, 1639,   80,  165,
 /*  1700 */   113,  562,  167,  986,  987, 1374,  466,  941,  138,  391,
 /*  1710 */  1040, 1639,   63, 1268,  450,  450,  414,  574, 1028,  872,
 /*  1720 */   159,  321,  575, 1267, 1602,  942,  138,  939,  138, 1269,
 /*  1730 */  1040, 1639,   81, 1609,  450,  450, 1359,  554, 1028,  313,
 /*  1740 */   281,  314, 1639,   64,  315,   12,  397, 1639,  170,   34,
 /*  1750 */   574,  459,  227, 1639,  171,  574,  124,  553,  566, 1639,
 /*  1760 */    88, 1188,  574,  122,  122,  574,  334, 1192, 1612,   34,
 /*  1770 */   123,  450,    4,  569,  246,  572,  574, 1028, 1028, 1030,
 /*  1780 */  1031, 1639,   65,  122,  122, 1307,  476,  582,  574, 1421,
 /*  1790 */   123,  450,    4,  292,  574,  572, 1416, 1028, 1028, 1030,
 /*  1800 */  1031,  300,  562,  396,  355,  396,  354,  395,  574,  277,
 /*  1810 */  1409,  393, 1426,  306, 1639,  146,  574,  859, 1425, 1639,
 /*  1820 */    83,  360,  574,  502, 1502,  574, 1639,  168,  410, 1639,
 /*  1830 */   148, 1040,  248, 1501,  342,  450,  450,  563,  574, 1028,
 /*  1840 */  1639,  142,  341, 1371,  210, 1372, 1370,  223, 1369,  211,
 /*  1850 */   574,  387, 1639,  169,  203,  473,  400, 1327, 1639,  162,
 /*  1860 */   574, 1326,  574, 1325,   40,  574,  250,  463,   93, 1626,
 /*  1870 */    34, 1624, 1639,  152,  177, 1625,  425,   43, 1297, 1318,
 /*  1880 */  1639,  151,  404,  893,  122,  122, 1639,  149,  476, 1639,
 /*  1890 */   150,  123,  450,    4,  249,  292,  572,  333, 1028, 1028,
 /*  1900 */  1030, 1031, 1639,   86, 1296,  396, 1317,  396, 1295,  395,
 /*  1910 */  1244,  277,  187,  393, 1639,   78,  414,  244, 1549,  859,
 /*  1920 */   274,  321,  575, 1547, 1639,   87, 1639,   85, 1241, 1639,
 /*  1930 */    52,   96,   99, 1507,  248,  428,  342,  100,  220, 1422,
 /*  1940 */   196,   13,  182,  486,  341,  191,  189,  192,  487,  579,
 /*  1950 */   256,  193,  194,  108,  408, 1428, 1427,  494,   15, 1430,
 /*  1960 */   411, 1496,  200,  509,  260,  106, 1518, 1192,  250,  515,
 /*  1970 */   282,  438,  262,  365,  519,  439,  177,  369,  311,   43,
 /*  1980 */   312,  141,  268, 1350,  551,  374,  269,  443, 1580, 1579,
 /*  1990 */  1394, 1393,  233,   11, 1483, 1594,  249,  117,  381,  319,
 /*  2000 */   109, 1349,  216,  529,  577,  388,  273, 1270,  275,  279,
 /*  2010 */  1198,  276,  278,  580,  385,  386, 1265, 1260,  414,  172,
 /*  2020 */   154,  173,  415,  321,  575, 1534,  186,  296,  416, 1535,
 /*  2030 */   225,  226, 1533,  174,  335, 1532,   89,  235,  184,  455,
 /*  2040 */    92,   23,  460,  325,   24,  175,  153,  891, 1194,  217,
 /*  2050 */   328,  468,  904,   94,  297,  176, 1193,  337,  245,  156,
 /*  2060 */  1102,  345, 1100,  178,  188, 1223,  252,  190,  255,  925,
 /*  2070 */   353, 1116,  195,  179,  180,  431,  433,  102,  197,  181,
 /*  2080 */   103,  104, 1119,  257,  258, 1115,  105,  163,  259,  364,
 /*  2090 */    26, 1108,  201,  266,  514, 1238,  261,  202, 1154,  978,
 /*  2100 */   229,  265,  972,   27,  531,  112, 1155,   16,   28,   29,
 /*  2110 */   285,  287,  204,  138, 1160,   41,  206, 1069,   30,   31,
 /*  2120 */  1171,  538,  111,  164, 1185, 1175,    8, 1179,  113, 1178,
 /*  2130 */  1173,   32,   33,  114,  115,  119, 1073, 1083,  935, 1070,
 /*  2140 */  1068,   18, 1072, 1124,  270,   35,  861, 1617,  564, 1033,
 /*  2150 */    17,  578,  873,  125,   36,  394,  271,  570,  846,  581,
 /*  2160 */  1256,   37, 1256, 1256,  272,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */   217,  250,  213,  214,  259,  260,  187,  226,  187,  251,
 /*    10 */   259,  260,  231,  190,  187,  270,  193,  272,  187,  190,
 /*    20 */   187,  270,  193,  272,  187,  274,  275,  189,  245,  246,
 /*    30 */   247,  189,  190,  250,  187,  193,  250,   37,  219,  220,
 /*    40 */   221,  251,  259,  260,  187,  259,  260,  251,  187,   49,
 /*    50 */   219,  220,  221,  270,  187,  272,  270,   57,  272,  306,
 /*    60 */   307,   61,   62,   63,  281,   65,   66,   67,   68,   69,
 /*    70 */    70,  250,  251,  252,  202,   37,  253,  230,  251,  252,
 /*    80 */   259,  260,  253,  294,  251,  252,  303,  190,  251,  252,
 /*    90 */   193,  270,  271,  272,   85,   37,  229,  230,  277,   61,
 /*   100 */    62,   63,   93,   65,   66,   67,   68,   69,   70,  187,
 /*   110 */   199,  288,  251,  252,  114,  204,  255,  288,  115,   61,
 /*   120 */    62,   63,  250,   65,   66,   67,   68,   69,   70,  199,
 /*   130 */   288,  259,  260,  295,  204,  126,   98,  299,  100,   81,
 /*   140 */   114,  314,  270,  316,  272,  142,  137,  138,  315,  316,
 /*   150 */   293,  314,  114,  316,  187,  155,  156,  157,  158,  159,
 /*   160 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   170 */   170,  171,  114,  251,  252,  116,  117,  255,   84,  187,
 /*   180 */    65,   66,   67,   68,  187,  288,   37,  224,  187,  226,
 /*   190 */   114,  228,  187,  155,  156,  157,  158,  159,  160,  161,
 /*   200 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   210 */   116,  117,  240,  155,  156,  157,  158,  159,  160,  161,
 /*   220 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   230 */   187,   37,  173,  174,  176,  120,  177,  161,  162,  163,
 /*   240 */   164,  165,  166,  167,  168,  169,  170,  171,  251,  252,
 /*   250 */   283,   37,  251,  252,  187,   61,   62,   63,  233,   65,
 /*   260 */    66,   67,   68,   69,   70,  116,  187,  173,  271,  287,
 /*   270 */   278,  177,  271,   79,  277,   61,   62,   63,  277,   65,
 /*   280 */    66,   67,   68,   69,   70,  313,  187,  250,  283,   37,
 /*   290 */    88,  259,  260,   43,  251,  252,  259,  260,  255,  317,
 /*   300 */   318,   99,  270,  101,  272,  187,  104,  270,  114,  272,
 /*   310 */   211,   37,  201,   61,   62,   63,  187,   65,   66,   67,
 /*   320 */    68,   69,   70,   93,  257,   13,   42,   15,  114,   17,
 /*   330 */   251,  252,  233,  234,  255,   61,   62,   63,  114,   65,
 /*   340 */    66,   67,   68,   69,   70,  143,  114,  137,  138,  155,
 /*   350 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   360 */   166,  167,  168,  169,  170,  171,  114,  137,  138,  155,
 /*   370 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   380 */   166,  167,  168,  169,  170,  171,  176,  276,  114,  175,
 /*   390 */    21,  167,  168,  169,  170,  171,  112,  165,  166,  167,
 /*   400 */   168,  169,  170,  171,  120,  187,  187,  155,  156,  157,
 /*   410 */   158,  159,  160,  161,  162,  163,  164,  165,  166,  167,
 /*   420 */   168,  169,  170,  171,  187,  114,   37,  175,  178,  155,
 /*   430 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*   440 */   166,  167,  168,  169,  170,  171,   23,   37,  211,  175,
 /*   450 */    61,   62,   63,   43,   65,   66,   67,   68,   69,   70,
 /*   460 */    93,  177,   65,  179,  180,  181,   53,  187,   45,  251,
 /*   470 */   252,   61,   62,   63,  115,   65,   66,   67,   68,   69,
 /*   480 */    70,  170,  171,   60,  114,   37,  119,  190,  187,  271,
 /*   490 */   193,   78,  187,   84,   73,  277,  127,  128,   75,   86,
 /*   500 */   218,  219,  220,  114,  137,  138,   88,  148,   85,   61,
 /*   510 */    62,   63,  115,   65,   66,   67,   68,   69,   70,  101,
 /*   520 */   240,  267,  104,  269,  114,  116,  117,  157,  158,  159,
 /*   530 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   540 */   170,  171,  129,   43,  155,  156,  157,  158,  159,  160,
 /*   550 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*   560 */   171,  256,  114,   44,  175,  155,  156,  157,  158,  159,
 /*   570 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   580 */   170,  171,  173,  127,  128,  288,  177,  115,  116,  117,
 /*   590 */    37,  119,  312,  313,  176,  174,  148,   37,  301,  127,
 /*   600 */   116,  117,  187,  155,  156,  157,  158,  159,  160,  161,
 /*   610 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   620 */    43,   61,   62,   63,  187,   65,   66,   67,   68,   69,
 /*   630 */    70,  165,  166,  149,   37,  116,  117,  165,  166,   29,
 /*   640 */   174,  116,  117,  118,  187,  173,  121,  122,  123,  177,
 /*   650 */    71,  179,    8,  181,   10,   37,  131,  173,   61,   62,
 /*   660 */    63,  177,   65,   66,   67,   68,   69,   70,  115,  116,
 /*   670 */   218,  219,  220,  173,  114,  165,  166,  177,   43,   61,
 /*   680 */    62,   63,   72,   65,   66,   67,   68,   69,   70,  179,
 /*   690 */   250,  181,  173,  114,  257,  142,  177,  202,  173,  259,
 /*   700 */   260,  118,  177,  287,  121,  122,  123,  187,  251,  252,
 /*   710 */   270,  114,  272,  187,  131,  155,  156,  157,  158,  159,
 /*   720 */   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
 /*   730 */   170,  171,  114,  317,  318,  175,  157,  158,  159,  160,
 /*   740 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*   750 */   171,  174,  155,  156,  157,  158,  159,  160,  161,  162,
 /*   760 */   163,  164,  165,  166,  167,  168,  169,  170,  171,  113,
 /*   770 */    37,  187,  175,  155,  156,  157,  158,  159,  160,  161,
 /*   780 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*   790 */    37,  187,  134,  175,   61,   62,   63,  139,   65,   66,
 /*   800 */    67,   68,   69,   70,  187,  116,  117,   85,  173,  174,
 /*   810 */    78,  187,  177,  187,   61,   62,   63,  250,   65,   66,
 /*   820 */    67,   68,   69,   70,  250,  167,  259,  260,   37,   97,
 /*   830 */   174,  175,  176,  259,  260,  211,  187,  270,  106,  272,
 /*   840 */   133,    1,  135,  136,  270,  225,  272,  114,  126,  127,
 /*   850 */   128,  187,   61,   62,   63,  250,   65,   66,   67,   68,
 /*   860 */    69,   70,  173,  174,  259,  260,  177,  114,  251,  252,
 /*   870 */   244,   31,  255,  249,  173,  270,  144,  272,  177,  116,
 /*   880 */   117,  126,  127,  128,  137,  138,  153,  283,  155,  156,
 /*   890 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*   900 */   167,  168,  169,  170,  171,  114,  187,  187,  155,  156,
 /*   910 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*   920 */   167,  168,  169,  170,  171,  260,   49,  278,  175,   89,
 /*   930 */   167,   91,  116,  117,   57,  270,  173,  272,  187,   37,
 /*   940 */   177,  187,  225,  187,  153,   43,  155,  156,  157,  158,
 /*   950 */   159,  160,  161,  162,  163,  164,  165,  166,  167,  168,
 /*   960 */   169,  170,  171,   61,   62,   63,   37,   65,   66,   67,
 /*   970 */    68,   69,   70,  187,  310,  311,  257,  116,  117,  116,
 /*   980 */   117,  187,  187,  287,  116,  117,  187,  143,  187,  173,
 /*   990 */    61,   62,   63,  177,   65,   66,   67,   68,   69,   70,
 /*  1000 */    37,  250,  251,  252,  141,  251,  252,  251,  252,  141,
 /*  1010 */   259,  260,  225,  317,  318,  187,  114,  116,  117,  187,
 /*  1020 */   176,  270,  271,  272,   61,   62,   63,  271,   65,   66,
 /*  1030 */    67,   68,   69,   70,  173,  174,  173,  156,  177,  244,
 /*  1040 */   177,  173,  187,  114,  290,  177,  187,  116,  117,  187,
 /*  1050 */   251,  252,  251,  252,  255,  174,  255,  155,  156,  157,
 /*  1060 */   158,  159,  160,  161,  162,  163,  164,  165,  166,  167,
 /*  1070 */   168,  169,  170,  171,  173,  174,  244,  114,  177,  251,
 /*  1080 */   252,   37,   31,  255,  155,  156,  157,  158,  159,  160,
 /*  1090 */   161,  162,  163,  164,  165,  166,  167,  168,  169,  170,
 /*  1100 */   171,  187,  240,  187,  173,  174,   62,   63,  177,   65,
 /*  1110 */    66,   67,   68,   69,   70,   37,  257,   23,  155,  156,
 /*  1120 */   157,  158,  159,  160,  161,  162,  163,  164,  165,  166,
 /*  1130 */   167,  168,  169,  170,  171,  117,  113,  187,  283,   45,
 /*  1140 */    89,   63,   91,   65,   66,   67,   68,   69,   70,   42,
 /*  1150 */   188,  187,   47,  187,   60,  187,   51,   88,  114,  141,
 /*  1160 */   196,  302,  196,  187,  196,  251,  252,  251,  252,   75,
 /*  1170 */   101,  117,  201,  104,  312,  313,  205,  187,  187,   37,
 /*  1180 */   187,   39,   77,  134,   42,  271,  196,  271,  139,  196,
 /*  1190 */   187,  187,  114,  175,  176,  141,   54,  174,  175,  155,
 /*  1200 */   156,  157,  158,  159,  160,  161,  162,  163,  164,  165,
 /*  1210 */   166,  167,  168,  169,  170,  171,  187,  187,   13,  112,
 /*  1220 */    15,    1,  258,  116,  258,   83,  258,  120,  113,  279,
 /*  1230 */   176,    6,  256,  155,  156,  157,  158,  159,  160,  161,
 /*  1240 */   162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
 /*  1250 */    37,  258,   39,  187,  112,   42,  189,  111,  116,  117,
 /*  1260 */   257,   41,  120,  187,  173,  174,   37,   54,  177,  278,
 /*  1270 */   126,  127,  128,  309,   37,  309,   39,  309,  132,   42,
 /*  1280 */   173,  251,  252,   37,  177,  187,  179,  180,  181,  174,
 /*  1290 */   175,   54,  200,  151,  262,  187,   83,  187,  187,  309,
 /*  1300 */   187,  271,  309,  120,   67,  273,   93,  165,  166,  167,
 /*  1310 */   187,   65,   92,  130,  172,  173,  174,  251,  252,  177,
 /*  1320 */    83,  179,  180,  181,  182,  112,  128,  251,  252,  116,
 /*  1330 */   117,  242,  187,  120,   97,  113,  187,  271,  109,  102,
 /*  1340 */   311,  187,  122,  187,  194,  195,  187,  271,  150,  112,
 /*  1350 */   137,  138,   37,  116,  117,  140,  187,  120,   43,  251,
 /*  1360 */   252,  115,  251,  252,  151,  187,  299,   37,  143,   39,
 /*  1370 */   257,  240,   42,   37,  251,  252,  130,  279,  165,  166,
 /*  1380 */   240,  194,  195,  187,   54,  172,  173,  174,  151,  279,
 /*  1390 */   177,  176,  179,  180,  181,  182,  174,  175,  176,  187,
 /*  1400 */   251,  252,  165,  166,  240,  251,  252,  251,  252,  172,
 /*  1410 */   173,  174,   88,   83,  177,  187,  179,  180,  181,  182,
 /*  1420 */   251,  252,  107,   99,  279,  101,  187,   97,  104,  114,
 /*  1430 */   304,  305,  102,  118,  119,  120,  121,  122,  123,  124,
 /*  1440 */   116,  117,  112,  143,  313,  130,  116,  117,  240,  187,
 /*  1450 */   120,  187,  116,  313,   37,  262,   39,  279,  201,   42,
 /*  1460 */   187,  187,  205,  251,  252,  128,  273,  143,  274,  275,
 /*  1470 */   179,   54,  181,  262,  187,  175,  187,  313,  262,  251,
 /*  1480 */   252,  151,  187,  187,  273,  125,  187,  150,  262,  273,
 /*  1490 */   251,  252,  175,  176,  140,  165,  166,  173,  187,  273,
 /*  1500 */    83,  177,  172,  173,  174,  187,  147,  177,  187,  179,
 /*  1510 */   180,  181,  182,  187,   97,  251,  252,  147,  148,  102,
 /*  1520 */   152,  313,  154,  216,  217,  251,  252,  175,  176,  112,
 /*  1530 */   176,  175,  176,  116,  117,  176,  176,  120,  251,  252,
 /*  1540 */   251,  252,  175,  176,  119,  120,  199,  251,  252,  187,
 /*  1550 */   251,  252,  245,  246,  247,  173,  179,  250,  181,  177,
 /*  1560 */   187,  187,  251,  252,  175,  176,  259,  260,  151,  251,
 /*  1570 */   252,  187,  251,  252,  187,  175,  176,  270,  187,  272,
 /*  1580 */   187,  173,  165,  166,    0,  177,    2,    3,  281,  172,
 /*  1590 */   173,  174,  187,    9,  177,  238,  179,  180,  181,  182,
 /*  1600 */   187,  175,  176,   19,  174,   21,  187,   23,  178,   25,
 /*  1610 */   303,   27,  173,  251,  252,  187,  177,   33,  187,  216,
 /*  1620 */   217,  187,  187,  187,  251,  252,  319,  320,  321,  175,
 /*  1630 */   176,  187,   48,  187,   50,   37,  187,   39,  251,  252,
 /*  1640 */    42,  296,   58,  187,  251,  252,  187,  187,  245,  246,
 /*  1650 */   247,  187,   54,  250,  187,   37,  187,   39,  187,  187,
 /*  1660 */    42,  187,  259,  260,  251,  252,   82,  187,  175,  176,
 /*  1670 */   251,  252,   54,  270,   90,  272,  187,   93,  187,  251,
 /*  1680 */   252,   83,  187,  187,  281,  251,  252,  251,  252,  187,
 /*  1690 */   175,  176,  175,  176,  110,  187,  187,  251,  252,  175,
 /*  1700 */   176,   83,  192,   95,   96,  187,  303,  175,  176,  187,
 /*  1710 */   112,  251,  252,  187,  116,  117,  132,  187,  120,  175,
 /*  1720 */   176,  137,  138,  187,  321,  175,  176,  175,  176,  187,
 /*  1730 */   112,  251,  252,  187,  116,  117,  188,  139,  120,  188,
 /*  1740 */   243,  188,  251,  252,  188,  261,  207,  251,  252,  151,
 /*  1750 */   187,  222,  236,  251,  252,  187,   37,  139,   39,  251,
 /*  1760 */   252,   42,  187,  165,  166,  187,  253,  183,  248,  151,
 /*  1770 */   172,  173,  174,   54,  210,  177,  187,  179,  180,  181,
 /*  1780 */   182,  251,  252,  165,  166,  224,    2,    3,  187,  285,
 /*  1790 */   172,  173,  174,    9,  187,  177,  282,  179,  180,  181,
 /*  1800 */   182,  263,   83,   19,  264,   21,  300,   23,  187,   25,
 /*  1810 */   282,   27,  285,  264,  251,  252,  187,   33,  285,  251,
 /*  1820 */   252,  263,  187,  300,  253,  187,  251,  252,  285,  251,
 /*  1830 */   252,  112,   48,  253,   50,  116,  117,  292,  187,  120,
 /*  1840 */   251,  252,   58,  276,  267,  276,  276,  261,  276,  267,
 /*  1850 */   187,  263,  251,  252,  174,  115,  254,  237,  251,  252,
 /*  1860 */   187,  237,  187,  237,  176,  187,   82,  235,  174,  239,
 /*  1870 */   151,  237,  251,  252,   90,  239,   76,   93,  237,  227,
 /*  1880 */   251,  252,  254,  125,  165,  166,  251,  252,    2,  251,
 /*  1890 */   252,  172,  173,  174,  110,    9,  177,  237,  179,  180,
 /*  1900 */   181,  182,  251,  252,  222,   19,  227,   21,  237,   23,
 /*  1910 */    72,   25,  210,   27,  251,  252,  132,  210,  191,   33,
 /*  1920 */   178,  137,  138,  191,  251,  252,  251,  252,   56,  251,
 /*  1930 */   252,  215,  215,  293,   48,  191,   50,  149,  148,  286,
 /*  1940 */   174,  284,   61,   35,   58,  206,  203,  206,  191,   35,
 /*  1950 */   212,  206,  206,  147,  264,  286,  286,  264,  284,  203,
 /*  1960 */   264,  264,  203,  191,  212,  152,  298,  183,   82,   74,
 /*  1970 */   191,   43,  212,  297,  197,  114,   90,  254,  198,   93,
 /*  1980 */   198,  146,  191,  268,  143,  267,  103,   94,  241,  241,
 /*  1990 */   280,  280,    8,  174,  289,  318,  110,  152,  191,  291,
 /*  2000 */   145,  268,  266,  144,  254,  191,  212,  191,  212,   11,
 /*  2010 */    25,  209,  209,  208,  265,  264,  208,  208,  132,  232,
 /*  2020 */   223,  232,  308,  137,  138,  189,  305,  223,  308,  189,
 /*  2030 */   238,  238,  189,  232,  130,  189,  189,  124,  174,   79,
 /*  2040 */   174,  174,   79,  175,  174,  176,  175,  173,  183,  174,
 /*  2050 */    43,  108,   46,  174,   79,   55,  183,    4,   29,   31,
 /*  2060 */   175,  138,  175,  129,  149,  176,   43,  140,  142,   39,
 /*  2070 */    31,  183,  140,  129,  129,   73,   55,  156,  149,  129,
 /*  2080 */   156,  156,  116,   52,  178,  183,  156,    9,  115,    1,
 /*  2090 */   174,   80,   80,  176,   59,   87,  178,  115,  109,  116,
 /*  2100 */   178,   52,  141,   52,   43,  147,  175,  174,   52,   52,
 /*  2110 */   175,  175,  174,  176,  175,  174,  176,  175,   52,   52,
 /*  2120 */   100,  174,  176,  175,  175,   87,   62,   87,  176,  105,
 /*  2130 */    98,   52,   52,  140,  140,  176,   21,  175,  134,  175,
 /*  2140 */   175,   43,  175,  175,  174,  174,   39,  178,  176,  175,
 /*  2150 */   174,   37,  175,  174,  174,   29,  178,  176,    7,    5,
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
#define YY_SHIFT_MAX      (2154)
static const unsigned short int yy_shift_ofst[] = {
 /*     0 */  1784, 1584, 1886, 1213, 1213,  230,    9, 1237, 1330, 1417,
 /*    10 */  1719, 1719, 1719,  367,  230,  230,  230,  230,  230,    0,
 /*    20 */     0,  194,  929, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*    30 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,  472,  472,
 /*    40 */   525, 1324, 1324,   94,  409,  816,  816,  210,  210,  210,
 /*    50 */   210,   38,   58,  214,  252,  274,  389,  410,  448,  560,
 /*    60 */   597,  618,  733,  753,  791,  902,  929,  929,  929,  929,
 /*    70 */   929,  929,  929,  929,  929,  929,  929,  929,  929,  929,
 /*    80 */   929,  929,  929,  963,  929,  929, 1044, 1078, 1078, 1142,
 /*    90 */  1598, 1618, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   100 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   110 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   120 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   130 */  1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719, 1719,
 /*   140 */  1719, 1719,  579,  370,  370,  370,  370,  370,  370,  370,
 /*   150 */    76,  232,  224,  519, 1246,  816,  816,  816,  816,  510,
 /*   160 */   510, 1220,  311,  456,  644,  644,  644,  747,   26,   26,
 /*   170 */  2165, 2165, 1315, 1315, 1315,  519,  423,  484,   59,   59,
 /*   180 */    59,   59,  423,  202,  816,  656, 1222,  816,  816,  816,
 /*   190 */   816,  816,  816,  816,  816,  816,  816,  816,  816,  816,
 /*   200 */   816,  816,  816,  816,  816,  553,  816,  418,  418,  369,
 /*   210 */  1069, 1069,  701,  149,  149,  701, 1225, 2165, 2165, 2165,
 /*   220 */  2165, 2165, 2165, 2165, 1107,  284,  284,  583,  689,  763,
 /*   230 */   861,  863,  868,  901,  931,  816,  816,  816,  413,  413,
 /*   240 */   413,  816,  816,  816,  816,  816,  816,  816,  816,  816,
 /*   250 */   816,  816,  816,  816,  722,  816,  816,  816,  816,  816,
 /*   260 */   816,  816,  816,  816, 1018,  816,  816,  816,  635,  732,
 /*   270 */  1094,  816,  816,  816,  816,  816,  816,  816,  816,  816,
 /*   280 */   466,  755, 1105,  707,  500,  500,  500,  500, 1054,  658,
 /*   290 */   707,  707,  312, 1317, 1360,  577,  397,  877,  610,  359,
 /*   300 */  1370, 1215, 1370, 1336, 1359,  359,  359, 1359,  359, 1215,
 /*   310 */  1336,  877,  877,    3,    3,    3,    3,  844,  844, 1368,
 /*   320 */  1354, 1049, 1680, 1740, 1740, 1740, 1688, 1694, 1694, 1740,
 /*   330 */  1800, 1680, 1740, 1758, 1740, 1800, 1740, 1838, 1838, 1742,
 /*   340 */  1742, 1872, 1872, 1742, 1788, 1790, 1766, 1881, 1908, 1908,
 /*   350 */  1908, 1908, 1742, 1914, 1806, 1790, 1790, 1806, 1766, 1881,
 /*   360 */  1806, 1881, 1806, 1742, 1914, 1813, 1895, 1742, 1914, 1928,
 /*   370 */  1861, 1861, 1680, 1742, 1835, 1841, 1883, 1883, 1893, 1893,
 /*   380 */  1984, 1819, 1742, 1845, 1835, 1855, 1859, 1806, 1680, 1742,
 /*   390 */  1914, 1742, 1914, 1985, 1985, 1998, 1998, 1998, 2165, 2165,
 /*   400 */  2165, 2165, 2165, 2165, 2165, 2165, 2165, 2165, 2165, 2165,
 /*   410 */  2165, 2165, 2165,  115,  840, 1023, 1115, 1144, 1091, 1051,
 /*   420 */  1352, 1356, 1367, 1382, 1425, 1205, 1183, 1146,  881,  250,
 /*   430 */  1198, 1337, 1389,  421, 1400, 1426, 1454, 1493, 1229, 1408,
 /*   440 */  1515, 1517, 1524, 1608, 1300, 1532, 1291, 1377, 1544, 1550,
 /*   450 */  1430, 1552, 1439, 1904, 1913, 1864, 1960, 1866, 1867, 1868,
 /*   460 */  1870, 1963, 1869, 1871, 1865, 1873, 1875, 1874, 2007, 1943,
 /*   470 */  2006, 1879, 1975, 2000, 2053, 2029, 2028, 1885, 1887, 1923,
 /*   480 */  1915, 1934, 1889, 1889, 2023, 1927, 2030, 1926, 2039, 1888,
 /*   490 */  1932, 1944, 1889, 1945, 2002, 2021, 1889, 1929, 1921, 1924,
 /*   500 */  1925, 1930, 1950, 1966, 2031, 1906, 1902, 2078, 1916, 1973,
 /*   510 */  2088, 2011, 1917, 2012, 2008, 2035, 1918, 1982, 1931, 1933,
 /*   520 */  1989, 1922, 1935, 1936, 1983, 2049, 1938, 1961, 1937, 2051,
 /*   530 */  1939, 1941, 2061, 1940, 1942, 1946, 1948, 2056, 1958, 1947,
 /*   540 */  1949, 2057, 2066, 2067, 2020, 2038, 2032, 2064, 2040, 2024,
 /*   550 */  1952, 2079, 2080, 1993, 1994, 1962, 1937, 1964, 1965, 1967,
 /*   560 */  1959, 1968, 1970, 2115, 1971, 1972, 1976, 1974, 1977, 1979,
 /*   570 */  1980, 1981, 1969, 1978, 1986, 1987, 2004, 2098, 2107, 2114,
 /*   580 */  2126, 2151, 2154,
};
#define YY_REDUCE_COUNT (412)
#define YY_REDUCE_MIN   (-255)
#define YY_REDUCE_MAX   (1847)
static const short yy_reduce_ofst[] = {
 /*     0 */  1307, 1403, -217, -179,  751, -249, -128, -173, -167, -163,
 /*    10 */    -3,    1,  218, -214,   37,  440,  567,  574,  605, -177,
 /*    20 */  -171,  297, -158, -139,  -78,   43,   79,  756,  617,  799,
 /*    30 */   801,  914,  828,  916,  754, 1030, 1066, 1076, -181, -169,
 /*    40 */    99,  280,  862,  964,  966,  968,  993, -255,   32, -255,
 /*    50 */    32, -103, -103, -103, -103, -103, -103, -103, -103, -103,
 /*    60 */  -103, -103, -103, -103, -103, -103, -103, -103, -103, -103,
 /*    70 */  -103, -103, -103, -103, -103, -103, -103, -103, -103, -103,
 /*    80 */  -103, -103, -103, -103, -103, -103, -103, -103, -103,  457,
 /*    90 */  1108, 1111, 1123, 1149, 1154, 1156, 1169, 1212, 1228, 1239,
 /*   100 */  1264, 1274, 1287, 1289, 1296, 1299, 1311, 1318, 1321, 1362,
 /*   110 */  1373, 1387, 1393, 1413, 1419, 1428, 1434, 1436, 1446, 1460,
 /*   120 */  1480, 1491, 1496, 1502, 1508, 1530, 1563, 1568, 1575, 1578,
 /*   130 */  1589, 1601, 1607, 1621, 1629, 1635, 1638, 1651, 1663, 1673,
 /*   140 */  1675, 1678, -103, -103, -103, -103, -103, -103, -103, -103,
 /*   150 */  -103, -103, -103, -133,  -37,  624,  859,  990,  664,  282,
 /*   160 */   452, -211, -103, -162,  -18,  416,  696,  665, -103, -103,
 /*   170 */  -103, -103, -219, -219, -219, -153,  -89, -143,  -33,    5,
 /*   180 */   604,  855,  -70,  -28,  305, -247, -247,  237,   -8,  649,
 /*   190 */   991,   67,  437,  719, 1003,  626,  950,  795, 1098, 1110,
 /*   200 */   832, 1145, 1113,  976, 1178,  111, 1029, 1131, 1140, 1067,
 /*   210 */  1164, 1208, 1150,  971, 1257, 1187,  254, 1126, 1032, 1193,
 /*   220 */  1211, 1216, 1194, 1226, -242, -210, -204,   25,  118,  129,
 /*   230 */   219,  301,  415,  520,  526,  584,  720,  786,  620,  717,
 /*   240 */   787,  794, 1004, 1159, 1196, 1262, 1273, 1295, 1326, 1374,
 /*   250 */  1384, 1391, 1405, 1431,  495, 1435, 1444, 1449, 1456, 1459,
 /*   260 */  1464, 1467, 1469, 1471, 1092, 1472, 1474, 1489,  962, 1089,
 /*   270 */  1347, 1495,  129, 1509, 1518, 1522, 1526, 1536, 1542, 1546,
 /*   280 */  1357, 1345, 1497, 1510, 1548, 1551, 1553, 1556, 1092, 1484,
 /*   290 */  1510, 1510, 1539, 1516, 1529, 1520, 1561, 1513, 1564, 1504,
 /*   300 */  1514, 1538, 1528, 1506, 1540, 1527, 1533, 1549, 1543, 1558,
 /*   310 */  1523, 1571, 1580, 1567, 1569, 1570, 1572, 1577, 1582, 1545,
 /*   320 */  1588, 1586, 1602, 1620, 1624, 1626, 1632, 1630, 1636, 1634,
 /*   330 */  1652, 1628, 1641, 1682, 1660, 1679, 1671, 1702, 1707, 1727,
 /*   340 */  1732, 1716, 1717, 1744, 1640, 1653, 1657, 1743, 1739, 1741,
 /*   350 */  1745, 1746, 1757, 1738, 1690, 1669, 1670, 1693, 1674, 1756,
 /*   360 */  1696, 1759, 1697, 1772, 1752, 1668, 1676, 1779, 1760, 1777,
 /*   370 */  1780, 1782, 1723, 1791, 1715, 1718, 1747, 1748, 1710, 1711,
 /*   380 */  1677, 1705, 1807, 1708, 1733, 1736, 1749, 1751, 1750, 1814,
 /*   390 */  1794, 1816, 1796, 1802, 1803, 1805, 1808, 1809, 1787, 1789,
 /*   400 */  1797, 1714, 1720, 1721, 1804, 1792, 1793, 1801, 1836, 1840,
 /*   410 */  1843, 1846, 1847,
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
  /*  240 */ "range_or_rows",
  /*  241 */ "frame_exclude_opt",
  /*  242 */ "frame_exclude",
  /*  243 */ "trigger_time",
  /*  244 */ "trnm",
  /*  245 */ "cmdx",
  /*  246 */ "cmd",
  /*  247 */ "create_table",
  /*  248 */ "create_table_args",
  /*  249 */ "columnlist",
  /*  250 */ "select",
  /*  251 */ "term",
  /*  252 */ "expr",
  /*  253 */ "sortorder",
  /*  254 */ "eidlist_opt",
  /*  255 */ "sortlist",
  /*  256 */ "eidlist",
  /*  257 */ "fullname",
  /*  258 */ "wqlist",
  /*  259 */ "selectnowith",
  /*  260 */ "oneselect",
  /*  261 */ "distinct",
  /*  262 */ "selcollist",
  /*  263 */ "from",
  /*  264 */ "where_opt",
  /*  265 */ "groupby_opt",
  /*  266 */ "having_opt",
  /*  267 */ "orderby_opt",
  /*  268 */ "limit_opt",
  /*  269 */ "window_clause",
  /*  270 */ "values",
  /*  271 */ "nexprlist",
  /*  272 */ "mvalues",
  /*  273 */ "sclp",
  /*  274 */ "seltablist",
  /*  275 */ "stl_prefix",
  /*  276 */ "on_using",
  /*  277 */ "exprlist",
  /*  278 */ "xfullname",
  /*  279 */ "idlist",
  /*  280 */ "nulls",
  /*  281 */ "with",
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
   245,  /* (2) cmdx ::= cmd */
   246,  /* (3) cmd ::= BEGIN transtype trans_opt */
   207,  /* (4) transtype ::= */
   207,  /* (5) transtype ::= DEFERRED */
   207,  /* (6) transtype ::= IMMEDIATE */
   207,  /* (7) transtype ::= EXCLUSIVE */
   246,  /* (8) cmd ::= COMMIT|END trans_opt */
   246,  /* (9) cmd ::= ROLLBACK trans_opt */
   246,  /* (10) cmd ::= SAVEPOINT nm */
   246,  /* (11) cmd ::= RELEASE savepoint_opt nm */
   246,  /* (12) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
   247,  /* (13) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   217,  /* (14) createkw ::= CREATE */
   212,  /* (15) ifnotexists ::= */
   212,  /* (16) ifnotexists ::= IF NOT EXISTS */
   213,  /* (17) temp ::= TEMP */
   213,  /* (18) temp ::= */
   248,  /* (19) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   248,  /* (20) create_table_args ::= AS select */
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
   246,  /* (79) cmd ::= DROP TABLE ifexists fullname */
   206,  /* (80) ifexists ::= IF EXISTS */
   206,  /* (81) ifexists ::= */
   246,  /* (82) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   246,  /* (83) cmd ::= DROP VIEW ifexists fullname */
   246,  /* (84) cmd ::= select */
   250,  /* (85) select ::= WITH wqlist selectnowith */
   250,  /* (86) select ::= WITH RECURSIVE wqlist selectnowith */
   250,  /* (87) select ::= selectnowith */
   259,  /* (88) selectnowith ::= selectnowith multiselect_op oneselect */
   192,  /* (89) multiselect_op ::= UNION */
   192,  /* (90) multiselect_op ::= UNION ALL */
   192,  /* (91) multiselect_op ::= EXCEPT|INTERSECT */
   260,  /* (92) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
   260,  /* (93) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   270,  /* (94) values ::= VALUES LP nexprlist RP */
   260,  /* (95) oneselect ::= mvalues */
   272,  /* (96) mvalues ::= values COMMA LP nexprlist RP */
   272,  /* (97) mvalues ::= mvalues COMMA LP nexprlist RP */
   261,  /* (98) distinct ::= DISTINCT */
   261,  /* (99) distinct ::= ALL */
   261,  /* (100) distinct ::= */
   273,  /* (101) sclp ::= */
   262,  /* (102) selcollist ::= sclp scanpt expr scanpt as */
   262,  /* (103) selcollist ::= sclp scanpt STAR */
   262,  /* (104) selcollist ::= sclp scanpt nm DOT STAR */
   188,  /* (105) as ::= AS nm */
   188,  /* (106) as ::= */
   263,  /* (107) from ::= */
   263,  /* (108) from ::= FROM seltablist */
   275,  /* (109) stl_prefix ::= seltablist joinop */
   275,  /* (110) stl_prefix ::= */
   274,  /* (111) seltablist ::= stl_prefix nm dbnm as on_using */
   274,  /* (112) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   274,  /* (113) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   274,  /* (114) seltablist ::= stl_prefix LP select RP as on_using */
   274,  /* (115) seltablist ::= stl_prefix LP seltablist RP as on_using */
   191,  /* (116) dbnm ::= */
   191,  /* (117) dbnm ::= DOT nm */
   257,  /* (118) fullname ::= nm */
   257,  /* (119) fullname ::= nm DOT nm */
   278,  /* (120) xfullname ::= nm */
   278,  /* (121) xfullname ::= nm DOT nm */
   278,  /* (122) xfullname ::= nm DOT nm AS nm */
   278,  /* (123) xfullname ::= nm AS nm */
   200,  /* (124) joinop ::= COMMA|JOIN */
   200,  /* (125) joinop ::= JOIN_KW JOIN */
   200,  /* (126) joinop ::= JOIN_KW nm JOIN */
   200,  /* (127) joinop ::= JOIN_KW nm nm JOIN */
   276,  /* (128) on_using ::= ON expr */
   276,  /* (129) on_using ::= USING LP idlist RP */
   276,  /* (130) on_using ::= */
   205,  /* (131) indexed_opt ::= */
   201,  /* (132) indexed_by ::= INDEXED BY nm */
   201,  /* (133) indexed_by ::= NOT INDEXED */
   267,  /* (134) orderby_opt ::= */
   267,  /* (135) orderby_opt ::= ORDER BY sortlist */
   255,  /* (136) sortlist ::= sortlist COMMA expr sortorder nulls */
   255,  /* (137) sortlist ::= expr sortorder nulls */
   253,  /* (138) sortorder ::= ASC */
   253,  /* (139) sortorder ::= DESC */
   253,  /* (140) sortorder ::= */
   280,  /* (141) nulls ::= NULLS FIRST */
   280,  /* (142) nulls ::= NULLS LAST */
   280,  /* (143) nulls ::= */
   265,  /* (144) groupby_opt ::= */
   265,  /* (145) groupby_opt ::= GROUP BY nexprlist */
   266,  /* (146) having_opt ::= */
   266,  /* (147) having_opt ::= HAVING expr */
   268,  /* (148) limit_opt ::= */
   268,  /* (149) limit_opt ::= LIMIT expr */
   268,  /* (150) limit_opt ::= LIMIT expr OFFSET expr */
   268,  /* (151) limit_opt ::= LIMIT expr COMMA expr */
   246,  /* (152) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
   264,  /* (153) where_opt ::= */
   264,  /* (154) where_opt ::= WHERE expr */
   282,  /* (155) where_opt_ret ::= */
   282,  /* (156) where_opt_ret ::= WHERE expr */
   282,  /* (157) where_opt_ret ::= RETURNING selcollist */
   282,  /* (158) where_opt_ret ::= WHERE expr RETURNING selcollist */
   246,  /* (159) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   283,  /* (160) setlist ::= setlist COMMA nm EQ expr */
   283,  /* (161) setlist ::= setlist COMMA LP idlist RP EQ expr */
   283,  /* (162) setlist ::= nm EQ expr */
   283,  /* (163) setlist ::= LP idlist RP EQ expr */
   246,  /* (164) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   246,  /* (165) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
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
   279,  /* (177) idlist ::= idlist COMMA nm */
   279,  /* (178) idlist ::= nm */
   252,  /* (179) expr ::= LP expr RP */
   252,  /* (180) expr ::= ID|INDEXED|JOIN_KW */
   252,  /* (181) expr ::= nm DOT nm */
   252,  /* (182) expr ::= nm DOT nm DOT nm */
   251,  /* (183) term ::= NULL|FLOAT|BLOB */
   251,  /* (184) term ::= STRING */
   251,  /* (185) term ::= INTEGER */
   252,  /* (186) expr ::= VARIABLE */
   252,  /* (187) expr ::= expr COLLATE ID|STRING */
   252,  /* (188) expr ::= CAST LP expr AS typetoken RP */
   252,  /* (189) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   252,  /* (190) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   252,  /* (191) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   252,  /* (192) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   252,  /* (193) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   252,  /* (194) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   251,  /* (195) term ::= CTIME_KW */
   252,  /* (196) expr ::= LP nexprlist COMMA expr RP */
   252,  /* (197) expr ::= expr AND expr */
   252,  /* (198) expr ::= expr OR expr */
   252,  /* (199) expr ::= expr LT|GT|GE|LE expr */
   252,  /* (200) expr ::= expr EQ|NE expr */
   252,  /* (201) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
   252,  /* (202) expr ::= expr PLUS|MINUS expr */
   252,  /* (203) expr ::= expr STAR|SLASH|REM expr */
   252,  /* (204) expr ::= expr CONCAT expr */
   190,  /* (205) likeop ::= NOT LIKE_KW|MATCH */
   252,  /* (206) expr ::= expr likeop expr */
   252,  /* (207) expr ::= expr likeop expr ESCAPE expr */
   252,  /* (208) expr ::= expr ISNULL|NOTNULL */
   252,  /* (209) expr ::= expr NOT NULL */
   252,  /* (210) expr ::= expr IS expr */
   252,  /* (211) expr ::= expr IS NOT expr */
   252,  /* (212) expr ::= expr IS NOT DISTINCT FROM expr */
   252,  /* (213) expr ::= expr IS DISTINCT FROM expr */
   252,  /* (214) expr ::= NOT expr */
   252,  /* (215) expr ::= BITNOT expr */
   252,  /* (216) expr ::= PLUS|MINUS expr */
   252,  /* (217) expr ::= expr PTR expr */
   288,  /* (218) between_op ::= BETWEEN */
   288,  /* (219) between_op ::= NOT BETWEEN */
   252,  /* (220) expr ::= expr between_op expr AND expr */
   193,  /* (221) in_op ::= IN */
   193,  /* (222) in_op ::= NOT IN */
   252,  /* (223) expr ::= expr in_op LP exprlist RP */
   252,  /* (224) expr ::= LP select RP */
   252,  /* (225) expr ::= expr in_op LP select RP */
   252,  /* (226) expr ::= expr in_op nm dbnm paren_exprlist */
   252,  /* (227) expr ::= EXISTS LP select RP */
   252,  /* (228) expr ::= CASE case_operand case_exprlist case_else END */
   291,  /* (229) case_exprlist ::= case_exprlist WHEN expr THEN expr */
   291,  /* (230) case_exprlist ::= WHEN expr THEN expr */
   292,  /* (231) case_else ::= ELSE expr */
   292,  /* (232) case_else ::= */
   290,  /* (233) case_operand ::= */
   277,  /* (234) exprlist ::= */
   271,  /* (235) nexprlist ::= nexprlist COMMA expr */
   271,  /* (236) nexprlist ::= expr */
   289,  /* (237) paren_exprlist ::= */
   289,  /* (238) paren_exprlist ::= LP exprlist RP */
   246,  /* (239) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
   214,  /* (240) uniqueflag ::= UNIQUE */
   214,  /* (241) uniqueflag ::= */
   254,  /* (242) eidlist_opt ::= */
   254,  /* (243) eidlist_opt ::= LP eidlist RP */
   256,  /* (244) eidlist ::= eidlist COMMA nm collate sortorder */
   256,  /* (245) eidlist ::= nm collate sortorder */
   198,  /* (246) collate ::= */
   198,  /* (247) collate ::= COLLATE ID|STRING */
   246,  /* (248) cmd ::= DROP INDEX ifexists fullname */
   246,  /* (249) cmd ::= VACUUM vinto */
   246,  /* (250) cmd ::= VACUUM nm vinto */
   293,  /* (251) vinto ::= INTO expr */
   293,  /* (252) vinto ::= */
   246,  /* (253) cmd ::= PRAGMA nm dbnm */
   246,  /* (254) cmd ::= PRAGMA nm dbnm EQ nmnum */
   246,  /* (255) cmd ::= PRAGMA nm dbnm LP nmnum RP */
   246,  /* (256) cmd ::= PRAGMA nm dbnm EQ minus_num */
   246,  /* (257) cmd ::= PRAGMA nm dbnm LP minus_num RP */
   219,  /* (258) plus_num ::= PLUS INTEGER|FLOAT */
   220,  /* (259) minus_num ::= MINUS INTEGER|FLOAT */
   246,  /* (260) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
   294,  /* (261) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
   243,  /* (262) trigger_time ::= BEFORE|AFTER */
   243,  /* (263) trigger_time ::= INSTEAD OF */
   243,  /* (264) trigger_time ::= */
   296,  /* (265) trigger_event ::= DELETE|INSERT */
   296,  /* (266) trigger_event ::= UPDATE */
   296,  /* (267) trigger_event ::= UPDATE OF idlist */
   298,  /* (268) when_clause ::= */
   298,  /* (269) when_clause ::= WHEN expr */
   295,  /* (270) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
   295,  /* (271) trigger_cmd_list ::= trigger_cmd SEMI */
   244,  /* (272) trnm ::= nm DOT nm */
   300,  /* (273) tridxby ::= INDEXED BY nm */
   300,  /* (274) tridxby ::= NOT INDEXED */
   299,  /* (275) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
   299,  /* (276) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
   299,  /* (277) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
   299,  /* (278) trigger_cmd ::= scanpt select scanpt */
   252,  /* (279) expr ::= RAISE LP IGNORE RP */
   252,  /* (280) expr ::= RAISE LP raisetype COMMA expr RP */
   199,  /* (281) raisetype ::= ROLLBACK */
   199,  /* (282) raisetype ::= ABORT */
   199,  /* (283) raisetype ::= FAIL */
   246,  /* (284) cmd ::= DROP TRIGGER ifexists fullname */
   246,  /* (285) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
   246,  /* (286) cmd ::= DETACH database_kw_opt expr */
   301,  /* (287) key_opt ::= */
   301,  /* (288) key_opt ::= KEY expr */
   246,  /* (289) cmd ::= REINDEX */
   246,  /* (290) cmd ::= REINDEX nm dbnm */
   246,  /* (291) cmd ::= ANALYZE */
   246,  /* (292) cmd ::= ANALYZE nm dbnm */
   246,  /* (293) cmd ::= ALTER TABLE fullname RENAME TO nm */
   246,  /* (294) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
   246,  /* (295) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
   302,  /* (296) add_column_fullname ::= fullname */
   246,  /* (297) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
   246,  /* (298) cmd ::= create_vtab */
   246,  /* (299) cmd ::= create_vtab LP vtabarglist RP */
   303,  /* (300) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
   305,  /* (301) vtabarg ::= */
   306,  /* (302) vtabargtoken ::= ANY */
   306,  /* (303) vtabargtoken ::= lp anylist RP */
   307,  /* (304) lp ::= LP */
   281,  /* (305) with ::= WITH wqlist */
   281,  /* (306) with ::= WITH RECURSIVE wqlist */
   197,  /* (307) wqas ::= AS */
   197,  /* (308) wqas ::= AS MATERIALIZED */
   197,  /* (309) wqas ::= AS NOT MATERIALIZED */
   309,  /* (310) wqitem ::= withnm eidlist_opt wqas LP select RP */
   196,  /* (311) withnm ::= nm */
   258,  /* (312) wqlist ::= wqitem */
   258,  /* (313) wqlist ::= wqlist COMMA wqitem */
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
   240,  /* (324) range_or_rows ::= RANGE|ROWS|GROUPS */
   314,  /* (325) frame_bound_s ::= frame_bound */
   314,  /* (326) frame_bound_s ::= UNBOUNDED PRECEDING */
   315,  /* (327) frame_bound_e ::= frame_bound */
   315,  /* (328) frame_bound_e ::= UNBOUNDED FOLLOWING */
   316,  /* (329) frame_bound ::= expr PRECEDING|FOLLOWING */
   316,  /* (330) frame_bound ::= CURRENT ROW */
   241,  /* (331) frame_exclude_opt ::= */
   241,  /* (332) frame_exclude_opt ::= EXCLUDE frame_exclude */
   242,  /* (333) frame_exclude ::= NO OTHERS */
   242,  /* (334) frame_exclude ::= CURRENT ROW */
   242,  /* (335) frame_exclude ::= GROUP|TIES */
   269,  /* (336) window_clause ::= WINDOW windowdefn_list */
   287,  /* (337) filter_over ::= filter_clause over_clause */
   287,  /* (338) filter_over ::= over_clause */
   287,  /* (339) filter_over ::= filter_clause */
   318,  /* (340) over_clause ::= OVER LP window RP */
   318,  /* (341) over_clause ::= OVER nm */
   317,  /* (342) filter_clause ::= FILTER LP WHERE expr RP */
   251,  /* (343) term ::= QNUMBER */
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
   246,  /* (355) cmd ::= create_table create_table_args */
   229,  /* (356) table_option_set ::= table_option */
   249,  /* (357) columnlist ::= columnlist COMMA columnname carglist */
   249,  /* (358) columnlist ::= columnname carglist */
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
   259,  /* (376) selectnowith ::= oneselect */
   260,  /* (377) oneselect ::= values */
   273,  /* (378) sclp ::= selcollist COMMA */
   188,  /* (379) as ::= ID|STRING */
   205,  /* (380) indexed_opt ::= indexed_by */
   286,  /* (381) returning ::= */
   252,  /* (382) expr ::= term */
   190,  /* (383) likeop ::= LIKE_KW|MATCH */
   290,  /* (384) case_operand ::= expr */
   277,  /* (385) exprlist ::= nexprlist */
   221,  /* (386) nmnum ::= plus_num */
   221,  /* (387) nmnum ::= nm */
   221,  /* (388) nmnum ::= ON */
   221,  /* (389) nmnum ::= DELETE */
   221,  /* (390) nmnum ::= DEFAULT */
   219,  /* (391) plus_num ::= INTEGER|FLOAT */
   297,  /* (392) foreach_clause ::= */
   297,  /* (393) foreach_clause ::= FOR EACH ROW */
   244,  /* (394) trnm ::= nm */
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
   281,  /* (406) with ::= */
   310,  /* (407) windowdefn_list ::= windowdefn */
   312,  /* (408) window ::= frame_opt */
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
      case 17: /* temp ::= TEMP */ yytestcase(yyruleno==17);
      case 48: /* autoinc ::= AUTOINCR */ yytestcase(yyruleno==48);
      case 240: /* uniqueflag ::= UNIQUE */ yytestcase(yyruleno==240);
      case 399: /* kwcolumn_opt ::= COLUMNKW */ yytestcase(yyruleno==399);
{
    yymsp[0].minor.yy144 = 1;
}
        break;
      case 1: /* explain ::= EXPLAIN QUERY PLAN */
      case 309: /* wqas ::= AS NOT MATERIALIZED */ yytestcase(yyruleno==309);
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
      case 13: /* create_table ::= createkw temp TABLE ifnotexists nm dbnm */
{
    SyntaqliteSourceSpan tbl_name = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[0].minor.yy0) : syntaqlite_span(pCtx, yymsp[-1].minor.yy0);
    SyntaqliteSourceSpan tbl_schema = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-1].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-5].minor.yy391 = ast_create_table_stmt(pCtx->astCtx,
        tbl_name, tbl_schema, (uint8_t)yymsp[-4].minor.yy144, (uint8_t)yymsp[-2].minor.yy144,
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
    yymsp[-4].minor.yy391 = ast_create_table_stmt(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN, 0, 0,
        (SyntaqliteCreateTableStmtFlags){.raw = (uint8_t)yymsp[0].minor.yy144}, yymsp[-3].minor.yy391, yymsp[-2].minor.yy391, SYNTAQLITE_NULL_NODE);
}
        break;
      case 20: /* create_table_args ::= AS select */
{
    yymsp[-1].minor.yy391 = ast_create_table_stmt(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN, 0, 0,
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
    yylhsminor.yy487.name = syntaqlite_span(pCtx, yymsp[-1].minor.yy0);
    yylhsminor.yy487.typetoken = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[0].minor.yy0) : SYNTAQLITE_NO_SPAN;
}
  yymsp[-1].minor.yy487 = yylhsminor.yy487;
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
    yymsp[-1].minor.yy117.node = SYNTAQLITE_NULL_NODE;
    yymsp[-1].minor.yy117.pending_name = syntaqlite_span(pCtx, yymsp[0].minor.yy0);
}
        break;
      case 33: /* ccons ::= DEFAULT scantok term */
{
    yymsp[-2].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNTAQLITE_NO_SPAN,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        yymsp[0].minor.yy391, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-2].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 34: /* ccons ::= DEFAULT LP expr RP */
{
    yymsp[-3].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNTAQLITE_NO_SPAN,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        yymsp[-1].minor.yy391, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-3].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 35: /* ccons ::= DEFAULT PLUS scantok term */
{
    yymsp[-3].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNTAQLITE_NO_SPAN,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        yymsp[0].minor.yy391, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-3].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 36: /* ccons ::= DEFAULT MINUS scantok term */
{
    // Create a unary minus wrapping the term
    uint32_t neg = ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_MINUS, yymsp[0].minor.yy391);
    yymsp[-3].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNTAQLITE_NO_SPAN,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        neg, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-3].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 37: /* ccons ::= DEFAULT scantok ID|INDEXED */
{
    // Treat the identifier as a literal expression
    uint32_t lit = ast_literal(pCtx->astCtx,
        SYNTAQLITE_LITERAL_TYPE_STRING, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
    yymsp[-2].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNTAQLITE_NO_SPAN,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        lit, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-2].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 38: /* ccons ::= NOT NULL onconf */
{
    yymsp[-2].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NOT_NULL,
        SYNTAQLITE_NO_SPAN,
        (uint8_t)yymsp[0].minor.yy144, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-2].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 39: /* ccons ::= PRIMARY KEY sortorder onconf autoinc */
{
    yymsp[-4].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_PRIMARY_KEY,
        SYNTAQLITE_NO_SPAN,
        (uint8_t)yymsp[-1].minor.yy144, (uint8_t)yymsp[-2].minor.yy391, (uint8_t)yymsp[0].minor.yy144,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-4].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 40: /* ccons ::= UNIQUE onconf */
{
    yymsp[-1].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_UNIQUE,
        SYNTAQLITE_NO_SPAN,
        (uint8_t)yymsp[0].minor.yy144, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-1].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 41: /* ccons ::= CHECK LP expr RP */
{
    yymsp[-3].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_CHECK,
        SYNTAQLITE_NO_SPAN,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, yymsp[-1].minor.yy391, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-3].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 42: /* ccons ::= REFERENCES nm eidlist_opt refargs */
{
    // Decode refargs: low byte = on_delete, next byte = on_update
    SyntaqliteForeignKeyAction on_del = (SyntaqliteForeignKeyAction)(yymsp[0].minor.yy144 & 0xff);
    SyntaqliteForeignKeyAction on_upd = (SyntaqliteForeignKeyAction)((yymsp[0].minor.yy144 >> 8) & 0xff);
    uint32_t fk = ast_foreign_key_clause(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0), yymsp[-1].minor.yy391, on_del, on_upd, 0);
    yymsp[-3].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_REFERENCES,
        SYNTAQLITE_NO_SPAN,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, fk);
    yymsp[-3].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 43: /* ccons ::= defer_subclause */
{
    // Create a minimal constraint that just marks deferral.
    // In practice, this follows a REFERENCES ccons. We'll handle it
    // by updating the last constraint in the list if possible.
    // For simplicity, we create a separate REFERENCES constraint with just deferral info.
    // The printer will show it as a separate constraint entry.
    uint32_t fk = ast_foreign_key_clause(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION,
        SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION,
        (uint8_t)yymsp[0].minor.yy144);
    yylhsminor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_REFERENCES,
        SYNTAQLITE_NO_SPAN,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, fk);
    yylhsminor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
  yymsp[0].minor.yy117 = yylhsminor.yy117;
        break;
      case 44: /* ccons ::= COLLATE ID|STRING */
{
    yymsp[-1].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_COLLATE,
        SYNTAQLITE_NO_SPAN,
        0, 0, 0,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-1].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 45: /* generated ::= LP expr RP */
{
    yymsp[-2].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_GENERATED,
        SYNTAQLITE_NO_SPAN,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, yymsp[-1].minor.yy391, SYNTAQLITE_NULL_NODE);
    yymsp[-2].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 46: /* generated ::= LP expr RP ID */
{
    SyntaqliteGeneratedColumnStorage storage = SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL;
    if (yymsp[0].minor.yy0.n == 6 && strncasecmp(yymsp[0].minor.yy0.z, "stored", 6) == 0) {
        storage = SYNTAQLITE_GENERATED_COLUMN_STORAGE_STORED;
    }
    yymsp[-3].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_GENERATED,
        SYNTAQLITE_NO_SPAN,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        storage,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, yymsp[-2].minor.yy391, SYNTAQLITE_NULL_NODE);
    yymsp[-3].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
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
      case 308: /* wqas ::= AS MATERIALIZED */ yytestcase(yyruleno==308);
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
      case 130: /* on_using ::= */ yytestcase(yyruleno==130);
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
      case 406: /* with ::= */ yytestcase(yyruleno==406);
{
    yymsp[1].minor.yy391 = SYNTAQLITE_NULL_NODE;
}
        break;
      case 66: /* tconscomma ::= COMMA */
{ yymsp[0].minor.yy144 = 1; }
        break;
      case 68: /* tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
{
    yymsp[-6].minor.yy117.node = ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_PRIMARY_KEY,
        SYNTAQLITE_NO_SPAN,
        (uint8_t)yymsp[0].minor.yy144, (uint8_t)yymsp[-2].minor.yy144,
        yymsp[-3].minor.yy391, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-6].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 69: /* tcons ::= UNIQUE LP sortlist RP onconf */
{
    yymsp[-4].minor.yy117.node = ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_UNIQUE,
        SYNTAQLITE_NO_SPAN,
        (uint8_t)yymsp[0].minor.yy144, 0,
        yymsp[-2].minor.yy391, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-4].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 70: /* tcons ::= CHECK LP expr RP onconf */
{
    yymsp[-4].minor.yy117.node = ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_CHECK,
        SYNTAQLITE_NO_SPAN,
        (uint8_t)yymsp[0].minor.yy144, 0,
        SYNTAQLITE_NULL_NODE, yymsp[-2].minor.yy391, SYNTAQLITE_NULL_NODE);
    yymsp[-4].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 71: /* tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
{
    SyntaqliteForeignKeyAction on_del = (SyntaqliteForeignKeyAction)(yymsp[-1].minor.yy144 & 0xff);
    SyntaqliteForeignKeyAction on_upd = (SyntaqliteForeignKeyAction)((yymsp[-1].minor.yy144 >> 8) & 0xff);
    uint32_t fk = ast_foreign_key_clause(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-3].minor.yy0), yymsp[-2].minor.yy391, on_del, on_upd, (uint8_t)yymsp[0].minor.yy144);
    yymsp[-9].minor.yy117.node = ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_FOREIGN_KEY,
        SYNTAQLITE_NO_SPAN,
        0, 0,
        yymsp[-6].minor.yy391, SYNTAQLITE_NULL_NODE, fk);
    yymsp[-9].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
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
    yymsp[-3].minor.yy391 = ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TABLE, (uint8_t)yymsp[-1].minor.yy144, yymsp[0].minor.yy391);
}
        break;
      case 82: /* cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
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
      case 83: /* cmd ::= DROP VIEW ifexists fullname */
{
    yymsp[-3].minor.yy391 = ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_VIEW, (uint8_t)yymsp[-1].minor.yy144, yymsp[0].minor.yy391);
}
        break;
      case 85: /* select ::= WITH wqlist selectnowith */
{
    yymsp[-2].minor.yy391 = ast_with_clause(pCtx->astCtx, 0, yymsp[-1].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 86: /* select ::= WITH RECURSIVE wqlist selectnowith */
{
    yymsp[-3].minor.yy391 = ast_with_clause(pCtx->astCtx, 1, yymsp[-1].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 88: /* selectnowith ::= selectnowith multiselect_op oneselect */
{
    yymsp[-2].minor.yy391 = ast_compound_select(pCtx->astCtx, (uint8_t)yymsp[-1].minor.yy144, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
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
    yylhsminor.yy144 = (yymsp[0].minor.yy0.type == TK_INTERSECT) ? 2 : 3;
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 92: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
{
    yymsp[-8].minor.yy391 = ast_select_stmt(pCtx->astCtx, (SyntaqliteSelectStmtFlags){.raw = (uint8_t)yymsp[-7].minor.yy391}, yymsp[-6].minor.yy391, yymsp[-5].minor.yy391, yymsp[-4].minor.yy391, yymsp[-3].minor.yy391, yymsp[-2].minor.yy391, yymsp[-1].minor.yy391, yymsp[0].minor.yy391, SYNTAQLITE_NULL_NODE);
}
        break;
      case 93: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
{
    yymsp[-9].minor.yy391 = ast_select_stmt(pCtx->astCtx, (SyntaqliteSelectStmtFlags){.raw = (uint8_t)yymsp[-8].minor.yy391}, yymsp[-7].minor.yy391, yymsp[-6].minor.yy391, yymsp[-5].minor.yy391, yymsp[-4].minor.yy391, yymsp[-3].minor.yy391, yymsp[-1].minor.yy391, yymsp[0].minor.yy391, yymsp[-2].minor.yy391);
}
        break;
      case 94: /* values ::= VALUES LP nexprlist RP */
{
    yymsp[-3].minor.yy391 = ast_values_row_list(pCtx->astCtx, yymsp[-1].minor.yy391);
}
        break;
      case 95: /* oneselect ::= mvalues */
      case 377: /* oneselect ::= values */ yytestcase(yyruleno==377);
{
    yylhsminor.yy391 = ast_values_clause(pCtx->astCtx, yymsp[0].minor.yy391);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 96: /* mvalues ::= values COMMA LP nexprlist RP */
      case 97: /* mvalues ::= mvalues COMMA LP nexprlist RP */ yytestcase(yyruleno==97);
{
    yymsp[-4].minor.yy391 = ast_values_row_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, yymsp[-1].minor.yy391);
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
    uint32_t col = ast_result_column(pCtx->astCtx, (SyntaqliteResultColumnFlags){0}, SYNTAQLITE_NO_SPAN, yymsp[-2].minor.yy391);
    yylhsminor.yy391 = ast_result_column_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, col);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 103: /* selcollist ::= sclp scanpt STAR */
{
    uint32_t col = ast_result_column(pCtx->astCtx, (SyntaqliteResultColumnFlags){.star = 1}, SYNTAQLITE_NO_SPAN, SYNTAQLITE_NULL_NODE);
    yylhsminor.yy391 = ast_result_column_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, col);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 104: /* selcollist ::= sclp scanpt nm DOT STAR */
{
    uint32_t col = ast_result_column(pCtx->astCtx, (SyntaqliteResultColumnFlags){.star = 1}, syntaqlite_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NULL_NODE);
    yylhsminor.yy391 = ast_result_column_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, col);
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
      case 128: /* on_using ::= ON expr */ yytestcase(yyruleno==128);
      case 147: /* having_opt ::= HAVING expr */ yytestcase(yyruleno==147);
      case 154: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==154);
      case 156: /* where_opt_ret ::= WHERE expr */ yytestcase(yyruleno==156);
      case 231: /* case_else ::= ELSE expr */ yytestcase(yyruleno==231);
      case 251: /* vinto ::= INTO expr */ yytestcase(yyruleno==251);
      case 269: /* when_clause ::= WHEN expr */ yytestcase(yyruleno==269);
      case 288: /* key_opt ::= KEY expr */ yytestcase(yyruleno==288);
      case 305: /* with ::= WITH wqlist */ yytestcase(yyruleno==305);
      case 336: /* window_clause ::= WINDOW windowdefn_list */ yytestcase(yyruleno==336);
{
    yymsp[-1].minor.yy391 = yymsp[0].minor.yy391;
}
        break;
      case 109: /* stl_prefix ::= seltablist joinop */
{
    yymsp[-1].minor.yy391 = ast_join_prefix(pCtx->astCtx, yymsp[-1].minor.yy391, (SyntaqliteJoinType)yymsp[0].minor.yy144);
}
        break;
      case 111: /* seltablist ::= stl_prefix nm dbnm as on_using */
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
      case 112: /* seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
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
      case 113: /* seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
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
      case 114: /* seltablist ::= stl_prefix LP select RP as on_using */
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
      case 115: /* seltablist ::= stl_prefix LP seltablist RP as on_using */
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
      case 116: /* dbnm ::= */
{ yymsp[1].minor.yy0.z = NULL; yymsp[1].minor.yy0.n = 0; }
        break;
      case 117: /* dbnm ::= DOT nm */
{ yymsp[-1].minor.yy0 = yymsp[0].minor.yy0; }
        break;
      case 118: /* fullname ::= nm */
{
    yylhsminor.yy391 = ast_qualified_name(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        SYNTAQLITE_NO_SPAN);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 119: /* fullname ::= nm DOT nm */
{
    yylhsminor.yy391 = ast_qualified_name(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0));
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 120: /* xfullname ::= nm */
{
    yylhsminor.yy391 = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 121: /* xfullname ::= nm DOT nm */
{
    yylhsminor.yy391 = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0), syntaqlite_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NO_SPAN);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 122: /* xfullname ::= nm DOT nm AS nm */
{
    yylhsminor.yy391 = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0), syntaqlite_span(pCtx, yymsp[-4].minor.yy0), syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 123: /* xfullname ::= nm AS nm */
{
    yylhsminor.yy391 = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NO_SPAN, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 124: /* joinop ::= COMMA|JOIN */
{
    yylhsminor.yy144 = (yymsp[0].minor.yy0.type == TK_COMMA)
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
      case 129: /* on_using ::= USING LP idlist RP */
{
    yymsp[-3].minor.yy391 = yymsp[-1].minor.yy391;
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
    uint32_t term = ast_ordering_term(pCtx->astCtx, yymsp[-2].minor.yy391, (uint8_t)yymsp[-1].minor.yy391, (uint8_t)yymsp[0].minor.yy391);
    yylhsminor.yy391 = ast_order_by_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, term);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 137: /* sortlist ::= expr sortorder nulls */
{
    uint32_t term = ast_ordering_term(pCtx->astCtx, yymsp[-2].minor.yy391, (uint8_t)yymsp[-1].minor.yy391, (uint8_t)yymsp[0].minor.yy391);
    yylhsminor.yy391 = ast_order_by_list(pCtx->astCtx, term);
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
    yymsp[-1].minor.yy391 = ast_limit_clause(pCtx->astCtx, yymsp[0].minor.yy391, SYNTAQLITE_NULL_NODE);
}
        break;
      case 150: /* limit_opt ::= LIMIT expr OFFSET expr */
{
    yymsp[-3].minor.yy391 = ast_limit_clause(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 151: /* limit_opt ::= LIMIT expr COMMA expr */
{
    yymsp[-3].minor.yy391 = ast_limit_clause(pCtx->astCtx, yymsp[0].minor.yy391, yymsp[-2].minor.yy391);
}
        break;
      case 152: /* cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
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
      case 160: /* setlist ::= setlist COMMA nm EQ expr */
{
    uint32_t clause = ast_set_clause(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy391);
    yylhsminor.yy391 = ast_set_clause_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, clause);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 161: /* setlist ::= setlist COMMA LP idlist RP EQ expr */
{
    uint32_t clause = ast_set_clause(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, yymsp[-3].minor.yy391, yymsp[0].minor.yy391);
    yylhsminor.yy391 = ast_set_clause_list_append(pCtx->astCtx, yymsp[-6].minor.yy391, clause);
}
  yymsp[-6].minor.yy391 = yylhsminor.yy391;
        break;
      case 162: /* setlist ::= nm EQ expr */
{
    uint32_t clause = ast_set_clause(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0), SYNTAQLITE_NULL_NODE, yymsp[0].minor.yy391);
    yylhsminor.yy391 = ast_set_clause_list(pCtx->astCtx, clause);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 163: /* setlist ::= LP idlist RP EQ expr */
{
    uint32_t clause = ast_set_clause(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, yymsp[-3].minor.yy391, yymsp[0].minor.yy391);
    yymsp[-4].minor.yy391 = ast_set_clause_list(pCtx->astCtx, clause);
}
        break;
      case 164: /* cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
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
      case 165: /* cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
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
    uint32_t col = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
    yymsp[-2].minor.yy391 = ast_expr_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, col);
}
        break;
      case 178: /* idlist ::= nm */
{
    uint32_t col = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
    yylhsminor.yy391 = ast_expr_list(pCtx->astCtx, col);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 180: /* expr ::= ID|INDEXED|JOIN_KW */
{
    yylhsminor.yy391 = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NO_SPAN);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 181: /* expr ::= nm DOT nm */
{
    yylhsminor.yy391 = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0),
        SYNTAQLITE_NO_SPAN);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 182: /* expr ::= nm DOT nm DOT nm */
{
    yylhsminor.yy391 = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-4].minor.yy0));
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 183: /* term ::= NULL|FLOAT|BLOB */
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
      case 184: /* term ::= STRING */
{
    yylhsminor.yy391 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_STRING, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 185: /* term ::= INTEGER */
{
    yylhsminor.yy391 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_INTEGER, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 186: /* expr ::= VARIABLE */
{
    yylhsminor.yy391 = ast_variable(pCtx->astCtx, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 187: /* expr ::= expr COLLATE ID|STRING */
{
    yylhsminor.yy391 = ast_collate_expr(pCtx->astCtx, yymsp[-2].minor.yy391, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 188: /* expr ::= CAST LP expr AS typetoken RP */
{
    yymsp[-5].minor.yy391 = ast_cast_expr(pCtx->astCtx, yymsp[-3].minor.yy391, syntaqlite_span(pCtx, yymsp[-1].minor.yy0));
}
        break;
      case 189: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
{
    yylhsminor.yy391 = ast_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-4].minor.yy0),
        (SyntaqliteFunctionCallFlags){.raw = (uint8_t)yymsp[-2].minor.yy391},
        yymsp[-1].minor.yy391,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 190: /* expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
{
    yylhsminor.yy391 = ast_aggregate_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-7].minor.yy0),
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
    yylhsminor.yy391 = ast_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-3].minor.yy0),
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
        (pCtx->astCtx->ast->arena + pCtx->astCtx->ast->offsets[yymsp[0].minor.yy391]);
    yylhsminor.yy391 = ast_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-5].minor.yy0),
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
        (pCtx->astCtx->ast->arena + pCtx->astCtx->ast->offsets[yymsp[0].minor.yy391]);
    yylhsminor.yy391 = ast_aggregate_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-8].minor.yy0),
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
        (pCtx->astCtx->ast->arena + pCtx->astCtx->ast->offsets[yymsp[0].minor.yy391]);
    yylhsminor.yy391 = ast_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-4].minor.yy0),
        (SyntaqliteFunctionCallFlags){.star = 1},
        SYNTAQLITE_NULL_NODE,
        fo->filter_expr,
        fo->over_def);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 195: /* term ::= CTIME_KW */
{
    yylhsminor.yy391 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_CURRENT, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 196: /* expr ::= LP nexprlist COMMA expr RP */
{
    yymsp[-4].minor.yy391 = ast_expr_list_append(pCtx->astCtx, yymsp[-3].minor.yy391, yymsp[-1].minor.yy391);
}
        break;
      case 197: /* expr ::= expr AND expr */
{
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_AND, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 198: /* expr ::= expr OR expr */
{
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_OR, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 199: /* expr ::= expr LT|GT|GE|LE expr */
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
      case 200: /* expr ::= expr EQ|NE expr */
{
    SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == TK_EQ) ? SYNTAQLITE_BINARY_OP_EQ : SYNTAQLITE_BINARY_OP_NE;
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 201: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
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
      case 202: /* expr ::= expr PLUS|MINUS expr */
{
    SyntaqliteBinaryOp op = (yymsp[-1].minor.yy0.type == TK_PLUS) ? SYNTAQLITE_BINARY_OP_PLUS : SYNTAQLITE_BINARY_OP_MINUS;
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, op, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 203: /* expr ::= expr STAR|SLASH|REM expr */
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
      case 204: /* expr ::= expr CONCAT expr */
{
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_CONCAT, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
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
    uint8_t negated = (yymsp[-1].minor.yy0.n & 0x80000000) ? 1 : 0;
    yylhsminor.yy391 = ast_like_expr(pCtx->astCtx, negated, yymsp[-2].minor.yy391, yymsp[0].minor.yy391, SYNTAQLITE_NULL_NODE);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 207: /* expr ::= expr likeop expr ESCAPE expr */
{
    uint8_t negated = (yymsp[-3].minor.yy0.n & 0x80000000) ? 1 : 0;
    yylhsminor.yy391 = ast_like_expr(pCtx->astCtx, negated, yymsp[-4].minor.yy391, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 208: /* expr ::= expr ISNULL|NOTNULL */
{
    SyntaqliteIsOp op = (yymsp[0].minor.yy0.type == TK_ISNULL) ? SYNTAQLITE_IS_OP_ISNULL : SYNTAQLITE_IS_OP_NOTNULL;
    yylhsminor.yy391 = ast_is_expr(pCtx->astCtx, op, yymsp[-1].minor.yy391, SYNTAQLITE_NULL_NODE);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 209: /* expr ::= expr NOT NULL */
{
    yylhsminor.yy391 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_NOTNULL, yymsp[-2].minor.yy391, SYNTAQLITE_NULL_NODE);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 210: /* expr ::= expr IS expr */
{
    yylhsminor.yy391 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 211: /* expr ::= expr IS NOT expr */
{
    yylhsminor.yy391 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_NOT, yymsp[-3].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-3].minor.yy391 = yylhsminor.yy391;
        break;
      case 212: /* expr ::= expr IS NOT DISTINCT FROM expr */
{
    yylhsminor.yy391 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_NOT_DISTINCT, yymsp[-5].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-5].minor.yy391 = yylhsminor.yy391;
        break;
      case 213: /* expr ::= expr IS DISTINCT FROM expr */
{
    yylhsminor.yy391 = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_DISTINCT, yymsp[-4].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 214: /* expr ::= NOT expr */
{
    yymsp[-1].minor.yy391 = ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_NOT, yymsp[0].minor.yy391);
}
        break;
      case 215: /* expr ::= BITNOT expr */
{
    yymsp[-1].minor.yy391 = ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_BITNOT, yymsp[0].minor.yy391);
}
        break;
      case 216: /* expr ::= PLUS|MINUS expr */
{
    SyntaqliteUnaryOp op = (yymsp[-1].minor.yy0.type == TK_MINUS) ? SYNTAQLITE_UNARY_OP_MINUS : SYNTAQLITE_UNARY_OP_PLUS;
    yylhsminor.yy391 = ast_unary_expr(pCtx->astCtx, op, yymsp[0].minor.yy391);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 217: /* expr ::= expr PTR expr */
{
    yylhsminor.yy391 = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_PTR, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 220: /* expr ::= expr between_op expr AND expr */
{
    yylhsminor.yy391 = ast_between_expr(pCtx->astCtx, (uint8_t)yymsp[-3].minor.yy391, yymsp[-4].minor.yy391, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 221: /* in_op ::= IN */
{ yymsp[0].minor.yy144 = 0; }
        break;
      case 223: /* expr ::= expr in_op LP exprlist RP */
{
    yymsp[-4].minor.yy391 = ast_in_expr(pCtx->astCtx, (uint8_t)yymsp[-3].minor.yy144, yymsp[-4].minor.yy391, yymsp[-1].minor.yy391);
}
        break;
      case 224: /* expr ::= LP select RP */
{
    yymsp[-2].minor.yy391 = ast_subquery_expr(pCtx->astCtx, yymsp[-1].minor.yy391);
}
        break;
      case 225: /* expr ::= expr in_op LP select RP */
{
    uint32_t sub = ast_subquery_expr(pCtx->astCtx, yymsp[-1].minor.yy391);
    yymsp[-4].minor.yy391 = ast_in_expr(pCtx->astCtx, (uint8_t)yymsp[-3].minor.yy144, yymsp[-4].minor.yy391, sub);
}
        break;
      case 226: /* expr ::= expr in_op nm dbnm paren_exprlist */
{
    // Table-valued function IN expression - stub for now
    (void)yymsp[-2].minor.yy0; (void)yymsp[-1].minor.yy0; (void)yymsp[0].minor.yy391;
    yymsp[-4].minor.yy391 = ast_in_expr(pCtx->astCtx, (uint8_t)yymsp[-3].minor.yy144, yymsp[-4].minor.yy391, SYNTAQLITE_NULL_NODE);
}
        break;
      case 227: /* expr ::= EXISTS LP select RP */
{
    yymsp[-3].minor.yy391 = ast_exists_expr(pCtx->astCtx, yymsp[-1].minor.yy391);
}
        break;
      case 228: /* expr ::= CASE case_operand case_exprlist case_else END */
{
    yymsp[-4].minor.yy391 = ast_case_expr(pCtx->astCtx, yymsp[-3].minor.yy391, yymsp[-1].minor.yy391, yymsp[-2].minor.yy391);
}
        break;
      case 229: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
{
    uint32_t w = ast_case_when(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
    yylhsminor.yy391 = ast_case_when_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, w);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 230: /* case_exprlist ::= WHEN expr THEN expr */
{
    uint32_t w = ast_case_when(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
    yymsp[-3].minor.yy391 = ast_case_when_list(pCtx->astCtx, w);
}
        break;
      case 235: /* nexprlist ::= nexprlist COMMA expr */
{
    yylhsminor.yy391 = ast_expr_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 236: /* nexprlist ::= expr */
{
    yylhsminor.yy391 = ast_expr_list(pCtx->astCtx, yymsp[0].minor.yy391);
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
      case 244: /* eidlist ::= eidlist COMMA nm collate sortorder */
{
    (void)yymsp[-1].minor.yy144; (void)yymsp[0].minor.yy391;
    uint32_t col = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0),
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NO_SPAN);
    yymsp[-4].minor.yy391 = ast_expr_list_append(pCtx->astCtx, yymsp[-4].minor.yy391, col);
}
        break;
      case 245: /* eidlist ::= nm collate sortorder */
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
      case 248: /* cmd ::= DROP INDEX ifexists fullname */
{
    yymsp[-3].minor.yy391 = ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_INDEX, (uint8_t)yymsp[-1].minor.yy144, yymsp[0].minor.yy391);
}
        break;
      case 249: /* cmd ::= VACUUM vinto */
{
    yymsp[-1].minor.yy391 = ast_vacuum_stmt(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN,
        yymsp[0].minor.yy391);
}
        break;
      case 250: /* cmd ::= VACUUM nm vinto */
{
    yymsp[-2].minor.yy391 = ast_vacuum_stmt(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-1].minor.yy0),
        yymsp[0].minor.yy391);
}
        break;
      case 253: /* cmd ::= PRAGMA nm dbnm */
{
    SyntaqliteSourceSpan name_span = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[0].minor.yy0) : syntaqlite_span(pCtx, yymsp[-1].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-1].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-2].minor.yy391 = ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, SYNTAQLITE_NO_SPAN, 0);
}
        break;
      case 254: /* cmd ::= PRAGMA nm dbnm EQ nmnum */
      case 256: /* cmd ::= PRAGMA nm dbnm EQ minus_num */ yytestcase(yyruleno==256);
{
    SyntaqliteSourceSpan name_span = yymsp[-2].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-2].minor.yy0) : syntaqlite_span(pCtx, yymsp[-3].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[-2].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-3].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-4].minor.yy391 = ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, syntaqlite_span(pCtx, yymsp[0].minor.yy0), 1);
}
        break;
      case 255: /* cmd ::= PRAGMA nm dbnm LP nmnum RP */
      case 257: /* cmd ::= PRAGMA nm dbnm LP minus_num RP */ yytestcase(yyruleno==257);
{
    SyntaqliteSourceSpan name_span = yymsp[-3].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-3].minor.yy0) : syntaqlite_span(pCtx, yymsp[-4].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[-3].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-4].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-5].minor.yy391 = ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, syntaqlite_span(pCtx, yymsp[-1].minor.yy0), 2);
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
    SyntaqliteNode *trig = AST_NODE(pCtx->astCtx->ast, yymsp[-3].minor.yy391);
    trig->create_trigger_stmt.body = yymsp[-1].minor.yy391;
    yymsp[-4].minor.yy391 = yymsp[-3].minor.yy391;
}
        break;
      case 261: /* trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
{
    SyntaqliteSourceSpan trig_name = yymsp[-6].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-6].minor.yy0) : syntaqlite_span(pCtx, yymsp[-7].minor.yy0);
    SyntaqliteSourceSpan trig_schema = yymsp[-6].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-7].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yylhsminor.yy391 = ast_create_trigger_stmt(pCtx->astCtx,
        trig_name,
        trig_schema,
        (uint8_t)yymsp[-10].minor.yy144,
        (uint8_t)yymsp[-8].minor.yy144,
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
    yylhsminor.yy144 = (yymsp[0].minor.yy0.type == TK_BEFORE) ? (int)SYNTAQLITE_TRIGGER_TIMING_BEFORE
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
    SyntaqliteTriggerEventType evt = (yymsp[0].minor.yy0.type == TK_DELETE)
        ? SYNTAQLITE_TRIGGER_EVENT_TYPE_DELETE
        : SYNTAQLITE_TRIGGER_EVENT_TYPE_INSERT;
    yylhsminor.yy391 = ast_trigger_event(pCtx->astCtx, evt, SYNTAQLITE_NULL_NODE);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 266: /* trigger_event ::= UPDATE */
{
    yymsp[0].minor.yy391 = ast_trigger_event(pCtx->astCtx,
        SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE, SYNTAQLITE_NULL_NODE);
}
        break;
      case 267: /* trigger_event ::= UPDATE OF idlist */
{
    yymsp[-2].minor.yy391 = ast_trigger_event(pCtx->astCtx,
        SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE, yymsp[0].minor.yy391);
}
        break;
      case 270: /* trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
{
    yylhsminor.yy391 = ast_trigger_cmd_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[-1].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 271: /* trigger_cmd_list ::= trigger_cmd SEMI */
{
    yylhsminor.yy391 = ast_trigger_cmd_list(pCtx->astCtx, yymsp[-1].minor.yy391);
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
    uint32_t tbl = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-6].minor.yy0), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
    yymsp[-8].minor.yy391 = ast_update_stmt(pCtx->astCtx, (SyntaqliteConflictAction)yymsp[-7].minor.yy144, tbl, yymsp[-3].minor.yy391, yymsp[-2].minor.yy391, yymsp[-1].minor.yy391);
}
        break;
      case 276: /* trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
{
    uint32_t tbl = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-4].minor.yy0), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
    yymsp[-7].minor.yy391 = ast_insert_stmt(pCtx->astCtx, (SyntaqliteConflictAction)yymsp[-6].minor.yy144, tbl, yymsp[-3].minor.yy391, yymsp[-2].minor.yy391);
}
        break;
      case 277: /* trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
{
    uint32_t tbl = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-3].minor.yy0), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
    yymsp[-5].minor.yy391 = ast_delete_stmt(pCtx->astCtx, tbl, yymsp[-1].minor.yy391);
}
        break;
      case 279: /* expr ::= RAISE LP IGNORE RP */
{
    yymsp[-3].minor.yy391 = ast_raise_expr(pCtx->astCtx, SYNTAQLITE_RAISE_TYPE_IGNORE, SYNTAQLITE_NULL_NODE);
}
        break;
      case 280: /* expr ::= RAISE LP raisetype COMMA expr RP */
{
    yymsp[-5].minor.yy391 = ast_raise_expr(pCtx->astCtx, (SyntaqliteRaiseType)yymsp[-3].minor.yy144, yymsp[-1].minor.yy391);
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
    yymsp[-3].minor.yy391 = ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TRIGGER, (uint8_t)yymsp[-1].minor.yy144, yymsp[0].minor.yy391);
}
        break;
      case 285: /* cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
{
    yymsp[-5].minor.yy391 = ast_attach_stmt(pCtx->astCtx, yymsp[-3].minor.yy391, yymsp[-1].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 286: /* cmd ::= DETACH database_kw_opt expr */
{
    yymsp[-2].minor.yy391 = ast_detach_stmt(pCtx->astCtx, yymsp[0].minor.yy391);
}
        break;
      case 289: /* cmd ::= REINDEX */
{
    yymsp[0].minor.yy391 = ast_analyze_stmt(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NO_SPAN,
        1);
}
        break;
      case 290: /* cmd ::= REINDEX nm dbnm */
{
    SyntaqliteSourceSpan name_span = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[0].minor.yy0) : syntaqlite_span(pCtx, yymsp[-1].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-1].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-2].minor.yy391 = ast_analyze_stmt(pCtx->astCtx, name_span, schema_span, 1);
}
        break;
      case 291: /* cmd ::= ANALYZE */
{
    yymsp[0].minor.yy391 = ast_analyze_stmt(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NO_SPAN,
        0);
}
        break;
      case 292: /* cmd ::= ANALYZE nm dbnm */
{
    SyntaqliteSourceSpan name_span = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[0].minor.yy0) : syntaqlite_span(pCtx, yymsp[-1].minor.yy0);
    SyntaqliteSourceSpan schema_span = yymsp[0].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-1].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-2].minor.yy391 = ast_analyze_stmt(pCtx->astCtx, name_span, schema_span, 0);
}
        break;
      case 293: /* cmd ::= ALTER TABLE fullname RENAME TO nm */
{
    yymsp[-5].minor.yy391 = ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_RENAME_TABLE, yymsp[-3].minor.yy391,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        SYNTAQLITE_NO_SPAN);
}
        break;
      case 294: /* cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
{
    yymsp[-6].minor.yy391 = ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_ADD_COLUMN, SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NO_SPAN,
        yymsp[-1].minor.yy487.name);
}
        break;
      case 295: /* cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
{
    yymsp[-5].minor.yy391 = ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_DROP_COLUMN, yymsp[-3].minor.yy391,
        SYNTAQLITE_NO_SPAN,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0));
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
    yymsp[-7].minor.yy391 = ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_RENAME_COLUMN, yymsp[-5].minor.yy391,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        syntaqlite_span(pCtx, yymsp[-2].minor.yy0));
}
        break;
      case 299: /* cmd ::= create_vtab LP vtabarglist RP */
{
    // Capture module arguments span (content between parens)
    SyntaqliteNode *vtab = AST_NODE(pCtx->astCtx->ast, yymsp[-3].minor.yy391);
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
    SyntaqliteSourceSpan tbl_name = yymsp[-2].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-2].minor.yy0) : syntaqlite_span(pCtx, yymsp[-3].minor.yy0);
    SyntaqliteSourceSpan tbl_schema = yymsp[-2].minor.yy0.z ? syntaqlite_span(pCtx, yymsp[-3].minor.yy0) : SYNTAQLITE_NO_SPAN;
    yymsp[-7].minor.yy391 = ast_create_virtual_table_stmt(pCtx->astCtx,
        tbl_name,
        tbl_schema,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        (uint8_t)yymsp[-4].minor.yy144,
        SYNTAQLITE_NO_SPAN);  // module_args = none by default
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
      case 306: /* with ::= WITH RECURSIVE wqlist */
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
      case 307: /* wqas ::= AS */
      case 351: /* trans_opt ::= TRANSACTION */ yytestcase(yyruleno==351);
      case 353: /* savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==353);
{
    yymsp[0].minor.yy144 = 0;
}
        break;
      case 310: /* wqitem ::= withnm eidlist_opt wqas LP select RP */
{
    yylhsminor.yy391 = ast_cte_definition(pCtx->astCtx, syntaqlite_span(pCtx, yymsp[-5].minor.yy0), (uint8_t)yymsp[-3].minor.yy144, yymsp[-4].minor.yy391, yymsp[-1].minor.yy391);
}
  yymsp[-5].minor.yy391 = yylhsminor.yy391;
        break;
      case 311: /* withnm ::= nm */
{
    // Token passthrough - nm already produces SyntaqliteToken
}
        break;
      case 312: /* wqlist ::= wqitem */
{
    yylhsminor.yy391 = ast_cte_list(pCtx->astCtx, yymsp[0].minor.yy391);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 313: /* wqlist ::= wqlist COMMA wqitem */
{
    yymsp[-2].minor.yy391 = ast_cte_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
        break;
      case 314: /* windowdefn_list ::= windowdefn_list COMMA windowdefn */
{
    yylhsminor.yy391 = ast_named_window_def_list_append(pCtx->astCtx, yymsp[-2].minor.yy391, yymsp[0].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 315: /* windowdefn ::= nm AS LP window RP */
{
    yylhsminor.yy391 = ast_named_window_def(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-4].minor.yy0),
        yymsp[-1].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 316: /* window ::= PARTITION BY nexprlist orderby_opt frame_opt */
{
    yymsp[-4].minor.yy391 = ast_window_def(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN,
        yymsp[-2].minor.yy391,
        yymsp[-1].minor.yy391,
        yymsp[0].minor.yy391);
}
        break;
      case 317: /* window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
{
    yylhsminor.yy391 = ast_window_def(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-5].minor.yy0),
        yymsp[-2].minor.yy391,
        yymsp[-1].minor.yy391,
        yymsp[0].minor.yy391);
}
  yymsp[-5].minor.yy391 = yylhsminor.yy391;
        break;
      case 318: /* window ::= ORDER BY sortlist frame_opt */
{
    yymsp[-3].minor.yy391 = ast_window_def(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NULL_NODE,
        yymsp[-1].minor.yy391,
        yymsp[0].minor.yy391);
}
        break;
      case 319: /* window ::= nm ORDER BY sortlist frame_opt */
{
    yylhsminor.yy391 = ast_window_def(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-4].minor.yy0),
        SYNTAQLITE_NULL_NODE,
        yymsp[-1].minor.yy391,
        yymsp[0].minor.yy391);
}
  yymsp[-4].minor.yy391 = yylhsminor.yy391;
        break;
      case 320: /* window ::= nm frame_opt */
{
    yylhsminor.yy391 = ast_window_def(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[-1].minor.yy0),
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE,
        yymsp[0].minor.yy391);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 322: /* frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
{
    // Single bound: start=yymsp[-1].minor.yy391, end=CURRENT ROW (implicit)
    uint32_t end_bound = ast_frame_bound(pCtx->astCtx,
        SYNTAQLITE_FRAME_BOUND_TYPE_CURRENT_ROW,
        SYNTAQLITE_NULL_NODE);
    yylhsminor.yy391 = ast_frame_spec(pCtx->astCtx,
        (uint8_t)yymsp[-2].minor.yy144,
        (uint8_t)yymsp[0].minor.yy144,
        yymsp[-1].minor.yy391,
        end_bound);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 323: /* frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
{
    yylhsminor.yy391 = ast_frame_spec(pCtx->astCtx,
        (uint8_t)yymsp[-5].minor.yy144,
        (uint8_t)yymsp[0].minor.yy144,
        yymsp[-3].minor.yy391,
        yymsp[-1].minor.yy391);
}
  yymsp[-5].minor.yy391 = yylhsminor.yy391;
        break;
      case 324: /* range_or_rows ::= RANGE|ROWS|GROUPS */
{
    switch (yymsp[0].minor.yy0.type) {
        case TK_RANGE:  yylhsminor.yy144 = SYNTAQLITE_FRAME_TYPE_RANGE; break;
        case TK_ROWS:   yylhsminor.yy144 = SYNTAQLITE_FRAME_TYPE_ROWS; break;
        default:        yylhsminor.yy144 = SYNTAQLITE_FRAME_TYPE_GROUPS; break;
    }
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 326: /* frame_bound_s ::= UNBOUNDED PRECEDING */
{
    yymsp[-1].minor.yy391 = ast_frame_bound(pCtx->astCtx,
        SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_PRECEDING,
        SYNTAQLITE_NULL_NODE);
}
        break;
      case 328: /* frame_bound_e ::= UNBOUNDED FOLLOWING */
{
    yymsp[-1].minor.yy391 = ast_frame_bound(pCtx->astCtx,
        SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_FOLLOWING,
        SYNTAQLITE_NULL_NODE);
}
        break;
      case 329: /* frame_bound ::= expr PRECEDING|FOLLOWING */
{
    SyntaqliteFrameBoundType bt = (yymsp[0].minor.yy0.type == TK_PRECEDING)
        ? SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_PRECEDING
        : SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_FOLLOWING;
    yylhsminor.yy391 = ast_frame_bound(pCtx->astCtx, bt, yymsp[-1].minor.yy391);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 330: /* frame_bound ::= CURRENT ROW */
{
    yymsp[-1].minor.yy391 = ast_frame_bound(pCtx->astCtx,
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
    yylhsminor.yy144 = (yymsp[0].minor.yy0.type == TK_GROUP)
        ? SYNTAQLITE_FRAME_EXCLUDE_GROUP
        : SYNTAQLITE_FRAME_EXCLUDE_TIES;
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
        break;
      case 337: /* filter_over ::= filter_clause over_clause */
{
    // Unpack the over_clause FilterOver to combine with filter expr
    SyntaqliteFilterOver *fo_over = (SyntaqliteFilterOver*)
        (pCtx->astCtx->ast->arena + pCtx->astCtx->ast->offsets[yymsp[0].minor.yy391]);
    yylhsminor.yy391 = ast_filter_over(pCtx->astCtx,
        yymsp[-1].minor.yy391,
        fo_over->over_def,
        SYNTAQLITE_NO_SPAN);
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 339: /* filter_over ::= filter_clause */
{
    yylhsminor.yy391 = ast_filter_over(pCtx->astCtx,
        yymsp[0].minor.yy391,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NO_SPAN);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 340: /* over_clause ::= OVER LP window RP */
{
    yymsp[-3].minor.yy391 = ast_filter_over(pCtx->astCtx,
        SYNTAQLITE_NULL_NODE,
        yymsp[-1].minor.yy391,
        SYNTAQLITE_NO_SPAN);
}
        break;
      case 341: /* over_clause ::= OVER nm */
{
    // Create a WindowDef with just base_window_name to represent a named window ref
    uint32_t wdef = ast_window_def(pCtx->astCtx,
        syntaqlite_span(pCtx, yymsp[0].minor.yy0),
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE);
    yymsp[-1].minor.yy391 = ast_filter_over(pCtx->astCtx,
        SYNTAQLITE_NULL_NODE,
        wdef,
        SYNTAQLITE_NO_SPAN);
}
        break;
      case 342: /* filter_clause ::= FILTER LP WHERE expr RP */
{
    yymsp[-4].minor.yy391 = yymsp[-1].minor.yy391;
}
        break;
      case 343: /* term ::= QNUMBER */
{
    yylhsminor.yy391 = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_QNUMBER, syntaqlite_span(pCtx, yymsp[0].minor.yy0));
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
}
        break;
      case 348: /* ecmd ::= cmdx SEMI */
      case 378: /* sclp ::= selcollist COMMA */ yytestcase(yyruleno==378);
{
    yylhsminor.yy391 = yymsp[-1].minor.yy391;
}
  yymsp[-1].minor.yy391 = yylhsminor.yy391;
        break;
      case 349: /* ecmd ::= explain cmdx SEMI */
{
    yylhsminor.yy391 = ast_explain_stmt(pCtx->astCtx, (uint8_t)(yymsp[-2].minor.yy144 - 1), yymsp[-1].minor.yy391);
}
  yymsp[-2].minor.yy391 = yylhsminor.yy391;
        break;
      case 355: /* cmd ::= create_table create_table_args */
{
    // yymsp[0].minor.yy391 is either: (1) a CreateTableStmt node with columns/constraints filled in
    // or: (2) a CreateTableStmt node with as_select filled in
    // yymsp[-1].minor.yy391 has the table name/schema/temp/ifnotexists info packed as a node.
    // We need to merge yymsp[-1].minor.yy391 info into yymsp[0].minor.yy391.
    SyntaqliteNode *ct_node = AST_NODE(pCtx->astCtx->ast, yymsp[-1].minor.yy391);
    SyntaqliteNode *args_node = AST_NODE(pCtx->astCtx->ast, yymsp[0].minor.yy391);
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
    uint32_t col = ast_column_def(pCtx->astCtx, yymsp[-1].minor.yy487.name, yymsp[-1].minor.yy487.typetoken, yymsp[0].minor.yy637.list);
    yylhsminor.yy391 = ast_column_def_list_append(pCtx->astCtx, yymsp[-3].minor.yy391, col);
}
  yymsp[-3].minor.yy391 = yylhsminor.yy391;
        break;
      case 358: /* columnlist ::= columnname carglist */
{
    uint32_t col = ast_column_def(pCtx->astCtx, yymsp[-1].minor.yy487.name, yymsp[-1].minor.yy487.typetoken, yymsp[0].minor.yy637.list);
    yylhsminor.yy391 = ast_column_def_list(pCtx->astCtx, col);
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
    if (yymsp[0].minor.yy117.node != SYNTAQLITE_NULL_NODE) {
        // Apply pending constraint name from the list to this node
        SyntaqliteNode *node = AST_NODE(pCtx->astCtx->ast, yymsp[0].minor.yy117.node);
        node->column_constraint.constraint_name = yymsp[-1].minor.yy637.pending_name;
        if (yymsp[-1].minor.yy637.list == SYNTAQLITE_NULL_NODE) {
            yylhsminor.yy637.list = ast_column_constraint_list(pCtx->astCtx, yymsp[0].minor.yy117.node);
        } else {
            yylhsminor.yy637.list = ast_column_constraint_list_append(pCtx->astCtx, yymsp[-1].minor.yy637.list, yymsp[0].minor.yy117.node);
        }
        yylhsminor.yy637.pending_name = SYNTAQLITE_NO_SPAN;
    } else if (yymsp[0].minor.yy117.pending_name.length > 0) {
        // CONSTRAINT nm — store pending name for next constraint
        yylhsminor.yy637.list = yymsp[-1].minor.yy637.list;
        yylhsminor.yy637.pending_name = yymsp[0].minor.yy117.pending_name;
    } else {
        yylhsminor.yy637 = yymsp[-1].minor.yy637;
    }
}
  yymsp[-1].minor.yy637 = yylhsminor.yy637;
        break;
      case 366: /* carglist ::= */
{
    yymsp[1].minor.yy637.list = SYNTAQLITE_NULL_NODE;
    yymsp[1].minor.yy637.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 367: /* ccons ::= NULL onconf */
{
    yymsp[-1].minor.yy117.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NULL,
        SYNTAQLITE_NO_SPAN,
        (uint8_t)yymsp[0].minor.yy144, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    yymsp[-1].minor.yy117.pending_name = SYNTAQLITE_NO_SPAN;
}
        break;
      case 368: /* ccons ::= GENERATED ALWAYS AS generated */
{
    yymsp[-3].minor.yy117 = yymsp[0].minor.yy117;
}
        break;
      case 369: /* ccons ::= AS generated */
{
    yymsp[-1].minor.yy117 = yymsp[0].minor.yy117;
}
        break;
      case 370: /* conslist_opt ::= COMMA conslist */
{
    yymsp[-1].minor.yy391 = yymsp[0].minor.yy637.list;
}
        break;
      case 371: /* conslist ::= conslist tconscomma tcons */
{
    // If comma separator was present, clear pending constraint name
    SyntaqliteSourceSpan pending = yymsp[-1].minor.yy144 ? SYNTAQLITE_NO_SPAN : yymsp[-2].minor.yy637.pending_name;
    if (yymsp[0].minor.yy117.node != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *node = AST_NODE(pCtx->astCtx->ast, yymsp[0].minor.yy117.node);
        node->table_constraint.constraint_name = pending;
        if (yymsp[-2].minor.yy637.list == SYNTAQLITE_NULL_NODE) {
            yylhsminor.yy637.list = ast_table_constraint_list(pCtx->astCtx, yymsp[0].minor.yy117.node);
        } else {
            yylhsminor.yy637.list = ast_table_constraint_list_append(pCtx->astCtx, yymsp[-2].minor.yy637.list, yymsp[0].minor.yy117.node);
        }
        yylhsminor.yy637.pending_name = SYNTAQLITE_NO_SPAN;
    } else if (yymsp[0].minor.yy117.pending_name.length > 0) {
        yylhsminor.yy637.list = yymsp[-2].minor.yy637.list;
        yylhsminor.yy637.pending_name = yymsp[0].minor.yy117.pending_name;
    } else {
        yylhsminor.yy637 = yymsp[-2].minor.yy637;
    }
}
  yymsp[-2].minor.yy637 = yylhsminor.yy637;
        break;
      case 372: /* conslist ::= tcons */
{
    if (yymsp[0].minor.yy117.node != SYNTAQLITE_NULL_NODE) {
        yylhsminor.yy637.list = ast_table_constraint_list(pCtx->astCtx, yymsp[0].minor.yy117.node);
        yylhsminor.yy637.pending_name = SYNTAQLITE_NO_SPAN;
    } else {
        yylhsminor.yy637.list = SYNTAQLITE_NULL_NODE;
        yylhsminor.yy637.pending_name = yymsp[0].minor.yy117.pending_name;
    }
}
  yymsp[0].minor.yy637 = yylhsminor.yy637;
        break;
      case 373: /* tconscomma ::= */
{ yymsp[1].minor.yy144 = 0; }
        break;
      case 375: /* resolvetype ::= raisetype */
{
    // raisetype: ROLLBACK=1, ABORT=2, FAIL=3 (SyntaqliteRaiseType enum values)
    // ConflictAction: ROLLBACK=1, ABORT=2, FAIL=3 (same values, direct passthrough)
    yylhsminor.yy144 = yymsp[0].minor.yy144;
}
  yymsp[0].minor.yy144 = yylhsminor.yy144;
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
      case 407: /* windowdefn_list ::= windowdefn */
{
    yylhsminor.yy391 = ast_named_window_def_list(pCtx->astCtx, yymsp[0].minor.yy391);
}
  yymsp[0].minor.yy391 = yylhsminor.yy391;
        break;
      case 408: /* window ::= frame_opt */
{
    yylhsminor.yy391 = ast_window_def(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN,
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
