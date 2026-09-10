"""Exercise actual Grail-helper source against independent eligibility/call oracles.

The reduced native fixture covers the fields used by this helper, not VC6
layout or inlining. Values are bounded; unobserved HeroDestination fields
are deliberately absent. Checks preserve existing vector entries, map/search
coordinates, ordered helper calls, artifact fields, friendly-distance ties,
hero blocking, patrol limits, victory value and the movement-cost floor.
Five deliberately wrong variants must fail. The default checks adopted source;
--source selects another snapshot. --all-forms additionally exercises the
pre-adoption lifetime family and requires its original anchors.
"""

import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


ROOT = Path(__file__).resolve().parents[2]
SOURCE = None
ALL_FORMS = False

FIXTURE = r"""
#include <cstdio>
#include <vector>
enum { HERO=34, VICTORY_CONDITION_BUILD_GRAIL=4, ARTIFACT_HOLY_GRAIL=2 };
struct type_point { int m_x,m_y,m_z; };
struct HeroDestination {
    type_point m_point; long m_value,m_moveCost; unsigned char m_isCritical;
};
struct type_artifact {
    int m_artifactId,m_extra;
    explicit type_artifact(int id):m_artifactId(id),m_extra(-1) {}
    type_artifact(int id,int extra):m_artifactId(id),m_extra(extra) {}
};
int g_mapWidth,g_mapHeight,artifactValue,expectedOwner,mobility;
type_point expectedPoint;
bool valid,patrol;
std::vector<int> calls;
bool samePoint(type_point a,type_point b) {
    return a.m_x==b.m_x && a.m_y==b.m_y && a.m_z==b.m_z;
}
struct pathCell { bool m_visited; unsigned short m_cost; } path;
struct searchArray {
    pathCell* getCell(type_point point,int flag) const {
        valid=valid && samePoint(point,expectedPoint) && flag==0;
        calls.push_back(1); return &path;
    }
};
struct NewmapCell { int m_type; bool m_isTrigger; unsigned long m_extraInfo; } cell;
struct NewfullMap {
    NewmapCell* cell(int x,int y,int z) {
        type_point point={x,y,z}; valid=valid && samePoint(point,expectedPoint);
        calls.push_back(2); return &::cell;
    }
};
struct playerData { type_point m_puzzleGuess; };
struct game {
    playerData m_players[3]; NewfullMap m_worldMap;
    struct { struct { int m_type; } m_victoryCondition; } m_mapHeader;
    NewmapCell* getCell(type_point point) {
        return m_worldMap.cell(point.m_x,point.m_y,point.m_z);
    }
} storage;
game* g_game=&storage;
struct hero {
    int m_owner,m_id,m_movePoints;
    bool isInPatrolRadius(type_point point) {
        valid=valid && samePoint(point,expectedPoint); calls.push_back(3); return patrol;
    }
    int getMobility() { calls.push_back(5); return mobility; }
};
long aiGetValueOfArtifact(const type_artifact& artifact,long player) {
    valid=valid && artifact.m_artifactId==ARTIFACT_HOLY_GRAIL &&
        artifact.m_extra==-1 && player==expectedOwner;
    calls.push_back(4); return artifactValue;
}
int max(int a,int b) { return a>b?a:b; }
int min(int a,int b) { return a<b?a:b; }
typedef void (*GrailHelper)(const hero*,const searchArray*,
                           std::vector<HeroDestination>&,const unsigned short*);
"""

