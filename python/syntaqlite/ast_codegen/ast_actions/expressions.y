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

expr(A) ::= expr(L) EQ|NE(OP) expr(R). {
    SyntaqliteBinaryOp op = (OP.type == TK_EQ) ? SYNTAQLITE_BINARY_OP_EQ : SYNTAQLITE_BINARY_OP_NE;
    A = ast_binary_expr(pCtx->astCtx, op, L, R);
}

expr(A) ::= expr(L) AND expr(R). {
    A = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_AND, L, R);
}

expr(A) ::= expr(L) OR expr(R). {
    A = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_OR, L, R);
}

expr(A) ::= expr(L) BITAND|BITOR|LSHIFT|RSHIFT(OP) expr(R). {
    SyntaqliteBinaryOp op;
    switch (OP.type) {
        case TK_BITAND: op = SYNTAQLITE_BINARY_OP_BITAND; break;
        case TK_BITOR:  op = SYNTAQLITE_BINARY_OP_BITOR; break;
        case TK_LSHIFT: op = SYNTAQLITE_BINARY_OP_LSHIFT; break;
        default:        op = SYNTAQLITE_BINARY_OP_RSHIFT; break;
    }
    A = ast_binary_expr(pCtx->astCtx, op, L, R);
}

expr(A) ::= expr(L) CONCAT expr(R). {
    A = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_CONCAT, L, R);
}

expr(A) ::= expr(L) PTR expr(R). {
    A = ast_binary_expr(pCtx->astCtx, SYNTAQLITE_BINARY_OP_PTR, L, R);
}

// ============ Unary Expressions ============

expr(A) ::= PLUS|MINUS(OP) expr(B). [BITNOT] {
    SyntaqliteUnaryOp op = (OP.type == TK_MINUS) ? SYNTAQLITE_UNARY_OP_MINUS : SYNTAQLITE_UNARY_OP_PLUS;
    A = ast_unary_expr(pCtx->astCtx, op, B);
}

expr(A) ::= BITNOT expr(B). {
    A = ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_BITNOT, B);
}

expr(A) ::= NOT expr(B). {
    A = ast_unary_expr(pCtx->astCtx, SYNTAQLITE_UNARY_OP_NOT, B);
}
