// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Single-pass semantic analysis engine.

use std::collections::{HashMap, HashSet};

use syntaqlite_syntax::ParserConfig;
use syntaqlite_syntax::any::{
    AnyNodeId, AnyParseError, AnyParsedStatement, AnyParser, AnyTokenType, FieldValue, NodeFields,
    ParseOutcome, TokenCategory,
};

use crate::dialect::AnyDialect;
use crate::dialect::{FIELD_ABSENT, SemanticRole};

use super::catalog::{
    AritySpec, Catalog, CatalogLayer, ColumnResolution, FunctionCategory, FunctionCheckResult,
    columns_from_select,
};
use super::diagnostics::{Diagnostic, DiagnosticMessage, Help};
use super::fuzzy::best_suggestion;
use super::model::{
    CompletionContext, CompletionInfo, DefinitionLocation, Resolution, ResolvedSymbol,
    SemanticModel, SemanticToken, StoredComment, StoredToken,
};
use super::{AnalysisMode, CheckConfig, CheckLevel, ValidationConfig};

/// Long-lived semantic analysis engine.
///
/// Create once for a dialect and reuse across inputs. The dialect layer is
/// built at construction and never changes. The database and document layers
/// are reset on each [`analyze`](Self::analyze) call.
///
/// Set [`AnalysisMode::Execute`] via [`with_mode`](Self::with_mode) to make
/// DDL accumulate across calls (interactive session semantics).
///
/// # Example
///
/// ```
/// # use syntaqlite::{
/// #     SemanticAnalyzer, Catalog, ValidationConfig,
/// # };
/// # use syntaqlite::semantic::{CatalogLayer, Severity};
/// // 1. Create analyzer (reusable across many inputs).
/// let mut analyzer = SemanticAnalyzer::new();
///
/// // 2. Set up a catalog describing the database schema.
/// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
/// catalog
///     .layer_mut(CatalogLayer::Database)
///     .insert_table("users", Some(vec!["id".into(), "name".into()]), false);
///
/// // 3. Analyze a query.
/// let config = ValidationConfig::default();
/// let model = analyzer.analyze("SELECT id, name FROM users;", &catalog, &config);
///
/// // 4. No diagnostics — the query is valid against the schema.
/// assert!(model.diagnostics().is_empty());
/// ```
pub struct SemanticAnalyzer {
    dialect: AnyDialect,
    catalog: Catalog,
    mode: AnalysisMode,
    macro_fallback: bool,
}

#[expect(dead_code)]
impl SemanticAnalyzer {
    /// Create an analyzer for the built-in `SQLite` dialect.
    ///
    /// This is the most common entry point. The returned analyzer is ready to
    /// use with [`analyze`](Self::analyze). For custom or third-party dialects,
    /// use [`with_dialect`](Self::with_dialect) instead.
    ///
    /// # Example
    ///
    /// ```
    /// # use syntaqlite::SemanticAnalyzer;
    /// let mut analyzer = SemanticAnalyzer::new();
    /// ```
    #[cfg(feature = "sqlite")]
    pub fn new() -> Self {
        Self::with_dialect(crate::sqlite::dialect::dialect())
    }

    /// The analyzer's internal catalog (includes DDL from last analysis).
    pub(crate) fn catalog(&self) -> &Catalog {
        &self.catalog
    }

    /// Create an analyzer bound to a specific dialect.
    pub fn with_dialect(dialect: impl Into<AnyDialect>) -> Self {
        let dialect = dialect.into();
        SemanticAnalyzer {
            catalog: Catalog::new(dialect.clone()),
            dialect,
            mode: AnalysisMode::default(),
            macro_fallback: false,
        }
    }

    /// Set the analysis mode (builder pattern). See [`AnalysisMode`] for details.
    #[must_use]
    pub fn with_mode(mut self, mode: AnalysisMode) -> Self {
        self.mode = mode;
        self
    }

    /// Set the analysis mode on an existing analyzer.
    pub fn set_mode(&mut self, mode: AnalysisMode) {
        self.mode = mode;
    }

    /// Enable macro fallback: unregistered `name!(args)` calls parse as
    /// identifiers and record [`MacroRegion`]s on the resulting model.
    #[must_use]
    pub(crate) fn with_macro_fallback(mut self, enabled: bool) -> Self {
        self.macro_fallback = enabled;
        self
    }

    /// Return the dialect this analyzer was constructed for.
    pub(crate) fn dialect(&self) -> AnyDialect {
        self.dialect.clone()
    }

    /// Run a complete single-pass analysis: parse, collect tokens, walk AST.
    ///
    /// `user_catalog` supplies the database layer (user-provided schema). Its
    /// database layer is merged into the analyzer's catalog for this pass only.
    /// The document layer is cleared and rebuilt statement-by-statement so that
    /// DDL seen earlier in the file is visible to queries that follow it.
    ///
    /// In [`AnalysisMode::Execute`], DDL from this call is promoted to the
    /// connection layer so it persists across subsequent calls.
    ///
    /// # Example
    ///
    /// ```
    /// # use syntaqlite::{
    /// #     SemanticAnalyzer, Catalog, ValidationConfig,
    /// # };
    /// # use syntaqlite::semantic::{CatalogLayer, Severity};
    /// let mut analyzer = SemanticAnalyzer::new();
    /// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
    /// catalog
    ///     .layer_mut(CatalogLayer::Database)
    ///     .insert_table("users", Some(vec!["id".into(), "name".into()]), false);
    ///
    /// let config = ValidationConfig::default();
    ///
    /// // Referencing a column that does not exist produces a diagnostic.
    /// let model = analyzer.analyze("SELECT email FROM users;", &catalog, &config);
    /// assert!(!model.diagnostics().is_empty());
    /// assert_eq!(model.diagnostics()[0].severity(), Severity::Warning);
    /// ```
    pub fn analyze(
        &mut self,
        source: &str,
        user_catalog: &Catalog,
        config: &ValidationConfig,
    ) -> SemanticModel {
        self.catalog.new_document();
        match self.mode {
            AnalysisMode::Document => {
                self.catalog.copy_schema_layers_from(user_catalog);
            }
            AnalysisMode::Execute => {
                // Only copy Database — Connection accumulates executed DDL.
                self.catalog.copy_database_from(user_catalog);
            }
        }
        let model = self.analyze_inner(source, config);
        if self.mode == AnalysisMode::Execute {
            self.catalog.promote_document_to_connection();
        }
        model
    }

    /// Semantic tokens for syntax highlighting, derived from a prior
    /// [`analyze`](Self::analyze) result.
    pub(crate) fn semantic_tokens(&self, model: &SemanticModel) -> Vec<SemanticToken> {
        use syntaqlite_syntax::any::TokenCategory;

        let mut out = Vec::new();
        for t in &model.tokens {
            let cat = self.dialect.classify_token(t.token_type, t.flags);
            if cat != TokenCategory::Other {
                out.push(SemanticToken {
                    offset: t.offset,
                    length: t.length,
                    category: cat,
                });
            }
        }
        for c in &model.comments {
            out.push(SemanticToken {
                offset: c.offset,
                length: c.length,
                category: TokenCategory::Comment,
            });
        }
        out.sort_by_key(|t| t.offset);
        out
    }

    /// Expected tokens and semantic context at `offset` (for completion).
    pub(crate) fn completion_info(&self, model: &SemanticModel, offset: usize) -> CompletionInfo {
        let source = model.source();
        let tokens = &model.tokens;
        let cursor = offset.min(source.len());
        let (boundary, backtracked) = completion_boundary(source, tokens, cursor);
        let start = statement_token_start(tokens, boundary);
        let stmt_tokens = &tokens[start..boundary];

        let grammar = (*self.dialect).clone();
        let parser = AnyParser::with_config(grammar, &ParserConfig::default());
        let mut cursor_p = parser.incremental_parse(source);
        // Do not call expected_tokens() before feeding any tokens: the C parser
        // returns a garbage `total` count when no tokens have been fed yet,
        // which would trigger a multi-GiB allocation and SIGKILL.
        let mut last_expected: Vec<AnyTokenType> = Vec::new();

        for (i, tok) in stmt_tokens.iter().enumerate() {
            let span = tok.offset..(tok.offset + tok.length);
            if cursor_p.feed_token(tok.token_type, span).is_some() {
                let qualifier = detect_qualifier(source, &stmt_tokens[..=i], &self.dialect);
                return CompletionInfo {
                    tokens: last_expected,
                    context: CompletionContext::from_parser(cursor_p.completion_context()),
                    qualifier,
                };
            }
            last_expected.clear();
            last_expected.extend(cursor_p.expected_tokens());
        }

        let context = CompletionContext::from_parser(cursor_p.completion_context());

        if backtracked && let Some(extra) = tokens.get(boundary) {
            let span = extra.offset..(extra.offset + extra.length);
            if cursor_p.feed_token(extra.token_type, span).is_none() {
                merge_expected_tokens(
                    &mut last_expected,
                    cursor_p.expected_tokens().collect::<Vec<AnyTokenType>>(),
                );
            }
        }

        let qualifier = detect_qualifier(source, stmt_tokens, &self.dialect);

        CompletionInfo {
            tokens: last_expected,
            context,
            qualifier,
        }
    }

    // ── Private ───────────────────────────────────────────────────────────────

    fn analyze_inner(&mut self, source: &str, config: &ValidationConfig) -> SemanticModel {
        let grammar = (*self.dialect).clone();
        let parser = AnyParser::with_config(
            grammar,
            &ParserConfig::default()
                .with_collect_tokens(true)
                .with_macro_fallback(self.macro_fallback),
        );
        let mut session = parser.parse(source);

        let mut tokens: Vec<StoredToken> = Vec::new();
        let mut comments: Vec<StoredComment> = Vec::new();
        let mut diagnostics: Vec<Diagnostic> = Vec::new();
        let mut definition_offsets: HashMap<String, (usize, usize)> = HashMap::new();
        let mut resolutions: Vec<Resolution> = Vec::new();
        let mut query_lineage: Option<super::lineage::QueryLineage> = None;

        loop {
            let stmt = match session.next() {
                ParseOutcome::Done => break,
                ParseOutcome::Ok(s) => s,
                ParseOutcome::Err(e) => {
                    let message = DiagnosticMessage::ParseError(e.message().to_owned());
                    if let Some(severity) = config.checks().level_for(&message).to_severity() {
                        let (start, end) = parse_error_span(&e, source);
                        diagnostics.push(Diagnostic {
                            start_offset: start,
                            end_offset: end,
                            message,
                            severity,
                            help: None,
                        });
                    }
                    collect_tokens(e.tokens(), &mut tokens);
                    collect_comments(e.comments(), &mut comments);
                    continue;
                }
            };

            collect_tokens(stmt.tokens(), &mut tokens);
            collect_comments(stmt.comments(), &mut comments);

            let erased = stmt.erase();
            self.analyze_statement(
                &erased,
                config,
                &mut diagnostics,
                &mut resolutions,
                &mut definition_offsets,
                &mut query_lineage,
            );
        }

        SemanticModel {
            source: source.to_owned(),
            tokens,
            comments,
            diagnostics,
            resolutions,
            definition_offsets,
            query_lineage,
        }
    }

    fn analyze_statement(
        &mut self,
        erased: &AnyParsedStatement<'_>,
        config: &ValidationConfig,
        diagnostics: &mut Vec<Diagnostic>,
        resolutions: &mut Vec<Resolution>,
        definition_offsets: &mut HashMap<String, (usize, usize)>,
        query_lineage: &mut Option<super::lineage::QueryLineage>,
    ) {
        let root_id = erased.root_id();

        self.catalog
            .accumulate_ddl(CatalogLayer::Document, erased, root_id, &self.dialect);

        // Record DDL definition offsets for go-to-definition (same-file).
        if let Some((table_name, off)) = ddl_name_offset(erased, root_id, &self.dialect) {
            for (col_name, col_start, col_end) in
                super::catalog::ddl_column_spans(erased, root_id, self.dialect.roles())
            {
                let key = format!("{table_name}.{col_name}");
                definition_offsets.insert(key, (col_start, col_end));
            }
            definition_offsets.insert(table_name, off);
        }

        ValidationPass::run(
            erased,
            root_id,
            &self.dialect,
            &mut self.catalog,
            config,
            diagnostics,
            resolutions,
            definition_offsets,
        );

        *query_lineage =
            super::lineage::compute_lineage(erased, root_id, &self.catalog, self.dialect.roles())
                .or(query_lineage.take());
    }
}

#[cfg(feature = "sqlite")]
impl Default for SemanticAnalyzer {
    fn default() -> Self {
        Self::new()
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

/// If the last two tokens in `tokens` are `identifier DOT`, return the
/// identifier text as the qualifier. This is used to detect `table.` prefixes
/// for qualified column completion.
fn detect_qualifier(source: &str, tokens: &[StoredToken], dialect: &AnyDialect) -> Option<String> {
    if tokens.len() < 2 {
        return None;
    }
    let dot_tok = &tokens[tokens.len() - 1];
    let ident_tok = &tokens[tokens.len() - 2];

    // The DOT must be a single-character punctuation token.
    if dot_tok.length != 1 || source.as_bytes().get(dot_tok.offset) != Some(&b'.') {
        return None;
    }

    // The token before the DOT must be an identifier (use raw category,
    // not classify_token, since parser flags may reclassify it).
    let cat = dialect.token_category(ident_tok.token_type);
    if cat != TokenCategory::Identifier {
        return None;
    }

    let name = &source[ident_tok.offset..ident_tok.offset + ident_tok.length];
    Some(name.to_string())
}

fn format_arity(name: &str, arity: AritySpec) -> String {
    match arity {
        AritySpec::Exact(n) => {
            let params: Vec<String> = (0..n).map(|i| format!("arg{}", i + 1)).collect();
            format!("{}({})", name, params.join(", "))
        }
        AritySpec::AtLeast(n) => {
            let mut params: Vec<String> = (0..n).map(|i| format!("arg{}", i + 1)).collect();
            params.push("...".to_string());
            format!("{}({})", name, params.join(", "))
        }
        AritySpec::Any => format!("{name}(...)"),
    }
}

fn collect_tokens<'a>(
    iter: impl Iterator<Item = syntaqlite_syntax::any::AnyParserToken<'a>>,
    tokens: &mut Vec<StoredToken>,
) {
    for tok in iter {
        tokens.push(StoredToken {
            offset: tok.offset() as usize,
            length: tok.length() as usize,
            token_type: tok.token_type(),
            flags: tok.flags(),
        });
    }
}

fn collect_comments<'a>(
    iter: impl Iterator<Item = syntaqlite_syntax::Comment<'a>>,
    comments: &mut Vec<StoredComment>,
) {
    for c in iter {
        comments.push(StoredComment {
            offset: c.offset() as usize,
            length: c.length() as usize,
        });
    }
}

