// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Measures how much of the authored token stream the AST can reproduce.
//!
//! The formatter prints from the AST. Any syntax the AST does not model is
//! therefore lost: an authored `ASC`, an implicit alias, a redundant paren.
//! This harness makes that loss measurable. For each statement it formats the
//! source and compares the two token streams; a difference is a construct the
//! AST cannot yet reproduce, and the name of that construct is the work item.
//!
//! Keyword casing is a rendering choice rather than lost information, so
//! keywords compare case-insensitively. Everything else compares exactly.
//!
//! Reads a corpus as JSON Lines on stdin or from a file argument, one
//! `{"name": ..., "sql": ...}` per line, and writes one JSON Lines record per
//! statement that does not round-trip. A summary goes to stderr.
//!
//! ```text
//! cargo run --release --features serde-json --example token_preservation -- corpus.jsonl
//! ```

use std::collections::{HashMap, HashSet};
use std::io::{BufRead, BufReader, Read, Write};

use syntaqlite::any::{AnyDialect, AnyTokenType, AnyTokenizer, TokenCategory};
use syntaqlite::util::{SqliteFlag, SqliteFlags, SqliteVersion};
use syntaqlite::{Formatter, sqlite_dialect};

/// One token as the oracle compares it.
struct Lexeme {
    text: String,
    keyword: bool,
}

impl Lexeme {
    /// Whether this lexeme is a quoted identifier or a literal, whose text the
    /// formatter emits from the source span verbatim.
    fn verbatim(&self) -> bool {
        matches!(
            self.text.as_bytes().first(),
            Some(b'"' | b'`' | b'[' | b'\'')
        )
    }

    /// Whether two lexemes are the same authored syntax.
    ///
    /// A case-only difference can only come from a keyword: the formatter
    /// prints identifiers and literals straight out of their source span, so
    /// the only text it ever recases is a keyword literal from the bytecode.
    /// That covers contextual keywords such as STRICT and ROWID, which the
    /// tokenizer sees as identifiers. Quoted text must still match exactly.
    fn same(&self, other: &Self) -> bool {
        if self.verbatim() || other.verbatim() {
            self.text == other.text
        } else {
            self.text.eq_ignore_ascii_case(&other.text)
        }
    }

    /// How this lexeme is named in a work item.
    fn label(&self) -> String {
        if self.keyword {
            self.text.to_ascii_uppercase()
        } else {
            self.text.clone()
        }
    }
}

/// Significant tokens of `sql`, with whitespace and comments dropped.
///
/// Comments are a side channel the formatter places separately, so they are
/// not part of what the AST is asked to reproduce. A single trailing `;` is
/// also dropped, because emitting one is a `FormatConfig` choice rather than
/// information the AST carries.
fn lex(
    tokenizer: &AnyTokenizer,
    dialect: &AnyDialect,
    keywords: &HashSet<AnyTokenType>,
    sql: &str,
) -> Vec<Lexeme> {
    let mut lexemes: Vec<Lexeme> = tokenizer
        .tokenize(sql)
        .filter_map(|token| {
            let token_type = token.token_type();
            if matches!(dialect.token_category(token_type), TokenCategory::Comment) {
                return None;
            }
            let text = token.text();
            if text.trim().is_empty() {
                return None;
            }
            Some(Lexeme {
                text: text.to_owned(),
                // Keyword-ness comes from the token type, not the presentation
                // category: a contextual keyword such as COLUMN is categorised
                // as an identifier but is still recased by the formatter.
                keyword: keywords.contains(&token_type),
            })
        })
        .collect();
    // Normalise statement terminators. A run of them, or one at either end,
    // delimits empty statements that carry no syntax, and whether a final `;`
    // is printed at all is a FormatConfig choice. Terminators *between*
    // statements are kept, so losing one inside a trigger body still shows up.
    lexemes.dedup_by(|a, b| a.text == ";" && b.text == ";");
    if lexemes.first().is_some_and(|first| first.text == ";") {
        lexemes.remove(0);
    }
    if lexemes.last().is_some_and(|last| last.text == ";") {
        lexemes.pop();
    }
    lexemes
}

