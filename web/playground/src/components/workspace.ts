// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import m from "mithril";
import * as monaco from "monaco-editor";
import type {App, Attrs} from "../app/app";
import type {Engine} from "syntaqlite";
import type {Dialect, LanguageMode} from "../app/url_state";
import {getSqlPresetLibrary} from "./workspace/sql_presets";
import {debounce} from "../base/debounce";
import type {DiagnosticEntry} from "../types";
import {EditorPane, type LanguageMode as EditorLanguageMode} from "./editor_pane";
import {OutputPanel} from "./output_panel";
import {ResizeHandle} from "../widgets/resize_handle";
import "./workspace.css";

const SEVERITY_MAP: Record<DiagnosticEntry["severity"], monaco.MarkerSeverity> = {
  error: monaco.MarkerSeverity.Error,
  warning: monaco.MarkerSeverity.Warning,
  info: monaco.MarkerSeverity.Info,
  hint: monaco.MarkerSeverity.Hint,
};
const CUSTOM_PRESET_ID = "custom";

/** The built-in preset dialect IDs that can be encoded in the URL. */
const SHAREABLE_DIALECTS = new Set<string>(["sqlite", "perfetto"]);

/** Convert a byte offset to a 1-based line and column in `source`. */
function offsetToLineCol(source: string, offset: number): {line: number; col: number} {
  const clamped = Math.min(offset, source.length);
  let line = 1;
  let col = 1;
  for (let i = 0; i < clamped; i++) {
    if (source[i] === "\n") {
      line++;
      col = 1;
    } else {
      col++;
    }
  }
  return {line, col};
}

/** Return the 1-based statement index for a byte offset (semicolons delimit statements). */
function countStatements(source: string, offset: number): number {
  let count = 1;
  for (let i = 0; i < offset && i < source.length; i++) {
    if (source[i] === ";") count++;
  }
  return count;
}

export class Workspace implements m.ClassComponent<Attrs> {
  private sql = "";
  private editor: monaco.editor.IStandaloneCodeEditor | undefined = undefined;
  private debouncedUpdate: (sql: string) => void;
  private debouncedDiagnostics: (engine: Engine, sql: string) => void;
  private presetSelectionByDialect = new Map<string, string>();
  private customSqlByDialect = new Map<string, string>();
  private appRef: App | undefined = undefined;
  private applyingPreset = false;
  private lastAppliedDialectPtr: number | undefined = undefined;
  /** Track the last dialect pointer we ran diagnostics against. */
  private lastDiagnosticDialectPtr: number | undefined = undefined;
  private lastDiagnosticCfgKey: string | undefined = undefined;
  private lastDiagnosticSchemaKey: string | undefined = undefined;
  /** Left pane fraction (0..1), undefined = equal split. */
  private splitFraction: number | undefined = undefined;
  private workspaceEl: HTMLElement | undefined = undefined;
  /** True after the first view() call has run and URL state has been applied. */
  private initialized = false;

  constructor() {
    this.debouncedUpdate = debounce((sql: string) => {
      this.sql = sql;
      m.redraw();
    }, 150);

    this.debouncedDiagnostics = debounce(
      (engine: Engine, sql: string) => this.updateDiagnostics(engine, sql),
      100,
    );
  }

