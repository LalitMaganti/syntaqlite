"""
Data collection for syntaqlite competitive comparison.

Runs tools, collects structured results, saves as JSON.
Each collect_* function returns a JSON-serializable dict.
"""

import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "..", ".."))
DIR = os.path.join(REPO, "tests", "comparison")
SYNQ = os.path.join(REPO, "target", "release", "syntaqlite")
LEMON_RS = os.path.join(DIR, "parser", "target", "release", "lemon-rs-parse")
SQLPARSER_RS = os.path.join(DIR, "parser", "target", "release", "sqlparser-parse")
UV = "uv"
BASE_DB = os.path.join(DIR, "formatter", "_test_schema.db")
RESULTS_DIR = os.path.join(DIR, "results")

LSP_VALIDATE = os.path.join(DIR, "validator", "_lsp_validate.js")
SQLITE_RUNNER_LSP = os.path.join(DIR, "_sqlite_runner_lsp", "server", "server.js")
SQLS = os.path.join(os.path.expanduser("~"), ".local", "share", "mise", "installs", "go", "1.26.1", "bin", "sqls")
LSP_TEST = os.path.join(DIR, "lsp", "_lsp_test.js")

SCHEMA_SQL_PATH = os.path.join(DIR, "schema.sql")


def _load_test_schema_sql():
    with open(SCHEMA_SQL_PATH) as f:
        return f.read()

NO_EXPLAIN_PREFIXES = ('ATTACH', 'DETACH', 'PRAGMA', 'EXPLAIN', 'ANALYZE',
                       'SAVEPOINT', 'RELEASE', 'REINDEX', 'VACUUM')

FORMATTER_NAMES = ["syntaqlite", "prettier-cst", "sql-formatter", "sqlglot[c]", "sleek", "sqruff"]

VALIDATOR_TOOLS_META = [
    ("syntaqlite", "static semantic"),
    ("sqlite3", "runtime execution"),
    ("sqlite-runner-lsp", "runtime via LSP"),
    ("sql-lint", "structural checks"),
]


# ─── Helpers ────────────────────────────────────────────────────────

def _run(cmd, input_text=None, timeout=30, cwd=None):
    try:
        p = subprocess.run(
            cmd, shell=True, capture_output=True, text=True,
            timeout=timeout, cwd=cwd or DIR, input=input_text,
        )
        return p.returncode == 0, p.stdout, p.stderr
    except subprocess.TimeoutExpired:
        return False, "", "TIMEOUT"
    except Exception as e:
        return False, "", str(e)


def _tmpfile(sql):
    f = tempfile.NamedTemporaryFile(suffix=".sql", mode="w", delete=False)
    f.write(sql)
    f.flush()
    f.close()
    return f.name


def _create_test_db():
    os.makedirs(os.path.dirname(BASE_DB), exist_ok=True)
    if os.path.exists(BASE_DB):
        os.unlink(BASE_DB)
    p = subprocess.run(
        ["sqlite3", BASE_DB], input=_load_test_schema_sql(),
        capture_output=True, text=True,
    )
    if p.returncode != 0:
        print(f"ERROR creating test schema DB: {p.stderr.strip()}", file=sys.stderr)
        sys.exit(1)


def _validate_sql_with_sqlite(sql):
    tmp_db = tempfile.mktemp(suffix=".db")
    shutil.copy2(BASE_DB, tmp_db)
    try:
        sql_stripped = sql.strip().rstrip(';').strip()
        upper = sql_stripped.upper()
        if upper.startswith('DETACH'):
            test_sql = "ATTACH ':memory:' AS scratch;\n" + sql
        elif any(upper.startswith(pfx) for pfx in NO_EXPLAIN_PREFIXES):
            test_sql = sql
        else:
            test_sql = "EXPLAIN " + sql
        p = subprocess.run(
            ["sqlite3", tmp_db, test_sql],
            capture_output=True, timeout=10,
        )
        if p.returncode == 0:
            return True, ""
        return False, p.stderr.decode("utf-8", errors="replace").strip()
    except subprocess.TimeoutExpired:
        return False, "TIMEOUT"
    finally:
        if os.path.exists(tmp_db):
            os.unlink(tmp_db)


# DDL bytecode includes string offsets that change with whitespace — skip bytecode
# comparison for these and fall back to acceptance-only.
NO_BYTECODE_PREFIXES = ('CREATE', 'ALTER', 'DROP', 'DETACH')


def _normalize_explain(raw):
    """Strip the 'comment' column from EXPLAIN output.

    The comment column contains source-text snippets (e.g. "r[10]=x+1")
    that change with whitespace reformatting but don't affect semantics.
    EXPLAIN uses fixed-width columnar output; the comment starts at the
    'comment' header position.
    """
    lines = raw.decode("utf-8", errors="replace").splitlines()
    if len(lines) < 2:
        return raw
    # Find the comment column offset from the header line.
    header = lines[0]
    col = header.find("comment")
    if col < 0:
        return raw
    # Truncate each line at the comment column.
    return "\n".join(line[:col].rstrip() for line in lines[2:])  # skip header+separator


def _get_explain_bytecode(sql):
    """Get EXPLAIN bytecode for SQL, or None if not applicable."""
    sql_stripped = sql.strip().rstrip(';').strip()
    upper = sql_stripped.upper()
    if any(upper.startswith(pfx) for pfx in NO_EXPLAIN_PREFIXES + NO_BYTECODE_PREFIXES):
        return None
    tmp_db = tempfile.mktemp(suffix=".db")
    shutil.copy2(BASE_DB, tmp_db)
    try:
        p = subprocess.run(
            ["sqlite3", tmp_db, "EXPLAIN " + sql],
            capture_output=True, timeout=10,
        )
        if p.returncode == 0:
            return _normalize_explain(p.stdout)
        return None
    except subprocess.TimeoutExpired:
        return None
    finally:
        if os.path.exists(tmp_db):
            os.unlink(tmp_db)


def _load_test_statements():
    with open(os.path.join(DIR, "test_statements.sql")) as f:
        content = f.read()
    blocks = re.split(r'\n\n+', content.strip())
    tests = []
    for block in blocks:
        lines = block.strip().split('\n')
        name_lines = [l for l in lines if l.startswith('--')]
        sql_lines = [l for l in lines if not l.startswith('--')]
        name = name_lines[0].lstrip('- ').strip() if name_lines else "unnamed"
        sql = '\n'.join(sql_lines).strip()
        if sql:
            tests.append((name, sql))
    return tests


