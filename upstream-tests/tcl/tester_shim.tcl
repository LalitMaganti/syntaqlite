# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

# Minimal replacement for SQLite's tester.tcl. Provides stubs for the
# test framework commands that upstream .test files expect, routing all
# SQL through our tclsyntaqlite extension (which runs dual-path
# comparison against real SQLite).
#
# Usage: set env(TCLLIBPATH) to include the extension directory, then
#   source this file before running a .test file.

# ---------------------------------------------------------------------------
# Global state
# ---------------------------------------------------------------------------

set testprefix ""
set sqlite_options(default_autovacuum) 0
set sqlite_options(default_page_size) 4096

# Default database handle.
if {![info exists ::dbhandle]} {
  set ::dbhandle "db"
}

# Create the default database.
if {[info commands db] eq ""} {
  sqlite3 db :memory:
}

# ---------------------------------------------------------------------------
# SQL execution procs
# ---------------------------------------------------------------------------

proc execsql {sql {db db}} {
  $db eval $sql
}

proc catchsql {sql {db db}} {
  if {[catch {$db eval $sql} msg]} {
    return [list 1 $msg]
  }
  return [list 0 {}]
}

proc execsql_pp {sql {db db}} {
  $db eval $sql
}

# ---------------------------------------------------------------------------
# Test harness procs
# ---------------------------------------------------------------------------

proc do_test {name cmd expected} {
  # Run the test body to exercise SQL, but don't check results.
  catch {uplevel 1 $cmd} result
}

proc do_execsql_test {name sql {expected {}}} {
  catch {execsql $sql}
}

proc do_catchsql_test {name sql expected} {
  catch {catchsql $sql}
}

proc do_eqp_test {name sql expected} {
  catch {execsql $sql}
}

proc do_timed_execsql_test {name sql {expected {}}} {
  catch {execsql $sql}
}

# ---------------------------------------------------------------------------
# ifcapable — capability checking
# ---------------------------------------------------------------------------

# Map of capability names. By default all capabilities are enabled.
# Tests use `ifcapable !fts3 { finish_test; return }` to skip.
proc ifcapable {expr code args} {
  # Replace capability names with 1 (all enabled).
  set e [regsub -all {[a-zA-Z_][a-zA-Z0-9_]*} $expr {1}]
  if {[catch {set r [expr $e]} msg]} {
    set r 1
  }
  if {$r} {
    set rc [catch {uplevel 1 $code} result]
  } elseif {[llength $args] >= 2} {
    set rc [catch {uplevel 1 [lindex $args 1]} result]
  } else {
    return
  }
  return -code $rc $result
}

proc capable {expr} {
  return 1
}

# ---------------------------------------------------------------------------
# Test counters and finish
# ---------------------------------------------------------------------------

proc set_test_counter {args} {}
proc finish_test {} {}
proc reset_db {} {
  # Close and reopen default database.
  catch {db close}
  sqlite3 db :memory:
}

# ---------------------------------------------------------------------------
# Stubs for commonly used utility procs
# ---------------------------------------------------------------------------

proc database_may_be_corrupt {} {}
proc database_never_corrupt {} {}
proc optimization_control {db flag {val ""}} {}

proc forcedelete {args} {
  foreach f $args {
    catch {file delete -force $f}
  }
}

proc forcecopy {from to} {
  catch {file copy -force $from $to}
}

proc do_faultsim_test {args} {}
proc faultsim_test {args} {}
proc faultsim_delete_and_reopen {args} {
  reset_db
}
proc faultsim_restore_and_reopen {args} {
  reset_db
}
proc faultsim_save_and_close {} {
  catch {db close}
}

proc sqlite3_memdebug_settitle {args} {}
proc sqlite3_memdebug_log {args} {}
proc sqlite3_soft_heap_limit {args} { return 0 }
proc sqlite3_hard_heap_limit {args} { return 0 }

proc do_malloc_test {args} {}
proc do_ioerr_test {args} {}

proc speed_trial {name numstmt sql} {
  catch {execsql $sql}
}

proc speed_trial_tcl {name numstmt script} {
  catch {uplevel 1 $script}
}

# Return a dummy version string.
proc sqlite3 {args} {
  # If called with -version, return version string.
  if {[llength $args] == 1 && [lindex $args 0] eq "-version"} {
    return "3.51.2"
  }
  # Forward to the real sqlite3 command.
  uplevel 1 [list ::_real_sqlite3 {*}$args]
}

# Save the real sqlite3 command and replace with our wrapper.
if {[info commands ::_real_sqlite3] eq ""} {
  rename ::sqlite3 ::_real_sqlite3
  # Redefine sqlite3 to handle -version specially.
  proc sqlite3 {args} {
    if {[llength $args] == 1 && [lindex $args 0] eq "-version"} {
      return "3.51.2"
    }
    uplevel 1 [list ::_real_sqlite3 {*}$args]
  }
}

# PRAGMA and compile_options stubs.
proc sqlite3_compileoption_used {opt} { return 1 }
proc sqlite3_compileoption_get {n} { return "" }

proc testvfs {args} {}
proc register_echo_module {db} {}
proc register_tclvar_module {db} {}

# Suppress crash-test and corruption-test infrastructure.
proc crashsql {args} { return 0 }
proc integrity_check {db} { return "ok" }

proc db_save {} {}
proc db_save_prng_state {} {}
proc db_restore_prng_state {} {}
proc db_restore {} { reset_db }
proc db_delete_and_reopen {args} { reset_db }

proc sorter_test_fakeheap {args} {}
proc wal_set_journal_mode {db mode} {
  catch {$db eval "PRAGMA journal_mode=$mode"}
}

proc do_select_tests {prefix cases} {
  foreach {tn sql result} $cases {
    catch {execsql $sql}
  }
}

proc do_multicol_test {prefix script} {
  catch {uplevel 1 $script}
}

# Provide a no-op for sqlite3_db_config and similar C-level test helpers.
proc sqlite3_db_config {args} { return 0 }
proc sqlite3_extended_result_codes {args} {}
proc sqlite3_limit {args} { return 0 }
proc sqlite3_sleep {ms} { after $ms }

# ---------------------------------------------------------------------------
# Source the test file
# ---------------------------------------------------------------------------

# The actual test file is sourced by the runner after this shim is loaded.
