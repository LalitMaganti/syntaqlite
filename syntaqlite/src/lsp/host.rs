// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::collections::HashMap;
use std::path::{Path, PathBuf};

use syntaqlite_syntax::any::TokenCategory;
use syntaqlite_syntax::util::is_suggestable_keyword;

use crate::dialect::AnyDialect;
use crate::fmt::FormatConfig;
use crate::fmt::formatter::Formatter;
use crate::semantic::Catalog;
use crate::semantic::ValidationConfig;
use crate::semantic::analyzer::SemanticAnalyzer;
use crate::semantic::diagnostics::Diagnostic;
use crate::semantic::model::{
    DefinitionResult, ResolvedSymbol, SemanticModel, SemanticToken, StoredToken, SymbolIdentity,
};

use super::{CompletionEntry, CompletionInfo, CompletionKind};

// ── SchemaMap ─────────────────────────────────────────────────────────────────

/// Per-file schema resolution: maps glob patterns to pre-built [`Catalog`]s.
///
/// The LSP uses this to resolve the correct schema catalog for each open
/// document based on the `[schemas]` section of `syntaqlite.toml`. Files
/// matching a glob pattern get that pattern's catalog with `strict_schema=true`;
/// unmatched files fall back to the `default` catalog (from the top-level
/// `schema` key) or no catalog at all.
pub struct SchemaMap {
    config_dir: PathBuf,
    default: Option<Catalog>,
    entries: Vec<(glob::Pattern, Catalog)>,
}

impl SchemaMap {
    /// Create a new schema map.
    ///
    /// - `config_dir`: the directory containing `syntaqlite.toml` (used to
    ///   resolve relative glob patterns against file URIs).
    /// - `default`: catalog built from the top-level `schema` key (if any).
    /// - `entries`: `(glob_pattern, catalog)` pairs from `[schemas]`.
    pub fn new(
        config_dir: PathBuf,
        default: Option<Catalog>,
        entries: Vec<(glob::Pattern, Catalog)>,
    ) -> Self {
        SchemaMap {
            config_dir,
            default,
            entries,
        }
    }

    /// Resolve a `file://` URI to its matching catalog.
    ///
    /// Returns `Some((catalog, true))` if a glob pattern matched (strict mode),
    /// `Some((catalog, false))` if only the default catalog matched (strict mode
    /// for default too), or `None` if no catalog applies.
    pub fn resolve(&self, file_uri: &str) -> Option<(&Catalog, bool)> {
        let path = file_uri.strip_prefix("file://")?;
        let path = Path::new(path);
        let relative = path.strip_prefix(&self.config_dir).ok()?;
        let relative_str = relative.to_string_lossy();
        let match_opts = glob::MatchOptions {
            require_literal_separator: true,
            ..Default::default()
        };
        for (pattern, catalog) in &self.entries {
            if pattern.matches_with(&relative_str, match_opts) {
                return Some((catalog, true));
            }
        }
        self.default.as_ref().map(|cat| (cat, true))
    }
}

// ── Document store ────────────────────────────────────────────────────────────

struct Document {
    version: i32,
    source: String,
    /// Cached analysis result. `None` when dirty (source changed or catalog changed).
    model: Option<SemanticModel>,
    /// Parse errors from the last analysis (derived from `model`).
    cached_parse_diags: Option<Vec<Diagnostic>>,
    /// Semantic tokens from the last analysis (derived from `model`).
    cached_sem_tokens: Option<Vec<SemanticToken>>,
}

/// Run analysis for `doc` if no model is cached yet.
fn ensure_model(
    doc: &mut Document,
    analyzer: &mut SemanticAnalyzer,
    user_catalog: &Catalog,
    validation_config: &ValidationConfig,
) {
    if doc.model.is_some() {
        return;
    }
    let model = analyzer.analyze(&doc.source, user_catalog, validation_config);
    let parse_diags = model
        .diagnostics()
        .filter(|d| d.message().is_parse_error())
        .cloned()
        .collect();
    doc.cached_parse_diags = Some(parse_diags);
    doc.model = Some(model);
}

/// Resolve the correct catalog for `uri` via `schema_map`, then call
/// [`ensure_model`]. Returns `false` if the document is not found.
fn ensure_model_for(
    uri: &str,
    documents: &mut HashMap<String, Document>,
    analyzer: &mut SemanticAnalyzer,
    schema_map: Option<&SchemaMap>,
    user_catalog: &Catalog,
    base_validation_config: &ValidationConfig,
) -> bool {
    let Some(doc) = documents.get_mut(uri) else {
        return false;
    };
    let (catalog, config) = if let Some(map) = schema_map {
        if let Some((cat, _strict)) = map.resolve(uri) {
            (cat, base_validation_config.with_strict_schema())
        } else {
            (user_catalog, *base_validation_config)
        }
    } else {
        (user_catalog, *base_validation_config)
    };
    ensure_model(doc, analyzer, catalog, &config);
    true
}

// ── LspHost ───────────────────────────────────────────────────────────────────

/// Main integration point for embedding syntaqlite analysis in an editor or
/// language-aware tool.
///
/// `LspHost` manages a set of open documents keyed by URI and lazily computes
/// analysis results on first access after each edit. The typical lifecycle is:
///
/// 1. **Open / update** a document with [`update_document`](Self::update_document).
/// 2. **Query** the document for diagnostics, semantic tokens, completions,
///    hover information, signature help, or formatting.
/// 3. **Optionally set schema context** via [`set_session_context`](Self::set_session_context),
///    [`set_session_context_from_ddl`](Self::set_session_context_from_ddl), or
///    [`set_session_context_from_json`](Self::set_session_context_from_json) to
///    enable table/column/function validation.
///
/// Analysis is cached per-document and invalidated automatically when the
/// source text or catalog context changes. Semantic validation delegates to
/// [`SemanticAnalyzer`].
///
/// Use this when you are building an LSP server, a web-based editor plugin,
/// or any tool that needs incremental SQL analysis tied to document identity.
/// For one-shot analysis without document management, use
/// [`SemanticAnalyzer`] directly.
///
/// # Example
///
/// ```
/// use syntaqlite::lsp::LspHost;
///
/// let mut host = LspHost::new(); // SQLite dialect by default
///
/// // Feed a document into the host.
/// host.update_document("file:///query.sql", 1, "SELECT * FROM users;".into());
///
/// // Retrieve semantic tokens for syntax highlighting.
/// let tokens = host.semantic_tokens_encoded("file:///query.sql", None);
///
/// // Retrieve completions at a cursor position.
/// let items = host.completion_items("file:///query.sql", 9);
/// ```
pub struct LspHost {
    dialect: AnyDialect,
    /// User-provided schema (tables, views, functions).
    user_catalog: Catalog,
    analyzer: SemanticAnalyzer,
    documents: HashMap<String, Document>,
    /// Format config from project config file. `None` means use defaults.
    format_config: Option<FormatConfig>,
    /// Validation config (`strict_schema` is set when a schema is provided).
    validation_config: ValidationConfig,
    /// Per-file schema resolution from `[schemas]` globs.
    schema_map: Option<SchemaMap>,
}

#[cfg(feature = "sqlite")]
impl Default for LspHost {
    fn default() -> Self {
        Self::new()
    }
}

impl LspHost {
    /// Create a host for the built-in `SQLite` dialect.
    #[cfg(feature = "sqlite")]
    pub fn new() -> Self {
        let dialect = crate::sqlite::dialect::any_dialect();
        LspHost {
            user_catalog: Catalog::new(dialect.clone()),
            analyzer: SemanticAnalyzer::new(),
            dialect,
            documents: HashMap::new(),
            format_config: None,
            validation_config: ValidationConfig::default(),
            schema_map: None,
        }
    }

    /// Create a host bound to `dialect`.
    pub fn with_dialect(dialect: impl Into<AnyDialect>) -> Self {
        let dialect = dialect.into();
        LspHost {
            user_catalog: Catalog::new(dialect.clone()),
            analyzer: SemanticAnalyzer::with_dialect(dialect.clone()),
            dialect,
            documents: HashMap::new(),
            format_config: None,
            validation_config: ValidationConfig::default(),
            schema_map: None,
        }
    }

    // ── Configuration ─────────────────────────────────────────────────────────

