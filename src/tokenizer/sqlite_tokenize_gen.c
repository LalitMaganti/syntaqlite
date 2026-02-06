/*
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
** SQLite tokenizer for synq.
** Extracted from SQLite's tokenize.c with minimal modifications.
** DO NOT EDIT - regenerate with: python3 python/tools/extract_sqlite.py
*/

#include "src/common/synq_sqlite_defs.h"
#include "src/tokenizer/sqlite_charmap_gen.h"

/*
** Synq tokenizer injection support.
** When SYNQ_KEYWORDHASH_DATA_FILE is defined, external keyword data is used.
*/
#ifdef SYNQ_KEYWORDHASH_DATA_FILE
#include SYNQ_KEYWORDHASH_DATA_FILE
#define _SYNQ_EXTERNAL_KEYWORDHASH 1
#endif

#ifndef _SYNQ_EXTERNAL_KEYWORDHASH
#include "syntaqlite/sqlite_tokens_gen.h"
/* Hash score: 231 */
/* zKWText[] encodes 1007 bytes of keyword text in 667 bytes */
/*   REINDEXEDESCAPEACHECKEYBEFOREIGNOREGEXPLAINSTEADDATABASELECT       */
/*   ABLEFTHENDEFERRABLELSEXCLUDELETEMPORARYISNULLSAVEPOINTERSECT       */
/*   IESNOTNULLIKEXCEPTRANSACTIONATURALTERAISEXCLUSIVEXISTS             */
/*   CONSTRAINTOFFSETRIGGERANGENERATEDETACHAVINGLOBEGINNEREFERENCES     */
/*   UNIQUERYWITHOUTERELEASEATTACHBETWEENOTHINGROUPSCASCADEFAULT        */
/*   CASECOLLATECREATECURRENT_DATEIMMEDIATEJOINSERTMATCHPLANALYZE       */
/*   PRAGMATERIALIZEDEFERREDISTINCTUPDATEVALUESVIRTUALWAYSWHENWHERE     */
/*   CURSIVEABORTAFTERENAMEANDROPARTITIONAUTOINCREMENTCASTCOLUMN        */
/*   COMMITCONFLICTCROSSCURRENT_TIMESTAMPRECEDINGFAILASTFILTER          */
/*   EPLACEFIRSTFOLLOWINGFROMFULLIMITIFORDERESTRICTOTHERSOVER           */
/*   ETURNINGRIGHTROLLBACKROWSUNBOUNDEDUNIONUSINGVACUUMVIEWINDOWBY      */
/*   INITIALLYPRIMARY                                                   */
static const char synq_zKWText[666] = {
  'R','E','I','N','D','E','X','E','D','E','S','C','A','P','E','A','C','H',
  'E','C','K','E','Y','B','E','F','O','R','E','I','G','N','O','R','E','G',
  'E','X','P','L','A','I','N','S','T','E','A','D','D','A','T','A','B','A',
  'S','E','L','E','C','T','A','B','L','E','F','T','H','E','N','D','E','F',
  'E','R','R','A','B','L','E','L','S','E','X','C','L','U','D','E','L','E',
  'T','E','M','P','O','R','A','R','Y','I','S','N','U','L','L','S','A','V',
  'E','P','O','I','N','T','E','R','S','E','C','T','I','E','S','N','O','T',
  'N','U','L','L','I','K','E','X','C','E','P','T','R','A','N','S','A','C',
  'T','I','O','N','A','T','U','R','A','L','T','E','R','A','I','S','E','X',
  'C','L','U','S','I','V','E','X','I','S','T','S','C','O','N','S','T','R',
  'A','I','N','T','O','F','F','S','E','T','R','I','G','G','E','R','A','N',
  'G','E','N','E','R','A','T','E','D','E','T','A','C','H','A','V','I','N',
  'G','L','O','B','E','G','I','N','N','E','R','E','F','E','R','E','N','C',
  'E','S','U','N','I','Q','U','E','R','Y','W','I','T','H','O','U','T','E',
  'R','E','L','E','A','S','E','A','T','T','A','C','H','B','E','T','W','E',
  'E','N','O','T','H','I','N','G','R','O','U','P','S','C','A','S','C','A',
  'D','E','F','A','U','L','T','C','A','S','E','C','O','L','L','A','T','E',
  'C','R','E','A','T','E','C','U','R','R','E','N','T','_','D','A','T','E',
  'I','M','M','E','D','I','A','T','E','J','O','I','N','S','E','R','T','M',
  'A','T','C','H','P','L','A','N','A','L','Y','Z','E','P','R','A','G','M',
  'A','T','E','R','I','A','L','I','Z','E','D','E','F','E','R','R','E','D',
  'I','S','T','I','N','C','T','U','P','D','A','T','E','V','A','L','U','E',
  'S','V','I','R','T','U','A','L','W','A','Y','S','W','H','E','N','W','H',
  'E','R','E','C','U','R','S','I','V','E','A','B','O','R','T','A','F','T',
  'E','R','E','N','A','M','E','A','N','D','R','O','P','A','R','T','I','T',
  'I','O','N','A','U','T','O','I','N','C','R','E','M','E','N','T','C','A',
  'S','T','C','O','L','U','M','N','C','O','M','M','I','T','C','O','N','F',
  'L','I','C','T','C','R','O','S','S','C','U','R','R','E','N','T','_','T',
  'I','M','E','S','T','A','M','P','R','E','C','E','D','I','N','G','F','A',
  'I','L','A','S','T','F','I','L','T','E','R','E','P','L','A','C','E','F',
  'I','R','S','T','F','O','L','L','O','W','I','N','G','F','R','O','M','F',
  'U','L','L','I','M','I','T','I','F','O','R','D','E','R','E','S','T','R',
  'I','C','T','O','T','H','E','R','S','O','V','E','R','E','T','U','R','N',
  'I','N','G','R','I','G','H','T','R','O','L','L','B','A','C','K','R','O',
  'W','S','U','N','B','O','U','N','D','E','D','U','N','I','O','N','U','S',
  'I','N','G','V','A','C','U','U','M','V','I','E','W','I','N','D','O','W',
  'B','Y','I','N','I','T','I','A','L','L','Y','P','R','I','M','A','R','Y',
};
/* aKWHash[i] is the hash value for the i-th keyword */
static const unsigned char synq_aKWHash[127] = {
    84,  92, 134,  82, 105,  29,   0,   0,  94,   0,  85,  72,   0,
    53,  35,  86,  15,   0,  42,  97,  54,  89, 135,  19,   0,   0,
   140,   0,  40, 129,   0,  22, 107,   0,   9,   0,   0, 123,  80,
     0,  78,   6,   0,  65, 103, 147,   0, 136, 115,   0,   0,  48,
     0,  90,  24,   0,  17,   0,  27,  70,  23,  26,   5,  60, 142,
   110, 122,   0,  73,  91,  71, 145,  61, 120,  74,   0,  49,   0,
    11,  41,   0, 113,   0,   0,   0, 109,  10, 111, 116, 125,  14,
    50, 124,   0, 100,   0,  18, 121, 144,  56, 130, 139,  88,  83,
    37,  30, 126,   0,   0, 108,  51, 131, 128,   0,  34,   0,   0,
   132,   0,  98,  38,  39,   0,  20,  45, 117,  93,
};
/* aKWNext[] forms the hash collision chain.  If aKWHash[i]==0
** then the i-th keyword has no more hash collisions.  Otherwise,
** the next keyword with the same hash is aKWHash[i]-1. */
static const unsigned char synq_aKWNext[148] = {0,
     0,   0,   0,   0,   4,   0,  43,   0,   0, 106, 114,   0,   0,
     0,   2,   0,   0, 143,   0,   0,   0,  13,   0,   0,   0,   0,
   141,   0,   0, 119,  52,   0,   0, 137,  12,   0,   0,  62,   0,
   138,   0, 133,   0,   0,  36,   0,   0,  28,  77,   0,   0,   0,
     0,  59,   0,  47,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,  69,   0,   0,   0,   0,   0, 146,   3,   0,  58,   0,   1,
    75,   0,   0,   0,  31,   0,   0,   0,   0,   0, 127,   0, 104,
     0,  64,  66,  63,   0,   0,   0,   0,   0,  46,   0,  16,   8,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  81, 101,   0,
   112,  21,   7,  67,   0,  79,  96, 118,   0,   0,  68,   0,   0,
    99,  44,   0,  55,   0,  76,   0,  95,  32,  33,  57,  25,   0,
   102,   0,   0,  87,
};
/* aKWLen[i] is the length (in bytes) of the i-th keyword */
static const unsigned char synq_aKWLen[148] = {0,
     7,   7,   5,   4,   6,   4,   5,   3,   6,   7,   3,   6,   6,
     7,   7,   3,   8,   2,   6,   5,   4,   4,   3,  10,   4,   7,
     6,   9,   4,   2,   6,   5,   9,   9,   4,   7,   3,   2,   4,
     4,   6,  11,   6,   2,   7,   5,   5,   9,   6,  10,   4,   6,
     2,   3,   7,   5,   9,   6,   6,   4,   5,   5,  10,   6,   5,
     7,   4,   5,   7,   6,   7,   7,   6,   5,   7,   3,   7,   4,
     7,   6,  12,   9,   4,   6,   5,   4,   7,   6,  12,   8,   8,
     2,   6,   6,   7,   6,   4,   5,   9,   5,   5,   6,   3,   4,
     9,  13,   2,   2,   4,   6,   6,   8,   5,  17,  12,   7,   9,
     4,   4,   6,   7,   5,   9,   4,   4,   5,   2,   5,   8,   6,
     4,   9,   5,   8,   4,   3,   9,   5,   5,   6,   4,   6,   2,
     2,   9,   3,   7,
};
/* aKWOffset[i] is the index into zKWText[] of the start of
** the text for the i-th keyword. */
static const unsigned short int synq_aKWOffset[148] = {0,
     0,   2,   2,   8,   9,  14,  16,  20,  23,  25,  25,  29,  33,
    36,  41,  46,  48,  53,  54,  59,  62,  65,  67,  69,  78,  81,
    86,  90,  90,  94,  99, 101, 105, 111, 119, 123, 123, 123, 126,
   129, 132, 137, 142, 146, 147, 152, 156, 160, 168, 174, 181, 184,
   184, 187, 189, 195, 198, 206, 211, 216, 219, 222, 226, 236, 239,
   244, 244, 248, 252, 259, 265, 271, 277, 277, 283, 284, 288, 295,
   299, 306, 312, 324, 333, 335, 341, 346, 348, 355, 359, 370, 377,
   378, 385, 391, 397, 402, 408, 412, 415, 424, 429, 433, 439, 441,
   444, 453, 455, 457, 466, 470, 476, 482, 490, 495, 495, 495, 511,
   520, 523, 527, 532, 539, 544, 553, 557, 560, 565, 567, 571, 579,
   585, 588, 597, 602, 610, 610, 614, 623, 628, 633, 639, 642, 645,
   648, 650, 655, 659,
};
/* aKWCode[i] is the parser symbol code for the i-th keyword */
static const unsigned char synq_aKWCode[148] = {0,
  SYNTAQLITE_TOKEN_REINDEX,    SYNTAQLITE_TOKEN_INDEXED,    SYNTAQLITE_TOKEN_INDEX,      SYNTAQLITE_TOKEN_DESC,       SYNTAQLITE_TOKEN_ESCAPE,     
  SYNTAQLITE_TOKEN_EACH,       SYNTAQLITE_TOKEN_CHECK,      SYNTAQLITE_TOKEN_KEY,        SYNTAQLITE_TOKEN_BEFORE,     SYNTAQLITE_TOKEN_FOREIGN,    
  SYNTAQLITE_TOKEN_FOR,        SYNTAQLITE_TOKEN_IGNORE,     SYNTAQLITE_TOKEN_LIKE_KW,    SYNTAQLITE_TOKEN_EXPLAIN,    SYNTAQLITE_TOKEN_INSTEAD,    
  SYNTAQLITE_TOKEN_ADD,        SYNTAQLITE_TOKEN_DATABASE,   SYNTAQLITE_TOKEN_AS,         SYNTAQLITE_TOKEN_SELECT,     SYNTAQLITE_TOKEN_TABLE,      
  SYNTAQLITE_TOKEN_JOIN_KW,    SYNTAQLITE_TOKEN_THEN,       SYNTAQLITE_TOKEN_END,        SYNTAQLITE_TOKEN_DEFERRABLE, SYNTAQLITE_TOKEN_ELSE,       
  SYNTAQLITE_TOKEN_EXCLUDE,    SYNTAQLITE_TOKEN_DELETE,     SYNTAQLITE_TOKEN_TEMP,       SYNTAQLITE_TOKEN_TEMP,       SYNTAQLITE_TOKEN_OR,         
  SYNTAQLITE_TOKEN_ISNULL,     SYNTAQLITE_TOKEN_NULLS,      SYNTAQLITE_TOKEN_SAVEPOINT,  SYNTAQLITE_TOKEN_INTERSECT,  SYNTAQLITE_TOKEN_TIES,       
  SYNTAQLITE_TOKEN_NOTNULL,    SYNTAQLITE_TOKEN_NOT,        SYNTAQLITE_TOKEN_NO,         SYNTAQLITE_TOKEN_NULL,       SYNTAQLITE_TOKEN_LIKE_KW,    
  SYNTAQLITE_TOKEN_EXCEPT,     SYNTAQLITE_TOKEN_TRANSACTION,SYNTAQLITE_TOKEN_ACTION,     SYNTAQLITE_TOKEN_ON,         SYNTAQLITE_TOKEN_JOIN_KW,    
  SYNTAQLITE_TOKEN_ALTER,      SYNTAQLITE_TOKEN_RAISE,      SYNTAQLITE_TOKEN_EXCLUSIVE,  SYNTAQLITE_TOKEN_EXISTS,     SYNTAQLITE_TOKEN_CONSTRAINT, 
  SYNTAQLITE_TOKEN_INTO,       SYNTAQLITE_TOKEN_OFFSET,     SYNTAQLITE_TOKEN_OF,         SYNTAQLITE_TOKEN_SET,        SYNTAQLITE_TOKEN_TRIGGER,    
  SYNTAQLITE_TOKEN_RANGE,      SYNTAQLITE_TOKEN_GENERATED,  SYNTAQLITE_TOKEN_DETACH,     SYNTAQLITE_TOKEN_HAVING,     SYNTAQLITE_TOKEN_LIKE_KW,    
  SYNTAQLITE_TOKEN_BEGIN,      SYNTAQLITE_TOKEN_JOIN_KW,    SYNTAQLITE_TOKEN_REFERENCES, SYNTAQLITE_TOKEN_UNIQUE,     SYNTAQLITE_TOKEN_QUERY,      
  SYNTAQLITE_TOKEN_WITHOUT,    SYNTAQLITE_TOKEN_WITH,       SYNTAQLITE_TOKEN_JOIN_KW,    SYNTAQLITE_TOKEN_RELEASE,    SYNTAQLITE_TOKEN_ATTACH,     
  SYNTAQLITE_TOKEN_BETWEEN,    SYNTAQLITE_TOKEN_NOTHING,    SYNTAQLITE_TOKEN_GROUPS,     SYNTAQLITE_TOKEN_GROUP,      SYNTAQLITE_TOKEN_CASCADE,    
  SYNTAQLITE_TOKEN_ASC,        SYNTAQLITE_TOKEN_DEFAULT,    SYNTAQLITE_TOKEN_CASE,       SYNTAQLITE_TOKEN_COLLATE,    SYNTAQLITE_TOKEN_CREATE,     
  SYNTAQLITE_TOKEN_CTIME_KW,   SYNTAQLITE_TOKEN_IMMEDIATE,  SYNTAQLITE_TOKEN_JOIN,       SYNTAQLITE_TOKEN_INSERT,     SYNTAQLITE_TOKEN_MATCH,      
  SYNTAQLITE_TOKEN_PLAN,       SYNTAQLITE_TOKEN_ANALYZE,    SYNTAQLITE_TOKEN_PRAGMA,     SYNTAQLITE_TOKEN_MATERIALIZED, SYNTAQLITE_TOKEN_DEFERRED,   
  SYNTAQLITE_TOKEN_DISTINCT,   SYNTAQLITE_TOKEN_IS,         SYNTAQLITE_TOKEN_UPDATE,     SYNTAQLITE_TOKEN_VALUES,     SYNTAQLITE_TOKEN_VIRTUAL,    
  SYNTAQLITE_TOKEN_ALWAYS,     SYNTAQLITE_TOKEN_WHEN,       SYNTAQLITE_TOKEN_WHERE,      SYNTAQLITE_TOKEN_RECURSIVE,  SYNTAQLITE_TOKEN_ABORT,      
  SYNTAQLITE_TOKEN_AFTER,      SYNTAQLITE_TOKEN_RENAME,     SYNTAQLITE_TOKEN_AND,        SYNTAQLITE_TOKEN_DROP,       SYNTAQLITE_TOKEN_PARTITION,  
  SYNTAQLITE_TOKEN_AUTOINCR,   SYNTAQLITE_TOKEN_TO,         SYNTAQLITE_TOKEN_IN,         SYNTAQLITE_TOKEN_CAST,       SYNTAQLITE_TOKEN_COLUMNKW,   
  SYNTAQLITE_TOKEN_COMMIT,     SYNTAQLITE_TOKEN_CONFLICT,   SYNTAQLITE_TOKEN_JOIN_KW,    SYNTAQLITE_TOKEN_CTIME_KW,   SYNTAQLITE_TOKEN_CTIME_KW,   
  SYNTAQLITE_TOKEN_CURRENT,    SYNTAQLITE_TOKEN_PRECEDING,  SYNTAQLITE_TOKEN_FAIL,       SYNTAQLITE_TOKEN_LAST,       SYNTAQLITE_TOKEN_FILTER,     
  SYNTAQLITE_TOKEN_REPLACE,    SYNTAQLITE_TOKEN_FIRST,      SYNTAQLITE_TOKEN_FOLLOWING,  SYNTAQLITE_TOKEN_FROM,       SYNTAQLITE_TOKEN_JOIN_KW,    
  SYNTAQLITE_TOKEN_LIMIT,      SYNTAQLITE_TOKEN_IF,         SYNTAQLITE_TOKEN_ORDER,      SYNTAQLITE_TOKEN_RESTRICT,   SYNTAQLITE_TOKEN_OTHERS,     
  SYNTAQLITE_TOKEN_OVER,       SYNTAQLITE_TOKEN_RETURNING,  SYNTAQLITE_TOKEN_JOIN_KW,    SYNTAQLITE_TOKEN_ROLLBACK,   SYNTAQLITE_TOKEN_ROWS,       
  SYNTAQLITE_TOKEN_ROW,        SYNTAQLITE_TOKEN_UNBOUNDED,  SYNTAQLITE_TOKEN_UNION,      SYNTAQLITE_TOKEN_USING,      SYNTAQLITE_TOKEN_VACUUM,     
  SYNTAQLITE_TOKEN_VIEW,       SYNTAQLITE_TOKEN_WINDOW,     SYNTAQLITE_TOKEN_DO,         SYNTAQLITE_TOKEN_BY,         SYNTAQLITE_TOKEN_INITIALLY,  
  SYNTAQLITE_TOKEN_ALL,        SYNTAQLITE_TOKEN_PRIMARY,    
};
/* Hash table decoded:
**   0: INSERT
**   1: IS
**   2: ROLLBACK TRIGGER
**   3: IMMEDIATE
**   4: PARTITION
**   5: TEMP
**   6:
**   7:
**   8: VALUES WITHOUT
**   9:
**  10: MATCH
**  11: NOTHING
**  12:
**  13: OF
**  14: TIES IGNORE
**  15: PLAN
**  16: INSTEAD INDEXED
**  17:
**  18: TRANSACTION RIGHT
**  19: WHEN
**  20: SET HAVING
**  21: MATERIALIZED IF
**  22: ROWS
**  23: SELECT
**  24:
**  25:
**  26: VACUUM SAVEPOINT
**  27:
**  28: LIKE UNION VIRTUAL REFERENCES
**  29: RESTRICT
**  30:
**  31: THEN REGEXP
**  32: TO
**  33:
**  34: BEFORE
**  35:
**  36:
**  37: FOLLOWING COLLATE CASCADE
**  38: CREATE
**  39:
**  40: CASE REINDEX
**  41: EACH
**  42:
**  43: QUERY
**  44: AND ADD
**  45: PRIMARY ANALYZE
**  46:
**  47: ROW ASC DETACH
**  48: CURRENT_TIME CURRENT_DATE
**  49:
**  50:
**  51: EXCLUSIVE TEMPORARY
**  52:
**  53: DEFERRED
**  54: DEFERRABLE
**  55:
**  56: DATABASE
**  57:
**  58: DELETE VIEW GENERATED
**  59: ATTACH
**  60: END
**  61: EXCLUDE
**  62: ESCAPE DESC
**  63: GLOB
**  64: WINDOW ELSE
**  65: COLUMN
**  66: FIRST
**  67:
**  68: GROUPS ALL
**  69: DISTINCT DROP KEY
**  70: BETWEEN
**  71: INITIALLY
**  72: BEGIN
**  73: FILTER CHECK ACTION
**  74: GROUP INDEX
**  75:
**  76: EXISTS DEFAULT
**  77:
**  78: FOR CURRENT_TIMESTAMP
**  79: EXCEPT
**  80:
**  81: CROSS
**  82:
**  83:
**  84:
**  85: CAST
**  86: FOREIGN AUTOINCREMENT
**  87: COMMIT
**  88: CURRENT AFTER ALTER
**  89: FULL FAIL CONFLICT
**  90: EXPLAIN
**  91: CONSTRAINT
**  92: FROM ALWAYS
**  93:
**  94: ABORT
**  95:
**  96: AS DO
**  97: REPLACE WITH RELEASE
**  98: BY RENAME
**  99: RANGE RAISE
** 100: OTHERS
** 101: USING NULLS
** 102: PRAGMA
** 103: JOIN ISNULL OFFSET
** 104: NOT
** 105: OR LAST LEFT
** 106: LIMIT
** 107:
** 108:
** 109: IN
** 110: INTO
** 111: OVER RECURSIVE
** 112: ORDER OUTER
** 113:
** 114: INTERSECT UNBOUNDED
** 115:
** 116:
** 117: RETURNING ON
** 118:
** 119: WHERE
** 120: NO INNER
** 121: NULL
** 122:
** 123: TABLE
** 124: NATURAL NOTNULL
** 125: PRECEDING
** 126: UPDATE UNIQUE
*/
/* Check to see if z[0..n-1] is a keyword. If it is, write the
** parser symbol code for that keyword into *pType.  Always
** return the integer n (the length of the token). */
#endif /* _SYNQ_EXTERNAL_KEYWORDHASH */

