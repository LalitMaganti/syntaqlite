// AST building actions for CREATE VIRTUAL TABLE grammar rules.
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

// ============ CREATE VIRTUAL TABLE ============

// Without arguments
cmd(A) ::= create_vtab(X). {
    A = X;
}

// With arguments in parentheses
cmd(A) ::= create_vtab(X) LP(L) vtabarglist RP(R). {
    // Capture module arguments span (content between parens)
    SyntaqliteNode *vtab = AST_NODE(&pCtx->astCtx->ast, X);
    const char *args_start = L.z + L.n;
    const char *args_end = R.z;
    vtab->create_virtual_table_stmt.module_args = (SyntaqliteSourceSpan){
        (uint32_t)(args_start - pCtx->zSql),
        (uint16_t)(args_end - args_start)
    };
    A = X;
}

// create_vtab builds the node with table name, schema, module name
create_vtab(A) ::= createkw VIRTUAL TABLE ifnotexists(E) nm(X) dbnm(Y) USING nm(Z). {
    SyntaqliteSourceSpan tbl_name = Y.z ? synq_span(pCtx, Y) : synq_span(pCtx, X);
    SyntaqliteSourceSpan tbl_schema = Y.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_ast_create_virtual_table_stmt(pCtx->astCtx,
        tbl_name,
        tbl_schema,
        synq_span(pCtx, Z),
        (SyntaqliteBool)E,
        SYNQ_NO_SPAN);  // module_args = none by default
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
