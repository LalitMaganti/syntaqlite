// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import m from "mithril";
import {App} from "./app/app";
import * as monaco from "monaco-editor";
import "monaco-editor/esm/vs/basic-languages/sql/sql.contribution";
import "monaco-editor/esm/vs/basic-languages/python/python.contribution";
import "monaco-editor/esm/vs/basic-languages/typescript/typescript.contribution";
import {FORMATTED_MODEL_URI, INPUT_MODEL_URI} from "./app/editor_models";
import type {Engine} from "syntaqlite";
import {LspSession} from "./lsp_session";
import {AppComponent} from "./components/app";
import "./styles/main.css";

function setupMonacoWorkers() {
  self.MonacoEnvironment = {
    getWorker(_workerId: string, label: string): Worker {
      if (label === "json") {
        return new Worker(
          new URL("monaco-editor/esm/vs/language/json/json.worker.js", import.meta.url),
          {type: "module"},
        );
      }
      if (label === "css" || label === "scss" || label === "less") {
        return new Worker(
          new URL("monaco-editor/esm/vs/language/css/css.worker.js", import.meta.url),
          {type: "module"},
        );
      }
      if (label === "html" || label === "handlebars" || label === "razor") {
        return new Worker(
          new URL("monaco-editor/esm/vs/language/html/html.worker.js", import.meta.url),
          {type: "module"},
        );
      }
      if (label === "typescript" || label === "javascript") {
        return new Worker(
          new URL("monaco-editor/esm/vs/language/typescript/ts.worker.js", import.meta.url),
          {type: "module"},
        );
      }
      return new Worker(new URL("monaco-editor/esm/vs/editor/editor.worker.js", import.meta.url), {
        type: "module",
      });
    },
  };
}

// Legend order must match SEMANTIC_TOKEN_LEGEND in syntaqlite-runtime.
const TOKEN_LEGEND: monaco.languages.SemanticTokensLegend = {
  tokenTypes: [
    "keyword", // 0
    "variable", // 1
    "string", // 2
    "number", // 3
    "operator", // 4
    "comment", // 5
    "punctuation", // 6
    "identifier", // 7
    "function", // 8
    "type", // 9
  ],
  tokenModifiers: [],
};

function registerSemanticTokensProvider(engine: Engine): void {
  const provider: monaco.languages.DocumentRangeSemanticTokensProvider = {
    getLegend: () => TOKEN_LEGEND,

    provideDocumentRangeSemanticTokens(
      model: monaco.editor.ITextModel,
      range: monaco.Range,
    ): monaco.languages.ProviderResult<monaco.languages.SemanticTokens> {
      if (!engine.ready) return {data: new Uint32Array(0)};
      const uri = model.uri.toString();
      if (uri !== INPUT_MODEL_URI && uri !== FORMATTED_MODEL_URI) {
        return {data: new Uint32Array(0)};
      }
      const source = model.getValue();
      const version = model.getVersionId();
      // engine.runSemanticTokens() dispatches to the correct implementation
      // (SQL or embedded) based on the language mode set via engine.setLanguageMode().
      // In embedded mode the WASM ignores the range and returns full-document tokens.
      const rangeStart = model.getOffsetAt(range.getStartPosition());
      const rangeEnd = model.getOffsetAt(range.getEndPosition());
      const data = engine.runSemanticTokens(source, rangeStart, rangeEnd, version);
      return {data: data ?? new Uint32Array(0)};
    },
  };

  monaco.languages.registerDocumentRangeSemanticTokensProvider("sql", provider);
  monaco.languages.registerDocumentRangeSemanticTokensProvider("python", provider);
  monaco.languages.registerDocumentRangeSemanticTokensProvider("typescript", provider);
}

/** LSP `CompletionItemKind` values → Monaco kinds. */
const LSP_COMPLETION_KINDS = new Map<number, monaco.languages.CompletionItemKind>([
  [3, monaco.languages.CompletionItemKind.Function],
  [5, monaco.languages.CompletionItemKind.Field], // column
  [14, monaco.languages.CompletionItemKind.Keyword],
  [22, monaco.languages.CompletionItemKind.Struct], // table
]);

