# Multi-Version SQLite Support

## 1. Problem

Syntaqlite currently extracts its tokenizer, keyword table, and character tables from a single vendored SQLite source tree (`third_party/src/sqlite/`, currently 3.51.2). This pins every consumer to one SQLite version's keyword recognition and token behavior, and compiles with all features enabled (no `SQLITE_OMIT_*` support).

Real consumers need version- and cflag-aware behavior:

- **Embedded tools** targeting a specific SQLite version (e.g., Android's bundled 3.32, iOS's 3.39) need the keyword set and token behavior to match that version, potentially with specific `SQLITE_OMIT_*` flags active.
- **Developer tools** (LSPs, playgrounds, linters) need to switch target version and cflags at runtime without recompilation, e.g., "show me how SQLite 3.35 with `SQLITE_OMIT_WINDOWFUNC` parses this query."

## 2. Approach

### 2.1 Key findings from the version analysis

Phase 1 analyzed 66 SQLite versions (3.12.2–3.51.2). See `sqlite-version-analysis.md` for full results. Key findings:

- **SQLite is purely additive**: no backwards-incompatible removals across the entire version range. Every version check is `>= threshold`.
- The tokenizer has 10 raw variants, but only two behaviorally significant non-keyword differences: `TK_PTR` (3.38) and `TK_QNUMBER` (3.46).
- Keywords have 7 addition points (3.24–3.47), totaling 148 keywords in the latest version.
- The grammar grew from 326 to 411 rules, all removals are refactorings.
- `TK_COMMENT` was split from `TK_SPACE` in 3.49. Syntaqlite depends on `TK_COMMENT` everywhere and always emits it regardless of target version.

### 2.2 Two axes: version and cflags

Version and `SQLITE_OMIT_*` cflags are independent axes that both affect the SQL surface:

- **Version** determines which keywords and token types exist (additive over time).
- **CFlags** determine which features are compiled in. They primarily affect keyword recognition and grammar rules.

Both axes compose: a keyword is recognized only if it exists in the target version AND its feature isn't omitted by cflags.

### 2.3 Strategy: one tokenizer, version + cflag gating

Instead of maintaining multiple tokenizer variants, we use a single tokenizer (the latest, 3.51.2) and gate version/cflag-dependent behavior via a config struct passed as a parameter.

**Dual-mode dispatch**: the same generated code serves both compile-time and runtime:

- **Compile-time** (`-DSYNQ_SQLITE_VERSION=3035000`): caller passes a static const config. The compiler constant-folds all comparisons and eliminates dead branches. Zero runtime cost.
- **Runtime**: caller constructs a config and passes it. All branches are live but cheap (integer comparisons).

### 2.4 What gets version/cflag-gated

**1. Keywords**: the keyword lookup function checks two conditions per keyword:

- `since <= config->sqlite_version`: does this keyword exist in this version?
- cflag check with polarity: is this keyword's feature enabled given the active cflags?

When a keyword isn't recognized, the tokenizer returns `TK_ID`. The parser then naturally cannot match syntax that depends on that keyword: e.g., if `RETURNING` isn't a keyword, the RETURNING clause syntax is unparseable. No AST validation needed for keyword-gated features.

**2. Token reclassification**: a small postlude in `GetToken` handles non-keyword tokenizer differences:

| Token                  | Since | Reclassification for older versions                                                                                             |
| ---------------------- | ----- | ------------------------------------------------------------------------------------------------------------------------------- |
| `TK_PTR` (`->`, `->>`) | 3.38  | Set length=1, return `TK_MINUS`. Next call naturally tokenizes `>` as `TK_GT`.                                                  |
| `TK_QNUMBER` (`1_000`) | 3.46  | Find first `_` in token, truncate length to that position, return `TK_INTEGER` or `TK_FLOAT`. Next call sees `_000` as `TK_ID`. |

**3. Subquery flag**: `SQLITE_OMIT_SUBQUERY` is the only cflag that affects grammar rules without being keyword-gated (subqueries use `(SELECT ...)`: all baseline tokens). Instead of a post-parse AST walk, the parser action code sets a flag (`pParse->sawSubquery = 1`) when a subquery production is reduced. After parsing, if `SQLITE_OMIT_SUBQUERY` is active and the flag is set, emit a diagnostic. O(1) check.

### 2.5 What does NOT get version/cflag-gated

- **`TK_COMMENT`**: syntaqlite always emits `TK_COMMENT` for comments regardless of target version. Upstream only split this from `TK_SPACE` in 3.49, but syntaqlite depends on the distinction. Deliberate enhancement that doesn't affect parsing behavior.
- **Character tables** (`aiClass`, `cc_defines`, `ctypeMap`, `upperToLower`): always use the latest. These are internal to the tokenizer we're already using.
- **Grammar**: always parse with the full latest grammar. Version enforcement happens at the token level (keyword suppression). Cflag enforcement is keyword suppression + the subquery flag.
- **Bug fixes** (NUL handling, BOM handling, i64 return type): kept unconditionally.

### 2.6 Audit of grammar-affecting `SQLITE_OMIT_*` flags

All `%ifdef`/`%ifndef` blocks in `parse.y` (3.51.2) were audited. Every flag except `SQLITE_OMIT_SUBQUERY` gates syntax that is entered through a keyword, so keyword suppression is sufficient:

