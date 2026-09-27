"""Keep the diagnostic link punch list faithful to VC6's actual symbols."""
import unittest
from homm3.build.link import unresolved_symbols


class UnresolvedSymbolsTest(unittest.TestCase):
    def test_cpp_declarations_do_not_collapse_to_the_first_word(self):
        output = '''a.obj : warning LNK4088: unrelated
x.obj : error LNK2001: unresolved external symbol "int g_a" (?g_a@@3HA)
y.obj : error LNK2001: unresolved external symbol "int g_b" (?g_b@@3HA)
z.obj : error LNK2001: unresolved external symbol "public: void __thiscall C::f(void)" (?f@C@@QAEXXZ)
x.obj : error LNK2001: unresolved external symbol _malloc
x.obj : error LNK2001: unresolved external symbol _malloc
'''
        self.assertEqual(unresolved_symbols(output),
                         ['?f@C@@QAEXXZ', '?g_a@@3HA', '?g_b@@3HA', '_malloc'])

    def test_unknown_diagnostic_spelling_is_preserved(self):
        self.assertEqual(unresolved_symbols('error: unresolved external symbol "int x"'),
                         ['"int x"'])
