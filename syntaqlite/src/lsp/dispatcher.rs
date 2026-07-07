// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Transport-agnostic LSP message dispatch.
//!
//! [`LspDispatcher`] owns an [`LspHost`] and turns incoming JSON-RPC
//! messages into outgoing ones — responses for requests, plus
//! server-initiated notifications such as `textDocument/publishDiagnostics`.
//! It handles the LSP lifecycle (`initialize`, `shutdown`, `exit`) itself,
//! so any transport that can shuttle strings — stdio, a web worker
//! `postMessage` channel, a test harness — can run a full server on top of
//! it. The stdio [`LspServer`](crate::lsp::LspServer) is one such transport.
//!
//! # Extension methods
//!
//! Beyond standard LSP, the dispatcher accepts `syntaqlite/setSessionContext`
//! (advertised under `capabilities.experimental.syntaqlite`): a request that
//! configures the schema catalog used for analysis. It is accepted even
//! before `initialize` so embedders can configure the server up front, and
//! it re-publishes diagnostics for every open document. Params are one of:
//!
//! - `{"ddl": "CREATE TABLE ..."}` — parse DDL into a catalog
//! - `{"context": {...}}` — structured catalog JSON
//! - `{}` or `null` — clear the session context
//!
//! The result is `{"errors": [...]}`; DDL that only partially parses is
//! still applied, with the parse errors reported in `errors`.

// Items here are only reachable via the `lsp` module re-exports.
#![allow(unreachable_pub)]

use std::collections::HashMap;
use std::path::PathBuf;

use crate::AnalysisConfig;

use lsp_server::{ErrorCode, Message, Notification, Request, Response};
use lsp_types::notification::{
    DidChangeTextDocument, DidCloseTextDocument, DidOpenTextDocument, Exit, Initialized,
    Notification as _, PublishDiagnostics,
};
use lsp_types::request::{
    Completion, DocumentHighlightRequest, Formatting, GotoDefinition, HoverRequest, Initialize,
    PrepareRenameRequest, References, Rename, Request as _, SemanticTokensFullRequest,
    SemanticTokensRangeRequest, Shutdown, SignatureHelpRequest,
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
    /// Analysis config (check levels) from project config file.
    pub analysis_config: Option<AnalysisConfig>,
    /// Per-file schema resolution from `[schemas]` globs.
    pub schema_map: Option<SchemaMap>,
}

// ── LspDispatcher ─────────────────────────────────────────────────────────

/// Extension request configuring the schema catalog used for analysis.
/// See the module docs for the protocol.
const SET_SESSION_CONTEXT_METHOD: &str = "syntaqlite/setSessionContext";

/// LSP lifecycle phase, advanced by `initialize`/`shutdown`/`exit`.
#[derive(PartialEq, Eq)]
enum Lifecycle {
    Uninitialized,
    Initialized,
    ShutDown,
}

/// Transport-agnostic LSP server core.
///
/// Feed incoming messages as JSON strings to [`handle_json`](Self::handle_json)
/// and forward the returned messages to the client. The dispatcher
/// implements the full lifecycle: it answers
/// `initialize`, rejects requests sent before initialization, and records
/// the `exit` notification, exposed via [`exited`](Self::exited) so the
/// transport knows when to stop.
///
/// # Supported capabilities
///
/// - `textDocument/didOpen`, `didChange`, `didClose`
/// - `textDocument/completion` (keywords and functions)
/// - `textDocument/hover` (table, column, and function info)
/// - `textDocument/signatureHelp` (function arities)
/// - `textDocument/semanticTokens/full` + `semanticTokens/range`
/// - `textDocument/formatting`
/// - `textDocument/references` (find all references)
/// - `textDocument/rename` + `textDocument/prepareRename`
/// - `textDocument/publishDiagnostics` (parse + semantic errors; each
///   diagnostic carries the full structured form in `data`)
/// - `syntaqlite/setSessionContext` (schema context — see module docs)
///
/// # Example
///
/// ```
/// use syntaqlite::lsp::LspDispatcher;
///
/// let mut dispatcher = LspDispatcher::new(syntaqlite::sqlite_dialect());
/// let out = dispatcher.handle_json(
///     r#"{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{}}}"#,
/// );
/// assert_eq!(out.len(), 1); // initialize response
/// ```
pub struct LspDispatcher {
    host: LspHost,
    lifecycle: Lifecycle,
    exited: bool,
}

