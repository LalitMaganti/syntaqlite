# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Execute a single LSP diff-test blueprint against a running LSP server.

The test's SQL carries a `<|>` cursor marker. The marker is stripped, the
document is opened, the requested operation runs at the cursor position, and
the JSON-RPC response is rendered to deterministic text for comparison.
"""

from __future__ import annotations

import time
from dataclasses import dataclass
from typing import Any

from python.dev.diff_tests.lsp_client import LspClient
from python.dev.diff_tests.test_executor import TestResult, normalize_output
from python.dev.diff_tests.testing import LspDiffTestBlueprint

CURSOR_MARKER = "<|>"

# Completion item kinds emitted by the syntaqlite LSP (see lsp/server.rs).
# We tag entries with these names and use them for sort ordering so the
# rendered output is deterministic.
_KIND_NAMES: dict[int, str] = {
    14: "keyword",
    3: "function",
    22: "table",
    5: "column",
}

# Column completions first, then tables, then keywords, then functions
# (matches `CompletionKind::sort_priority` in the Rust side so the rendered
# order is stable across either dispatch path).
_KIND_ORDER: dict[str, int] = {
    "column": 0,
    "table": 1,
    "keyword": 2,
    "function": 3,
}


def _split_cursor(sql: str, required: bool = True) -> tuple[str, int, int]:
    """Strip the cursor marker and return (text, line, character).

    `character` is a 0-indexed column assuming ASCII (one byte per char),
    matching how the LSP source_map converts positions for the tests here.
    When `required` is False and no marker is present, returns the original
    text with position (0, 0); used for ops like `diagnostics` that don't
    need a cursor.
    """
    idx = sql.find(CURSOR_MARKER)
    if idx < 0:
        if required:
            raise ValueError(f"sql missing cursor marker {CURSOR_MARKER!r}")
        return sql, 0, 0
    if sql.find(CURSOR_MARKER, idx + len(CURSOR_MARKER)) >= 0:
        raise ValueError(f"sql contains multiple cursor markers {CURSOR_MARKER!r}")
    before = sql[:idx]
    line = before.count("\n")
    last_nl = before.rfind("\n")
    character = len(before) - (last_nl + 1) if last_nl >= 0 else len(before)
    text = before + sql[idx + len(CURSOR_MARKER):]
    return text, line, character


def _render_hover(result: Any) -> str:
    if result is None:
        return "(no hover)"
    contents = result.get("contents")
    if isinstance(contents, dict):
        # MarkupContent: { kind, value }
        return str(contents.get("value", ""))
    if isinstance(contents, str):
        return contents
    if isinstance(contents, list):
        parts: list[str] = []
        for item in contents:
            if isinstance(item, str):
                parts.append(item)
            elif isinstance(item, dict):
                parts.append(str(item.get("value", "")))
        return "\n".join(parts)
    return str(contents)


def _fmt_range(r: dict[str, Any]) -> str:
    start = r["start"]
    end = r["end"]
    return f"{start['line']}:{start['character']}..{end['line']}:{end['character']}"


def _render_definition(result: Any, own_uri: str) -> str:
    if result is None:
        return "(no definition)"
    locations: list[dict[str, Any]]
    if isinstance(result, list):
        locations = result
    else:
        locations = [result]
    lines: list[str] = []
    for loc in locations:
        uri = loc.get("uri") or loc.get("targetUri", "")
        rng = loc.get("range") or loc.get("targetRange")
        if rng is None:
            continue
        # Omit URI when the target is in the same document (the common case);
        # include it only when go-to-def crosses files.
        if uri == own_uri:
            lines.append(f"target: {_fmt_range(rng)}")
        else:
            lines.append(f"target: {uri}:{_fmt_range(rng)}")
    return "\n".join(lines) if lines else "(no definition)"


def _render_references(result: Any, own_uri: str) -> str:
    if result is None:
        return "(no references)"
    if not isinstance(result, list):
        return "(no references)"
    lines: list[str] = []
    for loc in result:
        uri = loc.get("uri", "")
        rng = loc.get("range")
        if rng is None:
            continue
        if uri == own_uri:
            lines.append(_fmt_range(rng))
        else:
            lines.append(f"{uri}:{_fmt_range(rng)}")
    if not lines:
        return "(no references)"
    lines.sort()
    return "\n".join(lines)


def _render_prepare_rename(result: Any) -> str:
    if result is None:
        return "(no rename)"
    if isinstance(result, dict):
        rng = result.get("range") or result
        placeholder = result.get("placeholder", "")
        if "start" in rng and "end" in rng:
            suffix = f' "{placeholder}"' if placeholder else ""
            return f"{_fmt_range(rng)}{suffix}"
    return "(no rename)"


def _render_rename(result: Any, own_uri: str) -> str:
    if result is None:
        return "(no rename)"
    changes = result.get("changes") if isinstance(result, dict) else None
    if not changes:
        return "(no rename)"
    lines: list[str] = []
    for edit_uri, edits in changes.items():
        for edit in edits:
            rng = edit.get("range")
            new_text = edit.get("newText", "")
            if rng is None:
                continue
            location = (
                _fmt_range(rng)
                if edit_uri == own_uri
                else f"{edit_uri}:{_fmt_range(rng)}"
            )
            lines.append(f'{location} -> "{new_text}"')
    if not lines:
        return "(no rename)"
    lines.sort()
    return "\n".join(lines)


_SEVERITY_NAMES: dict[int, str] = {1: "error", 2: "warning", 3: "info", 4: "hint"}


def _render_diagnostics(diags: list[dict[str, Any]]) -> str:
    if not diags:
        return "(no diagnostics)"
    lines: list[str] = []
    for d in diags:
        sev = _SEVERITY_NAMES.get(d.get("severity", 0), "unknown")
        rng = d.get("range")
        msg = d.get("message", "")
        if rng is None:
            continue
        lines.append(f"{sev} {_fmt_range(rng)}: {msg}")
    lines.sort()
    return "\n".join(lines)


def _render_completion(result: Any) -> str:
    if result is None:
        return "(no completions)"
    items: list[dict[str, Any]]
    if isinstance(result, dict) and "items" in result:
        items = result["items"]
    elif isinstance(result, list):
        items = result
    else:
        items = []
    if not items:
        return "(no completions)"
    rows: list[tuple[int, str, str]] = []
    for item in items:
        kind_name = _KIND_NAMES.get(item.get("kind", 0), "other")
        label = item.get("label", "")
        rows.append((_KIND_ORDER.get(kind_name, 99), kind_name, label))
    rows.sort(key=lambda r: (r[0], r[1], r[2]))
    return "\n".join(f"{kind}: {label}" for _, kind, label in rows)


def _test_uri(name: str) -> str:
    """Deterministic document URI for a test, derived from its name."""
    return f"file:///test/{name.replace('.', '/')}.sql"


def _open_and_run(
    client: LspClient,
    name: str,
    blueprint: LspDiffTestBlueprint,
) -> str:
    """Open a fresh document for this test, run the op, render output."""
    cursor_required = blueprint.op != "diagnostics"
    text, line, character = _split_cursor(blueprint.sql, required=cursor_required)
    uri = _test_uri(name)

    client.send_notification(
        "textDocument/didOpen",
        {
            "textDocument": {
                "uri": uri,
                "languageId": "sql",
                "version": 1,
                "text": text,
            },
        },
    )

    position = {"line": line, "character": character}
    op = blueprint.op

    if op == "hover":
        result = client.send_request(
            "textDocument/hover",
            {"textDocument": {"uri": uri}, "position": position},
        )
        rendered = _render_hover(result)
    elif op == "definition":
        result = client.send_request(
            "textDocument/definition",
            {"textDocument": {"uri": uri}, "position": position},
        )
        rendered = _render_definition(result, uri)
    elif op == "completion":
        result = client.send_request(
            "textDocument/completion",
            {"textDocument": {"uri": uri}, "position": position},
        )
        rendered = _render_completion(result)
    elif op == "references":
        result = client.send_request(
            "textDocument/references",
            {
                "textDocument": {"uri": uri},
                "position": position,
                "context": {"includeDeclaration": blueprint.include_declaration},
            },
        )
        rendered = _render_references(result, uri)
    elif op == "prepare-rename":
        result = client.send_request(
            "textDocument/prepareRename",
            {"textDocument": {"uri": uri}, "position": position},
        )
        rendered = _render_prepare_rename(result)
    elif op == "rename":
        if blueprint.new_name is None:
            raise ValueError("rename op requires new_name on the blueprint")
        result = client.send_request(
            "textDocument/rename",
            {
                "textDocument": {"uri": uri},
                "position": position,
                "newName": blueprint.new_name,
            },
        )
        rendered = _render_rename(result, uri)
    elif op == "diagnostics":
        # Collect the publishDiagnostics notification that the server sends
        # after didOpen. The cursor position is ignored for this op.
        # Filter by URI so stale notifications from earlier tests (one
        # LSP process serves the whole suite) are discarded.
        diags = client.collect_diagnostics(uri=uri)
        rendered = _render_diagnostics(diags)
    else:
        raise ValueError(f"unsupported op: {op!r}")

    client.send_notification(
        "textDocument/didClose",
        {"textDocument": {"uri": uri}},
    )
    return rendered


def execute_lsp_test(
    client: LspClient,
    name: str,
    blueprint: LspDiffTestBlueprint,
) -> TestResult:
    t0 = time.monotonic()
    try:
        actual_raw = _open_and_run(client, name, blueprint)
    except Exception as e:  # noqa: BLE001
        elapsed_ms = int((time.monotonic() - t0) * 1000)
        return TestResult(
            name=name,
            passed=False,
            elapsed_ms=elapsed_ms,
            error=f"{type(e).__name__}: {e}",
            sql=blueprint.sql,
        )
    elapsed_ms = int((time.monotonic() - t0) * 1000)
    actual = normalize_output(actual_raw)
    expected = normalize_output(blueprint.out)
    return TestResult(
        name=name,
        passed=(actual == expected),
        elapsed_ms=elapsed_ms,
        actual=actual,
        expected=expected,
        sql=blueprint.sql,
    )
