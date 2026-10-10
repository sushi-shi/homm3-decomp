import tempfile
import threading
import time
import unittest
from pathlib import Path
from unittest.mock import patch

from homm3.core import clang
from homm3.core.clang import _fstream, _functional


class PublishTests(unittest.TestCase):
    """Concurrent clang tools share the generated include trees."""

    def populate_counted(self, calls):
        def populate(tmp):
            calls.append(tmp)
            time.sleep(0.05)
            (tmp / 'header.h').write_text('int x;\n')
        return populate

    def test_concurrent_regeneration_never_exposes_a_missing_tree(self):
        with tempfile.TemporaryDirectory() as tmp:
            destination = Path(tmp) / 'msvc-include'
            destination.mkdir()
            (destination / 'header.h').write_text('int old;\n')
            (destination / '.mirror-stamp').write_text('old\n')
            calls, misses, stop = [], [], threading.Event()

            def read():
                while not stop.is_set():
                    if not (destination / 'header.h').is_file():
                        misses.append(time.monotonic())
            reader = threading.Thread(target=read)
            reader.start()
            writers = [threading.Thread(target=clang._publish,
                                        args=(destination, 'new\n', self.populate_counted(calls)))
                       for _ in range(4)]
            for writer in writers:
                writer.start()
            for writer in writers:
                writer.join()
            stop.set()
            reader.join()
            self.assertEqual(len(calls), 1)  # the others waited and found it fresh
            self.assertEqual(misses, [])
            self.assertEqual((destination / '.mirror-stamp').read_text(), 'new\n')
            self.assertEqual((destination / 'header.h').read_text(), 'int x;\n')
            self.assertEqual(sorted(p.name for p in Path(tmp).iterdir()),
                             ['.msvc-include.lock', 'msvc-include'])

    def test_fresh_tree_is_not_rebuilt_and_fallback_swap_leaves_no_temporaries(self):
        with tempfile.TemporaryDirectory() as tmp:
            destination = Path(tmp) / 'msvc-include'
            calls = []
            clang._publish(destination, 'a\n', self.populate_counted(calls))
            clang._publish(destination, 'a\n', self.populate_counted(calls))
            self.assertEqual(len(calls), 1)
            with patch.object(clang, '_exchange', return_value=False):
                clang._publish(destination, 'b\n', self.populate_counted(calls))
            self.assertEqual(len(calls), 2)
            self.assertEqual((destination / '.mirror-stamp').read_text(), 'b\n')
            self.assertEqual(sorted(p.name for p in Path(tmp).iterdir()),
                             ['.msvc-include.lock', 'msvc-include'])

    def test_mirror_stamp_follows_the_header_source(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / 'project'
            first, second = Path(tmp) / 'vc6', Path(tmp) / 'vc4'
            for toolchain, text in ((first, 'vc6\n'), (second, 'vc4\n')):
                (toolchain / 'include').mkdir(parents=True)
                (toolchain / 'include/STDDEF.H').write_text(text)
            header = root / 'build/gen/msvc-include/stddef.h'
            stamp = root / 'build/gen/msvc-include/.mirror-stamp'
            self.assertEqual(clang.mirror(root, first), root / 'build/gen/msvc-include')
            self.assertEqual(header.read_text(), 'vc6\n')
            self.assertEqual(stamp.read_text().splitlines()[0],
                             str((first / 'include').resolve()))
            clang.mirror(root, second)
            self.assertEqual(header.read_text(), 'vc4\n')
            inode = stamp.stat().st_ino
            clang.mirror(root, second)
            self.assertEqual(stamp.stat().st_ino, inode)


class FstreamMirrorTests(unittest.TestCase):
    def test_flags_are_qualified_but_codecvt_calls_keep_their_identity(self):
        original = """openmode mode = in | out | trunc;
setstate(failbit);
_Pcvt->out(state, first, last, next);
_Pcvt -> in(state, first, last, next);
cvt . out(state, first, last, next);
ios_base::in;
"""
        expected = """openmode mode = ios_base::in | ios_base::out | ios_base::trunc;
setstate(ios_base::failbit);
_Pcvt->out(state, first, last, next);
_Pcvt -> in(state, first, last, next);
cvt . out(state, first, last, next);
ios_base::in;
"""
        self.assertEqual(_fstream(original), expected)
        self.assertEqual(_fstream(expected), expected)


class FunctionalMirrorTests(unittest.TestCase):
    def test_dependent_argument_cast_instantiates(self):
        import clang.cindex as cx
        source = """
template<class _Bfn, class T>
typename _Bfn::second_argument_type boundValue(const T& value) {
    return _Bfn::second_argument_type(value);
}
struct Predicate { typedef bool second_argument_type; };
bool probe() { return boundValue<Predicate>(1); }
"""
        def errors(text):
            tu = cx.Index.create().parse(
                'probe.cpp', args=['-x', 'c++', '-std=c++98', '-fms-extensions'],
                unsaved_files=[('probe.cpp', text)])
            return [str(d) for d in tu.diagnostics if d.severity >= cx.Diagnostic.Error]
        self.assertTrue(errors(source))
        patched = _functional(source)
        self.assertEqual(errors(patched), [])
        self.assertEqual(_functional(patched), patched)


if __name__ == '__main__':
    unittest.main()
