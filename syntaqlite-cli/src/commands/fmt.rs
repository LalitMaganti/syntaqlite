// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `fmt` subcommand.

use std::fs;
use std::io;

use syntaqlite::Diagnostic;
use syntaqlite::analysis::{DiagnosticMessage, Severity};
use syntaqlite::any::AnyDialect;
use syntaqlite::fmt::FormatError;
use syntaqlite::util::DiagnosticRenderer;
use syntaqlite::{FormatConfig, Formatter};

use crate::cli::{FmtArgs, FmtOutput};
use crate::config::{self, ConfigMode};
use crate::util::{self, Source};

pub(crate) fn run(
    dialect: &AnyDialect,
    config_mode: &ConfigMode<'_>,
    args: &FmtArgs,
) -> Result<(), String> {
    let runner = FmtRun::new(dialect, config_mode, args);
    let sources = util::load_sources(&args.files, args.expression.as_deref())?;
    runner.run(&sources)
}

struct FmtRun<'a> {
    dialect: &'a AnyDialect,
    config: FormatConfig,
    output: FmtOutput,
    in_place: bool,
    check: bool,
}

impl<'a> FmtRun<'a> {
    fn new(dialect: &'a AnyDialect, config_mode: &ConfigMode<'_>, args: &FmtArgs) -> Self {
        let file_config = config::resolve(config_mode);
        let config = util::build_format_config(
            file_config.as_ref().map(|(c, _)| &c.format),
            args.line_width,
            args.indent_width,
            args.keyword_case,
            args.semicolons,
        );
        Self {
            dialect,
            config,
            output: args.output,
            in_place: args.in_place,
            check: args.check,
        }
    }

    fn run(&self, sources: &[Source]) -> Result<(), String> {
        // Debug output modes bypass the normal format-and-write pipeline.
        if let FmtOutput::Bytecode | FmtOutput::DocTree = self.output {
            return self.dump_debug(sources);
        }
        self.format_all(sources)
    }

    fn format_all(&self, sources: &[Source]) -> Result<(), String> {
        let multi = sources.len() > 1;
        let mut errors = Vec::new();
        let mut unformatted = Vec::new();

        for src in sources {
            if !src.is_file() && (self.in_place || self.check) {
                let flag = if self.check { "check" } else { "in-place" };
                return Err(format!("--{flag} requires file arguments"));
            }

            match self.format_text(&src.text) {
                Ok(out) => self.write_output(src, &out, multi, &mut unformatted)?,
                Err(e) => {
                    render_format_error(&e, &src.text, &src.label);
                    errors.push(format!("{}: {e}", src.label));
                }
            }
        }

        if !errors.is_empty() {
            return Err(errors.join("\n"));
        }
        if !unformatted.is_empty() {
            for f in &unformatted {
                eprintln!("would reformat {f}");
            }
            return Err(format!(
                "{} file(s) would be reformatted",
                unformatted.len()
            ));
        }
        Ok(())
    }

    fn write_output(
        &self,
        src: &Source,
        formatted: &str,
        multi: bool,
        unformatted: &mut Vec<String>,
    ) -> Result<(), String> {
        let Some(path) = src.path.as_ref() else {
            // stdin / -e: always print to stdout.
            print!("{formatted}");
            return Ok(());
        };

        if self.check {
            if formatted != src.text {
                unformatted.push(src.label.clone());
            }
        } else if self.in_place {
            if formatted != src.text {
                fs::write(path, formatted).map_err(|e| format!("{}: {e}", path.display()))?;
                eprintln!("formatted {}", path.display());
            }
        } else {
            if multi {
                println!("==> {} <==", src.label);
            }
            print!("{formatted}");
        }
        Ok(())
    }

    fn dump_debug(&self, sources: &[Source]) -> Result<(), String> {
        let mut formatter = Formatter::with_dialect_config(self.dialect.clone(), &self.config);
        let multi = sources.len() > 1;
        for src in sources {
            if multi && src.is_file() {
                println!("==> {} <==", src.label);
            }
            let dump = match self.output {
                FmtOutput::Bytecode => formatter
                    .dump_bytecode(&src.text)
                    .map_err(|e| e.to_string())?,
                FmtOutput::DocTree => formatter
                    .dump_doc_tree(&src.text)
                    .map_err(|e| e.to_string())?,
                FmtOutput::Formatted => unreachable!(),
            };
            print!("{dump}");
        }
        Ok(())
    }

    fn format_text(&self, source: &str) -> Result<String, FormatError> {
        Formatter::with_dialect_config(self.dialect.clone(), &self.config).format(source)
    }
}

fn render_format_error(e: &FormatError, source: &str, file: &str) {
    let range = e.range().unwrap_or_default();
    let diag = Diagnostic::new(
        range,
        DiagnosticMessage::ParseError(e.message().to_owned()),
        Severity::Error,
        None,
    );
    DiagnosticRenderer::new(source, file)
        .render_diagnostic(&diag, &mut io::stderr())
        .ok();
}
