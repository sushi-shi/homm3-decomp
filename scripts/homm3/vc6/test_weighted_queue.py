import unittest

from homm3.match.status import MatchRow
from homm3.vc6.weighted_queue import ranked_rows


class WeightedQueueTests(unittest.TestCase):
    def test_hist_does_not_exclude_current_implementation_headroom(self):
        report = {"units": [{"name": "game/example", "functions": [
            {"name": "exact", "fuzzy_match_percent": 100.0},
            {"name": "dip", "fuzzy_match_percent": 80.0},
            {"name": "alias", "fuzzy_match_percent": 90.0},
            {"name": "unscored"},
            {"name": "stub"},
        ]}]}
        baseline = {
            ("example", "exact"): MatchRow(100, 100, 100, 1),
            ("example", "dip"): MatchRow(80, 80, 100, 2),
            ("example", "alias"): MatchRow(90, 90, 90, 2),
            ("example", "unscored"): MatchRow(None, 30, 30, 3),
            ("example", "stub"): MatchRow(None, 0, 0, 4),
        }
        sizes = {1: 900, 2: 500, 3: 100, 4: 200, 5: 1000}
        categories = {rva: "target" for rva in sizes}
        compiled = set(baseline) - {("example", "stub")}
        rows = ranked_rows(report, baseline, categories, sizes, compiled)
        self.assertEqual([r["va"] for r in rows],
                         ["0x00400003", "0x00400002"])
        self.assertEqual(rows[0]["state"], "admitted")
        self.assertIsNone(rows[0]["current"])
        self.assertAlmostEqual(rows[0]["remaining_bytes"], 70)
        self.assertEqual(rows[1]["historical"], 100)
        self.assertAlmostEqual(rows[1]["remaining_bytes"], 50)

    def test_orders_by_peak_then_size_and_deduplicates_aliases(self):
        report = {"units": [{"name": "u", "functions": [
            {"name": "low", "fuzzy_match_percent": 50},
            {"name": "dip", "fuzzy_match_percent": 10},
            {"name": "small", "fuzzy_match_percent": 40},
            {"name": "alias", "fuzzy_match_percent": 20},
            {"name": "runtime", "fuzzy_match_percent": 0},
        ]}]}
        baseline = {
            ("u", "low"): MatchRow(50, 50, 50, 1),
            ("u", "dip"): MatchRow(10, 90, 90, 2),
            ("u", "small"): MatchRow(40, 40, 50, 3),
            ("u", "alias"): MatchRow(20, 20, 20, 1),
            ("u", "runtime"): MatchRow(0, 0, 0, 4),
        }
        sizes = {1: 200, 2: 900, 3: 100, 4: 1000}
        rows = ranked_rows(report, baseline,
                           {1: "target", 2: "target", 3: "zlib", 4: "runtime"},
                           sizes, set(baseline))
        self.assertEqual([r["function"] for r in rows], ["small", "low", "dip"])
        self.assertAlmostEqual(rows[-1]["remaining_bytes"], 90)

    def test_missing_checkpoint_identity_fails_instead_of_inventing_zero(self):
        with self.assertRaisesRegex(ValueError, "checkpoint RVA"):
            ranked_rows({"units": [{"name": "u", "functions": [
                {"name": "new", "fuzzy_match_percent": 100}]}]},
                {}, {}, {}, {("u", "new")})


if __name__ == "__main__":
    unittest.main()
