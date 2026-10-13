// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Grammar (parse.y) analysis across `SQLite` versions.
//!
//! Uses `LemonGrammar::parse()` to extract rules, tokens, `token_classes`,
//! fallbacks, and precedences from each version's parse.y, then compares
//! them to produce a structured diff report.

use std::collections::BTreeSet;
use std::fmt::Write;

use serde::Serialize;

use crate::util::grammar_parser::LemonGrammar;

use super::SqliteVersion;

/// Structured summary of a single version's grammar.
#[derive(Debug, Clone, Serialize)]
pub struct GrammarSummary {
    /// Canonical rule signatures, sorted. E.g. "cmd ::= EXPLAIN cmd"
    pub(crate) rule_signatures: Vec<String>,
    /// Set of nonterminal names (LHS of rules).
    pub(crate) nonterminals: BTreeSet<String>,
    /// Set of terminal/token names from %token declarations.
    pub(crate) token_decls: BTreeSet<String>,
    /// Token class declarations as "name = tokens" strings.
    pub(crate) token_classes: Vec<String>,
    /// Fallback declarations as "target <- token1 token2 ..." strings.
    pub(crate) fallbacks: Vec<String>,
    /// Precedence declarations as "assoc token1 token2 ..." strings.
    pub(crate) precedences: Vec<String>,
}

/// Changes between two consecutive grammar versions.
#[derive(Debug, Clone, Serialize)]
pub struct GrammarDiff {
    pub(crate) from_version: SqliteVersion,
    pub(crate) to_version: SqliteVersion,
    pub(crate) rules_added: Vec<String>,
    pub(crate) rules_removed: Vec<String>,
    pub(crate) tokens_added: Vec<String>,
    pub(crate) tokens_removed: Vec<String>,
    pub(crate) nonterminals_added: Vec<String>,
    pub(crate) nonterminals_removed: Vec<String>,
    pub(crate) token_classes_added: Vec<String>,
    pub(crate) token_classes_removed: Vec<String>,
    pub(crate) fallbacks_added: Vec<String>,
    pub(crate) fallbacks_removed: Vec<String>,
    pub(crate) precedences_added: Vec<String>,
    pub(crate) precedences_removed: Vec<String>,
}

impl GrammarDiff {
    pub(crate) const fn is_empty(&self) -> bool {
        self.rules_added.is_empty()
            && self.rules_removed.is_empty()
            && self.tokens_added.is_empty()
            && self.tokens_removed.is_empty()
            && self.nonterminals_added.is_empty()
            && self.nonterminals_removed.is_empty()
            && self.token_classes_added.is_empty()
            && self.token_classes_removed.is_empty()
            && self.fallbacks_added.is_empty()
            && self.fallbacks_removed.is_empty()
            && self.precedences_added.is_empty()
            && self.precedences_removed.is_empty()
    }
}

/// Full grammar analysis across all versions.
#[derive(Debug, Clone, Serialize)]
pub struct GrammarAnalysis {
    /// Per-version grammar summaries, in version order.
    pub per_version: Vec<(SqliteVersion, GrammarSummary)>,
    /// Diffs between consecutive versions where grammar changed.
    pub diffs: Vec<GrammarDiff>,
    /// Versions where grammar parsing failed.
    pub errors: Vec<(SqliteVersion, String)>,
}

/// Extract a `GrammarSummary` from parse.y source text.
pub(crate) fn extract_grammar_summary(parse_y: &str) -> Result<GrammarSummary, String> {
    let grammar = LemonGrammar::parse(parse_y).map_err(|e| {
        format!(
            "grammar parse error at line {}:{}: {}",
            e.line, e.column, e.message
        )
    })?;

    let mut rule_signatures: Vec<String> = grammar.rules.iter().map(ToString::to_string).collect();
    rule_signatures.sort();

    let nonterminals: BTreeSet<String> = grammar.rules.iter().map(|r| r.lhs.to_string()).collect();

    let token_decls: BTreeSet<String> = grammar.tokens.iter().map(|t| t.name.to_string()).collect();

    let mut token_classes: Vec<String> = grammar
        .token_classes
        .iter()
        .map(|tc| format!("{} = {}", tc.name, tc.tokens))
        .collect();
    token_classes.sort();

    let mut fallbacks: Vec<String> = grammar
        .fallbacks
        .iter()
        .map(|fb| {
            let tokens = fb.tokens.join(" ");
            format!("{} <- {}", fb.target, tokens)
        })
        .collect();
    fallbacks.sort();

    let precedences: Vec<String> = grammar
        .precedences
        .iter()
        .map(|p| {
            let tokens = p.tokens.join(" ");
            format!("{} {}", p.assoc, tokens)
        })
        .collect();

    Ok(GrammarSummary {
        rule_signatures,
        nonterminals,
        token_decls,
        token_classes,
        fallbacks,
        precedences,
    })
}

