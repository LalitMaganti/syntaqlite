// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! LSP protocol server — stdio JSON-RPC message loop.

// `LspServer` is intentionally `pub` so it can be re-exported by `lsp/mod.rs`.
// The `server` submodule is private; items here are only reachable via that re-export.
#![allow(unreachable_pub)]

use std::collections::HashMap;
use std::error::Error;
use std::path::PathBuf;

use crate::AnalysisConfig;

use lsp_server::{Connection, Message, Notification, Request, Response};
use lsp_types::notification::{
    DidChangeConfiguration, DidChangeTextDocument, DidCloseTextDocument, DidOpenTextDocument,
    Notification as _, PublishDiagnostics,
};
use lsp_types::request::{
    Completion, DocumentHighlightRequest, Formatting, GotoDefinition, HoverRequest,
    PrepareRenameRequest, References, Rename, Request as _, SemanticTokensFullRequest,
    SignatureHelpRequest,
};
use lsp_types::{
    CompletionItem, CompletionItemKind, CompletionOptions, CompletionResponse, DiagnosticSeverity,
    GotoDefinitionResponse, Hover, HoverContents, HoverProviderCapability, InitializeParams,
    Location, MarkupContent, MarkupKind, ParameterInformation, ParameterLabel, Position,
    PositionEncodingKind, PrepareRenameResponse, Range, RenameOptions, SemanticTokenType,
    SemanticTokensFullOptions, SemanticTokensLegend, SemanticTokensOptions, SemanticTokensResult,
    SemanticTokensServerCapabilities, ServerCapabilities, SignatureHelp, SignatureHelpOptions,
    SignatureInformation, TextDocumentSyncCapability, TextDocumentSyncKind, TextEdit, Uri,
    WorkDoneProgressOptions, WorkspaceEdit,
};
use syntaqlite_syntax::source::{DocOffset, DocRange, DocText, Utf16Col, Utf16Line};

use crate::analysis::Catalog;
use crate::analysis::diagnostics::Severity;
use crate::dialect::AnyDialect;
use crate::fmt::FormatConfig;
use crate::lsp::host::SchemaMap;
use crate::lsp::{CompletionKind, LspHost, SEMANTIC_TOKEN_LEGEND, SourceMap};

// ── LspConfig ─────────────────────────────────────────────────────────────

/// Configuration for the LSP server, resolved from a project config file.
#[derive(Default)]
pub struct LspConfig {
    /// Format config from project config file.
    pub format_config: Option<FormatConfig>,
    /// Pre-loaded schema catalog from project config file.
    pub schema_catalog: Option<Catalog>,
    /// Validation config (check levels) from project config file.
    pub analysis_config: Option<AnalysisConfig>,
    /// Per-file schema resolution from `[schemas]` globs.
    pub schema_map: Option<SchemaMap>,
}

// ── LspServer ─────────────────────────────────────────────────────────────

/// Stdio LSP server for a syntaqlite dialect.
///
/// Runs a JSON-RPC message loop on stdin/stdout, driving an [`LspHost`]
/// for all analysis requests. Exits cleanly when the client sends a
/// `shutdown` request.
///
/// Use this when you want a turnkey LSP binary that editors can launch as a
/// child process. For programmatic access (e.g., in a web worker or test
/// harness), use [`LspHost`] directly instead.
///
/// # Supported capabilities
///
/// - `textDocument/didOpen`, `didChange`, `didClose`
/// - `textDocument/completion` (keywords and functions)
/// - `textDocument/hover` (table, column, and function info)
/// - `textDocument/signatureHelp` (function arities)
/// - `textDocument/semanticTokens/full`
/// - `textDocument/formatting`
/// - `textDocument/references` (find all references)
/// - `textDocument/rename` + `textDocument/prepareRename`
/// - `textDocument/publishDiagnostics` (parse + semantic errors)
///
/// # Example
///
/// ```no_run
/// use syntaqlite::lsp::LspServer;
///
/// // Blocks on stdin/stdout — typically launched by an editor.
/// LspServer::run(syntaqlite::sqlite_dialect()).expect("LSP server failed");
/// ```
pub struct LspServer;

impl LspServer {
    /// Start the LSP server bound to `dialect` and block until shutdown.
    ///
    /// # Errors
    /// Returns `Err` if the LSP connection fails or an unrecoverable I/O error occurs.
    pub fn run(dialect: impl Into<AnyDialect>) -> Result<(), Box<dyn Error + Sync + Send>> {
        Self::run_with_config(dialect, LspConfig::default())
    }

