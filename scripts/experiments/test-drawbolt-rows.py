"""Compare the actual adopted DrawBolt with the saved pre-edit body.

Usage: python scripts/experiments/test-drawbolt-rows.py FAMILY_INPUT.json
Uses the source-family manifest's immutable control, actual SBolt, palette data,
RGBto16 and accessor bodies. The small shell supplies the screen and deterministic
RNG only. These native equivalence checks do not replace the VC6 retail verdict.
"""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]


def block(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


spells = (root / 'src/spells.cpp').read_text()
cmbt = (root / 'include/cmbtmgr.h').read_text()
manifest = json.loads(Path(sys.argv[1]).read_text())
reference = manifest['axes'][0]['find']
assert reference.startswith('void combatManager::drawBolt(')
assert 'g_boltSpectrumColors[k - spanFirst][0]' in reference
actual = block(spells, 'void combatManager::drawBolt(')
reference = reference.replace('::drawBolt(', '::drawBoltReference(', 1)
tables = '\n'.join(block(spells, 'unsigned char ' + name) + ';' for name in (
    'g_boltWhiteSpanColors[', 'g_boltGreenSpanColors[', 'g_boltSpectrumColors['))
fixture = r'''
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
using std::abs;
@BOLT@;
@COLORS@;
@DEPTHS@;
struct PixelFormat { unsigned dwRBitMask, dwGBitMask, dwBBitMask; } g_pixelFormat;
@RGB@
@TABLES@
int randomCalls;
int random(int low, int high) { ++randomCalls; return low + (high-low)/2; }
struct Bitmap {
    unsigned short* m_map;
    int m_pitch;
    @GETMAP@
};
struct Window { Bitmap* m_screenBitmap; } window;
Window* g_windowManager = &window;
struct combatManager {
    @BOUNDS@
    void drawBolt(SBolt*, int);
    void drawBoltReference(SBolt*, int);
};
@REFERENCE@
@ACTUAL@
int main() {
    const unsigned masks[3][3] = {{0xf800,0x7e0,0x1f}, {0x7c00,0x3e0,0x1f}, {0x1f,0x7e0,0xf800}};
    const int colors[] = {BOLT_COLOR_0, BOLT_COLOR_1, BOLT_COLOR_2, BOLT_COLOR_3,
                          BOLT_COLOR_4, BOLT_COLOR_CHAIN_LIGHTNING, 0x1234};
    std::vector<unsigned short> before(800*556+32), after(before.size());
    combatManager manager;
    int cases = 0;
    for (int format = 0; format < 3; ++format)
    for (int color : colors)
    for (int shallow = 0; shallow < 2; ++shallow)
    for (int scenario = 0; scenario < 7; ++scenario) {
        g_pixelFormat = {masks[format][0], masks[format][1], masks[format][2]};
        SBolt a = {};
        a.m_x = scenario == 1 ? 799 : scenario == 2 ? 0 : 400;
        a.m_y = scenario == 1 ? 555 : scenario == 2 ? 0 : 278;
        a.m_destX = 425;
        a.m_destY = 300;
        a.m_shallow = shallow;
        a.m_angle = scenario == 2 ? -2.3f : scenario == 3 ? 0.785398f : 0.0f;
        a.m_color = color;
        int radius = (color == BOLT_COLOR_0 || color == BOLT_COLOR_3 ||
                      color == BOLT_COLOR_CHAIN_LIGHTNING) ? 7 : 4;
        a.m_spanFirst = -radius;
        a.m_spanLast = radius;
        a.m_done = scenario >= 4;
        a.m_closestDistance = scenario == 4 ? 100 : 1;
        if (scenario == 5) { a.m_destX = 400; a.m_destY = 279; }
        const int length = scenario == 6 ? 0 : 24;
        SBolt b = a;
        std::fill(before.begin(), before.end(), 0xa55a);
        after = before;
        Bitmap bitmap = {before.data()+16, 1600};
        window.m_screenBitmap = &bitmap;
        randomCalls = 0;
        manager.drawBoltReference(&a, length);
        if (randomCalls != 1) return 2;
        bitmap.m_map = after.data()+16;
        randomCalls = 0;
        manager.drawBolt(&b, length);
        if (randomCalls != 1 || std::memcmp(&a, &b, sizeof a) || before != after) {
            std::fprintf(stderr, "Mismatch format=%d color=%d shallow=%d scenario=%d\n",
                         format, color, shallow, scenario);
            return 1;
        }
        for (int i = 0; i < 16; ++i)
            if (after[i] != 0xa55a || after[after.size()-1-i] != 0xa55a) return 2;
        ++cases;
    }
    std::printf("Passed %d drawBolt equivalence cases\n", cases);
}
'''
replacements = {
    '@BOLT@': block(cmbt, 'struct SBolt {'),
    '@COLORS@': block(cmbt, 'enum EBoltColor {'),
    '@DEPTHS@': block((root / 'include/spells.h').read_text(), 'enum EBoltSpanDepth'),
    '@RGB@': block((root / 'include/wingraph.h').read_text(), 'inline unsigned rgBto16('),
    '@GETMAP@': block((root / 'include/bitmap16.h').read_text(), 'unsigned short* getMap(int'),
    '@BOUNDS@': block(cmbt, 'unsigned char inCombatArea('),
    '@TABLES@': tables,
    '@REFERENCE@': reference,
}
for key, value in replacements.items():
    fixture = fixture.replace(key, value)
controls = {
    'actual': actual,
    'wrong-channel': actual.replace('rgb[0], rgb[1], rgb[2]', 'rgb[2], rgb[1], rgb[0]'),
    'missing-white-pixel': actual.replace('rgBto16(rgb[0], rgb[1], rgb[2])', '0', 1),
}
assert all(body != actual for name, body in controls.items() if name != 'actual')
with tempfile.TemporaryDirectory(prefix='homm3-drawbolt-') as directory:
    for optimization in ('-O0', '-O2'):
        for name, body in controls.items():
            source = Path(directory) / (name + '.cpp')
            binary = Path(directory) / name
            source.write_text(fixture.replace('@ACTUAL@', body))
            subprocess.run(['c++', '-std=c++17', optimization, str(source), '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], capture_output=True, text=True)
            assert result.returncode == (0 if name == 'actual' else 1), result.stderr
            print(optimization, name, 'PASS' if name == 'actual' else 'REJECTED', result.stdout.strip())
