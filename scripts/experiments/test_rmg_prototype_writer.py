"""Independent little-endian wire records for the real prototype-writer bodies."""
import os
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class PrototypeWriterTests(unittest.TestCase):
    def test_manifest_and_adopted_control(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-prototype-writer-family.py")
        source = (root / module.SOURCE).read_text()
        axes = module.axes(source)
        self.assertEqual(len(axes[0]["options"]), 60)
        self.assertEqual(axes[0]["find"], module.definition(source))
        payload = dict(schema=1, units=["rmg"], axes=axes)
        with tempfile.TemporaryDirectory(prefix="rmg-prototype-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            _, originals, parsed = module.source_families.load_manifest(path, root)
        self.assertEqual(module.source_families.render(originals, parsed, (0,)), originals)
        seed = module.seed_body(source)
        for _, body in module.loop_refinements(seed):
            self.assertEqual(body.count("getImageName()"), 2)
            self.assertEqual(body.count("outfile->write("), 11)
        with self.assertRaisesRegex(ValueError, "review"):
            module.axes(source.replace("unsigned char mask[6]", "unsigned char mask[5]"))

    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_buffers_wire_order_and_negative_controls(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-prototype-writer-family.py")
        source = (root / module.SOURCE).read_text()
        forms = [option["replace"] for option in module.axes(source)[0]["options"]]
        self.assertEqual(len(forms), 60)
        for selected in filter(None, os.environ.get("HOMM3_PROTOTYPE_WRITER_MANIFEST", "").split(":")):
            manifest = Path(selected)
            if not manifest.is_absolute():
                manifest = root / manifest
            # Historical bodies remain useful semantic controls after adoption;
            # this imports no old scores and does not rebase a compiler search.
            payload = json.loads(manifest.read_text())
            self.assertEqual(payload["schema"], 1)
            self.assertEqual(payload["units"], ["rmg"])
            self.assertEqual(len(payload["axes"]), 1)
            axis = payload["axes"][0]
            self.assertEqual(axis["source"], module.SOURCE)
            for option in axis["options"]:
                self.assertFalse(option.get("extra_edits"))
                body = option.get("replace", axis["find"])
                self.assertEqual(module.definition(body), body)
                forms.append(body)
        forms = list(dict.fromkeys(forms))
        positive = len(forms)
        baseline = module.seed_body(source)
        for before, after in (
            ("int nameLength = prototype->getImageName().size();", "int nameLength = prototype->getImageName().size() + 1;"),
            ("prototype->getImageName().c_str()", "prototype->m_name.c_str()"),
            ("prototype->m_passableMask.test", "prototype->m_triggerMask.test"),
            ("CObjectType::getBitPos(x, y)", "CObjectType::getBitPos(x, 5 - y)"),
            ("for (int x = 7; x >= 0; --x)", "for (int x = 6; x >= 0; --x)"),
            ("terrain < 10", "terrain < 9"),
            ("unsigned char mask[6] = {0};", "unsigned char mask[6] = {1};"),
            ("int value = prototype->m_subtype;", "int value = prototype->m_objectType;"),
            ("char value = prototype->m_slotCategory;", "char value = 0;"),
            ("char value = prototype->m_isUnderlay;", "char value = !!prototype->m_isUnderlay;"),
            ("int reserved[4] = {0, 0, 0, 0};", "int reserved[4] = {0, 0, 1, 0};"),
            ("outfile->write(prototype->getImageName().c_str(), nameLength);", "prototype->getImageName();"),
        ):
            self.assertIn(before, baseline)
            forms.append(baseline.replace(before, after))
        objects = (root / "include/advmgr_objects.h").read_text()
        def field(name):
            # Some names are shared with CObjectType. Use the owning record.
            owned = objects[objects.index("struct TObjectType {"):objects.index("class TObjectTypeTable {")]
            matches = re.findall(r"^\s*([^\n;]+\b" + name + r"\s*;)", owned, re.M)
            self.assertEqual(len(matches), 1, name)
            return matches[0]
        fields = "\n".join(field(name) for name in ("m_passableMask", "m_triggerMask", "m_terrainMask",
            "m_recommendedTerrainMask", "m_objectType", "m_subtype", "m_slotCategory", "m_isUnderlay"))
        mapcell = (root / "include/mapcell.h").read_text()
        start = mapcell.index("enum TAdventureObjectType {")
        enum = mapcell[start:mapcell.index("\n};", start) + 3]
        get_bit = re.search(r"    static unsigned getBitPos\(unsigned x, unsigned y\)\s*\{[^}]+}", objects)[0]
        program = """#include <algorithm>
#include <bitset>
#include <climits>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
""" + enum + "\n" + (root / "include/abstractfile.h").read_text()
        program += "\nclass CObjectType { public:\n" + get_bit + "\n};\nstruct TObjectType {\n" + fields + r"""
    std::string m_name;
    std::vector<int>* m_trace;
    const std::string& getImageName() { m_trace->push_back(100); return m_name; }
};
typedef std::vector<unsigned char> Bytes;
typedef std::vector<Bytes> Records;
struct Failure {};
static void mutate(TObjectType& object, int stage, bool enabled) {
    if (!enabled) return;
    object.m_passableMask.flip((stage * 7 + 3) % 48);
    object.m_triggerMask.flip((stage * 11 + 4) % 48);
    object.m_terrainMask.flip(stage % 10);
    object.m_recommendedTerrainMask.flip((stage + 3) % 10);
    object.m_objectType = object.m_objectType == ARTIFACT ? BOAT : ARTIFACT;
    object.m_subtype ^= 0x13579;
    object.m_slotCategory ^= 0x93;
    object.m_isUnderlay ^= 0xff;
    if (!stage) object.m_name.assign(object.m_name.size() + 8, 'Z');
}
static Bytes wire(unsigned long long value, int size) {
    Bytes result;
    for (int i = 0; i < size; ++i) {
        result.push_back(static_cast<unsigned char>(value));
        value >>= 8;
    }
    return result;
}
static bool same(const TObjectType& a, const TObjectType& b) {
    return a.m_passableMask == b.m_passableMask && a.m_triggerMask == b.m_triggerMask
        && a.m_terrainMask == b.m_terrainMask && a.m_recommendedTerrainMask == b.m_recommendedTerrainMask
        && a.m_objectType == b.m_objectType && a.m_subtype == b.m_subtype
        && a.m_slotCategory == b.m_slotCategory && a.m_isUnderlay == b.m_isUnderlay && a.m_name == b.m_name;
}
struct Sink : TAbstractFile {
    TObjectType* m_object;
    Records m_records;
    int m_throwAt, m_returnMode;
    bool m_mutation;
    int read(void*, int) { return 0; }
    int write(const void* buffer, int size) {
        int stage = static_cast<int>(m_records.size());
        const unsigned char* bytes = static_cast<const unsigned char*>(buffer);
        m_records.push_back(Bytes(bytes, bytes + size));
        m_object->m_trace->push_back(stage);
        if (stage == m_throwAt) throw Failure();
        mutate(*m_object, stage, m_mutation);
        return m_returnMode == 0 ? size : m_returnMode == 1 ? 0 : -1;
    }
};
typedef void (*Writer)(TAbstractFile*, TObjectType*);
static bool check(Writer writer) {
    if (sizeof(int) != 4 || CHAR_BIT != 8) return false;
    const int lengths[] = {0, 1, 5, 16, 31, 64};
    const int scalars[] = {0, 1, -1, 127, 128, 255, 256, -256, INT_MIN, INT_MAX};
    for (int seed = 0; seed < 1024; ++seed)
    for (int mutation = 0; mutation < 2; ++mutation)
    for (int result = 0; result < 3; ++result)
    for (int throwAt = -1; throwAt < 11; ++throwAt) {
        TObjectType object;
        unsigned long long pattern = seed % 4 == 0 ? 0 : seed % 4 == 1 ? 0xffffffffffffULL
            : seed % 4 == 2 ? (1ULL << ((seed / 4) % 48)) : (0x123456789abcULL ^ (seed * 0x159abcdefULL));
        object.m_passableMask = std::bitset<48>(pattern);
        object.m_triggerMask = std::bitset<48>(pattern ^ 0xa5963cf01827ULL);
        object.m_terrainMask = std::bitset<10>(seed);
        object.m_recommendedTerrainMask = std::bitset<10>(1023 - seed);
        object.m_objectType = seed % 2 ? ARTIFACT : BOAT;
        object.m_subtype = scalars[seed % 10];
        object.m_slotCategory = scalars[(seed / 3) % 10];
        object.m_isUnderlay = static_cast<unsigned char>(scalars[(seed / 7) % 10]);
        object.m_name.assign(lengths[seed % 6], static_cast<char>('A' + seed % 26));
        if (object.m_name.size() > 2 && seed % 2) object.m_name[1] = '\0';
        std::vector<int> trace, expectedTrace;
        object.m_trace = &trace;
        TObjectType expectedObject = object;
        Records expected;
        int nameLength = static_cast<int>(object.m_name.size());
        expectedTrace.push_back(100);
        for (int stage = 0; stage < 11; ++stage) {
            Bytes record;
            switch (stage) {
            case 0: record = wire(nameLength, 4); break;
            case 1:
                expectedTrace.push_back(100);
                record.assign(expectedObject.m_name.begin(), expectedObject.m_name.begin() + nameLength); break;
            case 2: record = wire(expectedObject.m_passableMask.to_ullong(), 6); break;
            case 3: record = wire(expectedObject.m_triggerMask.to_ullong(), 6); break;
            case 4: record = wire(expectedObject.m_terrainMask.to_ulong(), 2); break;
            case 5: record = wire(expectedObject.m_recommendedTerrainMask.to_ulong(), 2); break;
            case 6: record = wire(static_cast<unsigned int>(expectedObject.m_objectType), 4); break;
            case 7: record = wire(static_cast<unsigned int>(expectedObject.m_subtype), 4); break;
            case 8: record = wire(static_cast<unsigned char>(expectedObject.m_slotCategory), 1); break;
            case 9: record = wire(expectedObject.m_isUnderlay, 1); break;
            case 10: record.assign(16, 0); break;
            }
            expected.push_back(record);
            expectedTrace.push_back(stage);
            if (stage == throwAt) break;
            mutate(expectedObject, stage, mutation != 0);
        }
        Sink sink;
        sink.m_object = &object; sink.m_throwAt = throwAt; sink.m_returnMode = result; sink.m_mutation = mutation != 0;
        bool caught = false;
        try { writer(&sink, &object); } catch (const Failure&) { caught = true; }
        if (caught != (throwAt >= 0) || sink.m_records != expected || trace != expectedTrace
            || !same(object, expectedObject) || object.m_trace != &trace) return false;
    }
    return true;
}
"""
        for index, body in enumerate(forms):
            program += body.replace("__fastcall ", "").replace("writeRmgObjectPrototype(", "candidate" + str(index) + "(") + "\n"
        program += "int main() {\n"
        for index in range(len(forms)):
            program += 'if (' + ('!' if index < positive else '') + 'check(candidate' + str(index) + ')) { std::fprintf(stderr, "writer form ' + str(index) + ' failed\\n"); return 1; }\n'
        program += "return 0;\n}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-prototype-wire-") as raw:
            cpp, exe = Path(raw) / "oracle.cpp", Path(raw) / "oracle"
            cpp.write_text(program)
            run = subprocess.run(["g++", "-std=c++11", "-O2", str(cpp), "-o", str(exe)], capture_output=True, text=True, timeout=180)
            self.assertEqual(run.returncode, 0, run.stderr[-6000:])
            run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=240)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        print(f"{positive} prototype writers: 73728 scenarios each; 12 negative controls rejected")


if __name__ == "__main__":
    unittest.main()
