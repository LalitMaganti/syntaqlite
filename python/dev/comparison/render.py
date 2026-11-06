"""
Markdown rendering for syntaqlite competitive comparison.

Loads .md.tmpl templates and substitutes {{PLACEHOLDER}}s with
data from cached JSON results.
"""

import os
import re

from python.dev.comparison import bench

FORMATTER_NAMES = ["syntaqlite", "prettier-cst", "sql-formatter", "sqlglot[c]", "sleek", "sqruff"]

TMPL_DIR = os.path.dirname(os.path.abspath(__file__))


# ─── Markdown table helper ───────────────────────────────────────

def _md_table(headers, rows, align=None):
    cols = len(headers)
    if align is None:
        align = ['l'] * cols
    widths = [len(h) for h in headers]
    for row in rows:
        for i, cell in enumerate(row):
            plain = re.sub(r'\033\[[0-9;]*m', '', str(cell))
            widths[i] = max(widths[i], len(plain))
    hdr = '| ' + ' | '.join(
        h.ljust(widths[i]) if align[i] == 'l' else h.rjust(widths[i]) if align[i] == 'r' else h.center(widths[i])
        for i, h in enumerate(headers)
    ) + ' |'
    sep_parts = []
    for i in range(cols):
        if align[i] == 'r':
            sep_parts.append('-' * (widths[i] - 1) + ':')
        elif align[i] == 'c':
            sep_parts.append(':' + '-' * (widths[i] - 2) + ':')
        else:
            sep_parts.append('-' * widths[i])
    sep = '| ' + ' | '.join(sep_parts) + ' |'
    lines = [hdr, sep]
    for row in rows:
        cells = []
        for i, cell in enumerate(row):
            s = str(cell)
            plain = re.sub(r'\033\[[0-9;]*m', '', s)
            pad = widths[i] - len(plain)
            if align[i] == 'r':
                cells.append(' ' * pad + s)
            elif align[i] == 'c':
                lp = pad // 2
                rp = pad - lp
                cells.append(' ' * lp + s + ' ' * rp)
            else:
                cells.append(s + ' ' * pad)
        lines.append('| ' + ' | '.join(cells) + ' |')
    return '\n'.join(lines)


def _table(headers, rows, align=None):
    return _md_table(headers, rows, align)


# ─── Placeholder builders ────────────────────────────────────────

def _build_placeholders(results):
    """Build a dict of {{KEY}} -> rendered string from JSON results."""
    ph = {}

    # Meta
    for cat in ["parser", "formatter", "validator", "lsp"]:
        if cat in results and "meta" in results[cat]:
            meta = results[cat]["meta"]
            ts = meta.get("timestamp", "unknown")
            date = ts[:10] if len(ts) >= 10 else ts
            plat = meta.get("platform", "unknown")
            ver = meta.get("syntaqlite_version", "unknown")
            ph["META_LINE"] = f"Generated on `{plat}` with syntaqlite `{ver}` on {date}."
            break
    ph.setdefault("META_LINE", "")

    # Parser
    if "parser" in results:
        _build_parser(ph, results["parser"])

    # Formatter
    if "formatter" in results:
        _build_formatter(ph, results["formatter"])

    # Validator
    if "validator" in results:
        _build_validator(ph, results["validator"])

    # LSP
    if "lsp" in results:
        _build_lsp(ph, results["lsp"])

    return ph


