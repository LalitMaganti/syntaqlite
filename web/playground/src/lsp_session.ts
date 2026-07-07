// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import type {Engine} from "syntaqlite";

/** JSON-RPC error code for "server not initialized". */
const SERVER_NOT_INITIALIZED = -32002;

interface LspMessage {
  id?: number;
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

/** Minimal LSP client over `Engine.lspMessage` for editor features.
 *
 *  Documents are synced lazily: each request first brings the server's copy
 *  of the document up to the caller's version (didOpen/didChange with full
 *  text). Server pushes (publishDiagnostics) are discarded — the playground
 *  has its own diagnostics pipeline.
 *
 *  Dialect switches rebuild the WASM session's language server, which
 *  resets its lifecycle. The client self-heals: on a "server not
 *  initialized" error it re-runs the handshake, re-opens the document, and
 *  retries once. */
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

  private syncDocument(uri: string, text: string, version: number): void {
    const synced = this.syncedVersions.get(uri);
    if (synced === version) return;
    if (synced === undefined) {
      this.send({
        jsonrpc: "2.0", method: "textDocument/didOpen",
        params: {textDocument: {uri, languageId: "sql", version, text}},
      });
    } else {
      this.send({
        jsonrpc: "2.0", method: "textDocument/didChange",
        params: {textDocument: {uri, version}, contentChanges: [{text}]},
      });
    }
    this.syncedVersions.set(uri, version);
  }

  /** Request completions at a zero-based UTF-16 `line`/`character` position,
   *  syncing the document first. Returns an empty list on any failure. */
  completions(
    uri: string,
    text: string,
    version: number,
    line: number,
    character: number,
  ): LspCompletionItem[] {
    try {
      return this.completionsOnce(uri, text, version, line, character, true);
    } catch (e) {
      console.warn("LSP completion failed:", e);
      return [];
    }
  }

  private completionsOnce(
    uri: string,
    text: string,
    version: number,
    line: number,
    character: number,
    retry: boolean,
  ): LspCompletionItem[] {
    this.ensureInitialized();
    this.syncDocument(uri, text, version);
    const id = this.nextId++;
    const out = this.send({
      jsonrpc: "2.0", id, method: "textDocument/completion",
      params: {textDocument: {uri}, position: {line, character}},
    });
    const response = out.find((m) => m.id === id);
    if (response?.error) {
      // The WASM language server is rebuilt on dialect switches, losing the
      // handshake and document state; redo both and retry once.
      if (retry && response.error.code === SERVER_NOT_INITIALIZED) {
        this.reset();
        return this.completionsOnce(uri, text, version, line, character, false);
      }
      console.warn("LSP completion error:", response.error.message);
      return [];
    }
    const result = response?.result;
    if (Array.isArray(result)) return result as LspCompletionItem[];
    // CompletionList shape ({isIncomplete, items}) or null.
    const items = (result as {items?: unknown} | null | undefined)?.items;
    return Array.isArray(items) ? (items as LspCompletionItem[]) : [];
  }
}
