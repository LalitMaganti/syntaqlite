// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Low-level API integration tests.

use syntaqlite::nodes::Stmt;
use syntaqlite::parse::ParserConfig;
use syntaqlite::parse::TokenType;
use syntaqlite::{ParseOutcome, Parser};

/// Feed tokens for "SELECT 1" via the low-level API and verify same AST
/// as the high-level parse.
#[test]
fn feed_tokens_select_1() {
    let source = "SELECT 1";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    // Feed SELECT token.
    assert!(session.feed_token(TokenType::Select, 0..6).is_none());

    // Feed integer literal token.
    assert!(session.feed_token(TokenType::Integer, 7..8).is_none());

    // finish() synthesizes SEMI + EOF, triggering the ecmd reduction.
    let stmt = session
        .finish()
        .expect("expected Some")
        .expect("expected a statement");
    assert!(matches!(stmt.root(), Some(Stmt::SelectStmt(_))));
}

/// Feed tokens with an explicit SEMI. SEMI immediately completes the statement —
/// no lookahead from the next token is required.
#[test]
fn feed_tokens_with_semicolon() {
    let source = "SELECT 1;";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    session.feed_token(TokenType::Select, 0..6);
    session.feed_token(TokenType::Integer, 7..8);

    // SEMI completes the statement immediately.
    let stmt = session
        .feed_token(TokenType::Semi, 8..9)
        .expect("SEMI should complete the statement")
        .expect("expected Ok");
    assert!(matches!(stmt.root(), Some(Stmt::SelectStmt(_))));
}

/// Multiple statements: SEMI immediately completes the first statement,
/// so the second statement's tokens are entirely independent.
#[test]
fn feed_tokens_multi_statement() {
    let source = "SELECT 1; SELECT 2";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    // First statement: SELECT 1 ;
    session.feed_token(TokenType::Select, 0..6);
    session.feed_token(TokenType::Integer, 7..8);

    // SEMI completes stmt 1 immediately.
    let stmt1 = session.feed_token(TokenType::Semi, 8..9);
    assert!(stmt1.is_some(), "first statement should complete on SEMI");

    // Second statement tokens belong entirely to stmt 2.
    session.feed_token(TokenType::Select, 10..16);
    session.feed_token(TokenType::Integer, 17..18);

    assert!(
        session.finish().is_some(),
        "second statement should complete"
    );
}

/// `TK_SPACE` should be silently ignored.
#[test]
fn feed_token_skips_space() {
    let source = "SELECT 1";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    session.feed_token(TokenType::Select, 0..6);

    // Feed a space — should be silently skipped.
    assert!(session.feed_token(TokenType::Space, 6..7).is_none());

    session.feed_token(TokenType::Integer, 7..8);

    let stmt = session
        .finish()
        .expect("expected Some")
        .expect("expected a statement");
    assert!(matches!(stmt.root(), Some(Stmt::SelectStmt(_))));
}

/// `TK_COMMENT` should be recorded as a comment.
#[test]
fn feed_token_records_comment() {
    let source = "SELECT -- hello\n1";
    let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
    let mut session = parser.incremental_parse(source);

    session.feed_token(TokenType::Select, 0..6);
    session.feed_token(TokenType::Comment, 7..15);
    session.feed_token(TokenType::Integer, 16..17);

    let stmt = session
        .finish()
        .expect("expected Some")
        .expect("expected a statement");

    let comments: Vec<_> = stmt.comments().collect();
    assert_eq!(comments.len(), 1);
    assert_eq!(comments[0].length(), 8);
}

/// `begin_macro` / `end_macro` records macro regions.
#[test]
fn macro_regions_recorded() {
    let source = "SELECT 1";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    session.begin_macro(7..7 + 13);
    session.feed_token(TokenType::Select, 0..6);
    session.feed_token(TokenType::Integer, 7..8);
    session.end_macro();

    let stmt = session
        .finish()
        .expect("expected Some")
        .expect("expected a statement");

    let regions: Vec<_> = stmt.erase().macro_regions().collect();
    assert_eq!(regions.len(), 1);
    assert_eq!(regions[0].call_offset(), 7);
    assert_eq!(regions[0].call_length(), 13);
}

