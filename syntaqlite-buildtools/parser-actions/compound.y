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
    A = synq_parse_compound_select(pCtx, (SyntaqliteCompoundOp)Y, A, Z);
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
    A = synq_parse_in_expr(pCtx, (SyntaqliteBool)N, A, Y);
}

expr(A) ::= expr(A) in_op(N) LP select(Y) RP. [IN] {
    pCtx->saw_subquery = 1;
    // Pass the raw select node directly — InExpr's fmt block already adds
    // the surrounding parens, so wrapping in SubqueryExpr would double them.
    A = synq_parse_in_expr(pCtx, (SyntaqliteBool)N, A, Y);
}

expr(A) ::= expr(A) in_op(N) nm(Y) dbnm(Z) paren_exprlist(E). [IN] {
    // Table-valued function IN expression - stub for now
    (void)Y; (void)Z; (void)E;
    A = synq_parse_in_expr(pCtx, (SyntaqliteBool)N, A, SYNTAQLITE_NULL_NODE);
}

// ============ Helper rules ============

dbnm(A) ::= . { A.z = NULL; A.n = 0; }
dbnm(A) ::= DOT nm(X). { A = X; }

paren_exprlist(A) ::= . { A = SYNTAQLITE_NULL_NODE; }
paren_exprlist(A) ::= LP exprlist(X) RP. { A = synq_pass(pCtx, X); }