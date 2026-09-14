"""Check recovered map readers and their caller's discarded-status contract."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class MapReaderHelperTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('g++'), 'requires native C++ compiler')
    def test_reader_effects_short_reads_and_discarded_status(self):
        spec = importlib.util.spec_from_file_location(
            'map_reader_family', ROOT / 'scripts/experiments/generate-map-reader-helper-family.py')
        family = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(family)
        source = (ROOT / 'src/mapcell.cpp').read_text()
        start = source.index(family.SIGNATURE)
        caller = source[start:source.index('\n}', start) + 2]
        names = ['readBoatData', 'readHolyGrailData', 'readShrineData', 'readShipyardData']
        helpers = []
        for name in names:
            signature = 'int NewfullMap::' + name + '(TAbstractFile* infile'
            if signature in source:
                start = source.index(signature)
                helpers.append(source[start:source.index('\n}', start) + 2])
        self.assertIn(len(helpers), (0, 4), 'Do not silently skip a partial restoration')
        planned = '\n'.join(family.HELPERS.values())
        variants = [('Planned', planned, True)]
        if helpers:
            variants.append(('Current', '\n'.join(helpers), True))
        variants += [
            ('WrongBoatOwner', planned.replace('boatObject->m_z, -1, 1, boatType',
                                               'boatObject->m_z, 0, 1, boatType'), False),
            ('WrongRadius', planned.replace('m_ultimateRadius = charBuffer',
                                            'm_ultimateRadius = 0'), False),
            ('WrongShrine', planned.replace('m_spell = charBuffer', 'm_spell = 0'), False),
            ('WrongShipyard', planned.replace('m_boatY = 0xff', 'm_boatY = 0'), False),
            ('WrongStatus', planned.replace('return -1;', 'return 0;'), False),
        ]
        arms = []
        for arm, next_arm in [('BOAT', 'RANDOM_TOWN'), ('HOLY_GRAIL', 'BLACK_BOX'),
                              ('SHRINE1', 'OCEAN_BOTTLE'), ('SHIPYARD', 'RANDOM_RESOURCE')]:
            start = caller.index('    case ' + arm + ':')
            end = caller.index('    case ' + next_arm + ':', start)
            arms.append(caller[start:end])
        dispatch = ('int NewfullMap::dispatch(TAbstractFile* infile, CObject* tempObject, int type)\n'
                    '{\n    char value;\n    switch(type) {\n' + ''.join(arms) + '\n    }\n    return 1;\n}')
        fixture = r'''
struct TAbstractFile {
    int first, second, calls, byte;
    std::vector<int> sizes;
    TAbstractFile(int a, int b, int value) : first(a), second(b), calls(0), byte(value) {}
    int read(void* buffer, unsigned size) {
        sizes.push_back(size);
        ++calls;
        int result = calls == 1 ? first : second;
        if (result > 0) std::memset(buffer, byte, size);
        return result;
    }
};
struct CObject {
    unsigned char m_x, m_y, m_z;
    unsigned short m_typeIndex;
    int m_extraInfo;
    struct { int m_spell; } m_shrineInfo;
    struct { int m_owner; unsigned char m_boatX, m_boatY; } m_shipyardInfo;
    int triggers;
    CObject() : m_x(8), m_y(9), m_z(1), m_typeIndex(0), m_extraInfo(-8), triggers(0) {
        m_shrineInfo.m_spell = 42;
        m_shipyardInfo.m_owner = 3;
        m_shipyardInfo.m_boatX = 12;
        m_shipyardInfo.m_boatY = 13;
    }
    void findTrigger(int& x, int& y) { ++triggers; x = 18; y = 19; }
};
struct Game {
    short m_ultimateArtifactX, m_ultimateArtifactY;
    unsigned char m_ultimateArtifactZ, m_ultimateRadius;
    std::vector<int> boats;
    Game() : m_ultimateArtifactX(-1), m_ultimateArtifactY(-2),
             m_ultimateArtifactZ(7), m_ultimateRadius(11) {}
    int createBoat(int x, int y, int z, int owner, int initial, signed char type) {
        boats.push_back(x); boats.push_back(y); boats.push_back(z);
        boats.push_back(owner); boats.push_back(initial); boats.push_back(type);
        return 123;
    }
};
Game* g_game;
enum { BOAT, HOLY_GRAIL, SHRINE1, SHRINE2, SHRINE3, SHIPYARD };
struct NewfullMap {
    struct ObjectType { int m_extra; };
    std::vector<ObjectType> m_objectTypes;
    NewfullMap() : m_objectTypes(1) { m_objectTypes[0].m_extra = 255; }
    int readBoatData(TAbstractFile*, CObject*);
    int readHolyGrailData(TAbstractFile*, CObject*);
    int readShrineData(TAbstractFile*, CObject*);
    int readShipyardData(TAbstractFile*, CObject*);
    int dispatch(TAbstractFile*, CObject*, int);
};
// @HELPERS@
// @DISPATCH@
bool check() {
    for (int type = BOAT; type <= SHIPYARD; ++type)
    for (int first = 0; first <= 1; ++first)
    for (int second = 0; second <= 3; ++second)
    for (int byte = 0; byte <= 255; ++byte)
    for (int caller = 0; caller <= 1; ++caller) {
        Game game;
        g_game = &game;
        NewfullMap map;
        CObject object;
        TAbstractFile input(first, second, byte);
        int result;
        if (caller) result = map.dispatch(&input, &object, type);
        else if (type == BOAT) result = map.readBoatData(&input, &object);
        else if (type == HOLY_GRAIL) result = map.readHolyGrailData(&input, &object);
        else if (type == SHIPYARD) result = map.readShipyardData(&input, &object);
        else result = map.readShrineData(&input, &object);
        if (result != (caller ? 1 : (type == BOAT || (first == 1 && second == 3) ? 0 : -1))) return false;
        int count = type == BOAT ? 0 : first ? 2 : 1;
        if (input.calls != count || input.sizes.size() != static_cast<unsigned>(count)) return false;
        if (count && input.sizes[0] != 1) return false;
        if (count == 2 && input.sizes[1] != 3) return false;
        if (type == BOAT) {
            const int expected[] = {18,19,1,-1,1,-1};
            if (object.triggers != 1 || object.m_extraInfo != 123 || game.boats.size() != 6) return false;
            for (int i = 0; i != 6; ++i) if (game.boats[i] != expected[i]) return false;
        } else if (!game.boats.empty() || object.triggers || object.m_extraInfo != -8) return false;
        bool grail = type == HOLY_GRAIL && first;
        if (game.m_ultimateArtifactX != (grail ? 8 : -1)
            || game.m_ultimateArtifactY != (grail ? 9 : -2)
            || game.m_ultimateArtifactZ != (grail ? 1 : 7)
            || game.m_ultimateRadius != (grail ? byte : 11)) return false;
        bool shrine = type >= SHRINE1 && type <= SHRINE3 && first;
        int signedByte = byte < 128 ? byte : byte - 256;
        if (object.m_shrineInfo.m_spell != (shrine ? signedByte : 42)) return false;
        bool shipyard = type == SHIPYARD && first;
        if (object.m_shipyardInfo.m_owner != (shipyard ? signedByte : 3)) return false;
        bool ready = shipyard && second == 3;
        if (object.m_shipyardInfo.m_boatX != (ready ? 255 : 12)
            || object.m_shipyardInfo.m_boatY != (ready ? 255 : 13)) return false;
    }
    return true;
}
'''
        programs, checks = [], []
        for name, body, valid in variants:
            if not valid:
                self.assertNotEqual(body, planned, name)
            program = fixture.replace('// @HELPERS@', body).replace('// @DISPATCH@', dispatch)
            programs.append('namespace ' + name + ' {\n' + program + '\n}')
            checks.append('if (' + name + '::check() != ' + str(valid).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = '#include <vector>\n#include <cstring>\n#include <cstdio>\n' + '\n'.join(programs)
        program += '\nint main() {\n' + '\n'.join(checks) + '\n}\n'
        with tempfile.TemporaryDirectory(prefix='homm3-map-readers-') as directory:
            cpp, executable = Path(directory) / 'oracle.cpp', Path(directory) / 'oracle'
            cpp.write_text(program)
            result = subprocess.run(['g++', '-std=c++98', '-O1', '-fsigned-char', str(cpp),
                                     '-o', str(executable)], capture_output=True, text=True, timeout=180)
            self.assertEqual(result.returncode, 0, result.stderr[-6000:])
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
