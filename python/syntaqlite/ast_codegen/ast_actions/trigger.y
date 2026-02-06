// AST building actions for CREATE TRIGGER grammar rules.
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

// ============ CREATE TRIGGER ============

// The main cmd rule: completes the trigger with its body
cmd(A) ::= createkw trigger_decl(D) BEGIN trigger_cmd_list(S) END. {
    // D is a partially-built CreateTriggerStmt, fill in the body
    SyntaqliteNode *trig = AST_NODE(pCtx->astCtx->ast, D);
    trig->create_trigger_stmt.body = S;
    A = D;
}

// trigger_decl builds a partial CreateTriggerStmt (without body)
trigger_decl(A) ::= temp(T) TRIGGER ifnotexists(NOERR) nm(B) dbnm(Z)
                    trigger_time(C) trigger_event(D)
                    ON fullname(E) foreach_clause when_clause(G). {
    SyntaqliteSourceSpan trig_name = Z.z ? syntaqlite_span(pCtx, Z) : syntaqlite_span(pCtx, B);
    SyntaqliteSourceSpan trig_schema = Z.z ? syntaqlite_span(pCtx, B) : SYNTAQLITE_NO_SPAN;
    A = ast_create_trigger_stmt(pCtx->astCtx,
        trig_name,
        trig_schema,
        (uint8_t)T,
        (uint8_t)NOERR,
        (SyntaqliteTriggerTiming)C,
        D,
        E,
        G,
        SYNTAQLITE_NULL_NODE);  // body filled in by cmd rule
}

// ============ Trigger timing ============

trigger_time(A) ::= BEFORE|AFTER(X). {
    A = (X.type == TK_BEFORE) ? (int)SYNTAQLITE_TRIGGER_TIMING_BEFORE
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
    SyntaqliteTriggerEventType evt = (X.type == TK_DELETE)
        ? SYNTAQLITE_TRIGGER_EVENT_TYPE_DELETE
        : SYNTAQLITE_TRIGGER_EVENT_TYPE_INSERT;
    A = ast_trigger_event(pCtx->astCtx, evt, SYNTAQLITE_NULL_NODE);
}

trigger_event(A) ::= UPDATE. {
    A = ast_trigger_event(pCtx->astCtx,
        SYNTAQLITE_TRIGGER_EVENT_TYPE_UPDATE, SYNTAQLITE_NULL_NODE);
}

trigger_event(A) ::= UPDATE OF idlist(X). {
    A = ast_trigger_event(pCtx->astCtx,
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
    A = X;
}

// ============ Trigger command list ============

trigger_cmd_list(A) ::= trigger_cmd_list(L) trigger_cmd(X) SEMI. {
    A = ast_trigger_cmd_list_append(pCtx->astCtx, L, X);
}

trigger_cmd_list(A) ::= trigger_cmd(X) SEMI. {
    A = ast_trigger_cmd_list(pCtx->astCtx, X);
}

// ============ trnm (table name in trigger context) ============

trnm(A) ::= nm(A). {
    // Token passthrough
}

trnm(A) ::= nm DOT nm(X). {
    A = X;
    // Qualified names not allowed in triggers, but grammar accepts them
}

// ============ tridxby (index hints in triggers - ignored) ============

tridxby ::= . {
    // empty
}

tridxby ::= INDEXED BY nm. {
    // Not allowed in triggers, but grammar accepts
}

tridxby ::= NOT INDEXED. {
    // Not allowed in triggers, but grammar accepts
}

// ============ Trigger commands ============

// UPDATE within trigger
trigger_cmd(A) ::= UPDATE orconf(R) trnm(X) tridxby SET setlist(Y) from(F) where_opt(Z) scanpt. {
    uint32_t tbl = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, X), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
    A = ast_update_stmt(pCtx->astCtx, (SyntaqliteConflictAction)R, tbl, Y, F, Z);
}

// INSERT within trigger
trigger_cmd(A) ::= scanpt insert_cmd(R) INTO trnm(X) idlist_opt(F) select(S) upsert scanpt. {
    uint32_t tbl = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, X), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
    A = ast_insert_stmt(pCtx->astCtx, (SyntaqliteConflictAction)R, tbl, F, S);
}

// DELETE within trigger
trigger_cmd(A) ::= DELETE FROM trnm(X) tridxby where_opt(Y) scanpt. {
    uint32_t tbl = ast_table_ref(pCtx->astCtx,
        syntaqlite_span(pCtx, X), SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN);
    A = ast_delete_stmt(pCtx->astCtx, tbl, Y);
}

// SELECT within trigger
trigger_cmd(A) ::= scanpt select(X) scanpt. {
    A = X;
}
