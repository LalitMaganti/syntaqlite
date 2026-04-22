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

console.log("node smoke test: OK");
