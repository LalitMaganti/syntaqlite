// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#[cfg(feature = "sqlite")]
use crate::source::{DocText, StatementBase, StmtLen, StmtOffset, StmtText, TokenIdx};

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
/// - Hides dialect setup and returns SQLite SQL-native result types.
/// - Reusable across many SQL inputs.
/// - Supports batch/script parsing via [`parse`](Self::parse).
/// - Supports editor-style token feeds via [`incremental_parse`](Self::incremental_parse).
///
/// Advanced generic APIs exist in [`crate::typed`] and [`crate::any`].
#[cfg(feature = "sqlite")]
#[doc(hidden)]
pub struct Parser(pub(super) TypedParser<crate::sqlite::dialect::Dialect>);

#[cfg(feature = "sqlite")]
impl Parser {
    /// Create a parser for the `SQLite` dialect with default configuration.
    pub fn new() -> Self {
        Parser(TypedParser::new(crate::sqlite::dialect::dialect()))
    }

    /// Create a parser for the `SQLite` dialect with custom configuration.
    pub fn with_config(config: &ParserConfig) -> Self {
        Parser(TypedParser::with_config(
            crate::sqlite::dialect::dialect(),
            config,
        ))
    }

    /// Install a macro lookup handler.
    ///
    /// When the parser encounters `name!(args)`, it calls the handler to
    /// resolve the macro. Pass `None` to disable macro expansion.
    pub fn set_macro_lookup(&mut self, handler: Option<Box<dyn super::MacroLookup>>) {
        self.0.set_macro_lookup(handler);
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
    /// use syntaqlite_syntax::source::DocOffset;
    /// use syntaqlite_syntax::{Parser, TokenType};
    ///
    /// let parser = Parser::new();
    /// let mut session = parser.incremental_parse("SELECT 1");
    ///
    /// assert!(session.feed_token(TokenType::Select, DocOffset::from_raw(0)..DocOffset::from_raw(6)).is_none());
    /// assert!(session.feed_token(TokenType::Integer, DocOffset::from_raw(7)..DocOffset::from_raw(8)).is_none());
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
pub struct ParseSession(pub(super) TypedParseSession<crate::sqlite::dialect::Dialect>);

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

    /// Full SQL source bound to this session.
    pub fn full_text(&self) -> &str {
        self.0.full_text()
    }

    /// Post-expansion source — the bound source with every currently-
    /// active macro call replaced by its expansion.  See
    /// [`AnyParsedStatement::expanded_text`](super::AnyParsedStatement::expanded_text)
    /// for lifetime semantics.
    pub fn expanded_text(&self) -> &str {
        self.0.expanded_text()
    }

    /// Return a dialect-agnostic view over the current parse arena state.
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
pub struct ParserToken<'a>(pub(super) TypedParserToken<'a, crate::sqlite::dialect::Dialect>);

#[cfg(feature = "sqlite")]
impl<'a> ParserToken<'a> {
    /// Exact source text for this token.
    ///
    /// Preserves original casing and quoting from input SQL.
    pub fn text(&self) -> &'a str {
        self.0.text()
    }

    /// Token kind from the `SQLite` SQL dialect.
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

    /// Statement-relative byte offset of the token start.
    pub fn offset(&self) -> StmtOffset {
        self.0.offset()
    }

    /// Byte length of the token text.
    pub fn length(&self) -> StmtLen {
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
#[derive(Debug)]
pub struct ParsedStatement<'a>(
    pub(super) TypedParsedStatement<'a, crate::sqlite::dialect::Dialect>,
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

    /// See [`AnyParsedStatement::text`](super::AnyParsedStatement::text).
    pub fn text(&self) -> &'a StmtText {
        self.0.text()
    }

