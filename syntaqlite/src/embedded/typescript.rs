// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! TypeScript/JavaScript template literal SQL extraction.

use super::{
    EmbeddedFragment, HOLE_PLACEHOLDER, Hole, doc_offset_from_usize, doc_range_from_usize,
    starts_with_sql_keyword,
};

/// Extract SQL fragments from TypeScript/JavaScript source code.
///
/// **Experimental:** this function is part of the experimental embedded SQL API.
///
/// Scans for template literals (`` `...` ``) and checks if their content starts
/// with a SQL keyword. For qualifying strings, interpolation holes (`${expr}`)
/// are replaced with [`HOLE_PLACEHOLDER`](super::HOLE_PLACEHOLDER).
///
/// Regular strings (`"..."`, `'...'`) and comments are skipped.
///
/// # Example
///
/// ```
/// use syntaqlite::embedded::extract_typescript;
///
/// let source = r#"
/// const uid = 42;
/// db.prepare(`SELECT name FROM users WHERE id = ${uid}`).all();
/// "#;
/// let fragments = extract_typescript(source);
/// assert_eq!(fragments.len(), 1);
/// assert!(fragments[0].sql_text().starts_with("SELECT"));
/// assert_eq!(fragments[0].holes().len(), 1);
/// ```
pub fn extract_typescript(source: &str) -> Vec<EmbeddedFragment> {
    let bytes = source.as_bytes();
    let len = bytes.len();
    let mut fragments = Vec::new();
    let mut i = 0;

    while i < len {
        match bytes[i] {
            b'/' => {
                i = skip_js_comment(bytes, i);
            }
            b'"' | b'\'' => {
                i = super::skip_single_line_string(bytes, i, len);
            }
            b'`' => {
                let content_start = i + 1;
                let Some((content_end, literal_end)) =
                    find_template_literal_end(bytes, content_start)
                else {
                    i = content_start;
                    continue;
                };

                let content = &source[content_start..content_end];
                if starts_with_sql_keyword(content)
                    && let Some(fragment) =
                        extract_template_fragment(source, content_start, content_end)
                {
                    fragments.push(fragment);
                }

                i = literal_end;
            }
            _ => i += 1,
        }
    }

    fragments
}

/// Find the end of a template literal. Returns `(content_end, literal_end)`.
/// Handles nested template literals inside `${expr}` holes.
fn find_template_literal_end(bytes: &[u8], start: usize) -> Option<(usize, usize)> {
    let len = bytes.len();
    let mut j = start;

    while j < len {
        match bytes[j] {
            b'\\' => {
                j += 2;
            }
            b'`' => {
                return Some((j, j + 1));
            }
            b'$' if j + 1 < len && bytes[j + 1] == b'{' => {
                // Skip over the ${expr} hole — we don't need to parse it for
                // finding the end, but we must handle nested template literals.
                let brace_content = j + 2;
                let close = find_matching_brace_js(bytes, brace_content, len)?;
                j = close + 1;
            }
            _ => j += 1,
        }
    }
    None
}

