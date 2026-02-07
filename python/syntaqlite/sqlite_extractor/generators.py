# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Header file generation utilities for SQLite code extraction."""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path

# SQLite's blessing comment for generated files
SQLITE_BLESSING = """**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
"""


@dataclass
class FileGenerator:
    """Generator for C files with optional include guards.

    For headers, provide a guard name. For source files, leave guard as None.
    """

    guard: str | None = None
    description: str = ""
    regenerate_cmd: str = ""
    includes: list[str] = field(default_factory=list)
    use_blessing: bool = True
    verbose: bool = False

    def generate(self, content: str) -> str:
        """Generate a complete file with optional guards."""
        parts = ["/*\n"]
        if self.use_blessing:
            parts.append(SQLITE_BLESSING)
        if self.description:
            parts.append(f"** {self.description}\n")
        if self.regenerate_cmd:
            parts.append(f"** DO NOT EDIT - regenerate with: {self.regenerate_cmd}\n")
        parts.append("*/\n")

        if self.guard:
            parts.append(f"#ifndef {self.guard}\n#define {self.guard}\n\n")
        else:
            parts.append("\n")

        for inc in self.includes:
            fmt = "#include {}\n" if inc.startswith("<") else '#include "{}"\n'
            parts.append(fmt.format(inc))
        if self.includes:
            parts.append("\n")

        content = content.strip()
        if content:
            parts.append(f"{content}\n")

        if self.guard:
            parts.append(f"\n#endif /* {self.guard} */\n")

        return "".join(parts)

    def write(self, path: Path, content: str) -> None:
        """Generate and write to a file."""
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(self.generate(content))
        if self.verbose:
            print(f"  Written: {path}")

    def write_raw(self, path: Path, content: str) -> None:
        """Write content to a file without wrapping (guards already present)."""
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
        if self.verbose:
            print(f"  Written: {path}")


def HeaderGenerator(
    guard: str,
    description: str = "",
    regenerate_cmd: str = "",
    includes: list[str] | None = None,
    use_blessing: bool = True,
    verbose: bool = False,
) -> FileGenerator:
    """Create a FileGenerator configured for header files."""
    return FileGenerator(
        guard=guard,
        description=description,
        regenerate_cmd=regenerate_cmd,
        includes=includes or [],
        use_blessing=use_blessing,
        verbose=verbose,
    )


def SourceFileGenerator(
    description: str = "",
    regenerate_cmd: str = "",
    use_blessing: bool = True,
    verbose: bool = False,
) -> FileGenerator:
    """Create a FileGenerator configured for source files."""
    return FileGenerator(
        guard=None,
        description=description,
        regenerate_cmd=regenerate_cmd,
        use_blessing=use_blessing,
        verbose=verbose,
    )


def format_keyword_table_entry(keyword: str, padding: int = 18) -> str:
    """Format a keyword table entry with proper alignment.

    Args:
        keyword: The keyword name (e.g., "WITHOUT").
        padding: Total width for the keyword field.

    Returns:
        A formatted table entry string.

    Example:
        >>> format_keyword_table_entry("WITHOUT")
        '  { "WITHOUT",          "TK_WITHOUT",      ALWAYS,           1      },'
    """
    name_pad = padding - len(keyword) - 2  # Account for quotes
    token = f"TK_{keyword}"
    token_pad = 15 - len(token) - 2

    return f'  {{ "{keyword}",{" " * name_pad}"{token}",{" " * token_pad}ALWAYS,           1      }},'


def extract_token_defines(parse_h_content: str) -> list[str]:
    """Extract SYNTAQLITE_TOKEN_* define lines from lemon-generated parse.h.

    Args:
        parse_h_content: Content of the parse.h file.

    Returns:
        List of #define SYNTAQLITE_TOKEN_* lines.
    """
    return re.findall(r"^#define SYNTAQLITE_TOKEN_\w+\s+\d+", parse_h_content, re.MULTILINE)


# Backward-compatible alias
extract_tk_defines = extract_token_defines
