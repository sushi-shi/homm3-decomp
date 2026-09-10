"""Native actual-body checks for palette and serialized-diff ownership.

These are semantic checks, not substitutes for the VC6 ABI/retail build.
The resource shell only supplies the constructors' unrelated base operation.
All changed palette code, the raw-copy constructor, diff accessors and Apply
are extracted from production. Inputs are valid palettes/diff records; malformed
stream handling and legacy object-lifetime conventions are not certified.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]


palette = (root / 'src/palette.cpp').read_text()
bodies = '\n'.join(function(palette, sig) for sig in (
    'void TPalette16::convert24to16(',
    'TPalette16::TPalette16(const TPalette24* p24, int rbits,',
    'TPalette16::TPalette16(const char* name, const TPalette24* p24,',
    'TPalette16::TPalette16(const TPalette24* p24)\n',
    'TPalette24::TPalette24(const unsigned char*',
))
diff = function((root / 'src/diff.cpp').read_text(),
                'void* CDiffFile::apply(')
fixture = r'''
#include <cstring>
#include <cstdint>
#include "diff.h"
enum { RESOURCE_TYPE_NONE, RESOURCE_TYPE_PALETTE };
struct resource { resource(const char*, int) {} };
struct paletteHiColor { unsigned char m_data[256][3]; };
struct TPalette24 : resource {
    paletteHiColor m_colors;
    TPalette24() : resource(0, 0) {}
    TPalette24(const unsigned char*);
};
struct TPalette16 : resource {
    static unsigned int s_redMask, s_greenMask, s_blueMask;
    unsigned short m_data[256] = {};
    TPalette16(const TPalette24*, int, int, int, int, int, int);
    TPalette16(const char*, const TPalette24*, int, int, int, int, int, int);
    TPalette16(const TPalette24*);
    void convert24to16(const unsigned char*, int, int, int, int, int, int);
};
unsigned int TPalette16::s_redMask, TPalette16::s_greenMask, TPalette16::s_blueMask;
@BODIES@
@DIFF@
bool checkPalette() {
    const int formats[3][6] = {{5,10,5,5,5,0}, {5,11,6,5,5,0}, {5,0,6,5,5,11}};
    for (int format = 0; format < 3; ++format)
    for (int seed = 0; seed < 16; ++seed) {
        TPalette24 input;
        for (int i = 0; i < 256; ++i)
            for (int c = 0; c < 3; ++c)
                input.m_colors.m_data[i][c] = (i * (2*c+1) + 37*seed + 61*c) & 255;
        paletteHiColor unchanged = input.m_colors;
        const int* f = formats[format];
        unsigned int masks[3];
        for (int c = 0; c < 3; ++c)
            masks[c] = ((1u << f[2*c]) - 1) << f[2*c+1];
        TPalette16::s_redMask = masks[0];
        TPalette16::s_greenMask = masks[1];
        TPalette16::s_blueMask = masks[2];
        TPalette16 a(&input), b(&input, f[0],f[1],f[2],f[3],f[4],f[5]);
        TPalette16 c("palette", &input, f[0],f[1],f[2],f[3],f[4],f[5]);
        TPalette24 copy(static_cast<const unsigned char*>(
            static_cast<const void*>(&input.m_colors)));
        if (std::memcmp(&copy.m_colors, &unchanged, sizeof unchanged)
            || std::memcmp(&input.m_colors, &unchanged, sizeof unchanged)) return false;
        for (int i = 0; i < 256; ++i) {
            unsigned int expected = 0;
            for (int channel = 0; channel < 3; ++channel)
                expected |= (unchanged.m_data[i][channel] >> (8-f[2*channel])) << f[2*channel+1];
            if (a.m_data[i] != expected || b.m_data[i] != expected
                || c.m_data[i] != expected) return false;
        }
    }
    return true;
}
bool checkDiff() {
    static_assert(sizeof(CDiffFile) == 4, "size-word header");
    static_assert(sizeof(CDiffHeader) == 12, "record header");
    unsigned char* storage = new unsigned char[64];
    std::memset(storage, 0xcc, 64);
    CDiffFile* file = static_cast<CDiffFile*>(static_cast<void*>(storage));
    file->m_numBytes = 8;
    if (file->getBase() != storage || file->getData() != storage + 4) return false;
    CDiffHeader same(3, 0, 0), changed(2, 1, 2), last(3, 0, 0);
    std::memcpy(storage + 4, &same, 12);
    std::memcpy(storage + 16, &changed, 12);
    std::memcpy(storage + 28, "XY", 2);
    std::memcpy(storage + 30, &last, 12);
    unsigned char old[] = "abcdefgh";
    unsigned char snapshot[64];
    std::memcpy(snapshot, storage, 64);
    unsigned char* result = static_cast<unsigned char*>(file->apply(old, 8));
    bool okay = !std::memcmp(result, "abcXYfgh", 8)
        && !std::memcmp(storage, snapshot, 64)
        && !std::memcmp(old, "abcdefgh", 9);
    delete[] result;
    delete[] storage;
    return okay;
}
int main() { return checkPalette() && checkDiff() ? 0 : 1; }
'''
actual = fixture.replace('@BODIES@', bodies).replace('@DIFF@', diff)
variants = [('actual', actual, True)]
for name, before, after in (
    ('palette-misses-last', 'index < 256', 'index < 255'),
    ('palette-wrong-channel', '(*src)[2] * blueScale', '(*src)[0] * blueScale'),
    ('diff-wrong-source-offset', 'oldOffset += header->m_numBytes;',
     'oldOffset += header->m_numBytes - 1;'),
):
    assert before in actual, name
    variants.append((name, actual.replace(before, after), False))

with tempfile.TemporaryDirectory(prefix='homm3-owner-payload-') as directory:
    scratch = Path(directory)
    for opt in ('-O0', '-O2'):
        for name, code, expected in variants:
            source = scratch / (name + '.cpp')
            source.write_text(code)
            binary = scratch / name
            subprocess.run(['c++', '-std=c++17', opt, '-U_FORTIFY_SOURCE', '-I', str(root / 'include'),
                            str(source), '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], timeout=10)
            assert result.returncode == (0 if expected else 1), (name, opt, result.returncode)
            print(opt, name, 'PASS' if expected else 'correctly rejected', flush=True)