/* Stub for parser fallback - not needed for pure tokenization */
static inline int synq_sqlite3ParserFallback(int token) {
  (void)token;
  return 0;
}

/* Forward declaration */
i64 synq_sqlite3GetToken(const unsigned char *z, int *tokenType);

/* Keyword lookup function */
int synq_keywordCode(const char *z, int n, int *pType){
  int i, j;
  const char *zKW;
  assert( n>=2 );
  i = ((synq_sqlite3Tolower(z[0])*4) ^ (synq_sqlite3Tolower(z[n-1])*3) ^ n*1) % 127;
  for(i=(int)synq_aKWHash[i]; i>0; i=synq_aKWNext[i]){
    if( synq_aKWLen[i]!=n ) continue;
    zKW = &synq_zKWText[synq_aKWOffset[i]];
#ifdef SQLITE_ASCII
    if( (z[0]&~0x20)!=zKW[0] ) continue;
    if( (z[1]&~0x20)!=zKW[1] ) continue;
    j = 2;
    while( j<n && (z[j]&~0x20)==zKW[j] ){ j++; }
#endif
#ifdef SQLITE_EBCDIC
    if( toupper(z[0])!=zKW[0] ) continue;
    if( toupper(z[1])!=zKW[1] ) continue;
    j = 2;
    while( j<n && toupper(z[j])==zKW[j] ){ j++; }
#endif
    if( j<n ) continue;
    *pType = synq_aKWCode[i];
    break;
  }
  return n;
}


