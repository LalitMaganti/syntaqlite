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
use std::sync::OnceLock;

/// Empty runtime-keys set reused by `Emitter::new` for modes that don't
/// wrap any files in the `SYNTAQLITE_OMIT_RUNTIME` guard.
static EMPTY_KEYS: OnceLock<HashSet<String>> = OnceLock::new();

// ---------------------------------------------------------------------------
// Warning suppressions emitted into the amalgamation .c file
// ---------------------------------------------------------------------------

/// Warnings inherent to Lemon-generated parser code that cannot be fixed
/// upstream.  The amalgamation wraps the entire .c in a diagnostic push/pop
/// so consumers compiling with `-Wall -Wextra` need no extra `-Wno-*` flags.
///
/// These are supported by both GCC and Clang in C and C++.
const SUPPRESSED_WARNINGS: &[&str] = &[
    "-Wunused-parameter",
    "-Wunused-variable",
    "-Wmissing-field-initializers",
    "-Wtype-limits",
    "-Wimplicit-fallthrough",
    "-Wswitch-enum",
    "-Wsign-conversion",
    "-Wcast-qual",
    "-Wunused-macros",
    "-Wformat-nonliteral",
    "-Wformat",
    "-Wcast-align",
    "-Wunreachable-code",
    "-Wunused-function",
    "-Wswitch-default",
    "-Wpadded",
];

/// Warnings valid only in C. GCC errors with `-Werror=pragmas` if these
/// appear in a C++ translation unit, so they are guarded by
/// `#ifndef __cplusplus`. The amalgamated header is `#include`d from C++
/// consumers (e.g. perfetto) as well as C.
const SUPPRESSED_WARNINGS_C_ONLY: &[&str] =
    &["-Wdeclaration-after-statement", "-Wmissing-prototypes"];

/// Warnings valid only in C++; guarded by `#ifdef __cplusplus`.
///
/// `-Wzero-as-null-pointer-constant` fires on `0` used where a pointer is
/// expected — Lemon-generated code uses this idiom, and it surfaces when
/// the amalgamation is compiled as C++ (notably on Windows toolchains).
const SUPPRESSED_WARNINGS_CXX_ONLY: &[&str] = &["-Wzero-as-null-pointer-constant"];

/// Warnings that only Clang understands; emitted inside `#ifdef __clang__`.
///
/// `-Wimplicit-void-ptr-cast` and `-Wimplicit-int-enum-cast` are clang 19+
/// additions that fire under `-Weverything` when the amalgamation is
/// reached via a C++ translation unit (the strict-warnings test includes
/// the header from a `.cpp` driver).  The generator emits C-idiomatic
/// implicit conversions that are safe in context but would require
/// pervasive casts to quiet strict C++; suppress them consistent with the
/// rest of this list. We ignore `-Wunknown-warning-option` first inside
/// the clang block so older clangs that don't know these flag names
/// silently skip them instead of erroring under `-Werror`.
const SUPPRESSED_WARNINGS_CLANG_ONLY: &[&str] = &[
    "-Wunknown-warning-option",
    "-Wextra-semi-stmt",
    "-Wold-style-cast",
    "-Wmissing-variable-declarations",
    "-Wimplicit-int-conversion",
    "-Wimplicit-int-enum-cast",
    "-Wimplicit-void-ptr-cast",
    "-Wshorten-64-to-32",
];

/// Warnings understood only by GCC and only in C mode; emitted inside
/// `#elif defined(__GNUC__) && !defined(__cplusplus)`.
const SUPPRESSED_WARNINGS_GCC_C_ONLY: &[&str] = &["-Wold-style-declaration"];

