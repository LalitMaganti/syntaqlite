// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#[cfg(feature = "sqlite")]
use super::{
    AnyParsedStatement, Comment, IncrementalParseSession, ParseErrorKind, ParseOutcome,
    ParserConfig, ParserTokenFlags, TypedParseError, TypedParseSession, TypedParsedStatement,
    TypedParser, TypedParserToken,
};

/// High-level entry point for parsing `SQLite` SQL into typed AST statements.
///
/// Use this in most applications.
///
/// - Hides grammar setup and returns SQLite SQL-native result types.
/// - Reusable across many SQL inputs.
/// - Supports batch/script parsing via [`parse`](Self::parse).
/// - Supports editor-style token feeds via [`incremental_parse`](Self::incremental_parse).
///
/// Advanced generic APIs exist in [`crate::typed`] and [`crate::any`].
#[cfg(feature = "sqlite")]
#[doc(hidden)]
pub struct Parser(pub(super) TypedParser<crate::sqlite::grammar::Grammar>);

#[cfg(feature = "sqlite")]
impl Parser {
    /// Create a parser for the `SQLite` grammar with default configuration.
    pub fn new() -> Self {
        Parser(TypedParser::new(crate::sqlite::grammar::grammar()))
    }

    /// Create a parser for the `SQLite` grammar with custom configuration.
    pub fn with_config(config: &ParserConfig) -> Self {
        Parser(TypedParser::with_config(
            crate::sqlite::grammar::grammar(),
            config,
        ))
    }

    /// Register a template macro with the parser.
    ///
    /// The macro body uses `$param` placeholders (e.g. `"$x + 1"`).
    /// All strings are copied; the caller may free them after this call returns.
    pub fn register_macro(&mut self, name: &str, params: &[&str], body: &str) {
        self.0.register_macro(name, params, body);
    }

    /// Deregister a macro by name. Returns `true` if it was found and removed.
    pub fn deregister_macro(&mut self, name: &str) -> bool {
        self.0.deregister_macro(name)
    }

    /// Parse a SQL script and return a statement-by-statement session.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use syntaqlite_syntax::{ParseErrorKind, Parser};
    ///
    /// let parser = Parser::new();
    /// let mut session = parser.parse("SELECT 1; SELECT FROM;");
    /// let mut ok_count = 0;
    ///
    /// loop {
    ///     match session.next() {
    ///         syntaqlite_syntax::ParseOutcome::Ok(stmt) => {
    ///             ok_count += 1;
    ///             let _ = stmt.root();
    ///         }
    ///         syntaqlite_syntax::ParseOutcome::Err(err) => {
    ///             assert!(!err.message().is_empty());
    ///             if err.kind() == ParseErrorKind::Fatal {
    ///                 break;
    ///             }
    ///         }
    ///         syntaqlite_syntax::ParseOutcome::Done => break,
    ///     }
    /// }
    ///
    /// assert!(ok_count >= 1);
    /// ```
    ///
    /// # Panics
    ///
    /// Panics if another session from this parser is still active.
    /// Drop the previous session before starting a new one.
    pub fn parse(&self, source: &str) -> ParseSession {
        ParseSession(self.0.parse(source))
    }

    /// Start an incremental parse session for token-by-token input.
    ///
    /// This mode is intended for IDEs, completion engines, and other workflows
    /// where SQL is consumed progressively.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use syntaqlite_syntax::{Parser, TokenType};
    ///
    /// let parser = Parser::new();
    /// let mut session = parser.incremental_parse("SELECT 1");
    ///
    /// assert!(session.feed_token(TokenType::Select, 0..6).is_none());
    /// assert!(session.feed_token(TokenType::Integer, 7..8).is_none());
    ///
    /// let stmt = session.finish().and_then(Result::ok).unwrap();
    /// let _ = stmt.root();
    /// ```
    ///
    /// # Panics
    ///
    /// Panics if another session from this parser is still active.
    /// Drop the previous session before starting a new one.
    pub fn incremental_parse(&self, source: &str) -> IncrementalParseSession {
        self.0.incremental_parse(source).into()
    }
}

