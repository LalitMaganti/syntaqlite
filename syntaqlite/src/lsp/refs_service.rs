// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Cross-document reference tracking: find-references, rename, external
//! definition-site lookup.
//!
//! Pure functions over [`DocumentStore`] + [`SemanticAnalyzer`] state. The
//! LSP facade assembles the call by threading its borrowed state in.

use std::collections::HashMap;

use syntaqlite_syntax::source::{DocOffset, DocRange};

use crate::semantic::ValidationConfig;
use crate::semantic::analyzer::SemanticAnalyzer;
use crate::semantic::catalog::Catalog;

use super::analysis_data::{ExternalDefinitions, SymbolIdentity};
use super::document_store::DocumentStore;
use super::host::{SchemaMap, ensure_analysis_for};

/// Find all references to the symbol at `offset` across all open documents.
///
/// When `include_declaration` is true, the definition site (in-file or
/// external) is included in the results. Returns an empty vec when the
/// cursor is not on a resolvable symbol.
#[expect(clippy::too_many_arguments)]
pub(super) fn find_references(
    documents: &mut DocumentStore,
    analyzer: &mut SemanticAnalyzer,
    mut schema_map: Option<&mut SchemaMap>,
    user_catalog: &mut Catalog,
    validation_config: &ValidationConfig,
    external_defs: &ExternalDefinitions,
    uri: &str,
    offset: DocOffset,
    include_declaration: bool,
) -> Vec<(String, DocRange)> {
    // Identify the symbol at the cursor.
    if !ensure_analysis_for(
        uri,
        documents,
        analyzer,
        schema_map.as_deref_mut(),
        user_catalog,
        validation_config,
        Some(external_defs),
    ) {
        return Vec::new();
    }
    let identity = documents
        .get(uri)
        .and_then(|doc| doc.analysis.as_ref())
        .and_then(|data| super::hover_service::symbol_identity_at(data, offset));
    let Some(identity) = identity else {
        return Vec::new();
    };

    let mut results = Vec::new();

    // Collect matching resolutions from all open documents.
    let uris: Vec<String> = documents.uris();
    for doc_uri in &uris {
        ensure_analysis_for(
            doc_uri,
            documents,
            analyzer,
            schema_map.as_deref_mut(),
            user_catalog,
            validation_config,
            Some(external_defs),
        );
        let doc = documents
            .get(doc_uri.as_str())
            .expect("doc_uri came from keys()");
        let data = doc
            .analysis
            .as_ref()
            .expect("ensure_analysis sets analysis");
        for range in data.references_matching(&identity) {
            results.push((doc_uri.clone(), range));
        }
        if include_declaration {
            let key = identity.definition_key();
            if let Some(&range) = data.definition_offsets.get(&key) {
                // Avoid duplicates (definition might also be in resolutions).
                let already = results.iter().any(|(u, r)| u == doc_uri && *r == range);
                if !already {
                    results.push((doc_uri.clone(), range));
                }
            }
        }
    }

    // Include external (schema) definition site if requested.
    if include_declaration
        && let Some(def_site) = external_definition_site(external_defs, &identity)
    {
        let already = results
            .iter()
            .any(|(u, r)| *u == def_site.0 && *r == def_site.1);
        if !already {
            results.push(def_site);
        }
    }

    results
}

/// Rename the symbol at `offset` to `new_name` across all open documents.
///
/// Returns a map of `uri -> Vec<(range, new_text)>` edits.
#[expect(clippy::too_many_arguments)]
pub(super) fn rename(
    documents: &mut DocumentStore,
    analyzer: &mut SemanticAnalyzer,
    schema_map: Option<&mut SchemaMap>,
    user_catalog: &mut Catalog,
    validation_config: &ValidationConfig,
    external_defs: &ExternalDefinitions,
    uri: &str,
    offset: DocOffset,
    new_name: &str,
) -> HashMap<String, Vec<(DocRange, String)>> {
    let refs = find_references(
        documents,
        analyzer,
        schema_map,
        user_catalog,
        validation_config,
        external_defs,
        uri,
        offset,
        true,
    );
    let mut edits: HashMap<String, Vec<(DocRange, String)>> = HashMap::new();
    for (ref_uri, range) in refs {
        edits
            .entry(ref_uri)
            .or_default()
            .push((range, new_name.to_string()));
    }
    edits
}

/// Look up an external (schema-file) definition site for a symbol.
pub(super) fn external_definition_site(
    external_defs: &ExternalDefinitions,
    identity: &SymbolIdentity,
) -> Option<(String, DocRange)> {
    match identity {
        SymbolIdentity::Table(name) => {
            let site = external_defs.relation(name)?;
            Some((site.file_uri.clone(), site.range))
        }
        SymbolIdentity::Column { table, column } => {
            let site = external_defs.column(table, column)?;
            Some((site.file_uri.clone(), site.range))
        }
    }
}
