# syntaqlite

A fast, portable C library for tokenizing, parsing, and formatting SQLite SQL — built from SQLite's own grammar for 100% compatibility.

## Building

```bash
tools/dev/install-build-deps
tools/dev/setup-all-configs --config mac_debug
tools/dev/ninja -C out/mac_debug
```

## Testing

```bash
tools/tests/run-ast-diff-tests --binary out/mac_debug/ast_test
```

## License

Apache 2.0. SQLite components are public domain under the [SQLite blessing](https://www.sqlite.org/copyright.html). See [LICENSE](LICENSE) for details.
