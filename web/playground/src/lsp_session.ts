// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import type {DiagnosticEntry, Engine} from "syntaqlite";

const SERVER_NOT_INITIALIZED = -32002;

interface LspMessage {
  id?: unknown;
  method?: string;
  result?: unknown;
  error?: {code: number; message: string};
  params?: unknown;
}

export interface LspCompletionItem {
  label: string;
  kind?: number;
  sortText?: string;
  detail?: string;
}

/** Zero-based UTF-16 range, LSP-style. */
export interface LspRange {
  startLine: number;
  startCharacter: number;
  endLine: number;
  endCharacter: number;
}

/** LSP client over `Engine.lspMessage`.
 *
 *  Each call syncs the server's copy of the document (didOpen/didChange
 *  with full text) before requesting. Dialect switches rebuild the WASM
 *  language server, resetting its lifecycle; the client self-heals by
 *  redoing the handshake and retrying once. */
export class LspSession {
  private nextId = 1;
  private initialized = false;
  /** Last version synced to the server, per document URI. */
  private syncedVersions = new Map<string, number>();

  constructor(private engine: Engine) {}

  private send(msg: object): LspMessage[] {
    return this.engine.lspMessage(msg) as LspMessage[];
  }

  private reset(): void {
    this.initialized = false;
    this.syncedVersions.clear();
  }

  private ensureInitialized(): void {
    if (this.initialized) return;
    this.send({
      jsonrpc: "2.0", id: this.nextId++, method: "initialize",
      params: {capabilities: {}},
    });
    this.send({jsonrpc: "2.0", method: "initialized", params: {}});
    this.initialized = true;
  }

  /** Returns the messages the sync produced (e.g. publishDiagnostics).
   *  `force` re-sends even when the version is already synced. */
  private syncDocument(uri: string, text: string, version: number, force = false): LspMessage[] {
    const synced = this.syncedVersions.get(uri);
    if (!force && synced === version) return [];
    const out =
      synced === undefined
        ? this.send({
            jsonrpc: "2.0", method: "textDocument/didOpen",
            params: {textDocument: {uri, languageId: "sql", version, text}},
          })
        : this.send({
            jsonrpc: "2.0", method: "textDocument/didChange",
            params: {textDocument: {uri, version}, contentChanges: [{text}]},
          });
    this.syncedVersions.set(uri, version);
    return out;
  }

  private request(
    method: string,
    params: object,
    uri: string,
    text: string,
    version: number,
    retry = true,
  ): unknown {
    this.ensureInitialized();
    this.syncDocument(uri, text, version);
    const id = this.nextId++;
    const out = this.send({jsonrpc: "2.0", id, method, params});
    const response = out.find((m) => m.id === id);
    if (response?.error) {
      if (retry && response.error.code === SERVER_NOT_INITIALIZED) {
        this.reset();
        return this.request(method, params, uri, text, version, false);
      }
      throw new Error(response.error.message);
    }
    return response?.result;
  }

  /** Diagnostics from the publishDiagnostics push the sync triggers, as
   *  the structured entries carried in each diagnostic's `data`. */
  diagnostics(uri: string, text: string, version: number): DiagnosticEntry[] {
    try {
      return this.diagnosticsOnce(uri, text, version, true);
    } catch (e) {
      console.warn("LSP diagnostics failed:", e);
      return [];
    }
  }

  private diagnosticsOnce(
    uri: string,
    text: string,
    version: number,
    retry: boolean,
  ): DiagnosticEntry[] {
    this.ensureInitialized();
    const out = this.syncDocument(uri, text, version, true);
    const publish = out.find(
      (m) =>
        m.method === "textDocument/publishDiagnostics" &&
        (m.params as {uri?: string} | undefined)?.uri === uri,
    );
    if (!publish) {
      // Notifications are dropped when the server lost its lifecycle, so a
      // silent sync means the handshake must be redone.
      if (retry) {
        this.reset();
        return this.diagnosticsOnce(uri, text, version, false);
      }
      return [];
    }
    const diags = (publish.params as {diagnostics: Array<{data?: DiagnosticEntry}>}).diagnostics;
    return diags.map((d) => d.data).filter((d): d is DiagnosticEntry => d !== undefined);
  }

  /** Completions at a zero-based UTF-16 position. Empty on failure. */
  completions(
    uri: string,
    text: string,
    version: number,
    line: number,
    character: number,
  ): LspCompletionItem[] {
    try {
      const result = this.request(
        "textDocument/completion",
        {textDocument: {uri}, position: {line, character}},
        uri, text, version,
      );
      if (Array.isArray(result)) return result as LspCompletionItem[];
      const items = (result as {items?: unknown} | null | undefined)?.items;
      return Array.isArray(items) ? (items as LspCompletionItem[]) : [];
    } catch (e) {
      console.warn("LSP completion failed:", e);
      return [];
    }
  }

  /** Semantic tokens (5 u32s per token), full-document or ranged.
   *  Undefined on failure. */
  semanticTokens(
    uri: string,
    text: string,
    version: number,
    range?: LspRange,
  ): Uint32Array | undefined {
    try {
      const method = range
        ? "textDocument/semanticTokens/range"
        : "textDocument/semanticTokens/full";
      const params = range
        ? {
            textDocument: {uri},
            range: {
              start: {line: range.startLine, character: range.startCharacter},
              end: {line: range.endLine, character: range.endCharacter},
            },
          }
        : {textDocument: {uri}};
      const result = this.request(method, params, uri, text, version) as
        | {data?: number[]}
        | null
        | undefined;
      const data = result?.data;
      return Array.isArray(data) ? Uint32Array.from(data) : new Uint32Array(0);
    } catch (e) {
      console.warn("LSP semantic tokens failed:", e);
      return undefined;
    }
  }
}
