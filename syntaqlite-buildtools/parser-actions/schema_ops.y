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

%type columnname {SynqColumnNameValue}
%type ifexists {int}
%type transtype {int}
%type trans_opt {int}
%type savepoint_opt {int}
%type kwcolumn_opt {int}

// ============ Qualified name (fullname) ============

fullname(A) ::= nmorerr(X). {
    A = synq_parse_qualified_name(pCtx,
        X,
        SYNTAQLITE_NULL_NODE);
}

fullname(A) ::= nmorerr(X) DOT nmorerr(Y). {
    A = synq_parse_qualified_name(pCtx,
        Y,
        X);
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
    A = synq_parse_drop_stmt(pCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TABLE, (SyntaqliteBool)E, X);
}

cmd(A) ::= DROP VIEW ifexists(E) fullname(X). {
    A = synq_parse_drop_stmt(pCtx, SYNTAQLITE_DROP_OBJECT_TYPE_VIEW, (SyntaqliteBool)E, X);
}

cmd(A) ::= DROP INDEX ifexists(E) fullname(X). {
    A = synq_parse_drop_stmt(pCtx, SYNTAQLITE_DROP_OBJECT_TYPE_INDEX, (SyntaqliteBool)E, X);
}

cmd(A) ::= DROP TRIGGER ifexists(NOERR) fullname(X). {
    A = synq_parse_drop_stmt(pCtx, SYNTAQLITE_DROP_OBJECT_TYPE_TRIGGER, (SyntaqliteBool)NOERR, X);
}

// ============ ALTER TABLE ============

cmd(A) ::= ALTER TABLE fullname(X) RENAME TO nmorerr(Z). {
    A = synq_parse_alter_table_stmt(pCtx,
        SYNTAQLITE_ALTER_OP_RENAME_TABLE, X,
        Z,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE);
}

cmd(A) ::= ALTER TABLE fullname(X) RENAME kwcolumn_opt nmorerr(Y) TO nmorerr(Z). {
    A = synq_parse_alter_table_stmt(pCtx,
        SYNTAQLITE_ALTER_OP_RENAME_COLUMN, X,
        Z,
        Y,
        SYNTAQLITE_NULL_NODE);
}

cmd(A) ::= ALTER TABLE fullname(X) DROP kwcolumn_opt nmorerr(Y). {
    A = synq_parse_alter_table_stmt(pCtx,
        SYNTAQLITE_ALTER_OP_DROP_COLUMN, X,
        SYNTAQLITE_NULL_NODE,
        Y,
        SYNTAQLITE_NULL_NODE);
}

cmd(A) ::= ALTER TABLE add_column_fullname(F) ADD kwcolumn_opt columnname(Y) carglist(CG). {
    uint32_t col = synq_parse_column_def(pCtx, Y.name, Y.typetoken, CG.list);
    A = synq_parse_alter_table_stmt(pCtx,
        SYNTAQLITE_ALTER_OP_ADD_COLUMN, F,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE,
        col);
}

// ============ ALTER TABLE support rules ============

add_column_fullname(A) ::= fullname(X). {
    A = X;
}

kwcolumn_opt(A) ::= . {
    A = 0;
}

kwcolumn_opt(A) ::= COLUMNKW. {
    A = 1;
}

columnname(A) ::= nmorerr(X) typetoken(Y). {
    A.name = X;
    A.typetoken = Y.z ? synq_span(pCtx, Y) : SYNQ_NO_SPAN;
    // Starting a column ends the previous one's pending CONSTRAINT name.
    pCtx->constraint_name = SYNQ_NO_SPAN;
}

// ============ Transaction control ============

cmd(A) ::= BEGIN transtype(Y) trans_opt. {
    A = synq_parse_transaction_stmt(pCtx,
        SYNTAQLITE_TRANSACTION_OP_BEGIN,
        (SyntaqliteTransactionType)Y);
}

cmd(A) ::= COMMIT|END trans_opt. {
    A = synq_parse_transaction_stmt(pCtx,
        SYNTAQLITE_TRANSACTION_OP_COMMIT,
        SYNTAQLITE_TRANSACTION_TYPE_DEFERRED);
}

cmd(A) ::= ROLLBACK trans_opt. {
    A = synq_parse_transaction_stmt(pCtx,
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

cmd(A) ::= SAVEPOINT nmorerr(X). {
    A = synq_parse_savepoint_stmt(pCtx,
        SYNTAQLITE_SAVEPOINT_OP_SAVEPOINT,
        X);
}

cmd(A) ::= RELEASE savepoint_opt nmorerr(X). {
    A = synq_parse_savepoint_stmt(pCtx,
        SYNTAQLITE_SAVEPOINT_OP_RELEASE,
        X);
}

cmd(A) ::= ROLLBACK trans_opt TO savepoint_opt nmorerr(X). {
    A = synq_parse_savepoint_stmt(pCtx,
        SYNTAQLITE_SAVEPOINT_OP_ROLLBACK_TO,
        X);
}
