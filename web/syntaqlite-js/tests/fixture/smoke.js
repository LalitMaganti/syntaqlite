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

  const diag = engine.runDiagnostics("selec 1 frm t");
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
