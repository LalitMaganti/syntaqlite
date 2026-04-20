# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Python API imperative integration tests.

Exercises the public :mod:`syntaqlite` package in-process: parse /
parse_raw / format_sql / tokenize / validate / lifecycle. Each test
constructs its own :class:`syntaqlite.Syntaqlite` so the READY
handshake, subprocess spawn, and close path are covered on every case.
"""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path

from python.dev.integration_tests.suite import SuiteContext

NAME = "python-api"
DESCRIPTION = "Python API imperative tests (in-process)"

_GREEN = "\033[32m"
_RED = "\033[31m"
_RESET = "\033[0m"


def _pass(name: str) -> None:
    print(f"  {_GREEN}PASS{_RESET}  {name}")


def _fail(name: str, detail: str) -> None:
    print(f"  {_RED}FAIL{_RESET}  {name}: {detail}")


# ── Parse ─────────────────────────────────────────────────────────────────────


def _test_parse_simple_select(stq) -> bool:
    with stq.Syntaqlite() as sq:
        stmts = sq.parse("SELECT 1")
    if len(stmts) != 1 or type(stmts[0]).__name__ != "SelectStmt":
        _fail("parse_simple_select", f"got {[type(s).__name__ for s in stmts]!r}")
        return False
    _pass("parse_simple_select")
    return True


def _test_parse_cte(stq) -> bool:
    with stq.Syntaqlite() as sq:
        stmts = sq.parse("WITH x AS (SELECT 1) SELECT * FROM x")
    if len(stmts) != 1 or type(stmts[0]).__name__ != "WithClause":
        _fail("parse_cte", f"expected WithClause, got {[type(s).__name__ for s in stmts]!r}")
        return False
    _pass("parse_cte")
    return True


def _test_parse_insert(stq) -> bool:
    with stq.Syntaqlite() as sq:
        stmts = sq.parse("INSERT INTO t VALUES (1)")
    if len(stmts) != 1 or type(stmts[0]).__name__ != "InsertStmt":
        _fail("parse_insert", f"got {[type(s).__name__ for s in stmts]!r}")
        return False
    _pass("parse_insert")
    return True


def _test_parse_update(stq) -> bool:
    with stq.Syntaqlite() as sq:
        stmts = sq.parse("UPDATE t SET x=1")
    if len(stmts) != 1 or type(stmts[0]).__name__ != "UpdateStmt":
        _fail("parse_update", f"got {[type(s).__name__ for s in stmts]!r}")
        return False
    _pass("parse_update")
    return True


def _test_parse_delete(stq) -> bool:
    with stq.Syntaqlite() as sq:
        stmts = sq.parse("DELETE FROM t")
    if len(stmts) != 1 or type(stmts[0]).__name__ != "DeleteStmt":
        _fail("parse_delete", f"got {[type(s).__name__ for s in stmts]!r}")
        return False
    _pass("parse_delete")
    return True


def _test_parse_create_table(stq) -> bool:
    with stq.Syntaqlite() as sq:
        stmts = sq.parse("CREATE TABLE t (a INT)")
    if len(stmts) != 1 or type(stmts[0]).__name__ != "CreateTableStmt":
        _fail("parse_create_table", f"got {[type(s).__name__ for s in stmts]!r}")
        return False
    _pass("parse_create_table")
    return True


def _test_parse_compound_select(stq) -> bool:
    with stq.Syntaqlite() as sq:
        stmts = sq.parse("SELECT 1 UNION SELECT 2")
    if len(stmts) != 1 or type(stmts[0]).__name__ != "CompoundSelect":
        _fail("parse_compound_select", f"got {[type(s).__name__ for s in stmts]!r}")
        return False
    _pass("parse_compound_select")
    return True


def _test_parse_multi_statement(stq) -> bool:
    with stq.Syntaqlite() as sq:
        stmts = sq.parse("SELECT 1; SELECT 2")
    names = [type(s).__name__ for s in stmts]
    if names != ["SelectStmt", "SelectStmt"]:
        _fail("parse_multi_statement", f"got {names!r}")
        return False
    _pass("parse_multi_statement")
    return True


def _test_parse_bad_sql_returns_empty(stq) -> bool:
    """Bad SQL from `parse` returns an empty list — it doesn't raise.
    (Diagnostics surface via `validate` and errors via `format_sql`.)"""
    with stq.Syntaqlite() as sq:
        stmts = sq.parse("SELECT FROM WHERE")
    if stmts:
        _fail("parse_bad_sql_returns_empty", f"expected [], got {[type(s).__name__ for s in stmts]!r}")
        return False
    _pass("parse_bad_sql_returns_empty")
    return True


def _test_parse_raw_returns_dicts(stq) -> bool:
    with stq.Syntaqlite() as sq:
        raw = sq.parse_raw("SELECT 1")
    if not raw or not isinstance(raw[0], dict) or raw[0].get("type") != "SelectStmt":
        _fail("parse_raw_returns_dicts", f"got {raw!r}")
        return False
    _pass("parse_raw_returns_dicts")
    return True


# ── format_sql ────────────────────────────────────────────────────────────────


def _test_format_keyword_case_upper(stq) -> bool:
    with stq.Syntaqlite() as sq:
        out = sq.format_sql("select 1", keyword_case="upper")
    if "SELECT" not in out or "select " in out:
        _fail("format_keyword_case_upper", f"got {out!r}")
        return False
    _pass("format_keyword_case_upper")
    return True


def _test_format_keyword_case_lower(stq) -> bool:
    with stq.Syntaqlite() as sq:
        out = sq.format_sql("SELECT 1", keyword_case="lower")
    if "select" not in out or "SELECT" in out:
        _fail("format_keyword_case_lower", f"got {out!r}")
        return False
    _pass("format_keyword_case_lower")
    return True


def _test_format_indent_width(stq) -> bool:
    """4-space indent should actually produce 4-space indented bodies."""
    sql = "SELECT a, b, c, d, e, f, g, h, i, j, k FROM my_table WHERE a > 0 AND b < 100"
    with stq.Syntaqlite() as sq:
        out = sq.format_sql(sql, line_width=40, indent_width=4)
    if "    " not in out:
        _fail("format_indent_width", f"expected 4-space indent, got:\n{out}")
        return False
    _pass("format_indent_width")
    return True


def _test_format_line_width_wraps(stq) -> bool:
    sql = "SELECT a, b, c, d, e, f, g, h FROM my_very_long_table_name_here"
    with stq.Syntaqlite() as sq:
        narrow = sq.format_sql(sql, line_width=30)
        wide = sq.format_sql(sql, line_width=200)
    if narrow.count("\n") <= wide.count("\n"):
        _fail(
            "format_line_width_wraps",
            f"narrow ({narrow.count(chr(10))} lines) should wrap more than wide "
            f"({wide.count(chr(10))} lines)\nnarrow={narrow!r}\nwide={wide!r}",
        )
        return False
    _pass("format_line_width_wraps")
    return True


def _test_format_semicolons_off(stq) -> bool:
    with stq.Syntaqlite() as sq:
        with_semi = sq.format_sql("SELECT 1", semicolons=True)
        without = sq.format_sql("SELECT 1", semicolons=False)
    if not with_semi.rstrip().endswith(";"):
        _fail("format_semicolons_off", f"expected trailing ';' with semicolons=True: {with_semi!r}")
        return False
    if without.rstrip().endswith(";"):
        _fail("format_semicolons_off", f"expected no trailing ';' with semicolons=False: {without!r}")
        return False
    _pass("format_semicolons_off")
    return True


def _test_format_bad_sql_raises(stq) -> bool:
    with stq.Syntaqlite() as sq:
        try:
            sq.format_sql("SELECT FROM WHERE")
        except stq.FormatError:
            _pass("format_bad_sql_raises")
            return True
    _fail("format_bad_sql_raises", "expected FormatError")
    return False


# ── tokenize ──────────────────────────────────────────────────────────────────


def _test_tokenize_keyword_category(stq) -> bool:
    with stq.Syntaqlite() as sq:
        toks = sq.tokenize("SELECT")
    if len(toks) != 1 or toks[0].get("category") != "keyword":
        _fail("tokenize_keyword_category", f"got {toks!r}")
        return False
    _pass("tokenize_keyword_category")
    return True


def _test_tokenize_mixed_categories(stq) -> bool:
    with stq.Syntaqlite() as sq:
        toks = sq.tokenize("SELECT my_col, 42, 'x' FROM t")
    pairs = [(t["text"], t["category"]) for t in toks]
    expected = {
        ("SELECT", "keyword"),
        ("my_col", "identifier"),
        ("42", "number"),
        ("'x'", "string"),
        ("FROM", "keyword"),
        ("t", "identifier"),
    }
    if not expected.issubset(pairs):
        _fail("tokenize_mixed_categories", f"missing categories in {pairs!r}")
        return False
    _pass("tokenize_mixed_categories")
    return True


def _test_tokenize_offset_and_length_align(stq) -> bool:
    sql = "SELECT my_col"
    with stq.Syntaqlite() as sq:
        toks = sq.tokenize(sql)
    for t in toks:
        if sql[t["offset"] : t["offset"] + t["length"]] != t["text"]:
            _fail(
                "tokenize_offset_and_length_align",
                f"slice mismatch for {t!r}: sql[{t['offset']}:{t['offset']+t['length']}]="
                f"{sql[t['offset']:t['offset']+t['length']]!r}",
            )
            return False
    _pass("tokenize_offset_and_length_align")
    return True


def _test_tokenize_comment_and_operator(stq) -> bool:
    with stq.Syntaqlite() as sq:
        toks = sq.tokenize("SELECT 1 + 2 -- trailing")
    cats = {t["category"] for t in toks}
    if "comment" not in cats or "operator" not in cats:
        _fail("tokenize_comment_and_operator", f"categories={cats!r}, tokens={toks!r}")
        return False
    _pass("tokenize_comment_and_operator")
    return True


# ── validate ──────────────────────────────────────────────────────────────────


def _test_validate_no_schema_unknown_table(stq) -> bool:
    with stq.Syntaqlite() as sq:
        result = sq.analyze("SELECT * FROM does_not_exist")
    matching = [d for d in result.diagnostics if d.code is stq.DiagnosticCode.UNKNOWN_TABLE]
    if not matching or "does_not_exist" not in matching[0].message:
        _fail("validate_no_schema_unknown_table", f"got {result.diagnostics!r}")
        return False
    _pass("validate_no_schema_unknown_table")
    return True


def _test_validate_unknown_column_code(stq) -> bool:
    with stq.Syntaqlite() as sq:
        schema = stq.Schema(tables=[stq.Table("users", ["id"])])
        result = sq.analyze("SELECT bogus FROM users", schema)
    matching = [d for d in result.diagnostics if d.code is stq.DiagnosticCode.UNKNOWN_COLUMN]
    if not matching or "bogus" not in matching[0].message:
        _fail("validate_unknown_column_code", f"got {result.diagnostics!r}")
        return False
    _pass("validate_unknown_column_code")
    return True


def _test_validate_unknown_function(stq) -> bool:
    with stq.Syntaqlite() as sq:
        schema = stq.Schema(tables=[stq.Table("t", ["a"])])
        result = sq.analyze("SELECT nosuch(a) FROM t", schema)
    matching = [d for d in result.diagnostics if d.code is stq.DiagnosticCode.UNKNOWN_FUNCTION]
    if not matching or "nosuch" not in matching[0].message:
        _fail("validate_unknown_function", f"got {result.diagnostics!r}")
        return False
    _pass("validate_unknown_function")
    return True


def _test_validate_function_arity(stq) -> bool:
    with stq.Syntaqlite() as sq:
        result = sq.analyze("SELECT length(1, 2)")
    matching = [d for d in result.diagnostics if d.code is stq.DiagnosticCode.FUNCTION_ARITY]
    if not matching:
        _fail("validate_function_arity", f"got {result.diagnostics!r}")
        return False
    _pass("validate_function_arity")
    return True


def _test_validate_cte_column_mismatch(stq) -> bool:
    with stq.Syntaqlite() as sq:
        result = sq.analyze("WITH c(a,b) AS (SELECT 1) SELECT * FROM c")
    matching = [d for d in result.diagnostics if d.code is stq.DiagnosticCode.CTE_COLUMN_COUNT_MISMATCH]
    if not matching:
        _fail("validate_cte_column_mismatch", f"got {result.diagnostics!r}")
        return False
    _pass("validate_cte_column_mismatch")
    return True


def _test_validate_parse_error_diagnostic(stq) -> bool:
    """Malformed SQL surfaces via `validate` as a PARSE_ERROR diagnostic."""
    with stq.Syntaqlite() as sq:
        result = sq.analyze("SELECT FROM WHERE")
    matching = [d for d in result.diagnostics if d.code is stq.DiagnosticCode.PARSE_ERROR]
    if not matching:
        _fail("validate_parse_error_diagnostic", f"got {result.diagnostics!r}")
        return False
    _pass("validate_parse_error_diagnostic")
    return True


def _test_validate_views_resolve(stq) -> bool:
    with stq.Syntaqlite() as sq:
        schema = stq.Schema(views=[stq.View("v", ["n"])])
        result = sq.analyze("SELECT n FROM v", schema)
    if result.diagnostics:
        _fail("validate_views_resolve", f"unexpected diags: {result.diagnostics!r}")
        return False
    _pass("validate_views_resolve")
    return True


def _test_validate_ddl_is_parsed(stq) -> bool:
    """Schema.ddl registers tables from raw CREATE statements."""
    with stq.Syntaqlite() as sq:
        schema = stq.Schema(ddl="CREATE TABLE t (a INT, b TEXT);")
        result = sq.analyze("SELECT a, b FROM t", schema)
    if result.diagnostics:
        _fail("validate_ddl_is_parsed", f"unexpected diags: {result.diagnostics!r}")
        return False
    _pass("validate_ddl_is_parsed")
    return True


def _test_validate_diagnostic_fields_present(stq) -> bool:
    """Every diagnostic exposes severity/message/start_offset/end_offset/code."""
    with stq.Syntaqlite() as sq:
        schema = stq.Schema(tables=[stq.Table("users", ["id"])])
        result = sq.analyze("SELECT bogus FROM users", schema)
    if not result.diagnostics:
        _fail("validate_diagnostic_fields_present", "expected at least one diag")
        return False
    d = result.diagnostics[0]
    missing = [
        f for f in ("severity", "message", "start_offset", "end_offset", "code")
        if not hasattr(d, f)
    ]
    if missing:
        _fail("validate_diagnostic_fields_present", f"missing fields: {missing!r}")
        return False
    if not isinstance(d.severity, str) or not isinstance(d.message, str):
        _fail("validate_diagnostic_fields_present", f"wrong types: {d!r}")
        return False
    if not isinstance(d.start_offset, int) or not isinstance(d.end_offset, int):
        _fail("validate_diagnostic_fields_present", f"offsets wrong type: {d!r}")
        return False
    if not isinstance(d.code, stq.DiagnosticCode):
        _fail("validate_diagnostic_fields_present", f"code wrong type: {type(d.code)}")
        return False
    _pass("validate_diagnostic_fields_present")
    return True


def _test_validate_lineage_union_first_branch(stq) -> bool:
    """UNION lineage columns come from the first branch."""
    with stq.Syntaqlite() as sq:
        result = sq.analyze("SELECT 1 AS a UNION SELECT 2")
    if result.lineage is None or [c.name for c in result.lineage.columns] != ["a"]:
        _fail(
            "validate_lineage_union_first_branch",
            f"expected ['a'], got {result.lineage!r}",
        )
        return False
    _pass("validate_lineage_union_first_branch")
    return True


def _test_validate_lineage_aggregate_drops_origin(stq) -> bool:
    """Computed columns (aggregates) have `origin is None`."""
    with stq.Syntaqlite() as sq:
        schema = stq.Schema(tables=[stq.Table("t", ["x"])])
        result = sq.analyze("SELECT count(*) AS n FROM t", schema)
    if result.lineage is None or not result.lineage.columns:
        _fail("validate_lineage_aggregate_drops_origin", f"no lineage: {result.lineage!r}")
        return False
    col = result.lineage.columns[0]
    if col.name != "n" or col.origin is not None:
        _fail(
            "validate_lineage_aggregate_drops_origin",
            f"expected name='n' origin=None, got {col!r}",
        )
        return False
    _pass("validate_lineage_aggregate_drops_origin")
    return True


def _test_validate_lineage_basic(stq) -> bool:
    with stq.Syntaqlite() as sq:
        schema = stq.Schema(tables=[stq.Table("users", ["id", "name", "email"])])
        result = sq.analyze("SELECT id, name FROM users", schema)
    if result.lineage is None or not result.lineage.complete:
        _fail("validate_lineage_basic", f"got {result.lineage!r}")
        return False
    names = [c.name for c in result.lineage.columns]
    if names != ["id", "name"]:
        _fail("validate_lineage_basic", f"expected ['id','name'], got {names!r}")
        return False
    first = result.lineage.columns[0]
    if first.origin is None or first.origin.table != "users" or first.origin.column != "id":
        _fail("validate_lineage_basic", f"expected users.id origin, got {first.origin!r}")
        return False
    if result.lineage.physical_tables != ["users"]:
        _fail("validate_lineage_basic", f"physical_tables={result.lineage.physical_tables!r}")
        return False
    _pass("validate_lineage_basic")
    return True


def _test_validate_relations(stq) -> bool:
    with stq.Syntaqlite() as sq:
        schema = stq.Schema(tables=[stq.Table("users", ["id"])])
        result = sq.analyze("SELECT id FROM users", schema)
    if result.lineage is None or not result.lineage.relations:
        _fail("validate_relations", f"no relations: {result.lineage!r}")
        return False
    rel = result.lineage.relations[0]
    if rel.name != "users" or rel.kind != "table":
        _fail("validate_relations", f"expected users/table, got {rel!r}")
        return False
    _pass("validate_relations")
    return True


def _test_validate_text_output_with_source_name(stq) -> bool:
    with stq.Syntaqlite() as sq:
        schema = stq.Schema(tables=[stq.Table("users", ["id", "name"])])
        rendered = sq.analyze(
            "SELECT nme FROM users",
            schema,
            output=stq.AnalysisOutput.TEXT,
            render_options=stq.RenderOptions(source_name="query.sql"),
        )
    if not isinstance(rendered, str):
        _fail("validate_text_output_with_source_name", f"expected str, got {type(rendered).__name__}")
        return False
    if "nme" not in rendered or "query.sql" not in rendered:
        _fail("validate_text_output_with_source_name", f"missing tokens in {rendered!r}")
        return False
    _pass("validate_text_output_with_source_name")
    return True


def _test_validate_schema_modules_passthrough(stq) -> bool:
    """Schema(modules=...) shouldn't break validation for SQLite (it ignores them)."""
    with stq.Syntaqlite() as sq:
        schema = stq.Schema(
            tables=[stq.Table("t", ["x"])],
            modules={"stdlib.helpers": "CREATE TABLE helper(a INT)"},
        )
        result = sq.analyze("SELECT x FROM t", schema)
    if result.diagnostics:
        _fail("validate_schema_modules_passthrough", f"unexpected diags: {result.diagnostics!r}")
        return False
    _pass("validate_schema_modules_passthrough")
    return True


