// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Stage 3 keyword hash generation: reads pre-extracted cflag data and calls
//! the mkkeyword subprocess to produce the final `keyword.c`.

use std::collections::HashMap;
use std::fs;

use super::sqlite_fragments::SqliteFragments;
use crate::codegen_api::TokenizerExtractResult;
use crate::util::c_transformer::CTransformer;
use crate::util::c_writer::CWriter;

/// Return the `SQLite` version (as integer, e.g. 3035000) in which a keyword was
/// first introduced.  Returns 0 for baseline keywords (present in 3.12.2).
fn keyword_since_version(name: &str) -> i32 {
    match name {
        "DO" | "NOTHING" => 3_024_000,
        "CURRENT" | "FILTER" | "FOLLOWING" | "OVER" | "PARTITION" | "PRECEDING" | "RANGE"
        | "ROWS" | "UNBOUNDED" | "WINDOW" => 3_025_000,
        "EXCLUDE" | "GROUPS" | "OTHERS" | "TIES" => 3_028_000,
        "FIRST" | "LAST" | "NULLS" => 3_030_000,
        "ALWAYS" | "GENERATED" => 3_031_000,
        "MATERIALIZED" | "RETURNING" => 3_035_000,
        "WITHIN" => 3_047_000,
        _ => 0,
    }
}

/// Parse the pre-extracted keyword cflag JSON data.
///
/// The JSON is grouped by category; currently only the `"parser"` group is used
/// here (all keyword-gated cflags are parser-category flags).
/// Cflag indices are group-local (i.e. local to the `"parser"` group).
fn parse_keyword_cflags(data: &str) -> HashMap<String, (u32, u8)> {
    #[derive(serde::Deserialize)]
    struct KeywordCflags {
        parser: Vec<KeywordCflagEntry>,
    }
    #[derive(serde::Deserialize)]
    struct KeywordCflagEntry {
        name: String,
        cflag: u32,
        polarity: u8,
    }

    let catalog: KeywordCflags = serde_json::from_str(data).expect("invalid cflags.json");
    catalog
        .parser
        .into_iter()
        .map(|e| (e.name, (e.cflag, e.polarity)))
        .collect()
}

/// Parse `testcase(i==N); /* NAME */` comments from raw mkkeywordhash output
/// to get (1-based index, keyword name) pairs.
fn parse_keyword_indices(code: &str) -> Vec<(usize, String)> {
    let mut result = Vec::new();
    for line in code.lines() {
        let trimmed = line.trim();
        // Format: "testcase( i==N ); /* NAME */"
        let Some(rest) = trimmed.strip_prefix("testcase( i==") else {
            continue;
        };
        let Some(space_pos) = rest.find(' ') else {
            continue;
        };
        let Ok(idx) = rest[..space_pos].parse::<usize>() else {
            continue;
        };
        let Some(cs) = rest.find("/* ") else {
            continue;
        };
        let after = &rest[cs + 3..];
        let Some(ce) = after.find(" */") else {
            continue;
        };
        result.push((idx, after[..ce].to_string()));
    }
    result
}