/// Compute diffs between consecutive version summaries.
pub(crate) fn compute_grammar_diffs(
    versions: &[(SqliteVersion, GrammarSummary)],
) -> Vec<GrammarDiff> {
    let mut diffs = Vec::new();

    for pair in versions.windows(2) {
        let (from_ver, from_sum) = &pair[0];
        let (to_ver, to_sum) = &pair[1];

        let diff = diff_summaries(from_ver, from_sum, to_ver, to_sum);
        if !diff.is_empty() {
            diffs.push(diff);
        }
    }

    diffs
}

fn diff_summaries(
    from_ver: &SqliteVersion,
    from: &GrammarSummary,
    to_ver: &SqliteVersion,
    to: &GrammarSummary,
) -> GrammarDiff {
    let from_rules: BTreeSet<&str> = from.rule_signatures.iter().map(String::as_str).collect();
    let to_rules: BTreeSet<&str> = to.rule_signatures.iter().map(String::as_str).collect();

    let from_tc: BTreeSet<&str> = from.token_classes.iter().map(String::as_str).collect();
    let to_tc: BTreeSet<&str> = to.token_classes.iter().map(String::as_str).collect();

    let from_fb: BTreeSet<&str> = from.fallbacks.iter().map(String::as_str).collect();
    let to_fb: BTreeSet<&str> = to.fallbacks.iter().map(String::as_str).collect();

    let from_prec: BTreeSet<&str> = from.precedences.iter().map(String::as_str).collect();
    let to_prec: BTreeSet<&str> = to.precedences.iter().map(String::as_str).collect();

    GrammarDiff {
        from_version: from_ver.clone(),
        to_version: to_ver.clone(),
        rules_added: sorted_diff(&to_rules, &from_rules),
        rules_removed: sorted_diff(&from_rules, &to_rules),
        tokens_added: sorted_diff(&to.token_decls, &from.token_decls),
        tokens_removed: sorted_diff(&from.token_decls, &to.token_decls),
        nonterminals_added: sorted_diff(&to.nonterminals, &from.nonterminals),
        nonterminals_removed: sorted_diff(&from.nonterminals, &to.nonterminals),
        token_classes_added: sorted_diff(&to_tc, &from_tc),
        token_classes_removed: sorted_diff(&from_tc, &to_tc),
        fallbacks_added: sorted_diff(&to_fb, &from_fb),
        fallbacks_removed: sorted_diff(&from_fb, &to_fb),
        precedences_added: sorted_diff(&to_prec, &from_prec),
        precedences_removed: sorted_diff(&from_prec, &to_prec),
    }
}

fn sorted_diff<T: Ord + ToString>(a: &BTreeSet<T>, b: &BTreeSet<T>) -> Vec<String> {
    a.difference(b).map(ToString::to_string).collect()
}