    /// Start the LSP server with project configuration pre-loaded.
    ///
    /// # Errors
    /// Returns `Err` if the LSP connection fails or an unrecoverable I/O error occurs.
    pub fn run_with_config(
        dialect: impl Into<AnyDialect>,
        config: LspConfig,
    ) -> Result<(), Box<dyn Error + Sync + Send>> {
        let dialect = dialect.into();
        let (connection, io_threads) = Connection::stdio();

        // VSCode only supports UTF-16 and UTF-32 position encodings.
        // Default to UTF-16 which is the LSP baseline.
        let server_capabilities = serde_json::to_value(ServerCapabilities {
            position_encoding: Some(PositionEncodingKind::UTF16),
            text_document_sync: Some(TextDocumentSyncCapability::Kind(TextDocumentSyncKind::FULL)),
            definition_provider: Some(lsp_types::OneOf::Left(true)),
            document_highlight_provider: Some(lsp_types::OneOf::Left(true)),
            references_provider: Some(lsp_types::OneOf::Left(true)),
            rename_provider: Some(lsp_types::OneOf::Right(RenameOptions {
                prepare_provider: Some(true),
                work_done_progress_options: WorkDoneProgressOptions::default(),
            })),
            hover_provider: Some(HoverProviderCapability::Simple(true)),
            document_formatting_provider: Some(lsp_types::OneOf::Left(true)),
            completion_provider: Some(CompletionOptions {
                trigger_characters: Some(vec![
                    " ".into(),
                    ".".into(),
                    "\n".into(),
                    "\t".into(),
                    ";".into(),
                ]),
                ..Default::default()
            }),
            signature_help_provider: Some(SignatureHelpOptions {
                trigger_characters: Some(vec!["(".into(), ",".into()]),
                retrigger_characters: Some(vec![",".into()]),
                ..Default::default()
            }),
            semantic_tokens_provider: Some(
                SemanticTokensServerCapabilities::SemanticTokensOptions(SemanticTokensOptions {
                    legend: SemanticTokensLegend {
                        token_types: SEMANTIC_TOKEN_LEGEND
                            .iter()
                            .map(|&name| SemanticTokenType::new(name))
                            .collect(),
                        token_modifiers: vec![],
                    },
                    full: Some(SemanticTokensFullOptions::Bool(true)),
                    ..Default::default()
                }),
            ),
            ..Default::default()
        })?;

        let init_params_raw = connection.initialize(server_capabilities)?;
        let init_params: InitializeParams = serde_json::from_value(init_params_raw)?;

        if let Some(root) = Self::workspace_root(&init_params) {
            eprintln!("syntaqlite-lsp: workspace root: {}", root.display());
        }

        let mut host = LspHost::with_dialect(dialect);

        // Apply project config if provided.
        let has_config_schema = config.schema_catalog.is_some() || config.schema_map.is_some();
        let has_analysis_config = config.analysis_config.is_some();
        if let Some(fmt) = config.format_config {
            host.set_format_config(fmt);
        }
        if let Some(validation) = config.analysis_config {
            host.set_analysis_config(validation);
        }
        if let Some(map) = config.schema_map {
            host.set_schema_map(map);
            eprintln!("syntaqlite-lsp: using per-file schema map");
        } else if let Some(catalog) = config.schema_catalog {
            host.set_session_context(catalog);
            // If no explicit validation config was provided, default schema
            // checks to deny when a schema is present.
            if !has_analysis_config {
                host.set_analysis_config(AnalysisConfig::default().with_strict_schema());
            }
            eprintln!("syntaqlite-lsp: using project config schema");
        }

        // Legacy fallback: load schema from initializationOptions.schemaPath
        // if no project config schema was provided. This supports older VS Code
        // extension versions that still send schemaPath.
        if !has_config_schema {
            Self::load_schema_from_options(&init_params, &mut host);
        }

        for msg in &connection.receiver {
            match msg {
                Message::Request(req) => {
                    if connection.handle_shutdown(&req)? {
                        return Ok(());
                    }
                    Self::handle_request(&connection, &mut host, req)?;
                }
                Message::Notification(notif) => {
                    Self::handle_notification(&connection, &mut host, notif)?;
                }
                Message::Response(_) => {}
            }
        }

        io_threads.join()?;
        Ok(())
    }

    // ── Request dispatch ──────────────────────────────────────────────────

    fn handle_request(
        connection: &Connection,
        host: &mut LspHost,
        req: Request,
    ) -> Result<(), Box<dyn Error + Sync + Send>> {
        let response = match req.method.as_str() {
            Completion::METHOD => Self::handle_completion(req, host),
            GotoDefinition::METHOD => Self::handle_definition(req, host),
            HoverRequest::METHOD => Self::handle_hover(req, host),
            SignatureHelpRequest::METHOD => Self::handle_signature_help(req, host),
            Formatting::METHOD => Self::handle_formatting(req, host),
            SemanticTokensFullRequest::METHOD => Self::handle_semantic_tokens(req, host),
            DocumentHighlightRequest::METHOD => Self::handle_document_highlight(req, host),
            References::METHOD => Self::handle_references(req, host),
            PrepareRenameRequest::METHOD => Self::handle_prepare_rename(req, host),
            Rename::METHOD => Self::handle_rename(req, host),
            _ => Response::new_err(
                req.id,
                lsp_server::ErrorCode::MethodNotFound as i32,
                format!("unknown request method: {}", req.method),
            ),
        };
        connection.sender.send(Message::Response(response))?;
        Ok(())
    }

