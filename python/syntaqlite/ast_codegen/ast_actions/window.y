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

// ============ Window Definition Lists ============

windowdefn_list(A) ::= windowdefn(B). {
    A = ast_named_window_def_list(pCtx->astCtx, B);
}

windowdefn_list(A) ::= windowdefn_list(B) COMMA windowdefn(C). {
    A = ast_named_window_def_list_append(pCtx->astCtx, B, C);
}

// ============ Window Definition ============

windowdefn(A) ::= nm(B) AS LP window(C) RP. {
    A = ast_named_window_def(pCtx->astCtx,
        syntaqlite_span(pCtx, B),
        C);
}

// ============ Window Specification ============

window(A) ::= PARTITION BY nexprlist(B) orderby_opt(C) frame_opt(D). {
    A = ast_window_def(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN,
        B,
        C,
        D);
}

window(A) ::= nm(B) PARTITION BY nexprlist(C) orderby_opt(D) frame_opt(E). {
    A = ast_window_def(pCtx->astCtx,
        syntaqlite_span(pCtx, B),
        C,
        D,
        E);
}

window(A) ::= ORDER BY sortlist(B) frame_opt(C). {
    A = ast_window_def(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NULL_NODE,
        B,
        C);
}

window(A) ::= nm(B) ORDER BY sortlist(C) frame_opt(D). {
    A = ast_window_def(pCtx->astCtx,
        syntaqlite_span(pCtx, B),
        SYNTAQLITE_NULL_NODE,
        C,
        D);
}

window(A) ::= frame_opt(B). {
    A = ast_window_def(pCtx->astCtx,
        SYNTAQLITE_NO_SPAN,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE,
        B);
}

window(A) ::= nm(B) frame_opt(C). {
    A = ast_window_def(pCtx->astCtx,
        syntaqlite_span(pCtx, B),
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE,
        C);
}

// ============ Frame Specification ============

frame_opt(A) ::= . {
    A = SYNTAQLITE_NULL_NODE;
}

frame_opt(A) ::= range_or_rows(B) frame_bound_s(C) frame_exclude_opt(D). {
    // Single bound: start=C, end=CURRENT ROW (implicit)
    uint32_t end_bound = ast_frame_bound(pCtx->astCtx,
        SYNTAQLITE_FRAME_BOUND_TYPE_CURRENT_ROW,
        SYNTAQLITE_NULL_NODE);
    A = ast_frame_spec(pCtx->astCtx,
        (SyntaqliteFrameType)B,
        (SyntaqliteFrameExclude)D,
        C,
        end_bound);
}

frame_opt(A) ::= range_or_rows(B) BETWEEN frame_bound_s(C) AND frame_bound_e(D) frame_exclude_opt(E). {
    A = ast_frame_spec(pCtx->astCtx,
        (SyntaqliteFrameType)B,
        (SyntaqliteFrameExclude)E,
        C,
        D);
}

// ============ Range or Rows ============

range_or_rows(A) ::= RANGE|ROWS|GROUPS(B). {
    switch (B.type) {
        case TK_RANGE:  A = SYNTAQLITE_FRAME_TYPE_RANGE; break;
        case TK_ROWS:   A = SYNTAQLITE_FRAME_TYPE_ROWS; break;
        default:        A = SYNTAQLITE_FRAME_TYPE_GROUPS; break;
    }
}

// ============ Frame Bounds ============

frame_bound_s(A) ::= frame_bound(B). {
    A = B;
}

frame_bound_s(A) ::= UNBOUNDED PRECEDING. {
    A = ast_frame_bound(pCtx->astCtx,
        SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_PRECEDING,
        SYNTAQLITE_NULL_NODE);
}

frame_bound_e(A) ::= frame_bound(B). {
    A = B;
}

frame_bound_e(A) ::= UNBOUNDED FOLLOWING. {
    A = ast_frame_bound(pCtx->astCtx,
        SYNTAQLITE_FRAME_BOUND_TYPE_UNBOUNDED_FOLLOWING,
        SYNTAQLITE_NULL_NODE);
}

frame_bound(A) ::= expr(B) PRECEDING|FOLLOWING(C). {
    SyntaqliteFrameBoundType bt = (C.type == TK_PRECEDING)
        ? SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_PRECEDING
        : SYNTAQLITE_FRAME_BOUND_TYPE_EXPR_FOLLOWING;
    A = ast_frame_bound(pCtx->astCtx, bt, B);
}

frame_bound(A) ::= CURRENT ROW. {
    A = ast_frame_bound(pCtx->astCtx,
        SYNTAQLITE_FRAME_BOUND_TYPE_CURRENT_ROW,
        SYNTAQLITE_NULL_NODE);
}

// ============ Frame Exclude ============

frame_exclude_opt(A) ::= . {
    A = SYNTAQLITE_FRAME_EXCLUDE_NONE;
}

frame_exclude_opt(A) ::= EXCLUDE frame_exclude(B). {
    A = B;
}

frame_exclude(A) ::= NO OTHERS. {
    A = SYNTAQLITE_FRAME_EXCLUDE_NO_OTHERS;
}

frame_exclude(A) ::= CURRENT ROW. {
    A = SYNTAQLITE_FRAME_EXCLUDE_CURRENT_ROW;
}

frame_exclude(A) ::= GROUP|TIES(B). {
    A = (B.type == TK_GROUP)
        ? SYNTAQLITE_FRAME_EXCLUDE_GROUP
        : SYNTAQLITE_FRAME_EXCLUDE_TIES;
}

// ============ WINDOW Clause ============

window_clause(A) ::= WINDOW windowdefn_list(B). {
    A = B;
}

// ============ Filter/Over ============

filter_over(A) ::= filter_clause(B) over_clause(C). {
    // Unpack the over_clause FilterOver to combine with filter expr
    SyntaqliteFilterOver *fo_over = (SyntaqliteFilterOver*)
        (pCtx->astCtx->ast->arena + pCtx->astCtx->ast->offsets[C]);
    A = ast_filter_over(pCtx->astCtx,
        B,
        fo_over->over_def,
        SYNTAQLITE_NO_SPAN);
}

filter_over(A) ::= over_clause(B). {
    A = B;
}

filter_over(A) ::= filter_clause(B). {
    A = ast_filter_over(pCtx->astCtx,
        B,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NO_SPAN);
}

// ============ Over Clause ============

over_clause(A) ::= OVER LP window(B) RP. {
    A = ast_filter_over(pCtx->astCtx,
        SYNTAQLITE_NULL_NODE,
        B,
        SYNTAQLITE_NO_SPAN);
}

over_clause(A) ::= OVER nm(B). {
    // Create a WindowDef with just base_window_name to represent a named window ref
    uint32_t wdef = ast_window_def(pCtx->astCtx,
        syntaqlite_span(pCtx, B),
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE);
    A = ast_filter_over(pCtx->astCtx,
        SYNTAQLITE_NULL_NODE,
        wdef,
        SYNTAQLITE_NO_SPAN);
}

// ============ Filter Clause ============

filter_clause(A) ::= FILTER LP WHERE expr(B) RP. {
    A = B;
}
