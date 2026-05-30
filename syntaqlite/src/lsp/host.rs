// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::collections::HashMap;
use std::path::{Path, PathBuf};

use syntaqlite_syntax::source::{DocOffset, DocRange};

use crate::analysis::AnalysisConfig;
use crate::analysis::AnalysisContext;
use crate::analysis::Catalog;
use crate::analysis::completion::{self, SignatureHelpInfo};
use crate::analysis::diagnostics::Diagnostic;
use crate::analysis::engine::Analyzer;
use crate::dialect::AnyDialect;
use crate::fmt::FormatConfig;
use crate::fmt::formatter::Formatter;

use super::analysis_data::{DefinitionResult, ExternalDefinitions, LspCapture};
use super::document_store::{Document, DocumentStore};
use super::semantic_tokens_codec::encode_semantic_tokens;
use super::{CompletionEntry, CompletionInfo};

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

    /// Mutable variant of [`resolve`](Self::resolve). Used by the analysis
    /// path, which needs `&mut Catalog` to accumulate per-document DDL.
    pub(crate) fn resolve_mut(&mut self, file_uri: &str) -> Option<(&mut Catalog, bool)> {
        let path = file_uri.strip_prefix("file://")?;
        let path = Path::new(path);
        let relative = path.strip_prefix(&self.config_dir).ok()?;
        let relative_str = relative.to_string_lossy();
        let match_opts = glob::MatchOptions {
            require_literal_separator: true,
            ..Default::default()
        };
        for (pattern, catalog) in &mut self.entries {
            if pattern.matches_with(&relative_str, match_opts) {
                return Some((catalog, true));
            }
        }
        self.default.as_mut().map(|cat| (cat, true))
    }
}

// ── Analysis dispatch ────────────────────────────────────────────────────────

/// Run analysis for `doc` if nothing is cached yet.
fn ensure_analysis(
    doc: &mut Document,
    analyzer: &mut Analyzer,
    user_catalog: &mut Catalog,
    analysis_config: &AnalysisConfig,
    external_defs: Option<&ExternalDefinitions>,
) {
    if doc.analysis.is_some() {
        return;
    }

    // sqlite3 shell scripts (`.read foo.sql`, column-0 `#` comments, `GO`/`/`
    // terminators) are a separate language layered above pure SQL. Auto-detect
    // them and route the SQL fragments through the embedded analyzer so the
    // shell-only lines are not flagged as SQL syntax errors (issue #88).
    #[cfg(feature = "experimental-embedded")]
    if crate::embedded::is_shell_script(&doc.source) {
        analyze_shell_doc(doc, analyzer, user_catalog, analysis_config, external_defs);
        return;
    }

    let mut capture = LspCapture::new(external_defs);
    let mut ctx = AnalysisContext::new(user_catalog).with_config(*analysis_config);
    let model = analyzer.analyze_with_visitor(&doc.source, &mut ctx, &mut capture);
    let all_diags: Vec<Diagnostic> = model.diagnostics().cloned().collect();
    let parse_diags: Vec<Diagnostic> = all_diags
        .iter()
        .filter(|d| d.message().is_parse_error())
        .cloned()
        .collect();
    doc.cached_parse_diags = Some(parse_diags);
    doc.cached_all_diags = Some(all_diags);
    doc.analysis = Some(capture.into_data());
}