/// What went wrong, named so that identical losses group together.
struct Divergence {
    /// Stable name for the construct, used to rank the work items.
    category: String,
    index: usize,
    authored: Option<String>,
    formatted: Option<String>,
}

/// First point at which the formatted stream stops reproducing the authored one.
///
/// A one-token lookahead separates a dropped token from an inserted one from a
/// changed one, which is enough to name the construct.
///
/// A parenthesis the formatter adds is tolerated: clarifying parentheses
/// around confusing precedence are a deliberate formatting choice, in the same
/// class as keyword casing, not information the AST lost. A parenthesis the
/// formatter *drops* is still reported, because that is real loss.
fn diverge(authored: &[Lexeme], formatted: &[Lexeme]) -> Option<Divergence> {
    /// How far ahead to look when deciding whether a token was dropped or
    /// inserted rather than changed.
    const WINDOW: usize = 4;

    let mut index = 0;
    let mut extra = 0;
    while index < authored.len() && index + extra < formatted.len() {
        let formatted = &formatted[index + extra..];
        if authored[index].same(&formatted[0]) {
            index += 1;
            continue;
        }
        if matches!(formatted[0].text.as_str(), "(" | ")") {
            extra += 1;
            continue;
        }
        // Look a little way ahead so a dropped or inserted run is named for
        // what it is rather than falling through to "changed". The nearer
        // match wins, which keeps a one-token edit labelled as one.
        let dropped = (1..=WINDOW).find(|k| {
            authored
                .get(index + k)
                .is_some_and(|a| a.same(&formatted[0]))
        });
        let inserted =
            (1..=WINDOW).find(|k| formatted.get(*k).is_some_and(|f| authored[index].same(f)));
        let dropped = dropped.is_some_and(|d| inserted.is_none_or(|i| d <= i));
        let inserted = !dropped
            && (1..=WINDOW).any(|k| formatted.get(k).is_some_and(|f| authored[index].same(f)));
        let authored_label = authored[index].label();
        let formatted_label = formatted[0].label();
        let (category, authored_out, formatted_out) = if dropped && !inserted {
            (
                format!("dropped {authored_label}"),
                Some(authored_label),
                None,
            )
        } else if inserted && !dropped {
            (
                format!("inserted {formatted_label}"),
                None,
                Some(formatted_label),
            )
        } else {
            (
                format!("changed {authored_label} -> {formatted_label}"),
                Some(authored_label),
                Some(formatted_label),
            )
        };
        return Some(Divergence {
            category,
            index,
            authored: authored_out,
            formatted: formatted_out,
        });
    }
    if index < authored.len() {
        let label = authored[index].label();
        return Some(Divergence {
            category: format!("dropped {label}"),
            index,
            authored: Some(label),
            formatted: None,
        });
    }
    // Any remaining formatted tokens are tolerated parentheses, since every
    // other mismatch would have been reported above.
    None
}

/// Outcome for one corpus entry.
enum Outcome {
    Reproduced,
    Lost(Divergence),
    FormatError(String),
}

fn check(
    formatter: &mut Formatter,
    tokenizer: &AnyTokenizer,
    dialect: &AnyDialect,
    keywords: &HashSet<AnyTokenType>,
    sql: &str,
) -> Outcome {
    let printed = match formatter.format(sql) {
        Ok(out) => out,
        Err(error) => return Outcome::FormatError(error.to_string()),
    };
    let authored = lex(tokenizer, dialect, keywords, sql);
    let produced = lex(tokenizer, dialect, keywords, &printed);
    match diverge(&authored, &produced) {
        Some(divergence) => Outcome::Lost(divergence),
        None => Outcome::Reproduced,
    }
}

