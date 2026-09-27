"""An unsupported PPC word must not hide the rest of the reference body."""
import unittest
import contextlib
import io
from types import SimpleNamespace
from unittest.mock import patch
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

    def test_claimed_retail_disassembly_does_not_resolve_candidates(self):
        from homm3.mac import __main__ as cli
        from homm3.mac.relocations import Address
        target = SimpleNamespace(mac_section=0, mac_offset=0x100, mac_size=4)
        pef = SimpleNamespace(code=lambda *args: bytes.fromhex("4e800020"))
        loader = SimpleNamespace(toc=lambda: Address(1, 0x8000), pointers={})
        with patch.object(cli, "_select", return_value=target), \
                patch.object(cli, "_image", return_value=pef), \
                patch.object(cli, "_raw_code_names", return_value={}) as names, \
                patch.object(cli, "Loader", return_value=loader), \
                patch.object(cli, "load_data", return_value=[]), \
                patch.object(cli.symbols, "addresses", side_effect=AssertionError("candidate join")), \
                patch.object(cli.build, "objects", side_effect=AssertionError("candidate build")), \
                contextlib.redirect_stdout(io.StringIO()) as output:
            self.assertEqual(cli.main(["disasm", "0x00400100"]), 0)
        names.assert_called_once()
        self.assertIn("00000100: 4E800020", output.getvalue())
        self.assertIn("blr", output.getvalue())

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