#[cfg(feature = "sqlite")]
impl Default for Parser {
    fn default() -> Self {
        Self::new()
    }
}

/// Cursor over statements parsed from one SQL source string.
///
/// Useful for SQL scripts containing multiple statements.
///
/// - Returns one statement at a time via [`next`](Self::next).
/// - Reports errors per statement instead of failing the whole script immediately.
/// - Can continue after recoverable errors.
#[cfg(feature = "sqlite")]
#[doc(hidden)]
pub struct ParseSession(pub(super) TypedParseSession<crate::sqlite::grammar::Grammar>);

#[cfg(feature = "sqlite")]
impl ParseSession {
    /// Parse and return the next statement as a tri-state outcome.
    ///
    /// Mirrors C parser return codes directly:
    /// - [`ParseOutcome::Done`]  -> `SYNTAQLITE_PARSE_DONE`
    /// - [`ParseOutcome::Ok`]    -> `SYNTAQLITE_PARSE_OK`
    /// - [`ParseOutcome::Err`]   -> `SYNTAQLITE_PARSE_ERROR`
    #[expect(clippy::should_implement_trait)]
    pub fn next(&mut self) -> ParseOutcome<ParsedStatement<'_>, ParseError<'_>> {
        self.0.next().map(ParsedStatement).map_err(ParseError)
    }

    /// Original SQL source bound to this session.
    pub fn source(&self) -> &str {
        self.0.source()
    }

    /// Return a grammar-agnostic view over the current parse arena state.
    ///
    /// Useful for generic introspection after consuming the session.
    ///
    /// # Examples
    ///
    /// ```rust
    /// let parser = syntaqlite_syntax::Parser::new();
    /// let mut session = parser.parse("SELECT 1;");
    /// let stmt = match session.next().transpose() {
    ///     Ok(Some(stmt)) => stmt,
    ///     Ok(None) => panic!("expected statement"),
    ///     Err(err) => panic!("unexpected parse error: {err}"),
    /// };
    /// let _ = stmt.root();
    ///
    /// let any = session.arena_result();
    /// assert!(!any.root_id().is_null());
    /// ```
    pub fn arena_result(&self) -> AnyParsedStatement<'_> {
        self.0.arena_result()
    }
}

/// One parser-observed token from a parsed statement.
///
/// Returned by [`ParsedStatement::tokens`]. This is useful when building
/// token-aware tooling such as:
///
/// - Semantic syntax highlighting.
/// - Identifier/function/type classification.
/// - Statement-level token diagnostics.
///
/// Requires `collect_tokens: true` in [`ParserConfig`].
///
/// # Examples
///
/// ```rust
/// use syntaqlite_syntax::{Parser, ParserConfig, TokenType};
///
/// let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
/// let mut session = parser.parse("SELECT max(x) FROM t;");
/// let stmt = session.next().transpose().unwrap().unwrap();
///
/// let tokens: Vec<_> = stmt.tokens().collect();
/// assert!(!tokens.is_empty());
/// assert!(tokens.iter().any(|t| t.token_type() == TokenType::Select));
///
/// // Flags expose parser-inferred role information (identifier/function/type).
/// let _has_semantic_role = tokens.iter().any(|t| {
///     let f = t.flags();
///     f.used_as_identifier() || f.used_as_function() || f.used_as_type()
/// });
/// ```
#[cfg(feature = "sqlite")]
pub struct ParserToken<'a>(pub(super) TypedParserToken<'a, crate::sqlite::grammar::Grammar>);

#[cfg(feature = "sqlite")]
impl<'a> ParserToken<'a> {
    /// Exact source text for this token.
    ///
    /// Preserves original casing and quoting from input SQL.
    pub fn text(&self) -> &'a str {
        self.0.text()
    }

    /// Token kind from the `SQLite` SQL grammar.
    ///
    /// This is the lexical class (keyword, identifier, operator, etc.).
    pub fn token_type(&self) -> crate::sqlite::tokens::TokenType {
        self.0.token_type()
    }

    /// Semantic usage flags inferred by the parser.
    ///
    /// Use this to distinguish contextual role, for example:
    ///
    /// - Keyword text used as an identifier.
    /// - Function-call names.
    /// - Type names.
    pub fn flags(&self) -> ParserTokenFlags {
        self.0.flags()
    }

    /// Byte offset of the token start within the statement source.
    pub fn offset(&self) -> u32 {
        self.0.offset()
    }

    /// Byte length of the token text.
    pub fn length(&self) -> u32 {
        self.0.length()
    }
}

