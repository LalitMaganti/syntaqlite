// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Stage 3 keyword lookup generation: lays out `SQLite`'s keyword tables the
//! way `mkkeywordhash` does and emits a perfect-hash `keywordCode`.

use std::collections::{HashMap, HashSet};
use std::fmt::Write as _;

use super::sqlite_fragments::SqliteFragments;
use super::sqlite_keywords::SQLITE_KEYWORDS;
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

/// A keyword and the token it maps to, without the `TK_` prefix.
#[derive(Debug, Clone, PartialEq, Eq)]
struct Keyword {
    name: String,
    token: String,
}

/// `SQLite`'s keywords followed by the dialect's extra ones, each mapping to
/// the token of the same name.
fn keyword_list(extra_keywords: &[String]) -> Result<Vec<Keyword>, String> {
    let mut keywords: Vec<Keyword> = SQLITE_KEYWORDS
        .iter()
        .map(|&(name, token)| Keyword {
            name: name.to_string(),
            token: token.to_string(),
        })
        .collect();
    let mut seen: HashSet<String> = keywords.iter().map(|k| k.name.clone()).collect();
    for extra in extra_keywords {
        let name = extra.trim().to_ascii_uppercase();
        if name.is_empty() || !seen.insert(name.clone()) {
            continue;
        }
        keywords.push(Keyword {
            token: name.clone(),
            name,
        });
    }
    for k in &keywords {
        let b = k.name.as_bytes();
        // The lookup compares bytes under `&~0x20`, which only folds letters,
        // and SQLite's hash reads the first and last bytes as letters.
        let valid = b.len() >= 2
            && b.iter().all(|&c| c.is_ascii_uppercase() || c == b'_')
            && b[0].is_ascii_uppercase()
            && b[b.len() - 1].is_ascii_uppercase();
        if !valid {
            return Err(format!(
                "keyword `{}` must be at least two letters or underscores, starting and \
                 ending with a letter",
                k.name
            ));
        }
    }
    Ok(keywords)
}

/// Keyword tables laid out as `mkkeywordhash` lays them out. Per-keyword
/// vectors are indexed from 1, matching the generated arrays; index 0 is
/// unused.
#[derive(Debug, PartialEq, Eq)]
struct KeywordTables {
    /// All keyword text, with keywords overlapping where one's suffix is
    /// another's prefix and keywords inside longer ones sharing their text.
    text: Vec<u8>,
    names: Vec<String>,
    tokens: Vec<String>,
    offsets: Vec<usize>,
}

/// A keyword being laid out by [`pack_keywords`], mirroring `mkkeywordhash`'s
/// `Keyword` struct.
struct PackEntry {
    /// 1-based position in the input keyword list.
    id: usize,
    /// The keyword's text not shared with the keyword laid out before it.
    text: Vec<u8>,
    longest_suffix: usize,
    /// The `id` of a longer keyword containing this one, or 0.
    substr_id: usize,
    substr_offset: usize,
    offset: usize,
}

/// Pack keyword text and order the keywords exactly as `mkkeywordhash` does.
fn pack_keywords(keywords: &[Keyword]) -> KeywordTables {
    let mut entries: Vec<PackEntry> = keywords
        .iter()
        .enumerate()
        .map(|(i, k)| PackEntry {
            id: i + 1,
            text: k.name.as_bytes().to_vec(),
            longest_suffix: 0,
            substr_id: 0,
            substr_offset: 0,
            offset: 0,
        })
        .collect();
    find_substrings(&mut entries);
    find_longest_suffixes(&mut entries);
    let text_len = assign_offsets(&mut entries);

    entries.sort_by(|a, b| a.offset.cmp(&b.offset).then_with(|| b.id.cmp(&a.id)));
    let mut tables = KeywordTables {
        text: Vec::with_capacity(text_len),
        names: vec![String::new()],
        tokens: vec![String::new()],
        offsets: vec![0],
    };
    for entry in &entries {
        if entry.substr_id == 0 {
            tables.text.extend_from_slice(&entry.text);
        }
        let keyword = &keywords[entry.id - 1];
        tables.names.push(keyword.name.clone());
        tables.tokens.push(keyword.token.clone());
        tables.offsets.push(entry.offset);
    }
    tables
}

