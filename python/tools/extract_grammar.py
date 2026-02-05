# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Extract grammar rules from SQLite parse.y using Lemon.

This script:
1. Builds lemon using the shared ToolRunner
2. Runs lemon -g on parse.y to preprocess the grammar
3. Extracts the grammar rules to a clean output file
"""
import argparse
import subprocess
import sys
from pathlib import Path

ROOT_DIR = Path(__file__).parent.parent.parent
if str(ROOT_DIR) not in sys.path:
    sys.path.insert(0, str(ROOT_DIR))

from python.sqlite_extractor import ToolRunner

THIRD_PARTY_DIR = ROOT_DIR / "third_party"
SQLITE_SRC_DIR = THIRD_PARTY_DIR / "src" / "sqlite"
PARSE_Y_PATH = SQLITE_SRC_DIR / "src" / "parse.y"
OUTPUT_DIR = THIRD_PARTY_DIR / "src" / "sqlite_grammar"


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

    # Build lemon using shared ToolRunner
    runner = ToolRunner(root_dir=ROOT_DIR)
    lemon_path = runner.build("lemon")

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
