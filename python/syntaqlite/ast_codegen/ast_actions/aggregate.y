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

// ============ Aggregate Function with ORDER BY ============

// Aggregate function call: func(args ORDER BY sortlist) or func(DISTINCT args ORDER BY sortlist)
expr(A) ::= ID|INDEXED|JOIN_KW(B) LP distinct(C) exprlist(D) ORDER BY sortlist(E) RP. {
    A = ast_aggregate_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, B),
        (SyntaqliteAggregateFunctionCallFlags){.raw = (uint8_t)C},
        D,
        E,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE);
}

// Aggregate function call with filter/over
expr(A) ::= ID|INDEXED|JOIN_KW(B) LP distinct(C) exprlist(D) ORDER BY sortlist(E) RP filter_over(F). {
    SyntaqliteFilterOver *fo = (SyntaqliteFilterOver*)
        (pCtx->astCtx->ast->arena + pCtx->astCtx->ast->offsets[F]);
    A = ast_aggregate_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, B),
        (SyntaqliteAggregateFunctionCallFlags){.raw = (uint8_t)C},
        D,
        E,
        fo->filter_expr,
        fo->over_def);
}
