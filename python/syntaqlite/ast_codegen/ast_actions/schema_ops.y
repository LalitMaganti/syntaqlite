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

// ============ Qualified name (fullname) ============

fullname(A) ::= nm(X). {
    A = ast_qualified_name(pCtx->astCtx,
        syntaqlite_span(pCtx, X),
        SYNTAQLITE_NO_SPAN);
}

fullname(A) ::= nm(X) DOT nm(Y). {
    A = ast_qualified_name(pCtx->astCtx,
        syntaqlite_span(pCtx, Y),
        syntaqlite_span(pCtx, X));
}

// ============ IF EXISTS ============

ifexists(A) ::= IF EXISTS. {
    A = 1;
}

ifexists(A) ::= . {
    A = 0;
}

// ============ DROP statements ============

cmd(A) ::= DROP TABLE ifexists(E) fullname(X). {
    A = ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TABLE, (uint8_t)E, X);
}

cmd(A) ::= DROP VIEW ifexists(E) fullname(X). {
    A = ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_VIEW, (uint8_t)E, X);
}

cmd(A) ::= DROP INDEX ifexists(E) fullname(X). {
    A = ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_INDEX, (uint8_t)E, X);
}

cmd(A) ::= DROP TRIGGER ifexists(NOERR) fullname(X). {
    A = ast_drop_stmt(pCtx->astCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TRIGGER, (uint8_t)NOERR, X);
}

// ============ ALTER TABLE ============

cmd(A) ::= ALTER TABLE fullname(X) RENAME TO nm(Z). {
    A = ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_RENAME_TABLE, X,
        syntaqlite_span(pCtx, Z),
        SYNTAQLITE_NO_SPAN);
}

cmd(A) ::= ALTER TABLE fullname(X) RENAME kwcolumn_opt nm(Y) TO nm(Z). {
    A = ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_RENAME_COLUMN, X,
        syntaqlite_span(pCtx, Z),
        syntaqlite_span(pCtx, Y));
}

cmd(A) ::= ALTER TABLE fullname(X) DROP kwcolumn_opt nm(Y). {
    A = ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_DROP_COLUMN, X,
        SYNTAQLITE_NO_SPAN,
        syntaqlite_span(pCtx, Y));
}

cmd(A) ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname(Y) carglist. {
    A = ast_alter_table_stmt(pCtx->astCtx,
        SYNTAQLITE_ALTER_OP_ADD_COLUMN, SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NO_SPAN,
        syntaqlite_span(pCtx, Y));
}

// ============ ALTER TABLE support rules ============

add_column_fullname ::= fullname. {
    // Passthrough - fullname already produces a node ID but we don't need it
    // for the ADD COLUMN action since add_column_fullname is consumed by cmd
}

kwcolumn_opt(A) ::= . {
    A = 0;
}

kwcolumn_opt(A) ::= COLUMNKW. {
    A = 1;
}

columnname(A) ::= nm(A) typetoken(Y). {
    // Token passthrough - nm already produces SyntaqliteToken
    // Also save typetoken for CREATE TABLE column definitions
    pCtx->astCtx->typetoken_span = Y.z ? syntaqlite_span(pCtx, Y) : SYNTAQLITE_NO_SPAN;
}

// ============ Transaction control ============

cmd(A) ::= BEGIN transtype(Y) trans_opt. {
    A = ast_transaction_stmt(pCtx->astCtx,
        SYNTAQLITE_TRANSACTION_OP_BEGIN,
        (SyntaqliteTransactionType)Y);
}

cmd(A) ::= COMMIT|END trans_opt. {
    A = ast_transaction_stmt(pCtx->astCtx,
        SYNTAQLITE_TRANSACTION_OP_COMMIT,
        SYNTAQLITE_TRANSACTION_TYPE_DEFERRED);
}

cmd(A) ::= ROLLBACK trans_opt. {
    A = ast_transaction_stmt(pCtx->astCtx,
        SYNTAQLITE_TRANSACTION_OP_ROLLBACK,
        SYNTAQLITE_TRANSACTION_TYPE_DEFERRED);
}

// ============ Transaction type ============

transtype(A) ::= . {
    A = (int)SYNTAQLITE_TRANSACTION_TYPE_DEFERRED;
}

transtype(A) ::= DEFERRED. {
    A = (int)SYNTAQLITE_TRANSACTION_TYPE_DEFERRED;
}

transtype(A) ::= IMMEDIATE. {
    A = (int)SYNTAQLITE_TRANSACTION_TYPE_IMMEDIATE;
}

transtype(A) ::= EXCLUSIVE. {
    A = (int)SYNTAQLITE_TRANSACTION_TYPE_EXCLUSIVE;
}

// ============ Transaction option ============

trans_opt(A) ::= . {
    A = 0;
}

trans_opt(A) ::= TRANSACTION. {
    A = 0;
}

trans_opt(A) ::= TRANSACTION nm. {
    A = 0;
}

// ============ Savepoint ============

savepoint_opt(A) ::= SAVEPOINT. {
    A = 0;
}

savepoint_opt(A) ::= . {
    A = 0;
}

cmd(A) ::= SAVEPOINT nm(X). {
    A = ast_savepoint_stmt(pCtx->astCtx,
        SYNTAQLITE_SAVEPOINT_OP_SAVEPOINT,
        syntaqlite_span(pCtx, X));
}

cmd(A) ::= RELEASE savepoint_opt nm(X). {
    A = ast_savepoint_stmt(pCtx->astCtx,
        SYNTAQLITE_SAVEPOINT_OP_RELEASE,
        syntaqlite_span(pCtx, X));
}

cmd(A) ::= ROLLBACK trans_opt TO savepoint_opt nm(X). {
    A = ast_savepoint_stmt(pCtx->astCtx,
        SYNTAQLITE_SAVEPOINT_OP_ROLLBACK_TO,
        syntaqlite_span(pCtx, X));
}
