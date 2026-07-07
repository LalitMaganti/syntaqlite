// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import type {
  AstResult,
  DiagnosticEntry,
  DiagnosticsResult,
  DialectBinding,
  EmbeddedExtractResult,
  EmbeddedFragment,
  EmbeddedLanguage,
  EmscriptenModule,
  EmscriptenModuleConfig,
  FormatOptions,
  FormatResult,
} from "./types.js";

export interface CflagEntry {
  name: string;
  minVersion: number;
  category: string;
}

const DEFAULT_RUNTIME_JS = new URL("../wasm/syntaqlite-runtime.js", import.meta.url).href;
const DEFAULT_RUNTIME_WASM = new URL("../wasm/syntaqlite-runtime.wasm", import.meta.url).href;

export interface EngineConfig {
  runtimeJsPath?: string;
  runtimeWasmPath?: string;
  /** Reuse an already-loaded runtime module instead of loading a new one.
   *  The engines share linear memory and loaded dialect modules but get
   *  independent WASM sessions (dialect, schema context, analysis state). */
  runtime?: EmscriptenModule;
}

type WasmFn = (...args: number[]) => number;

/** Host-language code for the embedded-SQL WASM exports. */
function embeddedLangCode(lang: EmbeddedLanguage): number {
  return lang === "python" ? 0 : 1;
}

export class Engine {
  status = "Loading...";
  statusError = false;

  private config: EngineConfig;
  private module: EmscriptenModule | undefined = undefined;
  private encoder = new TextEncoder();
  private decoder = new TextDecoder();

  private sessionNewRaw: WasmFn | undefined = undefined;
  private sessionFreeRaw: WasmFn | undefined = undefined;
  private setDialectRaw: WasmFn | undefined = undefined;
  private clearDialectRaw: WasmFn | undefined = undefined;
  private allocRaw: WasmFn | undefined = undefined;
  private freeRaw: WasmFn | undefined = undefined;
  private astJsonRaw: WasmFn | undefined = undefined;
  private fmtRaw: WasmFn | undefined = undefined;
  private resultPtrRaw: WasmFn | undefined = undefined;
  private resultLenRaw: WasmFn | undefined = undefined;
  private resultFreeRaw: WasmFn | undefined = undefined;
  private setSqliteVersionRaw: WasmFn | undefined = undefined;
  private setCflagRaw: WasmFn | undefined = undefined;
  private clearCflagRaw: WasmFn | undefined = undefined;
  private clearAllCflagsRaw: WasmFn | undefined = undefined;
  private getCflagListRaw: WasmFn | undefined = undefined;
  private embeddedExtractRaw: WasmFn | undefined = undefined;
  private embeddedDiagnosticsRaw: WasmFn | undefined = undefined;
  private embeddedSemanticTokensRaw: WasmFn | undefined = undefined;
  private lspMessageRaw: WasmFn | undefined = undefined;
  private currentLangMode: "sql" | EmbeddedLanguage = "sql";
  /** Handle for the WASM session all calls run against. 0 = not created yet. */
  private session = 0;
  /** Sequence for `syntaqlite/setSessionContext` request ids. */
  private contextSeq = 0;
  /** Last session context applied, so it can be re-applied after dialect switches. */
  private sessionContext: {kind: "json"; json: string} | {kind: "ddl"; sql: string} | null = null;

  constructor(config: EngineConfig = {}) {
    this.config = config;
  }

  get ready(): boolean {
    return this.module !== undefined;
  }

  /** The underlying Emscripten module, e.g. to share with another Engine
   *  via `EngineConfig.runtime`. Undefined until load() completes. */
  get runtimeModule(): EmscriptenModule | undefined {
    return this.module;
  }

  /** Free the WASM session. The engine must not be used after this. */
  dispose(): void {
    if (this.session !== 0 && this.sessionFreeRaw) {
      this.sessionFreeRaw(this.session);
      this.session = 0;
    }
  }

  updateStatus(text: string, isError = false): void {
    this.status = text;
    this.statusError = isError;
  }

