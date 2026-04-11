// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! High-level C code transformer that applies transformations in-place.
//!
//! Instead of manually extracting, transforming, and reassembling pieces,
//! `CTransformer` handles the orchestration automatically.
//!
//! ## Error handling
//!
//! Every mutation method expects its target (function, anchor substring,
//! pattern, etc.) to actually be present in the input.  If a target is
//! missing, the method records an error and leaves the content unchanged;
//! the accumulated errors are surfaced when [`CTransformer::finish`] is
//! called.  This is deliberate: codegen transformations are mandatory
//! steps, and a silent no-op here produces a subtly-broken-but-compiling
//! artifact downstream.  Callers should treat any `Err` from `finish()`
//! as a hard codegen failure.

use super::c_extractor::CExtractor;

pub(crate) struct CTransformer {
    content: String,
    errors: Vec<String>,
}

impl CTransformer {
    pub(crate) fn new(content: &str) -> Self {
        Self {
            content: content.to_string(),
            errors: Vec::new(),
        }
    }

    fn record_error(&mut self, msg: String) {
        self.errors.push(msg);
    }

    /// Remove `static` keyword from a declaration (array, variable, or function)
    ///
    /// This works for array declarations like `static int foo[]`,
    /// variable declarations like `static int bar = 42`,
    /// and function declarations like `static int baz(...)`
    pub(crate) fn remove_static_first(mut self, name: &str) -> Self {
        let lines: Vec<String> = self.content.lines().map(ToString::to_string).collect();

        // Try to find the declaration line
        for line in &lines {
            let trimmed = line.trim_start();

            // Check if this line contains the declaration
            if trimmed.starts_with("static ") && line.contains(name) {
                // Additional checks to ensure this is the actual declaration:
                // - Array: contains "name["
                // - Variable: contains "name " or " name "
                // - Function: contains "name("
                let is_array = line.contains(&format!("{name}["));
                let is_variable =
                    line.contains(&format!("{name} ")) || line.contains(&format!(" {name} "));
                let is_function = line.contains(&format!("{name}("));

                if is_array || is_variable || is_function {
                    let transformed = line.replacen("static ", "", 1);
                    self.content = self.content.replace(line, &transformed);
                    return self;
                }
            }
        }
        self.record_error(format!(
            "remove_static_first: no `static` declaration found for `{name}`"
        ));
        self
    }

    /// Add `static` keyword to an array declaration
    pub(crate) fn add_array_static(mut self, name: &str) -> Self {
        let extractor = CExtractor::new(&self.content);
        match extractor.extract_static_array(name) {
            Ok(array) => {
                if array.text.trim_start().starts_with("static ") {
                    self.record_error(format!(
                        "add_array_static: array `{name}` is already declared static"
                    ));
                } else {
                    let transformed =
                        array
                            .text
                            .replacen(&format!("{name}["), &format!("static {name}["), 1);
                    self.content = self.content.replace(&array.text, &transformed);
                }
            }
            Err(e) => {
                self.record_error(format!("add_array_static: {e}"));
            }
        }
        self
    }

    /// Replace all occurrences of a string throughout the content.
    ///
    /// Fails if `from` is not present anywhere in the content.
    pub(crate) fn replace_all(mut self, from: &str, to: &str) -> Self {
        if !self.content.contains(from) {
            self.record_error(format!(
                "replace_all: pattern `{from}` not found in content"
            ));
            return self;
        }
        self.content = self.content.replace(from, to);
        self
    }

    /// Insert content after all include directives
    pub(crate) fn insert_after_includes(mut self, content: &str) -> Self {
        let Some(pos) = self.content.rfind("#include") else {
            self.record_error(
                "insert_after_includes: no `#include` directive found in content".to_string(),
            );
            return self;
        };
        let Some(newline_pos) = self.content[pos..].find('\n') else {
            self.record_error(
                "insert_after_includes: `#include` line not terminated by newline".to_string(),
            );
            return self;
        };
        let insert_pos = pos + newline_pos + 1;
        self.content
            .insert_str(insert_pos, &format!("\n{content}\n"));
        self
    }

    /// Rename a function
    pub(crate) fn rename_function(mut self, old_name: &str, new_name: &str) -> Self {
        let pattern = format!("{old_name}(");
        let replacement = format!("{new_name}(");
        self.transform_function(
            old_name,
            |text| text.replace(&pattern, &replacement),
            "rename_function",
        );
        self
    }