/* Character classes for tokenizing
**
** In the sqlite3GetToken() function, a switch() on aiClass[c] is implemented
** using a lookup table, whereas a switch() directly on c uses a binary search.
** The lookup table is much faster.  To maximize speed, and to ensure that
** a lookup table is used, all of the classes need to be small integers and
** all of them need to be used within the switch.
*/
#define CC_X          0    /* The letter 'x', or start of BLOB literal */
#define CC_KYWD0      1    /* First letter of a keyword */
#define CC_KYWD       2    /* Alphabetics or '_'.  Usable in a keyword */
#define CC_DIGIT      3    /* Digits */
#define CC_DOLLAR     4    /* '$' */
#define CC_VARALPHA   5    /* '@', '#', ':'.  Alphabetic SQL variables */
#define CC_VARNUM     6    /* '?'.  Numeric SQL variables */
#define CC_SPACE      7    /* Space characters */
#define CC_QUOTE      8    /* '"', '\'', or '`'.  String literals, quoted ids */
#define CC_QUOTE2     9    /* '['.   [...] style quoted ids */
#define CC_PIPE      10    /* '|'.   Bitwise OR or concatenate */
#define CC_MINUS     11    /* '-'.  Minus or SQL-style comment */
#define CC_LT        12    /* '<'.  Part of < or <= or <> */
#define CC_GT        13    /* '>'.  Part of > or >= */
#define CC_EQ        14    /* '='.  Part of = or == */
#define CC_BANG      15    /* '!'.  Part of != */
#define CC_SLASH     16    /* '/'.  / or c-style comment */
#define CC_LP        17    /* '(' */
#define CC_RP        18    /* ')' */
#define CC_SEMI      19    /* ';' */
#define CC_PLUS      20    /* '+' */
#define CC_STAR      21    /* '*' */
#define CC_PERCENT   22    /* '%' */
#define CC_COMMA     23    /* ',' */
#define CC_AND       24    /* '&' */
#define CC_TILDA     25    /* '~' */
#define CC_DOT       26    /* '.' */
#define CC_ID        27    /* unicode characters usable in IDs */
#define CC_ILLEGAL   28    /* Illegal character */
#define CC_NUL       29    /* 0x00 */
#define CC_BOM       30    /* First byte of UTF8 BOM:  0xEF 0xBB 0xBF */

