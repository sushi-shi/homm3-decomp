import tempfile
import unittest
from pathlib import Path

from homm3.analysis.link_order import parse_unit


class OriginBoundaryTest(unittest.TestCase):
    def parse(self, source):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'owner.cpp'
            path.write_text(source)
            return parse_unit(path)

    def test_unclaimed_inline_cannot_anchor_following_comdat(self):
        source = r'''
// E:\gamedcs\owner.cpp:368
// Before normalization: SourceHelper.
static inline void helper()
{
    use();
}
#if 0
VA(0x0048e220, 0x2c6)
void unrelatedLibraryBody() {}
#endif
// E:\gamedcs\owner.cpp:230
VA(0x00577360, 0x1c5)
void ownedBody() {}
'''
        self.assertEqual(self.parse(source), [('owner.cpp', 230, 'VA', 0x577360, 0x1c5)])

    def test_unclaimed_prototype_consumes_its_origin(self):
        self.assertEqual(self.parse(r'''
// E:\gamedcs\owner.cpp:20
void prototype();
VA(0x00401000, 16)
void anotherBody() {}
'''), [])

    def test_comments_and_template_wrappers_preserve_valid_origin(self):
        self.assertEqual(self.parse(r'''
// E:\gamedcs\owner.cpp:42
/* Commentary with { and ; does not start a declaration. */
#define WRAPPER() { ignored(); }
#if 0
template<class T>
VA(0x00402000, 32)
void ownedTemplate() {}
#endif
// E:\gamedcs\shared.h
DC_ONLY(0x1234, 12)
void sharedBody() {}
'''), [('owner.cpp', 42, 'VA', 0x402000, 32), ('shared.h', 0, 'DC_ONLY', 0x1234, 12)])

    def test_claim_text_inside_literal_cannot_consume_an_origin(self):
        self.assertEqual(self.parse(r'''
// E:\gamedcs\owner.cpp:10
const char* text = "VA(0x00403000, 16)";
VA(0x00404000, 16)
void anotherBody() {}
'''), [])


if __name__ == '__main__':
    unittest.main()
