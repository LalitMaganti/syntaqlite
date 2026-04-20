// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import {test, expect} from "@playwright/test";

test("packed tarball formats and validates SQL in a real browser", async ({page}) => {
  const logs: string[] = [];
  page.on("console", (msg) => logs.push(`[${msg.type()}] ${msg.text()}`));
  page.on("pageerror", (err) => logs.push(`[pageerror] ${err.message}`));

  await page.goto("/");
  await expect(page.locator("#status")).toHaveText(/READY|ERROR/, {timeout: 30_000});

  const result = await page.evaluate(
    () => (window as unknown as {__syntaqlite_result?: unknown}).__syntaqlite_result,
  ) as {
    ok: boolean;
    error?: string;
    stack?: string;
    fmt?: {ok: boolean; text: string};
    diag?: {ok: boolean; diagnostics: unknown[]};
  } | undefined;

  if (!result?.ok) {
    const pageLog = await page.locator("#log").textContent();
    throw new Error(
      "smoke failed: " + (result?.error ?? "no result") + "\n" +
      "stack:\n" + (result?.stack ?? "") + "\n" +
      "page log:\n" + pageLog + "\n" +
      "console:\n" + logs.join("\n"),
    );
  }

  // Formatting: `select a,b from t where a=1` with upper + 2-space indent
  // should emit SELECT/FROM/WHERE in uppercase with a reformatted body.
  expect(result.fmt!.ok).toBe(true);
  expect(result.fmt!.text).toMatch(/SELECT/);
  expect(result.fmt!.text).toMatch(/FROM/);
  expect(result.fmt!.text).toMatch(/WHERE/);

  // Validation: `selec 1 frm t` is syntactically invalid; the validator
  // must report at least one diagnostic.
  expect(result.diag!.ok).toBe(true);
  expect(result.diag!.diagnostics.length).toBeGreaterThan(0);
});
