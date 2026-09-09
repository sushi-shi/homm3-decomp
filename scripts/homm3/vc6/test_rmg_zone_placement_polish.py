"""Validate the signed distance/lifetime matrix without a squared-threshold shortcut."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class ZonePlacementPolishTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-zone-placement-polish-hypotheses.py")
        self.source = (self.root / "src/rmg.cpp").read_text()

    def test_sixty_fragments_rebase_without_touching_predicate_guards(self):
        axis = self.module.make_manifest(self.source)["axes"][0]
        self.assertEqual(axis["find"], axis["options"][0]["replace"])
        self.assertEqual(len({row["replace"] for row in axis["options"]}), 60)
        for row in axis["options"]:
            fragment = row["replace"]
            self.assertEqual(fragment.count("getLevelPosition()"), 1)
            self.assertEqual(fragment.count("sqrt("), 1)
            self.assertNotIn("#pragma", fragment)
            changed = self.source.replace(axis["find"], fragment)
            self.assertEqual(changed.replace(fragment, axis["find"]), self.source)
            self.assertEqual(self.module.make_manifest(changed)["axes"][0]["options"][0]["replace"], fragment)
        with self.assertRaisesRegex(ValueError, "review the zone-distance block"):
            self.module.make_manifest(self.source.replace(axis["find"], axis["find"].replace("sqrt(", "fabs(")))

    @unittest.skipUnless(shutil.which("g++"), "signed distance oracle needs g++")
    def test_all_variants_keep_signed_distance_truncation_and_one_position_query(self):
        program = ["#include <cmath>\nusing std::sqrt;\n", r"""
struct TRmgMapPosition { int m_x, m_y, m_z; };
struct TRmgZone {
    TRmgMapPosition m_position;
    int m_queries;
    TRmgMapPosition getLevelPosition() { ++m_queries; return m_position; }
};
int integerRoot(unsigned int squared) {
    unsigned int lower = 0, upper = 2048;
    while (lower + 1 < upper) {
        unsigned int middle = (lower + upper) / 2;
        if (middle * middle <= squared) lower = middle; else upper = middle;
    }
    return lower;
}
"""]
        for index, (_, fragment) in enumerate(self.module.fragments()):
            program += [f"int distance{index}(const TRmgMapPosition& position, TRmgZone* otherZone) {{\n",
                        fragment, "return distance;\n}\n"]
        program += ["typedef int (*Distance)(const TRmgMapPosition&, TRmgZone*);\n",
                    "Distance variants[] = {" + ",".join(f"distance{index}" for index in range(60)) + "};\n", r"""
int main() {
    const int coordinates[] = {-512, -144, -10, -1, 0, 1, 2, 9, 50, 144, 512};
    TRmgMapPosition position; position.m_z = 0;
    TRmgZone zone; zone.m_position.m_z = 0;
    for (int x = 0; x != 11; ++x)
    for (int y = 0; y != 11; ++y)
    for (int otherX = 0; otherX != 11; ++otherX)
    for (int otherY = 0; otherY != 11; ++otherY) {
        position.m_x = coordinates[x]; position.m_y = coordinates[y];
        zone.m_position.m_x = coordinates[otherX]; zone.m_position.m_y = coordinates[otherY];
        int dx = position.m_x - zone.m_position.m_x, dy = position.m_y - zone.m_position.m_y;
        int expected = integerRoot(dx * dx + dy * dy);
        for (int variant = 0; variant != 60; ++variant) {
            zone.m_queries = 0;
            if (variants[variant](position, &zone) != expected || zone.m_queries != 1) return variant + 1;
            zone.m_queries = 0;
            if (variants[variant](zone.m_position, &zone) != 0 || zone.m_queries != 1) return variant + 1;
        }
    }
    return 0;
}
"""]
        with tempfile.TemporaryDirectory(prefix="rmg-zone-placement-polish-test-") as raw:
            path = Path(raw)
            cpp, executable = path / "distance.cpp", path / "distance"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
