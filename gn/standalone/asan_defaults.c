/*
 * Default ASAN/LSan options compiled into sanitizer-instrumented binaries.
 *
 * These weak-symbol functions are called by the ASAN runtime at startup.
 * Embedding them here means no runtime environment variables are needed.
 */

const char *__asan_default_options(void) {
  return "detect_leaks=1";
}

const char *__lsan_default_suppressions(void) {
  return
      /* macOS system library allocations (harmless no-ops on Linux) */
      "leak:libobjc\n"
      "leak:libxpc\n"
      "leak:dyld\n"
      "leak:libSystem\n";
}
