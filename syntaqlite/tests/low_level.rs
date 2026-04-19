// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Low-level API integration tests.

use syntaqlite::nodes::Stmt;
use syntaqlite::parse::ParserConfig;
use syntaqlite::parse::TokenType;
use syntaqlite::{ParseOutcome, Parser};
use syntaqlite_syntax::{MacroArg, MacroLookup, MacroOutput};

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
    assert_eq!(comments[0].length(), syntaqlite_syntax::source::StmtLen::from_raw(8));
}

/// Leading `TK_SPACE` fed via `feed_token` should NOT open the statement:
/// `stmt_start` should match `parser_next` semantics (first significant byte).
/// This keeps statement-relative offsets consistent between the tokenizer-
/// driven and incremental paths.
#[test]
fn feed_token_leading_whitespace_does_not_open_statement() {
    let source = "   SELECT 1";
    let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
    let mut session = parser.incremental_parse(source);

    session.feed_token(TokenType::Space, 0..3);
    session.feed_token(TokenType::Select, 3..9);
    session.feed_token(TokenType::Integer, 10..11);

    let stmt = session
        .finish()
        .expect("expected Some")
        .expect("expected a statement");

    // Statement opens at `SELECT`, not at byte 0 — matches parser_next.
    assert_eq!(stmt.text(), "SELECT 1");
    assert_eq!(stmt.statement_base().as_doc_offset().as_u32(), 3);
    let tokens: Vec<_> = stmt.tokens().collect();
    assert_eq!(tokens[0].offset(), syntaqlite_syntax::source::StmtOffset::default());
    assert_eq!(tokens[0].text(), "SELECT");
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
                let stmt_text = stmt.text();
                for t in stmt.tokens() {
                    if t.flags().used_as_type() {
                        let range = syntaqlite_syntax::source::StmtRange::from_offset_len(
                            t.offset(),
                            t.length(),
                        );
                        marked.push(stmt_text[range].to_string());
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
                    let start = off.as_usize();
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

/// Walk an AST collecting all non-empty `TextSpan` fields recursively.
fn collect_all_spans(
    stmt: &syntaqlite_syntax::any::AnyParsedStatement<'_>,
    node_id: syntaqlite_syntax::any::AnyNodeId,
    out: &mut Vec<syntaqlite_syntax::any::TextSpan>,
) {
    use syntaqlite_syntax::any::FieldValue;
    if node_id.is_null() {
        return;
    }
    if let Some((_, fields)) = stmt.extract_fields(node_id) {
        for idx in 0..fields.len() {
            match fields[idx] {
                FieldValue::Span(sp) if !sp.is_empty() => out.push(sp),
                FieldValue::NodeId(child) if !child.is_null() => {
                    collect_all_spans(stmt, child, out);
                }
                _ => {}
            }
        }
    }
    if let Some(children) = stmt.list_children(node_id) {
        for &child in children {
            collect_all_spans(stmt, child, out);
        }
    }
}

/// Nested macro calls inside an expansion buffer must be detected even when
/// whitespace or comments separate the ID from '!'. Regression test for
/// <https://github.com/LalitMaganti/syntaqlite/issues/130>.
#[test]
fn nested_macro_with_whitespace_before_bang() {
    struct TwoMacros;
    impl MacroLookup for TwoMacros {
        fn lookup(&mut self, name: &str, _args: &[MacroArg<'_>], out: &mut MacroOutput) -> bool {
            match name {
                // omacro!() expands to "imacro\n  !(42)" — newline+spaces before "!".
                "omacro" => {
                    out.write("imacro\n  !(42)");
                    true
                }
                // cmacro!() expands to "imacro/* comment */!(42)" — comment before "!".
                "cmacro" => {
                    out.write("imacro/* comment */!(42)");
                    true
                }
                "imacro" => {
                    out.write("100");
                    true
                }
                _ => false,
            }
        }
    }

    let config = ParserConfig::default()
        .with_collect_tokens(true)
        .with_macro_fallback(true);

    // Test whitespace (newline + spaces) between ID and '!'.
    let mut parser = Parser::with_config(&config);
    parser.set_macro_lookup(Some(Box::new(TwoMacros)));
    let mut session = parser.parse("SELECT omacro!();");
    let ParseOutcome::Ok(_) = session.next() else {
        panic!("nested macro with whitespace before '!' should parse")
    };

    // Test comment between ID and '!'.
    let mut parser = Parser::with_config(&config);
    parser.set_macro_lookup(Some(Box::new(TwoMacros)));
    let mut session = parser.parse("SELECT cmacro!();");
    let ParseOutcome::Ok(_) = session.next() else {
        panic!("nested macro with comment before '!' should parse")
    };
}

/// When a single statement has more than 255 macro expansions, the parser
/// must not wrap `_layer_id` at 256 (`uint8_t` overflow). Regression test
/// for <https://github.com/LalitMaganti/syntaqlite/issues/128>.
#[test]
fn layer_id_no_overflow_at_256_expansions() {
    struct ConstMacro;
    impl MacroLookup for ConstMacro {
        fn lookup(&mut self, _name: &str, _args: &[MacroArg<'_>], out: &mut MacroOutput) -> bool {
            out.write("42");
            true
        }
    }

    // Build a SELECT with 260 macro calls: SELECT mm!(), mm!(), ..., mm!();
    // Each call creates one expansion layer, pushing well past 255.
    let mut sql = String::from("SELECT ");
    for i in 0..260 {
        if i > 0 {
            sql.push_str(", ");
        }
        sql.push_str("mm!()");
    }
    sql.push(';');

    let config = ParserConfig::default()
        .with_collect_tokens(true)
        .with_collect_node_extents(true)
        .with_macro_fallback(true);
    let mut parser = Parser::with_config(&config);
    parser.set_macro_lookup(Some(Box::new(ConstMacro)));
    let mut session = parser.parse(&sql);

    let ParseOutcome::Ok(stmt) = session.next() else {
        panic!("expected successful parse with 260 macro calls")
    };
    let erased = stmt.erase();

    // Walk the AST collecting all Span fields, then check span_expanded_text.
    // Each mm!() expansion produces a span whose expanded text is "42".
    // If _layer_id wraps at 256, span_expanded_text reads from the source
    // buffer instead, returning garbage like "SE".
    let mut spans = Vec::new();
    collect_all_spans(&erased, erased.root_id(), &mut spans);

    // Every span_expanded_text must be a valid SQL fragment: either a
    // source token ("SELECT", ",") or the macro body ("42"). If _layer_id
    // wraps, we get source bytes at wrong offsets.
    let mut bad = Vec::new();
    for (i, &span) in spans.iter().enumerate() {
        let expanded = erased.span_expanded_text(span);
        // Source-layer tokens and correct macro expansions produce
        // recognizable text. The wrapped case returns "SE" (source[0..2])
        // for spans that should contain "42".
        if expanded == "42" || expanded == "SELECT" || expanded == "," {
            continue;
        }
        bad.push((i, expanded.to_string()));
    }
    assert!(
        bad.is_empty(),
        "span_expanded_text returned unexpected text for {} spans \
         (_layer_id likely wrapped at 256): {:?}",
        bad.len(),
        &bad[..5.min(bad.len())]
    );
}
