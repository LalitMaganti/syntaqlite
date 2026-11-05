import {describe, expect, it} from "vitest";
import {parseHash, serializeHash, DEFAULT_STATE} from "./url_state";
import type {PlaygroundState} from "./url_state";

// ---------------------------------------------------------------------------
// parseHash
// ---------------------------------------------------------------------------

describe("parseHash — empty / defaults", () => {
  it("returns defaults for empty string", () => {
    expect(parseHash("")).toEqual(DEFAULT_STATE);
  });

  it("returns defaults when all params are absent", () => {
    // A hash string that has no recognised keys should be identical to defaults.
    expect(parseHash("unknown=xyz")).toEqual(DEFAULT_STATE);
  });
});

describe("parseHash — dialect", () => {
  it("parses sqlite", () => {
    expect(parseHash("d=sqlite").dialect).toBe("sqlite");
  });

  it("parses perfetto", () => {
    expect(parseHash("d=perfetto").dialect).toBe("perfetto");
  });

  it("ignores unknown dialect values", () => {
    expect(parseHash("d=custom").dialect).toBe("sqlite");
    expect(parseHash("d=unknown").dialect).toBe("sqlite");
  });
});

describe("parseHash — language mode", () => {
  it("parses sql", () => {
    expect(parseHash("l=sql").languageMode).toBe("sql");
  });

  it("parses python", () => {
    expect(parseHash("l=python").languageMode).toBe("python");
  });

  it("parses typescript", () => {
    expect(parseHash("l=typescript").languageMode).toBe("typescript");
  });

  it("ignores unknown language values", () => {
    expect(parseHash("l=java").languageMode).toBe("sql");
  });
});

describe("parseHash — sqlite version and cflags", () => {
  it("parses sqliteVersion", () => {
    expect(parseHash("v=3.46.0").sqliteVersion).toBe("3.46.0");
  });

  it("parses cflags as sorted array", () => {
    expect(parseHash("f=JSON,FTS5").cflags).toEqual(["JSON", "FTS5"]);
  });

  it("returns empty cflags when param is absent", () => {
    expect(parseHash("").cflags).toEqual([]);
  });

  it("filters empty strings from cflags", () => {
    expect(parseHash("f=").cflags).toEqual([]);
  });
});

describe("parseHash — schema", () => {
  it("parses schemaFormat simple", () => {
    expect(parseHash("sf=simple").schemaFormat).toBe("simple");
  });

  it("parses schemaFormat ddl", () => {
    expect(parseHash("sf=ddl").schemaFormat).toBe("ddl");
  });

  it("ignores unknown schemaFormat values", () => {
    expect(parseHash("sf=json").schemaFormat).toBe("simple");
  });

  it("round-trips schema text via lz compression", () => {
    const sql = "users: id, name\norders: id, user_id";
    const hash = serializeHash({...DEFAULT_STATE, schema: sql});
    expect(parseHash(hash).schema).toBe(sql);
  });

  it("returns empty schema when param is absent", () => {
    expect(parseHash("").schema).toBe("");
  });
});

describe("parseHash — preset vs custom SQL", () => {
  it("parses named preset", () => {
    const s = parseHash("p=sqlite-basic-select");
    expect(s.preset).toBe("sqlite-basic-select");
    expect(s.sql).toBeNull();
  });

  it("parses custom SQL via lz compression", () => {
    const sql = "SELECT id FROM users;";
    const hash = serializeHash({...DEFAULT_STATE, preset: null, sql});
    const s = parseHash(hash);
    expect(s.preset).toBeNull();
    expect(s.sql).toBe(sql);
  });

  it("sql param takes precedence over preset param", () => {
    // Construct a hash that has both (malformed, but we should handle it).
    const sqlHash = serializeHash({...DEFAULT_STATE, preset: null, sql: "SELECT 1"});
    // Append a p= param manually.
    const hash = sqlHash + "&p=some-preset";
    const s = parseHash(hash);
    expect(s.sql).toBe("SELECT 1");
    expect(s.preset).toBeNull();
  });

  it("returns null preset and null sql when neither param is present", () => {
    const s = parseHash("");
    expect(s.preset).toBeNull();
    expect(s.sql).toBeNull();
  });
});

