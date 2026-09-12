//! Batch harness for the isolated token layout experiment.
#![allow(clippy::all, clippy::pedantic)]
use std::io::{self, BufRead};
#[path = "../tests/support/token_layout.rs"]
mod fmt;
use fmt::token_layout_prototype::TokenFormatter;
fn main() {
    let mut formatter = TokenFormatter::default();
    for line in io::stdin().lock().lines() {
        let line = line.expect("valid prototype request");
        let request: serde_json::Value =
            serde_json::from_str(&line).expect("valid prototype request");
        let input = request["sql"].as_str().expect("valid prototype request");
        let width = request["width"].as_u64().unwrap_or(80) as u32;
        let mut configured;
        let chosen = if let Some(flags) = request["cflags"].as_array() {
            use syntaqlite_syntax::util::{SqliteSyntaxFlag, SqliteSyntaxFlags};
            let mut options = SqliteSyntaxFlags::default();
            for flag in flags {
                options = options.with(
                    SqliteSyntaxFlag::from_name(flag.as_str().expect("valid prototype request"))
                        .expect("valid prototype request"),
                );
            }
            configured = TokenFormatter::with_dialect(
                syntaqlite_syntax::typed::dialect().with_cflags(options),
            );
            &mut configured
        } else {
            &mut formatter
        };
        match chosen.format(input, width) {
            Ok((out, stats)) => {
                let tokens = |sql: &str| {
                    syntaqlite_syntax::Tokenizer::new()
                        .tokenize(sql)
                        .filter(|t| !t.text().trim().is_empty())
                        .map(|t| t.text().to_owned())
                        .collect::<Vec<_>>()
                };
                let tokens_equal = tokens(input) == tokens(&out);
                println!(
                    "{}",
                    serde_json::json!({"output":out,"tokens_equal":tokens_equal,"shifts":stats.shifts,"reductions":stats.reductions,"max_stack":stats.max_stack})
                );
            }
            Err(error) => println!("{}", serde_json::json!({"error":error})),
        }
    }
}
