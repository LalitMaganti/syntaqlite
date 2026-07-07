// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import m from "mithril";
import type {App} from "../app/app";
import type {FormatOptions, FormatResult, KeywordCase} from "../types";
import {FormatOutput} from "./format_output";

export interface FormatTabAttrs {
  app: App;
  sql: string;
  active: boolean;
}

export class FormatTab implements m.ClassComponent<FormatTabAttrs> {
  private formatOptions: FormatOptions = {
    lineWidth: 80,
    indentWidth: 2,
    keywordCase: "upper",
    semicolons: true,
  };
  private formatResult: FormatResult | undefined = undefined;

  /** Format `sql`, folding engine errors into a renderable result. */
  private format(app: App, sql: string): FormatResult {
    try {
      return {ok: true, text: app.runtime.format(sql, this.formatOptions)};
    } catch (e) {
      return {ok: false, text: (e as Error).message};
    }
  }
  private lastSql: string | undefined = undefined;
  private lastOptionsKey: string | undefined = undefined;
  private lastDialectPtr: number | undefined = undefined;
  private lastConfigKey: string | undefined = undefined;

  view(vnode: m.Vnode<FormatTabAttrs>) {
    const {app, sql, active} = vnode.attrs;

    if (active && app.runtime.ready && app.dialect.active) {
      const isEmbedded = app.languageMode !== "sql";
      const fragIdx = app.selectedFragmentIndex;
      const optKey = `${this.formatOptions.lineWidth}:${this.formatOptions.indentWidth}:${this.formatOptions.keywordCase}:${this.formatOptions.semicolons}:${app.languageMode}:${fragIdx}`;
      const dPtr = app.dialect.active.ptr;
      const {sqliteVersion, cflags} = app.urlState.current;
      const cfgKey = `${sqliteVersion}|${cflags.join(",")}`;
      if (
        sql !== this.lastSql ||
        optKey !== this.lastOptionsKey ||
        dPtr !== this.lastDialectPtr ||
        cfgKey !== this.lastConfigKey
      ) {
        this.lastSql = sql;
        this.lastOptionsKey = optKey;
        this.lastDialectPtr = dPtr;
        this.lastConfigKey = cfgKey;

        if (isEmbedded && app.embeddedFragments.length > 0) {
          if (fragIdx >= 0 && fragIdx < app.embeddedFragments.length) {
            this.formatResult = this.format(app, app.embeddedFragments[fragIdx].sql);
          } else {
            // Format all fragments with separators.
            const parts: string[] = [];
            let allOk = true;
            for (let i = 0; i < app.embeddedFragments.length; i++) {
              const r = this.format(app, app.embeddedFragments[i].sql);
              if (!r.ok) {
                allOk = false;
                parts.push(`-- Fragment ${i + 1} (error)\n${r.text}`);
              } else {
                parts.push(`-- Fragment ${i + 1}\n${r.text}`);
              }
            }
            this.formatResult = {ok: allOk, text: parts.join("\n\n")};
          }
        } else {
          this.formatResult = this.format(app, sql);
        }
      }
    }

    const result = this.formatResult;
    const text = result ? (result.ok ? result.text : `Error: ${result.text}`) : "";

    return m("div.sq-tab-panel", {class: active ? "sq-tab-panel--active" : ""}, [
      m("div.sq-panel-options", [
        m("label.sq-panel-options__width-label", [
          "Width:",
          m("span.sq-panel-options__width-value", String(this.formatOptions.lineWidth)),
        ]),
        m("input[type=range]", {
          min: 20,
          max: 240,
          value: this.formatOptions.lineWidth,
          oninput: (e: Event) => {
            this.formatOptions.lineWidth = Number((e.target as HTMLInputElement).value);
          },
        }),
        m("label.sq-panel-options__width-label", [
          "Indent:",
          m("span.sq-panel-options__width-value", String(this.formatOptions.indentWidth)),
        ]),
        m("input[type=range]", {
          min: 1,
          max: 8,
          value: this.formatOptions.indentWidth,
          oninput: (e: Event) => {
            this.formatOptions.indentWidth = Number((e.target as HTMLInputElement).value);
          },
        }),
        m("label", "Keywords"),
        m(
          "select",
          {
            value: this.formatOptions.keywordCase,
            onchange: (e: Event) => {
              this.formatOptions.keywordCase = (e.target as HTMLSelectElement)
                .value as KeywordCase;
            },
          },
          [
            m("option", {value: "upper"}, "Upper"),
            m("option", {value: "lower"}, "Lower"),
          ],
        ),
        m("input[type=checkbox]", {
          checked: this.formatOptions.semicolons,
          onchange: (e: Event) => {
            this.formatOptions.semicolons = (e.target as HTMLInputElement).checked;
          },
        }),
        m("label", "Semicolons"),
      ]),
      m(FormatOutput, {text, theme: app.theme.current}),
    ]);
  }
}
