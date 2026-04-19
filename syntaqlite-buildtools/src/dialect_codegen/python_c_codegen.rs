// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Python C extension codegen: generates a C function that wraps AST nodes
//! as Python dicts, switching on the node tag.
//!
//! Produces `_py_ast_wrap.h` containing `syntaqlite_py_wrap_node()` which
//! builds Python objects eagerly from the C AST.

use std::collections::HashSet;

use crate::util::c_writer::CWriter;
use crate::util::synq_parser::{Field, Storage};
use crate::util::{pascal_to_snake, upper_snake};

use super::{AstModel, NodeLikeRef};

impl AstModel<'_> {
    /// Generate `_py_ast_wrap.h` — a C header containing the node wrapping
    /// function for the Python C extension.
    pub(crate) fn generate_python_c_wrap(&self) -> String {
        let enum_names = self.enum_names();
        let flags_names = self.flags_names();

        let mut w = CWriter::new();
        w.file_header();

        let guard = "SYNTAQLITE_PY_AST_WRAP_H";
        w.header_guard_start(guard);
        w.newline();

        // Forward declaration.
        w.line("static PyObject *syntaqlite_py_wrap_node(SyntaqliteParser *p, uint32_t node_id);");
        w.newline();

        // Helper: wrap a list node into a Python list.
        emit_wrap_list_fn(&mut w);
        w.newline();

        // Helper: wrap a source span into a Python str (or None).
        emit_wrap_span_fn(&mut w);
        w.newline();

        // Main wrap function.
        w.line("static PyObject *");
        w.line("syntaqlite_py_wrap_node(SyntaqliteParser *p, uint32_t node_id) {");
        w.indent();
        w.line("if (node_id == SYNTAQLITE_NULL_NODE)");
        w.indent();
        w.line("Py_RETURN_NONE;");
        w.dedent();
        w.newline();
        w.line("const void *raw = syntaqlite_parser_node(p, node_id);");
        w.line("if (!raw) Py_RETURN_NONE;");
        w.line("uint32_t tag = *(const uint32_t *)raw;");
        w.newline();
        w.line("switch (tag) {");

        for item in self.node_like_items() {
            match item {
                NodeLikeRef::Node(node) => {
                    let tag_const = format!("SYNTAQLITE_NODE_{}", upper_snake(node.name));
                    let c_type = format!("Syntaqlite{}", node.name);

                    w.line(&format!("case {tag_const}: {{"));
                    w.indent();
                    w.line(&format!("const {c_type} *n = (const {c_type} *)raw;"));
                    w.line("PyObject *d = PyDict_New();");
                    w.line("if (!d) return NULL;");

                    // Set "type" key.
                    w.line(&format!(
                        "PyDict_SetItemString(d, \"type\", PyUnicode_InternFromString(\"{}\"));",
                        node.name
                    ));

                    // Set each field.
                    for field in node.fields {
                        emit_node_field_setter(&mut w, field, enum_names, flags_names);
                    }

                    w.line("return d;");
                    w.dedent();
                    w.line("}");
                }
                NodeLikeRef::List(list) => {
                    let tag_const = format!("SYNTAQLITE_NODE_{}", upper_snake(list.name));
                    w.line(&format!("case {tag_const}:"));
                    w.indent();
                    w.line("return syntaqlite_py_wrap_list(p, raw);");
                    w.dedent();
                }
            }
        }

        w.line("default:");
        w.indent();
        w.line("Py_RETURN_NONE;");
        w.dedent();
        w.line("}"); // switch
        w.dedent();
        w.line("}"); // function
        w.newline();

        w.header_guard_end(guard);
        w.finish()
    }
}

