// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Token list for collecting raw tokens during parsing.

#ifndef SYNQ_SRC_PARSER_TOKEN_LIST_H
#define SYNQ_SRC_PARSER_TOKEN_LIST_H

#include "src/base/vec.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  // SYNQ_SRC_PARSER_TOKEN_LIST_H

// Compact token for internal storage: 8 bytes, offset-based.
typedef struct SynqRawToken {
    uint32_t offset;  // Byte offset into source text
    uint16_t length;  // Token length in bytes
    uint16_t type;    // SYNTAQLITE_TOKEN_* token type
} SynqRawToken;

// Growable array of raw tokens.
typedef struct SynqTokenList {
    SynqRawToken *data;
    uint32_t count;
    uint32_t capacity;
} SynqTokenList;

void synq_token_list_append(SynqTokenList *list, uint32_t offset,
                                  uint16_t length, uint16_t type);

#ifdef __cplusplus
}
#endif  // SYNQ_SRC_PARSER_TOKEN_LIST_H

#endif  // SYNQ_SRC_PARSER_TOKEN_LIST_H
