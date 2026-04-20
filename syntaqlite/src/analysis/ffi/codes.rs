// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! C-ABI numeric codes for severity, diagnostic kind, check level,
//! function category, and arity spec. Must match the definitions in
//! `syntaqlite/include/syntaqlite/analysis.h`.

use crate::analysis::CheckLevel;
use crate::analysis::catalog::{AritySpec, FunctionCategory};
use crate::analysis::diagnostics::{DiagnosticMessage, Severity};

pub(super) const SEVERITY_ERROR: u32 = 0;
pub(super) const SEVERITY_WARNING: u32 = 1;
pub(super) const SEVERITY_INFO: u32 = 2;
pub(super) const SEVERITY_HINT: u32 = 3;

pub(super) fn severity_to_c(s: Severity) -> u32 {
    match s {
        Severity::Error => SEVERITY_ERROR,
        Severity::Warning => SEVERITY_WARNING,
        Severity::Info => SEVERITY_INFO,
        Severity::Hint => SEVERITY_HINT,
    }
}

pub(crate) const DIAG_CODE_PARSE_ERROR: u32 = 0;
pub(crate) const DIAG_CODE_UNKNOWN_TABLE: u32 = 1;
pub(crate) const DIAG_CODE_UNKNOWN_COLUMN: u32 = 2;
pub(crate) const DIAG_CODE_UNKNOWN_FUNCTION: u32 = 3;
pub(crate) const DIAG_CODE_UNKNOWN_MODULE: u32 = 4;
pub(crate) const DIAG_CODE_FUNCTION_ARITY: u32 = 5;
pub(crate) const DIAG_CODE_CTE_COLUMN_COUNT_MISMATCH: u32 = 6;

pub(super) fn diagnostic_code_to_c(msg: &DiagnosticMessage) -> u32 {
    match msg {
        DiagnosticMessage::ParseError(_) => DIAG_CODE_PARSE_ERROR,
        DiagnosticMessage::UnknownTable { .. } => DIAG_CODE_UNKNOWN_TABLE,
        DiagnosticMessage::UnknownColumn { .. } => DIAG_CODE_UNKNOWN_COLUMN,
        DiagnosticMessage::UnknownFunction { .. } => DIAG_CODE_UNKNOWN_FUNCTION,
        DiagnosticMessage::UnknownModule { .. } => DIAG_CODE_UNKNOWN_MODULE,
        DiagnosticMessage::FunctionArity { .. } => DIAG_CODE_FUNCTION_ARITY,
        DiagnosticMessage::CteColumnCountMismatch { .. } => DIAG_CODE_CTE_COLUMN_COUNT_MISMATCH,
    }
}

pub(crate) const SYNTAQLITE_CHECK_ALLOW: u32 = 0;
pub(crate) const SYNTAQLITE_CHECK_WARN: u32 = 1;
pub(crate) const SYNTAQLITE_CHECK_DENY: u32 = 2;

pub(super) fn check_level_from_c(level: u32) -> Option<CheckLevel> {
    match level {
        SYNTAQLITE_CHECK_ALLOW => Some(CheckLevel::Allow),
        SYNTAQLITE_CHECK_WARN => Some(CheckLevel::Warn),
        SYNTAQLITE_CHECK_DENY => Some(CheckLevel::Deny),
        _ => None,
    }
}

pub(crate) const SYNTAQLITE_FUNCTION_SCALAR: u32 = 0;
pub(crate) const SYNTAQLITE_FUNCTION_AGGREGATE: u32 = 1;
pub(crate) const SYNTAQLITE_FUNCTION_WINDOW: u32 = 2;

pub(crate) const SYNTAQLITE_ARITY_EXACT: u32 = 0;
pub(crate) const SYNTAQLITE_ARITY_AT_LEAST: u32 = 1;
pub(crate) const SYNTAQLITE_ARITY_ANY: u32 = 2;

pub(super) fn function_category_from_c(category: u32) -> Option<FunctionCategory> {
    match category {
        SYNTAQLITE_FUNCTION_SCALAR => Some(FunctionCategory::Scalar),
        SYNTAQLITE_FUNCTION_AGGREGATE => Some(FunctionCategory::Aggregate),
        SYNTAQLITE_FUNCTION_WINDOW => Some(FunctionCategory::Window),
        _ => None,
    }
}

pub(super) fn arity_spec_from_c(kind: u32, value: u32) -> Option<AritySpec> {
    match kind {
        SYNTAQLITE_ARITY_EXACT => Some(AritySpec::Exact(value as usize)),
        SYNTAQLITE_ARITY_AT_LEAST => Some(AritySpec::AtLeast(value as usize)),
        SYNTAQLITE_ARITY_ANY => Some(AritySpec::Any),
        _ => None,
    }
}