fn emit_diagnostic_push(out: &mut String) {
    out.push_str("#if defined(__GNUC__) || defined(__clang__)\n");
    out.push_str("#pragma GCC diagnostic push\n");
    for w in SUPPRESSED_WARNINGS {
        let _ = writeln!(out, "#pragma GCC diagnostic ignored \"{w}\"");
    }
    out.push_str("#ifndef __cplusplus\n");
    for w in SUPPRESSED_WARNINGS_C_ONLY {
        let _ = writeln!(out, "#pragma GCC diagnostic ignored \"{w}\"");
    }
    out.push_str("#else\n");
    for w in SUPPRESSED_WARNINGS_CXX_ONLY {
        let _ = writeln!(out, "#pragma GCC diagnostic ignored \"{w}\"");
    }
    out.push_str("#endif\n");
    out.push_str("#ifdef __clang__\n");
    for w in SUPPRESSED_WARNINGS_CLANG_ONLY {
        let _ = writeln!(out, "#pragma clang diagnostic ignored \"{w}\"");
    }
    out.push_str("#elif defined(__GNUC__) && !defined(__cplusplus)\n");
    for w in SUPPRESSED_WARNINGS_GCC_C_ONLY {
        let _ = writeln!(out, "#pragma GCC diagnostic ignored \"{w}\"");
    }
    out.push_str("#endif\n");
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
/// Emit `#ifndef / #define / #endif` for a compile-time omit flag.
fn emit_omit_define(out: &mut String, name: &str) {
    let _ = write!(out, "#ifndef {name}\n#define {name}\n#endif\n\n");
}

/// Produce the runtime-only amalgamation: `syntaqlite_runtime.{h,c}` +
/// `syntaqlite_dialect.h`.
///
/// # Errors
///
/// Returns an error if reading source files from `runtime_dir` fails.
pub fn amalgamate_runtime(runtime_dir: &Path) -> Result<AmalgamateOutput, String> {
    let files = collect_files(&[&runtime_dir.join("csrc"), &runtime_dir.join("include")])?;
    Ok(emit(&files, EmitMode::RuntimeOnly, false))
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
        false,
    ))
}

/// Produce `syntaqlite_<dialect>.{h,c}` with the runtime inlined.
///
/// When `omit_macros` is true, `SYNTAQLITE_OMIT_MACROS` is injected into
/// the amalgamation header and source, compiling out all macro expansion
/// code.
///
/// The generated `.c` wraps runtime-origin sources (from `runtime_dir`) in
/// an `#ifndef SYNTAQLITE_OMIT_RUNTIME` guard so callers loading the output
/// as a plugin (`cdylib`) can compile with `-DSYNTAQLITE_OMIT_RUNTIME` to
/// exclude runtime implementations and rely on the host binary's copy.
///
/// # Errors
///
/// Returns an error if reading source files from `runtime_dir` or `dialect_dir` fails.
pub fn amalgamate_full(
    dialect: &str,
    runtime_dir: &Path,
    dialect_dir: &Path,
    omit_macros: bool,
) -> Result<AmalgamateOutput, String> {
    let runtime_files = collect_files(&[&runtime_dir.join("csrc"), &runtime_dir.join("include")])?;
    let dialect_files = collect_files(&[&dialect_dir.join("csrc"), &dialect_dir.join("include")])?;

    let mut files = FileMap::new();
    for (k, v) in &runtime_files {
        files.insert(k.clone(), v.clone());
    }
    for (k, v) in &dialect_files {
        files.insert(k.clone(), v.clone());
    }

    let runtime_keys: HashSet<String> = files
        .keys()
        .filter(|k| is_runtime_origin(k, dialect, &runtime_files, &dialect_files))
        .filter(|k| matches!(classify(k), FileKind::InternalHeader | FileKind::Source))
        .cloned()
        .collect();

    Ok(emit(
        &files,
        EmitMode::Full {
            dialect,
            runtime_keys: &runtime_keys,
        },
        omit_macros,
    ))
}

