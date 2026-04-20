// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Library entry point for the `syntaqlite` CLI.
//!
//! Downstream crates can build their own CLI binary that pre-specifies a
//! dialect by implementing [`CliApp`] and calling [`run`]. The default
//! `syntaqlite` binary in this crate is a thin wrapper that does exactly that
//! with the bundled `SQLite` dialect.

use std::path::PathBuf;

use clap::CommandFactory;
use syntaqlite::any::AnyDialect;

mod cli;
mod commands;
mod config;
mod util;

use cli::{Cli, Command};
use config::{ConfigMode, ProjectConfig};

/// Configuration trait for a `syntaqlite` CLI binary.
///
/// Implementors describe the program name, default dialect, and which dialect
/// override surfaces are exposed. Default methods produce the stock CLI's
/// behaviour minus the dialect itself; downstream wrappers typically override
/// [`CliApp::name`] and [`CliApp::default_dialect`].
///
/// All methods have defaults so adding new ones is non-breaking.
pub trait CliApp {
    /// Program name shown in `--help` and used as `argv[0]`.
    fn name(&self) -> &'static str;

    /// One-line description for `--help`.
    fn about(&self) -> &'static str {
        "SQL formatting and analysis tools"
    }

    /// Version string for `--version`.
    ///
    /// The default returns this crate's version. Wrapper crates should
    /// override to report their own version.
    fn version(&self) -> &'static str {
        env!("CARGO_PKG_VERSION")
    }

    /// Dialect baked into this binary. `None` means the user must supply one
    /// via `--dialect` (which requires [`CliApp::allow_dialect_override`]).
    fn default_dialect(&self) -> Option<AnyDialect> {
        None
    }

    /// Whether `--dialect` / `--dialect-name` are exposed to end users.
    /// When the `dynload` feature is disabled the flags are absent
    /// regardless of this setting.
    fn allow_dialect_override(&self) -> bool {
        false
    }

    /// Whether `--sqlite-version` / `--sqlite-cflag` are exposed.
    /// Set to `true` only for SQLite-derived dialects where these knobs apply.
    fn allow_sqlite_tuning(&self) -> bool {
        false
    }
}

/// Run the CLI with the given app configuration.
///
/// Reads `argv` from the process. On error, prints the message to stderr and
/// exits with status 1. On `--help` / `--version`, exits 0.
pub fn run<A: CliApp>(app: &A) {
    use clap::FromArgMatches;

    let cmd = build_command(app);
    let matches = cmd
        .try_get_matches_from(std::env::args())
        .unwrap_or_else(|e| e.exit());
    let cli = Cli::from_arg_matches(&matches).unwrap_or_else(|e| e.exit());

    if let Err(e) = enforce_visibility(app, &cli) {
        eprintln!("error: {e}");
        std::process::exit(2);
    }

    if let Err(e) = dispatch(cli, app.default_dialect()) {
        eprintln!("error: {e}");
        std::process::exit(1);
    }
}

/// Build the [`clap::Command`] for the given app, applying runtime visibility
/// rules from the [`CliApp`] trait.
fn build_command<A: CliApp>(app: &A) -> clap::Command {
    let name = app.name();
    let cmd = Cli::command()
        .name(name)
        .bin_name(name)
        .about(app.about())
        .version(app.version());

    let cmd = if app.allow_sqlite_tuning() {
        cmd
    } else {
        cmd.mut_arg("sqlite_version", |a: clap::Arg| a.hide(true))
            .mut_arg("sqlite_cflag", |a: clap::Arg| a.hide(true))
    };

    #[cfg(feature = "dynload")]
    let cmd = if app.allow_dialect_override() {
        cmd
    } else {
        cmd.mut_arg("dialect_path", |a: clap::Arg| a.hide(true))
            .mut_arg("dialect_name", |a: clap::Arg| a.hide(true))
    };

    cmd
}

/// Reject hidden flags that the trait says aren't allowed but were still passed.
fn enforce_visibility<A: CliApp>(app: &A, cli: &Cli) -> Result<(), String> {
    if !app.allow_sqlite_tuning() {
        if cli.sqlite_version.is_some() {
            return Err("--sqlite-version is not supported by this CLI".to_owned());
        }
        if !cli.sqlite_cflag.is_empty() {
            return Err("--sqlite-cflag is not supported by this CLI".to_owned());
        }
    }

    #[cfg(feature = "dynload")]
    if !app.allow_dialect_override() && cli.dialect_path.is_some() {
        return Err("--dialect is not supported by this CLI".to_owned());
    }

    Ok(())
}

// ── Dispatch ──────────────────────────────────────────────────────────────

