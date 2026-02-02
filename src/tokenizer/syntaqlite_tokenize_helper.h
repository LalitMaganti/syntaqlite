/*
** Helper header for syntaqlite tokenizer.
** DO NOT EDIT - regenerate with: python3 python/tools/extract_tokenizer.py
*/
#ifndef SYNTAQLITE_SRC_TOKENIZER_SYNTAQLITE_TOKENIZE_HELPER_H
#define SYNTAQLITE_SRC_TOKENIZER_SYNTAQLITE_TOKENIZE_HELPER_H

#include "src/tokenizer/syntaqlite_defs.h"

/* Forward declarations for functions defined in sqlite_tokenize.c */
i64 syntaqlite_sqlite3GetToken(const unsigned char *z, int *tokenType);
int syntaqlite_sqlite3AnalyzeWindowKeyword(const unsigned char *z);
int syntaqlite_sqlite3AnalyzeOverKeyword(const unsigned char *z, int lastToken);
int syntaqlite_sqlite3AnalyzeFilterKeyword(const unsigned char *z,
                                           int lastToken);

/* Stub for parser fallback - not needed for pure tokenization */
static inline int syntaqlite_sqlite3ParserFallback(int token) {
  (void)token;
  return 0;
}

#endif /* SYNTAQLITE_SRC_TOKENIZER_SYNTAQLITE_TOKENIZE_HELPER_H */
