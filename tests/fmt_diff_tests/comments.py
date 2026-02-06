from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class TrailingLineComment(TestSuite):
    def test_end_of_statement(self):
        return AstTestBlueprint(
            sql="SELECT a FROM t -- trailing",
            out="SELECT a FROM t -- trailing",
        )

    def test_after_column(self):
        return AstTestBlueprint(
            sql="""\
                SELECT
                  a, -- first col
                  b
                FROM t
            """,
            out="""\
                SELECT
                  a, -- first col
                  b
                FROM t
            """,
        )

    def test_after_where(self):
        return AstTestBlueprint(
            sql="SELECT a FROM t WHERE x = 1 -- filter active",
            out="SELECT a FROM t WHERE x = 1 -- filter active",
        )


class LeadingLineComment(TestSuite):
    def test_before_statement(self):
        return AstTestBlueprint(
            sql="""\
                -- main query
                SELECT a FROM t
            """,
            out="""\
                -- main query
                SELECT a FROM t
            """,
        )

    def test_before_clause(self):
        return AstTestBlueprint(
            sql="""\
                SELECT a
                -- apply filter
                FROM t
                WHERE x = 1
            """,
            out="""\
                SELECT a
                -- apply filter
                FROM t
                WHERE x = 1
            """,
        )


class BlockComment(TestSuite):
    def test_before_statement(self):
        return AstTestBlueprint(
            sql="/* count */ SELECT count(*) FROM t",
            out="/* count */ SELECT count(*) FROM t",
        )

    def test_trailing_block(self):
        return AstTestBlueprint(
            sql="SELECT a /* col */ FROM t",
            out="SELECT a /* col */ FROM t",
        )


class MultipleComments(TestSuite):
    def test_two_trailing(self):
        return AstTestBlueprint(
            sql="""\
                SELECT
                  a, -- first
                  b -- second
                FROM t
            """,
            out="""\
                SELECT
                  a, -- first
                  b -- second
                FROM t
            """,
        )
