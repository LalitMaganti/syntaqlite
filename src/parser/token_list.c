// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Token list implementation.

#include "src/parser/token_list.h"

void synq_token_list_append(SynqTokenList *list, uint32_t offset,
                                  uint16_t length, uint16_t type) {
    synq_vec_ensure(list, list->count + 1);
    list->data[list->count].offset = offset;
    list->data[list->count].length = length;
    list->data[list->count].type = type;
    list->count++;
}