/// Analyze a document detected as a sqlite3 shell script.
///
/// Extracts the SQL fragments that sit between shell lines (dot-commands,
/// column-0 `#` comments, `GO`/`/` terminators) and runs them through the
/// embedded analyzer, mapping diagnostics back to host-file offsets. The
/// shell-only lines therefore never reach the SQL parser and are not flagged as
/// syntax errors.
///
/// Semantic tokens, hover, and go-to-definition for shell files are out of scope
/// for now: an empty [`LspCapture`] keeps the cache invariants satisfied while
/// those queries return nothing.
// TODO: wire shell fragments into semantic tokens / hover so `.read` scripts get
// SQL highlighting and navigation in-editor.
#[cfg(feature = "experimental-embedded")]
fn analyze_shell_doc(
    doc: &mut Document,
    analyzer: &Analyzer,
    user_catalog: &Catalog,
    analysis_config: &AnalysisConfig,
    external_defs: Option<&ExternalDefinitions>,
) {
    use crate::embedded::{EmbeddedAnalyzer, extract_shell};

    let fragments = extract_shell(&doc.source);
    let mut emb = EmbeddedAnalyzer::new(analyzer.dialect())
        .with_catalog(user_catalog.clone())
        .with_config(*analysis_config);
    let all_diags = emb.analyze(&fragments);
    let parse_diags: Vec<Diagnostic> = all_diags
        .iter()
        .filter(|d| d.message().is_parse_error())
        .cloned()
        .collect();
    doc.cached_parse_diags = Some(parse_diags);
    doc.cached_all_diags = Some(all_diags);
    doc.analysis = Some(LspCapture::new(external_defs).into_data());
}

