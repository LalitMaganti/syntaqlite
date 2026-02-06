# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""DML (INSERT/UPDATE/DELETE) AST node definitions."""

from ..defs import Node, List, Enum, inline, index

ENUMS = [
    # Conflict resolution for INSERT OR .../UPDATE OR ...
    # Matches SQLite OE_ constants: Default=0, Rollback=1, Abort=2, Fail=3, Ignore=4, Replace=5
    Enum("ConflictAction",
        "DEFAULT",    # 0 - no OR clause
        "ROLLBACK",   # 1
        "ABORT",      # 2
        "FAIL",       # 3
        "IGNORE",     # 4
        "REPLACE",    # 5
    ),
]

NODES = [
    # DELETE FROM [schema.]table [WHERE expr]
    Node("DeleteStmt",
        table=index("Expr"),               # xfullname -> TableRef
        where=index("Expr"),               # nullable - WHERE clause
    ),

    # SET clause assignment: column = expr (or (col1, col2) = expr)
    Node("SetClause",
        column=inline("SyntaqliteSourceSpan"),  # column name (for simple nm = expr)
        columns=index("ExprList"),              # nullable - for (col1, col2) = expr
        value=index("Expr"),                    # right-hand side expression
    ),

    # List of SET clause assignments
    List("SetClauseList", child_type="SetClause"),

    # UPDATE [OR conflict] [schema.]table SET ... [FROM ...] [WHERE expr]
    Node("UpdateStmt",
        conflict_action=inline("ConflictAction"),
        table=index("Expr"),               # xfullname -> TableRef
        setlist=index("SetClauseList"),    # SET assignments
        from_clause=index("Expr"),         # nullable - FROM clause
        where=index("Expr"),               # nullable - WHERE clause
    ),

    # INSERT [OR conflict] INTO [schema.]table [(columns)] source
    # source is either a select (including VALUES) or null for DEFAULT VALUES
    Node("InsertStmt",
        conflict_action=inline("ConflictAction"),
        table=index("Expr"),               # xfullname -> TableRef
        columns=index("ExprList"),         # nullable - column list
        source=index("Stmt"),              # nullable - select/values or null for DEFAULT VALUES
    ),
]
