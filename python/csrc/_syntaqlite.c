/*
 * Python C extension module for syntaqlite.
 *
 * Exposes: parse, format_sql, validate, tokenize
 * Links against libsyntaqlite.a (static)
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <string.h>
#include "syntaqlite/parser.h"
#include "syntaqlite/tokenizer.h"
#include "syntaqlite/formatter.h"
#include "syntaqlite/validation.h"
#include "syntaqlite/dialect.h"
#include "syntaqlite_sqlite/sqlite_node.h"

/* ─── Dialect capsule ────────────────────────────────────────────────── */

static const char *DIALECT_CAPSULE_NAME = "syntaqlite.dialect";

static void
dialect_capsule_destructor(PyObject *capsule)
{
    SyntaqliteLoadedDialect *ld =
        (SyntaqliteLoadedDialect *)PyCapsule_GetPointer(capsule, DIALECT_CAPSULE_NAME);
    syntaqlite_loaded_dialect_destroy(ld);
}

/* Generated: tag switch that builds Python dicts from C AST nodes. */
#include "_py_ast_wrap.h"

/* Custom exception for format errors */
static PyObject *FormatError;

/* ─── helpers ───────────────────────────────────────────────────────── */

/*
 * Extract the SyntaqliteDialect from a PyCapsule, writing it to *out.
 * Returns 0 if dialect_obj is None (use SQLite default).
 * Returns 1 if a dialect was extracted into *out.
 * Returns -1 on error (Python exception set).
 */
static int
extract_dialect(PyObject *dialect_obj, SyntaqliteDialect *out)
{
    if (!dialect_obj || dialect_obj == Py_None)
        return 0;

    if (!PyCapsule_IsValid(dialect_obj, DIALECT_CAPSULE_NAME)) {
        PyErr_SetString(PyExc_TypeError,
            "dialect must be a Dialect object");
        return -1;
    }
    SyntaqliteLoadedDialect *ld =
        (SyntaqliteLoadedDialect *)PyCapsule_GetPointer(dialect_obj, DIALECT_CAPSULE_NAME);
    *out = syntaqlite_loaded_dialect_get(ld);
    return 1;
}

/* ─── load_dialect ──────────────────────────────────────────────────── */

static PyObject *
syntaqlite_py_load_dialect(PyObject *self, PyObject *args)
{
    const char *path;
    const char *name = NULL;

    if (!PyArg_ParseTuple(args, "s|z", &path, &name))
        return NULL;

    SyntaqliteLoadedDialect *ld = syntaqlite_dialect_load(path, name);
    if (!ld)
        return PyErr_NoMemory();

    const char *err = syntaqlite_loaded_dialect_error(ld);
    if (err) {
        PyErr_SetString(PyExc_OSError, err);
        syntaqlite_loaded_dialect_destroy(ld);
        return NULL;
    }

    PyObject *capsule = PyCapsule_New(ld, DIALECT_CAPSULE_NAME,
                                      dialect_capsule_destructor);
    if (!capsule) {
        syntaqlite_loaded_dialect_destroy(ld);
        return NULL;
    }

    return capsule;
}

/* ─── parse ─────────────────────────────────────────────────────────── */

static PyObject *
syntaqlite_py_parse(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *sql;
    Py_ssize_t sql_len;
    PyObject *dialect_obj = NULL;

    static char *kwlist[] = {"sql", "dialect", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|O", kwlist,
                                     &sql, &sql_len, &dialect_obj))
        return NULL;

    SyntaqliteDialect dialect;
    int has_dialect = extract_dialect(dialect_obj, &dialect);
    if (has_dialect < 0)
        return NULL;

    PyObject *result_list = PyList_New(0);
    if (!result_list)
        return NULL;

    SyntaqliteParser *p;
    if (has_dialect)
        p = syntaqlite_parser_create_with_dialect(NULL, dialect);
    else
        p = syntaqlite_parser_create(NULL);

    if (!p) {
        Py_DECREF(result_list);
        return PyErr_NoMemory();
    }

    syntaqlite_parser_reset(p, sql, (uint32_t)sql_len);

    for (;;) {
        int32_t rc = syntaqlite_parser_next(p);
        if (rc == SYNTAQLITE_PARSE_DONE)
            break;

        if (rc == SYNTAQLITE_PARSE_OK) {
            uint32_t root = syntaqlite_result_root(p);
            PyObject *node = syntaqlite_py_wrap_node(p, root);
            if (!node) {
                syntaqlite_parser_destroy(p);
                Py_DECREF(result_list);
                return NULL;
            }
            PyList_Append(result_list, node);
            Py_DECREF(node);
        } else {
            /* SYNTAQLITE_PARSE_ERROR — build error dict */
            PyObject *err_dict = PyDict_New();
            if (!err_dict) {
                syntaqlite_parser_destroy(p);
                Py_DECREF(result_list);
                return NULL;
            }

            PyDict_SetItemString(err_dict, "type",
                                 PyUnicode_InternFromString("Error"));

            const char *err = syntaqlite_result_error_msg(p);
            PyObject *err_str = PyUnicode_FromString(err ? err : "unknown parse error");
            if (err_str) {
                PyDict_SetItemString(err_dict, "message", err_str);
                Py_DECREF(err_str);
            }

            uint32_t err_off = syntaqlite_result_error_offset(p);
            uint32_t err_len = syntaqlite_result_error_length(p);
            PyObject *off_obj = PyLong_FromUnsignedLong(err_off);
            PyObject *len_obj = PyLong_FromUnsignedLong(err_len);
            if (off_obj) {
                PyDict_SetItemString(err_dict, "offset", off_obj);
                Py_DECREF(off_obj);
            }
            if (len_obj) {
                PyDict_SetItemString(err_dict, "length", len_obj);
                Py_DECREF(len_obj);
            }

            PyList_Append(result_list, err_dict);
            Py_DECREF(err_dict);
        }
    }

    syntaqlite_parser_destroy(p);
    return result_list;
}

