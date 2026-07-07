// Smoke-test the built package under Node. Run with:
//   npm run build && node tests/node-smoke.mjs
// Requires wasm/ to be populated (tools/build-web-playground --hermetic).
import {Engine, DialectManager} from "../dist/index.js";

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

const diag = engine.runDiagnostics("selec 1 frm t");
if (!diag.ok) throw new Error("diagnostics call failed");
if (diag.diagnostics.length === 0) throw new Error("expected at least one diagnostic");

const completions = engine.runCompletions("SELECT * FROM t WHERE ", 22);
if (!completions.ok) throw new Error("completions call failed");
if (completions.items.length === 0) throw new Error("expected completion items");

// Two engines share one runtime instance but have independent sessions:
// schema context set on one must not leak into the other.
const other = new Engine({runtime: engine.runtimeModule});
await other.load();
other.setDialectPointer(dm.active.ptr);
const ddl = other.setSessionContextDdl("CREATE TABLE t(a INTEGER, b TEXT);");
if (!ddl.ok) throw new Error(`session context DDL failed: ${ddl.error}`);
// `other` knows t's columns, so `c` is an unknown column there; the first
// session has no schema for t and cannot know its columns.
const strictDiag = other.runDiagnostics("SELECT c FROM t");
if (!strictDiag.ok) throw new Error("diagnostics with schema failed");
if (!strictDiag.diagnostics.some((d) => d.detail?.kind === "unknown_column")) {
  throw new Error("expected unknown-column diagnostic under schema context");
}
const unaffected = engine.runDiagnostics("SELECT c FROM t");
if (!unaffected.ok) throw new Error("diagnostics on first session failed");
if (unaffected.diagnostics.some((d) => d.detail?.kind === "unknown_column")) {
  throw new Error("schema context leaked between sessions");
}
other.dispose();

console.log("node smoke test: OK");