    /// See [`AnyParsedStatement::full_text`](super::AnyParsedStatement::full_text).
    pub fn full_text(&self) -> &'a DocText {
        self.0.full_text()
    }

    /// See [`AnyParsedStatement::statement_base`](super::AnyParsedStatement::statement_base).
    pub fn statement_base(&self) -> StatementBase {
        self.0.statement_base()
    }

    /// Post-expansion source — the bound source with every currently-
    /// active macro call replaced by its expansion.  See
    /// [`AnyParsedStatement::expanded_text`](super::AnyParsedStatement::expanded_text)
    /// for lifetime semantics.
    pub fn expanded_text(&self) -> &'a str {
        self.0.expanded_text()
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

    /// Comments that appear immediately before token `token_idx`.
    ///
    /// Requires `collect_tokens: true` in [`ParserConfig`].
    pub fn leading_comments(&self, token_idx: TokenIdx) -> impl Iterator<Item = Comment<'a>> {
        self.0.leading_comments(token_idx)
    }

    /// Comments that appear on the same source line as token `token_idx`,
    /// after it.
    ///
    /// Requires `collect_tokens: true` in [`ParserConfig`].
    pub fn trailing_comments(&self, token_idx: TokenIdx) -> impl Iterator<Item = Comment<'a>> {
        self.0.trailing_comments(token_idx)
    }

    /// Convert this result into the dialect-agnostic [`AnyParsedStatement`].
    ///
    /// Use this when handing statement data to dialect-independent tooling.
    pub fn erase(&self) -> AnyParsedStatement<'a> {
        self.0.clone().erase()
    }

    /// Returns `true` if all tokens of AST node `id` live in layer 0.
    /// See [`AnyParsedStatement::node_is_macro_free`](
    /// super::AnyParsedStatement::node_is_macro_free).
    pub fn node_is_macro_free(&self, id: super::AnyNodeId) -> bool {
        self.0.any.node_is_macro_free(id)
    }

    /// Source text of AST node `id` as `(text, offset)`.  See
    /// [`AnyParsedStatement::node_text`](super::AnyParsedStatement::node_text).
    pub fn node_text(&self, id: super::AnyNodeId) -> Option<(&'a str, StmtOffset)> {
        self.0.any.node_text(id)
    }

    /// Post-expansion text of AST node `id`.  See
    /// [`AnyParsedStatement::node_expanded_text`](
    /// super::AnyParsedStatement::node_expanded_text).
    pub fn node_expanded_text(&self, id: super::AnyNodeId) -> Option<&'a str> {
        self.0.any.node_expanded_text(id)
    }

    /// Returns `true` if the statement contains no macro expansions.
    /// See [`AnyParsedStatement::is_macro_free`](
    /// super::AnyParsedStatement::is_macro_free).
    pub fn is_macro_free(&self) -> bool {
        self.0.any.is_macro_free()
    }

    /// Macro rewrites recorded during parsing.  See [`super::MacroRewrite`]
    /// for the shape of each entry.
    pub fn macro_rewrites(&self) -> impl Iterator<Item = super::MacroRewrite<'a>> + use<'_, 'a> {
        self.0.macro_rewrites()
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
pub struct ParseError<'a>(pub(super) TypedParseError<'a, crate::sqlite::dialect::Dialect>);

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

    /// Statement-relative byte offset of the error token, if known.
    pub fn offset(&self) -> Option<StmtOffset> {
        self.0.offset()
    }

    /// Byte length of the offending range, if known.
    pub fn length(&self) -> Option<StmtLen> {
        self.0.length()
    }

    /// Document-absolute offset of the failing statement's first byte.
    /// Combine with [`Self::offset`] / [`Self::length`] to produce a
    /// document-absolute range.
    pub fn statement_base(&self) -> StatementBase {
        self.0.statement_base()
    }

    /// Partial AST recovered from invalid input, if available.
    ///
    /// Mirrors C `syntaqlite_result_recovery_root` for `PARSE_ERROR`.
    pub fn recovery_root(&'a self) -> Option<crate::sqlite::ast::Stmt<'a>> {
        self.0.recovery_root()
    }

    /// Source slice for just the failing statement.
    pub fn text(&self) -> &'a StmtText {
        self.0.0.text()
    }

    /// Full SQL source bound to the parse session.
    pub fn full_text(&self) -> &'a DocText {
        self.0.0.full_text()
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
    use std::cell::RefCell;
    use std::collections::HashMap;
    use std::panic::{self, AssertUnwindSafe};
    use std::rc::Rc;

    use crate::source::{
        LayerLen, LayerOffset, LayerRange, RewriteIdx, StmtLen, StmtOffset, TokenIdx,
    };

    use super::{ParseErrorKind, ParseOutcome, Parser, ParserConfig, ParserToken};
    use crate::parser::{MacroArg, MacroLookup, MacroOutput};
    use crate::{CommentKind, TokenType};

    struct TestMacroRegistry {
        macros: HashMap<String, (Vec<String>, String)>,
    }

    impl TestMacroRegistry {
        fn new() -> Self {
            Self {
                macros: HashMap::new(),
            }
        }
        fn register(&mut self, name: &str, params: &[&str], body: &str) {
            self.macros.insert(
                name.to_ascii_lowercase(),
                (
                    params.iter().map(ToString::to_string).collect(),
                    body.to_string(),
                ),
            );
        }
        fn deregister(&mut self, name: &str) -> bool {
            self.macros.remove(&name.to_ascii_lowercase()).is_some()
        }
    }

    impl MacroLookup for TestMacroRegistry {
        fn lookup(&mut self, name: &str, _args: &[MacroArg<'_>], out: &mut MacroOutput) -> bool {
            let Some((params, body)) = self.macros.get(&name.to_ascii_lowercase()) else {
                return false;
            };
            out.expand_template(body, params)
        }
    }

    struct SharedRegistry(Rc<RefCell<TestMacroRegistry>>);
    impl MacroLookup for SharedRegistry {
        fn lookup(&mut self, name: &str, args: &[MacroArg<'_>], out: &mut MacroOutput) -> bool {
            self.0.borrow_mut().lookup(name, args, out)
        }
    }

    #[test]
    fn statement_text_isolates_current_statement() {
        let parser = Parser::new();
        let mut session = parser.parse("CREATE TABLE t(x);\n  SELECT * FROM t;");

        let ParseOutcome::Ok(first) = session.next() else {
            panic!("first statement should parse");
        };
        assert_eq!(first.text(), "CREATE TABLE t(x);");
        assert_eq!(first.statement_base().as_doc_offset().as_u32(), 0);
        assert_eq!(first.full_text(), "CREATE TABLE t(x);\n  SELECT * FROM t;");

        let ParseOutcome::Ok(second) = session.next() else {
            panic!("second statement should parse");
        };
        assert_eq!(second.text(), "SELECT * FROM t;");
        assert_eq!(second.statement_base().as_doc_offset().as_u32(), 21);
        assert_eq!(second.full_text(), "CREATE TABLE t(x);\n  SELECT * FROM t;");
    }

    #[test]
    fn statement_text_includes_leading_comments() {
        let parser = Parser::new();
        let mut session = parser.parse("/* hi */ SELECT 1;");
        let ParseOutcome::Ok(stmt) = session.next() else {
            panic!("should parse");
        };
        assert_eq!(stmt.text(), "/* hi */ SELECT 1;");
        assert_eq!(stmt.statement_base().as_doc_offset().as_u32(), 0);
    }

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

    macro_rules! ok_stmt {
        ($session:expr) => {
            match $session.next() {
                ParseOutcome::Ok(stmt) => stmt,
                ParseOutcome::Done => panic!("statement is missing"),
                ParseOutcome::Err(err) => panic!("statement should parse: {err}"),
            }
        };
    }

    #[test]
    fn comment_attaches_as_trailing_when_same_line() {
        let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
        let mut session = parser.parse("SELECT 1 -- inline\nFROM t;");
        let stmt = ok_stmt!(session);

        let tokens: Vec<ParserToken<'_>> = stmt.tokens().collect();
        let one_idx = tokens
            .iter()
            .position(|t| t.text() == "1")
            .map(|i| TokenIdx::from_raw(u32::try_from(i).unwrap()))
            .expect("`1` token");

        let trailing: Vec<_> = stmt.trailing_comments(one_idx).collect();
        assert_eq!(trailing.len(), 1, "comment should be trailing of `1`");
        assert!(trailing[0].text().contains("inline"));

        let leading_one: Vec<_> = stmt.leading_comments(one_idx).collect();
        assert!(leading_one.is_empty(), "no leading comments on `1`");
    }

    #[test]
    fn comment_attaches_as_leading_when_on_own_line() {
        let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
        let mut session = parser.parse("SELECT 1\n-- standalone\nFROM t;");
        let stmt = ok_stmt!(session);

        let tokens: Vec<ParserToken<'_>> = stmt.tokens().collect();
        let from_idx = tokens
            .iter()
            .position(|t| t.token_type() == TokenType::From)
            .map(|i| TokenIdx::from_raw(u32::try_from(i).unwrap()))
            .expect("FROM token");

        let leading: Vec<_> = stmt.leading_comments(from_idx).collect();
        assert_eq!(leading.len(), 1, "comment should be leading of FROM");
        assert!(leading[0].text().contains("standalone"));
    }

    #[test]
    fn header_comment_attaches_as_leading_to_first_token() {
        let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
        let mut session = parser.parse("-- header\nSELECT 1;");
        let stmt = ok_stmt!(session);

        let leading: Vec<_> = stmt.leading_comments(TokenIdx::default()).collect();
        assert_eq!(leading.len(), 1);
        assert!(leading[0].text().contains("header"));
    }

    #[test]
    fn block_comment_same_line_attaches_as_trailing() {
        let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
        let mut session = parser.parse("SELECT 1 /* inline */\nFROM t;");
        let stmt = ok_stmt!(session);

        let tokens: Vec<ParserToken<'_>> = stmt.tokens().collect();
        let one_idx = tokens
            .iter()
            .position(|t| t.text() == "1")
            .map(|i| TokenIdx::from_raw(u32::try_from(i).unwrap()))
            .expect("`1` token");

        let trailing: Vec<_> = stmt.trailing_comments(one_idx).collect();
        assert_eq!(trailing.len(), 1);
        assert_eq!(trailing[0].kind(), CommentKind::Block);
    }

    #[test]
    fn inter_statement_same_line_comment_attaches_to_prev_semi() {
        // `SELECT 1; -- trailing` — the comment is on the same source line
        // as stmt 1's `;`, so it belongs to stmt 1 (TRAILING the SEMI),
        // not to stmt 2.
        let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
        let mut session = parser.parse("SELECT 1; -- trailing\nSELECT 2;");

        let stmt1 = ok_stmt!(session);
        let tokens1: Vec<ParserToken<'_>> = stmt1.tokens().collect();
        let semi_idx = tokens1
            .iter()
            .rposition(|t| t.text() == ";")
            .map(|i| TokenIdx::from_raw(u32::try_from(i).unwrap()))
            .expect("`;` token in stmt 1");

        let trailing: Vec<_> = stmt1.trailing_comments(semi_idx).collect();
        assert_eq!(
            trailing.len(),
            1,
            "same-line comment should trail stmt 1's `;`"
        );
        assert!(trailing[0].text().contains("trailing"));
        drop(stmt1);

        let stmt2 = ok_stmt!(session);
        assert_eq!(
            stmt2.comments().count(),
            0,
            "stmt 2 should have no comments"
        );
    }

    #[test]
    fn inter_statement_comment_rolls_forward_as_leading() {
        let parser = Parser::with_config(&ParserConfig::default().with_collect_tokens(true));
        let mut session = parser.parse("SELECT 1;\n-- between\nSELECT 2;");

        let stmt1 = ok_stmt!(session);
        // Inter-statement comment belongs to stmt2, not stmt1.
        assert_eq!(stmt1.comments().count(), 0);
        drop(stmt1);

        let stmt2 = ok_stmt!(session);
        let leading_first: Vec<_> = stmt2.leading_comments(TokenIdx::default()).collect();
        assert_eq!(leading_first.len(), 1);
        assert!(leading_first[0].text().contains("between"));
    }

    #[test]
    fn parser_collect_node_extents_records_root_text() {
        // The trailing `;` is a statement separator and does not
        // contribute to the SELECT's recorded range.
        let source = "SELECT 1;";
        let parser = Parser::with_config(&ParserConfig::default().with_collect_node_extents(true));
        let mut session = parser.parse(source);

        let statement = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("statement is missing"),
            ParseOutcome::Err(err) => panic!("statement should parse: {err}"),
        };

        let root_id = statement.erase().root_id();
        let (text, offset) = statement
            .node_text(root_id)
            .expect("root node should have recorded text");
        assert_eq!(text, "SELECT 1");
        assert_eq!(offset, StmtOffset::default());
    }

    #[test]
    fn parser_collect_node_extents_passthrough_rules_cover_full_range() {
        // Multi-RHS passthrough rules (A = B in grammar actions) must
        // re-record the extent so node_text returns the full rule range,
        // not just the child's original range.  Regression test for #118.
        let parser = Parser::with_config(&ParserConfig::default().with_collect_node_extents(true));

        // CREATE TABLE t AS SELECT 1  —  the cmd rule does A = ARGS (passthrough)
        let source = "CREATE TABLE t AS SELECT 1;";
        let mut session = parser.parse(source);
        let ParseOutcome::Ok(statement) = session.next() else {
            panic!("expected Ok");
        };
        let root_id = statement.erase().root_id();
        let (text, _) = statement
            .node_text(root_id)
            .expect("root node should have recorded text");
        assert_eq!(text, "CREATE TABLE t AS SELECT 1");
        drop(session);

        // (1 + 2) — the expr rule does A = B (LP expr RP passthrough)
        let source = "SELECT (1 + 2);";
        let mut session = parser.parse(source);
        let ParseOutcome::Ok(statement) = session.next() else {
            panic!("expected Ok");
        };
        let root_id = statement.erase().root_id();
        let (text, _) = statement
            .node_text(root_id)
            .expect("root node should have recorded text");
        assert_eq!(text, "SELECT (1 + 2)");
        drop(session);

        // CREATE VIRTUAL TABLE t USING mod(a, b) — the cmd rule does
        // A = X (passthrough with LP vtabarglist RP consumed).  Regression
        // test for #144.
        let source = "CREATE VIRTUAL TABLE t USING mod(a, b);";
        let mut session = parser.parse(source);
        let ParseOutcome::Ok(statement) = session.next() else {
            panic!("expected Ok");
        };
        let root_id = statement.erase().root_id();
        let (text, _) = statement
            .node_text(root_id)
            .expect("root node should have recorded text");
        assert_eq!(text, "CREATE VIRTUAL TABLE t USING mod(a, b)");
    }

    #[test]
    fn parser_collect_node_extents_attributes_macro_tokens_to_call_site() {
        // The SELECT node crosses layers: `SELECT` comes from the root
        // source, `42` from `id`'s expansion buffer.  `node_text`
        // returns the authored slice (with the macro call written
        // verbatim).  `node_expanded_text` materializes the
        // post-expansion view by inlining the expansion in place of
        // the call site.
        let mut parser = Parser::with_config(
            &ParserConfig::default()
                .with_collect_node_extents(true)
                .with_macro_fallback(true),
        );
        let mut reg = TestMacroRegistry::new();
        reg.register("id", &["x"], "$x");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT id!(42);";
        let mut session = parser.parse(source);
        let statement = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("statement is missing"),
            ParseOutcome::Err(err) => panic!("statement should parse: {err}"),
        };

        let root_id = statement.erase().root_id();
        let (text, offset) = statement
            .node_text(root_id)
            .expect("root node should have recorded text");
        assert_eq!(text, "SELECT id!(42)");
        assert_eq!(offset, StmtOffset::default());

        assert_eq!(statement.node_expanded_text(root_id), Some("SELECT 42"));
    }

    #[test]
    fn parser_collect_node_extents_cross_layer_sentinel_not_absorbed_by_parent_reduce() {
        // Regression test for #120: when a macro call is nested inside a
        // compound expression (e.g. CASE), the cross-layer sentinel from the
        // inner reduce was absorbed by the parent reduce's merge loop (both
        // epsilon and cross-layer used length==0).  The parent then gets a
        // non-sentinel extent and the fast path returns un-expanded source.
        let mut parser = Parser::with_config(
            &ParserConfig::default()
                .with_collect_node_extents(true)
                .with_macro_fallback(true),
        );
        let mut reg = TestMacroRegistry::new();
        reg.register("cast_string", &["value"], "CAST($value AS TEXT)");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT CASE WHEN 1 THEN cast_string!(y) END;";
        let mut session = parser.parse(source);
        let statement = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("statement is missing"),
            ParseOutcome::Err(err) => panic!("statement should parse: {err}"),
        };

        let root_id = statement.erase().root_id();
        // The root SELECT node crosses layers (SELECT/CASE/WHEN/THEN/END in
        // layer 0, CAST(y AS TEXT) in layer 1).  node_expanded_text must
        // return the post-expansion view, NOT the raw authored source.
        let expanded = statement
            .node_expanded_text(root_id)
            .expect("cross-layer node should produce expanded text");
        assert!(
            expanded.contains("CAST(y AS TEXT)"),
            "expected expanded macro in output, got: {expanded:?}",
        );
        assert!(
            !expanded.contains("cast_string!(y)"),
            "expanded text should not contain raw macro call, got: {expanded:?}",
        );
    }

    #[test]
    fn parser_collect_node_extents_expanded_text_is_source_for_root_nodes() {
        // For nodes built entirely from root-layer tokens,
        // `node_expanded_text` and `node_text` both return slices of
        // the input source — `expanded_text` is just the same bytes
        // without the authored-offset.
        let source = "SELECT 1;";
        let parser = Parser::with_config(&ParserConfig::default().with_collect_node_extents(true));
        let mut session = parser.parse(source);

        let statement = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("statement is missing"),
            ParseOutcome::Err(err) => panic!("statement should parse: {err}"),
        };

        let root_id = statement.erase().root_id();
        assert_eq!(statement.node_expanded_text(root_id), Some("SELECT 1"));
    }

    #[test]
    fn parser_collect_node_extents_expanded_text_is_expansion_buffer_for_pure_macro_nodes() {
        // When a statement is produced entirely by a macro expansion,
        // the root node's tokens all live in a single expansion
        // layer.  `node_expanded_text` returns the expansion-buffer
        // bytes (`SELECT 1`), while `node_text` collapses to the
        // authored call site (`id!(SELECT 1)`).
        let mut parser = Parser::with_config(
            &ParserConfig::default()
                .with_collect_node_extents(true)
                .with_macro_fallback(true),
        );
        let mut reg = TestMacroRegistry::new();
        reg.register("id", &["x"], "$x");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "id!(SELECT 1);";
        let mut session = parser.parse(source);
        let statement = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("statement is missing"),
            ParseOutcome::Err(err) => panic!("statement should parse: {err}"),
        };

        let root_id = statement.erase().root_id();
        assert_eq!(statement.node_expanded_text(root_id), Some("SELECT 1"));
        let (authored, _) = statement
            .node_text(root_id)
            .expect("root node should have authored text");
        assert_eq!(authored, "id!(SELECT 1)");
    }

    #[test]
    fn parser_expanded_text_materializes_macro_calls() {
        // `session.expanded_text()` materializes the whole input with
        // every currently-active macro call replaced by its expansion,
        // mirroring `syntaqlite_parser_expanded_text` at the C level.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("id", &["x"], "$x");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT id!(42);";
        let mut session = parser.parse(source);
        // Drive the parser forward so the macro layer exists.
        match session.next() {
            ParseOutcome::Ok(_) => {}
            ParseOutcome::Done => panic!("statement is missing"),
            ParseOutcome::Err(err) => panic!("statement should parse: {err}"),
        }

        assert_eq!(session.full_text(), "SELECT id!(42);");
        assert_eq!(session.expanded_text(), "SELECT 42;");
    }

    #[test]
    fn parser_collect_node_extents_off_returns_none() {
        let parser = Parser::with_config(&ParserConfig::default());
        let mut session = parser.parse("SELECT 1;");

        let statement = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("statement is missing"),
            ParseOutcome::Err(err) => panic!("statement should parse: {err}"),
        };

        let root_id = statement.erase().root_id();
        assert!(statement.node_text(root_id).is_none());
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
        let mut reg = TestMacroRegistry::new();
        reg.register("double", &["x"], "($x + $x)");
        parser.set_macro_lookup(Some(Box::new(reg)));

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
    fn macro_rewrites_records_single_expansion() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("id", &["x"], "$x");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT id!(42);";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        };

        let rewrites: Vec<_> = stmt.macro_rewrites().collect();
        assert_eq!(rewrites.len(), 1, "expected exactly one macro rewrite");
        let r = &rewrites[0];
        assert_eq!(r.parent(), None, "top-level rewrite");
        assert_eq!(r.name(), "id");
        assert_eq!(r.expansion(), "42");
        // call_offset is relative to stmt.text() for source-parent rewrites.
        // call_offset is a LayerOffset (generic "byte offset in parent's
        // text"); at the top level the parent is the statement, so the
        // pair reinterprets as a StmtRange.
        let stmt_text = stmt.text();
        let call_range = crate::source::StmtRange::from_offset_len(
            StmtOffset::from_raw(r.call_offset().as_u32()),
            StmtLen::from_raw(r.call_length().as_u32()),
        );
        let call_text = &stmt_text[call_range];
        assert_eq!(call_text, "id!(42)");
    }

    #[test]
    fn macro_rewrites_records_nested_expansion_tree() {
        // `mwrap` body contains an `mpass!(...)` call, so expanding `mwrap`
        // produces a nested expansion layer.  The flat rewrite list carries
        // a `parent` pointer so callers (e.g. Perfetto's SqlSource::Rewriter)
        // can reconstruct the tree.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("mpass", &["x"], "$x");
        reg.register("mwrap", &["x"], "mpass!($x)");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT mwrap!(42);";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        };

        let rewrites: Vec<_> = stmt.macro_rewrites().collect();
        assert_eq!(rewrites.len(), 2, "mwrap + mpass");

        // mwrap is top-level; call_offset/call_length are into `source`.
        let outer = &rewrites[0];
        assert_eq!(outer.parent(), None);
        assert_eq!(outer.name(), "mwrap");
        let outer_range = LayerRange::from_offset_len(outer.call_offset(), outer.call_length());
        let outer_call = &source[outer_range.start.as_usize()..outer_range.end.as_usize()];
        assert_eq!(outer_call, "mwrap!(42)");
        assert_eq!(outer.expansion(), "mpass!(42)");

        // mpass is nested inside mwrap; call_offset/call_length are into
        // outer.expansion() — the nested-rewrite case where parent is a
        // LayerText, and the Index<LayerRange> impl slices directly.
        let inner = &rewrites[1];
        assert_eq!(inner.parent(), Some(RewriteIdx::from_raw(0)));
        assert_eq!(inner.name(), "mpass");
        let outer_exp = outer.expansion();
        let inner_range = LayerRange::from_offset_len(inner.call_offset(), inner.call_length());
        let inner_call = &outer_exp[inner_range];
        assert_eq!(inner_call, "mpass!(42)");
        assert_eq!(inner.expansion(), "42");
    }

    #[test]
    fn macro_rewrite_exposes_arg_segments_with_body_positions() {
        // For macros defined via expand_template, each $param substitution
        // should be exposed as a MacroArgSegment that records (a) where
        // the $param token sits in the authored body, (b) where the arg
        // text landed in the expansion buffer, and (c) the origin of the
        // arg text so callers can drill back to authored source.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("id", &["x"], "($x)");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT id!(42);";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        };

        let rewrites: Vec<_> = stmt.macro_rewrites().collect();
        assert_eq!(rewrites.len(), 1);
        let r = &rewrites[0];
        let segs: Vec<_> = r.arg_segments().collect();
        assert_eq!(segs.len(), 1, "$x is the only substitution in ($x)");
        let seg = &segs[0];
        // Authored body is "($x)" — $x sits at offset 1, length 2.
        assert_eq!(seg.body_offset(), LayerOffset::from_raw(1));
        assert_eq!(seg.body_length(), LayerLen::from_raw(2));
        // Expansion buffer is "(42)" — arg text sits at offset 1, length 2.
        assert_eq!(seg.expansion_offset(), LayerOffset::from_raw(1));
        assert_eq!(seg.expansion_length(), LayerLen::from_raw(2));
        // Origin is the authored source.
        assert_eq!(seg.origin_parent(), None);
        let off = seg.origin_offset().as_usize();
        let len = seg.origin_length().as_usize();
        let origin_text = &source[off..off + len];
        assert_eq!(origin_text, "42");
    }

    #[test]
    fn macro_rewrite_nested_body_call_offsets_via_substituted_arg() {
        // Repro of the traceback example from issue #145:
        //   CREATE PERFETTO MACRO n(x Expr) RETURNS Expr AS ($x);
        //   CREATE PERFETTO MACRO m(x Expr) RETURNS Expr AS ($x);
        //   SELECT m!(n!(nonexistent_col));
        //
        // The inner n!(...) call is tokenized from the substituted text
        // of m's $x, not from m's authored body — downstream Rewriter
        // consumers need the arg-internal sentinel (u32::MAX on both
        // body_call_offset and body_call_length) to signal "descend via
        // arg segments" instead of "rewrite in the parent's body".
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("n", &["x"], "($x)");
        reg.register("m", &["x"], "($x)");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT m!(n!(42));";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        };

        let rewrites: Vec<_> = stmt.macro_rewrites().collect();
        assert_eq!(rewrites.len(), 2, "m + n");

        let outer = &rewrites[0];
        assert_eq!(outer.name(), "m");
        assert_eq!(outer.parent(), None);
        // For top-level calls the parent is the authored source, so
        // body_call_offset/length fall through to call_offset/length.
        assert_eq!(outer.body_call_offset(), outer.call_offset());
        assert_eq!(outer.body_call_length(), outer.call_length());

        let inner = &rewrites[1];
        assert_eq!(inner.name(), "n");
        assert_eq!(inner.parent(), Some(RewriteIdx::from_raw(0)));
        // The nested n!(42) call lives entirely inside m's substituted
        // $x arg — call_offset/length are in the post-substitution
        // expansion buffer, but there's no clean position in m's
        // authored body "($x)".  The arg-internal sentinel signals this.
        assert_eq!(
            inner.body_call_offset(),
            crate::MACRO_BODY_CALL_ARG_INTERNAL
        );
        assert_eq!(inner.body_call_length(), LayerLen::from_raw(u32::MAX));
    }

    #[test]
    fn macro_rewrite_nested_body_call_offsets_fixed_portion() {
        // When a nested call appears verbatim in the parent macro's
        // authored body (not via $param substitution), body_call_offset
        // and body_call_length should locate the call in that authored
        // body, with length shifts compensated by surrounding $param
        // substitutions.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("leaf", &["v"], "$v");
        // Body: "$a + leaf!(7)" — `leaf!(7)` is at body offset 5 length 8.
        reg.register("wrap", &["a"], "$a + leaf!(7)");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT wrap!(99);";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        };

        let rewrites: Vec<_> = stmt.macro_rewrites().collect();
        assert_eq!(rewrites.len(), 2, "wrap + leaf");
        let inner = &rewrites[1];
        assert_eq!(inner.name(), "leaf");
        assert_eq!(inner.parent(), Some(RewriteIdx::from_raw(0)));
        // `leaf!(7)` sits at offset 5 in the authored body "$a + leaf!(7)".
        assert_eq!(inner.body_call_offset(), LayerOffset::from_raw(5));
        assert_eq!(inner.body_call_length(), LayerLen::from_raw(8));
    }

    #[test]
    fn macro_rewrite_nested_body_call_offsets_shorter_substitution() {
        // Symmetric to the fixed_portion test, but with $param
        // substitutions that are *shorter* than the $param tokens
        // themselves.  body_shift must carry a signed delta so segments
        // before the nested call shift body_call_offset forward, and
        // segments strictly inside it grow body_call_length.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("leaf", &["v"], "$v");
        // Authored body "$a + leaf!($b + 7)" — `leaf!($b + 7)` sits at
        // offset 5 with length 13.
        reg.register("wrap", &["a", "b"], "$a + leaf!($b + 7)");
        parser.set_macro_lookup(Some(Box::new(reg)));

        // Both args are 1 char, so each substitution shortens the body
        // by 1: "$a" (2) → "9" (1), "$b" (2) → "8" (1).
        let source = "SELECT wrap!(9, 8);";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        };

        let rewrites: Vec<_> = stmt.macro_rewrites().collect();
        assert_eq!(rewrites.len(), 2, "wrap + leaf");
        let inner = &rewrites[1];
        assert_eq!(inner.name(), "leaf");
        assert_eq!(inner.parent(), Some(RewriteIdx::from_raw(0)));
        assert_eq!(inner.body_call_offset(), LayerOffset::from_raw(5));
        assert_eq!(inner.body_call_length(), LayerLen::from_raw(13));
    }

    #[test]
    fn macro_rewrite_arg_segment_chain_resolves_to_source() {
        // Reproduces the chain from issue #145: for `m!(n!(nonexistent_col))`,
        // downstream consumers walk each rewrite's arg_segments to anchor
        // the innermost token back to its authored position in the source.
        //
        // The chain for `nonexistent_col`:
        //   * n's $x segment: origin = rewrite(0) i.e. m's expansion buffer
        //     at the position of "nonexistent_col" within "(n!(nonexistent_col))"
        //     (n's arg text was tokenized from m's expansion, not the source).
        //   * m's $x segment at that position: origin = source, pointing at
        //     "n!(nonexistent_col)" in the authored SQL.
        //
        // Recursing one more step via n's arg text inside m's arg text gets
        // back to the "nonexistent_col" position in the source.
        use crate::parser::ArgOrigin;
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("n", &["x"], "($x)");
        reg.register("m", &["x"], "($x)");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT m!(n!(nonexistent_col));";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        };

        let rewrites: Vec<_> = stmt.macro_rewrites().collect();
        assert_eq!(rewrites.len(), 2);

        // m's arg segment points into the authored source; origin_offset
        // is statement-relative when origin is Source.
        let stmt_text = stmt.text();
        let m_segs: Vec<_> = rewrites[0].arg_segments().collect();
        assert_eq!(m_segs.len(), 1);
        assert_eq!(m_segs[0].origin(), ArgOrigin::Source);
        // Arg-segment origin_offset is a LayerOffset; at Source origin
        // the "layer" is the statement, so reinterpret as StmtRange.
        let m_arg_range = crate::source::StmtRange::from_offset_len(
            StmtOffset::from_raw(m_segs[0].origin_offset().as_u32()),
            StmtLen::from_raw(m_segs[0].origin_length().as_u32()),
        );
        let m_arg_text = &stmt_text[m_arg_range];
        assert_eq!(m_arg_text, "n!(nonexistent_col)");

        // n's arg segment origin is m's expansion buffer — one link in
        // the chain.  The consumer resolves by finding the m segment
        // whose expansion range contains n's arg, then recursing.
        let n_segs: Vec<_> = rewrites[1].arg_segments().collect();
        assert_eq!(n_segs.len(), 1);
        assert_eq!(
            n_segs[0].origin(),
            ArgOrigin::Rewrite(RewriteIdx::from_raw(0))
        );
        let m_expansion = rewrites[0].expansion();
        let n_arg_range =
            LayerRange::from_offset_len(n_segs[0].origin_offset(), n_segs[0].origin_length());
        let n_arg_in_m_expansion = &m_expansion[n_arg_range];
        assert_eq!(n_arg_in_m_expansion, "nonexistent_col");

        // Walk the chain: n's arg inside m's expansion, intersected with
        // m's segment, lands on the position of "nonexistent_col" in
        // the source.
        let n_arg_off_in_m = n_segs[0].origin_offset();
        let m_seg = &m_segs[0];
        assert!(
            n_arg_off_in_m >= m_seg.expansion_offset()
                && n_arg_off_in_m + n_segs[0].origin_length()
                    <= m_seg.expansion_offset() + m_seg.expansion_length()
        );
        let delta = n_arg_off_in_m - m_seg.expansion_offset();
        let source_off = m_seg.origin_offset() + delta;
        let source_len = n_segs[0].origin_length();
        let off = source_off.as_usize();
        let len = source_len.as_usize();
        let authored = &source[off..off + len];
        assert_eq!(authored, "nonexistent_col");
    }

    #[test]
    fn macro_expansion_multi_param() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("sum2", &["a", "b"], "($a + $b)");
        parser.set_macro_lookup(Some(Box::new(reg)));

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
    fn macro_arg_trailing_comment_does_not_corrupt_nested_expansion() {
        // Regression: scan_macro_args captured trailing whitespace/comments in
        // arg text.  When substituted into a nested expansion, a `-- comment`
        // consumed the rest of the line in the expansion buffer, corrupting
        // sibling tokens (e.g. commas, closing parens).
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("mpass", &["x"], "$x");
        reg.register("mwrap", &["x"], "(mpass!($x), mpass!($x))");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let sql = "SELECT mwrap!(\n  foo  -- a stray inline comment\n);";
        let mut session = parser.parse(sql);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        };

        // If the comment was trimmed correctly, the mwrap expansion produces
        // "(mpass!(foo), mpass!(foo))" which further expands to "(foo, foo)".
        // That parses as a parenthesized expression list — no syntax error.
        let mut dump = String::new();
        stmt.dump(&mut dump, 0);
        assert!(
            !dump.contains("error"),
            "expansion should not produce errors, got:\n{dump}"
        );
    }

    #[test]
    fn macro_nested_expansion_basic() {
        // Verify nested expansion works at all.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("mpass", &["x"], "$x");
        reg.register("mwrap", &["x"], "mpass!($x)");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let sql = "SELECT mwrap!(42);";
        let mut session = parser.parse(sql);
        match session.next() {
            ParseOutcome::Ok(stmt) => {
                let mut dump = String::new();
                stmt.dump(&mut dump, 0);
                assert!(
                    stmt.root().is_some(),
                    "nested expansion should work, got:\n{dump}"
                );
            }
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        }
    }

    #[test]
    fn macro_arg_leading_whitespace_trimmed() {
        // Args like `macro!(  x  )` should have leading/trailing whitespace
        // trimmed so the substituted text is just `x`.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("mpass", &["x"], "$x");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let sql = "SELECT mpass!(  42  );";
        let mut session = parser.parse(sql);
        match session.next() {
            ParseOutcome::Ok(stmt) => {
                let mut dump = String::new();
                stmt.dump(&mut dump, 0);
                // Should parse as SELECT 42 — no errors.
                assert!(stmt.root().is_some());
            }
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        }
    }

    #[test]
    fn macro_deregister_falls_back_to_legacy() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let reg = Rc::new(RefCell::new(TestMacroRegistry::new()));
        reg.borrow_mut().register("foo", &["x"], "$x");
        parser.set_macro_lookup(Some(Box::new(SharedRegistry(Rc::clone(&reg)))));
        assert!(reg.borrow_mut().deregister("foo"));

        // After deregistering, the macro call should not expand.
        // Legacy behavior: `foo` is parsed as a plain identifier.
        let mut session = parser.parse("SELECT foo!(1);");
        let _outcome = session.next();
        // We don't assert specific behavior here — just that it doesn't crash.
    }

    #[test]
    fn macro_deregister_nonexistent_returns_false() {
        let reg = TestMacroRegistry::new();
        assert!(!RefCell::new(reg).borrow_mut().deregister("nonexistent"));
    }

    // ── span_text / span_expanded_text accessors ────────────────────────────

    use super::super::AnyParsedStatement;

    // Walk the tree depth-first looking for a non-empty Span field; return
    // the raw TextSpan of the first one found.
    fn first_span_in_tree<'a>(stmt: &'a AnyParsedStatement<'a>) -> Option<crate::ast::TextSpan> {
        use crate::ast::FieldValue;
        fn walk<'a>(
            stmt: &'a AnyParsedStatement<'a>,
            id: crate::ast::AnyNodeId,
        ) -> Option<crate::ast::TextSpan> {
            if let Some((_, fields)) = stmt.extract_fields(id) {
                for i in 0..fields.len() {
                    if matches!(fields[i], FieldValue::Span { .. })
                        && let Ok(field_idx) = u8::try_from(i)
                        && let Some(sp) = stmt.field_span(id, field_idx)
                        && !sp.is_empty()
                    {
                        return Some(sp);
                    }
                }
            }
            for child in stmt.child_node_ids(id) {
                if let Some(found) = walk(stmt, child) {
                    return Some(found);
                }
            }
            None
        }
        let root = stmt.root_id();
        if root.is_null() {
            return None;
        }
        walk(stmt, root)
    }

    #[test]
    fn span_text_macro_free_equals_authored_slice() {
        let parser = Parser::new();
        let source = "SELECT foo FROM bar";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let erased = stmt.erase();
        let span = first_span_in_tree(&erased).expect("expected a Span field");

        // Macro-free: span_text == span_expanded_text, and both are slices of
        // the original source.
        let (span_text, _) = erased.span_text(span);
        let span_expanded = erased.span_expanded_text(span);
        assert_eq!(span_text, span_expanded);
        assert!(source.contains(span_text));
    }

    #[test]
    fn span_text_inside_macro_body_collapses_to_call_site() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        // Body produces an identifier token ("inner") that lives entirely in
        // the template — no parameter substitution at the span location.
        let mut reg = TestMacroRegistry::new();
        reg.register("idmac", &[], "inner");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT idmac!()";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let erased = stmt.erase();
        // The first span in the tree should be "inner" from the expansion.
        let span = first_span_in_tree(&erased).expect("expected a Span field");

        // span_text: authored slice — the whole macro call "idmac!()".
        assert_eq!(
            erased.span_text(span).0,
            "idmac!()",
            "span_text should collapse to the call site"
        );
        // span_expanded_text: the literal text the tokenizer saw — "inner".
        assert_eq!(erased.span_expanded_text(span), "inner");
    }

    #[test]
    fn span_text_inside_substituted_arg_drills_to_origin() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        // Body is purely a $param substitution. The identifier token in the
        // expansion is an arg-copy of the caller's text.
        let mut reg = TestMacroRegistry::new();
        reg.register("idmac", &["x"], "$x");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT idmac!(authored)";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let erased = stmt.erase();
        let span = first_span_in_tree(&erased).expect("expected a Span field");

        // Arg-segment drill: span_text points at the user's authored arg
        // text, not at the whole call site.
        assert_eq!(
            erased.span_text(span).0,
            "authored",
            "span_text should drill through arg segment to origin"
        );
        // Expanded text (the token the tokenizer actually saw) is also
        // "authored" because the arg was copied verbatim.
        assert_eq!(erased.span_expanded_text(span), "authored");
    }

    // ── traceback with arg-segment drilling ─────────────────────────────────

    // Walk the tree depth-first looking for the first non-empty Span field;
    // return (owning node, field index) for use with `traceback`.
    fn first_span_field<'a>(
        stmt: &'a AnyParsedStatement<'a>,
    ) -> Option<(crate::ast::AnyNodeId, u8)> {
        use crate::ast::FieldValue;
        fn walk<'a>(
            stmt: &'a AnyParsedStatement<'a>,
            id: crate::ast::AnyNodeId,
        ) -> Option<(crate::ast::AnyNodeId, u8)> {
            if let Some((_, fields)) = stmt.extract_fields(id) {
                for i in 0..fields.len() {
                    if matches!(fields[i], FieldValue::Span { .. })
                        && let Ok(field_idx) = u8::try_from(i)
                        && let Some(sp) = stmt.field_span(id, field_idx)
                        && !sp.is_empty()
                    {
                        return Some((id, field_idx));
                    }
                }
            }
            for child in stmt.child_node_ids(id) {
                if let Some(found) = walk(stmt, child) {
                    return Some(found);
                }
            }
            None
        }
        let root = stmt.root_id();
        if root.is_null() {
            return None;
        }
        walk(stmt, root)
    }

    #[test]
    fn traceback_macro_free_yields_single_root_frame() {
        let parser = Parser::new();
        let source = "SELECT foo FROM bar";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let mut erased = stmt.erase();
        let (nid, fidx) = first_span_field(&erased).expect("span field");

        let frames: Vec<_> = erased.traceback(nid, fidx).collect();
        assert_eq!(frames.len(), 1, "macro-free span → single root frame");
        let f = &frames[0];
        assert_eq!(f.name, None, "root frame has no macro name");
        assert_eq!(f.snippet, source, "root snippet is the authored source");
        // Offset points somewhere inside the snippet.
        assert!(f.offset_in_snippet < f.snippet.as_range().end);
    }

    #[test]
    fn traceback_span_inside_macro_body_yields_two_frames() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        // Body has a hardcoded identifier; not an arg substitution.
        let mut reg = TestMacroRegistry::new();
        reg.register("idmac", &[], "inner");
        parser.set_macro_lookup(Some(Box::new(reg)));
        let source = "SELECT idmac!()";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let mut erased = stmt.erase();
        let (nid, fidx) = first_span_field(&erased).expect("span field");

        let frames: Vec<_> = erased.traceback(nid, fidx).collect();
        assert_eq!(
            frames.len(),
            2,
            "span inside macro body → root + macro frames"
        );
        // Outermost = root
        assert_eq!(frames[0].name, None);
        assert_eq!(frames[0].snippet, source);
        // Innermost = macro expansion
        assert_eq!(frames[1].name, Some("idmac"));
        assert_eq!(frames[1].snippet, "inner");
        assert_eq!(frames[1].offset_in_snippet, LayerOffset::default());
    }

    #[test]
    fn traceback_span_inside_substituted_arg_drills_to_origin() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("idmac", &["x"], "$x");
        parser.set_macro_lookup(Some(Box::new(reg)));
        let source = "SELECT idmac!(authored)";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let mut erased = stmt.erase();
        let (nid, fidx) = first_span_field(&erased).expect("span field");

        let frames: Vec<_> = erased.traceback(nid, fidx).collect();
        // Arg-segment drill collapses the macro frame: the innermost frame
        // is the user's authored arg text in the original source.
        assert_eq!(
            frames.len(),
            1,
            "span in substituted arg collapses to a single root frame"
        );
        assert_eq!(frames[0].name, None, "root frame after drill");
        assert_eq!(frames[0].snippet, source);
        let range = LayerRange::from_offset_len(
            frames[0].offset_in_snippet,
            LayerLen::from_raw(u32::try_from("authored".len()).unwrap()),
        );
        assert_eq!(&frames[0].snippet[range], "authored");
    }

    #[test]
    fn statement_is_macro_free_true_for_plain_sql() {
        let parser = Parser::new();
        let source = "SELECT foo FROM bar";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        assert!(
            stmt.is_macro_free(),
            "plain SQL statement should be macro-free"
        );
    }

    #[test]
    fn statement_is_macro_free_false_for_expansion() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("idmac", &[], "inner");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT idmac!()";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        assert!(
            !stmt.is_macro_free(),
            "statement with macro expansion should not be macro-free"
        );
    }

    #[test]
    fn span_is_macro_free_true_for_plain_sql() {
        let parser = Parser::new();
        let source = "SELECT foo FROM bar";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let erased = stmt.erase();
        let span = first_span_in_tree(&erased).expect("expected a Span field");
        assert!(span.is_macro_free(), "plain SQL span should be macro-free");
    }

    #[test]
    fn span_is_macro_free_false_for_expansion() {
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("idmac", &[], "inner");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT idmac!()";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let erased = stmt.erase();
        let span = first_span_in_tree(&erased).expect("expected a Span field");
        assert!(
            !span.is_macro_free(),
            "span from macro expansion should not be macro-free"
        );
    }

    #[test]
    fn node_is_macro_free_true_for_plain_sql() {
        let parser = Parser::with_config(&ParserConfig::default().with_collect_node_extents(true));
        let source = "SELECT foo FROM bar";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let erased = stmt.erase();
        assert!(
            erased.node_is_macro_free(erased.root_id()),
            "root node in plain SQL should be macro-free"
        );
    }

    #[test]
    fn node_is_macro_free_false_for_expansion() {
        let mut parser = Parser::with_config(
            &ParserConfig::default()
                .with_macro_fallback(true)
                .with_collect_node_extents(true),
        );
        let mut reg = TestMacroRegistry::new();
        reg.register("idmac", &["x"], "$x");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT idmac!(col1) FROM t";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let erased = stmt.erase();
        // Root spans multiple layers (SELECT from source, col1 from expansion).
        assert!(
            !erased.node_is_macro_free(erased.root_id()),
            "root node with macro expansion should not be macro-free"
        );
    }

    /// A macro registry that uses permissive expansion (unknown $params pass through).
    struct PermissiveMacroRegistry {
        macros: HashMap<String, (Vec<String>, String)>,
    }
    impl PermissiveMacroRegistry {
        fn new() -> Self {
            Self {
                macros: HashMap::new(),
            }
        }
        fn register(&mut self, name: &str, params: &[&str], body: &str) {
            self.macros.insert(
                name.to_ascii_lowercase(),
                (
                    params.iter().map(ToString::to_string).collect(),
                    body.to_string(),
                ),
            );
        }
    }
    impl MacroLookup for PermissiveMacroRegistry {
        fn lookup(&mut self, name: &str, _args: &[MacroArg<'_>], out: &mut MacroOutput) -> bool {
            let Some((params, body)) = self.macros.get(&name.to_ascii_lowercase()) else {
                return false;
            };
            out.expand_template_permissive(body, params)
        }
    }

    #[test]
    fn expand_template_strict_rejects_unknown_param() {
        // Default (strict) expand_template should fail when the body
        // references a $param not in the declared param list.
        // We verify by checking that the lookup returns false.
        struct StrictCheckRegistry {
            lookup_failed: bool,
        }
        impl MacroLookup for StrictCheckRegistry {
            fn lookup(
                &mut self,
                _name: &str,
                _args: &[MacroArg<'_>],
                out: &mut MacroOutput,
            ) -> bool {
                let params = vec!["x".to_string()];
                let ok = out.expand_template("SELECT $x, $unknown", &params);
                if !ok {
                    self.lookup_failed = true;
                }
                ok
            }
        }

        struct Wrapper(Rc<RefCell<StrictCheckRegistry>>);
        impl MacroLookup for Wrapper {
            fn lookup(&mut self, name: &str, args: &[MacroArg<'_>], out: &mut MacroOutput) -> bool {
                self.0.borrow_mut().lookup(name, args, out)
            }
        }

        let reg = Rc::new(RefCell::new(StrictCheckRegistry {
            lookup_failed: false,
        }));
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        parser.set_macro_lookup(Some(Box::new(Wrapper(Rc::clone(&reg)))));

        let mut session = parser.parse("SELECT foo!(1);");
        let _ = session.next();
        assert!(
            reg.borrow().lookup_failed,
            "strict expand_template should reject unknown $param"
        );
    }

    #[test]
    fn expand_template_permissive_passes_through_unknown_param() {
        // Permissive expansion should copy unknown $params verbatim.
        let mut parser = Parser::with_config(
            &ParserConfig::default()
                .with_macro_fallback(true)
                .with_collect_node_extents(true),
        );
        let mut reg = PermissiveMacroRegistry::new();
        // Body uses $x (declared) and $y (not declared — should pass through).
        // $y is in a value position so the parser accepts it as a variable.
        reg.register("foo", &["x"], "SELECT $x, $y");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let mut session = parser.parse("foo!(42);");
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(err) => panic!("unexpected error: {}", err.message()),
        };

        // The expanded text should contain `$y` verbatim and `42` substituted.
        let root_id = stmt.erase().root_id();
        let expanded = stmt
            .node_expanded_text(root_id)
            .expect("should have expanded text");
        assert!(
            expanded.contains("42"),
            "expected substituted arg in expansion, got: {expanded:?}"
        );
        assert!(
            expanded.contains("$y"),
            "expected unknown $y to pass through verbatim, got: {expanded:?}"
        );
    }

    #[test]
    fn three_deep_nested_no_arg_macros_no_spurious_straddle() {
        // Regression test for #125: check_macro_straddle fired a false
        // positive on 3-deep nested no-arg macros because it compared
        // span offsets across different expansion layers without
        // accounting for _layer_id.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("_dfs", &[], "(SELECT node_id AS id)");
        reg.register("_tree_reach", &[], "(SELECT node_id FROM _dfs!())");
        reg.register(
            "_for_stack",
            &[],
            "(SELECT f.id, f.parent_id, f.name FROM _tree_reach!())",
        );
        parser.set_macro_lookup(Some(Box::new(reg)));

        let mut session = parser.parse("SELECT * FROM _for_stack!();");
        match session.next() {
            ParseOutcome::Ok(_) => {} // expected
            ParseOutcome::Done => panic!("expected a statement"),
            ParseOutcome::Err(e) => panic!("should parse without straddle error: {}", e.message()),
        }
    }

    #[test]
    fn macro_expansion_with_ne_after_substituted_id_no_spurious_straddle() {
        // Regression: inside a macro body, a substituted identifier followed by
        // `!=` (or any `!<not-lp>`) triggered expand_and_feed's nested-macro
        // detection (it sees `ID` then `!`). The lookup callback returned
        // "not a macro" for that identifier, and the failure path in
        // synq_parser_expand_and_feed_macro called synq_end_macro while
        // ctx.layer_id was still the *outer* layer. synq_end_macro then walked
        // one parent too far and clobbered ctx.layer_id from the outer macro's
        // layer down to 0 (source), causing every subsequent token fed from the
        // outer expansion to be tagged with layer_id=0. The straddle detector
        // then correctly flagged the mixed-layer RHS as a spurious straddle.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("m", &["d"], "(SELECT * FROM t WHERE $d != 1)");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let mut session = parser.parse("SELECT * FROM m!(dur);");
        match session.next() {
            ParseOutcome::Ok(_) => {}
            ParseOutcome::Done => panic!("expected a statement"),
            ParseOutcome::Err(e) => panic!("should parse without straddle error: {}", e.message()),
        }
    }

    #[test]
    fn node_is_macro_free_false_without_extent_tracking() {
        let parser = Parser::new(); // no extent tracking
        let source = "SELECT 1";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let erased = stmt.erase();
        assert!(
            !erased.node_is_macro_free(erased.root_id()),
            "should return false when extent tracking is disabled"
        );
    }

    // ── Tests for the call-site arg / is_fallback / parent_buffer API ──

    #[test]
    fn fallback_call_is_fallback_with_args_and_call_text() {
        // Unregistered macro goes down the fallback path; the parser
        // keeps the whole `foo!(...)` as a TK_ID but still exposes the
        // top-level arg spans so consumers can reformat without
        // retokenizing.
        let parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let source = "SELECT foo!(a, 1 + 2, 'x');";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };

        let rewrites: Vec<_> = stmt.macro_rewrites().collect();
        assert_eq!(rewrites.len(), 1);
        let r = &rewrites[0];
        assert!(r.is_fallback(), "unregistered call should be fallback");
        assert_eq!(r.name(), "foo");
        assert_eq!(r.call_text(), "foo!(a, 1 + 2, 'x')");
        // No expansion buffer on a fallback.
        assert_eq!(r.expansion().as_str(), "");

        let arg_texts: Vec<&str> = r.args().map(|a| a.text()).collect();
        assert_eq!(arg_texts, vec!["a", "1 + 2", "'x'"]);
    }

    #[test]
    fn registered_call_args_come_from_call_site_not_expansion() {
        // `args()` returns call-site arg text for registered macros
        // too — so the same consumer code works uniformly and does
        // not have to branch on is_fallback().  `is_fallback()` is
        // false because an expansion was produced.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("id", &["x", "y"], "($x + $y)");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT id!(3, 4);";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };

        let rewrites: Vec<_> = stmt.macro_rewrites().collect();
        assert_eq!(rewrites.len(), 1);
        let r = &rewrites[0];
        assert!(!r.is_fallback());
        assert_eq!(r.call_text(), "id!(3, 4)");
        assert_eq!(r.expansion().as_str(), "(3 + 4)");

        let arg_texts: Vec<&str> = r.args().map(|a| a.text()).collect();
        assert_eq!(arg_texts, vec!["3", "4"]);
    }

    #[test]
    fn empty_call_has_no_args() {
        let parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let source = "SELECT foo!();";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let rewrites: Vec<_> = stmt.macro_rewrites().collect();
        assert_eq!(rewrites.len(), 1);
        assert!(rewrites[0].is_fallback());
        assert_eq!(rewrites[0].args().count(), 0);
        assert_eq!(rewrites[0].call_text(), "foo!()");
    }

    #[test]
    fn top_level_parent_buffer_matches_stmt_text() {
        // For top-level rewrites the parent buffer *is* the current
        // statement slice: same bytes, so any call or arg offset can
        // be resolved by indexing into either one.
        let parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let source = "SELECT foo!(a);";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let stmt_text = stmt.text();
        let rewrites: Vec<_> = stmt.macro_rewrites().collect();
        let r = &rewrites[0];
        assert_eq!(r.parent_buffer().as_str(), stmt_text.as_str());
    }

    #[test]
    fn arg_buffer_matches_enclosing_parent_buffer() {
        // `MacroCallArg::buffer()` must equal the enclosing rewrite's
        // `parent_buffer()`; the invariant is what lets callers treat
        // `a.text()` as equivalent to slicing `r.parent_buffer()` at
        // `a.offset()`/`a.length()`.
        let parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let source = "SELECT foo!(x, y);";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };
        let rewrites: Vec<_> = stmt.macro_rewrites().collect();
        let r = &rewrites[0];
        let pb = r.parent_buffer();
        for a in r.args() {
            assert!(
                std::ptr::eq(std::ptr::from_ref(a.buffer()), std::ptr::from_ref(pb)),
                "arg buffer must be the same reference as the rewrite's parent_buffer",
            );
            let end = LayerOffset::from_raw(a.offset().as_u32() + a.length().as_u32());
            let manual = &pb[LayerRange {
                start: a.offset(),
                end,
            }];
            assert_eq!(a.text(), manual);
        }
    }

    #[test]
    fn nested_call_parent_buffer_is_parent_expansion() {
        // For a nested rewrite, `parent_buffer` is the *parent
        // rewrite's* expansion buffer — exactly the thing nested
        // `call_offset` / `args()` are measured into.  This lets a
        // consumer resolve nested-call text without pulling the
        // parent chain apart manually.
        let mut parser = Parser::with_config(&ParserConfig::default().with_macro_fallback(true));
        let mut reg = TestMacroRegistry::new();
        reg.register("mpass", &["x"], "$x");
        reg.register("mwrap", &["x"], "mpass!($x)");
        parser.set_macro_lookup(Some(Box::new(reg)));

        let source = "SELECT mwrap!(42);";
        let mut session = parser.parse(source);
        let stmt = match session.next() {
            ParseOutcome::Ok(stmt) => stmt,
            ParseOutcome::Done => panic!("expected statement"),
            ParseOutcome::Err(e) => panic!("unexpected error: {}", e.message()),
        };

        let rewrites: Vec<_> = stmt.macro_rewrites().collect();
        assert_eq!(rewrites.len(), 2, "mwrap + mpass");
        let outer = &rewrites[0];
        let inner = &rewrites[1];
        assert_eq!(inner.parent(), Some(RewriteIdx::from_raw(0)));
        assert_eq!(
            inner.parent_buffer().as_str(),
            outer.expansion().as_str(),
            "nested rewrite's parent_buffer == parent's expansion"
        );
        assert_eq!(inner.call_text(), "mpass!(42)");
        let inner_arg_texts: Vec<&str> = inner.args().map(|a| a.text()).collect();
        assert_eq!(inner_arg_texts, vec!["42"]);
    }
}
