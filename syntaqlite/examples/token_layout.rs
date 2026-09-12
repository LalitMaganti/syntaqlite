//! Batch verification harness for the production formatter.
use std::io::{self, BufRead};
#[path = "../tests/support/token_signature.rs"]
mod token_signature;
fn main() {
    for line in io::stdin().lock().lines() {
        let line = line.expect("valid request");
        let request: serde_json::Value = serde_json::from_str(&line).expect("valid JSON");
        let input = request["sql"].as_str().expect("SQL string");
        let width =
            usize::try_from(request["width"].as_u64().unwrap_or(80)).expect("width fits usize");
        let mut flags = syntaqlite::util::SqliteFlags::default();
        if let Some(names) = request["cflags"].as_array() {
            for name in names {
                flags = flags.with(
                    syntaqlite::util::SqliteFlag::from_name(name.as_str().expect("flag name"))
                        .expect("known flag"),
                );
            }
        }
        let dialect = syntaqlite::sqlite_dialect().erase().with_cflags(flags);
        let mut formatter = syntaqlite::Formatter::with_dialect_config(
            dialect.clone(),
            &syntaqlite::FormatConfig::default().with_line_width(width),
        );
        match formatter.format(input) {
            Ok(output) => println!(
                "{}",
                serde_json::json!({
                    "tokens_equal": token_signature::signature(input, &dialect) == token_signature::signature(&output, &dialect),
                    "output": output,
                })
            ),
            Err(error) => println!("{}", serde_json::json!({"error":error.to_string()})),
        }
    }
}
