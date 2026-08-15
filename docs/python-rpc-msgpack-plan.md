# Python RPC: msgpack + int-tag AST: future enhancement

## Status

Deferred. The current PR (#196) ships the CLI+RPC Python client on
**line-delimited JSON**, invoked as `syntaqlite serve json`. The CLI
already takes a protocol subcommand; a future `syntaqlite serve msgpack`
would live side-by-side under `syntaqlite-cli/src/commands/serve/`.
This doc captures the msgpack/msgspec path we considered and deferred,
so future work has the shape in hand.

## Motivation

The Python client spawns `syntaqlite serve` once and talks to it over
stdio, amortizing process startup + dialect init. That's the dominant
win over the retired CPython extension. Per-call framing/serialization
only starts to matter after that.

Two cost centres remain that msgpack + msgspec would compress:

1. **Payload size for `parse`**. The JSON AST is a tree of
   `{"type": "NodeName", ...}` dicts. Repeated `"type"` keys and
   PascalCase display strings dominate. On a typical 10-statement fixture
   the JSON encoding is 3–4× the size of a tagged msgpack array with
   integer tags.

2. **Python-side parse cost**. `json.loads` returns plain `dict`s; we
   then walk and wrap them via `_NODE_MAP` / `_wrap`. With msgspec we can
   describe the AST as a union of `msgspec.Struct(array_like=True, tag=N)`
   classes and have the decoder hand us typed instances directly: one
   pass, no intermediate dicts.

Benchmarks we ran on a stripped-down prototype showed roughly:

| op        | JSON (today) | msgpack+msgspec (deferred) |
|-----------|-------------:|---------------------------:|
| parse     | 100 %        | ~40 %                      |
| validate  | 100 %        | ~85 %                      |
| tokenize  | 100 %        | ~70 %                      |
| format    | ~unchanged   | ~unchanged                 |

(Small corpus, representative only. Format is bound by formatting cost,
not serialization.)

## Why defer

- **Adds a hard dep (`msgspec`)** on the Python side.
- **Adds a build-time dep (`rmp-serde`)** on the Rust side.
- **Needs a schema handshake** so the client can register its integer
  tag assignments with the server before the server emits tagged frames.
- **Duplicates the AST surface**: `nodes.py` would become tagged
  `msgspec.Struct` classes, divergent from the existing
  `__slots__ + _wrap` shape used by typed result dicts.
- The JSON path is *good enough* for current users (interactive
  notebooks, test suites, LSP-ish tooling). The wins only show up when
  you parse thousands of statements per second in Python.

When we come back to this, the sub-millisecond savings may matter (e.g.
batch tooling parsing entire corpora); today they don't move any user's
observed latency.

## Proposed wire shape

### Handshake

After `READY`, before any `parse`, the client sends a `tags` op:

```jsonl
{"op":"tags","map":{"AggregateFunctionCall":1,"SelectStmt":50,...}}
```

The server stores `HashMap<String, u32>` and uses it to emit positional
int-tagged arrays. The map is the same as the generated `NodeTag`
enum in `python/syntaqlite/enums.py`, so the client just sends
`{v.name: v.value for v in NodeTag}`.

Rationale: pinning tags client-side (rather than server-side) lets
future dialect extensions add nodes without breaking clients that pinned
their own tag map.

### PARSE frame

Request:

```
PARSE <len>\n<len bytes of msgpack: {"sql":"..."}>
```

Response (on success):

```
OK <len>\n<len bytes of msgpack: {"statements":[...], "errors":[...]}>
```

Each statement is a positional msgpack array:

```
[int_tag, field0, field1, ...]
```

List nodes:

```
[int_tag, count, [children]]
```

Enums are raw `u32` discriminants; flags are raw `u8` bitfields. This
matches what msgspec expects for `array_like=True, tag=N` struct unions.

The tag-array form is provided by a small serde wrapper in
`syntaqlite-syntax/src/ast.rs` (conceptually `AnyNodeArrayIntTag`)
behind a `serde-rmp` or similar feature, parallel to the existing
`impl Serialize for AnyNode` that produces the dict form.

### Other ops

`format`, `tokenize`, `validate` stay as plain msgpack maps with
string keys because they're small responses and the wins don't justify the
extra ceremony. Only `parse` uses the tagged-array form.

## Framing

Length-prefixed binary frames replace newline-delimited JSON, because
msgpack frames contain arbitrary bytes (including 0x0a):

```
<CMD> <len>\n<len bytes>[\n]
```

(The trailing `\n` is optional but helps debugging with `cat`/tail.)

This is identical to the framing LSP uses, minus the `Content-Type`
header, which the protocol doesn't need.

## `nodes.py` codegen changes

Current (JSON): `__slots__` classes, `d["type"]` string dispatch,
`_wrap` walks dicts and builds typed instances.

Proposed (msgpack): `msgspec.Struct(array_like=True, tag=N)` classes.
The decoder uses a single `msgspec.msgpack.Decoder(ParseResponse)` without
`_wrap`, no `_NODE_MAP`, no per-field dict lookups. Generated `nodes.py`
shrinks ~40 %.

Abstract types (`Expr`, `Stmt`, `TableSource`, …) become
`Union[Concrete1, Concrete2, ...]` aliases. msgspec dispatches via the
`tag` on each concrete struct.

## Open questions

- **Dialect extensions.** When a loaded dialect adds nodes, their tags
  live past the base-dialect tag range. Do we pre-negotiate the extended
  tag map on `Dialect(path, name)` construction? Or keep tag assignment
  server-side and send tag → name along with every response? (Adds
  overhead; probably not worth it.)
- **Versioning.** Today's JSON path gives us free schema flexibility.
  With pinned int tags, a codegen-renumbering on the server would break
  clients that pinned the old tags. We'd want a grammar-version handshake
 : client sends `{"op":"hello","version":"0.4.2"}` first, server checks
  compatibility.
- **Zero-copy spans.** msgpack supports raw-bytes fields, so spans could
  be emitted as `&[u8]` slices into the original request buffer, saving
  string copies. msgspec doesn't expose zero-copy string decoding yet,
  so this is academic until it does.

## Scope estimate

- `syntaqlite-syntax`: add serde wrapper for the tagged-array form,
  plus accessors (`AnyNode::id()`, `AnyNode::statement()`,
  `AnyParsedStatement::dialect()`) so external serializers can traverse
  without touching private fields. ~300 LOC.
- `syntaqlite-cli/src/commands/serve/msgpack.rs`: new sibling of
  `serve/json.rs`. Implements the length-prefixed msgpack framing and
  the `tags` handshake; keeps other ops as msgpack maps. Wired in via
  a new `ServeProtocol::Msgpack` variant in `cli.rs`. ~400 LOC.
- `syntaqlite-buildtools/src/dialect_codegen/python_codegen.rs`:
  regenerate `nodes.py` as msgspec structs. ~250 LOC delta.
- `python/syntaqlite/__init__.py`: swap `json` → `msgspec.msgpack`
  encoder/decoder, add tag handshake. ~80 LOC delta.
- `python/pyproject.toml`: add `msgspec>=0.18` dep.
- Tests: update `serve.py` suite for framing + tagged payloads.

Rough total: ~1000 LOC, maybe half a day if the deferred work above
isn't obsolete by then.
