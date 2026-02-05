# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Source file handling for SQLite code extraction."""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


@dataclass
class SourceFile:
    """Represents a source file with extraction utilities.

    Provides safe, declarative methods for extracting specific regions
    of C source code like functions, arrays, and text between markers.
    """

    path: Path
    content: str

    @classmethod
    def from_path(cls, path: Path) -> SourceFile:
        """Create a SourceFile by reading from disk."""
        return cls(path=path, content=path.read_text())

    def extract_between(
        self,
        start: str,
        end: str | None = None,
        *,
        include_start: bool = False,
        include_end: bool = False,
    ) -> str:
        """Extract content between two markers.

        Args:
            start: The start marker string to search for.
            end: The end marker string. If None, extracts to end of file.
            include_start: Include the start marker in the result.
            include_end: Include the end marker in the result.

        Returns:
            The extracted content.

        Raises:
            ValueError: If start marker is not found.
        """
        start_idx = self.content.find(start)
        if start_idx == -1:
            raise ValueError(f"Start marker not found: {start!r}")

        if include_start:
            extract_start = start_idx
        else:
            extract_start = start_idx + len(start)

        if end is None:
            return self.content[extract_start:]

        end_idx = self.content.find(end, start_idx + len(start))
        if end_idx == -1:
            raise ValueError(f"End marker not found: {end!r}")

        if include_end:
            extract_end = end_idx + len(end)
        else:
            extract_end = end_idx

        return self.content[extract_start:extract_end]

    def extract_function(self, name_pattern: str, *, include_static: bool = True) -> str:
        """Extract a C function by name pattern.

        Finds a function definition matching the pattern and extracts
        the entire function body including the closing brace.

        Args:
            name_pattern: Regex pattern to match the function name.
            include_static: Whether to include 'static' in the match.

        Returns:
            The complete function definition.

        Raises:
            ValueError: If function is not found.
        """
        # Pattern matches: [static] <return_type> <name>(<params>) {
        if include_static:
            prefix = r"(?:static\s+)?"
        else:
            prefix = ""

        # Match function signature
        pattern = rf"^{prefix}(\w+(?:\s+\w+)*)\s+({name_pattern})\s*\([^)]*\)\s*\{{"
        match = re.search(pattern, self.content, re.MULTILINE)
        if not match:
            raise ValueError(f"Function not found: {name_pattern}")

        func_start = match.start()

        # Find matching closing brace using brace counting
        brace_count = 0
        in_string = False
        in_char = False
        in_line_comment = False
        in_block_comment = False
        escape_next = False
        i = match.end() - 1  # Start at the opening brace

        while i < len(self.content):
            char = self.content[i]

            if escape_next:
                escape_next = False
                i += 1
                continue

            if char == "\\":
                escape_next = True
                i += 1
                continue

            # Handle comments
            if in_line_comment:
                if char == "\n":
                    in_line_comment = False
                i += 1
                continue

            if in_block_comment:
                if char == "*" and i + 1 < len(self.content) and self.content[i + 1] == "/":
                    in_block_comment = False
                    i += 2
                    continue
                i += 1
                continue

            # Handle strings and chars
            if in_string:
                if char == '"':
                    in_string = False
                i += 1
                continue

            if in_char:
                if char == "'":
                    in_char = False
                i += 1
                continue

            # Start of comments/strings
            if char == "/" and i + 1 < len(self.content):
                next_char = self.content[i + 1]
                if next_char == "/":
                    in_line_comment = True
                    i += 2
                    continue
                elif next_char == "*":
                    in_block_comment = True
                    i += 2
                    continue

            if char == '"':
                in_string = True
                i += 1
                continue

            if char == "'":
                in_char = True
                i += 1
                continue

            # Count braces
            if char == "{":
                brace_count += 1
            elif char == "}":
                brace_count -= 1
                if brace_count == 0:
                    return self.content[func_start : i + 1]

            i += 1

        raise ValueError(f"Could not find end of function: {name_pattern}")

    def extract_array(self, name_pattern: str, *, include_static: bool = True) -> str:
        """Extract a C array declaration by name pattern.

        Args:
            name_pattern: Regex pattern to match the array name.
            include_static: Whether to include 'static' in the match.

        Returns:
            The complete array declaration including initializer.

        Raises:
            ValueError: If array is not found.
        """
        if include_static:
            prefix = r"(?:static\s+)?"
        else:
            prefix = ""

        # Match array declaration: [static] [const] type name[size] = {
        pattern = rf"^{prefix}((?:const\s+)?\w+(?:\s+\w+)*)\s+({name_pattern})\s*\[[^\]]*\]\s*=\s*\{{"
        match = re.search(pattern, self.content, re.MULTILINE)
        if not match:
            raise ValueError(f"Array not found: {name_pattern}")

        array_start = match.start()

        # Find the closing brace and semicolon
        brace_count = 1
        i = match.end()

        while i < len(self.content) and brace_count > 0:
            char = self.content[i]
            if char == "{":
                brace_count += 1
            elif char == "}":
                brace_count -= 1
            i += 1

        # Include the semicolon if present
        while i < len(self.content) and self.content[i] in " \t\n":
            i += 1
        if i < len(self.content) and self.content[i] == ";":
            i += 1

        return self.content[array_start:i]

    def find_comment_block(self, containing: str) -> tuple[int, int]:
        """Find a C comment block containing specific text.

        Args:
            containing: Text that must appear within the comment.

        Returns:
            Tuple of (start_index, end_index) of the comment block.

        Raises:
            ValueError: If no matching comment block is found.
        """
        idx = self.content.find(containing)
        if idx == -1:
            raise ValueError(f"Text not found: {containing!r}")

        # Find the start of the comment
        comment_start = self.content.rfind("/*", 0, idx)
        if comment_start == -1:
            raise ValueError(f"No comment block found containing: {containing!r}")

        # Find the end of the comment
        comment_end = self.content.find("*/", idx)
        if comment_end == -1:
            raise ValueError(f"Unterminated comment block containing: {containing!r}")

        return comment_start, comment_end + 2

    def extract_region(self, start_pattern: str, end_pattern: str) -> str:
        """Extract a region matching regex patterns.

        Args:
            start_pattern: Regex pattern for the start of the region.
            end_pattern: Regex pattern for the end of the region.

        Returns:
            The extracted region including the matched patterns.

        Raises:
            ValueError: If patterns are not found.
        """
        start_match = re.search(start_pattern, self.content)
        if not start_match:
            raise ValueError(f"Start pattern not found: {start_pattern}")

        end_match = re.search(end_pattern, self.content[start_match.end() :])
        if not end_match:
            raise ValueError(f"End pattern not found: {end_pattern}")

        end_pos = start_match.end() + end_match.end()
        return self.content[start_match.start() : end_pos]
