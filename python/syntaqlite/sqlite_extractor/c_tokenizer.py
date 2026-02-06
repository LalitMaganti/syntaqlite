# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Simple C tokenizer for safe symbol renaming.

This module provides a basic C tokenizer that distinguishes between
identifiers, strings, and comments. This allows safe symbol renaming
that won't accidentally modify text in comments or string literals.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from enum import Enum, auto
from typing import Iterator


class TokenType(Enum):
    """Types of C tokens recognized by the tokenizer."""

    IDENTIFIER = auto()  # Variable/function names
    STRING = auto()  # String literals "..."
    CHAR = auto()  # Character literals '...'
    LINE_COMMENT = auto()  # // comments
    BLOCK_COMMENT = auto()  # /* ... */ comments
    WHITESPACE = auto()  # Spaces, tabs, newlines
    OTHER = auto()  # Operators, punctuation, numbers, etc.


@dataclass
class Token:
    """A token from C source code."""

    type: TokenType
    value: str
    start: int  # Position in original string
    end: int


def tokenize_c(content: str) -> Iterator[Token]:
    """Tokenize C code for safe text manipulation.

    Focuses on correctly identifying identifiers, string/char literals,
    and comments. Sufficient for safe symbol renaming operations.
    """
    n = len(content)
    i = 0

    def scan_quoted(quote: str) -> int:
        """Scan a quoted literal, returning end position."""
        pos = i + 1
        while pos < n:
            if content[pos] == "\\":
                pos += 2
            elif content[pos] == quote:
                return pos + 1
            elif content[pos] == "\n":
                return pos
            else:
                pos += 1
        return pos

    while i < n:
        start = i
        ch = content[i]

        # Whitespace
        if ch in " \t\n\r":
            while i < n and content[i] in " \t\n\r":
                i += 1
            yield Token(TokenType.WHITESPACE, content[start:i], start, i)

        # Line comment
        elif content[i:i + 2] == "//":
            i += 2
            while i < n and content[i] != "\n":
                i += 1
            yield Token(TokenType.LINE_COMMENT, content[start:i], start, i)

        # Block comment
        elif content[i:i + 2] == "/*":
            i += 2
            while i < n - 1 and content[i:i + 2] != "*/":
                i += 1
            if i < n - 1:
                i += 2
            yield Token(TokenType.BLOCK_COMMENT, content[start:i], start, i)

        # String or char literal
        elif ch in '"\'':
            i = scan_quoted(ch)
            tok_type = TokenType.STRING if ch == '"' else TokenType.CHAR
            yield Token(tok_type, content[start:i], start, i)

        # Identifier
        elif ch.isalpha() or ch == "_":
            while i < n and (content[i].isalnum() or content[i] == "_"):
                i += 1
            yield Token(TokenType.IDENTIFIER, content[start:i], start, i)

        # Number
        elif ch.isdigit():
            if ch == "0" and i + 1 < n and content[i + 1] in "xX":
                i += 2
                while i < n and content[i] in "0123456789abcdefABCDEF":
                    i += 1
            else:
                while i < n and content[i] in "0123456789.eEfFlLuU+-":
                    if content[i] in "+-" and content[i - 1] not in "eE":
                        break
                    i += 1
            yield Token(TokenType.OTHER, content[start:i], start, i)

        # Everything else
        else:
            i += 1
            yield Token(TokenType.OTHER, content[start:i], start, i)


def rename_symbols_safe(
    content: str,
    old_prefix: str,
    new_prefix: str,
    *,
    whole_word: bool = True,
) -> str:
    """Rename symbols while preserving comments and strings.

    This function tokenizes the C code and only renames identifiers,
    leaving string literals and comments untouched.

    Args:
        content: The C source code.
        old_prefix: The prefix to search for (e.g., "sqlite3").
        new_prefix: The replacement prefix (e.g., "synq_sqlite3").
        whole_word: Only match at word boundaries (default True).

    Returns:
        The modified source code with renamed symbols.

    Example:
        >>> code = 'sqlite3Init(); // calls sqlite3Init'
        >>> rename_symbols_safe(code, "sqlite3", "my_sqlite3")
        'my_sqlite3Init(); // calls sqlite3Init'
    """
    def rename_identifier(value: str) -> str:
        if whole_word:
            if value.startswith(old_prefix):
                return new_prefix + value[len(old_prefix):]
            return value
        return value.replace(old_prefix, new_prefix)

    return "".join(
        rename_identifier(tok.value) if tok.type == TokenType.IDENTIFIER else tok.value
        for tok in tokenize_c(content)
    )


def rename_symbol_exact(content: str, old_name: str, new_name: str) -> str:
    """Rename an exact symbol while preserving comments and strings.

    Unlike rename_symbols_safe which does prefix matching, this function
    only renames exact matches of the symbol name.

    Args:
        content: The C source code.
        old_name: The exact symbol name to replace.
        new_name: The new symbol name.

    Returns:
        The modified source code.
    """
    result_parts: list[str] = []

    for token in tokenize_c(content):
        if token.type == TokenType.IDENTIFIER and token.value == old_name:
            result_parts.append(new_name)
        else:
            result_parts.append(token.value)

    return "".join(result_parts)


def remove_function_calls(content: str, function_name: str) -> str:
    """Remove all calls to a specific function, including the entire line.

    This is useful for removing debugging macros like testcase().

    Args:
        content: The C source code.
        function_name: The name of the function to remove.

    Returns:
        The modified source code with function calls removed.
    """
    # Remove the entire line containing the function call
    # Pattern: optional leading whitespace, function call, semicolon, rest of line, newline
    # Uses [ \t]* instead of \s* to not match newlines at the start
    pattern = rf"[ \t]*{re.escape(function_name)}\s*\([^)]*\)\s*;[^\n]*\n"
    return re.sub(pattern, "", content)
