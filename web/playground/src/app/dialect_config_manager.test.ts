import {describe, expect, it} from "vitest";
import {DialectConfigManager, versionToInt} from "syntaqlite";

describe("versionToInt", () => {
  it("returns max int for 'latest'", () => {
    expect(versionToInt("latest")).toBe(0x7fffffff);
  });

  it("encodes 3-part version correctly", () => {
    expect(versionToInt("3.46.0")).toBe(3_046_000);
    expect(versionToInt("3.24.0")).toBe(3_024_000);
    expect(versionToInt("3.47.2")).toBe(3_047_002);
  });

  it("returns max int for malformed input", () => {
    expect(versionToInt("3.46")).toBe(0x7fffffff);
    expect(versionToInt("bad")).toBe(0x7fffffff);
  });
});

describe("DialectConfigManager", () => {
  it("visibleCflagEntries filters by version", () => {
    const mgr = new DialectConfigManager();
    mgr.availableCflags = [
      {name: "A", minVersion: 3_046_000, category: "x"},
      {name: "B", minVersion: 3_030_000, category: "x"},
    ];
    expect(mgr.visibleCflagEntries("3.35.0").map((e) => e.name)).toEqual(["B"]);
  });

  it("visibleCflagEntries includes minVersion=0 for all versions", () => {
    const mgr = new DialectConfigManager();
    mgr.availableCflags = [
      {name: "ALWAYS", minVersion: 0, category: "x"},
      {name: "NEW", minVersion: 3_046_000, category: "x"},
    ];
    expect(mgr.visibleCflagEntries("3.24.0").map((e) => e.name)).toEqual(["ALWAYS"]);
  });

  it("apply passes canonical flag name without SYNTAQLITE_CFLAG_ prefix", () => {
    const mgr = new DialectConfigManager();
    mgr.availableCflags = [{name: "SQLITE_OMIT_ALTERTABLE", minVersion: 0, category: "parser"}];
    const setCflagCalls: string[] = [];
    const fakeEngine = {
      setSqliteVersion: () => {},
      clearAllCflags: () => {},
      setCflag: (name: string) => {
        setCflagCalls.push(name);
      },
    } as unknown as import("syntaqlite").Engine;
    mgr.apply(fakeEngine, "latest", ["SQLITE_OMIT_ALTERTABLE"]);
    expect(setCflagCalls).toEqual(["SQLITE_OMIT_ALTERTABLE"]);
  });
});
