// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `tokenize` subcommand.

use serde::Serialize;
use syntaqlite::any::{AnyDialect, AnyTokenizer};

use crate::cli::{IntrospectOutput, TokenizeArgs};
use crate::util::{self, Source};

const SCHEMA_VERSION: u32 = 0;

pub(crate) fn run(dialect: &AnyDialect, args: &TokenizeArgs) -> Result<(), String> {
    let tokenizer = AnyTokenizer::new((**dialect).clone());
    let sources = util::load_sources(&args.files, args.expression.as_deref())?;
    let mut sink = select_sink(args.output);
    let multi = sources.len() > 1;

    for src in &sources {
        sink.on_source_start(src, multi);
        // Standalone SQL is one whole-document fragment, so the same loop covers
        // both it and embedded host files.
        let lang = util::resolve_language(args.lang, src, dialect);
        for fragment in syntaqlite::embedded::fragments(dialect.clone(), &src.text, lang) {
            let base = fragment.sql_range().start.as_usize();
            emit_text(
                dialect,
                &tokenizer,
                src,
                fragment.sql_text(),
                base,
                sink.as_mut(),
            );
        }
    }
    Ok(())
}

/// Tokenize `text` and emit each token, shifting offsets by `base` (the text's
/// byte offset in the host file; `0` for standalone SQL).
fn emit_text(
    dialect: &AnyDialect,
    tokenizer: &AnyTokenizer,
    src: &Source,
    text: &str,
    base: usize,
    sink: &mut dyn Sink,
) {
    let ptr_base = text.as_ptr() as usize;
    for tok in tokenizer.tokenize(text) {
        let tok_text = tok.text();
        let offset = (tok_text.as_ptr() as usize).saturating_sub(ptr_base) + base;
        let length = tok_text.len();
        let tt = tok.token_type();
        sink.on_token(TokenView {
            src,
            text: tok_text,
            offset,
            length,
            token_type: tt.into(),
            category: category_name(dialect.token_category(tt)),
        });
    }
}

// ── TokenView ──────────────────────────────────────────────────────────────

/// Minimal view of a single tokenized lexeme handed to each sink.
struct TokenView<'a> {
    src: &'a Source,
    text: &'a str,
    offset: usize,
    length: usize,
    token_type: u32,
    category: &'static str,
}

// ── Sink strategy ──────────────────────────────────────────────────────────

/// Output strategy for emitted tokens. Adding a new `--output` mode is a
/// new struct + `impl Sink`.
trait Sink {
    fn on_source_start(&mut self, _src: &Source, _multi: bool) {}
    fn on_token(&mut self, tok: TokenView<'_>);
}

fn select_sink(output: IntrospectOutput) -> Box<dyn Sink> {
    match output {
        IntrospectOutput::Text => Box::new(TextSink),
        IntrospectOutput::Json => Box::new(JsonSink),
    }
}

struct TextSink;

impl Sink for TextSink {
    fn on_source_start(&mut self, src: &Source, multi: bool) {
        if multi && src.is_file() {
            println!("==> {} <==", src.label);
        }
    }

    fn on_token(&mut self, tok: TokenView<'_>) {
        let end = tok.offset + tok.length;
        println!(
            "{offset:>6}..{end:<6} {category:<12} {text}",
            offset = tok.offset,
            category = tok.category,
            text = tok.text,
        );
    }
}

struct JsonSink;

impl Sink for JsonSink {
    fn on_token(&mut self, tok: TokenView<'_>) {
        let rec = TokenRecord {
            kind: "token",
            schema_version: SCHEMA_VERSION,
            file: &tok.src.label,
            text: tok.text,
            offset: tok.offset,
            length: tok.length,
            token_type: tok.token_type,
            category: tok.category,
        };
        match serde_json::to_string(&rec) {
            Ok(s) => println!("{s}"),
            Err(e) => eprintln!("error serializing token: {e}"),
        }
    }
}

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
