"""HoMM3 input adapters for the Gruntz data model."""
import os
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.core import msvc_names
from homm3.match import status
from homm3.retail_labels import data


class InputAdapterTests(unittest.TestCase):
    def test_vc6_array_and_file_static_names(self):
        self.assertEqual(msvc_names.data('?table@@3QBHB', internal=False,
                                         decorated=True), '?table@@3QBHB')
        self.assertEqual(msvc_names.data('_g_table', internal=True,
                                         decorated=True), '_g_table')
        self.assertEqual(msvc_names.mask('_local$S123'), '_local$S')
        self.assertNotEqual(msvc_names.mask('$S123'), msvc_names.mask('$S124'))

    def test_score_policy_reset_preserves_history_and_source_hash(self):
        row = status.MatchRow(95, 100, 100, 0x1234, 'tokens1:unchanged')
        old = {('test', 'f'): row}
        reset = status.policy_baseline(old, '# old policy\n')
        self.assertEqual(reset[('test', 'f')],
                         status.MatchRow(None, 0, 100, 0x1234, 'tokens1:unchanged'))
        self.assertEqual(status.policy_baseline(old,
            f'# score_policy={status.SCORE_POLICY}\n'), old)

    def test_type_cache_tracks_content_not_mtime_and_retains_errors(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / 'sample.cpp'; source.write_text('int x;')
            profiles = SimpleNamespace(project=SimpleNamespace(root=root),
                                       for_source=lambda _path: ['-m32'])
            answer = ({123: {'size': 4}}, ['explicit diagnostic'])
            with patch.object(data, '_uncached_declarations', return_value=answer) as parse:
                self.assertEqual(data.declarations(source, profiles), answer)
                self.assertEqual(data.declarations(source, profiles), answer)
                self.assertEqual(parse.call_count, 1)
                before = source.stat()
                source.write_text('int y;')
                os.utime(source, ns=(before.st_atime_ns, before.st_mtime_ns))
                self.assertEqual(data.declarations(source, profiles), answer)
                self.assertEqual(parse.call_count, 2)


if __name__ == '__main__':
    unittest.main()
