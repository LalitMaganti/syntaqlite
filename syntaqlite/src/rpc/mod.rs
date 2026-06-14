// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Transport-agnostic JSON-RPC dispatch.
//!
//! The single source of truth for per-request handling, backing both the
//! CLI's `serve json` stdio loop and the in-process C API. Owns the reused
//! state (parser, tokenizer, analyzer, formatter cache) and exposes a pure
//! request→response dispatch with no I/O or framing.
//!
//! ## Protocol
//!
//! Each request is one JSON object with an `"op"` field. Responses are
//! `{"ok":true,"result":<value>}` on success and `{"ok":false,"error":<string>}`
//! on failure. The `"quit"` op is a transport concern and is **not** handled
//! here (the CLI stdio loop and the FFI session lifetime own teardown).
//!
//! Ops:
//!
//! - `{"op":"parse","sql":"..."}` →
//!   `{"statements":[<AST JSON>,...],"errors":[<diag>,...]}`.
//! - `{"op":"format","sql":"...","line_width"?:int,"indent_width"?:int,"keyword_case"?:"upper"|"lower","semicolons"?:bool}` →
//!   `{"formatted":"..."}`.
//! - `{"op":"tokenize","sql":"..."}` →
//!   `{"tokens":[{"text","offset","length","type","category"},...]}`.
//! - `{"op":"analyze","sql":"...","tables"?:[...],"views"?:[...],"schema_ddl"?:"...","output"?:"structured"|"text"}` →
//!   `{"diagnostics":[...],"statements":[...],"lineage":<lineage>|null}` (or `{"rendered":"..."}` when `output="text"`).
//!
//! Response values are built from borrowed `Serialize` views where possible
//! so the hot paths don't allocate intermediate `Vec<Value>` trees.

use std::collections::HashMap;
use std::ops::Deref;

use serde::{Deserialize, Serialize, ser::SerializeSeq};
use serde_json::Value;
use syntaqlite_syntax::any::{AnyParser, AnyTokenizer, ParseOutcome};

use crate::analysis::{
    Analysis, CatalogLayer, DiagnosticMessage, Help, ModuleResolver, RelationKind,
    Severity as SemSeverity, StatementAnalysis,
};
use crate::any::AnyDialect;
use crate::fmt::KeywordCase;
use crate::util::DiagnosticRenderer;
use crate::{
    AnalysisConfig, AnalysisContext, Analyzer, Catalog, Diagnostic, FormatConfig, Formatter,
};

/// JSON-RPC C API (`include/syntaqlite/rpc.h`).
pub(crate) mod ffi;

/// A long-lived RPC session bundling the workers reused across requests.
///
/// Parser, tokenizer, and analyzer are stateless between requests; the
/// formatter cache keeps the per-config [`Formatter`] alive so its internal
/// arenas and render buffers survive across format calls with the same
/// config.
///
/// A session is single-threaded by design — it is **not** safe to issue
/// concurrent [`handle_request`] / [`call_json`] calls against one session
/// from multiple threads.
pub struct RpcSession {
    parser: AnyParser,
    tokenizer: AnyTokenizer,
    analyzer: Analyzer,
    formatter_cache: Option<(FormatConfig, Formatter)>,
    dialect: AnyDialect,
}

impl RpcSession {
    /// Construct a session bound to `dialect`, building the workers exactly
    /// as the CLI's `serve json` loop does.
    pub fn new(dialect: &AnyDialect) -> Self {
        let parser = AnyParser::new(dialect.deref().clone());
        let tokenizer = AnyTokenizer::new(dialect.deref().clone());
        let analyzer = Analyzer::with_dialect(dialect.clone());
        RpcSession {
            parser,
            tokenizer,
            analyzer,
            formatter_cache: None,
            dialect: dialect.clone(),
        }
    }
}

/// Dispatch a single parsed request against `session`, returning the
/// serialized `result` value on success or an error string on failure.
///
/// The `"quit"` op is intentionally not handled here — it is a transport
/// concern owned by the caller.
///
/// # Errors
///
/// Returns `Err(String)` when the op is unknown, the request shape is
/// invalid, or the underlying parse/format/analyze step fails.
pub fn handle_request(session: &mut RpcSession, req: &Value) -> Result<Value, String> {
    let Some(op) = req.get("op").and_then(Value::as_str) else {
        return Err("missing \"op\" field".to_string());
    };
    match op {
        "parse" => {
            let resp = handle_parse(&session.parser, req)?;
            serde_json::to_value(&resp).map_err(|e| format!("serialize: {e}"))
        }
        "format" => {
            let resp = handle_format(&session.dialect, &mut session.formatter_cache, req)?;
            serde_json::to_value(&resp).map_err(|e| format!("serialize: {e}"))
        }
        "tokenize" => {
            let resp = handle_tokenize(&session.dialect, &session.tokenizer, req)?;
            serde_json::to_value(&resp).map_err(|e| format!("serialize: {e}"))
        }
        "analyze" => handle_analyze(&session.dialect, &mut session.analyzer, req),
        other => Err(format!("unknown op: {other}")),
    }
}