/// Generate the three parallel arrays (aKWSince, aKWCFlag, aKWCFlagPolarity)
/// as C code, given the keyword index mapping from mkkeywordhash output.
fn generate_keyword_arrays(
    keyword_indices: &[(usize, String)],
    cflag_map: &HashMap<String, (u32, u8)>,
    dialect: &str,
) -> String {
    let max_idx = keyword_indices.iter().map(|(i, _)| *i).max().unwrap_or(0);
    let array_len = max_idx + 1;

    let mut since = vec![0i32; array_len];
    let mut cflag = vec![-1i32; array_len];
    let mut polarity = vec![0u8; array_len];

    for (idx, name) in keyword_indices {
        since[*idx] = keyword_since_version(name);
        if let Some(&(cflag_idx, pol)) = cflag_map.get(name) {
            cflag[*idx] = cflag_idx.cast_signed();
            polarity[*idx] = pol;
        }
    }

    let prefix = format!("synq_{dialect}");

    let fmt_array = |values: &[String], c_type: &str, name: &str| -> String {
        let mut s = format!("static const {c_type} {prefix}_{name}[{array_len}] = {{");
        for (i, v) in values.iter().enumerate() {
            if i % 13 == 0 {
                s.push_str("\n  ");
            }
            s.push_str(v);
            if i + 1 < values.len() {
                s.push(',');
            }
        }
        s.push_str("\n};\n");
        s
    };

    let since_strs: Vec<String> = since.iter().map(ToString::to_string).collect();
    let cflag_strs: Vec<String> = cflag.iter().map(ToString::to_string).collect();
    let pol_strs: Vec<String> = polarity.iter().map(ToString::to_string).collect();

    let mut out = String::new();
    out.push_str(&fmt_array(&since_strs, "int32_t", "aKWSince"));
    out.push_str(&fmt_array(&cflag_strs, "int8_t", "aKWCFlag"));
    out.push_str(&fmt_array(&pol_strs, "uint8_t", "aKWCFlagPolarity"));
    out
}

/// The version+cflag check code that replaces `*pType = aKWCode[i]; break;`
/// in `synq_sqlite3_keywordCode`. Uses `__SYNQ_DIALECT__` placeholder which
/// the caller replaces with the actual dialect prefix (e.g. `synq_perfetto`).
const fn keyword_check_code() -> &'static str {
    r"/* Version check: skip keywords newer than target version. */
    if( __SYNQ_DIALECT___aKWSince[i] != 0 && SYNQ_VER_LT(env, __SYNQ_DIALECT___aKWSince[i]) ){
      break;
    }
    /* CFlag check with polarity. */
    if( __SYNQ_DIALECT___aKWCFlag[i] >= 0 ){
      int flag_set = SYNQ_HAS_CFLAG(env, __SYNQ_DIALECT___aKWCFlag[i]);
      int is_enable = __SYNQ_DIALECT___aKWCFlagPolarity[i];
      if( flag_set != is_enable ){
        break;
      }
    }
    *pType = aKWCode[i];
    break;"
}

