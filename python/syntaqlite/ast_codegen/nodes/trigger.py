# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CREATE TRIGGER and CREATE VIRTUAL TABLE AST node definitions.

Covers: CREATE TRIGGER with trigger body commands (UPDATE/INSERT/DELETE/SELECT),
trigger timing, trigger events, CREATE VIRTUAL TABLE with module arguments.
"""

from ..defs import Node, List, Enum, inline, index

ENUMS = [
    # Trigger timing: BEFORE, AFTER, INSTEAD OF
    Enum("TriggerTiming",
        "BEFORE",      # 0
        "AFTER",       # 1
        "INSTEAD_OF",  # 2
    ),

    # Trigger event type
    Enum("TriggerEventType",
        "DELETE",   # 0
        "INSERT",   # 1
        "UPDATE",   # 2
    ),
]

NODES = [
    # Trigger event: event type + optional column list (for UPDATE OF)
    Node("TriggerEvent",
        event_type=inline("TriggerEventType"),
        columns=index("ExprList"),  # nullable - only for UPDATE OF col1, col2
    ),

    # List of trigger body commands (each child is a DML/select statement)
    List("TriggerCmdList", child_type="Stmt"),

    # CREATE [TEMP] TRIGGER [IF NOT EXISTS] [schema.]name
    # BEFORE|AFTER|INSTEAD OF DELETE|INSERT|UPDATE [OF columns] ON table
    # [FOR EACH ROW] [WHEN expr]
    # BEGIN trigger_cmd_list END
    Node("CreateTriggerStmt",
        trigger_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        is_temp=inline("u8"),
        if_not_exists=inline("u8"),
        timing=inline("TriggerTiming"),
        event=index("TriggerEvent"),
        table=index("Expr"),          # fullname -> QualifiedName
        when_expr=index("Expr"),      # nullable
        body=index("TriggerCmdList"),
    ),

    # CREATE VIRTUAL TABLE [IF NOT EXISTS] [schema.]name USING module[(args)]
    Node("CreateVirtualTableStmt",
        table_name=inline("SyntaqliteSourceSpan"),
        schema=inline("SyntaqliteSourceSpan"),
        module_name=inline("SyntaqliteSourceSpan"),
        if_not_exists=inline("u8"),
        has_args=inline("u8"),
    ),
]