static const unsigned char aiClass[] = {
#ifdef SQLITE_ASCII
/*         x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  xa  xb  xc  xd  xe  xf */
/* 0x */   29, 28, 28, 28, 28, 28, 28, 28, 28,  7,  7, 28,  7,  7, 28, 28,
/* 1x */   28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
/* 2x */    7, 15,  8,  5,  4, 22, 24,  8, 17, 18, 21, 20, 23, 11, 26, 16,
/* 3x */    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  5, 19, 12, 14, 13,  6,
/* 4x */    5,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 5x */    1,  1,  1,  1,  1,  1,  1,  1,  0,  2,  2,  9, 28, 28, 28,  2,
/* 6x */    8,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 7x */    1,  1,  1,  1,  1,  1,  1,  1,  0,  2,  2, 28, 10, 28, 25, 28,
/* 8x */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
/* 9x */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
/* Ax */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
/* Bx */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
/* Cx */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
/* Dx */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
/* Ex */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 30,
/* Fx */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27
#endif
#ifdef SQLITE_EBCDIC
/*         x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  xa  xb  xc  xd  xe  xf */
/* 0x */   29, 28, 28, 28, 28,  7, 28, 28, 28, 28, 28, 28,  7,  7, 28, 28,
/* 1x */   28, 28, 28, 28, 28,  7, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
/* 2x */   28, 28, 28, 28, 28,  7, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
/* 3x */   28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
/* 4x */    7, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 26, 12, 17, 20, 10,
/* 5x */   24, 28, 28, 28, 28, 28, 28, 28, 28, 28, 15,  4, 21, 18, 19, 28,
/* 6x */   11, 16, 28, 28, 28, 28, 28, 28, 28, 28, 28, 23, 22,  2, 13,  6,
/* 7x */   28, 28, 28, 28, 28, 28, 28, 28, 28,  8,  5,  5,  5,  8, 14,  8,
/* 8x */   28,  1,  1,  1,  1,  1,  1,  1,  1,  1, 28, 28, 28, 28, 28, 28,
/* 9x */   28,  1,  1,  1,  1,  1,  1,  1,  1,  1, 28, 28, 28, 28, 28, 28,
/* Ax */   28, 25,  1,  1,  1,  1,  1,  0,  2,  2, 28, 28, 28, 28, 28, 28,
/* Bx */   28, 28, 28, 28, 28, 28, 28, 28, 28, 28,  9, 28, 28, 28, 28, 28,
/* Cx */   28,  1,  1,  1,  1,  1,  1,  1,  1,  1, 28, 28, 28, 28, 28, 28,
/* Dx */   28,  1,  1,  1,  1,  1,  1,  1,  1,  1, 28, 28, 28, 28, 28, 28,
/* Ex */   28, 28,  1,  1,  1,  1,  1,  0,  2,  2, 28, 28, 28, 28, 28, 28,
/* Fx */    3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 28, 28, 28, 28, 28, 28,
#endif
};

