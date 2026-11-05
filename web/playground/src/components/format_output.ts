// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import m from "mithril";
import type {Theme} from "../types";
import {FORMATTED_MODEL_URI} from "../app/editor_models";
import {MonacoEditor} from "../widgets/monaco_editor";
import "./format_output.css";

export interface FormatOutputAttrs {
  text: string;
  theme: Theme;
}

export class FormatOutput implements m.ClassComponent<FormatOutputAttrs> {
  view(vnode: m.Vnode<FormatOutputAttrs>) {
    const {text, theme} = vnode.attrs;
    return m("div.sq-format-output.sq-format-output--readonly", [
      m(MonacoEditor, {
        theme,
        initialValue: text,
        modelUri: FORMATTED_MODEL_URI,
        readOnly: true,
        lineNumbers: "off",
        renderLineHighlight: "none",
      }),
    ]);
  }
}
