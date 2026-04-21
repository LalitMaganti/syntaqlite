// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Lemon grammar file parser
//!
//! **NOTE:** This is a minimal, relaxed parser designed specifically to extract
//! tokens and rules from `SQLite`'s parse.y grammar. It is NOT a general-purpose
//! Lemon parser and makes no attempt to validate grammar correctness. It simply
//! extracts the information we need while skipping everything else.

use std::fmt;

// ============================================================================
// Core Types
// ============================================================================

#[derive(Debug, Clone)]
pub(crate) struct LemonGrammar<'a> {
    pub(crate) tokens: Vec<TokenDecl<'a>>,
    pub(crate) rules: Vec<GrammarRule<'a>>,
    pub(crate) token_classes: Vec<TokenClass<'a>>,
    pub(crate) fallbacks: Vec<FallbackDecl<'a>>,
    pub(crate) precedences: Vec<PrecedenceDecl<'a>>,
    pub(crate) wildcard: Option<&'a str>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct PrecedenceDecl<'a> {
    pub(crate) assoc: &'a str, // "left", "right", or "nonassoc"
    pub(crate) tokens: Vec<&'a str>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct FallbackDecl<'a> {
    pub(crate) target: &'a str,
    pub(crate) tokens: Vec<&'a str>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct TokenDecl<'a> {
    pub(crate) name: &'a str,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct TokenClass<'a> {
    pub(crate) name: &'a str,
    pub(crate) tokens: &'a str, // Raw token list like "ID|STRING"
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct GrammarRule<'a> {
    pub(crate) lhs: &'a str,
    pub(crate) rhs: Vec<RhsSymbol<'a>>,
    pub(crate) precedence_override: Option<&'a str>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct RhsSymbol<'a> {
    pub(crate) name: &'a str,
    pub(crate) alias: Option<&'a str>,
}

// ============================================================================
// Display Implementations
// ============================================================================

impl fmt::Display for GrammarRule<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let rhs = self
            .rhs
            .iter()
            .map(|sym| sym.name)
            .collect::<Vec<_>>()
            .join(" ");

        match self.precedence_override {
            Some(prec) => write!(f, "{} ::= {} [{}]", self.lhs, rhs, prec),
            None => write!(f, "{} ::= {}", self.lhs, rhs),
        }
    }
}

impl fmt::Display for TokenClass<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "%token_class {}  {}", self.name, self.tokens)
    }
}

impl fmt::Display for TokenDecl<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.name)
    }
}

// ============================================================================
// Error Handling
// ============================================================================

#[derive(Debug)]
pub(crate) struct ParseError {
    pub(crate) line: usize,
    pub(crate) column: usize,
    pub(crate) message: String,
}

pub(crate) type Result<T> = std::result::Result<T, ParseError>;

// ============================================================================
// %if / %ifdef condition evaluator
// ============================================================================

use std::collections::HashSet;

/// Evaluate a boolean expression over a set of defined symbols.
///
/// Each identifier is looked up in `defined`: present → `true`, absent → `false`.
/// Supports `!` (negation), `||` (or), `&&` (and). `&&` binds tighter than `||`.
fn eval_condition(condition: &str, defined: &HashSet<&str>) -> bool {
    // Tokenize into: identifiers, `!`, `||`, `&&`.
    let mut tokens = Vec::new();
    let bytes = condition.as_bytes();
    let mut i = 0;
    while i < bytes.len() {
        let ch = bytes[i];
        if ch.is_ascii_alphanumeric() || ch == b'_' {
            let start = i;
            while i < bytes.len() && (bytes[i].is_ascii_alphanumeric() || bytes[i] == b'_') {
                i += 1;
            }
            tokens.push(&condition[start..i]);
        } else if ch == b'!' {
            i += 1;
            tokens.push("!");
        } else if ch == b'|' {
            i += 1;
            if i < bytes.len() && bytes[i] == b'|' {
                i += 1;
            }
            tokens.push("||");
        } else if ch == b'&' {
            i += 1;
            if i < bytes.len() && bytes[i] == b'&' {
                i += 1;
            }
            tokens.push("&&");
        } else {
            i += 1; // skip whitespace, etc.
        }
    }

    // Parse into atoms (with negation) and binary operators.
    let mut atoms: Vec<bool> = Vec::new();
    let mut ops: Vec<&str> = Vec::new();
    let mut negate = false;
    for tok in &tokens {
        match *tok {
            "!" => negate = !negate,
            "||" | "&&" => {
                ops.push(tok);
                negate = false;
            }
            ident => {
                let val = defined.contains(ident) ^ negate;
                atoms.push(val);
                negate = false;
            }
        }
    }

    if atoms.is_empty() {
        return false;
    }

    // Evaluate && first (higher precedence), then ||.
    let mut groups = vec![atoms[0]];
    let mut group_ops = Vec::new();
    for (i, op) in ops.iter().enumerate() {
        if *op == "&&" {
            let last = groups.last_mut().expect("groups is non-empty");
            *last = *last && atoms[i + 1];
        } else {
            group_ops.push(*op);
            groups.push(atoms[i + 1]);
        }
    }
    groups.iter().any(|&v| v)
}

