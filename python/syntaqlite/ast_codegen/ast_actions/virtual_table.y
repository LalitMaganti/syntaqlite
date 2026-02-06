// AST building actions for CREATE VIRTUAL TABLE grammar rules.
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

// ============ CREATE VIRTUAL TABLE ============

// Without arguments
cmd(A) ::= create_vtab(X). {
    A = X;
}

// With arguments in parentheses
cmd(A) ::= create_vtab(X) LP vtabarglist RP. {
    // Mark that arguments are present
    SyntaqliteNode *vtab = AST_NODE(pCtx->astCtx->ast, X);
    vtab->create_virtual_table_stmt.has_args = 1;
    A = X;
}

// create_vtab builds the node with table name, schema, module name
create_vtab(A) ::= createkw VIRTUAL TABLE ifnotexists(E) nm(X) dbnm(Y) USING nm(Z). {
    SyntaqliteSourceSpan tbl_name = Y.z ? syntaqlite_span(pCtx, Y) : syntaqlite_span(pCtx, X);
    SyntaqliteSourceSpan tbl_schema = Y.z ? syntaqlite_span(pCtx, X) : SYNTAQLITE_NO_SPAN;
    A = ast_create_virtual_table_stmt(pCtx->astCtx,
        tbl_name,
        tbl_schema,
        syntaqlite_span(pCtx, Z),
        (uint8_t)E,
        0);  // has_args = 0 by default
}

// ============ vtab argument list (grammar-level only, no AST values) ============

vtabarglist ::= vtabarg. {
    // consumed
}

vtabarglist ::= vtabarglist COMMA vtabarg. {
    // consumed
}

vtabarg ::= . {
    // empty
}

vtabarg ::= vtabarg vtabargtoken. {
    // consumed
}

vtabargtoken ::= ANY. {
    // consumed
}

vtabargtoken ::= lp anylist RP. {
    // consumed
}

lp ::= LP. {
    // consumed
}

anylist ::= . {
    // empty
}

anylist ::= anylist LP anylist RP. {
    // consumed
}

anylist ::= anylist ANY. {
    // consumed
}
