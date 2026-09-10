"""Exercise the actual markEnemy body against its flag/minimum-cost contract."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
SIGNATURE = 'void searchArray::markEnemy(long hex, long cost)'


class MarkEnemyTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('g++'), 'requires native C++ compiler')
    def test_flags_cost_narrowing_and_accessor_before_guard(self):
        source = (ROOT / 'src/findpath.cpp').read_text()
        start = source.index(SIGNATURE + '\n{')
        actual = source[start:source.index('\n}', start) + 2]
        variants = [
            ('Actual', actual, True),
            ('Empty', SIGNATURE + '\n{\n}\n', False),
            ('WrongGuard', actual.replace('cell->m_cost <= cost', 'cell->m_cost >= cost'), False),
            ('MissingFlag', actual.replace('combatCell->m_validMove = 1;', ''), False),
            ('WrongCost', actual.replace('static_cast<unsigned short>(cost)',
                                        'static_cast<unsigned short>(cost + 1)'), False),
            ('WrongCell', actual.replace('getHex(hex)', 'getHex(hex + 1)'), False),
        ]
        fixture = r'''
struct hexcell { unsigned char m_validMove; };
struct pathCell { unsigned short m_cost; };
struct CombatManager { hexcell m_cells[3]; };
CombatManager* g_combatManager;
struct searchArray {
    pathCell cells[3];
    mutable int calls;
    mutable long lastHex;
    pathCell* getHex(long hex) const {
        ++calls;
        lastHex = hex;
        return const_cast<pathCell*>(&cells[hex]);
    }
    void markEnemy(long hex, long cost);
};
// @BODY@
bool check() {
    const long costs[] = {-2147483647L - 1, -1, 0, 1, 7, 255, 256, 65534, 65535, 65536, 2147483647L};
    const unsigned short previous[] = {0, 1, 7, 255, 256, 65534, 65535};
    const unsigned char flags[] = {0, 1, 2, 255};
    for (unsigned c = 0; c != sizeof(costs)/sizeof(costs[0]); ++c)
    for (unsigned p = 0; p != sizeof(previous)/sizeof(previous[0]); ++p)
    for (unsigned f = 0; f != sizeof(flags)/sizeof(flags[0]); ++f) {
        CombatManager combat;
        g_combatManager = &combat;
        searchArray search;
        search.calls = 0;
        search.lastHex = -1;
        for (int i = 0; i != 3; ++i) {
            combat.m_cells[i].m_validMove = 17;
            search.cells[i].m_cost = 91;
        }
        combat.m_cells[1].m_validMove = flags[f];
        search.cells[1].m_cost = previous[p];
        search.markEnemy(1, costs[c]);
        bool update = !flags[f] || costs[c] < previous[p];
        unsigned char flag = update ? 1 : flags[f];
        unsigned short cost = update ? static_cast<unsigned short>(costs[c]) : previous[p];
        if (search.calls != 1 || search.lastHex != 1) return false;
        if (combat.m_cells[1].m_validMove != flag || search.cells[1].m_cost != cost) return false;
        if (combat.m_cells[0].m_validMove != 17 || combat.m_cells[2].m_validMove != 17
            || search.cells[0].m_cost != 91 || search.cells[2].m_cost != 91) return false;
    }
    return true;
}
'''
        programs, checks = [], []
        for name, body, valid in variants:
            if not valid:
                self.assertNotEqual(body, actual, name)
            programs.append('namespace ' + name + ' {\n' + fixture.replace('// @BODY@', body) + '\n}')
            checks.append('if (' + name + '::check() != ' + str(valid).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = '#include <cstdio>\n' + '\n'.join(programs)
        program += '\nint main() {\n' + '\n'.join(checks) + '\n}\n'
        with tempfile.TemporaryDirectory(prefix='homm3-mark-enemy-') as directory:
            cpp, executable = Path(directory) / 'oracle.cpp', Path(directory) / 'oracle'
            cpp.write_text(program)
            result = subprocess.run(['g++', '-std=c++98', '-O1', str(cpp), '-o', str(executable)],
                                    capture_output=True, text=True, timeout=180)
            self.assertEqual(result.returncode, 0, result.stderr[-6000:])
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
