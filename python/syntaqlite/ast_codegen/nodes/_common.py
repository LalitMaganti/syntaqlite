# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Common enum definitions shared across AST node submodules."""

from ..defs import Enum

ENUMS = [
    # Literal value types
    Enum("LiteralType",
        "INTEGER",   # 0
        "FLOAT",     # 1
        "STRING",    # 2
        "BLOB",      # 3
        "NULL",      # 4
    ),

    # Binary operators
    Enum("BinaryOp",
        "PLUS",      # 0
        "MINUS",     # 1
        "STAR",      # 2
        "SLASH",     # 3
        "REM",       # 4
        "LT",        # 5
        "GT",        # 6
        "LE",        # 7
        "GE",        # 8
        "EQ",        # 9
        "NE",        # 10
        "AND",       # 11
        "OR",        # 12
        "BITAND",    # 13
        "BITOR",     # 14
        "LSHIFT",    # 15
        "RSHIFT",    # 16
        "CONCAT",    # 17
    ),

    # Unary operators
    Enum("UnaryOp",
        "MINUS",     # 0 (negation)
        "PLUS",      # 1 (no-op)
        "BITNOT",    # 2 (~)
        "NOT",       # 3
    ),
]

NODES = []
