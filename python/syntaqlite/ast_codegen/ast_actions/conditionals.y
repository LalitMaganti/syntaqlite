// AST building actions for syntaqlite grammar.
// These rules get merged with SQLite's parse.y during code generation.
//
// Rule signatures MUST match upstream parse.y exactly.
// Python tooling validates coverage and consistency.
//
// Conventions:
// - pCtx: Parse context (SynqParseContext*)
// - pCtx->astCtx: AST context for builder calls
// - pCtx->zSql: Original SQL text (for computing offsets)
// - pCtx->root: Set to root node ID at input rule
// - Terminals are SynqToken with .z (pointer) and .n (length)
// - Non-terminals are u32 node IDs

// ============ IS / ISNULL / NOTNULL ============

expr(A) ::= expr(B) ISNULL|NOTNULL(E). {
    SynqIsOp op = (E.type == TK_ISNULL) ? SYNQ_IS_OP_ISNULL : SYNQ_IS_OP_NOTNULL;
    A = synq_ast_is_expr(pCtx->astCtx, op, B, SYNQ_NULL_NODE);
}

expr(A) ::= expr(B) NOT NULL. {
    A = synq_ast_is_expr(pCtx->astCtx, SYNQ_IS_OP_NOTNULL, B, SYNQ_NULL_NODE);
}

expr(A) ::= expr(B) IS expr(C). {
    A = synq_ast_is_expr(pCtx->astCtx, SYNQ_IS_OP_IS, B, C);
}

expr(A) ::= expr(B) IS NOT expr(C). {
    A = synq_ast_is_expr(pCtx->astCtx, SYNQ_IS_OP_IS_NOT, B, C);
}

expr(A) ::= expr(B) IS NOT DISTINCT FROM expr(C). {
    A = synq_ast_is_expr(pCtx->astCtx, SYNQ_IS_OP_IS_NOT_DISTINCT, B, C);
}

expr(A) ::= expr(B) IS DISTINCT FROM expr(C). {
    A = synq_ast_is_expr(pCtx->astCtx, SYNQ_IS_OP_IS_DISTINCT, B, C);
}

// ============ BETWEEN ============

between_op(A) ::= BETWEEN. {
    A = 0;
}

between_op(A) ::= NOT BETWEEN. {
    A = 1;
}

expr(A) ::= expr(B) between_op(C) expr(D) AND expr(E). [BETWEEN] {
    A = synq_ast_between_expr(pCtx->astCtx, (SynqBool)C, B, D, E);
}

// ============ LIKE / GLOB / MATCH ============

likeop(A) ::= LIKE_KW|MATCH(B). {
    A = B;
}

likeop(A) ::= NOT LIKE_KW|MATCH(B). {
    A = B;
    A.n |= 0x80000000;
}

expr(A) ::= expr(B) likeop(C) expr(D). [LIKE_KW] {
    SynqBool negated = (C.n & 0x80000000) ? SYNQ_BOOL_TRUE : SYNQ_BOOL_FALSE;
    A = synq_ast_like_expr(pCtx->astCtx, negated, B, D, SYNQ_NULL_NODE);
}

expr(A) ::= expr(B) likeop(C) expr(D) ESCAPE expr(E). [LIKE_KW] {
    SynqBool negated = (C.n & 0x80000000) ? SYNQ_BOOL_TRUE : SYNQ_BOOL_FALSE;
    A = synq_ast_like_expr(pCtx->astCtx, negated, B, D, E);
}

// ============ CASE ============

expr(A) ::= CASE case_operand(B) case_exprlist(C) case_else(D) END. {
    A = synq_ast_case_expr(pCtx->astCtx, B, D, C);
}

case_exprlist(A) ::= case_exprlist(B) WHEN expr(C) THEN expr(D). {
    uint32_t w = synq_ast_case_when(pCtx->astCtx, C, D);
    A = synq_ast_case_when_list_append(pCtx->astCtx, B, w);
}

case_exprlist(A) ::= WHEN expr(B) THEN expr(C). {
    uint32_t w = synq_ast_case_when(pCtx->astCtx, B, C);
    A = synq_ast_case_when_list(pCtx->astCtx, w);
}

case_else(A) ::= ELSE expr(B). {
    A = B;
}

case_else(A) ::= . {
    A = SYNQ_NULL_NODE;
}

case_operand(A) ::= expr(B). {
    A = B;
}

case_operand(A) ::= . {
    A = SYNQ_NULL_NODE;
}
