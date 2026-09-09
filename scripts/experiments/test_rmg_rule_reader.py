"""Independent parsed-row and last-matching-rule oracle for the reader family."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6.test_rmg_families import generator


class RuleReaderTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "native oracle needs g++")
    def test_parsed_scores_and_reverse_binding(self):
        root = Path(__file__).resolve().parents[2]
        module = generator("generate-rmg-rule-reader-family.py")
        source = (root / module.SOURCE).read_text()
        header = (root / "include/rmg.h").read_text()
        objects = (root / "include/advmgr_objects.h").read_text()
        object_names = (root / "include/objnames.h").read_text()
        def block(prefix):
            start = header.index(prefix + " {")
            return header[start:header.index("\n};", start) + 3]
        def field(text, name):
            return next(line.split("//")[0].strip() for line in text.splitlines() if name + ";" in line)
        types = block("struct TRmgObjectPlacementRule") + "\n" + block("enum ERmgObjectPlacementScore")
        enum_start = object_names.index("enum {")
        types += "\n" + object_names[enum_start:object_names.index("\n};", enum_start) + 3]
        types += "\nstruct TObjectType { " + field(objects, "m_subtype") + " " + field(objects, "m_recommendedTerrainMask") + " };\n"
        types += "struct TRmgObjectPropertiesRef { " + " ".join(field(header, name) for name in
            ("m_prototype", "m_preferredTerrain", "m_placementRule")) + " };\n"
        forms = [option["replace"] for option in module.variants(source)]
        self.assertEqual(len(forms), 60)
        self.assertEqual(len(set(forms)), 60)
        direct = [option["replace"] for option in module.variants(source, True)]
        self.assertEqual(len(direct), 13)
        self.assertEqual(direct[0], forms[0])
        forms += direct[1:]
        self.assertEqual(len(set(forms)), 72)
        constructors = [option["replace"] for option in module.constructor_frontier(source)]
        self.assertEqual(len(constructors), 54)
        forms = list(dict.fromkeys(forms + constructors))
        self.assertEqual(len(forms), 124)
        empty_initializers = [option["replace"] for option in module.empty_initialization_frontier(source)]
        self.assertEqual(len(empty_initializers), 16)
        forms = list(dict.fromkeys(forms + empty_initializers))
        self.assertEqual(len(forms), 139)
        row_forms = [option["replace"] for option in module.row_frontier(source)]
        self.assertEqual(len(row_forms), 60)
        forms = list(dict.fromkeys(forms + row_forms))
        self.assertEqual(len(forms), 198)
        count = len(forms)
        seed = forms[0]
        forms += [seed.replace("values[0][0] == ' '", "values[0][0] == '!'"),
                  seed.replace("rule.m_index = row - 3", "rule.m_index = row - 2"),
                  seed.replace("RMG_PLACEMENT_INVALID;", "0;"),
                  seed.replace("values[index + 16]", "values[index + 17]"),
                  seed.replace("values[index + ruleCount + 16]", "values[index + ruleCount + 15]"),
                  seed.replace("g_adventureObjectLandBlocked[objectType][8]", "g_adventureObjectLandBlocked[objectType][4]"),
                  seed.replace("subtypesByType[mappedType][terrain][match] != subtype", "subtypesByType[mappedType][terrain][match] == subtype"),
                  seed.replace("properties->m_placementRule = rulesByType[mappedType][terrain][match]", "properties->m_placementRule = rulesByType[mappedType][terrain][0]"),
                  seed.replace("sheet->dispose();", "")]
        self.assertTrue(all(body != seed for body in forms[count:]))
        text = "#include <vector>\n#include <bitset>\n#include <algorithm>\n#include <cstring>\n#include <cstdlib>\n#include <cstdio>\n"
        text += (root / "include/terrain_type.h").read_text() + "\n#define DATA_COMPGEN(va,name,text) text\n"
        for index, body in enumerate(forms):
            # Only accommodate VC6's old for-initializer scope in the host.
            body = body.replace("        for (int index = 0; index < ruleCount; ++index)",
                                "        int index;\n        for (index = 0; index < ruleCount; ++index)")
            text += f"namespace Case{index} {{\n" + types + r"""
