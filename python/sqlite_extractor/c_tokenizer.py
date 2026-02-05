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

    This is a simplified tokenizer that focuses on correctly identifying:
    - Identifiers (for symbol renaming)
    - String and character literals (to preserve)
    - Comments (to preserve)

    It doesn't handle all C syntax perfectly but is sufficient for
    safe symbol renaming operations.

    Args:
        content: The C source code to tokenize.

    Yields:
        Token objects for each token found.
    """
    i = 0
    n = len(content)

    while i < n:
        # Skip whitespace
        if content[i] in " \t\n\r":
            start = i
            while i < n and content[i] in " \t\n\r":
                i += 1
            yield Token(TokenType.WHITESPACE, content[start:i], start, i)
            continue

        # Line comment
        if content[i : i + 2] == "//":
            start = i
            i += 2
            while i < n and content[i] != "\n":
                i += 1
            yield Token(TokenType.LINE_COMMENT, content[start:i], start, i)
            continue

        # Block comment
        if content[i : i + 2] == "/*":
            start = i
            i += 2
            while i < n - 1 and content[i : i + 2] != "*/":
                i += 1
            if i < n - 1:
                i += 2  # Include closing */
            yield Token(TokenType.BLOCK_COMMENT, content[start:i], start, i)
            continue

        # String literal
        if content[i] == '"':
            start = i
            i += 1
            while i < n:
                if content[i] == "\\":
                    i += 2  # Skip escape sequence
                elif content[i] == '"':
                    i += 1
                    break
                elif content[i] == "\n":
                    # Unterminated string, stop here
                    break
                else:
                    i += 1
            yield Token(TokenType.STRING, content[start:i], start, i)
            continue

        # Character literal
        if content[i] == "'":
            start = i
            i += 1
            while i < n:
                if content[i] == "\\":
                    i += 2  # Skip escape sequence
                elif content[i] == "'":
                    i += 1
                    break
                elif content[i] == "\n":
                    # Unterminated char, stop here
                    break
                else:
                    i += 1
            yield Token(TokenType.CHAR, content[start:i], start, i)
            continue

        # Identifier (starts with letter or underscore)
        if content[i].isalpha() or content[i] == "_":
            start = i
            while i < n and (content[i].isalnum() or content[i] == "_"):
                i += 1
            yield Token(TokenType.IDENTIFIER, content[start:i], start, i)
            continue

        # Number (including hex, octal, float)
        if content[i].isdigit():
            start = i
            # Handle hex
            if content[i] == "0" and i + 1 < n and content[i + 1] in "xX":
                i += 2
                while i < n and (content[i].isdigit() or content[i] in "abcdefABCDEF"):
                    i += 1
            else:
                while i < n and (content[i].isdigit() or content[i] in ".eEfFlLuU+-"):
                    # Check for valid float/exponent notation
                    if content[i] in "+-" and i > 0 and content[i - 1] not in "eE":
                        break
                    i += 1
            yield Token(TokenType.OTHER, content[start:i], start, i)
            continue

        # Everything else (operators, punctuation)
        yield Token(TokenType.OTHER, content[i], i, i + 1)
        i += 1


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
        new_prefix: The replacement prefix (e.g., "syntaqlite_sqlite3").
        whole_word: Only match at word boundaries (default True).

    Returns:
        The modified source code with renamed symbols.

    Example:
        >>> code = 'sqlite3Init(); // calls sqlite3Init'
        >>> rename_symbols_safe(code, "sqlite3", "my_sqlite3")
        'my_sqlite3Init(); // calls sqlite3Init'
    """
    result_parts: list[str] = []

    for token in tokenize_c(content):
        if token.type == TokenType.IDENTIFIER:
            # Only rename identifiers
            if whole_word:
                # Check if the identifier starts with the prefix
                if token.value.startswith(old_prefix):
                    result_parts.append(new_prefix + token.value[len(old_prefix) :])
                else:
                    result_parts.append(token.value)
            else:
                result_parts.append(token.value.replace(old_prefix, new_prefix))
        else:
            # Preserve everything else unchanged
            result_parts.append(token.value)

    return "".join(result_parts)


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
