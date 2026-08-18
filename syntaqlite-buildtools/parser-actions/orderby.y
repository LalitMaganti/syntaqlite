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

// ============ Sort List (ORDER BY) ============

sortlist(A) ::= sortlist(B) COMMA expr(C) sortorder(D) nulls(E). {
    uint32_t term = synq_parse_ordering_term(pCtx, C, synq_sortorder(D), (SyntaqliteNullsOrder)E);
    A = synq_parse_order_by_list(pCtx, B, term);
}

sortlist(A) ::= expr(B) sortorder(C) nulls(D). {
    uint32_t term = synq_parse_ordering_term(pCtx, B, synq_sortorder(C), (SyntaqliteNullsOrder)D);
    A = synq_parse_order_by_list(pCtx, SYNTAQLITE_NULL_NODE, term);
}

// ============ Sort Order ============

sortorder(A) ::= ASC. {
    A = 0;
}

sortorder(A) ::= DESC. {
    A = 1;
}

// Distinct from ASC so eidlist can tell an explicit sort order from none;
// every other consumer normalises it back to ASC.
sortorder(A) ::= . {
    A = SYNQ_SORTORDER_NONE;
}

// ============ Nulls Order ============

nulls(A) ::= NULLS FIRST. {
    A = 1;
}

nulls(A) ::= NULLS LAST. {
    A = 2;
}

nulls(A) ::= . {
    A = 0;
}