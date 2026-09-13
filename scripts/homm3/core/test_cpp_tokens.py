"""Comment cleanup must preserve tokens and matching history."""
import unittest

from homm3.core.cpp_tokens import code_tokens, fingerprint
from homm3.match.status import MatchRow, migrate_source_hashes, update_rows


class FingerprintTest(unittest.TestCase):
    def test_comments_and_formatting_preserve_history(self):
        before = 'int value() { /* probe */ return 1; // matched\n}'
        after = 'int value()\n{\n    return 1;\n}'
        self.assertEqual(fingerprint(before), fingerprint(after))
        key = ('unit', 'value')
        previous = {key: MatchRow(90, 100, 100, 0x1234, fingerprint(before))}
        rows, stats = update_rows({key: 90}, previous, {key: 0x1234},
                                  {key: fingerprint(after)})
        self.assertEqual(rows[key].max, 100)
        self.assertEqual(stats['reset'], 0)

    def test_real_edits_remain_distinct(self):
        for before, after in [
            ('return 1;', 'return 2;'),
            ('a + +b', 'a++b'),
            ('long value;', 'longvalue;'),
            ('"// old"', '"// new"'),
            ("' '", "'x'"),
            ('1e+2', '1e + 2'),
            ('#define F(x) x\n', '#define F (x) x\n'),
            ('#define A x\ny', '#define A x y\n'),
        ]:
            with self.subTest(before=before):
                self.assertNotEqual(fingerprint(before), fingerprint(after))

    def test_preprocessing_comments_and_continuations(self):
        self.assertEqual(code_tokens('#define A /*\ncomment\n*/ x\n'),
                         code_tokens('#define A x\n'))
        self.assertEqual(code_tokens('#define A x \\\n+ y\n'),
                         code_tokens('#define A x + y\n'))
        self.assertEqual(code_tokens('x; // hidden \\\nreturn 1;\ny;'),
                         code_tokens('x; y;'))
        self.assertNotEqual(code_tokens('#define F/**/(x) x\n'),
                            code_tokens('#define F(x) x\n'))

    def test_migration_requires_verified_unchanged_text(self):
        same, changed = ('unit', 'same'), ('unit', 'changed')
        previous = {key: MatchRow(80, 98, 100, rva, 'old-text')
                    for key, rva in [(same, 1), (changed, 2)]}
        hashes = {key: fingerprint('return 1;') for key in previous}
        migrated = migrate_source_hashes(previous, hashes,
                                         {same: 'old-text', changed: 'new-text'})
        rows, stats = update_rows({same: 80, changed: 80}, migrated,
                                  {same: 1, changed: 2}, hashes)
        self.assertEqual((rows[same].max, rows[same].hist), (98, 100))
        self.assertEqual((rows[changed].max, rows[changed].hist), (80, 100))
        self.assertEqual(stats['reset'], 1)


if __name__ == '__main__':
    unittest.main()
