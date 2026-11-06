| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 4.1 ± 2.0 |  |  |
| `lemon-rs` | 2.2 ± 1.2 | -47.1% | (1.9x faster) |
| `sql-parser-cst` | 104.2 ± 2.6 | +2459.0% | (25.6x slower) |
| `sqlglot[c]` | 84.0 ± 1.2 | +1964.2% | (20.6x slower) |
| `sqlparser-rs` | 3.0 ± 1.8 | -25.2% |  |
| `node-sql-parser` | 89.9 ± 2.4 | +2108.5% | (22.1x slower) |
| `sqlfluff` | 505.8 ± 13.1 | +12324.7% | (124.2x slower) |