static unsigned char g_traits[232][16];
static const unsigned char (*g_adventureObjectLandBlocked)[16] = g_traits;
static int g_disposals;
static bool g_badFilename;
struct TSpreadsheetResource {
    typedef std::vector<char*> TStringVector;
    typedef std::vector<TStringVector*> TArray;
    TArray m_spreadsheet;
    int getNumberOfRows() const { return m_spreadsheet.size(); }
    const TStringVector& getRow(int row) const { return *m_spreadsheet[row]; }
    void dispose() { ++g_disposals; }
};
static TSpreadsheetResource g_sheet;
struct ResourceManager {
    static TSpreadsheetResource* getSpreadsheet(const char* name) {
        if (std::strcmp(name,"rand_trn.txt")) g_badFilename=true;
        return &g_sheet;
    }
};
struct TRmgGeneratorBase {
    std::vector<TRmgObjectPlacementRule> m_placementRules;
    std::vector<TRmgObjectPropertiesRef*> m_objectPrototypes[232];
    void readObjectPlacementRules();
};
""" + body + r"""
int value(int row,int column) { return (row+1)*100-column*13; }
int ruleType(int row) { return row%2 ? 17 : 0; }
int ruleSubtype(int row) { return row%3-1; }
int ruleTerrain(int row,int mask) { return (row%2+mask)%10; }
bool check() {
    const int groups[4]={0,17,54,231};
    for(int n=0;n<=7;++n) for(int sentinel=0;sentinel<3;++sentinel)
    for(int mapping=0;mapping<3;++mapping) for(int mask=0;mask<6;++mask) {
        TRmgGeneratorBase owner;
        TSpreadsheetResource::TStringVector rows[12];
        char cells[12][40][32];
        int rowCount=3+n+(sentinel==2?0:2);
        g_sheet.m_spreadsheet.clear();g_disposals=0;g_badFilename=false;
        for(int r=0;r<rowCount;++r) {
            for(int c=0;c<40;++c) {
                std::sprintf(cells[r][c],"%d",value(r-3,c));
                rows[r].push_back(cells[r][c]);
            }
            if(r>=3) {
                std::strcpy(cells[r][0],"rule");
                std::sprintf(cells[r][3],"%d",ruleType(r-3));
                std::sprintf(cells[r][4],"%d",ruleSubtype(r-3));
                std::sprintf(cells[r][6],"%d",ruleTerrain(r-3,mask));
            }
            g_sheet.m_spreadsheet.push_back(&rows[r]);
        }
        if(sentinel!=2) std::strcpy(cells[3+n][0],sentinel?" end":"");
        int remap[232];
        for(int type=0;type<232;++type) {
            remap[type]=mapping==2?0:mapping==1?(type==0?17:type==17?0:type):type;
            int wrong=231;
            std::memcpy(&g_traits[type][4],&wrong,sizeof(int));
            std::memcpy(&g_traits[type][8],&remap[type],sizeof(int));
        }
        TObjectType prototypes[16];TRmgObjectPropertiesRef properties[16];
        unsigned long masks[16];
        for(int item=0;item<16;++item) {
            int bit=(mask+item%2)%10;
            masks[item]=item%4==0?0:item%4==1?512:(1ul<<bit)|(item%4==2?256:0);
            prototypes[item].m_subtype=item%3-1;
            prototypes[item].m_recommendedTerrainMask=std::bitset<10>(masks[item]);
            properties[item].m_prototype=&prototypes[item];
            properties[item].m_preferredTerrain=-99;
            properties[item].m_placementRule=0;
            owner.m_objectPrototypes[groups[item/4]].push_back(&properties[item]);
        }
        owner.readObjectPlacementRules();
        if(g_disposals!=1 || g_badFilename || owner.m_placementRules.size()!=static_cast<unsigned>(n)) return false;
        for(int r=0;r<n;++r) {
            const TRmgObjectPlacementRule& rule=owner.m_placementRules[r];
            if(rule.m_index!=r || rule.m_adjacentScores.size()!=static_cast<unsigned>(n)
                || rule.m_blockedScores.size()!=static_cast<unsigned>(n)) return false;
            for(int t=0;t<10;++t) if(rule.m_terrainScores[t]!=(t<9?value(r,t+7):-5000)) return false;
            for(int j=0;j<n;++j) if(rule.m_adjacentScores[j]!=value(r,j+16)
                || rule.m_blockedScores[j]!=value(r,j+n+16)) return false;
        }
        for(int item=0;item<16;++item) {
            int preferred=9;
            for(int t=8;t>=0;--t) if(masks[item] & (1ul<<t)) preferred=t;
            int expected=-1;
            if(preferred!=9) for(int r=0;r<n;++r)
                if(ruleType(r)==remap[groups[item/4]] && ruleTerrain(r,mask)==preferred
                    && ruleSubtype(r)==item%3-1) expected=r;
            TRmgObjectPlacementRule* wanted=expected<0?0:&owner.m_placementRules[expected];
            if(properties[item].m_preferredTerrain!=preferred || properties[item].m_placementRule!=wanted
                || properties[item].m_prototype!=&prototypes[item] || prototypes[item].m_subtype!=item%3-1
                || prototypes[item].m_recommendedTerrainMask.to_ulong()!=masks[item]) return false;
            if(owner.m_objectPrototypes[groups[item/4]][item%4]!=&properties[item]) return false;
        }
    }
    return true;
}
}
"""
        text += "int main() {\n"
        text += "".join(f"if (!Case{i}::check()) return 1;\n" for i in range(count))
        text += "".join(f"if (Case{i}::check()) return {i-count+2};\n" for i in range(count, len(forms)))
        text += "return 0;\n}\n"
        with tempfile.TemporaryDirectory(prefix="rmg-rule-reader-") as raw:
            directory = Path(raw)
            cpp, binary = directory / "reader.cpp", directory / "reader"
            cpp.write_text(text)
            result = subprocess.run([shutil.which("g++"), "-std=c++98", str(cpp), "-o", str(binary)],
                                    capture_output=True, text=True, timeout=180)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=90)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
