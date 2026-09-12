//! Production formatter: token preservation, layout and convergence.
#![cfg(all(feature = "fmt", feature = "sqlite"))]
use syntaqlite_syntax::Tokenizer;

fn format(sql: &str, width: u32) -> Result<String, syntaqlite::fmt::FormatError> {
    syntaqlite::Formatter::with_config(
        &syntaqlite::FormatConfig::default().with_line_width(width as usize),
    )
    .format(sql)
}

fn tokens(sql: &str) -> Vec<String> {
    Tokenizer::new()
        .tokenize(sql)
        .filter(|token| !token.text().trim().is_empty() && token.text() != ";")
        .map(|token| token.text().to_owned())
        .collect()
}

#[test]
fn original_tokens_survive_every_comment_boundary() {
    let seeds = [
        "SELECT a == b, c != d FROM t ORDER BY a ASC, b DESC;",
        "SELECT a NOT NULL, b NOTNULL, c ISNULL;",
        "SELECT * FROM t LIMIT 20, 10;",
        "SELECT first_column, second_column FROM some_table WHERE first_column BETWEEN 1 AND 2 AND second_column > 0;",
        "SELECT - -a, a*-b, a NOT IN (1,2), a NOT BETWEEN 1 AND 2;",
        "SELECT CASE WHEN a THEN b ELSE c END alias FROM (SELECT 1) t;",
        "WITH x(a) AS (SELECT 1) SELECT a FROM x UNION ALL SELECT 2;",
        "CREATE TABLE t(a integer CONSTRAINT c1 REFERENCES r(id) DEFERRABLE, b text CONSTRAINT c2);",
        "CREATE TABLE t(a int GENERATED ALWAYS AS (1));",
        "CREATE VIRTUAL TABLE t USING fts5(a, b);",
        "CREATE TRIGGER tr AFTER INSERT ON t BEGIN UPDATE t SET a = 1; SELECT 2; END;",
        "SELECT a FROM t OUTER LEFT NATURAL JOIN u;",
        "SELECT sum(a) FILTER (WHERE b) OVER win FROM t WINDOW win AS (ORDER BY c RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW);",
        "INSERT INTO t VALUES(1) ON CONFLICT(a) DO UPDATE SET a = excluded.a RETURNING *;",
        "SELECT 1; SELECT 2;",
        ";; SELECT 1; ; SELECT 2;;",
    ];
    for seed in seeds {
        let mut gaps = vec![0];
        gaps.extend(
            Tokenizer::new()
                .tokenize(seed)
                .filter(|token| !token.text().trim().is_empty() && token.text() != ";")
                .map(|token| {
                    token.text().as_ptr() as usize - seed.as_ptr() as usize + token.text().len()
                }),
        );
        for width in [30, 80] {
            for gap in &gaps {
                for comment in [" /* probe */ ", " -- probe\n"] {
                    let input = format!("{}{comment}{}", &seed[..*gap], &seed[*gap..]);
                    let output = format(&input, width).expect("valid input");
                    assert_eq!(tokens(&input), tokens(&output), "{input}\n{output}");
                    let second = format(&output, width).expect("valid output");
                    assert_eq!(output, second, "{input}");
                }
            }
        }
    }
}

#[test]
fn no_syntax_normalization_or_source_matching() {
    for input in [
        "SELECT a == /* operator */ b;",
        "SELECT a NOT NULL;",
        "SELECT a FROM t ORDER BY a ASC LIMIT 20, 10;",
    ] {
        let output = format(input, 80).expect("valid input");
        eprintln!("{input}\n{output}");
        assert_eq!(output.trim_end(), input);
    }
}

#[test]
fn layout_has_structural_indentation_and_statement_boundaries() {
    for (input, width, expected) in [
        (
            "SELECT first_column, second_column, third_column FROM some_table;",
            32,
            "SELECT\n  first_column,\n  second_column,\n  third_column\nFROM some_table;",
        ),
        (
            "SELECT f(first_argument, second_argument, third_argument);",
            32,
            "SELECT\n  f(\n    first_argument,\n    second_argument,\n    third_argument\n  );",
        ),
        ("SELECT - -a, ~b, +c;", 80, "SELECT - -a, ~b, +c;"),
        ("SELECT 1; SELECT 2;", 80, "SELECT 1;\n\nSELECT 2;"),
    ] {
        let output = format(input, width).expect("valid input");
        eprintln!("=== input ===\n{input}\n=== actual ===\n{output}\n=== end ===");
        assert_eq!(output.trim_end(), expected);
        assert_eq!(tokens(input), tokens(&output));
        assert_eq!(format(&output, width).expect("valid output"), output);
    }
}

