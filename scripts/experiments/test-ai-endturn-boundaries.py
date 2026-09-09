"""Actual AI turn-close bodies against an independent bounded turn oracle.

Checks all 60 manifest forms at -O0/-O2: reserve clamps, strategy/purchase
order, retained mutable flag-array ownership, Marketplace lookup, AI-before-
human gifts, and ordered negative-resource warning construction. Five wrong
controls must fail. Native long values are bounded and non-overflowing; this
tests behavior, not VC6 layout, EH, or inlining. --source checks an adopted or
reproduced candidate directly, independently of the old manifest anchors.
"""

import argparse
import itertools
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


ROOT = Path(__file__).resolve().parents[2]
SOURCE = None


def bodies():
    module = generator("generate-ai-endturn-boundary-family.py")
    source = (SOURCE or ROOT / "src/ai_player.cpp").read_text()
    original = module.function(source, "void type_AI_player::endTurn()")
    if SOURCE or "    std::string warning;\n" not in original:
        helper = module.function(source, "void type_AI_player::purchaseBuildings()") if (
            "    purchaseBuildings();\n" in original) else ""
        return [("actual-source", original + "\n" + helper)]
    start = original.index("    std::string warning;\n")
    end = original.index("    if (warning.length())", start)
    warning = original[start:end]
    forms = []
    for (name, body), helper, positive in itertools.product(
            module.warning_forms(warning), (False, True), (False, True)):
        caller = original.replace(warning, body)
        if positive:
            caller = caller.replace("if (warning.length())", "if (warning.length() > 0)")
        if helper:
            caller = caller.replace(module.PURCHASE, "    purchaseBuildings();\n")
            caller += "\n" + module.HELPER
        forms.append((name + "/helper=" + str(helper) + "/positive=" + str(positive), caller))
    return forms


FIXTURE = r"""
enum { MARKETPLACE_ID=14 };
std::vector<int> events;
std::vector<std::pair<long, std::string> > formats;
bool valid;
int dialogCount;
std::string dialog;
const char* g_resourceNames[7]={"wood","mercury","ore","sulfur","crystal","gems","gold"};
const char* g_aiResourceWarningFormat="resource warning fixture";
struct playerData {
    struct AI { long m_turnProductionResource[7]; } m_ai;
    long m_resources[7]; signed char m_numTowns, m_townIds[4]; bool m_human;
    bool isHuman() const { return m_human; }
};
struct town {
    bool marketplace; int id;
    bool hasBuilding(int building, int included) const {
        valid = valid && building==MARKETPLACE_ID && included==1;
        events.push_back(50+id); return marketplace;
    }
};
struct game {
    playerData m_players[8]; bool m_playerDisabled[8]; town towns[4]; int groups[8];
    void calculateProduction() { events.push_back(1); }
    town* getTown(int id) { if(id<0 || id>=4) {valid=false; id=0;} return &towns[id]; }
    bool onSameTeam(int a,int b) const { return groups[a]==groups[b]; }
};
game storage; game* g_game=&storage;
struct type_garrison_purchaser {
    int team; explicit type_garrison_purchaser(int id):team(id) {}
    void checkTowns() { events.push_back(20+team); }
};
struct type_town_threat_checker {
    int team; explicit type_town_threat_checker(int id):team(id) {}
    void checkTowns() { events.push_back(30+team); }
};
void fillProhibitedArray(playerData* player, unsigned char* flags) {
    int owner=static_cast<int>(player-g_game->m_players);
    events.push_back(40+owner);
    for(int i=0;i<145;++i) flags[i]=static_cast<unsigned char>((i+owner)%2);
}
struct type_AI_player {
    short m_team; long m_reservedFunds[7]; int purchases, purchaseCalls;
    void endTurn(); void purchaseBuildings();
    unsigned char purchaseBuilding(unsigned char* flags) {
        events.push_back(4);
        for(int i=0;i<145;++i) {
            valid = valid && flags[i]==(i+m_team+purchaseCalls)%2;
            flags[i]=static_cast<unsigned char>(1-flags[i]);
        }
        ++purchaseCalls;
        if(purchases) {--purchases; return 1;} return 0;
    }
    bool hireHeroes() {events.push_back(5);return true;}
    void calculateDemand() {events.push_back(6);}
    void makeGift(long player) {events.push_back(100+static_cast<int>(player));}
};
std::string formatString(const char* format,...) {
    va_list args; va_start(args,format);
    long amount=va_arg(args,long); const char* name=va_arg(args,const char*);
    va_end(args); valid=valid && format==g_aiResourceWarningFormat;
    formats.push_back(std::make_pair(amount,std::string(name)));
    std::ostringstream out; out << name << '=' << amount << ';'; return out.str();
}
void normalDialog(const char* message,int a,int b,int c,int d,int e,int f,
                  int h,int i,int j,int k,int l) {
    valid=valid && a==1 && b==-1 && c==-1 && d==-1 && e==0 && f==-1 &&
        h==0 && i==-1 && j==0 && k==-1 && l==0;
    ++dialogCount; dialog=message;
}
"""