/* ─── format_sql ────────────────────────────────────────────────────── */

static PyObject *
syntaqlite_py_format_sql(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *sql;
    Py_ssize_t sql_len;
    unsigned int line_width = 80;
    unsigned int indent_width = 2;
    const char *keyword_case_str = "upper";
    int semicolons = 1;
    PyObject *dialect_obj = NULL;

    static char *kwlist[] = {"sql", "line_width", "indent_width",
                             "keyword_case", "semicolons", "dialect", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|IIspO", kwlist,
                                     &sql, &sql_len,
                                     &line_width, &indent_width,
                                     &keyword_case_str, &semicolons,
                                     &dialect_obj))
        return NULL;

    SyntaqliteDialect dialect;
    int has_dialect = extract_dialect(dialect_obj, &dialect);
    if (has_dialect < 0)
        return NULL;

    SyntaqliteFormatConfig config;
    config.line_width = line_width;
    config.indent_width = indent_width;
    config.semicolons = semicolons ? 1 : 0;

    if (strcmp(keyword_case_str, "lower") == 0)
        config.keyword_case = SYNTAQLITE_KEYWORD_LOWER;
    else
        config.keyword_case = SYNTAQLITE_KEYWORD_UPPER;

    SyntaqliteFormatter *f;
    if (has_dialect)
        f = syntaqlite_formatter_create_with_dialect(dialect, &config);
    else
        f = syntaqlite_formatter_create_sqlite_with_config(&config);
    if (!f)
        return PyErr_NoMemory();

    int32_t rc = syntaqlite_formatter_format(f, sql, (uint32_t)sql_len);
    if (rc != SYNTAQLITE_FORMAT_OK) {
        const char *err = syntaqlite_formatter_error_msg(f);
        PyObject *err_str = PyUnicode_FromString(err ? err : "format error");
        if (err_str) {
            PyErr_SetObject(FormatError, err_str);
            Py_DECREF(err_str);
        }
        syntaqlite_formatter_destroy(f);
        return NULL;
    }

    const char *output = syntaqlite_formatter_output(f);
    uint32_t output_len = syntaqlite_formatter_output_len(f);

    PyObject *result = PyUnicode_FromStringAndSize(output, output_len);
    syntaqlite_formatter_destroy(f);
    return result;
}

/* ─── validate helpers ──────────────────────────────────────────────── */

/*
 * Parse a Python list of relation dicts into a C array of SyntaqliteRelationDef
 * and call the given registration function. Returns 0 on success, -1 on error
 * (with Python exception set).
 */
