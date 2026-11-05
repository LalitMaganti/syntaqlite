// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import m from "mithril";
import type * as monaco from "monaco-editor";
import type {SqlPreset} from "./workspace/sql_presets";
import type {Theme} from "../types";
import {INPUT_MODEL_URI} from "../app/editor_models";
import {MonacoEditor} from "../widgets/monaco_editor";
import "./editor_pane.css";

export type LanguageMode = "sql" | "python" | "typescript";

export interface EditorPaneAttrs {
  theme: Theme;
  initialSql: string;
  presets: SqlPreset[];
  selectedPresetId: string;
  languageMode: LanguageMode;
  onLanguageChange: (lang: LanguageMode) => void;
  onPresetChange: (presetId: string) => void;
  onContentChange: (s: string) => void;
  onEditorCreated?: (editor: monaco.editor.IStandaloneCodeEditor) => void;
}

export class EditorPane implements m.ClassComponent<EditorPaneAttrs> {
  view(vnode: m.Vnode<EditorPaneAttrs>) {
    const {
      theme,
      initialSql,
      presets,
      selectedPresetId,
      languageMode,
      onLanguageChange,
      onPresetChange,
      onContentChange,
      onEditorCreated,
    } = vnode.attrs;
    const selectedPreset = presets.find((p) => p.id === selectedPresetId) ?? presets[0];
    const selectedDescription =
      selectedPresetId === "custom"
        ? "Custom mode. Editing the SQL keeps this mode selected."
        : (selectedPreset?.description ?? "");

    return m("section.sq-workspace__pane.sq-editor-pane", [
      m("div.sq-editor-pane__toolbar", [
        m("label.sq-editor-pane__label", {for: "sq-editor-lang"}, "Language"),
        m(
          "select#sq-editor-lang.sq-editor-pane__select",
          {
            value: languageMode,
            onchange: (e: Event) =>
              onLanguageChange((e.target as HTMLSelectElement).value as LanguageMode),
          },
          [
            m("option", {value: "sql"}, "SQL"),
            m("option", {value: "python"}, "Python (experimental)"),
            m("option", {value: "typescript"}, "TypeScript (experimental)"),
          ],
        ),
        m("label.sq-editor-pane__label", {for: "sq-editor-preset"}, "Presets"),
        m(
          "select#sq-editor-preset.sq-editor-pane__select.sq-editor-pane__select--preset",
          {
            value: selectedPresetId,
            onchange: (e: Event) =>
              onPresetChange((e.target as HTMLSelectElement).value),
          },
          [
            ...presets.map((preset) => m("option", {value: preset.id}, preset.label)),
            m("option", {value: "custom"}, "Custom"),
          ],
        ),
        m("span.sq-editor-pane__description", selectedDescription),
      ]),
      m("div.sq-editor-pane__editor", [
        m(MonacoEditor, {
          theme,
          initialValue: initialSql,
          modelUri: INPUT_MODEL_URI,
          language: languageMode,
          onContentChange,
          onEditorCreated,
          lineNumbers: "on",
          renderLineHighlight: "gutter",
        }),
      ]),
    ]);
  }
}