ORACLE = r"""
bool check(GrailHelper helper) {
    const int dimensions[][2]={{2,3},{3,2},{1,4},{4,1}};
    const int values[]={-1,0,7,2000};
    int scenarios=0;
    for(int d=0;d<4;++d) {
        g_mapWidth=dimensions[d][0]; g_mapHeight=dimensions[d][1];
        int volume=g_mapWidth*g_mapHeight*2;
        for(int position=0;position<volume;++position) for(int mode=0;mode<512;++mode) {
            ++scenarios; valid=true; calls.clear();
            hero current; current.m_owner=mode%3; current.m_id=17;
            current.m_movePoints=mode%31-7; expectedOwner=current.m_owner;
            mobility=mode%17; artifactValue=values[(mode/128+position)%4];
            expectedPoint.m_x=position%g_mapWidth;
            expectedPoint.m_y=(position/g_mapWidth)%g_mapHeight;
            expectedPoint.m_z=position/(g_mapWidth*g_mapHeight);
            storage.m_players[current.m_owner].m_puzzleGuess=expectedPoint;
            bool guessed=(mode&1)!=0;
            if(!guessed) storage.m_players[current.m_owner].m_puzzleGuess.m_x=-1;
            path.m_visited=(mode&2)!=0; path.m_cost=13+mode%37;
            cell.m_type=(mode&4)?HERO:1; cell.m_isTrigger=(mode&8)!=0;
            cell.m_extraInfo=(mode&16)?current.m_id:current.m_id+1;
            patrol=(mode&32)!=0;
            bool victory=(mode&64)!=0;
            storage.m_mapHeader.m_victoryCondition.m_type=victory?VICTORY_CONDITION_BUILD_GRAIL:0;
            std::vector<unsigned short> friendly(volume,60000);
            friendly[position]=path.m_cost+(mode/128)%3-1;
            std::vector<int> expected;
            bool eligible=false;
            if(guessed) {
                expected.push_back(1);
                if(path.m_visited) {
                    expected.push_back(2);
                    bool blocked=cell.m_type==HERO && cell.m_isTrigger &&
                        cell.m_extraInfo!=static_cast<unsigned long>(current.m_id);
                    if(!blocked) {
                        expected.push_back(3);
                        if(patrol && friendly[position]>=path.m_cost) {
                            if(!victory) expected.push_back(4);
                            expected.push_back(5);
                            eligible=(victory || artifactValue>0);
                        }
                    }
                }
            }
            HeroDestination prefix; prefix.m_point.m_x=-3; prefix.m_point.m_y=-4;
            prefix.m_point.m_z=-5; prefix.m_value=123; prefix.m_moveCost=456;
            prefix.m_isCritical=1;
            std::vector<HeroDestination> destinations(1,prefix);
            searchArray search;
            helper(&current,&search,destinations,&friendly[0]);
            if(!valid || calls!=expected || destinations.size()!=(eligible?2u:1u)) return false;
            const HeroDestination& first=destinations[0];
            if(!samePoint(first.m_point,prefix.m_point) || first.m_value!=123 ||
               first.m_moveCost!=456 || first.m_isCritical!=1) return false;
            if(eligible) {
                const HeroDestination& result=destinations[1];
                int floor=current.m_movePoints+mobility;
                int expectedCost=path.m_cost>floor?path.m_cost:floor;
                if(!samePoint(result.m_point,expectedPoint) || result.m_isCritical!=0 ||
                   result.m_value!=(victory?1968:artifactValue) ||
                   result.m_moveCost!=expectedCost) return false;
            }
        }
    }
    return scenarios==20480;
}
"""