    /// Set the format config from a project config file.
    pub fn set_format_config(&mut self, config: FormatConfig) {
        self.format_config = Some(config);
    }

    /// Get the format config (project config or default).
    pub(crate) fn format_config(&self) -> FormatConfig {
        self.format_config.clone().unwrap_or_default()
    }

    /// Set the validation config.
    pub fn set_validation_config(&mut self, config: ValidationConfig) {
        self.validation_config = config;
        for doc in self.documents.values_mut() {
            doc.model = None;
            doc.cached_parse_diags = None;
            doc.cached_sem_tokens = None;
        }
    }

    /// Set the per-file schema map from `[schemas]` config globs.
    /// When set, each document is resolved against the map to find its
    /// matching catalog and `strict_schema` is set automatically.
    pub fn set_schema_map(&mut self, map: SchemaMap) {
        self.schema_map = Some(map);
        for doc in self.documents.values_mut() {
            doc.model = None;
            doc.cached_parse_diags = None;
            doc.cached_sem_tokens = None;
        }
    }

    /// Set the session context (user-provided schema and functions).
    /// Invalidates all cached analysis.
    pub fn set_session_context(&mut self, ctx: Catalog) {
        self.user_catalog = ctx;
        for doc in self.documents.values_mut() {
            doc.model = None;
            doc.cached_parse_diags = None;
            doc.cached_sem_tokens = None;
        }
    }

    // ── Document lifecycle ─────────────────────────────────────────────────────

    /// Register a newly opened document.
    pub(crate) fn open_document(&mut self, uri: &str, version: i32, text: String) {
        self.documents.insert(
            uri.to_string(),
            Document {
                version,
                source: text,
                model: None,
                cached_parse_diags: None,
                cached_sem_tokens: None,
            },
        );
    }

    /// Update a document's content, invalidating cached analysis.
    pub fn update_document(&mut self, uri: &str, version: i32, text: String) {
        if let Some(doc) = self.documents.get_mut(uri) {
            doc.version = version;
            doc.source = text;
            doc.model = None;
            doc.cached_parse_diags = None;
            doc.cached_sem_tokens = None;
        } else {
            self.open_document(uri, version, text);
        }
    }

    /// Remove a document from the host.
    pub(crate) fn close_document(&mut self, uri: &str) {
        self.documents.remove(uri);
    }

    /// Source text for a document.
    pub(crate) fn document_source(&self, uri: &str) -> Option<&str> {
        self.documents.get(uri).map(|doc| doc.source.as_str())
    }

    // ── Analysis queries ───────────────────────────────────────────────────────

