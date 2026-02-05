// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Minimal, portable getopt_long implementation.
// No global state — all state is in sq_getopt_state.
// Supports short options, long options, required/optional/no argument.
// API mirrors POSIX getopt_long but is self-contained and reentrant.

#ifndef SYNTAQLITE_SRC_SQ_GETOPT_H
#define SYNTAQLITE_SRC_SQ_GETOPT_H

#include <string.h>

enum {
  SQ_NO_ARGUMENT = 0,
  SQ_REQUIRED_ARGUMENT = 1,
  SQ_OPTIONAL_ARGUMENT = 2,
};

struct sq_option {
  const char *name;  // Long option name (without --)
  int has_arg;       // SQ_NO_ARGUMENT, SQ_REQUIRED_ARGUMENT, SQ_OPTIONAL_ARGUMENT
  int *flag;         // If non-NULL, set *flag = val and return 0
  int val;           // Value to return (or store in *flag)
};

struct sq_getopt_state {
  int optind;       // Index of next argv element to process
  const char *arg;  // Argument value for current option (like optarg)
  int pos;          // Position within current short-option cluster
};

#define SQ_GETOPT_INIT {1, NULL, 0}

// Returns option val on match, '?' on error, -1 when done.
static inline int sq_getopt_long(struct sq_getopt_state *st, int argc,
                                 char *const *argv, const char *shortopts,
                                 const struct sq_option *longopts,
                                 int *longindex) {
  st->arg = NULL;

  if (st->optind >= argc)
    return -1;

  const char *cur = argv[st->optind];

  // Not an option — stop.
  if (cur[0] != '-' || cur[1] == '\0')
    return -1;

  // "--" — end of options.
  if (cur[1] == '-' && cur[2] == '\0') {
    st->optind++;
    return -1;
  }

  // Long option: --name or --name=value
  if (cur[1] == '-') {
    const char *name = cur + 2;
    const char *eq = strchr(name, '=');
    size_t namelen = eq ? (size_t)(eq - name) : strlen(name);

    for (int i = 0; longopts && longopts[i].name; i++) {
      if (strlen(longopts[i].name) != namelen)
        continue;
      if (memcmp(longopts[i].name, name, namelen) != 0)
        continue;

      // Match found.
      if (longindex)
        *longindex = i;
      st->optind++;

      if (longopts[i].has_arg == SQ_NO_ARGUMENT) {
        if (eq)
          return '?';  // Unexpected argument.
      } else if (longopts[i].has_arg == SQ_REQUIRED_ARGUMENT) {
        if (eq) {
          st->arg = eq + 1;
        } else if (st->optind < argc) {
          st->arg = argv[st->optind++];
        } else {
          return '?';  // Missing argument.
        }
      } else {  // SQ_OPTIONAL_ARGUMENT
        if (eq)
          st->arg = eq + 1;
      }

      if (longopts[i].flag) {
        *longopts[i].flag = longopts[i].val;
        return 0;
      }
      return longopts[i].val;
    }

    // No match.
    st->optind++;
    return '?';
  }

  // Short option: -x or -xyz (clustered)
  if (st->pos == 0)
    st->pos = 1;

  char c = cur[st->pos];
  const char *spec = shortopts ? strchr(shortopts, c) : NULL;
  if (!spec || c == ':') {
    // Advance past this cluster.
    if (!cur[++st->pos]) {
      st->pos = 0;
      st->optind++;
    }
    return '?';
  }

  // Check if this option takes an argument.
  if (spec[1] == ':') {
    // Has argument.
    if (cur[st->pos + 1]) {
      // Argument is rest of this argv element: -fVALUE
      st->arg = &cur[st->pos + 1];
    } else if (spec[2] != ':') {
      // Required argument — take next argv element.
      if (st->optind + 1 < argc) {
        st->arg = argv[st->optind + 1];
        st->optind++;
      } else {
        st->pos = 0;
        st->optind++;
        return '?';  // Missing argument.
      }
    }
    // For optional (::), if nothing attached, arg stays NULL.
    st->pos = 0;
    st->optind++;
  } else {
    // No argument — advance within cluster or to next argv.
    st->pos++;
    if (!cur[st->pos]) {
      st->pos = 0;
      st->optind++;
    }
  }

  return (unsigned char)c;
}

#endif  // SYNTAQLITE_SRC_SQ_GETOPT_H
