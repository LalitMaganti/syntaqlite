// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// AST building actions for CREATE TABLE grammar rules.
// These rules get merged with SQLite's parse.y during code generation.
//
// Rule signatures MUST match upstream parse.y exactly (after lemon -g expansion).
//
// Conventions:
// - pCtx: Parse context (SynqParseContext*)
// - pCtx->zSql: Original SQL text (for computing offsets)
// - Terminals are SynqParseToken with .z (pointer) and .n (length)
// - Non-terminals are u32 node IDs (default) or int/%type-declared types

%type scantok {SynqParseToken}
%type autoinc {int}
%type refargs {SynqRefArgsValue}
%type refarg {SynqRefArgValue}
%type refact {int}
%type defer_subclause {SynqDeferValue}
%type init_deferred_pred_opt {int}
%type defer_subclause_opt {SynqDeferValue}
%type table_option_set {int}
%type table_option {int}
%type tconscomma {int}
%type onconf {int}
%type ccons {SynqConstraintValue}
%type carglist {SynqConstraintListValue}
%type tcons {SynqConstraintValue}
%type conslist {SynqConstraintListValue}
%type generated {SynqConstraintValue}

// ============ CREATE TABLE top-level ============

// create_table produces a partially-built CreateTableStmt node (no columns/constraints yet).
// create_table_args fills in the rest. The cmd rule combines them.

cmd(A) ::= create_table(CT) create_table_args(ARGS). {
    // ARGS is either: (1) a CreateTableStmt node with columns/constraints filled in
    // or: (2) a CreateTableStmt node with as_select filled in
    // CT has the table name/schema/temp/ifnotexists info packed as a node.
    // We need to merge CT info into ARGS.
    SyntaqliteNode *ct_node = AST_NODE(&pCtx->ast, CT);
    SyntaqliteNode *args_node = AST_NODE(&pCtx->ast, ARGS);
    args_node->create_table_stmt.table_name = ct_node->create_table_stmt.table_name;
    args_node->create_table_stmt.schema = ct_node->create_table_stmt.schema;
    args_node->create_table_stmt.is_temp = ct_node->create_table_stmt.is_temp;
    args_node->create_table_stmt.if_not_exists = ct_node->create_table_stmt.if_not_exists;
    A = synq_pass(pCtx, ARGS);
}

