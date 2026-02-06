#!/usr/bin/env python3
# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Check formatter coverage: which nodes have fmt annotations and tests."""

import importlib
import os
import re
import sys
from pathlib import Path

ROOT_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(ROOT_DIR))

from python.syntaqlite.ast_codegen.defs import NodeDef, ListDef


# Mapping from node name to SQL keywords that indicate the node is exercised.
# Nodes not listed here are either always exercised by their parent (lists,
# internal nodes) or require manual annotation review.
_NODE_SQL_HINTS = {
    # Statements
    "SelectStmt": [r"\bSELECT\b"],
    "DeleteStmt": [r"\bDELETE\b"],
    "UpdateStmt": [r"\bUPDATE\b"],
    "InsertStmt": [r"\bINSERT\b", r"\bREPLACE\b"],
    "CreateTableStmt": [r"\bCREATE\s+TABLE\b", r"\bCREATE\s+TEMP\s+TABLE\b"],
    "CreateIndexStmt": [r"\bCREATE\s+(UNIQUE\s+)?INDEX\b"],
    "CreateViewStmt": [r"\bCREATE\s+(TEMP\s+)?VIEW\b"],
    "CreateTriggerStmt": [r"\bCREATE\s+TRIGGER\b"],
    "CreateVirtualTableStmt": [r"\bCREATE\s+VIRTUAL\s+TABLE\b"],
    "DropStmt": [r"\bDROP\s+(TABLE|INDEX|VIEW|TRIGGER)\b"],
    "AlterTableStmt": [r"\bALTER\s+TABLE\b"],
    "TransactionStmt": [r"\b(BEGIN|COMMIT|ROLLBACK|END)\b"],
    "SavepointStmt": [r"\bSAVEPOINT\b", r"\bRELEASE\b"],
    "PragmaStmt": [r"\bPRAGMA\b"],
    "AnalyzeStmt": [r"\bANALYZE\b", r"\bREINDEX\b"],
    "AttachStmt": [r"\bATTACH\b"],
    "DetachStmt": [r"\bDETACH\b"],
    "VacuumStmt": [r"\bVACUUM\b"],
    "ExplainStmt": [r"\bEXPLAIN\b"],
    "CompoundSelect": [r"\bUNION\b", r"\bINTERSECT\b", r"\bEXCEPT\b"],
    "WithClause": [r"\bWITH\b"],
    "CteDefinition": [r"\bWITH\b"],
    "ValuesClause": [r"\bVALUES\b"],
    # Expressions
    "BinaryExpr": [r"\b\d+\s*[\+\-\*\/\%]\s*\d+", r"\bAND\b", r"\bOR\b"],
    "UnaryExpr": [r"\bNOT\b", r"(?<!\w)-\w"],
    "Literal": [r"\b\d+\b", r"'[^']*'", r"\bNULL\b"],
    "ColumnRef": [r"\bFROM\b"],  # columns typically appear with FROM
    "IsExpr": [r"\bIS\s+(NOT\s+)?NULL\b", r"\bISNULL\b", r"\bNOTNULL\b"],
    "BetweenExpr": [r"\bBETWEEN\b"],
    "LikeExpr": [r"\bLIKE\b", r"\bGLOB\b"],
    "CaseExpr": [r"\bCASE\b"],
    "InExpr": [r"\bIN\s*\("],
    "SubqueryExpr": [r"\(\s*SELECT\b"],
    "ExistsExpr": [r"\bEXISTS\b"],
    "CastExpr": [r"\bCAST\b"],
    "CollateExpr": [r"\bCOLLATE\b"],
    "Variable": [r"[\?\:\@\$]\w*"],
    "RaiseExpr": [r"\bRAISE\b"],
    "FunctionCall": [r"\w+\s*\("],
    "AggregateFunctionCall": [r"\b(COUNT|SUM|AVG|MIN|MAX|GROUP_CONCAT|TOTAL)\s*\("],
    # Table sources / joins
    "TableRef": [r"\bFROM\b"],
    "SubqueryTableSource": [r"\bFROM\s*\("],
    "JoinClause": [r"\bJOIN\b"],
    "JoinPrefix": [r"\bJOIN\b"],
    # Window functions
    "WindowDef": [r"\bOVER\s*\("],
    "FrameSpec": [r"\b(ROWS|RANGE|GROUPS)\b"],
    "FrameBound": [r"\b(ROWS|RANGE|GROUPS)\b"],
    "FilterOver": [r"\bFILTER\s*\("],
    "NamedWindowDef": [r"\bWINDOW\b"],
    # CREATE TABLE internals
    "ColumnDef": [r"\bCREATE\s+TABLE\b"],
    "ColumnConstraint": [r"\bPRIMARY\s+KEY\b", r"\bNOT\s+NULL\b", r"\bUNIQUE\b",
                         r"\bDEFAULT\b", r"\bCHECK\b", r"\bREFERENCES\b"],
    "TableConstraint": [r"\bCONSTRAINT\b", r"\bPRIMARY\s+KEY\s*\("],
    "ForeignKeyClause": [r"\bREFERENCES\b", r"\bFOREIGN\s+KEY\b"],
    # DML internals
    "SetClause": [r"\bSET\b"],
    "QualifiedName": [r"\w+\.\w+"],
    # Trigger internals
    "TriggerEvent": [r"\bCREATE\s+TRIGGER\b"],
    # SELECT internals
    "ResultColumn": [r"\bSELECT\b"],
    "OrderingTerm": [r"\bORDER\s+BY\b"],
    "LimitClause": [r"\bLIMIT\b"],
}

