// Reviewed whitespace expectations for grammar-driven token layout.
// Update deliberately: preservation/idempotency alone cannot validate layout.
pub(super) const CASES: &[(&str, u32, &str, &str)] = &[
    (
        "projection",
        32,
        r"SELECT first_column, second_column, third_column FROM some_table;",
        r"SELECT
  first_column,
  second_column,
  third_column
FROM some_table;",
    ),
    (
        "function",
        32,
        r"SELECT f(first_argument, second_argument, third_argument);",
        r"SELECT
  f(
    first_argument,
    second_argument,
    third_argument
  );",
    ),
    (
        "boolean_chain",
        40,
        r"SELECT a FROM t WHERE first_column = 1 AND second_column = 2 AND third_column = 3 AND fourth_column = 4;",
        r"SELECT a
FROM t
WHERE
  first_column = 1
  AND second_column = 2
  AND third_column = 3
  AND fourth_column = 4;",
    ),
    (
        "precedence",
        40,
        r"SELECT a FROM t WHERE first_column = 1 OR second_column = 2 AND third_column = 3;",
        r"SELECT a
FROM t
WHERE
  first_column = 1
  OR second_column = 2
    AND third_column = 3;",
    ),
    (
        "between",
        32,
        r"SELECT a FROM t WHERE very_long_column BETWEEN lower_bound AND upper_bound AND other_column > 0;",
        r"SELECT a
FROM t
WHERE
  very_long_column BETWEEN
    lower_bound
    AND upper_bound
  AND other_column > 0;",
    ),
    (
        "nested_query",
        40,
        r"SELECT a, b FROM (SELECT first_column AS a, second_column AS b FROM some_table WHERE first_column > 0) AS nested WHERE a > 1;",
        r"SELECT a, b
FROM
  (
    SELECT
      first_column AS a,
      second_column AS b
    FROM some_table
    WHERE first_column > 0
  ) AS nested
WHERE a > 1;",
    ),
    (
        "joins",
        40,
        r"SELECT a FROM first_table AS t LEFT OUTER JOIN second_table AS u ON t.identifier = u.identifier JOIN third_table AS v USING(identifier) WHERE t.active;",
        r"SELECT a
FROM
  first_table AS t
  LEFT OUTER JOIN second_table AS u
    ON t.identifier = u.identifier
  JOIN third_table AS v
    USING (identifier)
WHERE t.active;",
    ),
    (
        "case",
        40,
        r"SELECT CASE WHEN first_column > 0 THEN first_result WHEN second_column > 0 THEN second_result ELSE fallback_result END AS result FROM t;",
        r"SELECT
  CASE
    WHEN first_column > 0 THEN
      first_result
    WHEN second_column > 0 THEN
      second_result
    ELSE fallback_result
  END AS result
FROM t;",
    ),
    (
        "cte",
        40,
        r"WITH first_cte AS (SELECT first_column FROM source_table), second_cte AS (SELECT second_column FROM other_table) SELECT * FROM first_cte JOIN second_cte USING(id);",
        r"WITH
  first_cte AS (
    SELECT first_column
    FROM source_table
  ),
  second_cte AS (
    SELECT second_column
    FROM other_table
  )
SELECT *
FROM
  first_cte
  JOIN second_cte USING (id);",
    ),
    (
        "compound",
        32,
        r"SELECT first_column FROM first_table UNION ALL SELECT second_column FROM second_table;",
        r"SELECT first_column
FROM first_table
UNION ALL
SELECT second_column
FROM second_table;",
    ),
    (
        "window",
        40,
        r"SELECT sum(value) FILTER (WHERE active) OVER (PARTITION BY account_id ORDER BY event_time ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) FROM events;",
        r"SELECT
  sum(value)
    FILTER (WHERE active)
    OVER (
      PARTITION BY account_id
      ORDER BY event_time
      ROWS BETWEEN
        UNBOUNDED PRECEDING
        AND CURRENT ROW
    )
FROM events;",
    ),
    (
        "table",
        50,
        r"CREATE TABLE t(first_column INTEGER NOT NULL DEFAULT 1, second_column TEXT CONSTRAINT name CHECK(length(second_column) > 0), CONSTRAINT pk PRIMARY KEY(first_column, second_column));",
        r"CREATE TABLE t (
  first_column INTEGER NOT NULL DEFAULT 1,
  second_column TEXT
    CONSTRAINT name
      CHECK (length(second_column) > 0),
  CONSTRAINT pk PRIMARY KEY (
    first_column,
    second_column
  )
);",
    ),
    (
        "index",
        40,
        r"CREATE INDEX idx ON table_name(first_column DESC, second_column COLLATE nocase) WHERE first_column > 0;",
        r"CREATE INDEX idx ON table_name(
  first_column DESC,
  second_column COLLATE nocase
)
WHERE first_column > 0;",
    ),
    (
        "values",
        32,
        r"INSERT INTO t(first_column, second_column) VALUES(100, 200),(300, 400) RETURNING first_column, second_column;",
        r"INSERT INTO t(
  first_column,
  second_column
)
VALUES (100, 200), (300, 400)
RETURNING
  first_column,
  second_column;",
    ),
    (
        "update",
        40,
        r"UPDATE table_name SET first_column = first_value + 1, second_column = second_value - 1 WHERE id = 10 RETURNING first_column, second_column;",
        r"UPDATE table_name
SET
  first_column = first_value + 1,
  second_column = second_value - 1
WHERE id = 10
RETURNING first_column, second_column;",
    ),
    (
        "upsert",
        40,
        r"INSERT INTO t(a,b) VALUES(1,2) ON CONFLICT(a) DO UPDATE SET a = excluded.a, b = excluded.b WHERE excluded.a > 0 RETURNING a,b;",
        r"INSERT INTO t(a, b)
VALUES (1, 2)
ON CONFLICT (a) DO UPDATE
SET a = excluded.a, b = excluded.b
WHERE excluded.a > 0
RETURNING a, b;",
    ),
    (
        "trigger",
        60,
        r"CREATE TRIGGER tr AFTER INSERT ON t BEGIN UPDATE t SET value = value + 1; INSERT INTO log VALUES(new.value); END;",
        r"CREATE TRIGGER tr AFTER INSERT ON t
BEGIN
  UPDATE t SET value = value + 1;
  INSERT INTO log VALUES (new.value);
END;",
    ),
    (
        "comments",
        32,
        r"SELECT f(first_argument, /* middle */ second_argument, -- last
 third_argument) FROM t; -- end
