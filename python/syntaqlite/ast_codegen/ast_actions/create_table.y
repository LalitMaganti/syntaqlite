// AST building actions for CREATE TABLE grammar rules.
// These rules get merged with SQLite's parse.y during code generation.
//
// Rule signatures MUST match upstream parse.y exactly (after lemon -g expansion).
//
// Conventions:
// - pCtx: Parse context (SyntaqliteParseContext*)
// - pCtx->astCtx: AST context for builder calls
// - pCtx->zSql: Original SQL text (for computing offsets)
// - Terminals are SyntaqliteToken with .z (pointer) and .n (length)
// - Non-terminals are u32 node IDs (default) or int/%type-declared types

// ============ CREATE TABLE top-level ============

// create_table produces a partially-built CreateTableStmt node (no columns/constraints yet).
// create_table_args fills in the rest. The cmd rule combines them.

cmd(A) ::= create_table(CT) create_table_args(ARGS). {
    // ARGS is either: (1) a CreateTableStmt node with columns/constraints filled in
    // or: (2) a CreateTableStmt node with as_select filled in
    // CT has the table name/schema/temp/ifnotexists info packed as a node.
    // We need to merge CT info into ARGS.
    SyntaqliteNode *ct_node = AST_NODE(&pCtx->astCtx->ast, CT);
    SyntaqliteNode *args_node = AST_NODE(&pCtx->astCtx->ast, ARGS);
    args_node->create_table_stmt.table_name = ct_node->create_table_stmt.table_name;
    args_node->create_table_stmt.schema = ct_node->create_table_stmt.schema;
    args_node->create_table_stmt.is_temp = ct_node->create_table_stmt.is_temp;
    args_node->create_table_stmt.if_not_exists = ct_node->create_table_stmt.if_not_exists;
    A = ARGS;
}

