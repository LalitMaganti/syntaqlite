# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Upstream SQLite test suite.

Runs SQLite's ~1,390 upstream TCL test files through both real SQLite
(sqlite3_prepare_v2) and syntaqlite's parser/validator side-by-side,
comparing results to detect regressions.

Disabled by default — run explicitly with:
    tools/run-integration-tests --suite upstream-sqlite
    tools/run-integration-tests --suite upstream-sqlite --analyze-only

Prerequisites:
    - tclsh and tcl-dev installed (e.g., apt install tcl-dev)
    - SQLite sources present (run tools/install-build-deps first)
"""

from __future__ import annotations

import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
from collections import Counter
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass, field
from pathlib import Path

from python.dev.integration_tests.suite import SuiteContext

NAME = "upstream-sqlite"
DESCRIPTION = "Upstream SQLite test files through syntaqlite parser/validator"
ENABLED_BY_DEFAULT = False
NEEDS_BINARY = False

# Persistent log directory (gitignored).
_LOGS_DIR_REL = "tests/upstream_baselines/logs"

# Tests that are known to timeout or hang in our harness (crash recovery,
# heavy fault injection, etc.).  Skipped entirely — not relevant to SQL parsing.
_SKIP_TESTS: set[str] = {
    "crash.test",       # Deliberate crash/recovery — hangs under our shim.
    "tkt2686.test",     # Heavy I/O regression test — exceeds 60s timeout.
}


@dataclass
class Summary:
    total: int = 0
    parse_ok: int = 0
    parse_error: int = 0
    both_accept: int = 0
    both_reject: int = 0
    false_positive: int = 0
    gap: int = 0


@dataclass
class FileResult:
    file: str
    entries: list[dict] = field(default_factory=list)
    error: str | None = None


# ---------------------------------------------------------------------------
# Build helpers
# ---------------------------------------------------------------------------


def _find_tcl_include() -> str | None:
    """Find tcl.h include directory."""
    if platform.system() == "Darwin":
        # Try Homebrew tcl-tk package first (includes tcl9 / tcl8.6).
        brew = shutil.which("brew")
        if brew:
            result = subprocess.run(
                ["brew", "--prefix", "tcl-tk"], capture_output=True, text=True,
            )
            if result.returncode == 0:
                prefix = Path(result.stdout.strip())
                # Homebrew tcl-tk 9.x puts headers under include/tcl-tk/.
                for sub in ["include/tcl-tk", "include"]:
                    p = prefix / sub
                    if (p / "tcl.h").exists():
                        return str(p)
        # Xcode SDK fallback.
        sdk = subprocess.run(
            ["xcrun", "--show-sdk-path"], capture_output=True, text=True,
        )
        if sdk.returncode == 0:
            p = Path(sdk.stdout.strip()) / "usr" / "include"
            if (p / "tcl.h").exists():
                return str(p)
        # Common paths.
        for d in ["/opt/homebrew/include", "/usr/local/include", "/usr/include"]:
            if Path(d, "tcl.h").exists():
                return d
    else:
        for d in ["/usr/include/tcl8.6", "/usr/include/tcl", "/usr/include"]:
            if Path(d, "tcl.h").exists():
                return d
    return None


def _find_tcl_lib_flags() -> list[str]:
    """Return linker flags for tcl."""
    if platform.system() == "Darwin":
        brew = shutil.which("brew")
        if brew:
            result = subprocess.run(
                ["brew", "--prefix", "tcl-tk"], capture_output=True, text=True,
            )
            if result.returncode == 0:
                lib_dir = Path(result.stdout.strip()) / "lib"
                if lib_dir.exists():
                    # Detect tcl9 vs tcl8.6.
                    for name in ["tcl9.0", "tclstub9.0", "tcl8.6"]:
                        if list(lib_dir.glob(f"lib{name}*")):
                            return [f"-L{lib_dir}", f"-l{name}"]
        return ["-ltcl8.6"]
    # Linux: distros ship different tcl versions (8.6 on Ubuntu, 9.0 on
    # Fedora). Probe common lib dirs for an available libtcl* rather than
    # hardcoding a version.
    for lib_dir in ["/usr/lib64", "/usr/lib/x86_64-linux-gnu", "/usr/lib"]:
        p = Path(lib_dir)
        if not p.is_dir():
            continue
        for name in ["tcl9.0", "tcl8.6", "tcl"]:
            if (p / f"lib{name}.so").exists():
                return [f"-L{lib_dir}", f"-l{name}"]
    return ["-ltcl8.6"]


def _build_extension(ctx: SuiteContext, verbose: bool) -> Path | None:
    """Build the tclsyntaqlite TCL extension. Returns the .so/.dylib path."""
    root = ctx.root_dir
    upstream_dir = root / "tests/upstream"
    csrc = upstream_dir / "csrc" / "tclsyntaqlite.c"

    if not csrc.exists():
        print(f"  error: {csrc} not found", file=sys.stderr)
        return None

    tcl_include = _find_tcl_include()
    if not tcl_include:
        print("  error: tcl.h not found. Install tcl-dev.", file=sys.stderr)
        return None

    # Build syntaqlite as a static library (staticlib for C FFI linking).
    print("  Building syntaqlite static lib...", end=" ", flush=True)
    proc = subprocess.run(
        ["cargo", "build", "-p", "syntaqlite", "--release"],
        cwd=root, capture_output=True, text=True,
    )
    if proc.returncode != 0:
        print("FAILED")
        print(proc.stderr, file=sys.stderr)
        return None
    print("OK")

    static_lib = root / "target" / "release" / "libsyntaqlite.a"
    if not static_lib.exists():
        print(f"  error: {static_lib} not found", file=sys.stderr)
        return None

    ext = ".dylib" if platform.system() == "Darwin" else ".so"
    output = root / "target" / f"tclsyntaqlite{ext}"

    syntax_include = root / "syntaqlite-syntax" / "include"
    lib_include = root / "syntaqlite" / "include"
    sqlite_amalg = root / "third_party" / "src" / "sqlite-amalgamation"

    tcl_lib_flags = _find_tcl_lib_flags()

    print("  Compiling tclsyntaqlite extension...", end=" ", flush=True)

    cc_cmd = [
        "cc", "-shared", "-fPIC", "-o", str(output),
        str(csrc),
        str(sqlite_amalg / "sqlite3.c"),
        f"-I{tcl_include}",
        f"-I{syntax_include}",
        f"-I{lib_include}",
        f"-I{sqlite_amalg}",
        # Link the static archive by its explicit path: syntaqlite is now
        # built as a cdylib too, so a libsyntaqlite.so sits beside the .a and
        # `-lsyntaqlite` would prefer it — but the cdylib doesn't re-export the
        # parser FFI symbols (they live in syntaqlite-syntax), leaving the
        # extension silently broken at runtime.
        str(static_lib),
        *tcl_lib_flags,
        "-lpthread", "-ldl", "-lm",
        "-O2",
        # Enable SQLite extensions so upstream tests can exercise them.
        "-DSQLITE_ENABLE_FTS3",
        "-DSQLITE_ENABLE_FTS4",
        "-DSQLITE_ENABLE_FTS5",
    ]

    if platform.system() == "Darwin":
        # macOS needs -undefined dynamic_lookup for Tcl symbols.
        cc_cmd.insert(3, "-undefined")
        cc_cmd.insert(4, "dynamic_lookup")
        # Remove -ldl (not needed on macOS).
        cc_cmd = [f for f in cc_cmd if f != "-ldl"]

    proc = subprocess.run(cc_cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        print("FAILED")
        if verbose:
            print(" ".join(cc_cmd))
        print(proc.stderr, file=sys.stderr)
        return None

    print("OK")
    return output


# ---------------------------------------------------------------------------
# Test execution
# ---------------------------------------------------------------------------


def _discover_test_files(
    test_dir: Path, filter_pat: str | None,
) -> tuple[list[Path], list[str]]:
    """Find .test files, optionally filtered by substring.

    Returns (files_to_run, skipped_names).
    """
    files = sorted(p for p in test_dir.iterdir() if p.suffix == ".test")
    if filter_pat:
        files = [f for f in files if filter_pat in f.stem]
    skipped = [f.name for f in files if f.name in _SKIP_TESTS]
    files = [f for f in files if f.name not in _SKIP_TESTS]
    return files, skipped


def _run_single_test(
    extension_lib: Path,
    tester_shim: Path,
    validate: bool,
    test_file: Path,
    log_file: Path,
) -> FileResult:
    """Run one .test file through tclsh and write JSONL log to log_file."""
    name = test_file.name

    shim_dir = tester_shim.parent
    script = (
        f"load {extension_lib} Tclsyntaqlite\n"
        f"set argv0 {shim_dir}/test.tcl\n"
        f"source {test_file}\n"
        "syntaqlite_summary\n"
    )

    env = os.environ.copy()
    env["SYNTAQLITE_TEST_LOG"] = str(log_file)
    env["SYNTAQLITE_TEST_VALIDATE"] = "1" if validate else "0"
    env["tcl_interactive"] = "0"

    # Concurrent tests would otherwise race over db files in a shared cwd.
    try:
        with tempfile.TemporaryDirectory(prefix="synq-upstream-") as workdir:
            proc = subprocess.run(
                ["tclsh", "-"],
                input=script, capture_output=True, text=True,
                env=env, timeout=60, cwd=workdir,
            )
    except subprocess.TimeoutExpired:
        log_file.unlink(missing_ok=True)
        return FileResult(file=name, error=f"tclsh timed out for {name}")
    except FileNotFoundError:
        log_file.unlink(missing_ok=True)
        return FileResult(file=name, error="tclsh not found")

    entries = _read_log_file(log_file)

    error = None
    if proc.returncode != 0:
        error = f"tclsh exited {proc.returncode}: {proc.stderr[:500]}"

    return FileResult(file=name, entries=entries, error=error)


def _read_log_file(log_file: Path) -> list[dict]:
    """Read JSONL entries from a log file."""
    entries: list[dict] = []
    if not log_file.exists():
        return entries
    for line in log_file.read_text().splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            entries.append(json.loads(line))
        except json.JSONDecodeError:
            pass
    return entries


# ---------------------------------------------------------------------------
# Aggregation
# ---------------------------------------------------------------------------


@dataclass
class Mismatch:
    """One statement where syntaqlite and SQLite disagree.

    `detail` is the first syntaqlite diagnostic or parse error (false
    positives) or the SQLite error (gaps). `category` is the normalized
    bucket used for baselines and triage.
    """
    file: str
    sql: str
    detail: str
    category: str


def _aggregate(results: list[FileResult]) -> tuple[Summary, list[Mismatch], list[Mismatch]]:
    """Compute summary statistics and collect false positives and gaps."""
    s = Summary()
    fps: list[Mismatch] = []
    gaps: list[Mismatch] = []
    for fr in results:
        for entry in fr.entries:
            s.total += 1
            parse_ok = entry.get("parse_ok", False)
            sqlite_ok = entry.get("sqlite_ok", False)

            if parse_ok:
                s.parse_ok += 1
            else:
                s.parse_error += 1

            diagnostics = entry.get("diagnostics") or []
            syntaqlite_ok = parse_ok and len(diagnostics) == 0

            if sqlite_ok and syntaqlite_ok:
                s.both_accept += 1
            elif not sqlite_ok and not syntaqlite_ok:
                s.both_reject += 1
            elif sqlite_ok and not syntaqlite_ok:
                s.false_positive += 1
                # Categorize by the *first* diagnostic — later ones are often
                # cascading (e.g. unknown column after unknown table).
                if not parse_ok:
                    detail = entry.get("parse_error", "")
                    category = "PARSE ERROR: " + _normalize_sqlite_error(detail)
                else:
                    detail = diagnostics[0]["message"]
                    category = _normalize_diagnostic(detail)
                fps.append(Mismatch(fr.file, entry.get("sql", ""), detail, category))
            else:
                s.gap += 1
                detail = entry.get("sqlite_error", "")
                category = _normalize_sqlite_error(detail) or "(no error message)"
                gaps.append(Mismatch(fr.file, entry.get("sql", ""), detail, category))

    return s, fps, gaps


def _print_summary(
    summary: Summary, file_count: int,
    error_files: list[tuple[str, str]],
    skipped: list[str],
    verbose: bool,
    false_positives: list[Mismatch],
) -> None:
    """Print the standard summary block."""
    print()
    print("  === Upstream Test Summary ===")
    print(f"  Files run:            {file_count}")
    if skipped:
        print(f"  Files skipped:        {len(skipped)} ({', '.join(skipped)})")
    print(f"  Files with errors:    {len(error_files)}")
    if error_files:
        for name, reason in error_files:
            print(f"    - {name}: {reason}")
    print()
    print(f"  Total SQL statements: {summary.total}")
    print(f"    Parse OK:           {summary.parse_ok}")
    print(f"    Parse error:        {summary.parse_error}")
    print()
    print(f"    Both accept:        {summary.both_accept} (agreement)")
    print(f"    Both reject:        {summary.both_reject} (agreement)")
    print(f"    False positives:    {summary.false_positive} (syntaqlite rejects valid SQL)")
    print(f"    Gaps:               {summary.gap} (syntaqlite accepts, sqlite rejects)")

    if false_positives and verbose:
        print()
        print("  === False Positives (syntaqlite rejects valid SQL) ===")
        for fp in false_positives[:50]:
            sql_display = fp.sql[:200]
            if len(fp.sql) > 200:
                sql_display += "..."
            print(f"    {fp.file}: {fp.detail}")
            print(f"      SQL: {sql_display}")
            print()
        if len(false_positives) > 50:
            print(f"    ... and {len(false_positives) - 50} more")


# ---------------------------------------------------------------------------
# Detailed analysis (runs on persisted logs)
# ---------------------------------------------------------------------------


def _normalize_diagnostic(msg: str) -> str:
    """Normalize a diagnostic message to a category key."""
    if msg.startswith("unknown table"):
        m = re.search(r"'([^']*)'", msg)
        table = m.group(1) if m else "?"
        # Group system tables together.
        if table.startswith("sqlite_"):
            return "unknown table (system): " + table
        return "unknown table (user-defined)"
    if msg.startswith("unknown column"):
        return "unknown column"
    if msg.startswith("unknown function"):
        m = re.search(r"'([^']*)'", msg)
        func = m.group(1) if m else "?"
        return "unknown function: " + func
    if "argument" in msg.lower() or "arity" in msg.lower():
        return "wrong function arity"
    if msg.startswith("CTE"):
        return "CTE column count mismatch"
    return msg[:80]


def _normalize_sqlite_error(msg: str) -> str:
    """Normalize a sqlite3_prepare_v2 error to a category key."""
    msg = msg.strip()
    # "no such table: X" → "no such table"
    if msg.startswith("no such table"):
        return "no such table"
    if msg.startswith("no such column"):
        return "no such column"
    if msg.startswith("no such function"):
        return "no such function"
    if msg.startswith("near "):
        return "syntax error (near ...)"
    if "syntax error" in msg:
        return "syntax error"
    if msg.startswith("no such module"):
        return "no such module"
    if msg.startswith("table ") and "already exists" in msg:
        return "table already exists"
    if "ambiguous column" in msg:
        return "ambiguous column name"
    if "not authorized" in msg:
        return "not authorized"
    if "misuse of aggregate" in msg:
        return "misuse of aggregate"
    if "no tables specified" in msg:
        return "no tables specified"
    # Fallback: these keys feed the per-category baseline ratchet, so mask
    # instance-specific data (quoted tokens, ordinals, numbers/identifiers
    # with digits, ": <object>" tails) to group by error class — otherwise
    # each instance becomes its own ratchet category and any shift reads as
    # a regression. triage.md still shows the raw message via `detail`.
    msg = re.sub(r'"[^"]*"?', '"?"', msg)
    msg = re.sub(r"\b\d+(?:st|nd|rd|th)\b", "Nth", msg)
    msg = re.sub(r"\b\w*\d\w*\b", "N", msg)
    msg = re.sub(r"(: ).*$", r"\1?", msg)
    return msg[:60] if len(msg) > 60 else msg


def _analyze_detailed(
    results: list[FileResult],
    fps: list[Mismatch],
    gaps: list[Mismatch],
    verbose: bool,
) -> None:
    """Print a detailed breakdown of false positives and gaps."""

    fp_diagnostics = Counter(fp.category for fp in fps)
    fp_by_file = Counter(fp.file for fp in fps)
    gap_errors = Counter(g.category for g in gaps)
    gap_by_file = Counter(g.file for g in gaps)
    parse_error_categories: Counter[str] = Counter()

    for fr in results:
        for entry in fr.entries:
            parse_ok = entry.get("parse_ok", False)
            sqlite_ok = entry.get("sqlite_ok", False)
            diagnostics = entry.get("diagnostics") or []
            syntaqlite_ok = parse_ok and len(diagnostics) == 0

            # Parse errors on statements SQLite also rejects (agreement).
            if not parse_ok and not sqlite_ok and not syntaqlite_ok:
                pe = entry.get("parse_error", "")
                parse_error_categories[_normalize_sqlite_error(pe)] += 1

    # --- False positive breakdown ---
    if fp_diagnostics:
        print()
        print("  === False Positive Breakdown ===")
        print(f"  {'Count':>7}  Category")
        print(f"  {'-----':>7}  {'--------'}")
        for cat, count in fp_diagnostics.most_common(30):
            print(f"  {count:>7}  {cat}")
        if len(fp_diagnostics) > 30:
            rest = sum(c for _, c in fp_diagnostics.most_common()[30:])
            print(f"  {rest:>7}  ... ({len(fp_diagnostics) - 30} more categories)")

        if verbose:
            print()
            print("  Top files with false positives:")
            for f, count in fp_by_file.most_common(20):
                print(f"    {count:>5}  {f}")

    # --- Gap breakdown ---
    if gap_errors:
        print()
        print("  === Gap Breakdown (syntaqlite accepts, sqlite rejects) ===")
        print(f"  {'Count':>7}  SQLite error category")
        print(f"  {'-----':>7}  {'--------------------'}")
        for cat, count in gap_errors.most_common(30):
            print(f"  {count:>7}  {cat}")
        if len(gap_errors) > 30:
            rest = sum(c for _, c in gap_errors.most_common()[30:])
            print(f"  {rest:>7}  ... ({len(gap_errors) - 30} more categories)")

        if verbose:
            print()
            print("  Top files with gaps:")
            for f, count in gap_by_file.most_common(20):
                print(f"    {count:>5}  {f}")

    # --- Parse error breakdown ---
    if parse_error_categories and verbose:
        print()
        print("  === Parse Error Breakdown (syntaqlite parse failures) ===")
        print(f"  {'Count':>7}  Category")
        print(f"  {'-----':>7}  {'--------'}")
        for cat, count in parse_error_categories.most_common(20):
            print(f"  {count:>7}  {cat}")


# ---------------------------------------------------------------------------
# Triage report
# ---------------------------------------------------------------------------

_TRIAGE_EXAMPLES_PER_CATEGORY = 5


def _write_triage_report(path: Path, fps: list[Mismatch], gaps: list[Mismatch]) -> None:
    """Write a markdown worklist of every disagreement, grouped by category.

    Regenerated on every run (including --analyze-only) so the current
    gap/false-positive backlog is always one command away. Categories are
    sorted by count — the top entries are the highest-leverage fixes.
    """
    out: list[str] = [
        "# Upstream SQLite triage worklist",
        "",
        "Generated by the `upstream-sqlite` suite. Regenerate with:",
        "`tools/run-integration-tests --suite upstream-sqlite --analyze-only`",
        "",
    ]

    def section(title: str, items: list[Mismatch]) -> None:
        by_cat: dict[str, list[Mismatch]] = {}
        for m in items:
            by_cat.setdefault(m.category, []).append(m)
        out.append(f"## {title} — {len(items)} across {len(by_cat)} categories")
        out.append("")
        for cat, ms in sorted(by_cat.items(), key=lambda kv: (-len(kv[1]), kv[0])):
            out.append(f"### {cat} ({len(ms)})")
            out.append("")
            for m in ms[:_TRIAGE_EXAMPLES_PER_CATEGORY]:
                sql = " ".join(m.sql.split())
                if len(sql) > 200:
                    sql = sql[:200] + "..."
                out.append(f"- `{m.file}`: {m.detail}")
                out.append(f"  - `{sql}`")
            if len(ms) > _TRIAGE_EXAMPLES_PER_CATEGORY:
                out.append(f"- ... and {len(ms) - _TRIAGE_EXAMPLES_PER_CATEGORY} more")
            out.append("")

    section("Gaps (syntaqlite accepts, SQLite rejects)", gaps)
    section("False positives (syntaqlite rejects valid SQL)", fps)

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(out) + "\n")


# ---------------------------------------------------------------------------
# Baseline comparison
# ---------------------------------------------------------------------------


def _platform_baseline_path(base_path: Path) -> Path:
    """Return the platform-specific baseline path.

    Looks for parse_acceptance.<platform>.json first (e.g. linux, darwin),
    falling back to the unsuffixed parse_acceptance.json.
    """
    system = platform.system().lower()  # "linux", "darwin", "windows"
    platform_path = base_path.with_suffix(f".{system}.json")
    if platform_path.exists():
        return platform_path
    return base_path


def _ratchet_mismatches(
    old: dict, scalar_key: str, label: str,
    new_total: int, new_categories: dict[str, int],
) -> int:
    """One-way ratchet on a mismatch count. Returns the regression delta.

    Prefers per-category comparison (a shift between categories is a
    regression even when the total is flat); falls back to the scalar total
    for baselines that predate category tracking.
    """
    old_categories = old.get(scalar_key + "_categories")
    if old_categories is None:
        old_total = old.get(scalar_key)
        if old_total is None or new_total <= old_total:
            return 0
        print(f"  Regression: {scalar_key} increased from {old_total} to {new_total}")
        return new_total - old_total

    regressions = 0
    for cat, count in new_categories.items():
        old_count = old_categories.get(cat, 0)
        if count > old_count:
            print(
                f"  Regression: {label} category {cat!r} "
                f"increased from {old_count} to {count}",
            )
            regressions += count - old_count
    return regressions


def _categorize_unique(items: list[Mismatch]) -> dict[str, int]:
    """Per-category counts over *unique* (file, category, sql) mismatches.

    Raw occurrence counts jitter run-to-run — some TCL tests execute the
    same statement a variable number of times — so the ratchet compares
    deduplicated counts, which are stable for a fixed corpus.
    """
    unique = {(m.file, m.category, m.sql) for m in items}
    return dict(sorted(Counter(cat for (_, cat, _) in unique).items()))


def _check_baseline(
    baseline_path: Path, summary: Summary,
    fps: list[Mismatch], gaps: list[Mismatch],
    rebaseline: bool,
) -> int:
    """Compare against baseline. Returns number of regressions (0 = pass).

    Note: editing the _normalize_* categorizers reshuffles category keys and
    requires a --rebaseline.
    """
    fp_categories = _categorize_unique(fps)
    gap_categories = _categorize_unique(gaps)
    data = {
        "total": summary.total,
        "parse_ok": summary.parse_ok,
        "parse_error": summary.parse_error,
        "both_accept": summary.both_accept,
        "both_reject": summary.both_reject,
        "false_positive": summary.false_positive,
        "gap": summary.gap,
        "false_positive_categories": fp_categories,
        "gap_categories": gap_categories,
    }

    # Use platform-specific baseline when available.
    system = platform.system().lower()
    platform_path = baseline_path.with_suffix(f".{system}.json")

    if rebaseline:
        target = platform_path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(json.dumps(data, indent=2) + "\n")
        print(f"\n  Baseline written to {target}")
        return 0

    effective_path = _platform_baseline_path(baseline_path)

    if not effective_path.exists():
        effective_path.parent.mkdir(parents=True, exist_ok=True)
        effective_path.write_text(json.dumps(data, indent=2) + "\n")
        print(f"\n  No baseline found. Created initial baseline at {effective_path}")
        return 0

    old = json.loads(effective_path.read_text())
    regressions = 0

    regressions += _ratchet_mismatches(
        old, "false_positive", "false positive", summary.false_positive, fp_categories,
    )
    regressions += _ratchet_mismatches(
        old, "gap", "gap", summary.gap, gap_categories,
    )

    if summary.parse_ok < old.get("parse_ok", 0):
        print(
            f"  Regression: parse_ok decreased from "
            f"{old['parse_ok']} to {summary.parse_ok}",
        )
        regressions += old["parse_ok"] - summary.parse_ok

    if regressions == 0:
        print("\n  No regressions from baseline.")
        improvements = sum(
            old[key] - data[key]
            for key in ("false_positive", "gap")
            if key in old and data[key] < old[key]
        )
        if improvements:
            print(
                f"  {improvements} fewer mismatch(es) than baseline — "
                "run with --rebaseline to lock this in.",
            )

    return regressions


# ---------------------------------------------------------------------------
# Load logs from disk (for --analyze-only)
# ---------------------------------------------------------------------------


def _load_logs(logs_dir: Path, filter_pat: str | None) -> list[FileResult]:
    """Load persisted JSONL logs from disk."""
    if not logs_dir.is_dir():
        return []
    log_files = sorted(logs_dir.glob("*.jsonl"))
    if filter_pat:
        log_files = [f for f in log_files if filter_pat in f.stem]
    results: list[FileResult] = []
    for lf in log_files:
        name = lf.stem + ".test"
        entries = _read_log_file(lf)
        results.append(FileResult(file=name, entries=entries))
    return results


# ---------------------------------------------------------------------------
# Main entry point
# ---------------------------------------------------------------------------


def run(ctx: SuiteContext) -> int:
    verbose = ctx.verbose >= 1
    root = ctx.root_dir
    logs_dir = root / _LOGS_DIR_REL

    # --analyze-only: skip building/running, load from persisted logs.
    if ctx.analyze_only:
        if not logs_dir.is_dir():
            print(
                f"  error: No logs found at {logs_dir}. "
                "Run without --analyze-only first.",
                file=sys.stderr,
            )
            return 1

        file_results = _load_logs(logs_dir, ctx.filter_pattern)
        if not file_results:
            print("  error: No log files found.", file=sys.stderr)
            return 1

        print(f"  Loaded {len(file_results)} log files from {logs_dir}")
        summary, false_positives, gaps = _aggregate(file_results)
        _print_summary(summary, len(file_results), [], [], verbose, false_positives)
        _analyze_detailed(file_results, false_positives, gaps, verbose)

        triage_path = root / "tests" / "upstream_baselines" / "triage.md"
        _write_triage_report(triage_path, false_positives, gaps)
        print(f"\n  Triage worklist written to {triage_path}")

        baseline_path = root / "tests" / "upstream_baselines" / "parse_acceptance.json"
        regressions = _check_baseline(
            baseline_path, summary, false_positives, gaps, ctx.rebaseline,
        )
        if regressions > 0:
            print(f"\n  {regressions} regression(s) detected!")
            return 1
        return 0

    # --- Normal mode: build, run, persist logs ---

    # Check for tclsh.
    if not shutil.which("tclsh"):
        print("error: tclsh not found. Install tcl (e.g., apt install tcl).", file=sys.stderr)
        return 1

    # Check for SQLite test directory.
    test_dir = root / "third_party" / "src" / "sqlite" / "test"
    if not test_dir.is_dir():
        print(f"error: SQLite test directory not found at {test_dir}", file=sys.stderr)
        print("Run tools/install-build-deps first.", file=sys.stderr)
        return 1

    tester_shim = root / "tests/upstream" / "tcl" / "tester.tcl"
    if not tester_shim.exists():
        print(f"error: tester shim not found at {tester_shim}", file=sys.stderr)
        return 1

    # Build the TCL extension.
    extension_lib = _build_extension(ctx, verbose)
    if extension_lib is None:
        return 1

    # Discover test files.
    filter_pat = ctx.filter_pattern
    test_files, skipped = _discover_test_files(test_dir, filter_pat)
    if not test_files:
        print(f"error: No .test files found in {test_dir}", file=sys.stderr)
        return 1

    jobs = ctx.jobs or os.cpu_count() or 1
    print(f"  Found {len(test_files)} test files (jobs={jobs})")

    # Prepare persistent log directory.
    logs_dir.mkdir(parents=True, exist_ok=True)

    # Run tests.
    validate = ctx.validate
    file_results: list[FileResult] = []
    done = 0
    total = len(test_files)

    if jobs == 1:
        for test_file in test_files:
            done += 1
            if verbose:
                print(f"\r  [{done}/{total}] {test_file.name}...", end="", flush=True)
            log_file = logs_dir / f"{test_file.stem}.jsonl"
            file_results.append(
                _run_single_test(extension_lib, tester_shim, validate, test_file, log_file),
            )
    else:
        with ProcessPoolExecutor(max_workers=jobs) as pool:
            futures = {
                pool.submit(
                    _run_single_test, extension_lib, tester_shim, validate, tf,
                    logs_dir / f"{tf.stem}.jsonl",
                ): tf
                for tf in test_files
            }
            for future in as_completed(futures):
                done += 1
                tf = futures[future]
                if verbose:
                    print(f"\r  [{done}/{total}] {tf.name}...", end="", flush=True)
                file_results.append(future.result())

    if verbose:
        print()  # Clear progress line.

    # Aggregate and print summary.
    summary, false_positives, gaps = _aggregate(file_results)
    error_files = [(r.file, r.error) for r in file_results if r.error]

    _print_summary(summary, len(file_results), error_files, skipped, verbose, false_positives)
    _analyze_detailed(file_results, false_positives, gaps, verbose)

    triage_path = root / "tests" / "upstream_baselines" / "triage.md"
    _write_triage_report(triage_path, false_positives, gaps)
    print(f"\n  Triage worklist written to {triage_path}")

    # Baseline comparison.
    baseline_path = root / "tests" / "upstream_baselines" / "parse_acceptance.json"
    regressions = _check_baseline(
        baseline_path, summary, false_positives, gaps, ctx.rebaseline,
    )
    if regressions > 0:
        print(f"\n  {regressions} regression(s) detected!")
        return 1

    return 0
