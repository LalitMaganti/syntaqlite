// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Stage 3 keyword hash generation: reads pre-extracted cflag data and calls
//! the mkkeyword subprocess to produce the final `keyword.c`.

use std::collections::HashMap;
use std::fmt::Write as _;
use std::fs;

use super::sqlite_fragments::SqliteFragments;
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

/// Uppercased bytes of a keyword or identifier, as the lookup compares them.
fn kw_upper(b: u8) -> u64 {
    u64::from(b & !0x20)
}

/// The lookup key: first two bytes, last two bytes and the length. Keywords
/// sharing all of these (e.g. `CURRENT_DATE` and `CURRENT_TIME`) are told
/// apart by the second-to-last byte.
fn kw_key(name: &[u8]) -> u64 {
    let n = name.len();
    kw_upper(name[0])
        | kw_upper(name[1]) << 8
        | kw_upper(name[n - 2]) << 16
        | kw_upper(name[n - 1]) << 24
        | (n as u64) << 32
}

/// A collision-free multiplicative hash of [`kw_key`] into a power-of-two
/// slot table, found by a deterministic search.
struct KeywordPerfectHash {
    bits: u32,
    mult: u64,
    /// `slots[h]` is the 1-based keyword index hashing to `h`, or 0.
    slots: Vec<usize>,
    /// `keys[i]` is the [`kw_key`] of keyword `i`; 0 for the unused index 0.
    keys: Vec<u64>,
}

fn find_keyword_perfect_hash(
    keyword_indices: &[(usize, String)],
) -> Result<KeywordPerfectHash, String> {
    let mut by_key: HashMap<u64, &str> = HashMap::new();
    for (_, name) in keyword_indices {
        if let Some(other) = by_key.insert(kw_key(name.as_bytes()), name) {
            return Err(format!(
                "keywords `{other}` and `{name}` share a lookup key (length and first and \
                 last two bytes), so no hash can tell them apart; widen `kw_key`"
            ));
        }
    }
    let keys: Vec<(usize, u64)> = keyword_indices
        .iter()
        .map(|(i, name)| (*i, kw_key(name.as_bytes())))
        .collect();
    // Start with at least four slots per key, so a collision-free multiplier
    // turns up within a few hundred thousand tries.
    let min_bits = (keys.len() * 4).next_power_of_two().trailing_zeros().max(8);
    for bits in min_bits..min_bits + 3 {
        // `taken[h] == attempt` marks slot `h` as used in the current attempt,
        // so the table is not cleared between attempts.
        let mut taken = vec![0u32; 1 << bits];
        let mut x: u64 = 0x9E37_79B9;
        for attempt in 1..=2_000_000u32 {
            x = x
                .wrapping_mul(6_364_136_223_846_793_005)
                .wrapping_add(1_442_695_040_888_963_407);
            let mult = x | 1;
            let slot =
                |key: u64| usize::try_from(key.wrapping_mul(mult) >> (64 - bits)).unwrap_or(0);
            let collision_free = keys.iter().all(|&(_, key)| {
                let h = slot(key);
                let free = taken[h] != attempt;
                taken[h] = attempt;
                free
            });
            if collision_free {
                let mut slots = vec![0usize; 1 << bits];
                let max_idx = keys.iter().map(|&(i, _)| i).max().unwrap_or(0);
                let mut by_idx = vec![0u64; max_idx + 1];
                for &(i, key) in &keys {
                    slots[slot(key)] = i;
                    by_idx[i] = key;
                }
                return Ok(KeywordPerfectHash {
                    bits,
                    mult,
                    slots,
                    keys: by_idx,
                });
            }
        }
    }
    Err("no collision-free keyword hash found".to_string())
}

