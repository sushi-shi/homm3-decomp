"""Actual fillRect body: output oracle plus checked pointer-formation steps.

Sanitizers alone need not notice an unused end+x pointer. The fixture's byte
view checks the requested offset before forming it, while retaining the actual
production statements. The shell does not model VC6 layout or union lifetime.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, default=root / 'src/bitmap16.cpp')
args = parser.parse_args()
source = args.source.read_text()
start = source.index('void Bitmap16Bit::fillRect(')
body = source[start:source.index('\n}', start) + 2]
fixture = r'''
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>
static unsigned short* allocation;
static std::ptrdiff_t pixelCount;
struct Bitmap16MapPointer {
    unsigned short* m_pixels;
    struct ByteView {
        Bitmap16MapPointer& owner;
        void operator+=(int stride) {
            const std::ptrdiff_t next = (owner.m_pixels - allocation) * 2 + stride;
            if (next < 0 || next > pixelCount * 2 || next % 2)
                throw std::out_of_range("row pointer formation");
            owner.m_pixels = allocation + next / 2;
        }
    } m_bytes;
    Bitmap16MapPointer() : m_pixels(0), m_bytes{*this} {}
};
struct Bitmap16Bit {
    int m_width, m_height, m_pitch;
    unsigned short* getMap(int x, int y) {
        const std::ptrdiff_t offset = y * (m_pitch / 2) + x;
        if (offset < 0 || offset > pixelCount) throw std::out_of_range("initial map");
        return allocation + offset;
    }
    void fillRect(int, int, int, int, unsigned short);
};
@BODY@
int main() {
    unsigned long cases = 0;
    try {
        for (int width = 1; width <= 7; ++width)
        for (int height = 1; height <= 7; ++height)
        for (int padding : {0, 1, 3}) {
            const int stride = width + padding;
            std::vector<unsigned short> image(stride * height);
            pixelCount = image.size();
            allocation = image.data();
            Bitmap16Bit bitmap = {width, height, stride * 2};
            for (int x = 0; x <= width; ++x)
            for (int y = 0; y <= height; ++y)
            for (int w = 0; w <= width + 2; ++w)
            for (int h = 0; h <= height + 2; ++h)
            for (unsigned short color : {0, 0xffff, 0xace1}) {
                std::fill(image.begin(), image.end(), 0x1234);
                bitmap.fillRect(x, y, w, h, color);
                for (int iy = 0; iy < height; ++iy)
                for (int ix = 0; ix < stride; ++ix) {
                    const bool painted = ix >= x && ix < std::min(width, x + w)
                        && iy >= y && iy < std::min(height, y + h);
                    if (image[iy * stride + ix] != (painted ? color : 0x1234)) return 1;
                }
                ++cases;
            }
        }
    } catch (const std::out_of_range&) { return 2; }
    return cases ? 0 : 3;
}
'''
step = '            if (row)\n                dst.m_bytes += m_pitch;\n'
assert step in body, 'test expects the reviewed next-row boundary implementation'
unguarded = body.replace(step, '').replace('                dst.m_pixels[col] = color;',
    '                dst.m_pixels[col] = color;\n            dst.m_bytes += m_pitch;')
variants = [('actual', body, True), ('original-final-advance', unguarded, False),
            ('skip-first-column', body.replace('int col = 0;', 'int col = 1;'), False),
            ('wrong-stride', body.replace('dst.m_bytes += m_pitch;',
                                         'dst.m_bytes += m_pitch + 2;'), False)]
with tempfile.TemporaryDirectory(prefix='homm3-fill-boundary-') as directory:
    scratch = Path(directory)
    for opt in ('-O0', '-O2'):
        for name, candidate, expected in variants:
            path = scratch / (name + '.cpp')
            path.write_text(fixture.replace('@BODY@', candidate))
            binary = scratch / name
            subprocess.run(['c++', '-std=c++17', opt, '-U_FORTIFY_SOURCE',
                            str(path), '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], timeout=30)
            assert (result.returncode == 0) == expected, (name, opt, result.returncode)
            print(opt, name, 'PASS' if expected else 'correctly rejected', flush=True)
