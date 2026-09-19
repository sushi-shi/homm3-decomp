"""Native allocation/registration-order oracle for the initializer family.

Stub records expose the requested concrete class and all constructor arguments.
This validates source transformations with fixed input tables, not VC6 inlining,
allocator failure behavior, or the real constructors' bodies.
"""
import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from homm3.vc6.test_rmg_families import generator


class Roster(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_all_source_forms_and_negative_controls(self):
        module = generator("generate-rmg-roster-family.py")
        original = module.definition((module.HOMM3_DIR / module.SOURCE).read_text())
        bodies = [x["replace"] for x in module.variants(original)]
        if os.environ.get("HOMM3_ROSTER_MANIFEST"):
            manifest = json.loads(Path(os.environ["HOMM3_ROSTER_MANIFEST"]).read_text())
            bodies = [x["replace"] for x in manifest["axes"][0]["options"]]
        negatives = [
            original.replace("m_mapVersion >= 1", "m_mapVersion > 1"),
            original.replace("creature--;", "--creature;"),
            original.replace("m_level >= 0", "m_level > 0"),
            original.replace("m_disabledKeyTents[player] = 0", "m_disabledKeyTents[player] = 1"),
            original.replace("quest < m_objectPrototypes[83].size()", "quest + 1 < m_objectPrototypes[83].size()"),
            original.replace("new type_prison_def(20000, 90000)", "new type_prison_def(20000, 500000)"),
        ]
        self.assertTrue(all(x != original for x in negatives))
        kinds = sorted({m.group(2) for m in module.REGISTER.finditer(original)})
        text = r'''
#include <vector>
#include <string>
#include <cstdio>
struct type_treasure_def {
    std::string kind;
    std::vector<int> args;
    template<class... A> type_treasure_def(A... a): kind("type_treasure_def"), args{a...} {}
    virtual ~type_treasure_def() {}
};
struct Trait { int m_level; };
static Trait g_creatureTypeTraits[145];
'''
        for kind in kinds:
            if kind == "type_treasure_def":
                continue
            text += (f'struct {kind}: type_treasure_def {{ template<class... A> {kind}(A... a)'
                     f': type_treasure_def(a...) {{ kind = "{kind}"; }} }};\n')
        text += r'''
struct Fixture {
    int m_mapVersion;
    std::vector<int> m_objectPrototypes[232];
    std::vector<unsigned char> m_disabledKeyTents;
    std::vector<type_treasure_def*> m_objectGenerators;
    ~Fixture() { for (auto p: m_objectGenerators) delete p; }
};
'''
        for n, body in enumerate(bodies + negatives):
            text += f"struct Case{n}: Fixture {{ void run() " + body[body.index("{"):] + " };\n"
        text += r'''
template<class Candidate> bool check() {
    for (int version = -1; version <= 2; ++version)
    for (int keys: {0, 1, 3, 8})
    for (int quests: {0, 1, 3})
    for (int levels = 0; levels < 3; ++levels) {
        Case0 reference;
        Candidate actual;
        reference.m_mapVersion = actual.m_mapVersion = version;
        reference.m_objectPrototypes[10].resize(keys);
        actual.m_objectPrototypes[10].resize(keys);
        reference.m_objectPrototypes[83].resize(quests);
        actual.m_objectPrototypes[83].resize(quests);
        reference.m_disabledKeyTents.assign(10, 7);
        actual.m_disabledKeyTents.assign(10, 7);
        for (int i = 0; i < 145; ++i)
            g_creatureTypeTraits[i].m_level = levels == 0 ? -1 : levels == 1 ? 0 : (i % 4 - 1);
        reference.run(); actual.run();
        if (reference.m_disabledKeyTents != actual.m_disabledKeyTents) return false;
        if (reference.m_objectGenerators.size() != actual.m_objectGenerators.size()) return false;
        int blackCreatures = 0, questCreatures = 0, dwellings = 0, tents = 0;
        for (unsigned i = 0; i < reference.m_objectGenerators.size(); ++i) {
            auto p = reference.m_objectGenerators[i];
            auto q = actual.m_objectGenerators[i];
            if (p->kind != q->kind || p->args != q->args) return false;
            blackCreatures += p->kind == "type_black_box_creature_def";
            questCreatures += p->kind == "type_quest_creature_def";
            dwellings += p->kind == "type_map_dwelling_def";
            tents += p->kind == "type_key_tent_def";
        }
        int expectedCreatures = 0;
        for (int i = 0; i < (version >= 1 ? 145 : 118); ++i)
            expectedCreatures += g_creatureTypeTraits[i].m_level >= 0;
        if (blackCreatures != expectedCreatures || questCreatures != quests * expectedCreatures
            || dwellings != (version >= 1 ? 80 : 58) || tents != 5 * keys) return false;
    }
    return true;
}
int main() {
'''
        for n in range(len(bodies) + len(negatives)):
            expected = "true" if n < len(bodies) else "false"
            text += f'if (check<Case{n}>() != {expected}) {{ std::printf("case {n} failed\\n"); return 1; }}\n'
        text += "}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-roster-") as directory:
            source = Path(directory) / "oracle.cpp"
            binary = Path(directory) / "oracle"
            source.write_text(text)
            subprocess.run(["g++", "-std=c++17", "-O0", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
