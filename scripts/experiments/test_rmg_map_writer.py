"""Ordered-stream contract for map-writer alternatives; reduced native owners."""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class MapWriterTests(unittest.TestCase):
    def test_owner_rewrites_preserve_member_paths(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-map-writer-scans-family.py")
        source = (root / module.SOURCE).read_text()
        for option in module.axes(source)[0]["options"]:
            for name, body in module.owners(option["replace"]):
                self.assertEqual(body.count("object->m_properties->m_prototype"), 2)
                self.assertNotIn("m_properties.m_prototype", body)
                if name in ("const_receivers", "const_array_pointer", "const_array_reference"):
                    self.assertNotIn("::iterator entry", body)

    @unittest.skipUnless(shutil.which("g++"), "requires native compiler")
    def test_stream_order_indices_returns_and_exceptions(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-map-writer-family.py")
        source = (root / module.SOURCE).read_text()
        original = module.definition(source)
        forms = [option["replace"] for option in module.axes(source)[0]["options"]]
        self.assertEqual(len(forms), 60)
        for selected in filter(None, os.environ.get("HOMM3_MAP_WRITER_MANIFEST", "").split(":")):
            path = Path(selected)
            if not path.is_absolute():
                path = root / path
            payload = json.loads(path.read_text())
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
        positives = len(forms)
        for before, after in (
            ("prototypeCount = 2", "prototypeCount = 3"),
            ("static_cast<int>(properties->m_refCount) > 0", "properties->m_refCount > 0"),
            ("m_objectPrototypes[124][0]", "m_objectPrototypes[71][0]"),
            ("++item;", "item += 0;"),
            ("int count = m_positions.size();", "int count = m_positions.size() + 1;"),
            ("advance(2000)", "advance(1999)"),
            ("== sizeof(reserved)", "!= sizeof(reserved)"),
            ("if (!g_adventureObjectLandBlocked", "if (g_adventureObjectLandBlocked"),
        ):
            self.assertIn(before, original)
            forms.append(original.replace(before, after))
        cached = module.replace(original,
            "    for (int objectType = 0; objectType < 232; ++objectType)\n        for (unsigned int prototype = 0; prototype < m_objectPrototypes[objectType].size(); ++prototype) {",
            "    for (int objectType = 0; objectType < 232; ++objectType) {\n        unsigned int cachedSize = m_objectPrototypes[objectType].size();\n        for (unsigned int prototype = 0; prototype < cachedSize; ++prototype) {")
        cached = module.replace(cached,
            "                writeRmgObjectPrototype(outfile, properties->m_prototype);\n        }",
            "                writeRmgObjectPrototype(outfile, properties->m_prototype);\n        }\n    }")
        forms.append(cached)
        header = (root / "include/rmg.h").read_text()
        start = header.index("struct TRmgMapPosition {")
        position = header[start:header.index("\n};", start) + 3]
        # Actual coordinate declaration and abstract stream, reduced other owners.
        fixture = r'''
#include <vector>
#include <algorithm>
#include <cstring>
#include <climits>
#include <iostream>
#define __fastcall
struct TPoint;
ABSTRACT
POSITION
struct Event { int kind, value; bool operator==(const Event& b) const {return kind==b.kind && value==b.value;} };
static std::vector<Event> events;
static int throwAt, callbacks;
static void record(int kind,int value) {events.push_back({kind,value}); if(++callbacks==throwAt) throw 1;}
struct Sink: TAbstractFile {
    int result;
    int read(void*,int) {return 0;}
    int write(const void* data,int size) {
        if(size!=4) throw 2;
        int value; std::memcpy(&value,data,4); record(1,value); return result;
    }
};
struct TObjectType { int m_objectType; int id; };
struct TRmgObjectPropertiesRef { TObjectType* m_prototype; unsigned m_refCount; int m_prototypeIndex; };
struct TRmgMapItem {int id; void write(TAbstractFile*) {record(2,id);} };
struct type_object { TRmgObjectPropertiesRef* m_properties; int id;
    void write(TAbstractFile*,int version) {record(4,id*100+version);} };
struct Progress { void advance(int amount) {record(5,amount);} };
static unsigned char g_adventureObjectLandBlocked[232][13];
void writeRmgObjectPrototype(TAbstractFile*,TObjectType* p);
struct Map {TRmgMapItem* m_mapItems; int m_numberLevels,m_mapHeight,m_mapWidth;};
struct type_random_map_generator {
    Map m_map;
    std::vector<TRmgObjectPropertiesRef*> m_objectPrototypes[232];
    std::vector<type_object*> m_positions;
    Progress* m_progress;
    int m_mapVersion;
    bool mutate;
    void writeMapHeader(TAbstractFile*) {record(0,m_mapVersion); if(mutate) ++m_map.m_mapWidth;}
    METHODS
};
static type_random_map_generator* active;
static TRmgObjectPropertiesRef* appended;
static bool appendOnWrite;
void writeRmgObjectPrototype(TAbstractFile*,TObjectType* p) {
    record(3,p->id);
    if(appendOnWrite && p->id==10) {
        appendOnWrite=false;
        active->m_objectPrototypes[231].push_back(appended);
    }
}
DEFINITIONS
typedef unsigned char(type_random_map_generator::*Writer)(TAbstractFile*);
static Writer writers[]={POINTERS};
int main() {
    unsigned total=0;
    for(unsigned form=0; form<sizeof(writers)/sizeof(writers[0]); ++form) {
        bool rejected=false;
        for(int shape=0;shape<12 && !rejected;++shape)
        for(int pattern=0;pattern<6 && !rejected;++pattern)
        for(int mode=0;mode<8 && !rejected;++mode)
        for(int resultIndex=0;resultIndex<5 && !rejected;++resultIndex) {
            type_random_map_generator g;
            TRmgMapItem cells[64]; for(int i=0;i<64;++i) cells[i].id=i;
            const int width=shape%4, height=(shape/4)+1, levels=shape%3;
            g.m_map={cells,levels,height,width}; g.mutate=(mode&1)!=0;
            Progress progress; g.m_progress=(mode&2)?&progress:0; g.m_mapVersion=pattern+14;
            TObjectType types[12]; TRmgObjectPropertiesRef props[12]; type_object objects[12];
            const int buckets[12]={0,0,7,71,71,124,124,180,200,230,231,231};
            const unsigned refs[6]={0,1,2,0x7fffffffu,0x80000000u,0xffffffffu};
            std::memset(g_adventureObjectLandBlocked,0,sizeof(g_adventureObjectLandBlocked));
            for(int i=0;i<12;++i) {
                types[i]={buckets[i],i}; props[i]={&types[i],refs[(i+pattern)%6],-100-i};
                g.m_objectPrototypes[buckets[i]].push_back(&props[i]); objects[i]={&props[i],i};
                g_adventureObjectLandBlocked[buckets[i]][12]=((buckets[i]+pattern)%3)?255:0;
            }
            int objectCount=pattern*2;
            for(int i=0;i<objectCount;++i) g.m_positions.push_back(&objects[(i*5)%12]);
            std::vector<Event> expected; expected.push_back({0,g.m_mapVersion}); expected.push_back({1,0});
            int cellCount=(width+int(g.mutate))*height*levels;
            for(int i=0;i<cellCount;++i) expected.push_back({2,i});
            int indices[12], count=2;
            for(int i=0;i<12;++i) indices[i]=(props[i].m_refCount>0 && props[i].m_refCount<=unsigned(INT_MAX))?count++:-100-i;
            int countEvent=expected.size(); expected.push_back({1,count});
            expected.push_back({3,3}); expected.push_back({3,5});
            for(int i=0;i<12;++i) if(indices[i]>=2) expected.push_back({3,i});
            if((mode&4) && indices[10]>=2 && indices[11]>=2) expected.push_back({3,11});
            expected.push_back({1,objectCount});
            std::vector<type_object*> ordered=g.m_positions;
            std::stable_partition(ordered.begin(),ordered.end(),[](type_object* o){return g_adventureObjectLandBlocked[o->m_properties->m_prototype->m_objectType][12]!=0;});
            for(auto o:ordered) expected.push_back({4,o->id*100+g.m_mapVersion});
            if(g.m_progress) expected.push_back({5,2000}); expected.push_back({1,0});
            const int returns[5]={4,0,-1,3,5}; Sink sink; sink.result=returns[resultIndex];
            for(int stop=0; stop<=int(expected.size()) && !rejected; ++stop) {
                events.clear(); callbacks=0; throwAt=stop;
                active=&g; appended=&props[11]; appendOnWrite=(mode&4)!=0;
                g.m_objectPrototypes[231].resize(2);
                g.m_map.m_mapWidth=width;
                for(int i=0;i<12;++i) props[i].m_prototypeIndex=-100-i;
                bool threw=false; int result=-1;
                try {result=(g.*writers[form])(&sink);} catch(int code) {threw=code==1;}
                size_t expectedSize=stop?stop:expected.size();
                bool okay=events.size()==expectedSize && std::equal(events.begin(),events.end(),expected.begin());
                okay=okay && (stop?threw:(!threw && result==int(sink.result==4)));
                for(int i=0;i<12;++i) {
                    int want=(!stop || stop>countEvent)?indices[i]:-100-i;
                    okay=okay && props[i].m_prototypeIndex==want && props[i].m_refCount==refs[(i+pattern)%6];
                }
                if(!okay) {
                    if(form<POSITIVES) {std::cerr<<"form "<<form<<" shape "<<shape<<" pattern "<<pattern<<" mode "<<mode<<" stop "<<stop<<" failed\n";return 1;}
                    rejected=true;
                }
                if(form==0) ++total;
            }
        }
        if(form>=POSITIVES && !rejected) {std::cerr<<"negative passed "<<form<<'\n';return 2;}
    }
    std::cout<<POSITIVES<<" map writers: "<<total<<" scenarios each; nine negative controls rejected\n";
}
'''
        names = [f"writer{i}" for i in range(len(forms))]
        fixture = fixture.replace("ABSTRACT", (root / "include/abstractfile.h").read_text()).replace("POSITION", position)
        fixture = fixture.replace("METHODS", "\n".join("unsigned char " + name + "(TAbstractFile*);" for name in names))
        fixture = fixture.replace("DEFINITIONS", "\n".join(body.replace("::writeMap(", "::" + name + "(") for body, name in zip(forms, names)))
        fixture = fixture.replace("POINTERS", ",".join("&type_random_map_generator::" + name for name in names)).replace("POSITIVES", str(positives))
        with tempfile.TemporaryDirectory(prefix="rmg-map-writer-") as raw:
            path = Path(raw)
            (path / "test.cpp").write_text(fixture)
            subprocess.run(["g++", "-std=c++11", "-O2", str(path / "test.cpp"), "-o", str(path / "test")], check=True)
            subprocess.run([str(path / "test")], check=True)


if __name__ == "__main__":
    unittest.main()
