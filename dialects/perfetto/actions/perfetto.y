// Perfetto dialect extension grammar rules.
//
// These rules extend the base SQLite grammar with PerfettoSQL syntax.
// Terminals like PERFETTO, FUNCTION, MODULE, RETURNS, INDEX are added
// to the tokenizer keyword table via extra_keywords extraction.

// Allow extension keywords to be used as regular identifiers.
%fallback ID PERFETTO FUNCTION MODULE RETURNS MACRO DELEGATES INCLUDE.

// ---------- Helper nonterminals ----------

%type perfetto_or_replace {int}
perfetto_or_replace(A) ::= .            { A = 0; }
perfetto_or_replace(A) ::= OR REPLACE.  { A = 1; }

// Argument type: plain ID or JOINID(table.col) form.
%type perfetto_arg_type {SynqParseToken}
perfetto_arg_type(A) ::= ID(B). {
    synq_mark_as_type(pCtx, B);
    A = B;
}
perfetto_arg_type(A) ::= ID(B) LP ID DOT ID RP(E). {
    synq_mark_as_type(pCtx, B);
    A = (SynqParseToken){
        .z = B.z,
        .n = (uint32_t)(E.z + E.n - B.z),
        .type = B.type,
        .token_idx = B.token_idx,
        .offset = B.offset,
        .buf_idx = B.buf_idx,
    };
}

// Argument definition list for functions and table schemas.
%type perfetto_arg_def_list {uint32_t}
perfetto_arg_def_list(A) ::= . { A = SYNTAQLITE_NULL_NODE; }
perfetto_arg_def_list(A) ::= perfetto_arg_def_list_ne(X). { A = X; }

%type perfetto_arg_def_list_ne {uint32_t}
perfetto_arg_def_list_ne(A) ::= ID(N) perfetto_arg_type(T). {
    uint32_t arg = synq_parse_perfetto_arg_def(pCtx,
        synq_parse_ident_name(pCtx, synq_span(pCtx, N)), synq_span(pCtx, T),
        SYNTAQLITE_BOOL_FALSE);
    A = synq_parse_perfetto_arg_def_list(pCtx, SYNTAQLITE_NULL_NODE, arg);
}
perfetto_arg_def_list_ne(A) ::= perfetto_arg_def_list_ne(L) COMMA ID(N) perfetto_arg_type(T). {
    uint32_t arg = synq_parse_perfetto_arg_def(pCtx,
        synq_parse_ident_name(pCtx, synq_span(pCtx, N)), synq_span(pCtx, T),
        SYNTAQLITE_BOOL_FALSE);
    A = synq_parse_perfetto_arg_def_list(pCtx, L, arg);
}
// Variadic argument: name TYPE...
perfetto_arg_def_list_ne(A) ::= ID(N) perfetto_arg_type(T) DOT DOT DOT. {
    uint32_t arg = synq_parse_perfetto_arg_def(pCtx,
        synq_parse_ident_name(pCtx, synq_span(pCtx, N)), synq_span(pCtx, T),
        SYNTAQLITE_BOOL_TRUE);
    A = synq_parse_perfetto_arg_def_list(pCtx, SYNTAQLITE_NULL_NODE, arg);
}
perfetto_arg_def_list_ne(A) ::= perfetto_arg_def_list_ne(L) COMMA ID(N) perfetto_arg_type(T) DOT DOT DOT. {
    uint32_t arg = synq_parse_perfetto_arg_def(pCtx,
        synq_parse_ident_name(pCtx, synq_span(pCtx, N)), synq_span(pCtx, T),
        SYNTAQLITE_BOOL_TRUE);
    A = synq_parse_perfetto_arg_def_list(pCtx, L, arg);
}

// Table schema: optional parenthesized arg list.
%type perfetto_table_schema {uint32_t}
perfetto_table_schema(A) ::= . { A = SYNTAQLITE_NULL_NODE; }
perfetto_table_schema(A) ::= LP perfetto_arg_def_list_ne(L) RP. { A = L; }