# ── validate — per-statement breakdown ───────────────────────────────────────


def _test_validate_statements_breakdown(stq) -> bool:
    """Each top-level statement gets its own per-stmt row with its SQL source."""
    with stq.Syntaqlite() as sq:
        result = sq.validate("CREATE TABLE users (id INT); SELECT id FROM users;")
    if len(result.statements) != 2:
        _fail("validate_statements_breakdown", f"expected 2 statements, got {len(result.statements)}")
        return False
    ddl, sel = result.statements
    if "CREATE TABLE" not in ddl.source or "SELECT" not in sel.source:
        _fail("validate_statements_breakdown", f"sources wrong: {ddl.source!r}, {sel.source!r}")
        return False
    if ddl.lineage is not None:
        _fail("validate_statements_breakdown", f"DDL should have no lineage, got {ddl.lineage!r}")
        return False
    if sel.lineage is None or [c.name for c in sel.lineage.columns] != ["id"]:
        _fail("validate_statements_breakdown", f"SELECT lineage wrong: {sel.lineage!r}")
        return False
    _pass("validate_statements_breakdown")
    return True


def _test_defined_relations_table(stq) -> bool:
    with stq.Syntaqlite() as sq:
        result = sq.validate("CREATE TABLE users (id INT)")
    stmt = result.statements[0]
    if len(stmt.defined_relations) != 1:
        _fail("defined_relations_table", f"got {stmt.defined_relations!r}")
        return False
    rel = stmt.defined_relations[0]
    if rel.name != "users" or rel.is_view:
        _fail("defined_relations_table", f"expected users/table, got {rel!r}")
        return False
    _pass("defined_relations_table")
    return True


