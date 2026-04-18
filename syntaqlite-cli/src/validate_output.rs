// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! JSON DTOs for the `validate` subcommand.
//!
//! `schema_version` is pinned at `0` during the pre-stable period. Bump when
//! making a compatible schema change; document breaking schema changes in
//! `CHANGELOG.md`.

use serde::Serialize;
use syntaqlite::Diagnostic;
use syntaqlite::semantic::{Help, Severity};

pub(crate) const SCHEMA_VERSION: u32 = 0;

#[derive(Serialize)]
#[serde(rename_all = "lowercase")]
pub(crate) enum JsonSeverity {
    Error,
    Warning,
    Info,
    Hint,
}

impl From<Severity> for JsonSeverity {
    fn from(s: Severity) -> Self {
        match s {
            Severity::Error => JsonSeverity::Error,
            Severity::Warning => JsonSeverity::Warning,
            Severity::Info => JsonSeverity::Info,
            Severity::Hint => JsonSeverity::Hint,
        }
    }
}

#[derive(Serialize)]
pub(crate) struct DiagnosticRecord<'a> {
    pub kind: &'static str,
    pub schema_version: u32,
    pub file: &'a str,
    pub severity: JsonSeverity,
    pub message: String,
    pub start_offset: usize,
    pub end_offset: usize,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub help: Option<String>,
}

impl<'a> DiagnosticRecord<'a> {
    pub(crate) fn from_diag(file: &'a str, diag: &Diagnostic) -> Self {
        let help = diag.help().map(|h| match h {
            Help::Suggestion(s) => s.clone(),
        });
        Self {
            kind: "diagnostic",
            schema_version: SCHEMA_VERSION,
            file,
            severity: diag.severity().into(),
            message: diag.message().to_string(),
            start_offset: diag.start_offset(),
            end_offset: diag.end_offset(),
            help,
        }
    }
}
