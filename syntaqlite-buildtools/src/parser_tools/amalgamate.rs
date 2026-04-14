// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! C amalgamation: produces single-file compilation units from the syntaqlite
//! runtime and dialect source trees.
//!
//! Three modes:
//! - **Runtime only** — engine (`syntaqlite_runtime.{h,c}`) + extension header (`syntaqlite_dialect.h`)
//! - **`TypedDialectEnv` only** — dialect sources that `#include` the runtime header and ext header
//! - **Full** — runtime + dialect inlined into one pair of files
//!
//! The amalgamator uses a single-pass recursive include expansion: starting from
//! root files (public headers for `.h`, source files for `.c`), it follows
//! `#include "..."` directives and inlines referenced files in encounter order,
//! using a `seen` set to deduplicate. Include guards from the original files are
//! preserved and re-emitted so the same file can safely appear in multiple
//! amalgamation products without double-definition.

use std::collections::{BTreeMap, HashSet};
use std::fmt::Write as _;
use std::fs;
use std::path::Path;

// ---------------------------------------------------------------------------
// Warning suppressions emitted into the amalgamation .c file
// ---------------------------------------------------------------------------

/// Warnings inherent to Lemon-generated parser code that cannot be fixed
/// upstream.  The amalgamation wraps the entire .c in a diagnostic push/pop
/// so consumers compiling with `-Wall -Wextra` need no extra `-Wno-*` flags.
const SUPPRESSED_WARNINGS: &[&str] = &[
    // Lemon-generated parser inherent warnings
    "-Wunused-parameter",
    "-Wunused-variable",
    "-Wmissing-field-initializers",
    "-Wtype-limits",
    "-Wold-style-declaration",
    "-Wimplicit-fallthrough",
    "-Wswitch-enum",
    "-Wdeclaration-after-statement",
    "-Wsign-conversion",
    "-Wextra-semi-stmt",
    "-Wcast-qual",
    "-Wold-style-cast",
    "-Wunused-macros",
    "-Wformat-nonliteral",
    "-Wformat",
    "-Wcast-align",
    "-Wmissing-variable-declarations",
    "-Wimplicit-int-conversion",
    "-Wmissing-prototypes",
    "-Wunreachable-code",
    "-Wunused-function",
    "-Wswitch-default",
    "-Wpadded",
];

fn emit_diagnostic_push(out: &mut String) {
    out.push_str("#if defined(__GNUC__) || defined(__clang__)\n");
    out.push_str("#pragma GCC diagnostic push\n");
    out.push_str("#ifdef __clang__\n");
    out.push_str("#pragma clang diagnostic ignored \"-Wunknown-warning-option\"\n");
    out.push_str("#endif\n");
    for w in SUPPRESSED_WARNINGS {
        let _ = writeln!(out, "#pragma GCC diagnostic ignored \"{w}\"");
    }
    out.push_str("#endif\n\n");
}

