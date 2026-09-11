//! Structural token-layout experiment: preserve lexical identity and converge.
#![cfg(all(feature = "fmt", feature = "sqlite"))]
use syntaqlite::fmt::token_layout_prototype::TokenFormatter;
use syntaqlite_syntax::Tokenizer;

fn tokens(sql: &str) -> Vec<String> {
    Tokenizer::new()
        .tokenize(sql)
        .filter(|token| !token.text().trim().is_empty())
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
    let mut formatter = TokenFormatter::default();
    for seed in seeds {
        let mut gaps = vec![0];
        gaps.extend(
            Tokenizer::new()
                .tokenize(seed)
                .filter(|token| !token.text().trim().is_empty())
                .map(|token| {
                    token.text().as_ptr() as usize - seed.as_ptr() as usize + token.text().len()
                }),
        );
        for width in [30, 80] {
            for gap in &gaps {
                for comment in [" /* probe */ ", " -- probe\n"] {
                    let input = format!("{}{comment}{}", &seed[..*gap], &seed[*gap..]);
                    let (output, _) = formatter.format(&input, width).expect("valid input");
                    assert_eq!(tokens(&input), tokens(&output), "{input}\n{output}");
                    let (second, _) = formatter.format(&output, width).expect("valid output");
                    assert_eq!(output, second, "{input}");
                }
            }
        }
    }
}

#[test]
fn no_syntax_normalization_or_source_matching() {
    let mut formatter = TokenFormatter::default();
    for input in [
        "SELECT a == /* operator */ b;",
        "SELECT a NOT NULL;",
        "SELECT a FROM t ORDER BY a ASC LIMIT 20, 10;",
    ] {
        let (output, _) = formatter.format(input, 80).expect("valid input");
        eprintln!("{input}\n{output}");
        assert_eq!(output, input);
    }
}