/// The default set of preprocessor symbols defined when parsing `SQLite`'s grammar.
///
/// `%ifdef SYM` includes the block when `SYM` is in this set.
/// `%if EXPR` evaluates identifiers against this set.
///
/// Convention: `SQLITE_ENABLE_*` features that syntaqlite opts into go here.
/// `SQLITE_OMIT_*` flags are **not** defined (features are available by default).
pub(crate) fn default_defines() -> HashSet<&'static str> {
    [
        // Opt-in features that syntaqlite includes in its grammar.
        "SQLITE_ENABLE_ORDERED_SET_AGGREGATES",
        // Accept ORDER BY / LIMIT on DELETE and UPDATE statements.
        // As a parser/formatter we should accept the superset of valid syntax.
        "SQLITE_ENABLE_UPDATE_DELETE_LIMIT",
    ]
    .into_iter()
    .collect()
}

// ============================================================================
// Public API
// ============================================================================

impl<'a> LemonGrammar<'a> {
    /// Parse with the default `SQLite` preprocessor defines.
    pub(crate) fn parse(input: &'a str) -> Result<Self> {
        Self::parse_with_defines(input, &default_defines())
    }

    /// Parse with a custom set of preprocessor defines.
    pub(crate) fn parse_with_defines(input: &'a str, defines: &HashSet<&str>) -> Result<Self> {
        Parser::parse_grammar(input, defines)
    }
}

// ============================================================================
// Parser Implementation
// ============================================================================

struct Parser<'a, 'b> {
    input: &'a str,
    pos: usize, // Current byte position
    line: usize,
    column: usize,
    defines: &'b HashSet<&'b str>,
}

