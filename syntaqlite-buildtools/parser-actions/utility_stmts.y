// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// AST building actions for syntaqlite grammar.
// These rules get merged with SQLite's parse.y during code generation.
//
// Rule signatures MUST match upstream parse.y exactly.
// Python tooling validates coverage and consistency.
//
// Conventions:
// - pCtx: Parse context (SynqParseContext*)
// - pCtx->zSql: Original SQL text (for computing offsets)
// - pCtx->root: Set to root node ID at input rule
// - Terminals are SynqParseToken with .z (pointer) and .n (length)
// - Non-terminals are u32 node IDs

%type createkw {SynqParseToken}
%type signed {SynqParseToken}
%type plus_num {SynqParseToken}
%type minus_num {SynqParseToken}
%type nmnum {SynqParseToken}
%type ifnotexists {int}
%type temp {int}
%type uniqueflag {int}
%type explain {int}

// ============ PRAGMA ============

// For nm dbnm pattern: when dbnm is non-empty, nm=schema and dbnm=actual name.
// We swap so pragma_name always has the pragma name, schema always has the schema.

cmd(A) ::= PRAGMA nm(X) dbnm(Z). {
    SyntaqliteTextSpan name_span = Z.z ? synq_span(pCtx, Z) : synq_span(pCtx, X);
    SyntaqliteTextSpan schema_span = Z.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_parse_pragma_stmt(pCtx, name_span, schema_span, SYNQ_NO_SPAN, SYNTAQLITE_PRAGMA_FORM_BARE);
}

cmd(A) ::= PRAGMA nm(X) dbnm(Z) EQ nmnum(Y). {
    SyntaqliteTextSpan name_span = Z.z ? synq_span(pCtx, Z) : synq_span(pCtx, X);
    SyntaqliteTextSpan schema_span = Z.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_parse_pragma_stmt(pCtx, name_span, schema_span, synq_span(pCtx, Y), SYNTAQLITE_PRAGMA_FORM_EQ);
}

cmd(A) ::= PRAGMA nm(X) dbnm(Z) LP nmnum(Y) RP. {
    SyntaqliteTextSpan name_span = Z.z ? synq_span(pCtx, Z) : synq_span(pCtx, X);
    SyntaqliteTextSpan schema_span = Z.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_parse_pragma_stmt(pCtx, name_span, schema_span, synq_span(pCtx, Y), SYNTAQLITE_PRAGMA_FORM_CALL);
}

cmd(A) ::= PRAGMA nm(X) dbnm(Z) EQ minus_num(Y). {
    SyntaqliteTextSpan name_span = Z.z ? synq_span(pCtx, Z) : synq_span(pCtx, X);
    SyntaqliteTextSpan schema_span = Z.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_parse_pragma_stmt(pCtx, name_span, schema_span, synq_span(pCtx, Y), SYNTAQLITE_PRAGMA_FORM_EQ);
}

cmd(A) ::= PRAGMA nm(X) dbnm(Z) LP minus_num(Y) RP. {
    SyntaqliteTextSpan name_span = Z.z ? synq_span(pCtx, Z) : synq_span(pCtx, X);
    SyntaqliteTextSpan schema_span = Z.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_parse_pragma_stmt(pCtx, name_span, schema_span, synq_span(pCtx, Y), SYNTAQLITE_PRAGMA_FORM_CALL);
}

// ============ NMNUM / PLUS_NUM / MINUS_NUM / SIGNED ============
// Upstream uses %token_class number number, which lemon -g expands.

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

plus_num(A) ::= PLUS number(X). {
    A = X;
}

plus_num(A) ::= number(A). {
    // Token passthrough
}

minus_num(A) ::= MINUS(M) number(X). {
    // Build a token that spans from the MINUS sign through the number
    A.z = M.z;
    A.n = (int)(X.z - M.z) + X.n;
    A.offset = M.offset;
    A.layer_id = M.layer_id;
}

signed(A) ::= plus_num(A). {
    // Token passthrough
}

signed(A) ::= minus_num(A). {
    // Token passthrough
}

// ============ ANALYZE ============

