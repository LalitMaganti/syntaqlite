// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import m from "mithril";
import "./table.css";

export interface TableColumn<T = unknown> {
  /** Unique key used for Mithril keying and the `<th>` key. */
  key: string;
  /** Header label. Empty string hides the header text but preserves the column. */
  label: string;
  /** Render the cell contents for a row. */
  render: (row: T) => m.Children;
  /** CSS width value, e.g. `"40px"` or `"20%"`. */
  width?: string;
  /** Text alignment for both header and cells. Defaults to `"left"`. */
  align?: "left" | "center" | "right";
}

export interface TableAttrs<T = unknown> {
  columns: TableColumn<T>[];
  rows: T[];
  /** Return a stable key for each row. Defaults to the row index. */
  rowKey?: (row: T, index: number) => string | number;
  /**
   * Content rendered inside a full-width cell when `rows` is empty.
   * Defaults to the plain text "No data".
   */
  emptyContent?: m.Children;
  /** Called when a row is clicked. */
  onRowClick?: (row: T, index: number) => void;
}

// The class is typed with `any` so callers can pass a typed `TableAttrs<T>`
// via `m<TableAttrs<MyType>>(Table, { ... })` without unsafe casts inside.
// eslint-disable-next-line @typescript-eslint/no-explicit-any
export class Table implements m.ClassComponent<TableAttrs<any>> {
  view(vnode: m.Vnode<TableAttrs<any>>): m.Children {
    const {columns, rows, rowKey, emptyContent, onRowClick} = vnode.attrs;

    const thead = m(
      "thead",
      m(
        "tr",
        columns.map((col) =>
          m(
            "th",
            {
              key: col.key,
              style: {
                width: col.width,
                textAlign: col.align ?? "left",
              },
            },
            col.label,
          ),
        ),
      ),
    );

    const tbody =
      rows.length === 0
        ? m(
            "tbody",
            m(
              "tr",
              m(
                "td.sq-table__empty",
                {colspan: columns.length},
                emptyContent ?? "No data",
              ),
            ),
          )
        : m(
            "tbody",
            rows.map((row, i) =>
              m(
                "tr",
                {
                  key: rowKey ? rowKey(row, i) : i,
                  class: onRowClick ? "sq-table__row--clickable" : "",
                  onclick: onRowClick ? () => onRowClick(row, i) : undefined,
                },
                columns.map((col) =>
                  m(
                    "td",
                    {
                      key: col.key,
                      style: {textAlign: col.align ?? "left"},
                    },
                    col.render(row),
                  ),
                ),
              ),
            ),
          );

    return m("div.sq-table-wrap", m("table.sq-table", [thead, tbody]));
  }
}