fn emit_diagnostic_pop(out: &mut String) {
    out.push_str("\n#if defined(__GNUC__) || defined(__clang__)\n");
    out.push_str("#pragma GCC diagnostic pop\n");
    out.push_str("#endif\n");
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/// Output of an amalgamation operation.
pub struct AmalgamateOutput {
    /// Amalgamated header file content.
    pub header: String,
    /// Amalgamated source file content.
    pub source: String,
    /// Extension header (present for runtime-only amalgamation).
    pub ext_header: Option<String>,
}

/// Produce `syntaqlite_runtime.{h,c}` and `syntaqlite_dialect.h`.
///
/// `runtime_dir` must contain **only** runtime files (written by
/// [`write_runtime_headers_to_dir`](super::base_files::write_runtime_headers_to_dir)) — no dialect-specific code.
/// Scans `csrc/` and `include/` subdirectories of the given directory.
///
/// # Errors
///
/// Returns an error if reading source files from `runtime_dir` fails.
pub fn amalgamate_runtime(runtime_dir: &Path) -> Result<AmalgamateOutput, String> {
    let files = collect_files(&[&runtime_dir.join("csrc"), &runtime_dir.join("include")])?;
    Ok(emit(&files, EmitMode::RuntimeOnly))
}

/// Produce `syntaqlite_<dialect>.{h,c}` that references `syntaqlite_runtime.h`
/// and `syntaqlite_dialect.h`.
///
/// Quoted `#include` directives that don't resolve to a file in the dialect
/// tree are stripped if they look like runtime headers; the emitted `.c` file
/// includes the runtime amalgamation header via `SYNTAQLITE_RUNTIME_HEADER`
/// and the extension header via `SYNTAQLITE_EXT_HEADER`.
///
/// `runtime_header` and `ext_header` control the default values baked into
/// the `#ifndef` guards. Pass `None` for the defaults (`"syntaqlite_runtime.h"`
/// and `"syntaqlite_dialect.h"`).
///
/// # Errors
///
/// Returns an error if reading source files from `dialect_dir` fails.
pub fn amalgamate_dialect(
    dialect: &str,
    dialect_dir: &Path,
    runtime_header: Option<&str>,
    ext_header: Option<&str>,
) -> Result<AmalgamateOutput, String> {
    let files = collect_files(&[&dialect_dir.join("csrc"), &dialect_dir.join("include")])?;
    Ok(emit(
        &files,
        EmitMode::DialectOnly {
            dialect,
            runtime_header: runtime_header.unwrap_or("syntaqlite_runtime.h"),
            ext_header: ext_header.unwrap_or("syntaqlite_dialect.h"),
        },
    ))
}

/// Produce `syntaqlite_<dialect>.{h,c}` with the runtime inlined.
///
/// # Errors
///
/// Returns an error if reading source files from `runtime_dir` or `dialect_dir` fails.
pub fn amalgamate_full(
    dialect: &str,
    runtime_dir: &Path,
    dialect_dir: &Path,
) -> Result<AmalgamateOutput, String> {
    let files = collect_files(&[
        &runtime_dir.join("csrc"),
        &runtime_dir.join("include"),
        &dialect_dir.join("csrc"),
        &dialect_dir.join("include"),
    ])?;
    Ok(emit(&files, EmitMode::Full(dialect)))
}

// ---------------------------------------------------------------------------
// Internal types
// ---------------------------------------------------------------------------

/// Classification of a file based on its include key.
#[derive(Clone, Copy, PartialEq, Eq)]
enum FileKind {
    PublicHeader,   // include/syntaqlite/ or include/syntaqlite_<name>/
    ExtHeader,      // include/syntaqlite_dialect/
    InternalHeader, // csrc/*.h
    Source,         // *.c
}

/// Map from include key (e.g. `"syntaqlite/parser.h"`) to raw file content.
/// `BTreeMap` gives deterministic iteration order.
type FileMap = BTreeMap<String, String>;

fn classify(key: &str) -> FileKind {
    if key.starts_with("syntaqlite_dialect/") {
        FileKind::ExtHeader
    } else if key.starts_with("syntaqlite/") || key.starts_with("syntaqlite_") {
        FileKind::PublicHeader
    } else if Path::new(key)
        .extension()
        .is_some_and(|ext| ext.eq_ignore_ascii_case("h"))
    {
        FileKind::InternalHeader
    } else {
        FileKind::Source
    }
}

// ---------------------------------------------------------------------------
// File collection
// ---------------------------------------------------------------------------

fn collect_files(dirs: &[&Path]) -> Result<FileMap, String> {
    let mut map = FileMap::new();
    for &dir in dirs {
        if !dir.is_dir() {
            continue;
        }
        let dir_name = dir.file_name().and_then(|n| n.to_str()).unwrap_or("");
        // include/ dirs: strip the "include" prefix so keys start with the
        // subdirectory (e.g. "syntaqlite/parser.h", "syntaqlite_dialect/arena.h").
        // csrc/ dirs: keep "csrc" as prefix (e.g. "csrc/parser.c").
        let prefix = if dir_name == "include" { "" } else { dir_name };
        walk_dir(dir, prefix, &mut map)?;
    }
    Ok(map)
}

fn walk_dir(dir: &Path, prefix: &str, map: &mut FileMap) -> Result<(), String> {
    let entries =
        fs::read_dir(dir).map_err(|e| format!("reading directory {}: {e}", dir.display()))?;
    for entry in entries {
        let path = entry.map_err(|e| format!("reading entry: {e}"))?.path();
        if path.is_dir() {
            let sub = path.file_name().and_then(|n| n.to_str()).unwrap_or("");
            let sub_prefix = if prefix.is_empty() {
                sub.to_string()
            } else {
                format!("{prefix}/{sub}")
            };
            walk_dir(&path, &sub_prefix, map)?;
        } else {
            let ext = path.extension().and_then(|e| e.to_str()).unwrap_or("");
            if ext == "c" || ext == "h" {
                let name = path.file_name().and_then(|n| n.to_str()).unwrap_or("");
                let key = if prefix.is_empty() {
                    name.to_string()
                } else {
                    format!("{prefix}/{name}")
                };
                if let std::collections::btree_map::Entry::Vacant(e) = map.entry(key) {
                    let content = fs::read_to_string(&path)
                        .map_err(|e| format!("reading {}: {e}", path.display()))?;
                    e.insert(content);
                }
            }
        }
    }
    Ok(())
}

// ---------------------------------------------------------------------------
// Include directive parsing
// ---------------------------------------------------------------------------

enum IncludeDirective<'a> {
    Quoted(&'a str),
    System,
    Other,
}

/// Parse an `#include` directive, handling the `# include "x"` spaced form.
fn parse_include_directive(line: &str) -> Option<IncludeDirective<'_>> {
    let trimmed = line.trim_start();
    if !trimmed.starts_with('#') {
        return None;
    }
    let after_hash = trimmed[1..].trim_start();
    let after_kw = after_hash.strip_prefix("include")?.trim_start();
    if let Some(rest) = after_kw.strip_prefix('"') {
        let end = rest.find('"')?;
        return Some(IncludeDirective::Quoted(&rest[..end]));
    }
    if let Some(rest) = after_kw.strip_prefix('<') {
        let _ = rest.find('>')?;
        return Some(IncludeDirective::System);
    }
    Some(IncludeDirective::Other)
}

