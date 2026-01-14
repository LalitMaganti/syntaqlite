# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Check that all includes in src/ use proper paths (not local includes)."""

import re
import sys
from pathlib import Path

ROOT_DIR = Path(__file__).parent.parent.parent

# Allowed include patterns:
# - "src/..." for internal headers
# - "syntaqlite/..." for public API headers
# - <...> for system headers
ALLOWED_PATTERNS = [
    r'^#include\s+"src/',
    r'^#include\s+"syntaqlite/',
    r'^#include\s+<',
]

# Pattern that matches any #include directive
INCLUDE_PATTERN = re.compile(r'^#include\s+[<"](.+)[>"]')


def check_file(path: Path) -> list:
    """Check a single file for invalid includes. Returns list of errors."""
    errors = []
    with open(path) as f:
        for lineno, line in enumerate(f, 1):
            line = line.strip()
            if not line.startswith("#include"):
                continue

            # Check if it matches any allowed pattern
            allowed = any(re.match(p, line) for p in ALLOWED_PATTERNS)
            if not allowed:
                match = INCLUDE_PATTERN.match(line)
                if match:
                    errors.append(f"{path}:{lineno}: invalid include: {line}")
                    errors.append(f"  Use 'src/{match.group(1)}' for local headers")

    return errors


def main():
    src_dir = ROOT_DIR / "src"
    if not src_dir.exists():
        print(f"src/ directory not found (this is OK if no source files exist yet)")
        return 0

    all_errors = []
    for path in sorted(src_dir.glob("**/*.[ch]")):
        errors = check_file(path)
        all_errors.extend(errors)

    if all_errors:
        print("Include path errors found:")
        for error in all_errors:
            print(f"  {error}")
        print(f"\nTotal: {len(all_errors) // 2} invalid includes")
        return 1

    print("All includes are valid.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
