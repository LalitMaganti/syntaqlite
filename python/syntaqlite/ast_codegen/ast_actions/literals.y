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

// ============ Literals ============

term(A) ::= INTEGER(B). {
    A = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_INTEGER, syntaqlite_span(pCtx, B));
}

term(A) ::= STRING(B). {
    A = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_STRING, syntaqlite_span(pCtx, B));
}

term(A) ::= NULL|FLOAT|BLOB(B). {
    SyntaqliteLiteralType lit_type;
    switch (B.type) {
        case TK_NULL:  lit_type = SYNTAQLITE_LITERAL_TYPE_NULL; break;
        case TK_FLOAT: lit_type = SYNTAQLITE_LITERAL_TYPE_FLOAT; break;
        case TK_BLOB:  lit_type = SYNTAQLITE_LITERAL_TYPE_BLOB; break;
        default:       lit_type = SYNTAQLITE_LITERAL_TYPE_NULL; break;
    }
    A = ast_literal(pCtx->astCtx, lit_type, syntaqlite_span(pCtx, B));
}