#[test]
fn recursive_lists_and_chains_have_constant_continuation_indent() {
    for count in [64, 256, 1024] {
        let columns = (0..count)
            .map(|i| format!("column_{i:04}"))
            .collect::<Vec<_>>();
        let predicates = columns
            .iter()
            .map(|c| format!("{c} = 1"))
            .collect::<Vec<_>>();
        for input in [
            format!("SELECT {} FROM t;", columns.join(", ")),
            format!("SELECT a FROM t WHERE {};", predicates.join(" AND ")),
            format!("SELECT {} FROM t;", columns.join(" + ")),
        ] {
            let output = format(&input, 40).expect("valid long sequence");
            assert_eq!(tokens(&input), tokens(&output));
            for line in output.lines() {
                assert!(line.len() <= 40, "overlong line: {line}");
                assert!(
                    line.len() - line.trim_start().len() <= 2,
                    "recursive indentation: {line}"
                );
            }
            assert_eq!(format(&output, 40).expect("valid output"), output);
        }
    }
}

#[test]
fn comments_use_the_receiving_boundary_indentation() {
    for (input, expected) in [
        (
            "SELECT f(first_argument, second_argument -- last\n);",
            "SELECT\n  f(\n    first_argument,\n    second_argument -- last\n  );",
        ),
        ("/* header */ SELECT 1;", "/* header */\nSELECT 1;"),
        (
            "SELECT 1; -- one\n\n-- two\nSELECT 2;",
            "SELECT 1; -- one\n\n-- two\n\nSELECT 2;",
        ),
    ] {
        let out = format(input, 32).expect("valid input");
        eprintln!("=== actual ===\n{out}\n=== end ===");
        assert_eq!(out.trim_end(), expected);
        assert_eq!(tokens(input), tokens(&out));
        assert_eq!(format(&out, 32).expect("valid output"), out);
    }
}

#[path = "fixtures/token_layout_cases.rs"]
mod layout_cases;

#[test]
fn reviewed_layouts_across_sql_constructs() {
    for &(name, width, input, expected) in layout_cases::CASES {
        let out = format(input, width).expect("valid layout fixture");
        eprintln!("=== {name} ===\n{out}\n=== end ===");
        assert_eq!(out.trim_end(), expected, "{name}");
        assert_eq!(tokens(input), tokens(&out), "{name}");
        assert!(
            out.lines().all(|line| line.len() <= width as usize),
            "{name}: width exceeded"
        );
        assert_eq!(format(&out, width).expect("valid output"), out, "{name}");
    }
}

#[test]
fn production_configuration_and_reuse() {
    use syntaqlite::fmt::KeywordCase;
    use syntaqlite::{FormatConfig, Formatter};
    let config = FormatConfig::default()
        .with_line_width(24)
        .with_indent_width(4)
        .with_keyword_case(KeywordCase::Lower)
        .with_semicolons(false);
    let mut formatter = Formatter::with_config(&config);
    let source = "SELECT FirstColumn, SecondColumn FROM SomeTable;";
    let output = formatter.format(source).expect("valid input");
    assert_eq!(
        output,
        "select\n    FirstColumn,\n    SecondColumn\nfrom SomeTable\n"
    );
    assert_eq!(formatter.format(&output).expect("second pass"), output);
    let bad = "SELECT 1; SELECT FROM t";
    let error = formatter.format(bad).expect_err("invalid SELECT");
    assert_eq!(error.range().expect("error location").start.as_usize(), 17);
    assert_eq!(formatter.format(source).expect("reuse after error"), output);
}

#[test]
fn production_classification_and_statement_terminators() {
    use syntaqlite::{FormatConfig, Formatter};
    let mut formatter = Formatter::new();
    assert_eq!(
        formatter
            .format("select action, CastMe(x) from MyTable;")
            .expect("valid names"),
        "SELECT action, CastMe(x) FROM MyTable;\n"
    );
    assert_eq!(
        formatter
            .format("select 1; select 2; -- end")
            .expect("two statements"),
        "SELECT 1;\n\nSELECT 2; -- end\n"
    );
    assert_eq!(
        formatter.format("-- only a comment\n").expect("comment"),
        "-- only a comment\n"
    );
    assert_eq!(formatter.format(";;").expect("empty statements"), "");
    let source = "CREATE TRIGGER tr AFTER INSERT ON t BEGIN SELECT 1; SELECT 2; END;";
    let output = Formatter::with_config(&FormatConfig::default().with_semicolons(false))
        .format(source)
        .expect("trigger");
    assert_eq!(output.matches(';').count(), 2, "keep internal terminators");
    assert!(output.ends_with("END\n"));
}

#[test]
fn production_observers_are_scoped_to_each_thread() {
    std::thread::scope(|scope| {
        for index in 0..8 {
            scope.spawn(move || {
                let mut formatter = syntaqlite::Formatter::new();
                for _ in 0..16 {
                    let sql = format!("select {index} /* own */;");
                    assert_eq!(
                        formatter.format(&sql).expect("independent parser"),
                        format!("SELECT {index} /* own */;\n")
                    );
                }
            });
        }
    });
}