/// Mark keywords that appear inside a longer keyword, which reuse its text.
fn find_substrings(entries: &mut [PackEntry]) {
    entries.sort_by(|a, b| {
        a.text
            .len()
            .cmp(&b.text.len())
            .then_with(|| a.text.cmp(&b.text))
    });
    let count = entries.len();
    for short in (0..count.saturating_sub(1)).rev() {
        let mut long = count - 1;
        while long > short && entries[short].substr_id == 0 {
            if entries[long].substr_id == 0 && entries[long].text.len() > entries[short].text.len()
            {
                let needle = entries[short].text.as_slice();
                if let Some(at) = entries[long]
                    .text
                    .windows(needle.len())
                    .position(|w| w == needle)
                {
                    entries[short].substr_id = entries[long].id;
                    entries[short].substr_offset = at;
                }
            }
            long -= 1;
        }
    }
}

/// Find the longest suffix of each keyword that is a prefix of another.
fn find_longest_suffixes(entries: &mut [PackEntry]) {
    for i in 0..entries.len() {
        if entries[i].substr_id != 0 {
            continue;
        }
        for j in 0..entries.len() {
            if j == i || entries[j].substr_id != 0 {
                continue;
            }
            let (len, other_len) = (entries[i].text.len(), entries[j].text.len());
            for k in entries[i].longest_suffix + 1..len.min(other_len) {
                if entries[i].text[len - k..] == entries[j].text[..k] {
                    entries[i].longest_suffix = k;
                }
            }
        }
    }
}

/// Lay keywords out longest suffix first, chaining each into the next keyword
/// that starts with its suffix. Returns the length of the packed text.
fn assign_offsets(entries: &mut [PackEntry]) -> usize {
    entries.sort_by(|a, b| {
        b.longest_suffix
            .cmp(&a.longest_suffix)
            .then_with(|| a.text.cmp(&b.text))
    });
    let count = entries.len();
    let mut text_len = 0;
    for start in 0..count {
        if entries[start].offset > 0 || entries[start].substr_id != 0 {
            continue;
        }
        let mut cur = start;
        entries[cur].offset = text_len;
        text_len += entries[cur].text.len();
        let mut overlap = entries[cur].text.len() - 1;
        while overlap >= 1 {
            let mut next = start + 1;
            while next < count {
                let cur_text = &entries[cur].text;
                let fits = entries[next].offset == 0
                    && entries[next].substr_id == 0
                    && entries[next].text.len() > overlap
                    && cur_text[cur_text.len() - overlap..] == entries[next].text[..overlap];
                if fits {
                    // mkkeywordhash restarts the scan from the new keyword.
                    cur = next;
                    entries[cur].offset = text_len - overlap;
                    text_len = entries[cur].offset + entries[cur].text.len();
                    entries[cur].text.drain(..overlap);
                    next = start;
                    overlap = entries[cur].text.len();
                }
                next += 1;
            }
            overlap -= 1;
        }
    }
    let offset_by_id: HashMap<usize, usize> = entries.iter().map(|e| (e.id, e.offset)).collect();
    for entry in entries.iter_mut() {
        if entry.substr_id != 0 {
            entry.offset = offset_by_id[&entry.substr_id] + entry.substr_offset;
        }
    }
    text_len
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
    /// `slots[h]` is the index of the keyword hashing to `h`, or 0.
    slots: Vec<usize>,
    /// `keys[i]` is the [`kw_key`] of keyword `i`; 0 for the unused index 0.
    keys: Vec<u64>,
}

