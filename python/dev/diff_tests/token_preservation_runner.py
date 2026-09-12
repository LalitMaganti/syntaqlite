# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Token preservation runner.

Measures how much of the authored token stream the AST can reproduce.

The formatter prints from the AST, so any syntax the AST does not model is
lost on the way out: an authored ASC, an implicit alias, a redundant paren.
This runner formats a corpus of SQL, compares the token stream before and
after, and groups every difference by the construct that caused it.  The
resulting ranked list is the work queue for making the AST lossless.

Keyword casing is a rendering choice rather than lost information, so
keywords compare case-insensitively.  Everything else compares exactly.

Outputs:
  - tests/token_preservation/triage.md  (gitignored) ranked work queue
  - tests/token_preservation/baseline.json  ratchet, one count per category

Any category exceeding its baseline count fails the run.  Lowering counts
requires --rebaseline to lock the improvement in.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from python.dev.diff_tests.test_loader import load_all_tests
from python.dev.diff_tests.utils import Colors, colorize

# Test directories to harvest SQL from.  These are curated statements that
# already cover the grammar broadly, so they make a good first corpus.
_SOURCE_DIRS = [
    "tests/ast_diff_tests",
    "tests/fmt_diff_tests",
]

_OUT_DIR_REL = "tests/token_preservation"

# Logs written by the upstream-sqlite suite, when it has been run. They hold
# every statement SQLite's own test files execute, which is a far broader
# corpus than the curated suites but is not checked in.
_UPSTREAM_LOGS_REL = "tests/upstream_baselines/logs"
_BASELINE_NAME = "baseline.json"
_TRIAGE_NAME = "triage.md"


def _harvest(root_dir: Path, filter_pattern: Optional[str], seen: set) -> List[dict]:
    """Collect corpus entries from the source suites, deduplicated by SQL.

    A blueprint's cflags and version travel with its SQL: without them a
    statement that needs, say, ordered-set aggregates looks like a parse
    failure rather than a measurement.
    """
    seen: set = set()
    corpus: List[dict] = []
    for test_dir in _SOURCE_DIRS:
        tag = test_dir.rsplit("/", 1)[-1].replace("_diff_tests", "")
        for name, blueprint in load_all_tests(root_dir, filter_pattern=None, test_dir=test_dir):
            sql = blueprint.sql.strip()
            if not sql or sql in seen:
                continue
            seen.add(sql)
            entry = {"name": f"{tag}/{name}", "sql": sql}
            if getattr(blueprint, "cflags", None):
                entry["cflags"] = list(blueprint.cflags)
            if getattr(blueprint, "version", None):
                entry["version"] = blueprint.version
            corpus.append(entry)
    if filter_pattern:
        pat = re.compile(filter_pattern, re.IGNORECASE)
        corpus = [e for e in corpus if pat.search(e["name"])]
    return corpus


def _harvest_upstream(root_dir: Path, seen: set) -> List[dict]:
    """Collect statements from the upstream-sqlite logs, if they exist.

    Only statements both engines accepted are useful here: the harness asks
    what the formatter does with valid SQL, not how it reports errors.
    """
    logs = root_dir / _UPSTREAM_LOGS_REL
    if not logs.is_dir():
        return []
    corpus: List[dict] = []
    for path in sorted(logs.glob("*.jsonl")):
        name = path.stem
        for index, line in enumerate(path.read_text(errors="replace").splitlines()):
            line = line.strip()
            if not line:
                continue
            try:
                record = json.loads(line)
            except ValueError:
                continue
            if str(record.get("parse_ok")) != "True" or str(record.get("sqlite_ok")) != "True":
                continue
            sql = (record.get("sql") or "").strip()
            if not sql or sql in seen:
                continue
            seen.add(sql)
            corpus.append({"name": f"upstream/{name}:{index}", "sql": sql})
    return corpus


def _run_harness(root_dir: Path, corpus_path: Path) -> Tuple[List[dict], str]:
    """Build and run the Rust harness over the corpus, returning its records."""
    build = subprocess.run(
        ["cargo", "build", "--release", "--features", "serde-json",
         "--example", "token_preservation"],
        cwd=root_dir, capture_output=True, text=True,
    )
    if build.returncode != 0:
        sys.stderr.write(build.stderr)
        raise SystemExit("failed to build the token_preservation harness")

    binary = root_dir / "target" / "release" / "examples" / "token_preservation"
    proc = subprocess.run(
        [str(binary), str(corpus_path)], cwd=root_dir, capture_output=True, text=True,
    )
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr)
        raise SystemExit("token_preservation harness failed")
    records = [json.loads(line) for line in proc.stdout.splitlines() if line.strip()]
    return records, proc.stderr.strip()


