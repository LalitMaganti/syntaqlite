// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! JSON DTOs for the `lineage` subcommand.
//!
//! `schema_version` is pinned at `0` during the pre-stable period. Bump when
//! making a compatible schema change; document breaking schema changes in
//! `CHANGELOG.md`.

use serde::Serialize;

pub(crate) const SCHEMA_VERSION: u32 = 0;

#[derive(Serialize)]
#[serde(rename_all = "lowercase")]
pub(crate) enum Status {
    Complete,
    Partial,
}

#[derive(Serialize)]
pub(crate) struct JsonOrigin {
    pub table: String,
    pub column: String,
}

#[derive(Serialize)]
pub(crate) struct JsonColumn {
    pub name: String,
    pub index: u32,
    pub origin: Option<JsonOrigin>,
}

#[derive(Serialize)]
#[serde(rename_all = "lowercase")]
pub(crate) enum JsonRelationKind {
    Table,
    View,
}

#[derive(Serialize)]
pub(crate) struct JsonRelation {
    pub name: String,
    pub kind: JsonRelationKind,
}

#[derive(Serialize)]
pub(crate) struct JsonPhysicalTable {
    pub name: String,
}

#[derive(Serialize)]
#[serde(rename_all = "lowercase")]
pub(crate) enum JsonTargetKind {
    Table,
    View,
}

#[derive(Serialize)]
pub(crate) struct JsonTarget {
    pub name: String,
    pub kind: JsonTargetKind,
}

/// Partial-reason entry. For now only `unexpanded_view` exists; additional
/// codes will be added as more causes of partial lineage surface.
#[derive(Serialize)]
#[serde(tag = "code", rename_all = "snake_case")]
pub(crate) enum JsonPartialReason {
    UnexpandedView { view: String },
}

#[derive(Serialize)]
pub(crate) struct LineageRecord {
    pub kind: &'static str,
    pub schema_version: u32,
    pub file: String,
    pub statement_index: u32,
    pub status: Status,
    pub partial_reasons: Vec<JsonPartialReason>,
    pub target: Option<JsonTarget>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub columns: Option<Vec<JsonColumn>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub relations: Option<Vec<JsonRelation>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub physical_tables: Option<Vec<JsonPhysicalTable>>,
}

#[derive(Serialize)]
#[serde(rename_all = "lowercase")]
pub(crate) enum ErrorStage {
    Parse,
    Validate,
}

#[derive(Serialize)]
pub(crate) struct ErrorRecord {
    pub kind: &'static str,
    pub schema_version: u32,
    pub file: String,
    pub statement_index: u32,
    pub stage: ErrorStage,
    pub message: String,
}