describe("parseHash — output tab and AST view mode", () => {
  it("parses outputTab format (default, no param)", () => {
    expect(parseHash("").outputTab).toBe("format");
  });

  it("parses outputTab ast", () => {
    expect(parseHash("ot=ast").outputTab).toBe("ast");
  });

  it("ignores unknown outputTab values", () => {
    expect(parseHash("ot=validation").outputTab).toBe("format");
  });

  it("parses astViewMode outline (default, no param)", () => {
    expect(parseHash("").astViewMode).toBe("outline");
  });

  it("parses astViewMode graph", () => {
    expect(parseHash("av=graph").astViewMode).toBe("graph");
  });

  it("ignores unknown astViewMode values", () => {
    expect(parseHash("av=tree").astViewMode).toBe("outline");
  });
});

// ---------------------------------------------------------------------------
// serializeHash
// ---------------------------------------------------------------------------

describe("serializeHash — omits defaults", () => {
  it("produces empty string for the default state", () => {
    // DEFAULT_STATE has no preset and no sql, so no meaningful params.
    // All fields are at their default values.
    expect(serializeHash(DEFAULT_STATE)).toBe("");
  });

  it("omits dialect=sqlite (default)", () => {
    const hash = serializeHash({...DEFAULT_STATE, dialect: "sqlite"});
    expect(new URLSearchParams(hash).has("d")).toBe(false);
  });

  it("omits languageMode=sql (default)", () => {
    const hash = serializeHash({...DEFAULT_STATE, languageMode: "sql"});
    expect(new URLSearchParams(hash).has("l")).toBe(false);
  });

  it("omits sqliteVersion=latest (default)", () => {
    const hash = serializeHash({...DEFAULT_STATE, sqliteVersion: "latest"});
    expect(new URLSearchParams(hash).has("v")).toBe(false);
  });

  it("omits empty cflags (default)", () => {
    const hash = serializeHash({...DEFAULT_STATE, cflags: []});
    expect(new URLSearchParams(hash).has("f")).toBe(false);
  });

  it("omits schemaFormat=simple (default)", () => {
    const hash = serializeHash({...DEFAULT_STATE, schemaFormat: "simple"});
    expect(new URLSearchParams(hash).has("sf")).toBe(false);
  });

  it("omits outputTab=format (default)", () => {
    const hash = serializeHash({...DEFAULT_STATE, outputTab: "format"});
    expect(new URLSearchParams(hash).has("ot")).toBe(false);
  });

  it("omits astViewMode=outline (default)", () => {
    const hash = serializeHash({...DEFAULT_STATE, astViewMode: "outline"});
    expect(new URLSearchParams(hash).has("av")).toBe(false);
  });
});