// ---------------------------------------------------------------------------
// Emit modes
// ---------------------------------------------------------------------------

#[derive(Clone, Copy)]
enum EmitMode<'a> {
    /// Runtime only: `syntaqlite_runtime.{h,c}` + `syntaqlite_dialect.h`.
    RuntimeOnly,
    /// `TypedDialectEnv` only: `syntaqlite_<name>.{h,c}`, expects external runtime/ext headers.
    DialectOnly {
        dialect: &'a str,
        runtime_header: &'a str,
        ext_header: &'a str,
    },
    /// Full: runtime + dialect inlined into `syntaqlite_<name>.{h,c}`.
    Full(&'a str),
}

// ---------------------------------------------------------------------------
// Recursive emitter
// ---------------------------------------------------------------------------

/// Which file kinds to recursively expand in the current output section.
///
/// When a resolved include doesn't match the expansion rule for the current
/// section, the include directive is silently dropped — the content either
/// lives in a different output section or is already provided by an explicit
/// `#include` at the top of the file.
///
/// Unresolved quoted includes that look like runtime paths (see
/// [`is_runtime_path`]) are always stripped, regardless of section. In
/// Full/RuntimeOnly modes all runtime files are in the map so there are no
/// unresolved runtime paths to speak of. In `DialectOnly` mode the runtime
/// files are absent from the map and must be stripped — they are provided
/// by the explicit `#include SYNTAQLITE_RUNTIME_HEADER` at the top.
#[derive(Clone, Copy)]
enum Section {
    /// `.h` output: expand `PublicHeader` includes only.
    Header,
    /// Extension `.h` output: expand `ExtHeader` includes only.
    ExtHeader,
    /// `.c` output: expand `InternalHeader` and `ExtHeader` includes.
    /// `PublicHeader` includes are stripped (they live in the `.h` output).
    Source,
}