/// Classify a file's origin.
///
/// Primary signal: which collection the key was found in.
///   - only in runtime     → runtime origin
///   - only in dialect     → dialect origin
///   - in both (overlap)   → path-based fallback (happens when the caller
///     passes the same on-disk tree for both runtime and dialect dirs,
///     e.g. `tools/build-amalgamation` reading the source checkout).
///
/// Path-based fallback: keys under `csrc/<dialect>/` or `syntaqlite_<dialect>/`
/// are dialect; everything else is runtime.
fn is_runtime_origin(
    key: &str,
    dialect: &str,
    runtime_files: &FileMap,
    dialect_files: &FileMap,
) -> bool {
    let in_rt = runtime_files.contains_key(key);
    let in_dl = dialect_files.contains_key(key);
    match (in_rt, in_dl) {
        (true, false) => true,
        (false, true | false) => false,
        (true, true) => {
            let dialect_csrc = format!("csrc/{dialect}/");
            let dialect_pub = format!("syntaqlite_{dialect}/");
            !(key.starts_with(&dialect_csrc) || key.starts_with(&dialect_pub))
        }
    }
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
    ///
    /// `runtime_keys` identifies files that originate from the runtime tree.
    /// Runtime-origin sources are wrapped in an `#ifndef SYNTAQLITE_OMIT_RUNTIME`
    /// guard; runtime-origin headers are emitted before dialect-origin headers
    /// so dialect declarations can reference runtime types.
    Full {
        dialect: &'a str,
        runtime_keys: &'a HashSet<String>,
    },
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

fn section_expands(section: Section, kind: FileKind) -> bool {
    match section {
        Section::Header => kind == FileKind::PublicHeader,
        Section::ExtHeader => kind == FileKind::ExtHeader,
        Section::Source => matches!(kind, FileKind::InternalHeader | FileKind::ExtHeader),
    }
}

struct Emitter<'a> {
    files: &'a FileMap,
    seen: HashSet<String>,
    /// Keys that should have their emitted content wrapped in
    /// `#ifndef SYNTAQLITE_OMIT_RUNTIME`. Empty means no wrapping (used by
    /// all modes except `Full`).
    runtime_keys: &'a HashSet<String>,
}

impl<'a> Emitter<'a> {
    fn new(files: &'a FileMap) -> Self {
        Self::with_runtime_keys(files, EMPTY_KEYS.get_or_init(HashSet::new))
    }

    fn with_runtime_keys(files: &'a FileMap, runtime_keys: &'a HashSet<String>) -> Self {
        Self {
            files,
            seen: HashSet::new(),
            runtime_keys,
        }
    }

    /// Emit one file. Children (locally-included files allowed by `section`)
    /// are emitted first in post-order DFS, each via this same function.
    /// Then the file's own body is emitted, with resolved local includes
    /// suppressed (their content was written earlier).
    ///
    /// If the file's key is in `runtime_keys`, its own body — but *not* its
    /// hoisted children — is wrapped in `#ifndef SYNTAQLITE_OMIT_RUNTIME`.
    /// Shared headers (like extension SPI) reached from both runtime and
    /// dialect files thus land outside any wrap, so dedup never traps their
    /// declarations inside a stripped region.
    fn emit_file(&mut self, key: &str, out: &mut String, section: Section) {
        if !self.seen.insert(key.to_string()) {
            return;
        }
        let content = match self.files.get(key) {
            Some(c) => c.clone(),
            None => return,
        };

        for line in content.lines() {
            let trimmed = line.trim();
            if let Some(IncludeDirective::Quoted(path)) = parse_include_directive(trimmed)
                && self.files.contains_key(path)
                && section_expands(section, classify(path))
            {
                self.emit_file(path, out, section);
            }
        }

        let wrap_runtime = self.runtime_keys.contains(key);

        let _ = writeln!(out, "/* ======== begin: {key} ======== */");
        if wrap_runtime {
            out.push_str("#ifndef SYNTAQLITE_OMIT_RUNTIME\n");
        }

        let guard = detect_include_guard(&content);
        if let Some(ref g) = guard {
            let _ = write!(out, "#ifndef {g}\n#define {g}\n");
        }

        let mut lines: Vec<&str> = content.lines().collect();

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
                            // Resolved locally — children were hoisted above,
                            // so drop the directive regardless of whether it
                            // expanded.
                            continue;
                        }
                        if is_runtime_path(path) {
                            continue;
                        }
                    }
                    IncludeDirective::System | IncludeDirective::Other => {}
                }
            }

            out.push_str(line);
            out.push('\n');
        }

        if let Some(ref g) = guard {
            let _ = writeln!(out, "#endif  /* {g} */");
        }
        if wrap_runtime {
            out.push_str("#endif /* !SYNTAQLITE_OMIT_RUNTIME */\n");
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

/// Locate an internal header in the file map by suffix. Used to find the
/// dialect's `<dn>_parse.h` / `<dn>_tokenize.h` without depending on the
/// layout (which determines whether they live flat under `csrc/` or
/// nested under `csrc/<dn>/`).
fn find_internal_header<'a>(files: &'a FileMap, suffix: &str) -> Option<&'a str> {
    files
        .keys()
        .find(|k| k.ends_with(suffix) && k.starts_with("csrc/"))
        .map(String::as_str)
}

