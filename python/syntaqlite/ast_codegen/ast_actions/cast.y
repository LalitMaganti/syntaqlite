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

// ============ CAST Expression ============

expr(A) ::= CAST LP expr(E) AS typetoken(T) RP. {
    A = ast_cast_expr(pCtx->astCtx, E, syntaqlite_span(pCtx, T));
}

// ============ Type Token ============

typetoken(A) ::= . {
    A.n = 0; A.z = 0;
}

typetoken(A) ::= typename(A). {
    (void)A;
}

typetoken(A) ::= typename(A) LP signed RP(Y). {
    A.n = (int)(&Y.z[Y.n] - A.z);
}

typetoken(A) ::= typename(A) LP signed COMMA signed RP(Y). {
    A.n = (int)(&Y.z[Y.n] - A.z);
}

// ============ Type Name ============
// Note: lemon -g inlines 'ids' as 'ID|STRING', so we use that directly.

typename(A) ::= ID|STRING(B). {
    A = B;
}

typename(A) ::= typename(A) ID|STRING(Y). {
    A.n = Y.n + (int)(Y.z - A.z);
}
