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

// ============ Expressions ============

// Error recovery: accept a parse error in expression position so that
// interpolation holes (e.g. f-string `{expr}`) can be represented as
// Error nodes without discarding the rest of the statement.
expr(A) ::= error. {
    A = synq_parse_error(pCtx, synq_error_span(pCtx));
}

expr(A) ::= term(B). {
    A = B;
}

expr(A) ::= LP expr(B) RP. {
    A = synq_pass(pCtx, B);
}

expr(A) ::= expr(L) PLUS|MINUS(OP) expr(R). {
    SyntaqliteBinaryOp op = (OP.type == SYNTAQLITE_TK_PLUS) ? SYNTAQLITE_BINARY_OP_PLUS : SYNTAQLITE_BINARY_OP_MINUS;
    A = synq_parse_binary_expr(pCtx, op, L, R);
}

expr(A) ::= expr(L) STAR|SLASH|REM(OP) expr(R). {
    SyntaqliteBinaryOp op;
    switch (OP.type) {
        case SYNTAQLITE_TK_STAR:  op = SYNTAQLITE_BINARY_OP_STAR; break;
        case SYNTAQLITE_TK_SLASH: op = SYNTAQLITE_BINARY_OP_SLASH; break;
        default:       op = SYNTAQLITE_BINARY_OP_REM; break;
    }
    A = synq_parse_binary_expr(pCtx, op, L, R);
}

expr(A) ::= expr(L) LT|GT|GE|LE(OP) expr(R). {
    SyntaqliteBinaryOp op;
    switch (OP.type) {
        case SYNTAQLITE_TK_LT: op = SYNTAQLITE_BINARY_OP_LT; break;
        case SYNTAQLITE_TK_GT: op = SYNTAQLITE_BINARY_OP_GT; break;
        case SYNTAQLITE_TK_LE: op = SYNTAQLITE_BINARY_OP_LE; break;
        default:    op = SYNTAQLITE_BINARY_OP_GE; break;
    }
    A = synq_parse_binary_expr(pCtx, op, L, R);
}

expr(A) ::= expr(L) EQ|NE(OP) expr(R). {
    SyntaqliteBinaryOp op = (OP.type == SYNTAQLITE_TK_EQ) ? SYNTAQLITE_BINARY_OP_EQ : SYNTAQLITE_BINARY_OP_NE;
    A = synq_parse_binary_expr(pCtx, op, L, R);
}

expr(A) ::= expr(L) AND expr(R). {
    A = synq_parse_binary_expr(pCtx, SYNTAQLITE_BINARY_OP_AND, L, R);
}

expr(A) ::= expr(L) OR expr(R). {
    A = synq_parse_binary_expr(pCtx, SYNTAQLITE_BINARY_OP_OR, L, R);
}

expr(A) ::= expr(L) BITAND|BITOR|LSHIFT|RSHIFT(OP) expr(R). {
    SyntaqliteBinaryOp op;
    switch (OP.type) {
        case SYNTAQLITE_TK_BITAND: op = SYNTAQLITE_BINARY_OP_BIT_AND; break;
        case SYNTAQLITE_TK_BITOR:  op = SYNTAQLITE_BINARY_OP_BIT_OR; break;
        case SYNTAQLITE_TK_LSHIFT: op = SYNTAQLITE_BINARY_OP_LSHIFT; break;
        default:        op = SYNTAQLITE_BINARY_OP_RSHIFT; break;
    }
    A = synq_parse_binary_expr(pCtx, op, L, R);
}

expr(A) ::= expr(L) CONCAT expr(R). {
    A = synq_parse_binary_expr(pCtx, SYNTAQLITE_BINARY_OP_CONCAT, L, R);
}

expr(A) ::= expr(L) PTR(OP) expr(R). {
    SyntaqliteBinaryOp op = (OP.n == 3) ? SYNTAQLITE_BINARY_OP_PTR2 : SYNTAQLITE_BINARY_OP_PTR;
    A = synq_parse_binary_expr(pCtx, op, L, R);
}

// ============ Unary Expressions ============

expr(A) ::= PLUS|MINUS(OP) expr(B). [BITNOT] {
    SyntaqliteUnaryOp op = (OP.type == SYNTAQLITE_TK_MINUS) ? SYNTAQLITE_UNARY_OP_MINUS : SYNTAQLITE_UNARY_OP_PLUS;
    A = synq_parse_unary_expr(pCtx, op, B);
}

expr(A) ::= BITNOT expr(B). {
    A = synq_parse_unary_expr(pCtx, SYNTAQLITE_UNARY_OP_BIT_NOT, B);
}

expr(A) ::= NOT expr(B). {
    A = synq_parse_unary_expr(pCtx, SYNTAQLITE_UNARY_OP_NOT, B);
}
