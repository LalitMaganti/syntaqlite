// Verifies that `npm pack` would include the WASM runtime files.
// Runs as the publish-time safety net (prepublishOnly) and from CI.
// Fails loudly so a broken tarball like the one reported in issue #189
// (missing `wasm/` directory) can never be published again.

import {execSync} from "node:child_process";

const REQUIRED = [
  "wasm/syntaqlite-runtime.js",
  "wasm/syntaqlite-runtime.wasm",
  "wasm/syntaqlite-sqlite.wasm",
  "dist/index.js",
  "dist/index.d.ts",
];

const raw = execSync("npm pack --dry-run --json", {encoding: "utf8"});
const parsed = JSON.parse(raw);
const entry = Array.isArray(parsed) ? parsed[0] : parsed;
const files = new Set((entry.files ?? []).map((f) => f.path));

const missing = REQUIRED.filter((p) => !files.has(p));
if (missing.length > 0) {
  console.error("ERROR: tarball is missing required files:");
  for (const p of missing) console.error("  - " + p);
  console.error("");
  console.error("Run `python3 tools/build-web-playground` from the repo root,");
  console.error("then `npm run build` in web/syntaqlite-js before publishing.");
  process.exit(1);
}

console.log("tarball verification passed (" + files.size + " files, " +
  entry.size + " bytes unpacked)");