/// Parse a single JSON request string, dispatch it, and serialize the
/// framed response (`{"ok":true,"result":..}` or `{"ok":false,"error":..}`).
///
/// This is the exact byte-shape the Python client's transport parses, and it
/// never returns an `Err` — all failures (including JSON parse errors) are
/// folded into an error envelope.
pub fn call_json(session: &mut RpcSession, request_json: &str) -> String {
    let req: Value = match serde_json::from_str(request_json) {
        Ok(v) => v,
        Err(e) => return err_frame(&format!("invalid JSON: {e}")),
    };
    match handle_request(session, &req) {
        Ok(result) => {
            let frame = OkFrame {
                ok: true,
                result: &result,
            };
            serde_json::to_string(&frame).unwrap_or_else(|e| err_frame(&format!("serialize: {e}")))
        }
        Err(msg) => err_frame(&msg),
    }
}

/// Serialize an error envelope for `msg`, falling back to a hardcoded string
/// if serialization itself fails (e.g. an embedded NUL — impossible for our
/// inputs, but keeps this infallible).
fn err_frame(msg: &str) -> String {
    let frame = ErrFrame {
        ok: false,
        error: msg,
    };
    serde_json::to_string(&frame)
        .unwrap_or_else(|_| r#"{"ok":false,"error":"failed to serialize error"}"#.to_string())
}

// ── framing ──────────────────────────────────────────────────────────────

#[derive(Serialize)]
struct OkFrame<'a, T: Serialize> {
    ok: bool,
    result: &'a T,
}

#[derive(Serialize)]
struct ErrFrame<'a> {
    ok: bool,
    error: &'a str,
}

// ── parse ────────────────────────────────────────────────────────────────

#[derive(Deserialize)]
struct ParseReq {
    sql: String,
}

#[derive(Serialize)]
struct ParseResp {
    statements: Vec<Value>,
    errors: Vec<ParseErrView>,
}

#[derive(Serialize)]
struct ParseErrView {
    severity: &'static str,
    message: String,
    start_offset: usize,
    end_offset: usize,
    code: u32,
}

fn handle_parse(parser: &AnyParser, req: &Value) -> Result<ParseResp, String> {
    let req: ParseReq = serde_json::from_value(req.clone()).map_err(|e| format!("parse: {e}"))?;
    let mut session = parser.parse(&req.sql);
    // Statements are buffered as `Value` because they borrow from the session
    // arena and we can't serialize them lazily while also iterating `session`.
    let mut statements: Vec<Value> = Vec::new();
    let mut errors: Vec<ParseErrView> = Vec::new();
    loop {
        match session.next() {
            ParseOutcome::Ok(stmt) => {
                let erased = stmt.erase();
                let node = erased
                    .root_node()
                    .and_then(|n| serde_json::to_value(n).ok())
                    .unwrap_or(Value::Null);
                statements.push(node);
            }
            ParseOutcome::Err(err) => {
                let base = err.statement_base();
                let stmt_offset = err.offset().unwrap_or_default();
                let length = err.length().unwrap_or_default();
                let start = stmt_offset.to_doc(base).as_usize();
                errors.push(ParseErrView {
                    severity: "error",
                    message: err.message().to_string(),
                    start_offset: start,
                    end_offset: start + length.as_usize(),
                    code: DIAG_CODE_PARSE_ERROR,
                });
            }
            ParseOutcome::Done => break,
        }
    }
    Ok(ParseResp { statements, errors })
}

// ── format ───────────────────────────────────────────────────────────────

#[derive(Deserialize)]
struct FormatReq {
    sql: String,
    line_width: Option<usize>,
    indent_width: Option<usize>,
    keyword_case: Option<String>,
    semicolons: Option<bool>,
}

#[derive(Serialize)]
struct FormatResp {
    formatted: String,
}