fn parse_error_span(err: &AnyParseError<'_>, source: &str) -> (usize, usize) {
    match (err.offset(), err.length()) {
        (Some(off), Some(len)) if len > 0 => (off, off + len),
        (Some(off), _) => {
            if off >= source.len() && !source.is_empty() {
                (source.len() - 1, source.len())
            } else {
                (off, (off + 1).min(source.len()))
            }
        }
        _ => {
            let end = source.len();
            let start = if end > 0 { end - 1 } else { 0 };
            (start, end)
        }
    }
}

fn completion_boundary(
    source: &str,
    tokens: &[StoredToken],
    cursor_offset: usize,
) -> (usize, bool) {
    let mut boundary = tokens.partition_point(|t| t.offset + t.length <= cursor_offset);

    while boundary > 0 {
        let tok = &tokens[boundary - 1];
        if tok.length == 0 && tok.offset == cursor_offset {
            boundary -= 1;
        } else {
            break;
        }
    }

    let mut backtracked = false;
    if boundary > 0
        && tokens[boundary - 1].offset + tokens[boundary - 1].length == cursor_offset
        && cursor_offset > 0
    {
        let prev = source.as_bytes()[cursor_offset - 1];
        if prev.is_ascii_alphanumeric() || prev == b'_' {
            boundary -= 1;
            backtracked = true;
        }
    }
    (boundary, backtracked)
}

/// Find the index of the first token in the statement that contains `offset`.
///
/// Uses `TokenType::Semi` — safe across all dialects because `SQLite` token
/// ordinals are stable and equal to `AnyTokenType` ordinals.
fn statement_token_start(tokens: &[StoredToken], boundary: usize) -> usize {
    let semi = AnyTokenType::from(syntaqlite_syntax::TokenType::Semi);
    tokens[..boundary]
        .iter()
        .rposition(|t| t.token_type == semi)
        .map_or(0, |idx| idx + 1)
}

fn merge_expected_tokens(into: &mut Vec<AnyTokenType>, extra: Vec<AnyTokenType>) {
    let mut seen: HashSet<AnyTokenType> = into.iter().copied().collect();
    for token in extra {
        if seen.insert(token) {
            into.push(token);
        }
    }
}

// ── QueryScope ─────────────────────────────────────────────────────────────────

/// Whether a table has an implicit rowid column.
#[derive(Clone, Copy, PartialEq, Eq)]
enum RowIdPolicy {
    /// Normal table — `rowid`, `oid`, `_rowid_` are valid implicit columns.
    WithRowId,
    /// WITHOUT ROWID table — no implicit rowid column exists.
    WithoutRowId,
}

impl From<bool> for RowIdPolicy {
    fn from(without_rowid: bool) -> Self {
        if without_rowid {
            Self::WithoutRowId
        } else {
            Self::WithRowId
        }
    }
}

/// A table entry in the query scope.
struct ActiveTable {
    /// `None` = table exists but column list is unknown (accept any ref).
    columns: Option<Vec<String>>,
    rowid: RowIdPolicy,
}

impl ActiveTable {
    /// Does this table have `column` (by name or implicit rowid)?
    fn has_column(&self, column: &str) -> bool {
        match &self.columns {
            Some(cs) => {
                cs.iter().any(|c| c.eq_ignore_ascii_case(column))
                    || (self.rowid == RowIdPolicy::WithRowId && is_rowid_alias(column))
            }
            None => true, // unknown columns — accept anything
        }
    }
}

/// A single scope frame: named tables plus anonymous sources.
#[derive(Default)]
struct ScopeFrame {
    /// Named tables/aliases, keyed by lowercase name.
    named: HashMap<String, ActiveTable>,
    /// Anonymous sources (unaliased subqueries). Participate in unqualified
    /// column resolution but cannot be referenced by name.
    anonymous: Vec<ActiveTable>,
}

/// Tracks which tables are "active" (in FROM/JOIN) for column resolution.
///
/// Completely separate from [`Catalog`] (schema-level).  Frames are
/// pushed/popped as we enter/leave SELECT statements, subqueries, and DML
/// blocks.  Inner frames are searched first (supporting correlated subqueries).
/// CTE definitions are never stored here — CTEs only contribute columns when
/// they appear in a FROM clause.
#[derive(Default)]
struct QueryScope {
    frames: Vec<ScopeFrame>,
}

impl QueryScope {
    fn has_frames(&self) -> bool {
        !self.frames.is_empty()
    }

    fn push(&mut self) {
        self.frames.push(ScopeFrame::default());
    }

    fn pop(&mut self) {
        self.frames.pop();
    }

    /// Register a named table (or alias) as active in the current scope frame.
    fn add_table(&mut self, name: &str, columns: Option<Vec<String>>, rowid: RowIdPolicy) {
        if let Some(frame) = self.frames.last_mut() {
            frame
                .named
                .insert(name.to_ascii_lowercase(), ActiveTable { columns, rowid });
        }
    }

    /// Register an anonymous source (unaliased subquery) in the current frame.
    fn add_anonymous(&mut self, columns: Option<Vec<String>>) {
        if let Some(frame) = self.frames.last_mut() {
            frame.anonymous.push(ActiveTable {
                columns,
                rowid: RowIdPolicy::WithRowId,
            });
        }
    }

    /// Resolve a column reference against active FROM tables.
    fn resolve_column(&self, table: Option<&str>, column: &str) -> ColumnResolution {
        if let Some(tbl) = table {
            self.resolve_qualified(tbl, column)
        } else {
            self.resolve_unqualified(column)
        }
    }

    fn resolve_qualified(&self, table: &str, column: &str) -> ColumnResolution {
        let key = table.to_ascii_lowercase();
        for frame in self.frames.iter().rev() {
            let Some(entry) = frame.named.get(&key) else {
                continue;
            };
            if entry.has_column(column) {
                return ColumnResolution::Found {
                    table: table.to_string(),
                    all_columns: entry.columns.clone().unwrap_or_default(),
                };
            }
            return match &entry.columns {
                Some(_) => ColumnResolution::TableFoundColumnMissing,
                None => ColumnResolution::Found {
                    table: table.to_string(),
                    all_columns: Vec::new(),
                },
            };
        }
        ColumnResolution::TableNotFound
    }

    fn resolve_unqualified(&self, column: &str) -> ColumnResolution {
        for frame in self.frames.iter().rev() {
            let mut frame_has_unknown = false;

            // Search named tables.
            for (tbl_name, entry) in &frame.named {
                if entry.has_column(column) {
                    return ColumnResolution::Found {
                        table: tbl_name.clone(),
                        all_columns: entry.columns.clone().unwrap_or_default(),
                    };
                }
                if entry.columns.is_none() {
                    frame_has_unknown = true;
                }
            }

            // Search anonymous sources.
            for entry in &frame.anonymous {
                if entry.has_column(column) {
                    return ColumnResolution::Found {
                        table: String::new(),
                        all_columns: entry.columns.clone().unwrap_or_default(),
                    };
                }
                if entry.columns.is_none() {
                    frame_has_unknown = true;
                }
            }

            // A source with unknown columns in this frame could own the column —
            // don't leak into outer scopes.
            if frame_has_unknown {
                return ColumnResolution::Found {
                    table: String::new(),
                    all_columns: Vec::new(),
                };
            }
        }
        ColumnResolution::NotFound
    }

    /// Collect all known column names (for fuzzy suggestions).
    fn all_column_names(&self, table: Option<&str>) -> Vec<String> {
        let mut names: Vec<String> = Vec::new();
        for frame in self.frames.iter().rev() {
            for (tbl_name, entry) in &frame.named {
                if table.is_none_or(|t| tbl_name.eq_ignore_ascii_case(t))
                    && let Some(cs) = &entry.columns
                {
                    names.extend(cs.iter().map(|c| c.to_ascii_lowercase()));
                }
            }
            if table.is_none() {
                for entry in &frame.anonymous {
                    if let Some(cs) = &entry.columns {
                        names.extend(cs.iter().map(|c| c.to_ascii_lowercase()));
                    }
                }
            }
        }
        names.sort_unstable();
        names.dedup();
        names
    }
}

/// Extract the definition name and byte-offset range from a DDL statement.
///
/// Returns `(lowercase_name, (start, end))` for CREATE TABLE / CREATE VIEW,
/// or `None` for non-DDL statements.
fn ddl_name_offset(
    stmt: &AnyParsedStatement<'_>,
    root: AnyNodeId,
    dialect: &AnyDialect,
) -> Option<(String, (usize, usize))> {
    let (tag, fields) = stmt.extract_fields(root)?;
    let role = dialect.roles().get(u32::from(tag) as usize)?;
    let name_idx = match role {
        SemanticRole::DefineTable { name, .. } | SemanticRole::DefineView { name, .. } => *name,
        _ => return None,
    };
    let FieldValue::Span { text: s, .. } = fields[name_idx as usize] else {
        return None;
    };
    if s.is_empty() {
        return None;
    }
    let off = s.as_ptr() as usize - stmt.source().as_ptr() as usize;
    Some((s.to_ascii_lowercase(), (off, off + s.len())))
}

/// `SQLite`'s implicit rowid aliases.
fn is_rowid_alias(column: &str) -> bool {
    column.eq_ignore_ascii_case("rowid")
        || column.eq_ignore_ascii_case("oid")
        || column.eq_ignore_ascii_case("_rowid_")
}

// ── ValidationPass ─────────────────────────────────────────────────────────────

/// Extracted info for a single CTE binding.
struct CteBindingInfo<'a> {
    name: &'a str,
    body_id: Option<AnyNodeId>,
    declared_cols: Option<Vec<&'a str>>,
}

struct ValidationPass<'a> {
    roles: &'static [SemanticRole],
    source_start: usize,
    catalog: &'a mut Catalog,
    config: &'a ValidationConfig,
    diagnostics: &'a mut Vec<Diagnostic>,
    resolutions: &'a mut Vec<Resolution>,
    scope: QueryScope,
    /// Maps `lowercase(name)` → `(start_offset, end_offset)` for definition sites.
    /// Populated from DDL (per-document) and CTE bindings (per-WITH scope).
    definition_offsets: &'a mut HashMap<String, (usize, usize)>,
}

impl CheckConfig {
    /// Get the check level for a diagnostic message's category.
    pub(crate) fn level_for(self, message: &DiagnosticMessage) -> CheckLevel {
        match message {
            DiagnosticMessage::UnknownTable { .. } => self.unknown_table,
            DiagnosticMessage::UnknownColumn { .. } => self.unknown_column,
            DiagnosticMessage::UnknownFunction { .. } => self.unknown_function,
            DiagnosticMessage::FunctionArity { .. } => self.function_arity,
            DiagnosticMessage::CteColumnCountMismatch { .. } => self.cte_columns,
            DiagnosticMessage::ParseError(_) => self.parse_errors,
        }
    }
}

impl<'a> ValidationPass<'a> {
    /// Push a diagnostic if its check category is not `allow`.
    /// Severity is determined entirely by the check level — callers do not
    /// specify it.
    fn emit(
        &mut self,
        start_offset: usize,
        end_offset: usize,
        message: DiagnosticMessage,
        help: Option<Help>,
    ) {
        if let Some(severity) = self.config.checks().level_for(&message).to_severity() {
            self.diagnostics.push(Diagnostic {
                start_offset,
                end_offset,
                message,
                severity,
                help,
            });
        }
    }

    #[expect(clippy::too_many_arguments)]
    fn run(
        stmt: &AnyParsedStatement<'a>,
        root: AnyNodeId,
        dialect: &AnyDialect,
        catalog: &'a mut Catalog,
        config: &'a ValidationConfig,
        diagnostics: &'a mut Vec<Diagnostic>,
        resolutions: &'a mut Vec<Resolution>,
        definition_offsets: &'a mut HashMap<String, (usize, usize)>,
    ) {
        let roles = dialect.roles();
        let source_start = stmt.source().as_ptr() as usize;
        let mut pass = ValidationPass {
            roles,
            source_start,
            catalog,
            config,
            diagnostics,
            resolutions,
            scope: QueryScope::default(),
            definition_offsets,
        };
        pass.visit(stmt, root);
    }

    // ── Core visitor ─────────────────────────────────────────────────────────

