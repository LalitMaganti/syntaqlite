# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

// New keywords added by PerfettoSQL.
%token PERFETTO MACRO INCLUDE MODULE RETURNS FUNCTION DELEGATES.

// PerfettoSQL additions to SQLite's fallback list.
%fallback ID FUNCTION MODULE PERFETTO.

// Rules (from lemon -g output)
pscantok ::=.
sql_argument_list ::=.
sql_argument_list ::= sql_argument_list_nonempty.
sql_argument_type ::= ID.
sql_argument_type ::= ID LP ID DOT ID RP.
sql_variadic_argument_type ::= ID DOT DOT DOT.
sql_argument_list_nonempty ::= sql_argument_list_nonempty COMMA ID sql_argument_type.
sql_argument_list_nonempty ::= ID sql_argument_type.
sql_argument_list_nonempty ::= sql_argument_list_nonempty COMMA ID sql_variadic_argument_type.
sql_argument_list_nonempty ::= ID sql_variadic_argument_type.
table_schema ::=.
table_schema ::= LP sql_argument_list_nonempty RP.
or_replace ::=.
or_replace ::= OR REPLACE.
cmd ::= CREATE or_replace PERFETTO FUNCTION ID LP sql_argument_list RP RETURNS return_type AS select pscantok.
cmd ::= CREATE or_replace PERFETTO FUNCTION ID LP sql_argument_list RP RETURNS return_type DELEGATES TO ID pscantok.
return_type ::= ID.
return_type ::= TABLE LP sql_argument_list_nonempty RP.
table_impl ::=.
table_impl ::= USING ID.
cmd ::= CREATE or_replace PERFETTO TABLE ID table_impl table_schema AS select pscantok.
cmd ::= CREATE or_replace PERFETTO VIEW ID table_schema AS select pscantok.
cmd ::= CREATE or_replace PERFETTO INDEX ID ON ID LP indexed_column_list RP.
indexed_column_list ::= indexed_column_list COMMA ID.
indexed_column_list ::= ID.
cmd ::= CREATE or_replace PERFETTO MACRO ID LP macro_argument_list RP RETURNS ID AS macro_body pscantok.
macro_argument_list_nonempty ::= macro_argument_list_nonempty COMMA ID ID.
macro_argument_list_nonempty ::= ID ID.
macro_argument_list ::=.
macro_argument_list ::= macro_argument_list_nonempty.
cmd ::= INCLUDE PERFETTO MODULE module_name.
module_name ::= ID|STAR|INTERSECT.
module_name ::= module_name DOT ID|STAR|INTERSECT.
cmd ::= DROP PERFETTO INDEX ID ON ID.
macro_body ::= ANY.
macro_body ::= macro_body ANY.