fn handle_format(
    dialect: &AnyDialect,
    cache: &mut Option<(FormatConfig, Formatter)>,
    req: &Value,
) -> Result<FormatResp, String> {
    let req: FormatReq = serde_json::from_value(req.clone()).map_err(|e| format!("format: {e}"))?;
    let mut config = FormatConfig::default();
    if let Some(w) = req.line_width {
        config = config.with_line_width(w);
    }
    if let Some(w) = req.indent_width {
        config = config.with_indent_width(w);
    }
    if let Some(case) = req.keyword_case.as_deref() {
        let kc = match case {
            "upper" => KeywordCase::Upper,
            "lower" => KeywordCase::Lower,
            other => {
                return Err(format!(
                    "keyword_case must be 'upper' or 'lower', got {other:?}"
                ));
            }
        };
        config = config.with_keyword_case(kc);
    }
    if let Some(s) = req.semicolons {
        config = config.with_semicolons(s);
    }

    // Reuse the cached formatter when its config matches — its arenas and
    // render buffers survive across calls. On a config change (rare in
    // practice) rebuild once and cache the new one.
    let formatter = match cache {
        Some((cfg, f)) if format_config_eq(cfg, &config) => f,
        slot => {
            *slot = Some((
                config.clone(),
                Formatter::with_dialect_config(dialect.clone(), &config),
            ));
            &mut slot.as_mut().expect("just inserted").1
        }
    };
    formatter
        .format(&req.sql)
        .map(|formatted| FormatResp { formatted })
        .map_err(|e| format!("{e}"))
}

fn format_config_eq(a: &FormatConfig, b: &FormatConfig) -> bool {
    a.line_width() == b.line_width()
        && a.indent_width() == b.indent_width()
        && a.keyword_case() == b.keyword_case()
        && a.semicolons() == b.semicolons()
}

// ── tokenize ─────────────────────────────────────────────────────────────

#[derive(Serialize)]
struct TokenizeResp<'a> {
    tokens: TokensView<'a>,
}

/// Serializes the tokenizer iterator directly into a JSON array without
/// collecting into an intermediate `Vec`.
struct TokensView<'a> {
    dialect: &'a AnyDialect,
    tokenizer: &'a AnyTokenizer,
    sql: &'a str,
}

impl Serialize for TokensView<'_> {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let base = self.sql.as_ptr() as usize;
        // `size_hint` isn't available; emit without a known length.
        let mut seq = serializer.serialize_seq(None)?;
        for tok in self.tokenizer.tokenize(self.sql) {
            let text = tok.text();
            let offset = (text.as_ptr() as usize).saturating_sub(base);
            let length = text.len();
            let tt = tok.token_type();
            seq.serialize_element(&TokenView {
                text,
                offset,
                length,
                token_type: u32::from(tt),
                category: token_category_name(self.dialect.token_category(tt)),
            })?;
        }
        seq.end()
    }
}

#[derive(Serialize)]
struct TokenView<'a> {
    text: &'a str,
    offset: usize,
    length: usize,
    #[serde(rename = "type")]
    token_type: u32,
    category: &'static str,
}

fn handle_tokenize<'a>(
    dialect: &'a AnyDialect,
    tokenizer: &'a AnyTokenizer,
    req: &'a Value,
) -> Result<TokenizeResp<'a>, String> {
    let sql = req
        .get("sql")
        .and_then(Value::as_str)
        .ok_or_else(|| "tokenize: missing 'sql'".to_string())?;
    Ok(TokenizeResp {
        tokens: TokensView {
            dialect,
            tokenizer,
            sql,
        },
    })
}

fn token_category_name(c: syntaqlite_syntax::any::TokenCategory) -> &'static str {
    use syntaqlite_syntax::any::TokenCategory as T;
    match c {
        T::Keyword => "keyword",
        T::Identifier => "identifier",
        T::String => "string",
        T::Number => "number",
        T::Operator => "operator",
        T::Punctuation => "punctuation",
        T::Comment => "comment",
        T::Parameter => "parameter",
        T::Function => "function",
        T::Type => "type",
        T::Other => "other",
    }
}

// ── validate ─────────────────────────────────────────────────────────────

#[derive(Deserialize)]
struct TableRec {
    name: String,
    columns: Option<Vec<String>>,
}

#[derive(Deserialize)]
struct ViewRec {
    name: String,
    columns: Option<Vec<String>>,
}