fn require_dialect(dialect: Option<AnyDialect>) -> Result<AnyDialect, String> {
    dialect.ok_or_else(|| {
        "this command requires a dialect; rebuild with --features=bundled-sqlite-dialect \
         or pass --dialect (with --features=dynload)"
            .to_string()
    })
}

/// Resolve the dialect, merge sqlite tuning from CLI + config file, then
/// hand off to the appropriate command module.
fn dispatch(cli: Cli, default_dialect: Option<AnyDialect>) -> Result<(), String> {
    // Destructure upfront so the global-arg fields can be borrowed while
    // `command` is later moved into the match.
    let Cli {
        config,
        no_config,
        #[cfg(feature = "dynload")]
        dialect_path,
        #[cfg(feature = "dynload")]
        dialect_name,
        sqlite_version,
        sqlite_cflag,
        command,
    } = cli;

    let config_mode = if no_config {
        ConfigMode::Disabled
    } else if let Some(ref path) = config {
        ConfigMode::Explicit(path)
    } else {
        ConfigMode::Discover
    };

    #[cfg(feature = "dynload")]
    let base = match dialect_path {
        Some(path) => Some(
            AnyDialect::load(&path, dialect_name.as_deref()).unwrap_or_else(|e| {
                eprintln!("error: {e}");
                std::process::exit(1);
            }),
        ),
        None => default_dialect,
    };
    #[cfg(not(feature = "dynload"))]
    let base = default_dialect;

    // Discover config early so it can contribute sqlite-version / sqlite-cflags
    // merging, and so commands can reuse the discovered config.
    let project_config = config::resolve(&config_mode);
    let configured = base
        .map(|d| {
            apply_sqlite_tuning(
                d,
                sqlite_version.as_ref(),
                &sqlite_cflag,
                project_config.as_ref(),
            )
        })
        .transpose()?;

    run_command(command, configured, &config_mode)
}

fn apply_sqlite_tuning(
    dialect: AnyDialect,
    cli_version: Option<&String>,
    cli_cflags: &[String],
    project_config: Option<&(ProjectConfig, PathBuf)>,
) -> Result<AnyDialect, String> {
    use syntaqlite::util::{SqliteFlag, SqliteFlags, SqliteVersion};

    // CLI flags take precedence over config file values.
    let version =
        cli_version.or_else(|| project_config.and_then(|(c, _)| c.sqlite_version.as_ref()));
    let cflags: &[String] = if cli_cflags.is_empty() {
        project_config
            .map(|(c, _)| c.sqlite_cflags.as_slice())
            .unwrap_or_default()
    } else {
        cli_cflags
    };

    let mut dialect = dialect;
    if let Some(v) = version {
        let ver = SqliteVersion::parse_with_latest(v)
            .map_err(|e| format!("invalid sqlite-version {v:?}: {e}"))?;
        dialect = dialect.with_version(ver);
    }
    if !cflags.is_empty() {
        let mut flags = SqliteFlags::default();
        for name in cflags {
            let flag = SqliteFlag::from_name(name)
                .ok_or_else(|| format!("unknown sqlite-cflag: {name}"))?;
            flags = flags.with(flag);
        }
        dialect = dialect.with_cflags(flags);
    }
    Ok(dialect)
}

fn run_command(
    command: Command,
    dialect: Option<AnyDialect>,
    config: &ConfigMode<'_>,
) -> Result<(), String> {
    match command {
        Command::Parse(args) => commands::parse::run(&require_dialect(dialect)?, &args),
        Command::Fmt(args) => commands::fmt::run(&require_dialect(dialect)?, config, &args),
        Command::Analyze(args) => commands::analyze::run(&require_dialect(dialect)?, config, &args),
        Command::Lineage(args) => commands::lineage::run(&require_dialect(dialect)?, config, &args),
        Command::Tokenize(args) => commands::tokenize::run(&require_dialect(dialect)?, &args),
        Command::Lsp => commands::lsp::run(require_dialect(dialect)?, config),
        #[cfg(feature = "mcp")]
        Command::Mcp => commands::mcp::run(require_dialect(dialect)?),
        Command::Serve { protocol } => match protocol {
            cli::ServeProtocol::Json => commands::serve::run_json(&require_dialect(dialect)?),
        },
        #[cfg(feature = "codegen")]
        Command::Dialect { command } => match command {
            cli::DialectSubcommand::Generate(args) => commands::codegen::generate(&args),
        },
        #[cfg(feature = "codegen")]
        Command::DialectTool(cmd) => commands::codegen::dispatch_tool(cmd),
        Command::Version => {
            println!("syntaqlite {}", env!("CARGO_PKG_VERSION"));
            Ok(())
        }
    }
}

/// Stock CLI configuration: the bundled `SQLite` dialect with all override
/// surfaces enabled.
#[cfg(feature = "bundled-sqlite-dialect")]
pub struct Stock;