static int
register_relations(SyntaqliteValidator *v, PyObject *list, const char *kind,
                   void (*add_fn)(SyntaqliteValidator*,
                                  const SyntaqliteRelationDef*, uint32_t))
{
    if (!list || list == Py_None)
        return 0;

    if (!PyList_Check(list)) {
        PyErr_Format(PyExc_TypeError, "%s must be a list", kind);
        return -1;
    }

    Py_ssize_t n = PyList_Size(list);
    if (n == 0)
        return 0;

    SyntaqliteRelationDef *defs = (SyntaqliteRelationDef *)calloc(n, sizeof(SyntaqliteRelationDef));
    const char ***all_columns = (const char ***)calloc(n, sizeof(const char **));
    if (!defs || !all_columns) {
        free(defs);
        free(all_columns);
        PyErr_NoMemory();
        return -1;
    }

    int ok = 1;
    for (Py_ssize_t i = 0; i < n && ok; i++) {
        PyObject *entry = PyList_GetItem(list, i);
        if (!PyDict_Check(entry)) {
            PyErr_Format(PyExc_TypeError,
                "each %s must be a dict with 'name' and optional 'columns'", kind);
            ok = 0;
            break;
        }

        PyObject *name_obj = PyDict_GetItemString(entry, "name");
        if (!name_obj || !PyUnicode_Check(name_obj)) {
            PyErr_Format(PyExc_TypeError, "%s 'name' must be a string", kind);
            ok = 0;
            break;
        }
        defs[i].name = PyUnicode_AsUTF8(name_obj);

        PyObject *cols_obj = PyDict_GetItemString(entry, "columns");
        if (cols_obj && cols_obj != Py_None && PyList_Check(cols_obj)) {
            Py_ssize_t n_cols = PyList_Size(cols_obj);
            const char **cols = (const char **)calloc(n_cols, sizeof(const char *));
            if (!cols) { ok = 0; PyErr_NoMemory(); break; }
            for (Py_ssize_t j = 0; j < n_cols; j++) {
                PyObject *col = PyList_GetItem(cols_obj, j);
                if (!PyUnicode_Check(col)) {
                    PyErr_SetString(PyExc_TypeError, "column names must be strings");
                    ok = 0;
                    break;
                }
                cols[j] = PyUnicode_AsUTF8(col);
            }
            defs[i].columns = cols;
            defs[i].column_count = (uint32_t)n_cols;
            all_columns[i] = cols;
        } else {
            defs[i].columns = NULL;
            defs[i].column_count = 0;
            all_columns[i] = NULL;
        }
    }

    if (ok)
        add_fn(v, defs, (uint32_t)n);

    for (Py_ssize_t i = 0; i < n; i++)
        free((void *)all_columns[i]);
    free(all_columns);
    free(defs);

    return ok ? 0 : -1;
}

/* ─── module resolver trampoline ─────────────────────────────────────── */

/*
 * C trampoline that calls a Python callable for module resolution.
 * user_data is a borrowed PyObject* (the callable).
 * Returns a malloc-allocated string or NULL.
 */
static char *
py_module_resolver_trampoline(const char *module_path, void *user_data)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject *callable = (PyObject *)user_data;
    PyObject *result = PyObject_CallFunction(callable, "s", module_path);
    char *out = NULL;

    if (result && result != Py_None && PyUnicode_Check(result)) {
        const char *s = PyUnicode_AsUTF8(result);
        if (s)
            out = strdup(s);
    }

    Py_XDECREF(result);
    PyGILState_Release(gstate);
    return out;
}

/* ─── validate ──────────────────────────────────────────────────────── */

