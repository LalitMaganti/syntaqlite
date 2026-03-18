// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::fs;

use zed_extension_api::{self as zed, settings::LspSettings, LanguageServerId, Result};

const SERVER_NAME: &str = "syntaqlite";

/// GitHub release asset names by platform.
fn github_asset_name(platform: zed::Os, arch: zed::Architecture) -> Result<&'static str> {
    match (platform, arch) {
        (zed::Os::Mac, zed::Architecture::Aarch64) => Ok("syntaqlite-macos-arm64.tar.gz"),
        (zed::Os::Mac, zed::Architecture::X8664) => Ok("syntaqlite-macos-x64.tar.gz"),
        (zed::Os::Linux, zed::Architecture::Aarch64) => Ok("syntaqlite-linux-arm64.tar.gz"),
        (zed::Os::Linux, zed::Architecture::X8664) => Ok("syntaqlite-linux-x64.tar.gz"),
        _ => Err(format!("unsupported platform: {platform:?} {arch:?}")),
    }
}

/// Binary name inside the extracted archive.
fn binary_name() -> &'static str {
    "syntaqlite"
}

struct SyntaqliteExtension {
    /// Path to the cached binary (set after first successful install/find).
    cached_binary_path: Option<String>,
}

impl SyntaqliteExtension {
    /// Ensure the syntaqlite binary is available, downloading if necessary.
    /// Returns the absolute path to the binary.
    fn language_server_binary_path(
        &mut self,
        language_server_id: &LanguageServerId,
        worktree: &zed::Worktree,
    ) -> Result<String> {
        // 1. If we already resolved the binary this session, reuse it.
        if let Some(path) = &self.cached_binary_path {
            if fs::metadata(path).map_or(false, |m| m.is_file()) {
                return Ok(path.clone());
            }
        }

        // 2. Check user-configured path via LSP settings.
        let lsp_settings = LspSettings::for_worktree(SERVER_NAME, worktree)?;
        if let Some(binary_settings) = lsp_settings.binary.as_ref() {
            if let Some(path) = binary_settings.path.as_ref() {
                self.cached_binary_path = Some(path.clone());
                return Ok(path.clone());
            }
        }

        // 3. Check PATH.
        if let Some(path) = worktree.which(binary_name()) {
            self.cached_binary_path = Some(path.clone());
            return Ok(path);
        }

        // 4. Download from GitHub releases.
        let release = zed::latest_github_release(
            "LalitMaganti/syntaqlite",
            zed::GithubReleaseOptions {
                require_assets: true,
                pre_release: false,
            },
        )?;

        let (platform, arch) = zed::current_platform();
        let asset_name = github_asset_name(platform, arch)?;

        let asset = release
            .assets
            .iter()
            .find(|a| a.name == asset_name)
            .ok_or_else(|| {
                format!("no release asset '{asset_name}' found in release {}", release.version)
            })?;

        let version_dir = format!("syntaqlite-{}", release.version);
        let binary_path = format!("{version_dir}/{}", binary_name());

        if !fs::metadata(&binary_path).map_or(false, |m| m.is_file()) {
            zed::set_language_server_installation_status(
                language_server_id,
                &zed::LanguageServerInstallationStatus::Downloading,
            );

            zed::download_file(
                &asset.download_url,
                &version_dir,
                zed::DownloadedFileType::GzipTar,
            )
            .map_err(|e| format!("failed to download {asset_name}: {e}"))?;

            // The archive extracts directly as the binary.
            let entries =
                fs::read_dir(&version_dir).map_err(|e| format!("read dir failed: {e}"))?;
            for entry in entries.flatten() {
                let name = entry.file_name();
                if name.to_string_lossy().starts_with("syntaqlite") {
                    let dest = format!("{version_dir}/{}", binary_name());
                    if entry.path().to_string_lossy() != dest {
                        fs::rename(entry.path(), &dest)
                            .map_err(|e| format!("rename failed: {e}"))?;
                    }
                    break;
                }
            }

            zed::make_file_executable(&binary_path)?;

            zed::set_language_server_installation_status(
                language_server_id,
                &zed::LanguageServerInstallationStatus::None,
            );
        }

        self.cached_binary_path = Some(binary_path.clone());
        Ok(binary_path)
    }
}

impl zed::Extension for SyntaqliteExtension {
    fn new() -> Self {
        Self {
            cached_binary_path: None,
        }
    }

    fn language_server_command(
        &mut self,
        language_server_id: &LanguageServerId,
        worktree: &zed::Worktree,
    ) -> Result<zed::Command> {
        let binary_path = self.language_server_binary_path(language_server_id, worktree)?;

        // Build args: optional --config, then "lsp".
        let mut args = Vec::new();

        // Check for user-provided config path in LSP initialization_options.
        let lsp_settings = LspSettings::for_worktree(SERVER_NAME, worktree)?;
        if let Some(init_opts) = lsp_settings.initialization_options.as_ref() {
            if let Some(config_path) = init_opts.get("config").and_then(|v| v.as_str()) {
                args.push("--config".to_string());
                args.push(config_path.to_string());
            }
        }

        args.push("lsp".to_string());

        Ok(zed::Command {
            command: binary_path,
            args,
            env: Vec::new(),
        })
    }
}

zed::register_extension!(SyntaqliteExtension);
