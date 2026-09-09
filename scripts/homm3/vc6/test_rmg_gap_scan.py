"""Check gap enumeration against an independent linearized-ring oracle."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class RmgGapScanTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-gap-scan-hypotheses.py")
        self.source = (self.root / "src/rmg_terrain.cpp").read_text()

    def test_sixty_variants_rebase_and_reject_unknown_scan(self):
        payload = self.module.make_manifest(self.source)
        axis = payload["axes"][0]
        self.assertEqual(len({row["replace"] for row in axis["options"]}), 60)
        self.assertEqual(axis["find"], axis["options"][0]["replace"])
        with tempfile.TemporaryDirectory(prefix="rmg-gap-scan-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            parsed = self.module.hypotheses.parse_manifest(path, root=self.root)
            variants = self.module.hypotheses.variants(parsed[4], parsed[5])
        self.assertEqual(len(variants), 60)
        for variant in variants:
            changed = variant.source.decode()
            self.assertEqual(len(self.module._source.find_definitions(changed, self.module.FUNCTION)), 1)
            rebased = self.module.make_manifest(changed)["axes"][0]
            self.assertEqual(rebased["find"], rebased["options"][0]["replace"])
        with self.assertRaisesRegex(ValueError, "review the gap enumeration before"):
            self.module.make_manifest(self.source.replace(axis["find"], axis["find"].replace(
                "diagonal ? 1 : 2", "diagonal ? 2 : 1")))

    def test_ten_parents_cross_six_inner_loops_and_rebase(self):
        parents = [name for name, _ in self.module.scans()][:10]
        payload = self.module.make_inner_manifest(self.source, parents)
        with tempfile.TemporaryDirectory(prefix="rmg-gap-inner-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            parsed = self.module.hypotheses.parse_manifest(path, root=self.root)
            variants = self.module.hypotheses.variants(parsed[4], parsed[5])
        self.assertEqual(len(variants), 60)
        for variant in variants:
            changed = variant.source.decode()
            self.assertEqual(len(self.module._source.find_definitions(changed, self.module.FUNCTION)), 1)
            self.assertEqual(len(self.module.make_inner_manifest(changed, parents)["axes"][0]["options"]), 60)
        with self.assertRaisesRegex(ValueError, "review ten unique"):
            self.module.make_inner_manifest(self.source, parents[:-1])

    @unittest.skipUnless(shutil.which("g++"), "ring oracle needs g++")
    def test_every_nonempty_ring_matches_independent_oracle(self):
        header = (self.root / "include/rmg_terrain.h").read_text()
        start = header.index("struct TRmgTerrainGap {")
        gap = header[start:header.index("\n};", start) + 3]
        program = ["#include <cstring>\n", gap, "\nenum { TILE_DIR_COUNT = 8 };\n"]
        bodies = [(name + "+" + inner, body) for inner in self.module.INNER_LOOPS
                  for name, body in self.module.scans(inner)]
        self.assertEqual(len({body for _, body in bodies}), 360)
        # Also execute the current authored scan, including main's verified
        # outer break. Historical generated forms alone do not cover adoption.
        definition = self.module._source.find_definitions(self.source, self.module.FUNCTION)[0]
        function = self.source[definition.head:definition.body_close + 1]
        start = function.index(self.module.START)
        adopted = function[start:function.index(self.module.END, start)]
        if adopted not in {body for _, body in bodies}:
            bodies.append(("adopted", adopted))
        for index, (_, body) in enumerate(bodies):
            program += [f"unsigned int scan{index}(const unsigned char* matches, TRmgTerrainGap* gaps) {{\n",
                        "unsigned int gapCount = 0, first = 0;\nwhile (!matches[first]) ++first;\n",
                        body, "gapsBuilt:\nreturn gapCount;\n}\n"]
        program += ["typedef unsigned int (*Scan)(const unsigned char*, TRmgTerrainGap*);\nScan scans[] = {",
                    ",".join(f"scan{i}" for i in range(len(bodies))), "};\n", r"""
int main() {
    // Zero cannot reach this region: hasSeparatedNeighbours requires a
    // matching point and separated runs. Also cover the safe one-run masks.
    for (unsigned int mask = 1; mask != 256; ++mask)
    for (unsigned int truth = 1; truth <= 255; truth += 254) {
        unsigned char matches[8];
        unsigned int first = 8;
        for (unsigned int i = 0; i != 8; ++i) {
            matches[i] = (mask & (1 << i)) ? truth : 0;
            if (matches[i] && first == 8) first = i;
        }
        TRmgTerrainGap expected[4];
        std::memset(expected, 0xa5, sizeof(expected));
        unsigned int count = 0;
        bool inGap = false;
        for (unsigned int offset = 1; offset != 8; ++offset) {
            unsigned int direction = (first + offset) % 8;
            if (matches[direction]) { inGap = false; continue; }
            if (!inGap) {
                expected[count].m_weight = 0;
                expected[count].m_start = direction;
                expected[count].m_length = 0;
                ++count;
                inGap = true;
            }
            expected[count - 1].m_weight += (direction % 2 == 0) ? 2 : 1;
            ++expected[count - 1].m_length;
        }
        for (unsigned int variant = 0; variant != sizeof(scans) / sizeof(*scans); ++variant) {
            // Canary slots catch a scan that writes beyond the four gaps.
            TRmgTerrainGap actual[6];
            std::memset(actual, 0xa5, sizeof(actual));
            unsigned int result = scans[variant](matches, actual + 1);
            if (result != count || std::memcmp(actual + 1, expected, sizeof(expected))) return 1;
            const unsigned char* bytes = reinterpret_cast<const unsigned char*>(actual);
            for (unsigned int b = 0; b != sizeof(TRmgTerrainGap); ++b)
                if (bytes[b] != 0xa5 || bytes[5 * sizeof(TRmgTerrainGap) + b] != 0xa5) return 2;
            for (unsigned int i = 0; i != 8; ++i)
                if (matches[i] != ((mask & (1 << i)) ? truth : 0)) return 3;
        }
    }
    return 0;
}
"""]
        with tempfile.TemporaryDirectory(prefix="rmg-gap-scan-oracle-") as raw:
            path = Path(raw)
            cpp, executable = path / "scan.cpp", path / "scan"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
