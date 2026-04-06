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

%type scanpt {SynqParseToken}
%type as {uint32_t}

// ============ SELECT ============

cmd(A) ::= select(B). {
    A = B;
}

select(A) ::= selectnowith(B). {
    A = B;
}

selectnowith(A) ::= oneselect(B). {
    A = B;
}

oneselect(A) ::= SELECT distinct(B) selcollist(C) from(D) where_opt(E) groupby_opt(F) having_opt(G) orderby_opt(H) limit_opt(I). {
    A = synq_parse_select_stmt(pCtx, (SyntaqliteSelectStmtFlags){.raw = (uint8_t)B}, C, D, E, F, G, H, I, SYNTAQLITE_NULL_NODE);
}

oneselect(A) ::= SELECT distinct(B) selcollist(C) from(D) where_opt(E) groupby_opt(F) having_opt(G) window_clause(R) orderby_opt(H) limit_opt(I). {
    A = synq_parse_select_stmt(pCtx, (SyntaqliteSelectStmtFlags){.raw = (uint8_t)B}, C, D, E, F, G, H, I, R);
}

// ============ Result columns ============

selcollist(A) ::= sclp(B) scanpt expr(C) scanpt as(D). {
    uint32_t col = synq_parse_result_column(pCtx, (SyntaqliteResultColumnFlags){0}, D, C);
    A = synq_parse_result_column_list(pCtx, B, col);
}

selcollist(A) ::= sclp(B) scanpt STAR. {
    uint32_t col = synq_parse_result_column(pCtx, (SyntaqliteResultColumnFlags){.bits = {.star = 1}}, SYNTAQLITE_NULL_NODE, SYNTAQLITE_NULL_NODE);
    A = synq_parse_result_column_list(pCtx, B, col);
}

sclp(A) ::= selcollist(B) COMMA. {
    A = B;
}

sclp(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

// scanpt captures position - not needed for AST
scanpt(A) ::= . {
    A.z = NULL; A.n = 0;
}

// as is optional alias
as(A) ::= AS nmorerr(B). {
    A = B;
}

as(A) ::= ids(B). {
    A = synq_parse_ident_name(pCtx, synq_span_dequote(pCtx, B));
}

as(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

// ============ DISTINCT / ALL ============

distinct(A) ::= DISTINCT. {
    A = 1;
}

distinct(A) ::= ALL. {
    A = 0;
}

distinct(A) ::= . {
    A = 0;
}

// ============ FROM clause ============

from(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

from(A) ::= FROM seltablist(B). {
    A = B;
}

// ============ WHERE/GROUP BY/HAVING/ORDER BY/LIMIT stubs ============

where_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

where_opt(A) ::= WHERE expr(B). {
    A = B;
}

groupby_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

groupby_opt(A) ::= GROUP BY nexprlist(B). {
    A = B;
}

having_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

having_opt(A) ::= HAVING expr(B). {
    A = B;
}

orderby_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

orderby_opt(A) ::= ORDER BY sortlist(B). {
    A = B;
}

limit_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

limit_opt(A) ::= LIMIT expr(B). {
    A = synq_parse_limit_clause(pCtx, B, SYNTAQLITE_NULL_NODE);
}

limit_opt(A) ::= LIMIT expr(B) OFFSET expr(C). {
    A = synq_parse_limit_clause(pCtx, B, C);
}

limit_opt(A) ::= LIMIT expr(B) COMMA expr(C). {
    A = synq_parse_limit_clause(pCtx, C, B);
}
