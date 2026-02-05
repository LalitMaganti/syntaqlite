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
#define YYNRULE_WITH_ACTION  0
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
 /*     0 */   464,  464,  130,  127,  234, 1622,  295,  161,  574,  462,
 /*    10 */   283,  574,  283, 1622, 1290,  464,  344,  429,  574,  462,
 /*    20 */   212,  462,  565,  574,  571,  378, 1542,  212,   19,   19,
 /*    30 */   574,   51,   51,  399,  574,  326,    6,  413,   51,   51,
 /*    40 */   398,  557,  421,   51,   51,  534,  290, 1105,  290,  983,
 /*    50 */    84,   84, 1279,  293,   60,   60,  379,  984,  565, 1293,
 /*    60 */   571,  137,  139,   91, 1279, 1228, 1228, 1063, 1066, 1053,
 /*    70 */  1053,  521,  290,  254,  290,  413,  290, 1292,  290,  130,
 /*    80 */   127,  234,  346, 1107,  565,  556,  571, 1107,  565,  535,
 /*    90 */   571,   22,   22,  478,  558,  413,  574, 1628,  522,  137,
 /*   100 */   139,   91, 1506, 1228, 1228, 1063, 1066, 1053, 1053,  466,
 /*   110 */   130,  127,  234,  511,  452,  506,   84,   84,  532,  137,
 /*   120 */   139,   91,  452, 1228, 1228, 1063, 1066, 1053, 1053,  290,
 /*   130 */   452,  290,  376,  376,  532,  452, 1174,  377, 1174,  140,
 /*   140 */   377,  565,  547,  571,   44,  547,  264,  230,  233,  293,
 /*   150 */   539,  528,  452,  490,  361,  135,  135,  136,  136,  136,
 /*   160 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   170 */   131,  128,  452,  133,  133,  132,  132,  132,  131,  128,
 /*   180 */   478,  477,  478,  132,  132,  132,  131,  128,  321,  575,
 /*   190 */   452,  321,  575,  135,  135,  136,  136,  136,  136,  134,
 /*   200 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   210 */   501, 1204, 1204,  135,  135,  136,  136,  136,  136,  134,
 /*   220 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   230 */   157,  413,  574, 1571,  110, 1331,  349,  134,  134,  134,
 /*   240 */   134,  133,  133,  132,  132,  132,  131,  128,  533,  533,
 /*   250 */  1019,  413,   84,   84, 1333,  137,  139,   91,   44, 1228,
 /*   260 */  1228, 1063, 1066, 1053, 1053,  574,  351,  299, 1204,  198,
 /*   270 */   284,  573, 1205,   97,  372,  137,  139,   91,  373, 1228,
 /*   280 */  1228, 1063, 1066, 1053, 1053,   19,   19,  556,  372,  413,
 /*   290 */   850,  359,  851, 1169,  852,  237,  523, 1331,  290,  294,
 /*   300 */   290,   45,  321,  575,  543,  495, 1169,  516,  452, 1169,
 /*   310 */   565,  413,  571,  137,  139,   91, 1333, 1228, 1228, 1063,
 /*   320 */  1066, 1053, 1053, 1207,  348, 1188,  350, 1207,  452,   48,
 /*   330 */   130,  127,  234, 1204, 1204,  137,  139,   91,  221, 1228,
 /*   340 */  1228, 1063, 1066, 1053, 1053, 1204, 1204, 1541,  542,  135,
 /*   350 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   360 */   133,  132,  132,  132,  131,  128,  452, 1204, 1204,  135,
 /*   370 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   380 */   133,  132,  132,  132,  131,  128, 1570,  372,  452,  323,
 /*   390 */  1204,  524,  490,  361, 1205, 1040,  949,  130,  127,  234,
 /*   400 */    48,  372, 1204, 1028,  321,  575, 1205,  135,  135,  136,
 /*   410 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*   420 */   132,  132,  131,  128, 1204,  527,  413,  467, 1205,  135,
 /*   430 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*   440 */   133,  132,  132,  132,  131,  128, 1126,  413,  568,  886,
 /*   450 */   137,  139,   91,   98, 1228, 1228, 1063, 1066, 1053, 1053,
 /*   460 */  1029,  409, 1028, 1028, 1030, 1328, 1311, 1307, 1127,  107,
 /*   470 */   525,  137,  139,   91, 1328, 1228, 1228, 1063, 1066, 1053,
 /*   480 */  1053,  447,  446, 1128,  452,  413,  236, 1577,    7,  461,
 /*   490 */   458,  457,  290,   46,  290, 1236,  526, 1236,  922,  456,
 /*   500 */  1110, 1110,  513,  452,  565, 1511,  571,  222,  923,  137,
 /*   510 */   139,   91,  231, 1228, 1228, 1063, 1066, 1053, 1053,  499,
 /*   520 */   568,  568,  497,  505,  452, 1204, 1204,  136,  136,  136,
 /*   530 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   540 */   131,  128,  392,  228,  135,  135,  136,  136,  136,  136,
 /*   550 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   560 */   128,  267,  452,  574,  879,  135,  135,  136,  136,  136,
 /*   570 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   580 */   131,  128, 1204,   82,   82, 1169, 1205, 1233, 1204, 1204,
 /*   590 */   301, 1235, 1511, 1513, 1511,  372,  219,  413, 1169, 1234,
 /*   600 */   368, 1169,  390,  135,  135,  136,  136,  136,  136,  134,
 /*   610 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   620 */   303,  137,  139,   91, 1598, 1228, 1228, 1063, 1066, 1053,
 /*   630 */  1053,   44,  158,  370,  413,  434,   50,  447,  446, 1204,
 /*   640 */  1204, 1204, 1204,  236,  383, 1204,  461,  458,  457, 1205,
 /*   650 */   129, 1236,  121, 1236,  118,  413,  456,  479,  137,  139,
 /*   660 */    91, 1277, 1228, 1228, 1063, 1066, 1053, 1053,  420,  574,
 /*   670 */   370, 1019,  100,  113,  452,  321,  575, 1659,  402,  137,
 /*   680 */   139,   91,  251, 1228, 1228, 1063, 1066, 1053, 1053,   19,
 /*   690 */    19, 1224,  290,  452,  290, 1224, 1204,  212, 1204,  382,
 /*   700 */  1205,  434, 1205,  432,  565,  440,  571,   50,  372,  267,
 /*   710 */   407,  452,  359,  485,  347,  135,  135,  136,  136,  136,
 /*   720 */   136,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   730 */   131,  128,  452,  183,  448, 1041,  136,  136,  136,  136,
 /*   740 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   750 */   128, 1498,  135,  135,  136,  136,  136,  136,  134,  134,
 /*   760 */   134,  134,  133,  133,  132,  132,  132,  131,  128,  551,
 /*   770 */   413,  298, 1024,  135,  135,  136,  136,  136,  136,  134,
 /*   780 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   790 */   413,  862,  338, 1187,  137,  139,   91, 1169, 1228, 1228,
 /*   800 */  1063, 1066, 1053, 1053,  516,  516,  448,  448,  902,  574,
 /*   810 */  1169,  574,  247, 1169,  137,  139,   91,  253, 1228, 1228,
 /*   820 */  1063, 1066, 1053, 1053, 1050, 1050, 1064, 1067,  413,   84,
 /*   830 */    84,   84,   84,  470,  291,  505,  291,  551,  308, 1224,
 /*   840 */    10,  903,  488, 1224, 1505,  427,  565,  452,  571,  530,
 /*   850 */   530,    7,  137,  139,   91, 1244, 1228, 1228, 1063, 1066,
 /*   860 */  1053, 1053, 1368,  316,  556,  499,  556,  452, 1345,  290,
 /*   870 */   138,  290, 1085,  555,  435,  444, 1204, 1204,  565, 1054,
 /*   880 */   571,  565, 1451,  571,  424,  384,  116,  375,  135,  135,
 /*   890 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   900 */   132,  132,  132,  131,  128,  452,  574,  474,  135,  135,
 /*   910 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*   920 */   132,  132,  132,  131,  128,  419,  145,  145, 1125,  183,
 /*   930 */   380, 1451,  358, 1204,  199, 1450,  304, 1205,  429,  413,
 /*   940 */   435,  372,  518,  559,  120,  215,  135,  135,  136,  136,
 /*   950 */   136,  136,  134,  134,  134,  134,  133,  133,  132,  132,
 /*   960 */   132,  131,  128,  137,  139,   91,  413, 1228, 1228, 1063,
 /*   970 */  1066, 1053, 1053,  290,  290,  290,  290,    6, 1204, 1204,
 /*   980 */  1250,  574,   47,  380, 1450,  565,  565,  571,  571,  549,
 /*   990 */   137,  139,   91, 1169, 1228, 1228, 1063, 1066, 1053, 1053,
 /*  1000 */   413,   19,   19,  213,  552,  510, 1169, 1339,  548, 1169,
 /*  1010 */   576,  507,  936,  936,  322,  437,  452, 1180,  452, 1204,
 /*  1020 */  1204, 1204, 1204,  482,  137,  126,   91,  336, 1228, 1228,
 /*  1030 */  1063, 1066, 1053, 1053,  412, 1204,    5,  335,  290, 1205,
 /*  1040 */   290,  401, 1249,  452,  971,  540,  540,    7,  541, 1324,
 /*  1050 */   565,  209,  571,   47,  372, 1180,  489,  135,  135,  136,
 /*  1060 */   136,  136,  136,  134,  134,  134,  134,  133,  133,  132,
 /*  1070 */   132,  132,  131,  128,  131,  128, 1204,  452, 1204,  499,
 /*  1080 */  1205,  413, 1205,  499,  135,  135,  136,  136,  136,  136,
 /*  1090 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*  1100 */   128,   14, 1204, 1204,  874,  160,  139,   91,  874, 1228,
 /*  1110 */  1228, 1063, 1066, 1053, 1053,  413, 1204, 1204,  135,  135,
 /*  1120 */   136,  136,  136,  136,  134,  134,  134,  134,  133,  133,
 /*  1130 */   132,  132,  132,  131,  128,  574, 1204, 1204,  516,  222,
 /*  1140 */  1449,   91, 1280, 1228, 1228, 1063, 1066, 1053, 1053, 1188,
 /*  1150 */   307,  574,  436,  372,  309,   19,   19,  429,  452, 1204,
 /*  1160 */    42,  970,  574, 1205,  512,  944,  574, 1204, 1204,  442,
 /*  1170 */   943,   19,   19, 1204,    3, 1223,  516, 1205, 1340,  124,
 /*  1180 */   516,  566,   84,   84, 1188,  207,   84,   84,  380, 1449,
 /*  1190 */   441, 1310,  452, 1204,   49,  505,  569, 1205,  536,  135,
 /*  1200 */   135,  136,  136,  136,  136,  134,  134,  134,  134,  133,
 /*  1210 */   133,  132,  132,  132,  131,  128, 1336,  317,  436, 1040,
 /*  1220 */   366,  318,  214,  882, 1204,  562,  524, 1028, 1205, 1309,
 /*  1230 */   512, 1126,  155,  135,  135,  136,  136,  136,  136,  134,
 /*  1240 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*  1250 */   124,  574,  566, 1127, 1040, 1188,  441,  574,  450,  450,
 /*  1260 */  1308,  574, 1028, 1575,    7,   49,  372,  569, 1128, 1282,
 /*  1270 */   418,   19,   19,  232,  124, 1147,  566,   84,   84, 1188,
 /*  1280 */   882,   19,   19,  561, 1029,  208, 1028, 1028, 1030,  232,
 /*  1290 */   574,  569,  310,   34,  331,  550,  562,  969, 1147,  240,
 /*  1300 */   239,  238, 1576,    7,    9,  525,   44,  122,  122,  948,
 /*  1310 */    84,   84,  445,  969,  123,  450,    4, 1574,    7,  572,
 /*  1320 */   562, 1028, 1028, 1030, 1031, 1040,  574, 1289,  453,  450,
 /*  1330 */   450,  286,  969, 1028,  545, 1250, 1149, 1657, 1657,  544,
 /*  1340 */   422,  185,  574, 1573,    7,  449,   84,   84,  969, 1040,
 /*  1350 */   321,  575,  426,  450,  450,  574,  241, 1028,  327, 1149,
 /*  1360 */  1658, 1658,  147,  147,   34,  406,  405,  124, 1552,  566,
 /*  1370 */   567,  418, 1188,  481,   95,   53,   53,  481,  122,  122,
 /*  1380 */   574,  451,  288,  230,  569,  123,  450,    4,   34, 1554,
 /*  1390 */   572,  503, 1028, 1028, 1030, 1031,  401, 1148, 1627,  911,
 /*  1400 */    54,   54,  122,  122,  574,  908,  417,  909,  302,  123,
 /*  1410 */   450,    4,  302,  562,  572,  483, 1028, 1028, 1030, 1031,
 /*  1420 */   574,  330,  469,  893,   55,   55,  412,  545,  574,  423,
 /*  1430 */   475,  574,  546,  243,  280,  329,  472,  332,  471,  242,
 /*  1440 */    56,   56, 1040,  574,  241,  330,  450,  450,   66,   66,
 /*  1450 */  1028,   67,   67,  983,  124,  574,  566,  492,  574, 1188,
 /*  1460 */   491,  984,  496,   21,   21,  574, 1550,  574,  412,  574,
 /*  1470 */   504,  569,  574,  412,  113,   57,   57,  574,   68,   68,
 /*  1480 */   320,   34,  356,  101,  218,   58,   58,   69,   69,   70,
 /*  1490 */    70,  412,   71,   71,  417,  122,  122,   72,   72,  108,
 /*  1500 */   562,  574,  123,  450,    4,  574,  339,  572,  944, 1028,
 /*  1510 */  1028, 1030, 1031,  943,  545,  574,  454,  263,  574,  544,
 /*  1520 */   889,   73,   73,  574,  889,   74,   74,  574,  228, 1040,
 /*  1530 */   574,  107,  574,  450,  450,   75,   75, 1028,   76,   76,
 /*  1540 */   574,  324,  113,   77,   77,  574,  480,   59,   59,  574,
 /*  1550 */    61,   61,   20,   20,  340,    2,  465,  465,  526,  343,
 /*  1560 */   143,  143,  295,  161, 1380,  144,  144,   39,   34,   79,
 /*  1570 */    79,  465,    2,  465,  465, 1144,  403,  901,  900,  295,
 /*  1580 */   161, 1379,  122,  122, 1601,   38,  476,  582,  465,  123,
 /*  1590 */   450,    4, 1366,  292,  572,  430, 1028, 1028, 1030, 1031,
 /*  1600 */   493,  520,  290,  396,  290,  396,  574,  395, 1104,  277,
 /*  1610 */  1104,  393,  357,  113,  565,  484,  571,  859,  574,  290,
 /*  1620 */  1557,  290,  305, 1103,  574, 1103,   62,   62,  574,  254,
 /*  1630 */   574,  565,  248,  571,  342,  124,  205,  566,   80,   80,
 /*  1640 */  1188,  574,  341,  574,   63,   63,  254,  574,   81,   81,
 /*  1650 */    64,   64,  569, 1021,  266,   90,  352,  566,  574, 1530,
 /*  1660 */  1188,  170,  170,  171,  171,  466,  250,   88,   88,  574,
 /*  1670 */  1589,  574,  569, 1153,  177,  498,  266,   43,   65,   65,
 /*  1680 */  1529,  562,  466,  574,  500,  266, 1255,    1,    1,  146,
 /*  1690 */   146,   83,   83, 1092,  249,  508,   25, 1092,  362,  113,
 /*  1700 */   363,  562,  289,  168,  168, 1602, 1616,  574,  986,  987,
 /*  1710 */  1040,  574, 1088,  263,  450,  450,  414, 1376, 1028,  574,
 /*  1720 */   537,  321,  575,  574,  367,  574,  371,  148,  148,  281,
 /*  1730 */  1040,  142,  142,  574,  450,  450, 1389,  554, 1028,  169,
 /*  1740 */   169,  974,  266,  162,  162,  152,  152,  165,  113,   34,
 /*  1750 */   941,  138,  166,  151,  151, 1434,  124,  553,  566, 1362,
 /*  1760 */   560, 1188, 1439,  122,  122,  872,  159, 1192,  389,   34,
 /*  1770 */   123,  450,    4,  569, 1374,  572, 1359, 1028, 1028, 1030,
 /*  1780 */  1031,  942,  138,  122,  122,  391,  476,  582, 1268, 1267,
 /*  1790 */   123,  450,    4,  292, 1269,  572, 1609, 1028, 1028, 1030,
 /*  1800 */  1031,  224,  562,  396,  574,  396, 1032,  395,  167,  277,
 /*  1810 */  1032,  393,  313,  397,  939,  138,   12,  859,  314,  315,
 /*  1820 */   574,  517, 1307,  574,  149,  149,  574,  227,  574, 1612,
 /*  1830 */   574, 1040,  248,  574,  342,  450,  450,  459,  246, 1028,
 /*  1840 */   150,  150,  341,   86,   86, 1416,   78,   78,   87,   87,
 /*  1850 */    85,   85,  334,   52,   52, 1421,  300, 1409, 1426,  355,
 /*  1860 */  1425,  354,  306,  360,  502,  410,  250, 1502, 1501, 1371,
 /*  1870 */    34,  203,  210,  400,  177,  211,  473,   43, 1327, 1372,
 /*  1880 */  1370, 1369,  563,  387,  122,  122,  223, 1326,  476, 1325,
 /*  1890 */    40,  123,  450,    4,  249,  292,  572,  463, 1028, 1028,
 /*  1900 */  1030, 1031,   93, 1626, 1625,  396, 1624,  396,  425,  395,
 /*  1910 */  1318,  277,  404,  393, 1297,  893,  414, 1296,  333,  859,
 /*  1920 */  1295,  321,  575, 1244,  274, 1317, 1241, 1549, 1547,  428,
 /*  1930 */   187,  100, 1507,  220,  248,  244,  342,   96,   99, 1422,
 /*  1940 */   196,   13,  182,  189,  341,  486,  191,  192,  193,  194,
 /*  1950 */   487,  579,  256,  108, 1430, 1428, 1427,  200,   15,  260,
 /*  1960 */   408,  494,  411, 1496,  106,  509, 1518, 1192,  250,  515,
 /*  1970 */   282,  365,  262,  438,  519,  439,  177,  369,  268,   43,
 /*  1980 */   311,  141,  312, 1350,  551,  374,  269,  443,  233, 1580,
 /*  1990 */  1579, 1594, 1394, 1393,   11,  381,  249, 1483,  117,  109,
 /*  2000 */   319, 1349,  529,  577,  386,  216,  385,  388,  273, 1198,
 /*  2010 */   276, 1270,  275,  279,  278,  580, 1265, 1260,  414,  154,
 /*  2020 */   296,  335,  235,  321,  575,  455,  225,  184,  226,   92,
 /*  2030 */   172,  173, 1534,   23,  460,  325,  174,  186,  415,  416,
 /*  2040 */  1535,   24, 1533, 1532,  175,   89,  153, 1194, 1193,  217,
 /*  2050 */   891,  328,  468,  904,   94,  297,  176,  337,  245,  156,
 /*  2060 */  1102,  345, 1100,  188,  178, 1223,  252,  190,  255,  925,
 /*  2070 */   353, 1116,  195,  431,  179,  433,  180,  197,  102,  181,
 /*  2080 */   103,  104, 1119,  257,  258,  105, 1115,  163,  259,   26,
 /*  2090 */   364, 1108,  201,  266,  514, 1155, 1238,  261,  202, 1154,
 /*  2100 */   265,  978,  972,   27,  531,   16,   28,  112,   29,  285,
 /*  2110 */   204,  229,  287,   41,  138, 1160,  538,  206, 1069,   30,
 /*  2120 */    31, 1171, 1173, 1175,  111,    8,  164, 1185, 1179,  113,
 /*  2130 */  1178,   32,   33,  114,  115,  119, 1073, 1083, 1070, 1068,
 /*  2140 */  1072,   18, 1124,  270,   35,  861,   17,  564,  935, 1033,
 /*  2150 */   873,  125,   36,  578,  394,  570,  846,  581, 1256, 1617,
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
   187,  /* (0) explain ::= EXPLAIN */
   187,  /* (1) explain ::= EXPLAIN QUERY PLAN */
   188,  /* (2) cmdx ::= cmd */
   189,  /* (3) cmd ::= BEGIN transtype trans_opt */
   190,  /* (4) transtype ::= */
   190,  /* (5) transtype ::= DEFERRED */
   190,  /* (6) transtype ::= IMMEDIATE */
   190,  /* (7) transtype ::= EXCLUSIVE */
   189,  /* (8) cmd ::= COMMIT|END trans_opt */
   189,  /* (9) cmd ::= ROLLBACK trans_opt */
   189,  /* (10) cmd ::= SAVEPOINT nm */
   189,  /* (11) cmd ::= RELEASE savepoint_opt nm */
   189,  /* (12) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
   194,  /* (13) create_table ::= createkw temp TABLE ifnotexists nm dbnm */
   195,  /* (14) createkw ::= CREATE */
   197,  /* (15) ifnotexists ::= */
   197,  /* (16) ifnotexists ::= IF NOT EXISTS */
   196,  /* (17) temp ::= TEMP */
   196,  /* (18) temp ::= */
   199,  /* (19) create_table_args ::= LP columnlist conslist_opt RP table_option_set */
   199,  /* (20) create_table_args ::= AS select */
   202,  /* (21) table_option_set ::= */
   202,  /* (22) table_option_set ::= table_option_set COMMA table_option */
   204,  /* (23) table_option ::= WITHOUT nm */
   204,  /* (24) table_option ::= nm */
   205,  /* (25) columnname ::= nm typetoken */
   206,  /* (26) typetoken ::= */
   206,  /* (27) typetoken ::= typename LP signed RP */
   206,  /* (28) typetoken ::= typename LP signed COMMA signed RP */
   207,  /* (29) typename ::= typename ID|STRING */
   209,  /* (30) scanpt ::= */
   210,  /* (31) scantok ::= */
   211,  /* (32) ccons ::= CONSTRAINT nm */
   211,  /* (33) ccons ::= DEFAULT scantok term */
   211,  /* (34) ccons ::= DEFAULT LP expr RP */
   211,  /* (35) ccons ::= DEFAULT PLUS scantok term */
   211,  /* (36) ccons ::= DEFAULT MINUS scantok term */
   211,  /* (37) ccons ::= DEFAULT scantok ID|INDEXED */
   211,  /* (38) ccons ::= NOT NULL onconf */
   211,  /* (39) ccons ::= PRIMARY KEY sortorder onconf autoinc */
   211,  /* (40) ccons ::= UNIQUE onconf */
   211,  /* (41) ccons ::= CHECK LP expr RP */
   211,  /* (42) ccons ::= REFERENCES nm eidlist_opt refargs */
   211,  /* (43) ccons ::= defer_subclause */
   211,  /* (44) ccons ::= COLLATE ID|STRING */
   220,  /* (45) generated ::= LP expr RP */
   220,  /* (46) generated ::= LP expr RP ID */
   216,  /* (47) autoinc ::= */
   216,  /* (48) autoinc ::= AUTOINCR */
   218,  /* (49) refargs ::= */
   218,  /* (50) refargs ::= refargs refarg */
   221,  /* (51) refarg ::= MATCH nm */
   221,  /* (52) refarg ::= ON INSERT refact */
   221,  /* (53) refarg ::= ON DELETE refact */
   221,  /* (54) refarg ::= ON UPDATE refact */
   222,  /* (55) refact ::= SET NULL */
   222,  /* (56) refact ::= SET DEFAULT */
   222,  /* (57) refact ::= CASCADE */
   222,  /* (58) refact ::= RESTRICT */
   222,  /* (59) refact ::= NO ACTION */
   219,  /* (60) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
   219,  /* (61) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
   223,  /* (62) init_deferred_pred_opt ::= */
   223,  /* (63) init_deferred_pred_opt ::= INITIALLY DEFERRED */
   223,  /* (64) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
   201,  /* (65) conslist_opt ::= */
   224,  /* (66) tconscomma ::= COMMA */
   225,  /* (67) tcons ::= CONSTRAINT nm */
   225,  /* (68) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
   225,  /* (69) tcons ::= UNIQUE LP sortlist RP onconf */
   225,  /* (70) tcons ::= CHECK LP expr RP onconf */
   225,  /* (71) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
   228,  /* (72) defer_subclause_opt ::= */
   214,  /* (73) onconf ::= */
   214,  /* (74) onconf ::= ON CONFLICT resolvetype */
   230,  /* (75) orconf ::= */
   230,  /* (76) orconf ::= OR resolvetype */
   229,  /* (77) resolvetype ::= IGNORE */
   229,  /* (78) resolvetype ::= REPLACE */
   189,  /* (79) cmd ::= DROP TABLE ifexists fullname */
   231,  /* (80) ifexists ::= IF EXISTS */
   231,  /* (81) ifexists ::= */
   189,  /* (82) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
   189,  /* (83) cmd ::= DROP VIEW ifexists fullname */
   189,  /* (84) cmd ::= select */
   203,  /* (85) select ::= WITH wqlist selectnowith */
   203,  /* (86) select ::= WITH RECURSIVE wqlist selectnowith */
   203,  /* (87) select ::= selectnowith */
   234,  /* (88) selectnowith ::= selectnowith multiselect_op oneselect */
   235,  /* (89) multiselect_op ::= UNION */
   235,  /* (90) multiselect_op ::= UNION ALL */
   235,  /* (91) multiselect_op ::= EXCEPT|INTERSECT */
   236,  /* (92) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
   236,  /* (93) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */
   246,  /* (94) values ::= VALUES LP nexprlist RP */
   236,  /* (95) oneselect ::= mvalues */
   248,  /* (96) mvalues ::= values COMMA LP nexprlist RP */
   248,  /* (97) mvalues ::= mvalues COMMA LP nexprlist RP */
   237,  /* (98) distinct ::= DISTINCT */
   237,  /* (99) distinct ::= ALL */
   237,  /* (100) distinct ::= */
   249,  /* (101) sclp ::= */
   238,  /* (102) selcollist ::= sclp scanpt expr scanpt as */
   238,  /* (103) selcollist ::= sclp scanpt STAR */
   238,  /* (104) selcollist ::= sclp scanpt nm DOT STAR */
   250,  /* (105) as ::= AS nm */
   250,  /* (106) as ::= */
   239,  /* (107) from ::= */
   239,  /* (108) from ::= FROM seltablist */
   252,  /* (109) stl_prefix ::= seltablist joinop */
   252,  /* (110) stl_prefix ::= */
   251,  /* (111) seltablist ::= stl_prefix nm dbnm as on_using */
   251,  /* (112) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */
   251,  /* (113) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */
   251,  /* (114) seltablist ::= stl_prefix LP select RP as on_using */
   251,  /* (115) seltablist ::= stl_prefix LP seltablist RP as on_using */
   198,  /* (116) dbnm ::= */
   198,  /* (117) dbnm ::= DOT nm */
   232,  /* (118) fullname ::= nm */
   232,  /* (119) fullname ::= nm DOT nm */
   257,  /* (120) xfullname ::= nm */
   257,  /* (121) xfullname ::= nm DOT nm */
   257,  /* (122) xfullname ::= nm DOT nm AS nm */
   257,  /* (123) xfullname ::= nm AS nm */
   253,  /* (124) joinop ::= COMMA|JOIN */
   253,  /* (125) joinop ::= JOIN_KW JOIN */
   253,  /* (126) joinop ::= JOIN_KW nm JOIN */
   253,  /* (127) joinop ::= JOIN_KW nm nm JOIN */
   254,  /* (128) on_using ::= ON expr */
   254,  /* (129) on_using ::= USING LP idlist RP */
   254,  /* (130) on_using ::= */
   259,  /* (131) indexed_opt ::= */
   255,  /* (132) indexed_by ::= INDEXED BY nm */
   255,  /* (133) indexed_by ::= NOT INDEXED */
   243,  /* (134) orderby_opt ::= */
   243,  /* (135) orderby_opt ::= ORDER BY sortlist */
   226,  /* (136) sortlist ::= sortlist COMMA expr sortorder nulls */
   226,  /* (137) sortlist ::= expr sortorder nulls */
   215,  /* (138) sortorder ::= ASC */
   215,  /* (139) sortorder ::= DESC */
   215,  /* (140) sortorder ::= */
   260,  /* (141) nulls ::= NULLS FIRST */
   260,  /* (142) nulls ::= NULLS LAST */
   260,  /* (143) nulls ::= */
   241,  /* (144) groupby_opt ::= */
   241,  /* (145) groupby_opt ::= GROUP BY nexprlist */
   242,  /* (146) having_opt ::= */
   242,  /* (147) having_opt ::= HAVING expr */
   244,  /* (148) limit_opt ::= */
   244,  /* (149) limit_opt ::= LIMIT expr */
   244,  /* (150) limit_opt ::= LIMIT expr OFFSET expr */
   244,  /* (151) limit_opt ::= LIMIT expr COMMA expr */
   189,  /* (152) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */
   240,  /* (153) where_opt ::= */
   240,  /* (154) where_opt ::= WHERE expr */
   262,  /* (155) where_opt_ret ::= */
   262,  /* (156) where_opt_ret ::= WHERE expr */
   262,  /* (157) where_opt_ret ::= RETURNING selcollist */
   262,  /* (158) where_opt_ret ::= WHERE expr RETURNING selcollist */
   189,  /* (159) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */
   263,  /* (160) setlist ::= setlist COMMA nm EQ expr */
   263,  /* (161) setlist ::= setlist COMMA LP idlist RP EQ expr */
   263,  /* (162) setlist ::= nm EQ expr */
   263,  /* (163) setlist ::= LP idlist RP EQ expr */
   189,  /* (164) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */
   189,  /* (165) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */
   266,  /* (166) upsert ::= */
   266,  /* (167) upsert ::= RETURNING selcollist */
   266,  /* (168) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */
   266,  /* (169) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */
   266,  /* (170) upsert ::= ON CONFLICT DO NOTHING returning */
   266,  /* (171) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */
   267,  /* (172) returning ::= RETURNING selcollist */
   264,  /* (173) insert_cmd ::= INSERT orconf */
   264,  /* (174) insert_cmd ::= REPLACE */
   265,  /* (175) idlist_opt ::= */
   265,  /* (176) idlist_opt ::= LP idlist RP */
   258,  /* (177) idlist ::= idlist COMMA nm */
   258,  /* (178) idlist ::= nm */
   213,  /* (179) expr ::= LP expr RP */
   213,  /* (180) expr ::= ID|INDEXED|JOIN_KW */
   213,  /* (181) expr ::= nm DOT nm */
   213,  /* (182) expr ::= nm DOT nm DOT nm */
   212,  /* (183) term ::= NULL|FLOAT|BLOB */
   212,  /* (184) term ::= STRING */
   212,  /* (185) term ::= INTEGER */
   213,  /* (186) expr ::= VARIABLE */
   213,  /* (187) expr ::= expr COLLATE ID|STRING */
   213,  /* (188) expr ::= CAST LP expr AS typetoken RP */
   213,  /* (189) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */
   213,  /* (190) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */
   213,  /* (191) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */
   213,  /* (192) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */
   213,  /* (193) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */
   213,  /* (194) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */
   212,  /* (195) term ::= CTIME_KW */
   213,  /* (196) expr ::= LP nexprlist COMMA expr RP */
   213,  /* (197) expr ::= expr AND expr */
   213,  /* (198) expr ::= expr OR expr */
   213,  /* (199) expr ::= expr LT|GT|GE|LE expr */
   213,  /* (200) expr ::= expr EQ|NE expr */
   213,  /* (201) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */
   213,  /* (202) expr ::= expr PLUS|MINUS expr */
   213,  /* (203) expr ::= expr STAR|SLASH|REM expr */
   213,  /* (204) expr ::= expr CONCAT expr */
   269,  /* (205) likeop ::= NOT LIKE_KW|MATCH */
   213,  /* (206) expr ::= expr likeop expr */
   213,  /* (207) expr ::= expr likeop expr ESCAPE expr */
   213,  /* (208) expr ::= expr ISNULL|NOTNULL */
   213,  /* (209) expr ::= expr NOT NULL */
   213,  /* (210) expr ::= expr IS expr */
   213,  /* (211) expr ::= expr IS NOT expr */
   213,  /* (212) expr ::= expr IS NOT DISTINCT FROM expr */
   213,  /* (213) expr ::= expr IS DISTINCT FROM expr */
   213,  /* (214) expr ::= NOT expr */
   213,  /* (215) expr ::= BITNOT expr */
   213,  /* (216) expr ::= PLUS|MINUS expr */
   213,  /* (217) expr ::= expr PTR expr */
   270,  /* (218) between_op ::= BETWEEN */
   270,  /* (219) between_op ::= NOT BETWEEN */
   213,  /* (220) expr ::= expr between_op expr AND expr */
   271,  /* (221) in_op ::= IN */
   271,  /* (222) in_op ::= NOT IN */
   213,  /* (223) expr ::= expr in_op LP exprlist RP */
   213,  /* (224) expr ::= LP select RP */
   213,  /* (225) expr ::= expr in_op LP select RP */
   213,  /* (226) expr ::= expr in_op nm dbnm paren_exprlist */
   213,  /* (227) expr ::= EXISTS LP select RP */
   213,  /* (228) expr ::= CASE case_operand case_exprlist case_else END */
   274,  /* (229) case_exprlist ::= case_exprlist WHEN expr THEN expr */
   274,  /* (230) case_exprlist ::= WHEN expr THEN expr */
   275,  /* (231) case_else ::= ELSE expr */
   275,  /* (232) case_else ::= */
   273,  /* (233) case_operand ::= */
   256,  /* (234) exprlist ::= */
   247,  /* (235) nexprlist ::= nexprlist COMMA expr */
   247,  /* (236) nexprlist ::= expr */
   272,  /* (237) paren_exprlist ::= */
   272,  /* (238) paren_exprlist ::= LP exprlist RP */
   189,  /* (239) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
   276,  /* (240) uniqueflag ::= UNIQUE */
   276,  /* (241) uniqueflag ::= */
   217,  /* (242) eidlist_opt ::= */
   217,  /* (243) eidlist_opt ::= LP eidlist RP */
   227,  /* (244) eidlist ::= eidlist COMMA nm collate sortorder */
   227,  /* (245) eidlist ::= nm collate sortorder */
   277,  /* (246) collate ::= */
   277,  /* (247) collate ::= COLLATE ID|STRING */
   189,  /* (248) cmd ::= DROP INDEX ifexists fullname */
   189,  /* (249) cmd ::= VACUUM vinto */
   189,  /* (250) cmd ::= VACUUM nm vinto */
   278,  /* (251) vinto ::= INTO expr */
   278,  /* (252) vinto ::= */
   189,  /* (253) cmd ::= PRAGMA nm dbnm */
   189,  /* (254) cmd ::= PRAGMA nm dbnm EQ nmnum */
   189,  /* (255) cmd ::= PRAGMA nm dbnm LP nmnum RP */
   189,  /* (256) cmd ::= PRAGMA nm dbnm EQ minus_num */
   189,  /* (257) cmd ::= PRAGMA nm dbnm LP minus_num RP */
   281,  /* (258) plus_num ::= PLUS INTEGER|FLOAT */
   280,  /* (259) minus_num ::= MINUS INTEGER|FLOAT */
   189,  /* (260) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
   282,  /* (261) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
   284,  /* (262) trigger_time ::= BEFORE|AFTER */
   284,  /* (263) trigger_time ::= INSTEAD OF */
   284,  /* (264) trigger_time ::= */
   285,  /* (265) trigger_event ::= DELETE|INSERT */
   285,  /* (266) trigger_event ::= UPDATE */
   285,  /* (267) trigger_event ::= UPDATE OF idlist */
   287,  /* (268) when_clause ::= */
   287,  /* (269) when_clause ::= WHEN expr */
   283,  /* (270) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
   283,  /* (271) trigger_cmd_list ::= trigger_cmd SEMI */
   289,  /* (272) trnm ::= nm DOT nm */
   290,  /* (273) tridxby ::= INDEXED BY nm */
   290,  /* (274) tridxby ::= NOT INDEXED */
   288,  /* (275) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */
   288,  /* (276) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */
   288,  /* (277) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */
   288,  /* (278) trigger_cmd ::= scanpt select scanpt */
   213,  /* (279) expr ::= RAISE LP IGNORE RP */
   213,  /* (280) expr ::= RAISE LP raisetype COMMA expr RP */
   291,  /* (281) raisetype ::= ROLLBACK */
   291,  /* (282) raisetype ::= ABORT */
   291,  /* (283) raisetype ::= FAIL */
   189,  /* (284) cmd ::= DROP TRIGGER ifexists fullname */
   189,  /* (285) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
   189,  /* (286) cmd ::= DETACH database_kw_opt expr */
   293,  /* (287) key_opt ::= */
   293,  /* (288) key_opt ::= KEY expr */
   189,  /* (289) cmd ::= REINDEX */
   189,  /* (290) cmd ::= REINDEX nm dbnm */
   189,  /* (291) cmd ::= ANALYZE */
   189,  /* (292) cmd ::= ANALYZE nm dbnm */
   189,  /* (293) cmd ::= ALTER TABLE fullname RENAME TO nm */
   189,  /* (294) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
   189,  /* (295) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */
   294,  /* (296) add_column_fullname ::= fullname */
   189,  /* (297) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */
   189,  /* (298) cmd ::= create_vtab */
   189,  /* (299) cmd ::= create_vtab LP vtabarglist RP */
   297,  /* (300) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
   299,  /* (301) vtabarg ::= */
   300,  /* (302) vtabargtoken ::= ANY */
   300,  /* (303) vtabargtoken ::= lp anylist RP */
   301,  /* (304) lp ::= LP */
   261,  /* (305) with ::= WITH wqlist */
   261,  /* (306) with ::= WITH RECURSIVE wqlist */
   303,  /* (307) wqas ::= AS */
   303,  /* (308) wqas ::= AS MATERIALIZED */
   303,  /* (309) wqas ::= AS NOT MATERIALIZED */
   304,  /* (310) wqitem ::= withnm eidlist_opt wqas LP select RP */
   305,  /* (311) withnm ::= nm */
   233,  /* (312) wqlist ::= wqitem */
   233,  /* (313) wqlist ::= wqlist COMMA wqitem */
   306,  /* (314) windowdefn_list ::= windowdefn_list COMMA windowdefn */
   307,  /* (315) windowdefn ::= nm AS LP window RP */
   308,  /* (316) window ::= PARTITION BY nexprlist orderby_opt frame_opt */
   308,  /* (317) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */
   308,  /* (318) window ::= ORDER BY sortlist frame_opt */
   308,  /* (319) window ::= nm ORDER BY sortlist frame_opt */
   308,  /* (320) window ::= nm frame_opt */
   309,  /* (321) frame_opt ::= */
   309,  /* (322) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */
   309,  /* (323) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */
   310,  /* (324) range_or_rows ::= RANGE|ROWS|GROUPS */
   311,  /* (325) frame_bound_s ::= frame_bound */
   311,  /* (326) frame_bound_s ::= UNBOUNDED PRECEDING */
   313,  /* (327) frame_bound_e ::= frame_bound */
   313,  /* (328) frame_bound_e ::= UNBOUNDED FOLLOWING */
   314,  /* (329) frame_bound ::= expr PRECEDING|FOLLOWING */
   314,  /* (330) frame_bound ::= CURRENT ROW */
   312,  /* (331) frame_exclude_opt ::= */
   312,  /* (332) frame_exclude_opt ::= EXCLUDE frame_exclude */
   315,  /* (333) frame_exclude ::= NO OTHERS */
   315,  /* (334) frame_exclude ::= CURRENT ROW */
   315,  /* (335) frame_exclude ::= GROUP|TIES */
   245,  /* (336) window_clause ::= WINDOW windowdefn_list */
   268,  /* (337) filter_over ::= filter_clause over_clause */
   268,  /* (338) filter_over ::= over_clause */
   268,  /* (339) filter_over ::= filter_clause */
   317,  /* (340) over_clause ::= OVER LP window RP */
   317,  /* (341) over_clause ::= OVER nm */
   316,  /* (342) filter_clause ::= FILTER LP WHERE expr RP */
   212,  /* (343) term ::= QNUMBER */
   318,  /* (344) input ::= cmdlist */
   319,  /* (345) cmdlist ::= cmdlist ecmd */
   319,  /* (346) cmdlist ::= ecmd */
   320,  /* (347) ecmd ::= SEMI */
   320,  /* (348) ecmd ::= cmdx SEMI */
   320,  /* (349) ecmd ::= explain cmdx SEMI */
   191,  /* (350) trans_opt ::= */
   191,  /* (351) trans_opt ::= TRANSACTION */
   191,  /* (352) trans_opt ::= TRANSACTION nm */
   193,  /* (353) savepoint_opt ::= SAVEPOINT */
   193,  /* (354) savepoint_opt ::= */
   189,  /* (355) cmd ::= create_table create_table_args */
   202,  /* (356) table_option_set ::= table_option */
   200,  /* (357) columnlist ::= columnlist COMMA columnname carglist */
   200,  /* (358) columnlist ::= columnname carglist */
   192,  /* (359) nm ::= ID|INDEXED|JOIN_KW */
   192,  /* (360) nm ::= STRING */
   206,  /* (361) typetoken ::= typename */
   207,  /* (362) typename ::= ID|STRING */
   208,  /* (363) signed ::= plus_num */
   208,  /* (364) signed ::= minus_num */
   296,  /* (365) carglist ::= carglist ccons */
   296,  /* (366) carglist ::= */
   211,  /* (367) ccons ::= NULL onconf */
   211,  /* (368) ccons ::= GENERATED ALWAYS AS generated */
   211,  /* (369) ccons ::= AS generated */
   201,  /* (370) conslist_opt ::= COMMA conslist */
   321,  /* (371) conslist ::= conslist tconscomma tcons */
   321,  /* (372) conslist ::= tcons */
   224,  /* (373) tconscomma ::= */
   228,  /* (374) defer_subclause_opt ::= defer_subclause */
   229,  /* (375) resolvetype ::= raisetype */
   234,  /* (376) selectnowith ::= oneselect */
   236,  /* (377) oneselect ::= values */
   249,  /* (378) sclp ::= selcollist COMMA */
   250,  /* (379) as ::= ID|STRING */
   259,  /* (380) indexed_opt ::= indexed_by */
   267,  /* (381) returning ::= */
   213,  /* (382) expr ::= term */
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
      default:
      /* (0) explain ::= EXPLAIN */ yytestcase(yyruleno==0);
      /* (1) explain ::= EXPLAIN QUERY PLAN */ yytestcase(yyruleno==1);
      /* (2) cmdx ::= cmd (OPTIMIZED OUT) */ assert(yyruleno!=2);
      /* (3) cmd ::= BEGIN transtype trans_opt */ yytestcase(yyruleno==3);
      /* (4) transtype ::= */ yytestcase(yyruleno==4);
      /* (5) transtype ::= DEFERRED */ yytestcase(yyruleno==5);
      /* (6) transtype ::= IMMEDIATE */ yytestcase(yyruleno==6);
      /* (7) transtype ::= EXCLUSIVE */ yytestcase(yyruleno==7);
      /* (8) cmd ::= COMMIT|END trans_opt */ yytestcase(yyruleno==8);
      /* (9) cmd ::= ROLLBACK trans_opt */ yytestcase(yyruleno==9);
      /* (10) cmd ::= SAVEPOINT nm */ yytestcase(yyruleno==10);
      /* (11) cmd ::= RELEASE savepoint_opt nm */ yytestcase(yyruleno==11);
      /* (12) cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */ yytestcase(yyruleno==12);
      /* (13) create_table ::= createkw temp TABLE ifnotexists nm dbnm */ yytestcase(yyruleno==13);
      /* (14) createkw ::= CREATE */ yytestcase(yyruleno==14);
      /* (15) ifnotexists ::= */ yytestcase(yyruleno==15);
      /* (16) ifnotexists ::= IF NOT EXISTS */ yytestcase(yyruleno==16);
      /* (17) temp ::= TEMP */ yytestcase(yyruleno==17);
      /* (18) temp ::= */ yytestcase(yyruleno==18);
      /* (19) create_table_args ::= LP columnlist conslist_opt RP table_option_set */ yytestcase(yyruleno==19);
      /* (20) create_table_args ::= AS select */ yytestcase(yyruleno==20);
      /* (21) table_option_set ::= */ yytestcase(yyruleno==21);
      /* (22) table_option_set ::= table_option_set COMMA table_option */ yytestcase(yyruleno==22);
      /* (23) table_option ::= WITHOUT nm */ yytestcase(yyruleno==23);
      /* (24) table_option ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=24);
      /* (25) columnname ::= nm typetoken */ yytestcase(yyruleno==25);
      /* (26) typetoken ::= */ yytestcase(yyruleno==26);
      /* (27) typetoken ::= typename LP signed RP */ yytestcase(yyruleno==27);
      /* (28) typetoken ::= typename LP signed COMMA signed RP */ yytestcase(yyruleno==28);
      /* (29) typename ::= typename ID|STRING */ yytestcase(yyruleno==29);
      /* (30) scanpt ::= */ yytestcase(yyruleno==30);
      /* (31) scantok ::= */ yytestcase(yyruleno==31);
      /* (32) ccons ::= CONSTRAINT nm */ yytestcase(yyruleno==32);
      /* (33) ccons ::= DEFAULT scantok term */ yytestcase(yyruleno==33);
      /* (34) ccons ::= DEFAULT LP expr RP */ yytestcase(yyruleno==34);
      /* (35) ccons ::= DEFAULT PLUS scantok term */ yytestcase(yyruleno==35);
      /* (36) ccons ::= DEFAULT MINUS scantok term */ yytestcase(yyruleno==36);
      /* (37) ccons ::= DEFAULT scantok ID|INDEXED */ yytestcase(yyruleno==37);
      /* (38) ccons ::= NOT NULL onconf */ yytestcase(yyruleno==38);
      /* (39) ccons ::= PRIMARY KEY sortorder onconf autoinc */ yytestcase(yyruleno==39);
      /* (40) ccons ::= UNIQUE onconf */ yytestcase(yyruleno==40);
      /* (41) ccons ::= CHECK LP expr RP */ yytestcase(yyruleno==41);
      /* (42) ccons ::= REFERENCES nm eidlist_opt refargs */ yytestcase(yyruleno==42);
      /* (43) ccons ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=43);
      /* (44) ccons ::= COLLATE ID|STRING */ yytestcase(yyruleno==44);
      /* (45) generated ::= LP expr RP */ yytestcase(yyruleno==45);
      /* (46) generated ::= LP expr RP ID */ yytestcase(yyruleno==46);
      /* (47) autoinc ::= */ yytestcase(yyruleno==47);
      /* (48) autoinc ::= AUTOINCR */ yytestcase(yyruleno==48);
      /* (49) refargs ::= */ yytestcase(yyruleno==49);
      /* (50) refargs ::= refargs refarg */ yytestcase(yyruleno==50);
      /* (51) refarg ::= MATCH nm */ yytestcase(yyruleno==51);
      /* (52) refarg ::= ON INSERT refact */ yytestcase(yyruleno==52);
      /* (53) refarg ::= ON DELETE refact */ yytestcase(yyruleno==53);
      /* (54) refarg ::= ON UPDATE refact */ yytestcase(yyruleno==54);
      /* (55) refact ::= SET NULL */ yytestcase(yyruleno==55);
      /* (56) refact ::= SET DEFAULT */ yytestcase(yyruleno==56);
      /* (57) refact ::= CASCADE */ yytestcase(yyruleno==57);
      /* (58) refact ::= RESTRICT */ yytestcase(yyruleno==58);
      /* (59) refact ::= NO ACTION */ yytestcase(yyruleno==59);
      /* (60) defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */ yytestcase(yyruleno==60);
      /* (61) defer_subclause ::= DEFERRABLE init_deferred_pred_opt */ yytestcase(yyruleno==61);
      /* (62) init_deferred_pred_opt ::= */ yytestcase(yyruleno==62);
      /* (63) init_deferred_pred_opt ::= INITIALLY DEFERRED */ yytestcase(yyruleno==63);
      /* (64) init_deferred_pred_opt ::= INITIALLY IMMEDIATE */ yytestcase(yyruleno==64);
      /* (65) conslist_opt ::= */ yytestcase(yyruleno==65);
      /* (66) tconscomma ::= COMMA */ yytestcase(yyruleno==66);
      /* (67) tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==67);
      /* (68) tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */ yytestcase(yyruleno==68);
      /* (69) tcons ::= UNIQUE LP sortlist RP onconf */ yytestcase(yyruleno==69);
      /* (70) tcons ::= CHECK LP expr RP onconf */ yytestcase(yyruleno==70);
      /* (71) tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */ yytestcase(yyruleno==71);
      /* (72) defer_subclause_opt ::= */ yytestcase(yyruleno==72);
      /* (73) onconf ::= */ yytestcase(yyruleno==73);
      /* (74) onconf ::= ON CONFLICT resolvetype */ yytestcase(yyruleno==74);
      /* (75) orconf ::= */ yytestcase(yyruleno==75);
      /* (76) orconf ::= OR resolvetype */ yytestcase(yyruleno==76);
      /* (77) resolvetype ::= IGNORE */ yytestcase(yyruleno==77);
      /* (78) resolvetype ::= REPLACE */ yytestcase(yyruleno==78);
      /* (79) cmd ::= DROP TABLE ifexists fullname */ yytestcase(yyruleno==79);
      /* (80) ifexists ::= IF EXISTS */ yytestcase(yyruleno==80);
      /* (81) ifexists ::= */ yytestcase(yyruleno==81);
      /* (82) cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */ yytestcase(yyruleno==82);
      /* (83) cmd ::= DROP VIEW ifexists fullname */ yytestcase(yyruleno==83);
      /* (84) cmd ::= select (OPTIMIZED OUT) */ assert(yyruleno!=84);
      /* (85) select ::= WITH wqlist selectnowith */ yytestcase(yyruleno==85);
      /* (86) select ::= WITH RECURSIVE wqlist selectnowith */ yytestcase(yyruleno==86);
      /* (87) select ::= selectnowith */ yytestcase(yyruleno==87);
      /* (88) selectnowith ::= selectnowith multiselect_op oneselect */ yytestcase(yyruleno==88);
      /* (89) multiselect_op ::= UNION */ yytestcase(yyruleno==89);
      /* (90) multiselect_op ::= UNION ALL */ yytestcase(yyruleno==90);
      /* (91) multiselect_op ::= EXCEPT|INTERSECT */ yytestcase(yyruleno==91);
      /* (92) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */ yytestcase(yyruleno==92);
      /* (93) oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt window_clause orderby_opt limit_opt */ yytestcase(yyruleno==93);
      /* (94) values ::= VALUES LP nexprlist RP */ yytestcase(yyruleno==94);
      /* (95) oneselect ::= mvalues */ yytestcase(yyruleno==95);
      /* (96) mvalues ::= values COMMA LP nexprlist RP */ yytestcase(yyruleno==96);
      /* (97) mvalues ::= mvalues COMMA LP nexprlist RP */ yytestcase(yyruleno==97);
      /* (98) distinct ::= DISTINCT */ yytestcase(yyruleno==98);
      /* (99) distinct ::= ALL */ yytestcase(yyruleno==99);
      /* (100) distinct ::= */ yytestcase(yyruleno==100);
      /* (101) sclp ::= */ yytestcase(yyruleno==101);
      /* (102) selcollist ::= sclp scanpt expr scanpt as */ yytestcase(yyruleno==102);
      /* (103) selcollist ::= sclp scanpt STAR */ yytestcase(yyruleno==103);
      /* (104) selcollist ::= sclp scanpt nm DOT STAR */ yytestcase(yyruleno==104);
      /* (105) as ::= AS nm */ yytestcase(yyruleno==105);
      /* (106) as ::= */ yytestcase(yyruleno==106);
      /* (107) from ::= */ yytestcase(yyruleno==107);
      /* (108) from ::= FROM seltablist */ yytestcase(yyruleno==108);
      /* (109) stl_prefix ::= seltablist joinop */ yytestcase(yyruleno==109);
      /* (110) stl_prefix ::= */ yytestcase(yyruleno==110);
      /* (111) seltablist ::= stl_prefix nm dbnm as on_using */ yytestcase(yyruleno==111);
      /* (112) seltablist ::= stl_prefix nm dbnm as indexed_by on_using */ yytestcase(yyruleno==112);
      /* (113) seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_using */ yytestcase(yyruleno==113);
      /* (114) seltablist ::= stl_prefix LP select RP as on_using */ yytestcase(yyruleno==114);
      /* (115) seltablist ::= stl_prefix LP seltablist RP as on_using */ yytestcase(yyruleno==115);
      /* (116) dbnm ::= */ yytestcase(yyruleno==116);
      /* (117) dbnm ::= DOT nm */ yytestcase(yyruleno==117);
      /* (118) fullname ::= nm */ yytestcase(yyruleno==118);
      /* (119) fullname ::= nm DOT nm */ yytestcase(yyruleno==119);
      /* (120) xfullname ::= nm */ yytestcase(yyruleno==120);
      /* (121) xfullname ::= nm DOT nm */ yytestcase(yyruleno==121);
      /* (122) xfullname ::= nm DOT nm AS nm */ yytestcase(yyruleno==122);
      /* (123) xfullname ::= nm AS nm */ yytestcase(yyruleno==123);
      /* (124) joinop ::= COMMA|JOIN */ yytestcase(yyruleno==124);
      /* (125) joinop ::= JOIN_KW JOIN */ yytestcase(yyruleno==125);
      /* (126) joinop ::= JOIN_KW nm JOIN */ yytestcase(yyruleno==126);
      /* (127) joinop ::= JOIN_KW nm nm JOIN */ yytestcase(yyruleno==127);
      /* (128) on_using ::= ON expr */ yytestcase(yyruleno==128);
      /* (129) on_using ::= USING LP idlist RP */ yytestcase(yyruleno==129);
      /* (130) on_using ::= */ yytestcase(yyruleno==130);
      /* (131) indexed_opt ::= */ yytestcase(yyruleno==131);
      /* (132) indexed_by ::= INDEXED BY nm */ yytestcase(yyruleno==132);
      /* (133) indexed_by ::= NOT INDEXED */ yytestcase(yyruleno==133);
      /* (134) orderby_opt ::= */ yytestcase(yyruleno==134);
      /* (135) orderby_opt ::= ORDER BY sortlist */ yytestcase(yyruleno==135);
      /* (136) sortlist ::= sortlist COMMA expr sortorder nulls */ yytestcase(yyruleno==136);
      /* (137) sortlist ::= expr sortorder nulls */ yytestcase(yyruleno==137);
      /* (138) sortorder ::= ASC */ yytestcase(yyruleno==138);
      /* (139) sortorder ::= DESC */ yytestcase(yyruleno==139);
      /* (140) sortorder ::= */ yytestcase(yyruleno==140);
      /* (141) nulls ::= NULLS FIRST */ yytestcase(yyruleno==141);
      /* (142) nulls ::= NULLS LAST */ yytestcase(yyruleno==142);
      /* (143) nulls ::= */ yytestcase(yyruleno==143);
      /* (144) groupby_opt ::= */ yytestcase(yyruleno==144);
      /* (145) groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==145);
      /* (146) having_opt ::= */ yytestcase(yyruleno==146);
      /* (147) having_opt ::= HAVING expr */ yytestcase(yyruleno==147);
      /* (148) limit_opt ::= */ yytestcase(yyruleno==148);
      /* (149) limit_opt ::= LIMIT expr */ yytestcase(yyruleno==149);
      /* (150) limit_opt ::= LIMIT expr OFFSET expr */ yytestcase(yyruleno==150);
      /* (151) limit_opt ::= LIMIT expr COMMA expr */ yytestcase(yyruleno==151);
      /* (152) cmd ::= with DELETE FROM xfullname indexed_opt where_opt_ret */ yytestcase(yyruleno==152);
      /* (153) where_opt ::= */ yytestcase(yyruleno==153);
      /* (154) where_opt ::= WHERE expr */ yytestcase(yyruleno==154);
      /* (155) where_opt_ret ::= */ yytestcase(yyruleno==155);
      /* (156) where_opt_ret ::= WHERE expr */ yytestcase(yyruleno==156);
      /* (157) where_opt_ret ::= RETURNING selcollist */ yytestcase(yyruleno==157);
      /* (158) where_opt_ret ::= WHERE expr RETURNING selcollist */ yytestcase(yyruleno==158);
      /* (159) cmd ::= with UPDATE orconf xfullname indexed_opt SET setlist from where_opt_ret */ yytestcase(yyruleno==159);
      /* (160) setlist ::= setlist COMMA nm EQ expr */ yytestcase(yyruleno==160);
      /* (161) setlist ::= setlist COMMA LP idlist RP EQ expr */ yytestcase(yyruleno==161);
      /* (162) setlist ::= nm EQ expr */ yytestcase(yyruleno==162);
      /* (163) setlist ::= LP idlist RP EQ expr */ yytestcase(yyruleno==163);
      /* (164) cmd ::= with insert_cmd INTO xfullname idlist_opt select upsert */ yytestcase(yyruleno==164);
      /* (165) cmd ::= with insert_cmd INTO xfullname idlist_opt DEFAULT VALUES returning */ yytestcase(yyruleno==165);
      /* (166) upsert ::= */ yytestcase(yyruleno==166);
      /* (167) upsert ::= RETURNING selcollist */ yytestcase(yyruleno==167);
      /* (168) upsert ::= ON CONFLICT LP sortlist RP where_opt DO UPDATE SET setlist where_opt upsert */ yytestcase(yyruleno==168);
      /* (169) upsert ::= ON CONFLICT LP sortlist RP where_opt DO NOTHING upsert */ yytestcase(yyruleno==169);
      /* (170) upsert ::= ON CONFLICT DO NOTHING returning */ yytestcase(yyruleno==170);
      /* (171) upsert ::= ON CONFLICT DO UPDATE SET setlist where_opt returning */ yytestcase(yyruleno==171);
      /* (172) returning ::= RETURNING selcollist */ yytestcase(yyruleno==172);
      /* (173) insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==173);
      /* (174) insert_cmd ::= REPLACE */ yytestcase(yyruleno==174);
      /* (175) idlist_opt ::= */ yytestcase(yyruleno==175);
      /* (176) idlist_opt ::= LP idlist RP */ yytestcase(yyruleno==176);
      /* (177) idlist ::= idlist COMMA nm */ yytestcase(yyruleno==177);
      /* (178) idlist ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=178);
      /* (179) expr ::= LP expr RP */ yytestcase(yyruleno==179);
      /* (180) expr ::= ID|INDEXED|JOIN_KW */ yytestcase(yyruleno==180);
      /* (181) expr ::= nm DOT nm */ yytestcase(yyruleno==181);
      /* (182) expr ::= nm DOT nm DOT nm */ yytestcase(yyruleno==182);
      /* (183) term ::= NULL|FLOAT|BLOB */ yytestcase(yyruleno==183);
      /* (184) term ::= STRING */ yytestcase(yyruleno==184);
      /* (185) term ::= INTEGER */ yytestcase(yyruleno==185);
      /* (186) expr ::= VARIABLE */ yytestcase(yyruleno==186);
      /* (187) expr ::= expr COLLATE ID|STRING */ yytestcase(yyruleno==187);
      /* (188) expr ::= CAST LP expr AS typetoken RP */ yytestcase(yyruleno==188);
      /* (189) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP */ yytestcase(yyruleno==189);
      /* (190) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP */ yytestcase(yyruleno==190);
      /* (191) expr ::= ID|INDEXED|JOIN_KW LP STAR RP */ yytestcase(yyruleno==191);
      /* (192) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist RP filter_over */ yytestcase(yyruleno==192);
      /* (193) expr ::= ID|INDEXED|JOIN_KW LP distinct exprlist ORDER BY sortlist RP filter_over */ yytestcase(yyruleno==193);
      /* (194) expr ::= ID|INDEXED|JOIN_KW LP STAR RP filter_over */ yytestcase(yyruleno==194);
      /* (195) term ::= CTIME_KW */ yytestcase(yyruleno==195);
      /* (196) expr ::= LP nexprlist COMMA expr RP */ yytestcase(yyruleno==196);
      /* (197) expr ::= expr AND expr */ yytestcase(yyruleno==197);
      /* (198) expr ::= expr OR expr */ yytestcase(yyruleno==198);
      /* (199) expr ::= expr LT|GT|GE|LE expr */ yytestcase(yyruleno==199);
      /* (200) expr ::= expr EQ|NE expr */ yytestcase(yyruleno==200);
      /* (201) expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */ yytestcase(yyruleno==201);
      /* (202) expr ::= expr PLUS|MINUS expr */ yytestcase(yyruleno==202);
      /* (203) expr ::= expr STAR|SLASH|REM expr */ yytestcase(yyruleno==203);
      /* (204) expr ::= expr CONCAT expr */ yytestcase(yyruleno==204);
      /* (205) likeop ::= NOT LIKE_KW|MATCH */ yytestcase(yyruleno==205);
      /* (206) expr ::= expr likeop expr */ yytestcase(yyruleno==206);
      /* (207) expr ::= expr likeop expr ESCAPE expr */ yytestcase(yyruleno==207);
      /* (208) expr ::= expr ISNULL|NOTNULL */ yytestcase(yyruleno==208);
      /* (209) expr ::= expr NOT NULL */ yytestcase(yyruleno==209);
      /* (210) expr ::= expr IS expr */ yytestcase(yyruleno==210);
      /* (211) expr ::= expr IS NOT expr */ yytestcase(yyruleno==211);
      /* (212) expr ::= expr IS NOT DISTINCT FROM expr */ yytestcase(yyruleno==212);
      /* (213) expr ::= expr IS DISTINCT FROM expr */ yytestcase(yyruleno==213);
      /* (214) expr ::= NOT expr */ yytestcase(yyruleno==214);
      /* (215) expr ::= BITNOT expr */ yytestcase(yyruleno==215);
      /* (216) expr ::= PLUS|MINUS expr */ yytestcase(yyruleno==216);
      /* (217) expr ::= expr PTR expr */ yytestcase(yyruleno==217);
      /* (218) between_op ::= BETWEEN */ yytestcase(yyruleno==218);
      /* (219) between_op ::= NOT BETWEEN */ yytestcase(yyruleno==219);
      /* (220) expr ::= expr between_op expr AND expr */ yytestcase(yyruleno==220);
      /* (221) in_op ::= IN */ yytestcase(yyruleno==221);
      /* (222) in_op ::= NOT IN */ yytestcase(yyruleno==222);
      /* (223) expr ::= expr in_op LP exprlist RP */ yytestcase(yyruleno==223);
      /* (224) expr ::= LP select RP */ yytestcase(yyruleno==224);
      /* (225) expr ::= expr in_op LP select RP */ yytestcase(yyruleno==225);
      /* (226) expr ::= expr in_op nm dbnm paren_exprlist */ yytestcase(yyruleno==226);
      /* (227) expr ::= EXISTS LP select RP */ yytestcase(yyruleno==227);
      /* (228) expr ::= CASE case_operand case_exprlist case_else END */ yytestcase(yyruleno==228);
      /* (229) case_exprlist ::= case_exprlist WHEN expr THEN expr */ yytestcase(yyruleno==229);
      /* (230) case_exprlist ::= WHEN expr THEN expr */ yytestcase(yyruleno==230);
      /* (231) case_else ::= ELSE expr */ yytestcase(yyruleno==231);
      /* (232) case_else ::= */ yytestcase(yyruleno==232);
      /* (233) case_operand ::= */ yytestcase(yyruleno==233);
      /* (234) exprlist ::= */ yytestcase(yyruleno==234);
      /* (235) nexprlist ::= nexprlist COMMA expr */ yytestcase(yyruleno==235);
      /* (236) nexprlist ::= expr */ yytestcase(yyruleno==236);
      /* (237) paren_exprlist ::= */ yytestcase(yyruleno==237);
      /* (238) paren_exprlist ::= LP exprlist RP */ yytestcase(yyruleno==238);
      /* (239) cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */ yytestcase(yyruleno==239);
      /* (240) uniqueflag ::= UNIQUE */ yytestcase(yyruleno==240);
      /* (241) uniqueflag ::= */ yytestcase(yyruleno==241);
      /* (242) eidlist_opt ::= */ yytestcase(yyruleno==242);
      /* (243) eidlist_opt ::= LP eidlist RP */ yytestcase(yyruleno==243);
      /* (244) eidlist ::= eidlist COMMA nm collate sortorder */ yytestcase(yyruleno==244);
      /* (245) eidlist ::= nm collate sortorder */ yytestcase(yyruleno==245);
      /* (246) collate ::= */ yytestcase(yyruleno==246);
      /* (247) collate ::= COLLATE ID|STRING */ yytestcase(yyruleno==247);
      /* (248) cmd ::= DROP INDEX ifexists fullname */ yytestcase(yyruleno==248);
      /* (249) cmd ::= VACUUM vinto */ yytestcase(yyruleno==249);
      /* (250) cmd ::= VACUUM nm vinto */ yytestcase(yyruleno==250);
      /* (251) vinto ::= INTO expr */ yytestcase(yyruleno==251);
      /* (252) vinto ::= */ yytestcase(yyruleno==252);
      /* (253) cmd ::= PRAGMA nm dbnm */ yytestcase(yyruleno==253);
      /* (254) cmd ::= PRAGMA nm dbnm EQ nmnum */ yytestcase(yyruleno==254);
      /* (255) cmd ::= PRAGMA nm dbnm LP nmnum RP */ yytestcase(yyruleno==255);
      /* (256) cmd ::= PRAGMA nm dbnm EQ minus_num */ yytestcase(yyruleno==256);
      /* (257) cmd ::= PRAGMA nm dbnm LP minus_num RP */ yytestcase(yyruleno==257);
      /* (258) plus_num ::= PLUS INTEGER|FLOAT */ yytestcase(yyruleno==258);
      /* (259) minus_num ::= MINUS INTEGER|FLOAT */ yytestcase(yyruleno==259);
      /* (260) cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */ yytestcase(yyruleno==260);
      /* (261) trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */ yytestcase(yyruleno==261);
      /* (262) trigger_time ::= BEFORE|AFTER */ yytestcase(yyruleno==262);
      /* (263) trigger_time ::= INSTEAD OF */ yytestcase(yyruleno==263);
      /* (264) trigger_time ::= */ yytestcase(yyruleno==264);
      /* (265) trigger_event ::= DELETE|INSERT */ yytestcase(yyruleno==265);
      /* (266) trigger_event ::= UPDATE */ yytestcase(yyruleno==266);
      /* (267) trigger_event ::= UPDATE OF idlist */ yytestcase(yyruleno==267);
      /* (268) when_clause ::= */ yytestcase(yyruleno==268);
      /* (269) when_clause ::= WHEN expr */ yytestcase(yyruleno==269);
      /* (270) trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */ yytestcase(yyruleno==270);
      /* (271) trigger_cmd_list ::= trigger_cmd SEMI */ yytestcase(yyruleno==271);
      /* (272) trnm ::= nm DOT nm */ yytestcase(yyruleno==272);
      /* (273) tridxby ::= INDEXED BY nm */ yytestcase(yyruleno==273);
      /* (274) tridxby ::= NOT INDEXED */ yytestcase(yyruleno==274);
      /* (275) trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist from where_opt scanpt */ yytestcase(yyruleno==275);
      /* (276) trigger_cmd ::= scanpt insert_cmd INTO trnm idlist_opt select upsert scanpt */ yytestcase(yyruleno==276);
      /* (277) trigger_cmd ::= DELETE FROM trnm tridxby where_opt scanpt */ yytestcase(yyruleno==277);
      /* (278) trigger_cmd ::= scanpt select scanpt */ yytestcase(yyruleno==278);
      /* (279) expr ::= RAISE LP IGNORE RP */ yytestcase(yyruleno==279);
      /* (280) expr ::= RAISE LP raisetype COMMA expr RP */ yytestcase(yyruleno==280);
      /* (281) raisetype ::= ROLLBACK */ yytestcase(yyruleno==281);
      /* (282) raisetype ::= ABORT */ yytestcase(yyruleno==282);
      /* (283) raisetype ::= FAIL */ yytestcase(yyruleno==283);
      /* (284) cmd ::= DROP TRIGGER ifexists fullname */ yytestcase(yyruleno==284);
      /* (285) cmd ::= ATTACH database_kw_opt expr AS expr key_opt */ yytestcase(yyruleno==285);
      /* (286) cmd ::= DETACH database_kw_opt expr */ yytestcase(yyruleno==286);
      /* (287) key_opt ::= */ yytestcase(yyruleno==287);
      /* (288) key_opt ::= KEY expr */ yytestcase(yyruleno==288);
      /* (289) cmd ::= REINDEX */ yytestcase(yyruleno==289);
      /* (290) cmd ::= REINDEX nm dbnm */ yytestcase(yyruleno==290);
      /* (291) cmd ::= ANALYZE */ yytestcase(yyruleno==291);
      /* (292) cmd ::= ANALYZE nm dbnm */ yytestcase(yyruleno==292);
      /* (293) cmd ::= ALTER TABLE fullname RENAME TO nm */ yytestcase(yyruleno==293);
      /* (294) cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */ yytestcase(yyruleno==294);
      /* (295) cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm */ yytestcase(yyruleno==295);
      /* (296) add_column_fullname ::= fullname */ yytestcase(yyruleno==296);
      /* (297) cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm */ yytestcase(yyruleno==297);
      /* (298) cmd ::= create_vtab */ yytestcase(yyruleno==298);
      /* (299) cmd ::= create_vtab LP vtabarglist RP */ yytestcase(yyruleno==299);
      /* (300) create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */ yytestcase(yyruleno==300);
      /* (301) vtabarg ::= */ yytestcase(yyruleno==301);
      /* (302) vtabargtoken ::= ANY */ yytestcase(yyruleno==302);
      /* (303) vtabargtoken ::= lp anylist RP */ yytestcase(yyruleno==303);
      /* (304) lp ::= LP */ yytestcase(yyruleno==304);
      /* (305) with ::= WITH wqlist */ yytestcase(yyruleno==305);
      /* (306) with ::= WITH RECURSIVE wqlist */ yytestcase(yyruleno==306);
      /* (307) wqas ::= AS */ yytestcase(yyruleno==307);
      /* (308) wqas ::= AS MATERIALIZED */ yytestcase(yyruleno==308);
      /* (309) wqas ::= AS NOT MATERIALIZED */ yytestcase(yyruleno==309);
      /* (310) wqitem ::= withnm eidlist_opt wqas LP select RP */ yytestcase(yyruleno==310);
      /* (311) withnm ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=311);
      /* (312) wqlist ::= wqitem (OPTIMIZED OUT) */ assert(yyruleno!=312);
      /* (313) wqlist ::= wqlist COMMA wqitem */ yytestcase(yyruleno==313);
      /* (314) windowdefn_list ::= windowdefn_list COMMA windowdefn */ yytestcase(yyruleno==314);
      /* (315) windowdefn ::= nm AS LP window RP */ yytestcase(yyruleno==315);
      /* (316) window ::= PARTITION BY nexprlist orderby_opt frame_opt */ yytestcase(yyruleno==316);
      /* (317) window ::= nm PARTITION BY nexprlist orderby_opt frame_opt */ yytestcase(yyruleno==317);
      /* (318) window ::= ORDER BY sortlist frame_opt */ yytestcase(yyruleno==318);
      /* (319) window ::= nm ORDER BY sortlist frame_opt */ yytestcase(yyruleno==319);
      /* (320) window ::= nm frame_opt */ yytestcase(yyruleno==320);
      /* (321) frame_opt ::= */ yytestcase(yyruleno==321);
      /* (322) frame_opt ::= range_or_rows frame_bound_s frame_exclude_opt */ yytestcase(yyruleno==322);
      /* (323) frame_opt ::= range_or_rows BETWEEN frame_bound_s AND frame_bound_e frame_exclude_opt */ yytestcase(yyruleno==323);
      /* (324) range_or_rows ::= RANGE|ROWS|GROUPS */ yytestcase(yyruleno==324);
      /* (325) frame_bound_s ::= frame_bound (OPTIMIZED OUT) */ assert(yyruleno!=325);
      /* (326) frame_bound_s ::= UNBOUNDED PRECEDING */ yytestcase(yyruleno==326);
      /* (327) frame_bound_e ::= frame_bound (OPTIMIZED OUT) */ assert(yyruleno!=327);
      /* (328) frame_bound_e ::= UNBOUNDED FOLLOWING */ yytestcase(yyruleno==328);
      /* (329) frame_bound ::= expr PRECEDING|FOLLOWING */ yytestcase(yyruleno==329);
      /* (330) frame_bound ::= CURRENT ROW */ yytestcase(yyruleno==330);
      /* (331) frame_exclude_opt ::= */ yytestcase(yyruleno==331);
      /* (332) frame_exclude_opt ::= EXCLUDE frame_exclude */ yytestcase(yyruleno==332);
      /* (333) frame_exclude ::= NO OTHERS */ yytestcase(yyruleno==333);
      /* (334) frame_exclude ::= CURRENT ROW */ yytestcase(yyruleno==334);
      /* (335) frame_exclude ::= GROUP|TIES */ yytestcase(yyruleno==335);
      /* (336) window_clause ::= WINDOW windowdefn_list */ yytestcase(yyruleno==336);
      /* (337) filter_over ::= filter_clause over_clause */ yytestcase(yyruleno==337);
      /* (338) filter_over ::= over_clause (OPTIMIZED OUT) */ assert(yyruleno!=338);
      /* (339) filter_over ::= filter_clause */ yytestcase(yyruleno==339);
      /* (340) over_clause ::= OVER LP window RP */ yytestcase(yyruleno==340);
      /* (341) over_clause ::= OVER nm */ yytestcase(yyruleno==341);
      /* (342) filter_clause ::= FILTER LP WHERE expr RP */ yytestcase(yyruleno==342);
      /* (343) term ::= QNUMBER */ yytestcase(yyruleno==343);
      /* (344) input ::= cmdlist */ yytestcase(yyruleno==344);
      /* (345) cmdlist ::= cmdlist ecmd */ yytestcase(yyruleno==345);
      /* (346) cmdlist ::= ecmd (OPTIMIZED OUT) */ assert(yyruleno!=346);
      /* (347) ecmd ::= SEMI */ yytestcase(yyruleno==347);
      /* (348) ecmd ::= cmdx SEMI */ yytestcase(yyruleno==348);
      /* (349) ecmd ::= explain cmdx SEMI */ yytestcase(yyruleno==349);
      /* (350) trans_opt ::= */ yytestcase(yyruleno==350);
      /* (351) trans_opt ::= TRANSACTION */ yytestcase(yyruleno==351);
      /* (352) trans_opt ::= TRANSACTION nm */ yytestcase(yyruleno==352);
      /* (353) savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==353);
      /* (354) savepoint_opt ::= */ yytestcase(yyruleno==354);
      /* (355) cmd ::= create_table create_table_args */ yytestcase(yyruleno==355);
      /* (356) table_option_set ::= table_option (OPTIMIZED OUT) */ assert(yyruleno!=356);
      /* (357) columnlist ::= columnlist COMMA columnname carglist */ yytestcase(yyruleno==357);
      /* (358) columnlist ::= columnname carglist */ yytestcase(yyruleno==358);
      /* (359) nm ::= ID|INDEXED|JOIN_KW */ yytestcase(yyruleno==359);
      /* (360) nm ::= STRING */ yytestcase(yyruleno==360);
      /* (361) typetoken ::= typename */ yytestcase(yyruleno==361);
      /* (362) typename ::= ID|STRING */ yytestcase(yyruleno==362);
      /* (363) signed ::= plus_num (OPTIMIZED OUT) */ assert(yyruleno!=363);
      /* (364) signed ::= minus_num (OPTIMIZED OUT) */ assert(yyruleno!=364);
      /* (365) carglist ::= carglist ccons */ yytestcase(yyruleno==365);
      /* (366) carglist ::= */ yytestcase(yyruleno==366);
      /* (367) ccons ::= NULL onconf */ yytestcase(yyruleno==367);
      /* (368) ccons ::= GENERATED ALWAYS AS generated */ yytestcase(yyruleno==368);
      /* (369) ccons ::= AS generated */ yytestcase(yyruleno==369);
      /* (370) conslist_opt ::= COMMA conslist */ yytestcase(yyruleno==370);
      /* (371) conslist ::= conslist tconscomma tcons */ yytestcase(yyruleno==371);
      /* (372) conslist ::= tcons (OPTIMIZED OUT) */ assert(yyruleno!=372);
      /* (373) tconscomma ::= */ yytestcase(yyruleno==373);
      /* (374) defer_subclause_opt ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=374);
      /* (375) resolvetype ::= raisetype (OPTIMIZED OUT) */ assert(yyruleno!=375);
      /* (376) selectnowith ::= oneselect (OPTIMIZED OUT) */ assert(yyruleno!=376);
      /* (377) oneselect ::= values */ yytestcase(yyruleno==377);
      /* (378) sclp ::= selcollist COMMA */ yytestcase(yyruleno==378);
      /* (379) as ::= ID|STRING */ yytestcase(yyruleno==379);
      /* (380) indexed_opt ::= indexed_by (OPTIMIZED OUT) */ assert(yyruleno!=380);
      /* (381) returning ::= */ yytestcase(yyruleno==381);
      /* (382) expr ::= term (OPTIMIZED OUT) */ assert(yyruleno!=382);
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
