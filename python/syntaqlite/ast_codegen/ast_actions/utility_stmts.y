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

// ============ PRAGMA ============

// For nm dbnm pattern: when dbnm is non-empty, nm=schema and dbnm=actual name.
// We swap so pragma_name always has the pragma name, schema always has the schema.

cmd(A) ::= PRAGMA nm(X) dbnm(Z). {
    SynqSourceSpan name_span = Z.z ? synq_span(pCtx, Z) : synq_span(pCtx, X);
    SynqSourceSpan schema_span = Z.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, SYNQ_NO_SPAN, SYNQ_PRAGMA_FORM_BARE);
}

cmd(A) ::= PRAGMA nm(X) dbnm(Z) EQ nmnum(Y). {
    SynqSourceSpan name_span = Z.z ? synq_span(pCtx, Z) : synq_span(pCtx, X);
    SynqSourceSpan schema_span = Z.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, synq_span(pCtx, Y), SYNQ_PRAGMA_FORM_EQ);
}

cmd(A) ::= PRAGMA nm(X) dbnm(Z) LP nmnum(Y) RP. {
    SynqSourceSpan name_span = Z.z ? synq_span(pCtx, Z) : synq_span(pCtx, X);
    SynqSourceSpan schema_span = Z.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, synq_span(pCtx, Y), SYNQ_PRAGMA_FORM_CALL);
}

cmd(A) ::= PRAGMA nm(X) dbnm(Z) EQ minus_num(Y). {
    SynqSourceSpan name_span = Z.z ? synq_span(pCtx, Z) : synq_span(pCtx, X);
    SynqSourceSpan schema_span = Z.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, synq_span(pCtx, Y), SYNQ_PRAGMA_FORM_EQ);
}

cmd(A) ::= PRAGMA nm(X) dbnm(Z) LP minus_num(Y) RP. {
    SynqSourceSpan name_span = Z.z ? synq_span(pCtx, Z) : synq_span(pCtx, X);
    SynqSourceSpan schema_span = Z.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_ast_pragma_stmt(pCtx->astCtx, name_span, schema_span, synq_span(pCtx, Y), SYNQ_PRAGMA_FORM_CALL);
}

// ============ NMNUM / PLUS_NUM / MINUS_NUM / SIGNED ============
// Upstream uses %token_class number INTEGER|FLOAT, which lemon -g expands.

nmnum(A) ::= plus_num(A). {
    // Token passthrough
}

nmnum(A) ::= nm(A). {
    // Token passthrough
}

nmnum(A) ::= ON(A). {
    // Token passthrough
}

nmnum(A) ::= DELETE(A). {
    // Token passthrough
}

nmnum(A) ::= DEFAULT(A). {
    // Token passthrough
}

plus_num(A) ::= PLUS INTEGER|FLOAT(X). {
    A = X;
}

plus_num(A) ::= INTEGER|FLOAT(A). {
    // Token passthrough
}

minus_num(A) ::= MINUS(M) INTEGER|FLOAT(X). {
    // Build a token that spans from the MINUS sign through the number
    A.z = M.z;
    A.n = (int)(X.z - M.z) + X.n;
}

signed(A) ::= plus_num(A). {
    // Token passthrough
}

signed(A) ::= minus_num(A). {
    // Token passthrough
}

// ============ ANALYZE ============

cmd(A) ::= ANALYZE. {
    A = synq_ast_analyze_stmt(pCtx->astCtx,
        SYNQ_NO_SPAN,
        SYNQ_NO_SPAN,
        SYNQ_ANALYZE_KIND_ANALYZE);
}

cmd(A) ::= ANALYZE nm(X) dbnm(Y). {
    SynqSourceSpan name_span = Y.z ? synq_span(pCtx, Y) : synq_span(pCtx, X);
    SynqSourceSpan schema_span = Y.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_ast_analyze_stmt(pCtx->astCtx, name_span, schema_span, SYNQ_ANALYZE_KIND_ANALYZE);
}

// ============ REINDEX ============

