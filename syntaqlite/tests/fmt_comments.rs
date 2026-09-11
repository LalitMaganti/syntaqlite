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
    use syntaqlite::typed::{TypedParser, dialect};

    let input = "SELECT 1;\n\n-- foo\nselect\n-- foo bar\n1\n-- foo\nfrom slice;";
    let config = ParserConfig::default().with_collect_tokens(true);
    let parser = TypedParser::with_config(dialect(), &config);
    let mut session = parser.parse(input);

    let mut stmt_num = 0;
    loop {
        match session.next() {
            ParseOutcome::Done => break,
            ParseOutcome::Ok(stmt) => {
                eprintln!("=== Statement {stmt_num} ===");
                let comments: Vec<_> = stmt.comments().collect();
                let stmt_text = stmt.text();
                eprintln!("  Comments ({}):", comments.len());
                for c in &comments {
                    let range =
                        syntaqlite::source::StmtRange::from_offset_len(c.offset(), c.length());
                    eprintln!(
                        "    offset={} len={} text={:?}",
                        c.offset(),
                        c.length(),
                        &stmt_text[range]
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

/// Regression: perfetto stdlib crash on a leading comment inside an untracked
/// parenthesized expression. Reduced shape:
///   WHERE a AND (
///     -- leading comment inside untracked parens
///     (b OR c)
///   )
/// The outer parens of `(b OR c)` have no `ParenExpr` AST node, so tokens like
/// `(` are skipped by `peek_keyword_tokens`. When the drain of the statement's
/// remaining comments fires, `prev_token_end()` has advanced past the comment's
/// offset, producing a reversed `StmtRange` and a slice panic.
#[test]
fn leading_comment_inside_untracked_parens() {
    let input = concat!(
        "SELECT 1\n",
        "WHERE\n",
        "  a\n",
        "  AND (\n",
        "    -- leading comment inside untracked parens\n",
        "    (\n",
        "      b AND c\n",
        "    )\n",
        "    -- another\n",
        "    OR (\n",
        "      d AND e\n",
        "    )\n",
        "  );\n",
    );
    let out = fmt(input);
    eprintln!("=== actual ===\n{out}=== end ===");
    assert!(out.contains("-- leading comment inside untracked parens"));
    assert!(out.contains("-- another"));
    let second_pass = fmt(&out);
    assert_eq!(out, second_pass, "formatting is not idempotent");
}

/// Issue #354: the authored name is a declaration, not a repeated span on
/// REFERENCES and DEFERRABLE. An unused name on the next column is retained.
#[test]
fn issue_354_named_deferred_foreign_key_between_columns() {
    let input = "CREATE TABLE t (\n    a integer constraint c1 references r(id) deferrable,\n    -- comment\n    b text constraint c2\n);";
    let out = fmt(input);
    eprintln!("=== actual ===\n{out}=== end ===");
    assert_eq!(
        out,
        "CREATE TABLE t(\n  a integer CONSTRAINT c1 REFERENCES r(id) DEFERRABLE,\n  -- comment\n  b text CONSTRAINT c2\n);\n"
    );
    assert_eq!(fmt(&out), out);
}

/// Every emitted terminal, including a list separator, must handle comments
/// before advancing its source cursor. These packets straddle the separator.
#[test]
fn comments_on_both_sides_of_list_separators() {
    let cases = [
        "SELECT a /* before */, -- after\n b;",
        "SELECT f(a /* before */, -- after\n b);",
        "SELECT a IN (1 /* before */, -- after\n 2);",
        "SELECT a FROM t ORDER BY a /* before */, -- after\n b;",
        "VALUES (1 /* before */, -- after\n 2);",
        "UPDATE t SET a = 1 /* before */, -- after\n b = 2;",
        "CREATE TABLE t(a CONSTRAINT unused /* before */, -- after\n b CHECK(b));",
    ];
    for input in cases {
        let out = fmt(input);
        eprintln!("=== input ===\n{input}\n=== actual ===\n{out}=== end ===");
        assert_eq!(out.matches("/* before */").count(), 1, "{input}");
        assert_eq!(out.matches("-- after").count(), 1, "{input}");
        let separator = out.find(',').expect("separator preserved");
        assert!(
            out.find("/* before */")
                .expect("comment before separator preserved")
                < separator
        );
        assert!(
            separator
                < out
                    .find("-- after")
                    .expect("comment after separator preserved")
        );
        assert_eq!(fmt(&out), out, "{input}");
    }
}

/// Name declarations own their source occurrence once, including declarations
/// that are superseded or never applied to a constraint.
#[test]
fn comments_on_authored_constraint_name_declarations() {
    for input in [
        "CREATE TABLE t(a CONSTRAINT c /* first */ CHECK(a > 0) CHECK(a < 9));",
        "CREATE TABLE t(a CONSTRAINT old /* first */ CONSTRAINT c CHECK(a));",
        "CREATE TABLE t(a CONSTRAINT c /* first */);",
        "ALTER TABLE t ADD COLUMN a CONSTRAINT c /* first */ CHECK(a > 0) CHECK(a < 9);",
    ] {
        let out = fmt(input);
        eprintln!("=== input ===\n{input}\n=== actual ===\n{out}=== end ===");
        assert_eq!(out.matches("/* first */").count(), 1, "{input}");
        assert_eq!(out.matches("CONSTRAINT c").count(), 1, "{input}");
        assert_eq!(fmt(&out), out, "{input}");
    }
}

#[test]
fn parser_provenance_places_comments_on_canonical_and_retired_syntax() {
    for (input, expected) in [
        (
            "SELECT a == /* operator */ b;",
            "SELECT a = /* operator */ b;\n",
        ),
        (
            "SELECT a /* value */ alias;",
            "SELECT a /* value */ AS alias;\n",
        ),
        (
            "SELECT a FROM t ORDER BY a ASC /* direction */, b;",
            "SELECT a FROM t ORDER BY a /* direction */, b;\n",
        ),
        (
            "CREATE TABLE t(a CHECK /* keyword */ ( /* open */ a > 0));",
            "CREATE TABLE t(a CHECK /* keyword */ ( /* open */ a > 0));\n",
        ),
    ] {
        let out = fmt(input);
        eprintln!("=== input ===\n{input}\n=== actual ===\n{out}=== end ===");
        assert_eq!(out, expected);
        assert_eq!(fmt(&out), out);
    }
}

/// Exercise the public formatter at every lexical boundary, independently of
/// the formatter's templates. Comments must neither change SQL tokens nor
/// disappear, and a second formatting pass must be stable.
#[test]
fn production_comment_ownership_at_every_token_boundary() {
    use syntaqlite::FormatConfig;
    use syntaqlite_syntax::Tokenizer;
    let tokenizer = Tokenizer::new();
    let sql_tokens = |sql: &str| {
        tokenizer
            .tokenize(sql)
            .filter(|t| !t.text().trim().is_empty() && !t.text().contains("ownership_probe"))
            .map(|t| t.text().to_owned())
            .collect::<Vec<_>>()
    };
    for sql in [
        "SELECT a == b, c != d FROM t ORDER BY a ASC, b DESC;",
        "SELECT (a + b) * c value FROM t WHERE a IS NOT NULL;",
        "SELECT count(DISTINCT a), CAST(b AS TEXT) FROM t GROUP BY b HAVING count(*) > 1;",
        "SELECT CASE WHEN a THEN b ELSE c END FROM t LIMIT 2 OFFSET 1;",
        "WITH x(a) AS (SELECT 1) SELECT a FROM x UNION ALL SELECT 2;",
        "SELECT a FROM t LEFT OUTER JOIN u ON t.id = u.id;",
        "SELECT * FROM t OUTER LEFT NATURAL JOIN u;",
        "SELECT * FROM t LEFT LEFT JOIN u ON t.id = u.id;",
        "SELECT * FROM t CROSS NATURAL JOIN u;",
        "SELECT sum(a) OVER (PARTITION BY b ORDER BY c ROWS 1 PRECEDING) FROM t;",
        "CREATE TABLE t(a CONSTRAINT c CHECK(a > 0), b REFERENCES u(id) ON UPDATE CASCADE ON DELETE RESTRICT);",
        "CREATE TABLE t(a, CONSTRAINT c UNIQUE(a) CHECK(a > 0));",
        "CREATE TEMPORARY VIEW v AS SELECT a FROM t;",
        "INSERT OR REPLACE INTO t(a) VALUES(1),(2) RETURNING a;",
        "UPDATE t SET a = 1, b = 2 WHERE c = 3 RETURNING a;",
        "DELETE FROM t WHERE a IN (1,2) RETURNING a;",
        "CREATE TRIGGER tr AFTER INSERT ON t BEGIN UPDATE t SET a = 1; SELECT 2; END;",
        "CREATE INDEX idx ON t(a DESC, b COLLATE nocase) WHERE a > 0;",
        "SELECT NOT a BETWEEN 1 AND 2, a NOTNULL, a NOT IN (1,2) FROM t;",
        "PRAGMA cache_size = -10;",
        "SELECT a ISNULL, a IS NOT DISTINCT FROM b, a NOT LIKE b ESCAPE c FROM t;",
        "SELECT a ->> '$.x', a GLOB b, a COLLATE nocase FROM t;",
        "SELECT * FROM (SELECT 1) x, t INDEXED BY idx;",
        "SELECT * FROM t NOT INDEXED JOIN u USING(id);",
        "SELECT sum(a) FILTER (WHERE b) OVER win FROM t WINDOW win AS (ORDER BY c RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW);",
        "INSERT INTO t VALUES(1) ON CONFLICT(a) DO UPDATE SET a = excluded.a RETURNING *;",
        "CREATE TABLE t(a INTEGER PRIMARY KEY AUTOINCREMENT, b TEXT DEFAULT 'x' NOT NULL) STRICT;",
        "CREATE VIRTUAL TABLE t USING fts5(a, b);",
        "ALTER TABLE t RENAME COLUMN a TO b;",
        "DROP TABLE IF EXISTS t;",
        "ATTACH DATABASE 'x.db' AS aux;",
        "DETACH DATABASE aux;",
        "VACUUM main INTO 'copy.db';",
        "REINDEX idx;",
        "ANALYZE main;",
        "BEGIN IMMEDIATE TRANSACTION; SAVEPOINT x; ROLLBACK TO SAVEPOINT x; RELEASE SAVEPOINT x; COMMIT;",
        "SELECT 1; SELECT 2;",
    ] {
        let mut gaps = vec![0];
        gaps.extend(
            tokenizer
                .tokenize(sql)
                .filter(|t| !t.text().trim().is_empty())
                .map(|t| t.text().as_ptr() as usize - sql.as_ptr() as usize + t.text().len()),
        );
        for width in [30, 80] {
            let mut formatter =
                Formatter::with_config(&FormatConfig::default().with_line_width(width));
            let canonical = formatter.format(sql).expect("valid seed");
            for offset in &gaps {
                for comment in [" /* ownership_probe */ ", " -- ownership_probe\n"] {
                    let input = format!("{}{comment}{}", &sql[..*offset], &sql[*offset..]);
                    let output = formatter
                        .format(&input)
                        .unwrap_or_else(|e| panic!("{input}\n{e}"));
                    assert_eq!(
                        output.matches("ownership_probe").count(),
                        1,
                        "{input}\n{output}"
                    );
                    assert_eq!(
                        sql_tokens(&output),
                        sql_tokens(&canonical),
                        "{input}\n{output}"
                    );
                    assert_eq!(
                        formatter.format(&output).expect("valid formatted SQL"),
                        output,
                        "{input}"
                    );
                }
            }
        }
    }
}

/// A source span owns its interior comments; its boundary attachments are
/// emitted separately. Reusing the formatter must also clear both paths.
#[test]
fn verbatim_comments_and_comment_free_reuse() {
    let mut formatter = Formatter::new();
    for input in [
        "CREATE VIRTUAL TABLE t USING fts5( /* inside */ a, b /* inside2 */) /* after */;",
        "SELECT 1;",
        "SELECT a /* before */ + b /* after */;",
        "SELECT 2;",
    ] {
        let output = formatter.format(input).expect("valid test SQL");
        eprintln!("=== input ===\n{input}\n=== actual ===\n{output}=== end ===");
        for marker in [
            "/* inside */",
            "/* inside2 */",
            "/* before */",
            "/* after */",
        ] {
            assert_eq!(
                output.matches(marker).count(),
                input.matches(marker).count(),
                "{input}"
            );
        }
        assert_eq!(
            formatter.format(&output).expect("valid formatted SQL"),
            output,
            "{input}"
        );
    }
}