create_table(A) ::= createkw temp(T) TABLE ifnotexists(E) nm(Y) dbnm(Z). {
    SyntaqliteSourceSpan tbl_name = Z.z ? syntaqlite_span(pCtx, Z) : syntaqlite_span(pCtx, Y);
    SyntaqliteSourceSpan tbl_schema = Z.z ? syntaqlite_span(pCtx, Y) : SYNTAQLITE_NO_SPAN;
    A = ast_create_table_stmt(pCtx->astCtx,
        tbl_name, tbl_schema, (SyntaqliteBool)T, (SyntaqliteBool)E,
        (SyntaqliteCreateTableStmtFlags){.raw = 0}, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
}

// ============ CREATE TABLE args ============

create_table_args(A) ::= LP columnlist(CL) conslist_opt(CO) RP table_option_set(F). {
    A = ast_create_table_stmt(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE,
        (SyntaqliteCreateTableStmtFlags){.raw = (uint8_t)F}, CL, CO, SYNTAQLITE_NULL_NODE);
}

create_table_args(A) ::= AS select(S). {
    A = ast_create_table_stmt(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE,
        (SyntaqliteCreateTableStmtFlags){.raw = 0}, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, S);
}

// ============ Table options ============

table_option_set(A) ::= . {
    A = 0;
}

table_option_set(A) ::= table_option(A). {
    // passthrough
}

table_option_set(A) ::= table_option_set(X) COMMA table_option(Y). {
    A = X | Y;
}

table_option(A) ::= WITHOUT nm(X). {
    // WITHOUT ROWID = bit 0
    if (X.n == 5 && strncasecmp(X.z, "rowid", 5) == 0) {
        A = 1;
    } else {
        A = 0;
    }
}

table_option(A) ::= nm(X). {
    // STRICT = bit 1
    if (X.n == 6 && strncasecmp(X.z, "strict", 6) == 0) {
        A = 2;
    } else {
        A = 0;
    }
}

// ============ Column list ============

columnlist(A) ::= columnlist(L) COMMA columnname(CN) carglist(CG). {
    uint32_t col = ast_column_def(pCtx->astCtx, CN.name, CN.typetoken, CG.list);
    A = ast_column_def_list_append(pCtx->astCtx, L, col);
}

columnlist(A) ::= columnname(CN) carglist(CG). {
    uint32_t col = ast_column_def(pCtx->astCtx, CN.name, CN.typetoken, CG.list);
    A = ast_column_def_list(pCtx->astCtx, col);
}

// columnname rule is in schema_ops.y (shared with ALTER TABLE ADD COLUMN)
// It returns SyntaqliteColumnNameValue with name + typetoken spans

// ============ Column constraint list (carglist) ============

carglist(A) ::= carglist(L) ccons(C). {
    if (C.node != SYNTAQLITE_NULL_NODE) {
        // Apply pending constraint name from the list to this node
        SyntaqliteNode *node = AST_NODE(&pCtx->astCtx->ast, C.node);
        node->column_constraint.constraint_name = L.pending_name;
        if (L.list == SYNTAQLITE_NULL_NODE) {
            A.list = ast_column_constraint_list(pCtx->astCtx, C.node);
        } else {
            A.list = ast_column_constraint_list_append(pCtx->astCtx, L.list, C.node);
        }
        A.pending_name = SYNTAQLITE_NO_SPAN;
    } else if (C.pending_name.length > 0) {
        // CONSTRAINT nm — store pending name for next constraint
        A.list = L.list;
        A.pending_name = C.pending_name;
    } else {
        A = L;
    }
}

carglist(A) ::= . {
    A.list = SYNTAQLITE_NULL_NODE;
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// ============ Column constraints (ccons) ============

// CONSTRAINT name - returns pending name for next constraint
ccons(A) ::= CONSTRAINT nm(X). {
    A.node = SYNTAQLITE_NULL_NODE;
    A.pending_name = syntaqlite_span(pCtx, X);
}

// DEFAULT scantok term
ccons(A) ::= DEFAULT scantok term(X). {
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// DEFAULT LP expr RP
ccons(A) ::= DEFAULT LP expr(X) RP. {
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// DEFAULT PLUS scantok term
ccons(A) ::= DEFAULT PLUS scantok term(X). {
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// DEFAULT MINUS scantok term
ccons(A) ::= DEFAULT MINUS scantok term(X). {
    // Create a unary minus wrapping the term
    uint32_t neg = ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_MINUS, X);
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        neg, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// DEFAULT scantok id (TRUE/FALSE/identifier default)
ccons(A) ::= DEFAULT scantok ID|INDEXED(X). {
    // Treat the identifier as a literal expression
    uint32_t lit = ast_literal(pCtx->astCtx,
        SYNTAQLITE_LITERAL_TYPE_STRING, syntaqlite_span(pCtx, X));
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        lit, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// NULL onconf
ccons(A) ::= NULL onconf(R). {
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NULL,
        SYNTAQLITE_NO_SPAN,
        (SyntaqliteConflictAction)R, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// NOT NULL onconf
ccons(A) ::= NOT NULL onconf(R). {
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NOT_NULL,
        SYNTAQLITE_NO_SPAN,
        (SyntaqliteConflictAction)R, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// PRIMARY KEY sortorder onconf autoinc
ccons(A) ::= PRIMARY KEY sortorder(Z) onconf(R) autoinc(I). {
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_PRIMARY_KEY,
        SYNTAQLITE_NO_SPAN,
        (SyntaqliteConflictAction)R, (SyntaqliteSortOrder)Z, (SyntaqliteBool)I,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// UNIQUE onconf
ccons(A) ::= UNIQUE onconf(R). {
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_UNIQUE,
        SYNTAQLITE_NO_SPAN,
        (SyntaqliteConflictAction)R, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// CHECK LP expr RP
ccons(A) ::= CHECK LP expr(X) RP. {
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_CHECK,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// REFERENCES nm eidlist_opt refargs
ccons(A) ::= REFERENCES nm(T) eidlist_opt(TA) refargs(R). {
    // Decode refargs: low byte = on_delete, next byte = on_update
    SyntaqliteForeignKeyAction on_del = (SyntaqliteForeignKeyAction)(R & 0xff);
    SyntaqliteForeignKeyAction on_upd = (SyntaqliteForeignKeyAction)((R >> 8) & 0xff);
    uint32_t fk = ast_foreign_key_clause(pCtx->astCtx,
        syntaqlite_span(pCtx, T), TA, on_del, on_upd, SYNTAQLITE_BOOL_FALSE);
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_REFERENCES,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, fk);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// defer_subclause (applied to preceding REFERENCES constraint)
ccons(A) ::= defer_subclause(D). {
    // Create a minimal constraint that just marks deferral.
    // In practice, this follows a REFERENCES ccons. We'll handle it
    // by updating the last constraint in the list if possible.
    // For simplicity, we create a separate REFERENCES constraint with just deferral info.
    // The printer will show it as a separate constraint entry.
    uint32_t fk = ast_foreign_key_clause(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION,
        SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION,
        (SyntaqliteBool)D);
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_REFERENCES,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, fk);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// COLLATE ids
ccons(A) ::= COLLATE ID|STRING(C). {
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_COLLATE,
        SYNTAQLITE_NO_SPAN,
        0, 0, 0,
        syntaqlite_span(pCtx, C),
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// GENERATED ALWAYS AS generated
ccons(A) ::= GENERATED ALWAYS AS generated(G). {
    A = G;
}

// AS generated
ccons(A) ::= AS generated(G). {
    A = G;
}


// ============ Generated column ============

generated(A) ::= LP expr(E) RP. {
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_GENERATED,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, E, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

generated(A) ::= LP expr(E) RP ID(TYPE). {
    SyntaqliteGeneratedColumnStorage storage = SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL;
    if (TYPE.n == 6 && strncasecmp(TYPE.z, "stored", 6) == 0) {
        storage = SYNTAQLITE_GENERATED_COLUMN_STORAGE_STORED;
    }
    A.node = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_GENERATED,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NO_SPAN,
        storage,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, E, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// ============ AUTOINCREMENT ============

autoinc(A) ::= . {
    A = 0;
}

autoinc(A) ::= AUTOINCR. {
    A = 1;
}

// ============ Foreign key reference args ============
// We pack on_delete in low byte, on_update in byte 1.
// ForeignKeyAction enum: NO_ACTION=0, SET_NULL=1, SET_DEFAULT=2, CASCADE=3, RESTRICT=4

refargs(A) ::= . {
    A = 0; // NO_ACTION for both
}

refargs(A) ::= refargs(A) refarg(Y). {
    // refarg encodes: low byte = value, byte 1 = shift amount (0 or 8)
    int val = Y & 0xff;
    int shift = (Y >> 8) & 0xff;
    // Clear the target byte in A and set new value
    A = (A & ~(0xff << shift)) | (val << shift);
}

// refarg encodes value + shift: low byte = action enum value, byte 1 = bit shift (0=DELETE, 8=UPDATE)
refarg(A) ::= MATCH nm. {
    A = 0; // MATCH is ignored
}

refarg(A) ::= ON INSERT refact. {
    A = 0; // ON INSERT is ignored
}

refarg(A) ::= ON DELETE refact(X). {
    A = X; // shift=0 for DELETE
}

refarg(A) ::= ON UPDATE refact(X). {
    A = X | (8 << 8); // shift=8 for UPDATE
}

// refact returns ForeignKeyAction enum values
refact(A) ::= SET NULL. {
    A = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_SET_NULL;
}

refact(A) ::= SET DEFAULT. {
    A = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_SET_DEFAULT;
}

refact(A) ::= CASCADE. {
    A = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_CASCADE;
}

refact(A) ::= RESTRICT. {
    A = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_RESTRICT;
}

refact(A) ::= NO ACTION. {
    A = (int)SYNTAQLITE_FOREIGN_KEY_ACTION_NO_ACTION;
}

// ============ Defer subclause ============

defer_subclause(A) ::= NOT DEFERRABLE init_deferred_pred_opt. {
    A = 0;
}

defer_subclause(A) ::= DEFERRABLE init_deferred_pred_opt(X). {
    A = X;
}

init_deferred_pred_opt(A) ::= . {
    A = 0;
}

init_deferred_pred_opt(A) ::= INITIALLY DEFERRED. {
    A = 1;
}

init_deferred_pred_opt(A) ::= INITIALLY IMMEDIATE. {
    A = 0;
}

// ============ Table constraint list support ============

conslist_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

conslist_opt(A) ::= COMMA conslist(L). {
    A = L.list;
}

conslist(A) ::= conslist(L) tconscomma(SEP) tcons(TC). {
    // If comma separator was present, clear pending constraint name
    SyntaqliteSourceSpan pending = SEP ? SYNTAQLITE_NO_SPAN : L.pending_name;
    if (TC.node != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *node = AST_NODE(&pCtx->astCtx->ast, TC.node);
        node->table_constraint.constraint_name = pending;
        if (L.list == SYNTAQLITE_NULL_NODE) {
            A.list = ast_table_constraint_list(pCtx->astCtx, TC.node);
        } else {
            A.list = ast_table_constraint_list_append(pCtx->astCtx, L.list, TC.node);
        }
        A.pending_name = SYNTAQLITE_NO_SPAN;
    } else if (TC.pending_name.length > 0) {
        A.list = L.list;
        A.pending_name = TC.pending_name;
    } else {
        A = L;
    }
}

conslist(A) ::= tcons(TC). {
    if (TC.node != SYNTAQLITE_NULL_NODE) {
        A.list = ast_table_constraint_list(pCtx->astCtx, TC.node);
        A.pending_name = SYNTAQLITE_NO_SPAN;
    } else {
        A.list = SYNTAQLITE_NULL_NODE;
        A.pending_name = TC.pending_name;
    }
}

tconscomma(A) ::= COMMA. { A = 1; }
tconscomma(A) ::= . { A = 0; }

// ============ Table constraints (tcons) ============

tcons(A) ::= CONSTRAINT nm(X). {
    A.node = SYNTAQLITE_NULL_NODE;
    A.pending_name = syntaqlite_span(pCtx, X);
}

tcons(A) ::= PRIMARY KEY LP sortlist(X) autoinc(I) RP onconf(R). {
    A.node = ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_PRIMARY_KEY,
        SYNTAQLITE_NO_SPAN,
        (SyntaqliteConflictAction)R, (SyntaqliteBool)I,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

tcons(A) ::= UNIQUE LP sortlist(X) RP onconf(R). {
    A.node = ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_UNIQUE,
        SYNTAQLITE_NO_SPAN,
        (SyntaqliteConflictAction)R, SYNTAQLITE_BOOL_FALSE,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

tcons(A) ::= CHECK LP expr(E) RP onconf(R). {
    A.node = ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_CHECK,
        SYNTAQLITE_NO_SPAN,
        (SyntaqliteConflictAction)R, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NULL_NODE, E, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

tcons(A) ::= FOREIGN KEY LP eidlist(FA) RP REFERENCES nm(T) eidlist_opt(TA) refargs(R) defer_subclause_opt(D). {
    SyntaqliteForeignKeyAction on_del = (SyntaqliteForeignKeyAction)(R & 0xff);
    SyntaqliteForeignKeyAction on_upd = (SyntaqliteForeignKeyAction)((R >> 8) & 0xff);
    uint32_t fk = ast_foreign_key_clause(pCtx->astCtx,
        syntaqlite_span(pCtx, T), TA, on_del, on_upd, (SyntaqliteBool)D);
    A.node = ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_FOREIGN_KEY,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_BOOL_FALSE,
        FA, SYNTAQLITE_NULL_NODE, fk);
    A.pending_name = SYNTAQLITE_NO_SPAN;
}

// ============ Defer subclause opt ============

defer_subclause_opt(A) ::= . {
    A = 0;
}

defer_subclause_opt(A) ::= defer_subclause(A). {
    // passthrough
}

// ============ ON CONFLICT (constraint conflict resolution) ============

onconf(A) ::= . {
    A = (int)SYNTAQLITE_CONFLICT_ACTION_DEFAULT;
}

onconf(A) ::= ON CONFLICT resolvetype(X). {
    A = X;
}

// ============ scantok (empty rule, produces lookahead token) ============

scantok(A) ::= . {
    A.z = NULL; A.n = 0;
}
