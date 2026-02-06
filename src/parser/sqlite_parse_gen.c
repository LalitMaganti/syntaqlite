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
#define SYNTAQLITE_TOKEN_SEMI                            2
#define SYNTAQLITE_TOKEN_ALTER                           3
#define SYNTAQLITE_TOKEN_EXPLAIN                         4
#define SYNTAQLITE_TOKEN_ADD                             5
#define SYNTAQLITE_TOKEN_QUERY                           6
#define SYNTAQLITE_TOKEN_WINDOW                          7
#define SYNTAQLITE_TOKEN_PLAN                            8
#define SYNTAQLITE_TOKEN_OVER                            9
#define SYNTAQLITE_TOKEN_BEGIN                          10
#define SYNTAQLITE_TOKEN_FILTER                         11
#define SYNTAQLITE_TOKEN_TRANSACTION                    12
#define SYNTAQLITE_TOKEN_COLUMN                         13
#define SYNTAQLITE_TOKEN_DEFERRED                       14
#define SYNTAQLITE_TOKEN_AGG_FUNCTION                   15
#define SYNTAQLITE_TOKEN_IMMEDIATE                      16
#define SYNTAQLITE_TOKEN_AGG_COLUMN                     17
#define SYNTAQLITE_TOKEN_EXCLUSIVE                      18
#define SYNTAQLITE_TOKEN_TRUEFALSE                      19
#define SYNTAQLITE_TOKEN_COMMIT                         20
#define SYNTAQLITE_TOKEN_FUNCTION                       21
#define SYNTAQLITE_TOKEN_END                            22
#define SYNTAQLITE_TOKEN_UPLUS                          23
#define SYNTAQLITE_TOKEN_ROLLBACK                       24
#define SYNTAQLITE_TOKEN_UMINUS                         25
#define SYNTAQLITE_TOKEN_SAVEPOINT                      26
#define SYNTAQLITE_TOKEN_TRUTH                          27
#define SYNTAQLITE_TOKEN_RELEASE                        28
#define SYNTAQLITE_TOKEN_REGISTER                       29
#define SYNTAQLITE_TOKEN_TO                             30
#define SYNTAQLITE_TOKEN_VECTOR                         31
#define SYNTAQLITE_TOKEN_TABLE                          32
#define SYNTAQLITE_TOKEN_SELECT_COLUMN                  33
#define SYNTAQLITE_TOKEN_CREATE                         34
#define SYNTAQLITE_TOKEN_IF_NULL_ROW                    35
#define SYNTAQLITE_TOKEN_IF                             36
#define SYNTAQLITE_TOKEN_ASTERISK                       37
#define SYNTAQLITE_TOKEN_NOT                            38
#define SYNTAQLITE_TOKEN_SPAN                           39
#define SYNTAQLITE_TOKEN_EXISTS                         40
#define SYNTAQLITE_TOKEN_ERROR                          41
#define SYNTAQLITE_TOKEN_TEMP                           42
#define SYNTAQLITE_TOKEN_QNUMBER                        43
#define SYNTAQLITE_TOKEN_LP                             44
#define SYNTAQLITE_TOKEN_SPACE                          45
#define SYNTAQLITE_TOKEN_RP                             46
#define SYNTAQLITE_TOKEN_COMMENT                        47
#define SYNTAQLITE_TOKEN_AS                             48
#define SYNTAQLITE_TOKEN_ILLEGAL                        49
#define SYNTAQLITE_TOKEN_COMMA                          50
#define SYNTAQLITE_TOKEN_WITHOUT                        51
#define SYNTAQLITE_TOKEN_ABORT                          52
#define SYNTAQLITE_TOKEN_ACTION                         53
#define SYNTAQLITE_TOKEN_AFTER                          54
#define SYNTAQLITE_TOKEN_ANALYZE                        55
#define SYNTAQLITE_TOKEN_ASC                            56
#define SYNTAQLITE_TOKEN_ATTACH                         57
#define SYNTAQLITE_TOKEN_BEFORE                         58
#define SYNTAQLITE_TOKEN_BY                             59
#define SYNTAQLITE_TOKEN_CASCADE                        60
#define SYNTAQLITE_TOKEN_CAST                           61
#define SYNTAQLITE_TOKEN_CONFLICT                       62
#define SYNTAQLITE_TOKEN_DATABASE                       63
#define SYNTAQLITE_TOKEN_DESC                           64
#define SYNTAQLITE_TOKEN_DETACH                         65
#define SYNTAQLITE_TOKEN_EACH                           66
#define SYNTAQLITE_TOKEN_FAIL                           67
#define SYNTAQLITE_TOKEN_OR                             68
#define SYNTAQLITE_TOKEN_AND                            69
#define SYNTAQLITE_TOKEN_IS                             70
#define SYNTAQLITE_TOKEN_ISNOT                          71
#define SYNTAQLITE_TOKEN_MATCH                          72
#define SYNTAQLITE_TOKEN_LIKE_KW                        73
#define SYNTAQLITE_TOKEN_BETWEEN                        74
#define SYNTAQLITE_TOKEN_IN                             75
#define SYNTAQLITE_TOKEN_ISNULL                         76
#define SYNTAQLITE_TOKEN_NOTNULL                        77
#define SYNTAQLITE_TOKEN_NE                             78
#define SYNTAQLITE_TOKEN_EQ                             79
#define SYNTAQLITE_TOKEN_GT                             80
#define SYNTAQLITE_TOKEN_LE                             81
#define SYNTAQLITE_TOKEN_LT                             82
#define SYNTAQLITE_TOKEN_GE                             83
#define SYNTAQLITE_TOKEN_ESCAPE                         84
#define SYNTAQLITE_TOKEN_ID                             85
#define SYNTAQLITE_TOKEN_COLUMNKW                       86
#define SYNTAQLITE_TOKEN_DO                             87
#define SYNTAQLITE_TOKEN_FOR                            88
#define SYNTAQLITE_TOKEN_IGNORE                         89
#define SYNTAQLITE_TOKEN_INITIALLY                      90
#define SYNTAQLITE_TOKEN_INSTEAD                        91
#define SYNTAQLITE_TOKEN_NO                             92
#define SYNTAQLITE_TOKEN_KEY                            93
#define SYNTAQLITE_TOKEN_OF                             94
#define SYNTAQLITE_TOKEN_OFFSET                         95
#define SYNTAQLITE_TOKEN_PRAGMA                         96
#define SYNTAQLITE_TOKEN_RAISE                          97
#define SYNTAQLITE_TOKEN_RECURSIVE                      98
#define SYNTAQLITE_TOKEN_REPLACE                        99
#define SYNTAQLITE_TOKEN_RESTRICT                       100
#define SYNTAQLITE_TOKEN_ROW                            101
#define SYNTAQLITE_TOKEN_ROWS                           102
#define SYNTAQLITE_TOKEN_TRIGGER                        103
#define SYNTAQLITE_TOKEN_VACUUM                         104
#define SYNTAQLITE_TOKEN_VIEW                           105
#define SYNTAQLITE_TOKEN_VIRTUAL                        106
#define SYNTAQLITE_TOKEN_WITH                           107
#define SYNTAQLITE_TOKEN_NULLS                          108
#define SYNTAQLITE_TOKEN_FIRST                          109
#define SYNTAQLITE_TOKEN_LAST                           110
#define SYNTAQLITE_TOKEN_CURRENT                        111
#define SYNTAQLITE_TOKEN_FOLLOWING                      112
#define SYNTAQLITE_TOKEN_PARTITION                      113
#define SYNTAQLITE_TOKEN_PRECEDING                      114
#define SYNTAQLITE_TOKEN_RANGE                          115
#define SYNTAQLITE_TOKEN_UNBOUNDED                      116
#define SYNTAQLITE_TOKEN_EXCLUDE                        117
#define SYNTAQLITE_TOKEN_GROUPS                         118
#define SYNTAQLITE_TOKEN_OTHERS                         119
#define SYNTAQLITE_TOKEN_TIES                           120
#define SYNTAQLITE_TOKEN_GENERATED                      121
#define SYNTAQLITE_TOKEN_ALWAYS                         122
#define SYNTAQLITE_TOKEN_MATERIALIZED                   123
#define SYNTAQLITE_TOKEN_REINDEX                        124
#define SYNTAQLITE_TOKEN_RENAME                         125
#define SYNTAQLITE_TOKEN_CTIME_KW                       126
#define SYNTAQLITE_TOKEN_ANY                            127
#define SYNTAQLITE_TOKEN_BITAND                         128
#define SYNTAQLITE_TOKEN_BITOR                          129
#define SYNTAQLITE_TOKEN_LSHIFT                         130
#define SYNTAQLITE_TOKEN_RSHIFT                         131
#define SYNTAQLITE_TOKEN_PLUS                           132
#define SYNTAQLITE_TOKEN_MINUS                          133
#define SYNTAQLITE_TOKEN_STAR                           134
#define SYNTAQLITE_TOKEN_SLASH                          135
#define SYNTAQLITE_TOKEN_REM                            136
#define SYNTAQLITE_TOKEN_CONCAT                         137
#define SYNTAQLITE_TOKEN_PTR                            138
#define SYNTAQLITE_TOKEN_COLLATE                        139
#define SYNTAQLITE_TOKEN_BITNOT                         140
#define SYNTAQLITE_TOKEN_ON                             141
#define SYNTAQLITE_TOKEN_INDEXED                        142
#define SYNTAQLITE_TOKEN_STRING                         143
#define SYNTAQLITE_TOKEN_JOIN_KW                        144
#define SYNTAQLITE_TOKEN_CONSTRAINT                     145
#define SYNTAQLITE_TOKEN_DEFAULT                        146
#define SYNTAQLITE_TOKEN_NULL                           147
#define SYNTAQLITE_TOKEN_PRIMARY                        148
#define SYNTAQLITE_TOKEN_UNIQUE                         149
#define SYNTAQLITE_TOKEN_CHECK                          150
#define SYNTAQLITE_TOKEN_REFERENCES                     151
#define SYNTAQLITE_TOKEN_AUTOINCR                       152
#define SYNTAQLITE_TOKEN_INSERT                         153
#define SYNTAQLITE_TOKEN_DELETE                         154
#define SYNTAQLITE_TOKEN_UPDATE                         155
#define SYNTAQLITE_TOKEN_SET                            156
#define SYNTAQLITE_TOKEN_DEFERRABLE                     157
#define SYNTAQLITE_TOKEN_FOREIGN                        158
#define SYNTAQLITE_TOKEN_DROP                           159
#define SYNTAQLITE_TOKEN_UNION                          160
#define SYNTAQLITE_TOKEN_ALL                            161
#define SYNTAQLITE_TOKEN_EXCEPT                         162
#define SYNTAQLITE_TOKEN_INTERSECT                      163
#define SYNTAQLITE_TOKEN_SELECT                         164
#define SYNTAQLITE_TOKEN_VALUES                         165
#define SYNTAQLITE_TOKEN_DISTINCT                       166
#define SYNTAQLITE_TOKEN_DOT                            167
#define SYNTAQLITE_TOKEN_FROM                           168
#define SYNTAQLITE_TOKEN_JOIN                           169
#define SYNTAQLITE_TOKEN_USING                          170
#define SYNTAQLITE_TOKEN_ORDER                          171
#define SYNTAQLITE_TOKEN_GROUP                          172
#define SYNTAQLITE_TOKEN_HAVING                         173
#define SYNTAQLITE_TOKEN_LIMIT                          174
#define SYNTAQLITE_TOKEN_WHERE                          175
#define SYNTAQLITE_TOKEN_RETURNING                      176
#define SYNTAQLITE_TOKEN_INTO                           177
#define SYNTAQLITE_TOKEN_NOTHING                        178
#define SYNTAQLITE_TOKEN_FLOAT                          179
#define SYNTAQLITE_TOKEN_BLOB                           180
#define SYNTAQLITE_TOKEN_INTEGER                        181
#define SYNTAQLITE_TOKEN_VARIABLE                       182
#define SYNTAQLITE_TOKEN_CASE                           183
#define SYNTAQLITE_TOKEN_WHEN                           184
#define SYNTAQLITE_TOKEN_THEN                           185
#define SYNTAQLITE_TOKEN_ELSE                           186
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
#define YYWILDCARD 127
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
#define YY_ACTTAB_COUNT (2417)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   522,    2,  161, 1637,  568, 1620, 1621,  481,  299,  290,
 /*    10 */  1633,  284,  130,  349, 1290,  234, 1644,  290, 1633,  574,
 /*    20 */   565, 1293,  571, 1281,  264,  230,  254,  574,  565, 1644,
 /*    30 */   571, 1277,  465, 1259,  295, 1292,  212, 1341,  413,  505,
 /*    40 */   290, 1633, 1659,  402,  351,  308,  290, 1633, 1643,  477,
 /*    50 */   478,  565,  316,  571,  290, 1633,  983,  565,  574,  571,
 /*    60 */   398, 1643, 1513, 1511,  984,  565, 1279,  571,  137,  139,
 /*    70 */    91, 1628, 1228, 1228, 1063, 1066, 1053, 1053,  135,  135,
 /*    80 */   136,  136,  136,  136,  212, 1639,   19,  518,  466,  421,
 /*    90 */  1300, 1368,  557, 1639,   84, 1622,  290, 1633,  303,   10,
 /*   100 */   326,  290, 1633,  267, 1255,    1, 1603,  565,  407,  571,
 /*   110 */   127,  413,  565,  556,  571,  348,  501,  350,  558,  136,
 /*   120 */   136,  136,  136,  129, 1639,   84,  247,  574,  134,  134,
 /*   130 */   134,  134,  133,  133,  132,  132,  132,  131,  128,  452,
 /*   140 */  1224,  137,  139,   91,  528, 1228, 1228, 1063, 1066, 1053,
 /*   150 */  1053,  135,  135,  136,  136,  136,  136,  132,  132,  132,
 /*   160 */   131,  128,  452,  419,  510,  574,  499,  134,  134,  134,
 /*   170 */   134,  133,  133,  132,  132,  132,  131,  128,  452,  237,
 /*   180 */  1019, 1019, 1244,  497,  413, 1174,  236, 1174,   44,  461,
 /*   190 */   458,  457,  130, 1639,   51,  234,  110,  574, 1224,  456,
 /*   200 */   505,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   210 */   131,  128,  452, 1204,  137,  139,   91,  452, 1228, 1228,
 /*   220 */  1063, 1066, 1053, 1053,  135,  135,  136,  136,  136,  136,
 /*   230 */   534, 1639,   60,  409,  359,  359,  485,  347,  559,  290,
 /*   240 */  1633,  140,  290, 1633,  507,  321,  575,  290, 1633,  499,
 /*   250 */   565,  535,  571,  565,  377,  571, 1582,  413,  565,  358,
 /*   260 */   571,  574,  301, 1639,   51,  136,  136,  136,  136, 1105,
 /*   270 */  1204, 1205, 1204,  573,  134,  134,  134,  134,  133,  133,
 /*   280 */   132,  132,  132,  131,  128,  452,   44,  137,  139,   91,
 /*   290 */   127, 1228, 1228, 1063, 1066, 1053, 1053,  135,  135,  136,
 /*   300 */   136,  136,  136,  133,  133,  132,  132,  132,  131,  128,
 /*   310 */   452,  499,   97,  134,  134,  134,  134,  133,  133,  132,
 /*   320 */   132,  132,  131,  128,  452,  376, 1584, 1639,   19, 1627,
 /*   330 */   413,  294, 1188,  911,  233, 1126,  539,  516,  323, 1050,
 /*   340 */  1050, 1064, 1067,  321,  575,  304,  574,  134,  134,  134,
 /*   350 */   134,  133,  133,  132,  132,  132,  131,  128,  452,  212,
 /*   360 */   137,  139,   91, 1127, 1228, 1228, 1063, 1066, 1053, 1053,
 /*   370 */   135,  135,  136,  136,  136,  136,  130,  524, 1128,  234,
 /*   380 */   448, 1620, 1621,  399,  574,  298,  134,  134,  134,  134,
 /*   390 */   133,  133,  132,  132,  132,  131,  128,  452, 1204,  574,
 /*   400 */   922,  490,  361,  413,  532, 1629,  293,  307,  157,  427,
 /*   410 */   923,  467, 1639,   51, 1054, 1040,  131,  128,  452,  338,
 /*   420 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*   430 */   128,  452, 1029,  137,  139,   91, 1028, 1228, 1228, 1063,
 /*   440 */  1066, 1053, 1053,  135,  135,  136,  136,  136,  136,  373,
 /*   450 */  1639,   19,  474,    6,  432, 1204, 1205, 1204,  236,  574,
 /*   460 */   549,  461,  458,  457,  574, 1639,   84,  574, 1028, 1028,
 /*   470 */  1030,  456, 1632,  547,  127, 1582,  413, 1331, 1204,  548,
 /*   480 */   107,  525,  527, 1568,  886,  556, 1204, 1542, 1180, 1204,
 /*   490 */   523,   45,  372,  134,  134,  134,  134,  133,  133,  132,
 /*   500 */   132,  132,  131,  128,  452,  429,  137,  139,   91,  526,
 /*   510 */  1228, 1228, 1063, 1066, 1053, 1053,  135,  135,  136,  136,
 /*   520 */   136,  136,  321,  575,    6, 1639,   19,  533, 1664,  437,
 /*   530 */  1639,   84,   95, 1639,   84, 1204, 1205, 1204,  949,  231,
 /*   540 */  1180, 1204, 1339, 1204, 1205, 1204, 1204, 1205, 1204,  413,
 /*   550 */   556,  290, 1633,  556,   46,  555,   48,  879,  444,  209,
 /*   560 */   574, 1598,  565,  489,  571, 1451,  134,  134,  134,  134,
 /*   570 */   133,  133,  132,  132,  132,  131,  128,  452,  100,  137,
 /*   580 */   139,   91,  198, 1228, 1228, 1063, 1066, 1053, 1053,  135,
 /*   590 */   135,  136,  136,  136,  136,  380, 1595,  346, 1204, 1205,
 /*   600 */  1204, 1169,  283, 1633,  130, 1569,  130,  234,  576,  234,
 /*   610 */   936,  936,  543,  565, 1169,  571, 1204, 1169,  574, 1204,
 /*   620 */   406,  405,  413, 1204,  574, 1311, 1639,   19,  228,   44,
 /*   630 */   442,  574,   98, 1307,  506, 1631,  344, 1328,  453,  134,
 /*   640 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*   650 */   452,  499,  137,  139,   91,  183, 1228, 1228, 1063, 1066,
 /*   660 */  1053, 1053,  135,  135,  136,  136,  136,  136,  479,  378,
 /*   670 */   542,  379,  241, 1204, 1205, 1204, 1204, 1205, 1204,  574,
 /*   680 */  1204, 1205, 1204,  574, 1639,   19,  321,  575,  207,  574,
 /*   690 */  1639,   19,  574, 1169,  208,  413,  574, 1639,   19,  183,
 /*   700 */   971,  550,  127,  970,  127,  521, 1169, 1107,  382, 1169,
 /*   710 */     7, 1107,  134,  134,  134,  134,  133,  133,  132,  132,
 /*   720 */   132,  131,  128,  452, 1281,  137,  139,   91,  322, 1228,
 /*   730 */  1228, 1063, 1066, 1053, 1053,  135,  135,  136,  136,  136,
 /*   740 */   136,  417, 1506, 1498,  511, 1639,   84,  309,  574, 1639,
 /*   750 */    84,  490,  361,  108,    7, 1639,   84,  330, 1639,   84,
 /*   760 */  1204,    7, 1639,   84,  429,  317,  462, 1613,  413,  318,
 /*   770 */  1568, 1639,   22,  370, 1568,  445, 1041,  505,  449,  372,
 /*   780 */   530, 1665,  451,  372,  862,  134,  134,  134,  134,  133,
 /*   790 */   133,  132,  132,  132,  131,  128,  452,  286,  137,  139,
 /*   800 */    91,  969, 1228, 1228, 1063, 1066, 1053, 1053,  135,  135,
 /*   810 */   136,  136,  136,  136, 1639,   82, 1207, 1204, 1205, 1204,
 /*   820 */  1110, 1110,  513,  532,  540, 1665,  291, 1633, 1149, 1149,
 /*   830 */  1657, 1658, 1577,  219, 1657, 1658,  310,  565,  113,  571,
 /*   840 */   520,  413,  574,   50,  420, 1568, 1568,   47,  488, 1024,
 /*   850 */   574,  383,  447,  446,  372,  372,  213,  574,  134,  134,
 /*   860 */   134,  134,  133,  133,  132,  132,  132,  131,  128,  452,
 /*   870 */   902,  137,  139,   91, 1207, 1228, 1228, 1063, 1066, 1053,
 /*   880 */  1053,  135,  135,  136,  136,  136,  136,  240,  239,  238,
 /*   890 */  1169, 1085, 1569, 1310,  536,  232, 1569,  969,  574, 1236,
 /*   900 */  1632, 1236,  470, 1169,  574, 1333, 1169, 1309, 1639,  145,
 /*   910 */   903, 1147, 1147,  574,  413,  574, 1639,  147,   49,  574,
 /*   920 */   969,  944, 1187, 1639,   53, 1153,  943,  574,  321,  575,
 /*   930 */  1450,  134,  134,  134,  134,  133,  133,  132,  132,  132,
 /*   940 */   131,  128,  452,  524,  137,  139,   91, 1571, 1228, 1228,
 /*   950 */  1063, 1066, 1053, 1053,  135,  135,  136,  136,  136,  136,
 /*   960 */   380, 1595, 1345,  392, 1639,   54,  424, 1569, 1570, 1324,
 /*   970 */  1639,   55,  565,  574,  571,  384,  574,  375,  574, 1639,
 /*   980 */    56, 1639,   66,  574,  429, 1639,   67,  413,  574, 1282,
 /*   990 */   418,  232, 1126, 1639,   21,  574, 1435,  850,  574,  851,
 /*  1000 */   574,  852,  516,  574,  134,  134,  134,  134,  133,  133,
 /*  1010 */   132,  132,  132,  131,  128,  452,  969,  137,  139,   91,
 /*  1020 */  1127, 1228, 1228, 1063, 1066, 1053, 1053,  135,  135,  136,
 /*  1030 */   136,  136,  136, 1637,  368, 1128,  390,  302,  516, 1639,
 /*  1040 */    57,  574, 1639,   68, 1639,   58,  574,  525,  574, 1639,
 /*  1050 */    69,  574, 1280,  574, 1639,   70,  574,  561,  574,  516,
 /*  1060 */   413, 1639,   71,   25, 1639,   72, 1639,   73, 1125, 1639,
 /*  1070 */    74,  574,  516,  574, 1541,  158,  214,  134,  134,  134,
 /*  1080 */   134,  133,  133,  132,  132,  132,  131,  128,  452,  434,
 /*  1090 */   137,  139,   91, 1308, 1228, 1228, 1063, 1066, 1053, 1053,
 /*  1100 */   135,  135,  136,  136,  136,  136,  430, 1639,   75,  267,
 /*  1110 */  1505,  516, 1639,   76, 1639,   77,  113, 1639,   59, 1639,
 /*  1120 */    61,  574, 1639,   20, 1639,  143,  574, 1435,  574, 1435,
 /*  1130 */   574, 1340,  574,  413,  116,  422,  185, 1639,  144, 1639,
 /*  1140 */    79,  574, 1435,  574, 1336,  574, 1224,  574,  331, 1449,
 /*  1150 */   134,  134,  134,  134,  133,  133,  132,  132,  132,  131,
 /*  1160 */   128,  452,  155,  137,  139,   91,   14, 1228, 1228, 1063,
 /*  1170 */  1066, 1053, 1053,  135,  135,  136,  136,  136,  136,  380,
 /*  1180 */  1595,  567,  418,  366,  370,  241,  574, 1639,   62,  288,
 /*  1190 */   230,  574, 1639,   80, 1639,   63, 1639,   81, 1639,   64,
 /*  1200 */   574, 1435,  574, 1289, 1224,  574,  413, 1639,  170, 1639,
 /*  1210 */   171, 1639,   88, 1639,   65,  574,  215,  574,  893,  574,
 /*  1220 */   435,    7,  436,  134,  134,  134,  134,  133,  133,  132,
 /*  1230 */   132,  132,  131,  128,  452,  512,  137,  139,   91,  551,
 /*  1240 */  1228, 1228, 1063, 1066, 1053, 1053,  135,  135,  136,  136,
 /*  1250 */   136,  136, 1639,  146,  417,  440,  983, 1639,   83,  574,
 /*  1260 */   401,  574, 1249,    7,  984,  574, 1639,  168, 1639,  148,
 /*  1270 */   574, 1639,  142,  574,  401,  574, 1148,    7,  574,  413,
 /*  1280 */   120, 1639,  169, 1639,  162, 1639,  152,  574,    7,  101,
 /*  1290 */   218, 1552, 1575,  228,  441,  495,  134,  134,  134,  134,
 /*  1300 */   133,  133,  132,  132,  132,  131,  128,  452,  503,  137,
 /*  1310 */   139,   91,  138, 1228, 1228, 1063, 1066, 1053, 1053,  135,
 /*  1320 */   135,  136,  136,  136,  136, 1639,  151, 1639,  149,  107,
 /*  1330 */   221, 1639,  150, 1554, 1576,  482, 1639,   86, 1223, 1639,
 /*  1340 */    78, 1639,   87, 1250, 1639,   85,  412,  166, 1574,  483,
 /*  1350 */   944,  492,  413, 1639,   52,  943,  496, 1250,  526, 1573,
 /*  1360 */   412,  121,  412,  118,  454,  251,  889,  412,  263,  134,
 /*  1370 */   134,  134,  134,  133,  133,  132,  132,  132,  131,  128,
 /*  1380 */   452,  160,  137,  126,   91,   38, 1228, 1228, 1063, 1066,
 /*  1390 */  1053, 1053,  135,  135,  136,  136,  136,  136,  320,  324,
 /*  1400 */  1144,  901,  900,  113,  403,  908,  336,  909,  491,  412,
 /*  1410 */   357,  222,  504,  493,  113, 1021,  335,  498, 1092,  266,
 /*  1420 */    39,  266,  874,  413,  889,  500,  362,  289, 1088,  266,
 /*  1430 */   113,  356,  263,  551,  974,  165,  305, 1032,  266,  113,
 /*  1440 */   480,  199,  134,  134,  134,  134,  133,  133,  132,  132,
 /*  1450 */   132,  131,  128,  452,  139,   91,  222, 1228, 1228, 1063,
 /*  1460 */  1066, 1053, 1053,  135,  135,  136,  136,  136,  136,  986,
 /*  1470 */   987,  941,  537,  475,  939,  138, 1092, 1104,  138, 1104,
 /*  1480 */   874, 1103, 1204, 1103,  253,  872,  942, 1550, 1589,  159,
 /*  1490 */   138,  339,  340,  413,  343, 1032,  426, 1380, 1379,  484,
 /*  1500 */  1557,  224,  352, 1530,  517,  560,  327, 1529,  508,  363,
 /*  1510 */  1376,  367,  371,  134,  134,  134,  134,  133,  133,  132,
 /*  1520 */   132,  132,  131,  128,  452,   91, 1366, 1228, 1228, 1063,
 /*  1530 */  1066, 1053, 1053,  135,  135,  136,  136,  136,  136, 1204,
 /*  1540 */  1205, 1204, 1389, 1434, 1362,   12,  205,  397, 1439,  389,
 /*  1550 */  1616,  167,  124, 1374,  566,  391,  459, 1188,    4, 1268,
 /*  1560 */  1267, 1269, 1609,  227,  281, 1359,  313, 1307, 1421,  314,
 /*  1570 */  1426, 1416,    5,  246,  315,  569, 1409,   42, 1612,  469,
 /*  1580 */   354, 1188, 1425,  134,  134,  134,  134,  133,  133,  132,
 /*  1590 */   132,  132,  131,  128,  452,  300,  124,  423,  566,  450,
 /*  1600 */   334, 1188,    4,  243,  280,  329,  472,  332,  471,  242,
 /*  1610 */   355,  562,  306, 1204,  410,  330,  360,  210, 1204,  569,
 /*  1620 */  1502,   44, 1501,  882, 1371,  203,  473, 1372,  563, 1327,
 /*  1630 */  1370,  502,    9, 1369,   40, 1326, 1325,  463,   93,  211,
 /*  1640 */  1040,  387, 1626,  450, 1625, 1624,  122,  122,    3,  425,
 /*  1650 */   400, 1318, 1297,  893,  123,  562,  450,  572,  450,  223,
 /*  1660 */   404, 1028, 1317, 1296, 1040, 1244,  333, 1295, 1549,  545,
 /*  1670 */  1204, 1205, 1204,  187,  544, 1204, 1205, 1204,  321,  575,
 /*  1680 */   882, 1029,  244,  274, 1040, 1028, 1547, 1241,   96, 1204,
 /*  1690 */   122,  122, 1204, 1028, 1028, 1030, 1031,   34,  123,   99,
 /*  1700 */   450,  572,  450,  428,  100, 1028, 1507,  220,  124, 1169,
 /*  1710 */   566,  196, 1422, 1188,    4,   13,  182, 1028, 1028, 1030,
 /*  1720 */   552,  189, 1169,  486,  579, 1169, 1204,  191,  192,  193,
 /*  1730 */   194,  569,  487,  408, 1428,  108, 1427, 1028, 1028, 1030,
 /*  1740 */  1031,   34,  256,   15,  494, 1430, 1204, 1205, 1204, 1204,
 /*  1750 */  1205, 1204,  124,  411,  566,  450,  200, 1188,    4, 1496,
 /*  1760 */   509,  260,  106, 1518,  515,  365,  282,  562,  262,  438,
 /*  1770 */   519,  439,  311,  447,  446,  569,  369,  268,  541,  141,
 /*  1780 */   312,  545, 1233, 1204, 1205, 1204,  546, 1235,  551, 1350,
 /*  1790 */   269, 1580,  443, 1579,  374, 1234, 1040,  233, 1394,  450,
 /*  1800 */  1594,   11,  122,  122, 1483,  381,  117, 1393,  109, 1349,
 /*  1810 */   123,  562,  450,  572,  450,  216,  529, 1028,  385,  577,
 /*  1820 */  1236,  386, 1236,  319,  388,  545, 1198,  273, 1270,  276,
 /*  1830 */   544,  275,  278,  279,  580, 1265, 1260,  415,  172,  154,
 /*  1840 */  1040,  173,  416,  174,  296,  186,  122,  122,  225, 1028,
 /*  1850 */  1028, 1030, 1031,   34,  123, 1534,  450,  572,  450,  335,
 /*  1860 */  1535, 1028,  235,  226, 1533,  455, 1532,   89,  184,   92,
 /*  1870 */    23,  460,   24,  325, 1194, 1193,  153,  217,  891,  175,
 /*  1880 */   328,  468,   94,  297,  904,  176,  124,  337,  566,  245,
 /*  1890 */   156, 1188,    4, 1028, 1028, 1030, 1031,   34, 1601, 1102,
 /*  1900 */  1192,  476,  582, 1100,  345,  188,  178,  925,  292,  569,
 /*  1910 */  1223,    2,  161,  252,  190,  255,  353, 1116,  396,  195,
 /*  1920 */   396,  179,  395,  180,  277,  433,  393,  431,  102,  197,
 /*  1930 */   103,  104,  859,  450,  105,  181,  254, 1119,  257, 1115,
 /*  1940 */   258,  163,  465, 1259,  295,  562,   26, 1341,  259,  364,
 /*  1950 */  1108,  266,  514,  248,  261,  342,  290, 1633,  201,  202,
 /*  1960 */  1155,   16, 1238,  341, 1154,  229,  285,  565,  287,  571,
 /*  1970 */   265,  204,   27,  972, 1040,  138,  978,   41, 1160,  206,
 /*  1980 */   122,  122,  948,  531, 1069,  164,  111,   28,  123,  112,
 /*  1990 */   450,  572,  450,  538,  250, 1028, 1185, 1171,  466,   29,
 /*  2000 */    30,   31,  177, 1173, 1175,   43, 1179, 1178,  113, 1083,
 /*  2010 */    32,   33,    8,  124,  114,  566, 1602, 1070, 1188,    4,
 /*  2020 */   161, 1073,  249, 1068, 1072, 1124,  119, 1028, 1028, 1030,
 /*  2030 */  1031,   34,  270,   90,  115,  566,  569,   35, 1188,    4,
 /*  2040 */   564,   17, 1033, 1617,  254,  125,  873,   36,  570,   37,
 /*  2050 */   464, 1259,  295,  578,  271, 1341,  569,  414,  935,  272,
 /*  2060 */   450,   18,  321,  575,  290, 1633,  861,  394,  846,  581,
 /*  2070 */  1256, 1256,  562, 1256, 1256,  565, 1256,  571, 1256, 1256,
 /*  2080 */   450, 1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256,
 /*  2090 */  1256, 1256,  562, 1256, 1256, 1256, 1256, 1256, 1256, 1256,
 /*  2100 */  1256, 1040, 1256, 1256, 1256, 1256,  466,  122,  122, 1256,
 /*  2110 */  1256, 1256, 1256, 1256, 1256,  123, 1256,  450,  572,  450,
 /*  2120 */  1256, 1040, 1028, 1256, 1256, 1256, 1256,  122,  122, 1256,
 /*  2130 */  1256, 1256, 1256, 1256, 1256,  123, 1256,  450,  572,  450,
 /*  2140 */  1256,  554, 1028, 1256, 1256, 1256,  124, 1256,  566, 1256,
 /*  2150 */  1256, 1188,    4, 1256, 1028, 1028, 1030, 1031,   34, 1256,
 /*  2160 */  1256,  553, 1256, 1256, 1192,  476,  582, 1256, 1256,  569,
 /*  2170 */  1256, 1256,  292, 1256, 1028, 1028, 1030, 1031,   34, 1256,
 /*  2180 */  1256, 1256,  396, 1256,  396, 1256,  395, 1256,  277, 1256,
 /*  2190 */   393, 1256, 1256,  450, 1256, 1256,  859, 1256, 1256, 1256,
 /*  2200 */  1256, 1256, 1256, 1256, 1256,  562, 1256, 1256, 1256, 1256,
 /*  2210 */  1256, 1256, 1256, 1256, 1256, 1256, 1256,  248, 1256,  342,
 /*  2220 */  1256, 1256, 1256, 1256, 1256, 1256, 1256,  341, 1256, 1256,
 /*  2230 */  1256, 1256, 1256, 1256, 1040, 1256, 1256, 1256, 1256, 1256,
 /*  2240 */   122,  122, 1256, 1256, 1256, 1256, 1256, 1256,  123, 1256,
 /*  2250 */   450,  572,  450, 1256,  476, 1028, 1256, 1256,  250, 1256,
 /*  2260 */  1256,  292, 1256, 1256, 1256, 1256,  177, 1256, 1256,   43,
 /*  2270 */  1256,  396, 1256,  396, 1256,  395, 1256,  277, 1256,  393,
 /*  2280 */  1256, 1256, 1256, 1256, 1256,  859,  249, 1028, 1028, 1030,
 /*  2290 */  1031,   34, 1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256,
 /*  2300 */  1256, 1256, 1256, 1256, 1256, 1256,  248, 1256,  342, 1256,
 /*  2310 */  1256, 1256, 1256, 1256, 1256, 1256,  341, 1256, 1256, 1256,
 /*  2320 */  1256,  414, 1256, 1256, 1256, 1256,  321,  575, 1256, 1256,
 /*  2330 */  1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256,
 /*  2340 */  1256, 1256, 1256, 1256, 1256, 1256, 1256,  250, 1256, 1256,
 /*  2350 */  1256, 1256, 1256, 1256, 1256,  177, 1256, 1256,   43, 1256,
 /*  2360 */  1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256,
 /*  2370 */  1256, 1256, 1256, 1256, 1256,  249, 1256, 1256, 1256, 1256,
 /*  2380 */  1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256,
 /*  2390 */  1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256,
 /*  2400 */  1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256, 1256,
 /*  2410 */   414, 1256, 1256, 1256, 1256,  321,  575,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */   252,  216,  217,  201,  218,  219,  220,  205,  252,  261,
 /*    10 */   262,  189,  190,    1,  253,  193,  187,  261,  262,  187,
 /*    20 */   272,  253,  274,  187,  276,  277,  241,  187,  272,  187,
 /*    30 */   274,  252,  247,  248,  249,  253,  187,  252,   38,  187,
 /*    40 */   261,  262,  306,  307,   32,  252,  261,  262,  219,  220,
 /*    50 */   221,  272,  201,  274,  261,  262,   56,  272,  187,  274,
 /*    60 */   211,  219,  220,  221,   64,  272,  230,  274,   68,   69,
 /*    70 */    70,  233,   72,   73,   74,   75,   76,   77,   78,   79,
 /*    80 */    80,   81,   82,   83,  187,  253,  254,  252,  303,  257,
 /*    90 */   226,  240,  252,  253,  254,  231,  261,  262,  246,   44,
 /*   100 */   251,  261,  262,   48,  319,  320,  321,  272,  211,  274,
 /*   110 */   288,   38,  272,  273,  274,  103,  187,  105,  278,   80,
 /*   120 */    81,   82,   83,   84,  253,  254,   30,  187,  128,  129,
 /*   130 */   130,  131,  132,  133,  134,  135,  136,  137,  138,  139,
 /*   140 */    85,   68,   69,   70,  273,   72,   73,   74,   75,   76,
 /*   150 */    77,   78,   79,   80,   81,   82,   83,  134,  135,  136,
 /*   160 */   137,  138,  139,  213,  214,  187,  187,  128,  129,  130,
 /*   170 */   131,  132,  133,  134,  135,  136,  137,  138,  139,   51,
 /*   180 */    99,   99,   86,  202,   38,  112,  145,  114,  107,  148,
 /*   190 */   149,  150,  190,  253,  254,  193,   50,  187,  143,  158,
 /*   200 */   187,  128,  129,  130,  131,  132,  133,  134,  135,  136,
 /*   210 */   137,  138,  139,   85,   68,   69,   70,  139,   72,   73,
 /*   220 */    74,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   230 */   252,  253,  254,  252,  153,  153,  154,  155,  252,  261,
 /*   240 */   262,   95,  261,  262,  294,  164,  165,  261,  262,  187,
 /*   250 */   272,  273,  274,  272,  314,  274,  316,   38,  272,  246,
 /*   260 */   274,  187,  283,  253,  254,   80,   81,   82,   83,   22,
 /*   270 */   142,  143,  144,  187,  128,  129,  130,  131,  132,  133,
 /*   280 */   134,  135,  136,  137,  138,  139,  107,   68,   69,   70,
 /*   290 */   288,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   300 */    81,   82,   83,  132,  133,  134,  135,  136,  137,  138,
 /*   310 */   139,  187,   93,  128,  129,  130,  131,  132,  133,  134,
 /*   320 */   135,  136,  137,  138,  139,  315,  316,  253,  254,   46,
 /*   330 */    38,  257,   43,   50,    9,   24,   11,  187,   46,   72,
 /*   340 */    73,   74,   75,  164,  165,  283,  187,  128,  129,  130,
 /*   350 */   131,  132,  133,  134,  135,  136,  137,  138,  139,  187,
 /*   360 */    68,   69,   70,   52,   72,   73,   74,   75,   76,   77,
 /*   370 */    78,   79,   80,   81,   82,   83,  190,   38,   67,  193,
 /*   380 */   218,  219,  220,  211,  187,  125,  128,  129,  130,  131,
 /*   390 */   132,  133,  134,  135,  136,  137,  138,  139,   85,  187,
 /*   400 */    89,  154,  155,   38,  187,  233,  234,  283,   50,  259,
 /*   410 */    99,   46,  253,  254,  147,  126,  137,  138,  139,  159,
 /*   420 */   128,  129,  130,  131,  132,  133,  134,  135,  136,  137,
 /*   430 */   138,  139,  143,   68,   69,   70,  147,   72,   73,   74,
 /*   440 */    75,   76,   77,   78,   79,   80,   81,   82,   83,  187,
 /*   450 */   253,  254,  302,  189,  257,  142,  143,  144,  145,  187,
 /*   460 */    92,  148,  149,  150,  187,  253,  254,  187,  179,  180,
 /*   470 */   181,  158,  199,  314,  288,  316,   38,  204,   85,  111,
 /*   480 */   141,  142,  187,  187,   46,  273,   85,  301,  120,   85,
 /*   490 */   278,   98,  196,  128,  129,  130,  131,  132,  133,  134,
 /*   500 */   135,  136,  137,  138,  139,  187,   68,   69,   70,  170,
 /*   510 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   81,
 /*   520 */    82,   83,  164,  165,  189,  253,  254,  310,  311,  257,
 /*   530 */   253,  254,   44,  253,  254,  142,  143,  144,  134,  187,
 /*   540 */   172,   85,  252,  142,  143,  144,  142,  143,  144,   38,
 /*   550 */   273,  261,  262,  273,   98,  278,  260,   46,  278,  295,
 /*   560 */   187,  187,  272,  299,  274,  287,  128,  129,  130,  131,
 /*   570 */   132,  133,  134,  135,  136,  137,  138,  139,  177,   68,
 /*   580 */    69,   70,   44,   72,   73,   74,   75,   76,   77,   78,
 /*   590 */    79,   80,   81,   82,   83,  317,  318,  279,  142,  143,
 /*   600 */   144,  102,  261,  262,  190,  309,  190,  193,  160,  193,
 /*   610 */   162,  163,  113,  272,  115,  274,   85,  118,  187,   85,
 /*   620 */   132,  133,   38,   85,  187,  225,  253,  254,   50,  107,
 /*   630 */   257,  187,   48,  224,  299,  226,  187,  228,   38,  128,
 /*   640 */   129,  130,  131,  132,  133,  134,  135,  136,  137,  138,
 /*   650 */   139,  187,   68,   69,   70,  187,   72,   73,   74,   75,
 /*   660 */    76,   77,   78,   79,   80,   81,   82,   83,  146,  255,
 /*   670 */   171,  255,   72,  142,  143,  144,  142,  143,  144,  187,
 /*   680 */   142,  143,  144,  187,  253,  254,  164,  165,  257,  187,
 /*   690 */   253,  254,  187,  102,  257,   38,  187,  253,  254,  187,
 /*   700 */   169,  257,  288,  169,  288,  187,  115,   54,  187,  118,
 /*   710 */   242,   58,  128,  129,  130,  131,  132,  133,  134,  135,
 /*   720 */   136,  137,  138,  139,  187,   68,   69,   70,  187,   72,
 /*   730 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   740 */    83,  141,  293,    1,   91,  253,  254,  283,  187,  253,
 /*   750 */   254,  154,  155,  175,  242,  253,  254,  157,  253,  254,
 /*   760 */    85,  242,  253,  254,  187,  273,  229,  230,   38,  273,
 /*   770 */   187,  253,  254,  187,  187,  273,   46,  187,  273,  196,
 /*   780 */   312,  313,  273,  196,   42,  128,  129,  130,  131,  132,
 /*   790 */   133,  134,  135,  136,  137,  138,  139,   46,   68,   69,
 /*   800 */    70,   50,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   810 */    80,   81,   82,   83,  253,  254,   85,  142,  143,  144,
 /*   820 */   153,  154,  155,  187,  312,  313,  261,  262,   44,   44,
 /*   830 */    46,   46,  313,  176,   50,   50,  246,  272,   50,  274,
 /*   840 */    38,   38,  187,  260,  258,  187,  187,  260,  106,   46,
 /*   850 */   187,  290,  132,  133,  196,  196,  279,  187,  128,  129,
 /*   860 */   130,  131,  132,  133,  134,  135,  136,  137,  138,  139,
 /*   870 */    60,   68,   69,   70,  143,   72,   73,   74,   75,   76,
 /*   880 */    77,   78,   79,   80,   81,   82,   83,  153,  154,  155,
 /*   890 */   102,  149,  309,  225,  134,  144,  309,   50,  187,  179,
 /*   900 */   199,  181,   92,  115,  187,  204,  118,  225,  253,  254,
 /*   910 */   100,  127,  127,  187,   38,  187,  253,  254,  260,  187,
 /*   920 */   169,  161,   46,  253,  254,  123,  166,  187,  164,  165,
 /*   930 */   287,  128,  129,  130,  131,  132,  133,  134,  135,  136,
 /*   940 */   137,  138,  139,   38,   68,   69,   70,  311,   72,   73,
 /*   950 */    74,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   960 */   317,  318,  262,   32,  253,  254,  156,  309,  309,  187,
 /*   970 */   253,  254,  272,  187,  274,  269,  187,  271,  187,  253,
 /*   980 */   254,  253,  254,  187,  187,  253,  254,   38,  187,  194,
 /*   990 */   195,  144,   24,  253,  254,  187,  187,   14,  187,   16,
 /*  1000 */   187,   18,  187,  187,  128,  129,  130,  131,  132,  133,
 /*  1010 */   134,  135,  136,  137,  138,  139,  169,   68,   69,   70,
 /*  1020 */    52,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*  1030 */    81,   82,   83,  201,  103,   67,  105,  205,  187,  253,
 /*  1040 */   254,  187,  253,  254,  253,  254,  187,  142,  187,  253,
 /*  1050 */   254,  187,  187,  187,  253,  254,  187,   89,  187,  187,
 /*  1060 */    38,  253,  254,   44,  253,  254,  253,  254,   46,  253,
 /*  1070 */   254,  187,  187,  187,  259,    7,  279,  128,  129,  130,
 /*  1080 */   131,  132,  133,  134,  135,  136,  137,  138,  139,  280,
 /*  1090 */    68,   69,   70,  187,   72,   73,   74,   75,   76,   77,
 /*  1100 */    78,   79,   80,   81,   82,   83,   87,  253,  254,   48,
 /*  1110 */   259,  187,  253,  254,  253,  254,   50,  253,  254,  253,
 /*  1120 */   254,  187,  253,  254,  253,  254,  187,  187,  187,  187,
 /*  1130 */   187,  259,  187,   38,  185,  304,  305,  253,  254,  253,
 /*  1140 */   254,  187,  187,  187,  259,  187,   85,  187,  187,  287,
 /*  1150 */   128,  129,  130,  131,  132,  133,  134,  135,  136,  137,
 /*  1160 */   138,  139,   44,   68,   69,   70,   48,   72,   73,   74,
 /*  1170 */    75,   76,   77,   78,   79,   80,   81,   82,   83,  317,
 /*  1180 */   318,  194,  195,  259,  187,   72,  187,  253,  254,  276,
 /*  1190 */   277,  187,  253,  254,  253,  254,  253,  254,  253,  254,
 /*  1200 */   187,  187,  187,  187,  143,  187,   38,  253,  254,  253,
 /*  1210 */   254,  253,  254,  253,  254,  187,   48,  187,  152,  187,
 /*  1220 */   280,  242,  280,  128,  129,  130,  131,  132,  133,  134,
 /*  1230 */   135,  136,  137,  138,  139,  280,   68,   69,   70,  171,
 /*  1240 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   81,
 /*  1250 */    82,   83,  253,  254,  141,  258,   56,  253,  254,  187,
 /*  1260 */    44,  187,   46,  242,   64,  187,  253,  254,  253,  254,
 /*  1270 */   187,  253,  254,  187,   44,  187,   46,  242,  187,   38,
 /*  1280 */   185,  253,  254,  253,  254,  253,  254,  187,  242,  175,
 /*  1290 */   176,  187,  313,   50,  280,  141,  128,  129,  130,  131,
 /*  1300 */   132,  133,  134,  135,  136,  137,  138,  139,   38,   68,
 /*  1310 */    69,   70,   50,   72,   73,   74,   75,   76,   77,   78,
 /*  1320 */    79,   80,   81,   82,   83,  253,  254,  253,  254,  141,
 /*  1330 */   176,  253,  254,  187,  313,  264,  253,  254,   50,  253,
 /*  1340 */   254,  253,  254,  127,  253,  254,  275,   46,  313,  264,
 /*  1350 */   161,  264,   38,  253,  254,  166,  264,  127,  170,  313,
 /*  1360 */   275,  184,  275,  186,   46,   48,   85,  275,   50,  128,
 /*  1370 */   129,  130,  131,  132,  133,  134,  135,  136,  137,  138,
 /*  1380 */   139,   44,   68,   69,   70,   44,   72,   73,   74,   75,
 /*  1390 */    76,   77,   78,   79,   80,   81,   82,   83,  264,   46,
 /*  1400 */    46,  146,  147,   50,   50,   14,  147,   16,  155,  275,
 /*  1410 */    46,  168,  142,  155,   50,   46,  157,   46,   85,   50,
 /*  1420 */    79,   50,   85,   38,  143,   46,   46,   44,   46,   50,
 /*  1430 */    50,  178,   50,  171,   46,   46,  178,   85,   50,   50,
 /*  1440 */   202,   44,  128,  129,  130,  131,  132,  133,  134,  135,
 /*  1450 */   136,  137,  138,  139,   69,   70,  168,   72,   73,   74,
 /*  1460 */    75,   76,   77,   78,   79,   80,   81,   82,   83,  109,
 /*  1470 */   110,   46,  171,  187,   46,   50,  143,  179,   50,  181,
 /*  1480 */   143,  179,   85,  181,  167,   46,   46,  187,  244,   50,
 /*  1490 */    50,  187,  187,   38,  187,  143,   38,  187,  187,  187,
 /*  1500 */   187,  238,  187,  187,  296,  199,   48,  187,  187,  187,
 /*  1510 */   187,  187,  187,  128,  129,  130,  131,  132,  133,  134,
 /*  1520 */   135,  136,  137,  138,  139,   70,  200,   72,   73,   74,
 /*  1530 */    75,   76,   77,   78,   79,   80,   81,   82,   83,  142,
 /*  1540 */   143,  144,  187,  187,  187,  263,  188,  207,  187,  187,
 /*  1550 */   167,  192,   38,  187,   40,  187,  222,   43,   44,  187,
 /*  1560 */   187,  187,  187,  236,  245,  188,  188,  224,  285,  188,
 /*  1570 */   285,  282,   44,  210,  188,   61,  282,   44,  250,  121,
 /*  1580 */   300,   43,  285,  128,  129,  130,  131,  132,  133,  134,
 /*  1590 */   135,  136,  137,  138,  139,  265,   38,  139,   40,   85,
 /*  1600 */   255,   43,   44,  145,  146,  147,  148,  149,  150,  151,
 /*  1610 */   266,   97,  266,   85,  285,  157,  265,  269,   85,   61,
 /*  1620 */   255,  107,  255,   85,  240,   44,  141,  240,  292,  237,
 /*  1630 */   240,  300,   74,  240,   50,  237,  237,  235,   44,  269,
 /*  1640 */   126,  265,  239,   85,  239,  237,  132,  133,   44,   90,
 /*  1650 */   256,  227,  237,  152,  140,   97,  142,  143,  144,  263,
 /*  1660 */   256,  147,  227,  222,  126,   86,  237,  237,  191,  111,
 /*  1670 */   142,  143,  144,  210,  116,  142,  143,  144,  164,  165,
 /*  1680 */   142,  143,  210,  167,  126,  147,  191,   63,  215,   85,
 /*  1690 */   132,  133,   85,  179,  180,  181,  182,  183,  140,  215,
 /*  1700 */   142,  143,  144,  191,  177,  147,  293,  176,   38,  102,
 /*  1710 */    40,   44,  286,   43,   44,  284,   68,  179,  180,  181,
 /*  1720 */   113,  203,  115,   36,   36,  118,   85,  206,  206,  206,
 /*  1730 */   206,   61,  191,  266,  286,  175,  286,  179,  180,  181,
 /*  1740 */   182,  183,  212,  284,  266,  203,  142,  143,  144,  142,
 /*  1750 */   143,  144,   38,  266,   40,   85,  203,   43,   44,  266,
 /*  1760 */   191,  212,  184,  298,   88,  297,  191,   97,  212,   48,
 /*  1770 */   197,  139,  198,  132,  133,   61,  256,  191,  171,  174,
 /*  1780 */   198,  111,  141,  142,  143,  144,  116,  146,  171,  270,
 /*  1790 */   117,  243,  108,  243,  269,  154,  126,    9,  281,   85,
 /*  1800 */   318,   44,  132,  133,  289,  191,  184,  281,  173,  270,
 /*  1810 */   140,   97,  142,  143,  144,  268,  172,  147,  267,  256,
 /*  1820 */   179,  266,  181,  291,  191,  111,   26,  212,  191,  209,
 /*  1830 */   116,  212,  209,   12,  208,  208,  208,  308,  232,  223,
 /*  1840 */   126,  232,  308,  232,  223,  305,  132,  133,  238,  179,
 /*  1850 */   180,  181,  182,  183,  140,  189,  142,  143,  144,  157,
 /*  1860 */   189,  147,  151,  238,  189,   93,  189,  189,   44,   44,
 /*  1870 */    44,   93,   44,   46,    2,    2,   46,   44,   85,   50,
 /*  1880 */    48,  122,   44,   93,   53,   62,   38,    5,   40,   30,
 /*  1890 */    32,   43,   44,  179,  180,  181,  182,  183,    0,   46,
 /*  1900 */     2,    3,    4,   46,  165,  177,  156,   40,   10,   61,
 /*  1910 */    50,  216,  217,   48,  168,  170,   32,    2,   20,  168,
 /*  1920 */    22,  156,   24,  156,   26,   62,   28,   87,   79,  177,
 /*  1930 */    79,   79,   34,   85,   79,  156,  241,  142,   59,    2,
 /*  1940 */   167,   10,  247,  248,  249,   97,   44,  252,  141,    1,
 /*  1950 */    94,   50,   66,   55,  167,   57,  261,  262,   94,  141,
 /*  1960 */    46,   44,  101,   65,  123,  167,   46,  272,   46,  274,
 /*  1970 */    59,   44,   59,  169,  126,   50,  142,   44,   46,   50,
 /*  1980 */   132,  133,  134,   48,   46,   46,   50,   59,  140,  175,
 /*  1990 */   142,  143,  144,   44,   96,  147,   46,  114,  303,   59,
 /*  2000 */    59,   59,  104,  112,  101,  107,  101,  119,   50,   46,
 /*  2010 */    59,   59,   69,   38,  168,   40,  321,   46,   43,   44,
 /*  2020 */   217,   22,  124,   46,   46,   46,   50,  179,  180,  181,
 /*  2030 */   182,  183,   44,   38,  168,   40,   61,   44,   43,   44,
 /*  2040 */    50,   44,   46,  167,  241,   44,   46,   44,   50,   44,
 /*  2050 */   247,  248,  249,   38,  167,  252,   61,  159,  161,  167,
 /*  2060 */    85,   48,  164,  165,  261,  262,   40,   30,    8,    6,
 /*  2070 */   322,  322,   97,  322,  322,  272,  322,  274,  322,  322,
 /*  2080 */    85,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2090 */   322,  322,   97,  322,  322,  322,  322,  322,  322,  322,
 /*  2100 */   322,  126,  322,  322,  322,  322,  303,  132,  133,  322,
 /*  2110 */   322,  322,  322,  322,  322,  140,  322,  142,  143,  144,
 /*  2120 */   322,  126,  147,  322,  322,  322,  322,  132,  133,  322,
 /*  2130 */   322,  322,  322,  322,  322,  140,  322,  142,  143,  144,
 /*  2140 */   322,  166,  147,  322,  322,  322,   38,  322,   40,  322,
 /*  2150 */   322,   43,   44,  322,  179,  180,  181,  182,  183,  322,
 /*  2160 */   322,  166,  322,  322,    2,    3,    4,  322,  322,   61,
 /*  2170 */   322,  322,   10,  322,  179,  180,  181,  182,  183,  322,
 /*  2180 */   322,  322,   20,  322,   22,  322,   24,  322,   26,  322,
 /*  2190 */    28,  322,  322,   85,  322,  322,   34,  322,  322,  322,
 /*  2200 */   322,  322,  322,  322,  322,   97,  322,  322,  322,  322,
 /*  2210 */   322,  322,  322,  322,  322,  322,  322,   55,  322,   57,
 /*  2220 */   322,  322,  322,  322,  322,  322,  322,   65,  322,  322,
 /*  2230 */   322,  322,  322,  322,  126,  322,  322,  322,  322,  322,
 /*  2240 */   132,  133,  322,  322,  322,  322,  322,  322,  140,  322,
 /*  2250 */   142,  143,  144,  322,    3,  147,  322,  322,   96,  322,
 /*  2260 */   322,   10,  322,  322,  322,  322,  104,  322,  322,  107,
 /*  2270 */   322,   20,  322,   22,  322,   24,  322,   26,  322,   28,
 /*  2280 */   322,  322,  322,  322,  322,   34,  124,  179,  180,  181,
 /*  2290 */   182,  183,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2300 */   322,  322,  322,  322,  322,  322,   55,  322,   57,  322,
 /*  2310 */   322,  322,  322,  322,  322,  322,   65,  322,  322,  322,
 /*  2320 */   322,  159,  322,  322,  322,  322,  164,  165,  322,  322,
 /*  2330 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2340 */   322,  322,  322,  322,  322,  322,  322,   96,  322,  322,
 /*  2350 */   322,  322,  322,  322,  322,  104,  322,  322,  107,  322,
 /*  2360 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2370 */   322,  322,  322,  322,  322,  124,  322,  322,  322,  322,
 /*  2380 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2390 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2400 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2410 */   159,  322,  322,  322,  322,  164,  165,  322,  322,  322,
 /*  2420 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2430 */   322,  322,  322,  322,  322,  322,  322,  322,  322,  322,
 /*  2440 */   322,  322,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2450 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2460 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2470 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2480 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2490 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2500 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2510 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2520 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2530 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2540 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2550 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2560 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2570 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2580 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2590 */   187,  187,  187,  187,  187,  187,  187,  187,  187,  187,
 /*  2600 */   187,  187,  187,  187,
};
#define YY_SHIFT_COUNT    (582)
#define YY_SHIFT_MIN      (0)
#define YY_SHIFT_MAX      (2251)
static const unsigned short int yy_shift_ofst[] = {
 /*     0 */  2162, 1898, 2251, 1514, 1514,  179,   81, 1558, 1670, 1714,
 /*    10 */  2108, 2108, 2108,  522,  179,  179,  179,  179,  179,    0,
 /*    20 */     0,  219, 1241, 2108, 2108, 2108, 2108, 2108, 2108, 2108,
 /*    30 */  2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108, 1641, 1641,
 /*    40 */   313, 1607, 1607,  393,  456,  675,  675,  358,  358,  358,
 /*    50 */   358,   73,  146,  292,  365,  438,  511,  584,  657,  730,
 /*    60 */   803,  876,  949, 1022, 1095, 1168, 1241, 1241, 1241, 1241,
 /*    70 */  1241, 1241, 1241, 1241, 1241, 1241, 1241, 1241, 1241, 1241,
 /*    80 */  1241, 1241, 1241, 1314, 1241, 1241, 1385, 1455, 1455, 1848,
 /*    90 */  1975, 1995, 2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108,
 /*   100 */  2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108,
 /*   110 */  2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108,
 /*   120 */  2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108,
 /*   130 */  2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108, 2108,
 /*   140 */  2108, 2108,   39,  185,  185,  185,  185,  185,  185,  185,
 /*   150 */   258,  171,   23,  128,  600,  675,  675,  675,  675,  720,
 /*   160 */   720,  742,  279,  597,  325,  325,  325,  764,   78,   78,
 /*   170 */  2417, 2417, 1458, 1458, 1458,  128,  311,  401,  538,  538,
 /*   180 */   538,  538,  311,  499,  675,  784,  785,  675,  675,  675,
 /*   190 */   675,  675,  675,  675,  675,  675,  675,  675,  675,  675,
 /*   200 */   675,  675,  675,  675,  675,  339,  675,  788,  788,  247,
 /*   210 */   591,  591,  731,  905,  905,  731, 1068, 2417, 2417, 2417,
 /*   220 */  2417, 2417, 2417, 2417, 1538,  289,  289,   41, 1397,  404,
 /*   230 */  1528,  531,  534, 1533, 1604,  675,  675,  675,  810,  810,
 /*   240 */   810,  675,  675,  675,  675,  675,  675,  675,  675,  675,
 /*   250 */   675,  675,  675,  675,   82,  675,  675,  675,  675,  675,
 /*   260 */   675,  675,  675,  675,  751,  675,  675,  675,   55,  368,
 /*   270 */   968,  675,  675,  675,  675,  675,  675,  675,  675,  675,
 /*   280 */   488,  667,  653,  448, 1061, 1061, 1061, 1061,  847,  760,
 /*   290 */   448,  448,  983,  283, 1066, 1118, 1113, 1200,   96, 1154,
 /*   300 */  1114, 1243, 1114, 1270,  578, 1154, 1154,  578, 1154, 1243,
 /*   310 */  1270, 1200, 1200, 1188, 1188, 1188, 1188, 1262, 1262, 1177,
 /*   320 */  1288, 1189, 1581, 1485, 1485, 1485, 1584, 1594, 1594, 1485,
 /*   330 */  1559, 1581, 1485, 1501, 1485, 1559, 1485, 1579, 1579, 1516,
 /*   340 */  1516, 1624, 1624, 1516, 1527, 1531, 1667, 1648, 1687, 1687,
 /*   350 */  1687, 1687, 1516, 1688, 1560, 1531, 1531, 1560, 1667, 1648,
 /*   360 */  1560, 1648, 1560, 1516, 1688, 1578, 1676, 1516, 1688, 1721,
 /*   370 */  1632, 1632, 1581, 1516, 1605, 1617, 1673, 1673, 1684, 1684,
 /*   380 */  1788, 1757, 1516, 1622, 1605, 1635, 1644, 1560, 1581, 1516,
 /*   390 */  1688, 1516, 1688, 1800, 1800, 1821, 1821, 1821, 2417, 2417,
 /*   400 */  2417, 2417, 2417, 2417, 2417, 2417, 2417, 2417, 2417, 2417,
 /*   410 */  2417, 2417, 2417,  267,   12, 1216, 1230,  734, 1337,  931,
 /*   420 */  1318, 1353, 1354, 1281, 1255, 1391, 1259,  260, 1341, 1317,
 /*   430 */  1253, 1258, 1364, 1019, 1369, 1371, 1379, 1380,  802, 1333,
 /*   440 */  1382, 1388, 1389, 1360, 1301, 1425, 1298, 1302, 1439, 1440,
 /*   450 */  1383, 1428, 1352, 1702, 1711, 1824, 1772, 1825, 1826, 1827,
 /*   460 */  1828, 1778, 1829, 1830, 1872, 1873, 1833, 1793, 1832, 1759,
 /*   470 */  1831, 1838, 1790, 1823, 1882, 1859, 1858, 1853, 1857, 1739,
 /*   480 */  1728, 1750, 1860, 1860, 1865, 1746, 1867, 1745, 1884, 1915,
 /*   490 */  1751, 1765, 1860, 1767, 1840, 1863, 1860, 1752, 1849, 1851,
 /*   500 */  1852, 1855, 1779, 1795, 1879, 1773, 1937, 1931, 1902, 1807,
 /*   510 */  1948, 1856, 1901, 1864, 1861, 1886, 1787, 1818, 1914, 1917,
 /*   520 */  1841, 1798, 1920, 1922, 1834, 1911, 1927, 1804, 1925, 1913,
 /*   530 */  1932, 1933, 1935, 1929, 1938, 1936, 1939, 1928, 1814, 1949,
 /*   540 */  1950, 1940, 1941, 1942, 1883, 1903, 1891, 1943, 1905, 1888,
 /*   550 */  1958, 1951, 1952, 1846, 1866, 1963, 1925, 1971, 1977, 1978,
 /*   560 */  1976, 1979, 1988, 1999, 1993, 1990, 1997, 1996, 2000, 2001,
 /*   570 */  2003, 1998, 1876, 1887, 1892, 2005, 1897, 2013, 2026, 2015,
 /*   580 */  2037, 2060, 2063,
};
#define YY_REDUCE_COUNT (412)
#define YY_REDUCE_MIN   (-264)
#define YY_REDUCE_MAX   (1803)
static const short yy_reduce_ofst[] = {
 /*     0 */  -215, 1695, 1803, -160,  -22, -252,  -19,  -60,   10,  159,
 /*    10 */   212,  277,  280, -244, -221, -207, -165,  -14,  290,  414,
 /*    20 */   416,  186, -178, -168,   74,  197,  272, -129,  373,  431,
 /*    30 */   437,  492,  444,  496,  561,  502,  505,  509, -171, -158,
 /*    40 */   172,  468,  512,  296,  583,  587,  658,  341,  565,  341,
 /*    50 */   565,    2,    2,    2,    2,    2,    2,    2,    2,    2,
 /*    60 */     2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
 /*    70 */     2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
 /*    80 */     2,    2,    2,    2,    2,    2,    2,    2,    2,  518,
 /*    90 */   655,  663,  670,  711,  717,  726,  728,  732,  740,  786,
 /*   100 */   789,  791,  796,  801,  808,  811,  813,  816,  854,  859,
 /*   110 */   861,  864,  866,  869,  871,  884,  886,  934,  939,  941,
 /*   120 */   943,  945,  954,  956,  958,  960,  999, 1004, 1013, 1015,
 /*   130 */  1018, 1028, 1030, 1032, 1072, 1074, 1078, 1083, 1086, 1088,
 /*   140 */  1091, 1100,    2,    2,    2,    2,    2,    2,    2,    2,
 /*   150 */     2,    2,    2,  537,  409, -151,  150,  659,  217, -214,
 /*   160 */   162,  -50,    2,  264,  278,  643,  862,  700,    2,    2,
 /*   170 */     2,    2, -136, -136, -136, -164,  273,  449,  -21,   62,
 /*   180 */   124,  464,  701,  519,  586, -264, -264, -103,  318,  577,
 /*   190 */   797,  815,  851,  872,  885, -148,  809,   13,  940,  942,
 /*   200 */   590,  955,  924,  997, 1014, -149,  636,  979, 1021,  335,
 /*   210 */  1035, 1046,  795, -198,  832,  987,  706,  831, 1071, 1085,
 /*   220 */  1087, 1092,  913, 1134, -239, -232, -218, -162,  -71,   86,
 /*   230 */   262,  295,  352,  374,  521,  541,  782,  865,  400,  668,
 /*   240 */   682,  906,  961, 1016, 1104, 1146, 1286, 1300, 1304, 1305,
 /*   250 */  1307, 1310, 1311, 1312, 1238, 1313, 1315, 1316, 1320, 1321,
 /*   260 */  1322, 1323, 1324, 1325, 1326, 1355, 1356, 1357, 1358, 1244,
 /*   270 */  1306, 1361,   86, 1362, 1366, 1368, 1372, 1373, 1374, 1375,
 /*   280 */  1263, 1208, 1319, 1359, 1377, 1378, 1381, 1386, 1326, 1282,
 /*   290 */  1359, 1359, 1340, 1327, 1334, 1328, 1343, 1345, 1363, 1283,
 /*   300 */  1289, 1330, 1294, 1280, 1344, 1285, 1297, 1346, 1329, 1351,
 /*   310 */  1331, 1365, 1367, 1384, 1387, 1390, 1393, 1348, 1370, 1336,
 /*   320 */  1376, 1396, 1394, 1392, 1398, 1399, 1402, 1403, 1405, 1408,
 /*   330 */  1424, 1404, 1415, 1441, 1429, 1435, 1430, 1463, 1472, 1477,
 /*   340 */  1495, 1473, 1484, 1512, 1413, 1426, 1431, 1518, 1521, 1522,
 /*   350 */  1523, 1524, 1541, 1530, 1467, 1448, 1450, 1478, 1459, 1542,
 /*   360 */  1487, 1553, 1493, 1569, 1549, 1465, 1468, 1575, 1556, 1573,
 /*   370 */  1574, 1582, 1520, 1586, 1519, 1525, 1548, 1550, 1517, 1526,
 /*   380 */  1482, 1515, 1614, 1532, 1539, 1547, 1551, 1555, 1563, 1633,
 /*   390 */  1615, 1637, 1619, 1620, 1623, 1626, 1627, 1628, 1606, 1609,
 /*   400 */  1616, 1529, 1534, 1540, 1621, 1610, 1625, 1611, 1666, 1671,
 /*   410 */  1675, 1677, 1678,
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
    0,  /*       SEMI => nothing */
    0,  /*      ALTER => nothing */
   85,  /*    EXPLAIN => ID */
    0,  /*        ADD => nothing */
   85,  /*      QUERY => ID */
    0,  /*     WINDOW => nothing */
   85,  /*       PLAN => ID */
    0,  /*       OVER => nothing */
   85,  /*      BEGIN => ID */
    0,  /*     FILTER => nothing */
    0,  /* TRANSACTION => nothing */
    0,  /*     COLUMN => nothing */
   85,  /*   DEFERRED => ID */
    0,  /* AGG_FUNCTION => nothing */
   85,  /*  IMMEDIATE => ID */
    0,  /* AGG_COLUMN => nothing */
   85,  /*  EXCLUSIVE => ID */
    0,  /*  TRUEFALSE => nothing */
    0,  /*     COMMIT => nothing */
    0,  /*   FUNCTION => nothing */
   85,  /*        END => ID */
    0,  /*      UPLUS => nothing */
   85,  /*   ROLLBACK => ID */
    0,  /*     UMINUS => nothing */
   85,  /*  SAVEPOINT => ID */
    0,  /*      TRUTH => nothing */
   85,  /*    RELEASE => ID */
    0,  /*   REGISTER => nothing */
    0,  /*         TO => nothing */
    0,  /*     VECTOR => nothing */
    0,  /*      TABLE => nothing */
    0,  /* SELECT_COLUMN => nothing */
    0,  /*     CREATE => nothing */
    0,  /* IF_NULL_ROW => nothing */
   85,  /*         IF => ID */
    0,  /*   ASTERISK => nothing */
    0,  /*        NOT => nothing */
    0,  /*       SPAN => nothing */
    0,  /*     EXISTS => nothing */
    0,  /*      ERROR => nothing */
   85,  /*       TEMP => ID */
    0,  /*    QNUMBER => nothing */
    0,  /*         LP => nothing */
    0,  /*      SPACE => nothing */
    0,  /*         RP => nothing */
    0,  /*    COMMENT => nothing */
    0,  /*         AS => nothing */
    0,  /*    ILLEGAL => nothing */
    0,  /*      COMMA => nothing */
   85,  /*    WITHOUT => ID */
   85,  /*      ABORT => ID */
   85,  /*     ACTION => ID */
   85,  /*      AFTER => ID */
   85,  /*    ANALYZE => ID */
   85,  /*        ASC => ID */
   85,  /*     ATTACH => ID */
   85,  /*     BEFORE => ID */
   85,  /*         BY => ID */
   85,  /*    CASCADE => ID */
   85,  /*       CAST => ID */
   85,  /*   CONFLICT => ID */
   85,  /*   DATABASE => ID */
   85,  /*       DESC => ID */
   85,  /*     DETACH => ID */
   85,  /*       EACH => ID */
   85,  /*       FAIL => ID */
    0,  /*         OR => nothing */
    0,  /*        AND => nothing */
    0,  /*         IS => nothing */
    0,  /*      ISNOT => nothing */
   85,  /*      MATCH => ID */
   85,  /*    LIKE_KW => ID */
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
   85,  /*   COLUMNKW => ID */
   85,  /*         DO => ID */
   85,  /*        FOR => ID */
   85,  /*     IGNORE => ID */
   85,  /*  INITIALLY => ID */
   85,  /*    INSTEAD => ID */
   85,  /*         NO => ID */
   85,  /*        KEY => ID */
   85,  /*         OF => ID */
   85,  /*     OFFSET => ID */
   85,  /*     PRAGMA => ID */
   85,  /*      RAISE => ID */
   85,  /*  RECURSIVE => ID */
   85,  /*    REPLACE => ID */
   85,  /*   RESTRICT => ID */
   85,  /*        ROW => ID */
   85,  /*       ROWS => ID */
   85,  /*    TRIGGER => ID */
   85,  /*     VACUUM => ID */
   85,  /*       VIEW => ID */
   85,  /*    VIRTUAL => ID */
   85,  /*       WITH => ID */
   85,  /*      NULLS => ID */
   85,  /*      FIRST => ID */
   85,  /*       LAST => ID */
   85,  /*    CURRENT => ID */
   85,  /*  FOLLOWING => ID */
   85,  /*  PARTITION => ID */
   85,  /*  PRECEDING => ID */
   85,  /*      RANGE => ID */
   85,  /*  UNBOUNDED => ID */
   85,  /*    EXCLUDE => ID */
   85,  /*     GROUPS => ID */
   85,  /*     OTHERS => ID */
   85,  /*       TIES => ID */
   85,  /*  GENERATED => ID */
   85,  /*     ALWAYS => ID */
    0,  /* MATERIALIZED => nothing */
   85,  /*    REINDEX => ID */
   85,  /*     RENAME => ID */
   85,  /*   CTIME_KW => ID */
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
    0,  /*        DOT => nothing */
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
    0,  /*      FLOAT => nothing */
    0,  /*       BLOB => nothing */
    0,  /*    INTEGER => nothing */
    0,  /*   VARIABLE => nothing */
    0,  /*       CASE => nothing */
    0,  /*       WHEN => nothing */
    0,  /*       THEN => nothing */
    0,  /*       ELSE => nothing */
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
  /*    2 */ "SEMI",
  /*    3 */ "ALTER",
  /*    4 */ "EXPLAIN",
  /*    5 */ "ADD",
  /*    6 */ "QUERY",
  /*    7 */ "WINDOW",
  /*    8 */ "PLAN",
  /*    9 */ "OVER",
  /*   10 */ "BEGIN",
  /*   11 */ "FILTER",
  /*   12 */ "TRANSACTION",
  /*   13 */ "COLUMN",
  /*   14 */ "DEFERRED",
  /*   15 */ "AGG_FUNCTION",
  /*   16 */ "IMMEDIATE",
  /*   17 */ "AGG_COLUMN",
  /*   18 */ "EXCLUSIVE",
  /*   19 */ "TRUEFALSE",
  /*   20 */ "COMMIT",
  /*   21 */ "FUNCTION",
  /*   22 */ "END",
  /*   23 */ "UPLUS",
  /*   24 */ "ROLLBACK",
  /*   25 */ "UMINUS",
  /*   26 */ "SAVEPOINT",
  /*   27 */ "TRUTH",
  /*   28 */ "RELEASE",
  /*   29 */ "REGISTER",
  /*   30 */ "TO",
  /*   31 */ "VECTOR",
  /*   32 */ "TABLE",
  /*   33 */ "SELECT_COLUMN",
  /*   34 */ "CREATE",
  /*   35 */ "IF_NULL_ROW",
  /*   36 */ "IF",
  /*   37 */ "ASTERISK",
  /*   38 */ "NOT",
  /*   39 */ "SPAN",
  /*   40 */ "EXISTS",
  /*   41 */ "ERROR",
  /*   42 */ "TEMP",
  /*   43 */ "QNUMBER",
  /*   44 */ "LP",
  /*   45 */ "SPACE",
  /*   46 */ "RP",
  /*   47 */ "COMMENT",
  /*   48 */ "AS",
  /*   49 */ "ILLEGAL",
  /*   50 */ "COMMA",
  /*   51 */ "WITHOUT",
  /*   52 */ "ABORT",
  /*   53 */ "ACTION",
  /*   54 */ "AFTER",
  /*   55 */ "ANALYZE",
  /*   56 */ "ASC",
  /*   57 */ "ATTACH",
  /*   58 */ "BEFORE",
  /*   59 */ "BY",
  /*   60 */ "CASCADE",
  /*   61 */ "CAST",
  /*   62 */ "CONFLICT",
  /*   63 */ "DATABASE",
  /*   64 */ "DESC",
  /*   65 */ "DETACH",
  /*   66 */ "EACH",
  /*   67 */ "FAIL",
  /*   68 */ "OR",
  /*   69 */ "AND",
  /*   70 */ "IS",
  /*   71 */ "ISNOT",
  /*   72 */ "MATCH",
  /*   73 */ "LIKE_KW",
  /*   74 */ "BETWEEN",
  /*   75 */ "IN",
  /*   76 */ "ISNULL",
  /*   77 */ "NOTNULL",
  /*   78 */ "NE",
  /*   79 */ "EQ",
  /*   80 */ "GT",
  /*   81 */ "LE",
  /*   82 */ "LT",
  /*   83 */ "GE",
  /*   84 */ "ESCAPE",
  /*   85 */ "ID",
  /*   86 */ "COLUMNKW",
  /*   87 */ "DO",
  /*   88 */ "FOR",
  /*   89 */ "IGNORE",
  /*   90 */ "INITIALLY",
  /*   91 */ "INSTEAD",
  /*   92 */ "NO",
  /*   93 */ "KEY",
  /*   94 */ "OF",
  /*   95 */ "OFFSET",
  /*   96 */ "PRAGMA",
  /*   97 */ "RAISE",
  /*   98 */ "RECURSIVE",
  /*   99 */ "REPLACE",
  /*  100 */ "RESTRICT",
  /*  101 */ "ROW",
  /*  102 */ "ROWS",
  /*  103 */ "TRIGGER",
  /*  104 */ "VACUUM",
  /*  105 */ "VIEW",
  /*  106 */ "VIRTUAL",
  /*  107 */ "WITH",
  /*  108 */ "NULLS",
  /*  109 */ "FIRST",
  /*  110 */ "LAST",
  /*  111 */ "CURRENT",
  /*  112 */ "FOLLOWING",
  /*  113 */ "PARTITION",
  /*  114 */ "PRECEDING",
  /*  115 */ "RANGE",
  /*  116 */ "UNBOUNDED",
  /*  117 */ "EXCLUDE",
  /*  118 */ "GROUPS",
  /*  119 */ "OTHERS",
  /*  120 */ "TIES",
  /*  121 */ "GENERATED",
  /*  122 */ "ALWAYS",
  /*  123 */ "MATERIALIZED",
  /*  124 */ "REINDEX",
  /*  125 */ "RENAME",
  /*  126 */ "CTIME_KW",
  /*  127 */ "ANY",
  /*  128 */ "BITAND",
  /*  129 */ "BITOR",
  /*  130 */ "LSHIFT",
  /*  131 */ "RSHIFT",
  /*  132 */ "PLUS",
  /*  133 */ "MINUS",
  /*  134 */ "STAR",
  /*  135 */ "SLASH",
  /*  136 */ "REM",
  /*  137 */ "CONCAT",
  /*  138 */ "PTR",
  /*  139 */ "COLLATE",
  /*  140 */ "BITNOT",
  /*  141 */ "ON",
  /*  142 */ "INDEXED",
  /*  143 */ "STRING",
  /*  144 */ "JOIN_KW",
  /*  145 */ "CONSTRAINT",
  /*  146 */ "DEFAULT",
  /*  147 */ "NULL",
  /*  148 */ "PRIMARY",
  /*  149 */ "UNIQUE",
  /*  150 */ "CHECK",
  /*  151 */ "REFERENCES",
  /*  152 */ "AUTOINCR",
  /*  153 */ "INSERT",
  /*  154 */ "DELETE",
  /*  155 */ "UPDATE",
  /*  156 */ "SET",
  /*  157 */ "DEFERRABLE",
  /*  158 */ "FOREIGN",
  /*  159 */ "DROP",
  /*  160 */ "UNION",
  /*  161 */ "ALL",
  /*  162 */ "EXCEPT",
  /*  163 */ "INTERSECT",
  /*  164 */ "SELECT",
  /*  165 */ "VALUES",
  /*  166 */ "DISTINCT",
  /*  167 */ "DOT",
  /*  168 */ "FROM",
  /*  169 */ "JOIN",
  /*  170 */ "USING",
  /*  171 */ "ORDER",
  /*  172 */ "GROUP",
  /*  173 */ "HAVING",
  /*  174 */ "LIMIT",
  /*  175 */ "WHERE",
  /*  176 */ "RETURNING",
  /*  177 */ "INTO",
  /*  178 */ "NOTHING",
  /*  179 */ "FLOAT",
  /*  180 */ "BLOB",
  /*  181 */ "INTEGER",
  /*  182 */ "VARIABLE",
  /*  183 */ "CASE",
  /*  184 */ "WHEN",
  /*  185 */ "THEN",
  /*  186 */ "ELSE",
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
