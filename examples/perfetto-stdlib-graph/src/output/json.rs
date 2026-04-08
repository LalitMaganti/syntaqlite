//! JSON output format.

use std::io::Write;

use crate::model::DependencyGraph;

/// Write the dependency graph as JSON.
pub(crate) fn write_json(
    graph: &DependencyGraph,
    writer: &mut dyn Write,
    pretty: bool,
) -> Result<(), String> {
    let result = if pretty {
        serde_json::to_writer_pretty(writer, graph)
    } else {
        serde_json::to_writer(writer, graph)
    };
    result.map_err(|e| format!("JSON serialization error: {e}"))
}
