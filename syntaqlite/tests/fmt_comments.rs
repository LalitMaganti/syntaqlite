#![cfg(all(feature = "fmt", feature = "sqlite"))]
//! Regression tests for comment preservation in the formatter.

use syntaqlite::Formatter;

/// Helper: format with default config, unwrap result.
fn fmt(sql: &str) -> String {
    Formatter::new()
        .format(sql)
        .expect("formatting should succeed")
}

// ── Multi-statement comment sync ─────────────────────────────────────────────

/// Regression: comments in second statement were reordered / dropped.
///
/// Input has three comments in statement 2:
///   - `-- foo`      on its own line, *before* `select`   → leading comment before SELECT
///   - `-- foo bar`  on its own line, *between* `select` and `1` → leading before `1`
///   - `-- foo`      on its own line, *before* `from`     → leading comment before FROM
///
/// All three must appear in source order.
#[test]
fn multi_stmt_comments_preserved_in_order() {
    let input = "SELECT 1;\n\n-- foo\nselect\n-- foo bar\n1\n-- foo\nfrom slice;";
    let out = fmt(input);
    eprintln!("=== actual ===\n{out}=== end ===");

    // All three comment texts must be present.
    assert!(out.contains("-- foo bar"), "'-- foo bar' missing");

    // Count occurrences of '-- foo' (not part of '-- foo bar').
    // The standalone comments are "-- foo\n" — find both.
    let first = out.find("-- foo\n").expect("first '-- foo' not found");
    let second = out[first + 1..]
        .find("-- foo\n")
        .map(|p| first + 1 + p)
        .expect("second '-- foo' not found");
    let bar_pos = out.find("-- foo bar").expect("'-- foo bar' missing");

    // Source order: first --foo < --foo bar < second --foo
    assert!(
        first < bar_pos,
        "'-- foo' (pre-SELECT) must come before '-- foo bar': first={first} bar={bar_pos}"
    );
    assert!(
        bar_pos < second,
        "'-- foo bar' must come before second '-- foo' (pre-FROM): bar={bar_pos} second={second}"
    );
}

/// Debug helper: print comment/token offsets per statement.
#[test]
fn debug_comment_token_offsets() {
    use syntaqlite::ParseOutcome;
    use syntaqlite::parse::ParserConfig;
    use syntaqlite::typed::{TypedParser, grammar};

    let input = "SELECT 1;\n\n-- foo\nselect\n-- foo bar\n1\n-- foo\nfrom slice;";
    let config = ParserConfig::default().with_collect_tokens(true);
    let parser = TypedParser::with_config(grammar(), &config);
    let mut session = parser.parse(input);

    let mut stmt_num = 0;
    loop {
        match session.next() {
            ParseOutcome::Done => break,
            ParseOutcome::Ok(stmt) => {
                eprintln!("=== Statement {stmt_num} ===");
                let comments: Vec<_> = stmt.comments().collect();
                eprintln!("  Comments ({}):", comments.len());
                for c in &comments {
                    let end = c.offset() as usize + c.length() as usize;
                    eprintln!(
                        "    offset={} len={} text={:?}",
                        c.offset(),
                        c.length(),
                        &input[c.offset() as usize..end]
                    );
                }
                let tokens: Vec<_> = stmt.tokens().collect();
                eprintln!("  Tokens ({}):", tokens.len());
                for t in &tokens {
                    eprintln!(
                        "    offset={} len={} text={:?}",
                        t.offset(),
                        t.length(),
                        t.text()
                    );
                }
                // Verify statement 0 has no lookahead contamination.
                if stmt_num == 0 {
                    assert_eq!(
                        tokens.len(),
                        3,
                        "stmt 0 should have exactly 3 tokens (SELECT, 1, ;)"
                    );
                    assert_eq!(comments.len(), 0, "stmt 0 should have no comments");
                }
                // Verify statement 1 has all its tokens and comments.
                if stmt_num == 1 {
                    assert_eq!(
                        tokens.len(),
                        5,
                        "stmt 1 should have 5 tokens (select, 1, from, slice, ;)"
                    );
                    assert_eq!(
                        comments.len(),
                        3,
                        "stmt 1 should have 3 comments (--foo, --foo bar, --foo)"
                    );
                }
                stmt_num += 1;
            }
            ParseOutcome::Err(e) => {
                eprintln!("=== Statement {} ERROR: {} ===", stmt_num, e.message());
                panic!(
                    "unexpected parse error on statement {}: {}",
                    stmt_num,
                    e.message()
                );
            }
        }
    }
    assert_eq!(stmt_num, 2, "expected exactly 2 statements");
}

/// Regression: a comment between JOIN and WHERE in a subquery was being
/// moved to the outer query, eating the semicolon terminator.
#[test]
fn comment_between_join_and_where_in_subquery() {
    let input = concat!(
        "SELECT *\n",
        "FROM (\n",
        "  SELECT a\n",
        "  FROM t1\n",
        "  JOIN t2 ON (t1.id = t2.id)\n",
        "  -- this comment belongs here\n",
        "  WHERE x > 0\n",
        ");\n",
    );
    let out = fmt(input);
    eprintln!("=== actual ===\n{out}=== end ===");
    // Comment must stay inside the subquery, not migrate to outer query.
    assert!(
        out.contains("-- this comment belongs here"),
        "comment was dropped"
    );
    // Semicolon must not be swallowed by the comment.
    let second_pass = fmt(&out);
    assert_eq!(out, second_pass, "formatting is not idempotent");
}
