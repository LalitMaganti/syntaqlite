# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Extract grammar rules from SQLite parse.y using Lemon.

This script:
1. Builds lemon into out/.extract_grammar/ if needed
2. Runs lemon -g on parse.y to preprocess the grammar
3. Extracts the grammar rules to a clean output file
"""
import argparse
import os
import subprocess
import sys
from pathlib import Path

ROOT_DIR = Path(__file__).parent.parent.parent
THIRD_PARTY_DIR = ROOT_DIR / "third_party"
SQLITE_SRC_DIR = THIRD_PARTY_DIR / "src" / "sqlite"
PARSE_Y_PATH = SQLITE_SRC_DIR / "src" / "parse.y"
OUTPUT_DIR = THIRD_PARTY_DIR / "src" / "sqlite_grammar"

# Dedicated build directory for this script
TOOLS_BUILD_DIR = ROOT_DIR / "out" / ".extract_grammar"


def build_lemon() -> Path:
    """Build lemon into out/.extract_grammar/ and return its path."""
    lemon_path = TOOLS_BUILD_DIR / "lemon"
    if os.name == "nt":
        lemon_path = TOOLS_BUILD_DIR / "lemon.exe"

    gn = ROOT_DIR / "tools" / "dev" / "gn"
    ninja = ROOT_DIR / "tools" / "dev" / "ninja"

    # Run gn gen if build.ninja doesn't exist
    build_ninja = TOOLS_BUILD_DIR / "build.ninja"
    if not build_ninja.exists():
        print(f"Configuring build in {TOOLS_BUILD_DIR}...")
        TOOLS_BUILD_DIR.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            [str(gn), "gen", str(TOOLS_BUILD_DIR), "--args=is_debug=false"],
            capture_output=True,
            text=True
        )
        # Check if build.ninja was created (gn may return non-zero even on success)
        if not build_ninja.exists():
            print("Failed to configure build", file=sys.stderr)
            sys.exit(1)

    # Build lemon
    print("Building lemon...")
    result = subprocess.run(
        [str(ninja), "-C", str(TOOLS_BUILD_DIR), "lemon"],
        capture_output=True,
        text=True
    )
    if result.returncode != 0:
        print(f"Failed to build lemon: {result.stderr}", file=sys.stderr)
        sys.exit(1)

    return lemon_path


def preprocess_grammar(lemon_path: Path, grammar_path: Path, defines: list = None) -> str:
    """Run lemon -g to preprocess the grammar.

    Args:
        lemon_path: Path to the lemon executable
        grammar_path: Path to the grammar file
        defines: List of macros to define

    Returns:
        Preprocessed grammar as a string
    """
    cmd = [str(lemon_path), "-g"]
    for d in (defines or []):
        cmd.append(f"-D{d}")
    cmd.append(str(grammar_path))

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"Lemon preprocessing failed: {result.stderr}")

    return result.stdout


def parse_preprocessed_grammar(output: str) -> list:
    """Parse the preprocessed grammar output from lemon -g.

    Returns a list of grammar rules.
    """
    rules = []
    for line in output.split("\n"):
        line = line.strip()
        # Skip symbol table comments
        if line.startswith("//"):
            continue
        # Collect rules
        if "::=" in line:
            rules.append(line)
    return rules


def main():
    parser = argparse.ArgumentParser(description="Extract grammar from SQLite parse.y")
    parser.add_argument(
        "--grammar",
        type=Path,
        default=PARSE_Y_PATH,
        help="Path to grammar file"
    )
    parser.add_argument(
        "--define", "-D",
        action="append",
        default=[],
        help="Define a macro (can be repeated)"
    )
    parser.add_argument(
        "--output", "-o",
        type=Path,
        default=OUTPUT_DIR / "grammar.txt",
        help="Output file"
    )
    args = parser.parse_args()

    # Check grammar file exists
    if not args.grammar.exists():
        print(f"Grammar file not found: {args.grammar}", file=sys.stderr)
        print("Run tools/dev/install-build-deps first.", file=sys.stderr)
        return 1

    # Build lemon
    lemon_path = build_lemon()

    # Preprocess grammar
    print(f"Extracting grammar from {args.grammar}...")
    output = preprocess_grammar(lemon_path, args.grammar, args.define)
    rules = parse_preprocessed_grammar(output)

    # Write output
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(rules))
    print(f"Written {len(rules)} rules to {args.output}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