def _test_defined_relations_view(stq) -> bool:
    with stq.Syntaqlite() as sq:
        result = sq.validate("CREATE VIEW v AS SELECT 1 AS x")
    stmt = result.statements[0]
    if not stmt.defined_relations:
        _fail("defined_relations_view", f"no relations: {stmt!r}")
        return False
    rel = stmt.defined_relations[0]
    if rel.name != "v" or not rel.is_view:
        _fail("defined_relations_view", f"expected v/view, got {rel!r}")
        return False
    _pass("defined_relations_view")
    return True


def _test_column_lineage_index(stq) -> bool:
    with stq.Syntaqlite() as sq:
        schema = stq.Schema(tables=[stq.Table("users", ["id", "name"])])
        result = sq.validate("SELECT id, name FROM users", schema)
    indices = [c.index for c in result.lineage.columns]
    if indices != [0, 1]:
        _fail("column_lineage_index", f"expected [0, 1], got {indices!r}")
        return False
    _pass("column_lineage_index")
    return True


def _test_lineage_unexpanded_views(stq) -> bool:
    """A view registered by name only (no SQL body) appears in `unexpanded_views`."""
    with stq.Syntaqlite() as sq:
        schema = stq.Schema(views=[stq.View("v", ["n"])])
        result = sq.validate("SELECT n FROM v", schema)
    if result.lineage is None:
        _fail("lineage_unexpanded_views", "no lineage")
        return False
    if result.lineage.complete:
        _fail("lineage_unexpanded_views", "expected partial lineage for unexpanded view")
        return False
    if result.lineage.unexpanded_views != ["v"]:
        _fail(
            "lineage_unexpanded_views",
            f"expected ['v'], got {result.lineage.unexpanded_views!r}",
        )
        return False
    _pass("lineage_unexpanded_views")
    return True