    fn visit(&mut self, stmt: &AnyParsedStatement<'a>, node_id: AnyNodeId) {
        if node_id.is_null() {
            return;
        }
        // List nodes have no role — visit their elements directly.
        if let Some(children) = stmt.list_children(node_id) {
            for child in children.iter().copied() {
                if !child.is_null() {
                    self.visit(stmt, child);
                }
            }
            return;
        }
        let Some((tag, fields)) = stmt.extract_fields(node_id) else {
            return;
        };
        let idx = u32::from(tag) as usize;
        let role = self
            .roles
            .get(idx)
            .copied()
            .unwrap_or(SemanticRole::Transparent);

        match role {
            // Catalog roles are handled in the accumulation pass, but we
            // still recurse into the SELECT body to validate table/column refs.
            SemanticRole::DefineTable { select, .. }
            | SemanticRole::DefineView { select, .. }
            | SemanticRole::DefineFunction { select, .. } => {
                if select != FIELD_ABSENT {
                    self.visit_opt(stmt, Self::field_node_id(&fields, select));
                }
            }
            SemanticRole::ReturnSpec { .. } | SemanticRole::Import { .. } => {}

            // Transparent: recurse into children without special handling.
            // ColumnDef and ResultColumn have no validation logic yet — child
            // expressions are reached via transparent traversal.
            SemanticRole::Transparent
            | SemanticRole::ColumnDef { .. }
            | SemanticRole::ResultColumn { .. }
            | SemanticRole::CteBinding { .. } => self.visit_children(stmt, node_id),

            SemanticRole::Call { name, args } => {
                self.visit_call(stmt, node_id, &fields, name, args);
            }
            SemanticRole::ColumnRef { column, table } => {
                self.visit_column_ref(&fields, column, table);
            }
            SemanticRole::SourceRef { name, alias, .. } => {
                self.visit_source_ref(stmt, &fields, name, alias);
            }
            SemanticRole::ScopedSource { body, alias } => {
                self.visit_scoped_source(stmt, &fields, body, alias);
            }
            SemanticRole::Query {
                from,
                columns,
                where_clause,
                groupby,
                having,
                orderby,
                limit_clause,
            } => self.visit_query(
                stmt,
                &fields,
                from,
                columns,
                where_clause,
                groupby,
                having,
                orderby,
                limit_clause,
            ),
            SemanticRole::CteScope {
                recursive,
                bindings,
                body,
            } => self.visit_cte_scope(stmt, &fields, recursive, bindings, body),
            SemanticRole::TriggerScope {
                target: _,
                when,
                body,
            } => {
                self.visit_trigger_scope(stmt, &fields, when, body);
            }
            SemanticRole::DmlScope => {
                self.scope.push();
                self.visit_children(stmt, node_id);
                self.scope.pop();
            }
        }
    }

    fn visit_children(&mut self, stmt: &AnyParsedStatement<'a>, node_id: AnyNodeId) {
        for child in stmt.child_node_ids(node_id) {
            if !child.is_null() {
                self.visit(stmt, child);
            }
        }
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    fn span_offset(&self, s: &str) -> usize {
        s.as_ptr() as usize - self.source_start
    }

    fn field_node_id(fields: &NodeFields<'_>, idx: u8) -> Option<AnyNodeId> {
        match fields[idx as usize] {
            FieldValue::NodeId(id) if !id.is_null() => Some(id),
            _ => None,
        }
    }

    fn visit_opt(&mut self, stmt: &AnyParsedStatement<'a>, id: Option<AnyNodeId>) {
        if let Some(id) = id {
            self.visit(stmt, id);
        }
    }

    /// Extract source text from a `Name` node (`IdentName` or `Error`).
    /// Both node kinds store their span at field 0.
    #[expect(clippy::unused_self)]
    fn name_text(&self, stmt: &AnyParsedStatement<'a>, node_id: Option<AnyNodeId>) -> &'a str {
        let Some(node_id) = node_id else {
            return "";
        };
        let Some((_, fields)) = stmt.extract_fields(node_id) else {
            return "";
        };
        if fields.is_empty() {
            return "";
        }
        match fields[0] {
            FieldValue::Span { text: s, .. } => s,
            _ => "",
        }
    }

    // ── Role handlers ─────────────────────────────────────────────────────────

    fn visit_source_ref(
        &mut self,
        stmt: &AnyParsedStatement<'a>,
        fields: &NodeFields<'a>,
        name_idx: u8,
        alias_idx: u8,
    ) {
        let FieldValue::Span { text: name, .. } = fields[name_idx as usize] else {
            return;
        };
        if name.is_empty() {
            return;
        }
        let offset = self.span_offset(name);

        let is_known =
            self.catalog.resolve_relation(name) || self.catalog.resolve_table_function(name);
        if !is_known {
            let mut candidates = self.catalog.all_relation_names();
            candidates.extend(self.catalog.all_table_function_names());
            let suggestion = best_suggestion(name, &candidates, self.config.suggestion_threshold());
            self.emit(
                offset,
                offset + name.len(),
                DiagnosticMessage::UnknownTable {
                    name: name.to_string(),
                },
                suggestion.map(Help::Suggestion),
            );
        }

        let alias = self.name_text(stmt, Self::field_node_id(fields, alias_idx));
        let scope_name = if alias.is_empty() { name } else { alias };
        let (columns, without_rowid) = self.catalog.table_source_info(name);

        if is_known {
            let definition = self
                .definition_offsets
                .get(&name.to_ascii_lowercase())
                .map(|&(start, end)| DefinitionLocation {
                    start,
                    end,
                    file_uri: None,
                })
                .or_else(|| {
                    self.catalog
                        .relation_definition_site(name)
                        .map(|site| DefinitionLocation {
                            start: site.start,
                            end: site.end,
                            file_uri: Some(site.file_uri.clone()),
                        })
                });
            self.resolutions.push(Resolution {
                start: offset,
                end: offset + name.len(),
                symbol: ResolvedSymbol::Table {
                    name: name.to_string(),
                    columns: columns.clone(),
                    definition,
                },
            });
        }

        self.scope
            .add_table(scope_name, columns, without_rowid.into());
    }

    fn visit_call(
        &mut self,
        stmt: &AnyParsedStatement<'a>,
        node_id: AnyNodeId,
        fields: &NodeFields<'a>,
        name_idx: u8,
        args_idx: u8,
    ) {
        if let FieldValue::Span { text: name, .. } = fields[name_idx as usize]
            && !name.is_empty()
        {
            let offset = self.span_offset(name);
            let args_id = Self::field_node_id(fields, args_idx);
            let arg_count = args_id
                .and_then(|id| stmt.list_children(id))
                .map_or(0, <[_]>::len);
            match self.catalog.check_function(name, arg_count) {
                FunctionCheckResult::Ok => {
                    if let Some((cat, arities)) = self.catalog.function_signature(name) {
                        let cat_str = match cat {
                            FunctionCategory::Scalar => "scalar function",
                            FunctionCategory::Aggregate => "aggregate function",
                            FunctionCategory::Window => "window function",
                        };
                        let arity_strs: Vec<String> =
                            arities.iter().map(|a| format_arity(name, *a)).collect();
                        self.resolutions.push(Resolution {
                            start: offset,
                            end: offset + name.len(),
                            symbol: ResolvedSymbol::Function {
                                category: cat_str.to_string(),
                                arities: arity_strs,
                            },
                        });
                    }
                }
                FunctionCheckResult::Unknown => {
                    let candidates = self.catalog.all_function_names();
                    let suggestion =
                        best_suggestion(name, &candidates, self.config.suggestion_threshold());
                    self.emit(
                        offset,
                        offset + name.len(),
                        DiagnosticMessage::UnknownFunction {
                            name: name.to_string(),
                        },
                        suggestion.map(Help::Suggestion),
                    );
                }
                FunctionCheckResult::WrongArity { expected } => {
                    self.emit(
                        offset,
                        offset + name.len(),
                        DiagnosticMessage::FunctionArity {
                            name: name.to_string(),
                            expected,
                            got: arg_count,
                        },
                        None,
                    );
                }
            }
        }
        self.visit_children(stmt, node_id);
    }

    fn visit_column_ref(&mut self, fields: &NodeFields<'a>, column_idx: u8, table_idx: u8) {
        // ColumnRef outside any query scope (e.g. ATTACH ... AS scratch)
        // is just a bare identifier — skip validation.
        if !self.scope.has_frames() {
            return;
        }
        let FieldValue::Span { text: column, .. } = fields[column_idx as usize] else {
            return;
        };
        if column.is_empty() {
            return;
        }
        let table = match fields[table_idx as usize] {
            FieldValue::Span { text: s, .. } if !s.is_empty() => Some(s),
            _ => None,
        };
        let offset = self.span_offset(column);

        match self.scope.resolve_column(table, column) {
            ColumnResolution::Found {
                table: resolved_table,
                all_columns,
            } => {
                if !resolved_table.is_empty() {
                    let def_key = format!(
                        "{}.{}",
                        resolved_table.to_ascii_lowercase(),
                        column.to_ascii_lowercase()
                    );
                    let definition = self
                        .definition_offsets
                        .get(&def_key)
                        .map(|&(start, end)| DefinitionLocation {
                            start,
                            end,
                            file_uri: None,
                        })
                        .or_else(|| {
                            self.catalog
                                .column_definition_site(&resolved_table, column)
                                .map(|site| DefinitionLocation {
                                    start: site.start,
                                    end: site.end,
                                    file_uri: Some(site.file_uri.clone()),
                                })
                        });
                    self.resolutions.push(Resolution {
                        start: offset,
                        end: offset + column.len(),
                        symbol: ResolvedSymbol::Column {
                            column: column.to_string(),
                            table: resolved_table,
                            all_columns,
                            definition,
                        },
                    });
                }
            }
            ColumnResolution::TableNotFound => {}
            ColumnResolution::TableFoundColumnMissing => {
                let tbl = table.expect("qualifier present when TableFoundColumnMissing");
                let candidates = self.scope.all_column_names(Some(tbl));
                let suggestion =
                    best_suggestion(column, &candidates, self.config.suggestion_threshold());
                self.emit(
                    offset,
                    offset + column.len(),
                    DiagnosticMessage::UnknownColumn {
                        column: column.to_string(),
                        table: Some(tbl.to_string()),
                    },
                    suggestion.map(Help::Suggestion),
                );
            }
            ColumnResolution::NotFound => {
                // SQLite resolves bare TRUE/FALSE identifiers to integer
                // literals 1/0 (see sqlite3ExprIdToTrueFalse), so they are
                // valid even when no column by that name exists.
                if column.eq_ignore_ascii_case("true") || column.eq_ignore_ascii_case("false") {
                    return;
                }
                let candidates = self.scope.all_column_names(None);
                let suggestion =
                    best_suggestion(column, &candidates, self.config.suggestion_threshold());
                self.emit(
                    offset,
                    offset + column.len(),
                    DiagnosticMessage::UnknownColumn {
                        column: column.to_string(),
                        table: None,
                    },
                    suggestion.map(Help::Suggestion),
                );
            }
        }
    }

    fn visit_scoped_source(
        &mut self,
        stmt: &AnyParsedStatement<'a>,
        fields: &NodeFields<'a>,
        body_idx: u8,
        alias_idx: u8,
    ) {
        self.scope.push();
        self.visit_opt(stmt, Self::field_node_id(fields, body_idx));
        self.scope.pop();

        let alias = self.name_text(stmt, Self::field_node_id(fields, alias_idx));
        let cols = Self::field_node_id(fields, body_idx)
            .and_then(|id| columns_from_select(stmt, id, self.roles));
        if alias.is_empty() {
            self.scope.add_anonymous(cols);
        } else {
            self.scope.add_table(alias, cols, RowIdPolicy::WithRowId);
        }
    }

    #[expect(clippy::too_many_arguments)]
    fn visit_query(
        &mut self,
        stmt: &AnyParsedStatement<'a>,
        fields: &NodeFields<'a>,
        from: u8,
        columns: u8,
        where_clause: u8,
        groupby: u8,
        having: u8,
        orderby: u8,
        limit_clause: u8,
    ) {
        // Push a fresh scope so that tables registered by visit_source_ref
        // (via add_query_table) are visible when we visit SELECT columns,
        // WHERE, ORDER BY, etc.  Without this, add_query_table is a silent
        // no-op when no query scope frame exists (e.g. at the top level),
        // causing column refs against unknown tables to be spuriously flagged.
        self.scope.push();
        self.visit_opt(stmt, Self::field_node_id(fields, from));
        self.visit_opt(stmt, Self::field_node_id(fields, columns));

        // Collect SELECT aliases so they are visible in WHERE, GROUP BY,
        // HAVING, ORDER BY, and LIMIT — matching SQLite's resolution rules.
        let aliases = self.collect_select_aliases(stmt, fields, columns);
        if !aliases.is_empty() {
            self.scope
                .add_table("", Some(aliases), RowIdPolicy::WithRowId);
        }

        for idx in [where_clause, groupby, having, orderby, limit_clause] {
            self.visit_opt(stmt, Self::field_node_id(fields, idx));
        }
        self.scope.pop();
    }

    /// Extract alias names from the SELECT result column list.
    fn collect_select_aliases(
        &self,
        stmt: &AnyParsedStatement<'a>,
        fields: &NodeFields<'a>,
        columns_idx: u8,
    ) -> Vec<String> {
        let mut aliases = Vec::new();
        let Some(list_id) = Self::field_node_id(fields, columns_idx) else {
            return aliases;
        };
        let Some(children) = stmt.list_children(list_id) else {
            return aliases;
        };
        for &child_id in children {
            if child_id.is_null() {
                continue;
            }
            let Some((child_tag, child_fields)) = stmt.extract_fields(child_id) else {
                continue;
            };
            let child_role = self
                .roles
                .get(u32::from(child_tag) as usize)
                .copied()
                .unwrap_or(SemanticRole::Transparent);
            let SemanticRole::ResultColumn {
                alias: alias_idx, ..
            } = child_role
            else {
                continue;
            };
            let alias_node = Self::field_node_id(&child_fields, alias_idx);
            let alias_text = self.name_text(stmt, alias_node);
            if !alias_text.is_empty() {
                aliases.push(alias_text.to_string());
            }
        }
        aliases
    }

    fn visit_cte_scope(
        &mut self,
        stmt: &AnyParsedStatement<'a>,
        fields: &NodeFields<'a>,
        recursive_idx: u8,
        bindings_idx: u8,
        body_idx: u8,
    ) {
        let is_recursive = matches!(fields[recursive_idx as usize], FieldValue::Bool(true));
        let cte_ids = Self::field_node_id(fields, bindings_idx)
            .and_then(|id| stmt.list_children(id))
            .unwrap_or(&[]);

        // Push a catalog scope so CTE names are resolvable as table names in
        // FROM clauses.  This is purely for relation-name resolution — CTE
        // columns only become active when a CTE is actually referenced in FROM
        // (handled by visit_source_ref → scope.add_table).
        self.catalog.push_query_scope();

        for &cte_id in cte_ids {
            let Some(binding) = self.extract_cte_binding(stmt, cte_id) else {
                continue;
            };

            // For recursive CTEs, register the name before visiting the body.
            if is_recursive && !binding.name.is_empty() {
                let cols = binding
                    .declared_cols
                    .as_ref()
                    .map(|v| v.iter().map(ToString::to_string).collect());
                self.catalog.add_query_table(binding.name, cols);
            }

            self.scope.push();
            self.visit_opt(stmt, binding.body_id);
            self.scope.pop();

            if binding.name.is_empty() {
                continue;
            }

            // Record CTE definition offset for go-to-definition.
            let name_offset = self.span_offset(binding.name);
            self.definition_offsets.insert(
                binding.name.to_ascii_lowercase(),
                (name_offset, name_offset + binding.name.len()),
            );

            // Determine the CTE's column list and register it in the catalog.
            let cte_key = binding.name.to_ascii_lowercase();
            let cols = if let Some(ref declared) = binding.declared_cols {
                self.check_cte_column_count(stmt, binding.name, declared, binding.body_id);
                // Record declared column definition offsets.
                for col_name in declared {
                    let col_offset = self.span_offset(col_name);
                    let key = format!("{cte_key}.{}", col_name.to_ascii_lowercase());
                    self.definition_offsets
                        .insert(key, (col_offset, col_offset + col_name.len()));
                }
                Some(declared.iter().map(ToString::to_string).collect())
            } else {
                // Record inferred column definition offsets from SELECT aliases.
                self.record_select_column_offsets(stmt, binding.body_id, &cte_key);
                binding
                    .body_id
                    .and_then(|id| columns_from_select(stmt, id, self.roles))
            };
            self.catalog.add_query_table(binding.name, cols);
        }

        self.visit_opt(stmt, Self::field_node_id(fields, body_idx));
        self.catalog.pop_query_scope();
    }

    /// Record definition offsets for columns inferred from a SELECT body.
    ///
    /// For `WITH foo AS (SELECT 1 AS a, 2 AS b)`, records offsets for the
    /// alias names `a` and `b` so go-to-definition can jump to them.
    fn record_select_column_offsets(
        &mut self,
        stmt: &AnyParsedStatement<'a>,
        body_id: Option<AnyNodeId>,
        table_key: &str,
    ) {
        let Some(body_id) = body_id else { return };
        let Some((tag, fields)) = stmt.extract_fields(body_id) else {
            return;
        };
        let Some(&SemanticRole::Query {
            columns: cols_idx, ..
        }) = self.roles.get(u32::from(tag) as usize)
        else {
            return;
        };
        let Some(list_id) = Self::field_node_id(&fields, cols_idx) else {
            return;
        };
        let Some(children) = stmt.list_children(list_id) else {
            return;
        };

        for &child_id in children {
            if child_id.is_null() {
                continue;
            }
            let Some((child_tag, child_fields)) = stmt.extract_fields(child_id) else {
                continue;
            };
            let child_role = self
                .roles
                .get(u32::from(child_tag) as usize)
                .copied()
                .unwrap_or(SemanticRole::Transparent);
            let SemanticRole::ResultColumn {
                alias: alias_idx, ..
            } = child_role
            else {
                continue;
            };
            let alias_node = Self::field_node_id(&child_fields, alias_idx);
            let alias_text = self.name_text(stmt, alias_node);
            if !alias_text.is_empty() {
                let off = self.span_offset(alias_text);
                let key = format!("{table_key}.{}", alias_text.to_ascii_lowercase());
                self.definition_offsets
                    .insert(key, (off, off + alias_text.len()));
            }
        }
    }

    /// Extract CTE binding info from a node, or `None` if it's not a CTE.
    fn extract_cte_binding(
        &self,
        stmt: &AnyParsedStatement<'a>,
        cte_id: AnyNodeId,
    ) -> Option<CteBindingInfo<'a>> {
        if cte_id.is_null() {
            return None;
        }
        let (tag, fields) = stmt.extract_fields(cte_id)?;
        let role = self
            .roles
            .get(u32::from(tag) as usize)
            .copied()
            .unwrap_or(SemanticRole::Transparent);
        let SemanticRole::CteBinding {
            name: name_idx,
            columns: cols_idx,
            body: body_idx,
        } = role
        else {
            return None;
        };

        let name = match fields[name_idx as usize] {
            FieldValue::Span { text: s, .. } => s,
            _ => "",
        };
        let body_id = Self::field_node_id(&fields, body_idx);
        let declared_cols = self.extract_declared_cols(stmt, &fields, cols_idx);
        Some(CteBindingInfo {
            name,
            body_id,
            declared_cols,
        })
    }

    /// Extract declared CTE column names from the column list field.
    fn extract_declared_cols(
        &self,
        stmt: &AnyParsedStatement<'a>,
        fields: &NodeFields<'a>,
        cols_idx: u8,
    ) -> Option<Vec<&'a str>> {
        if cols_idx == FIELD_ABSENT {
            return None;
        }
        let list_id = Self::field_node_id(fields, cols_idx)?;
        let children = stmt.list_children(list_id)?;
        let names: Vec<&'a str> = children
            .iter()
            .copied()
            .filter(|id| !id.is_null())
            .map(|id| self.name_text(stmt, Some(id)))
            .filter(|s| !s.is_empty())
            .collect();
        if names.is_empty() { None } else { Some(names) }
    }