    fn handle_completion(req: Request, host: &mut LspHost) -> Response {
        let params: lsp_types::CompletionParams = match serde_json::from_value(req.params) {
            Ok(p) => p,
            Err(e) => {
                return Response::new_err(
                    req.id,
                    lsp_server::ErrorCode::InvalidParams as i32,
                    e.to_string(),
                );
            }
        };
        let uri = params.text_document_position.text_document.uri;
        let position = params.text_document_position.position;
        let uri_str = uri.as_str();

        match host.document_source(uri_str) {
            Some(source) => {
                let offset = SourcePositionMap::new(source).position_to_offset(position);
                let items = host
                    .completion_items(uri_str, offset)
                    .into_iter()
                    .map(|entry| CompletionItem {
                        label: entry.label().to_string(),
                        sort_text: Some(format!(
                            "{}_{}",
                            entry.kind().sort_priority(),
                            entry.label()
                        )),
                        kind: Some(match entry.kind() {
                            CompletionKind::Keyword => CompletionItemKind::KEYWORD,
                            CompletionKind::Function => CompletionItemKind::FUNCTION,
                            CompletionKind::Table => CompletionItemKind::STRUCT,
                            CompletionKind::Column => CompletionItemKind::FIELD,
                        }),
                        detail: Some(entry.kind().as_str().into()),
                        ..Default::default()
                    })
                    .collect();
                Response::new_ok(req.id, CompletionResponse::Array(items))
            }
            None => Response::new_ok(req.id, Option::<CompletionResponse>::None),
        }
    }

    fn handle_hover(req: Request, host: &mut LspHost) -> Response {
        let params: lsp_types::HoverParams = match serde_json::from_value(req.params) {
            Ok(p) => p,
            Err(e) => {
                return Response::new_err(
                    req.id,
                    lsp_server::ErrorCode::InvalidParams as i32,
                    e.to_string(),
                );
            }
        };
        let uri = params.text_document_position_params.text_document.uri;
        let position = params.text_document_position_params.position;
        let uri_str = uri.as_str();

        let Some(source) = host.document_source(uri_str) else {
            return Response::new_ok(req.id, Option::<Hover>::None);
        };
        let offset = SourcePositionMap::new(source).position_to_offset(position);

        match host.hover_info(uri_str, offset) {
            Some((text, tok_range)) => {
                let source = host
                    .document_source(uri_str)
                    .expect("document must exist for hover");
                let map = SourcePositionMap::new(source);
                let positions = map.offsets_to_positions(&[tok_range.start, tok_range.end]);
                let range = Range::new(positions[0], positions[1]);
                let hover = Hover {
                    contents: HoverContents::Markup(MarkupContent {
                        kind: MarkupKind::Markdown,
                        value: text,
                    }),
                    range: Some(range),
                };
                Response::new_ok(req.id, hover)
            }
            None => Response::new_ok(req.id, Option::<Hover>::None),
        }
    }

    fn handle_definition(req: Request, host: &mut LspHost) -> Response {
        let params: lsp_types::GotoDefinitionParams = match serde_json::from_value(req.params) {
            Ok(p) => p,
            Err(e) => {
                return Response::new_err(
                    req.id,
                    lsp_server::ErrorCode::InvalidParams as i32,
                    e.to_string(),
                );
            }
        };
        let uri = params.text_document_position_params.text_document.uri;
        let position = params.text_document_position_params.position;
        let uri_str = uri.as_str();

        let offset = {
            let Some(source) = host.document_source(uri_str) else {
                return Response::new_ok(req.id, Option::<GotoDefinitionResponse>::None);
            };
            SourcePositionMap::new(source).position_to_offset(position)
        };

        let Some(def) = host.definition_info(uri_str, offset) else {
            return Response::new_ok(req.id, Option::<GotoDefinitionResponse>::None);
        };

        // Re-borrow source (immutably) to compute ranges.
        let source = host
            .document_source(uri_str)
            .expect("document must exist")
            .to_string();
        let origin_range = offsets_to_range(&source, def.origin);

        let (target_uri, target_source) = if let Some(ref file_uri) = def.target.file_uri {
            let target: Uri = file_uri.parse().unwrap_or(uri);
            let file_path = file_uri.strip_prefix("file://").unwrap_or(file_uri);
            (
                target,
                std::fs::read_to_string(file_path).unwrap_or_default(),
            )
        } else {
            (uri, source.clone())
        };
        let target_range = offsets_to_range(&target_source, def.target.range);
        let link = lsp_types::LocationLink {
            origin_selection_range: Some(origin_range),
            target_uri,
            target_range,
            target_selection_range: target_range,
        };
        Response::new_ok(req.id, GotoDefinitionResponse::Link(vec![link]))
    }

