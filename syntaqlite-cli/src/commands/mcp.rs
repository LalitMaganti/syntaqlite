// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `mcp` subcommand — Model Context Protocol server exposing format, parse,
//! and validate tools over stdio.

#![expect(
    clippy::needless_pass_by_value,
    reason = "rmcp #[tool(aggr)] requires by-value params"
)]

use std::fmt::Write;
use std::ops::Deref;

use rmcp::model::{Implementation, ProtocolVersion, ServerCapabilities, ServerInfo};
use rmcp::schemars;
use rmcp::{ServerHandler, tool};
use serde::Deserialize;
use syntaqlite::any::{AnyDialect, AnyParser, ParseOutcome};
use syntaqlite::fmt::KeywordCase;
use syntaqlite::{FormatConfig, Formatter};

#[derive(Debug, Deserialize, schemars::JsonSchema)]
struct FormatParams {
    /// The SQL to format.
    sql: String,
    /// Maximum line width (default 80).
    #[serde(default = "default_line_width")]
    line_width: usize,
    /// Keyword casing — "upper" or "lower" (default "upper").
    #[serde(default = "default_keyword_case")]
    keyword_case: String,
    /// Whether to append trailing semicolons (default true).
    #[serde(default = "default_true")]
    semicolons: bool,
}

fn default_line_width() -> usize {
    80
}
fn default_keyword_case() -> String {
    "upper".to_string()
}
fn default_true() -> bool {
    true
}

#[derive(Debug, Deserialize, schemars::JsonSchema)]
struct SqlParams {
    /// The SQL to process.
    sql: String,
}

#[derive(Clone)]
struct McpServer {
    dialect: AnyDialect,
}

#[tool(tool_box)]
impl McpServer {
    fn new(dialect: AnyDialect) -> Self {
        Self { dialect }
    }

    /// Format a SQL string.
    #[tool(description = "Format a SQL string")]
    fn format_sql(&self, #[tool(aggr)] params: FormatParams) -> Result<String, String> {
        let case = match params.keyword_case.as_str() {
            "lower" => KeywordCase::Lower,
            _ => KeywordCase::Upper,
        };
        let config = FormatConfig::default()
            .with_line_width(params.line_width)
            .with_keyword_case(case)
            .with_semicolons(params.semicolons);

        Formatter::with_dialect_config(self.dialect.clone(), &config)
            .format(&params.sql)
            .map_err(|e| format!("Error: {e}"))
    }

    /// Parse a SQL string and return its AST dump.
    #[tool(description = "Parse a SQL string and return its AST dump")]
    fn parse_sql(&self, #[tool(aggr)] params: SqlParams) -> String {
        let parser = AnyParser::new(self.dialect.deref().clone());
        let mut session = parser.parse(&params.sql);
        let mut output = String::new();
        let mut count = 0u64;
        let mut errors = Vec::new();

        loop {
            match session.next() {
                ParseOutcome::Ok(stmt) => {
                    if count > 0 {
                        output.push_str("----\n");
                    }
                    stmt.dump(&mut output, 0);
                    count += 1;
                }
                ParseOutcome::Err(err) => {
                    errors.push(err.message().to_string());
                }
                ParseOutcome::Done => break,
            }
        }

        if !errors.is_empty() {
            write!(output, "\nErrors:\n{}", errors.join("\n")).ok();
        }

        output
    }

    /// Check whether a SQL string is syntactically valid.
    ///
    /// Returns JSON with `valid` (bool) and `errors` (string, empty if valid).
    #[tool(
        description = "Check whether a SQL string is syntactically valid. Returns JSON with `valid` (bool) and `errors` (string)."
    )]
    fn validate_sql(&self, #[tool(aggr)] params: SqlParams) -> String {
        let parser = AnyParser::new(self.dialect.deref().clone());
        let mut session = parser.parse(&params.sql);
        let mut errors = Vec::new();

        loop {
            match session.next() {
                ParseOutcome::Ok(_) => {}
                ParseOutcome::Err(err) => {
                    errors.push(err.message().to_string());
                }
                ParseOutcome::Done => break,
            }
        }

        let response = serde_json::json!({
            "valid": errors.is_empty(),
            "errors": errors.join("\n"),
        });

        response.to_string()
    }
}

#[tool(tool_box)]
impl ServerHandler for McpServer {
    fn get_info(&self) -> ServerInfo {
        ServerInfo {
            protocol_version: ProtocolVersion::default(),
            capabilities: ServerCapabilities::builder().enable_tools().build(),
            server_info: Implementation {
                name: "syntaqlite".into(),
                version: env!("CARGO_PKG_VERSION").into(),
            },
            instructions: Some(
                "SQL formatting, parsing, and validation tools for SQLite SQL.".into(),
            ),
        }
    }
}

pub(crate) fn run(dialect: AnyDialect) -> Result<(), String> {
    tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()
        .map_err(|e| format!("failed to create tokio runtime: {e}"))?
        .block_on(async {
            use rmcp::ServiceExt;
            let server = McpServer::new(dialect);
            let service = server
                .serve(rmcp::transport::stdio())
                .await
                .map_err(|e| format!("MCP server error: {e}"))?;
            service
                .waiting()
                .await
                .map_err(|e| format!("MCP server error: {e}"))?;
            Ok(())
        })
}
