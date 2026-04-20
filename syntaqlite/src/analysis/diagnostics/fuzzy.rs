// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

/// Compute the Levenshtein distance between two strings (case-insensitive).
///
/// Uses `O(n)` space where `n = b.len()`. SQL identifiers are ASCII, so this
/// uses byte-level comparison with ASCII lowercase conversion.
pub(crate) fn levenshtein_distance(a: &str, b: &str) -> usize {
    let a: Vec<u8> = a.bytes().map(|c| c.to_ascii_lowercase()).collect();
    let b: Vec<u8> = b.bytes().map(|c| c.to_ascii_lowercase()).collect();

    let m = a.len();
    let n = b.len();

    if m == 0 {
        return n;
    }
    if n == 0 {
        return m;
    }

    let mut prev: Vec<usize> = (0..=n).collect();
    let mut curr = vec![0usize; n + 1];

    for i in 1..=m {
        curr[0] = i;
        for j in 1..=n {
            let cost = usize::from(a[i - 1] != b[j - 1]);
            curr[j] = (prev[j] + 1).min(curr[j - 1] + 1).min(prev[j - 1] + cost);
        }
        std::mem::swap(&mut prev, &mut curr);
    }

    prev[n]
}

/// Find the best matching candidate within a maximum Levenshtein distance.
///
/// Returns the closest match, or `None` if no candidate is within `threshold`.
pub(crate) fn best_suggestion(
    name: &str,
    candidates: &[String],
    threshold: usize,
) -> Option<String> {
    candidates
        .iter()
        .map(|c| (levenshtein_distance(name, c), c))
        .filter(|&(dist, _)| dist <= threshold)
        .min_by_key(|&(dist, _)| dist)
        .map(|(_, s)| s.clone())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn identical_strings() {
        assert_eq!(levenshtein_distance("hello", "hello"), 0);
    }

    #[test]
    fn case_insensitive() {
        assert_eq!(levenshtein_distance("Hello", "hello"), 0);
        assert_eq!(levenshtein_distance("ABC", "abc"), 0);
    }

    #[test]
    fn single_edit() {
        assert_eq!(levenshtein_distance("cat", "bat"), 1);
        assert_eq!(levenshtein_distance("cat", "cats"), 1);
        assert_eq!(levenshtein_distance("cat", "at"), 1);
    }

    #[test]
    fn multiple_edits() {
        assert_eq!(levenshtein_distance("kitten", "sitting"), 3);
    }

    #[test]
    fn empty_strings() {
        assert_eq!(levenshtein_distance("", ""), 0);
        assert_eq!(levenshtein_distance("abc", ""), 3);
        assert_eq!(levenshtein_distance("", "abc"), 3);
    }

    #[test]
    fn best_suggestion_within_threshold() {
        let candidates = vec![
            "users".to_string(),
            "orders".to_string(),
            "products".to_string(),
        ];
        assert_eq!(
            best_suggestion("usres", &candidates, 2),
            Some("users".to_string())
        );
    }

    #[test]
    fn best_suggestion_none_beyond_threshold() {
        let candidates = vec!["users".to_string(), "orders".to_string()];
        assert_eq!(best_suggestion("xyz", &candidates, 2), None);
    }

    #[test]
    fn best_suggestion_case_insensitive() {
        let candidates = vec!["Users".to_string(), "Orders".to_string()];
        assert_eq!(
            best_suggestion("users", &candidates, 0),
            Some("Users".to_string())
        );
    }
}
