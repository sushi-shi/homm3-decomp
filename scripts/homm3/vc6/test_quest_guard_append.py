"""Validate QUEST_GUARD append order, indexing and optional data registration."""
import importlib.util
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class QuestGuardAppendTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('g++'), 'requires native C++ compiler')
    def test_arm_and_supported_append_scopes(self):
        spec = importlib.util.spec_from_file_location(
            'quest_append_family', ROOT / 'scripts/experiments/generate-quest-guard-append-family.py')
        family = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(family)
        actual = family.original_body()
        current_arm = family.guard_arm(actual)
        bodies = [(name, family.guard_arm(body)) for name, body in family.variants(actual)]
        arms = list(dict.fromkeys(arm for _, arm in bodies))
        variants = [('Actual', current_arm, True)]
        variants.extend((f'Family{i}', arm, True) for i, arm in enumerate(arms))
        variants += [
            ('WrongIndex', current_arm.replace('.size() - 1', '.size()'), False),
            ('MissingRead', current_arm.replace('tempGuard.read(infile);', ''), False),
            ('WrongNullGuard', current_arm.replace('if (tempGuard.m_quest)', 'if (!tempGuard.m_quest)'), False),
            ('WrongVisited', current_arm.replace('tempGuard.read(infile);',
                                                'tempGuard.read(infile);\n        tempGuard.m_visitedPlayers = 1;'), False),
            ('MissingRegistration', re.sub(r'^            m_mapObjectData\.insert\([^\n]+\);\n',
                                           '', current_arm, count=1, flags=re.M), False),
        ]
        source = (ROOT / 'src/seerhut.cpp').read_text()
        helpers = []
        for signature in ('TQuestGuard::TQuestGuard()', 'void TQuestGuard::read(TAbstractFile* infile)'):
            start = source.index(signature + '\n{')
            helpers.append(source[start:source.index('\n}', start) + 2])
        fixture = r'''
struct TAbstractFile {
    unsigned char type;
    int reads, loads;
    int read(void* value, unsigned size) {
        ++reads;
        if (size != 1) return 0;
        *static_cast<unsigned char*>(value) = type;
        return 1;
    }
};
struct CMapObjectData {};
struct type_quest : CMapObjectData {
    void loadFromMap(TAbstractFile* file) { ++file->loads; }
};
int creates, receivedType, receivedMode;
type_quest* g_testQuest;
type_quest* createQuest(unsigned char type, int mode) {
    ++creates; receivedType = type; receivedMode = mode;
    return type ? g_testQuest : 0;
}
struct TQuestGuard {
    type_quest* m_quest;
    unsigned char m_visitedPlayers;
    TQuestGuard();
    void read(TAbstractFile*);
};
// @HELPERS@
struct CObject { int m_extraInfo; };
enum { QUEST_GUARD = 1 };
struct NewfullMap {
    std::vector<TQuestGuard> m_questGuardList;
    std::vector<CMapObjectData*> m_mapObjectData;
    void readArm(TAbstractFile*, CObject*);
};
void NewfullMap::readArm(TAbstractFile* infile, CObject* tempObject) {
    switch (QUEST_GUARD) {
// @ARM@
    }
}
bool check() {
    for (int prior = 0; prior != 4; ++prior)
    for (int dataPrior = 0; dataPrior != 4; ++dataPrior)
    for (int reserved = 0; reserved != 2; ++reserved)
    for (int present = 0; present != 2; ++present) {
        NewfullMap map;
        type_quest quest, oldQuest;
        CMapObjectData data;
        g_testQuest = &quest;
        if (reserved) { map.m_questGuardList.reserve(64); map.m_mapObjectData.reserve(64); }
        for (int i = 0; i != prior; ++i) {
            TQuestGuard old;
            old.m_quest = &oldQuest;
            old.m_visitedPlayers = 7;
            map.m_questGuardList.push_back(old);
        }
        for (int i = 0; i != dataPrior; ++i) map.m_mapObjectData.push_back(&data);
        TAbstractFile file;
        file.type = present ? 17 : 0;
        file.reads = file.loads = 0;
        creates = 0;
        receivedType = receivedMode = -1;
        CObject object;
        object.m_extraInfo = -10;
        map.readArm(&file, &object);
        if (file.reads != 1 || file.loads != present || creates != 1
            || receivedType != file.type || receivedMode != 0) return false;
        if (object.m_extraInfo != prior || map.m_questGuardList.size() != static_cast<unsigned>(prior + 1)) return false;
        for (int i = 0; i != prior; ++i)
            if (map.m_questGuardList[i].m_quest != &oldQuest || map.m_questGuardList[i].m_visitedPlayers != 7) return false;
        if (map.m_questGuardList.back().m_quest != (present ? &quest : 0)
            || map.m_questGuardList.back().m_visitedPlayers != 0) return false;
        if (map.m_mapObjectData.size() != static_cast<unsigned>(dataPrior + present)) return false;
        for (int i = 0; i != dataPrior; ++i) if (map.m_mapObjectData[i] != &data) return false;
        if (present && map.m_mapObjectData.back() != static_cast<CMapObjectData*>(static_cast<void*>(&quest))) return false;
    }
    return true;
}
'''
        fixture = fixture.replace('// @HELPERS@', '\n'.join(helpers))
        programs, checks = [], []
        for name, arm, valid in variants:
            if not valid:
                self.assertNotEqual(arm, current_arm, name)
            programs.append('namespace ' + name + ' {\n' + fixture.replace('// @ARM@', arm) + '\n}')
            checks.append('if (' + name + '::check() != ' + str(valid).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = '#include <vector>\n#include <cstdio>\n' + '\n'.join(programs)
        program += '\nint main() {\n' + '\n'.join(checks) + '\n}\n'
        with tempfile.TemporaryDirectory(prefix='homm3-quest-guard-') as directory:
            cpp, executable = Path(directory) / 'oracle.cpp', Path(directory) / 'oracle'
            cpp.write_text(program)
            result = subprocess.run(['g++', '-std=c++98', '-O1', str(cpp), '-o', str(executable)],
                                    capture_output=True, text=True, timeout=180)
            self.assertEqual(result.returncode, 0, result.stderr[-6000:])
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
