# Python RPC: programmatic module resolver — future enhancement

## Status

Deferred. The current PR (#196) ships an **eager modules dict** on
`Schema`: callers pre-resolve every module they expect to be referenced
and hand them to `validate()` in one shot. This doc captures the
bidirectional-callback path we considered and deferred, so future work
has the shape in hand.

## What we have today

`syntaqlite.Schema` accepts a `modules: dict[str, str]` — a map from
dotted module path to SQL source. The `serve json` handler installs a
`HashMapResolver` built from that map for the duration of one
`validate` call, then clears it.

```python
schema = syntaqlite.Schema(
    tables=[...],
    modules={"stdlib.foo": "CREATE TABLE t(x)", "stdlib.bar": "..."},
)
result = sq.validate(sql, schema)
```

This works when the caller can enumerate the module set upfront. For
Perfetto's stdlib (files on disk at known paths) that's fine — load
them all and pass. For callers with dynamic resolution (e.g. modules
generated on demand, fetched from a network cache, or keyed on a
database) it's not.

The resolver construction point lives in
`syntaqlite-cli/src/commands/serve/json.rs::build_module_resolver`. It
takes a `ValidateReq` and returns `Option<Box<dyn ModuleResolver>>`;
today that's only the eager hashmap path.

## Why defer the callback path

The Perfetto use case (the driver for this feature) is already covered
by the eager dict. Programmatic resolvers come up in specific tooling
contexts, not in the common Python-calls-syntaqlite path. The cost of
adding a bidirectional protocol is real:

- The response stream gains a third frame type (callback request).
- The Python `_call` loop has to handle callbacks mid-response.
- The Rust `serve` loop has to share its stdin/stdout with a resolver
  impl, which fights Rust's ownership model
  (`Box<dyn ModuleResolver + 'static>` vs. a borrowed `BufReader` /
  `BufWriter`).

None of these are hard on their own; all of them are dead weight if
nobody uses the feature.

## Proposed wire shape

Add a new frame type alongside the existing `{"ok": ...}`:

```jsonl
{"callback":"resolve_module","module":"stdlib.foo"}
```

The client replies in-band (no wrapping):

```jsonl
{"source":"CREATE TABLE t(x INT)"}   # or {"source":null}
```

The server continues reading responses and/or the final `{"ok":true,
"result":...}` frame. Any number of callbacks can interleave within a
single `validate` call (transitive imports resolve lazily).

Python opts in by passing a callable on `Schema`:

```python
schema = syntaqlite.Schema(
    module_resolver=lambda path: my_fetch(path),
)
```

When `module_resolver` is set, the Python client adds
`{"resolve_modules_via_callback": true}` to the request, and the `_call`
loop becomes:

```python
while True:
    line = self._stdout.readline()
    msg = json.loads(line)
    if "callback" in msg:
        source = resolver(msg["module"])
        self._stdin.write(json.dumps({"source": source}) + "\n")
        self._stdin.flush()
        continue
    # terminal frame
    return msg
```

## Rust plumbing sketch

`build_module_resolver` already owns the seam. Adding the callback
resolver is one more arm:

```rust
fn build_module_resolver(req: &ValidateReq, stdio: &Rc<RefCell<Stdio>>)
    -> Option<Box<dyn ModuleResolver>>
{
    if req.resolve_modules_via_callback.unwrap_or(false) {
        return Some(Box::new(RpcResolver { stdio: stdio.clone() }));
    }
    // eager path (unchanged)
    let modules = req.modules.as_ref()?;
    if modules.is_empty() { return None; }
    Some(Box::new(HashMapResolver(modules.clone())))
}

struct RpcResolver {
    stdio: Rc<RefCell<Stdio>>,
}

impl ModuleResolver for RpcResolver {
    fn resolve(&self, path: &str) -> Option<String> {
        let mut io = self.stdio.borrow_mut();
        writeln!(io.writer, r#"{{"callback":"resolve_module","module":{path:?}}}"#).ok()?;
        io.writer.flush().ok()?;
        let mut line = String::new();
        io.reader.read_line(&mut line).ok()?;
        let reply: Value = serde_json::from_str(line.trim()).ok()?;
        reply.get("source")?.as_str().map(ToOwned::to_owned)
    }
}
```

The serve loop moves `BufReader<Stdin>` and `BufWriter<Stdout>` into
`Rc<RefCell<Stdio>>` so both the loop and the resolver can grab a
mutable handle. `Box<dyn ModuleResolver + 'static>` is satisfied because
`Rc<RefCell<Stdio>>` is `'static` when `Stdio` is.

## Scope estimate

- `python/syntaqlite/__init__.py`: add `module_resolver` field on
  `Schema`; extend `_call` to handle callback frames. ~40 LOC.
- `syntaqlite-cli/src/commands/serve/json.rs`: add `RpcResolver`,
  refactor serve loop to share `Stdio` via `Rc<RefCell<>>`. ~100 LOC.
- Tests: a `serve` suite case that passes a callable which records
  which modules were requested.

Rough total: ~150 LOC, half a day with the existing seam in place.
