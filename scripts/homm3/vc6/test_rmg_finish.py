"""Check completion candidates against an independent two-phase worklist."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class RmgFinishTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-finish-hypotheses.py")
        self.source = (self.root / "src/rmg_terrain.cpp").read_text()

    def test_sixty_owned_snapshot_variants_and_rebase_guard(self):
        payload = self.module.make_manifest(self.source)
        axis = payload["axes"][0]
        self.assertEqual(len({row["replace"] for row in axis["options"]}), 60)
        self.assertEqual(axis["find"], axis["options"][0]["replace"])
        with tempfile.TemporaryDirectory(prefix="rmg-finish-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            parsed = self.module.hypotheses.parse_manifest(path, root=self.root)
            variants = self.module.hypotheses.variants(parsed[4], parsed[5])
        self.assertEqual(len(variants), 60)
        for index, variant in enumerate(variants):
            changed = variant.source.decode()
            self.assertEqual(len(self.module._source.find_definitions(changed, self.module.HELPER)), 1)
            body = axis["options"][index]["replace"]
            self.assertEqual(body.count("needsTerrainRepair(point)"), 1)
            self.assertEqual(body.count("m_secondaryPoints.erase(point)"), 1)
            self.assertNotIn("#pragma", body)
            self.assertNotIn("const TRmgGridPoint& point = *", body)
            if index % 7 == 0:
                rebased = self.module.make_manifest(changed)["axes"][0]
                self.assertEqual(rebased["find"], rebased["options"][0]["replace"])
        with self.assertRaisesRegex(ValueError, "review the painter worklist"):
            self.module.make_manifest(self.source.replace(axis["find"], axis["find"].replace(
                "m_secondaryPoints.erase(point);", "m_secondaryPoints.clear();")))

    def test_ten_parents_cross_six_predicate_forms(self):
        parents = [name for name, _ in self.module.bodies()][:10]
        payload = self.module.make_predicate_manifest(self.source, parents)
        self.assertEqual(len(payload["axes"][0]["options"]), 10)
        self.assertEqual(len(payload["axes"][1]["options"]), 6)
        with tempfile.TemporaryDirectory(prefix="rmg-finish-predicate-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            parsed = self.module.hypotheses.parse_manifest(path, root=self.root)
            variants = self.module.hypotheses.variants(parsed[4], parsed[5])
        self.assertEqual(len(variants), 60)
        for variant in variants:
            changed = variant.source.decode()
            for helper in (self.module.HELPER, self.module.PREDICATE):
                self.assertEqual(len(self.module._source.find_definitions(changed, helper)), 1)
        with self.assertRaisesRegex(ValueError, "review ten unique"):
            self.module.make_predicate_manifest(self.source, parents[:-1])

    def test_fifty_independent_snapshots_rebase_and_feed_predicate_pass(self):
        payload = self.module.make_manifest(self.source, split_snapshots=True)
        axis = payload["axes"][0]
        self.assertEqual(len({row["replace"] for row in axis["options"]}), 50)
        with tempfile.TemporaryDirectory(prefix="rmg-finish-split-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            parsed = self.module.hypotheses.parse_manifest(path, root=self.root)
            variants = self.module.hypotheses.variants(parsed[4], parsed[5])
        self.assertEqual(len(variants), 50)
        for index, variant in enumerate(variants):
            if index % 7 == 0:
                rebased = self.module.make_manifest(variant.source.decode(), split_snapshots=True)["axes"][0]
                self.assertEqual(rebased["find"], rebased["options"][0]["replace"])
        parents = [row["name"] for row in axis["options"]][:10]
        follow_up = self.module.make_predicate_manifest(self.source, parents)
        self.assertEqual(len(follow_up["axes"][0]["options"]), 10)
        self.assertEqual(len(follow_up["axes"][1]["options"]), 6)

    def test_joint_parents_cross_six_helper_orders_and_rebase(self):
        worklists = [name for name, _ in self.module.bodies()][:2]
        parents = [(work, pred) for work in worklists for pred, _ in self.module.predicate_bodies()][:10]
        payload = self.module.make_order_manifest(self.source, parents)
        with tempfile.TemporaryDirectory(prefix="rmg-finish-order-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            parsed = self.module.hypotheses.parse_manifest(path, root=self.root)
            variants = self.module.hypotheses.variants(parsed[4], parsed[5])
        self.assertEqual(len(variants), 60)
        for index, variant in enumerate(variants):
            changed = variant.source.decode()
            for helper in (self.module.HELPER, self.module.PREDICATE):
                self.assertEqual(len(self.module._source.find_definitions(changed, helper)), 1)
            if index % 7 == 0:
                self.assertEqual(len(self.module.make_order_manifest(changed, parents)["axes"][0]["options"]), 60)
        with self.assertRaisesRegex(ValueError, "review ten unique"):
            self.module.make_order_manifest(self.source, parents[:-1])

    @unittest.skipUnless(shutil.which("g++"), "predicate oracle needs g++")
    def test_predicates_preserve_short_circuit_calls_and_raw_byte_return(self):
        program = ["#include <vector>\n"]
        bodies = list(self.module.predicate_bodies())
        for index, (_, body) in enumerate(bodies):
            program += [f"namespace Case{index} {{\n", r"""
