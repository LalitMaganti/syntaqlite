// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Constructs `SQLite` rejects at parse time must not be accepted.
//!
//! Each case here was verified against sqlite3: the `reject` SQL produces a
//! prepare-time error, the neighbouring `accept` SQL does not.

use syntaqlite::{ParseOutcome, Parser};

fn parses(sql: &str) -> bool {
    let parser = Parser::new();
    let mut session = parser.parse(sql);
    loop {
        match session.next() {
            ParseOutcome::Ok(_) => {}
            ParseOutcome::Err(_) => return false,
            ParseOutcome::Done => return true,
        }
    }
}

#[track_caller]
fn reject(sql: &str) {
    assert!(!parses(sql), "should have been rejected: {sql}");
}

#[track_caller]
fn accept(sql: &str) {
    assert!(parses(sql), "should have been accepted: {sql}");
}

/// `a JOIN clause is required before ON/USING` (build.c sqlite3SrcListAppendFromTerm).
#[test]
fn dangling_on_using_without_join() {
    reject("SELECT * FROM t1 USING(a);");
    reject("SELECT a FROM t1 AS t ON b;");
    reject("SELECT * FROM (SELECT 1) ON x;");
    reject("SELECT * FROM tvf(1) USING(a);");

    accept("SELECT * FROM t1 JOIN t2 USING(a);");
    accept("SELECT * FROM t1 AS t JOIN t2 ON t.a = t2.a;");
    accept("SELECT * FROM t1;");
}

/// `syntax error after column name` (parse.y parserAddExprIdListTerm).
#[test]
fn eidlist_rejects_collate_and_sort_order() {
    reject("WITH c(a COLLATE nocase) AS (SELECT 1) SELECT * FROM c;");
    reject("CREATE VIEW v(a DESC) AS SELECT 1;");
    reject("CREATE VIEW v(a ASC) AS SELECT 1;");
    reject("CREATE TABLE t(a, b, FOREIGN KEY(a COLLATE nocase) REFERENCES p(x));");

    accept("WITH c(a) AS (SELECT 1) SELECT * FROM c;");
    accept("CREATE VIEW v(a) AS SELECT 1;");
    accept("CREATE INDEX i ON t(a COLLATE nocase DESC);");
}

/// `cannot use RETURNING in a trigger` (trigger.c).
#[test]
fn returning_inside_trigger_body() {
    reject(
        "CREATE TRIGGER tr AFTER INSERT ON t BEGIN INSERT INTO u VALUES (1,2) RETURNING a; END;",
    );
    reject("CREATE TRIGGER tr AFTER INSERT ON t BEGIN DELETE FROM u RETURNING a; END;");
    reject("CREATE TRIGGER tr AFTER INSERT ON t BEGIN UPDATE u SET a=1 RETURNING a; END;");

    accept("CREATE TRIGGER tr AFTER INSERT ON t BEGIN INSERT INTO u VALUES (1,2); END;");
    accept("INSERT INTO u VALUES (1,2) RETURNING a;");
}

/// `temporary trigger may not have qualified name` (trigger.c sqlite3BeginTrigger).
#[test]
fn temp_trigger_with_qualified_name() {
    reject("CREATE TEMP TRIGGER main.tr AFTER INSERT ON t BEGIN SELECT 1; END;");
    reject("CREATE TEMPORARY TRIGGER main.tr AFTER INSERT ON t BEGIN SELECT 1; END;");

    accept("CREATE TEMP TRIGGER tr AFTER INSERT ON t BEGIN SELECT 1; END;");
    accept("CREATE TRIGGER main.tr AFTER INSERT ON t BEGIN SELECT 1; END;");
}

/// `#N` register references are only legal inside a nested parse (parse.y expr ::= VARIABLE).
#[test]
fn register_references() {
    reject("SELECT #1;");
    reject("SELECT #12 + 1;");

    accept("SELECT ?1;");
    accept("SELECT :name;");
    accept("SELECT $x;");
    accept("SELECT @y;");
}

/// `unrecognized token` for malformed digit separators (util.c sqlite3DequoteNumber).
#[test]
fn malformed_digit_separators() {
    reject("SELECT 1_;");
    reject("SELECT 1__0;");
    reject("SELECT 1_000_;");
    reject("SELECT 1_.0;");
    reject("SELECT 1._0;");
    reject("SELECT 0x_1;");

    accept("SELECT 1_000;");
    accept("SELECT 1_0.5_0;");
    accept("SELECT 0x1_f;");
    accept("SELECT 1;");
}

/// `error in generated column` for any storage keyword other than STORED/VIRTUAL
/// (build.c sqlite3AddGenerated); quoted spellings are rejected too.
#[test]
fn generated_column_storage_keyword() {
    reject("CREATE TABLE t(k, a AS (1) FOO);");
    reject("CREATE TABLE t(k, a AS (1) \"stored\");");
    reject("CREATE TABLE t(k, a AS (1) [stored]);");
    reject("CREATE TABLE t(k, a AS (1) 'stored');");

    accept("CREATE TABLE g(k, a AS (1) STORED);");
    accept("CREATE TABLE g(k, a AS (1) Virtual);");
    accept("CREATE TABLE g(k, a AS (1));");
    accept("CREATE TABLE g(k, a GENERATED ALWAYS AS (1) STORED);");
}
