// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Minimal, portable getopt_long implementation.
// No global state — all state is in synq_getopt_state.
// Supports short options, long options, required/optional/no argument.
// API mirrors POSIX getopt_long but is self-contained and reentrant.

#ifndef SYNQ_SRC_BASE_SYNQ_GETOPT_H
#define SYNQ_SRC_BASE_SYNQ_GETOPT_H

enum {
  SYNQ_NO_ARGUMENT = 0,
  SYNQ_REQUIRED_ARGUMENT = 1,
  SYNQ_OPTIONAL_ARGUMENT = 2,
};

struct synq_option {
  const char *name;  // Long option name (without --)
  int has_arg;       // SYNQ_NO_ARGUMENT, SYNQ_REQUIRED_ARGUMENT, SYNQ_OPTIONAL_ARGUMENT
  int *flag;         // If non-NULL, set *flag = val and return 0
  int val;           // Value to return (or store in *flag)
};

struct synq_getopt_state {
  int optind;       // Index of next argv element to process
  const char *arg;  // Argument value for current option (like optarg)
  int pos;          // Position within current short-option cluster
};

#define SYNQ_GETOPT_INIT {1, NULL, 0}

// Returns option val on match, '?' on error, -1 when done.
int synq_getopt_long(struct synq_getopt_state *st, int argc,
                     char *const *argv, const char *shortopts,
                     const struct synq_option *longopts,
                     int *longindex);

#endif  // SYNQ_SRC_BASE_SYNQ_GETOPT_H
