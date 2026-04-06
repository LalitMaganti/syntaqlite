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

%type with {SynqWithValue}
%type insert_cmd {int}
%type orconf {int}
%type resolvetype {int}
%type indexed_opt {SynqParseToken}
%type where_opt_ret {SynqWhereRetValue}
%type upsert {SynqUpsertValue}
%type returning {uint32_t}

// ============ WITH for DML ============
// The 'with' nonterminal is used by DML statements (DELETE/UPDATE/INSERT).
// It coexists with the existing CTE-select rules (parser resolves via lookahead).

with(A) ::= . {
    A.cte_list = SYNTAQLITE_NULL_NODE;
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
// Extended form: accepts optional ORDER BY / LIMIT (SQLITE_ENABLE_UPDATE_DELETE_LIMIT).

cmd(A) ::= with(W) DELETE FROM xfullname(X) indexed_opt(I) where_opt_ret(E) orderby_opt(O) limit_opt(L). {
    if (O != SYNTAQLITE_NULL_NODE || L != SYNTAQLITE_NULL_NODE) {
        pCtx->saw_update_delete_limit = 1;
        if (!SYNQ_HAS_CFLAG(pCtx->env, SYNQ_CFLAG_IDX_ENABLE_UPDATE_DELETE_LIMIT)) {
            pCtx->error = 1;
        }
    }
    SyntaqliteIndexHint ih = (I.z != NULL) ? SYNTAQLITE_INDEX_HINT_INDEXED
                           : (I.n == 1)    ? SYNTAQLITE_INDEX_HINT_NOT_INDEXED
                           :                 SYNTAQLITE_INDEX_HINT_DEFAULT;
    uint32_t del = synq_parse_delete_stmt(pCtx, X, ih, synq_span(pCtx, I), E.where_expr, O, L, E.returning);
    if (W.cte_list != SYNTAQLITE_NULL_NODE) {
        A = synq_parse_with_clause(pCtx, W.is_recursive, W.cte_list, del);
    } else {
        A = del;
    }
}

// ============ UPDATE ============
// Extended form: accepts optional ORDER BY / LIMIT (SQLITE_ENABLE_UPDATE_DELETE_LIMIT).

cmd(A) ::= with(W) UPDATE orconf(R) xfullname(X) indexed_opt(I) SET setlist(Y) from(F) where_opt_ret(E) orderby_opt(O) limit_opt(L). {
    if (O != SYNTAQLITE_NULL_NODE || L != SYNTAQLITE_NULL_NODE) {
        pCtx->saw_update_delete_limit = 1;
        if (!SYNQ_HAS_CFLAG(pCtx->env, SYNQ_CFLAG_IDX_ENABLE_UPDATE_DELETE_LIMIT)) {
            pCtx->error = 1;
        }
    }
    SyntaqliteIndexHint ih = (I.z != NULL) ? SYNTAQLITE_INDEX_HINT_INDEXED
                           : (I.n == 1)    ? SYNTAQLITE_INDEX_HINT_NOT_INDEXED
                           :                 SYNTAQLITE_INDEX_HINT_DEFAULT;
    uint32_t upd = synq_parse_update_stmt(pCtx, (SyntaqliteConflictAction)R, X, ih, synq_span(pCtx, I), Y, F, E.where_expr, O, L, E.returning);
    if (W.cte_list != SYNTAQLITE_NULL_NODE) {
        A = synq_parse_with_clause(pCtx, W.is_recursive, W.cte_list, upd);
    } else {
        A = upd;
    }
}

// ============ INSERT ============

cmd(A) ::= with(W) insert_cmd(R) INTO xfullname(X) idlist_opt(F) select(S) upsert(U). {
    uint32_t ins = synq_parse_insert_stmt(pCtx, (SyntaqliteConflictAction)R, X, F, S, U.clauses, U.returning);
    if (W.cte_list != SYNTAQLITE_NULL_NODE) {
        A = synq_parse_with_clause(pCtx, W.is_recursive, W.cte_list, ins);
    } else {
        A = ins;
    }
}

cmd(A) ::= with(W) insert_cmd(R) INTO xfullname(X) idlist_opt(F) DEFAULT VALUES returning(V). {
    uint32_t ins = synq_parse_insert_stmt(pCtx, (SyntaqliteConflictAction)R, X, F, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, V);
    if (W.cte_list != SYNTAQLITE_NULL_NODE) {
        A = synq_parse_with_clause(pCtx, W.is_recursive, W.cte_list, ins);
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
    // raisetype: ROLLBACK=1, ABORT=2, FAIL=3 (SynqRaiseType enum values)
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
    A = synq_parse_table_ref(pCtx,
        synq_span_dequote(pCtx, X), SYNQ_NO_SPAN, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
}

xfullname(A) ::= nm(X) DOT nm(Y). {
    A = synq_parse_table_ref(pCtx,
        synq_span_dequote(pCtx, Y), synq_span_dequote(pCtx, X), SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
}

xfullname(A) ::= nm(X) DOT nm(Y) AS nm(Z). {
    uint32_t alias = synq_parse_ident_name(pCtx, synq_span_dequote(pCtx, Z));
    A = synq_parse_table_ref(pCtx,
        synq_span_dequote(pCtx, Y), synq_span_dequote(pCtx, X), alias, SYNTAQLITE_NULL_NODE);
}

xfullname(A) ::= nm(X) AS nm(Z). {
    uint32_t alias = synq_parse_ident_name(pCtx, synq_span_dequote(pCtx, Z));
    A = synq_parse_table_ref(pCtx,
        synq_span_dequote(pCtx, X), SYNQ_NO_SPAN, alias, SYNTAQLITE_NULL_NODE);
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
    A.where_expr = SYNTAQLITE_NULL_NODE;
    A.returning = SYNTAQLITE_NULL_NODE;
}

where_opt_ret(A) ::= WHERE expr(X). {
    A.where_expr = X;
    A.returning = SYNTAQLITE_NULL_NODE;
}

where_opt_ret(A) ::= RETURNING selcollist(X). {
    A.where_expr = SYNTAQLITE_NULL_NODE;
    A.returning = X;
}

where_opt_ret(A) ::= WHERE expr(X) RETURNING selcollist(Y). {
    A.where_expr = X;
    A.returning = Y;
}

// ============ SET list (UPDATE assignments) ============

setlist(A) ::= setlist(L) COMMA nm(X) EQ expr(Y). {
    uint32_t clause = synq_parse_set_clause(pCtx,
        synq_span(pCtx, X), SYNTAQLITE_NULL_NODE, Y);
    A = synq_parse_set_clause_list(pCtx, L, clause);
}

setlist(A) ::= setlist(L) COMMA LP idlist(X) RP EQ expr(Y). {
    uint32_t clause = synq_parse_set_clause(pCtx,
        SYNQ_NO_SPAN, X, Y);
    A = synq_parse_set_clause_list(pCtx, L, clause);
}

setlist(A) ::= nm(X) EQ expr(Y). {
    uint32_t clause = synq_parse_set_clause(pCtx,
        synq_span(pCtx, X), SYNTAQLITE_NULL_NODE, Y);
    A = synq_parse_set_clause_list(pCtx, SYNTAQLITE_NULL_NODE, clause);
}

setlist(A) ::= LP idlist(X) RP EQ expr(Y). {
    uint32_t clause = synq_parse_set_clause(pCtx,
        SYNQ_NO_SPAN, X, Y);
    A = synq_parse_set_clause_list(pCtx, SYNTAQLITE_NULL_NODE, clause);
}

// ============ Column list for INSERT ============

idlist_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

idlist_opt(A) ::= LP idlist(X) RP. {
    A = X;
}

// ============ UPSERT (ON CONFLICT clauses) ============

upsert(A) ::= . {
    A.clauses = SYNTAQLITE_NULL_NODE;
    A.returning = SYNTAQLITE_NULL_NODE;
}

upsert(A) ::= RETURNING selcollist(X). {
    A.clauses = SYNTAQLITE_NULL_NODE;
    A.returning = X;
}

upsert(A) ::= ON CONFLICT LP sortlist(T) RP where_opt(TW) DO UPDATE SET setlist(Z) where_opt(W) upsert(N). {
    uint32_t clause = synq_parse_upsert_clause(pCtx, T, TW, (SyntaqliteUpsertAction)SYNTAQLITE_UPSERT_ACTION_UPDATE, Z, W);
    A.clauses = synq_parse_upsert_clause_list(pCtx, N.clauses, clause);
    A.returning = N.returning;
}

upsert(A) ::= ON CONFLICT LP sortlist(T) RP where_opt(TW) DO NOTHING upsert(N). {
    uint32_t clause = synq_parse_upsert_clause(pCtx, T, TW, (SyntaqliteUpsertAction)SYNTAQLITE_UPSERT_ACTION_NOTHING, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.clauses = synq_parse_upsert_clause_list(pCtx, N.clauses, clause);
    A.returning = N.returning;
}

upsert(A) ::= ON CONFLICT DO NOTHING returning(V). {
    uint32_t clause = synq_parse_upsert_clause(pCtx, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, (SyntaqliteUpsertAction)SYNTAQLITE_UPSERT_ACTION_NOTHING, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.clauses = synq_parse_upsert_clause_list(pCtx, SYNTAQLITE_NULL_NODE, clause);
    A.returning = V;
}

upsert(A) ::= ON CONFLICT DO UPDATE SET setlist(Z) where_opt(W) returning(V). {
    uint32_t clause = synq_parse_upsert_clause(pCtx, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, (SyntaqliteUpsertAction)SYNTAQLITE_UPSERT_ACTION_UPDATE, Z, W);
    A.clauses = synq_parse_upsert_clause_list(pCtx, SYNTAQLITE_NULL_NODE, clause);
    A.returning = V;
}

// ============ RETURNING ============

returning(A) ::= RETURNING selcollist(X). {
    A = X;
}

returning(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}
