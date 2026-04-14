// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Stage 1 tokenizer extraction: parse raw `SQLite` source files and produce
//! individual C fragment files.

use std::fs;
use std::path::Path;

use crate::util::c_extractor::CExtractor;

/// All extracted tokenizer fragments.
pub(crate) struct TokenizerFragments {
    pub(crate) cc_defines: String,
    pub(crate) ai_class: String,
    pub(crate) ctype_map: String,
    pub(crate) upper_to_lower: String,
    pub(crate) is_macros: String,
    pub(crate) id_char: String,
    pub(crate) char_map: String,
    pub(crate) get_token_fn: String,
    pub(crate) window_keyword_analysis: String,
}

/// Extract tokenizer fragments from raw `SQLite` source files.
///
/// # Errors
///
/// Returns an error if any required C fragment cannot be extracted from the source files.
pub(crate) fn extract_fragments(
    tokenize_content: &str,
    global_content: &str,
    sqliteint_content: &str,
) -> Result<TokenizerFragments, String> {
    let tokenize_extractor = CExtractor::new(tokenize_content);
    let global_extractor = CExtractor::new(global_content);
    let sqliteint_extractor = CExtractor::new(sqliteint_content);

    let cc_defines = tokenize_extractor.extract_specific_defines(&[
        "CC_X",
        "CC_KYWD0",
        "CC_KYWD",
        "CC_DIGIT",
        "CC_DOLLAR",
        "CC_VARALPHA",
        "CC_VARNUM",
        "CC_SPACE",
        "CC_QUOTE",
        "CC_QUOTE2",
        "CC_PIPE",
        "CC_MINUS",
        "CC_LT",
        "CC_GT",
        "CC_EQ",
        "CC_BANG",
        "CC_SLASH",
        "CC_LP",
        "CC_RP",
        "CC_SEMI",
        "CC_PLUS",
        "CC_STAR",
        "CC_PERCENT",
        "CC_COMMA",
        "CC_AND",
        "CC_TILDA",
        "CC_DOT",
        "CC_ID",
        "CC_ILLEGAL",
        "CC_NUL",
        "CC_BOM",
    ])?;
    let ai_class = tokenize_extractor.extract_static_array("aiClass")?;
    let ctype_map = global_extractor.extract_static_array("sqlite3CtypeMap")?;
    let upper_to_lower = global_extractor.extract_static_array("sqlite3UpperToLower")?;
    let is_macros = sqliteint_extractor.extract_specific_defines(&[
        "sqlite3Isspace",
        "sqlite3Isdigit",
        "sqlite3Isxdigit",
    ])?;
    let id_char = tokenize_extractor.extract_defines_with_ifdef_context(&["IdChar"])?;
    let char_map = tokenize_extractor.extract_defines_with_ifdef_context(&["charMap"])?;
    let get_token_fn = tokenize_extractor.extract_function("sqlite3GetToken")?;

    // Window keyword analysis: getToken helper + analyze{Window,Over,Filter}Keyword.
    // These are used to context-sensitively reclassify WINDOW/OVER/FILTER tokens.
    let window_keyword_analysis = {
        let get_token_helper = tokenize_extractor.extract_function("getToken")?;
        let analyze_window = tokenize_extractor.extract_function("analyzeWindowKeyword")?;
        let analyze_over = tokenize_extractor.extract_function("analyzeOverKeyword")?;
        let analyze_filter = tokenize_extractor.extract_function("analyzeFilterKeyword")?;
        [
            get_token_helper.text.as_str(),
            analyze_window.text.as_str(),
            analyze_over.text.as_str(),
            analyze_filter.text.as_str(),
        ]
        .join("\n")
    };

    Ok(TokenizerFragments {
        cc_defines: cc_defines.text,
        ai_class: ai_class.text,
        ctype_map: ctype_map.text,
        upper_to_lower: upper_to_lower.text,
        is_macros: is_macros.text,
        id_char: id_char.text,
        char_map: char_map.text,
        get_token_fn: get_token_fn.text,
        window_keyword_analysis,
    })
}

/// Write extracted tokenizer fragments to the output directory.
///
/// # Errors
///
/// Returns an error if creating the output directory or writing any fragment file fails.
pub(crate) fn write_fragments(
    fragments: &TokenizerFragments,
    output_dir: &Path,
) -> Result<(), String> {
    fs::create_dir_all(output_dir)
        .map_err(|e| format!("creating fragment dir {}: {e}", output_dir.display()))?;

    let files = [
        ("cc_defines.c", &fragments.cc_defines),
        ("ai_class.c", &fragments.ai_class),
        ("ctype_map.c", &fragments.ctype_map),
        ("upper_to_lower.c", &fragments.upper_to_lower),
        ("is_macros.c", &fragments.is_macros),
        ("id_char.c", &fragments.id_char),
        ("char_map.c", &fragments.char_map),
        ("get_token_fn.c", &fragments.get_token_fn),
        (
            "window_keyword_analysis.c",
            &fragments.window_keyword_analysis,
        ),
    ];

    for (name, content) in &files {
        let path = output_dir.join(name);
        let with_blessing = format!("{}{}", super::SQLITE_BLESSING, content);
        fs::write(&path, with_blessing).map_err(|e| format!("writing {}: {e}", path.display()))?;
    }

    Ok(())
}