    /// Remove a function completely
    pub(crate) fn remove_function(mut self, name: &str) -> Self {
        let extractor = CExtractor::new(&self.content);
        match extractor.extract_function(name) {
            Ok(function) => {
                self.content = self.content.replace(&function.text, "");
            }
            Err(e) => {
                self.record_error(format!("remove_function(`{name}`): {e}"));
            }
        }
        self
    }

    /// Remove lines matching a pattern.
    ///
    /// Fails if no line contains the pattern.
    pub(crate) fn remove_lines_matching(mut self, pattern: &str) -> Self {
        let lines: Vec<String> = self.content.lines().map(ToString::to_string).collect();
        let any_matched = lines.iter().any(|line| line.contains(pattern));
        if !any_matched {
            self.record_error(format!(
                "remove_lines_matching: no line contained `{pattern}`"
            ));
            return self;
        }
        let filtered: Vec<String> = lines
            .into_iter()
            .filter(|line| !line.contains(pattern))
            .collect();
        self.content = filtered.join("\n") + "\n";
        self
    }

    /// Replace text within a specific function's body.
    ///
    /// Fails if the function doesn't exist or `from` is not present inside
    /// the function body.
    pub(crate) fn replace_in_function(mut self, name: &str, from: &str, to: &str) -> Self {
        // Pre-check that `from` is present in the function body so we can
        // report a precise error if it's missing.  transform_function will
        // otherwise silently apply an identity replace.
        {
            let extractor = CExtractor::new(&self.content);
            match extractor.extract_function(name) {
                Ok(function) => {
                    if !function.text.contains(from) {
                        self.record_error(format!(
                            "replace_in_function(`{name}`): anchor `{from}` not found in \
                             function body"
                        ));
                        return self;
                    }
                }
                Err(e) => {
                    self.record_error(format!("replace_in_function(`{name}`): {e}"));
                    return self;
                }
            }
        }
        self.transform_function(name, |text| text.replace(from, to), "replace_in_function");
        self
    }

    /// Extract a function by name, apply `f` to its text, and replace it in content.
    ///
    /// Records an error tagged with `op` if the function can't be found.
    fn transform_function(&mut self, name: &str, f: impl FnOnce(&str) -> String, op: &str) {
        let extractor = CExtractor::new(&self.content);
        match extractor.extract_function(name) {
            Ok(function) => {
                let transformed = f(&function.text);
                self.content = self.content.replace(&function.text, &transformed);
            }
            Err(e) => {
                self.record_error(format!("{op}(`{name}`): {e}"));
            }
        }
    }

    /// Remove the first `/* ... */` block comment containing `needle`.
    pub(crate) fn remove_block_comment_containing(mut self, needle: &str) -> Self {
        let Some(idx) = self.content.find(needle) else {
            self.record_error(format!(
                "remove_block_comment_containing: needle `{needle}` not found in content"
            ));
            return self;
        };
        let Some(start) = self.content[..idx].rfind("/*") else {
            self.record_error(format!(
                "remove_block_comment_containing: `{needle}` found but no preceding `/*`"
            ));
            return self;
        };
        let Some(end_rel) = self.content[start..].find("*/") else {
            self.record_error(format!(
                "remove_block_comment_containing: `{needle}` block has no closing `*/`"
            ));
            return self;
        };
        let end = start + end_rel + 2;
        // Also consume trailing newline
        let end = if self.content[end..].starts_with('\n') {
            end + 1
        } else {
            end
        };
        self.content.replace_range(start..end, "");
        self
    }

    /// Append content at the end of the file.
    ///
    /// This operation cannot fail — appending always succeeds.
    pub(crate) fn append(mut self, content: &str) -> Self {
        self.content.push_str(content);
        self
    }