  view(vnode: m.Vnode<Attrs>) {
    const {app} = vnode.attrs;
    this.appRef = app;

    // On the very first render, seed editor/preset state from URL.
    if (!this.initialized) {
      this.initialized = true;
      this.initFromUrlState(app);
    }

    const presetKey = app.languageMode === "sql"
      ? app.dialect.activePresetId
      : `${app.dialect.activePresetId}:${app.languageMode}`;
    const presetLibrary = getSqlPresetLibrary(presetKey);
    const selectedPresetId = this.ensurePresetSelection(presetLibrary);

    // Stable keys derived from urlState for dialect-config and schema
    // change detection (replaces the removed manager configKey fields).
    const {sqliteVersion, cflags, schema, schemaFormat} = app.urlState.current;
    const cfgKey = `${sqliteVersion}|${cflags.join(",")}`;
    const schemaKey = `${schemaFormat}:${schema}`;

    // Apply dialect-specific preset SQL and refresh analysis whenever the
    // active dialect pointer changes.
    if (app.runtime.ready && app.dialect.active) {
      const dPtr = app.dialect.active.ptr;
      if (dPtr !== this.lastAppliedDialectPtr) {
        this.lastAppliedDialectPtr = dPtr;
        this.applySelectionForDialect(app.runtime, presetLibrary.dialectId, selectedPresetId, true);
        // Force-reapply schema after dialect switch (resets LSP host).
        app.schemaContext.apply(app.runtime, schema, schemaFormat, true);
        this.lastDiagnosticDialectPtr = dPtr;
        this.lastDiagnosticCfgKey = cfgKey;
        this.lastDiagnosticSchemaKey = schemaKey;
        // Write full URL state now that dialect + preset/sql are known.
        this.writePresetUrlState(app, presetLibrary.dialectId, selectedPresetId);
      } else if (
        this.editor &&
        (dPtr !== this.lastDiagnosticDialectPtr ||
          cfgKey !== this.lastDiagnosticCfgKey ||
          schemaKey !== this.lastDiagnosticSchemaKey)
      ) {
        this.updateDiagnostics(app.runtime, this.sql);
        this.lastDiagnosticDialectPtr = dPtr;
        this.lastDiagnosticCfgKey = cfgKey;
        this.lastDiagnosticSchemaKey = schemaKey;
      }
    }

    const splitStyle: Record<string, string> = {};
    if (this.splitFraction != undefined) {
      const pct = (this.splitFraction * 100).toFixed(2);
      splitStyle.gridTemplateColumns = `${pct}% 5px 1fr`;
    }

    return m("section.sq-workspace", {
      style: splitStyle,
      oncreate: (v: m.VnodeDOM) => { this.workspaceEl = v.dom as HTMLElement; },
    }, [
      m(EditorPane, {
        theme: app.theme.current,
        initialSql: this.sql,
        presets: presetLibrary.presets,
        selectedPresetId,
        languageMode: app.languageMode as EditorLanguageMode,
        onLanguageChange: (lang: EditorLanguageMode) => {
          app.languageMode = lang;
          app.embeddedFragments = [];
          app.selectedFragmentIndex = -1;
          app.diagnostics = [];
          app.runtime.setLanguageMode(lang);
          if (this.editor) {
            const model = this.editor.getModel();
            if (model) monaco.editor.setModelMarkers(model, "syntaqlite", []);
          }
          // Apply the first preset of the new language.
          const newPresetKey = lang === "sql"
            ? app.dialect.activePresetId
            : `${app.dialect.activePresetId}:${lang}`;
          const newLib = getSqlPresetLibrary(newPresetKey);
          const sel = this.ensurePresetSelection(newLib);
          this.applySelectionForDialect(app.runtime, newLib.dialectId, sel, true);
          this.writePresetUrlState(app, newLib.dialectId, sel, {languageMode: lang});
          m.redraw();
        },
        onPresetChange: (presetId: string) => {
          this.presetSelectionByDialect.set(presetLibrary.dialectId, presetId);
          this.applySelectionForDialect(app.runtime, presetLibrary.dialectId, presetId);
          app.urlState.update({preset: presetId, sql: null});
        },
        onContentChange: (s: string) => {
          if (!this.applyingPreset) {
            const current = this.presetSelectionByDialect.get(presetLibrary.dialectId);
            if (current !== CUSTOM_PRESET_ID) {
              this.presetSelectionByDialect.set(presetLibrary.dialectId, CUSTOM_PRESET_ID);
              m.redraw();
            }
            this.customSqlByDialect.set(presetLibrary.dialectId, s);
            // Debounced — urlState handles the 800ms delay internally.
            app.urlState.update({preset: null, sql: s});
          }
          this.debouncedUpdate(s);
          this.debouncedDiagnostics(app.runtime, s);
        },
        onEditorCreated: (editor) => {
          this.editor = editor;
          app.revealDiagnostic = (d) => this.revealDiagnostic(d);
        },
      }),
      m(ResizeHandle, {
        axis: "vertical",
        onResize: (delta: number) => {
          const el = this.workspaceEl;
          if (!el) return;
          const totalWidth = el.getBoundingClientRect().width;
          const handleWidth = 5;
          const available = totalWidth - handleWidth;
          const current = this.splitFraction ?? 0.5;
          const currentPx = current * available;
          const minPx = 100;
          const newPx = Math.max(minPx, Math.min(available - minPx, currentPx + delta));
          this.splitFraction = newPx / available;
        },
        onResizeEnd: () => {
          if (this.editor) this.editor.layout();
        },
      }),
      m(OutputPanel, {app, sql: this.sql}),
    ]);
  }

