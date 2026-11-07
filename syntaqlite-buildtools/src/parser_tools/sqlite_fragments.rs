// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Pre-extracted `SQLite` C fragments loaded from committed data files.
//!
//! Stage 1 (`sqlite-extract`) extracts these fragments from raw `SQLite` source
//! and writes them to `data/sqlite_fragments/`. Stage 3 (always compiled)
//! loads them here via `include_str!` for dialect-specific assembly.

pub(crate) struct SqliteFragments {
    pub cc_defines: &'static str,
    pub ai_class: &'static str,
    pub ctype_map: &'static str,
    pub is_macros: &'static str,
    pub id_char: &'static str,
    pub get_token_fn: &'static str,
    pub parser_cflags: &'static str,
}

pub(crate) const fn load() -> SqliteFragments {
    SqliteFragments {
        cc_defines: include_str!(concat!(
            env!("CARGO_MANIFEST_DIR"),
            "/sqlite-vendored/sources/fragments/cc_defines.c"
        )),
        ai_class: include_str!(concat!(
            env!("CARGO_MANIFEST_DIR"),
            "/sqlite-vendored/sources/fragments/ai_class.c"
        )),
        ctype_map: include_str!(concat!(
            env!("CARGO_MANIFEST_DIR"),
            "/sqlite-vendored/sources/fragments/ctype_map.c"
        )),
        is_macros: include_str!(concat!(
            env!("CARGO_MANIFEST_DIR"),
            "/sqlite-vendored/sources/fragments/is_macros.c"
        )),
        id_char: include_str!(concat!(
            env!("CARGO_MANIFEST_DIR"),
            "/sqlite-vendored/sources/fragments/id_char.c"
        )),
        get_token_fn: include_str!(concat!(
            env!("CARGO_MANIFEST_DIR"),
            "/sqlite-vendored/sources/fragments/get_token_fn.c"
        )),
        parser_cflags: include_str!(concat!(
            env!("CARGO_MANIFEST_DIR"),
            "/sqlite-vendored/data/cflags.json"
        )),
    }
}
