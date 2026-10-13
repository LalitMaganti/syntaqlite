// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Grammar verification: compare rule signatures between upstream `parse.y`
//! and our concatenated action `.y` files.

use std::collections::BTreeSet;
use std::fmt;

use crate::util::grammar_parser::LemonGrammar;

/// A mismatch between upstream grammar and our action files.
#[derive(Debug)]
pub(crate) struct GrammarMismatch {
    pub rules_missing: Vec<String>,
    pub rules_extra: Vec<String>,
    pub fallbacks_missing: Vec<String>,
    pub fallbacks_extra: Vec<String>,
    pub precedences_missing: Vec<String>,
    pub precedences_extra: Vec<String>,
    pub token_classes_missing: Vec<String>,
    pub token_classes_extra: Vec<String>,
}

impl GrammarMismatch {
    fn is_empty(&self) -> bool {
        self.rules_missing.is_empty()
            && self.rules_extra.is_empty()
            && self.fallbacks_missing.is_empty()
            && self.fallbacks_extra.is_empty()
            && self.precedences_missing.is_empty()
            && self.precedences_extra.is_empty()
            && self.token_classes_missing.is_empty()
            && self.token_classes_extra.is_empty()
    }
}

impl fmt::Display for GrammarMismatch {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fn write_section(f: &mut fmt::Formatter<'_>, label: &str, items: &[String]) -> fmt::Result {
            if !items.is_empty() {
                writeln!(f, "\n{label} ({}):", items.len())?;
                for item in items {
                    writeln!(f, "  {item}")?;
                }
            }
            Ok(())
        }

        writeln!(f, "Grammar mismatch detected:")?;
        write_section(
            f,
            "Rules missing from actions (in upstream)",
            &self.rules_missing,
        )?;
        write_section(
            f,
            "Rules extra in actions (not in upstream)",
            &self.rules_extra,
        )?;
        write_section(f, "Fallbacks missing from actions", &self.fallbacks_missing)?;
        write_section(f, "Fallbacks extra in actions", &self.fallbacks_extra)?;
        write_section(
            f,
            "Precedences missing from actions",
            &self.precedences_missing,
        )?;
        write_section(f, "Precedences extra in actions", &self.precedences_extra)?;
        write_section(
            f,
            "Token classes missing from actions",
            &self.token_classes_missing,
        )?;
        write_section(
            f,
            "Token classes extra in actions",
            &self.token_classes_extra,
        )?;
        Ok(())
    }
}

impl std::error::Error for GrammarMismatch {}

/// Rules intentionally added to our grammar that are not in upstream `parse.y`.
/// These are filtered out before comparison.
const ALLOWED_EXTRA_RULES: &[&str] = &[
    // Error recovery: treat `error SEMI` as a valid command so the parser can
    // resynchronize after a syntax error and continue parsing subsequent statements.
    "ecmd ::= error SEMI",
    // Fine-grained error recovery for interpolation holes (embedded SQL in host
    // languages). These allow the parser to accept an error token in expression
    // or name position and continue parsing the rest of the statement.
    "expr ::= error",
    "nmorerr ::= nm",
    "nmorerr ::= error",
];

/// Intentional, tracked upstream rule substitutions.
///
/// Each tuple is `(upstream_rule, local_rule)`. A rewrite only applies when
/// both sides are present in the diff (upstream rule is missing AND local rule
/// is extra), keeping drift explicit and auditable.
const ALLOWED_RULE_REWRITES: &[(&str, &str)] = &[
    ("as ::= AS nm", "as ::= AS nmorerr"),
    ("fullname ::= nm", "fullname ::= nmorerr"),
    ("fullname ::= nm DOT nm", "fullname ::= nmorerr DOT nmorerr"),
    (
        "cmd ::= ALTER TABLE fullname RENAME TO nm",
        "cmd ::= ALTER TABLE fullname RENAME TO nmorerr",
    ),
    (
        "cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nm TO nm",
        "cmd ::= ALTER TABLE fullname RENAME kwcolumn_opt nmorerr TO nmorerr",
    ),
    (
        "cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nm",
        "cmd ::= ALTER TABLE fullname DROP kwcolumn_opt nmorerr",
    ),
    (
        "columnname ::= nm typetoken",
        "columnname ::= nmorerr typetoken",
    ),
    ("cmd ::= SAVEPOINT nm", "cmd ::= SAVEPOINT nmorerr"),
    (
        "cmd ::= RELEASE savepoint_opt nm",
        "cmd ::= RELEASE savepoint_opt nmorerr",
    ),
    (
        "cmd ::= ROLLBACK trans_opt TO savepoint_opt nm",
        "cmd ::= ROLLBACK trans_opt TO savepoint_opt nmorerr",
    ),
];