def _build_parser(ph, r):
    tallies = r["tallies"]
    total = r["total"]

    # Accuracy table
    ranked = sorted(tallies.items(), key=lambda x: (-x[1]["correct"], x[1]["reject_valid"]))
    rows = []
    for name, t in ranked:
        pct = t["correct"] * 100 // total
        bar = "\u2588" * (t["correct"] * 20 // total)
        rv = str(t['reject_valid']) if t['reject_valid'] > 0 else "-"
        ai = str(t['accept_invalid']) if t['accept_invalid'] > 0 else "-"
        rows.append([name, f"{t['correct']}/{total} ({pct}%) {bar}", rv, ai])
    ph["PARSER_ACCURACY_TABLE"] = _table(
        ["Tool", "Correct", "Rejects Valid", "Accepts Invalid"], rows, ['l', 'l', 'r', 'r'])

    # Speed
    _build_speed(ph, r["speed"], "PARSER")

    # Detail-only fields
    tool_names = r.get("tool_names", [])
    ph["PARSER_TOTAL"] = str(total)

    # Ground truth
    gt_rows = []
    n_valid = 0
    for gt in r["ground_truth"]:
        status = "OK" if gt["sqlite3_ok"] else f"SKIP ({gt['error']})"
        gt_rows.append([gt["name"], status])
        if gt["sqlite3_ok"]:
            n_valid += 1
    ph["PARSER_GROUND_TRUTH_TABLE"] = _table(["Statement", "sqlite3"], gt_rows)
    ph["PARSER_N_VALID"] = str(n_valid)

    # Per-statement
    headers = ["Test", "sqlite3"] + tool_names
    align = ['l', 'c'] + ['c'] * len(tool_names)
    rows = []
    for stmt in r["per_statement"]:
        row = [stmt["name"][:38], stmt["sqlite3"]]
        for tn in tool_names:
            row.append(stmt["tools"].get(tn, "?"))
        rows.append(row)
    ph["PARSER_PER_STMT_TABLE"] = _table(headers, rows, align)

    # Scoreboard (same as accuracy table for detail page)
    ph["PARSER_SCOREBOARD"] = ph["PARSER_ACCURACY_TABLE"]


def _build_formatter(ph, r):
    tallies = r["tallies"]
    total = r["total"]
    n_valid = r["n_valid"]
    fmt_names = r.get("formatter_names", FORMATTER_NAMES)

    # Accuracy table (sorted by correct count descending)
    ranked = sorted(fmt_names, key=lambda tn: -tallies[tn]['format_ok'])
    rows = []
    for tn in ranked:
        t = tallies[tn]
        pct = t['format_ok'] * 100 // total
        bar = "\u2588" * (t['format_ok'] * 20 // total)
        refused = total - t['format_ok'] - t['corrupted']
        rows.append([tn, f"{t['format_ok']}/{total} ({pct}%) {bar}", str(t['corrupted']) if t['corrupted'] else "-", str(refused) if refused else "-"])
    ph["FORMATTER_ACCURACY_TABLE"] = _table(
        ["Tool", "Correct", "Corrupt", "Refused"], rows, ['l', 'l', 'r', 'r'])

    # Speed
    _build_speed(ph, r["speed"], "FORMATTER")

    # Detail-only fields
    ph["FORMATTER_TOTAL"] = str(total)
    ph["FORMATTER_N_VALID"] = str(n_valid)

    # Ground truth
    gt_rows = []
    for gt in r["ground_truth"]:
        status = "OK" if gt["sqlite3_ok"] else f"SKIP ({gt['error']})"
        gt_rows.append([gt["name"], status])
    ph["FORMATTER_GROUND_TRUTH_TABLE"] = _table(["Statement", "sqlite3"], gt_rows)

    # Per-statement
    headers = ["Test"] + fmt_names
    align = ['l'] + ['c'] * len(fmt_names)
    rows = []
    for stmt in r["per_statement"]:
        row = [stmt["name"][:38]]
        for tn in fmt_names:
            row.append(stmt["tools"].get(tn, "?"))
        rows.append(row)
    ph["FORMATTER_PER_STMT_TABLE"] = _table(headers, rows, align)

    # Scoreboard
    ph["FORMATTER_SCOREBOARD"] = ph["FORMATTER_ACCURACY_TABLE"]

    # Corruption details
    if r.get("corruption_details"):
        cd_rows = [[d["tool"], d["test"], d["error"]] for d in r["corruption_details"]]
        ph["FORMATTER_CORRUPTION_DETAILS"] = (
            "### Corruption details\n\n"
            + _table(["Tool", "Test", "Error"], cd_rows)
        )
    else:
        ph["FORMATTER_CORRUPTION_DETAILS"] = ""

    # Slow tools
    if r.get("slow_tools"):
        slow_rows = [
            [st["name"], "failed" if st["time_ms"] is None else f"{st['time_ms']}ms"]
            for st in r["slow_tools"]
        ]
        ph["FORMATTER_SLOW_TOOLS"] = (
            "\n### Slow tools (single timed run)\n\n"
            + _table(["Tool", "Time"], slow_rows, ['l', 'r'])
        )
    else:
        ph["FORMATTER_SLOW_TOOLS"] = ""


def _build_validator(ph, r):
    tallies = r["tallies"]
    total = r["total"]
    demo = r["demo_diagnostics"]
    tool_meta = r.get("tool_meta", [])

    # Accuracy table
    ranked = sorted(tallies.items(), key=lambda x: (-x[1]["correct"], x[1]["fn"]))
    rows = []
    for name, t in ranked:
        approach = next((a for n, a in tool_meta if n == name), "")
        pct = t["correct"] * 100 // total
        bar = "\u2588" * (t["correct"] * 20 // total)
        fn_str = str(t["fn"]) if t["fn"] > 0 else "-"
        fp_str = str(t["fp"]) if t["fp"] > 0 else "-"
        rows.append([name, approach, f"{t['correct']}/{total} {bar}", fn_str, fp_str])
    ph["VALIDATOR_ACCURACY_TABLE"] = _table(
        ["Tool", "Approach", "Correct", "Missed", "FP"], rows, ['l', 'l', 'l', 'r', 'r'])

    # Diagnostic quality table
    dq_rows = []
    for name in ["syntaqlite", "sqlite3", "sqlite-runner-lsp", "sql-lint"]:
        if name not in demo:
            continue
        d = demo[name]
        found = d.get("errors_found", 0)
        finds_all = "Yes" if d.get("finds_all") else "No"
        dym = "Yes" if d.get("did_you_mean") else "No"
        dq_rows.append([name, d.get("approach", ""), f"{found}/2", finds_all, dym])
    ph["VALIDATOR_DIAGNOSTIC_QUALITY_TABLE"] = _table(
        ["Tool", "Approach", "Errors Found", "Finds All", "Did-you-mean"],
        dq_rows, ['l', 'l', 'c', 'c', 'c'])

    # Diagnostic output examples
    for name in ["syntaqlite", "sqlite3"]:
        d = demo.get(name, {})
        key = f"VALIDATOR_{name.upper()}_OUTPUT"
        ph[key] = d.get("output", "(no output)").strip()

    # Speed
    _build_speed(ph, r["speed"], "VALIDATOR")

    # Detail-only fields

    # Demo query
    ph["VALIDATOR_DEMO_QUERY"] = r.get("demo_query", "").strip()

    # Per-tool output showcase
    tool_output_lines = []
    for name in ["syntaqlite", "sqlite3", "sqlite-runner-lsp", "sql-lint"]:
        if name not in demo:
            continue
        d = demo[name]
        tool_output_lines.append(f"### {name}\n")
        if d.get("description"):
            tool_output_lines.append(f"{d['description']}\n")
        tool_output_lines.append("```")
        tool_output_lines.append(d.get("output", "(no output)"))
        tool_output_lines.append("```\n")
    ph["VALIDATOR_TOOL_OUTPUTS"] = "\n".join(tool_output_lines)

    # Per-case table
    tool_names_val = [n for n, _ in tool_meta]
    headers = ["Test", "Expect"] + tool_names_val
    align = ['l', 'c'] + ['c'] * len(tool_names_val)
    rows = []
    for case in r["per_case"]:
        row = [case["description"][:38], case["expected"]]
        for tn in tool_names_val:
            row.append(case["tools"].get(tn, "?"))
        rows.append(row)
    ph["VALIDATOR_PER_CASE_TABLE"] = _table(headers, rows, align)

    # Scoreboard
    ph["VALIDATOR_SCOREBOARD"] = ph["VALIDATOR_ACCURACY_TABLE"]


def _build_lsp(ph, r):
    tool_names = r["tool_names"]

    # Features table — mark sqls with footnote
    display_names = []
    for n in tool_names:
        display_names.append(f"{n} \u00b9" if n == "sqls" else n)
    headers = ["Feature"] + display_names
    align = ['l'] + ['c'] * len(tool_names)
    ph["LSP_FEATURES_TABLE"] = _table(headers, r["feature_rows"], align)

    # sqls footnote
    ph["LSP_SQLS_FOOTNOTE"] = (
        "\u00b9 sqls requires a live database connection. Completion and hover results come\n"
        "from the connected database schema, not static analysis. Without a database,\n"
        "these features return no results."
    )

    # Completion depth bar chart
    comp_rows = []
    for row in r["feature_rows"]:
        if row[0] == "Completion":
            for i, name in enumerate(tool_names):
                cell = row[i + 1]
                m = re.search(r'\((\d+) items?\)', cell)
                count = int(m.group(1)) if m else 0
                if count > 0:
                    label = f"{name} \u00b9" if name == "sqls" else name
                    comp_rows.append((label, count))
            break
    if comp_rows:
        max_count = max(c for _, c in comp_rows)
        comp_rows.sort(key=lambda x: -x[1])
        bar_rows = []
        for label, count in comp_rows:
            bar_len = max(1, count * 20 // max_count)
            bar = "\u2588" * bar_len
            bar_rows.append([label, f"{count} {bar}"])
        ph["LSP_COMPLETION_DEPTH"] = _table(["Tool", "Items"], bar_rows)
    else:
        ph["LSP_COMPLETION_DEPTH"] = ""

    # Speed
    speed = r.get("speed", {})
    ph["LSP_SPEED"] = bench.markdown_table(speed.get("results", []), speed.get("untimed"))

    # Diagnostic detail
    if r.get("diagnostic_detail"):
        lines = []
        for name in tool_names:
            detail = r["diagnostic_detail"].get(name, "(no data)")
            lines.append(f"### {name}\n")
            lines.append("```")
            lines.append(detail)
            lines.append("```\n")
        ph["LSP_DIAGNOSTIC_DETAIL"] = "\n".join(lines)
    else:
        ph["LSP_DIAGNOSTIC_DETAIL"] = ""


def _build_speed(ph, speed, prefix):
    bench_1x = speed.get("bench_1x", {})
    bench_30x = speed.get("bench_30x", {})
    ph[f"{prefix}_SPEED_1X"] = bench.markdown_table(bench_1x.get("results", []))
    ph[f"{prefix}_SPEED_30X"] = bench.markdown_table(bench_30x.get("results", []))
    ph[f"{prefix}_SPEED_1X_DESC"] = bench_1x.get("description", "")
    ph[f"{prefix}_SPEED_30X_DESC"] = bench_30x.get("description", "")


# ─── Template rendering ──────────────────────────────────────────

def _render_template(tmpl_path, placeholders):
    with open(tmpl_path) as f:
        template = f.read()

    def replace(m):
        key = m.group(1)
        return placeholders.get(key, m.group(0))

    return re.sub(r'\{\{(\w+)\}\}', replace, template)


# ─── Public API ──────────────────────────────────────────────────

def write_docs(results, docs_dir):
    """Write both summary and detail markdown files from templates."""
    ph = _build_placeholders(results)

    summary_tmpl = os.path.join(TMPL_DIR, "comparison.md.tmpl")
    summary_path = os.path.join(docs_dir, "comparison.md")
    with open(summary_path, "w") as f:
        f.write(_render_template(summary_tmpl, ph))
    print(f"Written: {summary_path}")

    detail_tmpl = os.path.join(TMPL_DIR, "comparison-details.md.tmpl")
    detail_path = os.path.join(docs_dir, "comparison-details.md")
    with open(detail_path, "w") as f:
        f.write(_render_template(detail_tmpl, ph))
    print(f"Written: {detail_path}")
