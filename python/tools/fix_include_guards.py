# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Check and fix include guards to use SYNTAQLITE_ prefix."""

import argparse
import re
import sys
from pathlib import Path

ROOT_DIR = Path(__file__).parent.parent.parent
GUARD_PREFIX = "SYNTAQLITE_"

# Pattern to match include guard ifndef/define
IFNDEF_PATTERN = re.compile(r"^#ifndef\s+(\w+)\s*$")
DEFINE_PATTERN = re.compile(r"^#define\s+(\w+)\s*$")
ENDIF_PATTERN = re.compile(r"^#endif\s*(?://.*|/\*.*\*/\s*)?$")


def path_to_guard(path: Path) -> str:
    """Convert a file path to the expected guard name."""
    rel = path.relative_to(ROOT_DIR)
    # Convert path to guard: src/lexer.h -> SYNTAQLITE_SRC_LEXER_H
    guard = str(rel).upper().replace("/", "_").replace(".", "_")
    return f"{GUARD_PREFIX}{guard}"


def check_file(path: Path) -> tuple:
    """
    Check a header file for proper include guard.
    Returns (is_valid, expected_guard, actual_guard).
    """
    expected = path_to_guard(path)

    with open(path) as f:
        lines = f.readlines()

    # Find the ifndef line (skip initial comments)
    ifndef_guard = None
    define_guard = None

    for i, line in enumerate(lines):
        if ifndef_guard is None:
            m = IFNDEF_PATTERN.match(line)
            if m:
                ifndef_guard = m.group(1)
        elif define_guard is None:
            m = DEFINE_PATTERN.match(line)
            if m:
                define_guard = m.group(1)
                break

    if ifndef_guard is None or define_guard is None:
        return (False, expected, None)

    if ifndef_guard != define_guard:
        return (False, expected, f"{ifndef_guard}/{define_guard} (mismatch)")

    if ifndef_guard == expected:
        return (True, expected, ifndef_guard)

    return (False, expected, ifndef_guard)


def fix_file(path: Path) -> bool:
    """Fix include guard in a header file. Returns True if changes were made."""
    expected = path_to_guard(path)

    with open(path) as f:
        content = f.read()
        lines = content.split("\n")

    # Find and replace guards
    ifndef_guard = None
    modified = False
    new_lines = []

    for i, line in enumerate(lines):
        if ifndef_guard is None:
            m = IFNDEF_PATTERN.match(line)
            if m:
                ifndef_guard = m.group(1)
                if ifndef_guard != expected:
                    new_lines.append(f"#ifndef {expected}")
                    modified = True
                    continue
        elif ifndef_guard is not None:
            m = DEFINE_PATTERN.match(line)
            if m and m.group(1) == ifndef_guard:
                if ifndef_guard != expected:
                    new_lines.append(f"#define {expected}")
                    modified = True
                    ifndef_guard = expected  # Track new guard for endif
                    continue

            # Check for endif with old guard comment
            if ENDIF_PATTERN.match(line):
                # Keep simple endif or update comment
                new_lines.append(f"#endif  // {expected}")
                modified = True
                continue

        new_lines.append(line)

    if modified:
        with open(path, "w") as f:
            f.write("\n".join(new_lines))

    return modified


def main():
    parser = argparse.ArgumentParser(
        description="Fix include guards (default) or check only"
    )
    parser.add_argument("--check-only", action="store_true", help="Only check, don't fix")
    args = parser.parse_args()

    # Find all header files in src/ and syntaqlite/
    headers = []
    for dir_name in ["src", "syntaqlite"]:
        dir_path = ROOT_DIR / dir_name
        if dir_path.exists():
            headers.extend(dir_path.glob("**/*.h"))

    if not headers:
        print("No header files found (this is OK if no source files exist yet)")
        return 0

    errors = []
    fixed = []

    for path in sorted(headers):
        is_valid, expected, actual = check_file(path)

        if not is_valid:
            if args.check_only:
                errors.append((path, expected, actual))
            else:
                if fix_file(path):
                    fixed.append((path, expected, actual))

    if args.check_only:
        if errors:
            print("Invalid include guards found:")
            for path, expected, actual in errors:
                print(f"  {path}:")
                print(f"    expected: {expected}")
                print(f"    actual:   {actual}")
            return 1
        else:
            print("All include guards are valid.")
            return 0
    else:
        if fixed:
            print(f"Fixed {len(fixed)} include guards:")
            for path, expected, actual in fixed:
                print(f"  {path}: {actual} -> {expected}")
        else:
            print("All include guards are valid.")
        return 0


if __name__ == "__main__":
    sys.exit(main())
