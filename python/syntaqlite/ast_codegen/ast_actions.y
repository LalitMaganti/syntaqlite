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

// ============ Entry point ============

input ::= cmdlist(B). {
    pCtx->root = B;
}

// ============ Command list ============

cmdlist(A) ::= cmdlist ecmd(B). {
    A = B;  // Just use the last command for now
}

cmdlist(A) ::= ecmd(B). {
    A = B;
}

// ============ Command wrapper ============

ecmd(A) ::= SEMI. {
    A = SYNTAQLITE_NULL_NODE;
}

ecmd(A) ::= cmdx(B) SEMI. {
    A = B;
}

cmdx(A) ::= cmd(B). {
    A = B;
}

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

oneselect(A) ::= SELECT distinct(B) selcollist(C) from where_opt groupby_opt having_opt orderby_opt limit_opt. {
    A = ast_select_stmt(pCtx->astCtx, (u8)B, C);
}

// ============ Result columns ============

selcollist(A) ::= sclp(B) scanpt expr(C) scanpt as. {
    u32 col = ast_result_column(pCtx->astCtx, 0, 0, 0, C);
    A = ast_result_column_list_append(pCtx->astCtx, B, col);
}

selcollist(A) ::= sclp(B) scanpt STAR. {
    u32 col = ast_result_column(pCtx->astCtx, 1, 0, 0, SYNTAQLITE_NULL_NODE);
    A = ast_result_column_list_append(pCtx->astCtx, B, col);
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
as(A) ::= AS nm(B). {
    A = B;
}

as(A) ::= ids(B). {
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

having_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

orderby_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

limit_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

// ============ Expressions ============

expr(A) ::= term(B). {
    A = B;
}

expr(A) ::= LP expr(B) RP. {
    A = B;
}

// ============ Literals ============

term(A) ::= NULL(B). {
    A = ast_literal(pCtx->astCtx, 4, (u32)(B.z - pCtx->zSql), (u16)B.n);
}

term(A) ::= INTEGER(B). {
    A = ast_literal(pCtx->astCtx, 0, (u32)(B.z - pCtx->zSql), (u16)B.n);
}

term(A) ::= FLOAT(B). {
    A = ast_literal(pCtx->astCtx, 1, (u32)(B.z - pCtx->zSql), (u16)B.n);
}

term(A) ::= STRING(B). {
    A = ast_literal(pCtx->astCtx, 2, (u32)(B.z - pCtx->zSql), (u16)B.n);
}

term(A) ::= BLOB(B). {
    A = ast_literal(pCtx->astCtx, 3, (u32)(B.z - pCtx->zSql), (u16)B.n);
}

// ============ Identifiers ============

nm(A) ::= ID(B). {
    A = B;
}

nm(A) ::= STRING(B). {
    A = B;
}

nm(A) ::= JOIN_KW(B). {
    A = B;
}

ids(A) ::= ID(B). {
    A = B;
}

ids(A) ::= STRING(B). {
    A = B;
}