/*
** The charMap() macro maps alphabetic characters (only) into their
** lower-case ASCII equivalent.  On ASCII machines, this is just
** an upper-to-lower case map.  On EBCDIC machines we also need
** to adjust the encoding.  The mapping is only valid for alphabetics
** which are the only characters for which this feature is used. 
**
** Used by keywordhash.h
*/
#ifdef SQLITE_ASCII
#endif
#ifdef SQLITE_EBCDIC
const unsigned char ebcdicToAscii[] = {
/* 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 0x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 1x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 2x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 3x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 4x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 5x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 95,  0,  0,  /* 6x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 7x */
   0, 97, 98, 99,100,101,102,103,104,105,  0,  0,  0,  0,  0,  0,  /* 8x */
   0,106,107,108,109,110,111,112,113,114,  0,  0,  0,  0,  0,  0,  /* 9x */
   0,  0,115,116,117,118,119,120,121,122,  0,  0,  0,  0,  0,  0,  /* Ax */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* Bx */
   0, 97, 98, 99,100,101,102,103,104,105,  0,  0,  0,  0,  0,  0,  /* Cx */
   0,106,107,108,109,110,111,112,113,114,  0,  0,  0,  0,  0,  0,  /* Dx */
   0,  0,115,116,117,118,119,120,121,122,  0,  0,  0,  0,  0,  0,  /* Ex */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* Fx */
};
#endif