/// Nested macro regions are both recorded.
#[test]
fn nested_macro_regions() {
    let source = "SELECT 1";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    session.begin_macro(0..30);
    session.begin_macro(10..10 + 5);
    session.feed_token(TokenType::Select, 0..6);
    session.feed_token(TokenType::Integer, 7..8);
    session.end_macro();
    session.end_macro();

    let stmt = session
        .finish()
        .expect("expected Some")
        .expect("expected a statement");

    let regions: Vec<_> = stmt.erase().macro_regions().collect();
    assert_eq!(regions.len(), 2);
    assert_eq!(regions[0].call_offset(), 0);
    assert_eq!(regions[0].call_length(), 30);
    assert_eq!(regions[1].call_offset(), 10);
    assert_eq!(regions[1].call_length(), 5);
}

/// A macro that expands to a complete expression (single node) is well-aligned.
/// The parser should accept it without error.
#[test]
fn macro_well_aligned_complete_expression() {
    let source = "SELECT foo!(1 + 2), 3";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    session.feed_token(TokenType::Select, 0..6);

    session.begin_macro(7..7 + 11);
    session.feed_token(TokenType::Integer, 12..13);
    session.feed_token(TokenType::Plus, 14..15);
    session.feed_token(TokenType::Integer, 16..17);
    session.end_macro();

    session.feed_token(TokenType::Comma, 18..19);
    session.feed_token(TokenType::Integer, 20..21);

    let stmt = session
        .finish()
        .expect("expected Some")
        .expect("expected a statement");
    assert!(matches!(stmt.root(), Some(Stmt::SelectStmt(_))));
}

/// A macro whose expanded tokens straddle a node boundary: the schema part of
/// `schema.table` comes from inside the macro but `table` is outside. This
/// produces a `TableRef` with `schema` inside the macro and `table_name`
/// outside — a genuine straddle that the parser must reject.
#[test]
fn macro_straddle_rejected_by_parser() {
    // source layout: "SELECT 1 FROM foo!(s).t"
    //                 0      7 9    14     21 22
    //  macro region: 14..21 covers "foo!(s)"
    //  schema token: Id at 19..20 (the 's', inside macro)
    //  dot:          Dot at 21..22 (outside macro)
    //  table token:  Id at 22..23 (the 't', outside macro)
    let source = "SELECT 1 FROM foo!(s).t";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    session.feed_token(TokenType::Select, 0..6);
    session.feed_token(TokenType::Integer, 7..8);
    session.feed_token(TokenType::From, 9..13);

    session.begin_macro(14..14 + 7); // foo!(s) = 7 chars
    session.feed_token(TokenType::Id, 19..20); // schema 's', inside macro
    session.end_macro();

    session.feed_token(TokenType::Dot, 21..22);
    session.feed_token(TokenType::Id, 22..23); // table name 't', outside macro

    let result = session.finish().expect("should return Some");
    let Err(err) = result else {
        panic!("expected straddle error but parse succeeded");
    };
    assert!(
        err.message().contains("straddle"),
        "expected straddle error, got: {}",
        err.message()
    );
}

/// Multi-statement via `feed_token`: both statements produce correct AST roots.
#[test]
fn feed_tokens_multi_statement_both_roots() {
    let source = "SELECT 1; SELECT 2";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    // First statement.
    session.feed_token(TokenType::Select, 0..6);
    session.feed_token(TokenType::Integer, 7..8);
    let stmt1 = session
        .feed_token(TokenType::Semi, 8..9)
        .expect("stmt 1 should complete")
        .expect("stmt 1 should be Ok");
    assert!(matches!(stmt1.root(), Some(Stmt::SelectStmt(_))));

    // Second statement.
    session.feed_token(TokenType::Select, 10..16);
    session.feed_token(TokenType::Integer, 17..18);
    let stmt2 = session
        .finish()
        .expect("stmt 2 should complete")
        .expect("stmt 2 should be Ok");
    assert!(matches!(stmt2.root(), Some(Stmt::SelectStmt(_))));
}

