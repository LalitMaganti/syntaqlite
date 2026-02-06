# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""RAISE expression AST node definition (used in triggers)."""

from ..defs import Node, Enum, inline, index

ENUMS = [
    # RAISE types (matches SQLite's OE_ constants conceptually)
    Enum("RaiseType",
        "IGNORE",    # 0 - RAISE(IGNORE)
        "ROLLBACK",  # 1 - RAISE(ROLLBACK, msg)
        "ABORT",     # 2 - RAISE(ABORT, msg)
        "FAIL",      # 3 - RAISE(FAIL, msg)
    ),
]

NODES = [
    # RAISE expression: RAISE(IGNORE) or RAISE(ROLLBACK|ABORT|FAIL, error_message)
    Node("RaiseExpr",
        raise_type=inline("RaiseType"),
        error_message=index("Expr"),   # nullable - not present for IGNORE
    ),
]
