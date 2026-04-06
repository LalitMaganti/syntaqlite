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

%type nm {SynqParseToken}
%type nmorerr {uint32_t}

// ============ Identifiers ============

nm(A) ::= idj(B). {
    synq_mark_as_id(pCtx, B);
    A = B;
}

nm(A) ::= STRING(B). {
    A = B;
}

// Localized name recovery wrapper used at selected grammar sites.
nmorerr(A) ::= nm(B). {
    A = synq_parse_ident_name(pCtx, synq_span_dequote(pCtx, B));
}

nmorerr(A) ::= error. {
    A = synq_parse_error(pCtx, synq_error_span(pCtx));
}
