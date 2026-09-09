"""Key-tent caller ownership oracle; opaque helper phase machine, not x86 ABI."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class KeyTentTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_reservation_ownership_and_cleanup(self):
        root = Path(__file__).resolve().parents[2]
        source = (root / "src/rmg.cpp").read_text()
        header = (root / "include/rmg.h").read_text()
        support = (root / "src/rmg_support.cpp").read_text()
        objects = (root / "include/advmgr_objects.h").read_text()
        mapcell = (root / "include/mapcell.h").read_text()
        module = generator("generate-rmg-key-tent-family.py")
        definition = generator("generate-rmg-position-family.py").definition
        original = module.definition(source)
        forms = [(body, name, "") for name, body in module.variants(original)]
        for path in filter(None, os.environ.get("HOMM3_KEY_TENT_MANIFEST", "").split(os.pathsep)):
            payload = json.loads(Path(path).read_text())
            for option in payload["axes"][0]["options"]:
                helpers = [edit["text"] for edit in option.get("extra_edits", [])
                           if "void type_random_map_generator::" in edit.get("text", "")]
                self.assertLessEqual(len(helpers), 1)
                forms.append((option["replace"], option["name"], helpers[0] if helpers else ""))
        forms = [(body, name, helper) for (body, helper), name in
                 dict(((body, helper), name) for body, name, helper in forms).items()]
        positive_count = len(forms)
        negatives = [
            ("wrong_color", "int color = object->m_properties->m_prototype->m_subtype;", "int color = 0;"),
            ("missing_reservation", "m_disabledKeyTents[color] = 1;", "m_disabledKeyTents[color] = 0;"),
            ("missing_rollback", "m_disabledKeyTents[color] = 0;", "m_disabledKeyTents[color] = 1;"),
            ("next_color", "m_nextKeyTentColor = 0;", "m_nextKeyTentColor = 1;"),
            ("wrong_value", "0, maxValue)", "0, maxValue + 1)"),
            ("missing_outline", "m_placementOutline = 1;", "m_placementOutline = 0;"),
            ("missing_release", "group.m_objects[i]->unknownOperation();", ""),
            ("missing_reset", "    group.reset();", ""),
            ("missing_guard_delete", "        delete guard;", ""),
            ("missing_object_delete", "        delete group.m_objects[i];", ""),
            ("wrong_origin", "m_zones[m_map.getMapItem(object->m_position)->m_zoneState.m_zone]", "m_zones[0]"),
            ("last_prototype", "TRmgObjectPropertiesRef* properties = m_objectPrototypes[BORDER_GUARD][index];",
             "TRmgObjectPropertiesRef* properties = m_objectPrototypes[BORDER_GUARD][m_objectPrototypes[BORDER_GUARD].size()-1];"),
            ("cached_cleanup", "for (unsigned int i = 0; i < group.m_objects.size(); ++i)",
             "unsigned int objectCount = group.m_objects.size();\n    for (unsigned int i = 0; i < objectCount; ++i)"),
        ]
        for name, old, new in negatives:
            self.assertIn(old, original, name)
            forms.append((original.replace(old, new), name, ""))

        def block(text, prefix):
            start = text.index(prefix + " {")
            return text[start:text.index("\n};", start) + 3]

        def field(text, name):
            return next(line.split("//")[0].strip() for line in text.splitlines() if name + ";" in line)

        text = "#include <vector>\n#include <cstring>\n#include <cstdio>\n#include <new>\n#include <algorithm>\n"
        for name in ("TRmgVector", "TPoint", "TRmgMapPosition", "TRmgZoneBounds", "TRmgZoneCellState", "TRmgGroundTileData"):
            text += block(header, "struct " + name) + "\n"
        text += definition(support, "TRmgMapPosition::TRmgMapPosition") + "\n"
        text += block(mapcell, "enum TAdventureObjectType") + "\n"
        text += "struct TObjectType { " + field(objects, "m_subtype") + " };\n"
        text += "struct TRmgObjectPropertiesRef { " + field(header, "m_prototype") + " " + field(header, "m_refCount") + " };\n"
        text += r'''
struct type_object;
struct TRmgTreasureGroup;
struct TRmgZone { int m_identity; };
static std::vector<int> g_trace;
static std::vector<type_object*> g_allocations;
static std::vector<unsigned char> g_alive;
static bool g_record,g_valid,g_append;
static int g_fill,g_add,g_place,g_count,g_outline,g_color,g_expectedZone,g_maxValue;
static int g_nextExpected;
static std::vector<unsigned char>* g_disabled;
static int* g_next;
static TRmgTreasureGroup* g_group;
static TRmgObjectPropertiesRef* g_fillProperties;
static std::vector<int> g_finalBits;
static void event(int code,int value=0) { if(g_record) { g_trace.push_back(code);g_trace.push_back(value); } }
static int identity(type_object* p) {
    for(int i=int(g_allocations.size())-1;i>=0;--i) if(g_allocations[i]==p && g_alive[i]) return i;
    return int(g_allocations.size());
}
struct type_object {
'''
        for name in ("m_properties", "m_position", "m_candidateCovers", "m_candidateBehind", "m_adjacentToCandidate", "m_overlapsCandidate", "m_blockedByCandidate"):
            text += field(header, name) + "\n"
        text += r'''
    type_object(TRmgObjectPropertiesRef*);
    void clearPlacementMarks();
    virtual ~type_object();
    virtual void unknownOperation();
    static void* operator new(std::size_t n) {
        void* p=::operator new(n);
        g_allocations.push_back(static_cast<type_object*>(p));g_alive.push_back(1);
        event(10,int(g_allocations.size())-1);return p;
    }
    static void operator delete(void* p) {
        int id=identity(static_cast<type_object*>(p));
        if(id>=int(g_alive.size()) || !g_alive[id]) { g_valid=false;return; }
        event(11,id);g_alive[id]=0;::operator delete(p);
    }
};
'''
        for name in ("type_object::type_object", "type_object::~type_object", "type_object::clearPlacementMarks"):
            text += definition(source, name) + "\n"
        text += r'''
struct TRmgMapItem { TRmgZoneCellState m_zoneState; TRmgGroundTileData m_tileData; };
struct type_random_map {
    int m_mapWidth,m_mapHeight,m_numberLevels;
    TRmgMapItem* m_mapItems;
    bool m_owned;
    type_random_map():m_mapItems(0),m_owned(false) {}
    type_random_map(int w,int h,int levels):m_mapWidth(w),m_mapHeight(h),m_numberLevels(levels),m_owned(true) {
        m_mapItems=new TRmgMapItem[w*h*levels];
        std::memset(m_mapItems,0,sizeof(TRmgMapItem)*w*h*levels);
        for(int i=0;i<w*h*levels;++i) m_mapItems[i].m_tileData.m_roadEntrance=i&1;
        event(1,w*10000+h*10+levels);
    }
    ~type_random_map() {
        if(m_owned) {
            g_finalBits.clear();
            for(int i=0;i<m_mapWidth*m_mapHeight;++i) {
                g_finalBits.push_back(m_mapItems[i].m_tileData.m_placementOutline);
                if(m_mapItems[i].m_tileData.m_roadEntrance!=(i&1)) g_valid=false;
            }
            event(9,(*g_disabled)[g_color]*100+*g_next);
            g_group=0;delete[] m_mapItems;
        }
    }
    TRmgMapItem* getMapItem(TRmgMapPosition point);
    TRmgMapItem* getMapItem(int x,int y);
'''
        text += definition(header, "getMapItem", parameters="int x, int y, int z") + "\n};\n"
        text += definition(source, "type_random_map::getMapItem", parameters="TRmgMapPosition point") + "\n"
        text += definition(source, "type_random_map::getMapItem", parameters="int x, int y") + "\n"
        text += block(header, "struct TRmgTreasureGroup") + "\n"
        text += r'''
void TRmgTreasureGroup::reset() {
    g_group=this;event(2,int(m_objects.size()));m_objects.clear();m_outline.clear();
    m_ready=0;m_hasGuard=0;
    for(int i=0;i<m_map.m_mapWidth*m_map.m_mapHeight;++i) m_map.m_mapItems[i].m_tileData.m_placementOutline=0;
}
unsigned char TRmgTreasureGroup::addGuard(type_object* guard) {
    event(4,identity(guard));if(g_add) m_objects.push_back(guard);return g_add;
}
void TRmgTreasureGroup::updateBounds() { event(5); }
void TRmgTreasureGroup::traceOutline() {
    event(6);
    for(int i=0;i<g_outline;++i) {
        TPoint point;point.m_x=2+i*3;point.m_y=1+i*2;m_outline.push_back(point);
    }
}
void type_object::unknownOperation() {
    event(8,identity(this));
    if(g_append) { g_append=false;g_group->m_objects.push_back(new type_object(g_fillProperties)); }
}
struct GeneratorFixture {
    type_random_map m_map;
    std::vector<TRmgZone*> m_zones;
    std::vector<TRmgObjectPropertiesRef*> m_objectPrototypes[232];
    std::vector<unsigned char> m_disabledKeyTents;
    int m_nextKeyTentColor;
    int fillTreasureGroup(TRmgZone* zone,TRmgTreasureGroup* group,unsigned char mode,int value) {
        event(3,zone->m_identity*100+mode);
        if(zone->m_identity!=g_expectedZone || value!=g_maxValue || mode || !m_disabledKeyTents[g_color]
            || m_nextKeyTentColor!=g_nextExpected) g_valid=false;
        for(int i=0;i<g_count;++i) group->m_objects.push_back(new type_object(g_fillProperties));
        return g_fill;
    }
    unsigned char placeQuestGroup(TRmgTreasureGroup* group,TRmgZone* zone) {
        event(7,zone->m_identity);
        if(zone->m_identity!=g_expectedZone || !group->m_ready) g_valid=false;
        for(int i=0;i<256;++i) {
            bool marked=false;
            for(int k=0;k<g_outline;++k) if(i==(1+k*2)*16+2+k*3) marked=true;
            if(group->m_map.m_mapItems[i].m_tileData.m_placementOutline!=marked) g_valid=false;
        }
        return g_place;
    }
};
static int firstFree(const std::vector<unsigned char>& disabled) {
    std::vector<int> available;
    for(int i=0;i<int(disabled.size());++i) if(!disabled[i]) available.push_back(i);
    return available.empty()?int(disabled.size()):available.front();
}
template<class Candidate> bool check() {
    int colors[]={0,1,3},masks[]={0,5,15},fills[]={0,1,-1},values[]={0,1,255};
    for(int ci=0;ci<3;++ci) for(int mi=0;mi<3;++mi) for(int level=0;level<2;++level)
    for(int fi=0;fi<3;++fi) for(int ai=0;ai<3;++ai) for(int pi=0;pi<3;++pi)
    for(int oi=0;oi<3;++oi) for(int count=0;count<3;++count) for(int match=0;match<2;++match)
    for(int append=0;append<2;++append) {
        Candidate owner;TRmgZone zones[2];zones[0].m_identity=0;zones[1].m_identity=1;
        owner.m_zones.push_back(&zones[0]);owner.m_zones.push_back(&zones[1]);
        std::vector<TRmgMapItem> cells(12);std::memset(&cells[0],0,sizeof(cells[0])*cells.size());
        for(int i=0;i<12;++i) cells[i].m_zoneState.m_zone=(i/6)^1;
        owner.m_map.m_mapWidth=3;owner.m_map.m_mapHeight=2;owner.m_map.m_numberLevels=2;owner.m_map.m_mapItems=&cells[0];
        std::vector<TRmgMapItem> saved=cells;
        TObjectType prototypes[5];TRmgObjectPropertiesRef properties[5];
        for(int i=0;i<5;++i) { prototypes[i].m_subtype=i;properties[i].m_prototype=&prototypes[i];properties[i].m_refCount=100; }
        int color=colors[ci];
        prototypes[4].m_subtype=color;
        // A wrong-color prefix and duplicate matching suffix check first-match selection.
        owner.m_objectPrototypes[BORDER_GUARD].push_back(&properties[(color+1)%4]);
        if(match) { owner.m_objectPrototypes[BORDER_GUARD].push_back(&properties[color]);owner.m_objectPrototypes[BORDER_GUARD].push_back(&properties[4]); }
        owner.m_disabledKeyTents.resize(4);
        for(int i=0;i<4;++i) owner.m_disabledKeyTents[i]=(masks[mi]>>i)&1;
        owner.m_nextKeyTentColor=firstFree(owner.m_disabledKeyTents);
        std::vector<unsigned char> expectedFlags=owner.m_disabledKeyTents;
        const std::vector<unsigned char> originalFlags=expectedFlags;
        int originalNext=owner.m_nextKeyTentColor;
        type_object tent(&properties[color]);tent.m_position.m_x=2;tent.m_position.m_y=1;tent.m_position.m_z=level;
        TRmgMapPosition input=tent.m_position;
        g_trace.clear();g_allocations.clear();g_alive.clear();g_finalBits.clear();g_valid=true;g_record=true;
        g_color=color;g_fill=fills[fi];g_add=values[ai];g_place=values[pi];g_count=count;g_outline=oi;
        g_expectedZone=level^1;g_maxValue=13+ci*19;g_append=append;g_group=0;
        g_disabled=&owner.m_disabledKeyTents;g_next=&owner.m_nextKeyTentColor;g_fillProperties=&properties[(color+2)%4];
        expectedFlags[color]=1;g_nextExpected=firstFree(expectedFlags);
        unsigned char actual=owner.placeKeyTentGuard(&tent,g_maxValue);
        bool success=match && g_fill && g_add && g_place;
        std::vector<int> expected;
        #define EXPECT(code,value) do { expected.push_back(code);expected.push_back(value); } while(0)
        if(match) {
            EXPECT(1,160161);EXPECT(2,0);EXPECT(10,0);EXPECT(3,g_expectedZone*100);
            std::vector<int> owned;
            for(int i=0;i<count;++i) { EXPECT(10,i+1);owned.push_back(i+1); }
            if(g_fill) { EXPECT(4,0);if(g_add) owned.push_back(0); }
            if(g_fill && g_add) { EXPECT(5,0);EXPECT(6,0);EXPECT(7,g_expectedZone); }
            else EXPECT(11,0);
            if(!success) {
                if(append && !owned.empty()) owned.push_back(count+1);
                for(int i=0;i<int(owned.size());++i) {
                    EXPECT(8,owned[i]);if(i==0 && append) EXPECT(10,count+1);EXPECT(11,owned[i]);
                }
                EXPECT(2,int(owned.size()));expectedFlags[color]=0;
            }
            EXPECT(9,expectedFlags[color]*100+firstFree(expectedFlags));
        } else {
            expectedFlags=originalFlags;
        }
        #undef EXPECT
        bool ok=g_valid && actual==success && g_trace==expected
            && owner.m_disabledKeyTents==expectedFlags
            && owner.m_nextKeyTentColor==(match?firstFree(expectedFlags):originalNext)
            && std::memcmp(&cells[0],&saved[0],sizeof(cells[0])*cells.size())==0
            && tent.m_position.m_x==input.m_x && tent.m_position.m_y==input.m_y && tent.m_position.m_z==input.m_z;
        for(int i=0;i<int(g_alive.size());++i) if(bool(g_alive[i])!=success) ok=false;
        for(int i=0;i<5;++i) {
            unsigned expectedRefs=100+(i==color);
            if(success) expectedRefs+=(i==color)+(i==(color+2)%4?count:0);
            if(properties[i].m_refCount!=expectedRefs) ok=false;
        }
        g_record=false;
        for(int i=0;i<int(g_alive.size());++i) if(g_alive[i]) delete g_allocations[i];
        if(!ok) return false;
    }
    return true;
}
'''
        for index, (body, name, helper) in enumerate(forms):
            declaration = ""
            if helper:
                start = helper.index("void type_random_map_generator::")
                declaration = helper[start:helper.index("\n{", start)].replace("type_random_map_generator::", "") + ";"
            text += "namespace N%d { struct type_random_map_generator : GeneratorFixture { unsigned char placeKeyTentGuard(type_object*,int); %s };\n" % (index, declaration)
            text += body + "\n" + helper + "\n}\n"
            if helper:
                invocation = "owner.setKeyTentDisabled(color,value);" if "::setKeyTentDisabled" in helper else "owner.m_disabledKeyTents[color]=value;owner.refreshKeyTentColor();"
                text += r'''
bool helperCheckINDEX() {
    for(int length=1;length<=8;++length) for(int mask=0;mask<(1<<length);++mask)
    for(int color=0;color<length;++color) for(int vi=0;vi<3;++vi) {
        int values[]={0,1,255};unsigned char value=values[vi];
        NINDEX::type_random_map_generator owner;owner.m_disabledKeyTents.resize(length);
        for(int i=0;i<length;++i) owner.m_disabledKeyTents[i]=(mask>>i)&1;
        owner.m_nextKeyTentColor=99;
        std::vector<unsigned char> expected=owner.m_disabledKeyTents;
        expected[color]=value;
        INVOCATION
        if(owner.m_disabledKeyTents!=expected || owner.m_nextKeyTentColor!=firstFree(expected)) return false;
    }
    return true;
}
'''.replace("INDEX", str(index)).replace("INVOCATION", invocation)
        text += "int main() {\n"
        for index, (body, name, helper) in enumerate(forms):
            expected = "true" if index < positive_count else "false"
            if helper:
                text += 'if(!helperCheck%d()) { std::fprintf(stderr,"failed helper %s\\n");return 1;}\n' % (index, name)
            text += 'if(check<N%d::type_random_map_generator>() != %s) { std::fprintf(stderr,"failed %s\\n"); return 1; }\n' % (index, expected, name)
        text += 'std::printf("%d key-tent forms; 17496 scenarios each; thirteen negative controls rejected\\n");}\n' % positive_count
        with tempfile.TemporaryDirectory(prefix="homm3-key-tent-") as directory:
            source_path = Path(directory) / "oracle.cpp"
            source_path.write_text(text)
            executable = Path(directory) / "oracle"
            subprocess.run(["g++", "-std=c++11", "-O2", "-fno-elide-constructors", str(source_path), "-o", str(executable)], check=True, timeout=120)
            subprocess.run([str(executable)], check=True, timeout=180)


if __name__ == "__main__":
    unittest.main()
