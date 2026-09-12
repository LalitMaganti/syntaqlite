//! Independent parser/lexer oracle: preserve all authored tokens and comments,
//! allowing configured keyword casing and statement-terminator normalization.
use std::collections::BTreeSet;
use syntaqlite::any::AnyDialect;
use syntaqlite_syntax::any::{AnyParser, AnyTokenizer, ParseOutcome, TokenCategory};

pub(crate) fn signature(sql: &str, dialect: &AnyDialect) -> Vec<String> {
    let parser = AnyParser::with_config(
        (**dialect).clone(),
        &syntaqlite_syntax::ParserConfig::default().with_collect_tokens(true),
    );
    let mut session = parser.parse(sql);
    let mut keywords = BTreeSet::new();
    while let ParseOutcome::Ok(stmt) = session.next() {
        for token in stmt.tokens() {
            if dialect.classify_token(token.token_type(), token.flags()) == TokenCategory::Keyword {
                keywords.insert(
                    token
                        .stmt_range()
                        .start
                        .to_doc(stmt.statement_base())
                        .as_usize(),
                );
            }
        }
    }
    AnyTokenizer::new((**dialect).clone())
        .tokenize(sql)
        .filter(|t| !t.text().trim().is_empty() && t.text() != ";")
        .map(|t| {
            if keywords.contains(&(t.text().as_ptr() as usize - sql.as_ptr() as usize)) {
                t.text().to_ascii_uppercase()
            } else {
                t.text().to_owned()
            }
        })
        .collect()
}