/// Hash `names`, which are indexed from 1 like [`KeywordTables::names`].
fn find_keyword_perfect_hash(names: &[String]) -> Result<KeywordPerfectHash, String> {
    let mut keys = vec![0u64];
    let mut by_key: HashMap<u64, &str> = HashMap::new();
    for name in &names[1..] {
        let key = kw_key(name.as_bytes());
        if let Some(other) = by_key.insert(key, name) {
            return Err(format!(
                "keywords `{other}` and `{name}` share a lookup key (length and first and \
                 last two bytes), so no hash can tell them apart; widen `kw_key`"
            ));
        }
        keys.push(key);
    }
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
            let collision_free = keys[1..].iter().all(|&key| {
                let h = slot(key);
                let free = taken[h] != attempt;
                taken[h] = attempt;
                free
            });
            if collision_free {
                let mut slots = vec![0usize; 1 << bits];
                for (i, &key) in keys.iter().enumerate().skip(1) {
                    slots[slot(key)] = i;
                }
                return Ok(KeywordPerfectHash {
                    bits,
                    mult,
                    slots,
                    keys,
                });
            }
        }
    }
    Err("no collision-free keyword hash found".to_string())
}

/// Append `{decl}[N] = {...};` with `per_line` values a line.
fn c_array(out: &mut String, decl: &str, values: &[String], per_line: usize) {
    let _ = write!(out, "{decl}[{}] = {{", values.len());
    for (i, v) in values.iter().enumerate() {
        if i % per_line == 0 {
            out.push_str("\n  ");
        }
        out.push_str(v);
        out.push(',');
    }
    out.push_str("\n};\n");
}

/// The version and cflag gating arrays, indexed like `tables`.
fn gating_arrays(
    out: &mut String,
    prefix: &str,
    tables: &KeywordTables,
    cflag_map: &HashMap<String, (u32, u8)>,
) {
    let since: Vec<i32> = tables
        .names
        .iter()
        .map(|n| keyword_since_version(n))
        .collect();
    // Few distinct versions gate keywords, so store a one-byte index into a
    // table of them; index 0 is the ungated baseline.
    let mut versions = vec![0i32];
    for &v in &since {
        if !versions.contains(&v) {
            versions.push(v);
        }
    }
    let version_strs: Vec<String> = versions.iter().map(ToString::to_string).collect();
    let since_strs: Vec<String> = since
        .iter()
        .map(|v| {
            versions
                .iter()
                .position(|x| x == v)
                .unwrap_or(0)
                .to_string()
        })
        .collect();
    let cflag = |n: &String| cflag_map.get(n.as_str());
    let cflag_strs: Vec<String> = tables
        .names
        .iter()
        .map(|n| cflag(n).map_or(-1, |&(c, _)| i64::from(c)).to_string())
        .collect();
    let pol_strs: Vec<String> = tables
        .names
        .iter()
        .map(|n| cflag(n).map_or(0, |&(_, p)| p).to_string())
        .collect();
    let _ = writeln!(
        out,
        "static const int32_t {prefix}_aKWSinceVersion[{}] = {{{}}};",
        versions.len(),
        version_strs.join(", ")
    );
    c_array(
        out,
        &format!("static const uint8_t {prefix}_aKWSince"),
        &since_strs,
        25,
    );
    c_array(
        out,
        &format!("static const int8_t {prefix}_aKWCFlag"),
        &cflag_strs,
        19,
    );
    c_array(
        out,
        &format!("static const uint8_t {prefix}_aKWCFlagPolarity"),
        &pol_strs,
        25,
    );
}

/// `synq_sqlite3_keywordCode`, which looks a keyword up with one probe of the
/// perfect hash and then applies the version and cflag checks.
fn keyword_lookup_code(out: &mut String, p: &str, hash: &KeywordPerfectHash) {
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
  i = {p}_aKWSlot[(key*0x{mult:016x}ull) >> {shift}];
  /* The key covers the length and the outer two bytes at each end, so only
  ** the middle is left to compare. Empty slot 0 has key 0, which never
  ** matches. */
  if( {p}_aKWKey[i]!=key ) return n;
  zKW = &{p}_zKWText[{p}_aKWOffset[i]];
  for(j=2; j<n-2; j++){{
    if( (z[j]&~0x20)!=zKW[j] ) return n;
  }}
  /* Version check: skip keywords newer than target version. */
  if( {p}_aKWSince[i] != 0
      && SYNQ_VER_LT(env, {p}_aKWSinceVersion[{p}_aKWSince[i]]) ){{
    return n;
  }}
  /* CFlag check with polarity. */
  if( {p}_aKWCFlag[i] >= 0 ){{
    int flag_set = SYNQ_HAS_CFLAG(env, {p}_aKWCFlag[i]);
    int is_enable = {p}_aKWCFlagPolarity[i];
    if( flag_set != is_enable ) return n;
  }}
  *pType = {p}_aKWCode[i];
  return n;
}}
",
        mult = hash.mult,
        shift = 64 - hash.bits,
    );
}

