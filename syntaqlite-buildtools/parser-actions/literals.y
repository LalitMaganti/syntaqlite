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

// ============ Literals ============

term(A) ::= INTEGER(B). {
    A = synq_parse_literal(pCtx, SYNTAQLITE_LITERAL_TYPE_INTEGER, synq_span(pCtx, B));
}

term(A) ::= STRING(B). {
    A = synq_parse_literal(pCtx, SYNTAQLITE_LITERAL_TYPE_STRING, synq_span(pCtx, B));
}

term(A) ::= NULL|FLOAT|BLOB(B). {
    SyntaqliteLiteralType lit_type;
    switch (B.type) {
        case SYNTAQLITE_TK_NULL:  lit_type = SYNTAQLITE_LITERAL_TYPE_NULL; break;
        case SYNTAQLITE_TK_FLOAT: lit_type = SYNTAQLITE_LITERAL_TYPE_FLOAT; break;
        case SYNTAQLITE_TK_BLOB:  lit_type = SYNTAQLITE_LITERAL_TYPE_BLOB; break;
        default:       lit_type = SYNTAQLITE_LITERAL_TYPE_NULL; break;
    }
    A = synq_parse_literal(pCtx, lit_type, synq_span(pCtx, B));
}

term(A) ::= QNUMBER(B). {
    if (!synq_qnumber_is_valid(B.z, B.n)) {
        pCtx->error = 1;
    }
    A = synq_parse_literal(pCtx, SYNTAQLITE_LITERAL_TYPE_QNUMBER, synq_span(pCtx, B));
}

// ============ Date/Time Keywords ============

term(A) ::= CTIME_KW(B). {
    A = synq_parse_literal(pCtx, SYNTAQLITE_LITERAL_TYPE_CURRENT, synq_span(pCtx, B));
}