// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

/** Emscripten module augmented with dynamic linking support. */
export interface EmscriptenModule {
  HEAPU8: Uint8Array;
  loadDynamicLibrary: (
    url: string,
    opts: {loadAsync: boolean; global: boolean; nodelete: boolean},
    scope?: Record<string, unknown>,
  ) => unknown;
  ccall: (ident: string, returnType: string, argTypes: string[], args: unknown[]) => unknown;
  cwrap: (ident: string, returnType: string, argTypes: string[]) => (...args: unknown[]) => number;
  [key: `_${string}`]: ((...args: number[]) => number) | undefined;
}

/** Module config passed to the Emscripten MODULARIZE factory. */
export interface EmscriptenModuleConfig {
  noInitialRun: boolean;
  locateFile: (path: string) => string;
  // Emscripten populates these after init (the factory returns the same object):
  HEAPU8?: Uint8Array;
  loadDynamicLibrary?: EmscriptenModule["loadDynamicLibrary"];
  ccall?: EmscriptenModule["ccall"];
  cwrap?: EmscriptenModule["cwrap"];
  [key: string]: unknown;
}

// ── AST JSON types ──
//
// The JSON mirrors the text dump format. Every node has a `type` key (the
// node name). List nodes additionally have `count` and `children`. Regular
// nodes carry their fields as direct keys whose values are:
//   - null           → absent / "(none)" in the dump
//   - string         → span or enum display name
//   - boolean        → bool field
//   - string[]       → flags (active flag names)
//   - AstJsonNode    → child node

export type AstFieldValue = AstJsonNode | string | boolean | string[] | null;

/** A list node: `{ "type": "ResultColumnList", "count": 2, "children": [...] }` */
export interface AstListNode {
  type: string;
  count: number;
  children: AstJsonNode[];
}

/** A regular node: `{ "type": "SelectStmt", "flags": [], "columns": {...}, ... }` */
export interface AstRegularNode {
  type: string;
  [field: string]: AstFieldValue | undefined;
}

export type AstJsonNode = AstListNode | AstRegularNode;

// ── Format types ──

export type KeywordCase = 0 | 1 | 2; // 0=preserve, 1=upper, 2=lower

export interface FormatOptions {
  lineWidth: number;
  indentWidth: number;
  keywordCase: KeywordCase;
  semicolons: boolean;
}

export interface FormatResult {
  ok: boolean;
  text: string;
}

// ── AST result types ──

export type AstResultOk = {ok: true; statements: AstJsonNode[]};
export type AstResultError = {ok: false; error: string};
export type AstResult = AstResultOk | AstResultError;

// ── Dialect types ──

export interface DialectBinding {
  symbol: string;
  ptr: number;
  label: string;
}

// ── Diagnostics types ──

/** Structured detail for the diagnostic message, matching `DiagnosticMessage` in Rust. */
export type DiagnosticDetail =
  | {kind: "unknown_table"; name: string}
  | {kind: "unknown_column"; column: string; table?: string}
  | {kind: "unknown_function"; name: string}
  | {kind: "function_arity"; name: string; expected: number[]; got: number}
  | null;

/** Structured detail for the help, matching `Help` in Rust. */
export type HelpDetail = {kind: "suggestion"; value: string} | null;

export interface DiagnosticEntry {
  startOffset: number;
  endOffset: number;
  /** Human-readable message (Display string). */
  message: string;
  /** Structured detail for machine consumption. `null` for parse errors. */
  detail: DiagnosticDetail;
  severity: "error" | "warning" | "info" | "hint";
  /** Human-readable help text (Display string). */
  help?: string;
  /** Structured help for machine consumption. */
  helpDetail?: HelpDetail;
  /** 1-based line number, populated by the consumer after offset conversion. */
  line?: number;
  /** 1-based column number, populated by the consumer after offset conversion. */
  col?: number;
  /** 1-based statement index (semicolon-delimited), populated by the consumer. */
  stmtIndex?: number;
}

export interface DiagnosticsResult {
  ok: boolean;
  diagnostics: DiagnosticEntry[];
}

/** One diagnostic from `Engine.analyze` (the CLI `analyze` op shape). */
export interface AnalyzeDiagnostic {
  severity: "error" | "warning" | "info" | "hint";
  message: string;
  start_offset: number;
  end_offset: number;
  /** Numeric diagnostic category (see `DIAG_CODE_*` in the Rust rpc module). */
  code: number;
  help?: string;
}

export interface AnalyzeOptions {
  /** DDL parsed into a schema catalog for schema-aware validation. */
  schemaDdl?: string;
}

export interface AnalyzeResult {
  diagnostics: AnalyzeDiagnostic[];
  /** Per-statement analysis (kind, references, lineage). */
  statements: unknown[];
  /** Lineage of the final query-bearing statement. */
  lineage: unknown | null;
}

// ── Embedded SQL types (experimental) ──

/** @experimental Embedded language support is experimental and may change. */
export type EmbeddedLanguage = "python" | "typescript";

/** @experimental */
export interface EmbeddedHole {
  start: number;
  end: number;
  placeholder: string;
}

/** @experimental */
export interface EmbeddedFragment {
  start: number;
  end: number;
  sql: string;
  holes: EmbeddedHole[];
}

/** @experimental */
export interface EmbeddedExtractResult {
  ok: boolean;
  fragments: EmbeddedFragment[];
}

// ── Completion types ──

export interface CompletionEntry {
  label: string;
  kind: "keyword" | "function" | "class";
}

export interface CompletionsResult {
  ok: boolean;
  items: CompletionEntry[];
}
