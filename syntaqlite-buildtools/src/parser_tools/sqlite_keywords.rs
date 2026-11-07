// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! `SQLite`'s keywords, mirrored from `aKeywordTable` in `tool/mkkeywordhash.c`.
//!
//! The tests below fail when a vendored `SQLite` changes the keyword table or
//! the way `keywordCode` matches keywords, since the generated perfect-hash
//! lookup replaces it and must keep returning the same keyword for every input.

/// `(keyword, token)` pairs in `aKeywordTable` order, with the `TK_` prefix
/// dropped from the token. The order decides how keyword text is packed.
pub(crate) const SQLITE_KEYWORDS: &[(&str, &str)] = &[
    ("ABORT", "ABORT"),
    ("ACTION", "ACTION"),
    ("ADD", "ADD"),
    ("AFTER", "AFTER"),
    ("ALL", "ALL"),
    ("ALTER", "ALTER"),
    ("ALWAYS", "ALWAYS"),
    ("ANALYZE", "ANALYZE"),
    ("AND", "AND"),
    ("AS", "AS"),
    ("ASC", "ASC"),
    ("ATTACH", "ATTACH"),
    ("AUTOINCREMENT", "AUTOINCR"),
    ("BEFORE", "BEFORE"),
    ("BEGIN", "BEGIN"),
    ("BETWEEN", "BETWEEN"),
    ("BY", "BY"),
    ("CASCADE", "CASCADE"),
    ("CASE", "CASE"),
    ("CAST", "CAST"),
    ("CHECK", "CHECK"),
    ("COLLATE", "COLLATE"),
    ("COLUMN", "COLUMNKW"),
    ("COMMIT", "COMMIT"),
    ("CONFLICT", "CONFLICT"),
    ("CONSTRAINT", "CONSTRAINT"),
    ("CREATE", "CREATE"),
    ("CROSS", "JOIN_KW"),
    ("CURRENT", "CURRENT"),
    ("CURRENT_DATE", "CTIME_KW"),
    ("CURRENT_TIME", "CTIME_KW"),
    ("CURRENT_TIMESTAMP", "CTIME_KW"),
    ("DATABASE", "DATABASE"),
    ("DEFAULT", "DEFAULT"),
    ("DEFERRED", "DEFERRED"),
    ("DEFERRABLE", "DEFERRABLE"),
    ("DELETE", "DELETE"),
    ("DESC", "DESC"),
    ("DETACH", "DETACH"),
    ("DISTINCT", "DISTINCT"),
    ("DO", "DO"),
    ("DROP", "DROP"),
    ("END", "END"),
    ("EACH", "EACH"),
    ("ELSE", "ELSE"),
    ("ESCAPE", "ESCAPE"),
    ("EXCEPT", "EXCEPT"),
    ("EXCLUSIVE", "EXCLUSIVE"),
    ("EXCLUDE", "EXCLUDE"),
    ("EXISTS", "EXISTS"),
    ("EXPLAIN", "EXPLAIN"),
    ("FAIL", "FAIL"),
    ("FILTER", "FILTER"),
    ("FIRST", "FIRST"),
    ("FOLLOWING", "FOLLOWING"),
    ("FOR", "FOR"),
    ("FOREIGN", "FOREIGN"),
    ("FROM", "FROM"),
    ("FULL", "JOIN_KW"),
    ("GENERATED", "GENERATED"),
    ("GLOB", "LIKE_KW"),
    ("GROUP", "GROUP"),
    ("GROUPS", "GROUPS"),
    ("HAVING", "HAVING"),
    ("IF", "IF"),
    ("IGNORE", "IGNORE"),
    ("IMMEDIATE", "IMMEDIATE"),
    ("IN", "IN"),
    ("INDEX", "INDEX"),
    ("INDEXED", "INDEXED"),
    ("INITIALLY", "INITIALLY"),
    ("INNER", "JOIN_KW"),
    ("INSERT", "INSERT"),
    ("INSTEAD", "INSTEAD"),
    ("INTERSECT", "INTERSECT"),
    ("INTO", "INTO"),
    ("IS", "IS"),
    ("ISNULL", "ISNULL"),
    ("JOIN", "JOIN"),
    ("KEY", "KEY"),
    ("LAST", "LAST"),
    ("LEFT", "JOIN_KW"),
    ("LIKE", "LIKE_KW"),
    ("LIMIT", "LIMIT"),
    ("MATCH", "MATCH"),
    ("MATERIALIZED", "MATERIALIZED"),
    ("NATURAL", "JOIN_KW"),
    ("NO", "NO"),
    ("NOT", "NOT"),
    ("NOTHING", "NOTHING"),
    ("NOTNULL", "NOTNULL"),
    ("NULL", "NULL"),
    ("NULLS", "NULLS"),
    ("OF", "OF"),
    ("OFFSET", "OFFSET"),
    ("ON", "ON"),
    ("OR", "OR"),
    ("ORDER", "ORDER"),
    ("OTHERS", "OTHERS"),
    ("OUTER", "JOIN_KW"),
    ("OVER", "OVER"),
    ("PARTITION", "PARTITION"),
    ("PLAN", "PLAN"),
    ("PRAGMA", "PRAGMA"),
    ("PRECEDING", "PRECEDING"),
    ("PRIMARY", "PRIMARY"),
    ("QUERY", "QUERY"),
    ("RAISE", "RAISE"),
    ("RANGE", "RANGE"),
    ("RECURSIVE", "RECURSIVE"),
    ("REFERENCES", "REFERENCES"),
    ("REGEXP", "LIKE_KW"),
    ("REINDEX", "REINDEX"),
    ("RELEASE", "RELEASE"),
    ("RENAME", "RENAME"),
    ("REPLACE", "REPLACE"),
    ("RESTRICT", "RESTRICT"),
    ("RETURNING", "RETURNING"),
    ("RIGHT", "JOIN_KW"),
    ("ROLLBACK", "ROLLBACK"),
    ("ROW", "ROW"),
    ("ROWS", "ROWS"),
    ("SAVEPOINT", "SAVEPOINT"),
    ("SELECT", "SELECT"),
    ("SET", "SET"),
    ("TABLE", "TABLE"),
    ("TEMP", "TEMP"),
    ("TEMPORARY", "TEMP"),
    ("THEN", "THEN"),
    ("TIES", "TIES"),
    ("TO", "TO"),
    ("TRANSACTION", "TRANSACTION"),
    ("TRIGGER", "TRIGGER"),
    ("UNBOUNDED", "UNBOUNDED"),
    ("UNION", "UNION"),
    ("UNIQUE", "UNIQUE"),
    ("UPDATE", "UPDATE"),
    ("USING", "USING"),
    ("VACUUM", "VACUUM"),
    ("VALUES", "VALUES"),
    ("VIEW", "VIEW"),
    ("VIRTUAL", "VIRTUAL"),
    ("WHEN", "WHEN"),
    ("WHERE", "WHERE"),
    ("WINDOW", "WINDOW"),
    ("WITH", "WITH"),
    ("WITHIN", "WITHIN"),
    ("WITHOUT", "WITHOUT"),
];

