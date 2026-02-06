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
    SyntaqliteNode *ct_node = AST_NODE(pCtx->astCtx->ast, CT);
    SyntaqliteNode *args_node = AST_NODE(pCtx->astCtx->ast, ARGS);
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
        tbl_name, tbl_schema, (uint8_t)T, (uint8_t)E,
        0, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
}

// ============ CREATE TABLE args ============

create_table_args(A) ::= LP columnlist(CL) conslist_opt(CO) RP table_option_set(F). {
    // CO is a SyntaqliteToken - if CO.z is non-null, there are table constraints
    // CL is a ColumnDefList, pCtx has accumulated table constraints
    (void)CO;
    A = ast_create_table_stmt(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN, 0, 0,
        (uint8_t)F, CL, pCtx->astCtx->tcons_list, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->tcons_list = SYNTAQLITE_NULL_NODE;
}

create_table_args(A) ::= AS select(S). {
    A = ast_create_table_stmt(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN, SYNTAQLITE_NO_SPAN, 0, 0,
        0, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, S);
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
    uint32_t col = ast_column_def(pCtx->astCtx,
        syntaqlite_span(pCtx, CN), pCtx->astCtx->typetoken_span,
        CG);
    A = ast_column_def_list_append(pCtx->astCtx, L, col);
    pCtx->astCtx->typetoken_span = SYNTAQLITE_NO_SPAN;
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

columnlist(A) ::= columnname(CN) carglist(CG). {
    uint32_t col = ast_column_def(pCtx->astCtx,
        syntaqlite_span(pCtx, CN), pCtx->astCtx->typetoken_span,
        CG);
    A = ast_column_def_list(pCtx->astCtx, col);
    pCtx->astCtx->typetoken_span = SYNTAQLITE_NO_SPAN;
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

// columnname rule is in schema_ops.y (shared with ALTER TABLE ADD COLUMN)
// It saves typetoken into pCtx->astCtx->typetoken_span

// ============ Column constraint list (carglist) ============

carglist(A) ::= carglist(L) ccons(C). {
    if (C != SYNTAQLITE_NULL_NODE) {
        if (L == SYNTAQLITE_NULL_NODE) {
            A = ast_column_constraint_list(pCtx->astCtx, C);
        } else {
            A = ast_column_constraint_list_append(pCtx->astCtx, L, C);
        }
    } else {
        A = L;
    }
}

carglist(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

// ============ Column constraints (ccons) ============

// CONSTRAINT name - sets a name for the next constraint
ccons(A) ::= CONSTRAINT nm(X). {
    pCtx->astCtx->constraint_name = syntaqlite_span(pCtx, X);
    A = SYNTAQLITE_NULL_NODE;
}

// DEFAULT scantok term
ccons(A) ::= DEFAULT scantok term(X). {
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        pCtx->astCtx->constraint_name,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

// DEFAULT LP expr RP
ccons(A) ::= DEFAULT LP expr(X) RP. {
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        pCtx->astCtx->constraint_name,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

// DEFAULT PLUS scantok term
ccons(A) ::= DEFAULT PLUS scantok term(X). {
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        pCtx->astCtx->constraint_name,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

// DEFAULT MINUS scantok term
ccons(A) ::= DEFAULT MINUS scantok term(X). {
    // Create a unary minus wrapping the term
    uint32_t neg = ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_MINUS, X);
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        pCtx->astCtx->constraint_name,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        neg, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

// DEFAULT scantok id (TRUE/FALSE/identifier default)
ccons(A) ::= DEFAULT scantok ID|INDEXED(X). {
    // Treat the identifier as a literal expression
    uint32_t lit = ast_literal(pCtx->astCtx,
        SYNTAQLITE_LITERAL_TYPE_STRING, syntaqlite_span(pCtx, X));
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_DEFAULT,
        pCtx->astCtx->constraint_name,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        lit, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

// NULL onconf
ccons(A) ::= NULL onconf(R). {
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NULL,
        pCtx->astCtx->constraint_name,
        (uint8_t)R, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

// NOT NULL onconf
ccons(A) ::= NOT NULL onconf(R). {
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_NOT_NULL,
        pCtx->astCtx->constraint_name,
        (uint8_t)R, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

// PRIMARY KEY sortorder onconf autoinc
ccons(A) ::= PRIMARY KEY sortorder(Z) onconf(R) autoinc(I). {
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_PRIMARY_KEY,
        pCtx->astCtx->constraint_name,
        (uint8_t)R, (uint8_t)Z, (uint8_t)I,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

// UNIQUE onconf
ccons(A) ::= UNIQUE onconf(R). {
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_UNIQUE,
        pCtx->astCtx->constraint_name,
        (uint8_t)R, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

// CHECK LP expr RP
ccons(A) ::= CHECK LP expr(X) RP. {
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_CHECK,
        pCtx->astCtx->constraint_name,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

// REFERENCES nm eidlist_opt refargs
ccons(A) ::= REFERENCES nm(T) eidlist_opt(TA) refargs(R). {
    // Decode refargs: low byte = on_delete, next byte = on_update
    SyntaqliteForeignKeyAction on_del = (SyntaqliteForeignKeyAction)(R & 0xff);
    SyntaqliteForeignKeyAction on_upd = (SyntaqliteForeignKeyAction)((R >> 8) & 0xff);
    uint32_t fk = ast_foreign_key_clause(pCtx->astCtx,
        syntaqlite_span(pCtx, T), TA, on_del, on_upd, 0);
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_REFERENCES,
        pCtx->astCtx->constraint_name,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, fk);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
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
        (uint8_t)D);
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_REFERENCES,
        pCtx->astCtx->constraint_name,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, fk);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

// COLLATE ids
ccons(A) ::= COLLATE ID|STRING(C). {
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_COLLATE,
        pCtx->astCtx->constraint_name,
        0, 0, 0,
        syntaqlite_span(pCtx, C),
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
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
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_GENERATED,
        pCtx->astCtx->constraint_name,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, E, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
}

generated(A) ::= LP expr(E) RP ID(TYPE). {
    SyntaqliteGeneratedColumnStorage storage = SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL;
    if (TYPE.n == 6 && strncasecmp(TYPE.z, "stored", 6) == 0) {
        storage = SYNTAQLITE_GENERATED_COLUMN_STORAGE_STORED;
    }
    A = ast_column_constraint(pCtx->astCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_KIND_GENERATED,
        pCtx->astCtx->constraint_name,
        0, 0, 0,
        SYNTAQLITE_NO_SPAN,
        storage,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, E, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->constraint_name = SYNTAQLITE_NO_SPAN;
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
    A.n = 0; A.z = 0;
    pCtx->astCtx->tcons_list = SYNTAQLITE_NULL_NODE;
}

conslist_opt(A) ::= COMMA(A) conslist. {
    // passthrough token
}

conslist ::= conslist tconscomma tcons(TC). {
    if (TC != SYNTAQLITE_NULL_NODE) {
        if (pCtx->astCtx->tcons_list == SYNTAQLITE_NULL_NODE) {
            pCtx->astCtx->tcons_list = ast_table_constraint_list(pCtx->astCtx, TC);
        } else {
            pCtx->astCtx->tcons_list = ast_table_constraint_list_append(pCtx->astCtx, pCtx->astCtx->tcons_list, TC);
        }
    }
}

conslist ::= tcons(TC). {
    if (TC != SYNTAQLITE_NULL_NODE) {
        pCtx->astCtx->tcons_list = ast_table_constraint_list(pCtx->astCtx, TC);
    }
}

tconscomma ::= COMMA. {
    pCtx->astCtx->tcons_constraint_name = SYNTAQLITE_NO_SPAN;
}

tconscomma ::= . {
    // empty
}

// ============ Table constraints (tcons) ============

tcons(A) ::= CONSTRAINT nm(X). {
    pCtx->astCtx->tcons_constraint_name = syntaqlite_span(pCtx, X);
    A = SYNTAQLITE_NULL_NODE;
}

tcons(A) ::= PRIMARY KEY LP sortlist(X) autoinc(I) RP onconf(R). {
    A = ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_PRIMARY_KEY,
        pCtx->astCtx->tcons_constraint_name,
        (uint8_t)R, (uint8_t)I,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->tcons_constraint_name = SYNTAQLITE_NO_SPAN;
}

tcons(A) ::= UNIQUE LP sortlist(X) RP onconf(R). {
    A = ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_UNIQUE,
        pCtx->astCtx->tcons_constraint_name,
        (uint8_t)R, 0,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->tcons_constraint_name = SYNTAQLITE_NO_SPAN;
}

tcons(A) ::= CHECK LP expr(E) RP onconf(R). {
    A = ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_CHECK,
        pCtx->astCtx->tcons_constraint_name,
        (uint8_t)R, 0,
        SYNTAQLITE_NULL_NODE, E, SYNTAQLITE_NULL_NODE);
    pCtx->astCtx->tcons_constraint_name = SYNTAQLITE_NO_SPAN;
}

tcons(A) ::= FOREIGN KEY LP eidlist(FA) RP REFERENCES nm(T) eidlist_opt(TA) refargs(R) defer_subclause_opt(D). {
    SyntaqliteForeignKeyAction on_del = (SyntaqliteForeignKeyAction)(R & 0xff);
    SyntaqliteForeignKeyAction on_upd = (SyntaqliteForeignKeyAction)((R >> 8) & 0xff);
    uint32_t fk = ast_foreign_key_clause(pCtx->astCtx,
        syntaqlite_span(pCtx, T), TA, on_del, on_upd, (uint8_t)D);
    A = ast_table_constraint(pCtx->astCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_KIND_FOREIGN_KEY,
        pCtx->astCtx->tcons_constraint_name,
        0, 0,
        FA, SYNTAQLITE_NULL_NODE, fk);
    pCtx->astCtx->tcons_constraint_name = SYNTAQLITE_NO_SPAN;
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