    fn handle_signature_help(req: Request, host: &mut LspHost) -> Response {
        let params: lsp_types::SignatureHelpParams = match serde_json::from_value(req.params) {
            Ok(p) => p,
            Err(e) => {
                return Response::new_err(
                    req.id,
                    lsp_server::ErrorCode::InvalidParams as i32,
                    e.to_string(),
                );
            }
        };
        let uri = params.text_document_position_params.text_document.uri;
        let position = params.text_document_position_params.position;
        let uri_str = uri.as_str();

        let Some(source) = host.document_source(uri_str) else {
            return Response::new_ok(req.id, Option::<SignatureHelp>::None);
        };
        let offset = SourcePositionMap::new(source).position_to_offset(position);

        match host.signature_help(uri_str, offset) {
            Some(info) => {
                use crate::analysis::catalog::AritySpec;

                let signatures: Vec<SignatureInformation> = info
                    .arities
                    .iter()
                    .map(|arity| {
                        let (label, params) = match arity {
                            AritySpec::Exact(n) => {
                                let names: Vec<String> =
                                    (0..*n).map(|i| format!("arg{}", i + 1)).collect();
                                let label = format!("{}({})", info.name, names.join(", "));
                                let params: Vec<ParameterInformation> = names
                                    .iter()
                                    .map(|name| ParameterInformation {
                                        label: ParameterLabel::Simple(name.clone()),
                                        documentation: None,
                                    })
                                    .collect();
                                (label, params)
                            }
                            AritySpec::AtLeast(n) => {
                                let mut names: Vec<String> =
                                    (0..*n).map(|i| format!("arg{}", i + 1)).collect();
                                names.push("...".to_string());
                                let label = format!("{}({})", info.name, names.join(", "));
                                let params: Vec<ParameterInformation> = names
                                    .iter()
                                    .map(|name| ParameterInformation {
                                        label: ParameterLabel::Simple(name.clone()),
                                        documentation: None,
                                    })
                                    .collect();
                                (label, params)
                            }
                            AritySpec::Any => {
                                let label = format!("{}(...)", info.name);
                                let params = vec![ParameterInformation {
                                    label: ParameterLabel::Simple("...".to_string()),
                                    documentation: None,
                                }];
                                (label, params)
                            }
                        };
                        SignatureInformation {
                            label,
                            documentation: None,
                            parameters: Some(params),
                            active_parameter: Some(info.active_parameter),
                        }
                    })
                    .collect();

                let help = SignatureHelp {
                    signatures,
                    active_signature: Some(0),
                    active_parameter: Some(info.active_parameter),
                };
                Response::new_ok(req.id, help)
            }
            None => Response::new_ok(req.id, Option::<SignatureHelp>::None),
        }
    }

    fn handle_formatting(req: Request, host: &mut LspHost) -> Response {
        let params: lsp_types::DocumentFormattingParams = match serde_json::from_value(req.params) {
            Ok(p) => p,
            Err(e) => {
                return Response::new_err(
                    req.id,
                    lsp_server::ErrorCode::InvalidParams as i32,
                    e.to_string(),
                );
            }
        };
        let uri = params.text_document.uri.as_str();
        let config = host.format_config();
        match host.format(uri, &config) {
            Ok(formatted) => {
                let edit = TextEdit {
                    range: Range::new(Position::new(0, 0), Position::new(i32::MAX as u32, 0)),
                    new_text: formatted,
                };
                Response::new_ok(req.id, Some(vec![edit]))
            }
            Err(e) => Response::new_err(
                req.id,
                lsp_server::ErrorCode::InternalError as i32,
                e.to_string(),
            ),
        }
    }

    fn handle_semantic_tokens(req: Request, host: &mut LspHost) -> Response {
        let params: lsp_types::SemanticTokensParams = match serde_json::from_value(req.params) {
            Ok(p) => p,
            Err(e) => {
                return Response::new_err(
                    req.id,
                    lsp_server::ErrorCode::InvalidParams as i32,
                    e.to_string(),
                );
            }
        };
        let uri = params.text_document.uri.as_str();
        let encoded = host.semantic_tokens_encoded(uri, None);
        let data: Vec<lsp_types::SemanticToken> = encoded
            .chunks_exact(5)
            .map(|c| lsp_types::SemanticToken {
                delta_line: c[0],
                delta_start: c[1],
                length: c[2],
                token_type: c[3],
                token_modifiers_bitset: c[4],
            })
            .collect();
        Response::new_ok(
            req.id,
            SemanticTokensResult::Tokens(lsp_types::SemanticTokens {
                result_id: None,
                data,
            }),
        )
    }

