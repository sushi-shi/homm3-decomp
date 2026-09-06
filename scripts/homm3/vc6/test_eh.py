"""Missing exception frames must remain visible to the diagnosis router."""
from pathlib import Path
import unittest
from unittest.mock import patch

from homm3.vc6 import _eh, diagnose, reg_model


PLAIN = "push ebp\nmov ebp, esp\nmov dword ptr [ebp - 0x4], 0x0\nret\n"
CATCH = """mov eax, dword ptr fs:[0x0]
mov dword ptr [ebp - 0x4], ebx
mov byte ptr [ebp - 0x4], 0x1
mov byte ptr [ebp - 0x4], 0x2
mov byte ptr [ebp - 0x4], 0x1
"""


class ExceptionFrameTest(unittest.TestCase):
    def setUp(self):
        self.enterContext(patch.object(reg_model, "_resolve_symbol", return_value="fn"))

    def test_readable_body_without_frame_has_no_eh_states(self):
        with patch.object(_eh._asm, "objdump", return_value=PLAIN):
            self.assertEqual(_eh.state_sequence(Path("unit.obj"), "fn"), [])

    def test_missing_or_empty_disassembly_is_unknown(self):
        for result in ({"side_effect": SystemExit(2)}, {"return_value": ""}):
            with self.subTest(result=result), patch.object(_eh._asm, "objdump", **result):
                self.assertIsNone(_eh.state_sequence(Path("unit.obj"), "fn"))

    def test_missing_frame_routes_before_inline_spelling(self):
        # Artifact's old body had no frame; retail has the catch transcript.
        with patch.object(_eh._asm, "objdump", side_effect=[PLAIN, CATCH]):
            diff = _eh.divergence("unit", "fn")
        self.assertEqual((diff["base"], diff["target"]), ([], [None, 1, 2, 1]))
        self.assertEqual(diff["kind"], "COUNT")
        self.assertIn("catch scope", diff["note"])
        with patch.object(diagnose.report, "_diagnose_one", return_value={
                "class": "inliner", "reg_dist": 1, "flow_dist": 1}), \
                patch.object(diagnose, "_inline_divergence", return_value="under-inline"), \
                patch.object(_eh, "divergence", return_value=diff):
            routes = diagnose.route("unit", "fn")[3]
        self.assertIsNone(routes[0][0])
        self.assertIn("EH cleanup", routes[0][1])
        self.assertEqual(routes[1][0], "predict-inline")

    def test_extra_frame_is_reported_and_two_plain_bodies_agree(self):
        with patch.object(_eh._asm, "objdump", side_effect=[CATCH, PLAIN]):
            self.assertEqual(_eh.divergence("unit", "fn")["kind"], "COUNT")
        with patch.object(_eh._asm, "objdump", return_value=PLAIN):
            self.assertIsNone(_eh.divergence("unit", "fn"))


if __name__ == "__main__":
    unittest.main()
