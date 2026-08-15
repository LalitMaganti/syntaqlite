# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Pre-push verification: all checks must pass before marking work done.

Usage:
    tools/pre-push          — run affected checks only (smart detection)
    tools/pre-push --all    — run all checks regardless of what changed
    tools/pre-push --fix    — auto-fix what's possible, then run remaining checks
    tools/pre-push -q       — quiet: no output unless a step fails (for agents)
    tools/pre-push -v       — verbose: step headers + full command output

Smart detection compares HEAD + staged + unstaged changes against the upstream
tracking branch (falling back to origin/main) to determine which checks are
actually needed.  Checks that cannot be affected by the changed files are
skipped.  Pass --all to run everything unconditionally.

Examples of what gets skipped:
  • Only syntaqlite-buildtools/ changed  → diff tests skipped
  • Only dialects/perfetto/ changed      → only perfetto diff tests run
  • Only syntaqlite/src/fmt/ changed     → only fmt + perfetto-fmt tests run
  • Only *.c / *.h changed               → Rust checks skipped
  • Only test baselines changed          → only the matching diff suite runs

Build cache strategy: the --all-features clippy step uses an isolated target
directory (target/clippy-all-features) so it doesn't invalidate the cache for
subsequent steps that run with default/no-default features. RUSTFLAGS are
never changed mid-script, avoiding full-workspace cache invalidation.

All cargo invocations go through tools/cargo (hermetic toolchain) to keep
RUSTFLAGS consistent and avoid cross-tool cache invalidation.

Phases (parallelism within each phase; phases run sequentially):
  1. Format        — cargo fmt
  2. Lint + C      — clippy variants + C format/deps (parallel lanes)
  3. Build + test  — check-public-api → build CLI → unit tests (sequential)
  4. Diff tests    — all diff/integration suites (parallel)
