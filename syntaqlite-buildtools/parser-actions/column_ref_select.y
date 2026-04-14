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

// ============ Table-qualified star in result columns ============

// table.* in result columns
selcollist(A) ::= sclp(B) scanpt nm(C) DOT STAR. {
    uint32_t expr = synq_parse_ident_name(pCtx, synq_span(pCtx, C));
    uint32_t col = synq_parse_result_column(pCtx, (SyntaqliteResultColumnFlags){.raw = 0x01}, SYNTAQLITE_NULL_NODE, expr);
    A = synq_parse_result_column_list(pCtx, B, col);
}
