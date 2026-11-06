#!/usr/bin/env node
// LSP feature tester: spawns an LSP server, tests multiple capabilities.
// Usage: node _lsp_test.js <server-cmd...> -- <sql-file> [--db <path>] [--schema <path>] [--test <feature>]
//
// Features: capabilities, diagnostics, completion, hover, formatting
// If --test is omitted, tests all features and prints JSON results.

const { spawn } = require("child_process");
const fs = require("fs");
const path = require("path");

const TIMEOUT_MS = 10000;

// Parse args
const dashIdx = process.argv.indexOf("--");
if (dashIdx < 0 || dashIdx < 3) {
  console.error("Usage: node _lsp_test.js <server-cmd...> -- <sql-file> [--db <path>] [--test <feature>]");
  process.exit(2);
}
const serverCmd = process.argv.slice(2, dashIdx);
const ourArgs = process.argv.slice(dashIdx + 1);
const sqlFile = ourArgs[0];
const dbIdx = ourArgs.indexOf("--db");
const dbPath = dbIdx >= 0 ? ourArgs[dbIdx + 1] : null;
const schemaIdx = ourArgs.indexOf("--schema");
const schemaPath = schemaIdx >= 0 ? ourArgs[schemaIdx + 1] : null;
const testIdx = ourArgs.indexOf("--test");
const testFeature = testIdx >= 0 ? ourArgs[testIdx + 1] : null;

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
let initialized = false;
const pendingRequests = new Map(); // id -> { resolve, method }
const results = {
  capabilities: null,
  diagnostics: null,
  completion: null,
  hover: null,
  formatting: null,
};

proc.stdout.on("data", (chunk) => {
  buffer += chunk.toString();
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
      handleMessage(JSON.parse(body));
    } catch (e) {}
  }
});

proc.stderr.on("data", () => {});

// A server that fails to start or exits early can't answer, so report what we
// have instead of waiting for the timeout.
let finished = false;
proc.on("error", () => { if (!finished) finish(); });
proc.on("exit", () => { if (!finished) finish(); });

function handleMessage(msg) {
  // Response to a request
  if (msg.id !== undefined && pendingRequests.has(msg.id)) {
    const { resolve } = pendingRequests.get(msg.id);
    pendingRequests.delete(msg.id);
    resolve(msg);
    return;
  }

  // Initialize response
  if (msg.id === 1 && !initialized) {
    initialized = true;
    const caps = msg.result?.capabilities || {};
    results.capabilities = {
      completion: !!caps.completionProvider,
      hover: !!caps.hoverProvider,
      definition: !!caps.definitionProvider,
      references: !!caps.referencesProvider,
      formatting: !!caps.documentFormattingProvider,
      diagnostics: true, // assumed if server starts
      codeAction: !!caps.codeActionProvider,
      rename: !!caps.renameProvider,
      signatureHelp: !!caps.signatureHelpProvider,
    };

    // Send initialized + didOpen
    proc.stdin.write(encode(notification("initialized", {})));
    proc.stdin.write(encode(notification("textDocument/didOpen", {
      textDocument: { uri: fileUri, languageId: "sql", version: 1, text: source },
    })));

    // Wait for server to finish initialization (sqls needs time to load DB cache)
    setTimeout(runTests, 3000);
    return;
  }

  // Notification: diagnostics
  if (msg.method === "textDocument/publishDiagnostics") {
    results.diagnostics = (msg.params.diagnostics || []).map(d => ({
      line: (d.range?.start?.line || 0) + 1,
      col: (d.range?.start?.character || 0) + 1,
      severity: ["", "error", "warning", "info", "hint"][d.severity] || "unknown",
      message: d.message,
    }));
  }
}

function sendRequest(method, params) {
  return new Promise((resolve) => {
    const req = request(method, params);
    pendingRequests.set(req.id, { resolve, method });
    proc.stdin.write(encode(req.obj));
    // Timeout per request
    setTimeout(() => {
      if (pendingRequests.has(req.id)) {
        pendingRequests.delete(req.id);
        resolve({ error: "timeout" });
      }
    }, 5000);
  });
}