static PyObject *
syntaqlite_py_validate(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *sql;
    Py_ssize_t sql_len;
    PyObject *tables_list = NULL;
    PyObject *views_list = NULL;
    const char *schema_ddl = NULL;
    Py_ssize_t schema_ddl_len = 0;
    int render = 0;
    PyObject *dialect_obj = NULL;
    PyObject *resolver_obj = NULL;

    static char *kwlist[] = {"sql", "tables", "views", "schema_ddl",
                             "render", "dialect", "module_resolver", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|OOz#pOO", kwlist,
                                     &sql, &sql_len,
                                     &tables_list, &views_list,
                                     &schema_ddl, &schema_ddl_len,
                                     &render, &dialect_obj, &resolver_obj))
        return NULL;

    SyntaqliteDialect dialect;
    int has_dialect = extract_dialect(dialect_obj, &dialect);
    if (has_dialect < 0)
        return NULL;

    SyntaqliteValidator *v;
    if (has_dialect)
        v = syntaqlite_validator_create_with_dialect(dialect);
    else
        v = syntaqlite_validator_create_sqlite();
    if (!v)
        return PyErr_NoMemory();

    int has_schema = 0;

    /* Register tables */
    if (register_relations(v, tables_list, "table",
                           syntaqlite_validator_add_tables) < 0) {
        syntaqlite_validator_destroy(v);
        return NULL;
    }
    if (tables_list && tables_list != Py_None && PyList_Size(tables_list) > 0)
        has_schema = 1;

    /* Register views */
    if (register_relations(v, views_list, "view",
                           syntaqlite_validator_add_views) < 0) {
        syntaqlite_validator_destroy(v);
        return NULL;
    }
    if (views_list && views_list != Py_None && PyList_Size(views_list) > 0)
        has_schema = 1;

    /* Load schema from DDL */
    if (schema_ddl) {
        syntaqlite_validator_load_schema_ddl(v, schema_ddl, (uint32_t)schema_ddl_len);
        has_schema = 1;
    }

    /* Any schema → strict mode so unresolved names are errors. */
    if (has_schema) {
        syntaqlite_validator_set_strict_schema(v, 1);
    }

    /* Set module resolver if provided */
    if (resolver_obj && resolver_obj != Py_None) {
        if (!PyCallable_Check(resolver_obj)) {
            PyErr_SetString(PyExc_TypeError, "module_resolver must be callable");
            syntaqlite_validator_destroy(v);
            return NULL;
        }
        syntaqlite_validator_set_module_resolver(
            v, py_module_resolver_trampoline, (void *)resolver_obj);
    }

    uint32_t n_diags = syntaqlite_validator_analyze(v, sql, (uint32_t)sql_len);

    if (render) {
        const char *rendered = syntaqlite_validator_render_diagnostics(v, NULL);
        PyObject *result = PyUnicode_FromString(rendered ? rendered : "");
        syntaqlite_validator_destroy(v);
        return result;
    }

    /* Build result dict with diagnostics + lineage */
    PyObject *result = PyDict_New();
    if (!result) {
        syntaqlite_validator_destroy(v);
        return NULL;
    }

    /* Diagnostics */
    PyObject *diag_list = PyList_New(0);
    if (!diag_list) {
        Py_DECREF(result);
        syntaqlite_validator_destroy(v);
        return NULL;
    }

    if (n_diags > 0) {
        const SyntaqliteDiagnostic *diags = syntaqlite_validator_diagnostics(v);
        for (uint32_t i = 0; i < n_diags; i++) {
            PyObject *d = PyDict_New();
            if (!d) {
                Py_DECREF(diag_list);
                Py_DECREF(result);
                syntaqlite_validator_destroy(v);
                return NULL;
            }

            const char *sev_str;
            switch (diags[i].severity) {
                case SYNTAQLITE_SEVERITY_ERROR:   sev_str = "error"; break;
                case SYNTAQLITE_SEVERITY_WARNING: sev_str = "warning"; break;
                case SYNTAQLITE_SEVERITY_INFO:    sev_str = "info"; break;
                case SYNTAQLITE_SEVERITY_HINT:    sev_str = "hint"; break;
                default:                          sev_str = "unknown"; break;
            }

            PyObject *sev = PyUnicode_FromString(sev_str);
            PyObject *msg = PyUnicode_FromString(diags[i].message ? diags[i].message : "");
            PyObject *start = PyLong_FromUnsignedLong(diags[i].start_offset);
            PyObject *end = PyLong_FromUnsignedLong(diags[i].end_offset);

            if (sev) { PyDict_SetItemString(d, "severity", sev); Py_DECREF(sev); }
            if (msg) { PyDict_SetItemString(d, "message", msg); Py_DECREF(msg); }
            if (start) { PyDict_SetItemString(d, "start_offset", start); Py_DECREF(start); }
            if (end) { PyDict_SetItemString(d, "end_offset", end); Py_DECREF(end); }

            PyList_Append(diag_list, d);
            Py_DECREF(d);
        }
    }
    PyDict_SetItemString(result, "diagnostics", diag_list);
    Py_DECREF(diag_list);

    /* Column lineage */
    uint32_t col_count = syntaqlite_validator_column_lineage_count(v);
    if (col_count > 0) {
        const SyntaqliteColumnLineage *cols = syntaqlite_validator_column_lineage(v);
        PyObject *lineage_dict = PyDict_New();
        if (!lineage_dict) {
            Py_DECREF(result);
            syntaqlite_validator_destroy(v);
            return NULL;
        }

        PyObject *complete = syntaqlite_validator_lineage_complete(v)
            ? Py_True : Py_False;
        Py_INCREF(complete);
        PyDict_SetItemString(lineage_dict, "complete", complete);
        Py_DECREF(complete);

        PyObject *col_list = PyList_New(0);
        if (!col_list) {
            Py_DECREF(lineage_dict);
            Py_DECREF(result);
            syntaqlite_validator_destroy(v);
            return NULL;
        }

        for (uint32_t i = 0; i < col_count; i++) {
            PyObject *c = PyDict_New();
            if (!c) {
                Py_DECREF(col_list);
                Py_DECREF(lineage_dict);
                Py_DECREF(result);
                syntaqlite_validator_destroy(v);
                return NULL;
            }

            PyObject *name = PyUnicode_FromString(cols[i].name ? cols[i].name : "");
            PyObject *idx = PyLong_FromUnsignedLong(cols[i].index);
            if (name) { PyDict_SetItemString(c, "name", name); Py_DECREF(name); }
            if (idx) { PyDict_SetItemString(c, "index", idx); Py_DECREF(idx); }

            if (cols[i].origin.table) {
                PyObject *origin = PyDict_New();
                if (origin) {
                    PyObject *tbl = PyUnicode_FromString(cols[i].origin.table);
                    PyObject *col_name = PyUnicode_FromString(cols[i].origin.column);
                    if (tbl) { PyDict_SetItemString(origin, "table", tbl); Py_DECREF(tbl); }
                    if (col_name) { PyDict_SetItemString(origin, "column", col_name); Py_DECREF(col_name); }
                    PyDict_SetItemString(c, "origin", origin);
                    Py_DECREF(origin);
                }
            } else {
                Py_INCREF(Py_None);
                PyDict_SetItemString(c, "origin", Py_None);
                Py_DECREF(Py_None);
            }

            PyList_Append(col_list, c);
            Py_DECREF(c);
        }
        PyDict_SetItemString(lineage_dict, "columns", col_list);
        Py_DECREF(col_list);

        /* Relations */
        uint32_t rel_count = syntaqlite_validator_relation_count(v);
        PyObject *rel_list = PyList_New(0);
        if (rel_list) {
            const SyntaqliteRelationAccess *rels = syntaqlite_validator_relations(v);
            for (uint32_t i = 0; i < rel_count; i++) {
                PyObject *r = PyDict_New();
                if (r) {
                    PyObject *rname = PyUnicode_FromString(rels[i].name ? rels[i].name : "");
                    PyObject *rkind = PyUnicode_FromString(
                        rels[i].kind == SYNTAQLITE_RELATION_VIEW ? "view" : "table");
                    if (rname) { PyDict_SetItemString(r, "name", rname); Py_DECREF(rname); }
                    if (rkind) { PyDict_SetItemString(r, "kind", rkind); Py_DECREF(rkind); }
                    PyList_Append(rel_list, r);
                    Py_DECREF(r);
                }
            }
            PyDict_SetItemString(lineage_dict, "relations", rel_list);
            Py_DECREF(rel_list);
        }

        /* Tables */
        uint32_t tbl_count = syntaqlite_validator_physical_table_count(v);
        PyObject *tbl_list = PyList_New(0);
        if (tbl_list) {
            const SyntaqlitePhysicalTableAccess *tbls = syntaqlite_validator_physical_tables(v);
            for (uint32_t i = 0; i < tbl_count; i++) {
                PyObject *tname = PyUnicode_FromString(tbls[i].name ? tbls[i].name : "");
                if (tname) {
                    PyList_Append(tbl_list, tname);
                    Py_DECREF(tname);
                }
            }
            PyDict_SetItemString(lineage_dict, "physical_tables", tbl_list);
            Py_DECREF(tbl_list);
        }

        /* Unexpanded views */
        uint32_t uv_count = syntaqlite_validator_unexpanded_view_count(v);
        PyObject *uv_list = PyList_New(0);
        if (uv_list) {
            const SyntaqliteUnexpandedView *uvs = syntaqlite_validator_unexpanded_views(v);
            for (uint32_t i = 0; i < uv_count; i++) {
                PyObject *uname = PyUnicode_FromString(uvs[i].name ? uvs[i].name : "");
                if (uname) {
                    PyList_Append(uv_list, uname);
                    Py_DECREF(uname);
                }
            }
            PyDict_SetItemString(lineage_dict, "unexpanded_views", uv_list);
            Py_DECREF(uv_list);
        }

        PyDict_SetItemString(result, "lineage", lineage_dict);
        Py_DECREF(lineage_dict);
    } else {
        Py_INCREF(Py_None);
        PyDict_SetItemString(result, "lineage", Py_None);
        Py_DECREF(Py_None);
    }

    /* Per-statement data */
    {
        uint32_t stmt_count = syntaqlite_validator_statement_count(v);
        PyObject *stmt_list = PyList_New(0);
        if (!stmt_list) {
            Py_DECREF(result);
            syntaqlite_validator_destroy(v);
            return NULL;
        }
        for (uint32_t si = 0; si < stmt_count; si++) {
            PyObject *stmt_dict = PyDict_New();
            if (!stmt_dict) {
                Py_DECREF(stmt_list);
                Py_DECREF(result);
                syntaqlite_validator_destroy(v);
                return NULL;
            }

            /* Per-statement diagnostics */
            uint32_t sd_count = syntaqlite_validator_statement_diagnostic_count(v, si);
            PyObject *sd_list = PyList_New(0);
            if (sd_list) {
                const SyntaqliteDiagnostic *sd = syntaqlite_validator_statement_diagnostics(v, si);
                for (uint32_t i = 0; i < sd_count; i++) {
                    PyObject *d = PyDict_New();
                    if (!d) break;
                    const char *sev_str;
                    switch (sd[i].severity) {
                        case SYNTAQLITE_SEVERITY_ERROR:   sev_str = "error"; break;
                        case SYNTAQLITE_SEVERITY_WARNING: sev_str = "warning"; break;
                        case SYNTAQLITE_SEVERITY_INFO:    sev_str = "info"; break;
                        case SYNTAQLITE_SEVERITY_HINT:    sev_str = "hint"; break;
                        default:                          sev_str = "unknown"; break;
                    }
                    PyObject *sev = PyUnicode_FromString(sev_str);
                    PyObject *msg = PyUnicode_FromString(sd[i].message ? sd[i].message : "");
                    PyObject *start = PyLong_FromUnsignedLong(sd[i].start_offset);
                    PyObject *end = PyLong_FromUnsignedLong(sd[i].end_offset);
                    if (sev) { PyDict_SetItemString(d, "severity", sev); Py_DECREF(sev); }
                    if (msg) { PyDict_SetItemString(d, "message", msg); Py_DECREF(msg); }
                    if (start) { PyDict_SetItemString(d, "start_offset", start); Py_DECREF(start); }
                    if (end) { PyDict_SetItemString(d, "end_offset", end); Py_DECREF(end); }
                    PyList_Append(sd_list, d);
                    Py_DECREF(d);
                }
                PyDict_SetItemString(stmt_dict, "diagnostics", sd_list);
                Py_DECREF(sd_list);
            }

            /* Per-statement source text */
            {
                const char *src = syntaqlite_validator_statement_source(v, si);
                if (src) {
                    PyObject *src_obj = PyUnicode_FromString(src);
                    if (src_obj) {
                        PyDict_SetItemString(stmt_dict, "source", src_obj);
                        Py_DECREF(src_obj);
                    }
                } else {
                    Py_INCREF(Py_None);
                    PyDict_SetItemString(stmt_dict, "source", Py_None);
                    Py_DECREF(Py_None);
                }
            }

            /* Per-statement lineage */
            uint32_t sc_count = syntaqlite_validator_statement_column_lineage_count(v, si);
            if (sc_count > 0) {
                const SyntaqliteColumnLineage *sc = syntaqlite_validator_statement_column_lineage(v, si);
                PyObject *lin = PyDict_New();
                if (lin) {
                    PyObject *col_list = PyList_New(0);
                    if (col_list) {
                        for (uint32_t i = 0; i < sc_count; i++) {
                            PyObject *c = PyDict_New();
                            if (!c) break;
                            PyObject *name = PyUnicode_FromString(sc[i].name ? sc[i].name : "");
                            PyObject *idx = PyLong_FromUnsignedLong(sc[i].index);
                            if (name) { PyDict_SetItemString(c, "name", name); Py_DECREF(name); }
                            if (idx) { PyDict_SetItemString(c, "index", idx); Py_DECREF(idx); }
                            if (sc[i].origin.table) {
                                PyObject *origin = PyDict_New();
                                if (origin) {
                                    PyObject *tbl = PyUnicode_FromString(sc[i].origin.table);
                                    PyObject *col_name = PyUnicode_FromString(sc[i].origin.column);
                                    if (tbl) { PyDict_SetItemString(origin, "table", tbl); Py_DECREF(tbl); }
                                    if (col_name) { PyDict_SetItemString(origin, "column", col_name); Py_DECREF(col_name); }
                                    PyDict_SetItemString(c, "origin", origin);
                                    Py_DECREF(origin);
                                }
                            } else {
                                Py_INCREF(Py_None);
                                PyDict_SetItemString(c, "origin", Py_None);
                                Py_DECREF(Py_None);
                            }
                            PyList_Append(col_list, c);
                            Py_DECREF(c);
                        }
                        PyDict_SetItemString(lin, "columns", col_list);
                        Py_DECREF(col_list);
                    }

                    /* Per-statement relations */
                    uint32_t sr_count = syntaqlite_validator_statement_relation_count(v, si);
                    PyObject *rel_list = PyList_New(0);
                    if (rel_list) {
                        const SyntaqliteRelationAccess *sr = syntaqlite_validator_statement_relations(v, si);
                        for (uint32_t i = 0; i < sr_count; i++) {
                            PyObject *r = PyDict_New();
                            if (r) {
                                PyObject *rname = PyUnicode_FromString(sr[i].name ? sr[i].name : "");
                                PyObject *rkind = PyUnicode_FromString(
                                    sr[i].kind == SYNTAQLITE_RELATION_VIEW ? "view" : "table");
                                if (rname) { PyDict_SetItemString(r, "name", rname); Py_DECREF(rname); }
                                if (rkind) { PyDict_SetItemString(r, "kind", rkind); Py_DECREF(rkind); }
                                PyList_Append(rel_list, r);
                                Py_DECREF(r);
                            }
                        }
                        PyDict_SetItemString(lin, "relations", rel_list);
                        Py_DECREF(rel_list);
                    }

                    /* Per-statement tables */
                    uint32_t st_count = syntaqlite_validator_statement_physical_table_count(v, si);
                    PyObject *tbl_list = PyList_New(0);
                    if (tbl_list) {
                        const SyntaqlitePhysicalTableAccess *st = syntaqlite_validator_statement_physical_tables(v, si);
                        for (uint32_t i = 0; i < st_count; i++) {
                            PyObject *tname = PyUnicode_FromString(st[i].name ? st[i].name : "");
                            if (tname) { PyList_Append(tbl_list, tname); Py_DECREF(tname); }
                        }
                        PyDict_SetItemString(lin, "physical_tables", tbl_list);
                        Py_DECREF(tbl_list);
                    }

                    PyObject *complete = Py_True;  /* per-stmt lineage always present if we got here */
                    Py_INCREF(complete);
                    PyDict_SetItemString(lin, "complete", complete);
                    Py_DECREF(complete);

                    PyDict_SetItemString(stmt_dict, "lineage", lin);
                    Py_DECREF(lin);
                }
            } else {
                Py_INCREF(Py_None);
                PyDict_SetItemString(stmt_dict, "lineage", Py_None);
                Py_DECREF(Py_None);
            }

            /* Per-statement relations (top-level, independent of lineage) */
            {
                uint32_t sr_count = syntaqlite_validator_statement_relation_count(v, si);
                PyObject *rel_list = PyList_New(0);
                if (rel_list) {
                    const SyntaqliteRelationAccess *sr = syntaqlite_validator_statement_relations(v, si);
                    for (uint32_t i = 0; i < sr_count; i++) {
                        PyObject *r = PyDict_New();
                        if (r) {
                            PyObject *rname = PyUnicode_FromString(sr[i].name ? sr[i].name : "");
                            PyObject *rkind = PyUnicode_FromString(
                                sr[i].kind == SYNTAQLITE_RELATION_VIEW ? "view" : "table");
                            if (rname) { PyDict_SetItemString(r, "name", rname); Py_DECREF(rname); }
                            if (rkind) { PyDict_SetItemString(r, "kind", rkind); Py_DECREF(rkind); }
                            PyList_Append(rel_list, r);
                            Py_DECREF(r);
                        }
                    }
                    PyDict_SetItemString(stmt_dict, "relations", rel_list);
                    Py_DECREF(rel_list);
                }
            }

            /* Per-statement defined relations */
            uint32_t dr_count = syntaqlite_validator_statement_defined_relation_count(v, si);
            PyObject *dr_list = PyList_New(0);
            if (dr_list) {
                const SyntaqliteDefinedRelation *drs = syntaqlite_validator_statement_defined_relations(v, si);
                for (uint32_t i = 0; i < dr_count; i++) {
                    PyObject *dr = PyDict_New();
                    if (dr) {
                        PyObject *drname = PyUnicode_FromString(drs[i].name ? drs[i].name : "");
                        PyObject *is_view = drs[i].is_view ? Py_True : Py_False;
                        Py_INCREF(is_view);
                        if (drname) { PyDict_SetItemString(dr, "name", drname); Py_DECREF(drname); }
                        PyDict_SetItemString(dr, "is_view", is_view);
                        Py_DECREF(is_view);
                        PyList_Append(dr_list, dr);
                        Py_DECREF(dr);
                    }
                }
                PyDict_SetItemString(stmt_dict, "defined_relations", dr_list);
                Py_DECREF(dr_list);
            }

            PyList_Append(stmt_list, stmt_dict);
            Py_DECREF(stmt_dict);
        }
        PyDict_SetItemString(result, "statements", stmt_list);
        Py_DECREF(stmt_list);
    }

    syntaqlite_validator_destroy(v);
    return result;
}