create_table(A) ::= createkw temp(T) TABLE ifnotexists(E) nm(Y) dbnm(Z). {
    SyntaqliteTextSpan tbl_name = Z.z ? synq_span_dequote(pCtx, Z) : synq_span_dequote(pCtx, Y);
    SyntaqliteTextSpan tbl_schema = Z.z ? synq_span_dequote(pCtx, Y) : SYNQ_NO_SPAN;
    A = synq_parse_create_table_stmt(pCtx,
        tbl_name, tbl_schema, (SyntaqliteBool)T, (SyntaqliteBool)E,
        (SyntaqliteCreateTableStmtFlags){.raw = 0}, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
}

// ============ CREATE TABLE args ============

create_table_args(A) ::= LP columnlist(CL) conslist_opt(CO) RP table_option_set(F). {
    A = synq_parse_create_table_stmt(pCtx,
        SYNQ_NO_SPAN, SYNQ_NO_SPAN, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE,
        (SyntaqliteCreateTableStmtFlags){.raw = (uint8_t)(F & 0xFF)}, CL, CO, SYNTAQLITE_NULL_NODE);
}

create_table_args(A) ::= AS select(S). {
    A = synq_parse_create_table_stmt(pCtx,
        SYNQ_NO_SPAN, SYNQ_NO_SPAN, SYNTAQLITE_BOOL_FALSE, SYNTAQLITE_BOOL_FALSE,
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
    if (X.n == 5 && SYNQ_STRNCASECMP(X.z, "rowid", 5) == 0) {
        A = 1;
    } else {
        A = 0;
        pCtx->error = 1;
    }
}

table_option(A) ::= nm(X). {
    // STRICT = bit 1
    if (X.n == 6 && SYNQ_STRNCASECMP(X.z, "strict", 6) == 0) {
        A = 2;
    } else {
        A = 0;
        pCtx->error = 1;
    }
}

// ============ Column list ============

columnlist(A) ::= columnlist(L) COMMA columnname(CN) carglist(CG). {
    uint32_t col = synq_parse_column_def(pCtx, CN.name, CN.typetoken, CG.list);
    A = synq_parse_column_def_list(pCtx, L, col);
}

columnlist(A) ::= columnname(CN) carglist(CG). {
    uint32_t col = synq_parse_column_def(pCtx, CN.name, CN.typetoken, CG.list);
    A = synq_parse_column_def_list(pCtx, SYNTAQLITE_NULL_NODE, col);
}

// columnname rule is in schema_ops.y (shared with ALTER TABLE ADD COLUMN)
// It returns SynqColumnNameValue with name + typetoken spans

// ============ Column constraint list (carglist) ============

carglist(A) ::= carglist(L) ccons(C). {
    if (C.node != SYNTAQLITE_NULL_NODE) {
        // Apply pending constraint name from the list to this node
        SyntaqliteNode *node = AST_NODE(&pCtx->ast, C.node);
        node->column_constraint.constraint_name = L.pending_name;
        if (L.list == SYNTAQLITE_NULL_NODE) {
            A.list = synq_parse_column_constraint_list(pCtx, SYNTAQLITE_NULL_NODE, C.node);
        } else {
            A.list = synq_parse_column_constraint_list(pCtx, L.list, C.node);
        }
        A.pending_name = SYNQ_NO_SPAN;
        A.last_node = C.node;
    } else if (C.pending_name.length > 0) {
        // CONSTRAINT nm — store pending name for next constraint
        A.list = L.list;
        A.pending_name = C.pending_name;
        A.last_node = L.last_node;
    } else {
        A = L;
    }
}

carglist(A) ::= . {
    A.list = SYNTAQLITE_NULL_NODE;
    A.pending_name = SYNQ_NO_SPAN;
    A.last_node = SYNTAQLITE_NULL_NODE;
}

// ============ Column constraints (ccons) ============

// CONSTRAINT name - returns pending name for next constraint
ccons(A) ::= CONSTRAINT nm(X). {
    A.node = SYNTAQLITE_NULL_NODE;
    A.pending_name = synq_span(pCtx, X);
}

// DEFAULT scantok term
ccons(A) ::= DEFAULT scantok term(X). {
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

// DEFAULT LP expr RP
ccons(A) ::= DEFAULT LP expr(X) RP. {
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

// DEFAULT PLUS scantok term
ccons(A) ::= DEFAULT PLUS scantok term(X). {
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

// DEFAULT MINUS scantok term
ccons(A) ::= DEFAULT MINUS scantok term(X). {
    // Create a unary minus wrapping the term
    uint32_t neg = synq_parse_unary_expr(pCtx, SYNTAQLITE_UNARY_OP_MINUS, X);
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        neg, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

// DEFAULT scantok id (TRUE/FALSE/CURRENT_TIMESTAMP/identifier default)
// Upstream SQLite creates TK_STRING here and later converts TRUE/FALSE
// to TK_TRUEFALSE during name resolution. We create a ColumnRef to
// match the expression path (DEFAULT LP expr RP), so that both forms
// produce the same AST.
ccons(A) ::= DEFAULT scantok id(X). {
    uint32_t ref = synq_parse_column_ref(pCtx,
        synq_span(pCtx, X), SYNQ_NO_SPAN, SYNQ_NO_SPAN);
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFAULT,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        ref, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

// NULL onconf
ccons(A) ::= NULL onconf(R). {
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_NULL,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)R, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

// NOT NULL onconf
ccons(A) ::= NOT NULL onconf(R). {
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_NOT_NULL,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)R, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

// PRIMARY KEY sortorder onconf autoinc
ccons(A) ::= PRIMARY KEY sortorder(Z) onconf(R) autoinc(I). {
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_PRIMARY_KEY,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)R, (SyntaqliteSortOrder)Z, (SyntaqliteBool)I,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

// UNIQUE onconf
ccons(A) ::= UNIQUE onconf(R). {
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_UNIQUE,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)R, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

// CHECK LP expr RP
ccons(A) ::= CHECK LP expr(X) RP. {
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_CHECK,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        SYNTAQLITE_NULL_NODE, X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

// REFERENCES nm eidlist_opt refargs
ccons(A) ::= REFERENCES nm(T) eidlist_opt(TA) refargs(R). {
    uint32_t fk = synq_parse_foreign_key_clause(pCtx,
        synq_span(pCtx, T), TA, R.match_name, R.on_delete, R.on_update,
        R.on_insert,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET);
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_REFERENCES,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, fk);
    A.pending_name = SYNQ_NO_SPAN;
}

// SQLite applies this to the table's most recent FK, not to the constraint
// before it, so it is kept as its own node rather than folded into one.
ccons(A) ::= defer_subclause(D). {
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_DEFERRABLE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        D.deferrable, D.initial,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

// COLLATE ids
ccons(A) ::= COLLATE ids(C). {
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_COLLATE,
        SYNQ_NO_SPAN,
        0, 0, 0,
        synq_span(pCtx, C),
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
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
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_GENERATED,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, E, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

generated(A) ::= LP expr(E) RP ID(TYPE). {
    SyntaqliteGeneratedColumnStorage storage = SYNTAQLITE_GENERATED_COLUMN_STORAGE_VIRTUAL;
    if (TYPE.n == 6 && SYNQ_STRNCASECMP(TYPE.z, "stored", 6) == 0) {
        storage = SYNTAQLITE_GENERATED_COLUMN_STORAGE_STORED;
    }
    A.node = synq_parse_column_constraint(pCtx,
        SYNTAQLITE_COLUMN_CONSTRAINT_TYPE_GENERATED,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_SORT_ORDER_ASC, SYNTAQLITE_BOOL_FALSE,
        SYNQ_NO_SPAN,
        storage,
        SYNTAQLITE_DEFERRABLE_UNSET, SYNTAQLITE_INITIAL_DEFER_MODE_UNSET,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, E, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

// ============ AUTOINCREMENT ============

autoinc(A) ::= . {
    A = 0;
}

autoinc(A) ::= AUTOINCR. {
    A = 1;
}

// ============ Foreign key reference args ============
// SQLite ignores MATCH semantically, but the text still has to survive.

refargs(A) ::= . {
    A.on_delete = SYNTAQLITE_FOREIGN_KEY_ACTION_UNSET;
    A.on_update = SYNTAQLITE_FOREIGN_KEY_ACTION_UNSET;
    A.on_insert = SYNTAQLITE_FOREIGN_KEY_ACTION_UNSET;
    A.match_name = SYNQ_NO_SPAN;
}

refargs(A) ::= refargs(A) refarg(Y). {
    switch (Y.kind) {
        case SYNQ_REFARG_DELETE: A.on_delete = Y.action; break;
        case SYNQ_REFARG_UPDATE: A.on_update = Y.action; break;
        case SYNQ_REFARG_INSERT: A.on_insert = Y.action; break;
        case SYNQ_REFARG_MATCH:  A.match_name = Y.match_name; break;
        default: break;
    }
}

refarg(A) ::= MATCH nm(X). {
    A.kind = SYNQ_REFARG_MATCH;
    A.action = SYNTAQLITE_FOREIGN_KEY_ACTION_UNSET;
    A.match_name = synq_span(pCtx, X);
}

refarg(A) ::= ON INSERT refact(X). {
    A.kind = SYNQ_REFARG_INSERT;
    A.action = (SyntaqliteForeignKeyAction)X;
    A.match_name = SYNQ_NO_SPAN;
}

refarg(A) ::= ON DELETE refact(X). {
    A.kind = SYNQ_REFARG_DELETE;
    A.action = (SyntaqliteForeignKeyAction)X;
    A.match_name = SYNQ_NO_SPAN;
}

refarg(A) ::= ON UPDATE refact(X). {
    A.kind = SYNQ_REFARG_UPDATE;
    A.action = (SyntaqliteForeignKeyAction)X;
    A.match_name = SYNQ_NO_SPAN;
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

defer_subclause(A) ::= NOT DEFERRABLE init_deferred_pred_opt(X). {
    A.deferrable = SYNTAQLITE_DEFERRABLE_NOT_DEFERRABLE;
    A.initial = (SyntaqliteInitialDeferMode)X;
}

defer_subclause(A) ::= DEFERRABLE init_deferred_pred_opt(X). {
    A.deferrable = SYNTAQLITE_DEFERRABLE_DEFERRABLE;
    A.initial = (SyntaqliteInitialDeferMode)X;
}

init_deferred_pred_opt(A) ::= . {
    A = (int)SYNTAQLITE_INITIAL_DEFER_MODE_UNSET;
}

init_deferred_pred_opt(A) ::= INITIALLY DEFERRED. {
    A = (int)SYNTAQLITE_INITIAL_DEFER_MODE_DEFERRED;
}

init_deferred_pred_opt(A) ::= INITIALLY IMMEDIATE. {
    A = (int)SYNTAQLITE_INITIAL_DEFER_MODE_IMMEDIATE;
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
    SyntaqliteTextSpan pending = SEP ? SYNQ_NO_SPAN : L.pending_name;
    if (TC.node != SYNTAQLITE_NULL_NODE) {
        SyntaqliteNode *node = AST_NODE(&pCtx->ast, TC.node);
        node->table_constraint.constraint_name = pending;
        if (L.list == SYNTAQLITE_NULL_NODE) {
            A.list = synq_parse_table_constraint_list(pCtx, SYNTAQLITE_NULL_NODE, TC.node);
        } else {
            A.list = synq_parse_table_constraint_list(pCtx, L.list, TC.node);
        }
        A.pending_name = SYNQ_NO_SPAN;
        A.last_node = TC.node;
    } else if (TC.pending_name.length > 0) {
        A.list = L.list;
        A.pending_name = TC.pending_name;
        A.last_node = L.last_node;
    } else {
        A = L;
    }
}

conslist(A) ::= tcons(TC). {
    if (TC.node != SYNTAQLITE_NULL_NODE) {
        A.list = synq_parse_table_constraint_list(pCtx, SYNTAQLITE_NULL_NODE, TC.node);
        A.pending_name = SYNQ_NO_SPAN;
        A.last_node = TC.node;
    } else {
        A.list = SYNTAQLITE_NULL_NODE;
        A.pending_name = TC.pending_name;
        A.last_node = SYNTAQLITE_NULL_NODE;
    }
}

tconscomma(A) ::= COMMA. { A = 1; }
tconscomma(A) ::= . { A = 0; }

// ============ Table constraints (tcons) ============

tcons(A) ::= CONSTRAINT nm(X). {
    A.node = SYNTAQLITE_NULL_NODE;
    A.pending_name = synq_span(pCtx, X);
}

tcons(A) ::= PRIMARY KEY LP sortlist(X) autoinc(I) RP onconf(R). {
    A.node = synq_parse_table_constraint(pCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_TYPE_PRIMARY_KEY,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)R, (SyntaqliteBool)I,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

tcons(A) ::= UNIQUE LP sortlist(X) RP onconf(R). {
    A.node = synq_parse_table_constraint(pCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_TYPE_UNIQUE,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)R, SYNTAQLITE_BOOL_FALSE,
        X, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

tcons(A) ::= CHECK LP expr(E) RP onconf(R). {
    A.node = synq_parse_table_constraint(pCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_TYPE_CHECK,
        SYNQ_NO_SPAN,
        (SyntaqliteConflictAction)R, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE, E, SYNTAQLITE_NULL_NODE);
    A.pending_name = SYNQ_NO_SPAN;
}

tcons(A) ::= FOREIGN KEY LP eidlist(FA) RP REFERENCES nm(T) eidlist_opt(TA) refargs(R) defer_subclause_opt(D). {
    uint32_t fk = synq_parse_foreign_key_clause(pCtx,
        synq_span(pCtx, T), TA, R.match_name, R.on_delete, R.on_update,
        R.on_insert,
        D.deferrable, D.initial);
    A.node = synq_parse_table_constraint(pCtx,
        SYNTAQLITE_TABLE_CONSTRAINT_TYPE_FOREIGN_KEY,
        SYNQ_NO_SPAN,
        SYNTAQLITE_CONFLICT_ACTION_DEFAULT, SYNTAQLITE_BOOL_FALSE,
        SYNTAQLITE_NULL_NODE, FA, SYNTAQLITE_NULL_NODE, fk);
    A.pending_name = SYNQ_NO_SPAN;
}

// ============ Defer subclause opt ============

defer_subclause_opt(A) ::= . {
    A.deferrable = SYNTAQLITE_DEFERRABLE_UNSET;
    A.initial = SYNTAQLITE_INITIAL_DEFER_MODE_UNSET;
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