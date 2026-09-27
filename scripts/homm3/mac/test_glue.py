"""CFM calls must preserve reloads, loader identity and indirect uncertainty."""
import struct
import unittest

from homm3.mac import calls, glue
from homm3.mac.object import CodeHunk, ObjectError
from homm3.mac.pef import PEF
from homm3.mac.relocations import Address, CallTarget, link_code
from homm3.mac.test_loader import container


class TestMacGlue(unittest.TestCase):
    def fixture(self, *, addend=0, pointer=True, duplicate=False):
        data = bytearray(64)
        struct.pack_into(">III", data, 0, 0, 0x20, addend)
        pef = container(bytes(data), (0x4600, 0x6000) if pointer else (0x4600,),
                        imports=("callee",))
        raw = bytearray(pef.data)
        start = pef.section(0).file_offset
        stub = bytes.fromhex("8182ffe8 90410014 800c0000 804c0004 7c0903a6 4e800420")
        raw[start:start + len(stub)] = stub
        if duplicate:
            raw[start + 32:start + 32 + len(stub)] = stub
        return PEF(bytes(raw))

    def test_import_identity_requires_loader_pointer_and_unique_stub(self):
        target = glue.imports(self.fixture())[".callee"]
        self.assertEqual(target, CallTarget(Address(0, 0), "import", "import:Library:callee"))
        self.assertEqual(glue.imports(self.fixture(addend=4)), {})
        self.assertEqual(glue.imports(self.fixture(pointer=False)), {})
        self.assertEqual(glue.imports(self.fixture(duplicate=True)), {})

    def test_import_and_dynamic_calls_restore_toc_instead_of_collapsing(self):
        # Shape checked against actual MWLink control, including a neighboring
        # ordinary call whose reload slot disappears and shifts the final branch.
        hunk = CodeHunk(".caller", bytes.fromhex(
            "48000001 60000000 48000001 60000000 48000001 60000000 4e800020"),
            ((0, "HUNK_XREF_24BIT", ".local"), (8, "HUNK_XREF_24BIT", ".import"),
             (16, "HUNK_XREF_24BIT", ".__ptr_glue")))
        targets = {".local": Address(0, 0x200),
                   ".import": CallTarget(Address(0, 0x300), "import", "import:Lib:fn"),
                   ".__ptr_glue": CallTarget(Address(0, 0x400), "indirect_tvector")}
        result = link_code(hunk, Address(0, 0x100), targets, collapse_reloads=True)
        self.assertEqual(result.data, bytes.fromhex(
            "48000101 480001fd 80410014 480002f5 80410014 4e800020"))
        self.assertEqual(result.removed_reload_slots, (4,))
        self.assertEqual(result.restored_reload_slots, (12, 20))
        raw = calls.analyze(hunk.data, Address(0, 0x100), targets, xrefs=hunk.xrefs)
        linked = calls.analyze(result.data, Address(0, 0x100), targets)
        self.assertEqual((raw["direct"], raw["indirect"]), (2, 1))
        self.assertEqual(raw["sites"][1]["target"], "import:Lib:fn")
        self.assertIsNone(raw["sites"][2]["target"])
        self.assertEqual(raw["sites"][2]["physical_target"], "mac:0:0x400")
        self.assertEqual(calls.compare(raw, linked)["state"], "indirect_targets_unknown")

    def test_changing_toc_requires_an_unowned_reload_slot(self):
        target = {".glue": CallTarget(Address(0, 0x100), "indirect_tvector")}
        for code, refs in (
            ("48000001 4e800020", ((0, "HUNK_XREF_24BIT", ".glue"),)),
            ("48000000 60000000", ((0, "HUNK_XREF_24BIT", ".glue"),)),
            ("48000001 60000000", ((0, "HUNK_XREF_24BIT", ".glue"),
                                  (4, "HUNK_XREF_24BIT", ".glue")))):
            with self.subTest(code=code), self.assertRaises(ObjectError):
                link_code(CodeHunk(".caller", bytes.fromhex(code), refs), Address(0, 0),
                          target, collapse_reloads=True)


if __name__ == "__main__":
    unittest.main()
