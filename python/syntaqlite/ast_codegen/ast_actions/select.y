// AST building actions for syntaqlite grammar.
// These rules get merged with SQLite's parse.y during code generation.
//
// Rule signatures MUST match upstream parse.y exactly.
// Python tooling validates coverage and consistency.
//
// Conventions:
// - pCtx: Parse context (SynqParseContext*)
// - pCtx->astCtx: AST context for builder calls
// - pCtx->zSql: Original SQL text (for computing offsets)
// - pCtx->root: Set to root node ID at input rule
// - Terminals are SynqToken with .z (pointer) and .n (length)
// - Non-terminals are u32 node IDs

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
    A = synq_ast_select_stmt(pCtx->astCtx, (SynqSelectStmtFlags){.raw = (uint8_t)B}, C, D, E, F, G, H, I, SYNQ_NULL_NODE);
}

oneselect(A) ::= SELECT distinct(B) selcollist(C) from(D) where_opt(E) groupby_opt(F) having_opt(G) window_clause(R) orderby_opt(H) limit_opt(I). {
    A = synq_ast_select_stmt(pCtx->astCtx, (SynqSelectStmtFlags){.raw = (uint8_t)B}, C, D, E, F, G, H, I, R);
}

// ============ Result columns ============

selcollist(A) ::= sclp(B) scanpt expr(C) scanpt as(D). {
    SynqSourceSpan alias = (D.z) ? synq_span(pCtx, D) : SYNQ_NO_SPAN;
    uint32_t col = synq_ast_result_column(pCtx->astCtx, (SynqResultColumnFlags){0}, alias, C);
    A = synq_ast_result_column_list_append(pCtx->astCtx, B, col);
}

selcollist(A) ::= sclp(B) scanpt STAR. {
    uint32_t col = synq_ast_result_column(pCtx->astCtx, (SynqResultColumnFlags){.star = 1}, SYNQ_NO_SPAN, SYNQ_NULL_NODE);
    A = synq_ast_result_column_list_append(pCtx->astCtx, B, col);
}

sclp(A) ::= selcollist(B) COMMA. {
    A = B;
}

sclp(A) ::= . {
    A = SYNQ_NULL_NODE;
}

// scanpt captures position - not needed for AST
scanpt(A) ::= . {
    A.z = NULL; A.n = 0;
}

// as is optional alias
as(A) ::= AS nm(B). {
    A = B;
}

as(A) ::= ID|STRING(B). {
    A = B;
}

as(A) ::= . {
    A.z = NULL; A.n = 0;
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
    A = SYNQ_NULL_NODE;
}

from(A) ::= FROM seltablist(B). {
    A = B;
}

// ============ WHERE/GROUP BY/HAVING/ORDER BY/LIMIT stubs ============

where_opt(A) ::= . {
    A = SYNQ_NULL_NODE;
}

where_opt(A) ::= WHERE expr(B). {
    A = B;
}

groupby_opt(A) ::= . {
    A = SYNQ_NULL_NODE;
}

groupby_opt(A) ::= GROUP BY nexprlist(B). {
    A = B;
}

having_opt(A) ::= . {
    A = SYNQ_NULL_NODE;
}

having_opt(A) ::= HAVING expr(B). {
    A = B;
}

orderby_opt(A) ::= . {
    A = SYNQ_NULL_NODE;
}

orderby_opt(A) ::= ORDER BY sortlist(B). {
    A = B;
}

limit_opt(A) ::= . {
    A = SYNQ_NULL_NODE;
}

limit_opt(A) ::= LIMIT expr(B). {
    A = synq_ast_limit_clause(pCtx->astCtx, B, SYNQ_NULL_NODE);
}

limit_opt(A) ::= LIMIT expr(B) OFFSET expr(C). {
    A = synq_ast_limit_clause(pCtx->astCtx, B, C);
}

limit_opt(A) ::= LIMIT expr(B) COMMA expr(C). {
    A = synq_ast_limit_clause(pCtx->astCtx, C, B);
}