/// A formatter for one corpus configuration.
///
/// # Panics
/// On an unknown flag name or an unparseable version, which would silently
/// measure the wrong dialect.
fn build_formatter(version: &str, cflags: &[String]) -> Formatter {
    let mut dialect = sqlite_dialect();
    if !version.is_empty() {
        let parsed = SqliteVersion::parse(version)
            .unwrap_or_else(|| panic!("corpus version {version} is not a SQLite version"));
        dialect = dialect.with_version(parsed);
    }
    if !cflags.is_empty() {
        let mut flags = SqliteFlags::default();
        for name in cflags {
            let flag = SqliteFlag::from_name(name)
                .unwrap_or_else(|| panic!("corpus cflag {name} is not a known flag"));
            flags = flags.with(flag);
        }
        dialect = dialect.with_cflags(flags);
    }
    Formatter::with_dialect_config(dialect, &syntaqlite::FormatConfig::default())
}

fn main() {
    let mut args = std::env::args().skip(1);
    let mut input = String::new();
    if let Some(path) = args.next() {
        input = std::fs::read_to_string(&path)
            .unwrap_or_else(|error| panic!("cannot read corpus {path}: {error}"));
    } else {
        let mut stdin: Box<dyn Read> = Box::new(std::io::stdin());
        stdin
            .read_to_string(&mut input)
            .expect("cannot read corpus from stdin");
    }

    let dialect: AnyDialect = sqlite_dialect().into();
    let tokenizer = AnyTokenizer::new((*dialect).clone());
    let keywords: HashSet<AnyTokenType> =
        dialect.keywords().map(|entry| entry.token_type()).collect();
    // A corpus entry may need compile-time flags or an emulated version, the
    // same way its source test does. Formatters are cached per configuration
    // so the common case still builds one.
    let mut formatters: HashMap<(String, String), Formatter> = HashMap::new();

    let stdout = std::io::stdout();
    let mut out = std::io::BufWriter::new(stdout.lock());
    let (mut total, mut reproduced, mut lost, mut errored) = (0u64, 0u64, 0u64, 0u64);

    for line in BufReader::new(input.as_bytes()).lines() {
        let line = line.expect("corpus line");
        if line.trim().is_empty() {
            continue;
        }
        let entry: serde_json::Value =
            serde_json::from_str(&line).expect("corpus line is JSON Lines");
        let name = entry
            .get("name")
            .and_then(serde_json::Value::as_str)
            .unwrap_or("<unnamed>");
        let sql = entry
            .get("sql")
            .and_then(serde_json::Value::as_str)
            .expect("corpus entry has a sql field");
        total += 1;

        let version = entry
            .get("version")
            .and_then(serde_json::Value::as_str)
            .unwrap_or_default()
            .to_owned();
        let cflags: Vec<String> = entry
            .get("cflags")
            .and_then(serde_json::Value::as_array)
            .map(|flags| {
                flags
                    .iter()
                    .filter_map(|f| f.as_str().map(str::to_owned))
                    .collect()
            })
            .unwrap_or_default();
        let key = (version.clone(), cflags.join(","));
        let formatter = formatters
            .entry(key)
            .or_insert_with(|| build_formatter(&version, &cflags));

        let record = match check(formatter, &tokenizer, &dialect, &keywords, sql) {
            Outcome::Reproduced => {
                reproduced += 1;
                continue;
            }
            Outcome::FormatError(message) => {
                errored += 1;
                serde_json::json!({
                    "name": name,
                    "status": "format_error",
                    "category": "format error",
                    "detail": message,
                    "sql": sql,
                })
            }
            Outcome::Lost(divergence) => {
                lost += 1;
                serde_json::json!({
                    "name": name,
                    "status": "lost",
                    "category": divergence.category,
                    "index": divergence.index,
                    "authored": divergence.authored,
                    "formatted": divergence.formatted,
                    "sql": sql,
                })
            }
        };
        writeln!(out, "{record}").expect("write result");
    }
    out.flush().expect("flush results");

    let rate = if total == 0 {
        100.0
    } else {
        f64::from(u32::try_from(reproduced).unwrap_or(u32::MAX)) * 100.0
            / f64::from(u32::try_from(total).unwrap_or(u32::MAX))
    };
    eprintln!(
        "token preservation: {reproduced}/{total} reproduced ({rate:.2}%), {lost} lost, {errored} format errors"
    );
}
