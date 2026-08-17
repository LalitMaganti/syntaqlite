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

%type on_using {SynqOnUsingValue}
%type joinop {SyntaqliteJoinType}
%type indexed_by {SynqParseToken}

// ============ FROM clause table sources ============

// stl_prefix carries the accumulated seltablist plus pending join type
stl_prefix(A) ::= seltablist(A) joinop(Y). {
    A = synq_parse_join_prefix(pCtx, A, Y);
}

stl_prefix(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

// Simple table reference: FROM t, FROM t AS x, FROM schema.t
seltablist(A) ::= stl_prefix(A) nm(Y) dbnm(D) as(Z) on_using(N). {
    uint32_t alias = Z;
    SyntaqliteTextSpan table_name;
    SyntaqliteTextSpan schema;
    if (D.z != NULL) {
        table_name = synq_span_dequote(pCtx, D);
        schema = synq_span_dequote(pCtx, Y);
    } else {
        table_name = synq_span_dequote(pCtx, Y);
        schema = SYNQ_NO_SPAN;
    }
    uint32_t tref = synq_parse_table_ref(pCtx, table_name, schema,
                                         SYNTAQLITE_BOOL_FALSE,
                                         alias, SYNTAQLITE_NULL_NODE,
                                         SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
    if (A == SYNTAQLITE_NULL_NODE) {
        A = tref;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->ast, A);
        A = synq_parse_join_clause(pCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            tref, N.on_expr, N.using_cols);
    }
}

// The hint constrains the planner and errors on a missing index, so it must survive.
seltablist(A) ::= stl_prefix(A) nm(Y) dbnm(D) as(Z) indexed_by(I) on_using(N). {
    uint32_t alias = Z;
    SyntaqliteTextSpan table_name;
    SyntaqliteTextSpan schema;
    if (D.z != NULL) {
        table_name = synq_span_dequote(pCtx, D);
        schema = synq_span_dequote(pCtx, Y);
    } else {
        table_name = synq_span_dequote(pCtx, Y);
        schema = SYNQ_NO_SPAN;
    }
    SyntaqliteIndexHint ih = (I.z != NULL) ? SYNTAQLITE_INDEX_HINT_INDEXED
                           : (I.n == 1)    ? SYNTAQLITE_INDEX_HINT_NOT_INDEXED
                           :                 SYNTAQLITE_INDEX_HINT_DEFAULT;
    uint32_t tref = synq_parse_table_ref(pCtx, table_name, schema,
                                         SYNTAQLITE_BOOL_FALSE,
                                         alias, SYNTAQLITE_NULL_NODE,
                                         ih, synq_span(pCtx, I));
    if (A == SYNTAQLITE_NULL_NODE) {
        A = tref;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->ast, A);
        A = synq_parse_join_clause(pCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            tref, N.on_expr, N.using_cols);
    }
}

// Table-valued function: FROM t(args) — `has_parens=TRUE` so an
// empty-args call `FROM t()` round-trips as `FROM t()`, distinct
// from the bare `FROM t` form.
seltablist(A) ::= stl_prefix(A) nm(Y) dbnm(D) LP exprlist(E) RP as(Z) on_using(N). {
    uint32_t alias = Z;
    SyntaqliteTextSpan table_name;
    SyntaqliteTextSpan schema;
    if (D.z != NULL) {
        table_name = synq_span_dequote(pCtx, D);
        schema = synq_span_dequote(pCtx, Y);
    } else {
        table_name = synq_span_dequote(pCtx, Y);
        schema = SYNQ_NO_SPAN;
    }
    uint32_t tref = synq_parse_table_ref(pCtx, table_name, schema,
                                         SYNTAQLITE_BOOL_TRUE,
                                         alias, E,
                                         SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
    if (A == SYNTAQLITE_NULL_NODE) {
        A = tref;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->ast, A);
        A = synq_parse_join_clause(pCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            tref, N.on_expr, N.using_cols);
    }
}

// Subquery table source: FROM (SELECT ...) AS t
seltablist(A) ::= stl_prefix(A) LP select(S) RP as(Z) on_using(N). {
    pCtx->saw_subquery = 1;
    uint32_t alias = Z;
    uint32_t sub = synq_parse_subquery_table_source(pCtx, S, alias);
    if (A == SYNTAQLITE_NULL_NODE) {
        A = sub;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->ast, A);
        A = synq_parse_join_clause(pCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            sub, N.on_expr, N.using_cols);
    }
}

// Parenthesized seltablist: FROM (a, b) - pass through
seltablist(A) ::= stl_prefix(A) LP seltablist(F) RP as(Z) on_using(N). {
    (void)Z; (void)N;
    if (A == SYNTAQLITE_NULL_NODE) {
        A = synq_pass(pCtx, F);
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->ast, A);
        A = synq_parse_join_clause(pCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            F, N.on_expr, N.using_cols);
    }
}

// ============ Join operators ============

joinop(X) ::= COMMA|JOIN(OP). {
    X = (OP.type == SYNTAQLITE_TK_COMMA)
        ? SYNTAQLITE_JOIN_TYPE_COMMA
        : SYNTAQLITE_JOIN_TYPE_INNER;
}

joinop(X) ::= JOIN_KW(A) JOIN. {
    X = synq_join_type(&A, NULL, NULL);
}

joinop(X) ::= JOIN_KW(A) nm(B) JOIN. {
    X = synq_join_type(&A, &B, NULL);
}

joinop(X) ::= JOIN_KW(A) nm(B) nm(C) JOIN. {
    X = synq_join_type(&A, &B, &C);
}

// ============ ON / USING clauses ============

on_using(N) ::= ON expr(E). {
    N.on_expr = E;
    N.using_cols = SYNTAQLITE_NULL_NODE;
}

on_using(N) ::= USING LP idlist(L) RP. {
    N.on_expr = SYNTAQLITE_NULL_NODE;
    N.using_cols = L;
}

on_using(N) ::= . [OR] {
    N.on_expr = SYNTAQLITE_NULL_NODE;
    N.using_cols = SYNTAQLITE_NULL_NODE;
}

// ============ INDEXED BY (stub - ignore in AST) ============

indexed_by(A) ::= INDEXED BY nm(X). {
    A = X;
}

indexed_by(A) ::= NOT INDEXED. {
    A.z = NULL; A.n = 1;
}

// ============ ID list (for USING clause) ============

idlist(A) ::= idlist(A) COMMA nm(Y). {
    uint32_t col = synq_parse_column_ref(pCtx,
        synq_span_dequote(pCtx, Y), SYNQ_NO_SPAN, SYNQ_NO_SPAN);
    A = synq_parse_expr_list(pCtx, A, col);
}

idlist(A) ::= nm(Y). {
    uint32_t col = synq_parse_column_ref(pCtx,
        synq_span_dequote(pCtx, Y), SYNQ_NO_SPAN, SYNQ_NO_SPAN);
    A = synq_parse_expr_list(pCtx, SYNTAQLITE_NULL_NODE, col);
}