/* ─── tokenize ──────────────────────────────────────────────────────── */

static PyObject *
syntaqlite_py_tokenize(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *sql;
    Py_ssize_t sql_len;
    PyObject *dialect_obj = NULL;

    static char *kwlist[] = {"sql", "dialect", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|O", kwlist,
                                     &sql, &sql_len, &dialect_obj))
        return NULL;

    SyntaqliteDialect dialect;
    int has_dialect = extract_dialect(dialect_obj, &dialect);
    if (has_dialect < 0)
        return NULL;

    PyObject *result_list = PyList_New(0);
    if (!result_list)
        return NULL;

    SyntaqliteTokenizer *tok;
    if (has_dialect)
        tok = syntaqlite_tokenizer_create_with_dialect(NULL, dialect);
    else
        tok = syntaqlite_tokenizer_create(NULL);
    if (!tok) {
        Py_DECREF(result_list);
        return PyErr_NoMemory();
    }

    syntaqlite_tokenizer_reset(tok, sql, (uint32_t)sql_len);

    SyntaqliteToken token;
    while (syntaqlite_tokenizer_next(tok, &token)) {
        PyObject *t = PyDict_New();
        if (!t) {
            Py_DECREF(result_list);
            syntaqlite_tokenizer_destroy(tok);
            return NULL;
        }

        PyObject *text = PyUnicode_FromStringAndSize(token.text, token.length);
        PyObject *off = PyLong_FromUnsignedLong((unsigned long)(token.text - sql));
        PyObject *length = PyLong_FromUnsignedLong(token.length);
        PyObject *type = PyLong_FromUnsignedLong(token.type);

        if (text) { PyDict_SetItemString(t, "text", text); Py_DECREF(text); }
        if (off) { PyDict_SetItemString(t, "offset", off); Py_DECREF(off); }
        if (length) { PyDict_SetItemString(t, "length", length); Py_DECREF(length); }
        if (type) { PyDict_SetItemString(t, "type", type); Py_DECREF(type); }

        PyList_Append(result_list, t);
        Py_DECREF(t);
    }

    syntaqlite_tokenizer_destroy(tok);
    return result_list;
}

