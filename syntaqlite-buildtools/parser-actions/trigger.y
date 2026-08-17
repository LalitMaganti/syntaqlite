// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// AST building actions for CREATE TRIGGER grammar rules.
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

%type trigger_time {int}
%type trnm {SynqParseToken}

// ============ CREATE TRIGGER ============

// The main cmd rule: completes the trigger with its body
cmd(A) ::= createkw trigger_decl(D) BEGIN trigger_cmd_list(S) END. {
    // D is a partially-built CreateTriggerStmt, fill in the body
    SyntaqliteNode *trig = AST_NODE(&pCtx->ast, D);
    trig->create_trigger_stmt.body = S;
    A = synq_pass(pCtx, D);
}

// trigger_decl builds a partial CreateTriggerStmt (without body)
trigger_decl(A) ::= temp(T) TRIGGER ifnotexists(NOERR) nm(B) dbnm(Z)
                    trigger_time(C) trigger_event(D)
                    ON fullname(E) foreach_clause when_clause(G). {
    SyntaqliteTextSpan trig_name = Z.z ? synq_span(pCtx, Z) : synq_span(pCtx, B);
    SyntaqliteTextSpan trig_schema = Z.z ? synq_span(pCtx, B) : SYNQ_NO_SPAN;
    A = synq_parse_create_trigger_stmt(pCtx,
        trig_name,
        trig_schema,
        (SyntaqliteBool)T,
        (SyntaqliteBool)NOERR,
        (SyntaqliteTriggerTiming)C,
        D,
        E,
        G,
        SYNTAQLITE_NULL_NODE);  // body filled in by cmd rule
}

// ============ Trigger timing ============

trigger_time(A) ::= BEFORE|AFTER(X). {
    A = (X.type == SYNTAQLITE_TK_BEFORE) ? (int)SYNTAQLITE_TRIGGER_TIMING_BEFORE
                               : (int)SYNTAQLITE_TRIGGER_TIMING_AFTER;
}

trigger_time(A) ::= INSTEAD OF. {
    A = (int)SYNTAQLITE_TRIGGER_TIMING_INSTEAD_OF;
}

trigger_time(A) ::= . {
    A = (int)SYNTAQLITE_TRIGGER_TIMING_BEFORE;
}

// ============ Trigger event ============

trigger_event(A) ::= DELETE|INSERT(X). {
    SyntaqliteTriggerEventType evt = (X.type == SYNTAQLITE_TK_DELETE)
        ? SYNTAQLITE_TRIGGER_EVENT_TYPE_DELETE
        : SYNTAQLITE_TRIGGER_EVENT_TYPE_INSERT;
    A = synq_parse_trigger_event(pCtx, evt, SYNTAQLITE_NULL_NODE);
}

trigger_event(A) ::= UPDATE. {
    A = synq_parse_trigger_event(pCtx,
        SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE, SYNTAQLITE_NULL_NODE);
}

trigger_event(A) ::= UPDATE OF idlist(X). {
    A = synq_parse_trigger_event(pCtx,
        SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE, X);
}

// ============ FOR EACH ROW (consumed, no value) ============

foreach_clause ::= . {
    // empty
}

foreach_clause ::= FOR EACH ROW. {
    // consumed
}

// ============ WHEN clause ============

when_clause(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

when_clause(A) ::= WHEN expr(X). {
    A = synq_pass(pCtx, X);
}

// ============ Trigger command list ============

trigger_cmd_list(A) ::= trigger_cmd_list(L) trigger_cmd(X) SEMI. {
    A = synq_parse_trigger_cmd_list(pCtx, L, X);
}

trigger_cmd_list(A) ::= trigger_cmd(X) SEMI. {
    A = synq_parse_trigger_cmd_list(pCtx, SYNTAQLITE_NULL_NODE, X);
}

// ============ trnm (table name in trigger context) ============

trnm(A) ::= nm(A). {
    // Token passthrough
}

trnm(A) ::= nm DOT nm(X). {
    A = X;
    pCtx->error = 1;
}

// ============ tridxby (index hints; SQLite rejects these in triggers) ============

tridxby ::= . {
    // empty
}

tridxby ::= INDEXED BY nm. {
    pCtx->error = 1;
}

tridxby ::= NOT INDEXED. {
    pCtx->error = 1;
}

// ============ Trigger commands ============

// UPDATE within trigger. Trigger bodies can't carry a WITH clause, so the
// CTE fields are always null.
trigger_cmd(A) ::= UPDATE orconf(R) trnm(X) tridxby SET setlist(Y) from(F) where_opt(Z) scanpt. {
    uint32_t tbl = synq_parse_table_ref(pCtx,
        synq_span(pCtx, X), SYNQ_NO_SPAN,
        SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
                                         SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
    A = synq_parse_update_stmt(pCtx,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_BOOL_FALSE,
        (SyntaqliteConflictAction)R, tbl,
        SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN,
        Y, F, Z, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
}

// INSERT within trigger
trigger_cmd(A) ::= scanpt insert_cmd(R) INTO trnm(X) idlist_opt(F) select(S) upsert(U) scanpt. {
    uint32_t tbl = synq_parse_table_ref(pCtx,
        synq_span(pCtx, X), SYNQ_NO_SPAN,
        SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
                                         SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
    A = synq_parse_insert_stmt(pCtx,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_BOOL_FALSE,
        (SyntaqliteConflictAction)R, tbl, F, S,
        U.clauses, U.returning);
}

// DELETE within trigger
trigger_cmd(A) ::= DELETE FROM trnm(X) tridxby where_opt(Y) scanpt. {
    uint32_t tbl = synq_parse_table_ref(pCtx,
        synq_span(pCtx, X), SYNQ_NO_SPAN,
        SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
                                         SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN);
    A = synq_parse_delete_stmt(pCtx,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_BOOL_FALSE,
        tbl,
        SYNTAQLITE_INDEX_HINT_DEFAULT, SYNQ_NO_SPAN,
        Y, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
}

// SELECT within trigger
trigger_cmd(A) ::= scanpt select(X) scanpt. {
    A = synq_pass(pCtx, X);
}
