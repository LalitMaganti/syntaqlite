// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Single-pass semantic analysis engine.

use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;

use syntaqlite_syntax::ParserConfig;
use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, AnyParser, FieldValue, ParseOutcome};
use syntaqlite_syntax::source::DocRange;
use syntaqlite_syntax::{MacroArg, MacroLookup, MacroOutput};

use crate::dialect::AnyDialect;
use crate::dialect::SemanticRole;

#[cfg(test)]
use super::catalog::Catalog;
use super::catalog::CatalogLayer;
use super::diagnostics::{Diagnostic, DiagnosticMessage};
use super::model::{Analysis, StatementAnalysis};
use super::{AnalysisContext, AnalysisMode};

use helpers::{extract_defined_relations, extract_macro_registration, parse_error_span};

mod helpers;
mod pass;
mod query_scope;
pub(crate) mod tokens;
pub(crate) mod walker;

use pass::{DiagnosticsPass, StatementWalkPass};
use query_scope::QueryScope;
use walker::{NoopWalkPass, WalkCtx, WalkPass, walk};

/// Stateless semantic analysis engine.
///
/// Holds only per-session policy (dialect, analysis mode, macro-fallback flag).
/// All per-call state (catalog, validation config, module resolver) is
/// bundled into an [`AnalysisContext`] the caller passes in by `&mut`. The
/// analyzer mutates the catalog in place (accumulating DDL, recording imports).
///
/// Set [`AnalysisMode::Execute`] via [`with_mode`](Self::with_mode) to make
/// DDL accumulate across calls (interactive session semantics).
///
/// # Example
///
/// ```
/// # use syntaqlite::{Catalog, Analyzer, AnalysisContext};
/// # use syntaqlite::analysis::{CatalogLayer, Severity};
/// // 1. Create analyzer (reusable across many inputs).
/// let mut analyzer = Analyzer::new();
///
/// // 2. Set up a catalog describing the database schema.
/// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
/// catalog
///     .layer_mut(CatalogLayer::Database)
///     .insert_table("users", Some(vec!["id".into(), "name".into()]), false);
///
/// // 3. Bundle the per-call state and analyze a query.
/// let mut ctx = AnalysisContext::new(&mut catalog);
/// let model = analyzer.analyze("SELECT id, name FROM users;", &mut ctx);
///
/// // 4. No diagnostics — the query is valid against the schema.
/// assert!(!model.has_diagnostics());
/// ```
pub struct Analyzer {
    dialect: AnyDialect,
    mode: AnalysisMode,
    macro_fallback: bool,
}

impl Analyzer {
    /// Create an analyzer for the built-in `SQLite` dialect.
    ///
    /// This is the most common entry point. The returned analyzer is ready to
    /// use with [`analyze`](Self::analyze). For custom or third-party dialects,
    /// use [`with_dialect`](Self::with_dialect) instead.
    ///
    /// # Example
    ///
    /// ```
    /// # use syntaqlite::Analyzer;
    /// let mut analyzer = Analyzer::new();
    /// ```
    #[cfg(feature = "sqlite")]
    pub fn new() -> Self {
        Self::with_dialect(crate::sqlite::dialect::dialect())
    }