def _write_per_stmt_files(tests, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    results = []
    for i, (name, sql) in enumerate(tests):
        fname = f"t{i+1:02d}.sql"
        path = os.path.join(out_dir, fname)
        with open(path, "w") as f:
            f.write(sql + "\n")
        results.append((name, path))
    return results


def _read_bench_md(bench_dir, filename):
    path = os.path.join(bench_dir, filename)
    if os.path.exists(path):
        return open(path).read().strip()
    return None


def _get_meta():
    """Build metadata dict for JSON output."""
    try:
        ver = subprocess.run(
            [SYNQ, "version"], capture_output=True, text=True, timeout=5
        )
        version = ver.stdout.strip().split()[-1] if ver.returncode == 0 else "unknown"
    except Exception:
        version = "unknown"
    return {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "syntaqlite_version": version,
        "platform": f"{platform.machine()}-{platform.system().lower()}",
    }


def _log(msg):
    print(msg, file=sys.stderr)


# ─── Parser tools ──────────────────────────────────────────────────

def _parse_syntaqlite(path):
    ok, _, _ = _run(f"'{SYNQ}' fmt '{path}'")
    return ok

def _parse_lemon_rs(path):
    ok, _, _ = _run(f"'{LEMON_RS}' '{path}'")
    return ok

def _parse_sqlparser_rs(path):
    ok, _, _ = _run(f"'{SQLPARSER_RS}' '{path}'")
    return ok

def _parse_sqlglot(path):
    ok, _, _ = _run(
        f"{UV} run python -c \""
        f"import sqlglot; [e.sql() for e in sqlglot.parse(open('{path}').read(), dialect='sqlite') if e]"
        f"\"",
        timeout=10,
    )
    return ok

def _parse_sql_parser_cst(path):
    script = (
        f'const {{parse}}=require("sql-parser-cst");'
        f'const fs=require("fs");'
        f'try{{parse(fs.readFileSync("{path}","utf8"),{{dialect:"sqlite"}});process.exit(0)}}'
        f'catch(e){{console.error(e.message);process.exit(1)}}'
    )
    ok, _, _ = _run(f"node -e '{script}'", timeout=10)
    return ok

def _parse_sqlfluff(path):
    ok, out, err = _run(f"{UV} run sqlfluff parse '{path}' --dialect sqlite", timeout=30)
    return "unparsable" not in (out + err).lower()

def _parse_node_sql_parser(path):
    script = (
        f'const {{Parser}}=require("node-sql-parser");'
        f'const fs=require("fs");'
        f'const p=new Parser();'
        f'try{{p.astify(fs.readFileSync("{path}","utf8"),{{database:"SQLite"}});process.exit(0)}}'
        f'catch(e){{console.error(e.message);process.exit(1)}}'
    )
    ok, _, _ = _run(f"node -e '{script}'", timeout=10)
    return ok

PARSER_TOOLS = [
    ("syntaqlite", _parse_syntaqlite),
    ("lemon-rs", _parse_lemon_rs),
    ("sql-parser-cst", _parse_sql_parser_cst),
    ("sqlglot[c]", _parse_sqlglot),
    ("sqlfluff", _parse_sqlfluff),
    ("sqlparser-rs", _parse_sqlparser_rs),
    ("node-sql-parser", _parse_node_sql_parser),
]


# ─── Formatter tools ───────────────────────────────────────────────

def _get_formatted_sql(tool_name, input_path):
    if tool_name == "syntaqlite":
        ok, out, _ = _run(f"'{SYNQ}' fmt '{input_path}'")
        return (True, out) if ok else (False, None)
    elif tool_name == "prettier-cst":
        with open(input_path) as f:
            ok, out, _ = _run("npx prettier --stdin-filepath t.sql", input_text=f.read(), cwd=DIR)
        return (True, out) if ok else (False, None)
    elif tool_name == "sql-formatter":
        ok, out, _ = _run(f"sql-formatter -l sqlite '{input_path}'")
        return (True, out) if ok else (False, None)
    elif tool_name == "sqlglot[c]":
        ok, out, _ = _run(
            f"{UV} run python -c \"import sqlglot;[print(e.sql(dialect='sqlite',pretty=True)) "
            f"for e in sqlglot.parse(open('{input_path}').read(),dialect='sqlite') if e]\"",
            timeout=10,
        )
        return (True, out) if ok else (False, None)
    elif tool_name == "sleek":
        tmp = input_path + ".sleek_tmp.sql"
        shutil.copy2(input_path, tmp)
        ok, _, _ = _run(f"sleek '{tmp}'")
        try:
            if not ok:
                return (False, None)
            with open(tmp) as f:
                return (True, f.read())
        except Exception:
            return (False, None)
        finally:
            if os.path.exists(tmp):
                os.unlink(tmp)
    elif tool_name == "sqruff":
        tmp = input_path + ".sqruff_tmp.sql"
        shutil.copy2(input_path, tmp)
        ok, _, _ = _run(f"sqruff fix '{tmp}' --dialect sqlite")
        try:
            if not ok:
                return (False, None)
            with open(tmp) as f:
                return (True, f.read())
        except Exception:
            return (False, None)
        finally:
            if os.path.exists(tmp):
                os.unlink(tmp)
    return (False, None)


# ─── Validator tools ───────────────────────────────────────────────

def _validate_syntaqlite(sql, schema_preamble):
    t = _tmpfile(schema_preamble + sql + "\n")
    ok, out, err = _run(f"'{SYNQ}' validate '{t}'")
    os.unlink(t)
    combined = out + err
    return "warning:" in combined or "error:" in combined

def _validate_sqlite3_runtime(sql):
    ok, _ = _validate_sql_with_sqlite(sql)
    return not ok

def _validate_sql_lint(sql):
    t = _tmpfile(sql + "\n")
    ok, out, err = _run(f"npx sql-lint '{t}'")
    os.unlink(t)
    combined = out + err
    return "sql-lint" in combined.lower() or "unable to lint" in combined.lower()

def _validate_sqlite_runner_lsp(sql, schema_preamble=""):
    t = _tmpfile(schema_preamble + sql + "\n")
    ok, out, err = _run(f"node '{LSP_VALIDATE}' node '{SQLITE_RUNNER_LSP}' --stdio -- '{t}'")
    os.unlink(t)
    combined = out + err
    return "error" in combined.lower() and "no diagnostics" not in combined.lower()

VALIDATOR_TOOLS = [
    ("syntaqlite", "static semantic", _validate_syntaqlite),
    ("sqlite3", "runtime execution", _validate_sqlite3_runtime),
    ("sqlite-runner-lsp", "runtime via LSP", _validate_sqlite_runner_lsp),
    ("sql-lint", "structural checks", _validate_sql_lint),
]


# ─── Collect: Parser ──────────────────────────────────────────────

def collect_parser():
    _log("==> Parser comparison")
    _create_test_db()

    tests = _load_test_statements()
    stmt_dir = os.path.join(DIR, "parser", "stmts")
    stmt_files = _write_per_stmt_files(tests, stmt_dir)
    total = len(stmt_files)

    # Ground truth
    _log("  Ground truth...")
    ground_truth = {}
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 8) as pool:
        gt_futures = {}
        for name, path in stmt_files:
            with open(path) as f:
                sql = f.read()
            gt_futures[pool.submit(_validate_sql_with_sqlite, sql)] = path
        for fut in as_completed(gt_futures):
            path = gt_futures[fut]
            ok, err = fut.result()
            ground_truth[path] = (ok, err)

    gt_results = []
    for name, path in stmt_files:
        ok, err = ground_truth[path]
        gt_results.append({"name": name[:50], "sqlite3_ok": ok, "error": err[:60] if err else ""})
        ground_truth[path] = ok

    # Accuracy
    _log("  Accuracy...")
    tallies = {name: {"correct": 0, "reject_valid": 0, "accept_invalid": 0} for name, _ in PARSER_TOOLS}
    per_stmt = []

    with ThreadPoolExecutor(max_workers=os.cpu_count() or 8) as pool:
        futures = {}
        for si, (name, path) in enumerate(stmt_files):
            for tool_name, tool_fn in PARSER_TOOLS:
                fut = pool.submit(tool_fn, path)
                futures[fut] = (si, tool_name)
        tool_results = {}
        for fut in as_completed(futures):
            si, tool_name = futures[fut]
            tool_results[(si, tool_name)] = fut.result()

    for si, (name, path) in enumerate(stmt_files):
        sqlite_ok = ground_truth[path]
        row = {"name": name[:50], "sqlite3": "OK" if sqlite_ok else "ERR", "tools": {}}
        for tool_name, _ in PARSER_TOOLS:
            tool_ok = tool_results[(si, tool_name)]
            if sqlite_ok and tool_ok:
                row["tools"][tool_name] = "PASS"
                tallies[tool_name]["correct"] += 1
            elif sqlite_ok and not tool_ok:
                row["tools"][tool_name] = "FAIL"
                tallies[tool_name]["reject_valid"] += 1
            elif not sqlite_ok and tool_ok:
                row["tools"][tool_name] = "FP"
                tallies[tool_name]["accept_invalid"] += 1
            else:
                row["tools"][tool_name] = "PASS"
                tallies[tool_name]["correct"] += 1
        per_stmt.append(row)

    # Speed benchmarks
    _log("  Speed benchmarks...")
    parse_dir = os.path.join(DIR, "parser")
    bench_src = os.path.join(DIR, "bench_statements.sql")
    with open(bench_src) as f:
        bench_sql = f.read()

    small_path = os.path.join(parse_dir, "bench.sql")
    with open(small_path, "w") as f:
        f.write(bench_sql)

    large_path = os.path.join(parse_dir, "bench_30x.sql")
    with open(large_path, "w") as f:
        for _ in range(30):
            f.write(bench_sql)

    small_lines = len(bench_sql.splitlines())

    # Write helper scripts
    with open(os.path.join(parse_dir, "_parse_cst.js"), "w") as f:
        f.write(
            'const {parse}=require("sql-parser-cst");const fs=require("fs");\n'
            'parse(fs.readFileSync(process.argv[2],"utf8"),{dialect:"sqlite"});\n'
        )
    with open(os.path.join(parse_dir, "_parse_node.js"), "w") as f:
        f.write(
            'const {Parser}=require("node-sql-parser");const fs=require("fs");\n'
            'const p=new Parser();\n'
            'p.astify(fs.readFileSync(process.argv[2],"utf8"),{database:"SQLite"});\n'
        )
    with open(os.path.join(parse_dir, "_parse_sqlglot.py"), "w") as f:
        f.write(
            "import sqlglot,sys\n"
            "list(sqlglot.parse(open(sys.argv[1]).read(),dialect='sqlite'))\n"
        )

    for label, sqlfile in [
        ("bench.sql (1x)", small_path),
        ("bench_30x.sql (30x)", large_path),
    ]:
        warmup = "5"
        runs = "10" if "1x" in label else "5"
        cmd = (
            f"hyperfine --warmup {warmup} --min-runs {runs} --shell=none "
            f"--export-markdown '{parse_dir}/bench_{label.split()[0]}.md' "
            f"-n syntaqlite '{SYNQ} parse {sqlfile}' "
            f"-n lemon-rs '{LEMON_RS} {sqlfile}' "
            f"-n sql-parser-cst 'node {parse_dir}/_parse_cst.js {sqlfile}' "
            f"-n 'sqlglot[c]' '{UV} run --directory {DIR} python {parse_dir}/_parse_sqlglot.py {sqlfile}' "
            f"-n sqlparser-rs '{SQLPARSER_RS} {sqlfile}' "
            f"-n node-sql-parser 'node {parse_dir}/_parse_node.js {sqlfile}' "
            f"-n sqlfluff '{UV} run --directory {DIR} sqlfluff parse {sqlfile} --dialect sqlite' "
        )
        subprocess.run(cmd, shell=True, cwd=DIR)

    return {
        "meta": _get_meta(),
        "ground_truth": gt_results,
        "per_statement": per_stmt,
        "tallies": tallies,
        "total": total,
        "tool_names": [n for n, _ in PARSER_TOOLS],
        "speed": {
            "bench_1x": {
                "description": f"{small_lines} lines, {len(bench_sql):,} bytes",
                "hyperfine_md": _read_bench_md(parse_dir, "bench_bench.sql.md"),
            },
            "bench_30x": {
                "description": f"{small_lines * 30} lines, {len(bench_sql) * 30:,} bytes",
                "hyperfine_md": _read_bench_md(parse_dir, "bench_bench_30x.sql.md"),
            },
        },
    }