| Flag                                   | Grammar effect                 | Keyword-gatable?                              |
| -------------------------------------- | ------------------------------ | --------------------------------------------- |
| `SQLITE_OMIT_EXPLAIN`                  | `EXPLAIN` statement            | Yes: `EXPLAIN` keyword                       |
| `SQLITE_OMIT_TEMPDB`                   | `TEMP` in CREATE               | Yes: `TEMP` keyword                          |
| `SQLITE_OMIT_COMPOUND_SELECT`          | `UNION`/`INTERSECT`/`EXCEPT`   | Yes: keywords                                |
| `SQLITE_OMIT_WINDOWFUNC`               | Window functions (~130 lines)  | Yes: `WINDOW`/`OVER`/`PARTITION`/etc.        |
| `SQLITE_OMIT_GENERATED_COLUMNS`        | `GENERATED ALWAYS`             | Yes: keywords                                |
| `SQLITE_OMIT_VIEW`                     | `CREATE VIEW`                  | Yes: `VIEW` keyword                          |
| `SQLITE_OMIT_CTE`                      | `WITH` clauses                 | Yes: `WITH` keyword (only used for CTEs)     |
| `SQLITE_OMIT_SUBQUERY`                 | Subqueries in FROM, IN, EXISTS | **No**: uses `LP select RP`, baseline tokens |
| `SQLITE_OMIT_CAST`                     | `CAST(x AS type)`              | Yes: `CAST` keyword                          |
| `SQLITE_OMIT_PRAGMA`                   | `PRAGMA` statements            | Yes: `PRAGMA` keyword                        |
| `SQLITE_OMIT_TRIGGER`                  | `CREATE TRIGGER` (~115 lines)  | Yes: `TRIGGER` keyword                       |
| `SQLITE_OMIT_ATTACH`                   | `ATTACH`/`DETACH`              | Yes: keywords                                |
| `SQLITE_OMIT_REINDEX`                  | `REINDEX`                      | Yes: keyword                                 |
| `SQLITE_OMIT_ANALYZE`                  | `ANALYZE`                      | Yes: keyword                                 |
| `SQLITE_OMIT_ALTERTABLE`               | `ALTER TABLE`                  | Yes: `ALTER` keyword                         |
| `SQLITE_OMIT_VIRTUALTABLE`             | `CREATE VIRTUAL TABLE`         | Yes: `VIRTUAL` keyword                       |
| `SQLITE_ENABLE_ORDERED_SET_AGGREGATES` | `WITHIN GROUP` syntax          | Yes: `WITHIN` keyword                        |

### 2.7 What we no longer need (vs the original plan)

- ~~Multiple tokenizer function variants~~ → one tokenizer, postlude checks
- ~~`ai_class` / `cc_defines` variant tracking~~ → always latest
- ~~`version_map.toml` / versioned variant files~~ → not needed
- ~~`SYNQ_VER_GE` / `SYNQ_HAS_FEATURE` dispatch macros~~ → plain `if` on config struct fields
- ~~AST validation pass~~ → keyword gating + subquery flag handles everything
- ~~Versioned variant files in `syntaqlite-codegen/sqlite/versioned/`~~ → not needed

---

## 3. Phase 1: Download and Analysis Tool (COMPLETED)

Implemented in `syntaqlite-codegen/src/version_analysis/` (feature-gated behind `version-analysis`). Results in `sqlite-version-analysis.md`.

### 3.1 What was built

- **Download script** (`tools/download-sqlite-versions`): downloads SQLite source files from GitHub mirror for 40+ versions (3.12.2–3.51.2). Idempotent, skips existing files.
- **Analysis tool** (`syntaqlite analyze-versions`): extracts 8 code fragments + keywords + grammar from each version, hashes for dedup, groups into variants, computes diffs. Outputs JSON to stdout + variant files + grammar report.
- **Modules**: `version_analysis/{mod,extract,hash,diff,keywords,grammar}.rs`
- **Dependencies**: `sha2` (hashing), `similar` (diffs), both optional behind `version-analysis` feature.

### 3.2 Key results

See `sqlite-version-analysis.md` for full analysis. Summary:

| Fragment         | Variants    | Notes                                          |
| ---------------- | ----------- | ---------------------------------------------- |
| `get_token`      | 10          | 2 behavioral: TK_PTR (3.38), TK_QNUMBER (3.46) |
| `ai_class`       | 6           | Coupled to tokenizer changes                   |
| `cc_defines`     | 4           | Coupled to tokenizer changes                   |
| `ctype_map`      | 2           | Quote char flag in 3.13                        |
| `upper_to_lower` | 2           | UBSAN tables in 3.36                           |
| `char_map`       | 1           | Frozen                                         |
| `id_char`        | 1           | Frozen                                         |
| `is_macros`      | 1           | Frozen                                         |
| Keywords         | 7 additions | 3.24–3.47, 148 total                           |
| Grammar          | 24 changes  | 326→411 rules, semantically additive           |

---

## 4. Phase 2: Config Struct and Version/CFlag Gating (COMPLETED)

### 4.1 The config struct

The config struct lives in **`syntaqlite-runtime`** rather than the dialect crate because version and cflag configuration are shared by every dialect.

```c
// syntaqlite-runtime/include/syntaqlite/dialect_config.h

typedef struct SyntaqliteDialectConfig {
    int32_t  sqlite_version;   // Target version (e.g., 3035000). INT32_MAX = latest.
    uint32_t cflags;           // Bitmask of active cflags. See below for semantics.
} SyntaqliteDialectConfig;

// Default config: latest version, no cflags.
#define SYNQ_DIALECT_CONFIG_DEFAULT { INT32_MAX, 0 }
```

**Sentinel value**: `INT32_MAX` means "latest version". This is the natural choice because `SYNQ_VER_LT(config, ver)` simplifies to a plain integer comparison with no special-case needed: `INT32_MAX` is always `>= ver` for any real version number.

**Config ownership**: parser and tokenizer store the config **by value** (not a pointer). The `set_dialect_config` API copies the caller's struct. Default-initialized configs use `SYNQ_DIALECT_CONFIG_DEFAULT`. The config is always valid and non-null, which eliminates null checks.

The `cflags` field is a bitmask representing the state of `SQLITE_OMIT_*` and `SQLITE_ENABLE_*` flags. Each flag maps to a named constant defined in the runtime: these are fixed SQLite concepts shared across all dialects:

```c
// syntaqlite-runtime/include/syntaqlite/sqlite_cflags.h
#define SYNQ_SQLITE_OMIT_WINDOWFUNC                0x00000001
#define SYNQ_SQLITE_OMIT_RETURNING                 0x00000002
#define SYNQ_SQLITE_OMIT_CTE                       0x00000004
#define SYNQ_SQLITE_OMIT_SUBQUERY                  0x00000008
#define SYNQ_SQLITE_ENABLE_ORDERED_SET_AGGREGATES   0x00000010
// ... etc
```

