// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Re-export all library types so existing component imports from "../types" continue to work.
export type {
  EmscriptenModule,
  EmscriptenModuleConfig,
  AstFieldValue,
  AstListNode,
  AstRegularNode,
  AstJsonNode,
  KeywordCase,
  ParseResult,
  FormatOptions,
  DialectBinding,
  DiagnosticDetail,
  HelpDetail,
  DiagnosticEntry,
  DiagnosticsResult,
  CompletionEntry,
  CompletionsResult,
} from "syntaqlite";

// ── Playground-only types ──

export type Theme = "dark" | "light";
export type ActiveTab = "format" | "ast" | "validation" | "schema";

// ── Playground-local view types ──

import type {AstJsonNode} from "syntaqlite";

/** Rendered outcome of a format run, including engine errors. */
export interface FormatResult {
  ok: boolean;
  text: string;
}

/** Rendered outcome of a parse run, including engine errors. */
export type AstResult = {ok: true; statements: AstJsonNode[]} | {ok: false; error: string};
