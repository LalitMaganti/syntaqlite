// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import m from "mithril";
import type {App} from "../app/app";
import type {AstResult} from "../types";
import type {AstViewMode} from "../app/url_state";
import {AstOutline} from "./ast_outline";
import {AstGraph} from "./ast_tree/ast_tree";

export interface AstTabAttrs {
  app: App;
  sql: string;
  active: boolean;
}

export class AstTab implements m.ClassComponent<AstTabAttrs> {
  private astViewMode: AstViewMode = "outline";
  private astShowEmpty = false;
  private astResult: AstResult | undefined = undefined;
  private lastSql: string | undefined = undefined;
  private lastDialectPtr: number | undefined = undefined;
  private lastConfigKey: string | undefined = undefined;
  private initialized = false;

  /** Parse `sql`, folding engine errors into a renderable result. */
  private parse(app: App, sql: string): AstResult {
    try {
      return {ok: true, statements: app.runtime.parse(sql).statements};
    } catch (e) {
      return {ok: false, error: (e as Error).message};
    }
  }

  view(vnode: m.Vnode<AstTabAttrs>) {
    const {app, sql, active} = vnode.attrs;

    // On first render, restore the saved view mode from URL state.
    if (!this.initialized) {
      this.initialized = true;
      this.astViewMode = app.urlState.current.astViewMode;
    }

    if (active && app.runtime.ready && app.dialect.active) {
      const isEmbedded = app.languageMode !== "sql";
      const fragIdx = app.selectedFragmentIndex;
      const cacheKey = `${sql}:${app.languageMode}:${fragIdx}`;
      const dPtr = app.dialect.active.ptr;
      const {sqliteVersion, cflags} = app.urlState.current;
      const cfgKey = `${sqliteVersion}|${cflags.join(",")}`;
      if (cacheKey !== this.lastSql || dPtr !== this.lastDialectPtr || cfgKey !== this.lastConfigKey) {
        this.lastSql = cacheKey;
        this.lastDialectPtr = dPtr;
        this.lastConfigKey = cfgKey;

        if (isEmbedded && app.embeddedFragments.length > 0) {
          if (fragIdx >= 0 && fragIdx < app.embeddedFragments.length) {
            this.astResult = this.parse(app, app.embeddedFragments[fragIdx].sql);
          } else {
            // Merge all fragments' ASTs.
            const allStatements: import("../types").AstJsonNode[] = [];
            for (const f of app.embeddedFragments) {
              const r = this.parse(app, f.sql);
              if (r.ok) {
                allStatements.push(...r.statements);
              }
            }
            this.astResult = {ok: true, statements: allStatements};
          }
        } else {
          this.astResult = this.parse(app, sql);
        }
      }
    }

    const isGraph = this.astViewMode === "graph";
    return m(
      "div.sq-tab-panel",
      {
        class: [active ? "sq-tab-panel--active" : "", isGraph ? "sq-tab-panel--graph" : ""]
          .filter(Boolean)
          .join(" "),
      },
      [
        m("div.sq-panel-options", [
          m("label", "View"),
          m(
            "select",
            {
              value: this.astViewMode,
              onchange: (e: Event) => {
                this.astViewMode = (e.target as HTMLSelectElement).value as AstViewMode;
                app.urlState.update({astViewMode: this.astViewMode});
              },
            },
            [m("option", {value: "outline"}, "Outline"), m("option", {value: "graph"}, "Graph")],
          ),
          m("input[type=checkbox]", {
            checked: this.astShowEmpty,
            onchange: (e: Event) => {
              this.astShowEmpty = (e.target as HTMLInputElement).checked;
            },
          }),
          m("label", "Show empty fields"),
        ]),
        isGraph
          ? m(AstGraph, {
              result: this.astResult,
              showEmpty: this.astShowEmpty,
              theme: app.theme.current,
            })
          : m(AstOutline, {result: this.astResult, showEmpty: this.astShowEmpty}),
      ],
    );
  }
}