/// Three statements: the middle one has an explicit SEMI, the last uses `finish()`.
#[test]
fn feed_tokens_three_statements() {
    let source = "SELECT 1; SELECT 2; SELECT 3";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    // Statement 1.
    session.feed_token(TokenType::Select, 0..6);
    session.feed_token(TokenType::Integer, 7..8);
    assert!(session.feed_token(TokenType::Semi, 8..9).is_some());

    // Statement 2.
    session.feed_token(TokenType::Select, 10..16);
    session.feed_token(TokenType::Integer, 17..18);
    assert!(session.feed_token(TokenType::Semi, 18..19).is_some());

    // Statement 3 — completed by finish().
    session.feed_token(TokenType::Select, 20..26);
    session.feed_token(TokenType::Integer, 27..28);
    let stmt3 = session
        .finish()
        .expect("stmt 3 should complete")
        .expect("stmt 3 should be Ok");
    assert!(matches!(stmt3.root(), Some(Stmt::SelectStmt(_))));
}

/// Bare semicolons between statements are silently skipped.
#[test]
fn feed_tokens_bare_semicolons() {
    let source = "; SELECT 1; ;";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    // Leading bare semicolon — should not produce a statement.
    assert!(
        session.feed_token(TokenType::Semi, 0..1).is_none(),
        "bare semicolon should not produce a statement"
    );

    // Real statement.
    session.feed_token(TokenType::Select, 2..8);
    session.feed_token(TokenType::Integer, 9..10);
    let stmt = session
        .feed_token(TokenType::Semi, 10..11)
        .expect("should complete")
        .expect("should be Ok");
    assert!(matches!(stmt.root(), Some(Stmt::SelectStmt(_))));

    // Trailing bare semicolon.
    assert!(
        session.feed_token(TokenType::Semi, 12..13).is_none(),
        "trailing bare semicolon should not produce a statement"
    );

    assert!(session.finish().is_none(), "nothing left");
}

/// EXPLAIN wrapping works correctly across statement boundaries.
#[test]
fn feed_tokens_explain_then_normal() {
    let source = "EXPLAIN SELECT 1; SELECT 2";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    // EXPLAIN SELECT 1;
    session.feed_token(TokenType::Explain, 0..7);
    session.feed_token(TokenType::Select, 8..14);
    session.feed_token(TokenType::Integer, 15..16);
    let stmt1 = session
        .feed_token(TokenType::Semi, 16..17)
        .expect("stmt 1 should complete")
        .expect("stmt 1 should be Ok");
    assert!(
        matches!(stmt1.root(), Some(Stmt::ExplainStmt(_))),
        "first statement should be EXPLAIN"
    );

    // SELECT 2 — should NOT be wrapped in EXPLAIN.
    session.feed_token(TokenType::Select, 18..24);
    session.feed_token(TokenType::Integer, 25..26);
    let stmt2 = session
        .finish()
        .expect("stmt 2 should complete")
        .expect("stmt 2 should be Ok");
    assert!(
        matches!(stmt2.root(), Some(Stmt::SelectStmt(_))),
        "second statement should be plain SELECT, not EXPLAIN"
    );
}

/// Normal statement followed by EXPLAIN — EXPLAIN must not leak backwards.
#[test]
fn feed_tokens_normal_then_explain() {
    let source = "SELECT 1; EXPLAIN SELECT 2";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    // SELECT 1;
    session.feed_token(TokenType::Select, 0..6);
    session.feed_token(TokenType::Integer, 7..8);
    let stmt1 = session
        .feed_token(TokenType::Semi, 8..9)
        .expect("stmt 1 should complete")
        .expect("stmt 1 should be Ok");
    assert!(matches!(stmt1.root(), Some(Stmt::SelectStmt(_))));

    // EXPLAIN SELECT 2
    session.feed_token(TokenType::Explain, 10..17);
    session.feed_token(TokenType::Select, 18..24);
    session.feed_token(TokenType::Integer, 25..26);
    let stmt2 = session
        .finish()
        .expect("stmt 2 should complete")
        .expect("stmt 2 should be Ok");
    assert!(
        matches!(stmt2.root(), Some(Stmt::ExplainStmt(_))),
        "second statement should be EXPLAIN"
    );
}