// Return type for CREATE PERFETTO FUNCTION.
%type perfetto_return_type {uint32_t}
perfetto_return_type(A) ::= ID(T). {
    synq_mark_as_type(pCtx, T);
    A = synq_parse_perfetto_return_type(pCtx,
        SYNTAQLITE_PERFETTO_RETURN_KIND_SCALAR,
        synq_span(pCtx, T),
        SYNTAQLITE_NULL_NODE);
}
perfetto_return_type(A) ::= TABLE LP perfetto_arg_def_list_ne(L) RP. {
    A = synq_parse_perfetto_return_type(pCtx,
        SYNTAQLITE_PERFETTO_RETURN_KIND_TABLE,
        SYNQ_NO_SPAN,
        L);
}

// Indexed column list for CREATE PERFETTO INDEX.
%type perfetto_indexed_col_list {uint32_t}
perfetto_indexed_col_list(A) ::= ID(N). {
    uint32_t col = synq_parse_perfetto_indexed_column(pCtx, synq_span(pCtx, N));
    A = synq_parse_perfetto_indexed_column_list(pCtx, SYNTAQLITE_NULL_NODE, col);
}
perfetto_indexed_col_list(A) ::= perfetto_indexed_col_list(L) COMMA ID(N). {
    uint32_t col = synq_parse_perfetto_indexed_column(pCtx, synq_span(pCtx, N));
    A = synq_parse_perfetto_indexed_column_list(pCtx, L, col);
}

// Macro argument list.
%type perfetto_macro_arg_list {uint32_t}
perfetto_macro_arg_list(A) ::= . { A = SYNTAQLITE_NULL_NODE; }
perfetto_macro_arg_list(A) ::= perfetto_macro_arg_list_ne(X). { A = X; }

%type perfetto_macro_arg_list_ne {uint32_t}
perfetto_macro_arg_list_ne(A) ::= ID(N) ID(T). {
    synq_mark_as_type(pCtx, T);
    uint32_t arg = synq_parse_perfetto_macro_arg(pCtx,
        synq_span(pCtx, N), synq_span(pCtx, T));
    A = synq_parse_perfetto_macro_arg_list(pCtx, SYNTAQLITE_NULL_NODE, arg);
}
perfetto_macro_arg_list_ne(A) ::= perfetto_macro_arg_list_ne(L) COMMA ID(N) ID(T). {
    synq_mark_as_type(pCtx, T);
    uint32_t arg = synq_parse_perfetto_macro_arg(pCtx,
        synq_span(pCtx, N), synq_span(pCtx, T));
    A = synq_parse_perfetto_macro_arg_list(pCtx, L, arg);
}

// Module name: dotted path like foo.bar.baz
%type perfetto_module_name {SynqParseToken}
perfetto_module_name(A) ::= ID|STAR|INTERSECT(B). { A = B; }
perfetto_module_name(A) ::= perfetto_module_name(B) DOT ID|STAR|INTERSECT(C). {
    A = (SynqParseToken){
        .z = B.z,
        .n = (uint32_t)(C.z + C.n - B.z),
        .type = B.type,
        .token_idx = B.token_idx,
        .offset = B.offset,
        .buf_idx = B.buf_idx,
    };
}

// ---------- CREATE PERFETTO TABLE ----------

cmd(A) ::= CREATE perfetto_or_replace(R) PERFETTO TABLE nm(N) perfetto_table_schema(S) AS select(E). {
    A = synq_parse_create_perfetto_table_stmt(pCtx,
        synq_span(pCtx, N),
        R ? SYNTAQLITE_BOOL_TRUE : SYNTAQLITE_BOOL_FALSE,
        S, E);
}

// ---------- CREATE PERFETTO VIEW ----------

cmd(A) ::= CREATE perfetto_or_replace(R) PERFETTO VIEW nm(N) perfetto_table_schema(S) AS select(E). {
    A = synq_parse_create_perfetto_view_stmt(pCtx,
        synq_span(pCtx, N),
        R ? SYNTAQLITE_BOOL_TRUE : SYNTAQLITE_BOOL_FALSE,
        S, E);
}

