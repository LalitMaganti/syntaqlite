// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import m from "mithril";
import type {Attrs} from "../app/app";
import {Header} from "./header";
import {DetailsPanel} from "./details_panel";
import {Workspace} from "./workspace";

export class AppComponent implements m.ClassComponent<Attrs> {
  view(vnode: m.Vnode<Attrs>) {
    const {app} = vnode.attrs;
    return m("main.sq-app", [
      m(Header, {app}),
      app.runtime.statusError
        ? m("div.sq-error-banner", app.runtime.status)
        : undefined,
      app.urlState.hadCustomDialect && !app.customDialectNoticeDismissed
        ? m("div.sq-info-banner", [
            m("span", "This link was shared from a session using a custom dialect. " +
              "It is shown here with a built-in dialect — some syntax or functions may not be recognised."),
            m("button.sq-info-banner__dismiss", {
              onclick: () => {
                app.customDialectNoticeDismissed = true;
                m.redraw();
              },
            }, "\u2715"),
          ])
        : undefined,
      m(Workspace, {app}),
      m(DetailsPanel, {app}),
    ]);
  }
}
