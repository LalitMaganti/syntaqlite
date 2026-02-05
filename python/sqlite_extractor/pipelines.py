# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Reusable transform pipelines for SQLite extraction.

This module provides factory functions for creating commonly-used
transform pipelines. These eliminate duplication between extract_sqlite.py
and build_extension_grammar.py.
"""

from __future__ import annotations

from .transforms import Pipeline, SymbolRenameExact
from .constants import KEYWORDHASH_ARRAY_SYMBOLS


def create_parser_symbol_rename_pipeline(prefix: str) -> Pipeline:
    """Create a pipeline for renaming Lemon-generated parser symbols.

    This pipeline renames the public API symbols (Parse*, etc.) and
    internal symbols (yyParser, yyStackEntry) to use the specified prefix.

    Args:
        prefix: Symbol prefix (e.g., "syntaqlite").

    Returns:
        Pipeline that renames all parser symbols.

    Example:
        >>> pipeline = create_parser_symbol_rename_pipeline("syntaqlite")
        >>> # Transforms "Parse" -> "syntaqlite_sqlite3Parser"
        >>> # Transforms "yyParser" -> "syntaqlite_yyParser"
    """
    sqlite3_prefix = f"{prefix}_sqlite3"

    return Pipeline([
        # Public API symbols: Parse -> {prefix}_sqlite3Parser
        SymbolRenameExact("Parse", f"{sqlite3_prefix}Parser"),
        SymbolRenameExact("ParseAlloc", f"{sqlite3_prefix}ParserAlloc"),
        SymbolRenameExact("ParseFree", f"{sqlite3_prefix}ParserFree"),
        SymbolRenameExact("ParseInit", f"{sqlite3_prefix}ParserInit"),
        SymbolRenameExact("ParseFinalize", f"{sqlite3_prefix}ParserFinalize"),
        SymbolRenameExact("ParseTrace", f"{sqlite3_prefix}ParserTrace"),
        SymbolRenameExact("ParseFallback", f"{sqlite3_prefix}ParserFallback"),
        SymbolRenameExact("ParseCoverage", f"{sqlite3_prefix}ParserCoverage"),
        SymbolRenameExact("ParseStackPeak", f"{sqlite3_prefix}ParserStackPeak"),
        # Internal symbols: yyParser -> {prefix}_yyParser
        SymbolRenameExact("yyParser", f"{prefix}_yyParser"),
        SymbolRenameExact("yyStackEntry", f"{prefix}_yyStackEntry"),
    ])


def create_keywordhash_rename_pipeline(prefix: str) -> Pipeline:
    """Create a pipeline for renaming keywordhash array symbols.

    This pipeline renames the six keywordhash arrays (zKWText, aKWHash, etc.)
    to use the specified prefix.

    Args:
        prefix: Symbol prefix (e.g., "syntaqlite").

    Returns:
        Pipeline that renames keywordhash array symbols.

    Example:
        >>> pipeline = create_keywordhash_rename_pipeline("syntaqlite")
        >>> # Transforms "zKWText" -> "syntaqlite_zKWText"
    """
    return Pipeline([
        SymbolRenameExact(symbol, f"{prefix}_{symbol}")
        for symbol in KEYWORDHASH_ARRAY_SYMBOLS
    ])
