# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import LspDiffTestBlueprint, TestSuite


class Hover(TestSuite):
    def test_on_ddl_table(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER, name TEXT); SELECT * FROM <|>users;",
            op="hover",
            out="""\
**table** `users`

```
id, name
```
""",
        )

    def test_on_ddl_column(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER, name TEXT);\nSELECT <|>name FROM users;",
            op="hover",
            out="""\
**column** in `users`

id, **name**
""",
        )

    def test_on_builtin_function(self):
        return LspDiffTestBlueprint(
            sql="SELECT <|>count(*);",
            op="hover",
            out="""\
            **window function**

            ```
            count()
            count(arg1)
            ```
""",
        )

    def test_on_unknown_identifier_returns_none(self):
        return LspDiffTestBlueprint(
            sql="SELECT * FROM <|>nonexistent;",
            op="hover",
            out="(no hover)",
        )
