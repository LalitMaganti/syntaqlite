+++
title = "CI integration"
description = "Enforce SQL formatting and validation in your CI pipeline."
weight = 5
+++

# CI integration

Run syntaqlite in CI to enforce consistent SQL formatting and catch schema
errors before they reach production.

If your project has a [`syntaqlite.toml`](@/reference/config-file.md),
formatting options, schema routing, and check levels are all read
automatically. No flags needed in CI. This keeps CI in sync with local
development.

## Format checking

Use `--check` to verify that files are already formatted without modifying
them. It exits with code 1 if any file would change:

```bash
syntaqlite fmt --check "**/*.sql"
```

## Validation

Run schema-aware validation to catch unknown tables, columns, and functions:

```bash
syntaqlite analyze "**/*.sql"
```

When a schema is configured in `syntaqlite.toml`, unresolved references are
**errors** and cause a non-zero exit code. Without a schema, the same issues
are **warnings** and the exit code remains zero, so `syntaqlite analyze`
won't fail the build until you've explicitly declared your schema.

If you're not using `syntaqlite.toml` and passing files directly, file order
matters: put DDL files first so the schema is available when queries are
validated. Within each file, `CREATE TABLE` and `CREATE VIEW` statements are
discovered and made available to subsequent statements.

```bash
syntaqlite analyze schema.sql "queries/**/*.sql"
```

## GitHub Actions

```yaml
name: SQL lint
on: [pull_request]

jobs:
  sql-check:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install syntaqlite
        run: pip install syntaqlite

      - name: Check formatting
        run: syntaqlite fmt --check "**/*.sql"

      - name: Validate SQL
        run: syntaqlite analyze "**/*.sql"
```

## Pre-push hook

Add a git hook to check SQL formatting before each push:

```bash
#!/bin/bash
# .git/hooks/pre-push

failed=0
for f in $(git diff --name-only origin/main --diff-filter=ACM | grep '\.sql$'); do
  if ! diff -q <(syntaqlite fmt "$f") "$f" > /dev/null 2>&1; then
    echo "Not formatted: $f"
    failed=1
  fi
done

exit $failed
```

Make it executable:

```bash
chmod +x .git/hooks/pre-push
```

## Formatting at scale

syntaqlite is fast: it reuses internal allocations across files, so formatting
thousands of files is practical:

```bash
syntaqlite fmt -i "**/*.sql"
```