cmd(A) ::= ANALYZE. {
    A = synq_parse_analyze_or_reindex_stmt(pCtx,
        SYNQ_NO_SPAN,
        SYNQ_NO_SPAN,
        SYNTAQLITE_ANALYZE_OR_REINDEX_OP_ANALYZE);
}

cmd(A) ::= ANALYZE nm(X) dbnm(Y). {
    SyntaqliteTextSpan name_span = Y.z ? synq_span(pCtx, Y) : synq_span(pCtx, X);
    SyntaqliteTextSpan schema_span = Y.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_parse_analyze_or_reindex_stmt(pCtx, name_span, schema_span, SYNTAQLITE_ANALYZE_OR_REINDEX_OP_ANALYZE);
}

// ============ REINDEX ============

cmd(A) ::= REINDEX. {
    A = synq_parse_analyze_or_reindex_stmt(pCtx,
        SYNQ_NO_SPAN,
        SYNQ_NO_SPAN,
        SYNTAQLITE_ANALYZE_OR_REINDEX_OP_REINDEX);
}

cmd(A) ::= REINDEX nm(X) dbnm(Y). {
    SyntaqliteTextSpan name_span = Y.z ? synq_span(pCtx, Y) : synq_span(pCtx, X);
    SyntaqliteTextSpan schema_span = Y.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_parse_analyze_or_reindex_stmt(pCtx, name_span, schema_span, 1);
}

// ============ ATTACH / DETACH ============

cmd(A) ::= ATTACH database_kw_opt expr(F) AS expr(D) key_opt(K). {
    A = synq_parse_attach_stmt(pCtx, F, D, K);
}

cmd(A) ::= DETACH database_kw_opt expr(D). {
    A = synq_parse_detach_stmt(pCtx, D);
}

database_kw_opt ::= DATABASE. {
    // Keyword consumed, no value needed
}

database_kw_opt ::= . {
    // Empty
}

key_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

key_opt(A) ::= KEY expr(X). {
    A = synq_pass(pCtx, X);
}

// ============ VACUUM ============

cmd(A) ::= VACUUM vinto(Y). {
    A = synq_parse_vacuum_stmt(pCtx,
        SYNQ_NO_SPAN,
        Y);
}

cmd(A) ::= VACUUM nm(X) vinto(Y). {
    A = synq_parse_vacuum_stmt(pCtx,
        synq_span(pCtx, X),
        Y);
}

vinto(A) ::= INTO expr(X). {
    A = synq_pass(pCtx, X);
}

vinto(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

// ============ EXPLAIN ============

ecmd(A) ::= explain(E) cmdx(B) SEMI. {
    (void)E;
    A = synq_pass(pCtx, B);
}

explain(A) ::= EXPLAIN. {
    A = 1;
    pCtx->pending_explain_mode = 1;
}

explain(A) ::= EXPLAIN QUERY PLAN. {
    A = 2;
    pCtx->pending_explain_mode = 2;
}

// ============ CREATE INDEX ============

cmd(A) ::= createkw uniqueflag(U) INDEX ifnotexists(NE) nm(X) dbnm(D) ON nm(Y) LP sortlist(Z) RP where_opt(W). {
    SyntaqliteTextSpan idx_name = D.z ? synq_span(pCtx, D) : synq_span(pCtx, X);
    SyntaqliteTextSpan idx_schema = D.z ? synq_span(pCtx, X) : SYNQ_NO_SPAN;
    A = synq_parse_create_index_stmt(pCtx,
        idx_name,
        idx_schema,
        synq_span(pCtx, Y),
        (SyntaqliteBool)U,
        (SyntaqliteBool)NE,
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
    SyntaqliteTextSpan view_name = Z.z ? synq_span(pCtx, Z) : synq_span(pCtx, Y);
    SyntaqliteTextSpan view_schema = Z.z ? synq_span(pCtx, Y) : SYNQ_NO_SPAN;
    A = synq_parse_create_view_stmt(pCtx,
        view_name,
        view_schema,
        (SyntaqliteBool)T,
        (SyntaqliteBool)E,
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