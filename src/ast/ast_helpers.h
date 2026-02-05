// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Helper functions for AST building in parser actions.

#ifndef SYNTAQLITE_AST_HELPERS_H
#define SYNTAQLITE_AST_HELPERS_H

#include "src/syntaqlite_sqlite_defs.h"
#include "src/ast/ast_nodes.h"

// Create source span from a token and parse context
static inline SyntaqliteSourceSpan token_span(SyntaqliteParseContext *ctx,
                                               SyntaqliteToken tok) {
    return syntaqlite_span((uint32_t)(tok.z - ctx->zSql), (uint16_t)tok.n);
}

#endif  // SYNTAQLITE_AST_HELPERS_H
