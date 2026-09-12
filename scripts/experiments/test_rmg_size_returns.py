"""Actual grid-value return semantics; reduced polymorphic map owners."""
import itertools
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class SizeReturnTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_all_pairs_and_broken_returns(self):
        module = generator("generate-rmg-size-return-family.py")
        root = Path(__file__).resolve().parents[2]
        header = (root / "include/rmg.h").read_text()
        text = "#include <climits>\n#include <cstdio>\n"
        for name in ("TRmgVector", "TPoint", "TRmgMapPosition"):
            start = header.index("struct " + name + " {")
            text += header[start:header.index("\n};", start) + 3] + "\n"
        start = header.index("template<class Coordinate>\nstruct TRmgGridPointT {")
        end = header.index("typedef TRmgGridPointT<unsigned int> TRmgGridPoint;", start)
        text += header[start:end] + "typedef TRmgGridPointT<unsigned int> TRmgGridPoint;\n"
        pairs = [(base, adapter) for (_, base), (_, adapter) in itertools.product(
            module.base_forms(), module.adapter_forms("TRmgMapAdapter"))]
        self.assertEqual(len(pairs), 60)
        base, adapter = pairs[0]
        pairs += [(base.replace("m_size.m_x, m_size.m_y", "m_size.m_y, m_size.m_x"), adapter),
                  (base, adapter.replace("return size;", "size.m_y ^= 1u; return size;")),
                  (base, adapter.replace("return size;", "m_map->getSize(); return size;"))]
        for index, (base, adapter) in enumerate(pairs):
            text += "namespace N%d {\n" % index
            text += "struct type_random_map { TRmgMapPosition m_size; virtual TRmgGridPoint getSize(); };\n"
            text += base + "\n"
            text += "struct ScriptedMap : type_random_map { int m_calls; TRmgGridPoint getSize() { ++m_calls; TRmgGridPoint size=type_random_map::getSize(); m_size.m_x ^= 0x55; m_size.m_y ^= 0x33; return size; } };\n"
            for owner in ("TRmgMapAdapter", "TRmgRoadMapAdapter"):
                text += "struct " + owner + " { type_random_map* m_map; TRmgGridPoint getSize(); };\n"
                text += adapter.replace("TRmgMapAdapter::", owner + "::") + "\n"
            text += r'''
bool check() {
    int dimensions[] = {0, 1, -1, 36, 144, INT_MIN, INT_MAX};
    for(int x=0;x<7;++x) for(int y=0;y<7;++y) {
        type_random_map map; map.m_size.m_x=dimensions[x]; map.m_size.m_y=dimensions[y];
        TRmgGridPoint direct=map.getSize();
        if(direct.m_x!=static_cast<unsigned>(dimensions[x]) || direct.m_y!=static_cast<unsigned>(dimensions[y])) return false;
        if(map.m_size.m_x!=dimensions[x] || map.m_size.m_y!=dimensions[y]) return false;
        for(int road=0;road<2;++road) {
            ScriptedMap scripted; scripted.m_size.m_x=dimensions[x]; scripted.m_size.m_y=dimensions[y]; scripted.m_calls=0;
            TRmgMapAdapter river; river.m_map=&scripted;
            TRmgRoadMapAdapter roads; roads.m_map=&scripted;
            TRmgGridPoint result=road ? roads.getSize() : river.getSize();
            if(result.m_x!=static_cast<unsigned>(dimensions[x]) || result.m_y!=static_cast<unsigned>(dimensions[y])) return false;
            if(scripted.m_calls!=1 || scripted.m_size.m_x!=(dimensions[x]^0x55) || scripted.m_size.m_y!=(dimensions[y]^0x33)) return false;
        }
    }
    return true;
}
}
'''
        text += "int main() {\n"
        for index in range(len(pairs)):
            condition = "!" if index < 60 else ""
            text += 'if(%sN%d::check()) { std::printf("failed form %d\\n"); return 1; }\n' % (condition, index, index)
        text += 'std::puts("60 size-return pairs: 49 dimension pairs, both adapters; three broken controls rejected"); return 0; }\n'
        with tempfile.TemporaryDirectory(prefix="rmg-size-return-oracle-") as folder:
            source, binary = Path(folder) / "oracle.cpp", Path(folder) / "oracle"
            source.write_text(text)
            subprocess.run(["g++", "-std=c++98", "-O2", "-fno-elide-constructors", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
