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
]

# ============================================================================
# Nodes
# ============================================================================

NODES = [
    # Binary expression: left OP right
    Node("BinaryExpr",
        op=inline("u8"),             # BINARY_OP_* code
        left=index("Expr"),          # Left operand (cyclic - Expr can contain BinaryExpr)
        right=index("Expr"),         # Right operand (cyclic)
    ),

    # Unary expression: OP operand
    Node("UnaryExpr",
        op=inline("u8"),             # UNARY_OP_* code
        operand=index("Expr"),       # Operand (cyclic)
    ),

    # Literal value (integer, float, string, blob, null)
    # Uses SyntaqliteSourceSpan for source location
    Node("Literal",
        literal_type=inline("u8"),         # SyntaqliteLiteralType
        source=inline("SyntaqliteSourceSpan"),  # Location in source text
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
