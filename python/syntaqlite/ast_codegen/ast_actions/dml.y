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

// ============ WITH for DML ============
// The 'with' nonterminal is used by DML statements (DELETE/UPDATE/INSERT).
// It coexists with the existing CTE-select rules (parser resolves via lookahead).

with(A) ::= . {
    A.cte_list = SYNQ_NULL_NODE;
    A.is_recursive = 0;
}

with(A) ::= WITH wqlist(W). {
    A.cte_list = W;
    A.is_recursive = 0;
}

with(A) ::= WITH RECURSIVE wqlist(W). {
    A.cte_list = W;
    A.is_recursive = 1;
}

// ============ DELETE ============

cmd(A) ::= with(W) DELETE FROM xfullname(X) indexed_opt(I) where_opt_ret(E). {
    (void)I;
    uint32_t del = synq_ast_delete_stmt(pCtx->astCtx, X, E);
    if (W.cte_list != SYNQ_NULL_NODE) {
        A = synq_ast_with_clause(pCtx->astCtx, W.is_recursive, W.cte_list, del);
    } else {
        A = del;
    }
}

// ============ UPDATE ============

cmd(A) ::= with(W) UPDATE orconf(R) xfullname(X) indexed_opt(I) SET setlist(Y) from(F) where_opt_ret(E). {
    (void)I;
    uint32_t upd = synq_ast_update_stmt(pCtx->astCtx, (SynqConflictAction)R, X, Y, F, E);
    if (W.cte_list != SYNQ_NULL_NODE) {
        A = synq_ast_with_clause(pCtx->astCtx, W.is_recursive, W.cte_list, upd);
    } else {
        A = upd;
    }
}

// ============ INSERT ============

cmd(A) ::= with(W) insert_cmd(R) INTO xfullname(X) idlist_opt(F) select(S) upsert(U). {
    (void)U;
    uint32_t ins = synq_ast_insert_stmt(pCtx->astCtx, (SynqConflictAction)R, X, F, S);
    if (W.cte_list != SYNQ_NULL_NODE) {
        A = synq_ast_with_clause(pCtx->astCtx, W.is_recursive, W.cte_list, ins);
    } else {
        A = ins;
    }
}

cmd(A) ::= with(W) insert_cmd(R) INTO xfullname(X) idlist_opt(F) DEFAULT VALUES returning. {
    uint32_t ins = synq_ast_insert_stmt(pCtx->astCtx, (SynqConflictAction)R, X, F, SYNQ_NULL_NODE);
    if (W.cte_list != SYNQ_NULL_NODE) {
        A = synq_ast_with_clause(pCtx->astCtx, W.is_recursive, W.cte_list, ins);
    } else {
        A = ins;
    }
}

// ============ INSERT command type ============

insert_cmd(A) ::= INSERT orconf(R). {
    A = R;
}

insert_cmd(A) ::= REPLACE. {
    A = (int)SYNQ_CONFLICT_ACTION_REPLACE;
}

// ============ OR conflict resolution ============

orconf(A) ::= . {
    A = (int)SYNQ_CONFLICT_ACTION_DEFAULT;
}

orconf(A) ::= OR resolvetype(X). {
    A = X;
}

resolvetype(A) ::= raisetype(X). {
    // raisetype: ROLLBACK=1, ABORT=2, FAIL=3 (SynqRaiseType enum values)
    // ConflictAction: ROLLBACK=1, ABORT=2, FAIL=3 (same values, direct passthrough)
    A = X;
}

resolvetype(A) ::= IGNORE. {
    A = (int)SYNQ_CONFLICT_ACTION_IGNORE;
}

resolvetype(A) ::= REPLACE. {
    A = (int)SYNQ_CONFLICT_ACTION_REPLACE;
}

// ============ xfullname (DML table reference) ============

xfullname(A) ::= nm(X). {
    A = synq_ast_table_ref(pCtx->astCtx,
        synq_span(pCtx, X), SYNQ_NO_SPAN, SYNQ_NO_SPAN);
}