struct Emitter<'a> {
    files: &'a FileMap,
    seen: HashSet<String>,
}

impl<'a> Emitter<'a> {
    fn new(files: &'a FileMap) -> Self {
        Self {
            files,
            seen: HashSet::new(),
        }
    }

    /// Emit one file, recursively expanding local `#include "..."` directives
    /// according to `section`. Already-seen files are skipped (deduplication).
    fn emit_file(&mut self, key: &str, out: &mut String, section: Section) {
        if !self.seen.insert(key.to_string()) {
            return;
        }
        let content = match self.files.get(key) {
            Some(c) => c.clone(),
            None => return,
        };

        let _ = writeln!(out, "/* ======== begin: {key} ======== */");

        let guard = detect_include_guard(&content);
        if let Some(ref g) = guard {
            let _ = write!(out, "#ifndef {g}\n#define {g}\n");
        }

        let mut lines: Vec<&str> = content.lines().collect();

        // Strip the trailing `#endif` of the original guard so we can re-emit
        // our own below (prevents the guard from spanning the end-marker comment).
        if guard.is_some() {
            for i in (0..lines.len()).rev() {
                let t = lines[i].trim();
                if t.is_empty() {
                    continue;
                }
                if t.starts_with("#endif") {
                    lines[i] = "";
                    break;
                }
                break;
            }
        }

        let mut guard_ifndef_seen = false;
        let mut guard_define_seen = false;

        for line in &lines {
            let trimmed = line.trim();

            // Strip the original `#ifndef GUARD` / `#define GUARD` pair — we
            // re-emitted them above, before the content block.
            if let Some(ref g) = guard {
                if !guard_ifndef_seen {
                    if let Some(rest) = trimmed.strip_prefix("#ifndef")
                        && rest.trim() == g.as_str()
                    {
                        guard_ifndef_seen = true;
                        continue;
                    }
                } else if !guard_define_seen
                    && let Some(rest) = trimmed.strip_prefix("#define")
                    && rest.trim() == g.as_str()
                {
                    guard_define_seen = true;
                    continue;
                }
            }

            if let Some(directive) = parse_include_directive(trimmed) {
                match directive {
                    IncludeDirective::Quoted(path) => {
                        if self.files.contains_key(path) {
                            // Resolved: inline it if appropriate for this section,
                            // otherwise silently drop the directive.
                            let kind = classify(path);
                            let should_expand = match section {
                                Section::Header => kind == FileKind::PublicHeader,
                                Section::ExtHeader => kind == FileKind::ExtHeader,
                                Section::Source => {
                                    matches!(kind, FileKind::InternalHeader | FileKind::ExtHeader)
                                }
                            };
                            if should_expand {
                                self.emit_file(path, out, section);
                            }
                            continue;
                        }
                        // Unresolved: strip runtime-style paths in all sections.
                        // In Full/RuntimeOnly modes these never arise (all runtime
                        // files are in the map). In DialectOnly mode they must be
                        // stripped since the runtime header is included explicitly.
                        if is_runtime_path(path) {
                            continue;
                        }
                    }
                    // System includes (`<...>`) and macro includes are always kept.
                    IncludeDirective::System | IncludeDirective::Other => {}
                }
            }

            out.push_str(line);
            out.push('\n');
        }

        if let Some(ref g) = guard {
            let _ = writeln!(out, "#endif  /* {g} */");
        }
        let _ = write!(out, "/* ======== end: {key} ======== */\n\n");
    }

    /// Emit all files of `kind` (in sorted key order), each recursively.
    fn emit_kind(&mut self, kind: FileKind, out: &mut String, section: Section) {
        let keys: Vec<String> = self
            .files
            .keys()
            .filter(|k| classify(k) == kind)
            .cloned()
            .collect();
        for key in keys {
            self.emit_file(&key, out, section);
        }
    }
}

