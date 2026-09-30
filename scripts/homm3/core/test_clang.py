import unittest

from homm3.core.clang import _fstream, _functional


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
