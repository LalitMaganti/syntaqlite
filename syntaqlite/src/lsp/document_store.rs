// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Per-document state and storage for the LSP host.
//!
//! Owns the URI → [`Document`] map and is responsible for inserting, updating,
//! removing, and looking up documents. Analysis caches live on the individual
//! [`Document`] values; the store exposes the map to analysis dispatchers via
//! [`DocumentStore::get`] and [`DocumentStore::get_mut`].

use std::collections::HashMap;

use crate::analysis::engine::tokens::SemanticToken;

use super::analysis_data::DocumentAnalysisData;
use crate::analysis::diagnostics::Diagnostic;

/// A single open document with lazily populated analysis caches.
pub(crate) struct Document {
    pub(crate) version: i32,
    pub(crate) source: String,
    /// Cached parse-error diagnostics. `None` when dirty.
    pub(crate) cached_parse_diags: Option<Vec<Diagnostic>>,
    /// All diagnostics produced by the last analysis pass (parse + semantic).
    pub(crate) cached_all_diags: Option<Vec<Diagnostic>>,
    /// Analysis events captured by `LspObserver`. `None` when dirty.
    pub(crate) analysis: Option<DocumentAnalysisData>,
    /// Semantic tokens derived from `analysis`. Populated on first request.
    pub(crate) cached_sem_tokens: Option<Vec<SemanticToken>>,
}

impl Document {
    pub(crate) fn invalidate(&mut self) {
        self.cached_parse_diags = None;
        self.cached_all_diags = None;
        self.analysis = None;
        self.cached_sem_tokens = None;
    }
}

/// Storage for all open documents keyed by URI.
pub(crate) struct DocumentStore {
    documents: HashMap<String, Document>,
}

impl DocumentStore {
    pub(crate) fn new() -> Self {
        DocumentStore {
            documents: HashMap::new(),
        }
    }

    /// Register a newly opened document.
    pub(crate) fn open(&mut self, uri: &str, version: i32, text: String) {
        self.documents.insert(
            uri.to_string(),
            Document {
                version,
                source: text,
                cached_parse_diags: None,
                cached_all_diags: None,
                analysis: None,
                cached_sem_tokens: None,
            },
        );
    }

    /// Update a document's content, invalidating cached analysis.
    /// Opens the document if it is not yet present.
    pub(crate) fn update(&mut self, uri: &str, version: i32, text: String) {
        if let Some(doc) = self.documents.get_mut(uri) {
            doc.version = version;
            doc.source = text;
            doc.invalidate();
        } else {
            self.open(uri, version, text);
        }
    }

    /// Remove a document from the store.
    pub(crate) fn close(&mut self, uri: &str) {
        self.documents.remove(uri);
    }

    /// Source text for a document.
    pub(crate) fn source(&self, uri: &str) -> Option<&str> {
        self.documents.get(uri).map(|doc| doc.source.as_str())
    }

    /// Immutable access to a document.
    pub(crate) fn get(&self, uri: &str) -> Option<&Document> {
        self.documents.get(uri)
    }

    /// Mutable access to a document.
    pub(crate) fn get_mut(&mut self, uri: &str) -> Option<&mut Document> {
        self.documents.get_mut(uri)
    }

    /// Invalidate cached analysis on every open document.
    pub(crate) fn invalidate_all(&mut self) {
        for doc in self.documents.values_mut() {
            doc.invalidate();
        }
    }

    /// All URIs currently tracked by the store.
    pub(crate) fn uris(&self) -> Vec<String> {
        self.documents.keys().cloned().collect()
    }
}