function registerCompletionProvider(app: App, engine: Engine, lsp: LspSession): void {
  monaco.languages.registerCompletionItemProvider("sql", {
    triggerCharacters: [" ", "\t", ";", "(", ","],
    provideCompletionItems(
      model: monaco.editor.ITextModel,
      position: monaco.Position,
    ): monaco.languages.ProviderResult<monaco.languages.CompletionList> {
      if (!engine.ready || !engine.lspSupported) return {suggestions: []};
      if (app.languageMode !== "sql") return {suggestions: []};
      if (model.uri.toString() !== INPUT_MODEL_URI) {
        return {suggestions: []};
      }

      // Monaco positions are 1-based UTF-16; LSP positions are 0-based UTF-16.
      const items = lsp.completions(
        INPUT_MODEL_URI,
        model.getValue(),
        model.getVersionId(),
        position.lineNumber - 1,
        position.column - 1,
      );
      if (items.length === 0) return {suggestions: []};

      const word = model.getWordUntilPosition(position);
      const range = new monaco.Range(
        position.lineNumber,
        word.startColumn,
        position.lineNumber,
        word.endColumn,
      );

      const suggestions: monaco.languages.CompletionItem[] = items.map((item) => ({
        label: item.label,
        insertText: item.label,
        sortText: item.sortText,
        detail: item.detail,
        kind:
          LSP_COMPLETION_KINDS.get(item.kind ?? 0) ?? monaco.languages.CompletionItemKind.Text,
        range,
      }));

      return {suggestions};
    },
  });
}

function registerCodeActionProvider(app: App): void {
  monaco.languages.registerCodeActionProvider("sql", {
    provideCodeActions(
      model: monaco.editor.ITextModel,
      range: monaco.Range,
    ): monaco.languages.ProviderResult<monaco.languages.CodeActionList> {
      if (app.languageMode !== "sql") return {actions: [], dispose() {}};
      if (model.uri.toString() !== INPUT_MODEL_URI) return {actions: [], dispose() {}};

      const actions: monaco.languages.CodeAction[] = [];
      for (const d of app.diagnostics) {
        if (!d.helpDetail || d.helpDetail.kind !== "suggestion") continue;
        if (d.line == null || d.col == null) continue;
        const suggestion = d.helpDetail.value;

        // Check if this diagnostic overlaps the requested range.
        const dStartLine = d.line;
        const dStartCol = d.col;
        const dEnd = model.getPositionAt(d.endOffset);
        const dRange = new monaco.Range(dStartLine, dStartCol, dEnd.lineNumber, dEnd.column);
        if (!monaco.Range.areIntersectingOrTouching(range, dRange)) continue;

        actions.push({
          title: `Change to '${suggestion}'`,
          kind: "quickfix",
          isPreferred: true,
          edit: {
            edits: [{
              resource: model.uri,
              textEdit: {range: dRange, text: suggestion},
              versionId: model.getVersionId(),
            }],
          },
        });
      }
      return {actions, dispose() {}};
    },
  });
}

async function main() {
  setupMonacoWorkers();

  const app = new App();
  const root = document.getElementById("app");
  if (!root) throw new Error("missing #app element");

  m.mount(root, {view: () => m(AppComponent, {app})});

  try {
    await app.runtime.load();

    // Load the dialect specified by URL state (defaults to sqlite).
    const {dialect, sqliteVersion, cflags, schema, schemaFormat} = app.urlState.current;
    const presets = app.dialect.getPresets();
    const presetToLoad = presets.find((p) => p.id === dialect) ?? presets[0];
    await app.dialect.selectPreset(app.runtime, presetToLoad);

    // Apply URL-specified SQLite version and compile flags.
    app.dialectConfig.loadAvailableCflags(app.runtime);
    app.dialectConfig.apply(app.runtime, sqliteVersion, cflags);

    // Apply URL-specified schema context (no-op if schema is empty).
    app.schemaContext.apply(app.runtime, schema, schemaFormat);

    registerSemanticTokensProvider(app.runtime);
    registerCompletionProvider(app, app.runtime, new LspSession(app.runtime));
    registerCodeActionProvider(app);
    app.runtime.updateStatus("Ready.");
  } catch (err) {
    app.runtime.updateStatus(`Failed to initialize: ${(err as Error).message}`, true);
  }
  m.redraw();
}

main();