/* ─── Module definition ─────────────────────────────────────────────── */

static PyMethodDef SyntaqliteMethods[] = {
    {"load_dialect", syntaqlite_py_load_dialect, METH_VARARGS,
     "Load a dialect from a shared library.\n\n"
     "Args:\n"
     "    path (str): Path to shared library (.so/.dylib/.dll)\n"
     "    name (str, optional): Dialect name (resolves syntaqlite_{name}_dialect symbol)\n\n"
     "Returns:\n"
     "    An opaque capsule representing the loaded dialect."},

    {"parse", (PyCFunction)syntaqlite_py_parse, METH_VARARGS | METH_KEYWORDS,
     "Parse SQL into a list of typed AST node dicts.\n\n"
     "Each dict has a 'type' key with the node type name (e.g. 'SelectStmt').\n"
     "Fields are keyed by their snake_case name. Child nodes are nested dicts.\n"
     "Lists are Python lists. Source spans are strings. Bools are True/False.\n\n"
     "Args:\n"
     "    sql (str): SQL to parse\n"
     "    dialect: Loaded dialect capsule (default: SQLite)"},

    {"format_sql", (PyCFunction)syntaqlite_py_format_sql, METH_VARARGS | METH_KEYWORDS,
     "Format SQL with configurable options.\n\n"
     "Args:\n"
     "    sql (str): SQL to format\n"
     "    line_width (int): Max line width (default 80)\n"
     "    indent_width (int): Spaces per indent (default 2)\n"
     "    keyword_case (str): 'upper' or 'lower' (default 'upper')\n"
     "    semicolons (bool): Append semicolons (default True)\n"
     "    dialect: Loaded dialect capsule (default: SQLite)\n\n"
     "Raises:\n"
     "    syntaqlite.FormatError: On parse error"},

    {"validate", (PyCFunction)syntaqlite_py_validate, METH_VARARGS | METH_KEYWORDS,
     "Validate SQL against optional schema.\n\n"
     "Args:\n"
     "    sql (str): SQL to validate\n"
     "    tables (list[dict]): Schema tables. Each dict: name (str), columns (list[str])\n"
     "    views (list[dict]): Schema views. Same format as tables\n"
     "    schema_ddl (str): DDL to parse as schema (CREATE TABLE/VIEW statements)\n"
     "    render (bool): If True, return rendered diagnostics string\n"
     "    dialect: Loaded dialect capsule (default: SQLite)\n\n"
     "Returns:\n"
     "    dict with diagnostics and lineage, or str when render=True"},

    {"tokenize", (PyCFunction)syntaqlite_py_tokenize, METH_VARARGS | METH_KEYWORDS,
     "Tokenize SQL into a list of token dicts.\n\n"
     "Each dict has: text (str), offset (int), length (int), type (int).\n\n"
     "Args:\n"
     "    sql (str): SQL to tokenize\n"
     "    dialect: Loaded dialect capsule (default: SQLite)"},

    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef syntaqlite_module = {
    PyModuleDef_HEAD_INIT,
    "_syntaqlite",
    "C extension for syntaqlite — parser, formatter, and validator for SQLite SQL.",
    -1,
    SyntaqliteMethods
};

PyMODINIT_FUNC PyInit__syntaqlite(void) {
    PyObject *m = PyModule_Create(&syntaqlite_module);
    if (m == NULL)
        return NULL;

    FormatError = PyErr_NewException("syntaqlite.FormatError", PyExc_Exception, NULL);
    if (FormatError) {
        Py_INCREF(FormatError);
        PyModule_AddObject(m, "FormatError", FormatError);
    }

    return m;
}
