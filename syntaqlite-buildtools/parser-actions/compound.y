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

%type multiselect_op {int}
%type in_op {int}
%type dbnm {SynqParseToken}

// ============ Compound SELECT ============

selectnowith(A) ::= selectnowith(A) multiselect_op(Y) oneselect(Z). {
    // ORDER BY / LIMIT parse inside the last arm (grammar shape) but apply
    // to the whole compound — hoist them onto the CompoundSelect node.
    // An ORDER BY on a non-last arm stays put (SQLite rejects it later).
    uint32_t orderby = SYNTAQLITE_NULL_NODE;
    uint32_t limit = SYNTAQLITE_NULL_NODE;
    SyntaqliteNode *arm = AST_NODE(&pCtx->ast, Z);
    if (arm->tag == SYNTAQLITE_NODE_SELECT_STMT) {
        orderby = arm->select_stmt.orderby;
        limit = arm->select_stmt.limit_clause;
        arm->select_stmt.orderby = SYNTAQLITE_NULL_NODE;
        arm->select_stmt.limit_clause = SYNTAQLITE_NULL_NODE;
    }
    A = synq_parse_compound_select(pCtx, (SyntaqliteCompoundOp)Y, A, Z, orderby, limit);
}

multiselect_op(A) ::= UNION(OP). { A = 0; (void)OP; }
multiselect_op(A) ::= UNION ALL. { A = 1; }
multiselect_op(A) ::= EXCEPT|INTERSECT(OP). {
    A = (OP.type == SYNTAQLITE_TK_INTERSECT) ? 2 : 3;
}

// ============ Subquery Expressions ============

expr(A) ::= LP select(X) RP. {
    pCtx->saw_subquery = 1;
    A = synq_parse_subquery_expr(pCtx, X);
}

expr(A) ::= EXISTS LP select(Y) RP. {
    pCtx->saw_subquery = 1;
    A = synq_parse_exists_expr(pCtx, Y);
}

// ============ IN Expressions ============

in_op(A) ::= IN. { A = 0; }
in_op(A) ::= NOT IN. { A = 1; }

expr(A) ::= expr(A) in_op(N) LP exprlist(Y) RP. [IN] {
    A = synq_parse_in_expr(pCtx, (SyntaqliteBool)N,
                           SYNTAQLITE_BOOL_FALSE, A, Y);
}

expr(A) ::= expr(A) in_op(N) LP select(Y) RP. [IN] {
    pCtx->saw_subquery = 1;
    // Pass the raw select node directly — InExpr's fmt block already adds
    // the surrounding parens, so wrapping in SubqueryExpr would double them.
    A = synq_parse_in_expr(pCtx, (SyntaqliteBool)N,
                           SYNTAQLITE_BOOL_FALSE, A, Y);
}

// `x IN tbl`, `x IN schema.tbl`, `x IN tvf(args)`, `x IN tvf()`, or
// the fully-qualified TVF form. The RHS is a single-column table or
// table-valued function, represented as a TableRef node.
//
// `E.has_parens` distinguishes `tbl` from `tvf()` so the formatter
// can round-trip both forms — SQLite treats them differently. And
// `bare_source=TRUE` on the InExpr suppresses the value-list parens
// `IN (...)` that SQLite's other IN productions imply; without it
// we'd emit `IN (tbl)` which SQLite parses as a one-element value
// list where `tbl` resolves as a column/variable, not a table.
expr(A) ::= expr(A) in_op(N) nm(Y) dbnm(Z) paren_exprlist(E). [IN] {
    SyntaqliteTextSpan table_name;
    SyntaqliteTextSpan schema;
    if (Z.z != NULL) {
        table_name = synq_span_dequote(pCtx, Z);
        schema = synq_span_dequote(pCtx, Y);
    } else {
        table_name = synq_span_dequote(pCtx, Y);
        schema = SYNQ_NO_SPAN;
    }
    uint32_t tref = synq_parse_table_ref(pCtx, table_name, schema,
                                         E.has_parens,
                                         SYNTAQLITE_NULL_NODE, E.args);
    A = synq_parse_in_expr(pCtx, (SyntaqliteBool)N,
                           SYNTAQLITE_BOOL_TRUE, A, tref);
}

// ============ Helper rules ============

dbnm(A) ::= . { A.z = NULL; A.n = 0; }
dbnm(A) ::= DOT nm(X). { A = X; }

%type paren_exprlist {SynqParenExprlistValue}
paren_exprlist(A) ::= . {
    A.args = SYNTAQLITE_NULL_NODE;
    A.has_parens = SYNTAQLITE_BOOL_FALSE;
}
paren_exprlist(A) ::= LP exprlist(X) RP. {
    A.args = synq_pass(pCtx, X);
    A.has_parens = SYNTAQLITE_BOOL_TRUE;
}