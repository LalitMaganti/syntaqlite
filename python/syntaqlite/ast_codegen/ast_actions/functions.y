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

// ============ Function Calls ============

// Function call with arguments: func(args) or func(DISTINCT args)
expr(A) ::= ID|INDEXED|JOIN_KW(B) LP distinct(C) exprlist(D) RP. {
    A = ast_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, B),
        (uint8_t)C,
        D);
}

// Function call with star: COUNT(*)
expr(A) ::= ID|INDEXED|JOIN_KW(B) LP STAR RP. {
    A = ast_function_call(pCtx->astCtx,
        syntaqlite_span(pCtx, B),
        2,
        SYNTAQLITE_NULL_NODE);
}
