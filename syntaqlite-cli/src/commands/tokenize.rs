// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `tokenize` subcommand.

use serde::Serialize;
use syntaqlite::any::{AnyDialect, AnyTokenizer};

use crate::cli::{IntrospectOutput, TokenizeArgs};
use crate::util::{self, Source};

const SCHEMA_VERSION: u32 = 0;

pub(crate) fn run(dialect: &AnyDialect, args: &TokenizeArgs) -> Result<(), String> {
    let runner = TokenizeRun::new(dialect, args.output);
    let sources = util::load_sources(&args.files, args.expression.as_deref())?;
    runner.run(&sources);
    Ok(())
}

struct TokenizeRun<'a> {
    dialect: &'a AnyDialect,
    tokenizer: AnyTokenizer,
    output: IntrospectOutput,
}

impl<'a> TokenizeRun<'a> {
    fn new(dialect: &'a AnyDialect, output: IntrospectOutput) -> Self {
        Self {
            dialect,
            tokenizer: AnyTokenizer::new((**dialect).clone()),
            output,
        }
    }

    fn run(&self, sources: &[Source]) {
        let multi = sources.len() > 1;
        for src in sources {
            if let IntrospectOutput::Text = self.output
                && multi
                && src.is_file()
            {
                println!("==> {} <==", src.label);
            }
            self.emit_tokens(src);
        }
    }

    fn emit_tokens(&self, src: &Source) {
        let base = src.text.as_ptr() as usize;
        for tok in self.tokenizer.tokenize(&src.text) {
            let text = tok.text();
            let offset = (text.as_ptr() as usize).saturating_sub(base);
            let length = text.len();
            let tt = tok.token_type();
            let category = category_name(self.dialect.token_category(tt));
            match self.output {
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
                        file: &src.label,
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