def _write_triage(path: Path, total: int, records: List[dict]) -> Counter:
    """Write the ranked work queue and return the per-category counts."""
    counts: Counter = Counter(r["category"] for r in records)
    examples: Dict[str, List[dict]] = defaultdict(list)
    for record in records:
        if len(examples[record["category"]]) < 3:
            examples[record["category"]].append(record)

    reproduced = total - len(records)
    rate = 100.0 if total == 0 else reproduced * 100.0 / total

    lines = [
        "# Token preservation triage",
        "",
        "Constructs the AST cannot currently reproduce, ranked by how often "
        "they occur in the corpus. Each entry is a work item: model the "
        "construct in `.synq` so the formatter can print it back.",
        "",
        f"Corpus: {total} statements. Reproduced: {reproduced} ({rate:.2f}%). "
        f"Lost: {len(records)}.",
        "",
    ]
    for category, count in counts.most_common():
        lines.append(f"## {category}  ({count})")
        lines.append("")
        for example in examples[category]:
            lines.append(f"- `{example['name']}`")
            lines.append("")
            lines.append("  ```sql")
            for sql_line in example["sql"].splitlines():
                lines.append(f"  {sql_line}")
            lines.append("  ```")
            lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")
    return counts


def _check_baseline(baseline_path: Path, counts: Counter, rebaseline: bool) -> int:
    """Compare counts against the ratchet, or rewrite it."""
    current = {category: count for category, count in counts.items()}
    if rebaseline:
        baseline_path.parent.mkdir(parents=True, exist_ok=True)
        baseline_path.write_text(
            json.dumps(dict(sorted(current.items())), indent=2) + "\n", encoding="utf-8"
        )
        print(f"rebaselined {len(current)} categor{'y' if len(current) == 1 else 'ies'}")
        return 0

    if not baseline_path.exists():
        print("no baseline yet; run with --rebaseline to create one")
        return 0

    baseline = json.loads(baseline_path.read_text(encoding="utf-8"))
    regressions = []
    improvements = []
    for category, count in sorted(current.items()):
        allowed = baseline.get(category, 0)
        if count > allowed:
            regressions.append((category, allowed, count))
        elif count < allowed:
            improvements.append((category, allowed, count))
    for category, allowed in sorted(baseline.items()):
        if category not in current and allowed > 0:
            improvements.append((category, allowed, 0))

    for category, allowed, count in improvements:
        print(colorize(f"  improved: {category}: {allowed} -> {count}", Colors.GREEN))
    for category, allowed, count in regressions:
        print(colorize(f"  REGRESSED: {category}: {allowed} -> {count}", Colors.RED))

    if regressions:
        print(colorize(f"\n{len(regressions)} categor(y/ies) regressed", Colors.RED))
        return 1
    if improvements:
        print("\nrun with --rebaseline to lock in the improvements")
    return 0


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--filter", dest="filter_pattern", help="Only corpus entries matching this pattern")
    parser.add_argument(
        "--upstream", action="store_true",
        help="Also measure every statement in the upstream-sqlite logs (much broader, "
             "needs that suite to have been run at least once)",
    )
    parser.add_argument("--rebaseline", action="store_true", help="Rewrite the baseline from this run")
    parser.add_argument("-v", "--verbose", action="count", default=0)
    args = parser.parse_args(argv)

    root_dir = Path(__file__).resolve().parents[3]
    out_dir = root_dir / _OUT_DIR_REL
    out_dir.mkdir(parents=True, exist_ok=True)

    seen: set = set()
    corpus = _harvest(root_dir, args.filter_pattern, seen)
    if args.upstream:
        upstream = _harvest_upstream(root_dir, seen)
        if not upstream:
            print(
                f"no upstream logs in {_UPSTREAM_LOGS_REL}; "
                "run the upstream-sqlite suite first"
            )
        corpus += upstream
    if not corpus:
        print("no corpus entries matched")
        return 0

    corpus_path = out_dir / "corpus.jsonl"
    with corpus_path.open("w", encoding="utf-8") as handle:
        for entry in corpus:
            handle.write(json.dumps(entry) + "\n")

    records, summary = _run_harness(root_dir, corpus_path)
    if summary:
        print(summary)

    counts = _write_triage(out_dir / _TRIAGE_NAME, len(corpus), records)
    if args.verbose:
        for category, count in counts.most_common():
            print(f"  {count:5d}  {category}")
    print(f"triage written to {_OUT_DIR_REL}/{_TRIAGE_NAME}")

    if args.upstream:
        # The upstream logs are gitignored and depend on a prior run of that
        # suite, so this corpus is not the same from machine to machine. Report
        # it, but leave the ratchet to the reproducible corpus.
        print("upstream corpus included; baseline not checked")
        return 0
    return _check_baseline(out_dir / _BASELINE_NAME, counts, args.rebaseline)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