impl<'a, 'b> Parser<'a, 'b> {
    fn parse_grammar(input: &'a str, defines: &'b HashSet<&'b str>) -> Result<LemonGrammar<'a>> {
        let mut parser = Self {
            input,
            pos: 0,
            line: 1,
            column: 1,
            defines,
        };
        let mut tokens = Vec::new();
        let mut rules = Vec::new();
        let mut token_classes = Vec::new();
        let mut fallbacks = Vec::new();
        let mut precedences = Vec::new();
        let mut wildcard = None;
        while parser.peek().is_some() {
            parser.skip_ws();
            match parser.peek() {
                Some('%') => {
                    parser.next();
                    let directive = parser.parse_identifier()?;
                    match directive {
                        "token" => {
                            parser.collect_tokens(&mut tokens)?;
                        }
                        "token_class" => {
                            parser.parse_token_class(&mut token_classes)?;
                        }
                        "fallback" => {
                            parser.parse_fallback(&mut fallbacks)?;
                        }
                        "left" | "right" | "nonassoc" => {
                            parser.parse_precedence(directive, &mut precedences)?;
                        }
                        "wildcard" => {
                            parser.skip_ws();
                            if let Ok(name) = parser.parse_identifier() {
                                wildcard = Some(name);
                            }
                            parser.skip_to_eol();
                        }
                        "ifdef" => {
                            if parser.ifdef_is_defined() {
                                parser.skip_to_eol();
                            } else {
                                let end = parser.skip_ifdef_block();
                                if end == "else" { /* include else branch */ }
                            }
                        }
                        "ifndef" if parser.ifdef_is_defined() => {
                            let end = parser.skip_ifdef_block();
                            if end == "else" { /* include else branch */ }
                        }
                        "if" => {
                            if parser.if_should_include() {
                                parser.skip_to_eol();
                            } else {
                                let end = parser.skip_ifdef_block();
                                if end == "else" { /* include else branch */ }
                            }
                        }
                        "else" => {
                            // We were including the if/ifdef/ifndef block and
                            // hit the else branch — skip it.
                            parser.skip_else_block();
                        }
                        "endif" => {
                            // Skip endif directive (already handled by skip_ifdef_block or matching ifndef)
                            parser.skip_to_eol();
                        }
                        _ => {
                            parser.skip_to_eol();
                        }
                    }
                }
                Some(ch) if ch.is_alphabetic() || ch == '_' => {
                    // Check if current line contains ::= (is a rule) or not (bare tokens)
                    let rest = &parser.input[parser.pos..];
                    let line_end = rest.find('\n').unwrap_or(rest.len());
                    let current_line = &rest[..line_end];

                    if current_line.contains("::=") {
                        rules.push(parser.parse_rule()?);
                        parser.skip_ws();
                        parser.skip_block();
                    } else {
                        // Bare token list, skip the line
                        parser.skip_to_eol();
                    }
                }
                _ => {
                    parser.next();
                }
            }
        }
        Ok(LemonGrammar {
            tokens,
            rules,
            token_classes,
            fallbacks,
            precedences,
            wildcard,
        })
    }

    fn parse_rule(&mut self) -> Result<GrammarRule<'a>> {
        let lhs = self.parse_identifier()?;

        // Skip optional LHS alias: name(X)
        self.skip_ws();
        if self.peek() == Some('(') {
            self.advance_until(')');
            self.next(); // consume ')'
        }

        // Expect ::=
        self.skip_ws();
        self.expect_multi(&[':', ':', '='])?;

        // Parse RHS symbols
        let mut rhs = Vec::new();
        loop {
            self.skip_ws();
            match self.peek() {
                Some('.') => {
                    self.next();
                    break;
                }
                Some('{') => self.skip_block(),
                Some(ch) if ch.is_alphabetic() || ch == '_' => {
                    rhs.push(self.parse_rhs_symbol()?);
                }
                _ => break,
            }
        }

        // Check for [PRECEDENCE] or {DIRECTIVE} after the dot
        self.skip_ws();
        let precedence_override = match self.peek() {
            Some('[') => {
                self.next();
                self.skip_ws();
                let prec = self.parse_identifier()?;
                self.skip_ws();
                self.expect(']')?;
                Some(prec)
            }
            Some('{') => {
                self.skip_block();
                None
            }
            _ => None,
        };

        Ok(GrammarRule {
            lhs,
            rhs,
            precedence_override,
        })
    }

    fn parse_rhs_symbol(&mut self) -> Result<RhsSymbol<'a>> {
        // Parse identifier, allowing | for alternatives (e.g., COMMIT|END)
        let start = self.pos;
        while let Some(ch) = self.peek() {
            if ch.is_alphanumeric() || ch == '_' || ch == '|' {
                self.next();
            } else {
                break;
            }
        }
        let name = &self.input[start..self.pos];

        // Check for alias: symbol(X)
        while matches!(self.peek(), Some(' ' | '\t')) {
            self.next();
        }
        let alias = if self.peek() == Some('(') {
            self.next();
            self.skip_ws();
            let a = self.parse_identifier()?;
            self.skip_ws();
            self.expect(')')?;
            Some(a)
        } else {
            None
        };

