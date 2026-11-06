"""Timing for the comparison: runs hyperfine and renders its results."""

import json
import math
import os
import shlex
import subprocess
import tempfile
import time


def _expand(argv, sql_path):
    return [a.replace("{sql}", sql_path) for a in argv]


def check_commands(commands, sql_path, cwd, ok_exit_codes=()):
    """Run each command once and exit if one fails.

    A tool that errors out would otherwise be timed at however long it takes
    to give up.
    """
    for name, argv in commands.items():
        with open(sql_path) as stdin:
            p = subprocess.run(_expand(argv, sql_path), cwd=cwd, stdin=stdin,
                               capture_output=True, text=True)
        if p.returncode != 0 and p.returncode not in ok_exit_codes:
            raise SystemExit(
                f"ERROR: {name} exited {p.returncode} on {sql_path}:\n{p.stderr[-2000:]}")


def time_commands(commands, sql_path, cwd, ok_exit_codes=(), warmup=3):
    """Time each command on `sql_path` with hyperfine.

    `commands` maps a tool name to its argv. "{sql}" in the argv is replaced
    by the benchmark file, which is also fed on stdin. hyperfine picks the run
    count (at least 10 runs and 3 s per command), so fast tools get enough runs
    to average out noise. `ok_exit_codes` lists non-zero exits that still mean
    success.

    Returns one dict per tool, in `commands` order, with times in ms.
    """
    check_commands(commands, sql_path, cwd, ok_exit_codes)
    with tempfile.NamedTemporaryFile(suffix=".json", delete=False) as f:
        json_path = f.name
    cmd = [
        "hyperfine", "--warmup", str(warmup), "--shell=none",
        "--input", sql_path, "--export-json", json_path,
    ]
    if ok_exit_codes:
        cmd.append("--ignore-failure=" + ",".join(str(c) for c in ok_exit_codes))
    for name, argv in commands.items():
        cmd += ["-n", name, shlex.join(_expand(argv, sql_path))]
    try:
        subprocess.run(cmd, cwd=cwd, check=True)
        with open(json_path) as f:
            runs = json.load(f)["results"]
    finally:
        os.unlink(json_path)
    results = []
    # hyperfine reports commands in the order given, under their full command
    # line, so pair them back up with the tool names.
    for name, r in zip(commands, runs, strict=True):
        # hyperfine 2 nests wall time under "summary"; 1.x has it at the top.
        t = r["summary"]["time_wall_clock"] if "summary" in r else r
        results.append({
            "name": name,
            "mean_ms": t["mean"] * 1000,
            "stddev_ms": (t["stddev"] or 0) * 1000,
            "min_ms": t["min"] * 1000,
            "max_ms": t["max"] * 1000,
        })
    return results


def time_once(commands, sql_path, cwd):
    """Time one run of each command, for tools too slow to repeat.

    Returns {name: ms}, or None for a tool that failed.
    """
    times = {}
    for name, argv in commands.items():
        with open(sql_path) as stdin:
            start = time.monotonic()
            p = subprocess.run(_expand(argv, sql_path), cwd=cwd, stdin=stdin,
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                               timeout=300)
            elapsed = time.monotonic() - start
        times[name] = round(elapsed * 1000) if p.returncode == 0 else None
    return times


def markdown_table(results, notes=None):
    """Mean ± σ, min, max and speed relative to the fastest tool.

    `notes` maps tool names that weren't timed to the reason, shown as a row.
    """
    lines = [
        "| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |",
        "|:---|---:|---:|---:|---:|",
    ]
    if results:
        fastest = min(results, key=lambda r: r["mean_ms"])
    for r in results:
        rel = r["mean_ms"] / fastest["mean_ms"]
        if r is fastest:
            rel_str = "1.00"
        else:
            # Propagate both tools' relative errors, as hyperfine does.
            rel_err = rel * math.hypot(
                r["stddev_ms"] / r["mean_ms"],
                fastest["stddev_ms"] / fastest["mean_ms"],
            )
            rel_str = f"{rel:.2f} ± {rel_err:.2f}"
        lines.append(
            f"| `{r['name']}` | {r['mean_ms']:.1f} ± {r['stddev_ms']:.1f} "
            f"| {r['min_ms']:.1f} | {r['max_ms']:.1f} | {rel_str} |"
        )
    for name, reason in (notes or {}).items():
        lines.append(f"| `{name}` | {reason} | | | |")
    return "\n".join(lines)