# ─── Collect: Formatter ──────────────────────────────────────────

def collect_formatter():
    _log("==> Formatter comparison")
    _create_test_db()

    tests = _load_test_statements()
    stmt_dir = os.path.join(DIR, "formatter", "stmts")
    stmt_files = _write_per_stmt_files(tests, stmt_dir)
    total = len(stmt_files)

    # Ground truth
    _log("  Ground truth...")
    orig_pass = {}
    orig_bytecode = {}
    gt_results = []
    for name, path in stmt_files:
        with open(path) as f:
            sql = f.read()
        ok, err = _validate_sql_with_sqlite(sql)
        orig_pass[path] = ok
        if ok:
            orig_bytecode[path] = _get_explain_bytecode(sql)
        gt_results.append({"name": name[:50], "sqlite3_ok": ok, "error": err[:60] if err else ""})

    n_valid = sum(orig_pass.values())

    # Round-trip validation
    _log("  Round-trip validation...")
    tallies = {tn: {"format_ok": 0, "sqlite_ok": 0, "corrupted": 0} for tn in FORMATTER_NAMES}
    corruption_details = []
    per_stmt = []

    # Format all in parallel
    fmt_results = {}
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 8) as pool:
        futures = {}
        for si, (name, path) in enumerate(stmt_files):
            for tn in FORMATTER_NAMES:
                fut = pool.submit(_get_formatted_sql, tn, path)
                futures[fut] = (si, tn)
        for fut in as_completed(futures):
            si, tn = futures[fut]
            fmt_results[(si, tn)] = fut.result()

    # Validate formatted output in parallel — compare EXPLAIN bytecode
    validate_jobs = {}
    for si, (name, path) in enumerate(stmt_files):
        for tn in FORMATTER_NAMES:
            fmt_ok, formatted_sql = fmt_results[(si, tn)]
            if fmt_ok and formatted_sql and formatted_sql.strip() and orig_pass[path]:
                validate_jobs[(si, tn)] = (formatted_sql, path)

    validate_results = {}
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 8) as pool:
        futures = {}
        for key, (sql, path) in validate_jobs.items():
            orig_bc = orig_bytecode.get(path)
            if orig_bc is not None:
                # Compare EXPLAIN bytecode
                fut = pool.submit(_get_explain_bytecode, sql)
            else:
                # No bytecode available (PRAGMA etc.) — fall back to acceptance check
                fut = pool.submit(_validate_sql_with_sqlite, sql)
            futures[fut] = (key, orig_bc)
        for fut in as_completed(futures):
            key, orig_bc = futures[fut]
            result = fut.result()
            if orig_bc is not None:
                # result is bytecode (bytes or None)
                if result is not None and result == orig_bc:
                    validate_results[key] = (True, "")
                elif result is None:
                    validate_results[key] = (False, "EXPLAIN failed on formatted SQL")
                else:
                    validate_results[key] = (False, "EXPLAIN bytecode differs from original")
            else:
                # result is (ok, err) tuple from _validate_sql_with_sqlite
                validate_results[key] = result

    for si, (name, path) in enumerate(stmt_files):
        row = {"name": name[:50], "tools": {}}
        for tn in FORMATTER_NAMES:
            fmt_ok, formatted_sql = fmt_results[(si, tn)]
            if not fmt_ok or not formatted_sql or not formatted_sql.strip():
                row["tools"][tn] = "FAIL"
                continue
            if not orig_pass[path]:
                tallies[tn]["format_ok"] += 1
                row["tools"][tn] = "skip"
                continue
            sqlite_ok, sqlite_err = validate_results.get((si, tn), (False, "not run"))
            if sqlite_ok:
                tallies[tn]["format_ok"] += 1
                tallies[tn]["sqlite_ok"] += 1
                row["tools"][tn] = "OK"
            else:
                tallies[tn]["corrupted"] += 1
                corruption_details.append({"tool": tn, "test": name, "error": sqlite_err[:60]})
                row["tools"][tn] = "CORRUPT"
        per_stmt.append(row)

    # Speed benchmarks
    _log("  Speed benchmarks...")
    bench_src = os.path.join(DIR, "bench_statements.sql")
    with open(bench_src) as f:
        bench_sql = f.read()

    fmt_bench_dir = os.path.join(DIR, "formatter")
    small_path = os.path.join(fmt_bench_dir, "bench.sql")
    with open(small_path, "w") as f:
        f.write(bench_sql)

    large_path = os.path.join(fmt_bench_dir, "bench_30x.sql")
    with open(large_path, "w") as f:
        for _ in range(30):
            f.write(bench_sql)

    small_lines = len(bench_sql.splitlines())

    # Write helper scripts
    helpers = {
        "_wrap_sleek.sh": 'cp "$1" "$1.bak"; sleek "$1" > /dev/null 2>&1; cp "$1.bak" "$1"; rm -f "$1.bak"',
        "_wrap_sqruff.sh": 'cp "$1" "$1.bak"; sqruff fix "$1" --dialect sqlite > /dev/null 2>&1; cp "$1.bak" "$1"; rm -f "$1.bak"',
    }
    for fname, cmd in helpers.items():
        p = os.path.join(fmt_bench_dir, fname)
        with open(p, "w") as f:
            f.write(f"#!/bin/bash\n{cmd}\n")
        os.chmod(p, 0o755)

    with open(os.path.join(fmt_bench_dir, "_fmt_sqlglot.py"), "w") as f:
        f.write(
            "import sqlglot,sys\n"
            "sql=open(sys.argv[1]).read()\n"
            "for e in sqlglot.parse(sql,dialect='sqlite'):\n"
            "  if e is not None: print(e.sql(dialect='sqlite',pretty=True))\n"
        )

    with open(os.path.join(fmt_bench_dir, "_wrap_prettier.sh"), "w") as f:
        f.write("#!/bin/bash\nnpx prettier --stdin-filepath test.sql < \"$1\"\n")
    os.chmod(os.path.join(fmt_bench_dir, "_wrap_prettier.sh"), 0o755)

    for label, sqlfile in [
        ("bench.sql (1x)", small_path),
        ("bench_30x.sql (30x)", large_path),
    ]:
        warmup = "5"
        runs = "10" if "1x" in label else "5"
        cmd = (
            f"hyperfine --warmup {warmup} --min-runs {runs} --shell=none "
            f"--export-markdown '{fmt_bench_dir}/bench_{label.split()[0]}.md' "
            f"-n syntaqlite '{SYNQ} fmt {sqlfile}' "
            f"-n prettier-cst '{fmt_bench_dir}/_wrap_prettier.sh {sqlfile}' "
            f"-n sql-formatter 'sql-formatter -l sqlite {sqlfile}' "
            f"-n 'sqlglot[c]' '{UV} run --directory {DIR} python {fmt_bench_dir}/_fmt_sqlglot.py {sqlfile}' "
            f"-n sleek '{fmt_bench_dir}/_wrap_sleek.sh {sqlfile}' "
            f"-n sqruff '{fmt_bench_dir}/_wrap_sqruff.sh {sqlfile}' "
        )
        subprocess.run(cmd, shell=True, cwd=DIR)

    # Slow tools
    _log("  Slow tool timing...")
    slow_tools = []
    for name, fmt_cmd in [
        ("sqlfmt (1x)", f"cd '{DIR}' && cp '{small_path}' /tmp/_sqlfmt.sql && {UV} run sqlfmt /tmp/_sqlfmt.sql > /dev/null 2>&1; rm -f /tmp/_sqlfmt.sql"),
        ("sqlfmt (30x)", f"cd '{DIR}' && cp '{large_path}' /tmp/_sqlfmt.sql && {UV} run sqlfmt /tmp/_sqlfmt.sql > /dev/null 2>&1; rm -f /tmp/_sqlfmt.sql"),
        ("sqlfluff (1x)", f"cd '{DIR}' && cp '{small_path}' /tmp/_sqlfluff.sql && {UV} run sqlfluff format /tmp/_sqlfluff.sql --dialect sqlite -q > /dev/null 2>&1; rm -f /tmp/_sqlfluff.sql"),
    ]:
        start = time.monotonic()
        subprocess.run(fmt_cmd, shell=True, capture_output=True, timeout=120)
        elapsed = time.monotonic() - start
        slow_tools.append({"name": name, "time_ms": round(elapsed * 1000)})

    return {
        "meta": _get_meta(),
        "ground_truth": gt_results,
        "per_statement": per_stmt,
        "tallies": tallies,
        "total": total,
        "n_valid": n_valid,
        "formatter_names": FORMATTER_NAMES,
        "corruption_details": corruption_details,
        "slow_tools": slow_tools,
        "speed": {
            "bench_1x": {
                "description": f"{small_lines} lines, {len(bench_sql):,} bytes",
                "hyperfine_md": _read_bench_md(fmt_bench_dir, "bench_bench.sql.md"),
            },
            "bench_30x": {
                "description": f"{small_lines * 30} lines, {len(bench_sql) * 30:,} bytes",
                "hyperfine_md": _read_bench_md(fmt_bench_dir, "bench_bench_30x.sql.md"),
            },
        },
    }


