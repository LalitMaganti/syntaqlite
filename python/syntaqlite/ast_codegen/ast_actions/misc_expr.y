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

// ============ Bind Parameters ============

expr(A) ::= VARIABLE(B). {
    A = ast_variable(pCtx->astCtx, syntaqlite_span(pCtx, B));
}

// ============ COLLATE Expression ============

expr(A) ::= expr(B) COLLATE ID|STRING(C). {
    A = ast_collate_expr(pCtx->astCtx, B, syntaqlite_span(pCtx, C));
}