/*
** The sqlite3KeywordCode function looks up an identifier to determine if
** it is a keyword.  If it is a keyword, the token code of that keyword is 
** returned.  If the input is not a keyword, TK_ID is returned.
**
** The implementation of this routine was generated by a program,
** mkkeywordhash.c, located in the tool subdirectory of the distribution.
** The output of the mkkeywordhash.c program is written into a file
** named keywordhash.h and then included into this source file by
** the #include below.
*/


/*
** If X is a character that can be used in an identifier then
** IdChar(X) will be true.  Otherwise it is false.
**
** For ASCII, any character with the high-order bit set is
** allowed in an identifier.  For 7-bit characters, 
** sqlite3IsIdChar[X] must be 1.
**
** For EBCDIC, the rules are more complex but have the same
** end result.
**
** Ticket #1066.  the SQL standard does not allow '$' in the
** middle of identifiers.  But many SQL implementations do. 
** SQLite will allow '$' in identifiers for compatibility.
** But the feature is undocumented.
*/
#ifdef SQLITE_ASCII
#define IdChar(C)  ((synq_sqlite3CtypeMap[(unsigned char)C]&0x46)!=0)
#endif
#ifdef SQLITE_EBCDIC
const char synq_sqlite3IsEbcdicIdChar[] = {
/* x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF */
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  /* 4x */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0,  /* 5x */
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0,  /* 6x */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,  /* 7x */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0,  /* 8x */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0,  /* 9x */
    1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0,  /* Ax */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* Bx */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,  /* Cx */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,  /* Dx */
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,  /* Ex */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0,  /* Fx */
};
#define IdChar(C)  (((c=C)>=0x42 && synq_sqlite3IsEbcdicIdChar[c-0x40]))
#endif

/* Make the IdChar function accessible from ctime.c and alter.c */
int synq_sqlite3IsIdChar(u8 c){ return IdChar(c); }

#ifndef SQLITE_OMIT_WINDOWFUNC
/*
** Return the id of the next token in string (*pz). Before returning, set
** (*pz) to point to the byte following the parsed token.
*/
static int getToken(const unsigned char **pz){
  const unsigned char *z = *pz;
  int t;                          /* Token type to return */
  do {
    z += synq_sqlite3GetToken(z, &t);
  }while( t==SYNTAQLITE_TOKEN_SPACE || t==SYNTAQLITE_TOKEN_COMMENT );
  if( t==SYNTAQLITE_TOKEN_ID 
   || t==SYNTAQLITE_TOKEN_STRING 
   || t==SYNTAQLITE_TOKEN_JOIN_KW 
   || t==SYNTAQLITE_TOKEN_WINDOW 
   || t==SYNTAQLITE_TOKEN_OVER 
   || synq_sqlite3ParserFallback(t)==SYNTAQLITE_TOKEN_ID 
  ){
    t = SYNTAQLITE_TOKEN_ID;
  }
  *pz = z;
  return t;
}

/*
** The following three functions are called immediately after the tokenizer
** reads the keywords WINDOW, OVER and FILTER, respectively, to determine
** whether the token should be treated as a keyword or an SQL identifier.
** This cannot be handled by the usual lemon %fallback method, due to
** the ambiguity in some constructions. e.g.
**
**   SELECT sum(x) OVER ...
**
** In the above, "OVER" might be a keyword, or it might be an alias for the 
** sum(x) expression. If a "%fallback ID OVER" directive were added to 
** grammar, then SQLite would always treat "OVER" as an alias, making it
** impossible to call a window-function without a FILTER clause.
**
** WINDOW is treated as a keyword if:
**
**   * the following token is an identifier, or a keyword that can fallback
**     to being an identifier, and
**   * the token after than one is TK_AS.
**
** OVER is a keyword if:
**
**   * the previous token was TK_RP, and
**   * the next token is either TK_LP or an identifier.
**
** FILTER is a keyword if:
**
**   * the previous token was TK_RP, and
**   * the next token is TK_LP.
*/
int synq_sqlite3AnalyzeWindowKeyword(const unsigned char *z){
  int t;
  t = getToken(&z);
  if( t!=SYNTAQLITE_TOKEN_ID ) return SYNTAQLITE_TOKEN_ID;
  t = getToken(&z);
  if( t!=SYNTAQLITE_TOKEN_AS ) return SYNTAQLITE_TOKEN_ID;
  return SYNTAQLITE_TOKEN_WINDOW;
}
int synq_sqlite3AnalyzeOverKeyword(const unsigned char *z, int lastToken){
  if( lastToken==SYNTAQLITE_TOKEN_RP ){
    int t = getToken(&z);
    if( t==SYNTAQLITE_TOKEN_LP || t==SYNTAQLITE_TOKEN_ID ) return SYNTAQLITE_TOKEN_OVER;
  }
  return SYNTAQLITE_TOKEN_ID;
}
int synq_sqlite3AnalyzeFilterKeyword(const unsigned char *z, int lastToken){
  if( lastToken==SYNTAQLITE_TOKEN_RP && getToken(&z)==SYNTAQLITE_TOKEN_LP ){
    return SYNTAQLITE_TOKEN_FILTER;
  }
  return SYNTAQLITE_TOKEN_ID;
}
#endif /* SQLITE_OMIT_WINDOWFUNC */