    /// Parse-error diagnostics for a document, lazily computed.
    pub(crate) fn diagnostics(&mut self, uri: &str) -> &[Diagnostic] {
        if !ensure_model_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_ref(),
            &self.user_catalog,
            &self.validation_config,
        ) {
            return &[];
        }
        self.documents
            .get(uri)
            .expect("ensure_model_for guarantees document exists")
            .cached_parse_diags
            .as_deref()
            .expect("ensure_model sets cached_parse_diags")
    }

    /// Semantic tokens delta-encoded for LSP `textDocument/semanticTokens/full`.
    ///
    /// # Panics
    /// Panics if the internal model or token cache is in an inconsistent state
    /// (this indicates a bug in `ensure_model`).
    pub fn semantic_tokens_encoded(
        &mut self,
        uri: &str,
        range: Option<(usize, usize)>,
    ) -> Vec<u32> {
        if !ensure_model_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_ref(),
            &self.user_catalog,
            &self.validation_config,
        ) {
            return Vec::new();
        }
        let doc = self
            .documents
            .get_mut(uri)
            .expect("ensure_model_for guarantees document exists");
        if doc.cached_sem_tokens.is_none() {
            let tokens = doc
                .model
                .as_ref()
                .expect("ensure_model sets model")
                .semantic_tokens(&self.analyzer.dialect());
            doc.cached_sem_tokens = Some(tokens);
        }
        let tokens = doc
            .cached_sem_tokens
            .as_deref()
            .expect("cached_sem_tokens just populated");
        encode_semantic_tokens(&doc.source, tokens, range)
    }

    /// Expected parser tokens and semantic context at a byte offset.
    pub(crate) fn completion_info_at_offset(&mut self, uri: &str, offset: usize) -> CompletionInfo {
        if !ensure_model_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_ref(),
            &self.user_catalog,
            &self.validation_config,
        ) {
            return CompletionInfo {
                tokens: Vec::new(),
                context: super::CompletionContext::Unknown,
                qualifier: None,
            };
        }
        let doc = self
            .documents
            .get(uri)
            .expect("ensure_model_for guarantees document exists");
        super::completion::completion_info(
            &self.analyzer.dialect(),
            doc.model.as_ref().expect("ensure_model sets model"),
            offset,
        )
    }

    /// Completion items (keywords + functions) at a byte offset.
    pub fn completion_items(&mut self, uri: &str, offset: usize) -> Vec<CompletionEntry> {
        use std::collections::HashSet;

        let info = self.completion_info_at_offset(uri, offset);
        let expected_set: HashSet<u32> = info.tokens.iter().map(|&t| u32::from(t)).collect();

        let mut seen: HashSet<String> = HashSet::new();
        let mut items: Vec<CompletionEntry> = Vec::new();

        for entry in self.dialect.keywords() {
            let code = u32::from(entry.token_type());
            if !expected_set.contains(&code) || !is_suggestable_keyword(entry.keyword()) {
                continue;
            }
            if seen.insert(entry.keyword().to_string()) {
                items.push(CompletionEntry::new(
                    entry.keyword().to_string(),
                    CompletionKind::Keyword,
                ));
            }
        }

        let catalog = self.analyzer.catalog();

        // When the cursor follows `qualifier.`, only suggest columns from that
        // table — no keywords, functions, or other tables.
        if let Some(ref qualifier) = info.qualifier {
            items.clear();
            seen.clear();
            for name in catalog.all_column_names(Some(qualifier)) {
                if seen.insert(name.clone()) {
                    items.push(CompletionEntry::new(name, CompletionKind::Column));
                }
            }
            return items;
        }

        match info.context {
            super::CompletionContext::TableRef => {
                for name in catalog.all_relation_names() {
                    if seen.insert(name.clone()) {
                        items.push(CompletionEntry::new(name, CompletionKind::Table));
                    }
                }
            }
            super::CompletionContext::Expression | super::CompletionContext::Unknown => {
                for name in catalog.all_function_names() {
                    if seen.insert(name.clone()) {
                        items.push(CompletionEntry::new(name, CompletionKind::Function));
                    }
                }
                for name in catalog.all_column_names(None) {
                    if seen.insert(name.clone()) {
                        items.push(CompletionEntry::new(name, CompletionKind::Column));
                    }
                }
                for name in catalog.all_relation_names() {
                    if seen.insert(name.clone()) {
                        items.push(CompletionEntry::new(name, CompletionKind::Table));
                    }
                }
            }
        }

        items.sort_by_key(|a| a.kind().sort_priority());
        items
    }

    // ── Semantic validation ────────────────────────────────────────────────────

    /// Version, source text, and all diagnostics (parse + semantic) in one call.
    ///
    /// Reads from the cached model populated by [`ensure_model`] — no re-analysis.
    #[cfg(feature = "lsp")]
    pub(crate) fn document_all_diagnostics(
        &mut self,
        uri: &str,
    ) -> Option<(i32, String, Vec<Diagnostic>)> {
        if !ensure_model_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_ref(),
            &self.user_catalog,
            &self.validation_config,
        ) {
            return None;
        }
        let doc = self
            .documents
            .get(uri)
            .expect("ensure_model_for guarantees document exists");
        let version = doc.version;
        let source = doc.source.clone();
        let diags = doc
            .model
            .as_ref()
            .expect("ensure_model sets model")
            .diagnostics()
            .cloned()
            .collect();
        Some((version, source, diags))
    }

    /// Semantic validation diagnostics for a document (non-parse-error issues only).
    ///
    /// Always re-analyzes with `user_catalog` and `config`; use
    /// [`diagnostics`](Self::diagnostics) for the cheaper cached parse-error path.
    #[cfg(feature = "validation")]
    pub(crate) fn validate(&mut self, uri: &str, config: &ValidationConfig) -> Vec<Diagnostic> {
        let Some(source) = self.documents.get(uri).map(|d| d.source.as_str()) else {
            return Vec::new();
        };
        let model = self.analyzer.analyze(source, &self.user_catalog, config);
        model
            .diagnostics()
            .filter(|d| !d.message.is_parse_error())
            .cloned()
            .collect()
    }

    /// Parse + semantic diagnostics combined.
    #[cfg(feature = "validation")]
    pub fn all_diagnostics(&mut self, uri: &str, config: &ValidationConfig) -> Vec<Diagnostic> {
        let mut result = self.diagnostics(uri).to_vec();
        result.extend(self.validate(uri, config));
        result
    }

    // ── Formatting ────────────────────────────────────────────────────────────

    /// Format a document's source text.
    #[cfg(feature = "fmt")]
    pub(crate) fn format(&self, uri: &str, config: &FormatConfig) -> Result<String, FormatError> {
        let doc = self
            .documents
            .get(uri)
            .ok_or(FormatError::UnknownDocument)?;
        let mut formatter = Formatter::with_dialect_config(self.dialect.clone(), config);
        formatter.format(&doc.source).map_err(FormatError::Format)
    }

    // ── Hover ──────────────────────────────────────────────────────────────────

    /// Hover information at a byte offset: returns (`hover_text`, `token_offset`, `token_length`).
    pub(crate) fn hover_info(
        &mut self,
        uri: &str,
        offset: usize,
    ) -> Option<(String, usize, usize)> {
        ensure_model_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_ref(),
            &self.user_catalog,
            &self.validation_config,
        );
        let doc = self.documents.get(uri)?;
        let model = doc.model.as_ref().expect("ensure_model sets model");

        let resolution = model.resolution_at(offset)?;
        let (start, end) = model
            .resolutions
            .iter()
            .find(|r| offset >= r.start && offset < r.end)
            .map(|r| (r.start, r.end))?;

        let hover = format_resolved_hover(resolution);
        Some((hover, start, end - start))
    }

    // ── Go-to-definition ───────────────────────────────────────────────────

    /// Return the definition location for the symbol at `offset`.
    pub(crate) fn definition_info(&mut self, uri: &str, offset: usize) -> Option<DefinitionResult> {
        ensure_model_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_ref(),
            &self.user_catalog,
            &self.validation_config,
        );
        let doc = self.documents.get(uri)?;
        let model = doc.model.as_ref().expect("ensure_model sets model");
        model.definition_at(offset)
    }

    // ── Find references ──────────────────────────────────────────────────────

    /// Find all references to the symbol at `offset` across all open documents.
    ///
    /// Returns a list of `(uri, start, end)` tuples. When `include_declaration`
    /// is true, the definition site (if known) is included in the results.
    pub(crate) fn find_references(
        &mut self,
        uri: &str,
        offset: usize,
        include_declaration: bool,
    ) -> Vec<(String, usize, usize)> {
        // Identify the symbol at the cursor.
        let identity = self.symbol_identity_at(uri, offset);
        let Some(identity) = identity else {
            return Vec::new();
        };

        let mut results = Vec::new();

        // Collect matching resolutions from all open documents.
        let uris: Vec<String> = self.documents.keys().cloned().collect();
        for doc_uri in &uris {
            ensure_model_for(
                doc_uri,
                &mut self.documents,
                &mut self.analyzer,
                self.schema_map.as_ref(),
                &self.user_catalog,
                &self.validation_config,
            );
            let doc = self
                .documents
                .get(doc_uri.as_str())
                .expect("doc_uri came from keys()");
            let model = doc.model.as_ref().expect("ensure_model sets model");
            for (start, end) in model.references_matching(&identity) {
                results.push((doc_uri.clone(), start, end));
            }
            if include_declaration {
                let key = identity.definition_key();
                if let Some(&(start, end)) = model.definition_offsets.get(&key) {
                    // Avoid duplicates (definition might also be in resolutions).
                    let already = results
                        .iter()
                        .any(|(u, s, e)| u == doc_uri && *s == start && *e == end);
                    if !already {
                        results.push((doc_uri.clone(), start, end));
                    }
                }
            }
        }

        // Include external (schema) definition site if requested.
        if include_declaration && let Some(def_site) = self.external_definition_site(&identity) {
            let already = results
                .iter()
                .any(|(u, s, e)| *u == def_site.0 && *s == def_site.1 && *e == def_site.2);
            if !already {
                results.push(def_site);
            }
        }

        results
    }

    // ── Rename ──────────────────────────────────────────────────────────────

    /// Check if the symbol at `offset` is renameable, returning `(start, end, current_name)`.
    pub(crate) fn prepare_rename(
        &mut self,
        uri: &str,
        offset: usize,
    ) -> Option<(usize, usize, String)> {
        ensure_model_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_ref(),
            &self.user_catalog,
            &self.validation_config,
        );
        let doc = self.documents.get(uri)?;
        let model = doc.model.as_ref().expect("ensure_model sets model");
        let res = model
            .resolutions
            .iter()
            .find(|r| offset >= r.start && offset < r.end)?;
        let name = match &res.symbol {
            ResolvedSymbol::Table { name, .. } => name.clone(),
            ResolvedSymbol::Column { column, .. } => column.clone(),
            ResolvedSymbol::Function { .. } => return None,
        };
        Some((res.start, res.end, name))
    }

    /// Rename the symbol at `offset` to `new_name` across all open documents.
    ///
    /// Returns a map of `uri -> Vec<(start, end, new_text)>` edits.
    pub(crate) fn rename(
        &mut self,
        uri: &str,
        offset: usize,
        new_name: &str,
    ) -> HashMap<String, Vec<(usize, usize, String)>> {
        let refs = self.find_references(uri, offset, true);
        let mut edits: HashMap<String, Vec<(usize, usize, String)>> = HashMap::new();
        for (ref_uri, start, end) in refs {
            edits
                .entry(ref_uri)
                .or_default()
                .push((start, end, new_name.to_string()));
        }
        edits
    }

    // ── Symbol identity helpers ─────────────────────────────────────────────

    /// Determine the symbol identity at `offset` — either from a resolution or
    /// from a definition site (CREATE TABLE / column-def).
    fn symbol_identity_at(&mut self, uri: &str, offset: usize) -> Option<SymbolIdentity> {
        ensure_model_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_ref(),
            &self.user_catalog,
            &self.validation_config,
        );
        let doc = self.documents.get(uri)?;
        let model = doc.model.as_ref().expect("ensure_model sets model");

        // First check resolutions (references in DML/queries).
        if let Some(sym) = model.resolution_at(offset) {
            return SymbolIdentity::from_resolved(sym);
        }

        // Fall back to definition_offsets (cursor on a CREATE TABLE name, etc).
        for (key, &(start, end)) in &model.definition_offsets {
            if offset >= start && offset < end {
                return if let Some((table, col)) = key.split_once('.') {
                    Some(SymbolIdentity::Column {
                        table: table.to_string(),
                        column: col.to_string(),
                    })
                } else {
                    Some(SymbolIdentity::Table(key.clone()))
                };
            }
        }
        None
    }

    /// Look up an external (schema-file) definition site for a symbol from the catalog.
    fn external_definition_site(
        &self,
        identity: &SymbolIdentity,
    ) -> Option<(String, usize, usize)> {
        match identity {
            SymbolIdentity::Table(name) => {
                let site = self.user_catalog.relation_definition_site(name)?;
                Some((site.file_uri.clone(), site.start, site.end))
            }
            SymbolIdentity::Column { table, column } => {
                let site = self.user_catalog.column_definition_site(table, column)?;
                Some((site.file_uri.clone(), site.start, site.end))
            }
        }
    }

    // ── Signature help ────────────────────────────────────────────────────────

    /// Signature help at a byte offset: finds enclosing function call and returns
    /// (`function_name`, `active_parameter`, overloads).
    pub(crate) fn signature_help(&mut self, uri: &str, offset: usize) -> Option<SignatureHelpInfo> {
        ensure_model_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_ref(),
            &self.user_catalog,
            &self.validation_config,
        );
        let doc = self.documents.get(uri)?;
        let model = doc.model.as_ref().expect("ensure_model sets model");
        let source = model.source();

        // Walk backwards from offset to find enclosing `name(` and count commas.
        let before = &source[..offset.min(source.len())];
        let (func_name, active_param) = find_enclosing_call(before, &model.tokens, &self.dialect)?;

        let (_category, arities) = self.user_catalog.function_signature(&func_name)?;

        Some(SignatureHelpInfo {
            name: func_name,
            arities,
            active_parameter: active_param,
        })
    }

    // ── Schema helpers ────────────────────────────────────────────────────────

    /// All function names available given the current dialect and user catalog.
    pub fn available_function_names(&self) -> Vec<String> {
        self.user_catalog.all_function_names()
    }

    /// Parse a JSON schema blob and use it as the session context.
    ///
    /// Convenience wrapper over [`Self::set_session_context`] that constructs a
    /// [`Catalog`] using the host's dialect, avoiding the need for callers to
    /// handle `Dialect` directly.
    ///
    /// # Errors
    ///
    /// Returns an error string if `json` is not a valid schema JSON blob.
    #[cfg(feature = "serde-json")]
    pub fn set_session_context_from_json(&mut self, json: &str) -> Result<(), String> {
        let catalog = Catalog::from_json(self.dialect.clone(), json)?;
        self.set_session_context(catalog);
        Ok(())
    }

    /// Parse DDL statements and use the resulting schema as the session context.
    ///
    /// Convenience wrapper over [`Self::set_session_context`] that constructs a
    /// [`Catalog`] using the host's dialect and DDL source, avoiding the need
    /// for callers to handle `Dialect` directly.
    ///
    /// # Errors
    ///
    /// Returns the parse-error messages (one per failing statement) if the DDL
    /// source contains any syntax errors. Partial results from successfully
    /// parsed statements are still applied as the session context.
    #[cfg(feature = "sqlite")]
    pub fn set_session_context_from_ddl(
        &mut self,
        ddl: &str,
        file_uri: Option<&str>,
    ) -> Result<(), Vec<String>> {
        let (catalog, errors) = Catalog::from_ddl(self.dialect.clone(), &[(ddl, file_uri)]);
        self.set_session_context(catalog);
        if errors.is_empty() {
            Ok(())
        } else {
            Err(errors)
        }
    }
}

