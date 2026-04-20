// Packs the package and installs the tarball into tests/fixture/ so the
// Playwright smoke test exercises the exact bytes that would be published
// to npm — catching issue #189 class bugs where required files are missing
// from the tarball.
import {execSync} from "node:child_process";
import {existsSync, rmSync} from "node:fs";
import {dirname, join, resolve} from "node:path";
import {fileURLToPath} from "node:url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const pkgDir = resolve(__dirname, "..");
const fixtureDir = join(__dirname, "fixture");

const source = process.argv[2] ?? "local";

let tarball;
if (source === "local") {
  console.log("> npm pack (in " + pkgDir + ")");
  tarball = execSync("npm pack --silent", {cwd: pkgDir, encoding: "utf8"}).trim();
  tarball = join(pkgDir, tarball);
} else if (source === "registry") {
  const version = process.argv[3] ?? "latest";
  console.log("> npm pack syntaqlite@" + version + " (from registry)");
  const out = execSync(
    "npm pack --silent syntaqlite@" + JSON.stringify(version),
    {cwd: pkgDir, encoding: "utf8"},
  ).trim();
  tarball = join(pkgDir, out);
} else {
  console.error("unknown source: " + source);
  process.exit(1);
}

if (!existsSync(tarball)) {
  console.error("tarball not found: " + tarball);
  process.exit(1);
}
console.log("  tarball: " + tarball);

console.log("> cleaning fixture node_modules");
rmSync(join(fixtureDir, "node_modules"), {recursive: true, force: true});
rmSync(join(fixtureDir, "package-lock.json"), {force: true});

console.log("> npm install " + tarball + " (in " + fixtureDir + ")");
execSync(
  "npm install --no-save --no-audit --no-fund --silent " + JSON.stringify(tarball),
  {cwd: fixtureDir, stdio: "inherit"},
);

console.log("> fixture ready at " + fixtureDir);
