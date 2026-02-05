# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Transform pipeline for SQLite code extraction."""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from typing import Protocol, runtime_checkable

from .c_tokenizer import (
    rename_symbol_exact,
    rename_symbols_safe,
    remove_function_calls,
)


@runtime_checkable
class Transform(Protocol):
    """Protocol for content transformations."""

    def apply(self, content: str) -> str:
        """Apply the transformation to content."""
        ...


@dataclass
class Pipeline:
    """A sequence of transforms to apply to content.

    Example:
        pipeline = Pipeline([
            TruncateAt("marker text"),
            ReplaceText("old", "new"),
            SymbolRename("sqlite3", "my_sqlite3"),
        ])
        result = pipeline.apply(source_content)
    """

    transforms: list[Transform] = field(default_factory=list)

    def apply(self, content: str) -> str:
        """Apply all transforms in sequence."""
        for transform in self.transforms:
            content = transform.apply(content)
        return content

    def add(self, transform: Transform) -> Pipeline:
        """Add a transform and return self for chaining."""
        self.transforms.append(transform)
        return self


@dataclass
class ReplaceText(Transform):
    """Simple text replacement transform."""

    old: str
    new: str
    count: int = -1  # -1 means replace all

    def apply(self, content: str) -> str:
        if self.count == -1:
            return content.replace(self.old, self.new)
        return content.replace(self.old, self.new, self.count)


@dataclass
class ReplaceRegex(Transform):
    """Regex-based text replacement."""

    pattern: str
    replacement: str
    flags: int = 0

    def apply(self, content: str) -> str:
        return re.sub(self.pattern, self.replacement, content, flags=self.flags)


@dataclass
class TruncateAt(Transform):
    """Truncate content at a marker.

    Removes everything from the marker onwards.
    """

    marker: str
    include_marker: bool = False

    def apply(self, content: str) -> str:
        idx = content.find(self.marker)
        if idx == -1:
            return content  # Marker not found, return unchanged
        if self.include_marker:
            return content[: idx + len(self.marker)]
        return content[:idx]


@dataclass
class TruncateAfter(Transform):
    """Truncate content after a marker.

    Keeps the marker and removes everything after.
    """

    marker: str

    def apply(self, content: str) -> str:
        idx = content.find(self.marker)
        if idx == -1:
            return content
        return content[: idx + len(self.marker)]


@dataclass
class RemoveLines(Transform):
    """Remove lines matching a pattern."""

    pattern: str
    flags: int = 0

    def apply(self, content: str) -> str:
        regex = re.compile(self.pattern, self.flags)
        lines = content.splitlines(keepends=True)
        return "".join(line for line in lines if not regex.search(line))


@dataclass
class RemoveRegex(Transform):
    """Remove text matching a regex pattern."""

    pattern: str
    flags: int = 0

    def apply(self, content: str) -> str:
        return re.sub(self.pattern, "", content, flags=self.flags)


@dataclass
class SymbolRename(Transform):
    """Safe symbol renaming using C tokenizer.

    Unlike regex-based renaming, this only renames actual identifiers
    and preserves text in comments and string literals.
    """

    old_prefix: str
    new_prefix: str
    whole_word: bool = True

    def apply(self, content: str) -> str:
        return rename_symbols_safe(
            content,
            self.old_prefix,
            self.new_prefix,
            whole_word=self.whole_word,
        )


@dataclass
class SymbolRenameExact(Transform):
    """Rename an exact symbol name."""

    old_name: str
    new_name: str

    def apply(self, content: str) -> str:
        return rename_symbol_exact(content, self.old_name, self.new_name)


@dataclass
class RemoveFunctionCalls(Transform):
    """Remove all calls to a specific function."""

    function_name: str

    def apply(self, content: str) -> str:
        return remove_function_calls(content, self.function_name)


@dataclass
class PrependText(Transform):
    """Prepend text to content."""

    text: str

    def apply(self, content: str) -> str:
        return self.text + content


@dataclass
class AppendText(Transform):
    """Append text to content."""

    text: str

    def apply(self, content: str) -> str:
        return content + self.text


@dataclass
class StripBlessingComment(Transform):
    """Remove SQLite's blessing comment from the start of content.

    SQLite files start with a comment containing "May you share freely".
    This transform finds and removes that entire comment block.
    """

    def apply(self, content: str) -> str:
        marker = "May you share freely"
        idx = content.find(marker)
        if idx == -1:
            return content

        # Find end of comment
        end = content.find("*/", idx)
        if end == -1:
            return content

        # Skip past the comment and any trailing whitespace
        end += 2
        while end < len(content) and content[end] in " \t\n":
            end += 1

        return content[end:]


@dataclass
class ExtractBetween(Transform):
    """Extract content between two markers, discarding the rest."""

    start: str
    end: str | None = None
    include_start: bool = False
    include_end: bool = False

    def apply(self, content: str) -> str:
        start_idx = content.find(self.start)
        if start_idx == -1:
            raise ValueError(f"Start marker not found: {self.start!r}")

        if self.include_start:
            extract_start = start_idx
        else:
            extract_start = start_idx + len(self.start)

        if self.end is None:
            return content[extract_start:]

        end_idx = content.find(self.end, start_idx + len(self.start))
        if end_idx == -1:
            raise ValueError(f"End marker not found: {self.end!r}")

        if self.include_end:
            extract_end = end_idx + len(self.end)
        else:
            extract_end = end_idx

        return content[extract_start:extract_end]


@dataclass
class ConditionalTransform(Transform):
    """Apply a transform only if a condition is met."""

    condition: callable  # Takes content, returns bool
    transform: Transform

    def apply(self, content: str) -> str:
        if self.condition(content):
            return self.transform.apply(content)
        return content
