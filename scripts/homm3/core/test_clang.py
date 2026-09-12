import unittest

from homm3.core.clang import _fstream


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


if __name__ == '__main__':
    unittest.main()
