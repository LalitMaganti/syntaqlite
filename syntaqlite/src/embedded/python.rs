// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Python f-string SQL extraction.

use super::{
    EmbeddedFragment, HOLE_PLACEHOLDER, Hole, doc_offset_from_usize, doc_range_from_usize,
    starts_with_sql_keyword,
};

/// Extract SQL fragments from Python source code.
///
/// **Experimental:** this function is part of the experimental embedded SQL API.
///
/// Scans for f-strings (`f"..."`, `f'...'`, `f"""..."""`, `f'''...'''`) and
/// checks if their content starts with a SQL keyword. For qualifying strings,
/// interpolation holes (`{expr}`) are replaced with [`HOLE_PLACEHOLDER`](super::HOLE_PLACEHOLDER).
///
/// Non-f-strings, regular strings, and comments are skipped.
///
/// # Example
///
/// ```
/// use syntaqlite::embedded::extract_python;
///
/// let source = r#"
/// name = "world"
/// db.execute(f"SELECT id FROM users WHERE name = {name}")
/// "#;
/// let fragments = extract_python(source);
/// assert_eq!(fragments.len(), 1);
/// assert!(fragments[0].sql_text().starts_with("SELECT"));
/// assert_eq!(fragments[0].holes().len(), 1);
/// ```
pub fn extract_python(source: &str) -> Vec<EmbeddedFragment> {
    let bytes = source.as_bytes();
    let len = bytes.len();
    let mut fragments = Vec::new();
    let mut i = 0;

    while i < len {
        let Some((quote_char, quote_len, content_start)) = detect_fstring_prefix(bytes, i) else {
            i += 1;
            continue;
        };

        let Some((content_end, string_end)) =
            find_string_end(bytes, content_start, quote_char, quote_len)
        else {
            i = content_start;
            continue;
        };

        let content = &source[content_start..content_end];
        if starts_with_sql_keyword(content)
            && let Some(fragment) = extract_fstring_fragment(source, content_start, content_end)
        {
            fragments.push(fragment);
        }

        i = string_end;
    }

    fragments
}

/// Detect an f-string prefix at position `i`. Returns `(quote_char, quote_len, content_start)`.
/// `quote_len` is 1 for single-quoted, 3 for triple-quoted.
fn detect_fstring_prefix(bytes: &[u8], i: usize) -> Option<(u8, usize, usize)> {
    let len = bytes.len();
    let (has_f, prefix_len) = if i + 1 < len && (bytes[i] == b'f' || bytes[i] == b'F') {
        if bytes[i + 1] == b'"' || bytes[i + 1] == b'\'' {
            (true, 1)
        } else if i + 2 < len
            && (bytes[i + 1] == b'r' || bytes[i + 1] == b'R')
            && (bytes[i + 2] == b'"' || bytes[i + 2] == b'\'')
        {
            (true, 2)
        } else {
            (false, 0)
        }
    } else if i + 2 < len
        && (bytes[i] == b'r' || bytes[i] == b'R')
        && (bytes[i + 1] == b'f' || bytes[i + 1] == b'F')
        && (bytes[i + 2] == b'"' || bytes[i + 2] == b'\'')
    {
        (true, 2)
    } else {
        (false, 0)
    };

    if !has_f {
        return None;
    }

    let quote_pos = i + prefix_len;
    let quote_char = bytes[quote_pos];

    if quote_pos + 2 < len
        && bytes[quote_pos + 1] == quote_char
        && bytes[quote_pos + 2] == quote_char
    {
        Some((quote_char, 3, quote_pos + 3))
    } else {
        Some((quote_char, 1, quote_pos + 1))
    }
}

/// Find the end of a string literal. Returns `(content_end, string_end)`.
fn find_string_end(
    bytes: &[u8],
    start: usize,
    quote_char: u8,
    quote_len: usize,
) -> Option<(usize, usize)> {
    let len = bytes.len();
    let mut j = start;
    while j < len {
        if bytes[j] == b'\\' {
            j += 2;
            continue;
        }
        if bytes[j] == quote_char {
            if quote_len == 3 {
                if j + 2 < len && bytes[j + 1] == quote_char && bytes[j + 2] == quote_char {
                    return Some((j, j + 3));
                }
            } else {
                return Some((j, j + 1));
            }
        }
        if quote_len == 1 && bytes[j] == b'\n' {
            return None;
        }
        j += 1;
    }
    None
}

