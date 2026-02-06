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

// ============ WITH for DML ============
// The 'with' nonterminal is used by DML statements (DELETE/UPDATE/INSERT).
// It coexists with the existing CTE-select rules (parser resolves via lookahead).

with(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

with(A) ::= WITH wqlist(W). {
    A = W;
}

with(A) ::= WITH RECURSIVE wqlist(W). {
    // Tag the CTE list as recursive by wrapping in a marker.
    // We'll handle this in the cmd rules when building the final statement.
    // Use high bit to signal recursive: set bit 31.
    // Actually, we need a cleaner approach. Store the recursive flag
    // by creating a CteList node and using a convention.
    // For now, negate the list ID... no, IDs are u32.
    // Simplest approach: store a sentinel node that wraps (recursive, cte_list).
    // Use the WithClause node with a null select, then extract in cmd rules.
    A = ast_with_clause(pCtx->astCtx, 1, W, SYNTAQLITE_NULL_NODE);
}

// ============ DELETE ============

cmd(A) ::= with(W) DELETE FROM xfullname(X) indexed_opt(I) where_opt_ret(E). {
    (void)I;
    uint32_t del = ast_delete_stmt(pCtx->astCtx, X, E);
    if (W != SYNTAQLITE_NULL_NODE) {
        // Check if W is a WithClause (recursive WITH) or a CteList (non-recursive WITH)
        SyntaqliteNode *wn = AST_NODE(pCtx->astCtx->ast, W);
        if (wn->tag == SYNTAQLITE_NODE_WITH_CLAUSE) {
            // Recursive WITH - extract ctes from the wrapper
            A = ast_with_clause(pCtx->astCtx, 1, wn->with_clause.ctes, del);
        } else {
            // Non-recursive WITH - W is a CteList directly
            A = ast_with_clause(pCtx->astCtx, 0, W, del);
        }
    } else {
        A = del;
    }
}

// ============ UPDATE ============

cmd(A) ::= with(W) UPDATE orconf(R) xfullname(X) indexed_opt(I) SET setlist(Y) from(F) where_opt_ret(E). {
    (void)I;
    uint32_t upd = ast_update_stmt(pCtx->astCtx, (SyntaqliteConflictAction)R, X, Y, F, E);
    if (W != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *wn = AST_NODE(pCtx->astCtx->ast, W);
        if (wn->tag == SYNTAQLITE_NODE_WITH_CLAUSE) {
            A = ast_with_clause(pCtx->astCtx, 1, wn->with_clause.ctes, upd);
        } else {
            A = ast_with_clause(pCtx->astCtx, 0, W, upd);
        }
    } else {
        A = upd;
    }
}

// ============ INSERT ============

cmd(A) ::= with(W) insert_cmd(R) INTO xfullname(X) idlist_opt(F) select(S) upsert(U). {
    (void)U;
    uint32_t ins = ast_insert_stmt(pCtx->astCtx, (SyntaqliteConflictAction)R, X, F, S);
    if (W != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *wn = AST_NODE(pCtx->astCtx->ast, W);
        if (wn->tag == SYNTAQLITE_NODE_WITH_CLAUSE) {
            A = ast_with_clause(pCtx->astCtx, 1, wn->with_clause.ctes, ins);
        } else {
            A = ast_with_clause(pCtx->astCtx, 0, W, ins);
        }
    } else {
        A = ins;
    }
}

cmd(A) ::= with(W) insert_cmd(R) INTO xfullname(X) idlist_opt(F) DEFAULT VALUES returning. {
    uint32_t ins = ast_insert_stmt(pCtx->astCtx, (SyntaqliteConflictAction)R, X, F, SYNTAQLITE_NULL_NODE);
    if (W != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *wn = AST_NODE(pCtx->astCtx->ast, W);
        if (wn->tag == SYNTAQLITE_NODE_WITH_CLAUSE) {
            A = ast_with_clause(pCtx->astCtx, 1, wn->with_clause.ctes, ins);
        } else {
            A = ast_with_clause(pCtx->astCtx, 0, W, ins);
        }
    } else {
        A = ins;
    }
}

// ============ INSERT command type ============

insert_cmd(A) ::= INSERT orconf(R). {
    A = R;
}

insert_cmd(A) ::= REPLACE. {
    A = (int)SYNTAQLITE_CONFLICT_ACTION_REPLACE;
}

// ============ OR conflict resolution ============

orconf(A) ::= . {
    A = (int)SYNTAQLITE_CONFLICT_ACTION_DEFAULT;
}

orconf(A) ::= OR resolvetype(X). {
    A = X;
}

resolvetype(A) ::= raisetype(X). {
    // raisetype: ROLLBACK=1, ABORT=2, FAIL=3 (SyntaqliteRaiseType enum values)
    // ConflictAction: ROLLBACK=1, ABORT=2, FAIL=3 (same values, direct passthrough)
    A = X;
}

resolvetype(A) ::= IGNORE. {
    A = (int)SYNTAQLITE_CONFLICT_ACTION_IGNORE;
}

resolvetype(A) ::= REPLACE. {
    A = (int)SYNTAQLITE_CONFLICT_ACTION_REPLACE;
}

// ============ xfullname (DML table reference) ============

xfullname(A) ::= nm(X). {
    A = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, X), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
}

