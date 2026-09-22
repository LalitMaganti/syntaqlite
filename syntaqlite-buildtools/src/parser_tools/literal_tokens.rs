// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Fixed operator spellings for dialect tokenizers.

use std::fmt::Write;

#[derive(Default)]
pub(crate) struct LiteralTokens(Vec<(String, String)>);

impl LiteralTokens {
    pub(crate) fn parse(files: &[(String, String)]) -> Result<Self, String> {
        let mut tokens = Vec::new();
        for (file, source) in files {
            for (line, text) in source.lines().enumerate() {
                let text = text.trim();
                if text.is_empty() || text.starts_with('#') {
                    continue;
                }
                let error = |reason| format!("{file}:{}: {reason}", line + 1);
                let (name, literal) = text
                    .split_once('=')
                    .ok_or_else(|| error("expected NAME = \"operator\";"))?;
                let name = name.trim();
                let literal = literal
                    .trim()
                    .strip_suffix(';')
                    .map(str::trim)
                    .and_then(|s| s.strip_prefix('"'))
                    .and_then(|s| s.strip_suffix('"'))
                    .ok_or_else(|| error("expected a quoted operator followed by ';'"))?;
                if !name.starts_with(|c: char| c.is_ascii_uppercase())
                    || !name
                        .bytes()
                        .all(|b| b.is_ascii_uppercase() || b.is_ascii_digit() || b == b'_')
                {
                    return Err(error("expected an uppercase token name"));
                }
                if literal.is_empty() || !literal.bytes().all(|b| b"+-*/%<>=!|&~^".contains(&b)) {
                    return Err(error(
                        "only nonempty operator punctuation is supported: +-*/%<>=!|&~^",
                    ));
                }
                if literal.starts_with("--")
                    || literal.starts_with("/*")
                    || [
                        "+", "-", "*", "/", "%", "<", ">", "=", "!", "|", "&", "~", "==", "<=",
                        ">=", "!=", "<>", "<<", ">>", "||", "->", "->>",
                    ]
                    .contains(&literal)
                {
                    return Err(error("operator conflicts with SQLite or macro syntax"));
                }
                if tokens.iter().any(|(n, s)| n == name || s == literal) {
                    return Err(error("duplicate token name or spelling"));
                }
                tokens.push((name.to_owned(), literal.to_owned()));
            }
        }
        tokens.sort_by(|a, b| b.1.len().cmp(&a.1.len()).then(a.cmp(b)));
        Ok(Self(tokens))
    }

    pub(crate) fn contains(&self, name: &str) -> bool {
        self.0.iter().any(|(n, _)| n == name)
    }

    pub(crate) fn declarations(&self) -> String {
        let mut out = String::new();
        for (name, _) in &self.0 {
            writeln!(out, "%token {name}.").expect("string write");
        }
        out
    }

    pub(crate) fn validate_ids(&self, defines: &[(String, u32)]) -> Result<(), String> {
        // BANG is the final base token in parser-actions/ztokens.y.
        let base_end = defines
            .iter()
            .find(|(n, _)| n == "BANG")
            .expect("base BANG token")
            .1;
        for (name, _) in &self.0 {
            if defines.iter().any(|(n, id)| n == name && *id <= base_end) {
                return Err(format!(
                    "literal token {name} conflicts with a base SQLite token"
                ));
            }
        }
        Ok(())
    }

    pub(crate) fn wrap(&self, source: String, function: &str) -> String {
        if self.0.is_empty() {
            return source;
        }
        let base = format!("{function}Base");
        let mut out = source.replace(&format!("i64 {function}("), &format!("static i64 {base}("));
        writeln!(out, "\ni64 {function}(const SyntaqliteDialect* env, const unsigned char* z, int* tokenType) {{").expect("string write");
        writeln!(out, "  i64 n = {base}(env, z, tokenType);").expect("string write");
        for (name, literal) in &self.0 {
            // Short-circuit byte comparisons never read past the terminating NUL.
            let condition = literal
                .bytes()
                .enumerate()
                .map(|(i, b)| format!("z[{i}] == {b}"))
                .collect::<Vec<_>>()
                .join(" && ");
            writeln!(
                out,
                "  if (n <= {} && {condition}) {{ *tokenType = SYNTAQLITE_TK_{name}; return {}; }}",
                literal.len(),
                literal.len()
            )
            .expect("string write");
        }
        out.push_str("  return n;\n}\n");
        out
    }
}

#[cfg(test)]
mod tests {
    use super::LiteralTokens;

    #[test]
    fn rejects_invalid_declarations() {
        for source in [
            "PIPE",
            "PIPE = \"|>\"",
            "pipe = \"|>\";",
            "PIPE = \"\";",
            "PIPE = \"word\";",
            "PIPE = \"--><\";",
            "PIPE = \"/*>\";",
            "PIPE = \"||\";",
            "PIPE = \"|>\";\nPIPE = \"^>\";",
            "PIPE = \"|>\";\nOTHER = \"|>\";",
        ] {
            let error = LiteralTokens::parse(&[("test.tokens".into(), source.into())])
                .err()
                .expect("invalid");
            assert!(error.starts_with("test.tokens:"), "{error}");
        }
    }
}