cmd(A) ::= REINDEX. {
    A = synq_ast_analyze_stmt(pCtx->astCtx,
        SYNQ_NO_SPAN,
        SYNQ_NO_SPAN,
        SYNQ_ANALYZE_KIND_REINDEX);
}

cmd(A) ::= REINDEX nm(X) dbnm(Y). {
    SynqSourceSpan name_span = Y.z ? synq_span(pCtx, Y) : synq_span(pCtx, X);
    SynqSourceSpan schema_span = Y.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_ast_analyze_stmt(pCtx->astCtx, name_span, schema_span, 1);
}

// ============ ATTACH / DETACH ============

cmd(A) ::= ATTACH database_kw_opt expr(F) AS expr(D) key_opt(K). {
    A = synq_ast_attach_stmt(pCtx->astCtx, F, D, K);
}

cmd(A) ::= DETACH database_kw_opt expr(D). {
    A = synq_ast_detach_stmt(pCtx->astCtx, D);
}

database_kw_opt ::= DATABASE. {
    // Keyword consumed, no value needed
}

database_kw_opt ::= . {
    // Empty
}

key_opt(A) ::= . {
    A = SYNQ_NULL_NODE;
}

key_opt(A) ::= KEY expr(X). {
    A = X;
}

// ============ VACUUM ============

cmd(A) ::= VACUUM vinto(Y). {
    A = synq_ast_vacuum_stmt(pCtx->astCtx,
        SYNQ_NO_SPAN,
        Y);
}

cmd(A) ::= VACUUM nm(X) vinto(Y). {
    A = synq_ast_vacuum_stmt(pCtx->astCtx,
        synq_span(pCtx, X),
        Y);
}

vinto(A) ::= INTO expr(X). {
    A = X;
}

vinto(A) ::= . {
    A = SYNQ_NULL_NODE;
}

// ============ EXPLAIN ============

ecmd(A) ::= explain(E) cmdx(B) SEMI. {
    A = synq_ast_explain_stmt(pCtx->astCtx, (SynqExplainMode)(E - 1), B);
}

explain(A) ::= EXPLAIN. {
    A = 1;
}

explain(A) ::= EXPLAIN QUERY PLAN. {
    A = 2;
}

// ============ CREATE INDEX ============

cmd(A) ::= createkw uniqueflag(U) INDEX ifnotexists(NE) nm(X) dbnm(D) ON nm(Y) LP sortlist(Z) RP where_opt(W). {
    SynqSourceSpan idx_name = D.z ? synq_span(pCtx, D) : synq_span(pCtx, X);
    SynqSourceSpan idx_schema = D.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_ast_create_index_stmt(pCtx->astCtx,
        idx_name,
        idx_schema,
        synq_span(pCtx, Y),
        (SynqBool)U,
        (SynqBool)NE,
        Z,
        W);
}

uniqueflag(A) ::= UNIQUE. {
    A = 1;
}

uniqueflag(A) ::= . {
    A = 0;
}

ifnotexists(A) ::= . {
    A = 0;
}

ifnotexists(A) ::= IF NOT EXISTS. {
    A = 1;
}

// ============ CREATE VIEW ============

cmd(A) ::= createkw temp(T) VIEW ifnotexists(E) nm(Y) dbnm(Z) eidlist_opt(C) AS select(S). {
    SynqSourceSpan view_name = Z.z ? synq_span(pCtx, Z) : synq_span(pCtx, Y);
    SynqSourceSpan view_schema = Z.z ? synq_span(pCtx, Y) : SYNQ_NO_SPAN;
    A = synq_ast_create_view_stmt(pCtx->astCtx,
        view_name,
        view_schema,
        (SynqBool)T,
        (SynqBool)E,
        C,
        S);
}

// ============ CREATE keyword / TEMP ============

createkw(A) ::= CREATE(A). {
    // Token passthrough
}

temp(A) ::= TEMP. {
    A = 1;
}

temp(A) ::= . {
    A = 0;
}