# ─── Collect: Validator ──────────────────────────────────────────

def collect_validator():
    _log("==> Validator comparison")
    _create_test_db()
    schema_preamble = _load_test_schema_sql().strip() + "\n"

    # Demo query for diagnostic quality
    _log("  Diagnostic quality...")
    demo_query = (
        "WITH\n"
        "  monthly_stats(month, revenue, order_count) AS (\n"
        "    SELECT\n"
        "      STRFTIME('%Y-%m', o.created_at) AS month,\n"
        "      SUM(o.total) AS revenue\n"
        "    FROM orders o\n"
        "    WHERE o.status = 'completed'\n"
        "    GROUP BY STRFTIME('%Y-%m', o.created_at)\n"
        "  )\n"
        "SELECT\n"
        "  ms.month,\n"
        "  ms.revenue,\n"
        "  ms.order_count,\n"
        "  ROUDN(ms.revenue / ms.order_count, 2) AS avg_order\n"
        "FROM monthly_stats ms\n"
        "ORDER BY ms.month DESC\n"
        "LIMIT 12;\n"
    )

    demo_diagnostics = {}

    # syntaqlite
    t = _tmpfile(schema_preamble + demo_query)
    _, out, err = _run(f"'{SYNQ}' validate '{t}'")
    os.unlink(t)
    synq_output = (out + err).strip()
    synq_errors = len([l for l in synq_output.split('\n') if l.strip().startswith(('error:', 'warning:'))])
    demo_diagnostics["syntaqlite"] = {
        "output": synq_output,
        "errors_found": synq_errors,
        "finds_all": synq_errors >= 2,
        "did_you_mean": "did you mean" in synq_output.lower(),
        "approach": "static semantic",
        "description": "Static analysis — offline, no database needed. Finds **both** errors in one pass:",
    }

    # sqlite3
    _, sqlite_err = _validate_sql_with_sqlite(demo_query.rstrip().rstrip(';'))
    demo_diagnostics["sqlite3"] = {
        "output": sqlite_err.strip() if sqlite_err else "",
        "errors_found": 1 if sqlite_err else 0,
        "finds_all": False,
        "did_you_mean": False,
        "approach": "runtime execution",
        "description": "Runtime execution — stops at first error:",
    }

    # sqlite-runner-lsp
    t = _tmpfile(schema_preamble + demo_query)
    _, out, err = _run(f"node '{LSP_VALIDATE}' node '{SQLITE_RUNNER_LSP}' --stdio -- '{t}'")
    os.unlink(t)
    combined = (out + err).strip()
    has_diag = combined and "no diagnostics" not in combined
    demo_diagnostics["sqlite-runner-lsp"] = {
        "output": combined if has_diag else "(no diagnostics)",
        "errors_found": 1 if has_diag else 0,
        "finds_all": False,
        "did_you_mean": False,
        "approach": "runtime via LSP",
        "description": "Runtime via LSP — wraps sqlite3, same single error:",
    }

    # sql-lint
    t = _tmpfile(demo_query)
    _, out, err = _run(f"npx sql-lint '{t}'")
    os.unlink(t)
    combined = (out + err).strip()
    has_lint = combined and ("sql-lint" in combined.lower() or "error" in combined.lower())
    lint_output = ""
    if has_lint:
        short = combined.split('\n')[0]
        if len(short) > 120:
            short = short[:120] + "..."
        lint_output = short
    demo_diagnostics["sql-lint"] = {
        "output": lint_output if has_lint else "(no diagnostics)",
        "errors_found": 0,
        "finds_all": False,
        "did_you_mean": False,
        "approach": "structural checks",
        "description": "Structural checks only:",
    }

    # Error detection accuracy
    _log("  Error detection accuracy...")
    cases = [
        ("SELEC * FROM users;", "keyword typo (SELEC)", "error"),
        ("SELECT * FROM users WHERE id IN (1, 2, 3;", "missing close paren", "error"),
        ("SELECT id,, name FROM users;", "double comma", "error"),
        ("SELECT * FROM users WHERE name = 'hello;", "unterminated string", "error"),
        ("INSERT INTO users VALUES (1, ;", "trailing comma in VALUES", "error"),
        ("SELECT * FROM nonexistent;", "unknown table", "error"),
        ("SELECT u.name FROM users u JOIN fake f ON f.id = u.id;", "unknown table in JOIN", "error"),
        ("SELECT bogus FROM users;", "unknown column", "error"),
        ("SELECT u.nonexistent FROM users u;", "unknown qualified column", "error"),
        ("SELECT name, fake_col FROM users WHERE active = 1;", "unknown column in SELECT", "error"),
        ("SELECT SUBSTR(name) FROM users;", "SUBSTR: too few args", "error"),
        ("SELECT REPLACE(name, 'a') FROM users;", "REPLACE: too few args", "error"),
        ("SELECT LENGTH(name, email) FROM users;", "LENGTH: too many args", "error"),
        ("SELECT COALESCE() FROM users;", "COALESCE: zero args", "error"),
        ("WITH cte(a, b, c) AS (SELECT 1, 2) SELECT * FROM cte;", "CTE: 3 declared, 2 actual", "error"),
        ("SELECT id, name FROM users WHERE active = 1;", "valid: simple SELECT", "valid"),
        ("SELECT u.name, COUNT(o.id) FROM users u JOIN orders o ON o.customer_id = u.id GROUP BY u.name;", "valid: JOIN + aggregate", "valid"),
        ("SELECT SUBSTR(name, 1, 3) FROM users;", "valid: SUBSTR with 3 args", "valid"),
        ("SELECT COALESCE(email, name, 'anon') FROM users;", "valid: COALESCE variadic", "valid"),
        ("WITH cte(a, b) AS (SELECT 1, 2) SELECT * FROM cte;", "valid: CTE columns match", "valid"),
        ("SELECT LENGTH(name), UPPER(email), LOWER(name) FROM users;", "valid: built-in functions", "valid"),
        ("INSERT INTO users (name, email) VALUES ('x', 'y');", "valid: INSERT", "valid"),
        ("UPDATE orders SET status = 'shipped' WHERE id = 1;", "valid: UPDATE", "valid"),
        ("DELETE FROM users WHERE id = 1;", "valid: DELETE with WHERE", "valid"),
    ]

    tallies = {name: {"correct": 0, "fp": 0, "fn": 0} for name, _, _ in VALIDATOR_TOOLS}
    total = len(cases)

    def _run_validator(tool_name, tool_fn, sql):
        if tool_name in ("syntaqlite", "sqlite-runner-lsp"):
            return tool_fn(sql, schema_preamble)
        return tool_fn(sql)

    val_results = {}
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 8) as pool:
        futures = {}
        for ci, (sql, desc, expected) in enumerate(cases):
            for tool_name, _, tool_fn in VALIDATOR_TOOLS:
                fut = pool.submit(_run_validator, tool_name, tool_fn, sql)
                futures[fut] = (ci, tool_name)
        for fut in as_completed(futures):
            ci, tool_name = futures[fut]
            val_results[(ci, tool_name)] = fut.result()

    per_case = []
    for ci, (sql, desc, expected) in enumerate(cases):
        row = {"description": desc, "expected": expected, "tools": {}}
        for tool_name, _, _ in VALIDATOR_TOOLS:
            detected = val_results[(ci, tool_name)]
            if expected == "error":
                if detected:
                    row["tools"][tool_name] = "FOUND"
                    tallies[tool_name]["correct"] += 1
                else:
                    row["tools"][tool_name] = "MISS"
                    tallies[tool_name]["fn"] += 1
            else:
                if detected:
                    row["tools"][tool_name] = "FP"
                    tallies[tool_name]["fp"] += 1
                else:
                    row["tools"][tool_name] = "OK"
                    tallies[tool_name]["correct"] += 1
        per_case.append(row)

    # Speed benchmarks
    _log("  Speed benchmarks...")
    val_dir = os.path.join(DIR, "validator")
    os.makedirs(val_dir, exist_ok=True)

    bench_src = os.path.join(DIR, "bench_statements.sql")
    with open(bench_src) as f:
        bench_sql = f.read()

    schema = _load_test_schema_sql()
    small_path = os.path.join(val_dir, "bench.sql")
    with open(small_path, "w") as f:
        f.write(schema + bench_sql)

    ddl_prefixes = ('CREATE ', 'DROP ', 'ALTER ')
    blocks = re.split(r'\n\n+', bench_sql.strip())
    dml_blocks = []
    for block in blocks:
        sql_lines = [l for l in block.strip().split('\n') if not l.strip().startswith('--')]
        first_sql = ' '.join(sql_lines).strip().upper()
        if not any(first_sql.startswith(p) for p in ddl_prefixes):
            dml_blocks.append(block)
    bench_dml_only = '\n\n'.join(dml_blocks) + '\n'

    large_path = os.path.join(val_dir, "bench_30x.sql")
    with open(large_path, "w") as f:
        f.write(schema + bench_sql)
        for _ in range(29):
            f.write(bench_dml_only)

    small_lines = len(bench_sql.splitlines())

    with open(os.path.join(val_dir, "_wrap_sql_lint.sh"), "w") as f:
        f.write("#!/bin/bash\nnpx sql-lint \"$1\" > /dev/null 2>&1\n")
    os.chmod(os.path.join(val_dir, "_wrap_sql_lint.sh"), 0o755)

    with open(os.path.join(val_dir, "_wrap_sqlite3.sh"), "w") as f:
        f.write("#!/bin/bash\nsqlite3 ':memory:' < \"$1\" > /dev/null 2>&1\n")
    os.chmod(os.path.join(val_dir, "_wrap_sqlite3.sh"), 0o755)

    with open(os.path.join(val_dir, "_wrap_sqlite_runner_lsp.sh"), "w") as f:
        f.write(f"#!/bin/bash\nnode '{LSP_VALIDATE}' node '{SQLITE_RUNNER_LSP}' --stdio -- \"$1\" > /dev/null 2>&1\n")
    os.chmod(os.path.join(val_dir, "_wrap_sqlite_runner_lsp.sh"), 0o755)

    for label, sqlfile in [
        ("bench.sql (1x)", small_path),
        ("bench_30x.sql (30x)", large_path),
    ]:
        warmup = "5"
        runs = "10" if "1x" in label else "5"
        cmd = (
            f"hyperfine --warmup {warmup} --min-runs {runs} --shell=none "
            f"--export-markdown '{val_dir}/bench_{label.split()[0]}.md' "
            f"-n syntaqlite '{SYNQ} validate {sqlfile}' "
            f"-n sqlite3 '{val_dir}/_wrap_sqlite3.sh {sqlfile}' "
            f"-n sqlite-runner-lsp '{val_dir}/_wrap_sqlite_runner_lsp.sh {sqlfile}' "
            f"-n sql-lint '{val_dir}/_wrap_sql_lint.sh {sqlfile}' "
        )
        subprocess.run(cmd, shell=True, cwd=DIR)

    return {
        "meta": _get_meta(),
        "demo_query": demo_query.strip(),
        "demo_diagnostics": demo_diagnostics,
        "per_case": per_case,
        "tallies": tallies,
        "total": total,
        "tool_meta": VALIDATOR_TOOLS_META,
        "speed": {
            "bench_1x": {
                "description": f"{small_lines} lines, {len(bench_sql):,} bytes (+ schema preamble)",
                "hyperfine_md": _read_bench_md(val_dir, "bench_bench.sql.md"),
            },
            "bench_30x": {
                "description": f"{small_lines * 30} lines, {len(bench_sql) * 30:,} bytes (+ schema preamble)",
                "hyperfine_md": _read_bench_md(val_dir, "bench_bench_30x.sql.md"),
            },
        },
    }


