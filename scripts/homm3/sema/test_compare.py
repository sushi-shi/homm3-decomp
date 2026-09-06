"""Pre-claim comparisons preserve local branches and normalize relocated fields."""
import struct
from types import SimpleNamespace
import unittest

from homm3.sema import _asm, compare, diff


class ReferenceTest(unittest.TestCase):
    def context(self):
        symbols = SimpleNamespace(
            funcs={0x2000: ('callee', 'u', 1, '')},
            datas={0x3000: ('global', 'u', 4, '')},
            owner=lambda addr: None)
        return SimpleNamespace(image=SimpleNamespace(image_base=0x400000),
                               symbols=symbols, relocs=[(0x1001, 0x403000)])

    def test_call_and_data_addends_match_coff_while_local_branch_stays_relative(self):
        text = ('401000: a1 00 30 40 00\tmov\teax, dword ptr [0x403000]\n'
                '401005: e8 f6 0f 00 00\tcall\t0x402000\n'
                '40100a: 74 00\tje\t0x40100c\n'
                '40100c: c3\tret\n')
        result = compare._reference(self.context(), 0x1000, 13, text)
        rows = _asm.reloc_rows(result)
        self.assertEqual(rows[0][1], bytes.fromhex('a100000000'))
        self.assertEqual(rows[1][1], bytes.fromhex('e800000000'))
        self.assertEqual(rows[2][1], bytes.fromhex('7400'))
        self.assertIn('IMAGE_REL_I386_DIR32\tglobal', result)
        self.assertIn('IMAGE_REL_I386_REL32\tcallee', result)
        self.assertEqual([r[2] for r in diff._ref_seq(result)[0]], ['global', 'callee'])

    def test_small_literals_keep_their_value_and_unknown_addresses_stay_explicit(self):
        text = '401000: 6a 04\tpush\t4\n401002: c3\tret\n'
        result = compare._reference(self.context(), 0x1000, 3, text)
        self.assertIn('push\t0x4', result)
        self.assertNotIn('IMAGE_REL', result)


if __name__ == '__main__':
    unittest.main()
