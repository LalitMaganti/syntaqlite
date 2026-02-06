// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Declarations for the Lemon-generated parser (sqlite_parse_gen.c).
// DO NOT EDIT - regenerate with: python3 python/tools/extract_sqlite.py

#ifndef SYNQ_SRC_PARSER_SQLITE_PARSE_GEN_H
#define SYNQ_SRC_PARSER_SQLITE_PARSE_GEN_H

#include "src/common/synq_sqlite_defs.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Allocate a new parser. pCtx is stored inside the parser struct.
void *synq_sqlite3ParserAlloc(void *(*mallocProc)(size_t),
                              SynqParseContext *pCtx);

// Initialize a pre-allocated parser.
void synq_sqlite3ParserInit(void *yypRawParser, SynqParseContext *pCtx);

// Feed a token to the parser.
void synq_sqlite3Parser(void *yyp, int yymajor, SynqToken yyminor);

// Release secondary allocations without freeing the parser itself.
void synq_sqlite3ParserFinalize(void *p);

// Free a parser previously allocated with synq_sqlite3ParserAlloc.
void synq_sqlite3ParserFree(void *p, void (*freeProc)(void *));

// Return the fallback token for iToken, or 0 if none.
int synq_sqlite3ParserFallback(int iToken);

// Enable parser tracing (debug builds only).
#ifndef NDEBUG
void synq_sqlite3ParserTrace(FILE *TraceFILE, char *zTracePrompt);
#endif

#ifdef __cplusplus
}
#endif

#endif // SYNQ_SRC_PARSER_SQLITE_PARSE_GEN_H