#[cfg(test)]
mod tests {
    use super::SQLITE_KEYWORDS;
    use crate::util::mkkeywordhash_parser::parse_keyword_table;

    const MKKEYWORDHASH: &str = include_str!(concat!(
        env!("CARGO_MANIFEST_DIR"),
        "/sqlite-vendored/sources/mkkeywordhash.c"
    ));

    #[test]
    fn keywords_match_upstream_table() {
        let table = parse_keyword_table(MKKEYWORDHASH).expect("keyword table");
        let start = MKKEYWORDHASH
            .find("aKeywordTable[] = {")
            .expect("aKeywordTable");
        let end = start + MKKEYWORDHASH[start..].find("\n};").expect("table end");
        let rows = MKKEYWORDHASH[start..end].matches("{ \"").count();
        assert_eq!(table.keywords.len(), rows, "unparsed aKeywordTable rows");
        let upstream: Vec<(String, String)> = table
            .keywords
            .iter()
            .map(|k| {
                (
                    k.name.clone(),
                    k.token.trim_start_matches("TK_").to_string(),
                )
            })
            .collect();
        let ours: Vec<(String, String)> = SQLITE_KEYWORDS
            .iter()
            .map(|&(n, t)| (n.to_string(), t.to_string()))
            .collect();
        assert_eq!(ours, upstream, "update SQLITE_KEYWORDS from aKeywordTable");
    }

