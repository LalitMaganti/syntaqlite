// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Dynamic dialect loading via dlopen (POSIX) or LoadLibrary (Windows).
//
// Lifecycle:
//   SyntaqliteLoadedDialect* ld = syntaqlite_dialect_load("perfetto.so", "perfetto");
//   SyntaqliteDialect d = syntaqlite_loaded_dialect_get(ld);
//   // use d with parser/formatter/validator
//   syntaqlite_loaded_dialect_destroy(ld);

#include "syntaqlite/dialect.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
typedef HMODULE synq_lib_handle_t;
#define SYNQ_DLOPEN(p) LoadLibraryA(p)
#define SYNQ_DLSYM(l, s) (void *)GetProcAddress(l, s)
#define SYNQ_DLCLOSE(l) FreeLibrary(l)
#define SYNQ_DLERROR() "LoadLibrary failed"
#else
#include <dlfcn.h>
typedef void *synq_lib_handle_t;
#define SYNQ_DLOPEN(p) dlopen(p, RTLD_NOW)
#define SYNQ_DLSYM(l, s) dlsym(l, s)
#define SYNQ_DLCLOSE(l) dlclose(l)
#define SYNQ_DLERROR() dlerror()
#endif

// ---------------------------------------------------------------------------
// Opaque handle
// ---------------------------------------------------------------------------

struct SyntaqliteLoadedDialect {
  SyntaqliteDialect dialect;
  synq_lib_handle_t lib;
  char error[512];
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

SYNTAQLITE_API SyntaqliteLoadedDialect *
syntaqlite_dialect_load(const char *path, const char *name) {
  SyntaqliteLoadedDialect *ld =
      (SyntaqliteLoadedDialect *)calloc(1, sizeof(SyntaqliteLoadedDialect));
  if (!ld)
    return NULL;

  ld->lib = SYNQ_DLOPEN(path);
  if (!ld->lib) {
    snprintf(ld->error, sizeof(ld->error), "failed to load '%s': %s", path,
             SYNQ_DLERROR());
    return ld;
  }

  // Build symbol name: syntaqlite_{name}_dialect or syntaqlite_dialect
  char sym[256];
  if (name)
    snprintf(sym, sizeof(sym), "syntaqlite_%s_dialect", name);
  else
    snprintf(sym, sizeof(sym), "syntaqlite_dialect");

  typedef SyntaqliteDialect (*dialect_fn_t)(void);
  dialect_fn_t fn = (dialect_fn_t)SYNQ_DLSYM(ld->lib, sym);
  if (!fn) {
    snprintf(ld->error, sizeof(ld->error), "symbol '%s' not found in '%s': %s",
             sym, path, SYNQ_DLERROR());
    SYNQ_DLCLOSE(ld->lib);
    ld->lib = 0;
    return ld;
  }

  ld->dialect = fn();
  return ld;
}

SYNTAQLITE_API const char *
syntaqlite_loaded_dialect_error(const SyntaqliteLoadedDialect *ld) {
  if (!ld || ld->error[0] == '\0')
    return NULL;
  return ld->error;
}

SYNTAQLITE_API SyntaqliteDialect
syntaqlite_loaded_dialect_get(const SyntaqliteLoadedDialect *ld) {
  return ld->dialect;
}

SYNTAQLITE_API void
syntaqlite_loaded_dialect_destroy(SyntaqliteLoadedDialect *ld) {
  if (!ld)
    return;
  if (ld->lib)
    SYNQ_DLCLOSE(ld->lib);
  free(ld);
}
