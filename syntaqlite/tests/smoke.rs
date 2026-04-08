// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Smoke tests for the core syntaqlite API.

use syntaqlite::any::AnyNodeTag;
use syntaqlite::nodes::{NodeTag, Stmt};
use syntaqlite::typed::{TypedParser, grammar};
use syntaqlite::util::{SqliteSyntaxFlag, SqliteSyntaxFlags};
use syntaqlite::{ParseOutcome, Parser};

fn new_parser() -> Parser {
    Parser::new()
}

#[test]
fn parse_select_1() {
    let parser = new_parser();
    let mut session = parser.parse("SELECT 1;");

    let ParseOutcome::Ok(stmt) = session.next() else {
        panic!("expected Ok")
    };
    assert!(matches!(stmt.root(), Some(Stmt::SelectStmt(_))));

    // No more statements.
    assert!(matches!(session.next(), ParseOutcome::Done));
}

#[test]
fn parse_multiple_statements() {
    let parser = new_parser();
    let mut session = parser.parse("SELECT 1; SELECT 2;");

    let ParseOutcome::Ok(stmt1) = session.next() else {
        panic!("expected Ok")
    };
    assert!(matches!(stmt1.root(), Some(Stmt::SelectStmt(_))));

    let ParseOutcome::Ok(stmt2) = session.next() else {
        panic!("expected Ok")
    };
    assert!(matches!(stmt2.root(), Some(Stmt::SelectStmt(_))));

    assert!(matches!(session.next(), ParseOutcome::Done));
}

#[test]
fn parse_error() {
    let parser = new_parser();
    let mut session = parser.parse("SELECT");

    assert!(matches!(session.next(), ParseOutcome::Err(_)));
}

#[test]
fn parse_error_select_bare() {
    // "SELECT " with trailing space — no column list, no semicolon.
    // Should return an error with a non-empty message, not silently return None.
    let parser = new_parser();
    let mut session = parser.parse("SELECT ");

    let ParseOutcome::Err(err) = session.next() else {
        panic!("expected parse error for bare SELECT");
    };
    assert!(
        !err.message().is_empty(),
        "error message should not be empty, got: {:?}",
        err.message()
    );
}

#[test]
fn parse_error_has_message_and_offset() {
    // A syntax error should carry a non-empty message.
    let parser = new_parser();
    let mut session = parser.parse("NOT VALID SQL;");

    let ParseOutcome::Err(err) = session.next() else {
        panic!("expected parse error")
    };
    assert!(
        !err.message().is_empty(),
        "error message should not be empty"
    );
}

#[test]
fn parse_error_recovery() {
    // After a parse error, the cursor continues parsing subsequent statements.
    // Lemon's built-in error recovery synchronises on `;`.
    let parser = new_parser();
    let mut session = parser.parse("NOT VALID SQL; SELECT 1;");

    assert!(
        matches!(session.next(), ParseOutcome::Err(_)),
        "expected parse error for invalid SQL"
    );

    // Recovery: cursor should continue and return the next valid statement.
    let ParseOutcome::Ok(stmt) = session.next() else {
        panic!("expected SelectStmt after recovery");
    };
    assert!(
        matches!(stmt.root(), Some(Stmt::SelectStmt(_))),
        "expected SelectStmt after recovery"
    );

    assert!(matches!(session.next(), ParseOutcome::Done));
}

#[test]
fn parse_error_recovery_at_eof() {
    // An unterminated statement (no trailing `;`) reports an error and then
    // next() returns Done.
    let parser = new_parser();
    let mut session = parser.parse("SELECT * FROM");

    assert!(matches!(session.next(), ParseOutcome::Err(_)));
    assert!(matches!(session.next(), ParseOutcome::Done));
}

#[test]
fn parse_error_mid_batch() {
    // Good → bad → good: the cursor recovers from a mid-batch error and
    // continues to parse subsequent valid statements.
    let parser = new_parser();
    let mut session = parser.parse("SELECT 1; SELECT * FROM; SELECT 2;");

    let ParseOutcome::Ok(stmt1) = session.next() else {
        panic!("expected Ok")
    };
    assert!(matches!(stmt1.root(), Some(Stmt::SelectStmt(_))));

    assert!(matches!(session.next(), ParseOutcome::Err(_)));

    let ParseOutcome::Ok(stmt3) = session.next() else {
        panic!("expected Ok")
    };
    assert!(matches!(stmt3.root(), Some(Stmt::SelectStmt(_))));

    assert!(matches!(session.next(), ParseOutcome::Done));
}

#[test]
fn parser_reuse() {
    let parser = new_parser();

    // First parse
    {
        let mut session = parser.parse("SELECT 1");
        let ParseOutcome::Ok(stmt) = session.next() else {
            panic!("expected Ok")
        };
        assert!(matches!(stmt.root(), Some(Stmt::SelectStmt(_))));
    }

    // Reuse with different input
    {
        let mut session = parser.parse("DELETE FROM t");
        let ParseOutcome::Ok(stmt) = session.next() else {
            panic!("expected Ok")
        };
        assert!(matches!(stmt.root(), Some(Stmt::DeleteStmt(_))));
    }
}

