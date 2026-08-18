// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// AST building actions for syntaqlite grammar.
// These rules get merged with SQLite's parse.y during code generation.
//
// Rule signatures MUST match upstream parse.y exactly.
// Python tooling validates coverage and consistency.
//
// Conventions:
// - pCtx: Parse context (SynqParseContext*)
// - pCtx->zSql: Original SQL text (for computing offsets)
// - pCtx->root: Set to root node ID at input rule
// - Terminals are SynqParseToken with .z (pointer) and .n (length)
// - Non-terminals are u32 node IDs

// ============ Bind Parameters ============

expr(A) ::= VARIABLE(B). {
    // `#N` names a VM register and is only legal in a nested parse, which we
    // never do.
    if (B.n >= 2 && B.z[0] == '#' && B.z[1] >= '0' && B.z[1] <= '9') {
        pCtx->error = 1;
    }
    A = synq_parse_variable(pCtx, synq_span(pCtx, B));
}

// ============ COLLATE Expression ============

expr(A) ::= expr(B) COLLATE ids(C). {
    A = synq_parse_collate_expr(pCtx, B, synq_span(pCtx, C));
}