// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import type {DialectBinding} from "./types.js";
import type {Engine} from "./engine.js";

export interface DialectPreset {
  id: string;
  label: string;
  wasmUrl: string;
  symbol: string;
}

export const BUILTIN_PRESETS: DialectPreset[] = [
  // SQLite is a side module; the runtime loads it via loadDynamicLibrary.
  {id: "sqlite", label: "SQLite", wasmUrl: "syntaqlite-sqlite.wasm", symbol: "syntaqlite_sqlite_dialect_template"},
];

export interface DialectManagerConfig {
  presets?: DialectPreset[];
  onDialectChanged?: () => void;
}

export class DialectManager {
  activePresetId: string;
  active: DialectBinding | undefined = undefined;
  customLabel: string | undefined = undefined;

  private presets: DialectPreset[];
  private onDialectChanged: (() => void) | undefined;
  private cache = new Map<string, DialectBinding>();

  constructor(config: DialectManagerConfig = {}) {
    this.presets = config.presets ?? BUILTIN_PRESETS;
    this.onDialectChanged = config.onDialectChanged;
    this.activePresetId = this.presets[0]?.id ?? "";
  }

  getPresets(): readonly DialectPreset[] {
    return this.presets;
  }

  async loadDefault(engine: Engine): Promise<void> {
    await this.selectPreset(engine, this.presets[0]);
  }

  async selectPreset(engine: Engine, preset: DialectPreset): Promise<void> {
    if (!engine.ready) {
      engine.updateStatus("Runtime is not initialized yet.", true);
      this.onDialectChanged?.();
      return;
    }
    try {
      const binding = await this.loadCached(engine, preset.wasmUrl, preset.symbol, preset.label);
      engine.setDialectPointer(binding.ptr);
      this.active = binding;
      this.activePresetId = preset.id;
      this.customLabel = undefined;
      engine.updateStatus(`Dialect: ${preset.label}`);
    } catch (err) {
      engine.updateStatus(`Failed to load ${preset.label}: ${(err as Error).message}`, true);
    }
    this.onDialectChanged?.();
  }

  async loadFromFile(engine: Engine, file: File, symbol: string): Promise<string | undefined> {
    if (!engine.ready) {
      return "Runtime is not initialized yet.";
    }
    const url = URL.createObjectURL(file);
    try {
      const binding = await engine.loadDialectFromUrl(url, symbol);
      binding.label = file.name;
      engine.setDialectPointer(binding.ptr);
      this.active = binding;
      this.activePresetId = "custom";
      this.customLabel = file.name;
      engine.updateStatus(`Dialect: ${file.name}`);
      return undefined;
    } catch (err) {
      return (err as Error).message;
    } finally {
      URL.revokeObjectURL(url);
    }
  }

  private async loadCached(
    engine: Engine,
    wasm: string,
    symbol: string,
    label: string,
  ): Promise<DialectBinding> {
    const cached = this.cache.get(symbol);
    if (cached) return cached;
    const binding = await engine.loadDialectFromUrl(wasm, symbol);
    binding.label = label;
    this.cache.set(symbol, binding);
    return binding;
  }
}