  async load(): Promise<void> {
    const module = this.config.runtime ?? (await loadRuntimeModule(this.config));
    this.module = module;
    this.sessionNewRaw = this.resolveRuntimeFn("wasm_session_new");
    this.sessionFreeRaw = this.resolveRuntimeFn("wasm_session_free");
    this.setDialectRaw = this.tryResolveRuntimeFn("wasm_set_dialect");
    this.clearDialectRaw = this.tryResolveRuntimeFn("wasm_clear_dialect");
    this.allocRaw = this.resolveRuntimeFn("wasm_alloc");
    this.freeRaw = this.resolveRuntimeFn("wasm_free");
    this.astJsonRaw = this.tryResolveRuntimeFn("wasm_ast_json");
    this.fmtRaw = this.resolveRuntimeFn("wasm_fmt");
    this.resultPtrRaw = this.resolveRuntimeFn("wasm_result_ptr");
    this.resultLenRaw = this.resolveRuntimeFn("wasm_result_len");
    this.resultFreeRaw = this.resolveRuntimeFn("wasm_result_free");
    this.setSqliteVersionRaw = this.tryResolveRuntimeFn("wasm_set_sqlite_version");
    this.setCflagRaw = this.tryResolveRuntimeFn("wasm_set_cflag");
    this.clearCflagRaw = this.tryResolveRuntimeFn("wasm_clear_cflag");
    this.clearAllCflagsRaw = this.tryResolveRuntimeFn("wasm_clear_all_cflags");
    this.getCflagListRaw = this.tryResolveRuntimeFn("wasm_get_cflag_list");
    this.embeddedExtractRaw = this.tryResolveRuntimeFn("wasm_embedded_extract");
    this.embeddedDiagnosticsRaw = this.tryResolveRuntimeFn("wasm_embedded_diagnostics");
    this.embeddedSemanticTokensRaw = this.tryResolveRuntimeFn("wasm_embedded_semantic_tokens");
    this.lspMessageRaw = this.tryResolveRuntimeFn("wasm_lsp_message");
    this.session = this.sessionNewRaw() >>> 0;
    if (this.session === 0) {
      throw new Error("wasm_session_new failed");
    }
  }

  private resolveRuntimeFn(symbol: string): WasmFn {
    const fn = this.module![`_${symbol}`];
    if (typeof fn !== "function") {
      throw new Error(`missing runtime function: _${symbol}`);
    }
    return fn;
  }

  /** Like resolveRuntimeFn but returns undefined if not found. */
  private tryResolveRuntimeFn(symbol: string): WasmFn | undefined {
    const fn = this.module![`_${symbol}`];
    return typeof fn === "function" ? fn : undefined;
  }

  private resolveDialectFn(
    symbol: string,
    localScope: Record<string, unknown> | undefined = undefined,
  ): WasmFn {
    if (localScope && typeof localScope[symbol] === "function") {
      return localScope[symbol] as WasmFn;
    }
    if (localScope && typeof localScope[`_${symbol}`] === "function") {
      return localScope[`_${symbol}`] as WasmFn;
    }
    const direct = this.module![`_${symbol}`];
    if (typeof direct === "function") {
      return direct;
    }
    if (typeof this.module!.cwrap === "function") {
      try {
        return this.module!.cwrap(symbol, "number", []);
      } catch {
        // Fall through to explicit error below.
      }
    }
    throw new Error(`missing dialect symbol: ${symbol}`);
  }

  private heapU8(): Uint8Array {
    const heap = this.module!.HEAPU8;
    if (!heap) throw new Error("runtime HEAPU8 is not available");
    return heap;
  }

  async loadDialectFromUrl(url: string, symbol: string): Promise<DialectBinding> {
    const localScope: Record<string, unknown> = {};
    if (url) {
      if (typeof this.module!.loadDynamicLibrary !== "function") {
        throw new Error("runtime module does not expose loadDynamicLibrary");
      }
      const maybePromise = this.module!.loadDynamicLibrary(
        url,
        {loadAsync: true, global: false, nodelete: true},
        localScope,
      );
      if (maybePromise && typeof (maybePromise as Promise<void>).then === "function") {
        await maybePromise;
      }
    }
    let ptr: number;
    try {
      const fn = this.resolveDialectFn(symbol, localScope);
      ptr = fn() >>> 0;
    } catch {
      throw new Error(`Symbol "${symbol}" not found in the WASM module.`);
    }
    if (ptr === 0) throw new Error(`Symbol "${symbol}" returned undefined.`);
    this.setDialectPointer(ptr);
    return {symbol, ptr, label: symbol};
  }