/// Generate the keyword lookup as a single `.c` file.
pub(crate) fn generate(
    fragments: &SqliteFragments,
    dialect: &str,
    extra_keywords: &[String],
    includes: &crate::dialect_codegen::c_dialect::DialectCIncludes<'_>,
) -> Result<String, String> {
    let keywords = keyword_list(extra_keywords)
        .map_err(|e| format!("Bad keyword for dialect `{dialect}`: {e}"))?;
    let tables = pack_keywords(&keywords);
    let hash = find_keyword_perfect_hash(&tables.names)
        .map_err(|e| format!("Failed to hash keywords for dialect `{dialect}`: {e}"))?;
    let cflag_map = parse_keyword_cflags(fragments.parser_cflags);
    let p = format!("synq_{dialect}");

    let mut out = String::new();
    out.push_str("/* All keyword text; keywords overlap where they can. */\n");
    let text: Vec<String> = tables
        .text
        .iter()
        .map(|&c| format!("'{}'", char::from(c)))
        .collect();
    c_array(&mut out, &format!("const char {p}_zKWText"), &text, 15);
    out.push_str("/* aKWLen[i] is the length (in bytes) of the i-th keyword */\n");
    let lens: Vec<String> = tables.names.iter().map(|n| n.len().to_string()).collect();
    c_array(
        &mut out,
        &format!("const unsigned char {p}_aKWLen"),
        &lens,
        22,
    );
    out.push_str("/* aKWOffset[i] is the index into zKWText[] of the i-th keyword */\n");
    let offsets: Vec<String> = tables.offsets.iter().map(ToString::to_string).collect();
    c_array(
        &mut out,
        &format!("const unsigned short int {p}_aKWOffset"),
        &offsets,
        15,
    );
    out.push_str("/* aKWCode[i] is the parser symbol code for the i-th keyword */\n");
    let codes: Vec<String> = tables
        .tokens
        .iter()
        .map(|t| {
            if t.is_empty() {
                "0".to_string()
            } else {
                format!("SYNTAQLITE_TK_{t}")
            }
        })
        .collect();
    c_array(
        &mut out,
        &format!("const unsigned char {p}_aKWCode"),
        &codes,
        1,
    );
    out.push('\n');
    gating_arrays(&mut out, &p, &tables, &cflag_map);
    out.push('\n');

    let slot_type = if hash.slots.iter().all(|&i| i <= 255) {
        "uint8_t"
    } else {
        "uint16_t"
    };
    out.push_str("/* aKWSlot[h] is the index of the keyword whose key hashes to h, or 0. */\n");
    let slots: Vec<String> = hash.slots.iter().map(ToString::to_string).collect();
    c_array(
        &mut out,
        &format!("static const {slot_type} {p}_aKWSlot"),
        &slots,
        16,
    );
    out.push_str(
        "/* aKWKey[i] is the lookup key of keyword i: its first two and last two\n\
         ** bytes, uppercased, and its length. */\n",
    );
    let keys: Vec<String> = hash.keys.iter().map(|k| format!("0x{k:x}ull")).collect();
    c_array(
        &mut out,
        &format!("static const uint64_t {p}_aKWKey"),
        &keys,
        4,
    );
    out.push('\n');
    keyword_lookup_code(&mut out, &p, &hash);

    let mut w = CWriter::new();
    w.sqlite_file_header();
    w.include_local("syntaqlite_dialect/sqlite_compat.h");
    w.include_local(includes.tokens_header);
    w.include_local("syntaqlite/dialect.h");
    w.include_local("syntaqlite_dialect/dialect_macros.h");
    w.newline();
    w.fragment(&out);
    w.newline();
    w.line(&format!(
        "const unsigned int {p}_nKeyword = sizeof({p}_aKWCode) / sizeof({p}_aKWCode[0]);"
    ));
    w.newline();
    Ok(w.finish())
}