xfullname(A) ::= nm(X) DOT nm(Y). {
    A = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, Y), syntaqlite_span(pCtx, X), SYNTAQLITE_NO_SPAN);
}

xfullname(A) ::= nm(X) DOT nm(Y) AS nm(Z). {
    A = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, Y), syntaqlite_span(pCtx, X), syntaqlite_span(pCtx, Z));
}

xfullname(A) ::= nm(X) AS nm(Z). {
    A = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, X), SYNTAQLITE_NO_SPAN, syntaqlite_span(pCtx, Z));
}

// ============ indexed_opt (ignore index hints in AST) ============

indexed_opt(A) ::= . {
    A.z = NULL; A.n = 0;
}

indexed_opt(A) ::= indexed_by(A). {
    // Token passthrough
}

// ============ where_opt_ret (WHERE with optional RETURNING) ============

where_opt_ret(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

where_opt_ret(A) ::= WHERE expr(X). {
    A = X;
}

where_opt_ret(A) ::= RETURNING selcollist(X). {
    // Ignore RETURNING clause for now (just discard the column list)
    (void)X;
    A = SYNTAQLITE_NULL_NODE;
}

where_opt_ret(A) ::= WHERE expr(X) RETURNING selcollist(Y). {
    // Keep WHERE, ignore RETURNING
    (void)Y;
    A = X;
}

// ============ SET list (UPDATE assignments) ============

setlist(A) ::= setlist(L) COMMA nm(X) EQ expr(Y). {
    uint32_t clause = ast_set_clause(pCtx->astCtx,
        syntaqlite_span(pCtx, X), SYNTAQLITE_NULL_NODE, Y);
    A = ast_set_clause_list_append(pCtx->astCtx, L, clause);
}

setlist(A) ::= setlist(L) COMMA LP idlist(X) RP EQ expr(Y). {
    uint32_t clause = ast_set_clause(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, X, Y);
    A = ast_set_clause_list_append(pCtx->astCtx, L, clause);
}

setlist(A) ::= nm(X) EQ expr(Y). {
    uint32_t clause = ast_set_clause(pCtx->astCtx,
        syntaqlite_span(pCtx, X), SYNTAQLITE_NULL_NODE, Y);
    A = ast_set_clause_list(pCtx->astCtx, clause);
}

setlist(A) ::= LP idlist(X) RP EQ expr(Y). {
    uint32_t clause = ast_set_clause(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, X, Y);
    A = ast_set_clause_list(pCtx->astCtx, clause);
}

// ============ Column list for INSERT ============

idlist_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

idlist_opt(A) ::= LP idlist(X) RP. {
    A = X;
}

// ============ UPSERT (stub - ignore ON CONFLICT for now) ============

upsert(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

upsert(A) ::= RETURNING selcollist(X). {
    (void)X;
    A = SYNTAQLITE_NULL_NODE;
}

upsert(A) ::= ON CONFLICT LP sortlist(T) RP where_opt(TW) DO UPDATE SET setlist(Z) where_opt(W) upsert(N). {
    (void)T; (void)TW; (void)Z; (void)W; (void)N;
    A = SYNTAQLITE_NULL_NODE;
}

upsert(A) ::= ON CONFLICT LP sortlist(T) RP where_opt(TW) DO NOTHING upsert(N). {
    (void)T; (void)TW; (void)N;
    A = SYNTAQLITE_NULL_NODE;
}

upsert(A) ::= ON CONFLICT DO NOTHING returning. {
    A = SYNTAQLITE_NULL_NODE;
}

upsert(A) ::= ON CONFLICT DO UPDATE SET setlist(Z) where_opt(W) returning. {
    (void)Z; (void)W;
    A = SYNTAQLITE_NULL_NODE;
}

// ============ RETURNING (stub) ============

returning ::= RETURNING selcollist(X). {
    (void)X;
}

returning ::= . {
    // empty
}
