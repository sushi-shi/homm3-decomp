"""Behavioral controls for the near-exact RMG selector's lifetime matrix."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class PrototypePolishTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-prototype-polish-hypotheses.py")
        self.source = (self.root / "src/rmg.cpp").read_text()

    def test_sixty_reviewed_lifetimes_rebase(self):
        for branches in (False, True):
            axis = self.module.make_manifest(self.source, branches)["axes"][0]
            reviewed = {body for _, body in self.module.bodies(branches)}
            self.assertEqual(len(reviewed), 60)
            self.assertEqual(axis["find"], axis["options"][0]["replace"])
            self.assertEqual({row["replace"] for row in axis["options"]}, reviewed | {axis["find"]})
            for row in axis["options"]:
                body = row["replace"]
                self.assertNotIn("#pragma", body)
                self.assertNotIn("TTerrainType", body)
                self.assertIn("int terrain, int objectType, int subtype", body)
                self.assertEqual(body.count("rand()"), 1)
                self.assertIn("return candidates[rand() % candidates.size()];", body)
                changed = self.source.replace(axis["find"], body)
                for next_branches in (False, True):
                    self.assertEqual(self.module.make_manifest(changed, next_branches)["axes"][0]["options"][0]["replace"], body)
        with self.assertRaisesRegex(ValueError, "review the prototype selector"):
            self.module.make_manifest(self.source.replace(axis["find"], axis["find"].replace(
                "rand() % candidates.size()", "(rand() + 1) % candidates.size()")))

    @unittest.skipUnless(shutil.which("g++"), "selector oracle needs g++")
    def test_all_variants_keep_order_rand_count_and_checked_mask_access(self):
        program = ["#include <vector>\n#include <bitset>\n#include <stdexcept>\n",
                   (self.root / "include/terrain_type.h").read_text(), "\n"]
        bodies = list(dict.fromkeys(body for branches in (False, True) for _, body in self.module.bodies(branches)))
        bodies += [option["replace"] for option in generator(
            "generate-rmg-prototype-receiver-family.py").variants(self.source)]
        if os.environ.get("HOMM3_PROTOTYPE_SELECTOR_MANIFEST"):
            from homm3.vc6 import source_families
            payload, originals, axes = source_families.load_manifest(
                Path(os.environ["HOMM3_PROTOTYPE_SELECTOR_MANIFEST"]), self.root)
            helper = generator("generate-rmg-position-family.py")
            bodies += [helper.definition(source_families.render(originals, axes, (index,))["src/rmg.cpp"],
                       "type_random_map_generator::selectObjectPrototype") for index in range(len(axes[0].options))]
        seed = bodies[0]
        positive_count = len(bodies)
        bodies += [seed.replace("m_subtype != subtype", "m_subtype == subtype"),
                   seed.replace("terrain == eTerrainWater", "terrain != eTerrainWater"),
                   seed.replace(".test(terrain)", ".test(0)"),
                   seed.replace("rand() % candidates.size()", "0"),
                   seed.replace("candidates.push_back(properties);", "candidates.insert(candidates.begin(), properties);")]
        self.assertTrue(all(body != seed for body in bodies[positive_count:]))
        for index, body in enumerate(bodies):
            # The native fixture supplies public-container behavior only. It
            # never changes project declarations or enters the VC6 snapshot.
            program += [f"namespace Case{index} {{\n", r"""
struct TObjectType {
    enum { SLOT_CATEGORY_4 = 4, SLOT_CATEGORY_5 = 5 };
    int m_subtype, m_slotCategory;
    std::bitset<10> m_recommendedTerrainMask;
};
struct TRmgObjectPropertiesRef { TObjectType* m_prototype; };
struct type_random_map_generator {
    std::vector<TRmgObjectPropertiesRef*> m_objectPrototypes[232];
    TRmgObjectPropertiesRef* selectObjectPrototype(int, int, int);
};
int g_draw, g_calls;
int rand() { ++g_calls; return g_draw; }
""", body, "\n", r"""
int check() {
    const int groups[] = {0, 17, 231};
    const int terrains[] = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    type_random_map_generator generator;
    TObjectType prototypes[24];
    TRmgObjectPropertiesRef properties[24];
    unsigned int masks[24];
    for (int seed = 0; seed != 6; ++seed) {
        for (int group = 0; group != 3; ++group) generator.m_objectPrototypes[groups[group]].clear();
        for (int item = 0; item != 24; ++item) {
            prototypes[item].m_subtype = item % 5 - 1;
            prototypes[item].m_slotCategory = (item + seed) % 8;
            masks[item] = seed == 0 ? 0 : seed == 1 ? 1023 : (item * 89 + seed * 151) & 1023;
            prototypes[item].m_recommendedTerrainMask = std::bitset<10>(masks[item]);
            properties[item].m_prototype = &prototypes[item];
            // Reverse source ordering in one range to expose accidental sorting.
            generator.m_objectPrototypes[0].push_back(&properties[item]);
            generator.m_objectPrototypes[17].insert(generator.m_objectPrototypes[17].begin(), &properties[item]);
        }
        // Group 231 deliberately remains empty, even for invalid mask indices.
        for (int group = 0; group != 3; ++group)
        for (int terrain = 0; terrain != 12; ++terrain)
        for (int subtype = -1; subtype != 5; ++subtype) {
            std::vector<TRmgObjectPropertiesRef*> expected;
            bool throws = false;
            const std::vector<TRmgObjectPropertiesRef*>& range = generator.m_objectPrototypes[groups[group]];
            for (unsigned int item = 0; item != range.size(); ++item) {
                int number = static_cast<int>(range[item] - properties);
                const TObjectType& prototype = prototypes[number];
                if (prototype.m_subtype != subtype) continue;
                if (prototype.m_slotCategory == 4 || prototype.m_slotCategory == 5) {
                    if (terrains[terrain] == 8) continue;
                } else {
                    if (terrains[terrain] < 0 || terrains[terrain] >= 10) { throws = true; break; }
                    if (!(masks[number] & (1u << terrains[terrain]))) continue;
                }
                expected.push_back(range[item]);
            }
            for (int draw = 0; draw != 26; ++draw) {
                g_draw = draw == 25 ? 32767 : draw; g_calls = 0;
                bool caught = false;
                TRmgObjectPropertiesRef* actual = 0;
                try {
                    actual = generator.selectObjectPrototype(terrains[terrain], groups[group], subtype);
                } catch (const std::out_of_range&) { caught = true; }
                if (caught != throws) return 1;
                int expectedCalls = !throws && !expected.empty() ? 1 : 0;
                if (g_calls != expectedCalls) return 2;
                if (!throws && actual != (expected.empty() ? 0 : expected[g_draw % expected.size()])) return 3;
            }
        }
    }
    return 0;
}
}
"""]
        program += ["int main() {\n"]
        program += [f"if (Case{index}::check()) return 1;\n" for index in range(positive_count)]
        program += [f"if (!Case{index}::check()) return 2;\n" for index in range(positive_count, len(bodies))]
        program += ["return 0;\n}\n"]
        with tempfile.TemporaryDirectory(prefix="rmg-prototype-polish-test-") as raw:
            path = Path(raw)
            cpp, executable = path / "selector.cpp", path / "selector"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