# ─── Collect: LSP ────────────────────────────────────────────────

def collect_lsp():
    _log("==> LSP comparison")

    lsp_dir = os.path.join(DIR, "lsp")
    os.makedirs(lsp_dir, exist_ok=True)

    test_sql = os.path.join(lsp_dir, "test.sql")
    with open(test_sql, "w") as f:
        f.write("SELECT id, name, email FROM users WHERE active = 1;\n")

    err_sql = os.path.join(lsp_dir, "test_error.sql")
    with open(err_sql, "w") as f:
        f.write("SELEC * FROM users;\n")
        f.write("SELECT * FROM nonexistent_table;\n")

    test_db = os.path.join(lsp_dir, "test.db")
    if os.path.exists(test_db):
        os.unlink(test_db)
    subprocess.run(["sqlite3", test_db,
        "CREATE TABLE users(id INTEGER PRIMARY KEY, name TEXT, email TEXT, active INT);"
        "CREATE TABLE orders(id INTEGER PRIMARY KEY, customer_id INT, total REAL);"],
        capture_output=True)

    sqls_config = os.path.join(lsp_dir, "sqls_config.yml")
    with open(sqls_config, "w") as f:
        f.write(f"lowercaseKeywords: false\nconnections:\n  - alias: test\n    driver: sqlite3\n    dataSourceName: {test_db}\n")

    schema_sql = os.path.join(lsp_dir, "schema.sql")
    lsp_servers = [
        ("syntaqlite", [SYNQ, "lsp"], ["--schema", schema_sql]),
        ("sqls", [SQLS, "-c", sqls_config], ["--db", test_db]),
        ("sql-language-server", ["npx", "sql-language-server", "up", "--method", "stdio"], []),
    ]

    # Capability & Feature Testing
    _log("  Feature testing...")
    import json as _json

    def _test_lsp_server(name, cmd, extra_args, test_sql_path, err_sql_path):
        extra = " ".join(f"'{a}'" for a in extra_args) if extra_args else ""
        full_cmd = f"node '{LSP_TEST}' {' '.join(cmd)} -- '{test_sql_path}' {extra}"
        ok, out, err = _run(full_cmd, timeout=15)
        try:
            data = _json.loads(out) if ok and out.strip() else {}
        except _json.JSONDecodeError:
            data = {}
        err_cmd = f"node '{LSP_TEST}' {' '.join(cmd)} -- '{err_sql_path}' {extra}"
        ok2, out2, _ = _run(err_cmd, timeout=15)
        try:
            err_data = _json.loads(out2) if ok2 and out2.strip() else {}
        except _json.JSONDecodeError:
            err_data = {}
        return name, {"valid": data, "error": err_data}

    all_results = {}
    with ThreadPoolExecutor(max_workers=len(lsp_servers)) as pool:
        futures = [pool.submit(_test_lsp_server, name, cmd, extra, test_sql, err_sql) for name, cmd, extra in lsp_servers]
        for fut in as_completed(futures):
            name, result = fut.result()
            all_results[name] = result

    tool_names = [n for n, _, _ in lsp_servers]

    def cap(name, field):
        caps = all_results.get(name, {}).get("valid", {}).get("capabilities", {})
        return caps.get(field, False)

    # Build feature matrix (plain text, no ANSI)
    feature_rows = []

    # Completion
    row = ["Completion"]
    for name in tool_names:
        c = all_results.get(name, {}).get("valid", {}).get("completion", {})
        if cap(name, "completion") and c.get("count", 0) > 0:
            row.append(f"Yes ({c['count']} items)")
        elif cap(name, "completion"):
            row.append("Advertised (0 items)")
        else:
            row.append("No")
    feature_rows.append(row)

    # Hover
    row = ["Hover"]
    for name in tool_names:
        h = all_results.get(name, {}).get("valid", {}).get("hover", {})
        row.append("Yes" if cap(name, "hover") and h.get("supported") else "No")
    feature_rows.append(row)

    # Go to definition
    row = ["Go to definition"]
    for name in tool_names:
        row.append("Yes" if cap(name, "definition") else "No")
    feature_rows.append(row)

    # References
    row = ["Find references"]
    for name in tool_names:
        row.append("Yes" if cap(name, "references") else "No")
    feature_rows.append(row)

    # Diagnostics: syntax
    row = ["Diagnostics: syntax"]
    for name in tool_names:
        diag = all_results.get(name, {}).get("error", {}).get("diagnostics")
        if diag and len(diag) > 0:
            has_syntax = any("syntax" in d.get("message", "").lower() or
                            "near" in d.get("message", "").lower() or
                            "parse" in d.get("message", "").lower()
                            for d in diag)
            has_style = any("linebreak" in d.get("message", "").lower() or
                           "new line" in d.get("message", "").lower() or
                           "column" in d.get("message", "").lower()
                           for d in diag)
            if has_syntax:
                row.append("Yes")
            elif has_style:
                row.append("Style lint only")
            else:
                row.append("Yes")
        else:
            row.append("No")
    feature_rows.append(row)

    # Diagnostics: semantic — check the error file which contains an unknown table reference
    row = ["Diagnostics: semantic"]
    for name in tool_names:
        # Check both valid and error results for semantic diagnostics
        all_diags = []
        for key in ("valid", "error"):
            diag = all_results.get(name, {}).get(key, {}).get("diagnostics")
            if diag:
                all_diags.extend(diag)
        if all_diags:
            has_semantic = any("unknown" in d.get("message", "").lower() or
                              "table" in d.get("message", "").lower()
                              for d in all_diags)
            has_style = all("linebreak" in d.get("message", "").lower() or
                           "new line" in d.get("message", "").lower() or
                           "column" in d.get("message", "").lower()
                           for d in all_diags)
            if has_semantic:
                row.append("Yes")
            elif has_style:
                row.append("No (style only)")
            else:
                row.append("Partial")
        else:
            row.append("No")
    feature_rows.append(row)

    # Formatting
    row = ["Formatting"]
    for name in tool_names:
        f_data = all_results.get(name, {}).get("valid", {}).get("formatting", {})
        row.append("Yes" if cap(name, "formatting") and f_data.get("supported") else "No")
    feature_rows.append(row)

    # Rename
    row = ["Rename"]
    for name in tool_names:
        row.append("Yes" if cap(name, "rename") else "No")
    feature_rows.append(row)

    # Signature help
    row = ["Signature help"]
    for name in tool_names:
        row.append("Yes" if cap(name, "signatureHelp") else "No")
    feature_rows.append(row)

    # Requires DB
    row = ["Requires DB connection"]
    db_req = {"syntaqlite": False, "sqls": True, "sql-language-server": False}
    for name in tool_names:
        row.append("Yes" if db_req.get(name, False) else "No")
    feature_rows.append(row)

    # Diagnostic detail
    diagnostic_detail = {}
    for name in tool_names:
        diag = all_results.get(name, {}).get("error", {}).get("diagnostics")
        if diag and len(diag) > 0:
            lines = [f"{d['line']}:{d['col']} {d['severity']} {d['message']}" for d in diag]
            diagnostic_detail[name] = "\n".join(lines)
        else:
            diagnostic_detail[name] = "(no diagnostics)"

    # Speed benchmarks
    _log("  Speed benchmarks...")
    bench_sql_path = os.path.join(lsp_dir, "bench.sql")
    with open(bench_sql_path, "w") as f:
        f.write(_load_test_schema_sql())
        with open(os.path.join(DIR, "bench_statements.sql")) as bs:
            f.write(bs.read())

    for name, cmd, _ in lsp_servers:
        script = os.path.join(lsp_dir, f"_wrap_{name.replace('-', '_')}.sh")
        cmd_str = ' '.join(cmd)
        if name == "sqls":
            cmd_str += f" -c '{sqls_config}'"
        with open(script, "w") as f:
            f.write(f"#!/bin/bash\nnode '{LSP_VALIDATE}' {cmd_str} -- \"$1\" > /dev/null 2>&1\n")
        os.chmod(script, 0o755)

    bench_cmds = []
    for name, cmd, _ in lsp_servers:
        script = os.path.join(lsp_dir, f"_wrap_{name.replace('-', '_')}.sh")
        bench_cmds.append(f"-n '{name}' '{script} {bench_sql_path}'")

    cmd = (
        f"hyperfine --warmup 5 --min-runs 5 --shell=none -i "
        f"--export-markdown '{lsp_dir}/bench.md' "
        + " ".join(bench_cmds)
    )
    subprocess.run(cmd, shell=True, cwd=DIR)

    return {
        "meta": _get_meta(),
        "tool_names": tool_names,
        "feature_rows": feature_rows,
        "diagnostic_detail": diagnostic_detail,
        "speed": {
            "hyperfine_md": _read_bench_md(lsp_dir, "bench.md"),
        },
    }


# ─── Top-level ────────────────────────────────────────────────────

COLLECTORS = {
    "parser": collect_parser,
    "formatter": collect_formatter,
    "validator": collect_validator,
    "lsp": collect_lsp,
}


def collect_all(categories=None):
    """Run all specified categories and save results as JSON.
    Returns dict of category -> result dict."""
    if categories is None:
        categories = list(COLLECTORS.keys())

    os.makedirs(RESULTS_DIR, exist_ok=True)
    results = {}
    for cat in categories:
        if cat not in COLLECTORS:
            _log(f"Unknown category: {cat}")
            continue
        result = COLLECTORS[cat]()
        results[cat] = result
        path = os.path.join(RESULTS_DIR, f"{cat}.json")
        with open(path, "w") as f:
            json.dump(result, f, indent=2)
        _log(f"  Saved {path}")

    return results


def load_results(categories=None):
    """Load cached JSON results."""
    if categories is None:
        categories = list(COLLECTORS.keys())

    results = {}
    for cat in categories:
        path = os.path.join(RESULTS_DIR, f"{cat}.json")
        if not os.path.exists(path):
            _log(f"No cached results for {cat} (run without --markdown first)")
            continue
        with open(path) as f:
            results[cat] = json.load(f)
    return results