    /// Emit a diagnostic if the CTE body has a different column count than declared.
    fn check_cte_column_count(
        &mut self,
        stmt: &AnyParsedStatement<'a>,
        cte_name: &str,
        declared: &[&str],
        body_id: Option<AnyNodeId>,
    ) {
        if let Some(actual) = self.count_result_columns(stmt, body_id)
            && actual != declared.len()
        {
            let offset = self.span_offset(cte_name);
            self.emit(
                offset,
                offset + cte_name.len(),
                DiagnosticMessage::CteColumnCountMismatch {
                    name: cte_name.to_string(),
                    declared: declared.len(),
                    actual,
                },
                None,
            );
        }
    }

    /// Count the result columns of a SELECT body node.
    ///
    /// Returns `None` if the body is not a plain `SelectStmt` or if any result
    /// column uses `*` (wildcard), which would require catalog expansion to count.
    fn count_result_columns(
        &self,
        stmt: &AnyParsedStatement<'a>,
        body_id: Option<AnyNodeId>,
    ) -> Option<usize> {
        let body_id = body_id?;
        let (body_tag, body_fields) = stmt.extract_fields(body_id)?;
        let body_role = self
            .roles
            .get(u32::from(body_tag) as usize)
            .copied()
            .unwrap_or(SemanticRole::Transparent);

        // Only handle direct SelectStmt (Query role); skip compound, VALUES, etc.
        let SemanticRole::Query {
            columns: cols_idx, ..
        } = body_role
        else {
            return None;
        };

        let list_id = Self::field_node_id(&body_fields, cols_idx)?;
        let children = stmt.list_children(list_id)?;

        let mut count = 0usize;
        for child_id in children.iter().copied() {
            if child_id.is_null() {
                continue;
            }
            let Some((child_tag, child_fields)) = stmt.extract_fields(child_id) else {
                continue;
            };
            let child_role = self
                .roles
                .get(u32::from(child_tag) as usize)
                .copied()
                .unwrap_or(SemanticRole::Transparent);
            let SemanticRole::ResultColumn {
                flags: flags_idx, ..
            } = child_role
            else {
                continue;
            };
            // STAR flag (bit 0) means wildcard — skip count check entirely.
            if let FieldValue::Flags(f) = child_fields[flags_idx as usize]
                && f & 1 != 0
            {
                return None;
            }
            count += 1;
        }
        Some(count)
    }

    fn visit_trigger_scope(
        &mut self,
        stmt: &AnyParsedStatement<'a>,
        fields: &NodeFields<'a>,
        when_idx: u8,
        body_idx: u8,
    ) {
        self.scope.push();
        self.scope.add_table("OLD", None, RowIdPolicy::WithRowId);
        self.scope.add_table("NEW", None, RowIdPolicy::WithRowId);
        self.visit_opt(stmt, Self::field_node_id(fields, when_idx));
        self.visit_opt(stmt, Self::field_node_id(fields, body_idx));
        self.scope.pop();
    }
}

// ── Tests ──────────────────────────────────────────────────────────────────────

#[cfg(test)]
mod tests {
    use super::super::catalog::{
        AritySpec, CatalogLayer, ColumnResolution, FunctionCategory, FunctionCheckResult,
    };
    use super::super::diagnostics::{DiagnosticMessage, Help, Severity};
    use super::super::render::DiagnosticRenderer;
    use super::*;

    fn sqlite_analyzer() -> SemanticAnalyzer {
        SemanticAnalyzer::new()
    }

    fn sqlite_catalog() -> Catalog {
        Catalog::new(crate::sqlite::dialect::dialect())
    }

    fn strict() -> ValidationConfig {
        ValidationConfig::default().with_strict_schema()
    }

    fn lenient() -> ValidationConfig {
        ValidationConfig::default()
    }

    // ── Catalog ────────────────────────────────────────────────────────────────