/// Returns true if `path` is a runtime/dialect-SPI include that should be
/// stripped in dialect-only source output (provided by `SYNTAQLITE_RUNTIME_HEADER`
/// / `SYNTAQLITE_EXT_HEADER` at the top of the file instead).
fn is_runtime_path(path: &str) -> bool {
    path.starts_with("syntaqlite/")
        || path.starts_with("syntaqlite_")
        || path.starts_with("syntaqlite_dialect/")
        || path.starts_with("csrc/")
}

// ---------------------------------------------------------------------------
// Include guard detection
// ---------------------------------------------------------------------------

/// Detect the include-guard macro of a header file, if any.
///
/// Returns `Some(guard)` when the first two preprocessor directives are
/// `#ifndef GUARD` / `#define GUARD` and the file ends with `#endif`.
fn detect_include_guard(content: &str) -> Option<String> {
    let lines: Vec<&str> = content.lines().collect();

    // Find first two preprocessor directives (skip blanks and comments).
    let mut pp = Vec::new();
    for &line in &lines {
        let t = line.trim();
        if t.is_empty()
            || t.starts_with("//")
            || t.starts_with("/*")
            || t.starts_with("**")
            || t.starts_with("*/")
        {
            continue;
        }
        if t.starts_with('#') {
            pp.push(t);
            if pp.len() == 2 {
                break;
            }
        } else {
            return None;
        }
    }

    if pp.len() < 2 {
        return None;
    }
    let guard = pp[0].strip_prefix("#ifndef")?.trim().to_string();
    if guard.is_empty() {
        return None;
    }
    let define_guard = pp[1].strip_prefix("#define")?.trim().to_string();
    if define_guard != guard {
        return None;
    }

    // Verify there's a trailing `#endif`.
    for &line in lines.iter().rev() {
        let t = line.trim();
        if t.is_empty() {
            continue;
        }
        if t.starts_with("#endif") {
            return Some(guard);
        }
        break;
    }
    None
}

// ---------------------------------------------------------------------------
// Emit
// ---------------------------------------------------------------------------