// ── Semantic tokens encoding ───────────────────────────────────────────────────

/// Delta-encode semantic tokens as a flat `u32` array (5 values per token:
/// `deltaLine`, `deltaStartChar`, `length`, `legendIndex`, `modifiers`).
///
/// Character offsets and lengths are in UTF-16 code units per the LSP spec.
fn encode_semantic_tokens(
    source: &str,
    semantic_tokens: &[SemanticToken],
    range: Option<(usize, usize)>,
) -> Vec<u32> {
    let src = source.as_bytes();
    let (range_start, range_end) = range.unwrap_or((0, src.len()));

    let mut result = Vec::with_capacity(semantic_tokens.len() * 5);
    let mut prev_line: u32 = 0;
    let mut prev_col: u32 = 0;
    let mut cur_line: u32 = 0;
    let mut cur_col: u32 = 0;
    let mut src_pos: usize = 0;

    for tok in semantic_tokens {
        while src_pos < tok.offset && src_pos < src.len() {
            if src[src_pos] == b'\n' {
                cur_line += 1;
                cur_col = 0;
                src_pos += 1;
            } else {
                let char_len = utf8_char_len(src[src_pos]);
                cur_col += if char_len == 4 { 2 } else { 1 };
                src_pos += char_len;
            }
        }

        if tok.offset < range_start || tok.offset >= range_end {
            continue;
        }
        if matches!(
            tok.category,
            TokenCategory::Other | TokenCategory::Operator | TokenCategory::Punctuation
        ) {
            continue;
        }

        let legend_idx = tok.category as u32;
        let delta_line = cur_line - prev_line;
        let delta_start = if delta_line == 0 {
            cur_col - prev_col
        } else {
            cur_col
        };

        // Compute token length in UTF-16 code units.
        let tok_end = (tok.offset + tok.length).min(src.len());
        let length_utf16 = utf16_len(&src[tok.offset..tok_end]);

        result.push(delta_line);
        result.push(delta_start);
        result.push(length_utf16);
        result.push(legend_idx);
        result.push(0);

        prev_line = cur_line;
        prev_col = cur_col;
    }

    result
}

/// Count the number of UTF-16 code units in a byte slice of valid UTF-8.
fn utf16_len(bytes: &[u8]) -> u32 {
    let mut n = 0u32;
    let mut i = 0;
    while i < bytes.len() {
        let char_len = utf8_char_len(bytes[i]);
        n += if char_len == 4 { 2 } else { 1 };
        i += char_len;
    }
    n
}

use super::utf8_char_len;

// ── Hover/signature helpers ────────────────────────────────────────────────────

use crate::semantic::catalog::AritySpec;

/// Signature help result from the host.
pub(crate) struct SignatureHelpInfo {
    pub name: String,
    pub arities: Vec<AritySpec>,
    pub active_parameter: u32,
}

fn format_resolved_hover(symbol: &ResolvedSymbol) -> String {
    match symbol {
        ResolvedSymbol::Table { name, columns, .. } => match columns {
            Some(cols) => format!("**table** `{name}`\n\n```\n{}\n```", cols.join(", ")),
            None => format!("**table** `{name}`"),
        },
        ResolvedSymbol::Column {
            column,
            table,
            all_columns,
            ..
        } => {
            let col_list: Vec<String> = all_columns
                .iter()
                .map(|c| {
                    if c.eq_ignore_ascii_case(column) {
                        format!("**{c}**")
                    } else {
                        c.clone()
                    }
                })
                .collect();
            format!("**column** in `{table}`\n\n{}", col_list.join(", "))
        }
        ResolvedSymbol::Function {
            category, arities, ..
        } => {
            format!("**{category}**\n\n```\n{}\n```", arities.join("\n"))
        }
    }
}

/// Walk backwards from cursor to find enclosing `func_name(` and count commas
/// to determine active parameter index.
fn find_enclosing_call(
    before: &str,
    tokens: &[StoredToken],
    dialect: &AnyDialect,
) -> Option<(String, u32)> {
    let bytes = before.as_bytes();
    let mut depth: i32 = 0;
    let mut commas: u32 = 0;
    let mut pos = bytes.len();

    // Scan backwards to find the matching `(`.
    while pos > 0 {
        pos -= 1;
        match bytes[pos] {
            b')' => depth += 1,
            b'(' => {
                if depth == 0 {
                    // Found the opening paren — look for the function name token before it.
                    let paren_offset = pos;
                    let func_token = tokens.iter().rev().find(|t| {
                        t.offset + t.length <= paren_offset
                            && dialect.classify_token(t.token_type, t.flags)
                                == TokenCategory::Function
                    })?;
                    // Make sure the function token is immediately before the paren
                    // (only whitespace between).
                    let between = &before[func_token.offset + func_token.length..paren_offset];
                    if between.trim().is_empty() {
                        let name = before[func_token.offset..func_token.offset + func_token.length]
                            .to_string();
                        return Some((name, commas));
                    }
                    return None;
                }
                depth -= 1;
            }
            b',' if depth == 0 => commas += 1,
            _ => {}
        }
    }
    None
}

// ── FormatError ───────────────────────────────────────────────────────────────

/// Errors that can occur during formatting.
#[derive(Debug)]
pub(crate) enum FormatError {
    /// The document URI was not found.
    UnknownDocument,
    /// The formatter returned an error.
    Format(crate::fmt::FormatError),
}

impl std::fmt::Display for FormatError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            FormatError::UnknownDocument => write!(f, "unknown document"),
            FormatError::Format(err) => write!(f, "format error: {err}"),
        }
    }
}

impl std::error::Error for FormatError {}

// ── Tests ─────────────────────────────────────────────────────────────────────

#[cfg(test)]
impl LspHost {
    /// Expected terminal token IDs (as `u32` ordinals) at a byte offset.
    pub(crate) fn expected_tokens_at_offset(&mut self, uri: &str, offset: usize) -> Vec<u32> {
        self.completion_info_at_offset(uri, offset)
            .tokens
            .iter()
            .map(|&t| u32::from(t))
            .collect()
    }
}

