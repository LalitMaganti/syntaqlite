// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import m from "mithril";
import type {Attrs} from "../app/app";
import {ResizeHandle} from "../widgets/resize_handle";
import {renderValidationBadge, renderValidationTab, renderSchemaTab} from "./details_shared";
import "./details_panel.css";

type DetailsTab = "validation" | "schema";

const TABS: {key: DetailsTab; label: string}[] = [
  {key: "validation", label: "Validation"},
  {key: "schema", label: "Schema"},
];

const DEFAULT_HEIGHT = 300;
const MOBILE_DEFAULT_HEIGHT = 160;
const MIN_HEIGHT = 60;
const MAX_HEIGHT_RATIO = 0.6;

export class DetailsPanel implements m.ClassComponent<Attrs> {
  private collapsed = false;
  private activeTab: DetailsTab = "validation";
  private panelHeight: number | undefined = undefined;
  private panelEl: HTMLElement | undefined = undefined;

  view(vnode: m.Vnode<Attrs>) {
    const {app} = vnode.attrs;
    const style: Record<string, string> = {};
    if (!this.collapsed) {
      const defaultH = app.window.isMobile ? MOBILE_DEFAULT_HEIGHT : DEFAULT_HEIGHT;
      style.height = `${this.panelHeight ?? defaultH}px`;
    }
    return m("div.sq-details-panel", {
      class: this.collapsed ? "sq-details-panel--collapsed" : "",
      style,
      oncreate: (v: m.VnodeDOM) => { this.panelEl = v.dom as HTMLElement; },
    }, [
      this.collapsed
        ? undefined
        : m(ResizeHandle, {
            axis: "horizontal",
            onResize: (delta: number) => {
              const current = this.panelHeight ?? this.panelEl?.getBoundingClientRect().height ?? 120;
              const maxHeight = window.innerHeight * MAX_HEIGHT_RATIO;
              this.panelHeight = Math.max(MIN_HEIGHT, Math.min(maxHeight, current - delta));
            },
          }),
      m("div.sq-details-panel__header", [
        m(
          "nav.sq-tab-bar",
          TABS.map((t) =>
            m(
              "button.sq-tab-bar__tab",
              {
                class: this.activeTab === t.key ? "sq-tab-bar__tab--active" : "",
                onclick: () => { this.activeTab = t.key; },
              },
              t.key === "validation"
                ? [t.label, renderValidationBadge(app.diagnostics)]
                : t.label,
            ),
          ),
        ),
        m(
          "button.sq-details-panel__toggle",
          {onclick: () => { this.collapsed = !this.collapsed; }},
          this.collapsed ? "\u25B4 Show" : "\u25BE Hide",
        ),
      ]),
      this.collapsed
        ? undefined
        : this.activeTab === "validation"
          ? renderValidationTab(app.diagnostics, app.revealDiagnostic)
          : this.activeTab === "schema"
            ? renderSchemaTab(app)
            : undefined,
    ]);
  }
}
