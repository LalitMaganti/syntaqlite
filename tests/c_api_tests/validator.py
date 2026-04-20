# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Validator C API scenarios.

Each test method returns a `CApiScenario` with a line-protocol `input`
fed to `validator_driver.c` and the byte-for-byte `expected` stdout.
See `tests/c_api_tests/validator_driver.c` for the supported verbs.
"""

from python.dev.integration_tests.suites.c_api import CApiScenario, CApiTestSuite


class BasicValidator(CApiTestSuite):
    def test_clean_query(self):
        return CApiScenario(
            input="""\
create
add_table users id,name
analyze
SELECT id FROM users;
.
dump_diagnostics
""",
            expected="""\
create ok
add_table ok
analyze ok stmts=1 diags=0
diagnostics count=0
.
""",
        )

    def test_no_handle_analyze(self):
        return CApiScenario(
            input="""\
analyze
SELECT 1;
.
""",
            expected="""\
analyze err no_handle
""",
        )

    def test_parse_error(self):
        return CApiScenario(
            input="""\
create
analyze
SELECT from where;
.
dump_diagnostics
""",
            expected="""\
create ok
analyze ok stmts=1 diags=1
diagnostics count=1
diag[0] sev=ERROR code=PARSE_ERROR off=7 end=11 msg="syntax error near 'from'"
.
""",
        )


class CatalogValidator(CApiTestSuite):
    def test_unknown_cols_accepted(self):
        return CApiScenario(
            input="""\
create
add_table users -
analyze
SELECT anything FROM users;
.
dump_diagnostics
""",
            expected="""\
create ok
add_table ok
analyze ok stmts=1 diags=0
diagnostics count=0
.
""",
        )

    def test_add_view(self):
        return CApiScenario(
            input="""\
create
add_table users id,name
add_view user_names name
analyze
SELECT name FROM user_names;
.
dump_diagnostics
dump_relations
""",
            expected="""\
create ok
add_table ok
add_view ok
analyze ok stmts=1 diags=0
diagnostics count=0
.
relations stmts=1
stmt[0] count=1
  rel[0] name=user_names kind=view
.
""",
        )

    def test_load_ddl_registers_tables(self):
        return CApiScenario(
            input="""\
create
load_ddl
CREATE TABLE users (id INT, name TEXT);
.
analyze
SELECT id FROM users;
.
dump_diagnostics
""",
            expected="""\
create ok
load_ddl ok errs=0
analyze ok stmts=1 diags=0
diagnostics count=0
.
""",
        )

    def test_reset_clears_catalog(self):
        return CApiScenario(
            input="""\
create
add_table users id,name
analyze
SELECT id FROM users;
.
dump_diagnostics
reset_catalog
analyze
SELECT id FROM users;
.
dump_diagnostics
""",
            expected="""\
create ok
add_table ok
analyze ok stmts=1 diags=0
diagnostics count=0
.
reset_catalog ok
analyze ok stmts=1 diags=1
diagnostics count=1
diag[0] sev=WARNING code=UNKNOWN_TABLE off=15 end=20 msg="unknown table 'users'"
.
""",
        )


class DiagnosticsValidator(CApiTestSuite):
    def test_unknown_table(self):
        return CApiScenario(
            input="""\
create
add_table users id,name
analyze
SELECT id FROM userz;
.
dump_diagnostics
""",
            expected="""\
create ok
add_table ok
analyze ok stmts=1 diags=1
diagnostics count=1
diag[0] sev=WARNING code=UNKNOWN_TABLE off=15 end=20 msg="unknown table 'userz'"
.
""",
        )

    def test_unknown_column(self):
        return CApiScenario(
            input="""\
create
add_table users id,name
analyze
SELECT emaill FROM users;
.
dump_diagnostics
""",
            expected="""\
create ok
add_table ok
analyze ok stmts=1 diags=1
diagnostics count=1
diag[0] sev=WARNING code=UNKNOWN_COLUMN off=7 end=13 msg="unknown column 'emaill'"
.
""",
        )

    def test_unknown_function(self):
        return CApiScenario(
            input="""\
create
add_table users id,name
analyze
SELECT noop(id) FROM users;
.
dump_diagnostics
""",
            expected="""\
create ok
add_table ok
analyze ok stmts=1 diags=1
diagnostics count=1
diag[0] sev=WARNING code=UNKNOWN_FUNCTION off=7 end=11 msg="unknown function 'noop'"
.
""",
        )


class PolicyValidator(CApiTestSuite):
    def test_strict_schema_promotes(self):
        return CApiScenario(
            input="""\
create
strict_schema 1
add_table users id,name
analyze
SELECT id FROM userz;
.
dump_diagnostics
""",
            expected="""\
create ok
strict_schema ok
add_table ok
analyze ok stmts=1 diags=1
diagnostics count=1
diag[0] sev=ERROR code=UNKNOWN_TABLE off=15 end=20 msg="unknown table 'userz'"
.
""",
        )

    def test_check_level_allow_suppresses(self):
        return CApiScenario(
            input="""\
