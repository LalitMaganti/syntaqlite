// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import type {EmbeddedFragment} from "./types.js";

/** Placeholder the extractor writes into `EmbeddedFragment.sql` for each hole. */
const HOLE_PLACEHOLDER = "__h__!()";

/** @experimental */
export interface PlainFragmentSql {
  /** The fragment's SQL with each hole replaced by a plain identifier. */
  sql: string;
  /** Put the original interpolation text back in place of those identifiers. */
  restore(text: string): string;
}

/** Make a fragment's SQL parseable and formattable under any dialect.
 *
 *  Holes are written into `fragment.sql` as macro calls, which only parse with
 *  macro fallback. This swaps each one for a unique identifier so the SQL can be
 *  passed to `Engine.parse` or `Engine.format`; `restore` maps the identifiers in
 *  the result back to the hole text from `source`.
 *  @experimental Embedded language support is experimental and may change. */
export function plainFragmentSql(fragment: EmbeddedFragment, source: string): PlainFragmentSql {
  // Hole offsets are UTF-8 byte offsets, so slice encoded bytes, not the strings.
  const enc = new TextEncoder();
  const dec = new TextDecoder();
  const sqlBytes = enc.encode(fragment.sql);
  const srcBytes = enc.encode(source);
  let sql = "";
  let cursor = 0;
  fragment.holes.forEach((hole, i) => {
    sql += dec.decode(sqlBytes.subarray(cursor, hole.sqlOffset)) + `__hole_${i}__`;
    cursor = hole.sqlOffset + HOLE_PLACEHOLDER.length;
  });
  sql += dec.decode(sqlBytes.subarray(cursor));
  const holeText = fragment.holes.map((h) => dec.decode(srcBytes.subarray(h.start, h.end)));
  const restore = (text: string) =>
    text.replace(/__hole_(\d+)__/g, (name, i: string) => holeText[Number(i)] ?? name);
  return {sql, restore};
}
