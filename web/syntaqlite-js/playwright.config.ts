// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import {defineConfig} from "@playwright/test";

export default defineConfig({
  testDir: "./tests",
  testMatch: "**/*.spec.ts",
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 1 : 0,
  workers: 1,
  reporter: "list",
  use: {
    baseURL: "http://localhost:4174/",
    trace: "retain-on-failure",
  },
  projects: [
    {name: "chromium", use: {browserName: "chromium"}},
  ],
  webServer: {
    command: "python3 -m http.server 4174 --directory tests/fixture",
    url: "http://localhost:4174/",
    reuseExistingServer: !process.env.CI,
    stdout: "ignore",
    stderr: "pipe",
  },
});
