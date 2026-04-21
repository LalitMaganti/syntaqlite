+++
title = "MCP server setup"
description = "Wire the syntaqlite MCP server into Claude Desktop, Cursor, Windsurf, or any MCP client."
weight = 9
+++

# MCP server

The `syntaqlite` binary ships a stdio-based MCP server. Any MCP-compatible
agent can spawn it and call its tools to format, analyze, and parse SQL.
This guide wires it up.

Prerequisite: `syntaqlite` on your `$PATH`. See the
[CLI tutorial](@/getting-started/cli.md) for install options.

## Add the server to your client's config

Every MCP client points at the same command. Drop this block into your
client's MCP config file:

```json
{
  "mcpServers": {
    "syntaqlite": {
      "command": "syntaqlite",
      "args": ["mcp"]
    }
  }
}
```

If the config file already has an `mcpServers` object, merge the `syntaqlite`
entry into it.

## Config file location

| Client | Path |
|--------|------|
| Claude Desktop (macOS) | `~/Library/Application Support/Claude/claude_desktop_config.json` |
| Claude Desktop (Linux) | `~/.config/Claude/claude_desktop_config.json` |
| Claude Desktop (Windows) | `%APPDATA%\Claude\claude_desktop_config.json` |
| Cursor | `.cursor/mcp.json` in your project |
| Windsurf | `~/.codeium/windsurf/mcp_config.json` |

Other MCP-compatible clients follow the same schema; consult their docs for
the config path.

## Restart and verify

Restart the client so it picks up the new server. Then ask it to format some
SQL, for example:

> Format this SQL: `select id,name from users where active=1`

The client should invoke syntaqlite's `format_sql` tool and return:

```sql
SELECT id, name FROM users WHERE active = 1;
```

If nothing happens, check the client's MCP/server log for spawn errors; the
usual cause is `syntaqlite` not being on the `$PATH` seen by the GUI app (GUI
apps often don't inherit your shell's environment — specify an absolute path
to `syntaqlite` in the `command` field if needed).

## Next steps

- [MCP tools reference](@/reference/mcp-tools.md) — every tool the server
  exposes and its parameters