impl LspDispatcher {
    /// Construct a dispatcher bound to `dialect` with default configuration.
    pub fn new(dialect: impl Into<AnyDialect>) -> Self {
        Self::with_config(dialect, LspConfig::default())
    }

    /// Construct a dispatcher with project configuration pre-loaded.
    pub fn with_config(dialect: impl Into<AnyDialect>, config: LspConfig) -> Self {
        let mut host = LspHost::with_dialect(dialect.into());

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
            // If no explicit analysis config was provided, default schema
            // checks to deny when a schema is present.
            if !has_analysis_config {
                host.set_analysis_config(AnalysisConfig::default().with_strict_schema());
            }
            eprintln!("syntaqlite-lsp: using project config schema");
        }

        LspDispatcher {
            host,
            lifecycle: Lifecycle::Uninitialized,
            exited: false,
        }
    }

    /// Whether the client has sent the `exit` notification. The transport
    /// should stop its message loop once this returns `true`.
    pub fn exited(&self) -> bool {
        self.exited
    }

    /// Handle one incoming message, returning the messages to send back —
    /// at most one response, plus any server-initiated notifications.
    ///
    /// Crate-private so `lsp_server` wire types stay out of the public API;
    /// external transports go through [`handle_json`](Self::handle_json).
    pub(crate) fn handle(&mut self, msg: Message) -> Vec<Message> {
        match msg {
            Message::Request(req) => self.handle_request(req),
            Message::Notification(notif) => self.handle_notification(notif),
            Message::Response(_) => Vec::new(),
        }
    }

    /// Handle one incoming message given as a JSON string, returning the
    /// outgoing messages serialized to JSON. A malformed message yields a
    /// single JSON-RPC parse-error response.
    pub fn handle_json(&mut self, json: &str) -> Vec<String> {
        let msg: Message = match serde_json::from_str(json) {
            Ok(msg) => msg,
            Err(e) => {
                // Spec: a response to an unparseable message has `"id": null`,
                // which `lsp_server::RequestId` cannot represent — build the
                // response directly.
                let resp = serde_json::json!({
                    "jsonrpc": "2.0",
                    "id": serde_json::Value::Null,
                    "error": {
                        "code": ErrorCode::ParseError as i32,
                        "message": format!("invalid JSON-RPC message: {e}"),
                    },
                });
                return vec![resp.to_string()];
            }
        };
        self.handle(msg).iter().map(serialize_message).collect()
    }

    // ── Lifecycle + request dispatch ──────────────────────────────────────

    fn handle_request(&mut self, req: Request) -> Vec<Message> {
        // Extension methods configure the server, so they are accepted in
        // any lifecycle state except after shutdown.
        if req.method == SET_SESSION_CONTEXT_METHOD {
            if self.lifecycle == Lifecycle::ShutDown {
                return vec![Message::Response(Response::new_err(
                    req.id,
                    ErrorCode::InvalidRequest as i32,
                    "server is shut down".to_string(),
                ))];
            }
            return self.handle_set_session_context(req);
        }
        let response = match (&self.lifecycle, req.method.as_str()) {
            (Lifecycle::Uninitialized, Initialize::METHOD) => self.handle_initialize(req),
            (Lifecycle::Uninitialized, _) => Response::new_err(
                req.id,
                ErrorCode::ServerNotInitialized as i32,
                "server not initialized".to_string(),
            ),
            (_, Initialize::METHOD) => Response::new_err(
                req.id,
                ErrorCode::InvalidRequest as i32,
                "server already initialized".to_string(),
            ),
            (Lifecycle::ShutDown, _) => Response::new_err(
                req.id,
                ErrorCode::InvalidRequest as i32,
                "server is shut down".to_string(),
            ),
            (Lifecycle::Initialized, Shutdown::METHOD) => {
                self.lifecycle = Lifecycle::ShutDown;
                Response::new_ok(req.id, serde_json::Value::Null)
            }
            (Lifecycle::Initialized, method) => {
                let host = &mut self.host;
                match method {
                    Completion::METHOD => handle_completion(req, host),
                    GotoDefinition::METHOD => handle_definition(req, host),
                    HoverRequest::METHOD => handle_hover(req, host),
                    SignatureHelpRequest::METHOD => handle_signature_help(req, host),
                    Formatting::METHOD => handle_formatting(req, host),
                    SemanticTokensFullRequest::METHOD => handle_semantic_tokens(req, host),
                    SemanticTokensRangeRequest::METHOD => handle_semantic_tokens_range(req, host),
                    DocumentHighlightRequest::METHOD => handle_document_highlight(req, host),
                    References::METHOD => handle_references(req, host),
                    PrepareRenameRequest::METHOD => handle_prepare_rename(req, host),
                    Rename::METHOD => handle_rename(req, host),
                    _ => Response::new_err(
                        req.id,
                        ErrorCode::MethodNotFound as i32,
                        format!("unknown request method: {}", req.method),
                    ),
                }
            }
        };
        vec![Message::Response(response)]
    }

    fn handle_initialize(&mut self, req: Request) -> Response {
        let params: InitializeParams = match serde_json::from_value(req.params) {
            Ok(p) => p,
            Err(e) => {
                return Response::new_err(req.id, ErrorCode::InvalidParams as i32, e.to_string());
            }
        };

        if let Some(root) = workspace_root(&params) {
            eprintln!("syntaqlite-lsp: workspace root: {}", root.display());
        }

        self.lifecycle = Lifecycle::Initialized;
        let result = serde_json::json!({
            "capabilities": server_capabilities(),
            "serverInfo": {
                "name": "syntaqlite-lsp",
                "version": env!("CARGO_PKG_VERSION"),
            },
        });
        Response::new_ok(req.id, result)
    }

    /// `syntaqlite/setSessionContext` — see the module docs for the protocol.
    fn handle_set_session_context(&mut self, req: Request) -> Vec<Message> {
        let Request { id, params, .. } = req;
        // Schema presence toggles strict schema checks, mirroring how
        // project-config schemas behave.
        let (errors, analysis) = if let Some(ddl) =
            params.get("ddl").and_then(serde_json::Value::as_str)
        {
            // DDL that only partially parses is still applied; the parse
            // errors are reported in the result.
            let errors = self
                .host
                .set_session_context_from_ddl(ddl, None)
                .err()
                .unwrap_or_default();
            (errors, AnalysisConfig::default().with_strict_schema())
        } else if let Some(context) = params.get("context") {
            if let Err(e) = self
                .host
                .set_session_context_from_json(&context.to_string())
            {
                return vec![Message::Response(Response::new_err(
                    id,
                    ErrorCode::InvalidParams as i32,
                    e,
                ))];
            }
            (Vec::new(), AnalysisConfig::default().with_strict_schema())
        } else if params.is_null() || params.as_object().is_some_and(serde_json::Map::is_empty) {
            self.host.clear_session_context();
            (Vec::new(), AnalysisConfig::default())
        } else {
            return vec![Message::Response(Response::new_err(
                id,
                ErrorCode::InvalidParams as i32,
                r#"expected {"ddl": string}, {"context": object}, or {} to clear"#.to_string(),
            ))];
        };
        self.host.set_analysis_config(analysis);

        let mut out = vec![Message::Response(Response::new_ok(
            id,
            serde_json::json!({"errors": errors}),
        ))];
        out.extend(self.republish_all().into_iter().map(Message::Notification));
        out
    }

    /// Re-publish diagnostics for every open document, e.g. after a context
    /// change invalidated all cached analysis.
    fn republish_all(&mut self) -> Vec<Notification> {
        self.host
            .document_uris()
            .into_iter()
            .filter_map(|uri| {
                let uri: Uri = uri.parse().ok()?;
                diagnostics_notification(&mut self.host, &uri)
            })
            .collect()
    }

    // ── Notification dispatch ─────────────────────────────────────────────

    fn handle_notification(&mut self, notif: Notification) -> Vec<Message> {
        if notif.method == Exit::METHOD {
            self.exited = true;
            return Vec::new();
        }
        if self.lifecycle != Lifecycle::Initialized {
            // Per the LSP spec, notifications other than `exit` are dropped
            // before initialization and after shutdown.
            return Vec::new();
        }
        match notif.method.as_str() {
            Initialized::METHOD => Vec::new(),
            DidOpenTextDocument::METHOD => {
                let Ok(params) =
                    serde_json::from_value::<lsp_types::DidOpenTextDocumentParams>(notif.params)
                else {
                    return Vec::new();
                };
                let uri = params.text_document.uri;
                self.host.open_document(
                    uri.as_str(),
                    params.text_document.version,
                    params.text_document.text,
                    Some(params.text_document.language_id),
                );
                diagnostics_notification(&mut self.host, &uri)
                    .map(Message::Notification)
                    .into_iter()
                    .collect()
            }
            DidChangeTextDocument::METHOD => {
                let Ok(params) =
                    serde_json::from_value::<lsp_types::DidChangeTextDocumentParams>(notif.params)
                else {
                    return Vec::new();
                };
                let uri = params.text_document.uri;
                if let Some(change) = params.content_changes.into_iter().last() {
                    self.host.update_document(
                        uri.as_str(),
                        params.text_document.version,
                        change.text,
                    );
                }
                diagnostics_notification(&mut self.host, &uri)
                    .map(Message::Notification)
                    .into_iter()
                    .collect()
            }
            DidCloseTextDocument::METHOD => {
                let Ok(params) =
                    serde_json::from_value::<lsp_types::DidCloseTextDocumentParams>(notif.params)
                else {
                    return Vec::new();
                };
                let uri = params.text_document.uri;
                self.host.close_document(uri.as_str());
                // Clear diagnostics for the closed document.
                let clear = lsp_types::PublishDiagnosticsParams {
                    uri,
                    diagnostics: vec![],
                    version: None,
                };
                vec![Message::Notification(Notification::new(
                    PublishDiagnostics::METHOD.to_string(),
                    clear,
                ))]
            }
            _ => Vec::new(),
        }
    }
}