    /// The statements `mkkeywordhash` prints for `keywordCode`, minus the
    /// `testcase` loop. The generated lookup agrees with `SQLite` on every input
    /// because a keyword matches iff the length and every byte under `&~0x20`
    /// agree, and the hash reads only the first and last bytes, which are
    /// letters in every keyword. If this changes, re-check that argument.
    const UPSTREAM_KEYWORD_CODE: &str = r##"printf("static int keywordCode(const char *z, int n, int *pType){\n");
printf("  int i, j;\n");
printf("  const char *zKW;\n");
printf("  assert( n>=2 );\n");
printf("  i = ((charMap(z[0])*%d) %c", HASH_C0, HASH_CC);
printf(" (charMap(z[n-1])*%d) %c", HASH_C1, HASH_CC);
printf(" n*%d) %% %d;\n", HASH_C2, bestSize);
printf("  for(i=(int)aKWHash[i]; i>0; i=aKWNext[i]){\n");
printf("    if( aKWLen[i]!=n ) continue;\n");
printf("    zKW = &zKWText[aKWOffset[i]];\n");
printf("#ifdef SQLITE_ASCII\n");
printf("    if( (z[0]&~0x20)!=zKW[0] ) continue;\n");
printf("    if( (z[1]&~0x20)!=zKW[1] ) continue;\n");
printf("    j = 2;\n");
printf("    while( j<n && (z[j]&~0x20)==zKW[j] ){ j++; }\n");
printf("#endif\n");
printf("#ifdef SQLITE_EBCDIC\n");
printf("    if( toupper(z[0])!=zKW[0] ) continue;\n");
printf("    if( toupper(z[1])!=zKW[1] ) continue;\n");
printf("    j = 2;\n");
printf("    while( j<n && toupper(z[j])==zKW[j] ){ j++; }\n");
printf("#endif\n");
printf("    if( j<n ) continue;\n");
printf("    *pType = aKWCode[i];\n");
printf("    break;\n");
printf("  }\n");
printf("  return n;\n");
printf("}\n");"##;

    #[test]
    fn keyword_matching_matches_upstream() {
        let start = MKKEYWORDHASH
            .find("printf(\"static int keywordCode(")
            .expect("keywordCode");
        let end = start
            + MKKEYWORDHASH[start..]
                .find("printf(\"int sqlite3KeywordCode(")
                .expect("sqlite3KeywordCode");
        let mut in_testcase = false;
        let mut actual = Vec::new();
        for line in MKKEYWORDHASH[start..end].lines().map(str::trim) {
            if line.starts_with("for(i=0; i<nKeyword; i++){") {
                in_testcase = true;
            } else if in_testcase {
                in_testcase = line != "}";
            } else if !line.is_empty() {
                actual.push(line);
            }
        }
        assert_eq!(actual.join("\n"), UPSTREAM_KEYWORD_CODE);
    }

    #[test]
    fn keyword_hash_reads_only_letters() {
        assert!(MKKEYWORDHASH.contains("#define charMap(X)   (0x20|(X))"));
        for (name, _) in SQLITE_KEYWORDS {
            let b = name.as_bytes();
            assert!(
                b[0].is_ascii_uppercase() && b[b.len() - 1].is_ascii_uppercase(),
                "{name}"
            );
        }
    }
}
