// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `lsp` subcommand (language server over stdio).

use syntaqlite::AnalysisConfig;
use syntaqlite::any::AnyDialect;

use crate::config::{self, ConfigMode};
use crate::util::{build_check_config, build_format_config, build_schema_catalog};

pub(crate) fn run(dialect: AnyDialect, config: &ConfigMode<'_>) -> Result<(), String> {
    let mut lsp_config = syntaqlite::lsp::LspConfig::default();

    if let Some((project_config, config_dir)) = config::resolve(config) {
        eprintln!(
            "syntaqlite-lsp: found config at {}",
            config_dir.join("syntaqlite.toml").display()
        );

        lsp_config.format_config = Some(build_format_config(
            Some(&project_config.format),
            None,
            None,
            None,
            None,
        ));

        let default_catalog = if let Some(ref schema) = project_config.schema {
            let paths: Vec<String> = schema
                .iter()
                .map(|s| config_dir.join(s).to_string_lossy().into_owned())
                .collect();
            match build_schema_catalog(&dialect, &paths) {
                Ok(catalog) => Some(catalog),
                Err(e) => {
                    eprintln!("syntaqlite-lsp: failed to load default schema: {e}");
                    None
                }
            }
        } else {
            None
        };

        let mut schema_entries = Vec::new();
        for (glob_str, schema_files) in &project_config.schemas {
            let pattern = match glob::Pattern::new(glob_str) {
                Ok(p) => p,
                Err(e) => {
                    eprintln!("syntaqlite-lsp: bad glob pattern {glob_str:?}: {e}");
                    continue;
                }
            };
            let paths: Vec<String> = schema_files
                .iter()
                .map(|s| config_dir.join(s).to_string_lossy().into_owned())
                .collect();
            match build_schema_catalog(&dialect, &paths) {
                Ok(catalog) => schema_entries.push((pattern, catalog)),
                Err(e) => eprintln!("syntaqlite-lsp: failed to load schema for {glob_str:?}: {e}"),
            }
        }

        let has_schema = default_catalog.is_some() || !schema_entries.is_empty();
        if has_schema {
            lsp_config.schema_map = Some(syntaqlite::lsp::SchemaMap::new(
                config_dir.clone(),
                default_catalog,
                schema_entries,
            ));
        }

        match build_check_config(has_schema, Some(&project_config.checks), &[], &[], &[]) {
            Ok(checks) => {
                lsp_config.analysis_config = Some(AnalysisConfig::default().with_checks(checks));
            }
            Err(e) => eprintln!("syntaqlite-lsp: invalid [checks] config: {e}"),
        }
    }

    syntaqlite::lsp::LspServer::run_with_config(dialect, lsp_config)
        .map_err(|e| format!("LSP error: {e}"))
}
