// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

export { Engine, type EngineConfig, type CflagEntry } from "./engine.js";
export { attachLspPort, type LspPortLike } from "./lsp.js";
export { DialectManager, BUILTIN_PRESETS, type DialectPreset, type DialectManagerConfig } from "./dialect.js";
export { DialectConfigManager, VERSION_OPTIONS, versionToInt } from "./dialect_config.js";
export { SchemaContextManager, parseSimple, type SchemaFormat, type SessionContextPayload } from "./schema.js";
export type {
  EmscriptenModule,
  EmscriptenModuleConfig,
  AstFieldValue,
  AstListNode,
  AstRegularNode,
  AstJsonNode,
  KeywordCase,
  FormatOptions,
  FormatResult,
  AstResultOk,
  AstResultError,
  AstResult,
  DialectBinding,
  DiagnosticDetail,
  HelpDetail,
  DiagnosticEntry,
  DiagnosticsResult,
  CompletionEntry,
  CompletionsResult,
  EmbeddedLanguage,
  EmbeddedHole,
  EmbeddedFragment,
  EmbeddedExtractResult,
} from "./types.js";