/// Compare rule signatures between upstream `parse.y` and concatenated action files.
///
/// Both inputs are raw `.y` file contents (text).
///
/// Returns `Ok(())` if the rule signatures, fallbacks, precedences, and token classes
/// match exactly (modulo [`ALLOWED_EXTRA_RULES`]). Returns `Err(GrammarMismatch)`
/// with details otherwise.
pub(crate) fn verify_grammar(
    upstream_parse_y: &str,
    action_files: &[(&str, &str)], // (filename, contents)
) -> Result<(), Box<GrammarMismatch>> {
    let upstream = LemonGrammar::parse(upstream_parse_y)
        .unwrap_or_else(|e| panic!("Failed to parse upstream parse.y: {e:?}"));

    // Concatenate action files sorted by name (matching concatenate_y_contents ordering).
    let mut sorted_actions: Vec<(&str, &str)> = action_files.to_vec();
    sorted_actions.sort_by_key(|(name, _)| *name);
    let combined: String = sorted_actions
        .iter()
        .map(|(_, content)| *content)
        .collect::<Vec<_>>()
        .join("\n");

    let actions = LemonGrammar::parse(&combined)
        .unwrap_or_else(|e| panic!("Failed to parse action files: {e:?}"));

    // Compare rules.
    let upstream_rules: BTreeSet<String> = upstream.rules.iter().map(ToString::to_string).collect();
    let actions_rules: BTreeSet<String> = actions.rules.iter().map(ToString::to_string).collect();

    let mut rules_missing: BTreeSet<String> =
        upstream_rules.difference(&actions_rules).cloned().collect();
    let mut rules_extra: BTreeSet<String> =
        actions_rules.difference(&upstream_rules).cloned().collect();

    for (upstream, local) in ALLOWED_RULE_REWRITES {
        let removed_upstream = rules_missing.remove(*upstream);
        let removed_local = rules_extra.remove(*local);
        if removed_upstream != removed_local {
            if removed_upstream {
                rules_missing.insert((*upstream).to_string());
            }
            if removed_local {
                rules_extra.insert((*local).to_string());
            }
        }
    }

    let allowed: BTreeSet<&str> = ALLOWED_EXTRA_RULES.iter().copied().collect();
    rules_extra.retain(|r| !allowed.contains(r.as_str()));

    let rules_missing: Vec<String> = rules_missing.into_iter().collect();
    let rules_extra: Vec<String> = rules_extra.into_iter().collect();

    // Compare fallbacks: normalize to "target <- tok1 tok2 ..." sorted strings.
    let upstream_fallbacks = collect_fallbacks(&upstream);
    let actions_fallbacks = collect_fallbacks(&actions);

    let fallbacks_missing: Vec<String> = upstream_fallbacks
        .difference(&actions_fallbacks)
        .cloned()
        .collect();
    let fallbacks_extra: Vec<String> = actions_fallbacks
        .difference(&upstream_fallbacks)
        .cloned()
        .collect();

    // Compare precedences: normalize to "assoc tok1 tok2 ..." sorted strings.
    let upstream_precs = collect_precedences(&upstream);
    let actions_precs = collect_precedences(&actions);

    let precedences_missing: Vec<String> =
        upstream_precs.difference(&actions_precs).cloned().collect();
    let precedences_extra: Vec<String> =
        actions_precs.difference(&upstream_precs).cloned().collect();

    // Compare token classes.
    let upstream_classes = collect_token_classes(&upstream);
    let actions_classes = collect_token_classes(&actions);

    let token_classes_missing: Vec<String> = upstream_classes
        .difference(&actions_classes)
        .cloned()
        .collect();
    let token_classes_extra: Vec<String> = actions_classes
        .difference(&upstream_classes)
        .cloned()
        .collect();

    let mismatch = GrammarMismatch {
        rules_missing,
        rules_extra,
        fallbacks_missing,
        fallbacks_extra,
        precedences_missing,
        precedences_extra,
        token_classes_missing,
        token_classes_extra,
    };

    if mismatch.is_empty() {
        Ok(())
    } else {
        Err(Box::new(mismatch))
    }
}