/// Generate keyword hash lookup as a single `.c` file.
///
/// Uses pre-extracted cflag data from `fragments.keyword_cflags` instead of
/// parsing `mkkeywordhash.c` at codegen time.
pub(crate) fn generate(
    extract_result: &TokenizerExtractResult,
    fragments: &SqliteFragments,
    dialect: &str,
    extra_keywords: &[String],
    includes: &crate::dialect_codegen::c_dialect::DialectCIncludes<'_>,
) -> Result<String, String> {
    let cflag_map = parse_keyword_cflags(fragments.parser_cflags);

    let mut cmd = crate::util::self_subcommand::self_subcommand("mkkeyword")?;

    let _kw_file = if extra_keywords.is_empty() {
        None
    } else {
        let f = tempfile::NamedTempFile::new()
            .map_err(|e| format!("Failed to create keyword temp file: {e}"))?;
        fs::write(f.path(), extra_keywords.join("\n"))
            .map_err(|e| format!("Failed to write keyword file: {e}"))?;
        cmd.arg("--extra-file").arg(f.path());
        Some(f)
    };

    let output = cmd
        .output()
        .map_err(|e| format!("Failed to spawn mkkeyword subprocess: {e}"))?;

    if !output.status.success() {
        let stderr = String::from_utf8_lossy(&output.stderr);
        return Err(format!(
            "mkkeyword failed with exit code: {}\n{}",
            output.status, stderr
        ));
    }

    let generated_code = String::from_utf8(output.stdout)
        .map_err(|e| format!("Invalid UTF-8 in mkkeyword output: {e}"))?;

    // Parse keyword indices from testcase comments before any transformations.
    let keyword_indices = parse_keyword_indices(&generated_code);
    let keyword_arrays = generate_keyword_arrays(&keyword_indices, &cflag_map, dialect);

    let kw_text_sym = format!("synq_{dialect}_zKWText");
    let kw_offset_sym = format!("synq_{dialect}_aKWOffset");
    let kw_len_sym = format!("synq_{dialect}_aKWLen");
    let kw_code_sym = format!("synq_{dialect}_aKWCode");
    let kw_count_sym = format!("synq_{dialect}_nKeyword");

    let processed_code = CTransformer::new(&generated_code)
        .remove_function("sqlite3KeywordCode")
        .remove_function("sqlite3_keyword_name")
        .remove_function("sqlite3_keyword_count")
        .remove_function("sqlite3_keyword_check")
        .remove_lines_matching("#define SQLITE_N_KEYWORD")
        .rename_function("keywordCode", "synq_sqlite3_keywordCode")
        .remove_static_first("synq_sqlite3_keywordCode")
        // Add config parameter to keywordCode signature.
        .replace_in_function(
            "synq_sqlite3_keywordCode",
            "synq_sqlite3_keywordCode(const char *z",
            "synq_sqlite3_keywordCode(const SyntaqliteGrammar *env, const char *z",
        )
        // Replace the simple assignment with version/cflag checks.
        .replace_in_function(
            "synq_sqlite3_keywordCode",
            "*pType = aKWCode[i];\n    break;",
            keyword_check_code(),
        )
        .remove_static_first("zKWText")
        .remove_static_first("aKWOffset")
        .remove_static_first("aKWLen")
        .remove_static_first("aKWCode")
        .replace_all("zKWText", &kw_text_sym)
        .replace_all("aKWOffset", &kw_offset_sym)
        .replace_all("aKWLen", &kw_len_sym)
        .replace_all("aKWCode", &kw_code_sym)
        .replace_all("TK_", "SYNTAQLITE_TK_")
        .replace_all("__SYNQ_DIALECT__", &format!("synq_{dialect}"))
        .finish();

    // Insert keyword arrays before the keywordCode function.
    let processed_code = {
        let fn_marker = "int synq_sqlite3_keywordCode(";
        match processed_code.find(fn_marker) {
            Some(pos) => {
                let mut result = processed_code[..pos].to_string();
                result.push_str(&keyword_arrays);
                result.push('\n');
                result.push_str(&processed_code[pos..]);
                result
            }
            None => processed_code,
        }
    };

    let mut w = CWriter::new();
    w.sqlite_file_header();
    w.include_local("syntaqlite_dialect/sqlite_compat.h");
    w.include_local(includes.tokens_header);
    w.include_local("syntaqlite/grammar.h");
    w.include_local("syntaqlite_dialect/dialect_macros.h");
    w.newline();

    w.fragment(&extract_result.upper_to_lower);
    w.newline();
    w.fragment(&extract_result.char_map);
    w.newline();

    w.fragment(&processed_code);
    w.newline();
    w.line(&format!(
        "const unsigned int {kw_count_sym} = sizeof({kw_code_sym}) / sizeof({kw_code_sym}[0]);"
    ));
    w.newline();

    Ok(w.finish())
}

/// Generate the `sqlite_keyword.h` header.
pub(crate) fn generate_keyword_h() -> String {
    let mut w = CWriter::new();
    w.sqlite_file_header();
    w.header_guard_start("SYNTAQLITE_SQLITE_KEYWORD_H");
    w.newline();
    w.line("#include \"syntaqlite/grammar.h\"");
    w.newline();
    w.line("int synq_sqlite3_keywordCode(const SyntaqliteGrammar *env, const char* z, int n, int* pType);");
    w.newline();
    w.header_guard_end("SYNTAQLITE_SQLITE_KEYWORD_H");
    w.finish()
}