# ── Lifecycle / public contract ──────────────────────────────────────────────


def _test_context_manager_raises_after_close(stq) -> bool:
    with stq.Syntaqlite() as sq:
        if "SELECT" not in sq.format_sql("select 1"):
            _fail("context_manager_raises_after_close", "format_sql failed before close")
            return False
    try:
        sq.parse("select 1")
    except stq.SyntaqliteError:
        _pass("context_manager_raises_after_close")
        return True
    _fail("context_manager_raises_after_close", "expected SyntaqliteError after close")
    return False


def _test_explicit_close_is_idempotent(stq) -> bool:
    sq = stq.Syntaqlite()
    sq.close()
    sq.close()  # must not raise
    _pass("explicit_close_is_idempotent")
    return True


def _test_parse_returns_wrapped_node_type(stq) -> bool:
    """`parse` returns typed AST nodes; `parse_raw` returns plain dicts."""
    with stq.Syntaqlite() as sq:
        wrapped = sq.parse("SELECT 1")
        raw = sq.parse_raw("SELECT 1")
    if isinstance(wrapped[0], dict):
        _fail("parse_returns_wrapped_node_type", "parse should wrap, not return dict")
        return False
    if not isinstance(raw[0], dict):
        _fail("parse_returns_wrapped_node_type", f"parse_raw should return dict, got {type(raw[0])}")
        return False
    _pass("parse_returns_wrapped_node_type")
    return True