  private withInput<T>(sql: string, fn: (ptr: number, len: number) => T): T {
    const input = this.encoder.encode(sql);
    const ptr = this.allocRaw!(input.length);
    if (input.length > 0 && ptr === 0) throw new Error("allocation failed");
    if (input.length > 0) this.heapU8().set(input, ptr);
    try {
      return fn(ptr, input.length);
    } finally {
      this.freeRaw!(ptr, input.length);
    }
  }

  private readAndClearResult(): string {
    const ptr = this.resultPtrRaw!();
    const len = this.resultLenRaw!();
    const text = len === 0 ? "" : this.decoder.decode(this.heapU8().subarray(ptr, ptr + len));
    this.resultFreeRaw!();
    return text;
  }

  setDialectPointer(ptr: number): void {
    if (!this.setDialectRaw) throw new Error("dialect switching not supported by this runtime");
    const status = this.setDialectRaw(this.session, ptr >>> 0);
    const detail = this.readAndClearResult();
    if (status !== 0) {
      throw new Error(detail || `wasm_set_dialect failed with status ${status}`);
    }
    // The WASM drops the language server on dialect switch, discarding any
    // session context. Re-apply it so callers don't have to track this.
    this.reapplySessionContext();
  }

  private reapplySessionContext(): void {
    if (!this.sessionContext) return;
    if (this.sessionContext.kind === "json") {
      this.applySessionContextJson(this.sessionContext.json);
    } else {
      this.applySessionContextDdl(this.sessionContext.sql);
    }
  }

  clearDialectPointer(): void {
    if (!this.clearDialectRaw) return;
    this.clearDialectRaw(this.session);
    this.readAndClearResult();
  }

  runAstJson(sql: string): AstResult {
    if (!this.astJsonRaw) return {ok: false, error: "AST JSON not supported by this runtime"};
    const count = this.withInput(sql, (ptr, len) => this.astJsonRaw!(this.session, ptr, len));
    const text = this.readAndClearResult();
    if (count < 0) return {ok: false, error: text};
    if (count === 0) return {ok: true, statements: []};
    try {
      return {ok: true, statements: JSON.parse(text, (_, v) => (v === null ? undefined : v))};
    } catch (e) {
      return {ok: false, error: `JSON parse error: ${(e as Error).message}`};
    }
  }

  runFmt(sql: string, opts: FormatOptions): FormatResult {
    const status = this.withInput(sql, (ptr, len) =>
      this.fmtRaw!(this.session, ptr, len, opts.lineWidth, opts.indentWidth, opts.keywordCase, opts.semicolons ? 1 : 0),
    );
    const text = this.readAndClearResult();
    return {ok: status === 0, text};
  }

  /** Semantic tokens for an embedded-language document (see setLanguageMode).
   *  Returns a pre-encoded Uint32Array (5 u32s per token) or undefined on
   *  failure. SQL documents are served over LSP (`textDocument/semanticTokens`).
   *  @experimental Embedded language support is experimental and may change. */
  runEmbeddedSemanticTokens(source: string): Uint32Array | undefined {
    if (this.currentLangMode === "sql" || !this.embeddedSemanticTokensRaw) return undefined;
    const lang = embeddedLangCode(this.currentLangMode);
    try {
      const count = this.withInput(source, (ptr, len) =>
        this.embeddedSemanticTokensRaw!(this.session, lang, ptr, len),
      );
      if (count <= 0) {
        this.resultFreeRaw!();
        return count === 0 ? new Uint32Array(0) : undefined;
      }
      // Read raw bytes from RESULT_BUF as a Uint32Array (5 u32s per token).
      const rptr = this.resultPtrRaw!();
      const rlen = this.resultLenRaw!();
      const bytes = this.heapU8().slice(rptr, rptr + rlen);
      this.resultFreeRaw!();
      return new Uint32Array(bytes.buffer, bytes.byteOffset, bytes.byteLength / 4);
    } catch (e) {
      console.warn("wasm_embedded_semantic_tokens failed:", e);
      return undefined;
    }
  }

  /** Diagnostics for an embedded-language document (see setLanguageMode).
   *  SQL documents are served over LSP (`textDocument/publishDiagnostics`).
   *  @experimental Embedded language support is experimental and may change. */
  runEmbeddedDiagnostics(source: string): DiagnosticsResult {
    if (this.currentLangMode === "sql" || !this.embeddedDiagnosticsRaw) {
      return {ok: false, diagnostics: []};
    }
    const lang = embeddedLangCode(this.currentLangMode);
    try {
      const count = this.withInput(source, (ptr, len) =>
        this.embeddedDiagnosticsRaw!(this.session, lang, ptr, len),
      );
      const text = this.readAndClearResult();
      if (count < 0) return {ok: false, diagnostics: []};
      if (count === 0) return {ok: true, diagnostics: []};
      const diagnostics: DiagnosticEntry[] = JSON.parse(text);
      return {ok: true, diagnostics};
    } catch (e) {
      console.warn("wasm_embedded_diagnostics failed:", e);
      return {ok: false, diagnostics: []};
    }
  }