ORACLE = r"""
bool check() {
    // 64 distinct player/town/alliance/availability/purchase states crossed
    // with every sign mask for the seven warning resources: 8192 per body.
    for(int mode=0;mode<64;++mode) for(int signs=0;signs<128;++signs) {
        valid=true; events.clear(); formats.clear(); dialogCount=0; dialog.clear();
        type_AI_player ai; ai.m_team=static_cast<short>((mode*3)%8);
        ai.purchases=mode%4; ai.purchaseCalls=0;
        for(int p=0;p<8;++p) {
            playerData& player=storage.m_players[p];
            player.m_numTowns=static_cast<signed char>(mode%5);
            for(int t=0;t<4;++t) player.m_townIds[t]=static_cast<signed char>((t+mode)%4);
            player.m_human=((mode*7+3) & (1<<p))!=0;
            storage.groups[p]=(p+mode/4)%3;
            storage.m_playerDisabled[p]=((mode*13) & (1<<p))!=0;
            for(int r=0;r<7;++r) {
                player.m_ai.m_turnProductionResource[r]=(mode+r)%6;
                long amount=r+1+mode%3;
                player.m_resources[r]=(signs & (1<<r)) ? -amount : (mode%2 ? amount : 0);
            }
        }
        for(int t=0;t<4;++t) {
            storage.towns[t].id=t;
            storage.towns[t].marketplace=((mode*11) & (1<<t))!=0;
        }
        long expectedFunds[7]; playerData before=storage.m_players[ai.m_team];
        for(int r=0;r<7;++r) {
            ai.m_reservedFunds[r]=(mode+2*r)%9;
            long deduction=before.m_ai.m_turnProductionResource[r];
            expectedFunds[r]=ai.m_reservedFunds[r]>deduction ? ai.m_reservedFunds[r]-deduction : 0;
        }
        std::vector<int> expected;
        expected.push_back(1); expected.push_back(20+ai.m_team); expected.push_back(30+ai.m_team);
        expected.push_back(40+ai.m_team);
        for(int n=0;n<=mode%4;++n) expected.push_back(4);
        expected.push_back(5); expected.push_back(6);
        bool market=false;
        for(int t=0;t<before.m_numTowns;++t) {
            int id=before.m_townIds[t]; expected.push_back(50+id);
            if(storage.towns[id].marketplace) {market=true;break;}
        }
        if(market) {
            std::vector<int> nonhumans, humans;
            for(int p=0;p<8;++p) if(p!=ai.m_team && !storage.m_playerDisabled[p] &&
                    storage.groups[p]==storage.groups[ai.m_team]) {
                if(storage.m_players[p].m_human) humans.push_back(100+p);
                else nonhumans.push_back(100+p);
            }
            expected.insert(expected.end(),nonhumans.begin(),nonhumans.end());
            expected.insert(expected.end(),humans.begin(),humans.end());
        }
        std::string expectedWarning; std::vector<std::pair<long,std::string> > expectedFormats;
        for(int r=0;r<7;++r) if(before.m_resources[r]<0) {
            char number[32]; std::snprintf(number,sizeof(number),"%ld",before.m_resources[r]);
            expectedWarning+=std::string(g_resourceNames[r])+"="+number+";";
            expectedFormats.push_back(std::make_pair(before.m_resources[r],std::string(g_resourceNames[r])));
        }
        ai.endTurn();
        if(!valid || events!=expected || formats!=expectedFormats ||
           dialogCount!=(expectedWarning.empty()?0:1) || dialog!=expectedWarning ||
           ai.purchases!=0 || ai.purchaseCalls!=mode%4+1) return false;
        for(int r=0;r<7;++r) if(ai.m_reservedFunds[r]!=expectedFunds[r] ||
            storage.m_players[ai.m_team].m_resources[r]!=before.m_resources[r] ||
            storage.m_players[ai.m_team].m_ai.m_turnProductionResource[r]!=before.m_ai.m_turnProductionResource[r])
            return false;
    }
    return true;
}
}
"""


class EndTurnTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_actual_bodies_and_negative_controls(self):
        forms = bodies()
        self.assertIn(len(forms), (1, 60))
        if SOURCE:
            self.assertEqual(len(forms), 1)
        positive = len(forms)
        original = forms[0][1]
        mutants = [
            ("missing-clamp", "m_reservedFunds[resource] = 0;", "m_reservedFunds[resource] = -1;"),
            ("missing-production", "    g_game->calculateProduction();\n", ""),
            ("wrong-marketplace", "hasBuilding(MARKETPLACE_ID, 1)", "hasBuilding(MARKETPLACE_ID, 0)"),
            ("wrong-gift-side", "!g_game->m_players[playerId].isHuman()", "g_game->m_players[playerId].isHuman()"),
            ("missing-warning", "    if (msg.length()" if "msg.length()" in original else "    if (warning.length()",
             "    if (0 && msg.length()" if "msg.length()" in original else "    if (0 && warning.length()"),
        ]
        for name, old, new in mutants:
            self.assertEqual(original.count(old), 1, name)
            forms.append((name, original.replace(old, new)))
        text = "#include <vector>\n#include <string>\n#include <sstream>\n#include <cstdio>\n#include <cstdarg>\n#define DC_ONLY(a,b)\n"
        for index, (_, body) in enumerate(forms):
            text += f"namespace Case{index} {{\n" + FIXTURE + body + "\n" + ORACLE
        text += "int main() {\n"
        for index in range(len(forms)):
            result = "!" if index < positive else ""
            text += f'if ({result}Case{index}::check()) {{ std::printf("failed form {index}\\n"); return 1; }}\n'
        text += "return 0; }\n"
        with tempfile.TemporaryDirectory(prefix="homm3-ai-endturn-oracle-") as folder:
            source, binary = Path(folder) / "oracle.cpp", Path(folder) / "oracle"
            source.write_text(text)
            for optimization in ("-O0", "-O2"):
                subprocess.run(["g++", "-std=c++11", optimization, "-Wno-unknown-pragmas",
                                str(source), "-o", str(binary)], check=True)
                subprocess.run([str(binary)], check=True)
        print(f"{positive} actual-body form(s), 8192 cases each, 5 wrong controls rejected at -O0/-O2")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path)
    args, unittest_args = parser.parse_known_args()
    SOURCE = args.source
    unittest.main(argv=[__file__] + unittest_args)