def _test_diagnostic_code_enum_members(stq) -> bool:
    """Public DiagnosticCode enum must expose exactly this set."""
    expected = {
        "PARSE_ERROR",
        "UNKNOWN_TABLE",
        "UNKNOWN_COLUMN",
        "UNKNOWN_FUNCTION",
        "UNKNOWN_MODULE",
        "FUNCTION_ARITY",
        "CTE_COLUMN_COUNT_MISMATCH",
    }
    actual = {m.name for m in stq.DiagnosticCode}
    if actual != expected:
        _fail("diagnostic_code_enum_members", f"expected {expected!r}, got {actual!r}")
        return False
    _pass("diagnostic_code_enum_members")
    return True


def _test_validate_output_enum_values(stq) -> bool:
    """AnalysisOutput must expose STRUCTURED and TEXT as string members."""
    if stq.AnalysisOutput.STRUCTURED.value != "structured":
        _fail("validate_output_enum_values", f"STRUCTURED={stq.AnalysisOutput.STRUCTURED.value!r}")
        return False
    if stq.AnalysisOutput.TEXT.value != "text":
        _fail("validate_output_enum_values", f"TEXT={stq.AnalysisOutput.TEXT.value!r}")
        return False
    _pass("validate_output_enum_values")
    return True


def _test_validate_rejects_string_output(stq) -> bool:
    """`validate` must refuse raw strings for `output`; only ValidateOutput members."""
    with stq.Syntaqlite() as sq:
        try:
            sq.validate("SELECT 1", output="text")
        except TypeError:
            _pass("validate_rejects_string_output")
            return True
    _fail("validate_rejects_string_output", "expected TypeError for output='text'")
    return False


