// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `parse` subcommand.

use std::io;
use std::ops::Deref;

use syntaqlite::Diagnostic;
use syntaqlite::analysis::{DiagnosticMessage, Severity};
use syntaqlite::any::{AnyDialect, AnyParser, ParseOutcome};
use syntaqlite::source::{DocOffset, DocRange, StmtRange};
use syntaqlite::util::DiagnosticRenderer;
use syntaqlite_syntax::any::AnyDialect as SyntaxAnyDialect;
use syntaqlite_syntax::typed::TypedParsedStatement;

use crate::cli::{ParseArgs, ParseOutput};
use crate::util::{self, Source};

pub(crate) fn run(dialect: &AnyDialect, args: &ParseArgs) -> Result<(), String> {
    let sources = util::load_sources(&args.files, args.expression.as_deref())?;
    let mut sink = select_sink(args.output);
    let multi = sources.len() > 1;
    let mut total_errors: u64 = 0;

    for src in &sources {
        sink.on_source_start(src, multi);
        let errors = parse_source(dialect, src, args.lang, sink.as_mut());
        total_errors += errors.len() as u64;
        sink.on_source_end(errors.len() as u64);
        if !errors.is_empty() {
            DiagnosticRenderer::new(&src.text, &src.label)
                .render_diagnostics(&errors, &mut io::stderr())
                .ok();
        }
    }

    sink.finish()?;
    if total_errors > 0 {
        Err(format!("{total_errors} syntax error(s)"))
    } else {
        Ok(())
    }
}

/// Drive the parser to exhaustion over a source, handing each successful
/// statement to the sink and collecting error diagnostics to render after the
/// loop. The source is split into embedded fragments (standalone SQL is one
/// whole-document fragment), and each fragment's error ranges are shifted into
/// host-file coordinates. The parser borrows from its own session buffer, so
/// sinks receive statements inside the loop rather than via a collected Vec.
fn parse_source(
    dialect: &AnyDialect,
    src: &Source,
    lang: Option<crate::cli::HostLanguage>,
    sink: &mut dyn Sink,
) -> Vec<Diagnostic> {
    let lang = util::resolve_language(lang, src, dialect);
    let mut errors = Vec::new();
    for fragment in syntaqlite::embedded::fragments(dialect.clone(), &src.text, lang) {
        let base = fragment.sql_range().start.as_usize();
        let parser = AnyParser::new(dialect.deref().clone());
        let mut session = parser.parse(fragment.sql_text());
        loop {
            match session.next() {
                ParseOutcome::Ok(stmt) => sink.on_stmt(StmtView { inner: stmt }),
                ParseOutcome::Err(err) => {
                    let start = err.offset().unwrap_or_default();
                    let length = err.length().unwrap_or_default();
                    let range =
                        StmtRange::from_offset_len(start, length).to_doc(err.statement_base());
                    errors.push(Diagnostic::new(
                        shift_range(range, base),
                        DiagnosticMessage::ParseError(err.message().to_string()),
                        Severity::Error,
                        None,
                    ));
                }
                ParseOutcome::Done => break,
            }
        }
    }
    errors
}

/// Shift a fragment-relative range into host-file coordinates.
fn shift_range(range: DocRange, base: usize) -> DocRange {
    if base == 0 {
        return range;
    }
    let shift =
        |o: DocOffset| DocOffset::from_raw(u32::try_from(o.as_usize() + base).unwrap_or(u32::MAX));
    DocRange {
        start: shift(range.start),
        end: shift(range.end),
    }
}

// ── StmtView ───────────────────────────────────────────────────────────────

/// Minimal handle over a parsed statement, exposing only the operations
/// sinks need. Shields sinks from the full [`TypedParsedStatement`] API.
struct StmtView<'a> {
    inner: TypedParsedStatement<'a, SyntaxAnyDialect>,
}

impl StmtView<'_> {
    fn dump(&self) -> String {
        let mut out = String::new();
        self.inner.dump(&mut out, 0);
        out
    }

    fn into_json(self) -> serde_json::Value {
        self.inner
            .erase()
            .root_node()
            .map_or(serde_json::Value::Null, |n| {
                serde_json::to_value(n).unwrap_or(serde_json::Value::Null)
            })
    }
}

// ── Sink strategy ──────────────────────────────────────────────────────────

/// Output strategy for parsed statements. Adding a new `--output` mode is a
/// new struct + `impl Sink`; the parse loop and top-level [`run`] don't need
/// to change.
trait Sink {
    fn on_source_start(&mut self, _src: &Source, _multi: bool) {}
    fn on_stmt(&mut self, stmt: StmtView<'_>);
    fn on_source_end(&mut self, _error_count: u64) {}
    fn finish(&mut self) -> Result<(), String>;
}

fn select_sink(output: ParseOutput) -> Box<dyn Sink> {
    match output {
        ParseOutput::Text => Box::new(TextSink::default()),
        ParseOutput::Json => Box::new(JsonSink::default()),
        ParseOutput::Summary => Box::new(SummarySink::default()),
    }
}

#[derive(Default)]
struct TextSink {
    stmts_this_source: u64,
}

impl Sink for TextSink {
    fn on_source_start(&mut self, src: &Source, multi: bool) {
        if multi && src.is_file() {
            println!("==> {} <==", src.label);
        }
        self.stmts_this_source = 0;
    }

    fn on_stmt(&mut self, stmt: StmtView<'_>) {
        if self.stmts_this_source > 0 {
            println!("----");
        }
        print!("{}", stmt.dump());
        self.stmts_this_source += 1;
    }

    fn finish(&mut self) -> Result<(), String> {
        Ok(())
    }
}

#[derive(Default)]
struct JsonSink {
    nodes: Vec<serde_json::Value>,
}

impl Sink for JsonSink {
    fn on_stmt(&mut self, stmt: StmtView<'_>) {
        self.nodes.push(stmt.into_json());
    }

    fn finish(&mut self) -> Result<(), String> {
        let out = serde_json::to_string_pretty(&self.nodes)
            .map_err(|e| format!("JSON serialization failed: {e}"))?;
        println!("{out}");
        Ok(())
    }
}

#[derive(Default)]
struct SummarySink {
    stmts: u64,
    errors: u64,
}

impl Sink for SummarySink {
    fn on_stmt(&mut self, _stmt: StmtView<'_>) {
        self.stmts += 1;
    }

    fn on_source_end(&mut self, error_count: u64) {
        self.errors += error_count;
    }

    fn finish(&mut self) -> Result<(), String> {
        println!("{} statements parsed, {} errors", self.stmts, self.errors);
        Ok(())
    }
}