#[cfg(feature = "sqlite")]
impl std::fmt::Debug for ParserToken<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("ParserToken")
            .field("text", &self.0.text())
            .field("token_type", &self.0.token_type())
            .field("flags", &self.0.flags())
            .finish()
    }
}

/// Parse result for one successfully recognized `SQLite` statement.
///
/// Contains statement-local data:
///
/// - Typed AST root (`root()`).
/// - Optional token stream (`tokens()`).
/// - Optional comments (`comments()`).
/// - Original source slice (`source()`).
#[cfg(feature = "sqlite")]
#[doc(hidden)]
pub struct ParsedStatement<'a>(
    pub(super) TypedParsedStatement<'a, crate::sqlite::grammar::Grammar>,
);

#[cfg(feature = "sqlite")]
impl<'a> ParsedStatement<'a> {
    /// Typed AST root for the statement.
    ///
    /// Returns `None` for comment-only input (valid SQL with no actual
    /// statement, e.g. `/* no-op */`).
    ///
    /// Mirrors C `syntaqlite_result_root` for `PARSE_OK`.
    pub fn root(&'a self) -> Option<crate::sqlite::ast::Stmt<'a>> {
        self.0.root()
    }

    /// The source text bound to this result.
    pub fn source(&self) -> &'a str {
        self.0.source()
    }

    /// Statement-local token stream with parser usage flags.
    ///
    /// Requires `collect_tokens: true` in [`ParserConfig`].
    pub fn tokens(&self) -> impl Iterator<Item = ParserToken<'a>> {
        self.0.tokens().map(ParserToken)
    }

    /// Comments that belong to this statement.
    ///
    /// Requires `collect_tokens: true` in [`ParserConfig`].
    pub fn comments(&self) -> impl Iterator<Item = Comment<'a>> {
        self.0.comments()
    }

    /// Convert this result into the grammar-agnostic [`AnyParsedStatement`].
    ///
    /// Use this when handing statement data to grammar-independent tooling.
    pub fn erase(&self) -> AnyParsedStatement<'a> {
        self.0.clone().erase()
    }

    /// Macro expansion call-site spans recorded during parsing.
    pub fn macro_regions(&self) -> impl Iterator<Item = super::MacroRegion> + use<'_, 'a> {
        self.0.macro_regions()
    }

    /// Dump the AST as indented text into `out`.
    pub fn dump(&self, out: &mut String, indent: usize) {
        self.0.dump(out, indent);
    }
}

/// Parse error for one `SQLite` statement.
///
/// Includes diagnostics you can show directly to users:
///
/// - Error class (`kind()`: recovered vs fatal).
/// - Error message (`message()`).
/// - Optional location (`offset()` / `length()`).
/// - Optional partial recovery tree (`recovery_root()`).
#[cfg(feature = "sqlite")]
#[doc(hidden)]
pub struct ParseError<'a>(pub(super) TypedParseError<'a, crate::sqlite::grammar::Grammar>);

#[cfg(feature = "sqlite")]
impl<'a> ParseError<'a> {
    /// Whether parsing recovered (`Recovered`) or fully failed (`Fatal`).
    pub fn kind(&self) -> ParseErrorKind {
        self.0.kind()
    }

    /// True if this error was recovered and yielded a partial tree.
    pub fn is_recovered(&self) -> bool {
        self.0.is_recovered()
    }

    /// True if this error is fatal (unrecoverable).
    pub fn is_fatal(&self) -> bool {
        self.0.is_fatal()
    }

    /// Human-readable diagnostic text.
    pub fn message(&self) -> &str {
        self.0.message()
    }