struct TRmgGridPoint { unsigned int m_x, m_y; };
struct Rule { unsigned char m_allowsSeparatedNeighbours; };
Rule rule;
Rule* g_rmgTerrainRules[] = {&rule};
struct rmgTerrainPainter {
    unsigned char horizontal, vertical, separated;
    std::vector<int> calls;
    unsigned char isHorizontalGap(const TRmgGridPoint&) { calls.push_back(1); return horizontal; }
    unsigned char isVerticalGap(const TRmgGridPoint&) { calls.push_back(2); return vertical; }
    int getTerrain(const TRmgGridPoint&) { calls.push_back(3); return 0; }
    unsigned char hasSeparatedNeighbours(const TRmgGridPoint&) { calls.push_back(4); return separated; }
    unsigned char needsTerrainRepair(const TRmgGridPoint&);
};
""", body, r"""
int check() {
    const unsigned char values[] = {0, 1, 7, 128, 255};
    for (unsigned int h = 0; h != 5; ++h)
    for (unsigned int v = 0; v != 5; ++v)
    for (unsigned int a = 0; a != 5; ++a)
    for (unsigned int s = 0; s != 5; ++s) {
        rmgTerrainPainter painter;
        painter.horizontal = values[h]; painter.vertical = values[v]; painter.separated = values[s];
        rule.m_allowsSeparatedNeighbours = values[a];
        std::vector<int> expectedCalls; expectedCalls.push_back(1);
        unsigned char expected = 1;
        if (h == 0) {
            expectedCalls.push_back(2);
            if (v == 0) {
                expectedCalls.push_back(3); expected = 0;
                if (a == 0) { expectedCalls.push_back(4); expected = values[s]; }
            }
        }
        TRmgGridPoint point = {3, 4};
        unsigned char result = painter.needsTerrainRepair(point);
        if (result != expected || painter.calls != expectedCalls) return 1;
    }
    return 0;
}
}
"""]
        program += ["int main() {\n"]
        program += [f"if (Case{index}::check()) return {index + 1};\n" for index in range(len(bodies))]
        program += ["return 0;\n}\n"]
        with tempfile.TemporaryDirectory(prefix="rmg-predicate-oracle-") as raw:
            path = Path(raw)
            cpp, executable = path / "predicate.cpp", path / "predicate"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)

    @unittest.skipUnless(shutil.which("g++"), "worklist oracle needs g++")
    def test_all_worklists_preserve_order_and_revisit_new_primary_work(self):
        header = (self.root / "include/rmg.h").read_text()
        start = header.index("struct TRmgGridPoint {")
        point = header[start:header.index("\n};", start) + 3]
        program = ["#include <set>\n#include <vector>\n"]
        bodies = list(dict.fromkeys(body for split in (False, True) for _, body in self.module.bodies(split)))
        for index, body in enumerate(bodies):
            program += [f"namespace Case{index} {{\nstruct TPoint {{ int m_x, m_y; }};\n", point, r"""