    fn handle_document_highlight(req: Request, host: &mut LspHost) -> Response {
        let params: lsp_types::DocumentHighlightParams = match serde_json::from_value(req.params) {
            Ok(p) => p,
            Err(e) => {
                return Response::new_err(
                    req.id,
                    lsp_server::ErrorCode::InvalidParams as i32,
                    e.to_string(),
                );
            }
        };
        let uri = params.text_document_position_params.text_document.uri;
        let position = params.text_document_position_params.position;
        let uri_str = uri.as_str();

        let Some(source) = host.document_source(uri_str) else {
            return Response::new_ok(req.id, Option::<Vec<lsp_types::DocumentHighlight>>::None);
        };
        let offset = SourcePositionMap::new(source).position_to_offset(position);

        // find_references with include_declaration=true, then filter to same file.
        let refs = host.find_references(uri_str, offset, true);
        let same_file: Vec<_> = refs
            .into_iter()
            .filter(|(ref_uri, _)| ref_uri == uri_str)
            .collect();
        if same_file.is_empty() {
            return Response::new_ok(req.id, Option::<Vec<lsp_types::DocumentHighlight>>::None);
        }

        let source = host
            .document_source(uri_str)
            .expect("document must exist")
            .to_string();
        let highlights: Vec<lsp_types::DocumentHighlight> = same_file
            .into_iter()
            .map(|(_, range)| lsp_types::DocumentHighlight {
                range: offsets_to_range(&source, range),
                kind: Some(lsp_types::DocumentHighlightKind::READ),
            })
            .collect();

        Response::new_ok(req.id, highlights)
    }

    fn handle_references(req: Request, host: &mut LspHost) -> Response {
        let params: lsp_types::ReferenceParams = match serde_json::from_value(req.params) {
            Ok(p) => p,
            Err(e) => {
                return Response::new_err(
                    req.id,
                    lsp_server::ErrorCode::InvalidParams as i32,
                    e.to_string(),
                );
            }
        };
        let uri = params.text_document_position.text_document.uri;
        let position = params.text_document_position.position;
        let include_declaration = params.context.include_declaration;
        let uri_str = uri.as_str();

        let Some(source) = host.document_source(uri_str) else {
            return Response::new_ok(req.id, Option::<Vec<Location>>::None);
        };
        let offset = SourcePositionMap::new(source).position_to_offset(position);

        let refs = host.find_references(uri_str, offset, include_declaration);
        if refs.is_empty() {
            return Response::new_ok(req.id, Option::<Vec<Location>>::None);
        }

        let locations: Vec<Location> = refs
            .into_iter()
            .filter_map(|(ref_uri, doc_range)| {
                let source = if ref_uri == uri_str {
                    host.document_source(&ref_uri)?.to_string()
                } else if let Some(s) = host.document_source(&ref_uri) {
                    s.to_string()
                } else {
                    let file_path = ref_uri.strip_prefix("file://")?;
                    std::fs::read_to_string(file_path).ok()?
                };
                let range = offsets_to_range(&source, doc_range);
                let target_uri: Uri = ref_uri.parse().ok()?;
                Some(Location {
                    uri: target_uri,
                    range,
                })
            })
            .collect();

        Response::new_ok(req.id, locations)
    }

    fn handle_prepare_rename(req: Request, host: &mut LspHost) -> Response {
        let params: lsp_types::TextDocumentPositionParams = match serde_json::from_value(req.params)
        {
            Ok(p) => p,
            Err(e) => {
                return Response::new_err(
                    req.id,
                    lsp_server::ErrorCode::InvalidParams as i32,
                    e.to_string(),
                );
            }
        };
        let uri = params.text_document.uri;
        let position = params.position;
        let uri_str = uri.as_str();

        let Some(source) = host.document_source(uri_str) else {
            return Response::new_ok(req.id, Option::<PrepareRenameResponse>::None);
        };
        let offset = SourcePositionMap::new(source).position_to_offset(position);

        let Some((doc_range, placeholder)) = host.prepare_rename(uri_str, offset) else {
            return Response::new_ok(req.id, Option::<PrepareRenameResponse>::None);
        };

        let source = host
            .document_source(uri_str)
            .expect("document must exist for prepare_rename")
            .to_string();
        let range = offsets_to_range(&source, doc_range);
        Response::new_ok(
            req.id,
            PrepareRenameResponse::RangeWithPlaceholder { range, placeholder },
        )
    }