/*
** Return the length (in bytes) of the token that begins at z[0]. 
** Store the token type in *tokenType before returning.
*/
i64 synq_sqlite3GetToken(const unsigned char *z, int *tokenType){
  i64 i;
  int c;
  switch( aiClass[*z] ){  /* Switch on the character-class of the first byte
                          ** of the token. See the comment on the CC_ defines
                          ** above. */
    case CC_SPACE: {
      for(i=1; synq_sqlite3Isspace(z[i]); i++){}
      *tokenType = SYNTAQLITE_TOKEN_SPACE;
      return i;
    }
    case CC_MINUS: {
      if( z[1]=='-' ){
        for(i=2; (c=z[i])!=0 && c!='\n'; i++){}
        *tokenType = SYNTAQLITE_TOKEN_COMMENT;
        return i;
      }else if( z[1]=='>' ){
        *tokenType = SYNTAQLITE_TOKEN_PTR;
        return 2 + (z[2]=='>');
      }
      *tokenType = SYNTAQLITE_TOKEN_MINUS;
      return 1;
    }
    case CC_LP: {
      *tokenType = SYNTAQLITE_TOKEN_LP;
      return 1;
    }
    case CC_RP: {
      *tokenType = SYNTAQLITE_TOKEN_RP;
      return 1;
    }
    case CC_SEMI: {
      *tokenType = SYNTAQLITE_TOKEN_SEMI;
      return 1;
    }
    case CC_PLUS: {
      *tokenType = SYNTAQLITE_TOKEN_PLUS;
      return 1;
    }
    case CC_STAR: {
      *tokenType = SYNTAQLITE_TOKEN_STAR;
      return 1;
    }
    case CC_SLASH: {
      if( z[1]!='*' || z[2]==0 ){
        *tokenType = SYNTAQLITE_TOKEN_SLASH;
        return 1;
      }
      for(i=3, c=z[2]; (c!='*' || z[i]!='/') && (c=z[i])!=0; i++){}
      if( c ) i++;
      *tokenType = SYNTAQLITE_TOKEN_COMMENT;
      return i;
    }
    case CC_PERCENT: {
      *tokenType = SYNTAQLITE_TOKEN_REM;
      return 1;
    }
    case CC_EQ: {
      *tokenType = SYNTAQLITE_TOKEN_EQ;
      return 1 + (z[1]=='=');
    }
    case CC_LT: {
      if( (c=z[1])=='=' ){
        *tokenType = SYNTAQLITE_TOKEN_LE;
        return 2;
      }else if( c=='>' ){
        *tokenType = SYNTAQLITE_TOKEN_NE;
        return 2;
      }else if( c=='<' ){
        *tokenType = SYNTAQLITE_TOKEN_LSHIFT;
        return 2;
      }else{
        *tokenType = SYNTAQLITE_TOKEN_LT;
        return 1;
      }
    }
    case CC_GT: {
      if( (c=z[1])=='=' ){
        *tokenType = SYNTAQLITE_TOKEN_GE;
        return 2;
      }else if( c=='>' ){
        *tokenType = SYNTAQLITE_TOKEN_RSHIFT;
        return 2;
      }else{
        *tokenType = SYNTAQLITE_TOKEN_GT;
        return 1;
      }
    }
    case CC_BANG: {
      if( z[1]!='=' ){
        *tokenType = SYNTAQLITE_TOKEN_ILLEGAL;
        return 1;
      }else{
        *tokenType = SYNTAQLITE_TOKEN_NE;
        return 2;
      }
    }
    case CC_PIPE: {
      if( z[1]!='|' ){
        *tokenType = SYNTAQLITE_TOKEN_BITOR;
        return 1;
      }else{
        *tokenType = SYNTAQLITE_TOKEN_CONCAT;
        return 2;
      }
    }
    case CC_COMMA: {
      *tokenType = SYNTAQLITE_TOKEN_COMMA;
      return 1;
    }
    case CC_AND: {
      *tokenType = SYNTAQLITE_TOKEN_BITAND;
      return 1;
    }
    case CC_TILDA: {
      *tokenType = SYNTAQLITE_TOKEN_BITNOT;
      return 1;
    }
    case CC_QUOTE: {
      int delim = z[0];
      for(i=1; (c=z[i])!=0; i++){
        if( c==delim ){
          if( z[i+1]==delim ){
            i++;
          }else{
            break;
          }
        }
      }
      if( c=='\'' ){
        *tokenType = SYNTAQLITE_TOKEN_STRING;
        return i+1;
      }else if( c!=0 ){
        *tokenType = SYNTAQLITE_TOKEN_ID;
        return i+1;
      }else{
        *tokenType = SYNTAQLITE_TOKEN_ILLEGAL;
        return i;
      }
    }
    case CC_DOT: {
#ifndef SQLITE_OMIT_FLOATING_POINT
      if( !synq_sqlite3Isdigit(z[1]) )
#endif
      {
        *tokenType = SYNTAQLITE_TOKEN_DOT;
        return 1;
      }
      /* If the next character is a digit, this is a floating point
      ** number that begins with ".".  Fall thru into the next case */
      /* no break */ deliberate_fall_through
    }
    case CC_DIGIT: {
      *tokenType = SYNTAQLITE_TOKEN_INTEGER;
#ifndef SQLITE_OMIT_HEX_INTEGER
      if( z[0]=='0' && (z[1]=='x' || z[1]=='X') && synq_sqlite3Isxdigit(z[2]) ){
        for(i=3; 1; i++){
          if( synq_sqlite3Isxdigit(z[i])==0 ){
            if( z[i]==SQLITE_DIGIT_SEPARATOR ){
              *tokenType = SYNTAQLITE_TOKEN_QNUMBER;
            }else{
              break;
            }
          }
        }
      }else
#endif
        {
        for(i=0; 1; i++){
          if( synq_sqlite3Isdigit(z[i])==0 ){
            if( z[i]==SQLITE_DIGIT_SEPARATOR ){
              *tokenType = SYNTAQLITE_TOKEN_QNUMBER;
            }else{
              break;
            }
          }
        }
#ifndef SQLITE_OMIT_FLOATING_POINT
        if( z[i]=='.' ){
          if( *tokenType==SYNTAQLITE_TOKEN_INTEGER ) *tokenType = SYNTAQLITE_TOKEN_FLOAT;
          for(i++; 1; i++){
            if( synq_sqlite3Isdigit(z[i])==0 ){
              if( z[i]==SQLITE_DIGIT_SEPARATOR ){
                *tokenType = SYNTAQLITE_TOKEN_QNUMBER;
              }else{
                break;
              }
            }
          }
        }
        if( (z[i]=='e' || z[i]=='E') &&
             ( synq_sqlite3Isdigit(z[i+1]) 
              || ((z[i+1]=='+' || z[i+1]=='-') && synq_sqlite3Isdigit(z[i+2]))
             )
        ){
          if( *tokenType==SYNTAQLITE_TOKEN_INTEGER ) *tokenType = SYNTAQLITE_TOKEN_FLOAT;
          for(i+=2; 1; i++){
            if( synq_sqlite3Isdigit(z[i])==0 ){
              if( z[i]==SQLITE_DIGIT_SEPARATOR ){
                *tokenType = SYNTAQLITE_TOKEN_QNUMBER;
              }else{
                break;
              }
            }
          }
        }
#endif
      }
      while( IdChar(z[i]) ){
        *tokenType = SYNTAQLITE_TOKEN_ILLEGAL;
        i++;
      }
      return i;
    }
    case CC_QUOTE2: {
      for(i=1, c=z[0]; c!=']' && (c=z[i])!=0; i++){}
      *tokenType = c==']' ? SYNTAQLITE_TOKEN_ID : SYNTAQLITE_TOKEN_ILLEGAL;
      return i;
    }
    case CC_VARNUM: {
      *tokenType = SYNTAQLITE_TOKEN_VARIABLE;
      for(i=1; synq_sqlite3Isdigit(z[i]); i++){}
      return i;
    }
    case CC_DOLLAR:
    case CC_VARALPHA: {
      int n = 0;
      *tokenType = SYNTAQLITE_TOKEN_VARIABLE;
      for(i=1; (c=z[i])!=0; i++){
        if( IdChar(c) ){
          n++;
#ifndef SQLITE_OMIT_TCL_VARIABLE
        }else if( c=='(' && n>0 ){
          do{
            i++;
          }while( (c=z[i])!=0 && !synq_sqlite3Isspace(c) && c!=')' );
          if( c==')' ){
            i++;
          }else{
            *tokenType = SYNTAQLITE_TOKEN_ILLEGAL;
          }
          break;
        }else if( c==':' && z[i+1]==':' ){
          i++;
#endif
        }else{
          break;
        }
      }
      if( n==0 ) *tokenType = SYNTAQLITE_TOKEN_ILLEGAL;
      return i;
    }
    case CC_KYWD0: {
      if( aiClass[z[1]]>CC_KYWD ){ i = 1;  break; }
      for(i=2; aiClass[z[i]]<=CC_KYWD; i++){}
      if( IdChar(z[i]) ){
        /* This token started out using characters that can appear in keywords,
        ** but z[i] is a character not allowed within keywords, so this must
        ** be an identifier instead */
        i++;
        break;
      }
      *tokenType = SYNTAQLITE_TOKEN_ID;
      return synq_keywordCode((char*)z, i, tokenType);
    }
    case CC_X: {
#ifndef SQLITE_OMIT_BLOB_LITERAL
      if( z[1]=='\'' ){
        *tokenType = SYNTAQLITE_TOKEN_BLOB;
        for(i=2; synq_sqlite3Isxdigit(z[i]); i++){}
        if( z[i]!='\'' || i%2 ){
          *tokenType = SYNTAQLITE_TOKEN_ILLEGAL;
          while( z[i] && z[i]!='\'' ){ i++; }
        }
        if( z[i] ) i++;
        return i;
      }
#endif
      /* If it is not a BLOB literal, then it must be an ID, since no
      ** SQL keywords start with the letter 'x'.  Fall through */
      /* no break */ deliberate_fall_through
    }
    case CC_KYWD:
    case CC_ID: {
      i = 1;
      break;
    }
    case CC_BOM: {
      if( z[1]==0xbb && z[2]==0xbf ){
        *tokenType = SYNTAQLITE_TOKEN_SPACE;
        return 3;
      }
      i = 1;
      break;
    }
    case CC_NUL: {
      *tokenType = SYNTAQLITE_TOKEN_ILLEGAL;
      return 0;
    }
    default: {
      *tokenType = SYNTAQLITE_TOKEN_ILLEGAL;
      return 1;
    }
  }
  while( IdChar(z[i]) ){ i++; }
  *tokenType = SYNTAQLITE_TOKEN_ID;
  return i;
}
