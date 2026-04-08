#!/bin/bash
# Build the Perfetto dialect and run perfetto-stdlib-graph.
#
# Usage:
#   ./examples/perfetto-stdlib-graph/run.sh ascii
#   ./examples/perfetto-stdlib-graph/run.sh ascii --table thread_slice
#   ./examples/perfetto-stdlib-graph/run.sh html -o /tmp/graph.html
#   ./examples/perfetto-stdlib-graph/run.sh json --pretty
#   ./examples/perfetto-stdlib-graph/run.sh dot --cluster

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DIALECT_DIR="$REPO_ROOT/dialects/perfetto"
BUILD_DIR="$(mktemp -d)"
trap 'rm -rf "$BUILD_DIR"' EXIT

CSRC_DIR="$BUILD_DIR/csrc"
mkdir -p "$CSRC_DIR"

# Step 1: Generate C sources for the Perfetto dialect.
echo "Generating Perfetto dialect C sources..." >&2
cargo run -p syntaqlite-cli --quiet -- dialect \
  --name perfetto \
  --actions-dir "$DIALECT_DIR/actions" \
  --nodes-dir "$DIALECT_DIR/nodes" \
  --macro-style rust \
  --output-dir "$CSRC_DIR"

# Step 2: Write runtime shim headers (same as test infrastructure).
cat > "$CSRC_DIR/syntaqlite_runtime.h" <<'EOF'
#ifndef SYNTAQLITE_RUNTIME_H
#define SYNTAQLITE_RUNTIME_H
#include "syntaqlite/config.h"
#include "syntaqlite/types.h"
#include "syntaqlite/grammar.h"
#include "syntaqlite/parser.h"
#include "syntaqlite/tokenizer.h"
#endif
EOF

cat > "$CSRC_DIR/syntaqlite_dialect.h" <<'EOF'
#ifndef SYNTAQLITE_EXT_H
#define SYNTAQLITE_EXT_H
#include "syntaqlite_dialect/sqlite_compat.h"
#include "syntaqlite_dialect/dialect_types.h"
#include "syntaqlite_dialect/dialect_macros.h"
#include "syntaqlite_dialect/arena.h"
#include "syntaqlite_dialect/vec.h"
#include "syntaqlite_dialect/ast_builder.h"
#endif
EOF

# Step 3: Compile to a shared library.
echo "Compiling Perfetto dialect shared library..." >&2
PARSER_SYS="$REPO_ROOT/syntaqlite-syntax"

cc -dynamiclib -fPIC \
  "$CSRC_DIR/syntaqlite_perfetto.c" \
  "$PARSER_SYS/csrc/parser.c" \
  "$PARSER_SYS/csrc/token_wrapped.c" \
  -DSYNTAQLITE_OMIT_SQLITE_API \
  -I "$CSRC_DIR" \
  -I "$PARSER_SYS" \
  -I "$PARSER_SYS/include" \
  -o "$BUILD_DIR/libsyntaqlite_perfetto.dylib"

# Step 4: Run the Python graph tool.
echo "Running perfetto-stdlib-graph..." >&2
python3 "$SCRIPT_DIR/perfetto_stdlib_graph.py" \
  --dialect "$BUILD_DIR/libsyntaqlite_perfetto.dylib" \
  --stdlib "$REPO_ROOT/third_party/src/perfetto/src/trace_processor/perfetto_sql/stdlib" \
  "$@"
