// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// AST dump — indented text representation of a parsed tree.
//
// Split out of parser.c for size/isolation: this code is self-contained and
// only reaches into the parser's arena + dialect field metadata.

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "csrc/parser_internal.h"  // IWYU pragma: keep
#include "syntaqlite/dialect.h"
#include "syntaqlite/parser.h"
#include "syntaqlite_dialect/ast_builder.h"
#include "syntaqlite_dialect/dialect_types.h"

typedef SYNQ_VEC(char) DumpBuf;

static void dump_append(DumpBuf* b,
                        SyntaqliteMemMethods mem,
                        const char* s,
                        uint32_t n) {
  syntaqlite_vec_push_n(b, s, n, mem);
}

SYNQ_PRINTF(3, 4)
static void dump_printf(DumpBuf* b,
                        SyntaqliteMemMethods mem,
                        const char* fmt,
                        ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  if (n <= 0)
    return;
  syntaqlite_vec_ensure(b, b->count + (uint32_t)n + 1, mem);
  va_start(ap, fmt);
  vsnprintf(b->data + b->count, (uint32_t)n + 1, fmt, ap);
  va_end(ap);
  b->count += (uint32_t)n;
}

static void dump_indent(DumpBuf* b, SyntaqliteMemMethods mem, uint32_t indent) {
  for (uint32_t i = 0; i < indent; i++)
    dump_append(b, mem, "  ", 2);
}

static void dump_node_recursive(DumpBuf* b,
                                SyntaqliteParser* p,
                                uint32_t node_id,
                                uint32_t indent) {
  if (node_id == SYNTAQLITE_NULL_NODE)
    return;
  uint32_t count = syntaqlite_vec_len(&p->ctx.ast.offsets);
  if (node_id >= count)
    return;

  const uint8_t* raw = synq_arena_cptr(&p->ctx.ast, node_id);
  uint32_t tag;
  memcpy(&tag, raw, sizeof(tag));

  const SyntaqliteDialectTemplate* g = p->dialect.tmpl;
  if (tag >= g->node_count)
    return;

  const char* name = g->node_names[tag];
  uint8_t field_count = g->field_meta_counts[tag];
  SyntaqliteMemMethods mem = p->mem;

  // List node: no field descriptors, has tag + count header.
  if (field_count == 0 && tag != 0) {
    SynqListHeader hdr;
    memcpy(&hdr, raw, sizeof(hdr));
    dump_indent(b, mem, indent);
    dump_printf(b, mem, "%s [%u items]\n", name, hdr.count);
    const uint32_t* children = (const uint32_t*)(raw + sizeof(SynqListHeader));
    for (uint32_t i = 0; i < hdr.count; i++) {
      dump_node_recursive(b, p, children[i], indent + 1);
    }
    return;
  }

  dump_indent(b, mem, indent);
  dump_printf(b, mem, "%s\n", name);

  if (field_count == 0)
    return;
  const SyntaqliteFieldMeta* fields = g->field_meta[tag];

  for (uint8_t fi = 0; fi < field_count; fi++) {
    const SyntaqliteFieldMeta* fm = &fields[fi];
    const uint8_t* field_ptr = raw + fm->offset;

    switch (fm->kind) {
      case SYNTAQLITE_FIELD_NODE_ID: {
        uint32_t child_id;
        memcpy(&child_id, field_ptr, sizeof(child_id));
        dump_indent(b, mem, indent + 1);
        if (child_id == SYNTAQLITE_NULL_NODE) {
          dump_printf(b, mem, "%s: (none)\n", fm->name);
        } else {
          dump_printf(b, mem, "%s:\n", fm->name);
          dump_node_recursive(b, p, child_id, indent + 2);
        }
        break;
      }
      case SYNTAQLITE_FIELD_SPAN: {
        SyntaqliteTextSpan sp;
        memcpy(&sp, field_ptr, sizeof(sp));
        dump_indent(b, mem, indent + 1);
        if (sp.length == 0) {
          dump_printf(b, mem, "%s: (none)\n", fm->name);
        } else {
          const char* base =
#ifdef SYNTAQLITE_OMIT_MACROS
              p->stmt_source;
#else
              sp._layer_id == 0
                  ? p->stmt_source
                  : p->macro.layers.data[sp._layer_id].expansion_data;
#endif
          dump_printf(b, mem, "%s: \"%.*s\"\n", fm->name, (int)sp.length,
                      base + sp.offset);
        }
        break;
      }
      case SYNTAQLITE_FIELD_BOOL: {
        uint32_t val;
        memcpy(&val, field_ptr, sizeof(val));
        dump_indent(b, mem, indent + 1);
        dump_printf(b, mem, "%s: %s\n", fm->name, val ? "TRUE" : "FALSE");
        break;
      }
      case SYNTAQLITE_FIELD_FLAGS: {
        uint8_t val = *field_ptr;
        // Mask to defined bits only — upper bits may contain struct padding.
        if (fm->display_count < 8)
          val &= (uint8_t)((1u << fm->display_count) - 1);
        dump_indent(b, mem, indent + 1);
        dump_printf(b, mem, "%s: ", fm->name);
        if (val == 0) {
          dump_append(b, mem, "(none)", 6);
        } else {
          int first = 1;
          for (int bit = 0; bit < fm->display_count; bit++) {
            if (val & (1 << bit)) {
              const char* flag_name = fm->display ? fm->display[bit] : "?";
              if (flag_name[0] == '\0')
                continue;
              if (!first)
                dump_append(b, mem, " ", 1);
              dump_append(b, mem, flag_name, (uint32_t)strlen(flag_name));
              first = 0;
            }
          }
          if (first)
            dump_append(b, mem, "(none)", 6);
        }
        dump_append(b, mem, "\n", 1);
        break;
      }
      case SYNTAQLITE_FIELD_ENUM: {
        uint32_t val;
        memcpy(&val, field_ptr, sizeof(val));
        dump_indent(b, mem, indent + 1);
        const char* label =
            (val < fm->display_count && fm->display) ? fm->display[val] : "?";
        dump_printf(b, mem, "%s: %s\n", fm->name, label);
        break;
      }
    }
  }
}

SYNTAQLITE_API char* syntaqlite_dump_node(SyntaqliteParser* p,
                                          uint32_t node_id,
                                          uint32_t indent) {
  DumpBuf buf;
  syntaqlite_vec_init(&buf);
  dump_node_recursive(&buf, p, node_id, indent);
  syntaqlite_vec_push(&buf, '\0', p->mem);
  return buf.data;
}
