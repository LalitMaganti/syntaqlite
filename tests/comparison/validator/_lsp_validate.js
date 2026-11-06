#!/usr/bin/env node
// Minimal LSP client: sends a .sql file to an LSP server, collects diagnostics.
// Usage: node _lsp_validate.js <server-cmd...> -- <sql-file> [--db <sqlite-db>]
//
// Spawns the LSP server, sends initialize + didOpen, waits for publishDiagnostics,
// prints each diagnostic as "line:col severity message", exits with 0 if no errors, 1 otherwise.

const { spawn } = require("child_process");
const fs = require("fs");
const path = require("path");

const TIMEOUT_MS = 10000;

// Parse args: everything before "--" is the server command, after is our args
const dashIdx = process.argv.indexOf("--");
if (dashIdx < 0 || dashIdx < 3) {
  console.error("Usage: node _lsp_validate.js <server-cmd...> -- <sql-file> [--db <path>]");
  process.exit(2);
}
const serverCmd = process.argv.slice(2, dashIdx);
const ourArgs = process.argv.slice(dashIdx + 1);
const sqlFile = ourArgs[0];
const dbIdx = ourArgs.indexOf("--db");
const dbPath = dbIdx >= 0 ? ourArgs[dbIdx + 1] : null;

if (!sqlFile) {
  console.error("No SQL file specified");
  process.exit(2);
}

const source = fs.readFileSync(sqlFile, "utf8");
const fileUri = "file://" + path.resolve(sqlFile);

// JSON-RPC helpers
let msgId = 0;
function request(method, params) {
  const id = ++msgId;
  return { id, obj: { jsonrpc: "2.0", id, method, params } };
}
function notification(method, params) {
  return { jsonrpc: "2.0", method, params };
}
function encode(obj) {
  const body = JSON.stringify(obj);
  return `Content-Length: ${Buffer.byteLength(body)}\r\n\r\n${body}`;
}

// Spawn LSP server
const proc = spawn(serverCmd[0], serverCmd.slice(1), {
  stdio: ["pipe", "pipe", "pipe"],
});

let buffer = "";
let diagnostics = null;
let initialized = false;

proc.stdout.on("data", (chunk) => {
  buffer += chunk.toString();
  // Parse LSP messages from buffer
  while (true) {
    const headerEnd = buffer.indexOf("\r\n\r\n");
    if (headerEnd < 0) break;
    const header = buffer.slice(0, headerEnd);
    const match = header.match(/Content-Length:\s*(\d+)/i);
    if (!match) break;
    const len = parseInt(match[1]);
    const bodyStart = headerEnd + 4;
    if (buffer.length < bodyStart + len) break;
    const body = buffer.slice(bodyStart, bodyStart + len);
    buffer = buffer.slice(bodyStart + len);
    try {
      const msg = JSON.parse(body);
      handleMessage(msg);
    } catch (e) {
      // ignore parse errors
    }
  }
});

proc.stderr.on("data", () => {}); // suppress

// A server that fails to start or exits early will never publish diagnostics,
// so stop now instead of waiting for the timeout.
function serverGone() {
  if (diagnostics === null) {
    console.log("(server exited before publishing diagnostics)");
    process.exit(2);
  }
}
proc.on("error", serverGone);
proc.on("exit", serverGone);

function handleMessage(msg) {
  if (msg.id === 1 && !initialized) {
    // initialize response — send initialized + didOpen
    initialized = true;
    proc.stdin.write(encode(notification("initialized", {})));

    // Build workspace config if needed
    const didOpenParams = {
      textDocument: {
        uri: fileUri,
        languageId: "sql",
        version: 1,
        text: source,
      },
    };
    proc.stdin.write(encode(notification("textDocument/didOpen", didOpenParams)));
  }

  if (msg.method === "textDocument/publishDiagnostics") {
    diagnostics = msg.params.diagnostics || [];
    finish();
  }
}

function finish() {
  if (diagnostics && diagnostics.length > 0) {
    for (const d of diagnostics) {
      const line = (d.range?.start?.line || 0) + 1;
      const col = (d.range?.start?.character || 0) + 1;
      const sev = ["", "error", "warning", "info", "hint"][d.severity] || "unknown";
      console.log(`${line}:${col} ${sev} ${d.message}`);
    }
  }
  proc.kill();
  process.exit(diagnostics && diagnostics.length > 0 ? 1 : 0);
}

// Send initialize request
const initParams = {
  processId: process.pid,
  capabilities: {
    textDocument: {
      publishDiagnostics: { relatedInformation: false },
    },
  },
  rootUri: "file://" + path.resolve("."),
  workspaceFolders: null,
};

// If a DB path is given, set workspace settings for sql-language-server / sqls
if (dbPath) {
  initParams.initializationOptions = {
    connectionConfig: {
      adapter: "sqlite3",
      filename: path.resolve(dbPath),
    },
  };
}

const init = request("initialize", initParams);
proc.stdin.write(encode(init.obj));

// Timeout
setTimeout(() => {
  if (diagnostics === null) {
    // No diagnostics received — report that
    console.log("(no diagnostics received within timeout)");
  }
  proc.kill();
  process.exit(diagnostics === null ? 0 : diagnostics.length > 0 ? 1 : 0);
}, TIMEOUT_MS);
