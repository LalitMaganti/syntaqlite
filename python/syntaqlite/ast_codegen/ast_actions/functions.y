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

// ============ Function Calls ============

// Function call with arguments: func(args) or func(DISTINCT args)
expr(A) ::= ID|INDEXED|JOIN_KW(B) LP distinct(C) exprlist(D) RP. {
    A = synq_ast_function_call(pCtx->astCtx,
        synq_span(pCtx, B),
        (SynqFunctionCallFlags){.raw = (uint8_t)C},
        D,
        SYNQ_NULL_NODE,
        SYNQ_NULL_NODE);
}

// Function call with star: COUNT(*)
expr(A) ::= ID|INDEXED|JOIN_KW(B) LP STAR RP. {
    A = synq_ast_function_call(pCtx->astCtx,
        synq_span(pCtx, B),
        (SynqFunctionCallFlags){.star = 1},
        SYNQ_NULL_NODE,
        SYNQ_NULL_NODE,
        SYNQ_NULL_NODE);
}

// Function call with arguments and filter/over: func(args) FILTER/OVER
expr(A) ::= ID|INDEXED|JOIN_KW(B) LP distinct(C) exprlist(D) RP filter_over(E). {
    SynqFilterOver *fo = (SynqFilterOver*)
        (pCtx->astCtx->ast.data + pCtx->astCtx->ast.offsets[E]);
    A = synq_ast_function_call(pCtx->astCtx,
        synq_span(pCtx, B),
        (SynqFunctionCallFlags){.raw = (uint8_t)C},
        D,
        fo->filter_expr,
        fo->over_def);
}

// Function call with star and filter/over: COUNT(*) FILTER/OVER
expr(A) ::= ID|INDEXED|JOIN_KW(B) LP STAR RP filter_over(C). {
    SynqFilterOver *fo = (SynqFilterOver*)
        (pCtx->astCtx->ast.data + pCtx->astCtx->ast.offsets[C]);
    A = synq_ast_function_call(pCtx->astCtx,
        synq_span(pCtx, B),
        (SynqFunctionCallFlags){.star = 1},
        SYNQ_NULL_NODE,
        fo->filter_expr,
        fo->over_def);
}
