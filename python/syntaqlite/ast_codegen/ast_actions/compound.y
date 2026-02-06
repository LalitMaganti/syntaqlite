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

// ============ Compound SELECT ============

selectnowith(A) ::= selectnowith(A) multiselect_op(Y) oneselect(Z). {
    A = synq_ast_compound_select(pCtx->astCtx, (SynqCompoundOp)Y, A, Z);
}

multiselect_op(A) ::= UNION(OP). { A = 0; UNUSED_PARAMETER(OP); }
multiselect_op(A) ::= UNION ALL. { A = 1; }
multiselect_op(A) ::= EXCEPT|INTERSECT(OP). {
    A = (OP.type == TK_INTERSECT) ? 2 : 3;
}

// ============ Subquery Expressions ============

expr(A) ::= LP select(X) RP. {
    A = synq_ast_subquery_expr(pCtx->astCtx, X);
}

expr(A) ::= EXISTS LP select(Y) RP. {
    A = synq_ast_exists_expr(pCtx->astCtx, Y);
}

// ============ IN Expressions ============

in_op(A) ::= IN. { A = 0; }
in_op(A) ::= NOT IN. { A = 1; }

expr(A) ::= expr(A) in_op(N) LP exprlist(Y) RP. [IN] {
    A = synq_ast_in_expr(pCtx->astCtx, (SynqBool)N, A, Y);
}

expr(A) ::= expr(A) in_op(N) LP select(Y) RP. [IN] {
    uint32_t sub = synq_ast_subquery_expr(pCtx->astCtx, Y);
    A = synq_ast_in_expr(pCtx->astCtx, (SynqBool)N, A, sub);
}

expr(A) ::= expr(A) in_op(N) nm(Y) dbnm(Z) paren_exprlist(E). [IN] {
    // Table-valued function IN expression - stub for now
    (void)Y; (void)Z; (void)E;
    A = synq_ast_in_expr(pCtx->astCtx, (SynqBool)N, A, SYNQ_NULL_NODE);
}

// ============ Helper rules ============

dbnm(A) ::= . { A.z = NULL; A.n = 0; }
dbnm(A) ::= DOT nm(X). { A = X; }

paren_exprlist(A) ::= . { A = SYNQ_NULL_NODE; }
paren_exprlist(A) ::= LP exprlist(X) RP. { A = X; }
