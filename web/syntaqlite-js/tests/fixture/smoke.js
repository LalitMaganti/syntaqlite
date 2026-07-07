// Exercises the installed syntaqlite package in a real browser.
// Imports are relative to the installed tarball contents under node_modules/,
// so the path layout (dist/ + wasm/ + root-relative fetches) matches what a
// real consumer's bundler would produce.
import {Engine, DialectManager} from "./node_modules/syntaqlite/dist/index.js";

const logEl = document.getElementById("log");
const statusEl = document.getElementById("status");
const log = (...args) => {
  logEl.textContent += args.join(" ") + "\n";
};

async function main() {
  const engine = new Engine();
  await engine.load();
  log("engine loaded");

  const dm = new DialectManager({
    presets: [{
      id: "sqlite",
      label: "SQLite",
      wasmUrl: "./node_modules/syntaqlite/wasm/syntaqlite-sqlite.wasm",
      symbol: "syntaqlite_sqlite_dialect_template",
    }],
  });
  await dm.loadDefault(engine);
  log("dialect loaded");

  const fmt = engine.runFmt("select a,b from t where a=1", {
    lineWidth: 80,
    indentWidth: 2,
    keywordCase: 1,
    semicolons: true,
  });
  log("fmt:", JSON.stringify(fmt));

  // Diagnostics are served over LSP: handshake, open a document, and read
  // the publishDiagnostics push.
  engine.lspMessage({jsonrpc: "2.0", id: 1, method: "initialize", params: {capabilities: {}}});
  engine.lspMessage({jsonrpc: "2.0", method: "initialized", params: {}});
  const out = engine.lspMessage({
    jsonrpc: "2.0", method: "textDocument/didOpen",
    params: {textDocument: {uri: "file:///s.sql", languageId: "sql", version: 1, text: "selec 1 frm t"}},
  });
  const pub = out.find((m) => m.method === "textDocument/publishDiagnostics");
  const diag = {ok: pub !== undefined, diagnostics: pub?.params.diagnostics ?? []};
  log("diag count:", diag.diagnostics.length);

  window.__syntaqlite_result = {ok: true, fmt, diag};
  statusEl.textContent = "READY";
}

main().catch((err) => {
  window.__syntaqlite_result = {
    ok: false,
    error: err?.message ?? String(err),
    stack: err?.stack,
  };
  log("error:", err?.message ?? String(err));
  statusEl.textContent = "ERROR";
});
