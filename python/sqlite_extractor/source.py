# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Source file handling for SQLite code extraction."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass
class SourceFile:
    """Represents a source file with extraction utilities."""

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
