#!/usr/bin/env python3
"""PerfettoSQL standard library dependency graph analyzer.

Scans PerfettoSQL stdlib .sql files using syntaqlite's semantic analyzer to
extract table/view definitions and their dependencies, and outputs the graph
as JSON, interactive HTML, ASCII, or Graphviz DOT.

Usage:
    python perfetto_stdlib_graph.py --dialect /path/to/perfetto.dylib json --pretty
    python perfetto_stdlib_graph.py --dialect /path/to/perfetto.dylib ascii
    python perfetto_stdlib_graph.py --dialect /path/to/perfetto.dylib ascii --table thread_slice
    python perfetto_stdlib_graph.py --dialect /path/to/perfetto.dylib html -o /tmp/graph.html
    python perfetto_stdlib_graph.py --dialect /path/to/perfetto.dylib dot --cluster
"""

from __future__ import annotations

import argparse
import json
import sys
from collections import defaultdict, deque
from pathlib import Path

import syntaqlite

# ── Extraction ───────────────────────────────────────────────────────────────


def _path_to_module(rel_path: str) -> str:
    return rel_path.removesuffix(".sql").replace("/", ".")


def _directory_resolver(root: Path):
    """Return a module resolver callback for the given root directory."""

    def resolve(module_path: str) -> str | None:
        path = root / module_path.replace(".", "/")
        path = path.with_suffix(".sql")
        try:
            return path.read_text()
        except OSError:
            return None

    return resolve


def extract_stdlib(root: Path, dialect: syntaqlite.Dialect) -> tuple[list, list]:
    """Walk stdlib and extract definitions + dependency edges."""
    sql_files = sorted(
        (str(p.relative_to(root)), p.read_text())
        for p in root.rglob("*.sql")
    )

    resolver = _directory_resolver(root)
    definitions = []
    edges = []
    seen = set()

    for rel_path, contents in sql_files:
        module = _path_to_module(rel_path)
        result = syntaqlite.validate(
            contents, module_resolver=resolver, dialect=dialect
        )

        for stmt in result.statements:
            stmt_defs = [
                {
                    "name": dr.name,
                    "is_view": dr.is_view,
                    "module": module,
                    "file": rel_path,
                    "is_private": dr.name.startswith("_"),
                    "sql": stmt.source,
                }
                for dr in stmt.defined_relations
            ]

            for rel in stmt.relations:
                for defn in stmt_defs:
                    if rel.name == defn["name"]:
                        continue
                    key = (defn["name"], rel.name)
                    if key not in seen:
                        seen.add(key)
                        edges.append({"from": defn["name"], "to": rel.name, "kind": "table_ref"})

            definitions.extend(stmt_defs)

    edges.sort(key=lambda e: (e["from"], e["to"]))
    return definitions, edges


# ── Graph construction ───────────────────────────────────────────────────────


def build_graph(definitions: list, edges: list) -> dict:
    known = {d["name"].lower() for d in definitions}
    external_refs = sorted({e["to"] for e in edges if e["to"].lower() not in known})
    return {
        "definitions": definitions,
        "edges": edges,
        "external_refs": external_refs,
        "stats": {
            "total_definitions": len(definitions),
            "tables": sum(1 for d in definitions if not d["is_view"]),
            "views": sum(1 for d in definitions if d["is_view"]),
            "total_edges": len(edges),
            "external_refs": len(external_refs),
        },
    }


# ── Graph helpers ────────────────────────────────────────────────────────────


def _transitive(name: str, adj: dict[str, set[str]]) -> set[str]:
    visited = set()
    queue = deque([name])
    while queue:
        current = queue.popleft()
        if current in visited:
            continue
        visited.add(current)
        for n in adj.get(current, ()):
            if n not in visited:
                queue.append(n)
    visited.discard(name)
    return visited


def _build_adj(edges: list) -> tuple[dict, dict]:
    fwd = defaultdict(set)  # deps_of
    rev = defaultdict(set)  # dependents_of
    for e in edges:
        fwd[e["from"]].add(e["to"])
        rev[e["to"]].add(e["from"])
    return fwd, rev