xfullname(A) ::= nm(X) DOT nm(Y). {
    A = synq_ast_table_ref(pCtx->astCtx,
        synq_span(pCtx, Y), synq_span(pCtx, X), SYNQ_NO_SPAN);
}

xfullname(A) ::= nm(X) DOT nm(Y) AS nm(Z). {
    A = synq_ast_table_ref(pCtx->astCtx,
        synq_span(pCtx, Y), synq_span(pCtx, X), synq_span(pCtx, Z));
}

xfullname(A) ::= nm(X) AS nm(Z). {
    A = synq_ast_table_ref(pCtx->astCtx,
        synq_span(pCtx, X), SYNQ_NO_SPAN, synq_span(pCtx, Z));
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
    A = SYNQ_NULL_NODE;
}

where_opt_ret(A) ::= WHERE expr(X). {
    A = X;
}

where_opt_ret(A) ::= RETURNING selcollist(X). {
    // Ignore RETURNING clause for now (just discard the column list)
    (void)X;
    A = SYNQ_NULL_NODE;
}

where_opt_ret(A) ::= WHERE expr(X) RETURNING selcollist(Y). {
    // Keep WHERE, ignore RETURNING
    (void)Y;
    A = X;
}

// ============ SET list (UPDATE assignments) ============

setlist(A) ::= setlist(L) COMMA nm(X) EQ expr(Y). {
    uint32_t clause = synq_ast_set_clause(pCtx->astCtx,
        synq_span(pCtx, X), SYNQ_NULL_NODE, Y);
    A = synq_ast_set_clause_list_append(pCtx->astCtx, L, clause);
}

setlist(A) ::= setlist(L) COMMA LP idlist(X) RP EQ expr(Y). {
    uint32_t clause = synq_ast_set_clause(pCtx->astCtx,
        SYNQ_NO_SPAN, X, Y);
    A = synq_ast_set_clause_list_append(pCtx->astCtx, L, clause);
}

setlist(A) ::= nm(X) EQ expr(Y). {
    uint32_t clause = synq_ast_set_clause(pCtx->astCtx,
        synq_span(pCtx, X), SYNQ_NULL_NODE, Y);
    A = synq_ast_set_clause_list(pCtx->astCtx, clause);
}

setlist(A) ::= LP idlist(X) RP EQ expr(Y). {
    uint32_t clause = synq_ast_set_clause(pCtx->astCtx,
        SYNQ_NO_SPAN, X, Y);
    A = synq_ast_set_clause_list(pCtx->astCtx, clause);
}

// ============ Column list for INSERT ============

idlist_opt(A) ::= . {
    A = SYNQ_NULL_NODE;
}

idlist_opt(A) ::= LP idlist(X) RP. {
    A = X;
}

// ============ UPSERT (stub - ignore ON CONFLICT for now) ============

upsert(A) ::= . {
    A = SYNQ_NULL_NODE;
}

upsert(A) ::= RETURNING selcollist(X). {
    (void)X;
    A = SYNQ_NULL_NODE;
}

upsert(A) ::= ON CONFLICT LP sortlist(T) RP where_opt(TW) DO UPDATE SET setlist(Z) where_opt(W) upsert(N). {
    (void)T; (void)TW; (void)Z; (void)W; (void)N;
    A = SYNQ_NULL_NODE;
}

upsert(A) ::= ON CONFLICT LP sortlist(T) RP where_opt(TW) DO NOTHING upsert(N). {
    (void)T; (void)TW; (void)N;
    A = SYNQ_NULL_NODE;
}

upsert(A) ::= ON CONFLICT DO NOTHING returning. {
    A = SYNQ_NULL_NODE;
}

upsert(A) ::= ON CONFLICT DO UPDATE SET setlist(Z) where_opt(W) returning. {
    (void)Z; (void)W;
    A = SYNQ_NULL_NODE;
}

// ============ RETURNING (stub) ============

returning ::= RETURNING selcollist(X). {
    (void)X;
}

returning ::= . {
    // empty
}