  /**
   * Seed preset/SQL state from URL on first render.
   * Must be called before the EditorPane renders so that `this.sql` and
   * `presetSelectionByDialect` are correct for the initial `initialSql` prop.
   */
  private initFromUrlState(app: App): void {
    const {dialect, languageMode, preset, sql} = app.urlState.current;
    const dialectId = languageMode === "sql" ? dialect : `${dialect}:${languageMode}`;

    if (preset !== null) {
      this.presetSelectionByDialect.set(dialectId, preset);
      // Pre-populate sql from the preset so the editor shows the right content
      // before the dialect WASM finishes loading (avoids a content flash).
      const lib = getSqlPresetLibrary(dialectId);
      const match = lib.presets.find((p) => p.id === preset);
      if (match) this.sql = match.sql;
    } else if (sql !== null) {
      this.presetSelectionByDialect.set(dialectId, CUSTOM_PRESET_ID);
      this.customSqlByDialect.set(dialectId, sql);
      this.sql = sql;
    }
    // If both are null the Workspace falls back to ensurePresetSelection →
    // first preset, which is the correct "no URL state" default.
  }

  /**
   * Write dialect + language + preset/sql to URL state.
   *
   * Custom (user-uploaded) dialects cannot be encoded in the URL, so if one
   * is active the `dialect` field is left unchanged — the URL retains whatever
   * built-in dialect was previously selected.  Pass `extra` to merge additional
   * fields (e.g. languageMode when changing language before the dialect pointer
   * update fires).
   */
  private writePresetUrlState(
    app: App,
    dialectId: string,
    selectedPresetId: string,
    extra: Partial<{languageMode: LanguageMode}> = {},
  ): void {
    const isCustomSql = selectedPresetId === CUSTOM_PRESET_ID;
    const customSql = isCustomSql ? this.customSqlByDialect.get(dialectId) : undefined;

    const patch: Parameters<typeof app.urlState.update>[0] = {
      languageMode: app.languageMode as LanguageMode,
      preset: isCustomSql ? null : selectedPresetId,
      sql: isCustomSql && customSql !== undefined ? customSql : null,
      ...extra,
    };

    const isCustomDialect = !SHAREABLE_DIALECTS.has(app.dialect.activePresetId);

    // Only update the dialect field when it's a built-in shareable preset.
    // Custom (uploaded) dialects can't be represented in the URL, so we leave
    // the dialect field as-is — the URL will still load the last known preset.
    if (!isCustomDialect) {
      patch.dialect = app.dialect.activePresetId as Dialect;
    }

    // Tell the URL manager whether a custom dialect is active so it can
    // include cd=1 in the hash, prompting recipients to show a notice.
    app.urlState.setCustomDialect(isCustomDialect);
    app.urlState.update(patch);
  }

  private ensurePresetSelection(presetLibrary: ReturnType<typeof getSqlPresetLibrary>): string {
    const {dialectId, presets} = presetLibrary;
    if (presets.length === 0) return "";
    const selected = this.presetSelectionByDialect.get(dialectId);
    if (selected === CUSTOM_PRESET_ID) return selected;
    if (selected && presets.some((preset) => preset.id === selected)) return selected;
    const first = presets[0].id;
    this.presetSelectionByDialect.set(dialectId, first);
    return first;
  }