#[expect(
    clippy::too_many_lines,
    reason = "large emit function; splitting would harm readability"
)]
fn emit(files: &FileMap, mode: EmitMode) -> AmalgamateOutput {
    let (guard, header_filename) = match &mode {
        EmitMode::DialectOnly { dialect: d, .. } | EmitMode::Full(d) => (
            format!("SYNTAQLITE_{}_H", d.to_uppercase()),
            format!("syntaqlite_{d}.h"),
        ),
        EmitMode::RuntimeOnly => (
            "SYNTAQLITE_RUNTIME_H".to_string(),
            "syntaqlite_runtime.h".to_string(),
        ),
    };

    // ── Build .h ──
    let mut header = String::new();
    header.push_str("/*\n");
    header.push_str("** syntaqlite amalgamation — machine generated, do not edit.\n");
    header.push_str("*/\n");
    let _ = write!(header, "#ifndef {guard}\n#define {guard}\n\n");
    emit_diagnostic_push(&mut header);

    match &mode {
        EmitMode::DialectOnly {
            dialect,
            runtime_header,
            ..
        } => {
            if *dialect != "sqlite" {
                header.push_str("#ifndef SYNTAQLITE_OMIT_SQLITE_API\n");
                header.push_str("#define SYNTAQLITE_OMIT_SQLITE_API\n");
                header.push_str("#endif\n\n");
            }
            header.push_str("#ifndef SYNTAQLITE_RUNTIME_HEADER\n");
            let _ = writeln!(
                header,
                "#define SYNTAQLITE_RUNTIME_HEADER \"{runtime_header}\""
            );
            header.push_str("#endif\n");
            header.push_str("#include SYNTAQLITE_RUNTIME_HEADER\n\n");
        }
        EmitMode::Full(dialect) if *dialect != "sqlite" => {
            header.push_str("#ifndef SYNTAQLITE_OMIT_SQLITE_API\n");
            header.push_str("#define SYNTAQLITE_OMIT_SQLITE_API\n");
            header.push_str("#endif\n\n");
        }
        _ => {}
    }

    let mut h_emitter = Emitter::new(files);
    h_emitter.emit_kind(FileKind::PublicHeader, &mut header, Section::Header);
    emit_diagnostic_pop(&mut header);
    let _ = write!(header, "\n#endif  /* {guard} */\n");

    // ── Build ext header (runtime-only mode) ──
    let ext_header = if matches!(mode, EmitMode::RuntimeOnly) {
        let has_ext = files.keys().any(|k| classify(k) == FileKind::ExtHeader);
        if has_ext {
            let mut ext = String::new();
            ext.push_str("/*\n");
            ext.push_str("** syntaqlite amalgamation — machine generated, do not edit.\n");
            ext.push_str("** Extension header for dialect authors.\n");
            ext.push_str("*/\n");
            ext.push_str("#ifndef SYNTAQLITE_EXT_H\n#define SYNTAQLITE_EXT_H\n\n");
            emit_diagnostic_push(&mut ext);
            ext.push_str("#include \"syntaqlite_runtime.h\"\n\n");
            let mut e_emitter = Emitter::new(files);
            e_emitter.emit_kind(FileKind::ExtHeader, &mut ext, Section::ExtHeader);
            emit_diagnostic_pop(&mut ext);
            ext.push_str("\n#endif  /* SYNTAQLITE_EXT_H */\n");
            Some(ext)
        } else {
            None
        }
    } else {
        None
    };

    // ── Build .c ──

    let mut source = String::new();
    source.push_str("/*\n");
    source.push_str("** syntaqlite amalgamation — machine generated, do not edit.\n");
    source.push_str("*/\n\n");

    // Suppress warnings inherent to Lemon-generated parser code.
    // These are pushed here and popped at the end of the file so that
    // consumers compiling with -Weverything -Werror need no -Wno-* flags.
    emit_diagnostic_push(&mut source);

    if let EmitMode::DialectOnly {
        dialect,
        runtime_header,
        ext_header,
    } = &mode
    {
        if *dialect != "sqlite" {
            source.push_str("#ifndef SYNTAQLITE_OMIT_SQLITE_API\n");
            source.push_str("#define SYNTAQLITE_OMIT_SQLITE_API\n");
            source.push_str("#endif\n\n");
        }
        source.push_str("#ifndef SYNTAQLITE_RUNTIME_HEADER\n");
        let _ = writeln!(
            source,
            "#define SYNTAQLITE_RUNTIME_HEADER \"{runtime_header}\""
        );
        source.push_str("#endif\n");
        source.push_str("#include SYNTAQLITE_RUNTIME_HEADER\n\n");
        source.push_str("#ifndef SYNTAQLITE_EXT_HEADER\n");
        let _ = writeln!(source, "#define SYNTAQLITE_EXT_HEADER \"{ext_header}\"");
        source.push_str("#endif\n");
        source.push_str("#include SYNTAQLITE_EXT_HEADER\n\n");
    } else if let EmitMode::Full(dialect) = &mode
        && *dialect != "sqlite"
    {
        source.push_str("#ifndef SYNTAQLITE_OMIT_SQLITE_API\n");
        source.push_str("#define SYNTAQLITE_OMIT_SQLITE_API\n");
        source.push_str("#endif\n\n");
    }
    let _ = write!(source, "#include \"{header_filename}\"\n\n");

    // Emit sources. They recursively pull in their internal and ext-header
    // dependencies in encounter order (driven by the include graph).
    let mut s_emitter = Emitter::new(files);
    s_emitter.emit_kind(FileKind::Source, &mut source, Section::Source);

    emit_diagnostic_pop(&mut source);

    AmalgamateOutput {
        header,
        source,
        ext_header,
    }
}

// ---------------------------------------------------------------------------
// Full API header amalgamation (syntaqlite.h)
// ---------------------------------------------------------------------------

