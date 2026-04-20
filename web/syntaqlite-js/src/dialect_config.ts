// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import type {CflagEntry, Engine} from "./engine.js";

export const VERSION_OPTIONS = [
  "latest",
  "3.47.0",
  "3.46.0",
  "3.45.0",
  "3.41.0",
  "3.39.0",
  "3.38.0",
  "3.35.0",
  "3.31.0",
  "3.30.0",
  "3.28.0",
  "3.25.0",
  "3.24.0",
  "3.23.0",
] as const;

/** Parse a dotted version string to the SQLite integer encoding. */
export function versionToInt(version: string): number {
  if (version === "latest") return 0x7fffffff;
  const parts = version.split(".");
  if (parts.length !== 3) return 0x7fffffff;
  const [major, minor, patch] = parts.map(Number);
  return major * 1_000_000 + minor * 1_000 + patch;
}

/**
 * Manages compile-flag metadata and applies dialect config to the engine.
 *
 * Serializable state (version, enabled cflags) lives in PlaygroundState /
 * UrlStateManager — this class holds only non-serializable engine metadata.
 */
export class DialectConfigManager {
  availableCflags: CflagEntry[] = [];

  loadAvailableCflags(engine: Engine): void {
    this.availableCflags = engine.getCflagList();
  }

  /** Return cflag entries whose minVersion <= the given version. */
  visibleCflagEntries(version: string): CflagEntry[] {
    const ver = versionToInt(version);
    return this.availableCflags.filter(
      (e) => e.minVersion === 0 || e.minVersion <= ver,
    );
  }

  /** Return cflag names whose minVersion <= the given version. */
  visibleCflags(version: string): string[] {
    return this.visibleCflagEntries(version).map((e) => e.name);
  }

  apply(engine: Engine, version: string, cflags: string[]): void {
    engine.setSqliteVersion(version);
    engine.clearAllCflags();
    const visible = new Set(this.visibleCflags(version));
    for (const suffix of cflags) {
      if (visible.has(suffix)) {
        engine.setCflag(suffix);
      }
    }
  }
}