/// `finish()` on an incomplete statement reports a syntax error.
#[test]
fn feed_tokens_incomplete_statement_error() {
    let source = "SELECT";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    session.feed_token(TokenType::Select, 0..6);
    // finish() synthesizes SEMI + EOF; SELECT alone is incomplete.
    let result = session.finish().expect("should return Some");
    assert!(result.is_err(), "incomplete SELECT should be a parse error");
}

/// Comments between statements belong to the correct statement.
#[test]
fn feed_tokens_comments_between_statements() {
    let source = "SELECT 1; -- between\nSELECT 2";
    let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
    let mut session = parser.incremental_parse(source);

    // Statement 1.
    session.feed_token(TokenType::Select, 0..6);
    session.feed_token(TokenType::Integer, 7..8);
    let stmt1 = session
        .feed_token(TokenType::Semi, 8..9)
        .expect("stmt 1 should complete")
        .expect("stmt 1 should be Ok");
    assert_eq!(
        stmt1.comments().count(),
        0,
        "stmt 1 should have no comments"
    );

    // Inter-statement comment belongs to statement 2.
    session.feed_token(TokenType::Comment, 10..20);
    session.feed_token(TokenType::Select, 21..27);
    session.feed_token(TokenType::Integer, 28..29);
    let stmt2 = session
        .finish()
        .expect("stmt 2 should complete")
        .expect("stmt 2 should be Ok");
    assert_eq!(
        stmt2.comments().count(),
        1,
        "stmt 2 should have the inter-statement comment"
    );
}

/// `finish()` without feeding any tokens returns None.
#[test]
fn finish_with_no_tokens() {
    let source = "";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    assert!(session.finish().is_none());
}

