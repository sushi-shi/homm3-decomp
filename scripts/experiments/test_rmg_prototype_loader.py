"""Filtering/remapping and prototype-only sorting for the actual loader bodies."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator
from homm3.vc6 import source_families


class PrototypeLoaderTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_filters_remapping_and_owned_references(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-prototype-loader-family.py")
        helper = generator("generate-rmg-position-family.py")
        source = (root / module.SOURCE).read_text()
        header = (root / "include/rmg.h").read_text()
        objects = (root / "include/advmgr_objects.h").read_text()
        mapcell = (root / "include/mapcell.h").read_text()
        start = mapcell.index("enum TAdventureObjectType {")
        enum = mapcell[start:mapcell.index("\n};", start) + 3]
        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]
        def field(text, name):
            return next(line.split("//")[0].strip() for line in text.splitlines() if name + ";" in line)
        properties = block("TRmgObjectPropertiesRef").replace(
            "    TRmgObjectPropertiesRef(TObjectType* prototype);",
            "    TRmgObjectPropertiesRef(TObjectType* prototype);\n"
            "    static void* operator new(size_t size) { void* p=::operator new(size); g_allocations.push_back(p); return p; }\n"
            "    static void operator delete(void* p) { ::operator delete(p); }")
        types = enum + "\n" + block("TRmgVector") + "\n" + block("TPoint")
        types += "\nstruct TObjectType { " + field(objects, "m_objectType") + " " + field(objects, "m_subtype") + " };\n"
        types += "struct TRmgObjectPlacementRule;\nstatic std::vector<void*> g_allocations;\n" + properties
        ctor = helper.definition(source, "TRmgObjectPropertiesRef::TRmgObjectPropertiesRef")
        forms = [option["replace"] for option in module.variants(source)]
        if os.environ.get("HOMM3_PROTOTYPE_LOADER_MANIFEST"):
            path = Path(os.environ["HOMM3_PROTOTYPE_LOADER_MANIFEST"])
            payload, originals, axes = source_families.load_manifest(path, root)
            forms = [helper.definition(source_families.render(originals, axes, (index,))[module.SOURCE], module.NAME)
                     for index in range(len(payload["axes"][0]["options"]))]
        count = len(forms)
        seed = module.historical_definition(helper.definition(source, module.NAME))
        forms += [seed.replace("type >= 222", "type > 222"),
                  seed.replace("type >= 165", "type > 165"),
                  seed.replace("object.m_subtype >= 3", "object.m_subtype > 3"),
                  seed.replace("memcpy(&type, &g_adventureObjectLandBlocked[type][8], sizeof(type));", ""),
                  seed.replace("std::swap(m_objectPrototypes[54][first]->m_prototype, m_objectPrototypes[54][second]->m_prototype);",
                               "std::swap(m_objectPrototypes[54][first], m_objectPrototypes[54][second]);"),
                  seed.replace("m_progress->advance(15300)", "m_progress->advance(15301)")]
        self.assertTrue(all(form != seed for form in forms[count:]))
        text = "#include <vector>\n#include <algorithm>\n#include <cstring>\n#include <cstdio>\n#include <new>\n"
        for index, body in enumerate(forms):
            # VC6 retains a scalar for-initializer variable in the containing
            # scope. Give the host the same lifetime without editing VC6 input.
            if "for (index = 0; index < m_objectPrototypes[54]" in body:
                body = body.replace("    for (unsigned int index = 0; index <",
                                    "    unsigned int index = 0;\n    for (; index <")
            text += f"namespace Case{index} {{\n" + types + "\n" + ctor + r"""