#[derive(Deserialize)]
struct ValidateReq {
    sql: String,
    tables: Option<Vec<TableRec>>,
    views: Option<Vec<ViewRec>>,
    schema_ddl: Option<String>,
    /// Optional map of dotted module path → SQL source, used to resolve
    /// `INCLUDE MODULE` imports. Callers must supply every module they
    /// expect to be referenced (transitively).
    modules: Option<HashMap<String, String>>,
    /// Output format: `"structured"` (default) returns a typed result
    /// object; `"text"` returns rendered diagnostics.
    output: Option<String>,
    /// Per-call render options (only honoured when `output == "text"`).
    render_options: Option<RenderOptions>,
}

#[derive(Deserialize, Default)]
struct RenderOptions {
    /// Source label shown in rendered diagnostics (analogous to a file path).
    /// Defaults to the empty string.
    #[serde(default)]
    source_name: String,
}

/// Construct the [`ModuleResolver`] for this request, if any.
///
/// This is the single seam where we pick a resolver strategy. Today the
/// only strategy is [`HashMapResolver`] driven by the request's `modules`
/// field (eager dict). Future strategies (e.g. bidirectional callbacks)
/// plug in here.
fn build_module_resolver(req: &ValidateReq) -> Option<Box<dyn ModuleResolver>> {
    let modules = req.modules.as_ref()?;
    if modules.is_empty() {
        return None;
    }
    Some(Box::new(HashMapResolver(modules.clone())))
}

/// A `ModuleResolver` that looks up a dotted module path in an in-memory map.
struct HashMapResolver(HashMap<String, String>);

impl ModuleResolver for HashMapResolver {
    fn resolve(&self, module_path: &str) -> Option<String> {
        self.0.get(module_path).cloned()
    }
}

#[derive(Serialize)]
struct RenderedResp {
    rendered: String,
}

/// The full validate response. All child views borrow from the
/// `Analysis`, so serialization is a single walk with zero
/// intermediate `Vec<Value>` allocations.
#[derive(Serialize)]
struct ValidateResp<'a> {
    diagnostics: AllDiagnostics<'a>,
    statements: StatementsView<'a>,
    lineage: Option<LineageView<'a>>,
}

fn handle_analyze(
    dialect: &AnyDialect,
    analyzer: &mut Analyzer,
    req: &Value,
) -> Result<Value, String> {
    let req: ValidateReq =
        serde_json::from_value(req.clone()).map_err(|e| format!("validate: {e}"))?;

    let mut catalog = if let Some(ddl) = req.schema_ddl.as_deref() {
        let (cat, errs) = Catalog::from_ddl(dialect.clone(), &[ddl]);
        if !errs.is_empty() {
            return Err(format!("schema_ddl: {}", errs.join("; ")));
        }
        cat
    } else {
        Catalog::new(dialect.clone())
    };
    if let Some(tables) = req.tables.as_deref() {
        let layer = catalog.layer_mut(CatalogLayer::Database);
        for t in tables {
            layer.insert_table(t.name.clone(), t.columns.clone(), false);
        }
    }
    if let Some(views) = req.views.as_deref() {
        let layer = catalog.layer_mut(CatalogLayer::Database);
        for v in views {
            layer.insert_view(v.name.clone(), v.columns.clone());
        }
    }

    let config = AnalysisConfig::default();
    let resolver = build_module_resolver(&req);
    let mut ctx = AnalysisContext::new(&mut catalog).with_config(config);
    if let Some(r) = resolver.as_deref() {
        ctx = ctx.with_resolver(r);
    }
    let model = analyzer.analyze(&req.sql, &mut ctx);

    match req.output.as_deref().unwrap_or("structured") {
        "structured" => {} // fall through to the structured path below.
        "text" => {
            let opts = req.render_options.unwrap_or_default();
            let r = DiagnosticRenderer::new(&req.sql, &opts.source_name);
            let mut buf: Vec<u8> = Vec::new();
            for d in model.diagnostics() {
                r.render_diagnostic(d, &mut buf)
                    .map_err(|e| format!("render: {e}"))?;
            }
            // `from_utf8` reuses the buffer (no extra allocation); the
            // renderer only writes UTF-8 so this can't fail in practice.
            let text = String::from_utf8(buf).map_err(|e| format!("utf-8: {e}"))?;
            return serde_json::to_value(RenderedResp { rendered: text })
                .map_err(|e| format!("serialize: {e}"));
        }
        other => return Err(format!("unknown output format: {other:?}")),
    }

    // `Analysis.lineage` exposes the lineage of the final
    // query-bearing statement (matches the pre-CLI-RPC Python API).
    let top_lineage = model.statements().iter().rev().find_map(lineage_view);

    let resp = ValidateResp {
        diagnostics: AllDiagnostics { model: &model },
        statements: StatementsView { model: &model },
        lineage: top_lineage,
    };
    serde_json::to_value(&resp).map_err(|e| format!("serialize: {e}"))
}