"""

from __future__ import annotations

import argparse
import os
import platform
import subprocess
import sys
from collections.abc import Callable
from concurrent.futures import ThreadPoolExecutor, as_completed

from python.tools.cargo_slots import acquire_slot

ROOT_DIR: str = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# ANSI color codes.
_BLUE: str = "\033[1;34m"
_RED: str = "\033[1;31m"
_GREEN: str = "\033[1;32m"
_YELLOW: str = "\033[1;33m"
_RESET: str = "\033[0m"

_DEAD_CODE_LINTS: list[str] = [
    "-D", "dead_code", "-D", "unused_imports",
    "-D", "unused_variables", "-D", "unused_mut",
]


def _cargo_path() -> str:
    return os.path.join(ROOT_DIR, "tools", "cargo")


def _tool(name: str) -> str:
    return os.path.join(ROOT_DIR, "tools", name)


def _cargo_cmd(*args: str) -> list[str]:
    """Build a cargo command list via the hermetic wrapper."""
    return [sys.executable, _cargo_path()] + list(args)


# ---------------------------------------------------------------------------
# Step results + runners
# ---------------------------------------------------------------------------

class StepResult:
    """Captured result of a single step."""
    __slots__ = ("desc", "returncode", "output")

    desc: str
    returncode: int
    output: str

    def __init__(self, desc: str, returncode: int, output: str) -> None:
        self.desc = desc
        self.returncode = returncode
        self.output = output

    @property
    def ok(self) -> bool:
        return self.returncode == 0


def _run_capturing(desc: str, cmd: list[str]) -> StepResult:
    """Run *cmd* capturing stdout+stderr into a StepResult."""
    try:
        proc = subprocess.run(
            cmd, cwd=ROOT_DIR,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        )
        return StepResult(desc, proc.returncode, proc.stdout)
    except Exception as exc:
        return StepResult(desc, 1, str(exc))


def run_step(desc: str, cmd: list[str], verbosity: int) -> StepResult:
    """Run a single sequential step with immediate reporting."""
    if verbosity >= 1:
        # Verbose: header + live output.
        print(f"\n{_BLUE}==> {desc}{_RESET}")
        rc = subprocess.call(cmd, cwd=ROOT_DIR)
        return StepResult(desc, rc, "")

    if verbosity >= 0:
        print(f"{_BLUE}==> {desc}{_RESET}")

    result = _run_capturing(desc, cmd)

    if not result.ok:
        if verbosity >= 0:
            print(f"{_RED}    FAILED. Output:{_RESET}")
        else:
            print(f"{_RED}==> FAILED: {desc}{_RESET}")
        if result.output:
            sys.stdout.write(result.output)
            if not result.output.endswith("\n"):
                sys.stdout.write("\n")

    return result


# ---------------------------------------------------------------------------
# Parallel execution
# ---------------------------------------------------------------------------

def _run_lane(steps: list[tuple[str, list[str]]]) -> list[StepResult]:
    """Run *steps* sequentially (capturing output). Stops on first failure.

    Each element of *steps* is ``(desc, cmd)``.  Returns ``list[StepResult]``.
    """
    results: list[StepResult] = []
    for desc, cmd in steps:
        result = _run_capturing(desc, cmd)
        results.append(result)
        if not result.ok:
            break
    return results


def run_parallel_group(lanes: list[list[tuple[str, list[str]]]], verbosity: int) -> bool:
    """Run *lanes* in parallel, report results in launch order.

    Each lane is a list of ``(desc, cmd)`` tuples executed sequentially within
    the lane.  Different lanes run concurrently via threads (subprocess-bound,
    so the GIL is not a bottleneck).

    Returns ``True`` if every step in every lane succeeded.
    """
    lane_results: list[list[StepResult] | None] = [None] * len(lanes)

    with ThreadPoolExecutor(max_workers=len(lanes)) as pool:
        futures = {}
        for i, lane in enumerate(lanes):
            futures[pool.submit(_run_lane, lane)] = i

        for future in as_completed(futures):
            lane_results[futures[future]] = future.result()

    # Report in launch order.
    all_ok = True
    for results in lane_results:
        if results is None:
            continue
        for r in results:
            if r.ok:
                if verbosity >= 1:
                    print(f"\n{_BLUE}==> {r.desc}{_RESET}")
                    if r.output:
                        sys.stdout.write(r.output)
                        if not r.output.endswith("\n"):
                            sys.stdout.write("\n")
                elif verbosity >= 0:
                    print(f"{_BLUE}==> {r.desc}{_RESET}")
            else:
                all_ok = False
                if verbosity >= 0:
                    print(f"{_BLUE}==> {r.desc}{_RESET}")
                    print(f"{_RED}    FAILED. Output:{_RESET}")
                else:
                    print(f"{_RED}==> FAILED: {r.desc}{_RESET}")
                if r.output:
                    sys.stdout.write(r.output)
                    if not r.output.endswith("\n"):
                        sys.stdout.write("\n")
    return all_ok


# ---------------------------------------------------------------------------
# Emscripten detection (mirrors the shell logic)
# ---------------------------------------------------------------------------

def _has_emscripten() -> bool:
    sys_name = platform.system().lower()
    machine = platform.machine().lower()
    arch = "arm64" if machine in ("arm64", "aarch64") else "amd64"

    plat_dirs: dict[str, str] = {"darwin": "mac-" + arch, "linux": "linux-" + arch}
    plat_dir = plat_dirs.get(sys_name)
    if not plat_dir:
        return False
    return os.path.isdir(
        os.path.join(ROOT_DIR, "third_party", "bin", plat_dir, "emscripten"),
    )


# ---------------------------------------------------------------------------
# Smart change detection
# ---------------------------------------------------------------------------

# Rust paths whose changes only affect specific diff-test domains.
# Files not matching any of these (and not in _RUST_NO_DIFF_PATHS) are
# treated as "core" and trigger all diff tests.
_RUST_PARSER_PATHS = (
    "syntaqlite-parser/",
    "syntaqlite-parser-sqlite/",
    "syntaqlite/src/parser/",
    "syntaqlite/src/sqlite/",
)
_RUST_FMT_PATHS = ("syntaqlite/src/fmt/",)
_RUST_SEMANTIC_PATHS = ("syntaqlite/src/semantic/",)
_RUST_PERFETTO_PATHS = ("dialects/perfetto/",)
_RUST_AMALG_PATHS = ("syntaqlite-wasm/",)

# Rust changes in these paths don't affect any diff test suites.
_RUST_NO_DIFF_PATHS = (
    "syntaqlite-buildtools/",
    "syntaqlite/src/lsp/",
)

# C/H files under these paths only affect amalgamation tests.
_AMALG_C_PATHS = ("sqlite-amalgamations/",)


def _get_changed_files() -> set[str] | None:
    """Return the set of file paths changed since the upstream tracking branch.

    Includes committed-but-not-pushed, staged, and unstaged changes.
    Returns None if the comparison base cannot be determined (run all checks).
    """
    # Find the upstream tracking branch; fall back to origin/main.
    proc = subprocess.run(
        ["git", "rev-parse", "--abbrev-ref", "--symbolic-full-name", "@{u}"],
        cwd=ROOT_DIR, capture_output=True, text=True,
    )
    upstream = proc.stdout.strip() if proc.returncode == 0 else "origin/main"

    # Find the common ancestor so we compare the right range.
    proc = subprocess.run(
        ["git", "merge-base", "HEAD", upstream],
        cwd=ROOT_DIR, capture_output=True, text=True,
    )
    if proc.returncode != 0:
        return None  # No common base; run all checks.
    merge_base = proc.stdout.strip()

    changed: set[str] = set()
    for git_args in (
        ["diff", "--name-only", merge_base, "HEAD"],  # committed since base
        ["diff", "--name-only", "--cached"],           # staged
        ["diff", "--name-only"],                       # unstaged
    ):
        proc = subprocess.run(
            ["git"] + git_args, cwd=ROOT_DIR, capture_output=True, text=True,
        )
        if proc.returncode == 0:
            changed.update(f for f in proc.stdout.strip().splitlines() if f)
    return changed


def _classify(changed: set[str] | None) -> dict[str, bool]:
    """Map changed file paths to check-domain flags.

    Returns a dict of bool flags.  If *changed* is None (unknown), all flags
    are set True (conservative: run everything).
    """
    all_true: dict[str, bool] = {
        "has_rust": True, "has_c": True,
        "run_ast": True, "run_fmt": True, "run_amalg": True,
        "run_perfetto_fmt": True, "run_perfetto_val": True,
        "run_grammar": True,
    }
    if changed is None:
        return all_true

    f: dict[str, bool] = {k: False for k in all_true}

    for path in changed:
        is_rust = path.endswith(".rs") or path.endswith("Cargo.toml") or path == "Cargo.lock"
        is_c = path.endswith(".c") or path.endswith(".h")
        is_synq = path.endswith(".synq")

        if is_rust:
            f["has_rust"] = True
        if is_c:
            f["has_c"] = True

        # .y grammar action files affect token IDs in all dialects.
        if path.endswith(".y"):
            f["run_grammar"] = True
            continue

        # .synq definitions drive all C and Rust codegen; touch every domain.
        if is_synq:
            f["has_rust"] = f["has_c"] = True
            f["run_ast"] = f["run_fmt"] = True
            f["run_perfetto_fmt"] = f["run_perfetto_val"] = True
            continue

        # Test-baseline changes only require running the matching suite.
        if path.startswith("tests/ast_diff_tests/"):
            f["run_ast"] = True
        if path.startswith("tests/fmt_diff_tests/"):
            f["run_fmt"] = True
        if path.startswith("tests/amalg_tests/"):
            f["run_amalg"] = True
        if path.startswith("tests/perfetto_fmt_diff_tests/"):
            f["run_perfetto_fmt"] = True
        if path.startswith("tests/perfetto_validation_diff_tests/"):
            f["run_perfetto_val"] = True

        # C file domain: sqlite amalgamation sources → amalg only; other C
        # files are parser sources that affect all non-amalg diff tests.
        if is_c:
            if path.startswith(_AMALG_C_PATHS):
                f["run_amalg"] = True
            else:
                f["run_ast"] = f["run_fmt"] = True
                f["run_perfetto_fmt"] = f["run_perfetto_val"] = True

        # Rust file domain classification.
        if is_rust:
            if path.startswith(_RUST_PARSER_PATHS):
                # Parser changes affect all output formats.
                f["run_ast"] = f["run_fmt"] = True
                f["run_perfetto_fmt"] = f["run_perfetto_val"] = True
            elif path.startswith(_RUST_FMT_PATHS):
                f["run_fmt"] = True
                f["run_perfetto_fmt"] = True
            elif path.startswith(_RUST_SEMANTIC_PATHS):
                f["run_perfetto_val"] = True
            elif path.startswith(_RUST_PERFETTO_PATHS):
                f["run_perfetto_fmt"] = True
                f["run_perfetto_val"] = True
                f["run_grammar"] = True
            elif path.startswith(_RUST_AMALG_PATHS):
                f["run_amalg"] = True
            elif path.startswith(_RUST_NO_DIFF_PATHS):
                # Grammar checks are relevant when the parser pipeline (which
                # controls token ordering) changes in buildtools.
                if path.startswith("syntaqlite-buildtools/src/parser_tools/"):
                    f["run_grammar"] = True
            else:
                # Core / shared Rust (cli, common, dialect, etc.) — conservative.
                f["run_ast"] = f["run_fmt"] = True
                f["run_perfetto_fmt"] = f["run_perfetto_val"] = True

        # Non-Rust, non-C, non-.synq files (docs, Python tools, etc.) don't
        # affect any compiled output, so we leave the diff-test flags alone.

    return f


# ---------------------------------------------------------------------------
# Main orchestrator
# ---------------------------------------------------------------------------

def _main(fix: bool, verbosity: int, run_all: bool) -> int:
    cargo = _cargo_cmd
    with acquire_slot() as clippy_all_target_dir:
        return _run(fix, verbosity, run_all, cargo, clippy_all_target_dir)


def _run(fix: bool, verbosity: int, run_all: bool, cargo: Callable[..., list[str]], clippy_all_target_dir: str) -> int:
    # ── Detect what changed ────────────────────────────────────────────
    if run_all:
        changed_files = None  # None → all flags True
    else:
        changed_files = _get_changed_files()

    flags = _classify(changed_files)

    need_rust = flags["has_rust"]
    need_c = flags["has_c"]
    need_compilation = need_rust or need_c
    need_diff_tests = any(
        flags[k] for k in ("run_ast", "run_fmt", "run_amalg", "run_perfetto_fmt", "run_perfetto_val", "run_grammar")
    )

    # If diff tests are needed but no Rust/C changed, verify the CLI binary
    # already exists; if not, force a build.
    if need_diff_tests and not need_compilation:
        cli_binary = os.path.join(ROOT_DIR, "target", "debug", "syntaqlite")
        if not os.path.exists(cli_binary):
            if verbosity >= 0:
                print(f"{_YELLOW}==> CLI binary not found — forcing build{_RESET}")
            need_compilation = True
            need_rust = True

    # Print a brief change-detection summary (normal verbosity only).
    if verbosity >= 0 and changed_files is not None:
        if not changed_files:
            print(f"\n{_GREEN}Nothing changed since upstream — skipping all checks.{_RESET}")
            return 0
        domains = []
        if need_rust:
            domains.append("Rust")
        if need_c:
            domains.append("C")
        if not domains:
            domains.append("non-code")
        active_diffs = [
            k.replace("run_", "") for k in
            ("run_ast", "run_fmt", "run_amalg", "run_perfetto_fmt", "run_perfetto_val", "run_grammar")
            if flags[k]
        ]
        diff_summary = ", ".join(active_diffs) if active_diffs else "none"
        print(f"{_BLUE}[smart] changed: {', '.join(domains)} | diff tests: {diff_summary}{_RESET}")

    def _skip(desc):
        if verbosity >= 0:
            print(f"{_YELLOW}==> {desc} (skipped — not affected){_RESET}")

    # ── Phase 1: Format (sequential) ──────────────────────────────────
    if need_rust:
        if fix:
            r = run_step("cargo fmt", cargo("fmt"), verbosity)
        else:
            r = run_step("cargo fmt --check", cargo("fmt", "--check"), verbosity)
        if not r.ok:
            return r.returncode
    else:
        _skip("cargo fmt --check")

    # ── Phase 2: Lint + C checks ──────────────────────────────────────
    if fix:
        # Fix mode: parallel lanes, then a sequential tail.
        #   Lane A — clippy --fix --all-features (isolated target dir)
        #   Lane B — format-c (fix) + check-c-deps (no cargo lock needed)
        lint_lanes = []
        if need_rust:
            lint_lanes.append([
                ("cargo clippy --fix --all-features",
                 cargo("clippy", "--tests", "--all-features", "--all-targets",
                       "--fix", "--allow-dirty", "--allow-staged",
                       "--target-dir", clippy_all_target_dir,
                       "--", "-D", "warnings")),
            ])
        else:
            _skip("cargo clippy --fix --all-features")
        if need_c:
            lint_lanes.append([
                ("tools/format-c", [_tool("format-c")]),
                ("tools/check-c-deps", [_tool("check-c-deps")]),
            ])
        else:
            _skip("tools/format-c")

        if lint_lanes:
            if not run_parallel_group(lint_lanes, verbosity):
                return 1

        # Sequential tail: re-format after clippy --fix, then dead-code.
        if need_rust:
            for desc, cmd in [
                ("cargo fmt (post-clippy-fix)", cargo("fmt")),
                ("dead code check: default features",
                 cargo("clippy", "--all-targets", "--", *_DEAD_CODE_LINTS)),
                ("dead code check: no default features",
                 cargo("clippy", "--all-targets", "--no-default-features",
                       "--", *_DEAD_CODE_LINTS)),
            ]:
                r = run_step(desc, cmd, verbosity)
                if not r.ok:
                    return r.returncode
        else:
            _skip("dead code checks")
    else:
        # Check mode: up to three parallel lanes.
        #   Lane A — clippy --all-features (isolated target dir)
        #   Lane B — clippy default → clippy no-default (shared target dir)
        #   Lane C — format-c --check + check-c-deps (no cargo)
        lint_lanes = []
        if need_rust:
            lint_lanes.append([
                ("cargo clippy --all-features",
                 cargo("clippy", "--tests", "--all-features", "--all-targets",
                       "--target-dir", clippy_all_target_dir,
                       "--", "-D", "warnings")),
            ])
            lint_lanes.append([
                ("dead code check: default features",
                 cargo("clippy", "--all-targets", "--", *_DEAD_CODE_LINTS)),
                ("dead code check: no default features",
                 cargo("clippy", "--all-targets", "--no-default-features",
                       "--", *_DEAD_CODE_LINTS)),
            ])
        else:
            _skip("cargo clippy --all-features")
            _skip("dead code checks")
        if need_c:
            lint_lanes.append([
                ("tools/format-c --check", [_tool("format-c"), "--check"]),
                ("tools/check-c-deps", [_tool("check-c-deps")]),
            ])
        else:
            _skip("tools/format-c --check")

        if lint_lanes:
            if not run_parallel_group(lint_lanes, verbosity):
                return 1

    # ── Phase 3: API + build + unit tests (sequential) ────────────────
    # All three use target/ — must be sequential to avoid cargo lock contention.
    if need_rust:
        r = run_step("tools/check-public-api", [_tool("check-public-api")], verbosity)
        if not r.ok:
            return r.returncode
    else:
        _skip("tools/check-public-api")

    if need_compilation:
        r = run_step(
            "cargo build -p syntaqlite-cli",
            cargo("build", "-p", "syntaqlite-cli"),
            verbosity,
        )
        if not r.ok:
            return r.returncode
    else:
        _skip("cargo build -p syntaqlite-cli")

    if need_rust:
        r = run_step("tools/run-unit-tests", [_tool("run-unit-tests")], verbosity)
        if not r.ok:
            return r.returncode
    else:
        _skip("tools/run-unit-tests")

    # ── Phase 4: Diff/integration tests (parallel) ────────────────────
    # Each suite uses isolated temp directories and only reads the CLI
    # binary — safe to run concurrently.
    diff_lanes = []
    diff_skipped = []

    _integration = _tool("run-integration-tests")
    for key, suite_name in (
        ("run_ast",           "ast"),
        ("run_fmt",           "fmt"),
        ("run_amalg",         "amalg"),
        ("run_perfetto_fmt",  "perfetto-fmt"),
        ("run_perfetto_val",  "perfetto-val"),
        ("run_grammar",       "grammar"),
    ):
        if flags[key]:
            diff_lanes.append([(
                f"integration-tests/{suite_name}",
                [_integration, "--suite", suite_name],
            )])
        else:
            diff_skipped.append(f"integration-tests/{suite_name}")

    # WASM playground: only relevant when any compilation is needed.
    run_wasm = need_compilation
    if run_wasm and _has_emscripten():
        diff_lanes.append([
            ("tools/build-web-playground", [_tool("build-web-playground"), "--hermetic"]),
        ])
    elif verbosity >= 0:
        reason = "not affected" if not run_wasm else "emscripten not installed"
        print(f"\n{_YELLOW}==> tools/build-web-playground (skipped — {reason}){_RESET}")

    for name in diff_skipped:
        _skip(name)

    if diff_lanes:
        if not run_parallel_group(diff_lanes, verbosity):
            return 1
    elif not need_rust and not need_c:
        # Nothing ran at all — be explicit.
        if verbosity >= 0:
            print(f"\n{_YELLOW}No compilation or diff tests needed.{_RESET}")

    print(f"\n{_GREEN}All pre-push checks passed.{_RESET}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Pre-push verification: all checks must pass.",
    )
    parser.add_argument(
        "--fix", action="store_true",
        help="Auto-fix what's possible, then run remaining checks",
    )
    parser.add_argument(
        "--all", action="store_true",
        help="Run all checks regardless of what changed (disables smart detection)",
    )
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "-q", "--quiet", action="store_true",
        help="No output unless a step fails (for agents)",
    )
    group.add_argument(
        "-v", "--verbose", action="store_true",
        help="Step headers + full command output",
    )
    args = parser.parse_args()

    verbosity = -1 if args.quiet else (1 if args.verbose else 0)

    try:
        return _main(args.fix, verbosity, args.all)
    except KeyboardInterrupt:
        print(f"\n{_RED}Interrupted.{_RESET}", file=sys.stderr)
        return 130


if __name__ == "__main__":
    sys.exit(main())
