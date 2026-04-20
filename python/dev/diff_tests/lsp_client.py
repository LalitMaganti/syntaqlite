# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Minimal JSON-RPC client for driving the syntaqlite LSP server over stdio.

Shared between the RPC integration suite (`suites/lsp.py`) and the declarative
diff-test suite (`suites/lsp_diff.py`).
"""

from __future__ import annotations

import json
import subprocess
import time
from pathlib import Path
from typing import Any


class LspClient:
    """Tiny JSON-RPC client that talks to an LSP server over stdin/stdout."""

    def __init__(self, proc: subprocess.Popen[bytes]):
        self._proc = proc
        self._id = 0

    def send_request(self, method: str, params: Any = None) -> Any:
        self._id += 1
        msg: dict[str, Any] = {"jsonrpc": "2.0", "id": self._id, "method": method}
        if params is not None:
            msg["params"] = params
        self._write(msg)
        return self._read_response(self._id)

    def send_notification(self, method: str, params: Any = None) -> None:
        msg: dict[str, Any] = {"jsonrpc": "2.0", "method": method}
        if params is not None:
            msg["params"] = params
        self._write(msg)

    def collect_diagnostics(
        self,
        timeout: float = 5.0,
        uri: str | None = None,
    ) -> list[dict[str, Any]]:
        """Read notifications until we get publishDiagnostics or timeout.

        When `uri` is given, publishDiagnostics notifications for other
        documents are skipped — important when one client serves multiple
        tests in sequence and stale notifications linger in the pipe.
        """
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            msg = self._read_message(timeout=deadline - time.monotonic())
            if msg is None:
                continue
            if msg.get("method") != "textDocument/publishDiagnostics":
                continue
            params = msg.get("params", {})
            if uri is not None and params.get("uri") != uri:
                continue
            return params.get("diagnostics", [])
        return []

    def shutdown(self) -> None:
        try:
            self.send_request("shutdown")
            self.send_notification("exit")
        except (BrokenPipeError, OSError):
            pass
        self._proc.wait(timeout=5)

    # ── Wire protocol ────────────────────────────────────────────────────

    def _write(self, msg: dict[str, Any]) -> None:
        body = json.dumps(msg).encode("utf-8")
        header = f"Content-Length: {len(body)}\r\n\r\n".encode("ascii")
        assert self._proc.stdin is not None
        self._proc.stdin.write(header + body)
        self._proc.stdin.flush()

    def _read_message(self, timeout: float = 5.0) -> dict[str, Any] | None:
        import select

        assert self._proc.stdout is not None
        fd = self._proc.stdout.fileno()

        ready, _, _ = select.select([fd], [], [], timeout)
        if not ready:
            return None

        header = b""
        while b"\r\n\r\n" not in header:
            ready, _, _ = select.select([fd], [], [], 2.0)
            if not ready:
                return None
            chunk = self._proc.stdout.read(1)
            if not chunk:
                return None
            header += chunk

        length = 0
        for line in header.split(b"\r\n"):
            if line.startswith(b"Content-Length:"):
                length = int(line.split(b":")[1].strip())

        if length == 0:
            return None

        body = b""
        while len(body) < length:
            chunk = self._proc.stdout.read(length - len(body))
            if not chunk:
                return None
            body += chunk

        return json.loads(body.decode("utf-8"))

    def _read_response(self, req_id: int) -> Any:
        """Read messages until we find the response matching req_id."""
        deadline = time.monotonic() + 10.0
        while time.monotonic() < deadline:
            msg = self._read_message(timeout=deadline - time.monotonic())
            if msg is None:
                continue
            if msg.get("id") == req_id:
                if "error" in msg:
                    raise RuntimeError(f"LSP error: {msg['error']}")
                return msg.get("result")
        raise TimeoutError(f"No response for request id={req_id}")


def spawn_lsp(binary: Path, init_options: dict[str, Any] | None = None) -> LspClient:
    """Spawn the LSP server and complete the initialize handshake."""
    proc = subprocess.Popen(
        [str(binary), "--no-config", "lsp"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        bufsize=0,
    )
    client = LspClient(proc)

    init_params: dict[str, Any] = {
        "processId": None,
        "capabilities": {},
        "rootUri": None,
    }
    if init_options is not None:
        init_params["initializationOptions"] = init_options

    client.send_request("initialize", init_params)
    client.send_notification("initialized", {})
    return client
