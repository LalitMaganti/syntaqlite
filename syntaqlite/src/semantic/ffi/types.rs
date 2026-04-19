// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! C-compatible struct definitions mirrored from `syntaqlite/include/syntaqlite/validation.h`.

use std::ffi::c_char;

#[repr(C)]
pub struct SyntaqliteDiagnostic {
    pub severity: u32,
    pub message: *const c_char,
    pub start_offset: u32,
    pub end_offset: u32,
    pub kind_code: u32,
}

#[repr(C)]
pub struct SyntaqliteRelationDef {
    pub name: *const c_char,
    pub columns: *const *const c_char,
    pub column_count: u32,
}

#[repr(C)]
pub struct SyntaqliteColumnOrigin {
    pub table: *const c_char,
    pub column: *const c_char,
}

#[repr(C)]
pub struct SyntaqliteColumnLineage {
    pub name: *const c_char,
    pub index: u32,
    pub origin: SyntaqliteColumnOrigin,
}

#[repr(C)]
pub struct SyntaqliteRelationAccess {
    pub name: *const c_char,
    pub kind: u32,
}

#[repr(C)]
pub struct SyntaqlitePhysicalTableAccess {
    pub name: *const c_char,
}

#[repr(C)]
pub struct SyntaqliteDefinedRelation {
    pub name: *const c_char,
    pub is_view: u32,
}

#[repr(C)]
pub struct SyntaqliteUnexpandedView {
    pub name: *const c_char,
}