/// The slot table and `synq_sqlite3_keywordCode`, which looks a keyword up
/// with one probe of the perfect hash and then applies the version and cflag
/// checks. New arrays use the `__SYNQ_DIALECT__` placeholder for the dialect
/// prefix; mkkeywordhash's arrays keep their bare names and are renamed later.
fn keyword_lookup_code(hash: &KeywordPerfectHash) -> String {
    let slot_type = if hash.slots.iter().all(|&i| i <= 255) {
        "uint8_t"
    } else {
        "uint16_t"
    };
    let mut out = format!(
        "/* __SYNQ_DIALECT___aKWSlot[h] is the 1-based index of the keyword whose\n\
         ** key hashes to h, or 0. */\n\
         static const {slot_type} __SYNQ_DIALECT___aKWSlot[{}] = {{",
        hash.slots.len()
    );
    for (i, v) in hash.slots.iter().enumerate() {
        if i % 16 == 0 {
            out.push_str("\n  ");
        }
        let _ = write!(out, "{v},");
    }
    out.push_str("\n};\n\n");
    out.push_str(
        "/* __SYNQ_DIALECT___aKWKey[i] is the lookup key of keyword i: its first\n\
         ** two and last two bytes, uppercased, and its length. */\n",
    );
    let _ = write!(
        out,
        "static const uint64_t __SYNQ_DIALECT___aKWKey[{}] = {{",
        hash.keys.len()
    );
    for (i, k) in hash.keys.iter().enumerate() {
        if i % 4 == 0 {
            out.push_str("\n  ");
        }
        let _ = write!(out, "0x{k:x}ull,");
    }
    out.push_str("\n};\n\n");
    let _ = write!(
        out,
        r"int synq_sqlite3_keywordCode(const SyntaqliteDialect *env, const char *z, int n, int *pType){{
  const unsigned char *u = (const unsigned char *)z;
  uint64_t key;
  const char *zKW;
  int i, j;
  assert( n>=2 );
  key = ((uint64_t)u[0] | (uint64_t)u[1]<<8 | (uint64_t)u[n-2]<<16
         | (uint64_t)u[n-1]<<24) & 0xDFDFDFDFu;
  key |= (uint64_t)n<<32;
  i = __SYNQ_DIALECT___aKWSlot[(key*0x{mult:016x}ull) >> {shift}];
  /* The key covers the length and the outer two bytes at each end, so only
  ** the middle is left to compare. Empty slot 0 has key 0, which never
  ** matches. */
  if( __SYNQ_DIALECT___aKWKey[i]!=key ) return n;
  zKW = &zKWText[aKWOffset[i]];
  for(j=2; j<n-2; j++){{
    if( (z[j]&~0x20)!=zKW[j] ) return n;
  }}
  /* Version check: skip keywords newer than target version. */
  if( __SYNQ_DIALECT___aKWSince[i] != 0 && SYNQ_VER_LT(env, __SYNQ_DIALECT___aKWSince[i]) ){{
    return n;
  }}
  /* CFlag check with polarity. */
  if( __SYNQ_DIALECT___aKWCFlag[i] >= 0 ){{
    int flag_set = SYNQ_HAS_CFLAG(env, __SYNQ_DIALECT___aKWCFlag[i]);
    int is_enable = __SYNQ_DIALECT___aKWCFlagPolarity[i];
    if( flag_set != is_enable ) return n;
  }}
  *pType = aKWCode[i];
  return n;
}}
",
        mult = hash.mult,
        shift = 64 - hash.bits,
    );
    out
}

/// Generate keyword hash lookup as a single `.c` file.
///
/// Uses pre-extracted cflag data from `fragments.keyword_cflags` instead of
/// parsing `mkkeywordhash.c` at codegen time.
pub(crate) fn generate(
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

    let perfect_hash = find_keyword_perfect_hash(&keyword_indices)
        .map_err(|e| format!("Failed to hash keywords for dialect `{dialect}`: {e}"))?;

    let processed_code = CTransformer::new(&generated_code)
        // The perfect hash replaces mkkeywordhash's chained hash table.
        .remove_array_definition("aKWHash")
        .remove_array_definition("aKWNext")
        .remove_block_comment_containing("Hash table decoded")
        .remove_block_comment_containing("Check to see if z[0..n-1] is a keyword")
        .remove_function("sqlite3KeywordCode")
        .remove_function("sqlite3_keyword_name")
        .remove_function("sqlite3_keyword_count")
        .remove_function("sqlite3_keyword_check")
        .remove_function("keywordCode")
        .remove_lines_matching("#define SQLITE_N_KEYWORD")
        .remove_static_first("zKWText")
        .remove_static_first("aKWOffset")
        .remove_static_first("aKWLen")
        .remove_static_first("aKWCode")
        .append(&keyword_arrays)
        .append("\n")
        .append(&keyword_lookup_code(&perfect_hash))
        .replace_all("zKWText", &kw_text_sym)
        .replace_all("aKWOffset", &kw_offset_sym)
        .replace_all("aKWLen", &kw_len_sym)
        .replace_all("aKWCode", &kw_code_sym)
        .replace_all("TK_", "SYNTAQLITE_TK_")
        .replace_all("__SYNQ_DIALECT__", &format!("synq_{dialect}"))
        .finish()
        .map_err(|e| format!("Failed to process keyword hash for dialect `{dialect}`: {e}"))?;

    let mut w = CWriter::new();
    w.sqlite_file_header();
    w.include_local("syntaqlite_dialect/sqlite_compat.h");
    w.include_local(includes.tokens_header);
    w.include_local("syntaqlite/dialect.h");
    w.include_local("syntaqlite_dialect/dialect_macros.h");
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
    w.line("#include \"syntaqlite/dialect.h\"");
    w.newline();
    w.line("int synq_sqlite3_keywordCode(const SyntaqliteDialect *env, const char* z, int n, int* pType);");
    w.newline();
    w.header_guard_end("SYNTAQLITE_SQLITE_KEYWORD_H");
    w.finish()
}