  /** Whether this runtime exposes the LSP JSON-RPC entry point. */
  get lspSupported(): boolean {
    return this.lspMessageRaw !== undefined;
  }

  /** Handle one LSP JSON-RPC message against this engine's in-process
   *  language server. Returns the outgoing messages as parsed objects
   *  (response plus server notifications). Drive it exactly like a
   *  standalone LSP server, lifecycle included. */
  lspMessage(message: string | object): unknown[] {
    if (!this.lspMessageRaw) {
      throw new Error("LSP not supported by this runtime");
    }
    const json = typeof message === "string" ? message : JSON.stringify(message);
    const count = this.withInput(json, (ptr, len) => this.lspMessageRaw!(this.session, ptr, len));
    const text = this.readAndClearResult();
    if (count < 0) {
      throw new Error(text || "wasm_lsp_message failed");
    }
    return count === 0 ? [] : (JSON.parse(text) as unknown[]);
  }

  /** Set the active language mode. Diagnostics, semantic tokens, and extraction
   *  dispatch to the SQL or embedded implementation based on this mode.
   *  @experimental Embedded language support is experimental and may change. */
  setLanguageMode(lang: "sql" | EmbeddedLanguage): void {
    this.currentLangMode = lang;
  }

  /** Extract SQL fragments from `source`. Returns empty in SQL mode (O(1) fast path).
   *  In embedded mode the WASM extractor runs for the language set by setLanguageMode.
   *  @experimental Embedded language support is experimental and may change. */
  runExtract(source: string): EmbeddedExtractResult {
    if (this.currentLangMode === "sql") return {ok: true, fragments: []};
    if (!this.embeddedExtractRaw) return {ok: true, fragments: []};
    const lang = embeddedLangCode(this.currentLangMode);
    try {
      const count = this.withInput(source, (ptr, len) => this.embeddedExtractRaw!(lang, ptr, len));
      const text = this.readAndClearResult();
      if (count < 0) return {ok: false, fragments: []};
      if (count === 0) return {ok: true, fragments: []};
      const fragments: EmbeddedFragment[] = JSON.parse(text);
      return {ok: true, fragments};
    } catch (e) {
      console.warn("wasm_embedded_extract failed:", e);
      return {ok: false, fragments: []};
    }
  }

  setSqliteVersion(version: string): void {
    if (!this.setSqliteVersionRaw) return;
    const status = this.withInput(version, (ptr, len) =>
      this.setSqliteVersionRaw!(this.session, ptr, len),
    );
    const detail = this.readAndClearResult();
    if (status !== 0) {
      throw new Error(detail || `wasm_set_sqlite_version failed with status ${status}`);
    }
  }

  setCflag(name: string): void {
    if (!this.setCflagRaw) return;
    const status = this.withInput(name, (ptr, len) => this.setCflagRaw!(this.session, ptr, len));
    const detail = this.readAndClearResult();
    if (status !== 0) {
      throw new Error(detail || `wasm_set_cflag failed with status ${status}`);
    }
  }

  clearCflag(name: string): void {
    if (!this.clearCflagRaw) return;
    const status = this.withInput(name, (ptr, len) => this.clearCflagRaw!(this.session, ptr, len));
    const detail = this.readAndClearResult();
    if (status !== 0) {
      throw new Error(detail || `wasm_clear_cflag failed with status ${status}`);
    }
  }

  clearAllCflags(): void {
    if (!this.clearAllCflagsRaw) return;
    this.clearAllCflagsRaw(this.session);
  }

  getCflagList(): CflagEntry[] {
    if (!this.getCflagListRaw) return [];
    this.getCflagListRaw();
    const text = this.readAndClearResult();
    if (!text) return [];
    try {
      return JSON.parse(text);
    } catch {
      return [];
    }
  }

  /** Set the schema session context from structured catalog JSON, via the
   *  `syntaqlite/setSessionContext` extension request. Throws on rejection. */
  setSessionContext(json: string): void {
    this.sessionContext = {kind: "json", json};
    this.applySessionContextJson(json);
  }