        Ok(RhsSymbol { name, alias })
    }

    fn parse_identifier(&mut self) -> Result<&'a str> {
        let start = self.pos;
        if !matches!(self.peek(), Some(ch) if ch.is_alphabetic() || ch == '_') {
            return Err(self.error("Expected identifier"));
        }
        while let Some(ch) = self.peek() {
            if ch.is_alphanumeric() || ch == '_' {
                self.next();
            } else {
                break;
            }
        }
        Ok(&self.input[start..self.pos])
    }

    fn collect_tokens(&mut self, tokens: &mut Vec<TokenDecl<'a>>) -> Result<()> {
        // Collect token names from %token directive until we hit a '.'
        loop {
            // Skip horizontal whitespace
            while matches!(self.peek(), Some(' ' | '\t')) {
                self.next();
            }

            match self.peek() {
                Some(ch) if ch.is_alphabetic() || ch == '_' => {
                    tokens.push(TokenDecl {
                        name: self.parse_identifier()?,
                    });
                }
                Some('/') => {
                    // Skip comments
                    self.skip_ws();
                }
                Some('\n') => {
                    self.next();
                    // Stop if next line doesn't start with whitespace
                    if !matches!(self.peek(), Some(' ' | '\t')) {
                        break;
                    }
                }
                Some('.') => {
                    self.next();
                    break;
                }
                _ => break,
            }
        }
        Ok(())
    }

    fn parse_token_class(&mut self, token_classes: &mut Vec<TokenClass<'a>>) -> Result<()> {
        // Format: %token_class name  TOKENS.
        self.skip_ws();
        let name = self.parse_identifier()?;
        self.skip_ws();

        // Capture the token list (e.g., "ID|STRING")
        let start = self.pos;
        while let Some(ch) = self.peek() {
            if ch == '.' {
                let tokens = &self.input[start..self.pos].trim();
                self.next(); // consume '.'
                token_classes.push(TokenClass { name, tokens });
                return Ok(());
            }
            if ch == '\n' {
                break;
            }
            self.next();
        }

        // If no '.' found, treat rest of line as tokens
        let tokens = &self.input[start..self.pos].trim();
        token_classes.push(TokenClass { name, tokens });
        Ok(())
    }

    fn parse_fallback(&mut self, fallbacks: &mut Vec<FallbackDecl<'a>>) -> Result<()> {
        self.skip_ws();
        let target = self.parse_identifier()?;
        let tokens = self.collect_identifiers_until_dot(true)?;
        fallbacks.push(FallbackDecl { target, tokens });
        Ok(())
    }

    fn parse_precedence(
        &mut self,
        assoc: &'a str,
        precedences: &mut Vec<PrecedenceDecl<'a>>,
    ) -> Result<()> {
        let tokens = self.collect_identifiers_until_dot(false)?;
        precedences.push(PrecedenceDecl { assoc, tokens });
        Ok(())
    }

    /// Collect identifier tokens until '.', optionally handling %ifdef/%ifndef/%endif inline.
    fn collect_identifiers_until_dot(&mut self, handle_ifdefs: bool) -> Result<Vec<&'a str>> {
        let mut names = Vec::new();
        loop {
            self.skip_ws();
            match self.peek() {
                Some('.') => {
                    self.next();
                    break;
                }
                Some('%') if handle_ifdefs => {
                    self.next();
                    let directive = self.parse_identifier()?;
                    match directive {
                        "ifdef" => {
                            if self.ifdef_is_defined() {
                                self.skip_to_eol();
                            } else {
                                self.skip_ifdef_block();
                            }
                        }
                        "ifndef" => {
                            if self.ifdef_is_defined() {
                                self.skip_ifdef_block();
                            } else {
                                self.skip_to_eol();
                            }
                        }
                        _ => self.skip_to_eol(),
                    }
                }
                Some(ch) if ch.is_alphabetic() || ch == '_' => {
                    names.push(self.parse_identifier()?);
                }
                _ => break,
            }
        }
        Ok(names)
    }

    fn skip_block(&mut self) {
        if self.peek() != Some('{') {
            return;
        }
        let mut depth = 0;
        while let Some(ch) = self.next() {
            if ch == '{' {
                depth += 1;
            } else if ch == '}' {
                depth -= 1;
                if depth == 0 {
                    return;
                }
            }
        }
    }

    /// Advance until target character is found (stops before the character).
    /// Returns true if found, false if end of input reached.
    fn advance_until(&mut self, target: char) -> bool {
        let rest = &self.input[self.pos..];
        let byte_count = rest.find(target).unwrap_or(rest.len());
        let found = byte_count < rest.len();

        let end_pos = (self.pos + byte_count).min(self.input.len());
        let skipped = &self.input[self.pos..end_pos];

        for ch in skipped.chars() {
            if ch == '\n' {
                self.line += 1;
                self.column = 1;
            } else {
                self.column += 1;
            }
        }

        self.pos = end_pos;
        found
    }

    fn skip_to_eol(&mut self) {
        if self.advance_until('\n') {
            self.next(); // consume the newline
        }
    }

    /// Read the symbol after `%ifdef`/`%ifndef` and check if it is defined.
    fn ifdef_is_defined(&mut self) -> bool {
        self.skip_ws();
        let start = self.pos;
        while let Some(ch) = self.peek() {
            if ch.is_alphanumeric() || ch == '_' {
                self.next();
            } else {
                break;
            }
        }
        let flag = &self.input[start..self.pos];
        self.defines.contains(flag)
    }

    /// Read the rest of the line as a `%if` condition and evaluate it.
    fn if_should_include(&mut self) -> bool {
        let start = self.pos;
        while let Some(ch) = self.peek() {
            if ch == '\n' {
                break;
            }
            self.next();
        }
        let condition = &self.input[start..self.pos];
        eval_condition(condition, self.defines)
    }

    /// Skip content until `%endif` (or `%else`). Returns the directive that
    /// ended the skip (`"endif"` or `"else"`).
    fn skip_ifdef_block(&mut self) -> &'a str {
        self.skip_to_eol();

        while self.advance_until('%') {
            self.next(); // consume %
            match self.parse_identifier().ok() {
                Some("endif") => {
                    self.skip_to_eol();
                    return "endif";
                }
                Some("else") => {
                    self.skip_to_eol();
                    return "else";
                }
                _ => {}
            }
        }
        "endif"
    }

    /// Skip from `%else` to `%endif`, discarding the else branch.
    fn skip_else_block(&mut self) {
        while self.advance_until('%') {
            self.next(); // consume %
            if self.parse_identifier().ok() == Some("endif") {
                self.skip_to_eol();
                return;
            }
        }
    }

    fn skip_ws(&mut self) {
        loop {
            match self.peek() {
                Some(ch) if ch.is_whitespace() => {
                    self.next();
                }
                Some('/') if self.pos + 1 < self.input.len() => {
                    match self.input[self.pos + 1..].chars().next() {
                        Some('/') => self.skip_line_comment(),
                        Some('*') => self.skip_block_comment(),
                        _ => return,
                    }
                }
                _ => return,
            }
        }
    }

    fn skip_line_comment(&mut self) {
        self.expect_multi(&['/', '/'])
            .expect("caller should have verified //");
        while let Some(ch) = self.next() {
            if ch == '\n' {
                break;
            }
        }
    }

    fn skip_block_comment(&mut self) {
        self.expect_multi(&['/', '*'])
            .expect("caller should have verified /*");
        while let Some(ch) = self.next() {
            if ch == '*' && self.pos < self.input.len() && self.input[self.pos..].starts_with('/') {
                self.next();
                break;
            }
        }
    }

    fn peek(&self) -> Option<char> {
        self.input[self.pos..].chars().next()
    }

    fn next(&mut self) -> Option<char> {
        let ch = self.input[self.pos..].chars().next()?;
        self.pos += ch.len_utf8();
        if ch == '\n' {
            self.line += 1;
            self.column = 1;
        } else {
            self.column += 1;
        }
        Some(ch)
    }

    fn expect(&mut self, expected: char) -> Result<()> {
        match self.peek() {
            Some(ch) if ch == expected => {
                self.next();
                Ok(())
            }
            Some(ch) => Err(self.error(&format!("Expected '{expected}', got '{ch}'"))),
            None => Err(self.error(&format!("Expected '{expected}', got EOF"))),
        }
    }

    fn expect_multi(&mut self, expected: &[char]) -> Result<()> {
        for &ch in expected {
            self.expect(ch)?;
        }
        Ok(())
    }

    fn error(&self, message: &str) -> ParseError {
        ParseError {
            line: self.line,
            column: self.column,
            message: message.to_string(),
        }
    }
}