/// High-level API still works after the refactor.
#[test]
fn high_level_api_still_works() {
    let parser = Parser::new();
    let mut session = parser.parse("SELECT 1; SELECT 2");

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

/// Batch parser: bare semicolons are skipped, real statements are returned.
#[test]
fn batch_parse_bare_semicolons() {
    let parser = Parser::new();
    let mut session = parser.parse("; SELECT 1; ; SELECT 2; ;");

    let ParseOutcome::Ok(stmt1) = session.next() else {
        panic!("expected Ok for stmt 1")
    };
    assert!(matches!(stmt1.root(), Some(Stmt::SelectStmt(_))));

    let ParseOutcome::Ok(stmt2) = session.next() else {
        panic!("expected Ok for stmt 2")
    };
    assert!(matches!(stmt2.root(), Some(Stmt::SelectStmt(_))));

    assert!(matches!(session.next(), ParseOutcome::Done));
}

/// Batch parser: EXPLAIN followed by a normal statement.
#[test]
fn batch_parse_explain_then_normal() {
    let parser = Parser::new();
    let mut session = parser.parse("EXPLAIN SELECT 1; SELECT 2");

    let ParseOutcome::Ok(stmt1) = session.next() else {
        panic!("expected Ok for stmt 1")
    };
    assert!(
        matches!(stmt1.root(), Some(Stmt::ExplainStmt(_))),
        "stmt 1 should be EXPLAIN"
    );

    let ParseOutcome::Ok(stmt2) = session.next() else {
        panic!("expected Ok for stmt 2")
    };
    assert!(
        matches!(stmt2.root(), Some(Stmt::SelectStmt(_))),
        "stmt 2 should be plain SELECT"
    );

    assert!(matches!(session.next(), ParseOutcome::Done));
}

/// Type names in `SQLite` type contexts should be marked with `AS_TYPE` so
/// semantic highlighting can render them as `type`.
#[test]
fn sqlite_type_tokens_are_marked_as_type() {
    let source = "CREATE TABLE t(a int, b TEXT); SELECT CAST(a AS varchar(10)) FROM t";
    let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
    let mut session = parser.parse(source);

    let mut marked = Vec::new();
    loop {
        match session.next() {
            ParseOutcome::Ok(stmt) => {
                for t in stmt.tokens() {
                    if t.flags().used_as_type() {
                        marked.push(
                            source[t.offset() as usize..(t.offset() + t.length()) as usize]
                                .to_string(),
                        );
                    }
                }
            }
            ParseOutcome::Err(e) => panic!("parse error: {}", e.message()),
            ParseOutcome::Done => break,
        }
    }

    assert_eq!(marked, vec!["int", "TEXT", "varchar"]);
}

/// Walk an AST collecting `(field_idx, source_start, source_end)` for every
/// span field, recursing through child nodes and list children.
fn collect_span_ranges(
    erased: &syntaqlite_syntax::any::AnyParsedStatement<'_>,
    node_id: syntaqlite_syntax::any::AnyNodeId,
    spans: &mut Vec<(u8, usize, usize)>,
) {
    use syntaqlite_syntax::any::FieldValue;
    if node_id.is_null() {
        return;
    }
    if let Some((_, fields)) = erased.extract_fields(node_id) {
        for idx in 0..fields.len() {
            let field_idx = u8::try_from(idx).expect("field index fits in u8");
            match fields[idx] {
                FieldValue::Span(sp) => {
                    let (text, off) = erased.span_text(sp);
                    let start = off as usize;
                    spans.push((field_idx, start, start + text.len()));
                }
                FieldValue::NodeId(child) if !child.is_null() => {
                    collect_span_ranges(erased, child, spans);
                }
                _ => {}
            }
        }
    }
    if let Some(children) = erased.list_children(node_id) {
        for &child in children {
            collect_span_ranges(erased, child, spans);
        }
    }
}

/// `FieldValue::Span.source` carries the direct source position for non-macro spans.
#[test]
fn field_source_range_direct_span() {
    // "SELECT 1"
    //  0      7
    let source = "SELECT 1";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    session.feed_token(TokenType::Select, 0..6);
    session.feed_token(TokenType::Integer, 7..8);

    let stmt = session
        .finish()
        .expect("expected Some")
        .expect("expected a statement");
    let erased = stmt.erase();

    // Walk the entire AST and collect all span ranges.
    let mut spans = Vec::new();
    collect_span_ranges(&erased, erased.root_id(), &mut spans);
    eprintln!("spans: {spans:?}");

    // The integer "1" should be at offset 7..8.
    assert!(
        spans.iter().any(|&(_, start, end)| start == 7 && end == 8),
        "expected to find span at range 7..8, got: {spans:?}"
    );
}

/// `FieldValue::Span.source` on a macro-expanded span returns the macro call site.
#[test]
fn field_source_range_macro_expansion() {
    // "SELECT foo!(1 + 2), 3"
    //  0      7          18 20
    // The macro call "foo!(1 + 2)" is at offset 7, length 11.
    // Tokens inside the expansion get spans relative to the expansion buffer,
    // but field_source_range should map them back to the call site.
    let source = "SELECT foo!(1 + 2), 3";
    let parser = Parser::new();
    let mut session = parser.incremental_parse(source);

    session.feed_token(TokenType::Select, 0..6);

    // Macro call at 7..18 = "foo!(1 + 2)"
    session.begin_macro(7..18);
    session.feed_token(TokenType::Integer, 0..1); // "1" in expansion buffer
    session.feed_token(TokenType::Plus, 2..3);
    session.feed_token(TokenType::Integer, 4..5);
    session.end_macro();

    session.feed_token(TokenType::Comma, 18..19);
    session.feed_token(TokenType::Integer, 20..21);

    let stmt = session
        .finish()
        .expect("expected Some")
        .expect("expected a statement");

    let erased = stmt.erase();

    // Collect all spans from the AST.
    let mut spans = Vec::new();
    collect_span_ranges(&erased, erased.root_id(), &mut spans);
    eprintln!("macro spans: {spans:?}");

    // Tokens inside the macro expansion should resolve to the call site 7..18.
    // The "3" outside should be at 20..21.
    assert!(
        spans.iter().any(|&(_, start, end)| start == 7 && end == 18),
        "expected to find a span mapped to macro call site 7..18, got: {spans:?}"
    );
}
