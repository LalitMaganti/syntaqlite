//! Graphviz DOT output format.

use std::collections::{BTreeMap, BTreeSet};
use std::io::Write;

use crate::model::DependencyGraph;

/// Write the dependency graph in Graphviz DOT format.
pub(crate) fn write_dot(
    graph: &DependencyGraph,
    w: &mut dyn Write,
    cluster_by_module: bool,
) -> Result<(), String> {
    let wr = |e: std::io::Error| format!("write error: {e}");

    writeln!(w, "digraph perfetto_stdlib {{").map_err(wr)?;
    writeln!(w, "  rankdir=LR;").map_err(wr)?;
    writeln!(w, "  node [fontname=\"Helvetica\" fontsize=10];").map_err(wr)?;
    writeln!(w).map_err(wr)?;

    if cluster_by_module {
        write_clustered(graph, w)?;
    } else {
        write_flat(graph, w)?;
    }

    writeln!(w).map_err(wr)?;
    for edge in &graph.edges {
        writeln!(
            w,
            "  \"{}\" -> \"{}\";",
            dot_escape(&edge.from),
            dot_escape(&edge.to)
        )
        .map_err(wr)?;
    }

    writeln!(w, "}}").map_err(wr)?;
    Ok(())
}

/// Write the DOT for a subgraph centered on a specific table.
pub(crate) fn write_dot_for_table(
    graph: &DependencyGraph,
    name: &str,
    w: &mut dyn Write,
) -> Result<(), String> {
    let wr = |e: std::io::Error| format!("write error: {e}");

    let deps = graph.transitive_deps(name);
    let dependents = graph.transitive_dependents(name);
    let mut nodes: BTreeSet<&str> = BTreeSet::new();
    nodes.insert(name);
    for d in &deps {
        nodes.insert(d.as_str());
    }
    for d in &dependents {
        nodes.insert(d.as_str());
    }

    writeln!(w, "digraph {name} {{").map_err(wr)?;
    writeln!(w, "  rankdir=LR;").map_err(wr)?;
    writeln!(w, "  node [fontname=\"Helvetica\" fontsize=10];").map_err(wr)?;
    writeln!(w).map_err(wr)?;

    for def in &graph.definitions {
        if nodes.contains(def.name.as_str()) {
            let color = if def.is_view { "#50c878" } else { "#4a90d9" };
            let shape = if def.is_view { "ellipse" } else { "box" };
            let bold = if def.name == name {
                " penwidth=3"
            } else {
                ""
            };
            writeln!(
                w,
                "  \"{}\" [shape={shape} style=filled fillcolor=\"{color}\"{bold}];",
                dot_escape(&def.name)
            )
            .map_err(wr)?;
        }
    }

    for ext in &graph.external_refs {
        if nodes.contains(ext.as_str()) {
            writeln!(
                w,
                "  \"{}\" [shape=box style=dashed fillcolor=\"#f0f0f0\"];",
                dot_escape(ext)
            )
            .map_err(wr)?;
        }
    }

    writeln!(w).map_err(wr)?;
    for edge in &graph.edges {
        if nodes.contains(edge.from.as_str()) && nodes.contains(edge.to.as_str()) {
            writeln!(
                w,
                "  \"{}\" -> \"{}\";",
                dot_escape(&edge.from),
                dot_escape(&edge.to)
            )
            .map_err(wr)?;
        }
    }

    writeln!(w, "}}").map_err(wr)?;
    Ok(())
}

fn write_flat(graph: &DependencyGraph, w: &mut dyn Write) -> Result<(), String> {
    let wr = |e: std::io::Error| format!("write error: {e}");

    for def in &graph.definitions {
        let color = if def.is_view { "#50c878" } else { "#4a90d9" };
        let shape = if def.is_view { "ellipse" } else { "box" };
        writeln!(
            w,
            "  \"{}\" [shape={shape} style=filled fillcolor=\"{color}\"];",
            dot_escape(&def.name)
        )
        .map_err(wr)?;
    }

    for ext in &graph.external_refs {
        writeln!(
            w,
            "  \"{}\" [shape=box style=dashed fillcolor=\"#f0f0f0\"];",
            dot_escape(ext)
        )
        .map_err(wr)?;
    }

    Ok(())
}

fn write_clustered(graph: &DependencyGraph, w: &mut dyn Write) -> Result<(), String> {
    let wr = |e: std::io::Error| format!("write error: {e}");

    let mut by_module: BTreeMap<&str, Vec<&crate::model::Definition>> = BTreeMap::new();
    for def in &graph.definitions {
        let top = def.module.split('.').next().unwrap_or(&def.module);
        by_module.entry(top).or_default().push(def);
    }

    for (i, (module, defs)) in by_module.iter().enumerate() {
        writeln!(w, "  subgraph cluster_{i} {{").map_err(wr)?;
        writeln!(w, "    label=\"{module}\";").map_err(wr)?;
        writeln!(w, "    style=rounded;").map_err(wr)?;

        for def in defs {
            let color = if def.is_view { "#50c878" } else { "#4a90d9" };
            let shape = if def.is_view { "ellipse" } else { "box" };
            writeln!(
                w,
                "    \"{}\" [shape={shape} style=filled fillcolor=\"{color}\"];",
                dot_escape(&def.name)
            )
            .map_err(wr)?;
        }
        writeln!(w, "  }}").map_err(wr)?;
    }

    for ext in &graph.external_refs {
        writeln!(
            w,
            "  \"{}\" [shape=box style=dashed fillcolor=\"#f0f0f0\"];",
            dot_escape(ext)
        )
        .map_err(wr)?;
    }

    Ok(())
}

fn dot_escape(s: &str) -> String {
    s.replace('\\', "\\\\").replace('"', "\\\"")
}
