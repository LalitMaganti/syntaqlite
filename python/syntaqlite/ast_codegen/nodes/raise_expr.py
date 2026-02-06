# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""RAISE expression AST node definition (used in triggers)."""

from ..defs import Node, Enum, inline, index
from ..fmt_dsl import seq, kw, child, if_set, switch

ENUMS = [
    Enum("RaiseType",
        "IGNORE",    # 0
        "ROLLBACK",  # 1
        "ABORT",     # 2
        "FAIL",      # 3
    ),
]

NODES = [
    # RAISE expression: RAISE(IGNORE) or RAISE(ROLLBACK|ABORT|FAIL, error_message)
    Node("RaiseExpr",
        raise_type=inline("RaiseType"),
        error_message=index("Expr"),
        fmt=seq(
            kw("RAISE("),
            switch("raise_type", {
                "IGNORE": kw("IGNORE"),
                "ROLLBACK": kw("ROLLBACK"),
                "ABORT": kw("ABORT"),
                "FAIL": kw("FAIL"),
            }),
            if_set("error_message", seq(kw(", "), child("error_message"))),
            kw(")"),
        ),
    ),
]