/// Serialize an outgoing message. Message construction is infallible JSON,
/// so serialization cannot fail.
fn serialize_message(msg: &Message) -> String {
    serde_json::to_string(msg).expect("LSP message serialization failed")
}

/// The capabilities advertised in the `initialize` response.
///
/// `VSCode` only supports UTF-16 and UTF-32 position encodings; default to
/// UTF-16, which is the LSP baseline.
fn server_capabilities() -> ServerCapabilities {
    ServerCapabilities {
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
        semantic_tokens_provider: Some(SemanticTokensServerCapabilities::SemanticTokensOptions(
            SemanticTokensOptions {
                legend: SemanticTokensLegend {
                    token_types: SEMANTIC_TOKEN_LEGEND
                        .iter()
                        .map(|&name| SemanticTokenType::new(name))
                        .collect(),
                    token_modifiers: vec![],
                },
                range: Some(true),
                full: Some(SemanticTokensFullOptions::Bool(true)),
                ..Default::default()
            },
        )),
        experimental: Some(serde_json::json!({
            "syntaqlite": {"setSessionContext": true},
        })),
        ..Default::default()
    }
}

// ── Request handlers ──────────────────────────────────────────────────────

fn handle_completion(req: Request, host: &mut LspHost) -> Response {
    let params: lsp_types::CompletionParams = match serde_json::from_value(req.params) {
        Ok(p) => p,
        Err(e) => {
            return Response::new_err(req.id, ErrorCode::InvalidParams as i32, e.to_string());
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
            return Response::new_err(req.id, ErrorCode::InvalidParams as i32, e.to_string());
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
            return Response::new_err(req.id, ErrorCode::InvalidParams as i32, e.to_string());
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
            return Response::new_err(req.id, ErrorCode::InvalidParams as i32, e.to_string());
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
            return Response::new_err(req.id, ErrorCode::InvalidParams as i32, e.to_string());
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
        Err(e) => Response::new_err(req.id, ErrorCode::InternalError as i32, e.to_string()),
    }
}

fn handle_semantic_tokens(req: Request, host: &mut LspHost) -> Response {
    let params: lsp_types::SemanticTokensParams = match serde_json::from_value(req.params) {
        Ok(p) => p,
        Err(e) => {
            return Response::new_err(req.id, ErrorCode::InvalidParams as i32, e.to_string());
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

fn handle_semantic_tokens_range(req: Request, host: &mut LspHost) -> Response {
    let params: lsp_types::SemanticTokensRangeParams = match serde_json::from_value(req.params) {
        Ok(p) => p,
        Err(e) => {
            return Response::new_err(req.id, ErrorCode::InvalidParams as i32, e.to_string());
        }
    };
    let uri = params.text_document.uri.as_str();
    let range = host.document_source(uri).map(|source| {
        let map = SourcePositionMap::new(source);
        DocRange {
            start: map.position_to_offset(params.range.start),
            end: map.position_to_offset(params.range.end),
        }
    });
    let encoded = host.semantic_tokens_encoded(uri, range);
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
        lsp_types::SemanticTokensRangeResult::Tokens(lsp_types::SemanticTokens {
            result_id: None,
            data,
        }),
    )
}

fn handle_document_highlight(req: Request, host: &mut LspHost) -> Response {
    let params: lsp_types::DocumentHighlightParams = match serde_json::from_value(req.params) {
        Ok(p) => p,
        Err(e) => {
            return Response::new_err(req.id, ErrorCode::InvalidParams as i32, e.to_string());
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
            return Response::new_err(req.id, ErrorCode::InvalidParams as i32, e.to_string());
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
    let params: lsp_types::TextDocumentPositionParams = match serde_json::from_value(req.params) {
        Ok(p) => p,
        Err(e) => {
            return Response::new_err(req.id, ErrorCode::InvalidParams as i32, e.to_string());
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
            return Response::new_err(req.id, ErrorCode::InvalidParams as i32, e.to_string());
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

// ── Diagnostics publishing ────────────────────────────────────────────────

/// Build a `textDocument/publishDiagnostics` notification for `uri`, or
/// `None` if the document is not open.
fn diagnostics_notification(host: &mut LspHost, uri: &Uri) -> Option<Notification> {
    let uri_str = uri.as_str();
    let (version, source, diags) = host.document_all_diagnostics(uri_str)?;

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
            // Full structured diagnostic (byte offsets, kind, help detail)
            // for clients that want more than the standard LSP fields.
            data: serde_json::to_value(d).ok(),
            ..Default::default()
        })
        .collect();

    let params = lsp_types::PublishDiagnosticsParams {
        uri: uri.clone(),
        diagnostics: lsp_diags,
        version: Some(version),
    };
    Some(Notification::new(
        PublishDiagnostics::METHOD.to_string(),
        params,
    ))
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

    // ── Dispatcher lifecycle + round-trips ─────────────────────────────

    fn dispatcher() -> LspDispatcher {
        LspDispatcher::new(crate::sqlite_dialect())
    }

    fn initialize(dispatcher: &mut LspDispatcher) {
        let out = dispatcher.handle_json(
            r#"{"jsonrpc":"2.0","id":0,"method":"initialize","params":{"capabilities":{}}}"#,
        );
        assert_eq!(out.len(), 1);
        let resp: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        assert!(resp["result"]["capabilities"].is_object());
        dispatcher.handle_json(r#"{"jsonrpc":"2.0","method":"initialized","params":{}}"#);
    }

    #[test]
    fn initialize_advertises_capabilities() {
        let mut d = dispatcher();
        let out = d.handle_json(
            r#"{"jsonrpc":"2.0","id":0,"method":"initialize","params":{"capabilities":{}}}"#,
        );
        let resp: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        let caps = &resp["result"]["capabilities"];
        assert_eq!(caps["positionEncoding"], "utf-16");
        assert!(caps["completionProvider"].is_object());
        assert_eq!(resp["result"]["serverInfo"]["name"], "syntaqlite-lsp");
    }

    #[test]
    fn request_before_initialize_is_rejected() {
        let mut d = dispatcher();
        let out = d.handle_json(
            r#"{"jsonrpc":"2.0","id":1,"method":"textDocument/completion","params":{}}"#,
        );
        let resp: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        assert_eq!(
            resp["error"]["code"],
            ErrorCode::ServerNotInitialized as i32
        );
    }

    #[test]
    fn double_initialize_is_rejected() {
        let mut d = dispatcher();
        initialize(&mut d);
        let out = d.handle_json(
            r#"{"jsonrpc":"2.0","id":9,"method":"initialize","params":{"capabilities":{}}}"#,
        );
        let resp: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        assert_eq!(resp["error"]["code"], ErrorCode::InvalidRequest as i32);
    }

    #[test]
    fn did_open_publishes_diagnostics() {
        let mut d = dispatcher();
        initialize(&mut d);
        let out = d.handle_json(
            r#"{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///q.sql","languageId":"sql","version":1,"text":"selec 1"}}}"#,
        );
        assert_eq!(out.len(), 1);
        let notif: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        assert_eq!(notif["method"], "textDocument/publishDiagnostics");
        assert_eq!(notif["params"]["uri"], "file:///q.sql");
        assert!(
            !notif["params"]["diagnostics"]
                .as_array()
                .unwrap()
                .is_empty(),
            "expected a parse diagnostic for 'selec 1'"
        );
    }

    #[test]
    fn completion_roundtrip() {
        let mut d = dispatcher();
        initialize(&mut d);
        d.handle_json(
            r#"{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///q.sql","languageId":"sql","version":1,"text":"SELECT "}}}"#,
        );
        let out = d.handle_json(
            r#"{"jsonrpc":"2.0","id":2,"method":"textDocument/completion","params":{"textDocument":{"uri":"file:///q.sql"},"position":{"line":0,"character":7}}}"#,
        );
        let resp: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        assert_eq!(resp["id"], 2);
        assert!(
            !resp["result"].as_array().unwrap().is_empty(),
            "expected completion items after SELECT"
        );
    }

    #[test]
    fn did_close_clears_diagnostics() {
        let mut d = dispatcher();
        initialize(&mut d);
        d.handle_json(
            r#"{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///q.sql","languageId":"sql","version":1,"text":"selec 1"}}}"#,
        );
        let out = d.handle_json(
            r#"{"jsonrpc":"2.0","method":"textDocument/didClose","params":{"textDocument":{"uri":"file:///q.sql"}}}"#,
        );
        let notif: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        assert_eq!(notif["method"], "textDocument/publishDiagnostics");
        assert!(
            notif["params"]["diagnostics"]
                .as_array()
                .unwrap()
                .is_empty()
        );
    }

    #[test]
    fn shutdown_then_exit() {
        let mut d = dispatcher();
        initialize(&mut d);
        let out = d.handle_json(r#"{"jsonrpc":"2.0","id":3,"method":"shutdown","params":null}"#);
        let resp: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        assert!(resp["result"].is_null());
        assert!(!d.exited());
        // Requests after shutdown are rejected.
        let out = d.handle_json(
            r#"{"jsonrpc":"2.0","id":4,"method":"textDocument/completion","params":{}}"#,
        );
        let resp: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        assert_eq!(resp["error"]["code"], ErrorCode::InvalidRequest as i32);
        let out = d.handle_json(r#"{"jsonrpc":"2.0","method":"exit","params":null}"#);
        assert!(out.is_empty());
        assert!(d.exited());
    }

    // ── syntaqlite/setSessionContext extension ────────────────────────

    fn set_ddl_context(d: &mut LspDispatcher, id: u32, ddl: &str) -> Vec<serde_json::Value> {
        d.handle_json(&format!(
            r#"{{"jsonrpc":"2.0","id":{id},"method":"syntaqlite/setSessionContext","params":{{"ddl":"{ddl}"}}}}"#,
        ))
        .iter()
        .map(|m| serde_json::from_str(m).unwrap())
        .collect()
    }

    #[test]
    fn set_session_context_accepted_before_initialize() {
        let mut d = dispatcher();
        let out = set_ddl_context(&mut d, 1, "CREATE TABLE t(a INTEGER);");
        assert_eq!(out.len(), 1);
        assert_eq!(out[0]["result"]["errors"].as_array().unwrap().len(), 0);
        // Schema survives the subsequent handshake.
        initialize(&mut d);
        let open = d.handle_json(
            r#"{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///q.sql","languageId":"sql","version":1,"text":"SELECT c FROM t"}}}"#,
        );
        let notif: serde_json::Value = serde_json::from_str(&open[0]).unwrap();
        let messages: Vec<&str> = notif["params"]["diagnostics"]
            .as_array()
            .unwrap()
            .iter()
            .filter_map(|d| d["message"].as_str())
            .collect();
        assert!(
            messages.iter().any(|m| m.contains("unknown column")),
            "expected unknown-column diagnostic, got: {messages:?}"
        );
    }

    #[test]
    fn set_session_context_republishes_open_documents() {
        let mut d = dispatcher();
        initialize(&mut d);
        d.handle_json(
            r#"{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///q.sql","languageId":"sql","version":1,"text":"SELECT c FROM t"}}}"#,
        );
        let out = set_ddl_context(&mut d, 2, "CREATE TABLE t(a INTEGER);");
        assert_eq!(out.len(), 2, "expected response + publishDiagnostics");
        assert!(out[0]["result"]["errors"].as_array().unwrap().is_empty());
        assert_eq!(out[1]["method"], "textDocument/publishDiagnostics");
        assert!(
            out[1]["params"]["diagnostics"]
                .as_array()
                .unwrap()
                .iter()
                .any(|d| d["message"].as_str().unwrap().contains("unknown column")),
        );
    }

    #[test]
    fn clear_session_context_republishes() {
        let mut d = dispatcher();
        initialize(&mut d);
        set_ddl_context(&mut d, 2, "CREATE TABLE t(a INTEGER);");
        d.handle_json(
            r#"{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///q.sql","languageId":"sql","version":1,"text":"SELECT c FROM t"}}}"#,
        );
        let out = d.handle_json(
            r#"{"jsonrpc":"2.0","id":3,"method":"syntaqlite/setSessionContext","params":{}}"#,
        );
        let notif: serde_json::Value = serde_json::from_str(&out[1]).unwrap();
        assert!(
            !notif["params"]["diagnostics"]
                .as_array()
                .unwrap()
                .iter()
                .any(|d| d["message"].as_str().unwrap().contains("unknown column")),
            "clearing the context should drop schema diagnostics"
        );
    }

    #[test]
    fn set_session_context_reports_ddl_errors() {
        let mut d = dispatcher();
        let out = set_ddl_context(&mut d, 1, "CREATE TABLE t(a INTEGER); NOT SQL;");
        assert!(
            !out[0]["result"]["errors"].as_array().unwrap().is_empty(),
            "expected DDL parse errors in result"
        );
    }

    #[test]
    fn set_session_context_rejects_bad_params() {
        let mut d = dispatcher();
        let out = d.handle_json(
            r#"{"jsonrpc":"2.0","id":1,"method":"syntaqlite/setSessionContext","params":{"bogus":1}}"#,
        );
        let resp: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        assert_eq!(resp["error"]["code"], ErrorCode::InvalidParams as i32);
    }

    #[test]
    fn initialize_advertises_extension_and_range_tokens() {
        let mut d = dispatcher();
        let out = d.handle_json(
            r#"{"jsonrpc":"2.0","id":0,"method":"initialize","params":{"capabilities":{}}}"#,
        );
        let resp: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        let caps = &resp["result"]["capabilities"];
        assert_eq!(
            caps["experimental"]["syntaqlite"]["setSessionContext"],
            true
        );
        assert_eq!(caps["semanticTokensProvider"]["range"], true);
    }

    #[test]
    fn semantic_tokens_range_roundtrip() {
        let mut d = dispatcher();
        initialize(&mut d);
        d.handle_json(
            r#"{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///q.sql","languageId":"sql","version":1,"text":"SELECT 1;\nSELECT 2;"}}}"#,
        );
        let out = d.handle_json(
            r#"{"jsonrpc":"2.0","id":2,"method":"textDocument/semanticTokens/range","params":{"textDocument":{"uri":"file:///q.sql"},"range":{"start":{"line":0,"character":0},"end":{"line":0,"character":9}}}}"#,
        );
        let resp: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        let data = resp["result"]["data"].as_array().unwrap();
        assert!(!data.is_empty(), "expected tokens for the first line");
        assert_eq!(data.len() % 5, 0);
    }

    #[test]
    fn diagnostics_carry_structured_data() {
        let mut d = dispatcher();
        initialize(&mut d);
        let out = d.handle_json(
            r#"{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///q.sql","languageId":"sql","version":1,"text":"selec 1"}}}"#,
        );
        let notif: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        let diag = &notif["params"]["diagnostics"][0];
        assert!(diag["data"]["startOffset"].is_number(), "data: {diag}");
        assert!(diag["data"]["endOffset"].is_number());
    }

    #[test]
    fn malformed_message_yields_parse_error() {
        let mut d = dispatcher();
        let out = d.handle_json("this is not json");
        assert_eq!(out.len(), 1);
        let resp: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        assert_eq!(resp["error"]["code"], ErrorCode::ParseError as i32);
    }

    #[test]
    fn unknown_method_yields_method_not_found() {
        let mut d = dispatcher();
        initialize(&mut d);
        let out = d.handle_json(
            r#"{"jsonrpc":"2.0","id":5,"method":"textDocument/frobnicate","params":{}}"#,
        );
        let resp: serde_json::Value = serde_json::from_str(&out[0]).unwrap();
        assert_eq!(resp["error"]["code"], ErrorCode::MethodNotFound as i32);
    }

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
