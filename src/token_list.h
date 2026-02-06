// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Token list for collecting raw tokens during parsing.

#ifndef SYNTAQLITE_SRC_TOKEN_LIST_H
#define SYNTAQLITE_SRC_TOKEN_LIST_H

#include "src/vec.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Raw token: 8 bytes, stores offset/length/type without pointer overhead.
typedef struct SyntaqliteRawToken {
    uint32_t offset;  // Byte offset into source text
    uint16_t length;  // Token length in bytes
    uint16_t type;    // TK_* token type
} SyntaqliteRawToken;

// Growable array of raw tokens.
typedef struct SyntaqliteTokenList {
    SyntaqliteRawToken *data;
    uint32_t count;
    uint32_t capacity;
} SyntaqliteTokenList;

void syntaqlite_token_list_append(SyntaqliteTokenList *list, uint32_t offset,
                                  uint16_t length, uint16_t type);

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_SRC_TOKEN_LIST_H
