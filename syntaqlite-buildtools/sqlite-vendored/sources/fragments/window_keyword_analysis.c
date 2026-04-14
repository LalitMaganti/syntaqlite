/*
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
*/
static int getToken(const unsigned char **pz){
  const unsigned char *z = *pz;
  int t;                          /* Token type to return */
  do {
    z += sqlite3GetToken(z, &t);
  }while( t==TK_SPACE || t==TK_COMMENT );
  if( t==TK_ID
   || t==TK_STRING
   || t==TK_JOIN_KW
   || t==TK_WINDOW
   || t==TK_OVER
   || sqlite3ParserFallback(t)==TK_ID
  ){
    t = TK_ID;
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
static int analyzeWindowKeyword(const unsigned char *z){
  int t;
  t = getToken(&z);
  if( t!=TK_ID ) return TK_ID;
  t = getToken(&z);
  if( t!=TK_AS ) return TK_ID;
  return TK_WINDOW;
}
static int analyzeOverKeyword(const unsigned char *z, int lastToken){
  if( lastToken==TK_RP ){
    int t = getToken(&z);
    if( t==TK_LP || t==TK_ID ) return TK_OVER;
  }
  return TK_ID;
}
static int analyzeFilterKeyword(const unsigned char *z, int lastToken){
  if( lastToken==TK_RP && getToken(&z)==TK_LP ){
    return TK_FILTER;
  }
  return TK_ID;
}