# ── JSON output ──────────────────────────────────────────────────────────────


def write_json(graph: dict, out, pretty: bool = False) -> None:
    indent = 2 if pretty else None
    json.dump(graph, out, indent=indent)
    out.write("\n")


# ── ASCII output ─────────────────────────────────────────────────────────────


def write_ascii_summary(graph: dict, out) -> None:
    by_top: dict[str, list[int, int]] = defaultdict(lambda: [0, 0])
    for d in graph["definitions"]:
        top = d["module"].split(".")[0]
        by_top[top][1 if d["is_view"] else 0] += 1

    out.write(f"\n  {'Module':<40} {'Tables':>6} {'Views':>6} {'Total':>6}\n")
    out.write(f"  {'-' * 58}\n")
    total = [0, 0]
    for module in sorted(by_top):
        t, v = by_top[module]
        out.write(f"  {module:<40} {t:>6} {v:>6} {t + v:>6}\n")
        total[0] += t
        total[1] += v
    out.write(f"  {'-' * 58}\n")
    out.write(f"  {'TOTAL':<40} {total[0]:>6} {total[1]:>6} {sum(total):>6}\n")
    out.write(f"\n  External references: {graph['stats']['external_refs']}\n")
    out.write(f"  Dependency edges: {graph['stats']['total_edges']}\n\n")


def write_ascii_detail(graph: dict, name: str, transitive: bool, out) -> None:
    ext_set = set(graph["external_refs"])
    defn = next((d for d in graph["definitions"] if d["name"].lower() == name.lower()), None)

    if defn is None:
        if name in ext_set:
            out.write(f"\n  {name} (external/builtin)\n")
            deps_by = [e for e in graph["edges"] if e["to"] == name]
            if deps_by:
                out.write("\n  Dependents:\n")
                for e in deps_by:
                    out.write(f"    -> {e['from']}\n")
            out.write("\n")
            return
        print(f"error: definition '{name}' not found.", file=sys.stderr)
        sys.exit(1)

    kind = "view" if defn["is_view"] else "table"
    out.write(f"\n  {defn['name']} ({kind}, module: {defn['module']}, file: {defn['file']})\n")

    deps = [e for e in graph["edges"] if e["from"] == name]
    if deps:
        out.write("\n  Dependencies:\n")
        for e in deps:
            ext = " (external)" if e["to"] in ext_set else ""
            out.write(f"    <- {e['to']}{ext}\n")

    dependents = [e for e in graph["edges"] if e["to"] == name]
    if dependents:
        out.write("\n  Dependents:\n")
        for e in dependents:
            out.write(f"    -> {e['from']}\n")

    if transitive:
        fwd, rev = _build_adj(graph["edges"])
        trans_deps = sorted(_transitive(name, fwd))
        if trans_deps:
            out.write(f"\n  Transitive dependencies ({len(trans_deps)}):\n")
            for dep in trans_deps:
                ext = " (external)" if dep in ext_set else ""
                out.write(f"    {dep}{ext}\n")

        trans_dependents = sorted(_transitive(name, rev))
        if trans_dependents:
            out.write(f"\n  Transitive dependents ({len(trans_dependents)}):\n")
            for dep in trans_dependents:
                out.write(f"    {dep}\n")

    out.write("\n")


# ── DOT output ───────────────────────────────────────────────────────────────


