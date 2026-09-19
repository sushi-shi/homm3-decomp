#!/usr/bin/env python3
"""Independent native radial-site/event oracle for the Zone family.

Compiles actual generated bodies. Imports point/slot fields, constants and
direction tables, but models opaque Zone/Voronoi operations deterministically.
All fixture scalars are initialized; this supplies no missing game initializer.
Checks successful-allocation behavior, not VC6 rounding, EH, inlining or the
order of two adjacent vector allocations. Registration is observed at addSite;
retail call review separately verifies the two push_back calls' order.
"""
import argparse
import json
import re
import subprocess
import tempfile
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.test_rmg_families import generator


HARNESS = r'''
#include <algorithm>
#include <vector>
#include <deque>
#include <cstring>
#include <cstdio>
#include <cstdlib>
using std::memset;
struct Event {
    int kind,a,b,c,d,e,f;
    Event(int k,int aa=0,int bb=0,int cc=0,int dd=0,int ee=0,int ff=0)
        :kind(k),a(aa),b(bb),c(cc),d(dd),e(ee),f(ff) {}
    bool operator==(const Event& o) const {
        return kind==o.kind&&a==o.a&&b==o.b&&c==o.c&&d==o.d&&e==o.e&&f==o.f;
    }
};
enum { DIAGRAM=1,SLOT,ZONE,SITE,QUERY,ZONE_END,SLOT_END,BUILD,LOCATE,TRACE,FILL,JOIN,DIAGRAM_END };
std::vector<Event> events;
bool recording=false;
int slotSerial=0,zoneSerial=0,acceptanceMode=0;
void emit(Event e) { if(recording) events.push_back(e); }
bool acceptPoint(int x,int y,int z) {
    return acceptanceMode==0 || (acceptanceMode==2 && std::abs(3*x+5*y+7*z)%3!=0);
}
@TYPES@
@TABLES@
struct TRmgZone {
    TRmgTownSlot* m_slot;
    int m_boundaryRoughness,m_terrain,id;
    TRmgMapPosition position;
    TRmgZone(TRmgTownSlot* s):m_slot(s),m_boundaryRoughness(s->m_size),m_terrain(0),id(++zoneSerial) {
        position.m_x=position.m_y=position.m_z=0;
        emit(Event(ZONE,id,s->id,s->m_zoneIndex,s->m_kind,s->m_size));
    }
    ~TRmgZone() { emit(Event(ZONE_END,id)); }
    TRmgMapPosition getLevelPosition() const { return position; }
    void setLevelPosition(TRmgMapPosition p) { position=p; }
};
struct TRmgTemplate { std::vector<TRmgTownSlot*> m_zones; };
struct TRmgBoundaryVertex { int x,y; TRmgBoundaryVertex(int a,int b):x(a),y(b) {} };
struct TRmgVoronoi {
    std::deque<TRmgBoundaryVertex> vertices;
    TRmgVoronoi() { emit(Event(DIAGRAM)); }
    ~TRmgVoronoi() { emit(Event(DIAGRAM_END)); }
    void addSite(TPoint p,TRmgZone* zone);
    void buildVertices() { emit(Event(BUILD)); }
    TRmgBoundaryVertex* locate(TPoint p) {
        emit(Event(LOCATE,p.m_x,p.m_y));
        vertices.push_back(TRmgBoundaryVertex(p.m_x,p.m_y));
        return &vertices.back();
    }
};
struct type_random_map_generator {
    struct Map { int m_mapWidth,m_mapHeight; } m_map;
    std::vector<TRmgZone*> m_zones;
    int m_waterContent;
    @DECLARATIONS@
    bool canPlaceZone(TRmgZone* zone) {
        TRmgMapPosition p=zone->getLevelPosition();
        bool answer=acceptPoint(p.m_x,p.m_y,p.m_z);
        emit(Event(QUERY,p.m_x,p.m_y,p.m_z,answer));return answer;
    }
    void traceZoneBoundary(TRmgBoundaryVertex* p,bool original) { emit(Event(TRACE,p->x,p->y,original)); }
    void fillZoneArea(TRmgZone* z,TRmgBoundaryVertex* p) { emit(Event(FILL,z->id,p->x,p->y)); }
    void joinExtraZones(int count,TRmgVoronoi*) { emit(Event(JOIN,count)); }
};
type_random_map_generator* activeOwner;
TRmgTemplate* activeTemplate;
void TRmgVoronoi::addSite(TPoint p,TRmgZone* zone) {
    emit(Event(SITE,p.m_x,p.m_y,zone?zone->id:0,
        activeTemplate->m_zones.size(),activeOwner->m_zones.size(),zone?zone->m_terrain:0));
}
@BODIES@
typedef void(type_random_map_generator::*Fn)(TRmgTemplate*,int);
struct ZoneData {
    int x,y,z,radius,id,terrain;
    ZoneData(int xx,int yy,int zz,int r,int i,int t):x(xx),y(yy),z(zz),radius(r),id(i),terrain(t) {}
};
// Reference uses a filtered radial-site list, not the authored nested guards.
// The eight signed direction pairs are independent of the imported 32-entry
// table; decimal diagonal constants intentionally match the recovered domain.
std::vector<Event> expected(const std::vector<ZoneData>& initial,
                          int width,int height,int level,int water) {
    std::vector<Event> out;
    std::vector<ZoneData> all=initial;
    int original=initial.size(),slots=original,zones=original;
    int nextSlot=original,nextZone=original;
    out.push_back(Event(DIAGRAM));
    for(unsigned i=0;i<initial.size();++i)
        if(initial[i].z==level)
            out.push_back(Event(SITE,initial[i].x,initial[i].y,initial[i].id,slots,zones,initial[i].terrain));
    if(level==1 || water!=RMG_WATER_NONE) {
        int tempSlot=++nextSlot,tempZone=++nextZone;
        out.push_back(Event(SLOT,tempSlot));
        out.push_back(Event(ZONE,tempZone,tempSlot,-1,RMG_TEMPLATE_JUNCTION,0));
        const double directions[8][2]={{1,0},{.7071,.7071},{0,1},{-.7071,.7071},
            {-1,0},{-.7071,-.7071},{0,-1},{.7071,-.7071}};
        for(unsigned i=0;i<initial.size();++i) {
            const ZoneData& z=initial[i];
            if(z.z!=level) continue;
            for(int direction=0;direction<8;++direction) {
                double dx=z.radius*directions[direction][0];
                double dy=z.radius*directions[direction][1];
                int x=static_cast<int>(z.x+2*dx),y=static_cast<int>(z.y+2*dy);
                bool xAllowed=(x>=0 || x>=dx) && (x<width || x<width+dx);
                bool yAllowed=(y>=0 || y>=dy) && (y<height || y<height+dy);
                if(!xAllowed || !yAllowed) continue;
                bool accepted=acceptPoint(x,y,z.z);
                out.push_back(Event(QUERY,x,y,z.z,accepted));
                if(!accepted) continue;
                int owner=0,terrain=0;
                if(z.z==0) {
                    int slot=++nextSlot;owner=++nextZone;terrain=eTerrainWater;
                    out.push_back(Event(SLOT,slot));
                    out.push_back(Event(ZONE,owner,slot,slots,RMG_TEMPLATE_JUNCTION,z.radius));
                    ++slots;++zones;
                    all.push_back(ZoneData(x,y,z.z,z.radius,owner,terrain));
                }
                out.push_back(Event(SITE,x,y,owner,slots,zones,terrain));
            }
        }
        out.push_back(Event(ZONE_END,tempZone));
        out.push_back(Event(SLOT_END,tempSlot));
    }
    out.push_back(Event(BUILD));
    for(unsigned i=0;i<all.size();++i) if(all[i].z==level) {
        out.push_back(Event(LOCATE,all[i].x,all[i].y));
        bool originalSite=i<initial.size();
        bool trace=originalSite && !(water==RMG_WATER_ISLANDS && level!=1);
        out.push_back(Event(TRACE,all[i].x,all[i].y,trace));
    }
    for(unsigned i=0;i<all.size();++i) if(all[i].z==level) {
        out.push_back(Event(LOCATE,all[i].x,all[i].y));
        out.push_back(Event(FILL,all[i].id,all[i].x,all[i].y));
    }
    out.push_back(Event(JOIN,original));out.push_back(Event(DIAGRAM_END));
    return out;
}
bool one(Fn fn,int width,int height,int level,int water,int count,int radius,int mode) {
    recording=false;slotSerial=zoneSerial=0;acceptanceMode=mode;
    type_random_map_generator owner;TRmgTemplate mapTemplate;
    owner.m_map.m_mapWidth=width;owner.m_map.m_mapHeight=height;owner.m_waterContent=water;
    std::vector<ZoneData> initial;
    for(int i=0;i<count;++i) {
        TRmgTownSlot* s=new TRmgTownSlot;s->m_size=radius+(i%2);s->m_zoneIndex=i;
        TRmgZone* z=new TRmgZone(s);z->m_terrain=eTerrainGrass+i;
        TRmgMapPosition p;p.m_x=i==0?width/2:i==1?width-1:0;
        p.m_y=i==0?height/2:i==1?0:height-1;p.m_z=i%2;z->setLevelPosition(p);
        owner.m_zones.push_back(z);mapTemplate.m_zones.push_back(s);
        initial.push_back(ZoneData(p.m_x,p.m_y,p.m_z,s->m_size,z->id,z->m_terrain));
    }
    activeOwner=&owner;activeTemplate=&mapTemplate;
    std::vector<Event> reference=expected(initial,width,height,level,water);
    events.clear();recording=true;(owner.*fn)(&mapTemplate,level);recording=false;
    bool ok=events==reference;
    // Also reclaim allocations intentionally omitted from registration by a
    // negative control; fixture ownership must not depend on the tested result.
    std::vector<TRmgTownSlot*> allocatedSlots=mapTemplate.m_zones;
    for(unsigned i=0;i<owner.m_zones.size();++i)
        if(std::find(allocatedSlots.begin(),allocatedSlots.end(),owner.m_zones[i]->m_slot)==allocatedSlots.end())
            allocatedSlots.push_back(owner.m_zones[i]->m_slot);
    for(unsigned i=0;i<owner.m_zones.size();++i) delete owner.m_zones[i];
    for(unsigned i=0;i<allocatedSlots.size();++i) delete allocatedSlots[i];
    return ok;
}
bool check(Fn fn) {
    const int widths[]={1,3,11},heights[]={1,4,9},counts[]={0,1,3},radii[]={0,1,3};
    for(int w=0;w<3;++w) for(int h=0;h<3;++h) for(int level=0;level<2;++level)
    for(int water=0;water<3;++water) for(int n=0;n<3;++n) for(int r=0;r<3;++r)
    for(int mode=0;mode<3;++mode)
        if(!one(fn,widths[w],heights[h],level,water,counts[n],radii[r],mode)) return false;
    return true;
}
int main() {
    @CHECKS@
    std::puts("@RESULT@");return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path)
    args = parser.parse_args()
    module = generator("generate-rmg-zone-boundary-family.py")
    source = (HOMM3_DIR / module.SOURCE).read_text()
    original = module.definition(source)
    forms = [dict(name="authored", replace=original)]
    if args.manifest:
        axis, = json.loads(args.manifest.read_text())["axes"]
        assert axis["source"] == module.SOURCE and axis["find"] == original
        forms = axis["options"]
    header = (HOMM3_DIR / "include/rmg.h").read_text()

    def block(name):
        start = header.index(name + " {")
        return header[start:header.index("\n};", start) + 3]

    slot = block("struct TRmgTownSlot")
    initialized = []
    for line in slot.splitlines():
        field = re.match(r"\s*(?:int|unsigned char|TRmgTreasureRange)\s+(m_\w+)(\[[0-9]+\])?;", line)
        if field:
            name, array = field.groups()
            initialized.append(f"memset({name},0,sizeof({name}));" if array else f"{name}=0;")
    assert len(initialized) == 19, initialized
    fixture = ("\n    int id;\n    TRmgTownSlot():id(++slotSerial) { "
               + " ".join(initialized) + " emit(Event(SLOT,id)); }\n"
               "    ~TRmgTownSlot() { emit(Event(SLOT_END,id)); }\n")
    slot = slot[:-3] + fixture + "};"
    types = "struct TPoint; struct TRmgTownSlot;\n"
    types += "\n".join(block(n) for n in ("struct TRmgVector", "struct TPoint",
        "struct TRmgMapPosition", "struct TRmgZoneConnection", "struct TRmgTreasureRange",
        "enum ERmgTemplateZoneKind", "enum ERmgConnectionConstants"))
    types += "\n" + slot + "\n" + (HOMM3_DIR / "include/terrain_type.h").read_text()
    tables = ""
    for name in ("g_rmgDirectionCosines", "g_rmgDirectionSines"):
        begin = source.index("double " + name + "[32] = {")
        tables += source[begin:source.index("\n};", begin) + 3] + "\n"
    controls = [
        ("wrong_level", "== level", "!= level"),
        ("wrong_direction", "direction += 4", "direction += 8"),
        ("wrong_edge_equality", "position.m_x < dx", "position.m_x <= dx"),
        ("wrong_slot_index", "slot->m_zoneIndex = mapTemplate->m_zones.size();",
            "slot->m_zoneIndex = mapTemplate->m_zones.size() + 1;"),
        ("missing_slot_registration", "mapTemplate->m_zones.push_back(slot);", ""),
        ("null_surface_owner", "diagram.addSite(TPoint(position.m_x, position.m_y), addedZone);",
            "diagram.addSite(TPoint(position.m_x, position.m_y), 0);"),
        ("nonnull_underground_owner", "diagram.addSite(TPoint(position.m_x, position.m_y), addedZone);",
            "diagram.addSite(TPoint(position.m_x, position.m_y), addedZone ? addedZone : &testZone);"),
        ("growing_join_extent", "joinExtraZones(originalZones, &diagram);",
            "joinExtraZones(m_zones.size(), &diagram);"),
        ("early_vertex_build", "    }\n    diagram.buildVertices();",
            "        diagram.buildVertices();\n    }"),
        ("wrong_trace_classification", "zone < originalZones &&", "zone >= originalZones &&"),
    ]
    bodies = [row["replace"] for row in forms]
    for name, before, after in controls:
        assert before in original, name
        bodies.append(original.replace(before, after))
    declarations, definitions, checks = [], [], []
    for i, body in enumerate(bodies):
        name = "boundary" + str(i)
        body = body.replace("::buildZoneBoundaries(", "::" + name + "(")
        # Only adapt VC6's for-initializer scope to the host language.
        body = body.replace("    for (int zone = 0;", "    int zone;\n    for (zone = 0;", 1)
        declarations.append(f"void {name}(TRmgTemplate*,int);")
        definitions.append(body)
        wanted = "!" if i < len(forms) else ""
        checks.append(f'if ({wanted}check(&type_random_map_generator::{name})) '
                      f'{{ std::fprintf(stderr,"case {i} failed\\n");return 1; }}')
    text = HARNESS
    for key, value in {"TYPES": types, "TABLES": tables, "DECLARATIONS": "\n".join(declarations),
            "BODIES": "\n".join(definitions), "CHECKS": "\n".join(checks),
            "RESULT": f"{len(forms)} generated bodies x 1458 scenarios passed; {len(controls)} negative controls rejected"}.items():
        text = text.replace("@" + key + "@", value)
    with tempfile.TemporaryDirectory(prefix="homm3-zone-boundary-") as directory:
        cpp = Path(directory) / "oracle.cpp"
        binary = Path(directory) / "oracle"
        cpp.write_text(text)
        subprocess.run(["g++", "-std=c++98", "-O1", "-fno-strict-aliasing", str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
