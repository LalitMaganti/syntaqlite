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

// ============ RAISE Expressions ============

// RAISE(IGNORE) - no error message
expr(A) ::= RAISE LP IGNORE RP. {
    A = synq_ast_raise_expr(pCtx->astCtx, SYNTAQLITE_RAISE_TYPE_IGNORE, SYNTAQLITE_NULL_NODE);
}

// RAISE(type, error_message)
expr(A) ::= RAISE LP raisetype(T) COMMA expr(Z) RP. {
    A = synq_ast_raise_expr(pCtx->astCtx, (SyntaqliteRaiseType)T, Z);
}

// ============ Raise Type ============

raisetype(A) ::= ROLLBACK. { A = SYNTAQLITE_RAISE_TYPE_ROLLBACK; }
raisetype(A) ::= ABORT. { A = SYNTAQLITE_RAISE_TYPE_ABORT; }
raisetype(A) ::= FAIL. { A = SYNTAQLITE_RAISE_TYPE_FAIL; }
