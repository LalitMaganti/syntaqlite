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

// ============ Table-qualified star in result columns ============

// table.* in result columns
selcollist(A) ::= sclp(B) scanpt nm(C) DOT STAR. {
    uint32_t col = synq_ast_result_column(pCtx->astCtx, (SynqResultColumnFlags){.star = 1}, synq_span(pCtx, C), SYNQ_NULL_NODE);
    A = synq_ast_result_column_list_append(pCtx->astCtx, B, col);
}