/// Format grammar analysis as a human-readable report section.
#[must_use]
pub fn format_grammar_report(analysis: &GrammarAnalysis) -> String {
    let mut out = String::new();

    let _ = writeln!(out, "## Grammar (`parse.y`)");
    let _ = writeln!(out);

    // Summary table.
    if let Some((first_ver, first_sum)) = analysis.per_version.first()
        && let Some((last_ver, last_sum)) = analysis.per_version.last()
    {
        let _ = writeln!(
            out,
            "Analyzed {} versions ({} through {}).",
            analysis.per_version.len(),
            first_ver,
            last_ver
        );
        let _ = writeln!(out);

        let _ = writeln!(out, "| Metric | {first_ver} | {last_ver} |");
        let _ = writeln!(out, "| --- | --- | --- |");
        let _ = writeln!(
            out,
            "| Rules | {} | {} |",
            first_sum.rule_signatures.len(),
            last_sum.rule_signatures.len()
        );
        let _ = writeln!(
            out,
            "| Nonterminals | {} | {} |",
            first_sum.nonterminals.len(),
            last_sum.nonterminals.len()
        );
        let _ = writeln!(
            out,
            "| Token declarations | {} | {} |",
            first_sum.token_decls.len(),
            last_sum.token_decls.len()
        );
        let _ = writeln!(
            out,
            "| Token classes | {} | {} |",
            first_sum.token_classes.len(),
            last_sum.token_classes.len()
        );
        let _ = writeln!(
            out,
            "| Fallback decls | {} | {} |",
            first_sum.fallbacks.len(),
            last_sum.fallbacks.len()
        );
        let _ = writeln!(
            out,
            "| Precedence decls | {} | {} |",
            first_sum.precedences.len(),
            last_sum.precedences.len()
        );
        let _ = writeln!(out);
    }

    // Errors.
    if !analysis.errors.is_empty() {
        let _ = writeln!(out, "### Parse errors");
        let _ = writeln!(out);
        for (ver, err) in &analysis.errors {
            let _ = writeln!(out, "- **{ver}**: {err}");
        }
        let _ = writeln!(out);
    }

    // Transition details.
    if analysis.diffs.is_empty() {
        let _ = writeln!(out, "No grammar changes detected across all versions.");
        return out;
    }

    let _ = writeln!(
        out,
        "### Transitions ({} change points)",
        analysis.diffs.len()
    );
    let _ = writeln!(out);

    for diff in &analysis.diffs {
        let _ = writeln!(out, "#### {} → {}", diff.from_version, diff.to_version);
        let _ = writeln!(out);

        format_diff_section(&mut out, "Rules added", &diff.rules_added);
        format_diff_section(&mut out, "Rules removed", &diff.rules_removed);
        format_diff_section(&mut out, "Tokens added", &diff.tokens_added);
        format_diff_section(&mut out, "Tokens removed", &diff.tokens_removed);
        format_diff_section(&mut out, "Nonterminals added", &diff.nonterminals_added);
        format_diff_section(&mut out, "Nonterminals removed", &diff.nonterminals_removed);
        format_diff_section(&mut out, "Token classes added", &diff.token_classes_added);
        format_diff_section(
            &mut out,
            "Token classes removed",
            &diff.token_classes_removed,
        );
        format_diff_section(&mut out, "Fallbacks added", &diff.fallbacks_added);
        format_diff_section(&mut out, "Fallbacks removed", &diff.fallbacks_removed);
        format_diff_section(&mut out, "Precedences added", &diff.precedences_added);
        format_diff_section(&mut out, "Precedences removed", &diff.precedences_removed);
    }

    out
}

fn format_diff_section(out: &mut String, label: &str, items: &[String]) {
    if items.is_empty() {
        return;
    }
    let _ = writeln!(out, "**{label}** ({}):", items.len());
    for item in items {
        let _ = writeln!(out, "- `{item}`");
    }
    let _ = writeln!(out);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn extract_simple_grammar() {
        let input = r"
%token PLUS MINUS.
%left PLUS MINUS.

expr ::= expr(A) PLUS expr(B). { A + B }
expr ::= INTEGER(A). { A }
";
        let summary = extract_grammar_summary(input).unwrap();
        assert_eq!(summary.rule_signatures.len(), 2);
        assert!(summary.nonterminals.contains("expr"));
        assert!(summary.token_decls.contains("PLUS"));
        assert!(summary.token_decls.contains("MINUS"));
        assert_eq!(summary.precedences.len(), 1);
    }

    #[test]
    fn diff_detects_added_rule() {
        let v1 = GrammarSummary {
            rule_signatures: vec!["expr ::= INTEGER".into()],
            nonterminals: ["expr".into()].into_iter().collect(),
            token_decls: BTreeSet::new(),
            token_classes: vec![],
            fallbacks: vec![],
            precedences: vec![],
        };
        let v2 = GrammarSummary {
            rule_signatures: vec!["expr ::= INTEGER".into(), "expr ::= expr PLUS expr".into()],
            nonterminals: ["expr".into()].into_iter().collect(),
            token_decls: BTreeSet::new(),
            token_classes: vec![],
            fallbacks: vec![],
            precedences: vec![],
        };

        let ver1 = SqliteVersion::parse("3.24.0").unwrap();
        let ver2 = SqliteVersion::parse("3.25.0").unwrap();
        let diff = diff_summaries(&ver1, &v1, &ver2, &v2);
        assert_eq!(diff.rules_added, vec!["expr ::= expr PLUS expr"]);
        assert_eq!(diff.rules_removed, Vec::<String>::new());
    }
}
