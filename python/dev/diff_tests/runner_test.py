"""Regression checks for safe snapshot replacement."""
import ast
from pathlib import Path
import tempfile
import textwrap
from types import SimpleNamespace
import unittest

from python.dev.diff_tests.runner import _rewrite_test_file


class SnapshotReplacementTest(unittest.TestCase):
    def test_replaces_only_requested_literals(self):
        source = '''class Fixtures:
    def test_first(self):
        return Blueprint(sql="α", out="""\\
            old
        """)
    def test_second(self):
        return Blueprint(sql="β", out="unchanged")
'''
        actual = 'first\\path\nsecond """ quoted'
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture.py"
            path.write_text(source)
            method = SimpleNamespace(__qualname__="Fixtures.test_first")
            self.assertEqual(_rewrite_test_file(str(path), [(method, actual)]), 1)
            before, after = ast.parse(source), ast.parse(path.read_bytes())
            first_out = after.body[0].body[0].body[0].value.keywords[1].value
            self.assertEqual(textwrap.dedent(first_out.value).strip(), actual)
            first_out.value = "old\n"
            original_out = before.body[0].body[0].body[0].value.keywords[1].value
            first_out.value = original_out.value
            self.assertEqual(ast.dump(before), ast.dump(after))

    def test_missing_literal_does_not_modify_file(self):
        source = 'class Fixtures:\n    def test_first(self):\n        return Blueprint(out=compute())\n'
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture.py"
            path.write_text(source)
            method = SimpleNamespace(__qualname__="Fixtures.test_first")
            with self.assertRaises(ValueError):
                _rewrite_test_file(str(path), [(method, "replacement")])
            self.assertEqual(path.read_text(), source)