#[cfg(feature = "bundled-sqlite-dialect")]
impl CliApp for Stock {
    fn name(&self) -> &'static str {
        "syntaqlite"
    }

    fn default_dialect(&self) -> Option<AnyDialect> {
        Some(syntaqlite::sqlite_dialect().into())
    }

    fn allow_dialect_override(&self) -> bool {
        true
    }

    fn allow_sqlite_tuning(&self) -> bool {
        true
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    struct Locked;
    impl CliApp for Locked {
        fn name(&self) -> &'static str {
            "locked"
        }
        fn about(&self) -> &'static str {
            "locked-down test CLI"
        }
    }

    struct Open;
    impl CliApp for Open {
        fn name(&self) -> &'static str {
            "open"
        }
        fn allow_dialect_override(&self) -> bool {
            true
        }
        fn allow_sqlite_tuning(&self) -> bool {
            true
        }
    }

    fn parse(app: &impl CliApp, argv: &[&str]) -> Result<Cli, clap::Error> {
        use clap::FromArgMatches;
        let matches = build_command(app).try_get_matches_from(argv)?;
        Cli::from_arg_matches(&matches)
    }

    fn help_text(app: &impl CliApp) -> String {
        build_command(app).render_help().to_string()
    }

    #[test]
    fn build_command_uses_app_name_and_about() {
        let help = help_text(&Locked);
        assert!(help.contains("locked"), "name missing from help:\n{help}");
        assert!(
            help.contains("locked-down test CLI"),
            "about missing from help:\n{help}"
        );
    }

    #[test]
    fn sqlite_tuning_flags_hidden_when_disallowed() {
        let help = help_text(&Locked);
        assert!(
            !help.contains("--sqlite-version"),
            "--sqlite-version should be hidden:\n{help}"
        );
        assert!(
            !help.contains("--sqlite-cflag"),
            "--sqlite-cflag should be hidden:\n{help}"
        );
    }

    #[test]
    fn sqlite_tuning_flags_visible_when_allowed() {
        let help = help_text(&Open);
        assert!(
            help.contains("--sqlite-version"),
            "--sqlite-version should be visible:\n{help}"
        );
    }

    #[test]
    fn enforce_visibility_rejects_sqlite_tuning_when_disallowed() {
        let cli =
            parse(&Locked, &["locked", "--sqlite-version", "3.47.0", "parse"]).expect("clap parse");
        let err = enforce_visibility(&Locked, &cli).expect_err("should reject");
        assert!(err.contains("--sqlite-version"), "got: {err}");
    }

    #[test]
    fn enforce_visibility_rejects_sqlite_cflag_when_disallowed() {
        let cli = parse(
            &Locked,
            &["locked", "--sqlite-cflag", "SQLITE_ENABLE_FTS5", "parse"],
        )
        .expect("clap parse");
        let err = enforce_visibility(&Locked, &cli).expect_err("should reject");
        assert!(err.contains("--sqlite-cflag"), "got: {err}");
    }

    #[test]
    fn enforce_visibility_accepts_sqlite_tuning_when_allowed() {
        let cli =
            parse(&Open, &["open", "--sqlite-version", "3.47.0", "parse"]).expect("clap parse");
        enforce_visibility(&Open, &cli).expect("should accept");
    }

    #[cfg(feature = "dynload")]
    #[test]
    fn dialect_override_flags_hidden_when_disallowed() {
        let help = help_text(&Locked);
        assert!(
            !help.contains("--dialect "),
            "--dialect should be hidden:\n{help}"
        );
    }

    #[cfg(feature = "dynload")]
    #[test]
    fn dialect_override_flags_visible_when_allowed() {
        let help = help_text(&Open);
        assert!(
            help.contains("--dialect "),
            "--dialect should be visible:\n{help}"
        );
    }

    #[cfg(feature = "dynload")]
    #[test]
    fn enforce_visibility_rejects_dialect_when_disallowed() {
        let cli =
            parse(&Locked, &["locked", "--dialect", "/tmp/x.so", "parse"]).expect("clap parse");
        let err = enforce_visibility(&Locked, &cli).expect_err("should reject");
        assert!(err.contains("--dialect"), "got: {err}");
    }

    #[cfg(feature = "dynload")]
    #[test]
    fn enforce_visibility_accepts_dialect_when_allowed() {
        let cli = parse(&Open, &["open", "--dialect", "/tmp/x.so", "parse"]).expect("clap parse");
        enforce_visibility(&Open, &cli).expect("should accept");
    }

    #[cfg(feature = "bundled-sqlite-dialect")]
    #[test]
    fn stock_impl_exposes_all_override_surfaces() {
        assert!(Stock.allow_dialect_override());
        assert!(Stock.allow_sqlite_tuning());
        assert!(Stock.default_dialect().is_some());
    }
}
