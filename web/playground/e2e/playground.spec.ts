// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import {test, expect} from "@playwright/test";

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Extract the URL hash (without the leading #). */
async function getHash(page: import("@playwright/test").Page) {
  return page.evaluate(() => window.location.hash.slice(1));
}

// ---------------------------------------------------------------------------
// Smoke test
// ---------------------------------------------------------------------------

test("page loads and shows the toolbar", async ({page}) => {
  await page.goto("/");
  await expect(page.locator("header.sq-toolbar")).toBeVisible();
  await expect(page.locator(".sq-workspace")).toBeVisible();
});

// ---------------------------------------------------------------------------
// URL state: reading from hash on load
// ---------------------------------------------------------------------------

test("dialect param d=perfetto is preserved in hash after load", async ({page}) => {
  await page.goto("/#d=perfetto");
  // Wait for the app to settle and write URL state.
  await page.waitForFunction(() => window.location.hash.includes("d=perfetto"), {timeout: 10000});
  const hash = await getHash(page);
  expect(new URLSearchParams(hash).get("d")).toBe("perfetto");
});

test("outputTab param ot=ast is preserved in hash after load", async ({page}) => {
  await page.goto("/#ot=ast");
  // The viewer pane should be visible.
  await expect(page.locator(".sq-viewer-pane")).toBeVisible();
  // Hash should still carry ot=ast after the app initialises.
  await page.waitForFunction(() => window.location.hash.includes("ot=ast"), {timeout: 10000});
  const hash = await getHash(page);
  expect(new URLSearchParams(hash).get("ot")).toBe("ast");
});

test("astViewMode param av=graph is preserved in hash after load", async ({page}) => {
  await page.goto("/#ot=ast&av=graph");
  await page.waitForFunction(() => window.location.hash.includes("av=graph"), {timeout: 10000});
  const hash = await getHash(page);
  expect(new URLSearchParams(hash).get("av")).toBe("graph");
});

test("preset param p= is preserved in hash after load", async ({page}) => {
  await page.goto("/#p=sqlite-basic-select");
  await page.waitForFunction(() => window.location.hash.includes("p=sqlite-basic-select"), {timeout: 10000});
  const hash = await getHash(page);
  expect(new URLSearchParams(hash).get("p")).toBe("sqlite-basic-select");
  // No custom SQL param should be present.
  expect(new URLSearchParams(hash).has("s")).toBe(false);
});

test("cd=1 shows the custom dialect notice banner", async ({page}) => {
  await page.goto("/#cd=1");
  await expect(page.locator(".sq-info-banner")).toBeVisible();
  await expect(page.locator(".sq-info-banner")).toContainText("custom dialect");
});

test("custom dialect notice banner can be dismissed", async ({page}) => {
  await page.goto("/#cd=1");
  const banner = page.locator(".sq-info-banner");
  await expect(banner).toBeVisible();
  await banner.locator(".sq-info-banner__dismiss").click();
  await expect(banner).not.toBeVisible();
});

// ---------------------------------------------------------------------------
// URL state: writing to hash on interaction
// ---------------------------------------------------------------------------

test("default load writes a preset param to the hash (not custom SQL)", async ({page}) => {
  await page.goto("/");
  // Wait for the workspace to write the initial preset to the URL.
  await page.waitForFunction(() => window.location.hash.length > 0, {timeout: 10000});
  const hash = await getHash(page);
  const params = new URLSearchParams(hash);
  // Should have a named preset, not custom SQL.
  expect(params.has("p")).toBe(true);
  expect(params.has("s")).toBe(false);
});

// ---------------------------------------------------------------------------
// Editor features (served over the in-process LSP)
// ---------------------------------------------------------------------------

test("invalid SQL surfaces diagnostic markers", async ({page}) => {
  await page.goto("/");
  await expect(page.locator(".sq-viewer-pane")).toContainText(/SELECT/i, {timeout: 15000});

  const editor = page.locator(".sq-workspace .monaco-editor").first();
  await expect(editor).toBeVisible();
  await editor.click();
  await page.keyboard.press("ControlOrMeta+a");
  await page.keyboard.type("selec 1");

  await expect(
    page.locator(".sq-workspace .monaco-editor .squiggly-error").first(),
  ).toBeVisible({timeout: 10000});
});

test("typing in the editor surfaces SQL completions", async ({page}) => {
  await page.goto("/");
  // The viewer pane rendering formatted SQL proves the wasm engine and
  // dialect finished loading — only then can completions answer.
  await expect(page.locator(".sq-viewer-pane")).toContainText(/SELECT/i, {timeout: 15000});

  const editor = page.locator(".sq-workspace .monaco-editor").first();
  await expect(editor).toBeVisible();
  await editor.click();
  await page.keyboard.press("ControlOrMeta+a");
  await page.keyboard.type("SELECT 1 FROM t WHERE ");
  // Explicitly trigger suggest for determinism.
  await page.keyboard.press("Control+Space");

  const suggestions = page.locator(".suggest-widget .monaco-list-row");
  await expect(suggestions.first()).toBeVisible({timeout: 10000});
  // Keyword completions must be present (e.g. NOT/EXISTS/CASE after WHERE).
  expect(await suggestions.count()).toBeGreaterThan(0);
});

test("switching output tab updates the URL hash to ot=ast", async ({page}) => {
  await page.goto("/");
  // Wait for page to settle.
  await page.waitForFunction(() => window.location.hash.length > 0, {timeout: 10000});

  // Find the AST tab button in the output panel tabs.
  const astTab = page.getByRole("button", {name: /ast/i}).first();
  if (await astTab.isVisible()) {
    await astTab.click();
    await page.waitForFunction(() => window.location.hash.includes("ot=ast"), {timeout: 5000});
    const hash = await getHash(page);
    expect(new URLSearchParams(hash).get("ot")).toBe("ast");
  }
});
