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

// ============ Column References ============

// Simple column reference: just a name
expr(A) ::= ID|INDEXED|JOIN_KW(B). {
    A = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, B),
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NO_SPAN);
}

// Qualified column reference: table.column
expr(A) ::= nm(B) DOT nm(C). {
    A = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, C),
        syntaqlite_span(pCtx, B),
        SYNTAQLITE_NO_SPAN);
}

// Fully qualified: schema.table.column
expr(A) ::= nm(B) DOT nm(C) DOT nm(D). {
    A = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, D),
        syntaqlite_span(pCtx, C),
        syntaqlite_span(pCtx, B));
}
