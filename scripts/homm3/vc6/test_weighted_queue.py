import unittest

from homm3.match.status import MatchRow
from homm3.vc6.weighted_queue import ranked_rows


class WeightedQueueTests(unittest.TestCase):
    def test_covers_unclaimed_unscored_and_banked_dips_once_per_rva(self):
        report = {"units": [{"name": "game/example", "functions": [
            {"name": "exact", "fuzzy_match_percent": 100.0},
            {"name": "dip", "fuzzy_match_percent": 80.0},
            {"name": "alias", "fuzzy_match_percent": 80.0},
            {"name": "unscored"},
        ]}]}
        baseline = {
            ("example", "exact"): MatchRow(100, 100, 100, 1),
            ("example", "dip"): MatchRow(80, 100, 100, 2),
            ("example", "alias"): MatchRow(80, 100, 100, 2),
            ("example", "unscored"): MatchRow(None, 0, 0, 3),
        }
        sizes = {1: 900, 2: 500, 3: 100, 4: 200, 5: 1000}
        categories = {rva: "target" for rva in sizes}
        categories[5] = "runtime"
        rows = ranked_rows(report, baseline, categories, sizes, {})
        self.assertEqual([r["va"] for r in rows],
                         ["0x00400004", "0x00400003", "0x00400002"])
        self.assertEqual(rows[-1]["maximum"], 100)
        self.assertAlmostEqual(rows[-1]["remaining_bytes"], 100)

    def test_missing_checkpoint_identity_fails_instead_of_inventing_zero(self):
        with self.assertRaisesRegex(ValueError, "checkpoint RVA"):
            ranked_rows({"units": [{"name": "u", "functions": [
                {"name": "new", "fuzzy_match_percent": 100}]}]}, {}, {}, {}, {})


if __name__ == "__main__":
    unittest.main()
