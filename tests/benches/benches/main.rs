// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::fmt::Write;

use criterion::{BenchmarkId, Criterion, Throughput, black_box, criterion_group, criterion_main};
use syntaqlite::ParseOutcome;

// ── SQL fixtures ────────────────────────────────────────────────────────

/// ~45 B — single simple SELECT
const SMALL_SQL: &str = "SELECT id, name FROM users WHERE active = 1;";

/// ~430 B — multi-join query with GROUP BY/HAVING/ORDER/LIMIT
const MEDIUM_SQL: &str = "\
SELECT u.id, u.name, u.email, o.order_id, o.total, p.product_name
FROM users u
INNER JOIN orders o ON o.user_id = u.id
LEFT JOIN order_items oi ON oi.order_id = o.order_id
LEFT JOIN products p ON p.product_id = oi.product_id
WHERE u.active = 1
  AND o.created_at > '2024-01-01'
  AND o.total > 100.00
GROUP BY u.id, u.name, u.email, o.order_id, o.total, p.product_name
HAVING COUNT(oi.item_id) > 2
ORDER BY o.total DESC
LIMIT 50 OFFSET 10;";

/// ~50 KB — 500 mixed statements (INSERTs, SELECTs, CREATEs, UPDATEs)
fn large_sql() -> String {
    let mut sql = String::with_capacity(60_000);
    for i in 0..500 {
        match i % 4 {
            0 => {
                let _ = writeln!(
                    sql,
                    "INSERT INTO metrics (ts, sensor_id, value, label) VALUES ('2024-01-{:02}', {i}, {:.2}, 'sensor_{i}');",
                    (i % 28) + 1,
                    f64::from(i) * 1.5
                );
            }
            1 => {
                let _ = writeln!(
                    sql,
                    "SELECT m.ts, m.value, s.name FROM metrics m JOIN sensors s ON s.id = m.sensor_id WHERE m.sensor_id = {i} AND m.value > {:.1} ORDER BY m.ts;",
                    f64::from(i) * 0.5
                );
            }
            2 => {
                let _ = writeln!(
                    sql,
                    "UPDATE metrics SET value = value + 1.0, label = 'updated_{i}' WHERE sensor_id = {i} AND ts > '2024-01-01';"
                );
            }
            _ => {
                let _ = writeln!(
                    sql,
                    "DELETE FROM metrics WHERE sensor_id = {i} AND value < {:.1};",
                    f64::from(i) * 0.1
                );
            }
        }
    }
    sql
}

/// ~60 KB — same as `large_sql` but every statement has a leading comment
/// and inline block comments, exercising the comment-handling path.
fn large_commented_sql() -> String {
    let mut sql = String::with_capacity(80_000);
    for i in 0..500 {
        match i % 4 {
            0 => {
                let _ = writeln!(
                    sql,
                    "-- insert row {i}\nINSERT INTO metrics (ts, sensor_id, value, label) VALUES ('2024-01-{:02}', {i}, {:.2}, 'sensor_{i}'); -- done",
                    (i % 28) + 1,
                    f64::from(i) * 1.5
                );
            }
            1 => {
                let _ = writeln!(
                    sql,
                    "/* query {i} */ SELECT m.ts, m.value, s.name FROM metrics m JOIN sensors s ON s.id = m.sensor_id WHERE m.sensor_id = {i} AND m.value > {:.1} ORDER BY m.ts;",
                    f64::from(i) * 0.5
                );
            }
            2 => {
                let _ = writeln!(
                    sql,
                    "-- update {i}\nUPDATE metrics SET value = value + 1.0, label = 'updated_{i}' WHERE sensor_id = {i} AND ts > '2024-01-01';"
                );
            }
            _ => {
                let _ = writeln!(
                    sql,
                    "/* cleanup {i} */ DELETE FROM metrics WHERE sensor_id = {i} AND value < {:.1};",
                    f64::from(i) * 0.1
                );
            }
        }
    }
    sql
}