/// Emit C code to set one field in the dict `d` from node pointer `n`.
fn emit_node_field_setter(
    w: &mut CWriter,
    field: &Field,
    enum_names: &HashSet<&str>,
    flags_names: &HashSet<&str>,
) {
    let fname = &field.name;
    let py_key = pascal_to_snake(fname);

    w.line("{");
    w.indent();

    match field.storage {
        Storage::Index => {
            w.line(&format!(
                "PyObject *val = syntaqlite_py_wrap_node(p, n->{fname});"
            ));
            w.line(&format!(
                "if (val) {{ PyDict_SetItemString(d, \"{py_key}\", val); Py_DECREF(val); }}"
            ));
        }
        Storage::Inline => {
            let t = &field.type_name;
            if t == "Bool" {
                w.line(&format!(
                    "PyDict_SetItemString(d, \"{py_key}\", n->{fname} ? Py_True : Py_False);"
                ));
            } else if t == "SyntaqliteTextSpan" {
                w.line(&format!(
                    "PyObject *val = syntaqlite_py_wrap_span(p, n->{fname});"
                ));
                w.line(&format!(
                    "if (val) {{ PyDict_SetItemString(d, \"{py_key}\", val); Py_DECREF(val); }}"
                ));
            } else if enum_names.contains(t.as_str()) || flags_names.contains(t.as_str()) {
                // Flags are a union with .raw; enums are plain uint32_t typedefs.
                let accessor = if flags_names.contains(t.as_str()) {
                    format!("n->{fname}.raw")
                } else {
                    format!("n->{fname}")
                };
                w.line(&format!(
                    "{{ PyObject *v = PyLong_FromLong((long){accessor}); \
                     if (v) {{ PyDict_SetItemString(d, \"{py_key}\", v); Py_DECREF(v); }} }}"
                ));
            } else {
                w.line("/* unknown inline type — skipped */");
            }
        }
    }

    w.dedent();
    w.line("}");
}

/// Emit the helper function that wraps a list node into a Python list.
fn emit_wrap_list_fn(w: &mut CWriter) {
    w.line("static PyObject *");
    w.line("syntaqlite_py_wrap_list(SyntaqliteParser *p, const void *raw) {");
    w.indent();
    w.line("uint32_t count = syntaqlite_list_count(raw);");
    w.line("PyObject *list = PyList_New((Py_ssize_t)count);");
    w.line("if (!list) return NULL;");
    w.line("for (uint32_t i = 0; i < count; i++) {");
    w.indent();
    w.line("uint32_t child_id = syntaqlite_list_child_id(raw, i);");
    w.line("PyObject *child = syntaqlite_py_wrap_node(p, child_id);");
    w.line("if (!child) { Py_DECREF(list); return NULL; }");
    w.line("PyList_SET_ITEM(list, (Py_ssize_t)i, child); /* steals ref */");
    w.dedent();
    w.line("}");
    w.line("return list;");
    w.dedent();
    w.line("}");
}

/// Emit the helper function that wraps a source span into a Python str.
fn emit_wrap_span_fn(w: &mut CWriter) {
    w.line("static PyObject *");
    w.line("syntaqlite_py_wrap_span(SyntaqliteParser *p, SyntaqliteTextSpan span) {");
    w.indent();
    w.line("if (span.length == 0) Py_RETURN_NONE;");
    w.line("const char *src = syntaqlite_parser_text(p, NULL, NULL);");
    w.line("return PyUnicode_FromStringAndSize(src + span.offset, span.length);");
    w.dedent();
    w.line("}");
}

#[cfg(test)]
mod tests {
    use crate::dialect_codegen::AstModel;
    use crate::util::synq_parser::parse_synq_file;

    #[test]
    fn generates_switch_for_nodes() {
        let items = parse_synq_file(
            r"
            node Foo { x: inline Bool  y: inline SyntaqliteTextSpan }
            node Bar { child: index Foo }
            list FooList { Foo }
            ",
        )
        .unwrap();
        let model = AstModel::new(&items);
        let code = model.generate_python_c_wrap();
        assert!(code.contains("case SYNTAQLITE_NODE_FOO:"), "{code}");
        assert!(code.contains("case SYNTAQLITE_NODE_BAR:"), "{code}");
        assert!(code.contains("case SYNTAQLITE_NODE_FOO_LIST:"), "{code}");
        assert!(code.contains("syntaqlite_py_wrap_node"), "{code}");
        assert!(code.contains("syntaqlite_py_wrap_span"), "{code}");
        assert!(code.contains("syntaqlite_py_wrap_list"), "{code}");
    }

    #[test]
    fn full_codegen_from_real_synq() {
        let base_synq = crate::base_files::base_synq_files();
        let mut all_items = Vec::new();
        for (name, content) in base_synq {
            let items =
                parse_synq_file(content).unwrap_or_else(|e| panic!("parse {name} failed: {e}"));
            all_items.extend(items);
        }
        let model = AstModel::new(&all_items);
        let code = model.generate_python_c_wrap();

        assert!(
            code.contains("case SYNTAQLITE_NODE_SELECT_STMT:"),
            "missing SelectStmt"
        );
        assert!(
            code.contains("PyUnicode_InternFromString(\"SelectStmt\")"),
            "missing type key"
        );
        assert!(
            code.contains("\"from_clause\""),
            "missing from_clause field"
        );
    }
}
