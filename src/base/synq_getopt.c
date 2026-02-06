// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Minimal, portable getopt_long implementation.

#include "src/base/synq_getopt.h"

#include <string.h>

int synq_getopt_long(struct synq_getopt_state *st, int argc,
                     char *const *argv, const char *shortopts,
                     const struct synq_option *longopts,
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

      if (longopts[i].has_arg == SYNQ_NO_ARGUMENT) {
        if (eq)
          return '?';  // Unexpected argument.
      } else if (longopts[i].has_arg == SYNQ_REQUIRED_ARGUMENT) {
        if (eq) {
          st->arg = eq + 1;
        } else if (st->optind < argc) {
          st->arg = argv[st->optind++];
        } else {
          return '?';  // Missing argument.
        }
      } else {  // SYNQ_OPTIONAL_ARGUMENT
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