    fn handle_rename(req: Request, host: &mut LspHost) -> Response {
        let params: lsp_types::RenameParams = match serde_json::from_value(req.params) {
            Ok(p) => p,
            Err(e) => {
                return Response::new_err(
                    req.id,
                    lsp_server::ErrorCode::InvalidParams as i32,
                    e.to_string(),
                );
            }
        };
        let uri = params.text_document_position.text_document.uri;
        let position = params.text_document_position.position;
        let new_name = params.new_name;
        let uri_str = uri.as_str();

        let Some(source) = host.document_source(uri_str) else {
            return Response::new_ok(req.id, Option::<WorkspaceEdit>::None);
        };
        let offset = SourcePositionMap::new(source).position_to_offset(position);

        let edits_by_uri = host.rename(uri_str, offset, &new_name);
        if edits_by_uri.is_empty() {
            return Response::new_ok(req.id, Option::<WorkspaceEdit>::None);
        }

        #[expect(
            clippy::mutable_key_type,
            reason = "Uri uses interior mutability but hashes stably"
        )]
        let mut changes: HashMap<Uri, Vec<TextEdit>> = HashMap::new();
        for (edit_uri, edits) in edits_by_uri {
            let source = if edit_uri == uri_str {
                host.document_source(&edit_uri)
                    .unwrap_or_default()
                    .to_string()
            } else if let Some(s) = host.document_source(&edit_uri) {
                s.to_string()
            } else {
                let file_path = edit_uri.strip_prefix("file://").unwrap_or(&edit_uri);
                std::fs::read_to_string(file_path).unwrap_or_default()
            };
            let target_uri: Uri = match edit_uri.parse() {
                Ok(u) => u,
                Err(_) => continue,
            };
            let text_edits: Vec<TextEdit> = edits
                .into_iter()
                .map(|(doc_range, text)| TextEdit {
                    range: offsets_to_range(&source, doc_range),
                    new_text: text,
                })
                .collect();
            changes.insert(target_uri, text_edits);
        }

        Response::new_ok(
            req.id,
            WorkspaceEdit {
                changes: Some(changes),
                ..Default::default()
            },
        )
    }

    // ── Notification dispatch ─────────────────────────────────────────────

    fn handle_notification(
        connection: &Connection,
        host: &mut LspHost,
        notif: Notification,
    ) -> Result<(), Box<dyn Error + Sync + Send>> {
        match notif.method.as_str() {
            DidOpenTextDocument::METHOD => {
                let params: lsp_types::DidOpenTextDocumentParams =
                    serde_json::from_value(notif.params)?;
                let uri = &params.text_document.uri;
                host.open_document(
                    uri.as_str(),
                    params.text_document.version,
                    params.text_document.text,
                );
                DiagnosticPublisher::publish(connection, host, uri)?;
            }
            DidChangeTextDocument::METHOD => {
                let params: lsp_types::DidChangeTextDocumentParams =
                    serde_json::from_value(notif.params)?;
                let uri = &params.text_document.uri;
                if let Some(change) = params.content_changes.into_iter().last() {
                    host.update_document(uri.as_str(), params.text_document.version, change.text);
                }
                DiagnosticPublisher::publish(connection, host, uri)?;
            }
            DidChangeConfiguration::METHOD => {
                let params: lsp_types::DidChangeConfigurationParams =
                    serde_json::from_value(notif.params)?;
                Self::load_schema_from_settings(&params.settings, host);
            }
            DidCloseTextDocument::METHOD => {
                let params: lsp_types::DidCloseTextDocumentParams =
                    serde_json::from_value(notif.params)?;
                let uri = params.text_document.uri;
                // Clear diagnostics before removing the document.
                let clear = lsp_types::PublishDiagnosticsParams {
                    uri: uri.clone(),
                    diagnostics: vec![],
                    version: None,
                };
                connection
                    .sender
                    .send(Message::Notification(Notification::new(
                        PublishDiagnostics::METHOD.to_string(),
                        clear,
                    )))?;
                host.close_document(uri.as_str());
            }
            _ => {}
        }
        Ok(())
    }

    // ── Helpers ───────────────────────────────────────────────────────────

    /// Load schema from `initializationOptions.schemaPath`.
    fn load_schema_from_options(params: &InitializeParams, host: &mut LspHost) {
        let Some(opts) = &params.initialization_options else {
            eprintln!("syntaqlite-lsp: no initializationOptions");
            return;
        };
        eprintln!("syntaqlite-lsp: initializationOptions: {opts}");
        Self::load_schema_from_settings(opts, host);
    }

    /// Load schema DDL from a `schemaPath` key in a JSON settings object.
    fn load_schema_from_settings(settings: &serde_json::Value, host: &mut LspHost) {
        eprintln!("syntaqlite-lsp: load_schema_from_settings: {settings}");
        let path_str = settings
            .get("schemaPath")
            .and_then(|v| v.as_str())
            .unwrap_or("");
        if path_str.is_empty() {
            eprintln!("syntaqlite-lsp: schemaPath is empty, skipping");
            return;
        }
        let path = PathBuf::from(path_str);
        let file_uri = format!("file://{}", path.display());
        match std::fs::read_to_string(&path) {
            Ok(contents) => match host.set_session_context_from_ddl(&contents, Some(&file_uri)) {
                Ok(()) => {
                    host.set_analysis_config(AnalysisConfig::default().with_strict_schema());
                    eprintln!("syntaqlite-lsp: loaded schema from {}", path.display());
                }
                Err(errors) => {
                    eprintln!(
                        "syntaqlite-lsp: schema loaded with {} parse error(s) from {}",
                        errors.len(),
                        path.display()
                    );
                }
            },
            Err(e) => {
                eprintln!(
                    "syntaqlite-lsp: failed to read schema file {}: {}",
                    path.display(),
                    e
                );
            }
        }
    }

    fn workspace_root(params: &InitializeParams) -> Option<PathBuf> {
        #[expect(deprecated)]
        if let Some(uri) = &params.root_uri {
            let s = uri.as_str();
            if let Some(path) = s.strip_prefix("file://") {
                return Some(PathBuf::from(path));
            }
        }
        #[expect(deprecated)]
        params.root_path.as_ref().map(PathBuf::from)
    }
}