struct Fixture {
    name: &'static str,
    sql: String,
}

fn fixtures() -> Vec<Fixture> {
    vec![
        Fixture {
            name: "small",
            sql: SMALL_SQL.to_string(),
        },
        Fixture {
            name: "medium",
            sql: MEDIUM_SQL.to_string(),
        },
        Fixture {
            name: "large",
            sql: large_sql(),
        },
        Fixture {
            name: "large_commented",
            sql: large_commented_sql(),
        },
    ]
}

// ── Tokenizer benchmarks ────────────────────────────────────────────────

fn bench_tokenizer(c: &mut Criterion) {
    let mut group = c.benchmark_group("tokenizer");
    let fixtures = fixtures();

    for f in &fixtures {
        group.throughput(Throughput::Bytes(f.sql.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(f.name), &f.sql, |b, sql| {
            let tok = syntaqlite::parse::Tokenizer::new();
            b.iter(|| {
                let cursor = tok.tokenize(black_box(sql));
                for item in cursor {
                    black_box(item);
                }
            });
        });
    }

    group.finish();
}

// ── Parser benchmarks ───────────────────────────────────────────────────

fn bench_parser(c: &mut Criterion) {
    let mut group = c.benchmark_group("parser");
    let fixtures = fixtures();

    for f in &fixtures {
        group.throughput(Throughput::Bytes(f.sql.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(f.name), &f.sql, |b, sql| {
            let parser = syntaqlite::Parser::new();
            b.iter(|| {
                let mut session = parser.parse(black_box(sql));
                loop {
                    match session.next() {
                        ParseOutcome::Done => break,
                        ParseOutcome::Ok(stmt) => {
                            black_box(stmt);
                        }
                        ParseOutcome::Err(err) => {
                            black_box(err);
                        }
                    }
                }
            });
        });
    }

    group.finish();
}

// ── Formatter benchmarks ────────────────────────────────────────────────

fn bench_formatter(c: &mut Criterion) {
    let mut group = c.benchmark_group("formatter");
    let fixtures = fixtures();

    for f in &fixtures {
        group.throughput(Throughput::Bytes(f.sql.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(f.name), &f.sql, |b, sql| {
            let mut fmt = syntaqlite::fmt::token_layout_prototype::TokenFormatter::default();
            b.iter(|| {
                black_box(fmt.format(black_box(sql), 80).unwrap());
            });
        });
    }

    group.finish();
}

// ── LSP Host benchmarks ────────────────────────────────────────────────

fn bench_lsp_host(c: &mut Criterion) {
    let mut group = c.benchmark_group("lsp_host");
    let fixtures = fixtures();

    for f in &fixtures {
        group.throughput(Throughput::Bytes(f.sql.len() as u64));
        group.bench_with_input(
            BenchmarkId::new("open_analyze", f.name),
            &f.sql,
            |b, sql| {
                b.iter(|| {
                    let mut host = syntaqlite::lsp::LspHost::new();
                    host.update_document("test://file.sql", 1, sql.clone());
                    black_box(host.semantic_tokens_encoded("test://file.sql", None));
                });
            },
        );
    }

    for f in &fixtures {
        group.throughput(Throughput::Bytes(f.sql.len() as u64));
        group.bench_with_input(
            BenchmarkId::new("update_cycle", f.name),
            &f.sql,
            |b, sql| {
                let mut host = syntaqlite::lsp::LspHost::new();
                host.update_document("test://file.sql", 1, sql.clone());
                host.semantic_tokens_encoded("test://file.sql", None);
                let mut version = 2;
                b.iter(|| {
                    host.update_document("test://file.sql", version, sql.clone());
                    version += 1;
                    black_box(host.semantic_tokens_encoded("test://file.sql", None));
                });
            },
        );
    }

    group.finish();
}

// ── Criterion setup ─────────────────────────────────────────────────────

criterion_group!(
    benches,
    bench_tokenizer,
    bench_parser,
    bench_formatter,
    bench_lsp_host,
);
criterion_main!(benches);
