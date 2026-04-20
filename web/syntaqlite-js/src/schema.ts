// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import type {Engine} from "./engine.js";

export type SchemaFormat = "simple" | "ddl";

interface TableEntry {
  name: string;
  columns: string[];
}

export interface SessionContextPayload {
  tables: TableEntry[];
  views: never[];
  functions: never[];
}

/** Parse "simple" schema format into a session context payload. */
export function parseSimple(rawText: string): SessionContextPayload {
  const tables: TableEntry[] = [];
  for (const raw of rawText.split("\n")) {
    const line = raw.trim();
    if (line === "" || line.startsWith("#")) continue;

    const colonIdx = line.indexOf(":");
    if (colonIdx === -1) {
      // Table name only, no columns.
      const name = line.trim();
      if (name) tables.push({name, columns: []});
    } else {
      const name = line.slice(0, colonIdx).trim();
      const colsPart = line.slice(colonIdx + 1).trim();
      const columns = colsPart
        ? colsPart.split(",").map((c) => c.trim()).filter(Boolean)
        : [];
      if (name) tables.push({name, columns});
    }
  }
  return {tables, views: [], functions: []};
}

/**
 * Applies user-defined schema context to the engine.
 *
 * Serializable state (rawText, format) lives in PlaygroundState /
 * UrlStateManager — this class holds only the computed feedback state
 * (parseError, parsedTableCount) produced by applying the schema.
 */
export class SchemaContextManager {
  parseError: string | undefined = undefined;
  /** Number of tables successfully loaded, or undefined when no schema is set. */
  parsedTableCount: number | undefined = undefined;
  private lastAppliedKey = "";

  /**
   * Apply schema to the engine.
   * Pass `force=true` to re-apply even when rawText/format haven't changed
   * (needed after dialect switches that reset the engine's LSP host).
   * Returns true if the schema was actually applied.
   */
  apply(engine: Engine, rawText: string, format: SchemaFormat, force = false): boolean {
    const key = `${format}:${rawText}`;
    if (!force && key === this.lastAppliedKey) return false;
    this.lastAppliedKey = key;

    if (rawText.trim() === "") {
      this.parseError = undefined;
      this.parsedTableCount = undefined;
      engine.clearSessionContext();
    } else if (format === "ddl") {
      const result = engine.setSessionContextDdl(rawText);
      if (result.ok) {
        this.parseError = undefined;
      } else {
        this.parseError = result.error;
      }
    } else {
      this.parseError = undefined;
      const payload = parseSimple(rawText);
      this.parsedTableCount = payload.tables.length;
      engine.setSessionContext(JSON.stringify(payload));
    }
    return true;
  }
}