**Polarity handling**: `SQLITE_OMIT_*` and `SQLITE_ENABLE_*` flags have opposite default states. The codegen handles this by encoding the polarity into the keyword mask table. Each keyword entry stores both a cflag bit and a polarity flag:

- `SQLITE_OMIT_*`: the keyword is active by **default**. Setting the bit in `cflags` **disables** it.
- `SQLITE_ENABLE_*`: the keyword is inactive by **default**. Setting the bit in `cflags` **enables** it.

The keyword lookup encodes both: `aKWCFlag[i]` stores the bit, `aKWCFlagPolarity[i]` stores whether setting the bit enables or disables the keyword. See section 4.4 for the check logic.

Users configure in terms of the `SQLITE_OMIT_*` / `SQLITE_ENABLE_*` names they know:

```c
SyntaqliteDialectConfig config = {
    .sqlite_version = 3035000,
    .cflags = SYNQ_SQLITE_OMIT_WINDOWFUNC
            | SYNQ_SQLITE_OMIT_CTE
            | SYNQ_SQLITE_ENABLE_ORDERED_SET_AGGREGATES,
};
```

A default config (`SYNQ_DIALECT_CONFIG_DEFAULT`) means "latest version (`INT32_MAX`), default cflags (all OMIT off, all ENABLE off)": identical to current behavior.

### 4.2 Function signature changes

`GetToken` and `keywordCode` gain a `const SyntaqliteDialectConfig *` parameter:

```c
// Was:
i64 SynqSqliteGetToken(const unsigned char *z, int *tokenType);

// Now:
i64 SynqSqliteGetToken(const SyntaqliteDialectConfig *config,
                        const unsigned char *z, int *tokenType);
```

### 4.3 `GetToken` postlude (COMPLETED)

The extracted `sqlite3GetToken` function is renamed to `SynqSqliteGetToken_base` (static, internal linkage). A public wrapper function calls `_base` and applies the version-dependent postlude:

```c
i64 SynqSqliteGetToken(const SyntaqliteDialectConfig* config,
                        const unsigned char *z, int *tokenType) {
  i64 len = SynqSqliteGetToken_base(config, z, tokenType);
  /* Version-dependent token reclassification. */
  if( SYNQ_VER_LT(config, 3038000) && *tokenType==SYNTAQLITE_TK_PTR ){
    /* -> and ->> operators added in 3.38.
    ** Return just the '-' as TK_MINUS; next call picks up '>' naturally. */
    *tokenType = SYNTAQLITE_TK_MINUS;
    return 1;
  }
  if( SYNQ_VER_LT(config, 3046000) && *tokenType==SYNTAQLITE_TK_QNUMBER ){
    /* Digit separators added in 3.46.
    ** Truncate to the first underscore. */
    i64 j;
    int saw_dot = 0;
    for(j=0; j<len; j++){
      if( z[j]=='_' ) break;
      if( z[j]=='.' ) saw_dot = 1;
    }
    *tokenType = saw_dot ? SYNTAQLITE_TK_FLOAT : SYNTAQLITE_TK_INTEGER;
    return j;
  }
  return len;
}
```

**Compile-time / runtime gating macros** (`dialect_config.h`):

```c
// When SYNQ_SQLITE_VERSION is defined, expands to a compile-time constant.
// When not defined, checks through the runtime config pointer.
#ifdef SYNQ_SQLITE_VERSION
  #define SYNQ_VER_LT(config, ver) (SYNQ_SQLITE_VERSION < (ver))
#else
  #define SYNQ_VER_LT(config, ver) ((config)->sqlite_version < (ver))
#endif

#ifdef SYNQ_SQLITE_CFLAGS
  #define SYNQ_HAS_CFLAG(config, flag) ((SYNQ_SQLITE_CFLAGS) & (flag))
#else
  #define SYNQ_HAS_CFLAG(config, flag) ((config)->cflags & (flag))
#endif
```

When `sqlite_version` is `INT32_MAX` (latest), `SYNQ_VER_LT` is always false and the postlude is a single branch-not-taken. When compiled with `-DSYNQ_SQLITE_VERSION=INT32_MAX`, the compiler constant-folds and eliminates dead branches entirely.

The wrapper function pattern avoids modifying the extracted SQLite tokenizer code (which has many return points) and keeps the reclassification logic isolated in generated codegen output (`sqlite_runtime_codegen.rs::generate_get_token_wrapper()`).

### 4.4 Keyword version + cflag gating (COMPLETED)

The keyword table (generated by `mkkeywordhash`) has parallel arrays for version and cflag gating:

```c
// Introduction version for each keyword (0 = always present).
static const int32_t synq_sqlite_aKWSince[148] = {
    0,          /* ABORT */
    0,          /* ACTION */
    ...
    3035000,    /* RETURNING */
    3047000,    /* WITHIN */
};

// CFlag bit for each keyword (0 = no cflag dependency).
static const uint32_t synq_sqlite_aKWCFlag[148] = {
    0,                                          /* ABORT */
    ...
    SYNQ_SQLITE_OMIT_RETURNING,                 /* RETURNING */
    SYNQ_SQLITE_OMIT_WINDOWFUNC,                /* WINDOW */
    SYNQ_SQLITE_ENABLE_ORDERED_SET_AGGREGATES,  /* WITHIN */
    ...
};

// Polarity: 0 = OMIT (keyword active by default, bit disables it),
//           1 = ENABLE (keyword inactive by default, bit enables it).
static const uint8_t synq_sqlite_aKWCFlagPolarity[148] = {
    0,  /* ABORT */
    ...
    0,  /* RETURNING: OMIT flag */
    0,  /* WINDOW: OMIT flag */
    1,  /* WITHIN: ENABLE flag */
    ...
};
```

The keyword lookup function checks both version and cflags, respecting polarity:

```c
int SynqSqliteKeywordCode(const SyntaqliteDialectConfig *config,
                           const char *z, int n, int *pType) {
    ...
    // Version check: skip keywords newer than target version.
    if (SYNQ_VER_LT(config, synq_sqlite_aKWSince[i])) {
      break;  // not a keyword, fall through to TK_ID
    }
    // CFlag check with polarity:
    //   OMIT (polarity=0): keyword disabled when bit IS set
    //   ENABLE (polarity=1): keyword disabled when bit IS NOT set
    if (synq_sqlite_aKWCFlag[i] != 0) {
      int bit_set = SYNQ_HAS_CFLAG(config, synq_sqlite_aKWCFlag[i]) != 0;
      int is_enable = synq_sqlite_aKWCFlagPolarity[i];
      if (bit_set != is_enable) {
        break;  // cflag disables this keyword, fall through to TK_ID
      }
    }
    *pType = synq_sqlite_aKWCode[i];
    break;
}
```

The polarity logic: for OMIT flags (`polarity=0`), the keyword is disabled when the bit is set (`bit_set=1, is_enable=0, 1 != 0 → skip`). For ENABLE flags (`polarity=1`), the keyword is disabled when the bit is NOT set (`bit_set=0, is_enable=1, 0 != 1 → skip`).

Note: version and cflag checks use the `SYNQ_VER_LT` and `SYNQ_HAS_CFLAG` macros, which resolve to compile-time constants when `SYNQ_SQLITE_VERSION` / `SYNQ_SQLITE_CFLAGS` are defined.

Most keywords have `since = 0` and `cflag = 0`, so both checks are almost always false.

### 4.5 Keyword `since` data

From the phase 1 analysis:

```
since 0:        All keywords present in 3.12.2 (the baseline)
since 3024000:  DO, NOTHING
since 3025000:  CURRENT, FILTER, FOLLOWING, OVER, PARTITION,
                PRECEDING, RANGE, ROWS, UNBOUNDED, WINDOW
since 3028000:  EXCLUDE, GROUPS, OTHERS, TIES
since 3030000:  FIRST, LAST, NULLS
since 3031000:  ALWAYS, GENERATED
since 3035000:  MATERIALIZED, RETURNING
since 3047000:  WITHIN
```

### 4.6 Keyword-to-cflag mapping

Each `SQLITE_OMIT_*` flag suppresses specific keywords:

| CFlag                                  | Keywords suppressed                                                                                                   |
| -------------------------------------- | --------------------------------------------------------------------------------------------------------------------- |
| `SQLITE_OMIT_WINDOWFUNC`               | WINDOW, OVER, PARTITION, CURRENT, FOLLOWING, PRECEDING, RANGE, ROWS, UNBOUNDED, FILTER, EXCLUDE, GROUPS, OTHERS, TIES |
| `SQLITE_OMIT_CTE`                      | WITH                                                                                                                  |
| `SQLITE_OMIT_COMPOUND_SELECT`          | UNION, INTERSECT, EXCEPT                                                                                              |
| `SQLITE_OMIT_GENERATED_COLUMNS`        | GENERATED, ALWAYS                                                                                                     |
| `SQLITE_OMIT_RETURNING`                | RETURNING                                                                                                             |
| `SQLITE_OMIT_CAST`                     | CAST                                                                                                                  |
| `SQLITE_OMIT_EXPLAIN`                  | EXPLAIN                                                                                                               |
| `SQLITE_OMIT_VIEW`                     | VIEW                                                                                                                  |
| `SQLITE_OMIT_TRIGGER`                  | TRIGGER                                                                                                               |
| `SQLITE_OMIT_ATTACH`                   | ATTACH, DETACH                                                                                                        |
| `SQLITE_OMIT_PRAGMA`                   | PRAGMA                                                                                                                |
| `SQLITE_OMIT_REINDEX`                  | REINDEX                                                                                                               |
| `SQLITE_OMIT_ANALYZE`                  | ANALYZE                                                                                                               |
| `SQLITE_OMIT_ALTERTABLE`               | ALTER                                                                                                                 |
| `SQLITE_OMIT_VIRTUALTABLE`             | VIRTUAL                                                                                                               |
| `SQLITE_OMIT_TEMPDB`                   | TEMP                                                                                                                  |
| `SQLITE_ENABLE_ORDERED_SET_AGGREGATES` | WITHIN (note: enable flag, not omit)                                                                                  |

This mapping is already encoded in `mkkeywordhash.c`'s mask table. The codegen pipeline extracts it.

### 4.7 `SQLITE_OMIT_SUBQUERY`: the special case

This is the only cflag that affects grammar rules without being keyword-gated. Subqueries use `(SELECT ...)`, which consists entirely of baseline tokens that can't be suppressed.

**Solution**: set a flag on the parse context when a subquery production fires.

