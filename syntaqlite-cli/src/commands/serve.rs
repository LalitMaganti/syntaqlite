// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `serve` subcommand — long-lived JSON RPC server over stdio.
//!
//! The Python bindings spawn `syntaqlite serve` once and multiplex all
//! parse/format/tokenize/validate calls over a single subprocess, amortizing
//! process startup and dialect initialization across a session.
//!
//! ## Protocol
//!
//! On startup, the server writes `READY\n` to stdout, then reads requests in
//! a loop until the client sends `{"op":"quit"}` or closes stdin.
//!
//! **Request:** one JSON object per line (newline-terminated, no embedded newlines).
//! **Response:** one JSON object per line — `{"ok":true,"result":<value>}` on
//! success, `{"ok":false,"error":<string>}` on failure.
//!
//! Ops:
//!
//! - `{"op":"parse","sql":"..."}` →
//!   `{"statements":[<AST JSON>,...],"errors":[<diag>,...]}`.
//!   AST shape matches `serde_json::to_value(AnyNode)` — see `syntaqlite parse -o json`.
//! - `{"op":"format","sql":"...","line_width"?:int,"indent_width"?:int,"keyword_case"?:"upper"|"lower","semicolons"?:bool}` →
//!   `{"formatted":"..."}`.
//! - `{"op":"tokenize","sql":"..."}` →
//!   `{"tokens":[{"text","offset","length","type","category"},...]}`.
//! - `{"op":"validate","sql":"...","tables"?:[...],"views"?:[...],"schema_ddl"?:"...","render"?:bool}` →
//!   `{"diagnostics":[...],"statements":[...]}` (or `{"rendered":"..."}` when `render=true`).
//! - `{"op":"quit"}` → server exits cleanly without a response.

use std::io::{self, BufRead, BufReader, Write};
use std::ops::Deref;

use serde::Deserialize;
use serde_json::{Value, json};
use syntaqlite::any::{AnyDialect, AnyParser, AnyTokenizer, ParseOutcome};
use syntaqlite::fmt::KeywordCase;
use syntaqlite::semantic::{CatalogLayer, DiagnosticMessage, Help, Severity as SemSeverity};
use syntaqlite::util::DiagnosticRenderer;
use syntaqlite::{Catalog, FormatConfig, Formatter, SemanticAnalyzer, ValidationConfig};

pub(crate) fn run(dialect: &AnyDialect) -> Result<(), String> {
    let stdin = io::stdin();
    let stdout = io::stdout();
    let mut reader = BufReader::new(stdin.lock());
    let mut writer = io::BufWriter::new(stdout.lock());

    writeln!(writer, "READY").map_err(|e| format!("write READY: {e}"))?;
    writer.flush().map_err(|e| format!("flush READY: {e}"))?;

    // Shared parser/tokenizer/analyzer live for the whole session so we
    // don't re-build them per request.
    let parser = AnyParser::new(dialect.deref().clone());
    let tokenizer = AnyTokenizer::new(dialect.deref().clone());
    let mut analyzer = SemanticAnalyzer::with_dialect(dialect.clone());

    let mut line = String::new();
    loop {
        line.clear();
        let n = reader
            .read_line(&mut line)
            .map_err(|e| format!("read: {e}"))?;
        if n == 0 {
            return Ok(()); // EOF
        }
        let trimmed = line.trim_end_matches(['\n', '\r']);
        if trimmed.is_empty() {
            continue;
        }

        let req: Value = match serde_json::from_str(trimmed) {
            Ok(v) => v,
            Err(e) => {
                write_err(&mut writer, &format!("invalid JSON: {e}"))?;
                continue;
            }
        };
        let Some(op) = req.get("op").and_then(Value::as_str) else {
            write_err(&mut writer, "missing \"op\" field")?;
            continue;
        };

        if op == "quit" {
            return Ok(());
        }

        let result = match op {
            "parse" => handle_parse(&parser, &req),
            "format" => handle_format(dialect, &req),
            "tokenize" => handle_tokenize(dialect, &tokenizer, &req),
            "validate" => handle_validate(dialect, &mut analyzer, &req),
            other => Err(format!("unknown op: {other}")),
        };

        match result {
            Ok(value) => write_ok(&mut writer, &value)?,
            Err(msg) => write_err(&mut writer, &msg)?,
        }
    }
}

// ── framing ──────────────────────────────────────────────────────────────

fn write_ok<W: Write>(w: &mut W, result: &Value) -> Result<(), String> {
    let frame = json!({"ok": true, "result": result});
    serde_json::to_writer(&mut *w, &frame).map_err(|e| format!("write: {e}"))?;
    w.write_all(b"\n").map_err(|e| format!("write: {e}"))?;
    w.flush().map_err(|e| format!("flush: {e}"))
}