class GrailBoundaries(unittest.TestCase):
    def test_map_extra_point_forwarding(self):
        module = generator("generate-ai-grail-map-extra-family.py")
        helper_module = generator("generate-ai-grail-lifetime-family.py")
        header = (ROOT / "include/advmgr.h").read_text()
        signature = "inline int getMapExtra(type_point point)"
        helper = helper_module.function(header if signature in header else module.SHARED,
                                        signature)
        fixture = r"""
#include <cstdio>
struct type_point { short m_x:10; short m_y:10; short m_z:4; };
int arguments[3],count;
unsigned short answer;
unsigned short getMapExtra(int x,int y,int z) {
    arguments[0]=x; arguments[1]=y; arguments[2]=z; ++count; return answer;
}
""" + helper + r"""
int main() {
    const int xy[]={-512,-511,-1,0,1,510,511};
    const int z[]={-8,-7,-1,0,1,6,7};
    const unsigned short values[]={0,1,255,256,32767,32768,65535};
    for(int i=0;i<7;++i) for(int j=0;j<7;++j)
        for(int k=0;k<7;++k) for(int v=0;v<7;++v) {
            type_point point; point.m_x=xy[i]; point.m_y=xy[j]; point.m_z=z[k];
            count=0; answer=values[v];
            int result=getMapExtra(point);
            if(result!=values[v] || count!=1 || arguments[0]!=xy[i] ||
               arguments[1]!=xy[j] || arguments[2]!=z[k] ||
               point.m_x!=xy[i] || point.m_y!=xy[j] || point.m_z!=z[k]) return 1;
        }
    return 0;
}
"""
        compiler = shutil.which("g++") or shutil.which("clang++")
        self.assertIsNotNone(compiler, "native C++ compiler required")
        with tempfile.TemporaryDirectory(prefix="homm3-map-extra-oracle-") as temporary:
            directory = Path(temporary)
            source = directory / "fixture.cpp"
            source.write_text(fixture)
            for optimization in ("-O0", "-O2"):
                program = directory / ("oracle" + optimization[1:])
                subprocess.run([compiler, "-std=c++11", optimization,
                                str(source), "-o", str(program)], check=True)
                subprocess.run([str(program)], check=True)
        print("PASS: map-extra forwarding, signed coordinates and unsigned result: "
              "2,401 cases at -O0/-O2")

    def test_actual_bodies_and_negative_controls(self):
        module = generator("generate-ai-grail-lifetime-family.py")
        source = (SOURCE or ROOT / "src/ai_player.cpp").read_text()
        original = module.function(source, "static void checkHolyGrail(")
        forms = list(module.helper_forms(original)) if ALL_FORMS else [("actual-source", original)]
        # Negatives use the unchanged helper, with each single semantic error
        # isolated. No reference answer is copied from a candidate body.
        mutations = [
            ("friendly-tie", "point.m_moveCost <= friendlyCost", "point.m_moveCost < friendlyCost"),
            ("hero-owner", "== static_cast<unsigned long>(currentHero->m_id)", "!= static_cast<unsigned long>(currentHero->m_id)"),
            ("victory-value", "point.m_value = 1968;", "point.m_value = 1967;"),
            ("movement-floor", "point.m_moveCost = max(", "point.m_moveCost = min("),
            ("positive-value", "point.m_value > 0", "point.m_value >= 0"),
        ]
        negatives = []
        for name, before, after in mutations:
            self.assertEqual(original.count(before), 1, name)
            negatives.append((name, original.replace(before, after)))
        rendered = [FIXTURE, ORACLE]
        for index, (_, body) in enumerate(forms + negatives):
            rendered.append(f"namespace variant{index} {{\n{body}\n}}\n")
        rendered.append("int main() {\n")
        for index, (name, _) in enumerate(forms + negatives):
            expected = "true" if index < len(forms) else "false"
            rendered.append(f'if(check(variant{index}::checkHolyGrail)!={expected}) {{ '
                            f'std::puts("FAIL: {name}"); return 1; }}\n')
        rendered.append("return 0;\n}\n")
        compiler = shutil.which("g++") or shutil.which("clang++")
        self.assertIsNotNone(compiler, "native C++ compiler required")
        with tempfile.TemporaryDirectory(prefix="homm3-grail-oracle-") as temporary:
            directory = Path(temporary)
            fixture = directory / "fixture.cpp"
            fixture.write_text("\n".join(rendered))
            for optimization in ("-O0", "-O2"):
                program = directory / ("oracle" + optimization[1:])
                subprocess.run([compiler, "-std=c++11", "-Wno-unknown-pragmas", optimization,
                                str(fixture), "-o", str(program)], check=True)
                subprocess.run([str(program)], check=True)
        print(f"PASS: {len(forms)} helper forms x 20,480 scenarios at -O0/-O2; "
              "five independent negative controls rejected")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--all-forms", action="store_true")
    args, remaining = parser.parse_known_args()
    SOURCE = args.source
    ALL_FORMS = args.all_forms
    unittest.main(argv=[__file__, *remaining])
