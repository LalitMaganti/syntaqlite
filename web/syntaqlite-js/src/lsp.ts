// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import type {Engine} from "./engine.js";

/** The subset of `MessagePort` / `DedicatedWorkerGlobalScope` needed to
 *  bridge LSP messages. */
export interface LspPortLike {
  postMessage(message: unknown): void;
  onmessage: ((ev: {data: unknown}) => void) | null;
}

/** Bridge LSP JSON-RPC messages between a `postMessage` channel and the
 *  engine's in-process language server, so a standard browser LSP client
 *  (CodeMirror, Monaco, ...) can talk to syntaqlite running in a Web Worker:
 *
 *  ```ts
 *  const engine = new Engine();
 *  await engine.load();
 *  await new DialectManager().loadDefault(engine);
 *  attachLspPort(engine, self as unknown as LspPortLike);
 *  ```
 */
export function attachLspPort(engine: Engine, port: LspPortLike): void {
  port.onmessage = (ev) => {
    for (const out of engine.lspMessage(ev.data as object)) {
      port.postMessage(out);
    }
  };
}