/// Emit the inline-dispatch macros for `dialect`, gated on the user's
/// opt-out / override flags. The macros expand to direct calls of the
/// dialect's Lemon-generated parser/tokenizer functions, whose decls
/// come from the parse/tokenize headers inlined immediately above.
fn emit_dispatch_macros(out: &mut String, dialect: &str) {
    let pascal = crate::util::pascal_case(dialect);
    let _ = writeln!(out, "// Inline-dispatch macros for the {dialect} dialect.");
    out.push_str(
        "#if !defined(SYNTAQLITE_NO_INLINE_DIALECT_DISPATCH) && \
         !defined(SYNTAQLITE_INLINE_DIALECT_DISPATCH)\n",
    );
    let _ = writeln!(
        out,
        "#define SYNQ_PARSER_ALLOC(d, m, c)   Synq{pascal}ParseAlloc(m, c)"
    );
    let _ = writeln!(
        out,
        "#define SYNQ_PARSER_INIT(d, p, c)    Synq{pascal}ParseInit(p, c)"
    );
    let _ = writeln!(
        out,
        "#define SYNQ_PARSER_FINALIZE(d, p)   Synq{pascal}ParseFinalize(p)"
    );
    let _ = writeln!(
        out,
        "#define SYNQ_PARSER_FREE(d, p, f)    Synq{pascal}ParseFree(p, f)"
    );
    let _ = writeln!(
        out,
        "#define SYNQ_PARSER_FEED(d, p, t, m) Synq{pascal}Parse(p, t, m)"
    );
    let _ = writeln!(
        out,
        "#define SYNQ_PARSER_TRACE(d, f, s)   Synq{pascal}ParseTrace(f, s)"
    );
    let _ = writeln!(
        out,
        "#define SYNQ_GET_TOKEN(env, z, t)    Synq{pascal}GetToken(env, z, t)"
    );
    out.push_str("#endif\n\n");
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
fn emit(files: &FileMap, mode: EmitMode, omit_macros: bool) -> AmalgamateOutput {
    let (guard, header_filename) = match &mode {
        EmitMode::DialectOnly { dialect: d, .. } | EmitMode::Full { dialect: d, .. } => (
            format!("SYNTAQLITE_{}_H", d.to_uppercase()),
            format!("syntaqlite_{d}.h"),
        ),
        EmitMode::RuntimeOnly => (
            "SYNTAQLITE_RUNTIME_H".to_string(),
            "syntaqlite_runtime.h".to_string(),
        ),
    };

    // Auto-inline dispatch kicks in for Full mode iff the dialect provides
    // a parser/tokenizer API header (`sqlite_parse.h`/`sqlite_tokenize.h`
    // — same file name for every dialect; the per-dialect symbol names
    // come from the contents). The amalgamator's contribution is (a) the
    // `SYNQ_AMALG_DIALECT` tag in the .h and (b) inlining the parse/tokenize
    // decls + a dispatch-macros block in the .c.
    let auto_inline = match &mode {
        EmitMode::Full { dialect, .. } => {
            let parse_key = find_internal_header(files, "/sqlite_parse.h");
            let tok_key = find_internal_header(files, "/sqlite_tokenize.h");
            match (parse_key, tok_key) {
                (Some(p), Some(t)) => Some((*dialect, p, t)),
                _ => None,
            }
        }
        _ => None,
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
                emit_omit_define(&mut header, "SYNTAQLITE_OMIT_SQLITE_API");
            }
            if omit_macros {
                emit_omit_define(&mut header, "SYNTAQLITE_OMIT_MACROS");
            }
            header.push_str("#ifndef SYNTAQLITE_RUNTIME_HEADER\n");
            let _ = writeln!(
                header,
                "#define SYNTAQLITE_RUNTIME_HEADER \"{runtime_header}\""
            );
            header.push_str("#endif\n");
            header.push_str("#include SYNTAQLITE_RUNTIME_HEADER\n\n");
        }
        EmitMode::Full { dialect, .. } if *dialect != "sqlite" => {
            emit_omit_define(&mut header, "SYNTAQLITE_OMIT_SQLITE_API");
            if omit_macros {
                emit_omit_define(&mut header, "SYNTAQLITE_OMIT_MACROS");
            }
        }
        EmitMode::Full { .. } if omit_macros => {
            emit_omit_define(&mut header, "SYNTAQLITE_OMIT_MACROS");
        }
        _ => {}
    }

    // Full amalgamation with auto-inline: tag this TU as a single-dialect
    // amalgamation so `syntaqlite/dialect.h` can compute the
    // `SYNTAQLITE_HAS_WITH_DIALECT_API` gate. Everything else
    // (`with_dialect` stripping, the pinned `create_<dialect>` wrappers)
    // lives in the C/H files and the dialect codegen output.
    if let Some((dialect, _, _)) = auto_inline {
        let _ = writeln!(header, "#define SYNQ_AMALG_DIALECT {dialect}\n");
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
            emit_omit_define(&mut source, "SYNTAQLITE_OMIT_SQLITE_API");
        }
        if omit_macros {
            emit_omit_define(&mut source, "SYNTAQLITE_OMIT_MACROS");
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
    } else if let EmitMode::Full { dialect, .. } = &mode
        && *dialect != "sqlite"
    {
        emit_omit_define(&mut source, "SYNTAQLITE_OMIT_SQLITE_API");
        if omit_macros {
            emit_omit_define(&mut source, "SYNTAQLITE_OMIT_MACROS");
        }
    } else if omit_macros {
        emit_omit_define(&mut source, "SYNTAQLITE_OMIT_MACROS");
    }
    let _ = write!(source, "#include \"{header_filename}\"\n\n");

    let mut s_emitter = match &mode {
        EmitMode::Full { runtime_keys, .. } => Emitter::with_runtime_keys(files, runtime_keys),
        _ => Emitter::new(files),
    };

    // Full amalgamation: inline the dialect's parse/tokenize API headers
    // (so their function decls are visible before any runtime source
    // references them), then emit the dispatch macros. Using the same
    // Emitter as the source pass means the seen set suppresses
    // re-emission when dialect sources include the same headers later.
    //
    // The macros are gated on the user's opt-out flags so a function-pointer
    // fallback can still be selected via `-DSYNTAQLITE_NO_INLINE_DIALECT_DISPATCH`
    // or `-DSYNTAQLITE_INLINE_DIALECT_DISPATCH=<custom-header>`. The
    // parse/tokenize decls remain unconditional — `dialect.c`'s struct
    // initializer always needs them.
    if let Some((dialect, parse_key, tok_key)) = &auto_inline {
        s_emitter.emit_file(parse_key, &mut source, Section::Source);
        s_emitter.emit_file(tok_key, &mut source, Section::Source);
        emit_dispatch_macros(&mut source, dialect);
    }

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

    fn runtime_set(keys: &[&str]) -> HashSet<String> {
        keys.iter().map(|s| (*s).to_string()).collect()
    }

    #[test]
    fn full_mode_wraps_each_runtime_source_individually() {
        let mut files = FileMap::new();
        files.insert(
            "csrc/parser.c".to_string(),
            "void runtime_api(void) {}\n".to_string(),
        );
        files.insert(
            "csrc/tokenizer.c".to_string(),
            "void runtime_tok(void) {}\n".to_string(),
        );
        files.insert(
            "csrc/sqlite/dialect.c".to_string(),
            "void sqlite_dialect_fn(void) {}\n".to_string(),
        );

        let runtime_keys = runtime_set(&["csrc/parser.c", "csrc/tokenizer.c"]);

        let out = emit(
            &files,
            EmitMode::Full {
                dialect: "sqlite",
                runtime_keys: &runtime_keys,
            },
            false,
        );

        let src = &out.source;

        let block_for = |key: &str| -> &str {
            let begin = src.find(&format!("begin: {key}")).expect("missing begin");
            let end = src.find(&format!("end: {key}")).expect("missing end");
            &src[begin..end]
        };

        let parser_block = block_for("csrc/parser.c");
        let tok_block = block_for("csrc/tokenizer.c");
        let dialect_block = block_for("csrc/sqlite/dialect.c");

        assert!(
            parser_block.contains("#ifndef SYNTAQLITE_OMIT_RUNTIME"),
            "runtime parser.c block should open the OMIT_RUNTIME guard"
        );
        assert!(
            parser_block.contains("#endif /* !SYNTAQLITE_OMIT_RUNTIME */"),
            "runtime parser.c block should close the OMIT_RUNTIME guard"
        );
        assert!(
            tok_block.contains("#ifndef SYNTAQLITE_OMIT_RUNTIME"),
            "runtime tokenizer.c block should open the OMIT_RUNTIME guard"
        );
        assert!(
            !dialect_block.contains("#ifndef SYNTAQLITE_OMIT_RUNTIME"),
            "dialect source block must not be wrapped"
        );
    }

    #[test]
    fn full_mode_shared_ext_header_survives_omit_runtime() {
        // Regression: when a runtime source and a dialect source both include
        // the same extension SPI header, dedup must not trap the header's
        // declarations inside the runtime source's OMIT_RUNTIME guard — the
        // dialect source needs those declarations after the guard strips the
        // runtime body.
        let mut files = FileMap::new();
        files.insert(
            "syntaqlite_dialect/shared.h".to_string(),
            "#ifndef SYNTAQLITE_DIALECT_SHARED_H\n\
             #define SYNTAQLITE_DIALECT_SHARED_H\n\
             typedef int shared_type;\n\
             #endif\n"
                .to_string(),
        );
        files.insert(
            "csrc/parser.c".to_string(),
            "#include \"syntaqlite_dialect/shared.h\"\n\
             void runtime_fn(shared_type x) { (void)x; }\n"
                .to_string(),
        );
        files.insert(
            "csrc/sqlite/dialect.c".to_string(),
            "#include \"syntaqlite_dialect/shared.h\"\n\
             void dialect_fn(shared_type x) { (void)x; }\n"
                .to_string(),
        );

        let runtime_keys = runtime_set(&["csrc/parser.c"]);

        let out = emit(
            &files,
            EmitMode::Full {
                dialect: "sqlite",
                runtime_keys: &runtime_keys,
            },
            false,
        );

        let src = &out.source;

        // Emitted exactly once (dedup).
        assert_eq!(
            src.matches("begin: syntaqlite_dialect/shared.h").count(),
            1,
            "shared header should be emitted exactly once"
        );

        // Simulate the preprocessor with OMIT_RUNTIME defined: every
        // `#ifndef SYNTAQLITE_OMIT_RUNTIME` block is stripped. The shared
        // header's declarations must survive.
        let stripped = strip_omit_runtime_blocks(src);
        assert!(
            stripped.contains("typedef int shared_type"),
            "shared header declarations must survive OMIT_RUNTIME; stripped output:\n{stripped}"
        );
        // Dialect function definition still present (it was never wrapped).
        assert!(
            stripped.contains("void dialect_fn("),
            "dialect source must survive OMIT_RUNTIME"
        );
        // Runtime implementation removed.
        assert!(
            !stripped.contains("void runtime_fn("),
            "runtime source must be stripped under OMIT_RUNTIME"
        );
    }

    /// Strip every `#ifndef SYNTAQLITE_OMIT_RUNTIME` ... `#endif /* !SYNTAQLITE_OMIT_RUNTIME */`
    /// block from `src`, simulating what the C preprocessor does when
    /// `SYNTAQLITE_OMIT_RUNTIME` is defined at compile time.
    fn strip_omit_runtime_blocks(src: &str) -> String {
        let mut out = String::new();
        let mut inside = false;
        for line in src.lines() {
            let t = line.trim();
            if !inside && t == "#ifndef SYNTAQLITE_OMIT_RUNTIME" {
                inside = true;
                continue;
            }
            if inside && t == "#endif /* !SYNTAQLITE_OMIT_RUNTIME */" {
                inside = false;
                continue;
            }
            if !inside {
                out.push_str(line);
                out.push('\n');
            }
        }
        out
    }

    #[test]
    fn is_runtime_origin_prefers_collection_membership() {
        let mut rt = FileMap::new();
        rt.insert("csrc/parser.c".to_string(), String::new());
        let mut dl = FileMap::new();
        dl.insert("csrc/dialect.c".to_string(), String::new());

        assert!(is_runtime_origin("csrc/parser.c", "sqlite", &rt, &dl));
        assert!(!is_runtime_origin("csrc/dialect.c", "sqlite", &rt, &dl));
    }

    #[test]
    fn is_runtime_origin_falls_back_to_path_when_dirs_overlap() {
        // Both collections contain the same keys — simulates the caller
        // passing the same on-disk tree for both runtime_dir and dialect_dir.
        let mut both = FileMap::new();
        both.insert("csrc/parser.c".to_string(), String::new());
        both.insert("csrc/sqlite/dialect.c".to_string(), String::new());
        both.insert("syntaqlite_sqlite/node.h".to_string(), String::new());
        both.insert("syntaqlite_dialect/arena.h".to_string(), String::new());

        assert!(is_runtime_origin("csrc/parser.c", "sqlite", &both, &both));
        assert!(!is_runtime_origin(
            "csrc/sqlite/dialect.c",
            "sqlite",
            &both,
            &both
        ));
        assert!(!is_runtime_origin(
            "syntaqlite_sqlite/node.h",
            "sqlite",
            &both,
            &both
        ));
        assert!(is_runtime_origin(
            "syntaqlite_dialect/arena.h",
            "sqlite",
            &both,
            &both
        ));
    }

    /// Helper: file map containing the dialect's parse/tokenize API
    /// headers (always named `sqlite_parse.h` / `sqlite_tokenize.h`, with
    /// dialect-specific symbols inside) that the amalgamator looks up to
    /// trigger auto-inline. `extra` adds further entries on top.
    fn flat_dialect_files(extra: &[(&str, &str)]) -> FileMap {
        let mut files = FileMap::new();
        files.insert(
            "csrc/sqlite_parse.h".to_string(),
            "void SynqSqliteParseAlloc(void);\n".to_string(),
        );
        files.insert(
            "csrc/sqlite_tokenize.h".to_string(),
            "void SynqSqliteGetToken(void);\n".to_string(),
        );
        for (k, v) in extra {
            files.insert((*k).to_string(), (*v).to_string());
        }
        files
    }

    fn nested_dialect_files(dialect: &str, extra: &[(&str, &str)]) -> FileMap {
        let mut files = FileMap::new();
        files.insert(
            format!("csrc/{dialect}/sqlite_parse.h"),
            "void SynqSqliteParseAlloc(void);\n".to_string(),
        );
        files.insert(
            format!("csrc/{dialect}/sqlite_tokenize.h"),
            "void SynqSqliteGetToken(void);\n".to_string(),
        );
        for (k, v) in extra {
            files.insert((*k).to_string(), (*v).to_string());
        }
        files
    }

    #[test]
    fn full_mode_emits_dispatch_macros_for_known_dialect() {
        // Trigger: both parse_h and tokenize_h are in the file map.
        let files = flat_dialect_files(&[("csrc/parser.c", "void runtime_api(void) {}\n")]);
        let runtime_keys = runtime_set(&["csrc/parser.c"]);
        let out = emit(
            &files,
            EmitMode::Full {
                dialect: "sqlite",
                runtime_keys: &runtime_keys,
            },
            false,
        );
        let src = &out.source;
        assert!(
            src.contains("#define SYNQ_PARSER_ALLOC(d, m, c)   SynqSqliteParseAlloc(m, c)"),
            "dispatch macros must be emitted with the Pascal-cased dialect name; got:\n{src}"
        );
        // Decls + macros must precede the runtime parser source.
        let parse_idx = src
            .find("begin: csrc/sqlite_parse.h")
            .expect("parse_h block missing");
        let macros_idx = src
            .find("Inline-dispatch macros")
            .expect("dispatch macros header missing");
        let parser_idx = src
            .find("begin: csrc/parser.c")
            .expect("parser.c block missing");
        assert!(
            parse_idx < macros_idx && macros_idx < parser_idx,
            "decls -> macros -> runtime sources, got indices {parse_idx} < {macros_idx} < {parser_idx}"
        );
    }

    #[test]
    fn full_mode_emits_dispatch_for_nested_layout() {
        // In-tree sqlite layout nests parse/tokenize under csrc/<dialect>/.
        let files = nested_dialect_files("sqlite", &[("csrc/parser.c", "")]);
        let runtime_keys = runtime_set(&["csrc/parser.c"]);
        let out = emit(
            &files,
            EmitMode::Full {
                dialect: "sqlite",
                runtime_keys: &runtime_keys,
            },
            false,
        );
        assert!(out.source.contains("SynqSqliteParseAlloc"));
    }

    #[test]
    fn full_mode_skips_auto_inline_when_dialect_headers_absent() {
        // No parse_h / tokenize_h in the map — amalgamator can't compose
        // a dispatch block, so it emits nothing.
        let mut files = FileMap::new();
        files.insert(
            "csrc/parser.c".to_string(),
            "void runtime_api(void) {}\n".to_string(),
        );
        let runtime_keys = runtime_set(&["csrc/parser.c"]);
        let out = emit(
            &files,
            EmitMode::Full {
                dialect: "perfetto",
                runtime_keys: &runtime_keys,
            },
            false,
        );
        assert!(
            !out.source.contains("SYNQ_PARSER_ALLOC"),
            "no dispatch macros without the dialect's parse/tokenize headers in the map"
        );
    }

    #[test]
    fn full_mode_tags_tu_with_amalg_dialect() {
        // The amalgamator's only contribution to the public API gate is a
        // single `#define SYNQ_AMALG_DIALECT <name>` in the header. All
        // downstream logic (hiding `_with_dialect`, exposing the pinned
        // wrappers) lives in the C headers / dialect codegen.
        let files = flat_dialect_files(&[]);
        let runtime_keys = HashSet::new();
        let out = emit(
            &files,
            EmitMode::Full {
                dialect: "sqlite",
                runtime_keys: &runtime_keys,
            },
            false,
        );
        assert!(
            out.header.contains("#define SYNQ_AMALG_DIALECT sqlite"),
            "Full-mode header must tag the TU with SYNQ_AMALG_DIALECT; got:\n{}",
            out.header
        );
    }

    #[test]
    fn dialect_only_mode_does_not_tag_amalg_dialect() {
        // DialectOnly doesn't auto-inline dispatch in its own TU, so the
        // `_with_dialect` API stays safe — no need to tag the TU.
        let mut files = flat_dialect_files(&[]);
        files.insert(
            "csrc/sqlite/dialect.c".to_string(),
            "void sqlite_dialect_fn(void) {}\n".to_string(),
        );
        let out = emit(
            &files,
            EmitMode::DialectOnly {
                dialect: "sqlite",
                runtime_header: "syntaqlite_runtime.h",
                ext_header: "syntaqlite_dialect.h",
            },
            false,
        );
        assert!(
            !out.header.contains("SYNQ_AMALG_DIALECT"),
            "DialectOnly header must not tag the TU"
        );
    }

    #[test]
    fn dialect_only_mode_does_not_auto_inline_dispatch() {
        let mut files = flat_dialect_files(&[]);
        files.insert(
            "csrc/sqlite/dialect.c".to_string(),
            "void sqlite_dialect_fn(void) {}\n".to_string(),
        );
        let out = emit(
            &files,
            EmitMode::DialectOnly {
                dialect: "sqlite",
                runtime_header: "syntaqlite_runtime.h",
                ext_header: "syntaqlite_dialect.h",
            },
            false,
        );
        assert!(
            !out.source.contains("SYNQ_PARSER_ALLOC"),
            "dialect-only mode is consumed by an external runtime TU and must \
             not auto-inline dispatch in its own .c"
        );
    }
}
