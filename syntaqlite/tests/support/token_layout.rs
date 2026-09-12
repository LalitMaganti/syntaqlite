//! Private harness wiring for the isolated prototype; not part of the library API.
#![allow(dead_code)]
use syntaqlite::fmt::{FormatConfig, KeywordCase};

// Compile the existing renderer and prototype directly, without copying either.
#[path = "../../src/fmt/doc.rs"]
mod doc;
#[path = "../../src/fmt/token_layout_prototype.rs"]
pub(crate) mod token_layout_prototype;
