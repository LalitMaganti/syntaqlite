// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Token list implementation.

#include "src/token_list.h"

#include <stdlib.h>

void syntaqlite_token_list_init(SyntaqliteTokenList *list) {
    list->tokens = NULL;
    list->count = 0;
    list->capacity = 0;
}

void syntaqlite_token_list_free(SyntaqliteTokenList *list) {
    free(list->tokens);
    list->tokens = NULL;
    list->count = 0;
    list->capacity = 0;
}

void syntaqlite_token_list_append(SyntaqliteTokenList *list, uint32_t offset,
                                  uint16_t length, uint16_t type) {
    if (list->count >= list->capacity) {
        uint32_t new_capacity = list->capacity * 2 + 64;
        SyntaqliteRawToken *new_tokens = (SyntaqliteRawToken *)realloc(
            list->tokens, new_capacity * sizeof(SyntaqliteRawToken));
        if (!new_tokens) return;
        list->tokens = new_tokens;
        list->capacity = new_capacity;
    }
    list->tokens[list->count].offset = offset;
    list->tokens[list->count].length = length;
    list->tokens[list->count].type = type;
    list->count++;
}