/// Produce a single `syntaqlite.h` that combines:
/// - All public headers from `syntaqlite-syntax` (parser, tokenizer, grammar, etc.)
/// - All FFI headers from `syntaqlite` (formatter, validator)
///
/// This is the header shipped with prebuilt shared libraries (`libsyntaqlite`).
///
/// # Errors
///
/// Returns an error if reading header files fails.
pub fn amalgamate_header(syntax_dir: &Path, lib_dir: &Path) -> Result<String, String> {
    let mut header = String::new();
    header.push_str("/*\n");
    header.push_str("** syntaqlite.h — single-file C API header\n");
    header.push_str("** Machine generated, do not edit.\n");
    header.push_str("**\n");
    header.push_str("** Parser, tokenizer, formatter, and validator for SQLite SQL.\n");
    header.push_str("*/\n");
    header.push_str("#ifndef SYNTAQLITE_H\n#define SYNTAQLITE_H\n\n");

    // Collect syntax public headers.
    let syntax_include = syntax_dir.join("include");
    let syntax_headers = collect_files(&[&syntax_include])?;

    // Emit syntax headers in order.
    let mut emitter = Emitter::new(&syntax_headers);
    emitter.emit_kind(FileKind::PublicHeader, &mut header, Section::Header);

    // Now append each lib header (formatter.h, validation.h) directly.
    // These are standalone headers with only <stdint.h> dependencies — no
    // recursive include expansion needed.
    let lib_include = lib_dir.join("include");
    if lib_include.is_dir() {
        let lib_headers = collect_files(&[&lib_include])?;
        for (key, content) in &lib_headers {
            if classify(key) == FileKind::PublicHeader {
                let _ = writeln!(header, "/* ======== begin: {key} ======== */");

                // Strip the include guard — we have our own outer SYNTAQLITE_H guard.
                let guard = detect_include_guard(content);
                let mut skip_ifndef = false;
                let mut skip_define = false;
                let lines: Vec<&str> = content.lines().collect();

                // Find last #endif to skip.
                let last_endif_idx = if guard.is_some() {
                    lines.iter().rposition(|l| l.trim().starts_with("#endif"))
                } else {
                    None
                };

                for (i, line) in lines.iter().enumerate() {
                    let trimmed = line.trim();

                    // Skip include guard directives.
                    if let Some(ref g) = guard {
                        if !skip_ifndef
                            && let Some(rest) = trimmed.strip_prefix("#ifndef")
                            && rest.trim() == g.as_str()
                        {
                            skip_ifndef = true;
                            continue;
                        } else if !skip_define
                            && let Some(rest) = trimmed.strip_prefix("#define")
                            && rest.trim() == g.as_str()
                        {
                            skip_define = true;
                            continue;
                        }
                        if Some(i) == last_endif_idx {
                            continue;
                        }
                    }

                    // Skip includes already provided by the syntax headers above.
                    if trimmed == "#include <stdint.h>"
                        || trimmed == "#include \"syntaqlite/config.h\""
                    {
                        continue;
                    }

                    header.push_str(line);
                    header.push('\n');
                }

                let _ = writeln!(header, "/* ======== end: {key} ======== */\n");
            }
        }
    }

    header.push_str("\n#endif  /* SYNTAQLITE_H */\n");
    Ok(header)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_include_directive_accepts_spaced_form() {
        let inc = parse_include_directive("# include \"syntaqlite/foo.h\"");
        assert!(matches!(
            inc,
            Some(IncludeDirective::Quoted("syntaqlite/foo.h"))
        ));
    }

    #[test]
    fn parse_include_directive_handles_system_and_macro() {
        let sys = parse_include_directive("#include <stdint.h>");
        assert!(matches!(sys, Some(IncludeDirective::System)));

        let mac = parse_include_directive("#include SYNTAQLITE_RUNTIME_HEADER");
        assert!(matches!(mac, Some(IncludeDirective::Other)));
    }

    #[test]
    fn is_runtime_path_identifies_known_prefixes() {
        assert!(is_runtime_path("syntaqlite/parser.h"));
        assert!(is_runtime_path("syntaqlite_dialect/arena.h"));
        assert!(is_runtime_path("csrc/dialect_dispatch.h"));
        assert!(!is_runtime_path("vendor/custom.h"));
    }
}
