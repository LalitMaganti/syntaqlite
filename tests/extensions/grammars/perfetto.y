# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

// New keywords added by PerfettoSQL.
%token PERFETTO MACRO INCLUDE MODULE RETURNS FUNCTION DELEGATES.

// PerfettoSQL additions to SQLite's fallback list.
%fallback ID FUNCTION MODULE PERFETTO.

// ============ Helper nonterminals ============

pscantok ::= . {
    // Scanner position marker (no value needed)
}

sql_argument_list(A) ::= . {
    A = 0;
}

sql_argument_list(A) ::= sql_argument_list_nonempty(X). {
    A = X;
}

sql_argument_type(A) ::= ID. {
    A = 0;
}

sql_argument_type(A) ::= ID LP ID DOT ID RP. {
    A = 0;
}

sql_variadic_argument_type(A) ::= ID DOT DOT DOT. {
    A = 0;
}

sql_argument_list_nonempty(A) ::= sql_argument_list_nonempty COMMA ID sql_argument_type. {
    A = 0;
}

sql_argument_list_nonempty(A) ::= ID sql_argument_type. {
    A = 0;
}

sql_argument_list_nonempty(A) ::= sql_argument_list_nonempty COMMA ID sql_variadic_argument_type. {
    A = 0;
}

sql_argument_list_nonempty(A) ::= ID sql_variadic_argument_type. {
    A = 0;
}

table_schema(A) ::= . {
    A = 0;
}

table_schema(A) ::= LP sql_argument_list_nonempty RP. {
    A = 0;
}

or_replace(A) ::= . {
    A = 0;
}

or_replace(A) ::= OR REPLACE. {
    A = 1;
}

return_type(A) ::= ID. {
    A = 0;
}

return_type(A) ::= TABLE LP sql_argument_list_nonempty RP. {
    A = 0;
}

table_impl(A) ::= . {
    A = 0;
}

table_impl(A) ::= USING ID. {
    A = 0;
}

indexed_column_list(A) ::= indexed_column_list COMMA ID. {
    A = 0;
}

indexed_column_list(A) ::= ID. {
    A = 0;
}

macro_argument_list_nonempty(A) ::= macro_argument_list_nonempty COMMA ID ID. {
    A = 0;
}

macro_argument_list_nonempty(A) ::= ID ID. {
    A = 0;
}

macro_argument_list(A) ::= . {
    A = 0;
}

macro_argument_list(A) ::= macro_argument_list_nonempty(X). {
    A = X;
}

module_name(A) ::= ID|STAR|INTERSECT(X). {
    A = synq_span(pCtx, X).offset;
}

module_name(A) ::= module_name DOT ID|STAR|INTERSECT. {
    // Keep offset from first segment
    (void)A;
}

macro_body(A) ::= ANY. {
    A = 0;
}

macro_body(A) ::= macro_body ANY. {
    (void)A;
}

// ============ CREATE PERFETTO FUNCTION ============

cmd(A) ::= CREATE or_replace(R) PERFETTO FUNCTION ID(N) LP sql_argument_list RP RETURNS return_type AS select(S) pscantok. {
    A = synq_ast_perfetto_function_stmt(pCtx->astCtx,
        synq_span(pCtx, N), S, (SyntaqliteBool)R);
}

cmd(A) ::= CREATE or_replace(R) PERFETTO FUNCTION ID(N) LP sql_argument_list RP RETURNS return_type DELEGATES TO ID pscantok. {
    A = synq_ast_perfetto_function_stmt(pCtx->astCtx,
        synq_span(pCtx, N), SYNTAQLITE_NULL_NODE, (SyntaqliteBool)R);
}

// ============ CREATE PERFETTO TABLE / VIEW ============

cmd(A) ::= CREATE or_replace(R) PERFETTO TABLE ID(N) table_impl table_schema AS select(S) pscantok. {
    A = synq_ast_perfetto_table_stmt(pCtx->astCtx,
        synq_span(pCtx, N), S, (SyntaqliteBool)R);
}

cmd(A) ::= CREATE or_replace(R) PERFETTO VIEW ID(N) table_schema AS select(S) pscantok. {
    A = synq_ast_perfetto_view_stmt(pCtx->astCtx,
        synq_span(pCtx, N), S, (SyntaqliteBool)R);
}

// ============ CREATE PERFETTO INDEX ============

cmd(A) ::= CREATE or_replace(R) PERFETTO INDEX ID(N) ON ID(T) LP indexed_column_list RP. {
    A = synq_ast_perfetto_index_stmt(pCtx->astCtx,
        synq_span(pCtx, N), synq_span(pCtx, T), (SyntaqliteBool)R);
}

// ============ CREATE PERFETTO MACRO ============

cmd(A) ::= CREATE or_replace(R) PERFETTO MACRO ID(N) LP macro_argument_list RP RETURNS ID AS macro_body pscantok. {
    A = synq_ast_perfetto_macro_stmt(pCtx->astCtx,
        synq_span(pCtx, N), (SyntaqliteBool)R);
}

// ============ INCLUDE PERFETTO MODULE ============

cmd(A) ::= INCLUDE PERFETTO MODULE module_name. {
    // module_name stores the offset of the first segment; compute span from
    // the MODULE token to end of input for simplicity.
    A = synq_ast_perfetto_include_module_stmt(pCtx->astCtx,
        SYNQ_NO_SPAN);
}

// ============ DROP PERFETTO INDEX ============

cmd(A) ::= DROP PERFETTO INDEX ID(N) ON ID(T). {
    A = synq_ast_perfetto_drop_index_stmt(pCtx->astCtx,
        synq_span(pCtx, N), synq_span(pCtx, T));
}
