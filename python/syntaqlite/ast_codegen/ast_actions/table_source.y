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

// ============ FROM clause table sources ============

// stl_prefix carries the accumulated seltablist plus pending join type
stl_prefix(A) ::= seltablist(A) joinop(Y). {
    A = ast_join_prefix(pCtx->astCtx, A, (SyntaqliteJoinType)Y);
}

stl_prefix(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

// Simple table reference: FROM t, FROM t AS x, FROM schema.t
seltablist(A) ::= stl_prefix(A) nm(Y) dbnm(D) as(Z) on_using(N). {
    SyntaqliteSourceSpan alias = (Z.z != NULL) ? syntaqlite_span(pCtx, Z) : SYNTAQLITE_NO_SPAN;
    SyntaqliteSourceSpan table_name;
    SyntaqliteSourceSpan schema;
    if (D.z != NULL) {
        table_name = syntaqlite_span(pCtx, D);
        schema = syntaqlite_span(pCtx, Y);
    } else {
        table_name = syntaqlite_span(pCtx, Y);
        schema = SYNTAQLITE_NO_SPAN;
    }
    uint32_t tref = ast_table_ref(pCtx->astCtx, table_name, schema, alias);
    if (A == SYNTAQLITE_NULL_NODE) {
        A = tref;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->astCtx->ast, A);
        A = ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            tref, N.on_expr, N.using_cols);
    }
}

// Table reference with INDEXED BY (ignore index hint in AST)
seltablist(A) ::= stl_prefix(A) nm(Y) dbnm(D) as(Z) indexed_by(I) on_using(N). {
    (void)I;
    SyntaqliteSourceSpan alias = (Z.z != NULL) ? syntaqlite_span(pCtx, Z) : SYNTAQLITE_NO_SPAN;
    SyntaqliteSourceSpan table_name;
    SyntaqliteSourceSpan schema;
    if (D.z != NULL) {
        table_name = syntaqlite_span(pCtx, D);
        schema = syntaqlite_span(pCtx, Y);
    } else {
        table_name = syntaqlite_span(pCtx, Y);
        schema = SYNTAQLITE_NO_SPAN;
    }
    uint32_t tref = ast_table_ref(pCtx->astCtx, table_name, schema, alias);
    if (A == SYNTAQLITE_NULL_NODE) {
        A = tref;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->astCtx->ast, A);
        A = ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            tref, N.on_expr, N.using_cols);
    }
}

// Table-valued function: FROM t(args)
seltablist(A) ::= stl_prefix(A) nm(Y) dbnm(D) LP exprlist(E) RP as(Z) on_using(N). {
    (void)E;
    SyntaqliteSourceSpan alias = (Z.z != NULL) ? syntaqlite_span(pCtx, Z) : SYNTAQLITE_NO_SPAN;
    SyntaqliteSourceSpan table_name;
    SyntaqliteSourceSpan schema;
    if (D.z != NULL) {
        table_name = syntaqlite_span(pCtx, D);
        schema = syntaqlite_span(pCtx, Y);
    } else {
        table_name = syntaqlite_span(pCtx, Y);
        schema = SYNTAQLITE_NO_SPAN;
    }
    uint32_t tref = ast_table_ref(pCtx->astCtx, table_name, schema, alias);
    if (A == SYNTAQLITE_NULL_NODE) {
        A = tref;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->astCtx->ast, A);
        A = ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            tref, N.on_expr, N.using_cols);
    }
}

// Subquery table source: FROM (SELECT ...) AS t
seltablist(A) ::= stl_prefix(A) LP select(S) RP as(Z) on_using(N). {
    SyntaqliteSourceSpan alias = (Z.z != NULL) ? syntaqlite_span(pCtx, Z) : SYNTAQLITE_NO_SPAN;
    uint32_t sub = ast_subquery_table_source(pCtx->astCtx, S, alias);
    if (A == SYNTAQLITE_NULL_NODE) {
        A = sub;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->astCtx->ast, A);
        A = ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            sub, N.on_expr, N.using_cols);
    }
}

// Parenthesized seltablist: FROM (a, b) - pass through
seltablist(A) ::= stl_prefix(A) LP seltablist(F) RP as(Z) on_using(N). {
    (void)Z; (void)N;
    if (A == SYNTAQLITE_NULL_NODE) {
        A = F;
    } else {
        SyntaqliteNode *pfx = AST_NODE(&pCtx->astCtx->ast, A);
        A = ast_join_clause(pCtx->astCtx,
            pfx->join_prefix.join_type,
            pfx->join_prefix.source,
            F, N.on_expr, N.using_cols);
    }
}

// ============ Join operators ============

