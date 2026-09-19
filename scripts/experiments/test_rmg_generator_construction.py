"""Behavior oracle for the constructor's hero policy and reverse overrides.

Exercises the changed source regions; it does not model constructor EH or
string inlining (those require the pinned VC6 object and retail comparison).
"""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class GeneratorConstruction(unittest.TestCase):
    def test_retained_hero_record_layout_and_canonical_word(self):
        from clang import cindex
        from homm3.core import clang, common
        root = common.HOMM3_DIR
        args = ['--driver-mode=cl', '/TP', *clang.FLAGS, '-U__clang__',
                '-D_MSC_VER=' + clang.MSC_VER, '-DHOMM3_SOURCE_OWNERSHIP',
                '-imsvc', str(clang.mirror()), '/I' + str(root / 'include'),
                '/I' + str(root / 'vendor/zlib-1.1.3')]
        tu = cindex.Index.create().parse(str(root / 'src/rmg.cpp'), args=args,
            options=cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
        self.assertEqual([str(d) for d in tu.diagnostics
                          if d.severity >= cindex.Diagnostic.Error], [])
        record = next(c for c in tu.cursor.get_children()
                      if c.spelling == 'THeroTraits' and c.is_definition())
        self.assertEqual(record.type.get_size(), 92)
        self.assertEqual(record.type.get_offset('m_attributes'), 56 * 8)
        fields = {}
        def visit(cursor):
            for child in cursor.get_children():
                if child.kind == cindex.CursorKind.FIELD_DECL:
                    fields[child.spelling] = child
                visit(child)
        visit(record)
        self.assertEqual(fields['m_attributes'].type.get_canonical().spelling, 'unsigned int')
        for name, byte in (('m_availableInOriginal', 0), ('m_availableInExpansion', 1), ('m_special', 2)):
            self.assertEqual(fields[name].type.get_canonical().spelling, 'unsigned char')
            self.assertEqual(fields[name].get_field_offsetof(), byte * 8)

    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_policy_and_reverse_overrides(self):
        module = generator("generate-rmg-generator-construction-family.py")
        source = (module.HOMM3_DIR / module.SOURCE).read_text()
        original_function = module.definition(source)
        variants = (list(module.variants(original_function)) if module.HERO in original_function
                    else [dict(name="authored", replace=original_function)])
        if os.environ.get("HOMM3_GENERATOR_MANIFEST"):
            variants = json.loads(Path(os.environ["HOMM3_GENERATOR_MANIFEST"]).read_text())["axes"][0]["options"]
        bodies = []
        resets = []
        for row in variants:
            body = row["replace"]
            start = body.index("\n", body.index("        initializeObjectGenerators();")) + 1
            end = body.rindex("\n    }")
            bodies.append(body[start:end])
            resets.append(body[body.index("        m_nextSeerHutPrototypeIndex = 0;"):body.index("        initializeObjectGenerators();")])
        original = bodies[0]
        if "m_availability.m_special" in original:
            special, wrong_special = "m_availability.m_special", "m_availability.m_availableInOriginal"
        elif "m_attributes >> 16" in original:
            special, wrong_special = "m_attributes >> 16", "m_attributes >> 24"
        else:
            special, wrong_special = "m_attributes)[2]", "m_attributes)[3]"
        negatives = [
            original.replace("m_mapVersion >= 1", "m_mapVersion > 1"),
            original.replace(special, wrong_special),
            original.replace("m_disabledHeroes[hero] = 1", "m_disabledHeroes[hero] = 0"),
            original.replace("mapLimit = 30", "mapLimit = 29"),
            original.replace("zoneLimit--;", "--zoneLimit;"),
            original.replace("mapLimit = 30; mapLimit--;", "mapLimit = 0; mapLimit < 30; ++mapLimit"),
        ]
        self.assertTrue(all(body != original for body in negatives))
        text = r"""#include <algorithm>
#include <cstring>
#include <cstdio>
struct THeroTraits { /* actual attribute layout */ };
struct TRmgObjectLimit { int m_objectType, m_limit; };
static THeroTraits g_heroTraits[156];
static int g_rmgZoneObjectLimits[232], g_rmgMapObjectLimits[232];
static TRmgObjectLimit g_rmgMapObjectLimitOverrides[30];
static TRmgObjectLimit g_rmgZoneObjectLimitOverrides[24];
struct Fixture {
    int m_mapVersion;
    int m_nextSeerHutPrototypeIndex;
    unsigned char m_usedQuestArtifacts[144];
    unsigned char m_disabledHeroes[156];
    int m_objectCountByType[232];
    unsigned char m_fixedHumanPlayers[8];
};
"""
        header = (module.HOMM3_DIR / "include/hero.h").read_text()
        start = header.index("    union {\n        unsigned int m_attributes;")
        end = header.index("\n    };", start) + len("\n    };")
        text = text.replace("/* actual attribute layout */", header[start:end])
        for i, body in enumerate(bodies + negatives):
            reset = resets[i] if i < len(resets) else resets[0]
            text += f"struct Case{i}: Fixture {{ void reset() {{\n{reset}\n}} void run() {{\n{body}\n}} }};\n"
        text += r"""template<class Candidate> bool check() {
    // Duplicate object IDs prove reverse traversal's first-record priority.
    for (int i = 0; i < 30; ++i) {
        g_rmgMapObjectLimitOverrides[i].m_objectType = (i * 13) % 17;
        g_rmgMapObjectLimitOverrides[i].m_limit = 100 + i;
    }
    for (int i = 0; i < 24; ++i) {
        g_rmgZoneObjectLimitOverrides[i].m_objectType = (i * 11) % 19;
        g_rmgZoneObjectLimitOverrides[i].m_limit = 200 + i;
    }
    // Unique final records also expose a truncated reverse range.
    g_rmgMapObjectLimitOverrides[29].m_objectType = 231;
    g_rmgZoneObjectLimitOverrides[23].m_objectType = 230;
    int expectedMap[232], expectedZone[232];
    std::fill_n(expectedMap, 232, 32000);
    std::fill_n(expectedZone, 232, 32000);
    for (int i = 0; i < 30; ++i) {
        const TRmgObjectLimit& r = g_rmgMapObjectLimitOverrides[i];
        if (expectedMap[r.m_objectType] == 32000) expectedMap[r.m_objectType] = r.m_limit;
    }
    for (int i = 0; i < 24; ++i) {
        const TRmgObjectLimit& r = g_rmgZoneObjectLimitOverrides[i];
        if (expectedZone[r.m_objectType] == 32000) expectedZone[r.m_objectType] = r.m_limit;
    }
    const unsigned int bytes[] = {0, 1, 2, 7, 127, 128, 254, 255};
    for (int combination = 0; combination < 4096; ++combination)
    for (int version = -1; version < 3; ++version) {
        Candidate candidate;
        std::memset(&candidate, 0x35, sizeof(candidate));
        candidate.reset();
        if (candidate.m_nextSeerHutPrototypeIndex) return false;
        for (int k = 0; k < 144; ++k) if (candidate.m_usedQuestArtifacts[k]) return false;
        for (int k = 0; k < 156; ++k) if (candidate.m_disabledHeroes[k]) return false;
        for (int k = 0; k < 232; ++k) if (candidate.m_objectCountByType[k]) return false;
        candidate.m_mapVersion = version;
        unsigned char expected[156];
        for (int hero = 0; hero < 156; ++hero) {
            int c = (combination + hero * 73) % 4096;
            unsigned int original = bytes[c % 8]; c /= 8;
            unsigned int expansion = bytes[c % 8]; c /= 8;
            unsigned int special = bytes[c % 8]; c /= 8;
            unsigned int unrelated = bytes[c % 8];
            g_heroTraits[hero].m_attributes = original + 256 * expansion
                + 65536 * special + 16777216u * unrelated;
            unsigned char initial = hero % 3 ? 0 : 9;
            candidate.m_disabledHeroes[hero] = initial;
            bool available = version < 1 ? original != 0 : expansion != 0;
            expected[hero] = special || !available ? 1 : initial;
        }
        candidate.run();
        for (int k = 0; k < 8; ++k) if (candidate.m_fixedHumanPlayers[k]) return false;
        if (std::memcmp(expected, candidate.m_disabledHeroes, sizeof(expected))) return false;
        if (std::memcmp(expectedMap, g_rmgMapObjectLimits, sizeof(expectedMap))) return false;
        if (std::memcmp(expectedZone, g_rmgZoneObjectLimits, sizeof(expectedZone))) return false;
    }
    return true;
}
int main() {
"""
        for i in range(len(bodies)):
            text += f'if (!check<Case{i}>()) {{ std::printf("valid {i} failed\\n"); return 1; }}\n'
        for i in range(len(bodies), len(bodies) + len(negatives)):
            text += f'if (check<Case{i}>()) {{ std::printf("negative {i} survived\\n"); return 2; }}\n'
        text += "return 0;\n}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-generator-construction-") as raw:
            root = Path(raw)
            source, binary = root / "constructors.cpp", root / "constructors"
            source.write_text(text)
            subprocess.run([shutil.which("g++"), "-std=c++98", "-O1", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
