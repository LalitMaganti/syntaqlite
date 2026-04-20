// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Single-pass semantic analysis engine.

use std::cell::RefCell;
use std::collections::{HashMap, HashSet};
use std::rc::Rc;

use syntaqlite_syntax::ParserConfig;
use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, AnyParser, FieldValue, ParseOutcome};
use syntaqlite_syntax::source::DocRange;
use syntaqlite_syntax::{MacroArg, MacroLookup, MacroOutput};

use crate::dialect::AnyDialect;
use crate::dialect::SemanticRole;

use super::catalog::{Catalog, CatalogLayer};
use super::ddl::DdlReader;
use super::diagnostics::{Diagnostic, DiagnosticMessage};
use super::model::{SemanticModel, StatementModel};
use super::observer::{AnalysisObserver, NoopObserver};
use super::{AnalysisMode, ValidationConfig};

use helpers::{extract_defined_relations, extract_macro_registration, parse_error_span};

mod helpers;
mod pass;
mod query_scope;

use pass::ValidationPass;

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
/// assert!(!model.has_diagnostics());
/// ```
pub struct SemanticAnalyzer {
    dialect: AnyDialect,
    catalog: Catalog,
    mode: AnalysisMode,
    macro_fallback: bool,
    resolver: Option<Box<dyn super::ModuleResolver>>,
    /// Modules already imported (by dotted path) — prevents cycles and
    /// duplicate imports.
    imported: HashSet<String>,
}

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
            resolver: None,
            imported: HashSet::new(),
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

    /// Attach a module resolver for handling import statements (e.g.
    /// `INCLUDE PERFETTO MODULE`).
    ///
    /// When the analyzer encounters an import, it calls the resolver to
    /// obtain the module's SQL source, analyzes it recursively, and
    /// accumulates the resulting DDL into the catalog.
    #[must_use]
    pub fn with_module_resolver(mut self, resolver: Box<dyn super::ModuleResolver>) -> Self {
        self.resolver = Some(resolver);
        self
    }

    /// Set (or clear) the module resolver on an existing analyzer.
    pub fn set_module_resolver(&mut self, resolver: Option<Box<dyn super::ModuleResolver>>) {
        self.resolver = resolver;
    }

    /// Enable macro fallback: unregistered `name!(args)` calls parse as
    /// identifiers and record [`MacroRewrite`]s on the resulting model.
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
    /// assert!(model.has_diagnostics());
    /// assert_eq!(model.diagnostics().next().unwrap().severity(), Severity::Warning);
    /// ```
    pub fn analyze(
        &mut self,
        source: &str,
        user_catalog: &Catalog,
        config: &ValidationConfig,
    ) -> SemanticModel {
        self.analyze_with_observer(source, user_catalog, config, &mut NoopObserver)
    }

    /// Run a complete single-pass analysis, forwarding resolution / token /
    /// comment / definition events to `observer` as they occur.
    ///
    /// This is the hook that in-crate LSP and embedded-SQL consumers use to
    /// capture the data they need (go-to-definition, find-references, semantic
    /// tokens, completion) without the analyzer knowing anything about them.
    pub(crate) fn analyze_with_observer(
        &mut self,
        source: &str,
        user_catalog: &Catalog,
        config: &ValidationConfig,
        observer: &mut dyn AnalysisObserver,
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
        let model = self.analyze_inner(source, config, observer);
        if self.mode == AnalysisMode::Execute {
            self.catalog.promote_document_to_connection();
        }
        model
    }

    #[expect(clippy::too_many_lines)]
    fn analyze_inner(
        &mut self,
        source: &str,
        config: &ValidationConfig,
        observer: &mut dyn AnalysisObserver,
    ) -> SemanticModel {
        type MacroRegistry = HashMap<String, (Vec<String>, String)>;

        struct SharedRegistryLookup(Rc<RefCell<MacroRegistry>>);
        impl MacroLookup for SharedRegistryLookup {
            fn lookup(
                &mut self,
                name: &str,
                _args: &[MacroArg<'_>],
                out: &mut MacroOutput,
            ) -> bool {
                let reg = self.0.borrow();
                let Some((params, body)) = reg.get(&name.to_ascii_lowercase()) else {
                    return false;
                };
                out.expand_template(body, params)
            }
        }

        let syntax = (*self.dialect).clone();
        let mut parser = AnyParser::with_config(
            syntax,
            &ParserConfig::default()
                .with_collect_tokens(true)
                .with_macro_fallback(self.macro_fallback),
        );

        // Macro registry shared between the lookup callback and the analysis
        // loop. The callback borrows it via Rc<RefCell<…>>.
        //
        // Only install the lookup callback for dialects with native macro
        // support (macro_style).  For dialects without it (e.g. SQLite
        // with macro_fallback for embedded SQL holes), unresolved macro
        // calls fall through to TK_ID without hitting the lookup path.
        // When a lookup IS installed, unresolved names are hard parse errors.
        let registry: Rc<RefCell<MacroRegistry>> = Rc::new(RefCell::new(HashMap::new()));
        if self.dialect.has_macro_style() {
            let registry_for_cb = Rc::clone(&registry);
            parser.set_macro_lookup(Some(Box::new(SharedRegistryLookup(registry_for_cb))));
        }

        let mut session = parser.parse(source);

        let mut statements: Vec<StatementModel> = Vec::new();
        let wants_tokens = observer.wants_tokens();
        let wants_comments = observer.wants_comments();

        loop {
            let stmt = match session.next() {
                ParseOutcome::Done => break,
                ParseOutcome::Ok(s) => s,
                ParseOutcome::Err(e) => {
                    let message = DiagnosticMessage::ParseError(e.message().to_owned());
                    if let Some(severity) = config.checks().level_for(&message).to_severity() {
                        let range = parse_error_span(&e, source);
                        let diag = Diagnostic {
                            range,
                            message,
                            severity,
                            help: None,
                            expansion_frames: Vec::new(),
                        };
                        statements.push(StatementModel::new(
                            String::new(),
                            vec![diag],
                            None,
                            Vec::new(),
                        ));
                    }
                    let base = e.statement_base();
                    if wants_tokens {
                        for tok in e.tokens() {
                            observer.on_token(
                                tok.offset().to_doc(base),
                                tok.length().into(),
                                tok.token_type(),
                                tok.flags(),
                            );
                        }
                    }
                    if wants_comments {
                        for c in e.comments() {
                            observer.on_comment(c.offset().to_doc(base), c.length().into());
                        }
                    }
                    continue;
                }
            };

            let base = stmt.statement_base();
            if wants_tokens {
                for tok in stmt.tokens() {
                    observer.on_token(
                        tok.offset().to_doc(base),
                        tok.length().into(),
                        tok.token_type(),
                        tok.flags(),
                    );
                }
            }
            if wants_comments {
                for c in stmt.comments() {
                    observer.on_comment(c.offset().to_doc(base), c.length().into());
                }
            }

            // Process the statement and extract macro registration info.
            // The erased statement borrows the session, so we must extract
            // owned macro data before dropping it and calling register_macro.
            let (stmt_model, macro_reg) = {
                let mut erased = stmt.erase();
                let model = self.analyze_statement(&mut erased, config, observer);
                let reg = extract_macro_registration(
                    &erased,
                    erased.root_id(),
                    self.dialect.macro_defs(),
                );
                (model, reg)
            };
            statements.push(stmt_model);

            // Register any macro defined by this statement so subsequent
            // `name!(args)` invocations are expanded inline by the parser.
            if let Some((name, params, body)) = macro_reg {
                registry
                    .borrow_mut()
                    .insert(name.to_ascii_lowercase(), (params, body));
            }
        }

        SemanticModel {
            source: source.to_owned(),
            statements,
        }
    }

    fn analyze_statement(
        &mut self,
        erased: &mut AnyParsedStatement<'_>,
        config: &ValidationConfig,
        observer: &mut dyn AnalysisObserver,
    ) -> StatementModel {
        let root_id = erased.root_id();
        let mut diagnostics: Vec<Diagnostic> = Vec::new();

        self.catalog
            .accumulate_ddl(CatalogLayer::Document, erased, root_id, &self.dialect);

        // Handle module imports: resolve and analyze imported source.
        self.handle_import(erased, root_id, config, &mut diagnostics);

        // Emit DDL definition events (tables/views and their columns).
        if observer.wants_definitions() {
            let reader = DdlReader::new(erased, self.dialect.roles());
            if let Some((table_name, table_range)) = reader.name_span(root_id) {
                observer.on_relation_definition(&table_name, table_range);
                for (col_name, col_range) in reader.column_spans(root_id) {
                    observer.on_column_definition(&table_name, &col_name, col_range);
                }
            }
        }

        ValidationPass::run(
            erased,
            root_id,
            &self.dialect,
            &mut self.catalog,
            config,
            &mut diagnostics,
            observer,
        );

        let lineage =
            super::lineage::compute_lineage(erased, root_id, &self.catalog, self.dialect.roles());

        let defined_relations = extract_defined_relations(erased, root_id, self.dialect.roles());

        StatementModel::new(
            erased.text().as_str().to_owned(),
            diagnostics,
            lineage,
            defined_relations,
        )
    }

    /// If this statement is an import, resolve and analyze the imported module.
    fn handle_import(
        &mut self,
        erased: &AnyParsedStatement<'_>,
        root_id: AnyNodeId,
        config: &ValidationConfig,
        diagnostics: &mut Vec<Diagnostic>,
    ) {
        if self.resolver.is_none() {
            return;
        }

        let Some((tag, fields)) = erased.extract_fields(root_id) else {
            return;
        };
        let idx = u32::from(tag) as usize;
        let Some(&role) = self.dialect.roles().get(idx) else {
            return;
        };
        let SemanticRole::Import { module } = role else {
            return;
        };

        let module_name = match fields[module as usize] {
            FieldValue::Span(sp) if !sp.is_empty() => erased.span_expanded_text(sp).to_string(),
            _ => return,
        };

        // Dedup / cycle detection.
        if !self.imported.insert(module_name.clone()) {
            return;
        }

        let Some(source) = self.resolver.as_ref().and_then(|r| r.resolve(&module_name)) else {
            let range = match fields[module as usize] {
                FieldValue::Span(sp) => erased.span_text_abs(sp).1,
                _ => DocRange::default(),
            };
            let message = DiagnosticMessage::UnknownModule { name: module_name };
            if let Some(severity) = config.checks().level_for(&message).to_severity() {
                diagnostics.push(Diagnostic::new(range, message, severity, None));
            }
            return;
        };

        // DDL accumulates into the Document layer (visible to subsequent
        // statements in the importing file).
        let _ = self.analyze_inner(&source, config, &mut NoopObserver);
    }
}

