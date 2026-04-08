//! Extraction of definitions and table references from `PerfettoSQL` stdlib
//! files using syntaqlite's semantic analyzer. Zero text scanning — all
//! information comes from the analyzer's per-statement API.

use std::collections::HashSet;
use std::path::Path;

use syntaqlite::any::AnyDialect;
use syntaqlite::semantic::DirectoryModuleResolver;
use syntaqlite::{Catalog, SemanticAnalyzer, ValidationConfig};

use crate::model::{Definition, Edge, EdgeKind};

/// Result of extracting all stdlib files.
pub(crate) struct Extraction {
    /// All definitions found.
    pub(crate) definitions: Vec<Definition>,
    /// Dependency edges (table references from DDL bodies).
    pub(crate) edges: Vec<Edge>,
}

/// Walk a stdlib directory and extract definitions + table references.
///
/// For each `.sql` file, creates a `SemanticAnalyzer` with
/// `DirectoryModuleResolver` and calls `analyze()`. Per-statement results
/// provide:
/// - `defined_relations()` — what DDL this statement created (table or view)
/// - `relations_accessed()` — what tables/views the inner SELECT references
///
/// Cross-file dependencies are resolved automatically by the module resolver
/// when a file uses `INCLUDE PERFETTO MODULE`.
pub(crate) fn extract_stdlib(
    root: &Path,
    dialect: AnyDialect,
) -> Result<Extraction, String> {
    let mut sql_files = Vec::new();
    collect_sql_files(root, root, &mut sql_files)?;
    sql_files.sort_by(|a, b| a.0.cmp(&b.0));

    let config = ValidationConfig::default();
    let mut definitions = Vec::new();
    let mut edges = Vec::new();

    for (rel_path, contents) in &sql_files {
        let module = path_to_module(rel_path);

        let mut analyzer = SemanticAnalyzer::with_dialect(dialect.clone())
            .with_module_resolver(Box::new(DirectoryModuleResolver::new(root.to_path_buf())));

        let catalog = Catalog::new(dialect.clone());
        let model = analyzer.analyze(contents, &catalog, &config);

        // Process each statement in the file.
        let mut seen = HashSet::new();
        for stmt in model.statements() {
            // Collect definitions (what this DDL created).
            let stmt_defs: Vec<_> = stmt
                .defined_relations()
                .iter()
                .map(|dr| Definition {
                    name: dr.name.clone(),
                    is_view: dr.is_view,
                    module: module.clone(),
                    file: rel_path.clone(),
                    is_private: dr.name.starts_with('_'),
                    sql: stmt.source().to_string(),
                })
                .collect();

            // Collect edges (what the inner SELECT references).
            if let Some(rels) = stmt.relations_accessed() {
                let rels = rels.into_inner();
                for rel in rels {
                    for def in &stmt_defs {
                        if rel.name == def.name {
                            continue;
                        }
                        let key = (def.name.clone(), rel.name.clone());
                        if seen.insert(key) {
                            edges.push(Edge {
                                from: def.name.clone(),
                                to: rel.name.clone(),
                                kind: EdgeKind::TableRef,
                            });
                        }
                    }
                }
            }

            definitions.extend(stmt_defs);
        }
    }

    edges.sort_by(|a, b| (&a.from, &a.to).cmp(&(&b.from, &b.to)));
    edges.dedup();

    Ok(Extraction {
        definitions,
        edges,
    })
}

fn collect_sql_files(
    root: &Path,
    dir: &Path,
    out: &mut Vec<(String, String)>,
) -> Result<(), String> {
    let entries = std::fs::read_dir(dir)
        .map_err(|e| format!("failed to read {}: {e}", dir.display()))?;
    for entry in entries {
        let entry = entry.map_err(|e| format!("dir entry error: {e}"))?;
        let path = entry.path();
        if path.is_dir() {
            collect_sql_files(root, &path, out)?;
        } else if path.extension().is_some_and(|ext| ext == "sql") {
            let rel = path
                .strip_prefix(root)
                .map_err(|e| e.to_string())?
                .to_string_lossy()
                .to_string();
            let contents = std::fs::read_to_string(&path)
                .map_err(|e| format!("failed to read {}: {e}", path.display()))?;
            out.push((rel, contents));
        }
    }
    Ok(())
}

fn path_to_module(rel_path: &str) -> String {
    rel_path
        .strip_suffix(".sql")
        .unwrap_or(rel_path)
        .replace('/', ".")
}