describe("serializeHash — includes non-defaults", () => {
  it("serializes perfetto dialect", () => {
    const hash = serializeHash({...DEFAULT_STATE, dialect: "perfetto"});
    expect(new URLSearchParams(hash).get("d")).toBe("perfetto");
  });

  it("serializes python language mode", () => {
    const hash = serializeHash({...DEFAULT_STATE, languageMode: "python"});
    expect(new URLSearchParams(hash).get("l")).toBe("python");
  });

  it("serializes sqlite version", () => {
    const hash = serializeHash({...DEFAULT_STATE, sqliteVersion: "3.46.0"});
    expect(new URLSearchParams(hash).get("v")).toBe("3.46.0");
  });

  it("serializes cflags as comma-separated string", () => {
    const hash = serializeHash({...DEFAULT_STATE, cflags: ["FTS5", "JSON"]});
    expect(new URLSearchParams(hash).get("f")).toBe("FTS5,JSON");
  });

  it("serializes ddl schema format", () => {
    const hash = serializeHash({...DEFAULT_STATE, schemaFormat: "ddl"});
    expect(new URLSearchParams(hash).get("sf")).toBe("ddl");
  });

  it("serializes named preset", () => {
    const hash = serializeHash({...DEFAULT_STATE, preset: "sqlite-window-functions", sql: null});
    const p = new URLSearchParams(hash);
    expect(p.get("p")).toBe("sqlite-window-functions");
    expect(p.has("s")).toBe(false);
  });

  it("serializes custom SQL and omits preset param", () => {
    const hash = serializeHash({...DEFAULT_STATE, preset: null, sql: "SELECT 1"});
    const p = new URLSearchParams(hash);
    expect(p.has("s")).toBe(true);
    expect(p.has("p")).toBe(false);
  });

  it("serializes ast output tab", () => {
    const hash = serializeHash({...DEFAULT_STATE, outputTab: "ast"});
    expect(new URLSearchParams(hash).get("ot")).toBe("ast");
  });

  it("serializes graph AST view mode", () => {
    const hash = serializeHash({...DEFAULT_STATE, astViewMode: "graph"});
    expect(new URLSearchParams(hash).get("av")).toBe("graph");
  });

  it("includes cd=1 when customDialect is true", () => {
    const hash = serializeHash(DEFAULT_STATE, true);
    expect(new URLSearchParams(hash).get("cd")).toBe("1");
  });

  it("omits cd when customDialect is false", () => {
    const hash = serializeHash(DEFAULT_STATE, false);
    expect(new URLSearchParams(hash).has("cd")).toBe(false);
  });
});

// ---------------------------------------------------------------------------
// Round-trip: parse(serialize(state)) === state
// ---------------------------------------------------------------------------

describe("round-trip", () => {
  function roundTrip(state: PlaygroundState, customDialect = false): PlaygroundState {
    return parseHash(serializeHash(state, customDialect));
  }

  it("round-trips default state", () => {
    expect(roundTrip(DEFAULT_STATE)).toEqual(DEFAULT_STATE);
  });

  it("round-trips full non-default state", () => {
    const state: PlaygroundState = {
      dialect: "perfetto",
      languageMode: "python",
      sqliteVersion: "3.46.0",
      cflags: ["FTS5", "JSON"],
      preset: null,
      sql: "SELECT ts, dur FROM slice WHERE dur > 1000;",
      schemaFormat: "ddl",
      schema: "CREATE TABLE slice (ts INT, dur INT);",
      outputTab: "ast",
      astViewMode: "graph",
    };
    expect(roundTrip(state)).toEqual(state);
  });

  it("round-trips named preset (sql stays null)", () => {
    const state: PlaygroundState = {
      ...DEFAULT_STATE,
      preset: "sqlite-window-functions",
      sql: null,
    };
    const result = roundTrip(state);
    expect(result.preset).toBe("sqlite-window-functions");
    expect(result.sql).toBeNull();
  });

  it("round-trips custom SQL with multiline content", () => {
    const sql = "SELECT\n  a,\n  b\nFROM t\nWHERE c = 1;";
    const state = {...DEFAULT_STATE, preset: null, sql};
    expect(roundTrip(state).sql).toBe(sql);
  });

  it("round-trips schema with special characters", () => {
    const schema = "users: id, email@domain, name with spaces";
    expect(roundTrip({...DEFAULT_STATE, schema}).schema).toBe(schema);
  });

  it("cd flag does not survive a round-trip (not part of PlaygroundState)", () => {
    // serializeHash with customDialect=true produces cd=1, but parseHash
    // does not put it into PlaygroundState — it stays out-of-band.
    const hash = serializeHash(DEFAULT_STATE, true);
    expect(new URLSearchParams(hash).get("cd")).toBe("1");
    // cd is not in PlaygroundState so the parsed result equals DEFAULT_STATE.
    expect(parseHash(hash)).toEqual(DEFAULT_STATE);
  });
});
