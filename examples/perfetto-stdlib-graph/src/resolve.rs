//! Graph construction from extraction results.

use std::collections::{BTreeSet, HashMap};

use crate::extract::Extraction;
use crate::model::{DependencyGraph, Stats};

/// Build the complete dependency graph from an extraction.
pub(crate) fn build_graph(ext: Extraction) -> DependencyGraph {
    let definitions = ext.definitions;
    let edges = ext.edges;

    let known_names: HashMap<String, usize> = definitions
        .iter()
        .enumerate()
        .map(|(i, d)| (d.name.to_ascii_lowercase(), i))
        .collect();

    let mut external_refs = BTreeSet::new();
    for edge in &edges {
        if !known_names.contains_key(&edge.to.to_ascii_lowercase()) {
            external_refs.insert(edge.to.clone());
        }
    }

    let stats = Stats {
        total_definitions: definitions.len(),
        tables: definitions.iter().filter(|d| !d.is_view).count(),
        views: definitions.iter().filter(|d| d.is_view).count(),
        total_edges: edges.len(),
        external_refs: external_refs.len(),
    };

    DependencyGraph {
        definitions,
        edges,
        external_refs,
        stats,
    }
}