def _dot_escape(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def write_dot(graph: dict, out, cluster: bool = False) -> None:
    ext_set = set(graph["external_refs"])
    out.write("digraph perfetto_stdlib {\n")
    out.write('  rankdir=LR;\n  node [fontname="Helvetica" fontsize=10];\n\n')

    if cluster:
        by_module: dict[str, list] = defaultdict(list)
        for d in graph["definitions"]:
            top = d["module"].split(".")[0]
            by_module[top].append(d)
        for i, (module, defs) in enumerate(sorted(by_module.items())):
            out.write(f"  subgraph cluster_{i} {{\n")
            out.write(f'    label="{module}";\n    style=rounded;\n')
            for d in defs:
                color, shape = ("#50c878", "ellipse") if d["is_view"] else ("#4a90d9", "box")
                out.write(f'    "{_dot_escape(d["name"])}" [shape={shape} style=filled fillcolor="{color}"];\n')
            out.write("  }\n")
    else:
        for d in graph["definitions"]:
            color, shape = ("#50c878", "ellipse") if d["is_view"] else ("#4a90d9", "box")
            out.write(f'  "{_dot_escape(d["name"])}" [shape={shape} style=filled fillcolor="{color}"];\n')

    for ext in sorted(ext_set):
        out.write(f'  "{_dot_escape(ext)}" [shape=box style=dashed fillcolor="#f0f0f0"];\n')

    out.write("\n")
    for e in graph["edges"]:
        out.write(f'  "{_dot_escape(e["from"])}" -> "{_dot_escape(e["to"])}";\n')
    out.write("}\n")


def write_dot_for_table(graph: dict, name: str, out) -> None:
    ext_set = set(graph["external_refs"])
    fwd, rev = _build_adj(graph["edges"])
    deps = _transitive(name, fwd)
    dependents = _transitive(name, rev)
    nodes = {name} | deps | dependents

    out.write(f"digraph {name} {{\n")
    out.write('  rankdir=LR;\n  node [fontname="Helvetica" fontsize=10];\n\n')

    for d in graph["definitions"]:
        if d["name"] in nodes:
            color, shape = ("#50c878", "ellipse") if d["is_view"] else ("#4a90d9", "box")
            bold = " penwidth=3" if d["name"] == name else ""
            out.write(f'  "{_dot_escape(d["name"])}" [shape={shape} style=filled fillcolor="{color}"{bold}];\n')

    for ext in sorted(ext_set):
        if ext in nodes:
            out.write(f'  "{_dot_escape(ext)}" [shape=box style=dashed fillcolor="#f0f0f0"];\n')

    out.write("\n")
    for e in graph["edges"]:
        if e["from"] in nodes and e["to"] in nodes:
            out.write(f'  "{_dot_escape(e["from"])}" -> "{_dot_escape(e["to"])}";\n')
    out.write("}\n")


# ── HTML output ──────────────────────────────────────────────────────────────

HTML_TEMPLATE = r"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>PerfettoSQL Stdlib — Dependency Explorer</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif; background: #1e1e2e; color: #cdd6f4; overflow: hidden; }
#app { display: flex; height: 100vh; }

/* Sidebar */
#sidebar { width: 300px; min-width: 300px; background: #181825; border-right: 1px solid #313244; display: flex; flex-direction: column; }
#sidebar h1 { font-size: 13px; padding: 12px 14px; border-bottom: 1px solid #313244; color: #f38ba8; }
#search-box { margin: 8px 10px; padding: 7px 10px; background: #1e1e2e; border: 1px solid #313244; border-radius: 5px; color: #cdd6f4; font-size: 13px; outline: none; }
#search-box:focus { border-color: #f38ba8; }
#search-box::placeholder { color: #585b70; }
#controls { padding: 6px 10px; font-size: 11px; display: flex; gap: 8px; }
#controls label { display: flex; align-items: center; gap: 3px; cursor: pointer; }
#controls input { accent-color: #f38ba8; }
#stats { padding: 5px 10px; font-size: 10px; color: #585b70; border-bottom: 1px solid #313244; }
#results { flex: 1; overflow-y: auto; }
.item { padding: 5px 14px; cursor: pointer; font-size: 12px; display: flex; align-items: center; gap: 5px; }
.item:hover { background: #1e1e2e; }
.item.active { background: #313244; }
.badge { font-size: 9px; padding: 1px 4px; border-radius: 3px; font-weight: 600; }
.badge-t { background: #89b4fa; color: #1e1e2e; }
.badge-v { background: #a6e3a1; color: #1e1e2e; }

/* Main area */
#main { flex: 1; display: flex; flex-direction: column; }
#graph-area { flex: 1; overflow: auto; padding: 20px; background: #1e1e2e; }
#graph-area svg { max-width: 100%; }
#empty { display: flex; align-items: center; justify-content: center; height: 100%; color: #585b70; font-size: 15px; }

/* SQL panel */
#sql-panel { display: none; height: 40%; min-height: 100px; border-top: 1px solid #313244; background: #181825; overflow: auto; }
#sql-panel .header { padding: 8px 14px; font-size: 12px; color: #f38ba8; border-bottom: 1px solid #313244; display: flex; justify-content: space-between; align-items: center; }
#sql-panel .header .close { cursor: pointer; color: #585b70; font-size: 16px; }
#sql-panel .header .close:hover { color: #f38ba8; }
#sql-panel pre { padding: 12px 14px; font-size: 12px; line-height: 1.5; color: #cdd6f4; white-space: pre-wrap; word-break: break-word; font-family: "SF Mono", "Fira Code", monospace; }
</style>
</head>
<body>
<div id="app">
  <div id="sidebar">
    <h1>PerfettoSQL Dependencies</h1>
    <input type="text" id="search-box" placeholder="Search tables/views..." autofocus>
    <div id="controls">
      <label><input type="checkbox" id="show-private" checked> Private (_)</label>
      <label><input type="checkbox" id="show-external" checked> External</label>
    </div>
    <div id="stats"></div>
    <div id="results"></div>
  </div>
  <div id="main">
    <div id="graph-area"><div id="empty">Search and select a table to explore</div></div>
    <div id="sql-panel">
      <div class="header"><span id="sql-title"></span><span class="close" onclick="closeSql()">&times;</span></div>
      <pre id="sql-code"></pre>
    </div>
  </div>
</div>

<script type="module">
import mermaid from 'https://cdn.jsdelivr.net/npm/mermaid@11/dist/mermaid.esm.min.mjs';
mermaid.initialize({ startOnLoad: false, theme: 'dark', flowchart: { curve: 'basis' } });

/*__GRAPH_DATA__*/

const defByName = new Map();
GRAPH.definitions.forEach(d => defByName.set(d.name, d));
const externalSet = new Set(GRAPH.external_refs);

const depsOf = new Map();
const depsBy = new Map();
GRAPH.edges.forEach(e => {
  if (!depsOf.has(e.from)) depsOf.set(e.from, new Set());
  depsOf.get(e.from).add(e.to);
  if (!depsBy.has(e.to)) depsBy.set(e.to, new Set());
  depsBy.get(e.to).add(e.from);
});

function transitive(start, adj) {
  const vis = new Set();
  const q = [start];
  while (q.length) {
    const c = q.shift();
    if (vis.has(c)) continue;
    vis.add(c);
    const ns = adj.get(c);
    if (ns) ns.forEach(n => { if (!vis.has(n)) q.push(n); });
  }
  vis.delete(start);
  return vis;
}

document.getElementById("stats").textContent =
  `${GRAPH.stats.total_definitions} definitions | ${GRAPH.stats.total_edges} edges`;

let currentRoot = null;

window.focusOn = focusOn;
window.showSql = showSql;
window.closeSql = closeSql;

async function focusOn(name) {
  currentRoot = name;
  const showPrivate = document.getElementById("show-private").checked;
  const showExternal = document.getElementById("show-external").checked;

  const upstream = transitive(name, depsOf);
  const downstream = transitive(name, depsBy);

  const visible = new Set([name]);
  [...upstream, ...downstream].forEach(n => {
    if (!showPrivate && n.startsWith("_")) return;
    if (!showExternal && externalSet.has(n)) return;
    visible.add(n);
  });

  const nodeIds = new Map();
  let counter = 0;
  function nid(name) {
    if (!nodeIds.has(name)) nodeIds.set(name, `n${counter++}`);
    return nodeIds.get(name);
  }

  let lines = ["graph LR"];

  visible.forEach(n => {
    const id = nid(n);
    const isExt = externalSet.has(n);
    const def = defByName.get(n);
    const label = n.length > 40 ? n.slice(0,38)+"..." : n;
    if (n === name) {
      lines.push(`  ${id}["${esc(label)}"]:::root`);
    } else if (isExt) {
      lines.push(`  ${id}["${esc(label)}"]:::ext`);
    } else if (def && def.is_view) {
      lines.push(`  ${id}(["${esc(label)}"]):::view`);
    } else {
      lines.push(`  ${id}["${esc(label)}"]:::tbl`);
    }
  });

  GRAPH.edges.forEach(e => {
    if (visible.has(e.from) && visible.has(e.to)) {
      lines.push(`  ${nid(e.from)} --> ${nid(e.to)}`);
    }
  });

  lines.push("  classDef root fill:#f38ba8,stroke:#f38ba8,color:#1e1e2e,stroke-width:2px");
  lines.push("  classDef tbl fill:#89b4fa,stroke:#89b4fa,color:#1e1e2e");
  lines.push("  classDef view fill:#a6e3a1,stroke:#a6e3a1,color:#1e1e2e");
  lines.push("  classDef ext fill:#585b70,stroke:#585b70,color:#cdd6f4");

  const graphDef = lines.join("\n");
  const area = document.getElementById("graph-area");
  area.innerHTML = `<pre class="mermaid">${graphDef}</pre>`;
  await mermaid.run({ nodes: area.querySelectorAll(".mermaid") });

  area.querySelectorAll(".node").forEach(el => {
    const elId = el.id?.replace("flowchart-","").replace(/-\d+$/,"");
    if (!elId) return;
    let nodeName = null;
    for (const [nm, id] of nodeIds) {
      if (id === elId) { nodeName = nm; break; }
    }
    if (!nodeName) return;
    el.style.cursor = "pointer";
    el.addEventListener("click", () => focusOn(nodeName));
    el.addEventListener("dblclick", (e) => { e.stopPropagation(); showSql(nodeName); });
  });

  renderSearch(document.getElementById("search-box").value);
}

function showSql(name) {
  const def = defByName.get(name);
  const panel = document.getElementById("sql-panel");
  const title = document.getElementById("sql-title");
  const code = document.getElementById("sql-code");
  if (def && def.sql) {
    title.textContent = `${name} (${def.is_view ? "view" : "table"} — ${def.module})`;
    code.textContent = def.sql;
  } else {
    title.textContent = `${name} (external)`;
    code.textContent = "-- No source available (built-in table)";
  }
  panel.style.display = "block";
}

function closeSql() {
  document.getElementById("sql-panel").style.display = "none";
}

function esc(s) { return String(s).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;"); }
function escJs(s) { return s.replace(/\\/g,"\\\\").replace(/"/g,'\\"'); }

const searchBox = document.getElementById("search-box");

function renderSearch(query) {
  const q = query.toLowerCase();
  const showPrivate = document.getElementById("show-private").checked;
  let items = GRAPH.definitions.filter(d => {
    if (!showPrivate && d.is_private) return false;
    return d.name.toLowerCase().includes(q) || d.module.toLowerCase().includes(q);
  });
  items.sort((a, b) => {
    const ap = a.name.toLowerCase().startsWith(q) ? 0 : 1;
    const bp = b.name.toLowerCase().startsWith(q) ? 0 : 1;
    if (ap !== bp) return ap - bp;
    return a.name.localeCompare(b.name);
  });
  items = items.slice(0, 100);

  document.getElementById("results").innerHTML = items.map(d => {
    const kl = d.is_view ? "v" : "t";
    const active = currentRoot === d.name ? " active" : "";
    return `<div class="item${active}" onclick="focusOn('${escJs(d.name)}')">` +
      `<span class="badge badge-${kl}">${kl.toUpperCase()}</span> ${esc(d.name)}</div>`;
  }).join("");
}

searchBox.addEventListener("input", () => renderSearch(searchBox.value));
document.getElementById("show-private").addEventListener("change", () => { if (currentRoot) focusOn(currentRoot); renderSearch(searchBox.value); });
document.getElementById("show-external").addEventListener("change", () => { if (currentRoot) focusOn(currentRoot); });

renderSearch("");
</script>
</body>
</html>"""


def write_html(graph: dict, out) -> None:
    graph_json = json.dumps(graph)
    html = HTML_TEMPLATE.replace("/*__GRAPH_DATA__*/", f"const GRAPH = {graph_json};")
    out.write(html)


# ── CLI ──────────────────────────────────────────────────────────────────────


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Analyze the PerfettoSQL standard library dependency graph."
    )
    parser.add_argument(
        "--stdlib",
        default="third_party/src/perfetto/src/trace_processor/perfetto_sql/stdlib",
        help="Path to the PerfettoSQL stdlib directory.",
    )
    parser.add_argument("--dialect", required=True, help="Path to compiled Perfetto dialect .dylib/.so.")

    sub = parser.add_subparsers(dest="command", required=True)

    p_json = sub.add_parser("json", help="Output as JSON.")
    p_json.add_argument("-o", "--output", help="Output file (stdout if omitted).")
    p_json.add_argument("--pretty", action="store_true", help="Pretty-print JSON.")

    p_html = sub.add_parser("html", help="Output interactive HTML visualization.")
    p_html.add_argument("-o", "--output", help="Output file (stdout if omitted).")

    p_ascii = sub.add_parser("ascii", help="Output ASCII summary or detail.")
    p_ascii.add_argument("--table", help="Show dependencies for a specific definition.")
    p_ascii.add_argument("--transitive", action="store_true", help="Include transitive deps.")

    p_dot = sub.add_parser("dot", help="Output Graphviz DOT format.")
    p_dot.add_argument("-o", "--output", help="Output file (stdout if omitted).")
    p_dot.add_argument("--cluster", action="store_true", help="Cluster by module.")
    p_dot.add_argument("--table", help="Graph for a specific table only.")

    args = parser.parse_args()

    stdlib = Path(args.stdlib)
    if not stdlib.exists():
        print(f"error: stdlib path does not exist: {stdlib}", file=sys.stderr)
        return 1

    dialect_path = Path(args.dialect)
    if not dialect_path.exists():
        print(
            f"error: dialect library does not exist: {dialect_path}\n"
            "Run tools/build-perfetto-dialect to compile it first.",
            file=sys.stderr,
        )
        return 1

    dialect = syntaqlite.Dialect(str(dialect_path), "perfetto")

    print(f"Analyzing stdlib at {stdlib}...", file=sys.stderr)

    definitions, edges = extract_stdlib(stdlib, dialect)
    graph = build_graph(definitions, edges)

    print(
        f"Found {graph['stats']['total_definitions']} definitions, "
        f"{graph['stats']['total_edges']} edges, "
        f"{graph['stats']['external_refs']} external refs",
        file=sys.stderr,
    )

    def open_output(path: str | None):
        if path:
            return open(path, "w")
        return sys.stdout

    if args.command == "json":
        with open_output(args.output) as f:
            write_json(graph, f, args.pretty)
    elif args.command == "html":
        with open_output(args.output) as f:
            write_html(graph, f)
    elif args.command == "ascii":
        if args.table:
            write_ascii_detail(graph, args.table, args.transitive, sys.stdout)
        else:
            write_ascii_summary(graph, sys.stdout)
    elif args.command == "dot":
        with open_output(args.output) as f:
            if args.table:
                write_dot_for_table(graph, args.table, f)
            else:
                write_dot(graph, f, args.cluster)

    return 0


if __name__ == "__main__":
    sys.exit(main())
