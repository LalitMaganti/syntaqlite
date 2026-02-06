from python.syntaqlite.diff_tests.testing import AstTestBlueprint, TestSuite


class SelectFormat(TestSuite):
    def test_stub(self):
        return AstTestBlueprint(
            sql="SELECT 1",
            out="/* TODO: formatter */",
        )