# ── Bundled binary dispatch ──────────────────────────────────────────────────


def _test_get_binary_path_honours_env(stq) -> bool:
    """`get_binary_path()` returns the SYNTAQLITE_BIN override when set."""
    expected = os.environ["SYNTAQLITE_BIN"]
    actual = stq.get_binary_path()
    if actual != expected:
        _fail("get_binary_path_honours_env", f"expected {expected!r}, got {actual!r}")
        return False
    _pass("get_binary_path_honours_env")
    return True


def _test_syntaqlite_binary_kwarg(stq) -> bool:
    """Passing `binary=` to Syntaqlite bypasses `get_binary_path`."""
    bin_path = os.environ["SYNTAQLITE_BIN"]
    original = os.environ.pop("SYNTAQLITE_BIN")
    try:
        with stq.Syntaqlite(binary=bin_path) as sq:
            out = sq.format_sql("select 1")
    finally:
        os.environ["SYNTAQLITE_BIN"] = original
    if "SELECT" not in out:
        _fail("syntaqlite_binary_kwarg", f"format_sql returned {out!r}")
        return False
    _pass("syntaqlite_binary_kwarg")
    return True


def _test_main_entrypoint_via_python_m(stq, ctx: SuiteContext) -> bool:
    """`python -m syntaqlite` invokes `main()` which executes the bundled CLI."""
    env = os.environ.copy()
    env["PYTHONPATH"] = str(ctx.root_dir / "python")
    env["SYNTAQLITE_BIN"] = str(ctx.binary)
    r = subprocess.run(
        [sys.executable, "-m", "syntaqlite", "--version"],
        env=env, capture_output=True, text=True, timeout=10,
    )
    if r.returncode != 0 or "syntaqlite" not in r.stdout:
        _fail(
            "main_entrypoint_via_python_m",
            f"rc={r.returncode} stdout={r.stdout!r} stderr={r.stderr!r}",
        )
        return False
    _pass("main_entrypoint_via_python_m")
    return True


