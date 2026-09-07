"""Compiler labels must preserve address, evidence and reference provenance."""
from pathlib import Path
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import Mock

from homm3.vc6 import disasm


class CompilerDisassembly(unittest.TestCase):
    def setUp(self):
        self.roles = {
            0x156f: disasm.Role(0x156f, "function", "cloneOperand", "docs/vc6/regalloc.md:1"),
            0xa0184: disasm.Role(0xa0184, "global", "operandSizes", "docs/vc6/regalloc.md:2"),
        }

    def test_selectors_retain_address_identity(self):
        for selector in ("cloneOperand", "0x156f", "156f", "0x1070156f", "FUN_1070156f"):
            self.assertEqual(disasm.resolve_selector(
                selector, self.roles, [(0x156f, "FUN_1070156f")]), 0x156f)
        with self.assertRaisesRegex(ValueError, "unknown"):
            disasm.resolve_selector("clone", self.roles)
        with self.assertRaisesRegex(ValueError, "ambiguous"):
            disasm.resolve_selector("cloneOperand", self.roles, [(0x9000, "cloneOperand")])
        for address in ("-1", "0x100000000"):
            with self.subTest(address=address), self.assertRaisesRegex(ValueError, "32-bit"):
                disasm.resolve_selector(address, self.roles)

    def test_only_real_references_get_roles(self):
        rows = [
            disasm.Instruction(0x1581, b"\xe8", "CALL 0x1070156f",
                               (disasm.Reference(0x156f, "call"),)),
            disasm.Instruction(0x1597, b"\x8b", "MOV ECX,dword ptr [EAX*0x4 + 0x107a0184]",
                               (disasm.Reference(0xa0184, "read"),)),
            # Address-looking immediate with no reference must remain literal.
            disasm.Instruction(0x1600, b"\xb8", "MOV EAX,0x1070156f"),
        ]
        text = disasm.render(rows, self.roles)
        self.assertIn("CALL cloneOperand", text)
        self.assertIn("[EAX*0x4 + operandSizes]", text)
        self.assertIn("rva 0x156f", text)
        self.assertIn("MOV EAX,0x1070156f", text)
        self.assertIn("Inferred roles, not original compiler symbols", text)
        self.assertIn("docs/vc6/regalloc.md:2", text)

    def test_local_branch_and_unknown_cold_block_keep_their_addresses(self):
        rows = [
            disasm.Instruction(0x156f, b"\xeb", "JZ 0x10701580",
                               (disasm.Reference(0x1580, "jump"),)),
            disasm.Instruction(0x1580, b"\xe9", "JMP 0x1077e45e",
                               (disasm.Reference(0x7e45e, "jump", "LAB_1077e45e"),)),
        ]
        text = disasm.render(rows, self.roles)
        self.assertIn("JZ loc_001580", text)
        self.assertIn("loc_001580:", text)
        self.assertIn("JMP LAB_1077e45e", text)
        self.assertNotIn("cloneOperand+", text)

    def test_references_do_not_replace_prefixes_inside_larger_numbers(self):
        row = disasm.Instruction(0x2000, b"", "MOV EAX,0x1070156f0",
                                  (disasm.Reference(0x156f, "address"),))
        self.assertIn("MOV EAX,0x1070156f0", disasm.render([row], self.roles))

    def test_local_range_rejects_reversed_or_missing_bounds(self):
        self.assertEqual(disasm.local_range("+0x10:+0x20"), (16, 32))
        for spec in ("+20:+10", "0:0", "-1:20", ":20", "0:", "0:1:2"):
            with self.subTest(spec=spec), self.assertRaises(ValueError):
                disasm.local_range(spec)

    def test_roles_are_read_from_owning_evidence_and_duplicates_fail(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            doc = root / "docs/vc6/regalloc.md"
            doc.parent.mkdir(parents=True)
            annotation = "<!-- c2-role: function 0x156f cloneOperand -->\n"
            doc.write_text("Supporting evidence.\n" + annotation)
            role = disasm.load_roles(root)[0x156f]
            self.assertEqual(role.evidence, "docs/vc6/regalloc.md:2")
            doc.write_text(annotation * 2)
            with self.assertRaisesRegex(ValueError, "duplicate"):
                disasm.load_roles(root)
            doc.write_text("<!-- c2-role: function 0x156f -->\n")
            with self.assertRaisesRegex(ValueError, "malformed"):
                disasm.load_roles(root)

    def test_real_document_annotations_are_valid(self):
        roles = disasm.load_roles()
        self.assertEqual(roles[0x156f].name, "cloneOperand")
        self.assertEqual(roles[0xac128].kind, "global")

    def test_instruction_bytes_must_match_the_pinned_file(self):
        from homm3.vc6 import atlas
        backend = atlas._load_script("c2_disasm")
        ins = Mock()
        ins.getAddress.return_value.getOffset.return_value = disasm.IMAGE_BASE + 0x156f
        ins.getBytes.return_value = [-125, -20, 8]
        ins.getReferencesFrom.return_value = []
        binary = SimpleNamespace(data=b"\x83\xec\x08", rva_to_off=lambda rva: 0)
        self.assertEqual(backend._row(Mock(), binary, ins).raw, binary.data)
        binary.data = b"\x90\x90\x90"
        with self.assertRaisesRegex(ValueError, "disagree with pinned DLL"):
            backend._row(Mock(), binary, ins)

    def test_ranges_do_not_decode_tables_or_split_instructions(self):
        from homm3.vc6 import atlas
        backend = atlas._load_script("c2_disasm")
        ins = Mock()
        ins.getAddress.return_value.getOffset.return_value = disasm.IMAGE_BASE + 0x156f
        ins.getLength.return_value = 3
        ins.getBytes.return_value = [-125, -20, 8]
        ins.getReferencesFrom.return_value = []
        program = Mock()
        listing = program.getListing.return_value
        listing.getInstructionAt.return_value = ins
        listing.getInstructions.return_value = [ins]
        # Only Ghidra's instruction is returned; trailing table bytes remain data.
        binary = SimpleNamespace(data=b"\x83\xec\x08\x90\x90", rva_to_off=lambda rva: 0)
        rows = backend.span_rows(program, binary, 0x156f, 0x1574)
        self.assertEqual([r.raw for r in rows], [b"\x83\xec\x08"])
        with self.assertRaisesRegex(ValueError, "use end 0x1572 to include it or 0x156f to exclude it"):
            backend.span_rows(program, binary, 0x156f, 0x1570)
        listing.getInstructionAt.return_value = None
        listing.getInstructionContaining.return_value = ins
        with self.assertRaisesRegex(ValueError, "Start at 0x156f to include it or 0x1572 to skip it"):
            backend.span_rows(program, binary, 0x1570, 0x1574)
        listing.getInstructionContaining.return_value = None
        with self.assertRaisesRegex(ValueError, "not a Ghidra instruction boundary"):
            backend.span_rows(program, binary, 0x1570, 0x1574)


if __name__ == "__main__":
    unittest.main()
