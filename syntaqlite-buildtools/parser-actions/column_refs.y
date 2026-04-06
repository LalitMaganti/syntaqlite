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

// ============ Column References ============

// Simple column reference: just a name
expr(A) ::= idj(B). {
    synq_mark_as_id(pCtx, B);
    A = synq_parse_column_ref(pCtx,
        synq_span_dequote(pCtx, B),
        SYNQ_NO_SPAN,
        SYNQ_NO_SPAN);
}

// Qualified column reference: table.column
expr(A) ::= nm(B) DOT nm(C). {
    A = synq_parse_column_ref(pCtx,
        synq_span_dequote(pCtx, C),
        synq_span_dequote(pCtx, B),
        SYNQ_NO_SPAN);
}

// Fully qualified: schema.table.column
expr(A) ::= nm(B) DOT nm(C) DOT nm(D). {
    A = synq_parse_column_ref(pCtx,
        synq_span_dequote(pCtx, D),
        synq_span_dequote(pCtx, C),
        synq_span_dequote(pCtx, B));
}