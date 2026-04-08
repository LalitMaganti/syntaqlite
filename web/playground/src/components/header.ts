// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import m from "mithril";
import type {Attrs} from "../app/app";
import {VERSION_OPTIONS} from "syntaqlite";
import {HelpTooltip} from "./help_tooltip";
import {SegmentedSwitch} from "./switch";
import "./header.css";

const THEME_SWITCH_OPTIONS = [
  {id: "light", label: "Light"},
  {id: "dark", label: "Dark"},
] as const;

export class Header implements m.ClassComponent<Attrs> {
  private customPopoverOpen = false;
  private customSymbol = "syntaqlite_dialect";
  private customFile: File | undefined = undefined;
  private customError: string | undefined = undefined;
  private customLoading = false;
  private configPopoverOpen = false;
  private dialectPopoverOpen = false;

  view(vnode: m.Vnode<Attrs>) {
    const {app} = vnode.attrs;
    const activeId = app.dialect.activePresetId;
    const {sqliteVersion, cflags} = app.urlState.current;

    return m("header.sq-toolbar", [
      m("div.sq-toolbar__left", [
        m("span.sq-toolbar__brand", [
          m("span.sq-toolbar__kicker", "syntaqlite"),
          m("span.sq-toolbar__title-full", " Playground"),
          m("span.sq-toolbar__title-mobile", "syntaqlite"),
        ]),
      ]),
      m("div.sq-toolbar__right", [
        m("div.sq-dialect-controls", [
          m("span.sq-dialect-controls__label", "Dialect"),
          m(HelpTooltip, {
            ariaLabel: "Dialect requirements help",
            text: "Dialects must be SQLite-based.",
            linkHref: "https://docs.syntaqlite.com/guides/custom-dialects/",
            linkLabel: "Custom dialects docs",
          }),
          // Desktop dialect switcher
          m("div.sq-dialect-switcher", [
            ...app.dialect.getPresets().map((preset) =>
              preset.id === "sqlite"
                ? m(
                    "div.sq-config-popover",
                    {class: this.configPopoverOpen ? "sq-config-popover--open" : ""},
                    [
                      m(
                        "button.sq-dialect-switcher__btn.sq-dialect-switcher__btn--with-chevron",
                        {
                          class: activeId === preset.id ? "sq-dialect-switcher__btn--active" : "",
                          onclick: () => app.dialect.selectPreset(app.runtime, preset),
                        },
                        [
                          m("span", preset.label),
                          activeId === "sqlite"
                            ? m("span.sq-config-chevron", {
                                onclick: (e: Event) => {
                                  e.stopPropagation();
                                  this.configPopoverOpen = !this.configPopoverOpen;
                                },
                              }, "\u25BE")
                            : undefined,
                        ],
                      ),
                      m("div.sq-config-popover__backdrop", {
                        onclick: () => {
                          this.configPopoverOpen = false;
                        },
                      }),
                      m("div.sq-config-popover__panel", {onclick: (e: Event) => e.stopPropagation()}, [
                        m("div.sq-config-popover__section", [
                          m("span.sq-config-popover__label", "SQLite Version"),
                          m(
                            "select.sq-config-popover__select",
                            {
                              value: sqliteVersion,
                              onchange: (e: Event) => {
                                const newVersion = (e.target as HTMLSelectElement).value;
                                // Drop any cflags no longer visible at the new version.
                                const visible = new Set(
                                  app.dialectConfig.visibleCflags(newVersion),
                                );
                                const newCflags = cflags.filter((f) => visible.has(f));
                                app.urlState.update({sqliteVersion: newVersion, cflags: newCflags});
                                app.dialectConfig.apply(app.runtime, newVersion, newCflags);
                                m.redraw();
                              },
                            },
                            VERSION_OPTIONS.map((v) => m("option", {value: v}, v)),
                          ),
                        ]),
                        m("div.sq-config-popover__section", [
                          m("span.sq-config-popover__label", "Compile Flags"),
                          m(
                            "div.sq-config-popover__cflag-list",
                            (() => {
                              const entries = app.dialectConfig.visibleCflagEntries(sqliteVersion);
                              const groups: Record<string, string[]> = {};
                              for (const e of entries) {
                                (groups[e.category] ??= []).push(e.name);
                              }
                              const categoryOrder = ["parser", "functions", "extensions", "vtable"];
                              const categoryLabels: Record<string, string> = {
                                parser: "Parser",
                                functions: "Functions",
                                extensions: "Extensions",
                                vtable: "Virtual Tables",
                              };
                              const renderGroup = (label: string, items: string[]) =>
                                items.length === 0
                                  ? undefined
                                  : [
                                      m("div.sq-config-popover__group-label", label),
                                      ...items.map((suffix) =>
                                        m("label.sq-config-popover__cflag-item", [
                                          m("input[type=checkbox]", {
                                            checked: cflags.includes(suffix),
                                            onchange: () => {
                                              const newCflags = cflags.includes(suffix)
                                                ? cflags.filter((f) => f !== suffix)
                                                : [...cflags, suffix].sort();
                                              app.urlState.update({cflags: newCflags});
                                              app.dialectConfig.apply(
                                                app.runtime,
                                                sqliteVersion,
                                                newCflags,
                                              );
                                              m.redraw();
                                            },
                                          }),
                                          m("span", suffix),
                                        ]),
                                      ),
                                    ];
                              return categoryOrder.map((cat) =>
                                renderGroup(categoryLabels[cat] ?? cat, groups[cat] ?? []),
                              );
                            })(),
                          ),
                        ]),
                        m("div.sq-config-popover__section", [
                          m(
                            "button.sq-config-popover__reset-btn",
                            {
                              onclick: () => {
                                app.urlState.update({sqliteVersion: "latest", cflags: []});
                                app.dialectConfig.apply(app.runtime, "latest", []);
                                m.redraw();
                              },
                            },
                            "Reset to Defaults",
                          ),
                        ]),
                      ]),
                    ],
                  )
                : m(
                    "button.sq-dialect-switcher__btn",
                    {
                      class: activeId === preset.id ? "sq-dialect-switcher__btn--active" : "",
                      onclick: () => app.dialect.selectPreset(app.runtime, preset),
                    },
                    preset.label,
                  ),
            ),
            m(
              "div.sq-dialect-popover",
              {class: this.customPopoverOpen ? "sq-dialect-popover--open" : ""},
              [
                m(
                  "button.sq-dialect-switcher__btn",
                  {
                    class: activeId === "custom" ? "sq-dialect-switcher__btn--active" : "",
                    title: activeId === "custom"
                      ? "Custom dialect active — not included in shared URLs"
                      : undefined,
                    onclick: (e: Event) => {
                      e.stopPropagation();
                      this.customPopoverOpen = !this.customPopoverOpen;
                      if (this.customPopoverOpen) this.customError = undefined;
                    },
                  },
                  activeId === "custom" && app.dialect.customLabel
                    ? app.dialect.customLabel
                    : "Custom",
                ),
                m("div.sq-dialect-popover__backdrop", {onclick: () => this.closePopover()}),
                m("div.sq-dialect-popover__panel", {onclick: (e: Event) => e.stopPropagation()}, [
                  m("div.sq-dialect-popover__row", [
                    m("span.sq-dialect-popover__label", "File"),
                    m(
                      "div.sq-dialect-popover__file-btn",
                      {
                        onclick: () => {
                          const input = document.getElementById(
                            "dialect-file-input",
                          ) as HTMLInputElement;
                          input?.click();
                        },
                      },
                      this.customFile ? this.customFile.name : "Choose .wasm file...",
                    ),
                    m("input.sq-dialect-popover__file-input#dialect-file-input[type=file]", {
                      accept: ".wasm,application/wasm",
                      onchange: (e: Event) => {
                        const input = e.target as HTMLInputElement;
                        const file = input.files?.[0];
                        if (file) {
                          this.customFile = file;
                          this.customError = undefined;
                        }
                      },
                    }),
                  ]),
                  m("div.sq-dialect-popover__row", [
                    m("span.sq-dialect-popover__label", "Symbol"),
                    m("input.sq-dialect-popover__name[type=text]", {
                      placeholder: "syntaqlite_xyz_dialect_template",
                      value: this.customSymbol,
                      oninput: (e: Event) => {
                        this.customSymbol = (e.target as HTMLInputElement).value;
                        this.customError = undefined;
                      },
                    }),
                  ]),
                  m("div.sq-dialect-popover__row.sq-dialect-popover__row--help", [
                    m("span.sq-dialect-popover__label", "Help"),
                    m(HelpTooltip, {
                      className: "sq-dialect-popover__help-tooltip",
                      ariaLabel: "Custom dialect generation help",
                      text: "Custom dialect modules must be generated from a SQLite-based dialect build.",
                      linkHref: "https://docs.syntaqlite.com/guides/custom-dialects/",
                      linkLabel: "Custom dialects docs",
                    }),
                  ]),
                  this.customError
                    ? m("div.sq-dialect-popover__error", this.customError)
                    : undefined,
                  m("div.sq-dialect-popover__row", [
                    m(
                      "button.sq-dialect-popover__load-btn",
                      {
                        disabled: !this.customFile || this.customLoading,
                        onclick: () => this.loadCustom(app),
                      },
                      this.customLoading ? "Loading..." : "Load",
                    ),
                  ]),
                ]),
              ],
            ),
          ]),
          // Mobile dialect trigger
          m(
            "div.sq-dialect-mobile-trigger",
            {class: this.dialectPopoverOpen ? "sq-dialect-mobile-trigger--open" : ""},
            [
              m(
                "button.sq-dialect-mobile-trigger__btn",
                {
                  onclick: () => {
                    this.dialectPopoverOpen = !this.dialectPopoverOpen;
                  },
                },
                [
                  m("span", this.activeDialectLabel(app)),
                  m("span.sq-dialect-mobile-trigger__chevron", "\u25BE"),
                ],
              ),
              m("div.sq-dialect-mobile-trigger__backdrop", {
                onclick: () => { this.dialectPopoverOpen = false; },
              }),
              m("div.sq-dialect-mobile-trigger__sheet", [
                m("div.sq-dialect-mobile-trigger__title", "Select Dialect"),
                ...app.dialect.getPresets().map((preset) =>
                  m(
                    "button.sq-dialect-mobile-trigger__option",
                    {
                      class: activeId === preset.id ? "sq-dialect-mobile-trigger__option--active" : "",
                      onclick: () => {
                        app.dialect.selectPreset(app.runtime, preset);
                        this.dialectPopoverOpen = false;
                      },
                    },
                    preset.label,
                  ),
                ),
                m(
                  "button.sq-dialect-mobile-trigger__option",
                  {
                    class: activeId === "custom" ? "sq-dialect-mobile-trigger__option--active" : "",
                    onclick: () => {
                      this.dialectPopoverOpen = false;
                      this.customPopoverOpen = true;
                    },
                  },
                  activeId === "custom" && app.dialect.customLabel
                    ? app.dialect.customLabel
                    : "Custom...",
                ),
              ]),
            ],
          ),
        ]),
        // Desktop theme controls
        m("div.sq-theme-controls", [
          m("span.sq-theme-controls__label", "Theme"),
          m(SegmentedSwitch, {
            options: THEME_SWITCH_OPTIONS,
            value: app.theme.current,
            ariaLabel: "Theme",
            onChange: (value) => {
              app.theme.set(value as "light" | "dark");
              m.redraw();
            },
          }),
        ]),
        // Mobile theme toggle
        m(
          "button.sq-theme-toggle-mobile",
          {
            onclick: () => {
              app.theme.toggle();
              m.redraw();
            },
            title: app.theme.current === "dark" ? "Switch to light mode" : "Switch to dark mode",
          },
          app.theme.current === "dark" ? "\u2600" : "\u263E",
        ),
      ]),
    ]);
  }

  private activeDialectLabel(app: InstanceType<typeof import("../app/app").App>): string {
    const activeId = app.dialect.activePresetId;
    if (activeId === "custom") return app.dialect.customLabel || "Custom";
    const preset = app.dialect.getPresets().find((p) => p.id === activeId);
    return preset?.label ?? activeId;
  }

  private async loadCustom(app: InstanceType<typeof import("../app/app").App>) {
    if (!this.customFile) return;
    this.customLoading = true;
    this.customError = undefined;
    m.redraw();
    const error = await app.dialect.loadFromFile(app.runtime, this.customFile, this.customSymbol);
    this.customLoading = false;
    if (error) {
      this.customError = error;
    } else {
      this.closePopover();
    }
    m.redraw();
  }

  private closePopover() {
    this.customPopoverOpen = false;
  }
}
