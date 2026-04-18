// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! CLI introspection commands. Currently just `tokenize`.

use std::io::{self, IsTerminal, Read};
use std::path::PathBuf;

use serde::Serialize;

use syntaqlite::any::{AnyDialect, AnyTokenizer};

use crate::IntrospectOutput;

const SCHEMA_VERSION: u32 = 0;

// ---------------------------------------------------------------------------
// tokenize
// ---------------------------------------------------------------------------

#[derive(Serialize)]
struct TokenRecord<'a> {
    kind: &'static str,
    schema_version: u32,
    file: &'a str,
    text: &'a str,
    offset: usize,
    length: usize,
    #[serde(rename = "type")]
    token_type: u32,
    category: &'static str,
}

pub(crate) fn cmd_tokenize(
    dialect: &AnyDialect,
    files: &[String],
    expression: Option<&str>,
    output: IntrospectOutput,
) -> Result<(), String> {
    let tokenizer = AnyTokenizer::new((**dialect).clone());

    let emit = |source: &str, file: &str| {
        let base = source.as_ptr() as usize;
        for tok in tokenizer.tokenize(source) {
            let text = tok.text();
            let offset = (text.as_ptr() as usize).saturating_sub(base);
            let length = text.len();
            let tt = tok.token_type();
            let category = category_name(dialect.token_category(tt));
            match output {
                IntrospectOutput::Text => {
                    println!(
                        "{offset:>6}..{end:<6} {category:<12} {text}",
                        end = offset + length,
                    );
                }
                IntrospectOutput::Json => {
                    let rec = TokenRecord {
                        kind: "token",
                        schema_version: SCHEMA_VERSION,
                        file,
                        text,
                        offset,
                        length,
                        token_type: tt.into(),
                        category,
                    };
                    match serde_json::to_string(&rec) {
                        Ok(s) => println!("{s}"),
                        Err(e) => eprintln!("error serializing token: {e}"),
                    }
                }
            }
        }
    };

    if let Some(expr) = expression {
        emit(expr, "<expression>");
        return Ok(());
    }

    if files.is_empty() {
        let source = read_stdin()?;
        emit(&source, "<stdin>");
        return Ok(());
    }

    let mut paths: Vec<PathBuf> = Vec::new();
    for pat in files {
        let matches: Vec<_> = glob::glob(pat)
            .map_err(|e| format!("bad glob pattern {pat:?}: {e}"))?
            .collect();
        if matches.is_empty() {
            return Err(format!("no files matched: {pat}"));
        }
        for entry in matches {
            let path = entry.map_err(|e| format!("glob error: {e}"))?;
            if path.is_file() {
                paths.push(path);
            }
        }
    }
    let multi = paths.len() > 1;
    for path in &paths {
        let file = path.display().to_string();
        if multi && matches!(output, IntrospectOutput::Text) {
            println!("==> {file} <==");
        }
        let source = std::fs::read_to_string(path).map_err(|e| format!("{file}: {e}"))?;
        emit(&source, &file);
    }
    Ok(())
}

fn category_name(c: syntaqlite_syntax::any::TokenCategory) -> &'static str {
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

fn read_stdin() -> Result<String, String> {
    if io::stdin().is_terminal() {
        eprintln!("reading from stdin; paste SQL then press Ctrl-D");
    }
    let mut buf = String::new();
    io::stdin()
        .read_to_string(&mut buf)
        .map_err(|e| format!("reading stdin: {e}"))?;
    Ok(buf)
}
