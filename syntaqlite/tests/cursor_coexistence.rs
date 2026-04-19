// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Tests that parser + session can be stored in the same struct.
//!
//! This is the key invariant of the Rc<RefCell> checkout pattern: `parse()`
//! takes `&self`, so the parser can be moved into the same struct as the
//! session it produced.

use syntaqlite::parse::Tokenizer;
use syntaqlite::parse::{IncrementalParseSession, ParseSession};
use syntaqlite::source::DocOffset;
use syntaqlite::{ParseOutcome, Parser};

// ── Parser + ParseSession coexistence ────────────────────────────────────

#[test]
fn parser_and_session_coexist() {
    struct S {
        _parser: Parser,
        session: ParseSession,
    }

    let parser = Parser::new();
    let session = parser.parse("SELECT 1");
    let mut s = S {
        _parser: parser,
        session,
    };
    assert!(matches!(s.session.next(), ParseOutcome::Ok(_)));
}

#[test]
fn parser_reuse_after_session_drop() {
    let parser = Parser::new();
    {
        let mut s = parser.parse("SELECT 1");
        assert!(matches!(s.next(), ParseOutcome::Ok(_)));
    }
    {
        let mut s = parser.parse("SELECT 2");
        assert!(matches!(s.next(), ParseOutcome::Ok(_)));
    }
}

// ── Parser + IncrementalParseSession coexistence ─────────────────────────

#[test]
fn parser_and_incremental_session_coexist() {
    use syntaqlite::parse::TokenType;

    struct S {
        _parser: Parser,
        session: IncrementalParseSession,
    }

    let parser = Parser::new();
    let session = parser.incremental_parse("SELECT 1");
    let mut s = S {
        _parser: parser,
        session,
    };

    s.session.feed_token(
        TokenType::Select,
        DocOffset::from_raw(0)..DocOffset::from_raw(6),
    );
    s.session.feed_token(
        TokenType::Integer,
        DocOffset::from_raw(7)..DocOffset::from_raw(8),
    );
    assert!(s.session.finish().is_some());
}

#[test]
fn parser_incremental_reuse_after_session_drop() {
    use syntaqlite::parse::TokenType;

    let parser = Parser::new();
    {
        let mut s = parser.incremental_parse("SELECT 1");
        s.feed_token(
            TokenType::Select,
            DocOffset::from_raw(0)..DocOffset::from_raw(6),
        );
        s.feed_token(
            TokenType::Integer,
            DocOffset::from_raw(7)..DocOffset::from_raw(8),
        );
        assert!(s.finish().is_some());
    }
    {
        let mut s = parser.incremental_parse("SELECT 2");
        s.feed_token(
            TokenType::Select,
            DocOffset::from_raw(0)..DocOffset::from_raw(6),
        );
        s.feed_token(
            TokenType::Integer,
            DocOffset::from_raw(7)..DocOffset::from_raw(8),
        );
        assert!(s.finish().is_some());
    }
}

// ── Tokenizer reuse ───────────────────────────────────────────────────────

#[test]
fn tokenizer_reuse_sequential() {
    let tokenizer = Tokenizer::new();
    {
        let tokens: Vec<_> = tokenizer.tokenize("SELECT 1").collect();
        assert!(!tokens.is_empty());
    }
    {
        let tokens: Vec<_> = tokenizer.tokenize("SELECT 2").collect();
        assert!(!tokens.is_empty());
    }
}