// ---------- CREATE PERFETTO FUNCTION ----------

cmd(A) ::= CREATE perfetto_or_replace(R) PERFETTO FUNCTION nm(N) LP perfetto_arg_def_list(ARGS) RP RETURNS perfetto_return_type(RT) AS select(E). {
    A = synq_parse_create_perfetto_function_stmt(pCtx,
        synq_span(pCtx, N),
        R ? SYNTAQLITE_BOOL_TRUE : SYNTAQLITE_BOOL_FALSE,
        ARGS, RT, E);
}

// ---------- CREATE PERFETTO FUNCTION (delegating) ----------

cmd(A) ::= CREATE perfetto_or_replace(R) PERFETTO FUNCTION nm(N) LP perfetto_arg_def_list(ARGS) RP RETURNS perfetto_return_type(RT) DELEGATES TO ID(I). {
    A = synq_parse_create_perfetto_delegating_function_stmt(pCtx,
        synq_span(pCtx, N),
        R ? SYNTAQLITE_BOOL_TRUE : SYNTAQLITE_BOOL_FALSE,
        ARGS, RT, synq_span(pCtx, I));
}

// ---------- CREATE PERFETTO INDEX ----------

cmd(A) ::= CREATE perfetto_or_replace(R) PERFETTO INDEX nm(N) ON nm(T) LP perfetto_indexed_col_list(L) RP. {
    A = synq_parse_create_perfetto_index_stmt(pCtx,
        synq_span(pCtx, N),
        R ? SYNTAQLITE_BOOL_TRUE : SYNTAQLITE_BOOL_FALSE,
        synq_span(pCtx, T),
        L);
}

// Empty marker rule that fires as a default reduction right after the
// parser shifts the return-type ID and BEFORE it shifts AS.  Lemon will
// reduce the empty rule eagerly to satisfy the LALR table, which sets
// the "in macro definition body" flag in the parser context.  The
// tokenizer then skips nested macro expansion while reading the body
// so the body is captured verbatim.  The flag is cleared in the main
// `cmd` action when the CREATE PERFETTO MACRO production reduces.
%type before_macro_body {int}
before_macro_body(A) ::= . {
    pCtx->in_macro_def_body++;
    A = 0;
}

// Macro body: consumes arbitrary tokens via the %wildcard ANY mechanism.
%type perfetto_macro_body {SynqParseToken}
perfetto_macro_body(A) ::= ANY(B). { A = B; }
perfetto_macro_body(A) ::= perfetto_macro_body(B) ANY(C). {
    A = (SynqParseToken){
        .z = B.z,
        .n = (uint32_t)(C.z + C.n - B.z),
        .type = B.type,
        .token_idx = B.token_idx,
        .offset = B.offset,
        .buf_idx = B.buf_idx,
    };
}

// ---------- CREATE PERFETTO MACRO ----------

cmd(A) ::= CREATE perfetto_or_replace(R) PERFETTO MACRO nm(N) LP perfetto_macro_arg_list(ARGS) RP RETURNS ID(T) before_macro_body AS perfetto_macro_body(BODY). {
    synq_mark_as_type(pCtx, T);
    if (pCtx->in_macro_def_body > 0) pCtx->in_macro_def_body--;
    A = synq_parse_create_perfetto_macro_stmt(pCtx,
        synq_span(pCtx, N),
        R ? SYNTAQLITE_BOOL_TRUE : SYNTAQLITE_BOOL_FALSE,
        synq_span(pCtx, T),
        synq_span(pCtx, BODY),
        ARGS);
}


// ---------- INCLUDE PERFETTO MODULE ----------

cmd(A) ::= INCLUDE PERFETTO MODULE perfetto_module_name(M). {
    A = synq_parse_include_perfetto_module_stmt(pCtx,
        synq_span(pCtx, M));
}

// ---------- DROP PERFETTO INDEX ----------

cmd(A) ::= DROP PERFETTO INDEX nm(N) ON nm(T). {
    A = synq_parse_drop_perfetto_index_stmt(pCtx,
        synq_span(pCtx, N),
        synq_span(pCtx, T));
}
