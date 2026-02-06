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

// ============ WITH/CTE ============

select(A) ::= WITH wqlist(W) selectnowith(X). {
    A = synq_ast_with_clause(pCtx->astCtx, 0, W, X);
}

select(A) ::= WITH RECURSIVE wqlist(W) selectnowith(X). {
    A = synq_ast_with_clause(pCtx->astCtx, 1, W, X);
}

// ============ CTE item ============

wqitem(A) ::= withnm(X) eidlist_opt(Y) wqas(M) LP select(Z) RP. {
    A = synq_ast_cte_definition(pCtx->astCtx, synq_span(pCtx, X), (SyntaqliteMaterialized)M, Y, Z);
}

// ============ CTE list ============

wqlist(A) ::= wqitem(X). {
    A = synq_ast_cte_list(pCtx->astCtx, X);
}

wqlist(A) ::= wqlist(A) COMMA wqitem(X). {
    A = synq_ast_cte_list_append(pCtx->astCtx, A, X);
}

// ============ CTE name ============

withnm(A) ::= nm(A). {
    // Token passthrough - nm already produces SynqToken
}

// ============ AS materialization hint ============

wqas(A) ::= AS. {
    A = (int)SYNTAQLITE_MATERIALIZED_DEFAULT;
}

wqas(A) ::= AS MATERIALIZED. {
    A = (int)SYNTAQLITE_MATERIALIZED_MATERIALIZED;
}

wqas(A) ::= AS NOT MATERIALIZED. {
    A = (int)SYNTAQLITE_MATERIALIZED_NOT_MATERIALIZED;
}

// ============ Column list (eidlist) ============

eidlist_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

eidlist_opt(A) ::= LP eidlist(X) RP. {
    A = X;
}

eidlist(A) ::= nm(Y) collate(C) sortorder(Z). {
    (void)C; (void)Z;
    uint32_t col = synq_ast_column_ref(pCtx->astCtx,
        synq_span(pCtx, Y),
        SYNQ_NO_SPAN,
        SYNQ_NO_SPAN);
    A = synq_ast_expr_list(pCtx->astCtx, col);
}

eidlist(A) ::= eidlist(A) COMMA nm(Y) collate(C) sortorder(Z). {
    (void)C; (void)Z;
    uint32_t col = synq_ast_column_ref(pCtx->astCtx,
        synq_span(pCtx, Y),
        SYNQ_NO_SPAN,
        SYNQ_NO_SPAN);
    A = synq_ast_expr_list_append(pCtx->astCtx, A, col);
}

// ============ COLLATE for eidlist ============

collate(C) ::= . {
    C = 0;
}

collate(C) ::= COLLATE ID|STRING. {
    C = 1;
}
