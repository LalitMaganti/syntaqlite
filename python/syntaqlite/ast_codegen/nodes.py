# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""AST node type definitions for syntaqlite.

Design rules:
- Non-cyclic data -> inline: Scalars, flags, source offsets/lengths
- Cyclic references -> index: When type A can contain type B which can contain A
- No pointers: Only indices (node IDs) and offsets (into source text)
- Variable sizes: Each node type uses exactly the space it needs

For nullable index fields, use SYNTAQLITE_NULL_NODE (0xFFFFFFFF).
"""

from .defs import Node, List, Enum, inline, index

# ============================================================================
# Enums
# ============================================================================

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

# ============================================================================
# Nodes
# ============================================================================

NODES = [
    # Binary expression: left OP right
    Node("BinaryExpr",
        op=inline("BinaryOp"),
        left=index("Expr"),
        right=index("Expr"),
    ),

    # Unary expression: OP operand
    Node("UnaryExpr",
        op=inline("UnaryOp"),
        operand=index("Expr"),
    ),

    # Literal value (integer, float, string, blob, null)
    Node("Literal",
        literal_type=inline("LiteralType"),
        source=inline("SyntaqliteSourceSpan"),
    ),

    # Result column: expr [AS alias] or *
    Node("ResultColumn",
        flags=inline("u8"),                # Is star, has alias, etc.
        alias=inline("SyntaqliteSourceSpan"),  # Alias (empty if none)
        expr=index("Expr"),                # Expression (nullable for table.*)
    ),

    # List of result columns
    List("ResultColumnList", child_type="ResultColumn"),

    # SELECT statement (minimal version)
    Node("SelectStmt",
        flags=inline("u8"),          # DISTINCT, ALL
        columns=index("ResultColumnList"), # Result columns
    ),
]
