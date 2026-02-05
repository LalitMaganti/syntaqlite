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


def guard_from_path(path: Path, prefix: str = "SYNTAQLITE") -> str:
    """Convert a file path to an include guard name.

    Args:
        path: The file path (relative or absolute).
        prefix: Prefix for the guard (default: SYNTAQLITE).

    Returns:
        An include guard string like SYNTAQLITE_SRC_FOO_H.

    Examples:
        >>> guard_from_path(Path("src/tokenizer/tokens.h"))
        'SYNTAQLITE_SRC_TOKENIZER_TOKENS_H'
        >>> guard_from_path(Path("include/foo.h"), prefix="MY_PREFIX")
        'MY_PREFIX_INCLUDE_FOO_H'
    """
    # Get just the relative parts we care about
    parts = path.parts

    # Convert to uppercase and replace separators
    name = "_".join(parts).upper()

    # Replace non-alphanumeric chars with underscore
    name = re.sub(r"[^A-Z0-9]", "_", name)

    # Remove consecutive underscores
    name = re.sub(r"_+", "_", name)

    # Remove leading/trailing underscores
    name = name.strip("_")

    return f"{prefix}_{name}"


@dataclass
class FileGenerator:
    """Generator for C files with optional include guards.

    For headers, provide a guard name. For source files, leave guard as None.

    Example:
        # Header file
        gen = FileGenerator(
            guard="SYNTAQLITE_SRC_TOKENS_H",
            description="Token definitions",
        )

        # Source file
        gen = FileGenerator(description="Implementation")
    """

    guard: str | None = None
    description: str = ""
    regenerate_cmd: str = ""
    includes: list[str] = field(default_factory=list)
    use_blessing: bool = True

    def generate(self, content: str) -> str:
        """Generate a complete file with optional guards."""
        parts = []

        # Opening comment block
        parts.append("/*\n")
        if self.use_blessing:
            parts.append(SQLITE_BLESSING)
        if self.description:
            parts.append(f"** {self.description}\n")
        if self.regenerate_cmd:
            parts.append(f"** DO NOT EDIT - regenerate with: {self.regenerate_cmd}\n")
        parts.append("*/\n")

        # Include guard (headers only)
        if self.guard:
            parts.append(f"#ifndef {self.guard}\n")
            parts.append(f"#define {self.guard}\n\n")
        else:
            parts.append("\n")

        # Includes
        for inc in self.includes:
            if inc.startswith("<"):
                parts.append(f"#include {inc}\n")
            else:
                parts.append(f'#include "{inc}"\n')
        if self.includes:
            parts.append("\n")

        # Main content
        content = content.strip()
        if content:
            parts.append(content)
            parts.append("\n")

        # Close guard (headers only)
        if self.guard:
            parts.append(f"\n#endif /* {self.guard} */\n")

        return "".join(parts)

    def write(self, path: Path, content: str) -> None:
        """Generate and write to a file."""
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(self.generate(content))
        print(f"  Written: {path}")


def HeaderGenerator(
    guard: str,
    description: str = "",
    regenerate_cmd: str = "",
    includes: list[str] | None = None,
    use_blessing: bool = True,
) -> FileGenerator:
    """Create a FileGenerator configured for header files."""
    return FileGenerator(
        guard=guard,
        description=description,
        regenerate_cmd=regenerate_cmd,
        includes=includes or [],
        use_blessing=use_blessing,
    )


def SourceFileGenerator(
    description: str = "",
    regenerate_cmd: str = "",
    use_blessing: bool = True,
) -> FileGenerator:
    """Create a FileGenerator configured for source files."""
    return FileGenerator(
        guard=None,
        description=description,
        regenerate_cmd=regenerate_cmd,
        use_blessing=use_blessing,
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


def extract_tk_defines(parse_h_content: str) -> list[str]:
    """Extract TK_* define lines from lemon-generated parse.h.

    Args:
        parse_h_content: Content of the parse.h file.

    Returns:
        List of #define TK_* lines.
    """
    return re.findall(r"^#define TK_\w+\s+\d+", parse_h_content, re.MULTILINE)