create
add_table users id,name
check_level unknown-table allow
analyze
SELECT id FROM userz;
.
dump_diagnostics
""",
            expected="""\
create ok
add_table ok
check_level ok
analyze ok stmts=1 diags=0
diagnostics count=0
.
""",
        )

    def test_check_level_deny_escalates(self):
        return CApiScenario(
            input="""\
create
add_table users id,name
check_level unknown-table deny
analyze
SELECT id FROM userz;
.
dump_diagnostics
""",
            expected="""\
create ok
add_table ok
check_level ok
analyze ok stmts=1 diags=1
diagnostics count=1
diag[0] sev=ERROR code=UNKNOWN_TABLE off=15 end=20 msg="unknown table 'userz'"
.
""",
        )

    def test_check_level_unknown_category(self):
        return CApiScenario(
            input="""\
create
check_level nonsense deny
""",
            expected="""\
create ok
check_level err unknown
""",
        )

    def test_mode_execute_persists_ddl(self):
        return CApiScenario(
            input="""\
create
mode execute
analyze
CREATE TABLE t (a, b);
.
analyze
SELECT a FROM t;
.
dump_diagnostics
""",
            expected="""\
create ok
mode ok
analyze ok stmts=1 diags=0
analyze ok stmts=1 diags=0
diagnostics count=0
.
""",
        )

    def test_mode_document_resets_ddl(self):
        return CApiScenario(
            input="""\
create
mode document
analyze
CREATE TABLE t (a, b);
.
analyze
SELECT a FROM t;
.
dump_diagnostics
""",
            expected="""\
create ok
mode ok
analyze ok stmts=1 diags=0
analyze ok stmts=1 diags=1
diagnostics count=1
diag[0] sev=WARNING code=UNKNOWN_TABLE off=14 end=15 msg="unknown table 't'"
.
""",
        )


class IntrospectionValidator(CApiTestSuite):
    def test_lineage_simple(self):
        return CApiScenario(
            input="""\
create
add_table users id,name,email
analyze
SELECT u.id, u.name AS display FROM users u;
.
dump_lineage
""",
            expected="""\
create ok
add_table ok
analyze ok stmts=1 diags=0
lineage stmts=1
stmt[0] count=2
  col[0] name=id idx=0 origin=users.id
  col[1] name=display idx=1 origin=users.name
.
""",
        )

    def test_relations_and_physical(self):
        return CApiScenario(
            input="""\
create
load_ddl
CREATE TABLE users (id, name);
CREATE VIEW active_users AS SELECT id FROM users;
.
analyze
SELECT id FROM active_users;
.
dump_relations
dump_physical_tables
""",
            expected="""\
create ok
load_ddl ok errs=0
analyze ok stmts=1 diags=0
relations stmts=1
stmt[0] count=1
  rel[0] name=active_users kind=view
.
physical_tables stmts=1
stmt[0] count=1
  tbl[0] name=active_users
.
""",
        )

    def test_defined_relations(self):
        return CApiScenario(
            input="""\
create
mode execute
analyze
CREATE TABLE t1 (a, b);
CREATE VIEW v1 AS SELECT * FROM t1;
.
dump_defined_relations
""",
            expected="""\
create ok
mode ok
analyze ok stmts=2 diags=0
defined_relations stmts=2
stmt[0] count=1
  def[0] name=t1 kind=table
stmt[1] count=1
  def[0] name=v1 kind=view
.
""",
        )

    def test_stmt_diagnostics_per_statement(self):
        return CApiScenario(
            input="""\
create
add_table users id,name
analyze
SELECT id FROM users;
SELECT id FROM userz;
.
stmt_count
stmt_diagnostics 0
stmt_diagnostics 1
""",
            expected="""\
create ok
add_table ok
analyze ok stmts=2 diags=1
stmt_count 2
stmt_diagnostics idx=0 count=0
.
stmt_diagnostics idx=1 count=1
diag[0] sev=WARNING code=UNKNOWN_TABLE off=37 end=42 msg="unknown table 'userz'"
.
""",
        )


class FunctionsValidator(CApiTestSuite):
    def test_register_scalar_fn(self):
        return CApiScenario(
            input="""\
create
add_table users id,name
add_function noop scalar exact 1
analyze
SELECT noop(id) FROM users;
.
dump_diagnostics
""",
            expected="""\
create ok
add_table ok
add_function ok
analyze ok stmts=1 diags=0
diagnostics count=0
.
""",
        )

    def test_function_arity_mismatch(self):
        return CApiScenario(
            input="""\
create
add_table users id,name
add_function noop scalar exact 1
analyze
SELECT noop(id, name) FROM users;
.
dump_diagnostics
""",
            expected="""\
create ok
add_table ok
add_function ok
analyze ok stmts=1 diags=1
diagnostics count=1
diag[0] sev=WARNING code=FUNCTION_ARITY off=7 end=11 msg="function 'noop' expects 1 argument(s), got 2"
.
""",
        )