joinop(X) ::= COMMA|JOIN(OP). {
    X = (OP.type == TK_COMMA)
        ? (int)SYNTAQLITE_JOIN_TYPE_COMMA
        : (int)SYNTAQLITE_JOIN_TYPE_INNER;
}

joinop(X) ::= JOIN_KW(A) JOIN. {
    // Single keyword: LEFT, RIGHT, INNER, OUTER, CROSS, NATURAL, FULL
    if (A.n == 4 && (A.z[0] == 'L' || A.z[0] == 'l')) {
        X = (int)SYNTAQLITE_JOIN_TYPE_LEFT;
    } else if (A.n == 5 && (A.z[0] == 'R' || A.z[0] == 'r')) {
        X = (int)SYNTAQLITE_JOIN_TYPE_RIGHT;
    } else if (A.n == 5 && (A.z[0] == 'I' || A.z[0] == 'i')) {
        X = (int)SYNTAQLITE_JOIN_TYPE_INNER;
    } else if (A.n == 5 && (A.z[0] == 'O' || A.z[0] == 'o')) {
        // OUTER alone is not valid but treat as INNER
        X = (int)SYNTAQLITE_JOIN_TYPE_INNER;
    } else if (A.n == 5 && (A.z[0] == 'C' || A.z[0] == 'c')) {
        X = (int)SYNTAQLITE_JOIN_TYPE_CROSS;
    } else if (A.n == 7 && (A.z[0] == 'N' || A.z[0] == 'n')) {
        X = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
    } else if (A.n == 4 && (A.z[0] == 'F' || A.z[0] == 'f')) {
        X = (int)SYNTAQLITE_JOIN_TYPE_FULL;
    } else {
        X = (int)SYNTAQLITE_JOIN_TYPE_INNER;
    }
}

joinop(X) ::= JOIN_KW(A) nm(B) JOIN. {
    // Two keywords: LEFT OUTER, NATURAL LEFT, NATURAL RIGHT, etc.
    (void)B;
    if (A.n == 7 && (A.z[0] == 'N' || A.z[0] == 'n')) {
        // NATURAL + something
        if (B.n == 4 && (B.z[0] == 'L' || B.z[0] == 'l')) {
            X = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_LEFT;
        } else if (B.n == 5 && (B.z[0] == 'R' || B.z[0] == 'r')) {
            X = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_RIGHT;
        } else if (B.n == 5 && (B.z[0] == 'I' || B.z[0] == 'i')) {
            X = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
        } else if (B.n == 4 && (B.z[0] == 'F' || B.z[0] == 'f')) {
            X = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_FULL;
        } else if (B.n == 5 && (B.z[0] == 'C' || B.z[0] == 'c')) {
            // NATURAL CROSS -> just CROSS
            X = (int)SYNTAQLITE_JOIN_TYPE_CROSS;
        } else {
            X = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
        }
    } else if (A.n == 4 && (A.z[0] == 'L' || A.z[0] == 'l')) {
        // LEFT OUTER
        X = (int)SYNTAQLITE_JOIN_TYPE_LEFT;
    } else if (A.n == 5 && (A.z[0] == 'R' || A.z[0] == 'r')) {
        // RIGHT OUTER
        X = (int)SYNTAQLITE_JOIN_TYPE_RIGHT;
    } else if (A.n == 4 && (A.z[0] == 'F' || A.z[0] == 'f')) {
        // FULL OUTER
        X = (int)SYNTAQLITE_JOIN_TYPE_FULL;
    } else {
        X = (int)SYNTAQLITE_JOIN_TYPE_INNER;
    }
}

joinop(X) ::= JOIN_KW(A) nm(B) nm(C) JOIN. {
    // Three keywords: NATURAL LEFT OUTER, NATURAL RIGHT OUTER, etc.
    (void)B; (void)C;
    if (A.n == 7 && (A.z[0] == 'N' || A.z[0] == 'n')) {
        // NATURAL X OUTER
        if (B.n == 4 && (B.z[0] == 'L' || B.z[0] == 'l')) {
            X = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_LEFT;
        } else if (B.n == 5 && (B.z[0] == 'R' || B.z[0] == 'r')) {
            X = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_RIGHT;
        } else if (B.n == 4 && (B.z[0] == 'F' || B.z[0] == 'f')) {
            X = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_FULL;
        } else {
            X = (int)SYNTAQLITE_JOIN_TYPE_NATURAL_INNER;
        }
    } else {
        X = (int)SYNTAQLITE_JOIN_TYPE_INNER;
    }
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
    uint32_t col = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, Y), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
    A = ast_expr_list_append(pCtx->astCtx, A, col);
}

idlist(A) ::= nm(Y). {
    uint32_t col = ast_column_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, Y), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
    A = ast_expr_list(pCtx->astCtx, col);
}
