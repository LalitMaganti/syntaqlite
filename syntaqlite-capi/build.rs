// Copyright 2026 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Exports libsyntaqlite's whole public C API from the shared library.
//!
//! A Rust cdylib exports only Rust-defined `no_mangle` functions. The parser
//! and tokenizer are C, linked in from `syntaqlite-syntax`'s static library,
//! so their functions are hidden, and dropped entirely if nothing in Rust
//! calls them. This passes the linker every function in `exports.txt`, both
//! to pull it in and to export it.

use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

fn main() {
    println!("cargo:rerun-if-changed=exports.txt");
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR"));
    let exports = fs::read_to_string(manifest_dir.join("exports.txt")).expect("read exports.txt");
    let symbols: Vec<&str> = exports
        .lines()
        .map(str::trim)
        .filter(|l| !l.is_empty() && !l.starts_with('#'))
        .collect();

    let os = env::var("CARGO_CFG_TARGET_OS").unwrap_or_default();
    let target_env = env::var("CARGO_CFG_TARGET_ENV").unwrap_or_default();
    match os.as_str() {
        "macos" | "ios" => {
            for s in &symbols {
                link_arg(&format!("-Wl,-u,_{s}"));
                link_arg(&format!("-Wl,-exported_symbol,_{s}"));
            }
        }
        "windows" if target_env == "msvc" => {
            for s in &symbols {
                link_arg(&format!("/INCLUDE:{s}"));
                link_arg(&format!("/EXPORT:{s}"));
            }
        }
        "linux" | "android" | "freebsd" => export_elf(&symbols),
        // Emscripten side modules (Pyodide) and other targets use their
        // own export rules.
        _ => {}
    }
}

/// rustc hides every symbol not in its own version script. GNU ld rejects a
/// second version script, but lld merges them, so link with lld: the copy
/// shipped with Rust if there is one, otherwise the system's.
fn export_elf(symbols: &[&str]) {
    let out = PathBuf::from(env::var("OUT_DIR").expect("OUT_DIR"));
    let script = out.join("libsyntaqlite.map");
    let mut body = String::from("{\n  global:\n");
    for s in symbols {
        body.push_str("    ");
        body.push_str(s);
        body.push_str(";\n");
    }
    body.push_str("};\n");
    fs::write(&script, body).expect("write version script");

    if let Some(dir) = rust_lld_dir() {
        link_arg(&format!("-B{}", dir.display()));
    }
    link_arg("-fuse-ld=lld");
    link_arg(&format!("-Wl,--version-script={}", script.display()));
    for s in symbols {
        link_arg(&format!("-Wl,--undefined={s}"));
    }
}

/// The `gcc-ld` directory holding the `ld.lld` that ships with the Rust
/// toolchain, as rustc itself uses on `x86_64` Linux.
fn rust_lld_dir() -> Option<PathBuf> {
    let rustc = env::var("RUSTC").unwrap_or_else(|_| "rustc".to_string());
    let host = env::var("HOST").ok()?;
    let output = Command::new(rustc)
        .args(["--print", "sysroot"])
        .output()
        .ok()?;
    let sysroot = String::from_utf8(output.stdout).ok()?;
    let dir = Path::new(sysroot.trim())
        .join("lib/rustlib")
        .join(host)
        .join("bin/gcc-ld");
    dir.join("ld.lld").exists().then_some(dir)
}

fn link_arg(arg: &str) {
    println!("cargo:rustc-cdylib-link-arg={arg}");
}