    #[test]
    fn catalog_add_table_and_resolve() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".to_string(), "name".to_string()]),
            false,
        );
        assert!(cat.resolve_relation("users"));
        assert!(cat.resolve_relation("USERS")); // case-insensitive
        assert!(!cat.resolve_relation("orders"));
    }

    #[test]
    fn catalog_add_view_and_resolve() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database)
            .insert_view("active_users", Some(vec!["id".to_string()]));
        assert!(cat.resolve_relation("active_users"));
    }

    #[test]
    fn catalog_add_function_and_check() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database)
            .insert_function_overload("my_func", FunctionCategory::Scalar, AritySpec::Exact(2));
        assert!(matches!(
            cat.check_function("my_func", 2),
            FunctionCheckResult::Ok
        ));
        assert!(matches!(
            cat.check_function("my_func", 1),
            FunctionCheckResult::WrongArity { .. }
        ));
    }

    #[test]
    fn catalog_add_variadic_function() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database)
            .insert_function_overload("variadic_fn", FunctionCategory::Scalar, AritySpec::Any);
        assert!(matches!(
            cat.check_function("variadic_fn", 0),
            FunctionCheckResult::Ok
        ));
        assert!(matches!(
            cat.check_function("variadic_fn", 100),
            FunctionCheckResult::Ok
        ));
    }

    #[test]
    fn catalog_builtin_functions_resolved() {
        let cat = sqlite_catalog();
        // SQLite has built-in functions like abs(), coalesce(), etc.
        assert!(!matches!(
            cat.check_function("abs", 1),
            FunctionCheckResult::Unknown
        ));
        assert!(!matches!(
            cat.check_function("coalesce", 2),
            FunctionCheckResult::Unknown
        ));
    }

    #[test]
    fn catalog_from_ddl_populates_tables() {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(
            dialect,
            &[("CREATE TABLE users (id INTEGER, name TEXT);", None)],
        )
        .0;
        assert!(cat.resolve_relation("users"));
    }

    #[test]
    fn catalog_from_ddl_populates_virtual_tables() {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(
            dialect,
            &[("CREATE VIRTUAL TABLE fts USING fts5(content);", None)],
        )
        .0;
        assert!(cat.resolve_relation("fts"));
    }

    #[test]
    fn virtual_table_in_from_clause_no_error() {
        // Virtual tables are known to exist; column refs are conservatively
        // accepted because the column list is not statically known.
        let source = "CREATE VIRTUAL TABLE fts USING fts5(content);\nSELECT * FROM fts;";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(source, &cat, &strict());
        let unknown_tables: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(d.message, DiagnosticMessage::UnknownTable { .. }))
            .collect();
        assert!(
            unknown_tables.is_empty(),
            "virtual table should be known: {unknown_tables:?}"
        );
    }

    #[test]
    fn unknown_columns_table_blocks_outer_scope_leaking() {
        // If the inner scope has a table with unknown columns (None), unqualified
        // column resolution must NOT leak through to a matching column in an outer
        // scope.  This prevents CTE columns from polluting sibling/outer queries.
        let mut scope = QueryScope::default();
        // Outer scope: simulates a correlated outer query with known columns.
        scope.push();
        scope.add_table(
            "a",
            Some(vec!["id".into(), "name".into()]),
            RowIdPolicy::WithRowId,
        );
        // Inner scope: "users" with unknown columns.
        scope.push();
        scope.add_table("users", None, RowIdPolicy::WithRowId);

        // Resolving "name" should NOT return table="a".
        let res = scope.resolve_column(None, "name");
        match res {
            ColumnResolution::Found { ref table, .. } => {
                assert_ne!(
                    table, "a",
                    "column 'name' should NOT resolve to outer scope's 'a'; \
                     inner scope has unknown-columns table that should block leaking"
                );
            }
            _ => panic!("expected Found, got {res:?}"),
        }

        scope.pop();
        scope.pop();
    }

    #[test]
    fn anonymous_source_columns_resolve_unqualified() {
        // Anonymous sources (unaliased subqueries) should participate in
        // unqualified column resolution.
        let mut scope = QueryScope::default();
        scope.push();
        scope.add_anonymous(Some(vec!["x".into(), "y".into()]));

        let res = scope.resolve_column(None, "x");
        assert!(
            matches!(res, ColumnResolution::Found { .. }),
            "anonymous source column 'x' should resolve: {res:?}"
        );

        // Unknown column should NOT resolve.
        let res = scope.resolve_column(None, "missing");
        assert!(
            matches!(res, ColumnResolution::NotFound),
            "column 'missing' should not resolve: {res:?}"
        );

        scope.pop();
    }

    #[test]
    fn anonymous_source_unknown_columns_blocks_leaking() {
        // An anonymous source with None columns (compound subquery) should
        // accept any column and block leaking to outer scopes.
        let mut scope = QueryScope::default();
        scope.push();
        scope.add_table("outer_tbl", Some(vec!["id".into()]), RowIdPolicy::WithRowId);
        scope.push();
        scope.add_anonymous(None); // unknown columns

        let res = scope.resolve_column(None, "anything");
        match res {
            ColumnResolution::Found { ref table, .. } => {
                assert_ne!(
                    table, "outer_tbl",
                    "should not leak to outer scope through anonymous unknown-columns source"
                );
            }
            _ => panic!("expected Found, got {res:?}"),
        }

        scope.pop();
        scope.pop();
    }

    #[test]
    fn anonymous_source_not_in_qualified_lookup() {
        // Anonymous sources should NOT be found by qualified (table.column) lookup.
        let mut scope = QueryScope::default();
        scope.push();
        scope.add_anonymous(Some(vec!["x".into()]));

        let res = scope.resolve_column(Some("sq"), "x");
        assert!(
            matches!(res, ColumnResolution::TableNotFound),
            "anonymous source should not match qualified lookup: {res:?}"
        );

        scope.pop();
    }

    #[test]
    fn without_rowid_table_rejects_implicit_rowid() {
        // A WITHOUT ROWID table should not accept rowid/oid/_rowid_ as columns.
        let mut az = sqlite_analyzer();
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database).insert_table(
            "kv",
            Some(vec!["key".to_string(), "value".to_string()]),
            true, // WITHOUT ROWID
        );

        let model = az.analyze("SELECT rowid FROM kv", &cat, &strict());
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            !col_errs.is_empty(),
            "rowid should be rejected for WITHOUT ROWID tables"
        );
    }

    #[test]
    fn regular_table_accepts_implicit_rowid() {
        // A normal table should accept rowid as an implicit column.
        let mut az = sqlite_analyzer();
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".to_string(), "name".to_string()]),
            false,
        );

        let model = az.analyze("SELECT rowid FROM users", &cat, &strict());
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "rowid should be accepted for regular tables: {col_errs:?}"
        );
    }

    #[test]
    fn catalog_clear_database() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database).insert_table(
            "tmp",
            Some(vec!["id".to_string()]),
            false,
        );
        assert!(cat.resolve_relation("tmp"));
        cat.new_database();
        assert!(!cat.resolve_relation("tmp"));
    }

    // ── Analyzer: no-error cases ───────────────────────────────────────────────

    #[test]
    fn analyze_select_from_known_table_no_errors() {
        let mut az = sqlite_analyzer();
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".to_string(), "name".to_string()]),
            false,
        );

        let model = az.analyze("SELECT id FROM users", &cat, &strict());
        let diags: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(d.message, DiagnosticMessage::UnknownTable { .. }))
            .collect();
        assert!(diags.is_empty(), "unexpected table error: {diags:?}");
    }

    #[test]
    fn analyze_empty_source_no_errors() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze("", &cat, &strict());
        assert!(model.diagnostics().is_empty());
    }

    #[test]
    fn analyze_pragma_no_errors() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze("PRAGMA journal_mode;", &cat, &strict());
        let sem_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| !d.message.is_parse_error())
            .collect();
        assert!(sem_errs.is_empty());
    }

    // ── Analyzer: unknown table / column ──────────────────────────────────────

    #[test]
    fn analyze_unknown_table_strict_is_error() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze("SELECT * FROM missing_table", &cat, &strict());
        let errs: Vec<_> = model.diagnostics().iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownTable { name } if name == "missing_table"))
            .collect();
        assert_eq!(errs.len(), 1);
        assert_eq!(errs[0].severity, Severity::Error);
    }

    #[test]
    fn analyze_unknown_table_lenient_is_warning() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze("SELECT * FROM missing_table", &cat, &lenient());
        let warns: Vec<_> = model.diagnostics().iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownTable { name } if name == "missing_table"))
            .collect();
        assert_eq!(warns.len(), 1);
        assert_eq!(warns[0].severity, Severity::Warning);
    }

    #[test]
    fn analyze_fuzzy_suggestion_for_unknown_table() {
        let mut az = sqlite_analyzer();
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".to_string()]),
            false,
        );
        let model = az.analyze("SELECT * FROM usres", &cat, &strict()); // typo
        let diag = model.diagnostics().iter().find(
            |d| matches!(&d.message, DiagnosticMessage::UnknownTable { name } if name == "usres"),
        );
        assert!(diag.is_some(), "expected unknown-table diagnostic");
        let diag = diag.unwrap();
        assert!(
            matches!(&diag.help, Some(Help::Suggestion(s)) if s == "users"),
            "expected 'users' suggestion, got {:?}",
            diag.help
        );
    }

    // ── Analyzer: DDL accumulation ─────────────────────────────────────────────

    #[test]
    fn analyze_create_table_then_select_no_error() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let src = "CREATE TABLE t (id INTEGER); SELECT id FROM t;";
        let model = az.analyze(src, &cat, &strict());
        let unknown: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownTable { .. }))
            .collect();
        assert!(
            unknown.is_empty(),
            "DDL-defined table not visible: {unknown:?}"
        );
    }

    #[test]
    fn analyze_create_view_then_select_no_error() {
        let mut az = sqlite_analyzer();
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".to_string()]),
            false,
        );
        let src = "CREATE VIEW vw AS SELECT id FROM users; SELECT id FROM vw;";
        let model = az.analyze(src, &cat, &strict());
        let unknown: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownTable { .. }))
            .collect();
        assert!(unknown.is_empty(), "VIEW not visible: {unknown:?}");
    }

    // ── Analyzer: DDL column extraction (role-based) ───────────────────────────

    #[test]
    fn ddl_columns_visible_to_column_ref() {
        // Verifies columns_from_column_list uses the ColumnDef role to extract
        // names precisely, and those names are visible to column-ref validation.
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let src = "CREATE TABLE t (id INTEGER, name TEXT); SELECT id FROM t;";
        let model = az.analyze(src, &cat, &strict());
        assert!(
            model.diagnostics().is_empty(),
            "id is a known column of t: {:?}",
            model.diagnostics()
        );
    }

    #[test]
    fn ddl_unknown_column_on_ddl_table_flagged() {
        // Verifies that column names extracted from DDL are used for validation:
        // a column not in the DDL definition is correctly flagged.
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let src = "CREATE TABLE t (id INTEGER); SELECT missing FROM t;";
        let model = az.analyze(src, &cat, &strict());
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert_eq!(
            errs.len(),
            1,
            "unknown column should be flagged: {:?}",
            model.diagnostics()
        );
    }

    // ── Analyzer: correlated subqueries ─────────────────────────────────────────

    #[test]
    fn correlated_subquery_outer_column_resolves() {
        // `name` only exists in `users` (outer). SQLite resolves it as a
        // correlated reference — no error expected.
        let src = "\
            CREATE TABLE users (id INTEGER, name TEXT);\
            CREATE TABLE orders (id INTEGER, user_id INTEGER);\
            SELECT * FROM users WHERE EXISTS (\
                SELECT 1 FROM orders WHERE name = 'Alice'\
            );";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &strict());
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "outer column 'name' should resolve via correlated reference: {col_errs:?}"
        );
    }

    #[test]
    fn correlated_subquery_inner_shadows_outer() {
        // Both tables have `id`. In the subquery, unqualified `id` should
        // resolve to `orders.id` (inner wins) — no error expected.
        let src = "\
            CREATE TABLE users (id INTEGER, name TEXT);\
            CREATE TABLE orders (id INTEGER, user_id INTEGER);\
            SELECT * FROM users WHERE EXISTS (\
                SELECT 1 FROM orders WHERE id = 1\
            );";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &strict());
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "inner 'id' should shadow outer — no error: {col_errs:?}"
        );
    }

    #[test]
    fn correlated_subquery_column_in_neither_scope_flagged() {
        // `bogus` doesn't exist in either table — should be flagged.
        let src = "\
            CREATE TABLE users (id INTEGER, name TEXT);\
            CREATE TABLE orders (id INTEGER, user_id INTEGER);\
            SELECT * FROM users WHERE EXISTS (\
                SELECT 1 FROM orders WHERE bogus = 1\
            );";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &strict());
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { column, .. } if column == "bogus"))
            .collect();
        assert_eq!(
            col_errs.len(),
            1,
            "column 'bogus' should be flagged as unknown: {:?}",
            model.diagnostics()
        );
    }

    // ── Analyzer: unaliased subqueries ─────────────────────────────────────────

    #[test]
    fn unaliased_subquery_columns_visible() {
        // Columns from an unaliased subquery should be resolvable in the outer
        // query — this was the main regression from the QueryScope refactor.
        let src = "\
            CREATE TABLE t1(a INTEGER, b TEXT);\
            SELECT a FROM (SELECT a, b FROM t1);";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &strict());
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "columns from unaliased subquery should be visible: {col_errs:?}"
        );
    }

    #[test]
    fn unaliased_subquery_rejects_missing_column() {
        // A column not in the subquery's SELECT list should still be flagged.
        let src = "\
            CREATE TABLE t1(a INTEGER, b TEXT);\
            SELECT missing FROM (SELECT a FROM t1);";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &strict());
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { column, .. } if column == "missing"))
            .collect();
        assert_eq!(
            col_errs.len(),
            1,
            "'missing' should be flagged as unknown: {:?}",
            model.diagnostics()
        );
    }

    #[test]
    fn unaliased_compound_subquery_accepts_columns() {
        // UNION/UNION ALL subqueries can't have columns inferred (returns None),
        // so all column references should be accepted conservatively.
        let src = "\
            CREATE TABLE t1(a INTEGER);\
            SELECT x FROM (SELECT a AS x FROM t1 UNION ALL SELECT a FROM t1);";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &strict());
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "compound subquery columns should be accepted: {col_errs:?}"
        );
    }

    #[test]
    fn aliased_subquery_columns_visible() {
        // Named subquery with inferred columns — column names should be known.
        let src = "\
            CREATE TABLE t1(a INTEGER, b TEXT);\
            SELECT sq.a FROM (SELECT a, b FROM t1) AS sq;";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &strict());
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "aliased subquery columns should be visible: {col_errs:?}"
        );
    }

    #[test]
    fn unaliased_subquery_in_join_columns_visible() {
        // Columns from an unaliased subquery in a JOIN should be resolvable.
        let src = "\
            CREATE TABLE t1(a INTEGER);\
            CREATE TABLE t2(x INTEGER);\
            SELECT a, x FROM t1 LEFT JOIN (SELECT x FROM t2) ON a = x;";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &strict());
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "unaliased subquery columns in JOIN should be visible: {col_errs:?}"
        );
    }

    // ── Analyzer: go-to-definition ──────────────────────────────────────────────

    #[test]
    fn definition_cte_reference_jumps_to_cte_name() {
        let src = "WITH cte AS (SELECT 1) SELECT * FROM cte";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &lenient());

        // Click on "cte" in FROM.
        let ref_offset = src.rfind("cte").unwrap();
        let def = model.definition_at(ref_offset);
        assert!(def.is_some(), "expected definition for CTE reference");
        let def = def.unwrap();
        let cte_def_offset = src.find("cte").unwrap();
        assert_eq!(
            def.target.start, cte_def_offset,
            "definition should point to CTE name"
        );
        assert_eq!(def.target.end, cte_def_offset + "cte".len());
    }

    #[test]
    fn definition_ddl_table_reference_jumps_to_create() {
        let src = "CREATE TABLE users (id INTEGER); SELECT id FROM users;";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &strict());

        // Click on "users" in FROM clause.
        let ref_offset = src.rfind("users").unwrap();
        let def = model.definition_at(ref_offset);
        assert!(
            def.is_some(),
            "expected definition for DDL table reference; resolutions: {:?}",
            model.resolutions
        );
        let def = def.unwrap();
        let ddl_offset = src.find("users").unwrap();
        assert_eq!(
            def.target.start, ddl_offset,
            "definition should point to CREATE TABLE name"
        );
        assert_eq!(def.target.end, ddl_offset + "users".len());
    }

    #[test]
    fn definition_cte_shadows_ddl() {
        let src = "CREATE TABLE t (id INTEGER); WITH t AS (SELECT 1 AS id) SELECT * FROM t;";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &strict());

        // Find "t" in FROM — should point to CTE definition, not CREATE TABLE.
        // "FROM t" — find the offset of the last "t"
        let from_t_offset = src.rfind("FROM t").unwrap() + 5; // offset of "t" after FROM
        let def = model.definition_at(from_t_offset);
        assert!(
            def.is_some(),
            "expected definition for CTE-shadowed reference"
        );
        let def = def.unwrap();
        // CTE "t" starts at "WITH t" — offset 29
        let cte_t_offset = src[29..].find('t').unwrap() + 29;
        assert_eq!(
            def.target.start, cte_t_offset,
            "definition should point to CTE, not DDL"
        );
    }

    #[test]
    fn definition_unknown_table_returns_none() {
        let src = "SELECT * FROM nonexistent";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &lenient());

        let from_offset = src.find("nonexistent").unwrap();
        let def = model.definition_at(from_offset);
        assert!(def.is_none(), "unknown table should have no definition");
    }

    // ── Go-to-definition: columns ──────────────────────────────────────────────

    /// Go-to-definition on a column ref should jump to the column in CREATE TABLE.
    #[test]
    fn definition_column_in_ddl_table() {
        let src = "CREATE TABLE users (id INTEGER, name TEXT);\nSELECT name FROM users;";
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(
            dialect,
            &[("CREATE TABLE users (id INTEGER, name TEXT);", None)],
        )
        .0;
        let mut az = sqlite_analyzer();
        let model = az.analyze(src, &cat, &strict());

        // "name" in "SELECT name" should point to "name" in the CREATE TABLE
        let select_name_offset = src.rfind("name").unwrap();
        let def = model.definition_at(select_name_offset);
        assert!(
            def.is_some(),
            "column 'name' should have a definition location"
        );
        let def = def.unwrap();
        // The definition should point to "name" in the CREATE TABLE column list
        let ddl_name_offset = src.find("name").unwrap();
        assert_eq!(
            def.target.start, ddl_name_offset,
            "definition should point to column in DDL, not SELECT"
        );
    }

    /// Go-to-definition on a column that doesn't exist should return None.
    #[test]
    fn definition_unknown_column_returns_none() {
        let src = "CREATE TABLE t (a INT);\nSELECT b FROM t;";
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(dialect, &[("CREATE TABLE t (a INT);", None)]).0;
        let mut az = sqlite_analyzer();
        let model = az.analyze(src, &cat, &strict());

        let b_offset = src.find('b').unwrap();
        let def = model.definition_at(b_offset);
        assert!(def.is_none(), "unknown column should have no definition");
    }

    // ── Go-to-definition: CTE columns ─────────────────────────────────────────

    /// Go-to-definition on a CTE column (inferred from alias) should jump to the alias.
    #[test]
    fn definition_cte_column_inferred_from_alias() {
        let src = "WITH foo AS (SELECT 1 AS a)\nSELECT a FROM foo;";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &lenient());

        // Click on "a" in "SELECT a FROM foo"
        let a_offset = src.find("SELECT a").unwrap() + "SELECT ".len();
        let def = model.definition_at(a_offset);
        assert!(
            def.is_some(),
            "CTE column 'a' should have a definition location"
        );
        let def = def.unwrap();
        // Should point to "a" in "1 AS a" inside the CTE
        let cte_a_offset = src.find("AS a").unwrap() + "AS ".len();
        assert_eq!(
            def.target.start, cte_a_offset,
            "definition should point to alias in CTE body"
        );
    }

    /// Go-to-definition on a CTE column (declared column list).
    #[test]
    fn definition_cte_column_from_declared_list() {
        let src = "WITH foo(x) AS (SELECT 1)\nSELECT x FROM foo;";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &lenient());

        let x_offset = src.find("SELECT x").unwrap() + "SELECT ".len();
        let def = model.definition_at(x_offset);
        assert!(
            def.is_some(),
            "CTE declared column 'x' should have a definition location"
        );
        let def = def.unwrap();
        // Should point to "x" in "foo(x)"
        let decl_x_offset = src.find("(x)").unwrap() + 1;
        assert_eq!(
            def.target.start, decl_x_offset,
            "definition should point to declared column in CTE"
        );
    }

    // ── Go-to-definition: cross-file schema ─────────────────────────────────────

    #[test]
    fn definition_schema_table_jumps_to_external_file() {
        let schema = "CREATE TABLE users (id INTEGER, name TEXT);";
        let file_uri = "file:///path/to/schema.sql";
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(dialect, &[(schema, Some(file_uri))]).0;

        let src = "SELECT * FROM users";
        let mut az = sqlite_analyzer();
        let model = az.analyze(src, &cat, &lenient());

        let ref_offset = src.find("users").unwrap();
        let def = model.definition_at(ref_offset);
        assert!(def.is_some(), "expected definition for schema table");
        let def = def.unwrap();
        assert_eq!(def.target.file_uri.as_deref(), Some(file_uri));
        let schema_offset = schema.find("users").unwrap();
        assert_eq!(def.target.start, schema_offset);
        assert_eq!(def.target.end, schema_offset + "users".len());
    }

    #[test]
    fn definition_same_file_ddl_shadows_schema() {
        // Same-file CREATE TABLE should win over external schema.
        let schema = "CREATE TABLE t (x INTEGER);";
        let file_uri = "file:///schema.sql";
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(dialect, &[(schema, Some(file_uri))]).0;

        let src = "CREATE TABLE t (y INTEGER); SELECT * FROM t;";
        let mut az = sqlite_analyzer();
        let model = az.analyze(src, &cat, &strict());

        let ref_offset = src.rfind(" t").unwrap() + 1;
        let def = model.definition_at(ref_offset);
        assert!(def.is_some(), "expected definition");
        let def = def.unwrap();
        // Should point to same-file definition, not external schema.
        assert!(
            def.target.file_uri.is_none(),
            "same-file DDL should shadow schema"
        );
        assert_eq!(def.target.start, src.find('t').unwrap());
    }

    // ── Analyzer: function validation ──────────────────────────────────────────

    #[test]
    fn analyze_unknown_function_flagged() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze("SELECT totally_unknown_fn(1)", &cat, &strict());
        let errs: Vec<_> = model.diagnostics().iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownFunction { name } if name == "totally_unknown_fn"))
            .collect();
        assert_eq!(errs.len(), 1);
    }

    #[test]
    fn analyze_builtin_abs_no_error() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze("SELECT abs(-1)", &cat, &strict());
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownFunction { .. }))
            .collect();
        assert!(errs.is_empty(), "abs() should be a known builtin: {errs:?}");
    }

    // ── Analyzer: cflag-gated function availability ───────────────────────────

    #[test]
    fn math_function_unknown_without_cflag() {
        // Without SQLITE_ENABLE_MATH_FUNCTIONS the math functions (acos, sin,
        // cos, …) are absent from the catalog and should produce an error.
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog(); // default: no cflags set
        let model = az.analyze("SELECT acos(1.0)", &cat, &strict());
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| {
                matches!(&d.message, DiagnosticMessage::UnknownFunction { name } if name == "acos")
            })
            .collect();
        assert_eq!(
            errs.len(),
            1,
            "acos() should be unknown without SQLITE_ENABLE_MATH_FUNCTIONS: {:?}",
            model.diagnostics()
        );
    }

    #[test]
    fn math_function_known_with_cflag() {
        // With SQLITE_ENABLE_MATH_FUNCTIONS set, acos should be in the catalog.
        use crate::util::{SqliteFlag, SqliteFlags};
        let dialect = crate::sqlite::dialect::dialect()
            .with_cflags(SqliteFlags::default().with(SqliteFlag::EnableMathFunctions));
        let mut az = SemanticAnalyzer::with_dialect(dialect.clone());
        let cat = Catalog::new(dialect);
        let model = az.analyze("SELECT acos(1.0)", &cat, &strict());
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownFunction { .. }))
            .collect();
        assert!(
            errs.is_empty(),
            "acos() should be known with SQLITE_ENABLE_MATH_FUNCTIONS: {errs:?}"
        );
    }

    // ── Analyzer: multiple statements ─────────────────────────────────────────

    #[test]
    fn analyze_multiple_selects_independent() {
        let mut az = sqlite_analyzer();
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".to_string()]),
            false,
        );
        let src = "SELECT id FROM users; SELECT id FROM users;";
        let model = az.analyze(src, &cat, &strict());
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownTable { .. }))
            .collect();
        assert!(errs.is_empty());
    }

    #[test]
    fn analyze_reuse_clears_document_layer() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();

        // First pass: CREATE TABLE makes 't' visible.
        az.analyze(
            "CREATE TABLE t (id INTEGER); SELECT id FROM t;",
            &cat,
            &strict(),
        );

        // Second pass: 't' should NOT be visible — document layer was cleared.
        let model = az.analyze("SELECT id FROM t;", &cat, &strict());
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(
                |d| matches!(&d.message, DiagnosticMessage::UnknownTable { name } if name == "t"),
            )
            .collect();
        assert_eq!(
            errs.len(),
            1,
            "document layer should be cleared between passes"
        );
    }

    // ── Connection layer ───────────────────────────────────────────────────────

    #[test]
    fn catalog_connection_table_resolves() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Connection).insert_table(
            "conn_tbl",
            Some(vec!["id".to_string()]),
            false,
        );
        let model = sqlite_analyzer().analyze("SELECT id FROM conn_tbl", &cat, &strict());
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownTable { name } if name == "conn_tbl"))
            .collect();
        assert!(
            errs.is_empty(),
            "connection table should be visible: {errs:?}"
        );
    }

    #[test]
    fn catalog_connection_shadows_database() {
        // "t" in database has only column "a"; in connection has only "b".
        // Querying "b" should succeed (connection takes priority, shadowing db entry).
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database)
            .insert_table("t", Some(vec!["a".to_string()]), false);
        cat.layer_mut(CatalogLayer::Connection).insert_table(
            "t",
            Some(vec!["b".to_string()]),
            false,
        );
        let model = sqlite_analyzer().analyze("SELECT b FROM t", &cat, &strict());
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "connection layer should shadow database layer: {col_errs:?}"
        );
    }

    #[test]
    fn catalog_document_shadows_connection() {
        // "t" in connection has only "a"; document DDL creates "t" with only "b".
        // Querying "b" should succeed.
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Connection).insert_table(
            "t",
            Some(vec!["a".to_string()]),
            false,
        );
        let model = sqlite_analyzer().analyze(
            "CREATE TABLE t (b INTEGER); SELECT b FROM t",
            &cat,
            &strict(),
        );
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "document layer should shadow connection layer: {col_errs:?}"
        );
    }

    #[test]
    fn catalog_clear_connection() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Connection).insert_table(
            "conn_tbl",
            Some(vec!["id".to_string()]),
            false,
        );
        cat.new_connection();
        let model = sqlite_analyzer().analyze("SELECT id FROM conn_tbl", &cat, &strict());
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownTable { name } if name == "conn_tbl"))
            .collect();
        assert_eq!(errs.len(), 1, "connection table should be gone after clear");
    }

    // ── DiagnosticRenderer ─────────────────────────────────────────────────────

    #[test]
    fn renderer_produces_output_for_error() {
        let source = "SELECT * FROM missing";
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(source, &cat, &strict());
        assert!(!model.diagnostics().is_empty());

        let renderer = DiagnosticRenderer::new(source, "test.sql");
        let mut out = Vec::new();
        let has_errors = renderer
            .render_diagnostics(model.diagnostics(), &mut out)
            .unwrap();
        let text = String::from_utf8(out).unwrap();
        assert!(has_errors);
        assert!(
            text.contains("error:"),
            "expected 'error:' in output:\n{text}"
        );
        assert!(
            text.contains("missing"),
            "expected table name in output:\n{text}"
        );
    }

    #[test]
    fn renderer_includes_suggestion() {
        let source = "SELECT * FROM usres";
        let mut az = sqlite_analyzer();
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".to_string()]),
            false,
        );
        let model = az.analyze(source, &cat, &strict());

        let renderer = DiagnosticRenderer::new(source, "test.sql");
        let mut out = Vec::new();
        renderer
            .render_diagnostics(model.diagnostics(), &mut out)
            .unwrap();
        let text = String::from_utf8(out).unwrap();
        assert!(
            text.contains("users"),
            "expected suggestion in output:\n{text}"
        );
    }

    // ── Fuzzy matching ─────────────────────────────────────────────────────────

    // ── CTE column list validation ────────────────────────────────────────────

    #[test]
    fn cte_without_column_list_no_error() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(
            "WITH cte AS (SELECT 1 AS x) SELECT x FROM cte",
            &cat,
            &strict(),
        );
        let sem_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| !d.message.is_parse_error())
            .collect();
        assert!(sem_errs.is_empty(), "unexpected diagnostics: {sem_errs:?}");
    }

    #[test]
    fn cte_column_list_count_matches_no_error() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(
            "WITH cte(a, b) AS (SELECT 1, 2) SELECT a, b FROM cte",
            &cat,
            &strict(),
        );
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::CteColumnCountMismatch { .. }))
            .collect();
        assert!(errs.is_empty(), "unexpected mismatch diagnostic: {errs:?}");
    }

    #[test]
    fn cte_column_list_count_mismatch_is_error() {
        // 1 declared column, 2 result columns
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(
            "WITH cte(a) AS (SELECT 1, 2) SELECT a FROM cte",
            &cat,
            &strict(),
        );
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::CteColumnCountMismatch { .. }))
            .collect();
        assert_eq!(errs.len(), 1, "expected CteColumnCountMismatch: {errs:?}");
        assert_eq!(errs[0].severity, Severity::Error);
        if let DiagnosticMessage::CteColumnCountMismatch {
            declared, actual, ..
        } = &errs[0].message
        {
            assert_eq!(*declared, 1);
            assert_eq!(*actual, 2);
        }
    }

    #[test]
    fn cte_column_list_too_many_declared_is_error() {
        // 3 declared columns, 2 result columns
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(
            "WITH cte(a, b, c) AS (SELECT 1, 2) SELECT a FROM cte",
            &cat,
            &strict(),
        );
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::CteColumnCountMismatch { .. }))
            .collect();
        assert_eq!(errs.len(), 1, "expected CteColumnCountMismatch: {errs:?}");
        if let DiagnosticMessage::CteColumnCountMismatch {
            declared, actual, ..
        } = &errs[0].message
        {
            assert_eq!(*declared, 3);
            assert_eq!(*actual, 2);
        }
    }

    #[test]
    fn cte_declared_column_names_visible_in_outer_query() {
        // WITH cte(x, y): x and y should be accessible; unknown z should fail
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(
            "WITH cte(x, y) AS (SELECT 1, 2) SELECT z FROM cte",
            &cat,
            &strict(),
        );
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { column, .. } if column == "z"))
            .collect();
        assert_eq!(errs.len(), 1, "expected UnknownColumn for 'z': {errs:?}");
    }

    #[test]
    fn cte_declared_columns_no_false_positive_for_valid_ref() {
        // WITH cte(x, y): selecting x should be fine
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(
            "WITH cte(x, y) AS (SELECT 1, 2) SELECT x FROM cte",
            &cat,
            &strict(),
        );
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(errs.is_empty(), "unexpected UnknownColumn: {errs:?}");
    }

    #[test]
    fn cte_star_body_skips_count_check() {
        // SELECT * in CTE body — count check should be skipped (no false error)
        let mut az = sqlite_analyzer();
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database).insert_table(
            "t",
            Some(vec!["a".to_string(), "b".to_string()]),
            false,
        );
        let model = az.analyze(
            "WITH cte(x, y) AS (SELECT * FROM t) SELECT x FROM cte",
            &cat,
            &strict(),
        );
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::CteColumnCountMismatch { .. }))
            .collect();
        assert!(
            errs.is_empty(),
            "should skip count check for SELECT *: {errs:?}"
        );
    }

    // ── CTE column inference (no declared list) ───────────────────────────────

    #[test]
    fn cte_inferred_column_invalid_ref_is_error() {
        // WITH cte AS (SELECT 1 AS x) — x is inferred, z is not; z should error
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(
            "WITH cte AS (SELECT 1 AS x) SELECT z FROM cte",
            &cat,
            &strict(),
        );
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { column, .. } if column == "z"))
            .collect();
        assert_eq!(errs.len(), 1, "expected UnknownColumn for 'z': {errs:?}");
    }

    #[test]
    fn cte_inferred_column_valid_ref_no_error() {
        // WITH cte AS (SELECT 1 AS x) — selecting x (the inferred column) is fine
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(
            "WITH cte AS (SELECT 1 AS x) SELECT x FROM cte",
            &cat,
            &strict(),
        );
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(errs.is_empty(), "unexpected UnknownColumn: {errs:?}");
    }

    #[test]
    fn cte_star_body_with_no_column_list_accepts_all() {
        // CTE body is SELECT * — inference skipped, all column refs accepted
        let mut az = sqlite_analyzer();
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database).insert_table(
            "t",
            Some(vec!["a".to_string(), "b".to_string()]),
            false,
        );
        let model = az.analyze(
            "WITH cte AS (SELECT * FROM t) SELECT anything FROM cte",
            &cat,
            &strict(),
        );
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            errs.is_empty(),
            "star body should accept any col ref: {errs:?}"
        );
    }

    // ── CREATE TABLE/VIEW AS SELECT column inference ──────────────────────────
    //
    // Shared test helpers.

    /// Assert `src` produces exactly one `UnknownColumn` for `col`.
    fn assert_unknown_col(src: &str, col: &str) {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &strict());
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| {
                matches!(&d.message, DiagnosticMessage::UnknownColumn { column, .. } if column == col)
            })
            .collect();
        assert_eq!(
            errs.len(),
            1,
            "expected exactly one UnknownColumn for '{col}': {errs:?}"
        );
    }

    /// Assert `src` produces no `UnknownColumn` diagnostics.
    fn assert_no_unknown_col(src: &str) {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(src, &cat, &strict());
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            errs.is_empty(),
            "unexpected UnknownColumn diagnostics: {errs:?}"
        );
    }

    /// Like `assert_no_unknown_col` but pre-loads named relations into the
    /// Database catalog layer before analysis.
    fn assert_no_unknown_col_with_relations(src: &str, relations: &[(&str, &[&str])]) {
        let mut az = sqlite_analyzer();
        let mut cat = sqlite_catalog();
        for (name, cols) in relations {
            cat.layer_mut(CatalogLayer::Database).insert_table(
                *name,
                Some(cols.iter().map(ToString::to_string).collect()),
                false,
            );
        }
        let model = az.analyze(src, &cat, &strict());
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            errs.is_empty(),
            "unexpected UnknownColumn diagnostics: {errs:?}"
        );
    }

    // ── Aliased columns (existing behaviour, must not regress) ────────────────

    #[test]
    fn create_table_as_select_invalid_column_is_error() {
        // SELECT 1 AS x → "x" inferred; "z" must error.
        assert_unknown_col("CREATE TABLE t AS SELECT 1 AS x; SELECT z FROM t;", "z");
    }

    #[test]
    fn create_table_as_select_valid_column_no_error() {
        // SELECT 1 AS x → "x" is the column; accessing it succeeds.
        assert_no_unknown_col("CREATE TABLE t AS SELECT 1 AS x; SELECT x FROM t;");
    }

    #[test]
    fn create_view_as_select_invalid_column_is_error() {
        // CREATE VIEW — same alias rule as CREATE TABLE.
        assert_unknown_col("CREATE VIEW v AS SELECT 1 AS x; SELECT z FROM v;", "z");
    }

    #[test]
    fn create_view_as_select_valid_column_no_error() {
        assert_no_unknown_col("CREATE VIEW v AS SELECT 1 AS x; SELECT x FROM v;");
    }

    // ── Unnamed expressions: ENAME_SPAN (SQLite sqlite3ExprListSetSpan) ───────
    //
    // When a result column has no alias, SQLite names it by the raw source text
    // of the expression (sqlite3.c:113660, ENAME_SPAN).  Any other name must be
    // rejected as an unknown column.

    #[test]
    fn create_table_as_select_unaliased_literal_flags_unknown_column() {
        // SELECT 1 → column "1"; any other name must error.
        assert_unknown_col(
            "CREATE TABLE t AS SELECT 1; SELECT t.order_id FROM t;",
            "order_id",
        );
    }

    #[test]
    fn create_view_as_select_unaliased_literal_flags_unknown_column() {
        // CREATE VIEW — same ENAME_SPAN rule.
        assert_unknown_col(
            "CREATE VIEW v AS SELECT 1; SELECT v.order_id FROM v;",
            "order_id",
        );
    }

    #[test]
    fn create_table_as_select_unaliased_null_flags_unknown_column() {
        // SELECT NULL → column "null"; any other name must error.
        assert_unknown_col(
            "CREATE TABLE t AS SELECT NULL; SELECT t.order_id FROM t;",
            "order_id",
        );
    }

    #[test]
    fn create_table_as_select_unaliased_binary_expr_flags_unknown_column() {
        // SELECT 1+2 → column "1+2"; any other name must error.
        assert_unknown_col(
            "CREATE TABLE t AS SELECT 1+2; SELECT t.order_id FROM t;",
            "order_id",
        );
    }

    #[test]
    fn create_table_as_select_unaliased_function_call_flags_unknown_column() {
        // SELECT abs(1) → column "abs(1)"; any other name must error.
        assert_unknown_col(
            "CREATE TABLE t AS SELECT abs(1); SELECT t.order_id FROM t;",
            "order_id",
        );
    }

    // ── Multiple / mixed columns ───────────────────────────────────────────────

    #[test]
    fn create_table_as_select_multiple_unnamed_columns_flags_unknown_column() {
        // SELECT 1, 2 → columns "1" and "2"; "order_id" matches neither.
        assert_unknown_col(
            "CREATE TABLE t AS SELECT 1, 2; SELECT t.order_id FROM t;",
            "order_id",
        );
    }

    #[test]
    fn create_table_as_select_mixed_named_unnamed_named_col_valid() {
        // SELECT 1, 2 AS y → columns "1" and "y"; accessing "y" succeeds.
        assert_no_unknown_col("CREATE TABLE t AS SELECT 1, 2 AS y; SELECT t.y FROM t;");
    }

    #[test]
    fn create_table_as_select_mixed_named_unnamed_wrong_col_errors() {
        // SELECT 1, 2 AS y → "z" is in neither column; must error.
        assert_unknown_col(
            "CREATE TABLE t AS SELECT 1, 2 AS y; SELECT t.z FROM t;",
            "z",
        );
    }

    // ── Regressions / edge cases ──────────────────────────────────────────────

    #[test]
    fn create_table_as_select_star_still_accepts_any_column() {
        // SELECT * → columns unknown (STAR); any ref must be accepted
        // conservatively — must not regress after the span-fallback change.
        assert_no_unknown_col_with_relations(
            "CREATE TABLE t AS SELECT * FROM src; SELECT t.anything FROM t;",
            &[("src", &["id"])],
        );
    }

    #[test]
    fn create_table_as_select_expression_span_does_not_break_alias_path() {
        // The expression-span fallback must not shadow the alias path.
        // SELECT 1 AS x → only "x" is valid; "z" must still error.
        assert_unknown_col("CREATE TABLE t AS SELECT 1 AS x; SELECT t.z FROM t;", "z");
    }

    // ── Bug regression: unknown table should not produce column errors ─────────

    /// When `FROM unknown_table` is encountered, the table is flagged as unknown
    /// but its columns CANNOT be validated — any column reference against an
    /// unknown table must be accepted conservatively.
    ///
    /// Root cause investigated: `visit_query` never pushed a query scope, so
    /// `add_query_table("users", None)` was a silent no-op (no frame to write
    /// into). Column resolution then found zero tables with `None` columns →
    /// `ColumnResolution::NotFound` → spurious `UnknownColumn` diagnostics.
    #[test]
    fn unknown_table_columns_not_flagged() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(
            "SELECT id, name, email FROM users WHERE age >= 0 AND active = 1 ORDER BY name",
            &cat,
            &strict(),
        );

        // UnknownTable for "users" is expected and correct.
        let table_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| {
                matches!(&d.message, DiagnosticMessage::UnknownTable { name } if name == "users")
            })
            .collect();
        assert_eq!(
            table_errs.len(),
            1,
            "expected exactly one UnknownTable for 'users': {:#?}",
            model.diagnostics()
        );

        // UnknownColumn must NOT appear — users is unknown so any column is ok.
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "unknown table means columns should be accepted conservatively, got: {col_errs:#?}"
        );
    }

    /// Two unknown tables in the same query: neither should generate column errors.
    #[test]
    fn unknown_tables_join_columns_not_flagged() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(
            "SELECT u.id, o.total FROM users u JOIN orders o ON u.id = o.user_id",
            &cat,
            &strict(),
        );

        // Two UnknownTable errors expected.
        let table_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownTable { .. }))
            .collect();
        assert_eq!(
            table_errs.len(),
            2,
            "expected UnknownTable for both 'users' and 'orders': {:#?}",
            model.diagnostics()
        );

        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "unknown tables should suppress all column errors, got: {col_errs:#?}"
        );
    }

    /// Subquery alias from unknown inner table should be accepted conservatively.
    #[test]
    fn unknown_table_in_subquery_columns_not_flagged() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(
            "SELECT sub.id FROM (SELECT id FROM users) AS sub",
            &cat,
            &strict(),
        );

        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "subquery with unknown table should suppress column errors, got: {col_errs:#?}"
        );
    }

    // ── DML: unknown table should not produce column errors ─────────────────

    /// `INSERT INTO unknown_table(col)` — column refs in the INSERT column list
    /// must not be flagged when the target table is unknown.
    #[test]
    fn insert_unknown_table_columns_not_flagged() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(
            "INSERT INTO unknown_tbl(a, b, c) VALUES(1, 2, 3)",
            &cat,
            &strict(),
        );
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "INSERT into unknown table should suppress column errors, got: {col_errs:#?}"
        );
    }

    /// `UPDATE unknown_table SET col=val WHERE other_col=1` — column refs in
    /// SET and WHERE must not be flagged when the target table is unknown.
    #[test]
    fn update_unknown_table_columns_not_flagged() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze(
            "UPDATE unknown_tbl SET stat='val' WHERE idx='t1a'",
            &cat,
            &strict(),
        );
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "UPDATE on unknown table should suppress column errors, got: {col_errs:#?}"
        );
    }

    /// `DELETE FROM unknown_table WHERE col=1` — column refs in WHERE must not
    /// be flagged when the target table is unknown.
    #[test]
    fn delete_unknown_table_columns_not_flagged() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze("DELETE FROM unknown_tbl WHERE idx='t1a'", &cat, &strict());
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "DELETE from unknown table should suppress column errors, got: {col_errs:#?}"
        );
    }

    // ── ORDER BY alias resolution ─────────────────────────────────────────────

    /// SELECT alias used in ORDER BY should not produce `UnknownColumn`.
    #[test]
    fn order_by_select_alias_no_unknown_column() {
        let dialect = crate::sqlite::dialect::dialect();
        let ddl = "CREATE TABLE users (id INTEGER, name TEXT, active INT);";
        let cat = Catalog::from_ddl(dialect, &[(ddl, None)]).0;
        let mut az = sqlite_analyzer();
        let model = az.analyze(
            "SELECT COUNT(*) AS cnt FROM users ORDER BY cnt",
            &cat,
            &strict(),
        );
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "ORDER BY referencing SELECT alias should not flag UnknownColumn, got: {col_errs:#?}"
        );
    }

    /// SELECT alias with expression + GROUP BY + ORDER BY.
    #[test]
    fn order_by_alias_with_group_by() {
        let dialect = crate::sqlite::dialect::dialect();
        let ddl = "CREATE TABLE employees (id INTEGER, dept TEXT, salary REAL);";
        let cat = Catalog::from_ddl(dialect, &[(ddl, None)]).0;
        let mut az = sqlite_analyzer();
        let model = az.analyze(
            "SELECT dept, SUM(salary) AS total_salary FROM employees GROUP BY dept ORDER BY total_salary DESC",
            &cat,
            &strict(),
        );
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "ORDER BY alias with GROUP BY should not flag UnknownColumn, got: {col_errs:#?}"
        );
    }

    /// HAVING clause can also reference SELECT aliases in `SQLite`.
    #[test]
    fn having_select_alias_no_unknown_column() {
        let dialect = crate::sqlite::dialect::dialect();
        let ddl = "CREATE TABLE users (id INTEGER, active INT);";
        let cat = Catalog::from_ddl(dialect, &[(ddl, None)]).0;
        let mut az = sqlite_analyzer();
        let model = az.analyze(
            "SELECT COUNT(*) AS n FROM users HAVING n > 0",
            &cat,
            &strict(),
        );
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "HAVING referencing SELECT alias should not flag UnknownColumn, got: {col_errs:#?}"
        );
    }

    /// WHERE clause can reference SELECT aliases in `SQLite` (real column wins on collision).
    #[test]
    fn where_select_alias_no_unknown_column() {
        let dialect = crate::sqlite::dialect::dialect();
        let ddl = "CREATE TABLE t (a INT, b INT);";
        let cat = Catalog::from_ddl(dialect, &[(ddl, None)]).0;
        let mut az = sqlite_analyzer();
        let model = az.analyze(
            "SELECT a + b AS total FROM t WHERE total > 10",
            &cat,
            &strict(),
        );
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "WHERE referencing SELECT alias should not flag UnknownColumn, got: {col_errs:#?}"
        );
    }

    /// ATTACH schema name is a bare identifier, not a column reference.
    #[test]
    fn attach_schema_name_no_unknown_column() {
        let mut az = sqlite_analyzer();
        let cat = sqlite_catalog();
        let model = az.analyze("ATTACH ':memory:' AS scratch", &cat, &strict());
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "ATTACH schema name should not flag UnknownColumn, got: {col_errs:#?}"
        );
    }

    /// GROUP BY can reference SELECT aliases in `SQLite` (real column wins on collision).
    #[test]
    fn group_by_select_alias_no_unknown_column() {
        let dialect = crate::sqlite::dialect::dialect();
        let ddl = "CREATE TABLE t (a INT, b INT);";
        let cat = Catalog::from_ddl(dialect, &[(ddl, None)]).0;
        let mut az = sqlite_analyzer();
        let model = az.analyze(
            "SELECT a + b AS total, COUNT(*) FROM t GROUP BY total",
            &cat,
            &strict(),
        );
        let col_errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(
            col_errs.is_empty(),
            "GROUP BY referencing SELECT alias should not flag UnknownColumn, got: {col_errs:#?}"
        );
    }

    // ── Quoted identifier dequoting ─────────────────────────────────────────
    //
    // End-to-end table/column dequoting is covered by the `validate`
    // integration suite.  These unit tests cover cases that the CLI tests
    // don't reach: column-level quoting variants and expression-span
    // column names from `CREATE TABLE AS SELECT`.

    fn assert_no_unknown_col_with_ddl(ddl: &str, query: &str) {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(dialect, &[(ddl, None)]).0;
        let mut az = sqlite_analyzer();
        let model = az.analyze(query, &cat, &strict());
        let errs: Vec<_> = model
            .diagnostics()
            .iter()
            .filter(|d| matches!(&d.message, DiagnosticMessage::UnknownColumn { .. }))
            .collect();
        assert!(errs.is_empty(), "unexpected UnknownColumn: {errs:#?}");
    }

    #[test]
    fn dequote_col_backtick_ddl_unquoted_query() {
        assert_no_unknown_col_with_ddl("CREATE TABLE t (`col` INTEGER);", "SELECT col FROM t");
    }

    #[test]
    fn dequote_col_bracket_ddl_unquoted_query() {
        assert_no_unknown_col_with_ddl("CREATE TABLE t ([col] INTEGER);", "SELECT col FROM t");
    }

    #[test]
    fn dequote_col_unquoted_ddl_double_quoted_query() {
        assert_no_unknown_col_with_ddl("CREATE TABLE t (col INTEGER);", r#"SELECT "col" FROM t"#);
    }

    #[test]
    fn dequote_reserved_word_column_qualified_ref() {
        assert_no_unknown_col_with_ddl(
            r#"CREATE TABLE t (id INTEGER, "set" TEXT);"#,
            r#"SELECT t."set" FROM t"#,
        );
    }

    #[test]
    fn dequote_expr_span_col_double_quoted() {
        assert_no_unknown_col(r#"CREATE TABLE t AS SELECT 1; SELECT t."1" FROM t;"#);
    }

    #[test]
    fn dequote_expr_span_col_backtick() {
        assert_no_unknown_col("CREATE TABLE t AS SELECT 1; SELECT t.`1` FROM t;");
    }

    #[test]
    fn dequote_expr_span_col_bracket() {
        assert_no_unknown_col("CREATE TABLE t AS SELECT 1; SELECT t.[1] FROM t;");
    }

    #[test]
    fn dequote_aliased_col_double_quoted() {
        assert_no_unknown_col(r#"CREATE TABLE t AS SELECT 1 AS x; SELECT t."x" FROM t;"#);
    }

    #[test]
    fn select_true_no_unknown_column() {
        assert_no_unknown_col("SELECT TRUE;");
    }

    #[test]
    fn select_false_no_unknown_column() {
        assert_no_unknown_col("SELECT FALSE;");
    }

    #[test]
    fn select_true_false_case_insensitive() {
        assert_no_unknown_col("SELECT true, False, TRUE, false;");
    }

    #[test]
    fn levenshtein_same_string_is_zero() {
        use super::super::fuzzy::levenshtein_distance;
        assert_eq!(levenshtein_distance("abc", "abc"), 0);
    }

    #[test]
    fn levenshtein_one_edit() {
        use super::super::fuzzy::levenshtein_distance;
        assert_eq!(levenshtein_distance("abc", "axc"), 1);
        assert_eq!(levenshtein_distance("abc", "abcd"), 1);
        assert_eq!(levenshtein_distance("abcd", "abc"), 1);
    }

    #[test]
    fn best_suggestion_finds_closest() {
        use super::super::fuzzy::best_suggestion;
        let candidates = vec![
            "users".to_string(),
            "orders".to_string(),
            "products".to_string(),
        ];
        let s = best_suggestion("usres", &candidates, 2);
        assert_eq!(s.as_deref(), Some("users"));
    }

    #[test]
    fn best_suggestion_none_when_too_far() {
        use super::super::fuzzy::best_suggestion;
        let candidates = vec!["users".to_string()];
        let s = best_suggestion("xyzzy", &candidates, 2);
        assert!(s.is_none());
    }
}

#[cfg(test)]
#[cfg(feature = "sqlite")]
mod detect_qualifier_test {
    use super::*;
    use crate::semantic::model::StoredToken;
    use syntaqlite_syntax::ParserTokenFlags;

    #[test]
    fn test_detect_qualifier_basic() {
        let dialect = crate::sqlite::dialect::dialect();
        let source = "SELECT t1.";
        let id_type = AnyTokenType::from(syntaqlite_syntax::TokenType::Id);
        let dot_type = AnyTokenType::from(syntaqlite_syntax::TokenType::Dot);

        let tokens = vec![
            StoredToken {
                offset: 7,
                length: 2,
                token_type: id_type,
                flags: ParserTokenFlags::default(),
            },
            StoredToken {
                offset: 9,
                length: 1,
                token_type: dot_type,
                flags: ParserTokenFlags::default(),
            },
        ];
        let result = detect_qualifier(source, &tokens, &dialect);
        assert_eq!(result.as_deref(), Some("t1"));
    }
}

#[cfg(test)]
#[cfg(feature = "sqlite")]
mod lineage_tests {
    use super::super::catalog::{Catalog, CatalogLayer};
    use super::super::lineage::{ColumnOrigin, RelationKind};
    use super::*;

    fn sqlite_analyzer() -> SemanticAnalyzer {
        SemanticAnalyzer::new()
    }

    fn sqlite_catalog() -> Catalog {
        Catalog::new(crate::sqlite::dialect::dialect())
    }

    fn lenient() -> ValidationConfig {
        ValidationConfig::default()
    }

    // ── Test 1: Simple direct columns ────────────────────────────────────────

    #[test]
    fn lineage_simple_direct_columns() {
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into(), "name".into()]),
            false,
        );
        let model = analyzer.analyze("SELECT id, name FROM users", &catalog, &lenient());

        // lineage should be Some(Complete(...))
        let lineage = model.lineage().expect("should be a query");
        assert!(lineage.is_complete());
        let cols = lineage.into_inner();
        assert_eq!(cols.len(), 2);
        assert_eq!(cols[0].name, "id");
        assert_eq!(cols[0].index, 0);
        assert_eq!(
            cols[0].origin,
            Some(ColumnOrigin {
                table: "users".into(),
                column: "id".into(),
            })
        );
        assert_eq!(cols[1].name, "name");
        assert_eq!(cols[1].index, 1);
        assert_eq!(
            cols[1].origin,
            Some(ColumnOrigin {
                table: "users".into(),
                column: "name".into(),
            })
        );

        // relations_accessed — only catalog relations (not CTEs/subqueries)
        let rels = model.relations_accessed().unwrap().into_inner();
        assert_eq!(rels.len(), 1);
        assert_eq!(rels[0].name, "users");
        assert_eq!(rels[0].kind, RelationKind::Table);

        // tables_accessed
        let tbls = model.tables_accessed().unwrap().into_inner();
        assert_eq!(tbls.len(), 1);
        assert_eq!(tbls[0].name, "users");
    }

    // ── Test 2: Expression — origin is None ──────────────────────────────────

    #[test]
    fn lineage_expression_no_origin() {
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into(), "name".into()]),
            false,
        );
        let model = analyzer.analyze("SELECT id + 1 AS x FROM users", &catalog, &lenient());

        let lineage = model.lineage().unwrap();
        assert!(lineage.is_complete());
        let cols = lineage.into_inner();
        assert_eq!(cols.len(), 1);
        assert_eq!(cols[0].name, "x");
        assert!(cols[0].origin.is_none(), "expression should have no origin");
    }

    // ── Test 3: Alias traces to physical column ──────────────────────────────

    #[test]
    fn lineage_alias() {
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into(), "name".into()]),
            false,
        );
        let model = analyzer.analyze("SELECT id AS user_id FROM users", &catalog, &lenient());

        let cols = model.lineage().unwrap().into_inner();
        assert_eq!(cols[0].name, "user_id");
        assert_eq!(
            cols[0].origin,
            Some(ColumnOrigin {
                table: "users".into(),
                column: "id".into(),
            })
        );
    }

    // ── Test 4: Multi-table join ─────────────────────────────────────────────

    #[test]
    fn lineage_multi_table_join() {
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into(), "name".into()]),
            false,
        );
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "orders",
            Some(vec!["id".into(), "user_id".into(), "amount".into()]),
            false,
        );
        let model = analyzer.analyze(
            "SELECT u.id, o.amount FROM users u JOIN orders o ON u.id = o.user_id",
            &catalog,
            &lenient(),
        );

        let cols = model.lineage().unwrap().into_inner();
        assert_eq!(cols.len(), 2);
        assert_eq!(
            cols[0].origin,
            Some(ColumnOrigin {
                table: "users".into(),
                column: "id".into(),
            })
        );
        assert_eq!(
            cols[1].origin,
            Some(ColumnOrigin {
                table: "orders".into(),
                column: "amount".into(),
            })
        );

        let tbls = model.tables_accessed().unwrap().into_inner();
        assert!(tbls.iter().any(|t| t.name == "users"));
        assert!(tbls.iter().any(|t| t.name == "orders"));
    }

    // ── Test 5: CTE transitive tracing ───────────────────────────────────────

    #[test]
    fn lineage_cte_transitive() {
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into(), "name".into()]),
            false,
        );
        let model = analyzer.analyze(
            "WITH cte AS (SELECT id FROM users) SELECT id FROM cte",
            &catalog,
            &lenient(),
        );

        let lineage = model.lineage().unwrap();
        assert!(lineage.is_complete());
        let cols = lineage.into_inner();
        assert_eq!(cols.len(), 1);
        assert_eq!(
            cols[0].origin,
            Some(ColumnOrigin {
                table: "users".into(),
                column: "id".into(),
            })
        );

        // relations — only catalog relations, CTE excluded
        let rels = model.relations_accessed().unwrap().into_inner();
        assert_eq!(rels.len(), 1);
        assert_eq!(rels[0].name, "users");
        assert_eq!(rels[0].kind, RelationKind::Table);

        // tables includes users (physical)
        let tbls = model.tables_accessed().unwrap().into_inner();
        assert_eq!(tbls.len(), 1);
        assert_eq!(tbls[0].name, "users");
    }

    // ── Test 6: Subquery transitive ──────────────────────────────────────────

    #[test]
    fn lineage_subquery_transitive() {
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into(), "name".into()]),
            false,
        );
        let model = analyzer.analyze(
            "SELECT id FROM (SELECT id FROM users) sub",
            &catalog,
            &lenient(),
        );

        let cols = model.lineage().unwrap().into_inner();
        assert_eq!(cols.len(), 1);
        assert_eq!(
            cols[0].origin,
            Some(ColumnOrigin {
                table: "users".into(),
                column: "id".into(),
            })
        );

        let tbls = model.tables_accessed().unwrap().into_inner();
        assert_eq!(tbls.len(), 1);
        assert_eq!(tbls[0].name, "users");
    }

    // ── Test 7: SELECT * expands from catalog ────────────────────────────────

    #[test]
    fn lineage_select_star() {
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into(), "name".into()]),
            false,
        );
        let model = analyzer.analyze("SELECT * FROM users", &catalog, &lenient());

        let cols = model.lineage().unwrap().into_inner();
        assert_eq!(cols.len(), 2);
        // Both should trace to users
        for col in cols {
            assert!(col.origin.is_some());
            assert_eq!(col.origin.as_ref().unwrap().table, "users");
        }
    }

    // ── Test 8: View produces Partial result ─────────────────────────────────

    #[test]
    fn lineage_view_partial() {
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        catalog
            .layer_mut(CatalogLayer::Database)
            .insert_view("active_users", Some(vec!["id".into(), "name".into()]));

        let model = analyzer.analyze("SELECT id FROM active_users", &catalog, &lenient());

        let lineage = model.lineage().unwrap();
        assert!(
            !lineage.is_complete(),
            "view with unavailable body should be Partial"
        );

        let rels = model.relations_accessed().unwrap().into_inner();
        assert_eq!(rels.len(), 1);
        assert_eq!(rels[0].name, "active_users");
        assert_eq!(rels[0].kind, RelationKind::View);
    }

    // ── Test 9: Non-SELECT returns None ──────────────────────────────────────

    #[test]
    fn lineage_non_select_returns_none() {
        let mut analyzer = sqlite_analyzer();
        let catalog = sqlite_catalog();
        let model = analyzer.analyze("CREATE TABLE t(x)", &catalog, &lenient());

        assert!(model.lineage().is_none());
        assert!(model.relations_accessed().is_none());
        assert!(model.tables_accessed().is_none());
    }

    // ── Test 10: CTE with expression — origin None ───────────────────────────

    #[test]
    fn lineage_cte_with_expression() {
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into(), "name".into()]),
            false,
        );
        let model = analyzer.analyze(
            "WITH cte AS (SELECT id+1 AS x FROM users) SELECT x FROM cte",
            &catalog,
            &lenient(),
        );

        let cols = model.lineage().unwrap().into_inner();
        assert_eq!(cols.len(), 1);
        assert_eq!(cols[0].name, "x");
        assert!(
            cols[0].origin.is_none(),
            "expression in CTE should have no origin"
        );
    }

    // ── Recursive CTE — must not stack-overflow ───────────────────────────

    #[test]
    fn lineage_recursive_cte_no_stackoverflow() {
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "users",
            Some(vec!["id".into(), "name".into()]),
            false,
        );
        // Recursive CTE: the body references `cte` itself.
        let model = analyzer.analyze(
            "WITH RECURSIVE cte(id) AS (
                SELECT id FROM users
                UNION ALL
                SELECT id FROM cte
            ) SELECT id FROM cte",
            &catalog,
            &lenient(),
        );

        // We just care that it doesn't stack-overflow / crash.
        // Lineage should still be Some (it's a SELECT).
        let lineage = model.lineage().expect("should be a query");
        let cols = lineage.into_inner();
        assert_eq!(cols.len(), 1);
        assert_eq!(cols[0].name, "id");
    }

    // ── Catalog: is_view ─────────────────────────────────────────────────────

    #[test]
    fn catalog_is_view() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database)
            .insert_table("users", Some(vec!["id".into()]), false);
        cat.layer_mut(CatalogLayer::Database)
            .insert_view("active_users", Some(vec!["id".into()]));

        assert!(!cat.is_view("users"));
        assert!(cat.is_view("active_users"));
        assert!(!cat.is_view("nonexistent"));
    }
}