#[cfg(feature = "sqlite")]
impl Default for SemanticAnalyzer {
    fn default() -> Self {
        Self::new()
    }
}

// ── Tests ──────────────────────────────────────────────────────────────────────
//
// Input-to-diagnostic behavior for the SQLite dialect is covered by the
// declarative suite in `tests/semantic_diff_tests/`. Unit tests here cover
// internal data structures (QueryScope, module-import dedup) that the CLI
// cannot observe directly.

#[cfg(test)]
mod tests {
    use super::super::catalog::ColumnResolution;
    use super::query_scope::{QueryScope, RowIdPolicy};
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

    #[test]
    fn unknown_columns_table_blocks_outer_scope_leaking() {
        let mut scope = QueryScope::default();
        scope.push();
        scope.add_table(
            "a",
            Some(vec!["id".into(), "name".into()]),
            RowIdPolicy::WithRowId,
        );
        scope.push();
        scope.add_table("users", None, RowIdPolicy::WithRowId);

        let res = scope.resolve_column(None, "name");
        match res {
            ColumnResolution::Found { ref table, .. } => {
                assert_ne!(table, "a");
            }
            _ => panic!("expected Found, got {res:?}"),
        }

        scope.pop();
        scope.pop();
    }

    #[test]
    fn anonymous_source_columns_resolve_unqualified() {
        let mut scope = QueryScope::default();
        scope.push();
        scope.add_anonymous(Some(vec!["x".into(), "y".into()]));

        let res = scope.resolve_column(None, "x");
        assert!(matches!(res, ColumnResolution::Found { .. }));

        let res = scope.resolve_column(None, "missing");
        assert!(matches!(res, ColumnResolution::NotFound));

        scope.pop();
    }

