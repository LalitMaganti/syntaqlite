//! Data model for the `PerfettoSQL` standard library dependency graph.

use std::collections::BTreeSet;

use serde::Serialize;

/// A table or view definition extracted from the stdlib.
#[derive(Debug, Clone, Serialize)]
pub(crate) struct Definition {
    /// Definition name.
    pub(crate) name: String,
    /// Whether this is a view (vs a table).
    pub(crate) is_view: bool,
    /// Dotted module path (e.g. `android.binder`).
    pub(crate) module: String,
    /// File path relative to the stdlib root.
    pub(crate) file: String,
    /// Whether the name starts with `_` (Perfetto private convention).
    pub(crate) is_private: bool,
    /// The SQL source text for this statement.
    pub(crate) sql: String,
}

/// Kind of dependency edge.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord, Serialize)]
#[serde(rename_all = "snake_case")]
pub(crate) enum EdgeKind {
    /// `FROM` / `JOIN` reference in the body.
    TableRef,
}

/// A directed edge in the dependency graph.
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize)]
pub(crate) struct Edge {
    /// Source definition name (the one that depends).
    pub(crate) from: String,
    /// Target definition or external table name (the dependency).
    pub(crate) to: String,
    /// What kind of dependency this is.
    pub(crate) kind: EdgeKind,
}

/// Summary statistics.
#[derive(Debug, Clone, Serialize)]
pub(crate) struct Stats {
    /// Total number of definitions.
    pub(crate) total_definitions: usize,
    /// Tables.
    pub(crate) tables: usize,
    /// Views.
    pub(crate) views: usize,
    /// Total dependency edges.
    pub(crate) total_edges: usize,
    /// Number of external (unresolved) references.
    pub(crate) external_refs: usize,
}

/// The complete dependency graph.
#[derive(Debug, Serialize)]
pub(crate) struct DependencyGraph {
    /// All definitions found in the stdlib.
    pub(crate) definitions: Vec<Definition>,
    /// Dependency edges between definitions.
    pub(crate) edges: Vec<Edge>,
    /// Names referenced but never defined in the stdlib (intrinsics/builtins).
    pub(crate) external_refs: BTreeSet<String>,
    /// Summary statistics.
    pub(crate) stats: Stats,
}

impl DependencyGraph {
    /// Get all edges originating from a definition (its dependencies).
    pub(crate) fn deps_of(&self, name: &str) -> Vec<&Edge> {
        self.edges.iter().filter(|e| e.from == name).collect()
    }

    /// Get all edges pointing to a definition (its dependents).
    pub(crate) fn dependents_of(&self, name: &str) -> Vec<&Edge> {
        self.edges.iter().filter(|e| e.to == name).collect()
    }

    /// Compute transitive closure of dependencies (BFS).
    pub(crate) fn transitive_deps(&self, name: &str) -> BTreeSet<String> {
        let mut visited = BTreeSet::new();
        let mut queue = std::collections::VecDeque::new();
        queue.push_back(name.to_string());
        while let Some(current) = queue.pop_front() {
            if !visited.insert(current.clone()) {
                continue;
            }
            for edge in &self.edges {
                if edge.from == current && !visited.contains(&edge.to) {
                    queue.push_back(edge.to.clone());
                }
            }
        }
        visited.remove(name);
        visited
    }

    /// Compute transitive closure of dependents (reverse BFS).
    pub(crate) fn transitive_dependents(&self, name: &str) -> BTreeSet<String> {
        let mut visited = BTreeSet::new();
        let mut queue = std::collections::VecDeque::new();
        queue.push_back(name.to_string());
        while let Some(current) = queue.pop_front() {
            if !visited.insert(current.clone()) {
                continue;
            }
            for edge in &self.edges {
                if edge.to == current && !visited.contains(&edge.from) {
                    queue.push_back(edge.from.clone());
                }
            }
        }
        visited.remove(name);
        visited
    }
}
