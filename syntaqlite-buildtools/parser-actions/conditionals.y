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

%type likeop {SynqParseToken}

// ============ IS / ISNULL / NOTNULL ============

expr(A) ::= expr(B) ISNULL|NOTNULL(E). {
    SyntaqliteIsOp op = (E.type == SYNTAQLITE_TK_ISNULL) ? SYNTAQLITE_IS_OP_IS_NULL : SYNTAQLITE_IS_OP_NOT_NULL;
    A = synq_parse_is_expr(pCtx, op, B, SYNTAQLITE_NULL_NODE);
}

expr(A) ::= expr(B) NOT NULL. {
    A = synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_NOT_NULL, B, SYNTAQLITE_NULL_NODE);
}

expr(A) ::= expr(B) IS expr(C). {
    A = synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_IS, B, C);
}

expr(A) ::= expr(B) IS NOT expr(C). {
    A = synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_IS_NOT, B, C);
}

expr(A) ::= expr(B) IS NOT DISTINCT FROM expr(C). {
    A = synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_IS_NOT_DISTINCT, B, C);
}

expr(A) ::= expr(B) IS DISTINCT FROM expr(C). {
    A = synq_parse_is_expr(pCtx, SYNTAQLITE_IS_OP_IS_DISTINCT, B, C);
}

// ============ BETWEEN ============

between_op(A) ::= BETWEEN. {
    A = 0;
}

between_op(A) ::= NOT BETWEEN. {
    A = 1;
}

expr(A) ::= expr(B) between_op(C) expr(D) AND expr(E). [BETWEEN] {
    A = synq_parse_between_expr(pCtx, (SyntaqliteBool)C, B, D, E);
}

// ============ LIKE / GLOB / MATCH ============

likeop(A) ::= LIKE_KW|MATCH(B). {
    A = B;
}

likeop(A) ::= NOT LIKE_KW|MATCH(B). {
    A = B;
    A.n |= 0x80000000;
}

// Map token to LikeKeyword enum by length + first char (case-insensitive).
// LIKE=4/L, GLOB=4/G, MATCH=5, REGEXP=6.

expr(A) ::= expr(B) likeop(C) expr(D). [LIKE_KW] {
    SyntaqliteBool negated = (C.n & 0x80000000) ? SYNTAQLITE_BOOL_TRUE : SYNTAQLITE_BOOL_FALSE;
    uint32_t len = C.n & 0x7FFFFFFF;
    SyntaqliteLikeKeyword kw = (len == 6) ? SYNTAQLITE_LIKE_KEYWORD_REGEXP
        : (len == 5) ? SYNTAQLITE_LIKE_KEYWORD_MATCH
        : (C.z[0] == 'g' || C.z[0] == 'G') ? SYNTAQLITE_LIKE_KEYWORD_GLOB
        : SYNTAQLITE_LIKE_KEYWORD_LIKE;
    A = synq_parse_like_expr(pCtx, negated, kw, B, D, SYNTAQLITE_NULL_NODE);
}

expr(A) ::= expr(B) likeop(C) expr(D) ESCAPE expr(E). [LIKE_KW] {
    SyntaqliteBool negated = (C.n & 0x80000000) ? SYNTAQLITE_BOOL_TRUE : SYNTAQLITE_BOOL_FALSE;
    uint32_t len = C.n & 0x7FFFFFFF;
    SyntaqliteLikeKeyword kw = (len == 6) ? SYNTAQLITE_LIKE_KEYWORD_REGEXP
        : (len == 5) ? SYNTAQLITE_LIKE_KEYWORD_MATCH
        : (C.z[0] == 'g' || C.z[0] == 'G') ? SYNTAQLITE_LIKE_KEYWORD_GLOB
        : SYNTAQLITE_LIKE_KEYWORD_LIKE;
    A = synq_parse_like_expr(pCtx, negated, kw, B, D, E);
}

// ============ CASE ============

expr(A) ::= CASE case_operand(B) case_exprlist(C) case_else(D) END. {
    A = synq_parse_case_expr(pCtx, B, D, C);
}

case_exprlist(A) ::= case_exprlist(B) WHEN expr(C) THEN expr(D). {
    uint32_t w = synq_parse_case_when(pCtx, C, D);
    A = synq_parse_case_when_list(pCtx, B, w);
}

case_exprlist(A) ::= WHEN expr(B) THEN expr(C). {
    uint32_t w = synq_parse_case_when(pCtx, B, C);
    A = synq_parse_case_when_list(pCtx, SYNTAQLITE_NULL_NODE, w);
}

case_else(A) ::= ELSE expr(B). {
    A = synq_pass(pCtx, B);
}

case_else(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

case_operand(A) ::= expr(B). {
    A = B;
}

case_operand(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}