// ── validate: borrowed views ─────────────────────────────────────────────

struct AllDiagnostics<'a> {
    model: &'a Analysis,
}

impl Serialize for AllDiagnostics<'_> {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let mut seq = serializer.serialize_seq(Some(self.model.diagnostic_count()))?;
        for d in self.model.diagnostics() {
            seq.serialize_element(&DiagView::from(d))?;
        }
        seq.end()
    }
}

struct StatementsView<'a> {
    model: &'a Analysis,
}

impl Serialize for StatementsView<'_> {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let stmts = self.model.statements();
        let mut seq = serializer.serialize_seq(Some(stmts.len()))?;
        for s in stmts {
            seq.serialize_element(&StatementView { inner: s })?;
        }
        seq.end()
    }
}

struct StatementView<'a> {
    inner: &'a StatementAnalysis,
}

impl Serialize for StatementView<'_> {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        use serde::ser::SerializeMap;
        let s = self.inner;
        let mut m = serializer.serialize_map(Some(4))?;
        m.serialize_entry("source", s.source())?;
        m.serialize_entry("diagnostics", &DiagsOfStatement(s))?;
        m.serialize_entry("defined_relations", &DefinedRelationsView(s))?;
        match lineage_view(s) {
            Some(l) => m.serialize_entry("lineage", &l)?,
            None => m.serialize_entry("lineage", &Value::Null)?,
        }
        m.end()
    }
}

struct DiagsOfStatement<'a>(&'a StatementAnalysis);

impl Serialize for DiagsOfStatement<'_> {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let diags = self.0.diagnostics();
        let mut seq = serializer.serialize_seq(Some(diags.len()))?;
        for d in diags {
            seq.serialize_element(&DiagView::from(d))?;
        }
        seq.end()
    }
}

struct DefinedRelationsView<'a>(&'a StatementAnalysis);

impl Serialize for DefinedRelationsView<'_> {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let rels = self.0.defined_relations();
        let mut seq = serializer.serialize_seq(Some(rels.len()))?;
        for d in rels {
            seq.serialize_element(&DefinedRelationRec {
                name: &d.name,
                is_view: d.is_view,
            })?;
        }
        seq.end()
    }
}

#[derive(Serialize)]
struct DefinedRelationRec<'a> {
    name: &'a str,
    is_view: bool,
}

// ── validate: diagnostic view ────────────────────────────────────────────

struct DiagView<'a> {
    inner: &'a Diagnostic,
}

impl<'a> From<&'a Diagnostic> for DiagView<'a> {
    fn from(inner: &'a Diagnostic) -> Self {
        Self { inner }
    }
}

impl Serialize for DiagView<'_> {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        use serde::ser::SerializeMap;
        let d = self.inner;
        let help = d.help().map(|h| match h {
            Help::Suggestion(s) => s.as_str(),
        });
        let len = if help.is_some() { 6 } else { 5 };
        let mut m = serializer.serialize_map(Some(len))?;
        m.serialize_entry(
            "severity",
            match d.severity() {
                SemSeverity::Error => "error",
                SemSeverity::Warning => "warning",
                SemSeverity::Info => "info",
                SemSeverity::Hint => "hint",
            },
        )?;
        m.serialize_entry("message", &d.message().to_string())?;
        m.serialize_entry("start_offset", &d.start().as_usize())?;
        m.serialize_entry("end_offset", &d.end().as_usize())?;
        m.serialize_entry("code", &diag_code(d.message()))?;
        if let Some(h) = help {
            m.serialize_entry("help", h)?;
        }
        m.end()
    }
}

// ── validate: lineage view ───────────────────────────────────────────────

struct LineageView<'a> {
    stmt: &'a StatementAnalysis,
}

fn lineage_view(s: &StatementAnalysis) -> Option<LineageView<'_>> {
    // `lineage()` returns None for non-query statements; bail out so the
    // JSON says `null` rather than an empty lineage record.
    s.lineage()?;
    Some(LineageView { stmt: s })
}