fn collect_fallbacks(grammar: &LemonGrammar<'_>) -> BTreeSet<String> {
    grammar
        .fallbacks
        .iter()
        .map(|fb| {
            let mut tokens: Vec<&str> = fb.tokens.clone();
            tokens.sort_unstable();
            format!("{} <- {}", fb.target, tokens.join(" "))
        })
        .collect()
}

fn collect_precedences(grammar: &LemonGrammar<'_>) -> BTreeSet<String> {
    // Precedences are order-dependent in Lemon (earlier = lower precedence).
    // We compare as ordered sequences, but since BTreeSet won't capture ordering,
    // we encode position into the string for exact comparison.
    grammar
        .precedences
        .iter()
        .enumerate()
        .map(|(i, p)| {
            let mut tokens: Vec<&str> = p.tokens.clone();
            tokens.sort_unstable();
            format!("[{}] {} {}", i, p.assoc, tokens.join(" "))
        })
        .collect()
}

fn collect_token_classes(grammar: &LemonGrammar<'_>) -> BTreeSet<String> {
    grammar
        .token_classes
        .iter()
        .map(ToString::to_string)
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn verify_grammar_unit_ok() {
        let upstream = r"
%fallback ID ABORT.
%left OR.
%left AND.
%token_class ids  ID|INDEXED.
cmd ::= SELECT expr.
expr ::= ID.
expr ::= INTEGER. [OR]
";

        let actions: &[(&str, &str)] = &[(
            "a.y",
            r"
%fallback ID ABORT.
%left OR.
%left AND.
%token_class ids  ID|INDEXED.
cmd ::= SELECT expr(X). { use(X); }
expr(A) ::= ID(B). { A = B; }
expr(A) ::= INTEGER(B). [OR] { A = B; }
",
        )];

        assert!(verify_grammar(upstream, actions).is_ok());
    }

    #[test]
    fn verify_grammar_unit_missing_rule() {
        let upstream = r"
cmd ::= SELECT expr.
expr ::= ID.
expr ::= INTEGER.
";

        let actions: &[(&str, &str)] = &[(
            "a.y",
            r"
cmd ::= SELECT expr(X). { use(X); }
expr(A) ::= ID(B). { A = B; }
",
        )];

        let err = verify_grammar(upstream, actions).unwrap_err();
        assert_eq!(err.rules_missing, vec!["expr ::= INTEGER"]);
        assert_eq!(err.rules_extra, Vec::<String>::new());
    }

    #[test]
    fn verify_grammar_unit_extra_rule() {
        let upstream = r"
cmd ::= SELECT expr.
";

        let actions: &[(&str, &str)] = &[(
            "a.y",
            r"
cmd ::= SELECT expr(X). { use(X); }
expr(A) ::= ID(B). { A = B; }
",
        )];

        let err = verify_grammar(upstream, actions).unwrap_err();
        assert_eq!(err.rules_missing, Vec::<String>::new());
        assert_eq!(err.rules_extra, vec!["expr ::= ID"]);
    }

    #[test]
    fn verify_grammar_unit_allowed_rewrite() {
        let upstream = r"
as ::= AS nm.
nm ::= ID.
";

        let actions: &[(&str, &str)] = &[(
            "a.y",
            r"
as(A) ::= AS nmorerr(B). { A = B; }
nm(A) ::= ID(B). { A = B; }
nmorerr(A) ::= nm(B). { A = B; }
nmorerr(A) ::= error. { A.z = 0; A.n = 0; }
",
        )];

        assert!(verify_grammar(upstream, actions).is_ok());
    }

    #[test]
    fn verify_grammar_unit_rewrite_requires_pairing() {
        let upstream = r"
as ::= AS nm.
";

        let actions: &[(&str, &str)] = &[(
            "a.y",
            r"
// Missing local rewrite counterpart for upstream rule.
",
        )];

        let err = verify_grammar(upstream, actions).unwrap_err();
        assert_eq!(err.rules_missing, vec!["as ::= AS nm"]);
        assert_eq!(err.rules_extra, Vec::<String>::new());
    }

    #[test]
    fn verify_sqlite_grammar_matches_actions() {
        let upstream = include_str!("../../third_party/src/sqlite/src/parse.y");
        let base_files = crate::base_files::base_y_files();
        let actions: Vec<(&str, &str)> = base_files
            .iter()
            .map(|(name, content)| (*name, *content))
            .collect();
        let result = verify_grammar(upstream, &actions);
        match result {
            Ok(()) => {}
            Err(mismatch) => panic!("{mismatch}"),
        }
    }
}