The `%ifndef SQLITE_OMIT_SUBQUERY` blocks in `parse.y` guard three groups of rules (subqueries in FROM, IN, and expression position). Since the grammar is **not autogenerated** (it's upstream SQLite with manual modifications), we add the flag directly to the grammar action code by hand:

```c
// Added to subquery grammar actions in parse.y:
pParse->sawSubquery = 1;
```

After parsing completes, the caller checks:

```c
if ((config->cflags & SYNQ_SQLITE_OMIT_SUBQUERY) && pParse->sawSubquery) {
    // emit diagnostic: "subqueries not available with SQLITE_OMIT_SUBQUERY"
}
```

This is an O(1) check and does not require an AST walk.

### 4.8 Dispatch macro update (COMPLETED)

The `SYNQ_GET_TOKEN` macro gains the config parameter:

```c
// syntaqlite/csrc/sqlite_dialect_dispatch.h (inline/amalgamation mode):
#define SYNQ_GET_TOKEN(d, cfg, z, t)  SynqSqliteGetToken(cfg, z, t)

// syntaqlite-runtime/csrc/dialect_dispatch.h (function pointer mode):
#define SYNQ_GET_TOKEN(d, cfg, z, t)  (d)->get_token(cfg, z, t)
```

The `get_token` function pointer signature in `SyntaqliteDialect` updates to:

```c
int64_t (*get_token)(const SyntaqliteDialectConfig *config,
                     const unsigned char *z, int *tokenType);
```

The config struct is defined in the runtime and shared by all dialects.

### 4.9 Version/config lives on the parser (COMPLETED)

The `SyntaqliteDialect` struct is static and immutable. The config is per-parser/tokenizer session, stored **by value**:

```c
// parser.c / tokenizer.c struct:
SyntaqliteDialectConfig dialect_config;  // By value, not pointer.

// Create initializes with default:
SyntaqliteDialectConfig default_config = SYNQ_DIALECT_CONFIG_DEFAULT;
p->dialect_config = default_config;

// API copies the caller's config:
int syntaqlite_parser_set_dialect_config(SyntaqliteParser *p,
                                         const SyntaqliteDialectConfig *config) {
    if (p->sealed) return -1;
    p->dialect_config = *config;  // Copy by value.
    return 0;
}
```

Usage:

```c
SyntaqliteDialectConfig config = {
    .sqlite_version = 3035000,
    .cflags = SYNQ_SQLITE_OMIT_WINDOWFUNC,
};
syntaqlite_parser_set_dialect_config(parser, &config);
```

The tokenizer has a matching API: `syntaqlite_tokenizer_set_dialect_config()`.

### 4.10 Compile-time mode (COMPLETED)

Compile-time gating is handled via macros in `dialect_config.h` (see section 4.3). When `-DSYNQ_SQLITE_VERSION=X` is defined, `SYNQ_VER_LT` and `SYNQ_HAS_CFLAG` expand to compile-time constants. The compiler constant-folds all comparisons and eliminates dead branches.

No static config struct or dispatch-level `#ifdef` is needed: the macros operate directly at the point of use (in the GetToken postlude and keyword lookup), which gives the compiler maximum visibility for optimization.

### 4.11 Rust-side changes (COMPLETED)

```rust
// In syntaqlite-runtime/src/dialect/ffi.rs: mirrors the C struct:
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct DialectConfig {
    pub sqlite_version: i32,    // i32::MAX = latest
    pub cflags: u32,            // 0 = default cflags
}

impl Default for DialectConfig {
    fn default() -> Self {
        Self { sqlite_version: i32::MAX, cflags: 0 }
    }
}

// Parser and Tokenizer store config by value (not Option):
pub struct Parser {
    dialect_config: DialectConfig,  // Default: DialectConfig::default()
    ...
}

impl Parser {
    pub fn set_dialect_config(&mut self, config: &DialectConfig) {
        self.dialect_config = *config;
        unsafe { ffi::syntaqlite_parser_set_dialect_config(self.raw, &self.dialect_config); }
    }
}
```

### 4.12 Code ownership: everything in `syntaqlite/` stays generated

A key invariant: **`syntaqlite/csrc/` is 100% generated code** (`@generated` marker). All version/cflag logic: the `GetToken` postlude, the `aKWSince[]`/`aKWCFlag[]`/`aKWCFlagPolarity[]` arrays, the keyword version/cflag checks: is emitted by the codegen pipeline. No hand-written C files in the dialect crate. The config struct and cflag constants live in the runtime (hand-written, shared across all dialects).

The codegen pipeline already transforms extracted code (symbol renaming via `c_transformer`, fragment assembly). Injecting the postlude into `GetToken` and emitting version/cflag data alongside the keyword table is the same kind of transformation. The "hand-written" decisions (which tokens to reclassify, version thresholds, cflag mappings) live as logic in `syntaqlite-codegen/src/`, not as C source files.

This keeps the scope to two `if` checks in the postlude and two data arrays for keywords. If the version-gating logic grows significantly, the crate structure can be revisited. For the current scope, code generation is the better trade-off.

### 4.13 Codegen changes

Generated output changes (all emitted by the codegen pipeline):

| File                        | Change                                                                                                   | Status    |
| --------------------------- | -------------------------------------------------------------------------------------------------------- | --------- |
| `sqlite_tokenize.c`         | `SynqSqliteGetToken_base` (static) + public wrapper with config param + postlude                         | COMPLETED |
| `sqlite_tokenize.h`         | `SynqSqliteGetToken` declaration gains `const SyntaqliteDialectConfig *` + `#include "dialect_config.h"` | COMPLETED |
| `sqlite_keyword.c`          | `aKWSince[]` + `aKWCFlag[]` + `aKWCFlagPolarity[]` arrays + checks in lookup                             | COMPLETED |
| `sqlite_dialect_dispatch.h` | `SYNQ_GET_TOKEN` macro gains config param                                                                | COMPLETED |
| `dialect.c`                 | Function pointer assignment (signature update)                                                           | COMPLETED |

Codegen pipeline changes (where the logic lives):

| Module                         | Change                                                                                                    | Status    |
| ------------------------------ | --------------------------------------------------------------------------------------------------------- | --------- |
| `sqlite_runtime_codegen.rs`    | Tokenizer extraction: `_base` rename, config param injection, `generate_get_token_wrapper()` postlude fn  | COMPLETED |
| `tools/mkkeyword.rs`           | Keyword generation: emit `aKWSince[]` + `aKWCFlag[]` + `aKWCFlagPolarity[]` arrays + version/cflag checks | COMPLETED |
| `dialect_codegen/c_dialect.rs` | Dispatch macro generation: add config param. Tokenize.h: add config param + include.                      | COMPLETED |

Runtime changes (hand-written, dialect-agnostic):

| File                                                     | Change                                                                                                                  | Status    |
| -------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- | --------- |
| `syntaqlite-runtime/include/syntaqlite/dialect_config.h` | NEW: `SyntaqliteDialectConfig` struct + `SYNQ_DIALECT_CONFIG_DEFAULT` + gating macros (`SYNQ_VER_LT`, `SYNQ_HAS_CFLAG`) | COMPLETED |
| `syntaqlite-runtime/include/syntaqlite/sqlite_cflags.h`  | NEW: `SYNQ_SQLITE_OMIT_*` / `SYNQ_SQLITE_ENABLE_*` constants                                                            | COMPLETED |
| `syntaqlite-runtime/include/syntaqlite/dialect.h`        | `get_token` function pointer signature                                                                                  | COMPLETED |
| `syntaqlite-runtime/include/syntaqlite/parser.h`         | `syntaqlite_parser_set_dialect_config()` API + include                                                                  | COMPLETED |
| `syntaqlite-runtime/include/syntaqlite/tokenizer.h`      | `syntaqlite_tokenizer_set_dialect_config()` API + include                                                               | COMPLETED |
| `syntaqlite-runtime/csrc/parser.c`                       | Store config by value, init with default, pass `&p->dialect_config` to `SYNQ_GET_TOKEN`                                 | COMPLETED |
| `syntaqlite-runtime/csrc/tokenizer.c`                    | Store config by value, init with default, pass `&tok->dialect_config` to `SYNQ_GET_TOKEN`                               | COMPLETED |
| `syntaqlite-runtime/csrc/dialect_dispatch.h`             | `SYNQ_GET_TOKEN` macro gains config param                                                                               | COMPLETED |
| `syntaqlite-runtime/src/dialect/ffi.rs`                  | `DialectConfig` struct + `Default` impl (`i32::MAX`, `0`)                                                               | COMPLETED |
| `syntaqlite-runtime/src/parser/ffi.rs`                   | FFI declarations for `set_dialect_config` (parser + tokenizer)                                                          | COMPLETED |
| `syntaqlite-runtime/src/parser/parser.rs`                | `dialect_config: DialectConfig` field + `set_dialect_config()` method                                                   | COMPLETED |
| `syntaqlite-runtime/src/parser/tokenizer.rs`             | `dialect_config: DialectConfig` field + `set_dialect_config()` method                                                   | COMPLETED |

---

## 5. Phase 3: Oracle Tests (COMPLETED)

### 5.1 Purpose

Verify that syntaqlite's version/cflag-gated tokenizer and keyword behavior matches real SQLite at key version boundaries.

### 5.2 Real SQLite verification (COMPLETED)

All expected behavior was verified against actual `sqlite3` shells compiled from official SQLite amalgamation downloads. The following versions were compiled and tested:

| Version | Purpose                                     | Key verification                                     |
| ------- | ------------------------------------------- | ---------------------------------------------------- |
| 3.23.1  | DO keyword boundary (before)                | `ON CONFLICT(x) DO NOTHING` → error near "ON"       |
| 3.24.0  | DO keyword boundary (after) / Window before | `ON CONFLICT DO` → success; `OVER ()` → error       |
| 3.25.0  | Window keyword boundary (after)             | `SELECT sum(1) OVER ()` → success                   |
| 3.34.1  | RETURNING/MATERIALIZED boundary (before)    | `INSERT ... RETURNING *` → error near "RETURNING"    |
| 3.35.0  | RETURNING/MATERIALIZED boundary (after)     | `INSERT ... RETURNING *` → success                   |
| 3.37.2  | TK_PTR boundary (before)                    | `SELECT '{"a":1}' -> '$.a'` → error near ">"        |
| 3.38.0  | TK_PTR boundary (after)                     | `SELECT '{"a":1}' -> '$.a'` → success               |
| 3.45.3  | TK_QNUMBER boundary (before)               | `SELECT 1_000` → "unrecognized token: 1_000"         |
| 3.46.0  | TK_QNUMBER boundary (after)                | `SELECT 1_000` → 1000                                |
| 3.47.0  | WITHIN keyword (cflag-gated)                | WITHIN in mkkeywordhash, excluded from standard builds |

Amalgamations downloaded from `https://www.sqlite.org/{year}/sqlite-amalgamation-{ver}.zip` and compiled with `cc -O2 -DSQLITE_THREADSAFE=0`. A 3.47.0 variant was also compiled with `-DSQLITE_ENABLE_ORDERED_SET_AGGREGATES`.

### 5.3 Integration tests (COMPLETED: 41 tests)

Test file: `syntaqlite/tests/multiversion.rs`

Tests use the runtime `Tokenizer` and `Parser` APIs directly (with `set_dialect_config`) and the `TokenType` enum (via `#[doc(hidden)] pub fn from_raw`).

**Token reclassification tests (11)**:

| Test                                                  | What it verifies                                       |
| ----------------------------------------------------- | ------------------------------------------------------ |
| `ptr_operator_tokenizes_as_ptr_on_latest`             | `1->2` → INTEGER, PTR, INTEGER at latest               |
| `ptr_operator_reclassified_to_minus_before_3_38`      | `1->2` → INTEGER, MINUS("-"), GT, INTEGER at 3.37      |
| `ptr_operator_works_at_3_38`                          | `1->2` → INTEGER, PTR, INTEGER at 3.38                 |
| `double_ptr_reclassified_before_3_38`                 | `1->>2` → INTEGER, MINUS("-"), RSHIFT, INTEGER at 3.37 |
| `ptr_reclassification_parse_fails_before_3_38`        | `SELECT 1->2;` fails to parse at 3.37                  |
| `ptr_reclassification_parse_succeeds_at_3_38`         | `SELECT 1->2;` parses at 3.38                          |
| `digit_separator_tokenizes_as_qnumber_on_latest`      | `1_000` → QNUMBER at latest                            |
| `digit_separator_reclassified_to_integer_before_3_46` | `1_000` → INTEGER("1") at 3.45                         |
| `digit_separator_float_reclassified_before_3_46`      | `1.5_0` → FLOAT("1.5") at 3.45                         |
| `digit_separator_works_at_3_46`                       | `1_000` → QNUMBER at 3.46                              |
| `basic_tokens_unaffected_by_version`                  | `SELECT 1 + 2` stable across 3.12, 3.37, 3.46, latest  |

**Keyword version gating tests (7)**:

| Test                                              | What it verifies                                        |
| ------------------------------------------------- | ------------------------------------------------------- |
| `returning_keyword_not_recognized_before_3_35`    | `RETURNING` does not tokenize as TK_RETURNING at 3.34   |
| `returning_keyword_recognized_at_3_35`            | `RETURNING` tokenizes as TK_RETURNING at 3.35           |
| `materialized_keyword_not_recognized_before_3_35` | `MATERIALIZED` is not TK_MATERIALIZED at 3.34           |
| `window_keyword_not_recognized_before_3_25`       | `WINDOW` is not TK_WINDOW at 3.24                       |
| `over_keyword_not_recognized_before_3_25`         | `OVER` is not TK_OVER at 3.24                           |
| `do_keyword_not_recognized_before_3_24`           | `DO` is not TK_DO at 3.23                               |
| `filter_keyword_not_recognized_before_3_25`       | `FILTER` is not TK_FILTER at 3.24                       |

**Keyword cflag gating tests (5: WITHIN / ENABLE_ORDERED_SET_AGGREGATES)**:

| Test                                        | What it verifies                                                |
| ------------------------------------------- | --------------------------------------------------------------- |
| `within_keyword_not_recognized_without_cflag` | WITHIN is not TK_WITHIN without ENABLE flag                    |
| `within_keyword_recognized_with_cflag`       | WITHIN is TK_WITHIN with ENABLE flag set                        |
| `within_keyword_not_recognized_before_3_47`  | WITHIN not recognized before 3.47 even with cflag               |
| `within_group_parses_with_cflag`             | `WITHIN GROUP (ORDER BY ...)` parses with cflag                 |
| `within_group_fails_without_cflag`           | `WITHIN GROUP (ORDER BY ...)` fails without cflag               |

### 5.4 OMIT cflag oracle tests (COMPLETED: 12 tests)

Tests verify that OMIT-polarity cflags suppress keywords at the tokenizer level, causing parse failures for gated syntax. Two tests per cflag (keyword fallback + parse failure):

| CFlag                    | Keyword   | Test SQL                                                      |
| ------------------------ | --------- | ------------------------------------------------------------- |
| `OMIT_WINDOWFUNC` (0x08) | `OVER`    | `SELECT sum(x) OVER () FROM t;`                               |
| `OMIT_CTE` (0x40)       | `WITH`    | `WITH t AS (SELECT 1) SELECT * FROM t;`                       |
| `OMIT_RETURNING` (0x10000) | `RETURNING` | `INSERT INTO t VALUES(1) RETURNING *;`                    |
| `OMIT_COMPOUND_SELECT` (0x04) | `UNION` | `SELECT 1 UNION SELECT 2;`                                |
| `OMIT_VIEW` (0x20)      | `VIEW`    | `CREATE VIEW v AS SELECT 1;`                                  |
| `OMIT_TRIGGER` (0x400)  | `TRIGGER` | `CREATE TRIGGER t AFTER INSERT ON x BEGIN SELECT 1; END;`    |

### 5.5 Subquery flag tests (COMPLETED: 6 tests)

Tests verify the `saw_subquery` flag on `StatementCursor`, which is set by grammar actions when a subquery production is reduced:

| Test                                  | SQL                                      | Expected `saw_subquery` |
| ------------------------------------- | ---------------------------------------- | ----------------------- |
| `subquery_detected_in_from`           | `SELECT * FROM (SELECT 1);`              | true                    |
| `subquery_detected_in_exists`         | `SELECT EXISTS (SELECT 1);`              | true                    |
| `subquery_detected_in_scalar_subquery`| `SELECT (SELECT 1);`                     | true                    |
| `subquery_detected_in_in_select`      | `SELECT 1 WHERE 1 IN (SELECT 2);`       | true                    |
| `no_subquery_in_simple_select`        | `SELECT 1;`                              | false                   |
| `no_subquery_in_in_list`              | `SELECT 1 WHERE 1 IN (1, 2, 3);`        | false                   |

### 5.6 Future: JSON oracle generation tooling

For comprehensive coverage, a JSON-based oracle approach may be added later:

```
tools/generate-oracle-data    # downloads amalgamations, compiles, runs
tools/oracle_tokenizer.c      # small C program that tokenizes test corpus
```

The current integration tests in `multiversion.rs` provide focused coverage of all version boundaries with behavior verified against real sqlite3 shells.

---

## 6. Implementation Order

1. **Config struct + signature changes**: COMPLETED. Defined `SyntaqliteDialectConfig` (with `INT32_MAX` sentinel for "latest"), updated `GetToken` signature, wired through dispatch macros, parser, tokenizer. Config stored by value with `SYNQ_DIALECT_CONFIG_DEFAULT`. Compile-time gating macros (`SYNQ_VER_LT`, `SYNQ_HAS_CFLAG`) in `dialect_config.h`.

2. **`GetToken` postlude**: COMPLETED. Wrapper function pattern: extracted code renamed to `_base` (static), public wrapper applies TK_PTR and TK_QNUMBER reclassification using `SYNQ_VER_LT` macros. Generated by `sqlite_runtime_codegen.rs::generate_get_token_wrapper()`.

3. **Oracle tests (tokenizer level)**: COMPLETED. `syntaqlite/tests/multiversion.rs` with 41 passing tests covering token reclassification (11), keyword version gating (7), keyword cflag gating (5), OMIT cflag oracle tests (12), and subquery flag tests (6). All behavior verified against real sqlite3 shells compiled from 10 official amalgamations (3.23.1–3.47.0).

4. **Keyword `since` + cflag arrays**: COMPLETED. `aKWSince[]`, `aKWCFlag[]`, and `aKWCFlagPolarity[]` emitted by the codegen pipeline. Version + cflag checks in keyword lookup using `SYNQ_VER_LT` and `SYNQ_HAS_CFLAG` macros. Cflag mapping auto-derived from embedded `mkkeywordhash.c` source (no hardcoded cflag map). All 7 keyword version gating tests + 5 WITHIN cflag tests pass.

5. **WITHIN GROUP (ordered-set aggregates)**: COMPLETED. Full AST support: `OrderedSetFunctionCall` node in `aggregate.synq`, grammar rules in `aggregate.y`, WITHIN keyword gated by `SQLITE_ENABLE_ORDERED_SET_AGGREGATES` cflag with ENABLE polarity. Grammar parser fixed to include `%ifdef SQLITE_ENABLE_*` blocks.

6. **Subquery flag**: COMPLETED. Added `saw_subquery` field to `SynqParseCtx` (C parse context) and `SyntaqliteParseResult`. Grammar actions in `compound.y` (3 rules: scalar subquery, EXISTS, IN-with-subquery) and `table_source.y` (1 rule: subquery table source) set `pCtx->saw_subquery = 1`. Reset per-statement in both high-level and feed-token APIs. Wired through Rust FFI (`ParseResult.saw_subquery`) and exposed as `StatementCursor::saw_subquery()` getter.

7. **OMIT cflag oracle tests**: COMPLETED. Added 18 tests to `syntaqlite/tests/multiversion.rs`: 12 OMIT-polarity keyword suppression tests (2 per cflag for WINDOWFUNC, CTE, RETURNING, COMPOUND_SELECT, VIEW, TRIGGER: keyword fallback + parse failure), and 6 `saw_subquery` flag tests (detected in FROM subquery, EXISTS, scalar subquery, IN-with-SELECT; not detected in simple SELECT or IN-with-literal-list). Total multiversion tests: 41.

---

## 7. Workflow: Adding a New SQLite Version

```
1. Download new version source

2. Run: syntaqlite analyze-versions --sqlite-source-dir ./sqlite-sources/
   Compare keyword list and tokenizer fragments.

3. If new keywords: add entries to the `since` data in codegen, re-run codegen.

4. If tokenizer behavioral change (new token type): add reclassification
   case to the GetToken postlude.

5. Generate oracle test data for the new version.

6. Run tests, verify, commit.
```

---

## 8. Key Design Decisions

| Decision                     | Choice                              | Rationale                                                                                                   |
| ---------------------------- | ----------------------------------- | ----------------------------------------------------------------------------------------------------------- |
| Single tokenizer vs variants | Single + postlude                   | Only 2 behavioral differences (TK_PTR, TK_QNUMBER). Postlude is simpler than managing 10 function variants. |
| Config location              | Parser, not dialect                 | Dialect is static const. Version/cflags are per-session configuration.                                      |
| Config representation        | Struct pointer                      | Extensible, compiler can see through `const` pointer to static const for optimization.                      |
| CFlag naming                 | `SQLITE_OMIT_*` names               | Users reason in SQLite terms. Direct mapping from mkkeywordhash masks.                                      |
| Grammar cflag enforcement    | Keyword suppression + subquery flag | 16/17 cflags are keyword-gated. Only SQLITE_OMIT_SUBQUERY needs special handling (O(1) flag check).         |
| TK_COMMENT handling          | Always emit                         | Syntaqlite depends on comment/space distinction. Deliberate divergence from pre-3.49 SQLite.                |
| Backwards compat tracking    | Not needed                          | SQLite is purely additive. Every check is `>= threshold`.                                                   |

---

## 9. Cost Summary

| Component                 | Runtime cost (version < latest)    | Runtime cost (latest / INT32_MAX) | Compile-time cost (`-DSYNQ_SQLITE_VERSION`) |
| ------------------------- | ---------------------------------- | --------------------------------- | ------------------------------------------- |
| Keyword version check     | 1 int comparison per keyword       | 1 int comparison (always false)   | 0 (dead code eliminated)                    |
| Keyword cflag check       | 1 bitmask check per keyword        | 1 bitmask check (always false)    | 0 (dead code eliminated)                    |
| GetToken postlude         | 2 `if` checks (TK_PTR, TK_QNUMBER) | 2 `if` checks (always false)      | 0 (dead code eliminated)                    |
| Subquery flag             | 1 assignment per subquery          | 1 assignment per subquery         | 0 if subquery not applicable                |
| Post-parse subquery check | 1 bitmask + flag check             | 0 (cflags=0)                      | 0 (dead code eliminated)                    |
| **Total**                 | **Negligible**                     | **Negligible**                    | **Zero**                                    |

---

## 10. Reference: Current Extraction Pipeline

For context, this is how extraction currently works:

**Build time** (`syntaqlite-codegen/build.rs`):

- Sets `SYNTAQLITE_SQLITE_TOKENIZE_C` env var pointing to `third_party/src/sqlite/src/tokenize.c`

**Codegen time** (`syntaqlite-codegen/src/lib.rs`):

- `embedded_sqlite_tokenize_c()` -> `include_str!(env!("SYNTAQLITE_SQLITE_TOKENIZE_C"))`
- `embedded_sqlite_global_c()` -> `include_str!("/../third_party/src/sqlite/src/global.c")`
- `embedded_sqlite_sqliteint_h()` -> `include_str!("/../third_party/src/sqlite/src/sqliteInt.h")`
- These are passed to `extract_tokenizer()` which runs `c_extractor` on each, then `c_transformer` to rename symbols.

**Keyword generation** (`sqlite_runtime_codegen.rs::generate_keyword_hash()`):

- Spawns `mkkeyword` subprocess (compiled from `syntaqlite-codegen/sqlite/mkkeywordhash.c`)
- Accepts optional `--extra-file` for dialect extension keywords
- Output processed by `c_transformer` to rename symbols

**Source files consumed:**

- `third_party/src/sqlite/src/tokenize.c`: GetToken, aiClass, CC\_\*, IdChar, charMap
- `third_party/src/sqlite/src/global.c`: CtypeMap, UpperToLower
- `third_party/src/sqlite/src/sqliteInt.h`: Isspace, Isdigit, Isxdigit macros
- `syntaqlite-codegen/sqlite/mkkeywordhash.c`: keyword table + masks
- `third_party/src/sqlite/src/parse.y`: referenced in build.rs for grammar

**c_extractor API** (`syntaqlite-codegen/src/c_source/c_extractor.rs`):

- `CExtractor::new(content: &str)`: wraps source text
- `.extract_function(name)` -> `CFunction { text }`
- `.extract_static_array(name)` -> `CStaticArray { text }`
- `.extract_specific_defines(names)` -> `CDefines { text }`
- `.extract_defines_with_ifdef_context(names)` -> `CDefines { text }`
