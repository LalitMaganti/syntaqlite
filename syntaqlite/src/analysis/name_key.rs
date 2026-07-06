// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Normalized identifier key for catalog and scope lookups.

/// A relation/function/source lookup key: an identifier *value* (quotes
/// stripped and escapes collapsed by the parser and the `*_ident_text`
/// accessors) folded for `SQLite`'s ASCII-case-insensitive name
/// comparison.
///
/// [`Catalog`](super::Catalog) and `QueryScope` maps are keyed by
/// `NameKey` and every lookup takes `&NameKey`, so probing with raw
/// source text is a type error. Construct with [`NameKey::new`], or
/// reuse one allocation across many probes with [`NameKey::set`].
#[derive(Debug, Clone, Default, PartialEq, Eq, Hash)]
pub(crate) struct NameKey(String);

impl NameKey {
    pub(crate) fn new(name: &str) -> Self {
        let mut key = Self(String::new());
        key.set(name);
        key
    }

    /// Re-point this key at `name`, reusing the allocation.
    pub(crate) fn set(&mut self, name: &str) {
        self.0.clear();
        self.0.push_str(name);
        // SQLite folds ASCII case only when comparing identifiers.
        self.0.make_ascii_lowercase();
    }

    pub(crate) fn as_str(&self) -> &str {
        &self.0
    }
}

#[cfg(test)]
mod tests {
    use super::NameKey;

    #[test]
    fn folds_ascii_case_only() {
        assert_eq!(NameKey::new("Users").as_str(), "users");
        assert_eq!(NameKey::new("USERS"), NameKey::new("users"));
        // Non-ASCII case is not folded, matching SQLite.
        assert_eq!(NameKey::new("Ä").as_str(), "Ä");
    }

    #[test]
    fn set_reuses_allocation() {
        let mut key = NameKey::new("first");
        let cap = {
            key.set("A_MUCH_LONGER_SECOND_NAME");
            assert_eq!(key.as_str(), "a_much_longer_second_name");
            key.0.capacity()
        };
        key.set("SHORT");
        assert_eq!(key.as_str(), "short");
        assert_eq!(key.0.capacity(), cap);
    }

    #[test]
    fn usable_as_map_key() {
        let mut map = std::collections::HashMap::new();
        map.insert(NameKey::new("Foo"), 1);
        let mut probe = NameKey::default();
        probe.set("FOO");
        assert_eq!(map.get(&probe), Some(&1));
    }
}