    #[test]
    fn anonymous_source_unknown_columns_blocks_leaking() {
        let mut scope = QueryScope::default();
        scope.push();
        scope.add_table("outer_tbl", Some(vec!["id".into()]), RowIdPolicy::WithRowId);
        scope.push();
        scope.add_anonymous(None);

        let res = scope.resolve_column(None, "anything");
        match res {
            ColumnResolution::Found { ref table, .. } => {
                assert_ne!(table, "outer_tbl");
            }
            _ => panic!("expected Found, got {res:?}"),
        }

        scope.pop();
        scope.pop();
    }

    #[test]
    fn anonymous_source_not_in_qualified_lookup() {
        let mut scope = QueryScope::default();
        scope.push();
        scope.add_anonymous(Some(vec!["x".into()]));

        let res = scope.resolve_column(Some("sq"), "x");
        assert!(matches!(res, ColumnResolution::TableNotFound));

        scope.pop();
    }

    struct MapResolver(HashMap<String, String>);

    impl super::super::ModuleResolver for MapResolver {
        fn resolve(&self, module_path: &str) -> Option<String> {
            self.0.get(module_path).cloned()
        }
    }

    #[test]
    fn module_resolver_with_analyzer_does_not_panic() {
        let resolver = MapResolver(HashMap::new());
        let mut analyzer = sqlite_analyzer().with_module_resolver(Box::new(resolver));
        let catalog = sqlite_catalog();
        let model = analyzer.analyze("SELECT 1", &catalog, &lenient());
        assert!(!model.has_diagnostics());
    }

    #[test]
    fn module_import_dedup_tracking() {
        let resolver = MapResolver(HashMap::new());
        let mut analyzer = sqlite_analyzer().with_module_resolver(Box::new(resolver));
        assert!(analyzer.imported.insert("test.module".to_string()));
        assert!(!analyzer.imported.insert("test.module".to_string()));
    }
}
