// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Stage 3 tokenizer assembly: combines pre-extracted C fragments with
//! dialect-specific naming to produce the final `tokenize.c`.

use super::sqlite_fragments::SqliteFragments;
use crate::codegen_api::TokenizerExtractResult;
use crate::dialect_codegen::c_dialect::DialectCIncludes;
use crate::util::c_transformer::CTransformer;
use crate::util::c_writer::CWriter;
use crate::util::pascal_case;

/// Assemble the tokenizer C source from pre-extracted fragments.
///
/// This replaces the old `extract_tokenizer()` path: instead of running
/// `CExtractor` on raw `SQLite` source at codegen time, we read committed
/// fragment files and apply only the dialect-specific `CTransformer` pass.
pub(crate) fn assemble(
    fragments: &SqliteFragments,
    dialect: &str,
    includes: &DialectCIncludes<'_>,
) -> (String, TokenizerExtractResult) {
    let combined = {
        let mut w = CWriter::new();
        w.sqlite_file_header();
        w.include_local("syntaqlite_dialect/sqlite_compat.h");
        w.include_local(includes.tokens_header);
        w.include_local(includes.keyword_h)
            .newline()
            .fragment(&fragments.cc_defines)
            .newline()
            .fragment(&fragments.ctype_map)
            .newline()
            .fragment(&fragments.is_macros)
            .newline()
            .fragment(&fragments.id_char)
            .newline()
            .fragment(&fragments.ai_class)
            .newline()
            .fragment(&fragments.get_token_fn);
        w.finish()
    };

    let get_token_name = format!("Synq{}GetToken", pascal_case(dialect));
    let output = CTransformer::new(&combined)
        .add_array_static("sqlite3CtypeMap")
        .insert_after_includes("#include \"syntaqlite/grammar.h\"")
        .replace_in_function(
            "sqlite3GetToken",
            "keywordCode((char*)",
            "synq_sqlite3_keywordCode(env, (char*)",
        )
        .replace_in_function(
            "sqlite3GetToken",
            "sqlite3GetToken(const unsigned char *z",
            "sqlite3GetToken(const SyntaqliteGrammar* env, const unsigned char *z",
        )
        .rename_function("sqlite3GetToken", &get_token_name)
        .replace_all("TK_", "SYNTAQLITE_TK_")
        .finish();

    (
        output,
        TokenizerExtractResult {
            char_map: fragments.char_map.to_string(),
            upper_to_lower: fragments.upper_to_lower.to_string(),
        },
    )
}
