| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 4.1 ± 2.1 |  |  |
| `lemon-rs` | 5.9 ± 2.5 | +46.3% | (1.5x slower) |
| `sql-parser-cst` | 251.6 ± 19.2 | +6099.8% | (62.0x slower) |
| `sqlglot[c]` | 231.5 ± 9.1 | +5606.1% | (57.1x slower) |
| `sqlparser-rs` | 16.6 ± 0.3 | +309.3% | (4.1x slower) |
| `node-sql-parser` | 240.0 ± 6.7 | +5815.7% | (59.2x slower) |
| `sqlfluff` | 8069.8 ± 49.8 | +198785.9% | (1988.9x slower) |