// -- DELETE / UPDATE with ORDER BY and LIMIT --

#[test]
fn parse_delete_with_order_by_limit() {
    let g = grammar()
        .with_cflags(SqliteSyntaxFlags::default().with(SqliteSyntaxFlag::EnableUpdateDeleteLimit));
    let parser = TypedParser::new(g);
    let mut session = parser.parse("DELETE FROM t ORDER BY id LIMIT 5;");

    let ParseOutcome::Ok(stmt) = session.next() else {
        panic!("expected Ok")
    };
    let root = stmt.root().expect("expected root");
    let Stmt::DeleteStmt(del) = root else {
        panic!("expected DeleteStmt, got {root:?}");
    };
    assert!(del.orderby().is_some(), "should have ORDER BY");
    assert!(del.limit_clause().is_some(), "should have LIMIT");
}

#[test]
fn parse_update_with_order_by_limit() {
    let g = grammar()
        .with_cflags(SqliteSyntaxFlags::default().with(SqliteSyntaxFlag::EnableUpdateDeleteLimit));
    let parser = TypedParser::new(g);
    let mut session = parser.parse("UPDATE t SET a = 1 ORDER BY id LIMIT 3;");

    let ParseOutcome::Ok(stmt) = session.next() else {
        panic!("expected Ok")
    };
    let root = stmt.root().expect("expected root");
    let Stmt::UpdateStmt(upd) = root else {
        panic!("expected UpdateStmt, got {root:?}");
    };
    assert!(upd.orderby().is_some(), "should have ORDER BY");
    assert!(upd.limit_clause().is_some(), "should have LIMIT");
}

#[test]
fn table_qualified_star_qualifier_in_expr_not_alias() {
    // SELECT t.* — "t" is the table qualifier, NOT an alias.
    // Regression: the parser used to swap the alias/expr arguments in
    // synq_parse_result_column() for the `nm DOT STAR` rule.
    let parser = Parser::new();
    let mut session = parser.parse("SELECT t.*");
    let ParseOutcome::Ok(stmt) = session.next() else {
        panic!("expected Ok");
    };

    // Verify via the typed API that flags=STAR and alias=None.
    let root = stmt.root().expect("expected root");
    let Stmt::SelectStmt(select) = root else {
        panic!("expected SelectStmt, got {root:?}");
    };
    let columns = select.columns().expect("expected result columns");
    let col = columns
        .iter()
        .next()
        .expect("expected at least one result column");
    assert!(col.flags().star(), "STAR flag should be set for t.*");
    assert!(
        col.alias().is_none(),
        "'t' is a table qualifier, not an alias — alias should be None"
    );
    let rc_id = col.node_id().into_inner();

    // Verify that "t" lives in the expr field as an IdentName child.
    let any_stmt = stmt.erase(); // borrows &self, no move
    let ident_tag = AnyNodeTag::from(NodeTag::IdentName);
    let has_ident_child = any_stmt
        .child_node_ids(rc_id)
        .any(|id| any_stmt.extract_fields(id).map(|(tag, _)| tag) == Some(ident_tag));
    assert!(
        has_ident_child,
        "expected IdentName child in ResultColumn expr field for t.*"
    );
}

#[test]
fn dump_star_column_flags() {
    // SELECT * should produce a ResultColumn with flags: STAR only — no stray bits.
    let parser = new_parser();
    let mut session = parser.parse("SELECT * FROM t");
    let ParseOutcome::Ok(stmt) = session.next() else {
        panic!("expected Ok");
    };
    let mut out = String::new();
    stmt.dump(&mut out, 0);
    eprintln!("dump output:\n{out}");
    assert!(
        out.contains("flags: STAR\n") && !out.contains("flags: STAR ?"),
        "expected 'flags: STAR' with no extra bits, got:\n{out}"
    );
}

#[test]
fn comment_only_returns_ok_with_none_root() {
    // Comment-only input should return ParseOutcome::Ok (matching SQLite's
    // sqlite3_prepare_v2 behavior) but root() should be None.
    let parser = new_parser();

    for sql in ["/* block comment */", "-- line comment", "/* c1 */ -- c2"] {
        let mut session = parser.parse(sql);
        let ParseOutcome::Ok(stmt) = session.next() else {
            panic!("expected Ok for comment-only input: {sql:?}");
        };
        assert!(
            stmt.root().is_none(),
            "root() should be None for comment-only input: {sql:?}"
        );
        assert!(matches!(session.next(), ParseOutcome::Done));
    }
}

#[test]
fn whitespace_only_returns_done() {
    // Pure whitespace (no comments) should return Done, not Ok.
    let parser = new_parser();
    let mut session = parser.parse("   \n\t  ");
    assert!(matches!(session.next(), ParseOutcome::Done));
}
