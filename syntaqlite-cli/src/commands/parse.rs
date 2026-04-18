// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `parse` subcommand.

use std::io;
use std::ops::Deref;

use syntaqlite::Diagnostic;
use syntaqlite::any::{AnyDialect, AnyParser, ParseOutcome};
use syntaqlite::semantic::{DiagnosticMessage, Severity};
use syntaqlite::util::DiagnosticRenderer;

use crate::cli::{ParseArgs, ParseOutput};
use crate::util::{self, Source};

pub(crate) fn run(dialect: &AnyDialect, args: &ParseArgs) -> Result<(), String> {
    let sources = util::load_sources(&args.files, args.expression.as_deref())?;
    let mut runner = ParseRun::new(dialect, args.output);
    let multi = sources.len() > 1;
    for src in &sources {
        runner.feed(src, multi);
    }
    runner.finish()
}

struct ParseRun<'a> {
    dialect: &'a AnyDialect,
    output: ParseOutput,
    totals: Totals,
}

#[derive(Default)]
struct Totals {
    statements: u64,
    errors: u64,
    json_nodes: Vec<serde_json::Value>,
}

impl<'a> ParseRun<'a> {
    fn new(dialect: &'a AnyDialect, output: ParseOutput) -> Self {
        Self {
            dialect,
            output,
            totals: Totals::default(),
        }
    }

    fn feed(&mut self, src: &Source, multi: bool) {
        if let ParseOutput::Text = self.output
            && multi
            && src.is_file()
        {
            println!("==> {} <==", src.label);
        }
        self.parse_one(src);
    }

    fn parse_one(&mut self, src: &Source) {
        let parser = AnyParser::new(self.dialect.deref().clone());
        let mut session = parser.parse(&src.text);
        let mut ast_out = String::new();
        let mut error_diags: Vec<Diagnostic> = Vec::new();
        let mut count: u64 = 0;

        loop {
            match session.next() {
                ParseOutcome::Ok(stmt) => {
                    match self.output {
                        ParseOutput::Text => {
                            if count > 0 {
                                ast_out.push_str("----\n");
                            }
                            stmt.dump(&mut ast_out, 0);
                        }
                        ParseOutput::Json => {
                            let val = stmt
                                .erase()
                                .root_node()
                                .map_or(serde_json::Value::Null, |n| {
                                    serde_json::to_value(n).unwrap_or(serde_json::Value::Null)
                                });
                            self.totals.json_nodes.push(val);
                        }
                        ParseOutput::Summary => {}
                    }
                    count += 1;
                }
                ParseOutcome::Err(err) => {
                    let start = err.offset().unwrap_or(0);
                    let end = start + err.length().unwrap_or(0);
                    error_diags.push(Diagnostic::new(
                        start,
                        end,
                        DiagnosticMessage::ParseError(err.message().to_string()),
                        Severity::Error,
                        None,
                    ));
                }
                ParseOutcome::Done => break,
            }
        }

        if let ParseOutput::Text = self.output {
            print!("{ast_out}");
        }

        self.totals.statements += count;
        self.totals.errors += error_diags.len() as u64;
        if !error_diags.is_empty() {
            DiagnosticRenderer::new(&src.text, &src.label)
                .render_diagnostics(&error_diags, &mut io::stderr())
                .ok();
        }
    }

    fn finish(self) -> Result<(), String> {
        match self.output {
            ParseOutput::Summary => {
                println!(
                    "{} statements parsed, {} errors",
                    self.totals.statements, self.totals.errors
                );
            }
            ParseOutput::Json => {
                println!(
                    "{}",
                    serde_json::to_string_pretty(&self.totals.json_nodes)
                        .map_err(|e| format!("JSON serialization failed: {e}"))?
                );
            }
            ParseOutput::Text => {}
        }

        if self.totals.errors > 0 {
            Err(format!("{} syntax error(s)", self.totals.errors))
        } else {
            Ok(())
        }
    }
}
