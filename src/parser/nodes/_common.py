# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Common enum definitions shared across AST node submodules."""

from python.syntaqlite.ast_codegen.defs import Enum

ENUMS = [
    # Boolean (for fields that are 0/1)
    Enum("Bool",
        "FALSE",     # 0
        "TRUE",      # 1
    ),

    # Literal value types
    Enum("LiteralType",
        "INTEGER",   # 0
        "FLOAT",     # 1
        "STRING",    # 2
        "BLOB",      # 3
        "NULL",      # 4
        "CURRENT",   # 5 - CURRENT_TIME, CURRENT_DATE, CURRENT_TIMESTAMP
        "QNUMBER",   # 6 - Quoted number literal
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
        "PTR",       # 18 - JSON pointer operator (->)
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
