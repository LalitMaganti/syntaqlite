// AST building actions for syntaqlite grammar.
// These rules get merged with SQLite's parse.y during code generation.
//
// Rule signatures MUST match upstream parse.y exactly.
// Python tooling validates coverage and consistency.
//
// Conventions:
// - pCtx: Parse context (SyntaqliteParseContext*)
// - pCtx->astCtx: AST context for builder calls
// - pCtx->zSql: Original SQL text (for computing offsets)
// - pCtx->root: Set to root node ID at input rule
// - Terminals are SyntaqliteToken with .z (pointer) and .n (length)
// - Non-terminals are u32 node IDs

// ============ IS / ISNULL / NOTNULL ============

expr(A) ::= expr(B) ISNULL|NOTNULL(E). {
    SyntaqliteIsOp op = (E.type == TK_ISNULL) ? SYNTAQLITE_IS_OP_ISNULL : SYNTAQLITE_IS_OP_NOTNULL;
    A = ast_is_expr(pCtx->astCtx, op, B, SYNTAQLITE_NULL_NODE);
}

expr(A) ::= expr(B) NOT NULL. {
    A = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_NOTNULL, B, SYNTAQLITE_NULL_NODE);
}

expr(A) ::= expr(B) IS expr(C). {
    A = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS, B, C);
}

expr(A) ::= expr(B) IS NOT expr(C). {
    A = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_NOT, B, C);
}

expr(A) ::= expr(B) IS NOT DISTINCT FROM expr(C). {
    A = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_NOT_DISTINCT, B, C);
}

expr(A) ::= expr(B) IS DISTINCT FROM expr(C). {
    A = ast_is_expr(pCtx->astCtx, SYNTAQLITE_IS_OP_IS_DISTINCT, B, C);
}

// ============ BETWEEN ============

between_op(A) ::= BETWEEN. {
    A = 0;
}

between_op(A) ::= NOT BETWEEN. {
    A = 1;
}

expr(A) ::= expr(B) between_op(C) expr(D) AND expr(E). {
    A = ast_between_expr(pCtx->astCtx, (uint8_t)C, B, D, E);
}

// ============ LIKE / GLOB / MATCH ============

likeop(A) ::= LIKE_KW|MATCH(B). {
    A = B;
}

likeop(A) ::= NOT LIKE_KW|MATCH(B). {
    A = B;
    A.n |= 0x80000000;
}

expr(A) ::= expr(B) likeop(C) expr(D). {
    uint8_t negated = (C.n & 0x80000000) ? 1 : 0;
    A = ast_like_expr(pCtx->astCtx, negated, B, D, SYNTAQLITE_NULL_NODE);
}

expr(A) ::= expr(B) likeop(C) expr(D) ESCAPE expr(E). {
    uint8_t negated = (C.n & 0x80000000) ? 1 : 0;
    A = ast_like_expr(pCtx->astCtx, negated, B, D, E);
}

// ============ CASE ============

expr(A) ::= CASE case_operand(B) case_exprlist(C) case_else(D) END. {
    A = ast_case_expr(pCtx->astCtx, B, D, C);
}

case_exprlist(A) ::= case_exprlist(B) WHEN expr(C) THEN expr(D). {
    uint32_t w = ast_case_when(pCtx->astCtx, C, D);
    A = ast_case_when_list_append(pCtx->astCtx, B, w);
}

case_exprlist(A) ::= WHEN expr(B) THEN expr(C). {
    uint32_t w = ast_case_when(pCtx->astCtx, B, C);
    A = ast_case_when_list(pCtx->astCtx, w);
}

case_else(A) ::= ELSE expr(B). {
    A = B;
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