fn write_err<W: Write>(w: &mut W, msg: &str) -> Result<(), String> {
    let frame = json!({"ok": false, "error": msg});
    serde_json::to_writer(&mut *w, &frame).map_err(|e| format!("write: {e}"))?;
    w.write_all(b"\n").map_err(|e| format!("write: {e}"))?;
    w.flush().map_err(|e| format!("flush: {e}"))
}

// ── parse ────────────────────────────────────────────────────────────────

#[derive(Deserialize)]
struct ParseReq {
    sql: String,
}

fn handle_parse(parser: &AnyParser, req: &Value) -> Result<Value, String> {
    let req: ParseReq = serde_json::from_value(req.clone()).map_err(|e| format!("parse: {e}"))?;
    let mut session = parser.parse(&req.sql);
    let mut statements: Vec<Value> = Vec::new();
    let mut errors: Vec<Value> = Vec::new();
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
                let doc_start = stmt_offset.to_doc(base).as_usize();
                let doc_end = doc_start + length.as_usize();
                errors.push(json!({
                    "severity": "error",
                    "message": err.message().to_string(),
                    "start_offset": doc_start,
                    "end_offset": doc_end,
                    "code": DIAG_CODE_PARSE_ERROR,
                }));
            }
            ParseOutcome::Done => break,
        }
    }
    Ok(json!({"statements": statements, "errors": errors}))
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

fn handle_format(dialect: &AnyDialect, req: &Value) -> Result<Value, String> {
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

    let mut formatter = Formatter::with_dialect_config(dialect.clone(), &config);
    formatter
        .format(&req.sql)
        .map(|out| json!({"formatted": out}))
        .map_err(|e| format!("{e}"))
}

// ── tokenize ─────────────────────────────────────────────────────────────

#[derive(Deserialize)]
struct TokenizeReq {
    sql: String,
}

fn handle_tokenize(
    dialect: &AnyDialect,
    tokenizer: &AnyTokenizer,
    req: &Value,
) -> Result<Value, String> {
    let req: TokenizeReq =
        serde_json::from_value(req.clone()).map_err(|e| format!("tokenize: {e}"))?;
    let base = req.sql.as_ptr() as usize;
    let mut tokens: Vec<Value> = Vec::new();
    for tok in tokenizer.tokenize(&req.sql) {
        let text = tok.text();
        let offset = (text.as_ptr() as usize).saturating_sub(base);
        let length = text.len();
        let tt = tok.token_type();
        let category = token_category_name(dialect.token_category(tt));
        tokens.push(json!({
            "text": text,
            "offset": offset,
            "length": length,
            "type": u32::from(tt),
            "category": category,
        }));
    }
    Ok(json!({"tokens": tokens}))
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
    render: Option<bool>,
}

fn handle_validate(
    dialect: &AnyDialect,
    analyzer: &mut SemanticAnalyzer,
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

    let config = ValidationConfig::default();
    let model = analyzer.analyze(&req.sql, &catalog, &config);

    if req.render.unwrap_or(false) {
        let diags: Vec<_> = model.diagnostics().cloned().collect();
        let mut buf: Vec<u8> = Vec::new();
        DiagnosticRenderer::new(&req.sql, "<stdin>")
            .render_diagnostics(&diags, &mut buf)
            .map_err(|e| format!("render: {e}"))?;
        let rendered = String::from_utf8(buf).map_err(|e| format!("utf-8: {e}"))?;
        return Ok(json!({"rendered": rendered}));
    }

    let diagnostics: Vec<Value> = model.diagnostics().map(diag_to_value).collect();
    let statements: Vec<Value> = model
        .statements()
        .iter()
        .map(|s| {
            json!({
                "source": s.source(),
                "diagnostics": s.diagnostics().iter().map(diag_to_value).collect::<Vec<_>>(),
                "defined_relations": s
                    .defined_relations()
                    .iter()
                    .map(|d| json!({"name": d.name, "is_view": d.is_view}))
                    .collect::<Vec<_>>(),
            })
        })
        .collect();

    Ok(json!({
        "diagnostics": diagnostics,
        "statements": statements,
    }))
}

fn diag_to_value(d: &syntaqlite::Diagnostic) -> Value {
    let severity = match d.severity() {
        SemSeverity::Error => "error",
        SemSeverity::Warning => "warning",
        SemSeverity::Info => "info",
        SemSeverity::Hint => "hint",
    };
    let help = d.help().map(|h| match h {
        Help::Suggestion(s) => s.clone(),
    });
    let mut out = json!({
        "severity": severity,
        "message": d.message().to_string(),
        "start_offset": d.start().as_usize(),
        "end_offset": d.end().as_usize(),
        "code": diag_code(d.message()),
    });
    if let Some(h) = help {
        out["help"] = Value::String(h);
    }
    out
}

// Keep in sync with `syntaqlite/src/semantic/ffi/codes.rs` — Python maps
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
