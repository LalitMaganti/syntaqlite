// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Module resolution for `INCLUDE` statements during analysis.

/// Callback for resolving `INCLUDE PERFETTO MODULE` (or similar) import
/// statements during analysis.
///
/// Implement this trait to teach the [`Analyzer`] how to locate
/// module source text. The analyzer calls [`resolve`](Self::resolve) when
/// it encounters an import statement; if the module is found, its DDL is
/// analyzed and accumulated into the catalog so that subsequent statements
/// can reference the imported definitions.
///
/// [`Analyzer`]: super::Analyzer
///
/// # Example
///
/// ```
/// use syntaqlite::analysis::ModuleResolver;
///
/// struct InMemoryResolver {
///     modules: std::collections::HashMap<String, String>,
/// }
///
/// impl ModuleResolver for InMemoryResolver {
///     fn resolve(&self, module_path: &str) -> Option<String> {
///         self.modules.get(module_path).cloned()
///     }
/// }
/// ```
pub trait ModuleResolver {
    /// Given a dotted module path (e.g. `"slices.flow"`), return the SQL
    /// source text for that module.
    ///
    /// Return `None` if the module cannot be found — the analyzer will emit
    /// an [`UnknownModule`](super::DiagnosticMessage::UnknownModule) diagnostic.
    fn resolve(&self, module_path: &str) -> Option<String>;
}

/// A [`ModuleResolver`] that maps dotted module paths to files under a
/// directory root.
///
/// The path `slices.flow` is resolved to `<root>/slices/flow.sql`.
///
/// # Example
///
/// ```no_run
/// use syntaqlite::analysis::{DirectoryModuleResolver, ModuleResolver};
/// use std::path::PathBuf;
///
/// let resolver = DirectoryModuleResolver::new(PathBuf::from("stdlib/"));
/// assert_eq!(
///     resolver.resolve("slices.flow").is_some(),
///     std::path::Path::new("stdlib/slices/flow.sql").exists(),
/// );
/// ```
pub struct DirectoryModuleResolver {
    root: std::path::PathBuf,
}

impl DirectoryModuleResolver {
    /// Create a resolver rooted at the given directory.
    pub fn new(root: std::path::PathBuf) -> Self {
        Self { root }
    }
}

impl ModuleResolver for DirectoryModuleResolver {
    fn resolve(&self, module_path: &str) -> Option<String> {
        let rel = module_path.replace('.', std::path::MAIN_SEPARATOR_STR);
        let path = self.root.join(rel).with_extension("sql");
        std::fs::read_to_string(path).ok()
    }
}
