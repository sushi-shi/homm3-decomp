"""An unsupported PPC word must not hide the rest of the reference body."""
import unittest
from homm3.mac.__main__ import _disassembly_words


class DisassemblyCoverage(unittest.TestCase):
    def test_unsupported_word_does_not_truncate_following_return(self):
        from capstone import Cs, CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN
        data = bytes.fromhex("7c0802a6 fc010040 4e800020")
        rows = list(_disassembly_words(
            Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN), data, 0x148a68))
        self.assertEqual([row.address for row in rows], [0x148a68, 0x148a6c, 0x148a70])
        self.assertEqual(b"".join(row.bytes for row in rows), data)
        self.assertEqual(rows[-1].mnemonic, "blr")
        if rows[1].mnemonic == ".long":
            self.assertIn("fc010040", rows[1].op_str)
            self.assertIn("undecoded", rows[1].op_str)

    def test_raw_span_partial_tail_remains_visible(self):
        from capstone import Cs, CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN
        data = bytes.fromhex("4e800020 abcd")
        rows = list(_disassembly_words(
            Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN), data, 0x100))
        self.assertEqual(b"".join(row.bytes for row in rows), data)
        self.assertEqual(rows[-1].address, 0x104)
        self.assertEqual(rows[-1].mnemonic, ".byte")


if __name__ == "__main__":
    unittest.main()