    /// Finish transformation and return the result.
    ///
    /// Returns `Err` if any previous mutation recorded an error.  Errors
    /// are reported in the order they occurred, joined by newlines.
    pub(crate) fn finish(self) -> Result<String, String> {
        if self.errors.is_empty() {
            Ok(self.content)
        } else {
            Err(format!(
                "CTransformer: {} operation(s) failed:\n  - {}",
                self.errors.len(),
                self.errors.join("\n  - ")
            ))
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // A minimal but realistic synthetic C file used by most of the tests.
    const SAMPLE: &str = concat!(
        "#include <stdio.h>\n",
        "#include <stdlib.h>\n",
        "\n",
        "/* automatically generated by something */\n",
        "static int gCounter = 0;\n",
        "\n",
        "static int aArray[] = {1, 2, 3};\n",
        "\n",
        "int foo(int x) {\n",
        "  int y = x + 1;\n",
        "  return y;\n",
        "}\n",
        "\n",
        "int bar(int x) {\n",
        "  return foo(x) * 2;\n",
        "}\n",
    );

    #[test]
    fn finish_ok_when_no_errors() {
        let out = CTransformer::new(SAMPLE).finish().expect("should succeed");
        assert_eq!(out, SAMPLE);
    }

    #[test]
    fn append_always_succeeds() {
        let out = CTransformer::new(SAMPLE)
            .append("/* trailer */\n")
            .finish()
            .expect("append should succeed");
        assert!(out.ends_with("/* trailer */\n"));
    }

    #[test]
    fn replace_all_fails_when_pattern_missing() {
        let err = CTransformer::new(SAMPLE)
            .replace_all("NOT_IN_SAMPLE", "whatever")
            .finish()
            .expect_err("should fail");
        assert!(err.contains("replace_all"));
        assert!(err.contains("NOT_IN_SAMPLE"));
    }

    #[test]
    fn replace_all_succeeds_when_pattern_present() {
        let out = CTransformer::new(SAMPLE)
            .replace_all("foo", "renamed_foo")
            .finish()
            .expect("should succeed");
        assert!(out.contains("renamed_foo"));
        assert!(!out.contains(" foo("));
    }

    #[test]
    fn insert_after_includes_fails_when_no_includes() {
        let err = CTransformer::new("int main() { return 0; }\n")
            .insert_after_includes("#include \"x.h\"")
            .finish()
            .expect_err("should fail");
        assert!(err.contains("insert_after_includes"));
        assert!(err.contains("#include"));
    }

    #[test]
    fn insert_after_includes_succeeds() {
        let out = CTransformer::new(SAMPLE)
            .insert_after_includes("#include \"extra.h\"")
            .finish()
            .expect("should succeed");
        assert!(out.contains("#include \"extra.h\""));
    }

    #[test]
    fn replace_in_function_fails_when_function_missing() {
        let err = CTransformer::new(SAMPLE)
            .replace_in_function("nonexistent_fn", "old", "new")
            .finish()
            .expect_err("should fail");
        assert!(err.contains("replace_in_function"));
        assert!(err.contains("nonexistent_fn"));
    }

    #[test]
    fn replace_in_function_fails_when_anchor_missing() {
        let err = CTransformer::new(SAMPLE)
            .replace_in_function("foo", "this_anchor_does_not_exist", "replacement")
            .finish()
            .expect_err("should fail");
        assert!(err.contains("replace_in_function"));
        assert!(err.contains("anchor"));
        assert!(err.contains("this_anchor_does_not_exist"));
    }

    #[test]
    fn replace_in_function_succeeds() {
        let out = CTransformer::new(SAMPLE)
            .replace_in_function("foo", "int y = x + 1;", "int y = x + 42;")
            .finish()
            .expect("should succeed");
        assert!(out.contains("int y = x + 42;"));
    }

    #[test]
    fn rename_function_fails_when_missing() {
        let err = CTransformer::new(SAMPLE)
            .rename_function("nonexistent_fn", "new_name")
            .finish()
            .expect_err("should fail");
        assert!(err.contains("rename_function"));
        assert!(err.contains("nonexistent_fn"));
    }

    #[test]
    fn rename_function_succeeds() {
        let out = CTransformer::new(SAMPLE)
            .rename_function("foo", "foo_renamed")
            .finish()
            .expect("should succeed");
        assert!(out.contains("foo_renamed("));
    }

    #[test]
    fn remove_function_fails_when_missing() {
        let err = CTransformer::new(SAMPLE)
            .remove_function("nonexistent_fn")
            .finish()
            .expect_err("should fail");
        assert!(err.contains("remove_function"));
        assert!(err.contains("nonexistent_fn"));
    }

    #[test]
    fn remove_function_succeeds() {
        let out = CTransformer::new(SAMPLE)
            .remove_function("foo")
            .finish()
            .expect("should succeed");
        assert!(!out.contains("int foo(int x)"));
    }

    #[test]
    fn remove_lines_matching_fails_when_no_match() {
        let err = CTransformer::new(SAMPLE)
            .remove_lines_matching("NOT_PRESENT_ANYWHERE")
            .finish()
            .expect_err("should fail");
        assert!(err.contains("remove_lines_matching"));
    }

    #[test]
    fn remove_lines_matching_succeeds() {
        let out = CTransformer::new(SAMPLE)
            .remove_lines_matching("gCounter")
            .finish()
            .expect("should succeed");
        assert!(!out.contains("gCounter"));
    }

    #[test]
    fn remove_block_comment_containing_fails_when_needle_missing() {
        let err = CTransformer::new(SAMPLE)
            .remove_block_comment_containing("absent needle")
            .finish()
            .expect_err("should fail");
        assert!(err.contains("remove_block_comment_containing"));
    }

    #[test]
    fn remove_block_comment_containing_succeeds() {
        let out = CTransformer::new(SAMPLE)
            .remove_block_comment_containing("automatically generated")
            .finish()
            .expect("should succeed");
        assert!(!out.contains("automatically generated"));
    }

    #[test]
    fn remove_static_first_fails_when_declaration_missing() {
        let err = CTransformer::new(SAMPLE)
            .remove_static_first("notDeclared")
            .finish()
            .expect_err("should fail");
        assert!(err.contains("remove_static_first"));
        assert!(err.contains("notDeclared"));
    }

    #[test]
    fn remove_static_first_succeeds_for_array() {
        let out = CTransformer::new(SAMPLE)
            .remove_static_first("aArray")
            .finish()
            .expect("should succeed");
        assert!(out.contains("int aArray[]"));
        assert!(!out.contains("static int aArray"));
    }

    #[test]
    fn remove_static_first_succeeds_for_variable() {
        let out = CTransformer::new(SAMPLE)
            .remove_static_first("gCounter")
            .finish()
            .expect("should succeed");
        assert!(out.contains("int gCounter = 0;"));
        assert!(!out.contains("static int gCounter"));
    }

    #[test]
    fn add_array_static_fails_when_missing() {
        let err = CTransformer::new(SAMPLE)
            .add_array_static("notAnArray")
            .finish()
            .expect_err("should fail");
        assert!(err.contains("add_array_static"));
    }

    #[test]
    fn add_array_static_fails_when_already_static() {
        // aArray is already declared static in SAMPLE.
        let err = CTransformer::new(SAMPLE)
            .add_array_static("aArray")
            .finish()
            .expect_err("should fail");
        assert!(err.contains("add_array_static"));
        assert!(err.contains("already"));
    }

    #[test]
    fn multiple_errors_are_all_reported() {
        let err = CTransformer::new(SAMPLE)
            .replace_all("NOT_PRESENT_1", "x")
            .replace_all("NOT_PRESENT_2", "y")
            .remove_function("nonexistent_fn")
            .finish()
            .expect_err("should fail");
        assert!(err.contains("NOT_PRESENT_1"));
        assert!(err.contains("NOT_PRESENT_2"));
        assert!(err.contains("nonexistent_fn"));
        assert!(err.contains("3 operation(s) failed"));
    }

    #[test]
    fn successful_chain_composes() {
        // Note: rename_function only renames occurrences inside the function
        // body of `old_name` (which is typically its own signature).  Call
        // sites in *other* functions are not touched — this mirrors how
        // real callers pair `rename_function` with a follow-up `replace_all`
        // for call-site fixups.
        let out = CTransformer::new(SAMPLE)
            .remove_block_comment_containing("automatically generated")
            .insert_after_includes("#include \"extra.h\"")
            .rename_function("foo", "foo_renamed")
            .replace_all("foo(", "foo_renamed(")
            .replace_in_function("bar", "* 2", "* 3")
            .append("/* appended */\n")
            .finish()
            .expect("should succeed");
        assert!(!out.contains("automatically generated"));
        assert!(out.contains("#include \"extra.h\""));
        assert!(out.contains("int foo_renamed(int x)"));
        assert!(out.contains("foo_renamed(x) * 3"));
        assert!(out.ends_with("/* appended */\n"));
    }
}
