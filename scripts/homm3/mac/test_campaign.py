import unittest

from homm3.mac.campaign import plan


class TestMacCampaign(unittest.TestCase):
    def test_wave_keeps_unit_ownership_and_setup_separate(self):
        rows = [dict(unit=unit, state=state, dispatchable=ready, size=size,
                     windows_max=score, retail_va=va, source=f"src/{unit}.cpp")
                for unit, state, ready, size, score, va in (
                    ("paired", "mac_instruction_difference", True, 100, 100, "0x1"),
                    ("small", "pairing_needed", False, 20, 98, "0x2"),
                    ("small", "pairing_needed", False, 40, 99, "0x3"),
                    ("deferred", "deferred", False, 4, 99, "0x4"),
                    ("later", "pairing_needed", False, 300, 80, "0x5"))]
        report = plan(dict(rows=rows, analysis_sha256="tooling"), workers=2)
        self.assertEqual([p["unit"] for p in report["packets"]], ["paired", "small"])
        self.assertEqual(report["packets"][1]["phase"], "establish_mac_evidence")
        assigned = [r["retail_va"] for p in report["packets"] for r in p["initial_targets"]]
        self.assertEqual(len(assigned), len(set(assigned)))
        self.assertEqual({r["unit"] for r in report["unassigned_tasks"]}, {"deferred", "later"})
        for units in (["small", "small"], ["deferred"], ["unknown"]):
            with self.assertRaises(ValueError):
                plan(dict(rows=rows, analysis_sha256="tooling"), units=units)


if __name__ == "__main__":
    unittest.main()