    /// Create an analyzer bound to a specific dialect.
    pub fn with_dialect(dialect: impl Into<AnyDialect>) -> Self {
        Analyzer {
            dialect: dialect.into(),
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
    /// The caller owns the [`Catalog`] via the [`AnalysisContext`]; the
    /// analyzer mutates it in place — accumulating DDL into the Document layer,
    /// recording imports on the Database layer's import cache. The Document
    /// layer is cleared at the start of each call and rebuilt statement-by-
    /// statement so that DDL seen earlier in the file is visible to queries
    /// that follow it.
    ///
    /// In [`AnalysisMode::Execute`], DDL from this call is promoted to the
    /// Connection layer so it persists across subsequent calls.
    ///
    /// # Example
    ///
    /// ```
    /// # use syntaqlite::{AnalysisContext, Catalog, Analyzer};
    /// # use syntaqlite::analysis::{CatalogLayer, Severity};
    /// let mut analyzer = Analyzer::new();
    /// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
    /// catalog
    ///     .layer_mut(CatalogLayer::Database)
    ///     .insert_table("users", Some(vec!["id".into(), "name".into()]), false);
    ///
    /// let mut ctx = AnalysisContext::new(&mut catalog);
    ///
    /// // Referencing a column that does not exist produces a diagnostic.
    /// let model = analyzer.analyze("SELECT email FROM users;", &mut ctx);
    /// assert!(model.has_diagnostics());
    /// assert_eq!(model.diagnostics().next().unwrap().severity(), Severity::Warning);
    /// ```
    pub fn analyze(&mut self, source: &str, ctx: &mut AnalysisContext<'_>) -> Analysis {
        self.analyze_with_pass(source, ctx, &mut NoopWalkPass)
    }

    /// Run a complete single-pass analysis, forwarding walk events to
    /// `user_pass` alongside the built-in diagnostics pass.
    ///
    /// This is the entry point in-crate LSP and embedded-SQL consumers use to
    /// capture the data they need (go-to-definition, find-references, semantic
    /// tokens, completion). The analyzer always runs its own
    /// [`DiagnosticsPass`] in composition with `user_pass` via
    /// [`StatementWalkPass`].
    pub(crate) fn analyze_with_pass<P: WalkPass>(
        &mut self,
        source: &str,
        ctx: &mut AnalysisContext<'_>,
        user_pass: &mut P,
    ) -> Analysis {
        ctx.catalog.new_document();
        let model = self.analyze_inner(source, CatalogLayer::Document, ctx, user_pass);
        if self.mode == AnalysisMode::Execute {
            ctx.catalog.promote_document_to_connection();
        }
        model
    }

    fn analyze_inner<P: WalkPass>(
        &mut self,
        source: &str,
        ddl_target: CatalogLayer,
        ctx: &mut AnalysisContext<'_>,
        user_pass: &mut P,
    ) -> Analysis {
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

        let mut statements: Vec<StatementAnalysis> = Vec::new();

        loop {
            let stmt = match session.next() {
                ParseOutcome::Done => break,
                ParseOutcome::Ok(s) => s,
                ParseOutcome::Err(e) => {
                    let message = DiagnosticMessage::ParseError(e.message().to_owned());
                    if let Some(severity) = ctx.config().checks().level_for(&message).to_severity()
                    {
                        let range = parse_error_span(&e, source);
                        let diag = Diagnostic {
                            range,
                            message,
                            severity,
                            help: None,
                            expansion_frames: Vec::new(),
                        };
                        statements.push(StatementAnalysis::new(
                            String::new(),
                            vec![diag],
                            None,
                            Vec::new(),
                        ));
                    }
                    if P::WANTS_STATEMENT_CONTEXT {
                        user_pass.on_parse_error(&e);
                    }
                    continue;
                }
            };

            // Process the statement and extract macro registration info.
            // The erased statement borrows the session, so we must extract
            // owned macro data before dropping it and calling register_macro.
            let (stmt_model, macro_reg) = {
                let mut erased = stmt.erase();
                if P::WANTS_STATEMENT_CONTEXT {
                    user_pass.on_parsed_statement(&erased);
                }
                let model = self.analyze_statement(&mut erased, ddl_target, ctx, user_pass);
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

        Analysis {
            source: source.to_owned(),
            statements,
        }
    }

    fn analyze_statement<P: WalkPass>(
        &mut self,
        erased: &mut AnyParsedStatement<'_>,
        ddl_target: CatalogLayer,
        ctx: &mut AnalysisContext<'_>,
        user_pass: &mut P,
    ) -> StatementAnalysis {
        let root_id = erased.root_id();
        let mut diagnostics: Vec<Diagnostic> = Vec::new();

        ctx.catalog
            .accumulate_ddl(ddl_target, erased, root_id, &self.dialect);

        // Handle module imports: resolve and analyze imported source.
        self.handle_import(erased, root_id, ctx, &mut diagnostics);

        // Compose DiagnosticsPass + the user's pass for this statement, then
        // run the walker. DDL definition events (CREATE TABLE / VIEW names
        // and columns) are emitted by the walker itself.
        let roles = self.dialect.roles();
        let config = ctx.config;
        let mut stmt_pass = StatementWalkPass {
            diagnostics: DiagnosticsPass::new(&config, &mut diagnostics),
            extra: user_pass,
        };
        let mut cx = WalkCtx {
            roles,
            catalog: ctx.catalog,
            scope: QueryScope::default(),
        };
        walk(erased, &mut cx, &mut stmt_pass, root_id);

        let lineage = super::lineage::compute_lineage(erased, root_id, ctx.catalog, roles);

        let defined_relations = extract_defined_relations(erased, root_id, roles);

        StatementAnalysis::new(
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
        ctx: &mut AnalysisContext<'_>,
        diagnostics: &mut Vec<Diagnostic>,
    ) {
        let Some(resolver) = ctx.resolver else {
            return;
        };

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

        // Dedup against the catalog's import cache (Database layer).
        // Once imported, the DDL is present in the catalog; re-importing would
        // be redundant work and also guards against import cycles.
        if ctx.catalog.is_imported(&module_name) {
            return;
        }

        let Some(source) = resolver.resolve(&module_name) else {
            let range = match fields[module as usize] {
                FieldValue::Span(sp) => erased.span_text_abs(sp).1,
                _ => DocRange::default(),
            };
            let message = DiagnosticMessage::UnknownModule { name: module_name };
            if let Some(severity) = ctx.config.checks().level_for(&message).to_severity() {
                diagnostics.push(Diagnostic::new(range, message, severity, None));
            }
            return;
        };

        // Mark before recursing so cycles terminate. Imported DDL lands in
        // the Database layer so it shares a lifetime with the import cache.
        ctx.catalog.mark_imported(module_name);
        let _ = self.analyze_inner(&source, CatalogLayer::Database, ctx, &mut NoopWalkPass);
    }
}

#[cfg(feature = "sqlite")]
impl Default for Analyzer {
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

    fn sqlite_analyzer() -> Analyzer {
        Analyzer::new()
    }

    fn sqlite_catalog() -> Catalog {
        Catalog::new(crate::sqlite::dialect::dialect())
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

    impl crate::analysis::ModuleResolver for MapResolver {
        fn resolve(&self, module_path: &str) -> Option<String> {
            self.0.get(module_path).cloned()
        }
    }

    fn diag_messages(model: &SemanticModel) -> Vec<String> {
        model
            .diagnostics()
            .map(|d: &Diagnostic| match d.message() {
                DiagnosticMessage::UnknownTable { name } => format!("unknown table: {name}"),
                DiagnosticMessage::UnknownColumn { column, table } => match table {
                    Some(t) => format!("unknown column: {t}.{column}"),
                    None => format!("unknown column: {column}"),
                },
                DiagnosticMessage::UnknownFunction { name } => format!("unknown function: {name}"),
                other => format!("{other:?}"),
            })
            .collect()
    }

    #[test]
    fn unqualified_column_suppressed_when_source_unresolved() {
        let mut analyzer = sqlite_analyzer();
        let catalog = sqlite_catalog();
        let model = analyzer.analyze("SELECT x FROM does_not_exist", &catalog, &lenient());
        let msgs = diag_messages(&model);
        assert_eq!(
            msgs,
            vec!["unknown table: does_not_exist".to_string()],
            "bare column lookup must not FP when a FROM source failed to resolve",
        );
    }

    #[test]
    fn qualified_column_suppressed_when_source_unresolved() {
        let mut analyzer = sqlite_analyzer();
        let catalog = sqlite_catalog();
        let model = analyzer.analyze(
            "SELECT does_not_exist.x FROM does_not_exist",
            &catalog,
            &lenient(),
        );
        let msgs = diag_messages(&model);
        assert_eq!(
            msgs,
            vec!["unknown table: does_not_exist".to_string()],
            "qualified column whose qualifier is unresolved must not FP",
        );
    }

    #[test]
    fn mixed_scope_partial_resolution_suppresses_column_fp() {
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "known_tbl",
            Some(vec!["id".into()]),
            false,
        );
        let model = analyzer.analyze(
            "SELECT y FROM known_tbl JOIN missing_tbl ON 1=1",
            &catalog,
            &lenient(),
        );
        let msgs = diag_messages(&model);
        assert_eq!(
            msgs,
            vec!["unknown table: missing_tbl".to_string()],
            "bare `y` might live in missing_tbl — must not FP",
        );
    }

    // SQLite double-quoted-string (DQS) bug-compat: a `"foo"` in expression
    // position that doesn't resolve to a column is re-interpreted as a string
    // literal rather than rejected.  See https://www.sqlite.org/quirks.html.
    #[test]
    fn dqs_fallback_no_scope() {
        let mut analyzer = sqlite_analyzer();
        let catalog = sqlite_catalog();
        let model = analyzer.analyze("SELECT \"hello\"", &catalog, &lenient());
        assert_eq!(diag_messages(&model), Vec::<String>::new());
    }

    #[test]
    fn dqs_fallback_in_function_argument() {
        let mut analyzer = sqlite_analyzer();
        let catalog = sqlite_catalog();
        let model = analyzer.analyze(
            "SELECT sqlite_compileoption_used(\"THREADSAFE\")",
            &catalog,
            &lenient(),
        );
        assert_eq!(diag_messages(&model), Vec::<String>::new());
    }

    #[test]
    fn dqs_fallback_against_known_scope() {
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        catalog.layer_mut(CatalogLayer::Database).insert_table(
            "t1",
            Some(vec!["a".into(), "b".into()]),
            false,
        );
        let model = analyzer.analyze(
            "SELECT b FROM t1 WHERE a IN (\"hello\", 'there')",
            &catalog,
            &lenient(),
        );
        assert_eq!(diag_messages(&model), Vec::<String>::new());
    }

    #[test]
    fn dqs_does_not_fire_for_backtick_or_bracket_quotes() {
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        catalog
            .layer_mut(CatalogLayer::Database)
            .insert_table("t1", Some(vec!["a".into()]), false);
        // Backtick- and bracket-quoted identifiers are always identifiers —
        // no DQS fallback.  `nope` is a real unknown column ref here.
        let model = analyzer.analyze("SELECT `nope` FROM t1", &catalog, &lenient());
        assert_eq!(
            diag_messages(&model),
            vec!["unknown column: nope".to_string()],
        );
    }

    #[test]
    fn module_resolver_with_analyzer_does_not_panic() {
        let resolver = MapResolver(HashMap::new());
        let mut analyzer = sqlite_analyzer();
        let mut catalog = sqlite_catalog();
        let mut ctx = AnalysisContext::new(&mut catalog).with_resolver(&resolver);
        let model = analyzer.analyze("SELECT 1", &mut ctx);
        assert!(!model.has_diagnostics());
    }

    #[test]
    fn module_import_dedup_cached_on_catalog() {
        let mut catalog = sqlite_catalog();
        assert!(!catalog.is_imported("test.module"));
        catalog.mark_imported("test.module");
        assert!(catalog.is_imported("test.module"));
    }
}