    /// Byte offset in the original source, if known.
    pub fn offset(&self) -> Option<usize> {
        self.0.offset()
    }

    /// Byte length of the offending range, if known.
    pub fn length(&self) -> Option<usize> {
        self.0.length()
    }

    /// Partial AST recovered from invalid input, if available.
    ///
    /// Mirrors C `syntaqlite_result_recovery_root` for `PARSE_ERROR`.
    pub fn recovery_root(&'a self) -> Option<crate::sqlite::ast::Stmt<'a>> {
        self.0.recovery_root()
    }

    /// The source text bound to this result.
    pub fn parse_source(&self) -> &'a str {
        self.0.0.source()
    }

    /// Tokens collected during the (partial) parse, if `collect_tokens` was enabled.
    pub fn tokens(&self) -> impl Iterator<Item = ParserToken<'a>> {
        self.0.tokens().map(ParserToken)
    }

    /// Comments collected during the (partial) parse, if `collect_tokens` was enabled.
    pub fn comments(&self) -> impl Iterator<Item = Comment<'a>> {
        self.0.comments()
    }
}

#[cfg(feature = "sqlite")]
impl std::fmt::Debug for ParseError<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        self.0.fmt(f)
    }
}

#[cfg(feature = "sqlite")]
impl std::fmt::Display for ParseError<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        self.0.fmt(f)
    }
}

#[cfg(feature = "sqlite")]
impl std::error::Error for ParseError<'_> {}

#[cfg(all(test, feature = "sqlite"))]
mod tests {
    use std::panic::{self, AssertUnwindSafe};

    use super::{ParseErrorKind, ParseOutcome, Parser, ParserConfig};
    use crate::{CommentKind, TokenType};

    #[test]
    fn parser_continues_after_statement_error() {
        let parser = Parser::new();
        let mut session = parser.parse("SELECT 1; SELECT ; SELECT 2;");

        let first = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("first statement missing"),
            ParseOutcome::Err(err) => panic!("first statement should parse: {err}"),
        };
        let _ = first.root();

        let error = match session.next() {
            ParseOutcome::Err(err) => err,
            ParseOutcome::Done => panic!("second statement missing"),
            ParseOutcome::Ok(_) => panic!("second statement should fail"),
        };
        assert!(!error.message().is_empty());
        assert_ne!(error.is_fatal(), error.is_recovered());
        assert!(matches!(
            error.kind(),
            ParseErrorKind::Recovered | ParseErrorKind::Fatal
        ));