  private applyPresetById(
    engine: Engine,
    dialectId: string,
    presetId: string,
    forceEditorRefresh = false,
  ): void {
    const presetLibrary = getSqlPresetLibrary(dialectId);
    const preset = presetLibrary.presets.find((item) => item.id === presetId);
    if (!preset) return;

    this.applySql(engine, preset.sql, forceEditorRefresh);
  }

  private applySelectionForDialect(
    engine: Engine,
    dialectId: string,
    presetId: string,
    forceEditorRefresh = false,
  ): void {
    if (presetId === CUSTOM_PRESET_ID) {
      const customSql = this.customSqlByDialect.get(dialectId);
      if (customSql !== undefined) {
        this.applySql(engine, customSql, forceEditorRefresh);
        return;
      }
      const fallback = getSqlPresetLibrary(dialectId).presets[0];
      if (fallback) {
        this.presetSelectionByDialect.set(dialectId, fallback.id);
        this.applySql(engine, fallback.sql, forceEditorRefresh);
      }
      return;
    }
    this.applyPresetById(engine, dialectId, presetId, forceEditorRefresh);
  }

  private applySql(engine: Engine, sql: string, forceEditorRefresh = false): void {
    this.applyingPreset = true;
    try {
      this.sql = sql;
      if (this.editor && (forceEditorRefresh || this.editor.getValue() !== sql)) {
        // Force setValue on dialect switch so Monaco requests fresh semantic tokens.
        this.editor.setValue(sql);
      }
    } finally {
      this.applyingPreset = false;
    }
    this.updateDiagnostics(engine, sql);
    m.redraw();
  }

  private updateDiagnostics(engine: Engine, sql: string): void {
    if (!engine.ready || !this.editor) return;

    const model = this.editor.getModel();
    if (!model) return;
    const app = this.appRef;
    if (!app) return;

    // Update SQL fragments for the embedded-mode UI. engine.extract() is a
    // fast no-op (O(1)) in SQL mode, so calling it unconditionally is safe.
    const extractResult = engine.extract(sql);
    app.embeddedFragments = extractResult.ok ? extractResult.fragments : [];

    let diagnostics: DiagnosticEntry[];
    if (app.languageMode !== "sql") {
      const result = engine.embeddedDiagnostics(sql);
      if (!result.ok) {
        monaco.editor.setModelMarkers(model, "syntaqlite", []);
        app.diagnostics = [];
        return;
      }
      diagnostics = result.diagnostics;
    } else {
      diagnostics = app.lsp.diagnostics(model.uri.toString(), sql, model.getVersionId());
    }

    // Enrich diagnostics with line/col/statement info for the details panel.
    for (const d of diagnostics) {
      const pos = offsetToLineCol(sql, d.startOffset);
      d.line = pos.line;
      d.col = pos.col;
      d.stmtIndex = countStatements(sql, d.startOffset);
    }

    app.diagnostics = diagnostics;

    const markers: monaco.editor.IMarkerData[] = diagnostics.map((d) => {
      const start = offsetToLineCol(sql, d.startOffset);
      const end = offsetToLineCol(sql, d.endOffset);
      return {
        severity: SEVERITY_MAP[d.severity] ?? monaco.MarkerSeverity.Error,
        message: d.help ? `${d.message}\nhelp: ${d.help}` : d.message,
        startLineNumber: start.line,
        startColumn: start.col,
        endLineNumber: end.line,
        endColumn: end.col,
      };
    });

    monaco.editor.setModelMarkers(model, "syntaqlite", markers);
  }

  private revealDiagnostic(d: DiagnosticEntry): void {
    if (!this.editor) return;
    const pos = offsetToLineCol(this.sql, d.endOffset);
    this.editor.setPosition({lineNumber: pos.line, column: pos.col});
    this.editor.revealLineInCenter(pos.line);
    this.editor.focus();
  }
}
