#![cfg(all(feature = "fmt", feature = "sqlite"))]
//! Integration tests for public formatter keyword-case configuration.

use syntaqlite::fmt::KeywordCase;
use syntaqlite::{FormatConfig, Formatter};

#[test]
fn keyword_case_upper() {
    let cfg = FormatConfig::default().with_keyword_case(KeywordCase::Upper);
    let mut fmt = Formatter::with_config(&cfg);
    let out = fmt
        .format("select 1")
        .expect("formatting should succeed for valid SQL");
    assert_eq!(out, "SELECT 1;\n");
}

#[test]
fn keyword_case_lower() {
    let cfg = FormatConfig::default().with_keyword_case(KeywordCase::Lower);
    let mut fmt = Formatter::with_config(&cfg);
    let out = fmt
        .format("SELECT 1")
        .expect("formatting should succeed for valid SQL");
    assert_eq!(out, "select 1;\n");
}

#[test]
fn contextual_keywords_follow_case_without_changing_names() {
    let cases = [
        (
            "create table t(x int primary key) without rowid, strict",
            "CREATE TABLE t (x int PRIMARY KEY) WITHOUT ROWID, STRICT;\n",
            "create table t (x int primary key) without rowid, strict;\n",
        ),
        (
            "alter table t add column x",
            "ALTER TABLE t ADD COLUMN x;\n",
            "alter table t add column x;\n",
        ),
        (
            "select * from a outer left natural join b",
            "SELECT * FROM a OUTER LEFT NATURAL JOIN b;\n",
            "select * from a outer left natural join b;\n",
        ),
        (
            "create table t(x int generated always as (1) stored)",
            "CREATE TABLE t (x int GENERATED ALWAYS AS (1) STORED);\n",
            "create table t (x int generated always as (1) stored);\n",
        ),
        (
            "select x isnull, y notnull from t",
            "SELECT x ISNULL, y NOTNULL FROM t;\n",
            "select x isnull, y notnull from t;\n",
        ),
        (
            "select column, natural, stored from t",
            "SELECT column, natural, stored FROM t;\n",
            "select column, natural, stored from t;\n",
        ),
    ];
    for (source, upper, lower) in cases {
        for (case, expected) in [(KeywordCase::Upper, upper), (KeywordCase::Lower, lower)] {
            let config = FormatConfig::default().with_keyword_case(case);
            let mut formatter = Formatter::with_config(&config);
            let output = formatter.format(source).expect("valid SQL");
            assert_eq!(output, expected);
        }
    }
}

#[test]
fn contextual_keywords_lower_from_uppercase_input() {
    let config = FormatConfig::default().with_keyword_case(KeywordCase::Lower);
    let mut formatter = Formatter::with_config(&config);
    for (source, expected) in [
        (
            "CREATE TABLE T(X INT PRIMARY KEY) WITHOUT ROWID, STRICT",
            "create table T (X INT primary key) without rowid, strict;\n",
        ),
        (
            "ALTER TABLE T ADD COLUMN X",
            "alter table T add column X;\n",
        ),
        (
            "SELECT * FROM A OUTER LEFT NATURAL JOIN B",
            "select * from A outer left natural join B;\n",
        ),
        (
            "CREATE TABLE T(X INT GENERATED ALWAYS AS (1) STORED)",
            "create table T (X INT generated always as (1) stored);\n",
        ),
        (
            "SELECT X ISNULL, Y NOTNULL FROM T",
            "select X isnull, Y notnull from T;\n",
        ),
        (
            "SELECT COLUMN, NATURAL, STORED FROM T",
            "select COLUMN, NATURAL, STORED from T;\n",
        ),
    ] {
        assert_eq!(formatter.format(source).expect("valid SQL"), expected);
    }
}

#[test]
fn keyword_case_preserves_comments_type_spelling_and_opaque_arguments() {
    for (source, expected) in [
        (
            "create table t(x decimal(10 , 2) generated always as (1) /*keep*/ stored)",
            "CREATE TABLE t (x decimal(10 , 2) GENERATED ALWAYS AS (1) /*keep*/ STORED);\n",
        ),
        (
            "select * from a left /*keep*/ outer join b",
            "SELECT * FROM a LEFT /*keep*/ OUTER JOIN b;\n",
        ),
        (
            "create virtual table t using module(select, column, stored)",
            "CREATE VIRTUAL TABLE t USING module (select, column, stored);\n",
        ),
    ] {
        let mut formatter = Formatter::new();
        let output = formatter.format(source).expect("valid SQL");
        assert_eq!(output, expected);
        assert_eq!(formatter.format(&output).expect("valid output"), output);
    }
}
