// Smoke-test the built package under Node. Run with:
//   npm run build && node tests/node-smoke.mjs
// Requires wasm/ to be populated (tools/build-web-playground --hermetic).
import {Engine, DialectManager} from "../dist/index.js";

function lspInit(engine) {
  const init = engine.lspMessage({
    jsonrpc: "2.0", id: 1, method: "initialize", params: {capabilities: {}},
  });
  if (init[0]?.result?.capabilities?.positionEncoding !== "utf-16") {
    throw new Error(`unexpected initialize result: ${JSON.stringify(init[0])}`);
  }
  if (init[0].result.capabilities.experimental?.syntaqlite?.setSessionContext !== true) {
    throw new Error("setSessionContext extension not advertised");
  }
  engine.lspMessage({jsonrpc: "2.0", method: "initialized", params: {}});
}

/** Sync a document and return the diagnostics its publish push carries. */
function lspDiagnostics(engine, uri, text, version) {
  const method = version === 1 ? "textDocument/didOpen" : "textDocument/didChange";
  const params = version === 1
    ? {textDocument: {uri, languageId: "sql", version, text}}
    : {textDocument: {uri, version}, contentChanges: [{text}]};
  const out = engine.lspMessage({jsonrpc: "2.0", method, params});
  const pub = out.find((m) => m.method === "textDocument/publishDiagnostics");
  if (!pub) throw new Error(`no publishDiagnostics from ${method}`);
  return pub.params.diagnostics;
}

const engine = new Engine();
await engine.load();
if (!engine.ready) throw new Error("engine.ready is false after load");

const dm = new DialectManager();
await dm.loadDefault(engine);
if (!dm.active) throw new Error(`dialect did not load: ${engine.status}`);

const fmt = engine.runFmt("select a,b from t where a=1", {
  lineWidth: 80,
  indentWidth: 2,
  keywordCase: 1,
  semicolons: true,
});
if (!fmt.ok) throw new Error(`fmt failed: ${fmt.text}`);
if (!/SELECT a, b FROM t WHERE a = 1/.test(fmt.text)) {
  throw new Error(`unexpected fmt output: ${fmt.text}`);
}

if (!engine.lspSupported) throw new Error("LSP entry point missing from runtime");
lspInit(engine);

const parseDiags = lspDiagnostics(engine, "file:///w.sql", "selec 1", 1);
if (parseDiags.length === 0) throw new Error("expected a parse diagnostic");
if (typeof parseDiags[0].data?.startOffset !== "number") {
  throw new Error(`diagnostic missing structured data: ${JSON.stringify(parseDiags[0])}`);
}

const comp = engine.lspMessage({
  jsonrpc: "2.0", id: 2, method: "textDocument/completion",
  params: {textDocument: {uri: "file:///w.sql"}, position: {line: 0, character: 7}},
});
if (!Array.isArray(comp[0]?.result) || comp[0].result.length === 0) {
  throw new Error("expected LSP completion items");
}

// Schema context applies through the syntaqlite/setSessionContext extension
// (Engine.setSessionContextDdl wraps it).
const ddl = engine.setSessionContextDdl("CREATE TABLE t(a INTEGER, b TEXT);");
if (!ddl.ok) throw new Error(`session context DDL failed: ${ddl.error}`);
const schemaDiags = lspDiagnostics(engine, "file:///w.sql", "SELECT c FROM t", 2);
if (!schemaDiags.some((d) => d.data?.detail?.kind === "unknown_column")) {
  throw new Error("expected unknown-column diagnostic under schema context");
}

// Two engines share one runtime instance but have independent sessions:
// schema context set on one must not leak into the other.
const other = new Engine({runtime: engine.runtimeModule});
await other.load();
other.setDialectPointer(dm.active.ptr);
lspInit(other);
const isolated = lspDiagnostics(other, "file:///w.sql", "SELECT c FROM t", 1);
if (isolated.some((d) => d.data?.detail?.kind === "unknown_column")) {
  throw new Error("schema context leaked between sessions");
}
other.dispose();

console.log("node smoke test: OK");