// ============================================================================
// Tests
// ============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_token_decl() {
        let input = "%token PLUS\n%token MINUS";
        let grammar = LemonGrammar::parse(input).unwrap();
        assert_eq!(grammar.tokens.len(), 2);
        assert_eq!(grammar.tokens[0].name, "PLUS");
        assert_eq!(grammar.tokens[1].name, "MINUS");
    }

    #[test]
    fn test_parse_simple_rule() {
        let input = "expr ::= term.";
        let grammar = LemonGrammar::parse(input).unwrap();
        assert_eq!(grammar.rules.len(), 1);
        assert_eq!(grammar.rules[0].lhs, "expr");
        assert_eq!(grammar.rules[0].rhs.len(), 1);
        assert_eq!(grammar.rules[0].rhs[0].name, "term");
    }

    #[test]
    fn test_parse_rule_with_alias() {
        let input = "expr ::= expr(A) PLUS expr(B).";
        let grammar = LemonGrammar::parse(input).unwrap();
        assert_eq!(grammar.rules[0].rhs.len(), 3);
        assert_eq!(grammar.rules[0].rhs[0].alias, Some("A"));
        assert_eq!(grammar.rules[0].rhs[2].alias, Some("B"));
    }

    #[test]
    fn test_skip_action_block() {
        let input = "expr ::= term. { /* action code */ }";
        let grammar = LemonGrammar::parse(input).unwrap();
        assert_eq!(grammar.rules.len(), 1);
    }

    #[test]
    fn test_precedence_override() {
        let input = "expr ::= expr PLUS expr. [PLUS]";
        let grammar = LemonGrammar::parse(input).unwrap();
        assert_eq!(grammar.rules[0].precedence_override, Some("PLUS"));
    }
}