/// Return the token names (without `TK_`) of `SQLite`'s keywords.
pub(crate) fn base_keyword_token_names() -> HashSet<String> {
    SQLITE_KEYWORDS
        .iter()
        .map(|&(_, token)| token.to_string())
        .collect()
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

#[cfg(test)]
mod tests {
    use super::*;

    fn extras(names: &[&str]) -> Vec<String> {
        names.iter().map(ToString::to_string).collect()
    }

    #[test]
    fn packed_text_holds_every_keyword() {
        let keywords = keyword_list(&extras(&["MACRO", "INCLUDE"])).unwrap();
        let tables = pack_keywords(&keywords);
        assert_eq!(tables.names.len(), keywords.len() + 1);
        for (i, name) in tables.names.iter().enumerate().skip(1) {
            let at = &tables.text[tables.offsets[i]..tables.offsets[i] + name.len()];
            assert_eq!(at, name.as_bytes(), "{name}");
        }
        let total: usize = keywords.iter().map(|k| k.name.len()).sum();
        assert!(tables.text.len() < total);
    }

    #[test]
    fn sqlite_text_packs_like_mkkeywordhash() {
        // mkkeywordhash packs SQLite's 148 keywords into 670 bytes starting
        // with REINDEXEDESCAPEACHECKEY.
        let tables = pack_keywords(&keyword_list(&[]).unwrap());
        assert_eq!(tables.text.len(), 670);
        assert!(tables.text.starts_with(b"REINDEXEDESCAPEACHECKEY"));
        assert_eq!(&tables.names[1..4], ["REINDEX", "INDEXED", "INDEX"]);
    }

    #[test]
    fn tokens_follow_keywords() {
        let tables = pack_keywords(&keyword_list(&extras(&["macro"])).unwrap());
        let token = |name: &str| {
            let i = tables.names.iter().position(|n| n == name).unwrap();
            tables.tokens[i].as_str()
        };
        assert_eq!(token("CROSS"), "JOIN_KW");
        assert_eq!(token("SELECT"), "SELECT");
        assert_eq!(token("MACRO"), "MACRO");
    }

    #[test]
    fn extra_keywords_are_deduplicated() {
        let base = keyword_list(&[]).unwrap().len();
        let keywords = keyword_list(&extras(&["select", "MACRO", " macro "])).unwrap();
        assert_eq!(keywords.len(), base + 1);
    }

    #[test]
    fn extra_keywords_must_be_letters() {
        for bad in ["A", "MACRO2", "_MACRO", "MACRO_", "MA-CRO"] {
            assert!(keyword_list(&extras(&[bad])).is_err(), "{bad}");
        }
        assert!(keyword_list(&extras(&["MY_MACRO"])).is_ok());
    }

    #[test]
    fn perfect_hash_places_every_keyword() {
        let tables = pack_keywords(&keyword_list(&extras(&["MACRO"])).unwrap());
        let hash = find_keyword_perfect_hash(&tables.names).unwrap();
        let mut placed: Vec<usize> = hash.slots.iter().copied().filter(|&i| i != 0).collect();
        placed.sort_unstable();
        assert_eq!(placed, (1..tables.names.len()).collect::<Vec<_>>());
        for (i, name) in tables.names.iter().enumerate().skip(1) {
            let h = usize::try_from(
                kw_key(name.as_bytes()).wrapping_mul(hash.mult) >> (64 - hash.bits),
            )
            .unwrap();
            assert_eq!(hash.slots[h], i, "{name}");
        }
    }
}