/// Extract a single f-string fragment, processing `{expr}` holes.
fn extract_fstring_fragment(
    source: &str,
    content_start: usize,
    content_end: usize,
) -> Option<EmbeddedFragment> {
    let bytes = source.as_bytes();
    let mut sql_text = String::new();
    let mut holes = Vec::new();
    let mut j = content_start;

    while j < content_end {
        if bytes[j] == b'{' {
            if j + 1 < content_end && bytes[j + 1] == b'{' {
                sql_text.push('{');
                j += 2;
                continue;
            }

            let hole_start = j;
            let hole_content_end = find_matching_brace(bytes, j + 1, content_end)?;
            let hole_end = hole_content_end + 1;

            let sql_offset = doc_offset_from_usize(sql_text.len());
            sql_text.push_str(HOLE_PLACEHOLDER);

            holes.push(Hole {
                host_range: doc_range_from_usize(hole_start, hole_end),
                sql_offset,
            });
            j = hole_end;
        } else if bytes[j] == b'}' && j + 1 < content_end && bytes[j + 1] == b'}' {
            sql_text.push('}');
            j += 2;
        } else {
            sql_text.push(bytes[j] as char);
            j += 1;
        }
    }

    Some(EmbeddedFragment {
        sql_range: doc_range_from_usize(content_start, content_end),
        sql_text,
        holes,
    })
}

/// Find the matching `}` for an f-string interpolation hole.
fn find_matching_brace(bytes: &[u8], start: usize, end: usize) -> Option<usize> {
    let mut depth = 1u32;
    let mut j = start;

    while j < end {
        match bytes[j] {
            b'{' => {
                depth += 1;
                j += 1;
            }
            b'}' => {
                depth -= 1;
                if depth == 0 {
                    return Some(j);
                }
                j += 1;
            }
            b'\'' | b'"' => {
                j = skip_python_string(bytes, j, end);
            }
            b'#' => {
                while j < end && bytes[j] != b'\n' {
                    j += 1;
                }
            }
            _ => j += 1,
        }
    }
    None
}

/// Skip past a Python string literal starting at `pos`.
fn skip_python_string(bytes: &[u8], pos: usize, end: usize) -> usize {
    let quote = bytes[pos];
    let mut j = pos + 1;

    // Triple-quoted string.
    if j + 1 < end && bytes[j] == quote && bytes[j + 1] == quote {
        j += 2;
        while j < end {
            if bytes[j] == b'\\' {
                j += 2;
                continue;
            }
            if j + 2 < end && bytes[j] == quote && bytes[j + 1] == quote && bytes[j + 2] == quote {
                return j + 3;
            }
            j += 1;
        }
        return j;
    }

    // Single-line string.
    super::skip_single_line_string(bytes, pos, end)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn extract_simple_fstring() {
        let source = r#"query = f"SELECT * FROM users WHERE id = {uid}""#;
        let fragments = extract_python(source);
        assert_eq!(fragments.len(), 1);

        let f = &fragments[0];
        assert_eq!(f.holes.len(), 1);
        assert!(f.sql_text.contains(HOLE_PLACEHOLDER));
        assert!(f.sql_text.starts_with("SELECT * FROM users WHERE id = "));
    }

    #[test]
    fn extract_escaped_braces() {
        let source = r#"s = f"SELECT '{{literal}}' FROM t""#;
        let fragments = extract_python(source);
        assert_eq!(fragments.len(), 1);
        assert_eq!(fragments[0].holes.len(), 0);
        assert!(fragments[0].sql_text.contains("{literal}"));
    }

    #[test]
    fn extract_multiple_holes() {
        let source = r#"q = f"SELECT {cols} FROM {table} WHERE {col} = {val}""#;
        let fragments = extract_python(source);
        assert_eq!(fragments.len(), 1);
        assert_eq!(fragments[0].holes.len(), 4);
    }

    #[test]
    fn skip_non_sql_fstring() {
        let source = r#"msg = f"Hello {name}, welcome!""#;
        let fragments = extract_python(source);
        assert_eq!(fragments.len(), 0);
    }

    #[test]
    fn extract_triple_quoted() {
        let source = "q = f\"\"\"\nSELECT *\nFROM users\nWHERE id = {uid}\n\"\"\"";
        let fragments = extract_python(source);
        assert_eq!(fragments.len(), 1);
        assert_eq!(fragments[0].holes.len(), 1);
    }

    #[test]
    fn extract_rf_prefix() {
        let source = r#"q = rf"SELECT * FROM users WHERE id = {uid}""#;
        let fragments = extract_python(source);
        assert_eq!(fragments.len(), 1);
    }

    #[test]
    fn extract_fr_prefix() {
        let source = r#"q = fr"SELECT * FROM users WHERE id = {uid}""#;
        let fragments = extract_python(source);
        assert_eq!(fragments.len(), 1);
    }
}
