"""Call counts must retain callee identity and coverage uncertainty."""
import unittest

from homm3.mac import calls
from homm3.mac.object import ObjectError
from homm3.mac.relocations import Address


class TestMacCalls(unittest.TestCase):
    def view(self, words, **kwargs):
        return calls.analyze(bytes.fromhex(words), Address(0, 0x100),
                             {".a": Address(0, 0x200), ".b": Address(0, 0x300)}, **kwargs)

    def test_direct_indirect_conditional_and_returns(self):
        view = self.view("48000101 4e800421 4e800021 418200f5 4e800020")
        self.assertEqual((view["total"], view["direct"], view["indirect"]), (4, 2, 2))
        self.assertEqual(view["sites"][3]["condition"], "bo=12,bi=2")
        self.assertEqual(calls.compare(view, view)["state"], "indirect_targets_unknown")

    def test_equal_counts_do_not_hide_wrong_or_reordered_callees(self):
        retail = self.view("48000101 480001fd 4e800020")
        candidate = self.view("48000201 480000fd 4e800020")
        comparison = calls.compare(retail, candidate)
        self.assertEqual(comparison["delta"], 0)
        self.assertEqual(comparison["state"], "call_target_difference")
        self.assertTrue(comparison["differences"])

    def test_relocation_names_before_linking_and_reload_collapse(self):
        retail = self.view("48000101 480001fd 4e800020")
        candidate = self.view("48000001 60000000 48000001 60000000 4e800020", xrefs=(
            (0, "HUNK_XREF_24BIT", ".a"), (8, "HUNK_XREF_24BIT", ".b")))
        self.assertEqual(calls.compare(retail, candidate)["state"], "call_sequence_agrees")
        self.assertEqual(candidate["offset_basis"], "object")
        self.assertEqual([s["offset"] for s in candidate["sites"]], [0, 8])

    def test_missing_call_names_the_actionable_difference(self):
        comparison = calls.compare(self.view("48000101 480001fd 4e800020"),
                                   self.view("48000101 4e800020"))
        self.assertEqual(comparison["delta"], -1)
        self.assertEqual(comparison["state"], "call_count_difference")
        self.assertEqual(comparison["differences"][0]["retail"][0]["symbol"], ".b")

    def test_unresolved_call_is_counted_without_inventing_address(self):
        candidate = self.view("48000001 4e800020", xrefs=((0, "HUNK_XREF_24BIT", ".unknown"),))
        comparison = calls.compare(self.view("48000101 4e800020"), candidate)
        self.assertEqual(candidate["total"], 1)
        self.assertEqual(candidate["unresolved_direct"], 1)
        self.assertEqual(candidate["sites"][0]["target"], "symbol:.unknown")
        self.assertEqual(comparison["state"], "unresolved_call_targets")

    def test_local_link_and_external_branches_are_separate(self):
        view = self.view("48000005 480000fc 4e800420 4e800020")
        self.assertEqual(view["total"], 0)
        self.assertEqual(len(view["local_link_branches"]), 1)
        self.assertEqual(len(view["external_branches"]), 2)
        self.assertEqual(self.view("48000001 4e800020")["total"], 1)

    def test_missing_candidate_is_not_zero_calls(self):
        comparison = calls.compare(self.view("4e800020"), None, error="compile failed")
        self.assertIsNone(comparison["delta"])
        self.assertIsNone(comparison["candidate"])
        self.assertEqual(calls.totals([comparison])["candidate"]["functions"], 0)
        self.assertIn("unavailable", calls.summary(comparison))

    def test_partial_instruction_and_bad_reference_are_rejected(self):
        with self.assertRaises(ObjectError):
            self.view("480001")
        with self.assertRaises(ObjectError):
            self.view("48000001", xrefs=((0, "HUNK_XREF_16BIT", ".a"),))


if __name__ == "__main__":
    unittest.main()
