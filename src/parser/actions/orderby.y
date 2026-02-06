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

// ============ Sort List (ORDER BY) ============

sortlist(A) ::= sortlist(B) COMMA expr(C) sortorder(D) nulls(E). {
    uint32_t term = synq_ast_ordering_term(pCtx->astCtx, C, (SyntaqliteSortOrder)D, (SyntaqliteNullsOrder)E);
    A = synq_ast_order_by_list_append(pCtx->astCtx, B, term);
}

sortlist(A) ::= expr(B) sortorder(C) nulls(D). {
    uint32_t term = synq_ast_ordering_term(pCtx->astCtx, B, (SyntaqliteSortOrder)C, (SyntaqliteNullsOrder)D);
    A = synq_ast_order_by_list(pCtx->astCtx, term);
}

// ============ Sort Order ============

sortorder(A) ::= ASC. {
    A = 0;
}

sortorder(A) ::= DESC. {
    A = 1;
}

sortorder(A) ::= . {
    A = 0;
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
