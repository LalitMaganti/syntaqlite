// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Fixed AST type definitions (not auto-generated).

#ifndef SYNTAQLITE_AST_DEFS_H
#define SYNTAQLITE_AST_DEFS_H

#include <stdint.h>

// Null node ID (used for nullable fields)
#define SYNTAQLITE_NULL_NODE 0xFFFFFFFFu

// Source location span (offset + length into source text)
typedef struct SyntaqliteSourceSpan {
    uint32_t offset;
    uint16_t length;
} SyntaqliteSourceSpan;

#endif  // SYNTAQLITE_AST_DEFS_H