# ── Dialect ──────────────────────────────────────────────────────────────────


_DIALECT_CACHE: dict[str, Path] = {}


def _perfetto_dialect_lib(ctx: SuiteContext, work_dir: Path) -> Path:
    """Compile the Perfetto dialect once per suite run; cache the path."""
    if "perfetto" in _DIALECT_CACHE:
        return _DIALECT_CACHE["perfetto"]
    from python.dev.diff_tests.perfetto_common import _compile_perfetto_dialect
    lib = _compile_perfetto_dialect(ctx.binary, work_dir)
    _DIALECT_CACHE["perfetto"] = lib
    return lib


def _test_dialect_parses_perfetto_syntax(stq, ctx: SuiteContext, work_dir: Path) -> bool:
    """CREATE PERFETTO TABLE is a PARSE_ERROR in SQLite but parses with the dialect."""
    # Baseline: the SQL fails under SQLite.
    with stq.Syntaqlite() as sq:
        r = sq.validate("CREATE PERFETTO TABLE t AS SELECT 1 AS x")
    if not any(d.code is stq.DiagnosticCode.PARSE_ERROR for d in r.diagnostics):
        _fail("dialect_parses_perfetto_syntax", f"SQLite baseline should PARSE_ERROR, got {r.diagnostics!r}")
        return False

    lib = _perfetto_dialect_lib(ctx, work_dir)
    with stq.Syntaqlite(dialect_path=str(lib), dialect_name="perfetto") as sq:
        stmts = sq.parse("CREATE PERFETTO TABLE t AS SELECT 1 AS x")
    if len(stmts) != 1:
        _fail("dialect_parses_perfetto_syntax", f"expected 1 stmt, got {len(stmts)}")
        return False
    _pass("dialect_parses_perfetto_syntax")
    return True