        let third = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("third statement missing"),
            ParseOutcome::Err(err) => panic!("third statement should parse: {err}"),
        };
        let _ = third.root();
        assert!(matches!(session.next(), ParseOutcome::Done));
    }

    #[test]
    fn parser_collect_tokens_and_comments() {
        let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
        let mut session = parser.parse("/* lead */ SELECT 1 -- tail\n;");

        let statement = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("statement is missing"),
            ParseOutcome::Err(err) => panic!("statement should parse: {err}"),
        };

        let token_types: Vec<_> = statement.tokens().map(|token| token.token_type()).collect();
        assert!(token_types.contains(&TokenType::Select));
        assert!(token_types.contains(&TokenType::Integer));

        let comments: Vec<_> = statement.comments().collect();
        assert!(
            comments
                .iter()
                .any(|comment| comment.kind() == CommentKind::Block
                    && comment.text().contains("lead"))
        );
        assert!(
            comments
                .iter()
                .any(|comment| comment.kind() == CommentKind::Line
                    && comment.text().contains("tail"))
        );
    }

    #[test]
    fn parser_collect_tokens_includes_semi() {
        let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
        let mut session = parser.parse("SELECT 1;");

        let statement = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("statement is missing"),
            ParseOutcome::Err(err) => panic!("statement should parse: {err}"),
        };

        let token_types: Vec<_> = statement.tokens().map(|t| t.token_type()).collect();
        assert!(
            token_types.contains(&TokenType::Semi),
            "Semi token should be in collected tokens, got: {token_types:?}"
        );
    }

    #[test]
    fn parser_allows_only_one_live_session() {
        let parser = Parser::new();
        let session = parser.parse("SELECT 1;");

        let reentrant_attempt = panic::catch_unwind(AssertUnwindSafe(|| {
            let _session = parser.parse("SELECT 2;");
        }));
        assert!(reentrant_attempt.is_err());

        drop(session);

        let mut second = parser.parse("SELECT 2;");
        let result = match second.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("statement is missing"),
            ParseOutcome::Err(err) => panic!("statement should parse: {err}"),
        };
        let _ = result.root();
    }

    #[test]
    fn parser_next_exposes_done_ok_err_states() {
        let parser = Parser::new();
        let mut ok_session = parser.parse("SELECT 1;");
        match ok_session.next() {
            ParseOutcome::Ok(stmt) => {
                let _ = stmt.root();
            }
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        }
        assert!(matches!(ok_session.next(), ParseOutcome::Done));
        drop(ok_session);

        let mut err_session = parser.parse("abc");
        match err_session.next() {
            ParseOutcome::Err(err) => assert!(err.is_fatal()),
            ParseOutcome::Done => panic!("expected fatal error"),
            ParseOutcome::Ok(_) => panic!("expected parse error"),
        }
    }

    #[test]
    fn parser_next_transposes_parse_outcome() {
        let parser = Parser::new();
        let mut ok_session = parser.parse("SELECT 1; SELECT 2;");
        let first = ok_session
            .next()
            .transpose()
            .expect("first should not error");
        let first = first.expect("first statement should exist");
        let _ = first.root();
        let second = ok_session
            .next()
            .transpose()
            .expect("second should not error");
        let second = second.expect("second statement should exist");
        let _ = second.root();
        assert!(
            ok_session
                .next()
                .transpose()
                .expect("done should not error")
                .is_none()
        );
        drop(ok_session);

        let mut err_session = parser.parse("abc");
        match err_session.next().transpose() {
            Err(err) => assert!(err.is_fatal()),
            Ok(_) => panic!("fatal error expected"),
        }
    }

    #[test]
    fn macro_expansion_simple_template() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        parser.register_macro("double", &["x"], "($x + $x)");

        let mut session = parser.parse("SELECT double!(1);");
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        };

        // The macro call `double!(1)` should expand to `(1 + 1)`.
        let mut dump = String::new();
        stmt.dump(&mut dump, 0);
        assert!(
            dump.contains("PLUS"),
            "expanded AST should contain a PLUS op, got:\n{dump}"
        );
    }

    #[test]
    fn macro_expansion_records_macro_region() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        parser.register_macro("id", &["x"], "$x");

        let source = "SELECT id!(42);";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        };

        let regions: Vec<_> = stmt.macro_regions().collect();
        assert_eq!(regions.len(), 1, "expected exactly one macro region");
        let r = &regions[0];
        let call_text = &source[r.call_offset as usize..(r.call_offset + r.call_length) as usize];
        assert_eq!(call_text, "id!(42)");
    }

    #[test]
    fn macro_expansion_multi_param() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        parser.register_macro("sum2", &["a", "b"], "($a + $b)");

        let mut session = parser.parse("SELECT sum2!(1, 2);");
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        };

        let mut dump = String::new();
        stmt.dump(&mut dump, 0);
        assert!(
            dump.contains("PLUS"),
            "expanded AST should contain PLUS, got:\n{dump}"
        );
    }

    #[test]
    fn macro_deregister_falls_back_to_legacy() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        parser.register_macro("foo", &["x"], "$x");
        assert!(parser.deregister_macro("foo"));

        // After deregistering, the macro call should not expand.
        // Legacy behavior: `foo` is parsed as a plain identifier.
        let mut session = parser.parse("SELECT foo!(1);");
        let _outcome = session.next();
        // We don't assert specific behavior here — just that it doesn't crash.
    }

    #[test]
    fn macro_deregister_nonexistent_returns_false() {
        let mut parser = Parser::new();
        assert!(!parser.deregister_macro("nonexistent"));
    }
}