#[cfg(test)]
#[cfg(feature = "sqlite")]
mod tests {
    use syntaqlite_syntax::TokenType;

    use super::LspHost;
    use crate::lsp::CompletionKind;
    use crate::semantic::Catalog;
    use crate::semantic::ValidationConfig;
    use crate::semantic::catalog::{AritySpec, CatalogLayer, FunctionCategory};
    use crate::semantic::diagnostics::{DiagnosticMessage, Severity};

    #[test]
    fn completions_fall_back_to_last_good_state_on_parse_error() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "SELECT * FR";
        host.open_document(uri, 1, sql.to_string());
        let expected = host.expected_tokens_at_offset(uri, sql.len());
        assert!(
            expected.contains(&(TokenType::From as u32)),
            "expected From after SELECT *, got {expected:?}"
        );
    }

    #[test]
    fn completions_ignore_prior_statement_errors_after_semicolon() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "SELEC 1; SELECT * FR";
        host.open_document(uri, 1, sql.to_string());
        let expected = host.expected_tokens_at_offset(uri, sql.len());
        assert!(
            expected.contains(&(TokenType::From as u32)),
            "expected From in second statement context, got {expected:?}"
        );
    }

    #[test]
    fn completions_include_join_after_from_alias_with_partial_next_token() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "SELECT * FROM s AS x J";
        host.open_document(uri, 1, sql.to_string());
        let expected = host.expected_tokens_at_offset(uri, sql.len());
        assert!(
            expected.contains(&(TokenType::JoinKw as u32)),
            "expected JoinKw after FROM alias, got {expected:?}"
        );
    }

    #[test]
    fn completions_include_join_after_from_table_with_trailing_space() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "SELECT * FROM slice ";
        host.open_document(uri, 1, sql.to_string());
        let expected = host.expected_tokens_at_offset(uri, sql.len());
        assert!(
            expected.contains(&(TokenType::Join as u32)),
            "expected Join"
        );
        assert!(
            !expected.contains(&(TokenType::Create as u32)),
            "Create should not appear"
        );
        assert!(
            !expected.contains(&(TokenType::Select as u32)),
            "Select should not appear"
        );
        assert!(
            !expected.contains(&(TokenType::Virtual as u32)),
            "Virtual should not appear"
        );
    }

    #[test]
    fn available_functions_default_config_includes_baseline() {
        let host = LspHost::new();
        let names = host.available_function_names();
        assert!(names.iter().any(|n| n == "abs"));
        assert!(names.iter().any(|n| n == "count"));
    }

    #[test]
    fn available_functions_merges_user_context() {
        let mut host = LspHost::new();
        let dialect = crate::sqlite::dialect::dialect();
        let mut ctx = Catalog::new(dialect);
        ctx.layer_mut(CatalogLayer::Database)
            .insert_function_overload(
                "my_custom_func",
                FunctionCategory::Scalar,
                AritySpec::Exact(2),
            );
        host.set_session_context(ctx);
        let names = host.available_function_names();
        assert!(names.iter().any(|n| n == "my_custom_func"));
        assert!(names.iter().any(|n| n == "abs"));
    }

    #[test]
    fn completion_context_after_from_is_table_ref() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "SELECT acos() as foo FROM ";
        host.open_document(uri, 1, sql.to_string());
        let info = host.completion_info_at_offset(uri, sql.len());
        assert_eq!(info.context, super::super::CompletionContext::TableRef);
    }

    #[test]
    fn completion_context_after_select_is_not_table_ref() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "SELECT ";
        host.open_document(uri, 1, sql.to_string());
        let info = host.completion_info_at_offset(uri, sql.len());
        assert_ne!(info.context, super::super::CompletionContext::TableRef);
    }

    #[test]
    fn completion_context_after_where_is_expression() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "SELECT * FROM t WHERE ";
        host.open_document(uri, 1, sql.to_string());
        let info = host.completion_info_at_offset(uri, sql.len());
        assert_eq!(info.context, super::super::CompletionContext::Expression);
    }

    #[test]
    fn completions_include_join_after_from_table_no_trailing_space() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "SELECT * FROM slice";
        host.open_document(uri, 1, sql.to_string());
        let expected = host.expected_tokens_at_offset(uri, sql.len());
        assert!(expected.contains(&(TokenType::Join as u32)));
    }

    #[test]
    fn validate_select_after_create_table_as_select_no_diags() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(
            uri,
            1,
            "CREATE TABLE orders AS SELECT 1 AS order_id;\nSELECT o.order_id FROM orders o;"
                .to_string(),
        );
        let diags = host.validate(uri, &ValidationConfig::default());
        assert!(diags.is_empty(), "unexpected diagnostics: {diags:?}");
    }

    #[test]
    fn validate_select_from_unknown_table_still_warns() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(uri, 1, "SELECT * FROM nonexistent;".to_string());
        let diags = host.validate(uri, &ValidationConfig::default());
        assert!(!diags.is_empty());
    }

    #[test]
    fn validate_forward_reference_warns() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(
            uri,
            1,
            "SELECT * FROM t;\nCREATE TABLE t (id INTEGER);".to_string(),
        );
        let diags = host.validate(uri, &ValidationConfig::default());
        assert!(!diags.is_empty());
    }

    #[test]
    fn syntax_error_produces_diagnostic_for_bare_select() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(uri, 1, "SELECT ".to_string());
        let diags = host.diagnostics(uri);
        assert!(!diags.is_empty());
        assert_eq!(diags[0].severity, Severity::Error);
    }

    #[test]
    fn syntax_error_produces_diagnostic_for_incomplete_from() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(uri, 1, "SELECT * FROM".to_string());
        let diags = host.diagnostics(uri);
        assert!(!diags.is_empty());
    }

    #[test]
    fn validation_returns_error_for_syntax_invalid_sql() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(uri, 1, "NOT VALID SQL;".to_string());
        let diags = host.diagnostics(uri);
        assert!(!diags.is_empty());
    }

    #[test]
    fn multiple_syntax_errors_all_reported() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(uri, 1, "include ;\ninclude ;\nSELECT 1;".to_string());
        let diags = host.diagnostics(uri);
        let errors: Vec<_> = diags
            .iter()
            .filter(|d| d.severity == Severity::Error)
            .collect();
        assert_eq!(errors.len(), 2, "got {}: {:?}", errors.len(), errors);
    }

    #[test]
    fn syntax_errors_do_not_suppress_later_valid_statements() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(uri, 1, "NOT VALID;\nSELECT 1;".to_string());
        let diags = host.diagnostics(uri);
        assert_eq!(diags.len(), 1, "got {}: {:?}", diags.len(), diags);
    }

    #[test]
    fn syntax_error_after_valid_statement_is_reported() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(uri, 1, "SELECT 1;\nNOT VALID;".to_string());
        let diags = host.diagnostics(uri);
        assert_eq!(diags.len(), 1, "got {}: {:?}", diags.len(), diags);
    }

    #[test]
    fn validate_does_not_duplicate_parse_error_diagnostics() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(uri, 1, "SELECT ;\nSELECT 1;".to_string());
        let diags = host.validate(uri, &ValidationConfig::default());
        assert_eq!(diags.len(), 0, "got: {diags:?}");
    }

    #[test]
    fn validate_continues_past_errors_to_check_later_statements() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(
            uri,
            1,
            "SELECT ;\nSELECT ;\nSELECT * FROM no_such_table;".to_string(),
        );
        let diags = host.validate(uri, &ValidationConfig::default());
        let table_diags: Vec<_> = diags
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownTable { .. }))
            .collect();
        assert_eq!(table_diags.len(), 1, "got: {diags:?}");
    }

    #[test]
    fn syntax_error_offset_points_at_error_token() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "select 1 from slice where foo = where x = y;";
        host.open_document(uri, 1, sql.to_string());
        let diags = host.diagnostics(uri);
        assert!(!diags.is_empty());
        let diag = &diags[0];
        assert_eq!(diag.severity, Severity::Error);
        let second_where = sql[31..].find("where").map(|i| i + 31).unwrap();
        assert_eq!(
            diag.start_offset,
            second_where,
            "got '{}' at {}",
            &sql[diag.start_offset..=diag.start_offset],
            diag.start_offset
        );
    }

    #[test]
    fn parse_and_validate_combined_no_duplicates() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(uri, 1, "SELECT ;\nSELECT * FROM no_such_table;".to_string());
        let parse_diags = host.diagnostics(uri).to_vec();
        let val_diags = host.validate(uri, &ValidationConfig::default());
        let all: Vec<_> = parse_diags.iter().chain(val_diags.iter()).collect();
        let errors = all.iter().filter(|d| d.severity == Severity::Error).count();
        let warnings = all
            .iter()
            .filter(|d| d.severity == Severity::Warning)
            .count();
        assert_eq!(errors, 1, "got {errors}: {all:?}");
        assert_eq!(warnings, 1, "got {warnings}: {all:?}");
    }

    #[test]
    fn set_session_context_from_ddl_returns_error_for_invalid_ddl() {
        let mut host = LspHost::new();
        let errors = host
            .set_session_context_from_ddl("create table orders as;", None)
            .expect_err("expected parse errors for invalid DDL, got Ok");
        assert!(!errors.is_empty(), "expected at least one error message");
        assert!(
            errors.iter().any(|e| !e.is_empty()),
            "expected non-empty error messages, got: {errors:?}"
        );
    }

    #[test]
    fn set_session_context_from_ddl_returns_ok_for_valid_ddl() {
        let mut host = LspHost::new();
        let result = host
            .set_session_context_from_ddl("CREATE TABLE orders (id INTEGER, total REAL);", None);
        assert!(result.is_ok(), "expected Ok for valid DDL, got: {result:?}");
    }

    #[test]
    fn definition_info_returns_cross_file_uri_for_schema_table() {
        let schema = "CREATE TABLE orders (id INTEGER, total REAL);";
        let file_uri = "file:///path/to/schema.sql";
        let mut host = LspHost::new();
        host.set_session_context_from_ddl(schema, Some(file_uri))
            .unwrap();

        let uri = "file:///query.sql";
        host.open_document(uri, 1, "SELECT * FROM orders".to_string());

        let ref_offset = "SELECT * FROM ".len();
        let result = host.definition_info(uri, ref_offset);
        assert!(result.is_some(), "expected definition for schema table");
        let def = result.unwrap();
        assert_eq!(def.target.file_uri.as_deref(), Some(file_uri));
        let schema_offset = schema.find("orders").unwrap();
        assert_eq!(def.target.start, schema_offset);
        assert_eq!(def.target.end, schema_offset + "orders".len());
    }

    #[test]
    fn definition_info_returns_cross_file_uri_for_schema_column() {
        let schema = "CREATE TABLE orders (id INTEGER, total REAL);";
        let file_uri = "file:///path/to/schema.sql";
        let mut host = LspHost::new();
        host.set_session_context_from_ddl(schema, Some(file_uri))
            .unwrap();

        let uri = "file:///query.sql";
        host.open_document(uri, 1, "SELECT total FROM orders".to_string());

        let ref_offset = "SELECT ".len(); // points to "total"
        let result = host.definition_info(uri, ref_offset);
        assert!(result.is_some(), "expected definition for schema column");
        let def = result.unwrap();
        assert_eq!(def.target.file_uri.as_deref(), Some(file_uri));
        let schema_offset = schema.find("total").unwrap();
        assert_eq!(def.target.start, schema_offset);
        assert_eq!(def.target.end, schema_offset + "total".len());
    }

    #[test]
    fn syntax_error_for_create_table_as_missing_select() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(uri, 1, "create table orders as;".to_string());
        let diags = host.all_diagnostics(uri, &ValidationConfig::default());
        assert!(
            !diags.is_empty(),
            "expected syntax error for 'create table orders as;', got none"
        );
        assert!(
            diags.iter().any(|d| d.severity == Severity::Error),
            "expected an error-severity diagnostic, got: {diags:?}"
        );
    }

    // ── Find-references tests ──────────────────────────────────────────────

    #[test]
    fn find_references_table_in_single_file() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE users (id INT);\nSELECT * FROM users;\nDELETE FROM users;";
        host.open_document(uri, 1, sql.to_string());

        // Click on "users" in the SELECT statement.
        let offset = sql.find("SELECT").unwrap() + "SELECT * FROM ".len();
        let refs = host.find_references(uri, offset, false);
        // Should find the two DML references (SELECT + DELETE), not the CREATE.
        assert_eq!(refs.len(), 2, "expected 2 refs, got: {refs:?}");
    }

    #[test]
    fn find_references_table_include_declaration() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE users (id INT);\nSELECT * FROM users;\nDELETE FROM users;";
        host.open_document(uri, 1, sql.to_string());

        let offset = sql.find("SELECT").unwrap() + "SELECT * FROM ".len();
        let refs = host.find_references(uri, offset, true);
        // Should find the two DML references + the CREATE TABLE definition.
        assert_eq!(refs.len(), 3, "expected 3 refs (incl decl), got: {refs:?}");
    }

    #[test]
    fn find_references_column_in_single_file() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE t (id INT, name TEXT);\nSELECT id FROM t;\nSELECT id, name FROM t;";
        host.open_document(uri, 1, sql.to_string());

        // Click on "id" in the first SELECT.
        let offset = sql.find("SELECT id").unwrap() + "SELECT ".len();
        let refs = host.find_references(uri, offset, false);
        assert_eq!(refs.len(), 2, "expected 2 column refs, got: {refs:?}");
    }

    #[test]
    fn find_references_cross_file() {
        let schema = "CREATE TABLE orders (id INTEGER, total REAL);";
        let file_uri = "file:///schema.sql";
        let mut host = LspHost::new();
        host.set_session_context_from_ddl(schema, Some(file_uri))
            .unwrap();

        let uri1 = "file:///a.sql";
        let uri2 = "file:///b.sql";
        host.open_document(uri1, 1, "SELECT * FROM orders;".to_string());
        host.open_document(uri2, 1, "DELETE FROM orders;".to_string());

        // Click on "orders" in a.sql.
        let offset = "SELECT * FROM ".len();
        let refs = host.find_references(uri1, offset, false);
        assert_eq!(refs.len(), 2, "expected refs in both files, got: {refs:?}");
        let ref_uris: Vec<&str> = refs.iter().map(|r| r.0.as_str()).collect();
        assert!(ref_uris.contains(&uri1));
        assert!(ref_uris.contains(&uri2));
    }

    #[test]
    fn find_references_cursor_on_definition() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE users (id INT);\nSELECT * FROM users;";
        host.open_document(uri, 1, sql.to_string());

        // Click on "users" in CREATE TABLE — should still find the SELECT reference.
        let offset = sql.find("users").unwrap();
        let refs = host.find_references(uri, offset, false);
        assert_eq!(
            refs.len(),
            1,
            "expected 1 ref from definition site, got: {refs:?}"
        );
    }

    #[test]
    fn find_references_cursor_on_definition_include_declaration() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE users (id INT);\nSELECT * FROM users;";
        host.open_document(uri, 1, sql.to_string());

        let offset = sql.find("users").unwrap();
        let refs = host.find_references(uri, offset, true);
        assert_eq!(refs.len(), 2, "expected 2 refs (incl decl), got: {refs:?}");
    }

    // ── Rename tests ────────────────────────────────────────────────────────

    #[test]
    fn prepare_rename_returns_range_for_table() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE users (id INT);\nSELECT * FROM users;";
        host.open_document(uri, 1, sql.to_string());

        let offset = sql.find("SELECT").unwrap() + "SELECT * FROM ".len();
        let result = host.prepare_rename(uri, offset);
        assert!(result.is_some(), "expected rename range");
        let (start, end, text) = result.unwrap();
        assert_eq!(text, "users");
        assert_eq!(&sql[start..end], "users");
    }

    #[test]
    fn rename_table_in_single_file() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE users (id INT);\nSELECT * FROM users;\nDELETE FROM users;";
        host.open_document(uri, 1, sql.to_string());

        let offset = sql.find("SELECT").unwrap() + "SELECT * FROM ".len();
        let edits = host.rename(uri, offset, "accounts");
        // Should produce edits for all 3 occurrences (definition + 2 refs).
        let file_edits = edits.get(uri).expect("expected edits for test file");
        assert_eq!(file_edits.len(), 3, "expected 3 edits, got: {file_edits:?}");
        for (_, _, text) in file_edits {
            assert_eq!(text.as_str(), "accounts");
        }
    }

    #[test]
    fn rename_column_in_single_file() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE t (id INT, name TEXT);\nSELECT id FROM t;\nSELECT id, name FROM t;";
        host.open_document(uri, 1, sql.to_string());

        let offset = sql.find("SELECT id").unwrap() + "SELECT ".len();
        let edits = host.rename(uri, offset, "user_id");
        let file_edits = edits.get(uri).expect("expected edits for test file");
        // 2 column refs + 1 definition = 3 edits.
        assert_eq!(file_edits.len(), 3, "expected 3 edits, got: {file_edits:?}");
    }

    #[test]
    fn rename_cross_file() {
        let schema = "CREATE TABLE orders (id INTEGER, total REAL);";
        let schema_uri = "file:///schema.sql";
        let mut host = LspHost::new();
        host.set_session_context_from_ddl(schema, Some(schema_uri))
            .unwrap();

        let uri1 = "file:///a.sql";
        let uri2 = "file:///b.sql";
        host.open_document(uri1, 1, "SELECT * FROM orders;".to_string());
        host.open_document(uri2, 1, "DELETE FROM orders;".to_string());

        let offset = "SELECT * FROM ".len();
        let edits = host.rename(uri1, offset, "invoices");
        // Should have edits in both open files.
        assert!(edits.contains_key(uri1), "expected edits in a.sql");
        assert!(edits.contains_key(uri2), "expected edits in b.sql");
    }

    #[test]
    fn completion_on_suggested_after_join_target() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "SELECT * FROM slice JOIN thread ";
        host.open_document(uri, 1, sql.to_string());
        let items = host.completion_items(uri, sql.len());
        let labels: Vec<&str> = items.iter().map(|e| e.label.as_str()).collect();
        assert!(
            labels.contains(&"ON"),
            "ON should be suggested, got: {labels:?}"
        );
    }

    #[test]
    fn completion_qualifier_detected_after_dot() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE t1 (a INT, b TEXT);\nCREATE TABLE t2 (c INT);\nSELECT t1.";
        host.open_document(uri, 1, sql.to_string());
        let info = host.completion_info_at_offset(uri, sql.len());
        assert_eq!(
            info.qualifier.as_deref(),
            Some("t1"),
            "should detect t1 as qualifier, got: {:?}",
            info.qualifier
        );
    }

    #[test]
    fn completion_qualified_column_only_from_table() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE t1 (a INT, b TEXT);\nCREATE TABLE t2 (c INT);\nSELECT t1.";
        host.open_document(uri, 1, sql.to_string());
        let items = host.completion_items(uri, sql.len());
        let labels: Vec<&str> = items.iter().map(|e| e.label.as_str()).collect();
        assert!(labels.contains(&"a"), "should suggest column a");
        assert!(labels.contains(&"b"), "should suggest column b");
        assert!(
            !labels.contains(&"c"),
            "should NOT suggest column c from t2"
        );
        assert!(
            items.iter().all(|e| e.kind == CompletionKind::Column),
            "all items should be columns, got: {labels:?}"
        );
    }

    #[test]
    fn completion_tables_after_from() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE users (id INT);\nSELECT * FROM ";
        host.open_document(uri, 1, sql.to_string());
        let items = host.completion_items(uri, sql.len());
        let labels: Vec<&str> = items.iter().map(|e| e.label.as_str()).collect();
        assert!(
            labels.contains(&"users"),
            "should suggest table users, got: {labels:?}"
        );
    }

    #[test]
    fn completion_columns_after_select() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE users (id INT, name TEXT);\nSELECT ";
        host.open_document(uri, 1, sql.to_string());
        let items = host.completion_items(uri, sql.len());
        let labels: Vec<&str> = items.iter().map(|e| e.label.as_str()).collect();
        assert!(labels.contains(&"id"), "should suggest column id");
        assert!(labels.contains(&"name"), "should suggest column name");
        assert!(labels.contains(&"abs"), "should suggest function abs");
    }

    #[test]
    fn completion_columns_sorted_before_functions_in_expression() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE slice (id INT, name TEXT);\n\
                   CREATE TABLE thread (tid INT, parent INT);\n\
                   SELECT slice.id, thread.tid\nFROM slice\nJOIN thread ON ";
        host.open_document(uri, 1, sql.to_string());
        let items = host.completion_items(uri, sql.len());

        // Find the first column and first function in the list.
        let first_column_pos = items
            .iter()
            .position(|e| e.kind() == CompletionKind::Column);
        let first_function_pos = items
            .iter()
            .position(|e| e.kind() == CompletionKind::Function);

        assert!(first_column_pos.is_some(), "should have column completions");
        assert!(
            first_function_pos.is_some(),
            "should have function completions"
        );
        assert!(
            first_column_pos.unwrap() < first_function_pos.unwrap(),
            "columns should appear before functions after ON, \
             first column at {}, first function at {}",
            first_column_pos.unwrap(),
            first_function_pos.unwrap(),
        );
    }

    // ── SchemaMap tests ──────────────────────────────────────────────────

    mod schema_map_tests {
        use std::path::PathBuf;

        use super::super::SchemaMap;
        use crate::semantic::Catalog;

        fn empty_catalog() -> Catalog {
            Catalog::new(crate::sqlite::dialect::any_dialect())
        }

        #[test]
        fn resolve_matches_glob_pattern() {
            let config_dir = PathBuf::from("/project");
            let pattern = glob::Pattern::new("queries/**/*.sql").unwrap();
            let map = SchemaMap::new(config_dir, None, vec![(pattern, empty_catalog())]);

            assert!(
                map.resolve("file:///project/queries/foo/bar.sql").is_some(),
                "should match queries/**/*.sql"
            );
            assert!(
                map.resolve("file:///project/queries/top.sql").is_some(),
                "should match queries/*.sql via **"
            );
            assert!(
                map.resolve("file:///project/other/bar.sql").is_none(),
                "should not match files outside queries/"
            );
        }

        #[test]
        fn resolve_returns_none_for_no_match_no_default() {
            let config_dir = PathBuf::from("/project");
            let pattern = glob::Pattern::new("src/*.sql").unwrap();
            let map = SchemaMap::new(config_dir, None, vec![(pattern, empty_catalog())]);

            assert!(map.resolve("file:///project/other/q.sql").is_none());
        }

        #[test]
        fn resolve_falls_back_to_default() {
            let config_dir = PathBuf::from("/project");
            let map = SchemaMap::new(config_dir, Some(empty_catalog()), vec![]);

            let result = map.resolve("file:///project/any/file.sql");
            assert!(result.is_some(), "should fall back to default catalog");
        }

        #[test]
        fn resolve_prefers_glob_over_default() {
            let config_dir = PathBuf::from("/project");
            let pattern = glob::Pattern::new("special/*.sql").unwrap();

            let map = SchemaMap::new(
                config_dir,
                Some(empty_catalog()),
                vec![(pattern, empty_catalog())],
            );

            // Matched file should get the glob catalog (from entries[0]).
            let (resolved, _) = map.resolve("file:///project/special/q.sql").unwrap();
            let glob_ptr = std::ptr::from_ref::<Catalog>(resolved);
            // The glob entry catalog lives in map.entries, not map.default.
            let expected_glob_ptr = &raw const map.entries[0].1;
            assert_eq!(
                glob_ptr, expected_glob_ptr,
                "matched file should get glob catalog"
            );

            // Unmatched file should get the default catalog.
            let (resolved, _) = map.resolve("file:///project/other/q.sql").unwrap();
            let default_ptr = std::ptr::from_ref::<Catalog>(resolved);
            let expected_default_ptr = std::ptr::from_ref::<Catalog>(map.default.as_ref().unwrap());
            assert_eq!(
                default_ptr, expected_default_ptr,
                "unmatched file should get default catalog"
            );
        }

        #[test]
        fn resolve_requires_file_uri_prefix() {
            let config_dir = PathBuf::from("/project");
            let map = SchemaMap::new(config_dir, Some(empty_catalog()), vec![]);
            assert!(map.resolve("untitled:Untitled-1").is_none());
        }

        #[test]
        fn resolve_requires_path_under_config_dir() {
            let config_dir = PathBuf::from("/project");
            let map = SchemaMap::new(config_dir, Some(empty_catalog()), vec![]);
            assert!(
                map.resolve("file:///other-project/foo.sql").is_none(),
                "files outside config_dir should not match"
            );
        }

        #[test]
        fn resolve_strict_flag() {
            let config_dir = PathBuf::from("/project");
            let pattern = glob::Pattern::new("*.sql").unwrap();
            let map = SchemaMap::new(config_dir, None, vec![(pattern, empty_catalog())]);

            let (_cat, strict) = map.resolve("file:///project/q.sql").unwrap();
            assert!(strict, "matched files should have strict=true");
        }
    }

    // ── LspHost + SchemaMap integration test ─────────────────────────────

    #[test]
    fn schema_map_selects_correct_catalog_per_document() {
        use crate::semantic::diagnostics::Severity;
        use std::path::PathBuf;

        let mut host = LspHost::new();

        // Build a catalog that knows about "users" table.
        let dialect = crate::sqlite::dialect::any_dialect();
        let (users_catalog, _) = Catalog::from_ddl(
            dialect.clone(),
            &[("CREATE TABLE users (id INTEGER, name TEXT);", None)],
        );

        // Build a SchemaMap where "matched/*.sql" gets the users catalog,
        // unmatched files get nothing.
        let pattern = glob::Pattern::new("matched/*.sql").unwrap();
        let map = super::SchemaMap::new(
            PathBuf::from("/workspace"),
            None,
            vec![(pattern, users_catalog)],
        );
        host.set_schema_map(map);

        // Open a matched file referencing `users` — should get no "unknown table" error.
        let matched_uri = "file:///workspace/matched/query.sql";
        host.open_document(matched_uri, 1, "SELECT id FROM users;".to_string());
        let diags = host.document_all_diagnostics(matched_uri);
        let (_, _, diags) = diags.unwrap();
        let schema_errors: Vec<_> = diags
            .iter()
            .filter(|d| d.severity() == Severity::Error)
            .collect();
        assert!(
            schema_errors.is_empty(),
            "matched file should have no errors with correct schema, got: {schema_errors:?}"
        );

        // Open an unmatched file referencing `users` — should get a warning
        // (not an error, because no catalog = lenient mode).
        let unmatched_uri = "file:///workspace/other/query.sql";
        host.open_document(unmatched_uri, 1, "SELECT id FROM users;".to_string());
        let diags = host.document_all_diagnostics(unmatched_uri);
        let (_, _, diags) = diags.unwrap();
        let errors: Vec<_> = diags
            .iter()
            .filter(|d| d.severity() == Severity::Error)
            .collect();
        assert!(
            errors.is_empty(),
            "unmatched file should have no errors (lenient mode), got: {errors:?}"
        );
    }

    #[test]
    fn goto_definition_cte_reference() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let src = "WITH cte AS (SELECT 1) SELECT * FROM cte";
        host.open_document(uri, 1, src.to_string());

        let ref_offset = src.rfind("cte").unwrap();
        let def = host.definition_info(uri, ref_offset).expect("definition");
        let cte_def_offset = src.find("cte").unwrap();
        assert_eq!(def.target.start, cte_def_offset);
        assert_eq!(def.target.end, cte_def_offset + "cte".len());
    }

    #[test]
    fn goto_definition_ddl_table_reference() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let src = "CREATE TABLE users (id INTEGER); SELECT id FROM users;";
        host.open_document(uri, 1, src.to_string());

        let ref_offset = src.rfind("users").unwrap();
        let def = host.definition_info(uri, ref_offset).expect("definition");
        let ddl_offset = src.find("users").unwrap();
        assert_eq!(def.target.start, ddl_offset);
        assert_eq!(def.target.end, ddl_offset + "users".len());
    }

    #[test]
    fn goto_definition_cte_shadows_ddl() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let src = "CREATE TABLE t (id INTEGER); WITH t AS (SELECT 1 AS id) SELECT * FROM t;";
        host.open_document(uri, 1, src.to_string());

        let from_t_offset = src.rfind("FROM t").unwrap() + 5;
        let def = host
            .definition_info(uri, from_t_offset)
            .expect("definition");
        let cte_t_offset = src[29..].find('t').unwrap() + 29;
        assert_eq!(def.target.start, cte_t_offset);
    }

    #[test]
    fn goto_definition_unknown_table_returns_none() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let src = "SELECT * FROM nonexistent";
        host.open_document(uri, 1, src.to_string());

        let from_offset = src.find("nonexistent").unwrap();
        assert!(host.definition_info(uri, from_offset).is_none());
    }

    #[test]
    fn goto_definition_column_in_ddl_table() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let src = "CREATE TABLE users (id INTEGER, name TEXT);\nSELECT name FROM users;";
        host.open_document(uri, 1, src.to_string());

        let select_name_offset = src.rfind("name").unwrap();
        let def = host
            .definition_info(uri, select_name_offset)
            .expect("definition");
        let ddl_name_offset = src.find("name").unwrap();
        assert_eq!(def.target.start, ddl_name_offset);
    }

    #[test]
    fn goto_definition_unknown_column_returns_none() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let src = "CREATE TABLE t (a INT);\nSELECT b FROM t;";
        host.open_document(uri, 1, src.to_string());

        let b_offset = src.find('b').unwrap();
        assert!(host.definition_info(uri, b_offset).is_none());
    }

    #[test]
    fn goto_definition_cte_column_inferred_from_alias() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let src = "WITH foo AS (SELECT 1 AS a)\nSELECT a FROM foo;";
        host.open_document(uri, 1, src.to_string());

        let a_offset = src.find("SELECT a").unwrap() + "SELECT ".len();
        let def = host.definition_info(uri, a_offset).expect("definition");
        let cte_a_offset = src.find("AS a").unwrap() + "AS ".len();
        assert_eq!(def.target.start, cte_a_offset);
    }

    #[test]
    fn goto_definition_cte_column_from_declared_list() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let src = "WITH foo(x) AS (SELECT 1)\nSELECT x FROM foo;";
        host.open_document(uri, 1, src.to_string());

        let x_offset = src.find("SELECT x").unwrap() + "SELECT ".len();
        let def = host.definition_info(uri, x_offset).expect("definition");
        let decl_x_offset = src.find("(x)").unwrap() + 1;
        assert_eq!(def.target.start, decl_x_offset);
    }

    #[test]
    fn goto_definition_schema_table_jumps_to_external_file() {
        use crate::semantic::catalog::Catalog;
        let schema = "CREATE TABLE users (id INTEGER, name TEXT);";
        let file_uri = "file:///path/to/schema.sql";
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(dialect, &[(schema, Some(file_uri))]).0;

        let mut host = LspHost::new();
        host.set_session_context(cat);

        let uri = "file:///test.sql";
        let src = "SELECT * FROM users";
        host.open_document(uri, 1, src.to_string());

        let ref_offset = src.find("users").unwrap();
        let def = host.definition_info(uri, ref_offset).expect("definition");
        assert_eq!(def.target.file_uri.as_deref(), Some(file_uri));
        let schema_offset = schema.find("users").unwrap();
        assert_eq!(def.target.start, schema_offset);
        assert_eq!(def.target.end, schema_offset + "users".len());
    }

    #[test]
    fn goto_definition_same_file_ddl_shadows_schema() {
        use crate::semantic::catalog::Catalog;
        let schema = "CREATE TABLE t (x INTEGER);";
        let file_uri = "file:///schema.sql";
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(dialect, &[(schema, Some(file_uri))]).0;

        let mut host = LspHost::new();
        host.set_session_context(cat);

        let uri = "file:///test.sql";
        let src = "CREATE TABLE t (y INTEGER); SELECT * FROM t;";
        host.open_document(uri, 1, src.to_string());

        let ref_offset = src.rfind(" t").unwrap() + 1;
        let def = host.definition_info(uri, ref_offset).expect("definition");
        assert!(def.target.file_uri.is_none());
        assert_eq!(def.target.start, src.find('t').unwrap());
    }
}
