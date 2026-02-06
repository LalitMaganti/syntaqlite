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

// ============ Entry point ============

input ::= cmdlist(B). {
    pCtx->root = B;
}

// ============ Command list ============

cmdlist(A) ::= cmdlist ecmd(B). {
    A = B;  // Just use the last command for now
}

cmdlist(A) ::= ecmd(B). {
    A = B;
}

// ============ Command wrapper ============

ecmd(A) ::= SEMI. {
    A = SYNQ_NULL_NODE;
}

ecmd(A) ::= cmdx(B) SEMI. {
    A = B;
}

cmdx(A) ::= cmd(B). {
    A = B;
}