/// Resolve the correct catalog for `uri` via `schema_map`, then call
/// [`ensure_analysis`]. Returns `false` if the document is not found.
pub(super) fn ensure_analysis_for(
    uri: &str,
    documents: &mut DocumentStore,
    analyzer: &mut Analyzer,
    schema_map: Option<&mut SchemaMap>,
    user_catalog: &mut Catalog,
    base_analysis_config: &AnalysisConfig,
    external_defs: Option<&ExternalDefinitions>,
) -> bool {
    let Some(doc) = documents.get_mut(uri) else {
        return false;
    };
    let (catalog, config) = if let Some(map) = schema_map {
        if let Some((cat, _strict)) = map.resolve_mut(uri) {
            (cat, base_analysis_config.with_strict_schema())
        } else {
            (user_catalog, *base_analysis_config)
        }
    } else {
        (user_catalog, *base_analysis_config)
    };
    ensure_analysis(doc, analyzer, catalog, &config, external_defs);
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
///    enable table/column/function analysis.
///
/// Analysis is cached per-document and invalidated automatically when the
/// source text or catalog context changes. Analysis delegates to
/// [`Analyzer`].
///
/// Use this when you are building an LSP server, a web-based editor plugin,
/// or any tool that needs incremental SQL analysis tied to document identity.
/// For one-shot analysis without document management, use
/// [`Analyzer`] directly.
///
/// # Example
///
/// ```
/// use syntaqlite::lsp::LspHost;
/// use syntaqlite::source::DocOffset;
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
/// let items = host.completion_items("file:///query.sql", DocOffset::from_raw(9));
/// ```
pub struct LspHost {
    dialect: AnyDialect,
    /// User-provided schema (tables, views, functions).
    user_catalog: Catalog,
    /// External definition sites (populated from schema DDL that carries a
    /// `file://` URI). Consulted by [`LspObserver`] to build cross-file
    /// go-to-definition targets.
    external_defs: ExternalDefinitions,
    analyzer: Analyzer,
    documents: DocumentStore,
    /// Format config from project config file. `None` means use defaults.
    format_config: Option<FormatConfig>,
    /// Analysis config (`strict_schema` is set when a schema is provided).
    analysis_config: AnalysisConfig,
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
            external_defs: ExternalDefinitions::new(),
            analyzer: Analyzer::new(),
            dialect,
            documents: DocumentStore::new(),
            format_config: None,
            analysis_config: AnalysisConfig::default(),
            schema_map: None,
        }
    }

    /// Create a host bound to `dialect`.
    pub fn with_dialect(dialect: impl Into<AnyDialect>) -> Self {
        let dialect = dialect.into();
        LspHost {
            user_catalog: Catalog::new(dialect.clone()),
            external_defs: ExternalDefinitions::new(),
            analyzer: Analyzer::with_dialect(dialect.clone()),
            dialect,
            documents: DocumentStore::new(),
            format_config: None,
            analysis_config: AnalysisConfig::default(),
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

    /// Set the analysis config.
    pub fn set_analysis_config(&mut self, config: AnalysisConfig) {
        self.analysis_config = config;
        self.documents.invalidate_all();
    }

    /// Set the per-file schema map from `[schemas]` config globs.
    /// When set, each document is resolved against the map to find its
    /// matching catalog and `strict_schema` is set automatically.
    pub fn set_schema_map(&mut self, map: SchemaMap) {
        self.schema_map = Some(map);
        self.documents.invalidate_all();
    }

    /// Set the session context (user-provided schema and functions).
    /// Invalidates all cached analysis.
    pub fn set_session_context(&mut self, ctx: Catalog) {
        self.user_catalog = ctx;
        self.documents.invalidate_all();
    }

    // ── Document lifecycle ─────────────────────────────────────────────────────

    /// Register a newly opened document.
    pub(crate) fn open_document(&mut self, uri: &str, version: i32, text: String) {
        self.documents.open(uri, version, text);
    }

    /// Update a document's content, invalidating cached analysis.
    pub fn update_document(&mut self, uri: &str, version: i32, text: String) {
        self.documents.update(uri, version, text);
    }

    /// Remove a document from the host.
    pub(crate) fn close_document(&mut self, uri: &str) {
        self.documents.close(uri);
    }

    /// Source text for a document.
    pub(crate) fn document_source(&self, uri: &str) -> Option<&str> {
        self.documents.source(uri)
    }

    // ── Analysis queries ───────────────────────────────────────────────────────

    /// Parse-error diagnostics for a document, lazily computed.
    pub(crate) fn diagnostics(&mut self, uri: &str) -> &[Diagnostic] {
        if !ensure_analysis_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_mut(),
            &mut self.user_catalog,
            &self.analysis_config,
            Some(&self.external_defs),
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
    pub fn semantic_tokens_encoded(&mut self, uri: &str, range: Option<DocRange>) -> Vec<u32> {
        if !ensure_analysis_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_mut(),
            &mut self.user_catalog,
            &self.analysis_config,
            Some(&self.external_defs),
        ) {
            return Vec::new();
        }
        let doc = self
            .documents
            .get_mut(uri)
            .expect("ensure_model_for guarantees document exists");
        if doc.cached_sem_tokens.is_none() {
            let tokens = doc
                .analysis
                .as_ref()
                .expect("ensure_analysis sets analysis")
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
    pub(crate) fn completion_info_at_offset(
        &mut self,
        uri: &str,
        offset: DocOffset,
    ) -> CompletionInfo {
        if !ensure_analysis_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_mut(),
            &mut self.user_catalog,
            &self.analysis_config,
            Some(&self.external_defs),
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
        let analysis = doc
            .analysis
            .as_ref()
            .expect("ensure_analysis sets analysis");
        completion::completion_info(
            &self.analyzer.dialect(),
            &doc.source,
            &analysis.tokens,
            offset,
        )
    }

    /// Completion items (keywords + functions) at a byte offset.
    pub fn completion_items(&mut self, uri: &str, offset: DocOffset) -> Vec<CompletionEntry> {
        let info = self.completion_info_at_offset(uri, offset);
        super::completion_service::build_completion_items(&info, &self.dialect, &self.user_catalog)
    }

    // ── Analysis ────────────────────────────────────────────────────

    /// Version, source text, and all diagnostics (parse + semantic) in one call.
    ///
    /// Reads from the cached model populated by [`ensure_model`] — no re-analysis.
    #[cfg(feature = "lsp")]
    pub(crate) fn document_all_diagnostics(
        &mut self,
        uri: &str,
    ) -> Option<(i32, String, Vec<Diagnostic>)> {
        if !ensure_analysis_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_mut(),
            &mut self.user_catalog,
            &self.analysis_config,
            Some(&self.external_defs),
        ) {
            return None;
        }
        let doc = self
            .documents
            .get(uri)
            .expect("ensure_analysis_for guarantees document exists");
        let version = doc.version;
        let source = doc.source.clone();
        let diags = doc
            .cached_all_diags
            .as_ref()
            .expect("ensure_analysis sets cached_all_diags")
            .clone();
        Some((version, source, diags))
    }

    /// Analysis diagnostics for a document (non-parse-error issues only).
    ///
    /// Always re-analyzes with `user_catalog` and `config`; use
    /// [`diagnostics`](Self::diagnostics) for the cheaper cached parse-error path.
    #[cfg(feature = "analysis")]
    pub(crate) fn analyze(&mut self, uri: &str, config: &AnalysisConfig) -> Vec<Diagnostic> {
        let Some(source) = self.documents.get(uri).map(|d| d.source.as_str()) else {
            return Vec::new();
        };
        let mut ctx = AnalysisContext::new(&mut self.user_catalog).with_config(*config);
        let model = self.analyzer.analyze(source, &mut ctx);
        model
            .diagnostics()
            .filter(|d| !d.message.is_parse_error())
            .cloned()
            .collect()
    }

    /// Parse + semantic diagnostics combined.
    #[cfg(feature = "analysis")]
    pub fn all_diagnostics(&mut self, uri: &str, config: &AnalysisConfig) -> Vec<Diagnostic> {
        let mut result = self.diagnostics(uri).to_vec();
        result.extend(self.analyze(uri, config));
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

    /// Hover information at a byte offset: returns (`hover_text`, `token_range`).
    pub(crate) fn hover_info(
        &mut self,
        uri: &str,
        offset: DocOffset,
    ) -> Option<(String, DocRange)> {
        ensure_analysis_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_mut(),
            &mut self.user_catalog,
            &self.analysis_config,
            Some(&self.external_defs),
        );
        let doc = self.documents.get(uri)?;
        let data = doc
            .analysis
            .as_ref()
            .expect("ensure_analysis sets analysis");
        super::hover_service::hover_info(data, offset)
    }

    // ── Go-to-definition ───────────────────────────────────────────────────

    /// Return the definition location for the symbol at `offset`.
    pub(crate) fn definition_info(
        &mut self,
        uri: &str,
        offset: DocOffset,
    ) -> Option<DefinitionResult> {
        ensure_analysis_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_mut(),
            &mut self.user_catalog,
            &self.analysis_config,
            Some(&self.external_defs),
        );
        let doc = self.documents.get(uri)?;
        let data = doc
            .analysis
            .as_ref()
            .expect("ensure_analysis sets analysis");
        super::hover_service::definition_info(data, offset)
    }

    // ── Find references ──────────────────────────────────────────────────────

    /// Find all references to the symbol at `offset` across all open documents.
    ///
    /// Returns a list of `(uri, range)` tuples. When `include_declaration`
    /// is true, the definition site (if known) is included in the results.
    pub(crate) fn find_references(
        &mut self,
        uri: &str,
        offset: DocOffset,
        include_declaration: bool,
    ) -> Vec<(String, DocRange)> {
        super::refs_service::find_references(
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_mut(),
            &mut self.user_catalog,
            &self.analysis_config,
            &self.external_defs,
            uri,
            offset,
            include_declaration,
        )
    }

    // ── Rename ──────────────────────────────────────────────────────────────

    /// Check if the symbol at `offset` is renameable, returning `(range, current_name)`.
    pub(crate) fn prepare_rename(
        &mut self,
        uri: &str,
        offset: DocOffset,
    ) -> Option<(DocRange, String)> {
        ensure_analysis_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_mut(),
            &mut self.user_catalog,
            &self.analysis_config,
            Some(&self.external_defs),
        );
        let doc = self.documents.get(uri)?;
        let data = doc
            .analysis
            .as_ref()
            .expect("ensure_analysis sets analysis");
        super::hover_service::prepare_rename(data, offset)
    }

    /// Rename the symbol at `offset` to `new_name` across all open documents.
    ///
    /// Returns a map of `uri -> Vec<(range, new_text)>` edits.
    pub(crate) fn rename(
        &mut self,
        uri: &str,
        offset: DocOffset,
        new_name: &str,
    ) -> HashMap<String, Vec<(DocRange, String)>> {
        super::refs_service::rename(
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_mut(),
            &mut self.user_catalog,
            &self.analysis_config,
            &self.external_defs,
            uri,
            offset,
            new_name,
        )
    }

    // ── Signature help ────────────────────────────────────────────────────────

    /// Signature help at a byte offset: finds enclosing function call and returns
    /// (`function_name`, `active_parameter`, overloads).
    pub(crate) fn signature_help(
        &mut self,
        uri: &str,
        offset: DocOffset,
    ) -> Option<SignatureHelpInfo> {
        ensure_analysis_for(
            uri,
            &mut self.documents,
            &mut self.analyzer,
            self.schema_map.as_mut(),
            &mut self.user_catalog,
            &self.analysis_config,
            Some(&self.external_defs),
        );
        let doc = self.documents.get(uri)?;
        let data = doc
            .analysis
            .as_ref()
            .expect("ensure_analysis sets analysis");
        let source = doc.source.as_str();

        // Walk backwards from offset to find enclosing `name(` and count commas.
        let cursor_byte = std::cmp::min(offset.as_usize(), source.len());
        let before = &source[..cursor_byte];
        let (func_name, active_param) =
            completion::find_enclosing_call(before, &data.tokens, &self.dialect)?;

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
        let (catalog, errors) = Catalog::from_ddl(self.dialect.clone(), &[ddl]);
        self.set_session_context(catalog);
        if let Some(uri) = file_uri {
            record_external_definitions(&mut self.external_defs, &self.dialect, ddl, uri);
        }
        if errors.is_empty() {
            Ok(())
        } else {
            Err(errors)
        }
    }
}