static unsigned char g_traits[232][16];
static const unsigned char (*g_adventureObjectLandBlocked)[16]=g_traits;
static std::vector<TObjectType> g_input;
static bool g_loaded,g_bound,g_badPhase;
struct TObjectTypeTable {
    std::vector<TObjectType> m_objectTypes;
    void load(const char* name) {
        if(std::strcmp(name,"objects.txt")) g_badPhase=true;
        m_objectTypes=g_input;g_loaded=true;
    }
};
struct TProgressSink {
    int m_calls,m_amount;
    TProgressSink():m_calls(0),m_amount(0) {}
    void advance(int amount) { if(!g_bound) g_badPhase=true; ++m_calls;m_amount+=amount; }
};
struct TRmgGeneratorBase {
    int m_mapVersion;
    TObjectTypeTable m_objectsTxt;
    std::vector<TRmgObjectPropertiesRef*> m_objectPrototypes[232];
    TProgressSink* m_progress;
    TProgressSink* m_replacement;
    void loadObjectPrototypes();
    void readObjectPlacementRules() { if(!g_loaded) g_badPhase=true; g_bound=true;m_progress=m_replacement; }
    ~TRmgGeneratorBase() { for(int i=0;i<232;++i) for(unsigned j=0;j<m_objectPrototypes[i].size();++j) delete m_objectPrototypes[i][j]; }
};
""" + body + r"""
bool check() {
    for(int version=-1;version<=3;++version) for(int pattern=0;pattern<6;++pattern)
    for(int remap=0;remap<3;++remap) for(int progress=0;progress<2;++progress) {
        g_allocations.clear();g_input.clear();g_loaded=g_bound=g_badPhase=false;
        int mapping[232];
        for(int type=0;type<232;++type) {
            mapping[type]=(remap && type%11==0) ? 54 : type;
            if(remap==2 && type%13==0) mapping[type]=17;
            std::memcpy(&g_traits[type][8],&mapping[type],sizeof(int));
        }
        for(int type=0;type<256;++type) {
            TObjectType object;object.m_objectType=static_cast<TAdventureObjectType>(type);
            object.m_subtype=(type+pattern)%6;g_input.push_back(object);
        }
        std::vector<int> expected[232],ownership[232];
        int allocations=0;
        for(unsigned i=0;i<g_input.size();++i) {
            int type=g_input[i].m_objectType,subtype=g_input[i].m_subtype;
            bool eligible=type<232;
            if(version<2 && type>=222) eligible=false;
            if(version<1 && type>=165) eligible=false;
            if(version<2 && (type==43 || type==44 || type==45) && subtype>=3) eligible=false;
            if(eligible) { expected[mapping[type]].push_back(i);ownership[mapping[type]].push_back(allocations++); }
        }
        TRmgGeneratorBase graph;TProgressSink original,replacement;
        graph.m_mapVersion=version;graph.m_progress=&original;
        graph.m_replacement=progress ? &replacement : 0;
        graph.loadObjectPrototypes();
        if(g_badPhase || !g_loaded || !g_bound || int(g_allocations.size())!=allocations
            || original.m_calls || replacement.m_calls!=progress || replacement.m_amount!=15300*progress) return false;
        for(int type=0;type<232;++type) {
            if(graph.m_objectPrototypes[type].size()!=expected[type].size()) return false;
            std::vector<int> actual;
            int previous=-1;
            for(unsigned n=0;n<expected[type].size();++n) {
                TRmgObjectPropertiesRef* properties=graph.m_objectPrototypes[type][n];
                if(properties!=g_allocations[ownership[type][n]] || properties->m_preferredTerrain!=-1
                    || properties->m_refCount || properties->m_prototypeIndex || properties->m_placementRule
                    || properties->m_prioritiesInitialized || !properties->m_outline.empty()) return false;
                int found=-1;
                for(unsigned p=0;p<graph.m_objectsTxt.m_objectTypes.size();++p)
                    if(properties->m_prototype==&graph.m_objectsTxt.m_objectTypes[p]) found=p;
                if(found<0) return false;
                actual.push_back(found);
                if(type==54) { int subtype=properties->m_prototype->m_subtype;if(subtype<previous) return false;previous=subtype; }
            }
            if(type==54) { std::sort(actual.begin(),actual.end());std::sort(expected[type].begin(),expected[type].end()); }
            if(actual!=expected[type]) return false;
        }
    }
    return true;
}
}
"""
        text += "int main(){\n"
        for index in range(len(forms)):
            condition = f"!Case{index}::check()" if index < count else f"Case{index}::check()"
            text += f'if({condition}) {{ std::printf("failed loader form {index}\\n"); return 1; }}\n'
        text += "return 0;}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-prototype-loader-") as folder:
            cpp,binary=Path(folder)/"oracle.cpp",Path(folder)/"oracle"
            cpp.write_text(text)
            subprocess.run(["g++","-std=c++98","-O1",str(cpp),"-o",str(binary)],check=True)
            subprocess.run([str(binary)],check=True,timeout=60)


if __name__ == "__main__":
    unittest.main()
