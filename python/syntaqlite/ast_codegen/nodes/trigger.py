# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""CREATE TRIGGER and CREATE VIRTUAL TABLE AST node definitions.

Covers: CREATE TRIGGER with trigger body commands (UPDATE/INSERT/DELETE/SELECT),
trigger timing, trigger events, CREATE VIRTUAL TABLE with module arguments.
"""

from ..defs import Node, List, Enum, inline, index
from ..fmt_dsl import (
    seq, kw, span, child, hardline, nest,
    if_set, if_flag, if_span, switch, for_each_child,
)

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
        fmt=switch("event_type", {
            "DELETE": kw("DELETE"),
            "INSERT": kw("INSERT"),
            "UPDATE": seq(kw("UPDATE"), if_set("columns", seq(kw(" OF "), child("columns")))),
        }),
    ),

    # List of trigger body commands (each child is a DML/select statement)
    List("TriggerCmdList", child_type="Stmt",
        fmt=for_each_child(seq(child("_item"), kw(";")), separator=hardline()),
    ),

    # CREATE [TEMP] TRIGGER [IF NOT EXISTS] [schema.]name
    # BEFORE|AFTER|INSTEAD OF DELETE|INSERT|UPDATE [OF columns] ON table
    # [FOR EACH ROW] [WHEN expr]
    # BEGIN trigger_cmd_list END
    Node("CreateTriggerStmt",
        trigger_name=inline("SynqSourceSpan"),
        schema=inline("SynqSourceSpan"),
        is_temp=inline("Bool"),
        if_not_exists=inline("Bool"),
        timing=inline("TriggerTiming"),
        event=index("TriggerEvent"),
        table=index("Expr"),          # fullname -> QualifiedName
        when_expr=index("Expr"),      # nullable
        body=index("TriggerCmdList"),
        fmt=seq(
            kw("CREATE"),
            if_flag("is_temp", kw(" TEMP")),
            kw(" TRIGGER"),
            if_flag("if_not_exists", kw(" IF NOT EXISTS")),
            kw(" "),
            if_span("schema", seq(span("schema"), kw("."))),
            span("trigger_name"),
            kw(" "),
            switch("timing", {
                "BEFORE": kw("BEFORE"),
                "AFTER": kw("AFTER"),
                "INSTEAD_OF": kw("INSTEAD OF"),
            }),
            kw(" "), child("event"),
            kw(" ON "), child("table"),
            if_set("when_expr", seq(hardline(), kw("WHEN "), child("when_expr"))),
            hardline(), kw("BEGIN"),
            if_set("body", nest(seq(hardline(), child("body")))),
            hardline(), kw("END"),
        ),
    ),

    # CREATE VIRTUAL TABLE [IF NOT EXISTS] [schema.]name USING module[(args)]
    Node("CreateVirtualTableStmt",
        table_name=inline("SynqSourceSpan"),
        schema=inline("SynqSourceSpan"),
        module_name=inline("SynqSourceSpan"),
        if_not_exists=inline("Bool"),
        module_args=inline("SynqSourceSpan"),
        fmt=seq(
            kw("CREATE VIRTUAL TABLE"),
            if_flag("if_not_exists", kw(" IF NOT EXISTS")),
            kw(" "),
            if_span("schema", seq(span("schema"), kw("."))),
            span("table_name"),
            kw(" USING "), span("module_name"),
            if_span("module_args", seq(kw("("), span("module_args"), kw(")"))),
        ),
    ),
]