  clearSessionContext(): void {
    this.sessionContext = null;
    try {
      this.applySessionContext({});
    } catch (e) {
      // No dialect loaded yet means there is no context to clear.
      console.warn("clearSessionContext skipped:", e);
    }
  }

  /** Set the schema session context from DDL. DDL that fails to parse is
   *  reported in `error`; statements that did parse are still applied. */
  setSessionContextDdl(sql: string): {ok: true} | {ok: false; error: string} {
    const result = this.applySessionContextDdl(sql);
    if (result.ok) this.sessionContext = {kind: "ddl", sql};
    return result;
  }

  private applySessionContextJson(json: string): void {
    this.applySessionContext({context: JSON.parse(json) as object});
  }

  private applySessionContextDdl(sql: string): {ok: true} | {ok: false; error: string} {
    try {
      const errors = this.applySessionContext({ddl: sql});
      if (errors.length > 0) return {ok: false, error: errors.join("\n")};
      return {ok: true};
    } catch (e) {
      return {ok: false, error: (e as Error).message};
    }
  }

  /** Send a `syntaqlite/setSessionContext` extension request to the
   *  language server. Returns the (possibly empty) DDL parse errors;
   *  throws if the server rejects the request. */
  private applySessionContext(params: object): string[] {
    const id = `session-context-${this.contextSeq++}`;
    const out = this.lspMessage({
      jsonrpc: "2.0",
      id,
      method: "syntaqlite/setSessionContext",
      params,
    }) as Array<{id?: unknown; result?: {errors?: string[]}; error?: {message: string}}>;
    const response = out.find((m) => m && typeof m === "object" && m.id === id);
    if (response?.error) {
      throw new Error(response.error.message);
    }
    return response?.result?.errors ?? [];
  }
}

type RuntimeFactory = (config: EmscriptenModuleConfig) => Promise<EmscriptenModule>;

// Global name the Emscripten MODULARIZE build assigns the factory to when
// loaded as a classic <script> in the browser. Must match EXPORT_NAME in
// tools/build-web-playground.
const FACTORY_NAME = "createSyntaqliteRuntime";

async function loadRuntimeModule(config: EngineConfig): Promise<EmscriptenModule> {
  const jsPath = config.runtimeJsPath ?? DEFAULT_RUNTIME_JS;
  const wasmPath = config.runtimeWasmPath
    ?? (config.runtimeJsPath ? config.runtimeJsPath.replace(/\.js$/, ".wasm") : DEFAULT_RUNTIME_WASM);
  const moduleConfig: EmscriptenModuleConfig = {
    noInitialRun: true,
    locateFile(path: string) {
      if (path === "syntaqlite_wasm.wasm" || path === "syntaqlite-wasm.wasm") {
        return wasmPath;
      }
      return path;
    },
  };
  const factory = await loadFactory(jsPath);
  return factory(moduleConfig);
}

function loadFactory(jsPath: string): Promise<RuntimeFactory> {
  if (typeof document !== "undefined") {
    // Browser: inject <script>; Emscripten assigns the factory to a global.
    return new Promise((resolve, reject) => {
      const existing = (globalThis as Record<string, unknown>)[FACTORY_NAME];
      if (typeof existing === "function") {
        resolve(existing as RuntimeFactory);
        return;
      }
      const script = document.createElement("script");
      script.src = jsPath;
      script.async = true;
      script.onload = () => {
        const fn = (globalThis as Record<string, unknown>)[FACTORY_NAME];
        if (typeof fn !== "function") {
          reject(new Error(`${jsPath} did not define ${FACTORY_NAME}`));
          return;
        }
        resolve(fn as RuntimeFactory);
      };
      script.onerror = () => reject(new Error(`failed to load ${jsPath}`));
      document.head.appendChild(script);
    });
  }
  // Node / Bun: dynamic import. The runtime ships with a wasm/package.json
  // hint ({"type": "commonjs"}) so Node's CJS loader handles the Emscripten
  // output; the factory comes back as the ESM default export.
  return import(/* @vite-ignore */ /* webpackIgnore: true */ jsPath).then((mod) => {
    const factory = (mod as {default?: unknown}).default ?? mod;
    if (typeof factory !== "function") {
      throw new Error(`${jsPath} did not export a factory function`);
    }
    return factory as RuntimeFactory;
  });
}