impl Serialize for LineageView<'_> {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        use serde::ser::SerializeMap;
        let s = self.stmt;
        let cols = s.lineage().expect("guarded by lineage_view");
        let rels = s
            .relations_accessed()
            .expect("relations_accessed present when lineage is present");
        let tables = s
            .physical_tables_accessed()
            .expect("physical_tables_accessed present when lineage is present");

        let mut m = serializer.serialize_map(Some(5))?;
        m.serialize_entry("complete", &cols.is_complete())?;
        m.serialize_entry("columns", &ColumnsSeq(cols.into_inner()))?;
        m.serialize_entry("relations", &RelationsSeq(rels.into_inner()))?;
        m.serialize_entry("physical_tables", &PhysicalTablesSeq(tables.into_inner()))?;
        m.serialize_entry(
            "unexpanded_views",
            &UnexpandedViewsSeq(s.unexpanded_views()),
        )?;
        m.end()
    }
}

struct ColumnsSeq<'a>(&'a [crate::analysis::ColumnLineage]);

impl Serialize for ColumnsSeq<'_> {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let mut seq = serializer.serialize_seq(Some(self.0.len()))?;
        for c in self.0 {
            let origin = c.origin.as_ref().map(|o| OriginRec {
                table: &o.table,
                column: &o.column,
            });
            seq.serialize_element(&ColumnRec {
                name: &c.name,
                index: c.index,
                origin,
            })?;
        }
        seq.end()
    }
}

#[derive(Serialize)]
struct ColumnRec<'a> {
    name: &'a str,
    index: u32,
    origin: Option<OriginRec<'a>>,
}

#[derive(Serialize)]
struct OriginRec<'a> {
    table: &'a str,
    column: &'a str,
}

struct RelationsSeq<'a>(&'a [crate::analysis::RelationAccess]);

impl Serialize for RelationsSeq<'_> {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let mut seq = serializer.serialize_seq(Some(self.0.len()))?;
        for r in self.0 {
            seq.serialize_element(&RelationRec {
                name: &r.name,
                kind: match r.kind {
                    RelationKind::Table => "table",
                    RelationKind::View => "view",
                },
            })?;
        }
        seq.end()
    }
}

#[derive(Serialize)]
struct RelationRec<'a> {
    name: &'a str,
    kind: &'static str,
}

struct PhysicalTablesSeq<'a>(&'a [crate::analysis::PhysicalTableAccess]);

impl Serialize for PhysicalTablesSeq<'_> {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let mut seq = serializer.serialize_seq(Some(self.0.len()))?;
        for t in self.0 {
            seq.serialize_element(t.name.as_str())?;
        }
        seq.end()
    }
}

struct UnexpandedViewsSeq<'a>(&'a [String]);

impl Serialize for UnexpandedViewsSeq<'_> {
    fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        let mut seq = serializer.serialize_seq(Some(self.0.len()))?;
        for v in self.0 {
            seq.serialize_element(v.as_str())?;
        }
        seq.end()
    }
}

// Keep in sync with `syntaqlite/src/analysis/ffi/codes.rs` — Python maps
// these back via `DiagnosticCode(d["code"])`.
const DIAG_CODE_PARSE_ERROR: u32 = 0;
const DIAG_CODE_UNKNOWN_TABLE: u32 = 1;
const DIAG_CODE_UNKNOWN_COLUMN: u32 = 2;
const DIAG_CODE_UNKNOWN_FUNCTION: u32 = 3;
const DIAG_CODE_UNKNOWN_MODULE: u32 = 4;
const DIAG_CODE_FUNCTION_ARITY: u32 = 5;
const DIAG_CODE_CTE_COLUMN_COUNT_MISMATCH: u32 = 6;

fn diag_code(msg: &DiagnosticMessage) -> u32 {
    match msg {
        DiagnosticMessage::ParseError(_) => DIAG_CODE_PARSE_ERROR,
        DiagnosticMessage::UnknownTable { .. } => DIAG_CODE_UNKNOWN_TABLE,
        DiagnosticMessage::UnknownColumn { .. } => DIAG_CODE_UNKNOWN_COLUMN,
        DiagnosticMessage::UnknownFunction { .. } => DIAG_CODE_UNKNOWN_FUNCTION,
        DiagnosticMessage::UnknownModule { .. } => DIAG_CODE_UNKNOWN_MODULE,
        DiagnosticMessage::FunctionArity { .. } => DIAG_CODE_FUNCTION_ARITY,
        DiagnosticMessage::CteColumnCountMismatch { .. } => DIAG_CODE_CTE_COLUMN_COUNT_MISMATCH,
    }
}
