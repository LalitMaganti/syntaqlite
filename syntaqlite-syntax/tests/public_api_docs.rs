// Copyright 2026 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#![cfg(feature = "sqlite")]
#![allow(missing_docs)]

use syntaqlite_syntax::{
    CommentKind, ParseErrorKind, ParseOutcome, Parser, ParserConfig, TokenType, Tokenizer,
};

#[test]
fn tokenizing_example_breaks_source_into_tokens() {
    let tokenizer = Tokenizer::new();
    let tokens: Vec<_> = tokenizer
        .tokenize("SELECT 1")
        .map(|token| (token.token_type(), token.text().to_owned()))
        .collect();

    assert_eq!(
        tokens,
        vec![
            (TokenType::Select, "SELECT".to_owned()),
            (TokenType::Space, " ".to_owned()),
            (TokenType::Integer, "1".to_owned()),
        ]
    );
}

#[test]
fn parsing_example_loop_yields_successful_statement() {
    let parser = Parser::new();
    let mut session = parser.parse("SELECT 1");

    let mut ok_count = 0usize;
    loop {
        match session.next() {
            ParseOutcome::Ok(statement) => {
                ok_count += 1;
                let _ = statement.root();
            }
            ParseOutcome::Err(error) => {
                assert_ne!(error.message(), "");
                if error.kind() == ParseErrorKind::Fatal {
                    break;
                }
            }
            ParseOutcome::Done => break,
        }
    }

    assert_eq!(ok_count, 1);
}

#[test]
fn parser_can_be_reused_across_multiple_inputs() {
    let parser = Parser::new();

    let mut first = parser.parse("SELECT 1");
    let first_stmt = first
        .next()
        .transpose()
        .expect("first parse should succeed");
    let first_stmt = first_stmt.expect("first parse should produce a statement");
    let _ = first_stmt.root();
    assert!(matches!(first.next(), ParseOutcome::Done));
    drop(first);

    let mut second = parser.parse("SELECT 2");
    let second_stmt = second
        .next()
        .transpose()
        .expect("second parse should succeed");
    let second_stmt = second_stmt.expect("second parse should produce a statement");
    let _ = second_stmt.root();
    assert!(matches!(second.next(), ParseOutcome::Done));
}

#[test]
fn parse_session_continues_after_statement_error() {
    let parser = Parser::new();
    let mut session = parser.parse("SELECT 1; SELECT ; SELECT 2;");

    let first = session.next().transpose().expect("first should not error");
    let first = first.expect("first statement should exist");
    let _ = first.root();

    let error = match session.next() {
        ParseOutcome::Err(error) => error,
        ParseOutcome::Done => panic!("second statement should exist"),
        ParseOutcome::Ok(_) => panic!("second statement should fail"),
    };
    assert_ne!(error.message(), "");
    assert!(matches!(
        error.kind(),
        ParseErrorKind::Recovered | ParseErrorKind::Fatal
    ));

    let third = session.next().transpose().expect("third should not error");
    let third = third.expect("session should continue to next statement");
    let _ = third.root();
    assert!(matches!(session.next(), ParseOutcome::Done));
}

#[test]
fn collect_tokens_and_comments_follows_parser_config() {
    let mut disabled = Parser::new().parse("/* lead */ SELECT 1 -- tail\n;");
    let disabled_stmt = disabled.next().transpose().expect("statement should parse");
    let disabled_stmt = disabled_stmt.expect("statement should exist");
    assert_eq!(disabled_stmt.tokens().count(), 0);
    assert_eq!(disabled_stmt.comments().count(), 0);

    let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
    let mut enabled = parser.parse("/* lead */ SELECT 1 -- tail\n;");
    let enabled_stmt = enabled.next().transpose().expect("statement should parse");
    let enabled_stmt = enabled_stmt.expect("statement should exist");

    let token_types: Vec<_> = enabled_stmt
        .tokens()
        .map(|token| token.token_type())
        .collect();
    assert!(token_types.contains(&TokenType::Select));
    assert!(token_types.contains(&TokenType::Integer));

    let comments: Vec<_> = enabled_stmt.comments().collect();
    assert!(
        comments
            .iter()
            .any(|comment| comment.kind() == CommentKind::Block && comment.text().contains("lead"))
    );
    assert!(
        comments
            .iter()
            .any(|comment| comment.kind() == CommentKind::Line && comment.text().contains("tail"))
    );
}