// ── DiagnosticPublisher ───────────────────────────────────────────────────

/// Converts host diagnostics to LSP format and pushes them to the client.
struct DiagnosticPublisher;

impl DiagnosticPublisher {
    fn publish(
        connection: &Connection,
        host: &mut LspHost,
        uri: &Uri,
    ) -> Result<(), Box<dyn Error + Sync + Send>> {
        let uri_str = uri.as_str();
        let Some((version, source, diags)) = host.document_all_diagnostics(uri_str) else {
            return Ok(());
        };

        // Collect all offsets and convert in a single O(n) pass.
        let mut offsets: Vec<DocOffset> = Vec::with_capacity(diags.len() * 2);
        for d in &diags {
            offsets.push(d.start());
            offsets.push(d.end());
        }
        let map = SourcePositionMap::new(&source);
        let positions = map.offsets_to_positions(&offsets);

        let lsp_diags: Vec<lsp_types::Diagnostic> = diags
            .iter()
            .enumerate()
            .map(|(i, d)| lsp_types::Diagnostic {
                range: Range::new(positions[i * 2], positions[i * 2 + 1]),
                severity: Some(match d.severity() {
                    Severity::Error => DiagnosticSeverity::ERROR,
                    Severity::Warning => DiagnosticSeverity::WARNING,
                    Severity::Info => DiagnosticSeverity::INFORMATION,
                    Severity::Hint => DiagnosticSeverity::HINT,
                }),
                message: match d.help() {
                    Some(help) => format!("{} ({help})", d.message()),
                    None => d.message().to_string(),
                },
                source: Some("syntaqlite".to_string()),
                ..Default::default()
            })
            .collect();

        let params = lsp_types::PublishDiagnosticsParams {
            uri: uri.clone(),
            diagnostics: lsp_diags,
            version: Some(version),
        };
        connection
            .sender
            .send(Message::Notification(Notification::new(
                PublishDiagnostics::METHOD.to_string(),
                params,
            )))?;
        Ok(())
    }
}

/// Convert a `DocRange` to an LSP `Range`.
fn offsets_to_range(source: &str, range: DocRange) -> Range {
    let map = SourcePositionMap::new(source);
    let positions = map.offsets_to_positions(&[range.start, range.end]);
    Range::new(positions[0], positions[1])
}

// ── SourcePositionMap ─────────────────────────────────────────────────────

/// Thin adapter around [`SourceMap`] that translates to and from
/// `lsp_types::Position`.  The position-mapping arithmetic lives in
/// [`SourceMap`]; this adapter only packs/unpacks the LSP wire types.
pub(super) struct SourcePositionMap<'a> {
    inner: SourceMap<'a>,
}

impl<'a> SourcePositionMap<'a> {
    pub(crate) fn new(source: &'a str) -> Self {
        SourcePositionMap {
            inner: SourceMap::new(DocText::new(source)),
        }
    }

    /// Convert multiple byte offsets to LSP `Position`s in one O(n) pass.
    pub(crate) fn offsets_to_positions(&self, offsets: &[DocOffset]) -> Vec<Position> {
        self.inner
            .byte_offsets_to_utf16(offsets)
            .into_iter()
            .map(|(line, col)| Position::new(line.as_u32(), col.as_u32()))
            .collect()
    }

