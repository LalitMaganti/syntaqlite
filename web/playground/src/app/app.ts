// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import m from "mithril";
import {
  Engine,
  DialectManager,
  DialectConfigManager,
  SchemaContextManager,
} from "syntaqlite";
import type {DiagnosticEntry, EmbeddedFragment, EmbeddedLanguage} from "syntaqlite";
import {ThemeManager} from "./theme_manager";
import {WindowManager} from "./window_manager";
import {UrlStateManager} from "./url_state";
import {LspSession} from "../lsp_session";

export interface Attrs {
  app: App;
}

export class App {
  urlState: UrlStateManager;
  theme: ThemeManager;
  runtime: Engine;
  /** LSP client over the runtime's in-process language server. All SQL
   *  editor features (diagnostics, completions, semantic tokens) go
   *  through this. */
  lsp: LspSession;
  dialect: DialectManager;
  dialectConfig: DialectConfigManager;
  schemaContext: SchemaContextManager;
  window: WindowManager;
  diagnostics: DiagnosticEntry[] = [];
  customDialectNoticeDismissed = false;
  /** Set by the workspace to reveal a diagnostic in the editor. */
  revealDiagnostic: ((d: DiagnosticEntry) => void) | undefined = undefined;

  /** Current editor language mode. Embedded modes (python, typescript) are experimental. */
  languageMode: "sql" | EmbeddedLanguage = "sql";
  /** Extracted SQL fragments in embedded mode (experimental). */
  embeddedFragments: EmbeddedFragment[] = [];
  /** Selected fragment index (-1 = show all). */
  selectedFragmentIndex = -1;

  constructor() {
    // UrlStateManager is constructed first: it is the single source of truth
    // for all serializable state. Other managers hold only non-serializable
    // (computed / engine-derived) state.
    this.urlState = new UrlStateManager();

    this.theme = new ThemeManager();
    this.runtime = new Engine({runtimeJsPath: "./syntaqlite-runtime.js"});
    this.lsp = new LspSession(this.runtime);
    this.dialect = new DialectManager({
      presets: [
        {
          id: "sqlite",
          label: "SQLite",
          wasmUrl: "./syntaqlite-sqlite.wasm",
          symbol: "syntaqlite_sqlite_dialect_template",
        },
        {
          id: "perfetto",
          label: "PerfettoSQL",
          wasmUrl: "./syntaqlite-perfetto.wasm",
          symbol: "syntaqlite_perfetto_dialect_template",
        },
      ],
      onDialectChanged: () => m.redraw(),
    });
    this.dialectConfig = new DialectConfigManager();
    this.schemaContext = new SchemaContextManager();

    this.languageMode = this.urlState.current.languageMode;

    this.window = new WindowManager();
  }
}