# ── Entry point ──────────────────────────────────────────────────────────────


# Tests that take (stq) — the vast majority.
_TESTS = [
    # Parse
    _test_parse_simple_select,
    _test_parse_cte,
    _test_parse_insert,
    _test_parse_update,
    _test_parse_delete,
    _test_parse_create_table,
    _test_parse_compound_select,
    _test_parse_multi_statement,
    _test_parse_bad_sql_returns_empty,
    _test_parse_raw_returns_dicts,
    # format_sql
    _test_format_keyword_case_upper,
    _test_format_keyword_case_lower,
    _test_format_indent_width,
    _test_format_line_width_wraps,
    _test_format_semicolons_off,
    _test_format_bad_sql_raises,
    # tokenize
    _test_tokenize_keyword_category,
    _test_tokenize_mixed_categories,
    _test_tokenize_offset_and_length_align,
    _test_tokenize_comment_and_operator,
    # validate — diagnostics
    _test_validate_no_schema_unknown_table,
    _test_validate_unknown_column_code,
    _test_validate_unknown_function,
    _test_validate_function_arity,
    _test_validate_cte_column_mismatch,
    _test_validate_parse_error_diagnostic,
    _test_validate_views_resolve,
    _test_validate_ddl_is_parsed,
    _test_validate_diagnostic_fields_present,
    # validate — lineage & output modes
    _test_validate_lineage_basic,
    _test_validate_lineage_union_first_branch,
    _test_validate_lineage_aggregate_drops_origin,
    _test_validate_relations,
    _test_validate_text_output_with_source_name,
    _test_validate_schema_modules_passthrough,
    # validate — per-statement
    _test_validate_statements_breakdown,
    _test_defined_relations_table,
    _test_defined_relations_view,
    _test_column_lineage_index,
    _test_lineage_unexpanded_views,
    # Lifecycle / public contract
    _test_context_manager_raises_after_close,
    _test_explicit_close_is_idempotent,
    _test_parse_returns_wrapped_node_type,
    _test_diagnostic_code_enum_members,
    _test_validate_output_enum_values,
    _test_validate_rejects_string_output,
    # Bundled binary dispatch
    _test_get_binary_path_honours_env,
    _test_syntaqlite_binary_kwarg,
]

# Tests that need the SuiteContext (subprocess invocations).
_CTX_TESTS = [
    _test_main_entrypoint_via_python_m,
]

# Tests that need a shared work_dir (dialect compilation).
_WORK_DIR_TESTS = [
    _test_dialect_parses_perfetto_syntax,
]


def run(ctx: SuiteContext) -> int:
    sys.path.insert(0, str(ctx.root_dir / "python"))
    os.environ["SYNTAQLITE_BIN"] = str(ctx.binary)
    import syntaqlite  # noqa: E402 — must follow sys.path + env setup

    passed = 0
    total = len(_TESTS) + len(_CTX_TESTS) + len(_WORK_DIR_TESTS)

    for test in _TESTS:
        if test(syntaqlite):
            passed += 1
    for test in _CTX_TESTS:
        if test(syntaqlite, ctx):
            passed += 1
    with tempfile.TemporaryDirectory(prefix="synq-python-api-") as work_dir:
        wd = Path(work_dir)
        for test in _WORK_DIR_TESTS:
            if test(syntaqlite, ctx, wd):
                passed += 1

    print(f"\n  {passed}/{total} python-api tests passed.")
    return 0 if passed == total else 1