bool operator<(const TRmgGridPoint& a, const TRmgGridPoint& b) {
    return a.m_y < b.m_y || (a.m_y == b.m_y && a.m_x < b.m_x);
}
unsigned int key(const TRmgGridPoint& p) { return p.m_x + 8 * p.m_y; }
struct rmgTerrainPainter {
    std::set<TRmgGridPoint> m_primaryPoints, m_secondaryPoints;
    std::vector<unsigned int> events;
    unsigned int mode;
    bool bad;
    void repairTerrainPoint(const TRmgGridPoint& p) {
        m_primaryPoints.erase(p);
        events.push_back(100 + key(p));
        if (p.m_x < 4) m_secondaryPoints.insert(TRmgGridPoint(p.m_x + 1, p.m_y));
    }
    unsigned char needsTerrainRepair(const TRmgGridPoint& p) {
        if (m_secondaryPoints.count(p)) bad = true;
        events.push_back(200 + key(p));
        return (p.m_x + mode) % 3 != 0;
    }
    void paintPoint(const TRmgGridPoint& p) {
        events.push_back(300 + key(p));
        if (p.m_x < 6) m_primaryPoints.insert(TRmgGridPoint(p.m_x + 1, p.m_y));
    }
    void paintTransitions() {
        if (!m_primaryPoints.empty() || !m_secondaryPoints.empty()) bad = true;
        events.push_back(400);
    }
    void finish();
};
""", body, r"""
int check() {
    for (unsigned int mask = 0; mask != 16; ++mask)
    for (unsigned int mode = 0; mode != 3; ++mode) {
        rmgTerrainPainter painter; painter.mode = mode; painter.bad = false;
        std::set<unsigned int> primary, secondary;
        for (unsigned int i = 0; i != 4; ++i) if (mask & (1u << i)) {
            unsigned int k = i + (i % 2) * 8;
            if (i % 2) secondary.insert(k); else primary.insert(k);
        }
        for (std::set<unsigned int>::const_iterator i = primary.begin(); i != primary.end(); ++i)
            painter.m_primaryPoints.insert(TRmgGridPoint(*i % 8, *i / 8));
        for (std::set<unsigned int>::const_iterator i = secondary.begin(); i != secondary.end(); ++i)
            painter.m_secondaryPoints.insert(TRmgGridPoint(*i % 8, *i / 8));
        std::vector<unsigned int> expected;
        bool primaryPhase = true;
        // Independent state machine: drain each phase fully, including
        // secondary work added by primary repair, then revisit primary work.
        for (;;) {
            if (primaryPhase && primary.empty()) primaryPhase = false;
            if (primaryPhase) {
                unsigned int k = *primary.begin(); primary.erase(k);
                expected.push_back(100 + k);
                if (k % 8 < 4) secondary.insert(k + 1);
            } else if (!secondary.empty()) {
                unsigned int k = *secondary.begin(); secondary.erase(k);
                expected.push_back(200 + k);
                if ((k % 8 + mode) % 3 != 0) {
                    expected.push_back(300 + k);
                    if (k % 8 < 6) primary.insert(k + 1);
                }
            } else if (!primary.empty()) primaryPhase = true;
            else break;
        }
        expected.push_back(400);
        painter.finish();
        if (painter.bad || painter.events != expected || !painter.m_primaryPoints.empty()
            || !painter.m_secondaryPoints.empty()) return 1;
    }
    return 0;
}
}
"""]
        program += ["int main() {\n"]
        program += [f"if (Case{index}::check()) return {index + 1};\n" for index in range(len(bodies))]
        program += ["return 0;\n}\n"]
        with tempfile.TemporaryDirectory(prefix="rmg-finish-oracle-") as raw:
            path = Path(raw)
            cpp, executable = path / "finish.cpp", path / "finish"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
