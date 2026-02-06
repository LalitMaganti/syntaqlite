// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#include "syntaqlite/tokenizer.h"

#include "src/common/synq_sqlite_defs.h"
#include "src/tokenizer/sqlite_tokenize_gen.h"

#include <stdlib.h>

struct SyntaqliteTokenizer {
    const unsigned char *source;
    uint32_t length;
    uint32_t pos;
};

SyntaqliteTokenizer *syntaqlite_tokenizer_create(void) {
    SyntaqliteTokenizer *tok =
        (SyntaqliteTokenizer *)calloc(1, sizeof(SyntaqliteTokenizer));
    return tok;
}

void syntaqlite_tokenizer_reset(SyntaqliteTokenizer *tok, const char *source,
                                uint32_t len) {
    tok->source = (const unsigned char *)source;
    tok->length = len;
    tok->pos = 0;
}

int syntaqlite_tokenizer_next(SyntaqliteTokenizer *tok,
                              SyntaqliteToken *out) {
    if (tok->pos >= tok->length) return 0;
    if (tok->source[tok->pos] == '\0') return 0;

    int tokenType;
    i64 tokenLen =
        synq_sqlite3GetToken(tok->source + tok->pos, &tokenType);
    if (tokenLen <= 0) return 0;

    out->text = (const char *)(tok->source + tok->pos);
    out->length = (uint32_t)tokenLen;
    out->type = (uint16_t)tokenType;

    tok->pos += (uint32_t)tokenLen;
    return 1;
}

void syntaqlite_tokenizer_destroy(SyntaqliteTokenizer *tok) {
    free(tok);
}
