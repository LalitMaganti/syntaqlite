//! ASCII text output: summary tables and per-definition dependency trees.

use std::collections::BTreeMap;
use std::io::Write;

use crate::model::DependencyGraph;

/// Write a summary table of definitions grouped by module.
pub(crate) fn write_summary(graph: &DependencyGraph, w: &mut dyn Write) -> Result<(), String> {
    let wr = |e: std::io::Error| format!("write error: {e}");

    writeln!(w).map_err(wr)?;
    writeln!(
        w,
        "  {:<40} {:>6} {:>6} {:>6}",
        "Module", "Tables", "Views", "Total"
    )
    .map_err(wr)?;
    writeln!(w, "  {}", "-".repeat(58)).map_err(wr)?;

    let mut by_top: BTreeMap<String, [usize; 2]> = BTreeMap::new();
    for def in &graph.definitions {
        let top = def
            .module
            .split('.')
            .next()
            .unwrap_or(&def.module)
            .to_string();
        let counts = by_top.entry(top).or_insert([0; 2]);
        if def.is_view {
            counts[1] += 1;
        } else {
            counts[0] += 1;
        }
    }

    let mut total = [0usize; 2];
    for (module, counts) in &by_top {
        let sum: usize = counts.iter().sum();
        writeln!(
            w,
            "  {:<40} {:>6} {:>6} {:>6}",
            module, counts[0], counts[1], sum
        )
        .map_err(wr)?;
        total[0] += counts[0];
        total[1] += counts[1];
    }

    let grand: usize = total.iter().sum();
    writeln!(w, "  {}", "-".repeat(58)).map_err(wr)?;
    writeln!(
        w,
        "  {:<40} {:>6} {:>6} {:>6}",
        "TOTAL", total[0], total[1], grand
    )
    .map_err(wr)?;
    writeln!(w).map_err(wr)?;
    writeln!(
        w,
        "  External references: {}",
        graph.external_refs.len()
    )
    .map_err(wr)?;
    writeln!(w, "  Dependency edges: {}", graph.edges.len()).map_err(wr)?;
    writeln!(w).map_err(wr)?;

    Ok(())
}

/// Write a dependency tree for a specific definition.
pub(crate) fn write_table_detail(
    graph: &DependencyGraph,
    name: &str,
    transitive: bool,
    w: &mut dyn Write,
) -> Result<(), String> {
    let wr = |e: std::io::Error| format!("write error: {e}");

    let def = graph
        .definitions
        .iter()
        .find(|d| d.name.eq_ignore_ascii_case(name));

    let Some(def) = def else {
        if graph.external_refs.contains(name) {
            writeln!(w, "\n  {name} (external/builtin)").map_err(wr)?;
            let dependents = graph.dependents_of(name);
            if !dependents.is_empty() {
                writeln!(w, "\n  Dependents:").map_err(wr)?;
                for e in &dependents {
                    writeln!(w, "    -> {}", e.from).map_err(wr)?;
                }
            }
            writeln!(w).map_err(wr)?;
            return Ok(());
        }
        return Err(format!(
            "definition '{name}' not found. Use --table with a valid name."
        ));
    };

    let kind_label = if def.is_view { "view" } else { "table" };
    writeln!(w).map_err(wr)?;
    writeln!(
        w,
        "  {} ({kind_label}, module: {}, file: {})",
        def.name, def.module, def.file
    )
    .map_err(wr)?;

    let deps = graph.deps_of(name);
    if !deps.is_empty() {
        writeln!(w, "\n  Dependencies:").map_err(wr)?;
        for e in &deps {
            let ext = if graph.external_refs.contains(&e.to) {
                " (external)"
            } else {
                ""
            };
            writeln!(w, "    <- {}{ext}", e.to).map_err(wr)?;
        }
    }

    let dependents = graph.dependents_of(name);
    if !dependents.is_empty() {
        writeln!(w, "\n  Dependents:").map_err(wr)?;
        for e in &dependents {
            writeln!(w, "    -> {}", e.from).map_err(wr)?;
        }
    }

    if transitive {
        let trans_deps = graph.transitive_deps(name);
        if !trans_deps.is_empty() {
            writeln!(w, "\n  Transitive dependencies ({}):", trans_deps.len()).map_err(wr)?;
            for dep in &trans_deps {
                let ext = if graph.external_refs.contains(dep) {
                    " (external)"
                } else {
                    ""
                };
                writeln!(w, "    {dep}{ext}").map_err(wr)?;
            }
        }

        let trans_dependents = graph.transitive_dependents(name);
        if !trans_dependents.is_empty() {
            writeln!(
                w,
                "\n  Transitive dependents ({}):",
                trans_dependents.len()
            )
            .map_err(wr)?;
            for dep in &trans_dependents {
                writeln!(w, "    {dep}").map_err(wr)?;
            }
        }
    }

    writeln!(w).map_err(wr)?;
    Ok(())
}
