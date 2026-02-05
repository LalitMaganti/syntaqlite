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
    A = ast_select_stmt(pCtx->astCtx, (uint8_t)B, C);
}

// ============ Result columns ============

selcollist(A) ::= sclp(B) scanpt expr(C) scanpt as. {
    uint32_t col = ast_result_column(pCtx->astCtx, 0, SYNTAQLITE_NO_SPAN, C);
    A = ast_result_column_list_append(pCtx->astCtx, B, col);
}

selcollist(A) ::= sclp(B) scanpt STAR. {
    uint32_t col = ast_result_column(pCtx->astCtx, 1, SYNTAQLITE_NO_SPAN, SYNTAQLITE_NULL_NODE);
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

expr(A) ::= expr(L) PLUS|MINUS(OP) expr(R). {
    SyntaqliteBinaryOp op = (OP.type == TK_PLUS) ? SYNTAQLITE_BINARY_OP_PLUS : SYNTAQLITE_BINARY_OP_MINUS;
    A = ast_binary_expr(pCtx->astCtx, op, L, R);
}

expr(A) ::= expr(L) STAR|SLASH|REM(OP) expr(R). {
    SyntaqliteBinaryOp op;
    switch (OP.type) {
        case TK_STAR:  op = SYNTAQLITE_BINARY_OP_STAR; break;
        case TK_SLASH: op = SYNTAQLITE_BINARY_OP_SLASH; break;
        default:       op = SYNTAQLITE_BINARY_OP_REM; break;
    }
    A = ast_binary_expr(pCtx->astCtx, op, L, R);
}

expr(A) ::= expr(L) LT|GT|GE|LE(OP) expr(R). {
    SyntaqliteBinaryOp op;
    switch (OP.type) {
        case TK_LT: op = SYNTAQLITE_BINARY_OP_LT; break;
        case TK_GT: op = SYNTAQLITE_BINARY_OP_GT; break;
        case TK_LE: op = SYNTAQLITE_BINARY_OP_LE; break;
        default:    op = SYNTAQLITE_BINARY_OP_GE; break;
    }
    A = ast_binary_expr(pCtx->astCtx, op, L, R);
}

// ============ Literals ============

term(A) ::= INTEGER(B). {
    A = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_INTEGER, syntaqlite_span(pCtx, B));
}

term(A) ::= STRING(B). {
    A = ast_literal(pCtx->astCtx, SYNTAQLITE_LITERAL_TYPE_STRING, syntaqlite_span(pCtx, B));
}

term(A) ::= NULL|FLOAT|BLOB(B). {
    SyntaqliteLiteralType lit_type;
    switch (B.type) {
        case TK_NULL:  lit_type = SYNTAQLITE_LITERAL_TYPE_NULL; break;
        case TK_FLOAT: lit_type = SYNTAQLITE_LITERAL_TYPE_FLOAT; break;
        case TK_BLOB:  lit_type = SYNTAQLITE_LITERAL_TYPE_BLOB; break;
        default:       lit_type = SYNTAQLITE_LITERAL_TYPE_NULL; break;
    }
    A = ast_literal(pCtx->astCtx, lit_type, syntaqlite_span(pCtx, B));
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
