import {describe, expect, it} from "vitest";
import {parseSimple} from "syntaqlite";

describe("parseSimple", () => {
  it("returns empty tables for empty input", () => {
    expect(parseSimple("").tables).toEqual([]);
  });

  it("skips comments and blank lines", () => {
    const result = parseSimple("# comment\n\n  \n# another");
    expect(result.tables).toEqual([]);
  });

  it("parses table name without columns", () => {
    const result = parseSimple("users");
    expect(result.tables).toEqual([{name: "users"}]);
  });

  it("parses table with columns", () => {
    const result = parseSimple("users: id, name, email");
    expect(result.tables).toEqual([{name: "users", columns: ["id", "name", "email"]}]);
  });

  it("handles colon with no columns", () => {
    const result = parseSimple("users:");
    expect(result.tables).toEqual([{name: "users"}]);
  });

  it("trims whitespace from names and columns", () => {
    const result = parseSimple("  users  :  id ,  name  ");
    expect(result.tables).toEqual([{name: "users", columns: ["id", "name"]}]);
  });

  it("parses multiple tables", () => {
    const result = parseSimple("users: id, name\n# skip\norders: id, user_id");
    expect(result.tables).toHaveLength(2);
    expect(result.tables[0].name).toBe("users");
    expect(result.tables[1].name).toBe("orders");
  });
});
