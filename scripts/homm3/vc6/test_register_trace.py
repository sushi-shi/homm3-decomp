"""Temporary identities must not masquerade as recovered source locals."""
import unittest
from unittest.mock import patch

from homm3.vc6 import disasm, register_trace
from homm3.vc6.__main__ import _build_parser


class RegisterTraceTest(unittest.TestCase):
    def test_report_keeps_function_site_and_snapshot_provenance(self):
        slots = " ".join(f"{reg}=free" for reg in register_trace.REGISTERS)
        slots = slots.replace("ecx=free", "ecx=3:00000055")
        rows = ["sym abc ?caller", "register root=abc site=0003356e "
                "selected=ebx value=3:00000460 " + slots]
        roles = {0x3356e: disasm.Role(0x3356e, "site", "storeTemporaryBinding", "evidence.md:12")}
        with patch.object(disasm, "load_roles", return_value=roles), patch.object(
                register_trace.undname, "demangle", return_value={"?caller": "void caller(int)"}):
            text = register_trace.format_observations(rows)
        self.assertIn("Function: void caller(int)", text)
        self.assertIn("ebx <- temporary #460", text)
        self.assertIn("ecx=temporary #55", text)
        self.assertIn("storeTemporaryBinding (C2 RVA 0x3356e)", text)
        self.assertIn("evidence.md:12", text)
        self.assertIn("snapshot precedes the store", text)
        self.assertIn("not original source variables", text)

    def test_unknown_categories_and_missing_names_remain_explicit(self):
        slots = " ".join(f"{reg}=unreadable" for reg in register_trace.REGISTERS)
        rows = ["register root=abc site=00001234 selected=eax value=4:00000055 " + slots]
        with patch.object(disasm, "load_roles", return_value={}):
            text = register_trace.format_observations(rows)
        self.assertIn("<unresolved function abc>", text)
        self.assertIn("category-4 value #55", text)
        self.assertIn("unlabeled store (C2 RVA 0x1234)", text)
        self.assertIn("eax=unreadable", text)

    def test_cli_requires_a_function_and_manifest_unit(self):
        args = _build_parser().parse_args(["trace-registers", "rmg", "--fn", "createShipyardConnection"])
        self.assertEqual((args.unit, args.fn), ("rmg", "createShipyardConnection"))


if __name__ == "__main__":
    unittest.main()