    /// Convert an LSP `Position` (with UTF-16 character offset) to a document-absolute byte offset.
    pub(crate) fn position_to_offset(&self, pos: Position) -> DocOffset {
        self.inner.utf16_to_byte_offset(
            Utf16Line::from_raw(pos.line),
            Utf16Col::from_raw(pos.character),
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // ── offsets_to_positions ──────────────────────────────────────────

    #[test]
    fn ascii_positions() {
        let map = SourcePositionMap::new("ab\ncd");
        let positions = map.offsets_to_positions(&[
            DocOffset::from_raw(0),
            DocOffset::from_raw(1),
            DocOffset::from_raw(2),
            DocOffset::from_raw(3),
            DocOffset::from_raw(4),
        ]);
        assert_eq!(positions[0], Position::new(0, 0)); // 'a'
        assert_eq!(positions[1], Position::new(0, 1)); // 'b'
        assert_eq!(positions[2], Position::new(0, 2)); // '\n'
        assert_eq!(positions[3], Position::new(1, 0)); // 'c'
        assert_eq!(positions[4], Position::new(1, 1)); // 'd'
    }

    #[test]
    fn two_byte_utf8_char_is_one_utf16_unit() {
        // 'é' (U+00E9) = 2 UTF-8 bytes, 1 UTF-16 code unit
        let src = "aé b";
        let map = SourcePositionMap::new(src);
        // byte offsets: a=0, é=1..3, ' '=3, 'b'=4
        let positions = map.offsets_to_positions(&[
            DocOffset::from_raw(0),
            DocOffset::from_raw(3),
            DocOffset::from_raw(4),
        ]);
        assert_eq!(positions[0], Position::new(0, 0)); // 'a'
        assert_eq!(positions[1], Position::new(0, 2)); // ' ' — after 'a' (1) + 'é' (1)
        assert_eq!(positions[2], Position::new(0, 3)); // 'b'
    }

    #[test]
    fn three_byte_utf8_char_is_one_utf16_unit() {
        // '中' (U+4E2D) = 3 UTF-8 bytes, 1 UTF-16 code unit
        let src = "a中b";
        let map = SourcePositionMap::new(src);
        // byte offsets: a=0, 中=1..4, b=4
        let positions = map.offsets_to_positions(&[DocOffset::from_raw(0), DocOffset::from_raw(4)]);
        assert_eq!(positions[0], Position::new(0, 0)); // 'a'
        assert_eq!(positions[1], Position::new(0, 2)); // 'b' — after 'a' (1) + '中' (1)
    }

    #[test]
    fn four_byte_utf8_char_is_two_utf16_units() {
        // '😀' (U+1F600) = 4 UTF-8 bytes, 2 UTF-16 code units (surrogate pair)
        let src = "a😀b";
        let map = SourcePositionMap::new(src);
        // byte offsets: a=0, 😀=1..5, b=5
        let positions = map.offsets_to_positions(&[DocOffset::from_raw(0), DocOffset::from_raw(5)]);
        assert_eq!(positions[0], Position::new(0, 0)); // 'a'
        assert_eq!(positions[1], Position::new(0, 3)); // 'b' — after 'a' (1) + '😀' (2)
    }

    #[test]
    fn multibyte_after_newline() {
        let src = "x\né";
        let map = SourcePositionMap::new(src);
        // byte offsets: x=0, \n=1, é=2..4
        let positions = map.offsets_to_positions(&[DocOffset::from_raw(2), DocOffset::from_raw(4)]);
        assert_eq!(positions[0], Position::new(1, 0)); // start of 'é'
        assert_eq!(positions[1], Position::new(1, 1)); // after 'é'
    }

    // ── position_to_offset ───────────────────────────────────────────

    #[test]
    fn ascii_position_to_offset() {
        let map = SourcePositionMap::new("ab\ncd");
        assert_eq!(
            map.position_to_offset(Position::new(0, 0)),
            DocOffset::from_raw(0)
        );
        assert_eq!(
            map.position_to_offset(Position::new(0, 1)),
            DocOffset::from_raw(1)
        );
        assert_eq!(
            map.position_to_offset(Position::new(1, 0)),
            DocOffset::from_raw(3)
        );
        assert_eq!(
            map.position_to_offset(Position::new(1, 1)),
            DocOffset::from_raw(4)
        );
    }

    #[test]
    fn two_byte_char_position_to_offset() {
        // 'é' (U+00E9) = 2 UTF-8 bytes, 1 UTF-16 code unit
        let src = "aé b";
        let map = SourcePositionMap::new(src);
        // UTF-16 col 0 = byte 0 ('a')
        // UTF-16 col 1 = byte 1 (start of 'é')
        // UTF-16 col 2 = byte 3 (' ')
        // UTF-16 col 3 = byte 4 ('b')
        assert_eq!(
            map.position_to_offset(Position::new(0, 0)),
            DocOffset::from_raw(0)
        );
        assert_eq!(
            map.position_to_offset(Position::new(0, 1)),
            DocOffset::from_raw(1)
        );
        assert_eq!(
            map.position_to_offset(Position::new(0, 2)),
            DocOffset::from_raw(3)
        );
        assert_eq!(
            map.position_to_offset(Position::new(0, 3)),
            DocOffset::from_raw(4)
        );
    }

    #[test]
    fn four_byte_char_position_to_offset() {
        // '😀' (U+1F600) = 4 UTF-8 bytes, 2 UTF-16 code units
        let src = "a😀b";
        let map = SourcePositionMap::new(src);
        // UTF-16 col 0 = byte 0 ('a')
        // UTF-16 col 1 = byte 1 (start of '😀')
        // UTF-16 col 3 = byte 5 ('b')
        assert_eq!(
            map.position_to_offset(Position::new(0, 0)),
            DocOffset::from_raw(0)
        );
        assert_eq!(
            map.position_to_offset(Position::new(0, 1)),
            DocOffset::from_raw(1)
        );
        assert_eq!(
            map.position_to_offset(Position::new(0, 3)),
            DocOffset::from_raw(5)
        );
    }
}
