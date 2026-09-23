"""A failed compile may leave a file; it must never receive a fresh stamp."""
import contextlib
import io
import os
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.core import cc_wrap
from homm3.build import compiled_freshness


class CompileProvenanceTest(unittest.TestCase):
    def test_failed_compiler_file_is_not_certified_and_inherited_flags_are_cleared(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, output = root/'a.cpp', root/'a.obj'
            source.write_text('int value;')
            toolchain = root/'msvc'
            (toolchain/'include').mkdir(parents=True)
            compiled_freshness.stamp_path(output).write_text('old stamp')
            def fail(cmd, out):
                self.assertEqual(os.environ['CL'], '')
                self.assertEqual(os.environ['_CL_'], '')
                out.write_bytes(b'incomplete object')
                return 'error', 1
            with contextlib.ExitStack() as stack:
                stack.enter_context(patch.dict(os.environ, {'CL': '/DWRONG', '_CL_': '/Od'}))
                stack.enter_context(patch('sys.argv', ['cc_wrap', '--src', str(source), '--out', str(output)]))
                stack.enter_context(patch.object(cc_wrap, 'HOMM3_DIR', root))
                stack.enter_context(patch.object(cc_wrap, 'msvc_dir', return_value=toolchain))
                stack.enter_context(patch.object(cc_wrap, 'find_ci', return_value=toolchain/'bin/cl.exe'))
                stack.enter_context(patch.object(cc_wrap.shutil, 'which', return_value='wine'))
                stack.enter_context(patch.object(cc_wrap, 'ensure_wineserver'))
                stack.enter_context(patch.object(cc_wrap, 'winepath_w', side_effect=str))
                stack.enter_context(patch.object(cc_wrap, '_run_cl', side_effect=fail))
                stack.enter_context(patch('homm3.core.project.Project', return_value=SimpleNamespace(includes=[])))
                stack.enter_context(patch.object(compiled_freshness, 'snapshot', return_value={}))
                stamp = stack.enter_context(patch.object(compiled_freshness, 'write'))
                stack.enter_context(contextlib.redirect_stderr(io.StringIO()))
                with self.assertRaises(SystemExit) as result:
                    cc_wrap.main()
                self.assertEqual(result.exception.code, 1)
                stamp.assert_not_called()
                self.assertFalse(compiled_freshness.stamp_path(output).exists())


if __name__ == '__main__':
    unittest.main()