SELECT 2;",
        r"SELECT
  f(
    first_argument, /* middle */
    second_argument, -- last
    third_argument
  )
FROM t; -- end

SELECT 2;",
    ),
    (
        "type",
        40,
        r"CREATE TABLE t(a DECIMAL(10,2) DEFAULT -1, b VARCHAR(100));",
        r"CREATE TABLE t (
  a DECIMAL(10,2) DEFAULT -1,
  b VARCHAR(100)
);",
    ),
    (
        "long_alias",
        32,
        r"SELECT x FROM table_with_a_very_long_name alias_with_a_very_long_name;",
        r"SELECT x
FROM
  table_with_a_very_long_name
    alias_with_a_very_long_name;",
    ),
    (
        "view_header",
        32,
        r"CREATE VIEW a_view_with_a_long_name AS SELECT 1;",
        r"CREATE VIEW
  a_view_with_a_long_name AS
SELECT 1;",
    ),
    (
        "sort_modifiers",
        32,
        r"SELECT a FROM t ORDER BY long_ordering_column DESC NULLS LAST;",
        r"SELECT a
FROM t
ORDER BY
  long_ordering_column
    DESC NULLS LAST;",
    ),
    (
        "foreign_key",
        40,
        r"CREATE TABLE t(a INTEGER CONSTRAINT fk REFERENCES other_table(identifier) ON UPDATE CASCADE ON DELETE SET NULL DEFERRABLE INITIALLY DEFERRED);",
        r"CREATE TABLE t (
  a INTEGER
    CONSTRAINT fk
      REFERENCES other_table(identifier)
        ON UPDATE CASCADE
        ON DELETE SET NULL
    DEFERRABLE INITIALLY DEFERRED
);",
    ),
    (
        "trigger_header",
        40,
        r"CREATE TRIGGER long_trigger_name AFTER INSERT ON very_long_table_name WHEN new.some_column > 0 BEGIN SELECT 1; END;",
        r"CREATE TRIGGER long_trigger_name
AFTER INSERT ON very_long_table_name
WHEN new.some_column > 0
BEGIN
  SELECT 1;
END;",
    ),
];
