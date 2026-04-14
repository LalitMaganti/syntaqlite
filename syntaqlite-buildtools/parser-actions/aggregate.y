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

// ============ Aggregate Function with ORDER BY ============

// Aggregate function call: func(args ORDER BY sortlist) or func(DISTINCT args ORDER BY sortlist)
expr(A) ::= idj(B) LP distinct(C) exprlist(D) ORDER BY sortlist(E) RP. {
    synq_mark_as_function(pCtx, B);
    A = synq_parse_aggregate_function_call(pCtx,
        synq_span(pCtx, B),
        (SyntaqliteAggregateFunctionCallFlags){.raw = (uint8_t)(C & 0xFF)},
        D,
        E,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE);
}

// Aggregate function call with filter/over
expr(A) ::= idj(B) LP distinct(C) exprlist(D) ORDER BY sortlist(E) RP filter_over(F). {
    SyntaqliteFilterOver *fo = AST_NODE_AS(SyntaqliteFilterOver, &pCtx->ast, F);
    synq_mark_as_function(pCtx, B);
    A = synq_parse_aggregate_function_call(pCtx,
        synq_span(pCtx, B),
        (SyntaqliteAggregateFunctionCallFlags){.raw = (uint8_t)(C & 0xFF)},
        D,
        E,
        fo->filter_expr,
        fo->over_def);
}

// ============ Ordered-Set Aggregate (WITHIN GROUP) ============
// e.g. percentile(0.5) WITHIN GROUP (ORDER BY salary)

expr(A) ::= idj(B) LP distinct(C) exprlist(D) RP WITHIN GROUP LP ORDER BY expr(E) RP. {
    synq_mark_as_function(pCtx, B);
    A = synq_parse_ordered_set_function_call(pCtx,
        synq_span(pCtx, B),
        (SyntaqliteAggregateFunctionCallFlags){.raw = (uint8_t)(C & 0xFF)},
        D,
        E,
        SYNTAQLITE_NULL_NODE,
        SYNTAQLITE_NULL_NODE);
}

// Ordered-set aggregate with filter/over
expr(A) ::= idj(B) LP distinct(C) exprlist(D) RP WITHIN GROUP LP ORDER BY expr(E) RP filter_over(F). {
    SyntaqliteFilterOver *fo = AST_NODE_AS(SyntaqliteFilterOver, &pCtx->ast, F);
    synq_mark_as_function(pCtx, B);
    A = synq_parse_ordered_set_function_call(pCtx,
        synq_span(pCtx, B),
        (SyntaqliteAggregateFunctionCallFlags){.raw = (uint8_t)(C & 0xFF)},
        D,
        E,
        fo->filter_expr,
        fo->over_def);
}