# Nodes that are always formatted inline by their parent (list nodes,
# internal helper nodes). These don't need their own fmt annotation or
# dedicated test — they're covered when the parent is tested.
_INLINE_NODES = {
    "ExprList": "list (comma-separated by parent)",
    "ResultColumnList": "list (comma-separated by parent)",
    "OrderByList": "list (comma-separated by parent)",
    "SetClauseList": "list (comma-separated by parent)",
    "CteList": "list (comma-separated by parent)",
    "NamedWindowDefList": "list (comma-separated by parent)",
    "WindowDefList": "list (comma-separated by parent)",
    "ColumnDefList": "list (comma-separated by parent)",
    "ColumnConstraintList": "list (comma-separated by parent)",
    "TableConstraintList": "list (comma-separated by parent)",
    "TriggerCmdList": "list (comma-separated by parent)",
    "ValuesRowList": "list (comma-separated by parent)",
    "CaseWhenList": "list (inline in CaseExpr)",
    "CaseWhen": "inline in CaseExpr",
    "JoinPrefix": "delegates to JoinClause",
}


def collect_all_nodes():
    """Import and return all node definitions."""
    from python.syntaqlite.ast_codegen.nodes import NODES
    return NODES


def has_fmt_annotation(node_def):
    """Check whether a node definition has a fmt= annotation."""
    return getattr(node_def, "fmt", None) is not None


def collect_test_sql(test_dir: Path) -> list[str]:
    """Collect all SQL strings from fmt diff test files."""
    sqls = []
    if not test_dir.exists():
        return sqls
    for py_file in sorted(test_dir.glob("*.py")):
        if py_file.name.startswith("_"):
            continue
        text = py_file.read_text()
        # Extract sql="..." values from AstTestBlueprint
        for m in re.finditer(r'sql\s*=\s*"([^"]*)"', text):
            sqls.append(m.group(1))
        # Also match triple-quoted sql="""..."""
        for m in re.finditer(r'sql\s*=\s*"""(.*?)"""', text, re.DOTALL):
            sqls.append(m.group(1))
    return sqls


def node_has_test(name: str, all_sql: list[str]) -> bool:
    """Heuristic: check if any test SQL exercises this node type."""
    patterns = _NODE_SQL_HINTS.get(name)
    if not patterns:
        return False
    combined = " ".join(all_sql)
    return any(re.search(p, combined, re.IGNORECASE) for p in patterns)


def main():
    nodes = collect_all_nodes()
    test_dir = ROOT_DIR / "tests" / "fmt_diff_tests"
    all_sql = collect_test_sql(test_dir)

    # Categorize
    rows = []
    for node_def in nodes:
        name = node_def.name
        is_list = isinstance(node_def, ListDef)
        inline_note = _INLINE_NODES.get(name)
        has_fmt = has_fmt_annotation(node_def)
        has_test = node_has_test(name, all_sql)

        if inline_note:
            note = inline_note
        elif is_list:
            note = "list"
        else:
            note = ""

        rows.append((name, has_fmt, has_test, inline_note is not None, note))

    # Print table
    name_w = max(len(r[0]) for r in rows)
    print(f"{'Node':<{name_w}}  fmt?   test?  notes")
    print(f"{'─' * name_w}  ─────  ─────  ─────")

    n_fmt = 0
    n_test = 0
    n_need_fmt = 0  # non-inline nodes needing fmt
    n_need_test = 0

    for name, has_fmt, has_test, is_inline, note in rows:
        fmt_str = "  ✓  " if has_fmt else " —  " if is_inline else "     "
        test_str = "  ✓  " if has_test else " —  " if is_inline else "     "

        if has_fmt:
            n_fmt += 1
        if has_test:
            n_test += 1
        if not is_inline and not has_fmt:
            n_need_fmt += 1
        if not is_inline and not has_test:
            n_need_test += 1

        print(f"{name:<{name_w}}  {fmt_str}  {test_str}  {note}")

    # Summary
    total = len(rows)
    inline_count = sum(1 for r in rows if r[3])
    actionable = total - inline_count
    print()
    print(f"Total nodes: {total}  (inline/delegated: {inline_count}, actionable: {actionable})")
    print(f"With fmt annotation: {n_fmt}/{actionable}")
    print(f"With test coverage:  {n_test}/{actionable}")
    if n_need_fmt:
        print(f"Missing fmt: {n_need_fmt}")
    if n_need_test:
        print(f"Missing test: {n_need_test}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
