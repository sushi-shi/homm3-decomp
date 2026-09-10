"""Exercise the actual required-definition lookup bodies in a native shell.

The shell supplies vectors, records and sprite acquisition, not the search or
publication algorithm. Tests cover reverse precedence, terrain selection,
cached indices, successful publication and failure before any publication.
This validates behavior, not VC6 layout or the unrelated malformed-input API.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, default=root / 'src/mapcell.cpp')
args = parser.parse_args()
source = args.source.read_text()


def function(signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]


bodies = '\n'.join(function(signature) for signature in (
    'static void missingMapObjectDefinition(',
    'CObjectType* NewfullMap::newfullMapFn00505EA0(',
    'void NewfullMap::newfullMapFn00505F20(',
))
fixture = r'''
#include <bitset>
#include <stdexcept>
#include <string>
#include <vector>
struct CSprite {};
struct CObjectType {
    int m_extra;
    std::bitset<32> m_mask34;
    unsigned short m_objectTypeIndex;
    std::string m_imageName;
    CObjectType(int extra, unsigned mask, unsigned short index = 0xffff)
        : m_extra(extra), m_mask34(mask), m_objectTypeIndex(index), m_imageName("object") {}
};
struct CObject { unsigned short m_typeIndex; };
struct ResourceManager {
    static int calls;
    static CSprite sprite;
    static CSprite* getSprite(const char*) { ++calls; return &sprite; }
};
int ResourceManager::calls = 0;
CSprite ResourceManager::sprite;
struct NewfullMap {
    std::vector<CObjectType> m_objectTypeIndex[4];
    std::vector<CObjectType> m_objectTypes;
    std::vector<CSprite*> m_sprites;
    CObjectType* newfullMapFn00505EA0(int, int);
    void newfullMapFn00505F20(CObject*, int, int, int);
};
@BODIES@
bool check() {
    for (int objectType = 0; objectType < 4; ++objectType) {
        NewfullMap map;
        auto& types = map.m_objectTypeIndex[objectType];
        types.push_back(CObjectType(7, 1u << 2, 17));
        types.push_back(CObjectType(9, 1u << 3, 18));
        types.push_back(CObjectType(7, 1u << 4, 19));
        if (map.newfullMapFn00505EA0(objectType, 7) != &types[2]
            || map.newfullMapFn00505EA0(objectType, 9) != &types[1]) return false;
        CObject object = {0x5555};
        map.newfullMapFn00505F20(&object, objectType, 7, -1);
        if (object.m_typeIndex != 19) return false;
        map.newfullMapFn00505F20(&object, objectType, 7, 2);
        if (object.m_typeIndex != 17) return false;
        map.newfullMapFn00505F20(&object, objectType, 7, 4);
        if (object.m_typeIndex != 19) return false;
        if (!map.m_objectTypes.empty() || !map.m_sprites.empty()) return false;

        types.push_back(CObjectType(11, 1u << 5));
        const int before = ResourceManager::calls;
        map.newfullMapFn00505F20(&object, objectType, 11, 5);
        if (object.m_typeIndex != 0 || types[3].m_objectTypeIndex != 0
            || map.m_objectTypes.size() != 1 || map.m_sprites.size() != 1
            || map.m_objectTypes[0].m_extra != 11
            || map.m_sprites[0] != &ResourceManager::sprite
            || ResourceManager::calls != before + 1) return false;
        map.newfullMapFn00505F20(&object, objectType, 11, -1);
        if (map.m_objectTypes.size() != 1 || ResourceManager::calls != before + 1) return false;

        // Exercise missing extras, missing applicable terrain and an empty
        // class. A failure must precede both vector publication and output.
        for (int empty = 0; empty < 2; ++empty) {
            if (empty) types.clear();
            bool threw = false;
            try { map.newfullMapFn00505EA0(objectType, 123); }
            catch (const std::out_of_range&) { threw = true; }
            if (!threw) return false;
            for (int terrain : {-1, 0, 2, 5, 31}) {
                for (int extra : {7, 123}) {
                    if (!empty && extra == 7 && (terrain == -1 || terrain == 2)) continue;
                    object.m_typeIndex = 0x5555;
                    threw = false;
                    try { map.newfullMapFn00505F20(&object, objectType, extra, terrain); }
                    catch (const std::out_of_range&) { threw = true; }
                    if (!threw || object.m_typeIndex != 0x5555
                        || map.m_objectTypes.size() != 1 || map.m_sprites.size() != 1
                        || ResourceManager::calls != before + 1) return false;
                }
            }
        }
    }
    return true;
}
int main() { return check() ? 0 : 1; }
'''
variants = [('actual', bodies, True)]
for name, before, after in (
    ('wrong-extra', '.m_extra == extra', '.m_extra != extra'),
    ('wrong-terrain', 'if (m_objectTypeIndex[objectType][i].m_mask34[terrain])',
     'if (!m_objectTypeIndex[objectType][i].m_mask34[terrain])'),
    ('missing-failure-check', 'if (i < 0)', 'if (i < -1)'),
):
    assert before in bodies, name
    variants.append((name, bodies.replace(before, after), False))

with tempfile.TemporaryDirectory(prefix='homm3-map-lookup-boundary-') as directory:
    scratch = Path(directory)
    for opt in ('-O0', '-O2'):
        for name, body, expected in variants:
            path = scratch / (name + '.cpp')
            path.write_text(fixture.replace('@BODIES@', body))
            binary = scratch / name
            subprocess.run(['c++', '-std=c++17', opt, '-U_FORTIFY_SOURCE', '-D_GLIBCXX_ASSERTIONS',
                            '-Wno-unknown-pragmas', str(path), '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], capture_output=True, timeout=10)
            assert (result.returncode == 0) == expected, (name, opt, result.returncode, result.stderr)
            print(opt, name, 'PASS' if expected else 'correctly rejected', flush=True)
