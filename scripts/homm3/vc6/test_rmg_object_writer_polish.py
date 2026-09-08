"""Check joint RMG writer hypotheses against independent ordered wire records."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class ObjectWriterPolishTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-object-writer-polish-hypotheses.py")
        self.source = (self.root / "src/rmg.cpp").read_text()

    def test_sixty_atomic_four_writer_variants_and_scope_rebases(self):
        payload = self.module.make_manifest(self.source)
        axis = payload["axes"][0]
        candidates = dict(self.module.bundles())
        self.assertEqual(len(candidates), 60)
        self.assertEqual(axis["find"], axis["options"][0]["replace"])
        with tempfile.TemporaryDirectory(prefix="rmg-writer-manifest-test-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            parsed = self.module.hypotheses.parse_manifest(path, root=self.root)
            variants = self.module.hypotheses.variants(parsed[4], parsed[5])
        self.assertEqual(len(variants), 60)
        for variant in variants:
            expected = candidates[variant.labels["object_writer_buffers"]]
            changed = variant.source.decode()
            for body in expected.values():
                self.assertEqual(changed.count(body), 1)
                self.assertEqual(body.count("type_object::write(outfile, parameter);"), 1)
                self.assertNotIn("#pragma", body)
                self.assertNotIn("inline", body)
            # One complete scope/signedness/initialization cross-section also
            # checks that a retained candidate can become the next baseline.
            if variant.index % 7 == 0:
                rebased = self.module.make_manifest(changed)["axes"][0]
                self.assertEqual(rebased["find"], rebased["options"][0]["replace"])
        changed = self.source.replace(axis["find"], axis["find"].replace(
            "hasCustomTreasure = 0", "hasCustomTreasure = 1"))
        with self.assertRaisesRegex(ValueError, "review the four object writers"):
            self.module.make_manifest(changed)

    @unittest.skipUnless(shutil.which("g++"), "writer wire-record oracle needs g++")
    def test_all_variants_preserve_each_write_and_wire_byte(self):
        found = self.module._source.find_definitions(self.source, "type_object::write")
        self.assertEqual(len(found), 1)
        item = found[0]
        start = self.source.rfind("\n", 0, item.head) + 1
        base = self.source[start:item.body_close + 1]
        program = ["#include <vector>\n#include <cstring>\n#include <climits>\n"]
        for index, (_, bodies) in enumerate(self.module.bundles()):
            program += [f"namespace Case{index} {{\n", r"""
struct TAbstractFile { virtual void write(const void*, unsigned int) = 0; };
struct Sink : TAbstractFile {
    std::vector<std::vector<unsigned char> > writes;
    virtual void write(const void* data, unsigned int size) {
        const unsigned char* bytes = static_cast<const unsigned char*>(data);
        writes.push_back(std::vector<unsigned char>(bytes, bytes + size));
    }
};
struct TRmgMapPosition { int m_x, m_y, m_z; };
struct TRmgObjectPropertiesRef { int m_prototypeIndex; };
struct type_object {
    TRmgMapPosition m_position;
    TRmgObjectPropertiesRef* m_properties;
    virtual void write(TAbstractFile*, int);
};
""", base, "\n"]
            for owner, body in bodies.items():
                program += ["struct " + owner + " : type_object { virtual void write(TAbstractFile*, int); };\n",
                            body, "\n"]
            program += [r"""
void append(std::vector<std::vector<unsigned char> >& writes, unsigned int value, unsigned int size) {
    std::vector<unsigned char> bytes;
    for (unsigned int i = 0; i != size; ++i) bytes.push_back(i < 4 ? value >> (8 * i) : 0);
    writes.push_back(bytes);
}
int check() {
    if (sizeof(int) != 4 || sizeof(short) != 2 || CHAR_BIT != 8) return 1;
    const int inputs[] = {0, 1, -1, 127, 128, 255, 256, -256, INT_MIN, INT_MAX};
    for (int seed = 0; seed != 10; ++seed) {
        TRmgObjectPropertiesRef properties; properties.m_prototypeIndex = inputs[(seed + 3) % 10];
        rmgArtifactObject artifact; rmgResourceObject resource;
        rmgScholarObject scholar; rmgShrineObject shrine;
        type_object* objects[] = {&artifact, &resource, &scholar, &shrine};
        for (int kind = 0; kind != 4; ++kind) {
            type_object* object = objects[kind];
            object->m_properties = &properties;
            object->m_position.m_x = inputs[seed];
            object->m_position.m_y = inputs[(seed + 1) % 10];
            object->m_position.m_z = inputs[(seed + 2) % 10];
            std::vector<std::vector<unsigned char> > expected;
            append(expected, inputs[seed], 1);
            append(expected, inputs[(seed + 1) % 10], 1);
            append(expected, inputs[(seed + 2) % 10], 1);
            append(expected, properties.m_prototypeIndex, 4);
            append(expected, 0, 5);
            if (kind == 0) append(expected, 0, 1);
            else if (kind == 1) {
                append(expected, 0, 1); append(expected, 0, 4); append(expected, 0, 4);
            } else if (kind == 2) {
                append(expected, 255, 1); append(expected, 0, 1);
                append(expected, 0, 4); append(expected, 0, 2);
            } else {
                append(expected, 255, 1); append(expected, 0, 2); append(expected, 0, 1);
            }
            for (int version = -1; version != 5; ++version) {
                Sink sink;
                object->write(&sink, version);
                if (sink.writes != expected) return 2;
                if (object->m_properties != &properties || properties.m_prototypeIndex != inputs[(seed + 3) % 10]
                    || object->m_position.m_x != inputs[seed]
                    || object->m_position.m_y != inputs[(seed + 1) % 10]
                    || object->m_position.m_z != inputs[(seed + 2) % 10]) return 3;
            }
        }
    }
    return 0;
}
}
"""]
        program += ["int main() {\n"]
        program += [f"if (Case{index}::check()) return {index + 1};\n" for index in range(60)]
        program += ["return 0;\n}\n"]
        with tempfile.TemporaryDirectory(prefix="rmg-writer-wire-test-") as raw:
            path = Path(raw)
            cpp, executable = path / "writers.cpp", path / "writers"
            cpp.write_text("".join(program))
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
