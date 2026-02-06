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

// ============ Expression Lists ============

exprlist(A) ::= nexprlist(B). {
    A = B;
}

exprlist(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

nexprlist(A) ::= nexprlist(B) COMMA expr(C). {
    A = synq_ast_expr_list_append(pCtx->astCtx, B, C);
}

nexprlist(A) ::= expr(B). {
    A = synq_ast_expr_list(pCtx->astCtx, B);
}

// ============ Row Value Tuple ============
// (1, 2, 3) => ExprList with 3 elements

expr(A) ::= LP nexprlist(X) COMMA expr(Y) RP. {
    A = synq_ast_expr_list_append(pCtx->astCtx, X, Y);
}
