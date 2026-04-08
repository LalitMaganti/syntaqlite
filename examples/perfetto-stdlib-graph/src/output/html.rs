//! Self-contained HTML output with interactive Mermaid.js dependency explorer.

use std::io::Write;

use crate::model::DependencyGraph;

/// Write a self-contained HTML file with an interactive dependency explorer.
pub(crate) fn write_html(graph: &DependencyGraph, w: &mut dyn Write) -> Result<(), String> {
    let json = serde_json::to_string(graph).map_err(|e| format!("JSON error: {e}"))?;

    let html = HTML_TEMPLATE.replace("/*__GRAPH_DATA__*/", &format!("const GRAPH = {json};"));

    w.write_all(html.as_bytes())
        .map_err(|e| format!("write error: {e}"))
}

const HTML_TEMPLATE: &str = r##"<!DOCTYPE html>
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

// Indexes.
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

// Make functions globally accessible for onclick handlers.
window.focusOn = focusOn;
window.showSql = showSql;
window.closeSql = closeSql;

async function focusOn(name) {
  currentRoot = name;
  const showPrivate = document.getElementById("show-private").checked;
  const showExternal = document.getElementById("show-external").checked;

  const upstream = transitive(name, depsOf);
  const downstream = transitive(name, depsBy);

  // Filter nodes.
  const visible = new Set([name]);
  [...upstream, ...downstream].forEach(n => {
    if (!showPrivate && n.startsWith("_")) return;
    if (!showExternal && externalSet.has(n)) return;
    visible.add(n);
  });

  // Build Mermaid graph definition.
  // Node IDs must be safe for Mermaid — use a mapping.
  const nodeIds = new Map();
  let counter = 0;
  function nid(name) {
    if (!nodeIds.has(name)) nodeIds.set(name, `n${counter++}`);
    return nodeIds.get(name);
  }

  let lines = ["graph LR"];

  // Nodes.
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

  // Edges.
  GRAPH.edges.forEach(e => {
    if (visible.has(e.from) && visible.has(e.to)) {
      lines.push(`  ${nid(e.from)} --> ${nid(e.to)}`);
    }
  });

  // Styles.
  lines.push("  classDef root fill:#f38ba8,stroke:#f38ba8,color:#1e1e2e,stroke-width:2px");
  lines.push("  classDef tbl fill:#89b4fa,stroke:#89b4fa,color:#1e1e2e");
  lines.push("  classDef view fill:#a6e3a1,stroke:#a6e3a1,color:#1e1e2e");
  lines.push("  classDef ext fill:#585b70,stroke:#585b70,color:#cdd6f4");

  const graphDef = lines.join("\n");

  const area = document.getElementById("graph-area");
  area.innerHTML = `<pre class="mermaid">${graphDef}</pre>`;
  await mermaid.run({ nodes: area.querySelectorAll(".mermaid") });

  // Attach click (navigate) and double-click (show SQL) to rendered nodes.
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

// Search.
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
</html>
"##;
