#ifndef SYNQ_SRC_PARSER_SQLITE_PARSE_DEFS_H
#define SYNQ_SRC_PARSER_SQLITE_PARSE_DEFS_H

#include "src/common/synq_sqlite_defs.h"
#include "src/parser/ast_base.h"
#include "src/parser/ast_builder_gen.h"

/* Control defines */
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

/* Parser struct definitions */
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

#endif /* SYNQ_SRC_PARSER_SQLITE_PARSE_DEFS_H */