/// Extract a single template literal fragment, processing `${expr}` holes.
fn extract_template_fragment(
    source: &str,
    content_start: usize,
    content_end: usize,
) -> Option<EmbeddedFragment> {
    let bytes = source.as_bytes();
    let mut sql_text = String::new();
    let mut holes = Vec::new();
    let mut j = content_start;

    while j < content_end {
        if bytes[j] == b'\\' && j + 1 < content_end {
            // Escaped character — emit the escaped char literally.
            sql_text.push(bytes[j + 1] as char);
            j += 2;
        } else if bytes[j] == b'$' && j + 1 < content_end && bytes[j + 1] == b'{' {
            let hole_start = j;
            let brace_content = j + 2;
            let hole_content_end = find_matching_brace_js(bytes, brace_content, content_end)?;
            let hole_end = hole_content_end + 1;

            let sql_offset = doc_offset_from_usize(sql_text.len());
            sql_text.push_str(HOLE_PLACEHOLDER);

            holes.push(Hole {
                host_range: doc_range_from_usize(hole_start, hole_end),
                sql_offset,
            });
            j = hole_end;
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

/// Find the matching `}` for a JS `${expr}` hole.
/// Handles nested braces, strings (`"`, `'`, `` ` ``), and comments (`//`, `/* */`).
fn find_matching_brace_js(bytes: &[u8], start: usize, end: usize) -> Option<usize> {
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
            b'"' | b'\'' => {
                j = super::skip_single_line_string(bytes, j, end);
            }
            b'`' => {
                // Nested template literal inside a hole expression.
                let inner_start = j + 1;
                let (_, inner_end) = find_template_literal_end(bytes, inner_start)?;
                j = inner_end;
            }
            b'/' => {
                j = skip_js_comment(bytes, j);
            }
            _ => j += 1,
        }
    }
    None
}

/// Skip a JS comment (`// ...` or `/* ... */`) starting at `pos`.
/// If not a comment, advances by 1.
fn skip_js_comment(bytes: &[u8], pos: usize) -> usize {
    let len = bytes.len();
    if pos + 1 < len {
        if bytes[pos + 1] == b'/' {
            // Line comment — skip to end of line.
            let mut j = pos + 2;
            while j < len && bytes[j] != b'\n' {
                j += 1;
            }
            return j;
        }
        if bytes[pos + 1] == b'*' {
            // Block comment — skip to `*/`.
            let mut j = pos + 2;
            while j + 1 < len {
                if bytes[j] == b'*' && bytes[j + 1] == b'/' {
                    return j + 2;
                }
                j += 1;
            }
            return len;
        }
    }
    pos + 1
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn ts_simple_template_literal() {
        let source = r"const q = `SELECT * FROM users`;";
        let fragments = extract_typescript(source);
        assert_eq!(fragments.len(), 1);
        assert_eq!(fragments[0].holes.len(), 0);
        assert_eq!(fragments[0].sql_text, "SELECT * FROM users");
    }

    #[test]
    fn ts_template_with_holes() {
        let source = r"const q = `SELECT * FROM users WHERE id = ${userId}`;";
        let fragments = extract_typescript(source);
        assert_eq!(fragments.len(), 1);
        assert_eq!(fragments[0].holes.len(), 1);
        assert!(fragments[0].sql_text.contains(HOLE_PLACEHOLDER));
        assert!(
            fragments[0]
                .sql_text
                .starts_with("SELECT * FROM users WHERE id = ")
        );
    }

    #[test]
    fn ts_multiple_holes() {
        let source = r"const q = `SELECT ${cols} FROM ${table} WHERE ${col} = ${val}`;";
        let fragments = extract_typescript(source);
        assert_eq!(fragments.len(), 1);
        assert_eq!(fragments[0].holes.len(), 4);
    }

    #[test]
    fn ts_skip_non_sql() {
        let source = r"const msg = `Hello ${name}, welcome!`;";
        let fragments = extract_typescript(source);
        assert_eq!(fragments.len(), 0);
    }

    #[test]
    fn ts_multiline() {
        let source = "const q = `\n  SELECT *\n  FROM users\n  WHERE id = ${uid}\n`;";
        let fragments = extract_typescript(source);
        assert_eq!(fragments.len(), 1);
        assert_eq!(fragments[0].holes.len(), 1);
    }

    #[test]
    fn ts_skip_in_line_comment() {
        let source = "// const q = `SELECT * FROM users`;\nconst x = 1;";
        let fragments = extract_typescript(source);
        assert_eq!(fragments.len(), 0);
    }

    #[test]
    fn ts_skip_in_block_comment() {
        let source = "/* const q = `SELECT * FROM users`; */\nconst x = 1;";
        let fragments = extract_typescript(source);
        assert_eq!(fragments.len(), 0);
    }

    #[test]
    fn ts_skip_in_string() {
        let source = r#"const s = "const q = `SELECT * FROM users`";"#;
        let fragments = extract_typescript(source);
        assert_eq!(fragments.len(), 0);
    }

    #[test]
    fn ts_multiple_fragments() {
        let source = r"
const q1 = `SELECT * FROM users`;
const msg = `Hello world`;
const q2 = `INSERT INTO logs (msg) VALUES (${m})`;
";
        let fragments = extract_typescript(source);
        assert_eq!(fragments.len(), 2);
    }

    #[test]
    fn ts_nested_template_in_hole() {
        // A template literal inside a ${} hole should not confuse the parser.
        let source = r#"const q = `SELECT * FROM users WHERE name = ${`"${n}"`}`;"#;
        let fragments = extract_typescript(source);
        assert_eq!(fragments.len(), 1);
        assert_eq!(fragments[0].holes.len(), 1);
    }

    #[test]
    fn ts_escaped_backtick() {
        let source = r"const q = `SELECT '\`' FROM users`;";
        let fragments = extract_typescript(source);
        assert_eq!(fragments.len(), 1);
        assert_eq!(fragments[0].holes.len(), 0);
    }
}