async function runTests() {
  const lines = source.split("\n");
  // Find positions for completion and hover tests
  let compLine = 0, compCol = 0;
  let hoverLine = 0, hoverCol = 0;
  for (let i = 0; i < lines.length; i++) {
    const sel = lines[i].indexOf("SELECT");
    if (sel >= 0) {
      compLine = i;
      // Position after "SELECT " for completion context
      const fromIdx = lines[i].indexOf("FROM");
      if (fromIdx > 0) {
        compCol = fromIdx - 1;
        // For hover, position on the first word after FROM (table name)
        const afterFrom = lines[i].substring(fromIdx + 4).match(/\s+(\w+)/);
        if (afterFrom) {
          hoverLine = i;
          hoverCol = fromIdx + 4 + afterFrom.index + 1; // on the table name
        } else {
          // Table name might be on the next line
          hoverLine = i;
          hoverCol = fromIdx + 1; // fallback: on FROM keyword
        }
      } else {
        compCol = sel + 7;
        hoverLine = i;
        hoverCol = sel + 7;
      }
      break;
    }
  }

  // Test completion at multiple positions to find the best result.
  // Different servers respond better at different cursor positions.
  if (!testFeature || testFeature === "completion") {
    const positions = [];
    for (let i = 0; i < lines.length; i++) {
      const line = lines[i];
      const selIdx = line.indexOf("SELECT");
      if (selIdx >= 0) {
        // After "SELECT " — column completions
        positions.push({ line: i, character: selIdx + 7 });
        const fromIdx = line.indexOf("FROM");
        if (fromIdx > 0) {
          // After "FROM " — table completions
          positions.push({ line: i, character: fromIdx + 5 });
        }
        break;
      }
    }
    if (positions.length === 0) {
      positions.push({ line: 0, character: 0 });
    }

    let bestCount = 0;
    let supported = false;
    for (const pos of positions) {
      const compRes = await sendRequest("textDocument/completion", {
        textDocument: { uri: fileUri },
        position: pos,
      });
      if (!compRes.error && compRes.result) {
        supported = true;
        const items = Array.isArray(compRes.result) ? compRes.result : (compRes.result.items || []);
        if (items.length > bestCount) bestCount = items.length;
      }
    }
    results.completion = { supported, count: bestCount };
  }

  // Test hover
  if (!testFeature || testFeature === "hover") {
    const hoverRes = await sendRequest("textDocument/hover", {
      textDocument: { uri: fileUri },
      position: { line: hoverLine, character: hoverCol },
    });
    if (hoverRes.error) {
      results.hover = { supported: false, content: null };
    } else if (hoverRes.result?.contents) {
      const c = hoverRes.result.contents;
      const text = typeof c === "string" ? c : (c.value || JSON.stringify(c));
      results.hover = { supported: true, content: text.substring(0, 200) };
    } else {
      results.hover = { supported: false, content: null };
    }
  }

  // Test formatting
  if (!testFeature || testFeature === "formatting") {
    const fmtRes = await sendRequest("textDocument/formatting", {
      textDocument: { uri: fileUri },
      options: { tabSize: 2, insertSpaces: true },
    });
    if (fmtRes.error) {
      results.formatting = { supported: false, edits: 0 };
    } else if (Array.isArray(fmtRes.result)) {
      results.formatting = { supported: true, edits: fmtRes.result.length };
    } else {
      results.formatting = { supported: false, edits: 0 };
    }
  }

  finish();
}

function finish() {
  finished = true;
  console.log(JSON.stringify(results, null, 2));
  proc.kill();
  process.exit(0);
}

// Send initialize request
const initParams = {
  processId: process.pid,
  capabilities: {
    textDocument: {
      publishDiagnostics: { relatedInformation: false },
      completion: { completionItem: { snippetSupport: false } },
      hover: { contentFormat: ["plaintext"] },
    },
  },
  rootUri: "file://" + path.resolve("."),
  workspaceFolders: null,
};

if (dbPath || schemaPath) {
  initParams.initializationOptions = {};
  if (dbPath) {
    initParams.initializationOptions.connectionConfig = {
      adapter: "sqlite3",
      filename: path.resolve(dbPath),
    };
  }
  if (schemaPath) {
    initParams.initializationOptions.schemaPath = path.resolve(schemaPath);
  }
}

const init = request("initialize", initParams);
proc.stdin.write(encode(init.obj));

setTimeout(() => {
  finish();
}, TIMEOUT_MS);