/// Parse `ddl` and record table/view/column definition spans into `defs`,
/// tagged with `file_uri`.
#[cfg(feature = "sqlite")]
fn record_external_definitions(
    defs: &mut ExternalDefinitions,
    dialect: &AnyDialect,
    ddl: &str,
    file_uri: &str,
) {
    use syntaqlite_syntax::ParseOutcome;

    use crate::analysis::stmt_reader::StmtReader;

    let parser = syntaqlite_syntax::Parser::new();
    let mut session = parser.parse(ddl);
    loop {
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => break,
            ParseOutcome::Err(_) => continue,
        };
        let Some(root) = stmt.root() else { continue };
        let root_id = root.node_id().into();
        let erased = stmt.erase();
        let reader = StmtReader::new(&erased, dialect.roles());
        if let Some((name, range)) = reader.name_span(root_id) {
            defs.insert_relation(&name, file_uri, range);
            for (col_name, col_range) in reader.column_spans(root_id) {
                defs.insert_column(&name, &col_name, file_uri, col_range);
            }
        }
    }
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
#[cfg(feature = "sqlite")]
mod tests {
    use syntaqlite_syntax::source::DocOffset;

    use super::LspHost;
    use crate::analysis::AnalysisConfig;
    use crate::analysis::Catalog;
    use crate::analysis::catalog::{AritySpec, CatalogLayer, FunctionCategory};
    use crate::analysis::diagnostics::Severity;
    use crate::lsp::CompletionKind;

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
    fn completion_context_after_select_is_not_table_ref() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "SELECT ";
        host.open_document(uri, 1, sql.to_string());
        let info = host
            .completion_info_at_offset(uri, DocOffset::from_raw(u32::try_from(sql.len()).unwrap()));
        assert_ne!(info.context, super::super::CompletionContext::TableRef);
    }

    #[test]
    fn completion_context_after_where_is_expression() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "SELECT * FROM t WHERE ";
        host.open_document(uri, 1, sql.to_string());
        let info = host
            .completion_info_at_offset(uri, DocOffset::from_raw(u32::try_from(sql.len()).unwrap()));
        assert_eq!(info.context, super::super::CompletionContext::Expression);
    }

    #[test]
    fn analyze_does_not_duplicate_parse_error_diagnostics() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(uri, 1, "SELECT ;\nSELECT 1;".to_string());
        let diags = host.analyze(uri, &AnalysisConfig::default());
        assert_eq!(diags.len(), 0, "got: {diags:?}");
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
        let start = diag.start().as_usize();
        assert_eq!(
            start,
            second_where,
            "got '{}' at {}",
            &sql[start..=start],
            start
        );
    }

    #[test]
    fn parse_and_validate_combined_no_duplicates() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        host.open_document(uri, 1, "SELECT ;\nSELECT * FROM no_such_table;".to_string());
        let parse_diags = host.diagnostics(uri).to_vec();
        let val_diags = host.analyze(uri, &AnalysisConfig::default());
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
        let result =
            host.definition_info(uri, DocOffset::from_raw(u32::try_from(ref_offset).unwrap()));
        assert!(result.is_some(), "expected definition for schema table");
        let def = result.unwrap();
        assert_eq!(def.target.file_uri.as_deref(), Some(file_uri));
        let schema_offset = schema.find("orders").unwrap();
        assert_eq!(def.target.range.start.as_usize(), schema_offset);
        assert_eq!(
            def.target.range.end.as_usize(),
            schema_offset + "orders".len()
        );
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
        let result =
            host.definition_info(uri, DocOffset::from_raw(u32::try_from(ref_offset).unwrap()));
        assert!(result.is_some(), "expected definition for schema column");
        let def = result.unwrap();
        assert_eq!(def.target.file_uri.as_deref(), Some(file_uri));
        let schema_offset = schema.find("total").unwrap();
        assert_eq!(def.target.range.start.as_usize(), schema_offset);
        assert_eq!(
            def.target.range.end.as_usize(),
            schema_offset + "total".len()
        );
    }

    // ── Find-references tests ──────────────────────────────────────────────

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
        let refs = host.find_references(
            uri1,
            DocOffset::from_raw(u32::try_from(offset).unwrap()),
            false,
        );
        assert_eq!(refs.len(), 2, "expected refs in both files, got: {refs:?}");
        let ref_uris: Vec<&str> = refs.iter().map(|r| r.0.as_str()).collect();
        assert!(ref_uris.contains(&uri1));
        assert!(ref_uris.contains(&uri2));
    }

    // ── Rename tests ────────────────────────────────────────────────────────

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
        let edits = host.rename(
            uri1,
            DocOffset::from_raw(u32::try_from(offset).unwrap()),
            "invoices",
        );
        // Should have edits in both open files.
        assert!(edits.contains_key(uri1), "expected edits in a.sql");
        assert!(edits.contains_key(uri2), "expected edits in b.sql");
    }

    #[test]
    fn completion_columns_sorted_before_functions_in_expression() {
        let mut host = LspHost::new();
        let uri = "file:///test.sql";
        let sql = "CREATE TABLE slice (id INT, name TEXT);\n\
                   CREATE TABLE thread (tid INT, parent INT);\n\
                   SELECT slice.id, thread.tid\nFROM slice\nJOIN thread ON ";
        host.open_document(uri, 1, sql.to_string());
        let items =
            host.completion_items(uri, DocOffset::from_raw(u32::try_from(sql.len()).unwrap()));

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
        use crate::analysis::Catalog;

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
        use crate::analysis::diagnostics::Severity;
        use std::path::PathBuf;

        let mut host = LspHost::new();

        // Build a catalog that knows about "users" table.
        let dialect = crate::sqlite::dialect::any_dialect();
        let (users_catalog, _) = Catalog::from_ddl(
            dialect.clone(),
            &["CREATE TABLE users (id INTEGER, name TEXT);"],
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
    fn goto_definition_schema_table_jumps_to_external_file() {
        let schema = "CREATE TABLE users (id INTEGER, name TEXT);";
        let file_uri = "file:///path/to/schema.sql";

        let mut host = LspHost::new();
        host.set_session_context_from_ddl(schema, Some(file_uri))
            .expect("schema parses");

        let uri = "file:///test.sql";
        let src = "SELECT * FROM users";
        host.open_document(uri, 1, src.to_string());

        let ref_offset = src.find("users").unwrap();
        let def = host
            .definition_info(uri, DocOffset::from_raw(u32::try_from(ref_offset).unwrap()))
            .expect("definition");
        assert_eq!(def.target.file_uri.as_deref(), Some(file_uri));
        let schema_offset = schema.find("users").unwrap();
        assert_eq!(def.target.range.start.as_usize(), schema_offset);
        assert_eq!(
            def.target.range.end.as_usize(),
            schema_offset + "users".len()
        );
    }

    // ── Shell script (sqlite3 CLI dot-commands) — issue #88 ──────────────

    /// Issue #88: a file consisting of sqlite3 shell `.read` dot-commands plus
    /// SQL must NOT be flagged "syntax error near '.'". The LSP host should
    /// auto-detect shell mode and route the SQL fragments through the embedded
    /// analyzer, so the dot-command lines are treated as shell content (not SQL)
    /// and produce zero parse-error diagnostics.
    #[test]
    fn shell_dot_commands_produce_no_parse_errors() {
        let mut host = LspHost::new();
        let uri = "file:///init.sql";
        let src = ".read adapters/sqlite/scripts/includes/01-pragma.sql\n\
                   .read adapters/sqlite/scripts/includes/02-drops.sql\n\
                   SELECT 1;";
        host.open_document(uri, 1, src.to_string());

        let (_, _, diags) = host.document_all_diagnostics(uri).expect("document exists");
        let parse_errors: Vec<_> = diags
            .iter()
            .filter(|d| d.message().is_parse_error())
            .collect();
        assert!(
            parse_errors.is_empty(),
            "shell .read lines must not be flagged as SQL syntax errors, got: {parse_errors:?}"
        );
    }

    /// Companion to [`shell_dot_commands_produce_no_parse_errors`]: a *pure* SQL
    /// document containing a stray `GO` on its own line must STILL report a
    /// parse error. `GO` is only a terminator in shell mode; we must not have
    /// globally swallowed it.
    #[test]
    fn stray_go_in_pure_sql_still_errors() {
        let mut host = LspHost::new();
        let uri = "file:///plain.sql";
        // A terminated statement followed by a bare `GO` line: in shell mode
        // `GO` is a terminator, but a pure-SQL file (no shell markers) must
        // still treat the trailing `GO` as a syntax error.
        let src = "SELECT 1;\nGO GO GO";
        host.open_document(uri, 1, src.to_string());

        let (_, _, diags) = host.document_all_diagnostics(uri).expect("document exists");
        let parse_errors: Vec<_> = diags
            .iter()
            .filter(|d| d.message().is_parse_error())
            .collect();
        assert!(
            !parse_errors.is_empty(),
            "a stray GO in a pure-SQL file must remain a parse error"
        );
    }

    #[test]
    fn goto_definition_same_file_ddl_shadows_schema() {
        let schema = "CREATE TABLE t (x INTEGER);";
        let file_uri = "file:///schema.sql";

        let mut host = LspHost::new();
        host.set_session_context_from_ddl(schema, Some(file_uri))
            .expect("schema parses");

        let uri = "file:///test.sql";
        let src = "CREATE TABLE t (y INTEGER); SELECT * FROM t;";
        host.open_document(uri, 1, src.to_string());

        let ref_offset = src.rfind(" t").unwrap() + 1;
        let def = host
            .definition_info(uri, DocOffset::from_raw(u32::try_from(ref_offset).unwrap()))
            .expect("definition");
        assert!(def.target.file_uri.is_none());
        assert_eq!(def.target.range.start.as_usize(), src.find('t').unwrap());
    }
}
