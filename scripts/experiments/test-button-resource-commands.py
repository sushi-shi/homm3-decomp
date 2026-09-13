"""Exercise the authored button command arm and palette helper.

The event oracle distinguishes palette failure from icon replacement, which
retail handles with separate return/dispose paths. Compile independent wrong
reload and missing-disposal controls alongside the actual source.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'src/button.cpp').read_text()
start = source.index('void button::setPalette(const char* paletteName)')
helper = source[start:source.index('\n}', start) + 2]
start = source.index('    case MESSAGE_WIDGET: {', source.index('int button::main('))
end = source.index('    case MESSAGE_KEY_DOWN:', start)
arm = source[start:end]
assert arm.count('setPalette(msg.m_extraText);') == 1
assert arm.count('m_buttonIcon->dispose();') == 1

fixture = r'''
#include <cstdio>
#include <string>
#include <vector>
static std::vector<std::string> events;
static bool paletteAvailable;
struct TPalette16 {
    unsigned short m_data[1];
    void dispose() { events.push_back("palette.dispose"); }
};
struct CSprite {
    void setPalette(const unsigned short*) { events.push_back("sprite.setPalette"); }
    void dispose() { events.push_back("sprite.dispose"); }
};
static CSprite oldSprite, newSprite;
static TPalette16 palette;
struct ResourceManager {
    static TPalette16* getPalette(const char*) {
        events.push_back("getPalette"); return paletteAvailable ? &palette : 0;
    }
    static CSprite* getSprite(const char*) {
        events.push_back("getSprite"); return &newSprite;
    }
};
struct widget {
    enum { WIDGET_SET_PALETTE, WIDGET_SET_ICON_NAME, WIDGET_SET_TEXT,
           WIDGET_SET_PLAYER_PALETTE_COLORS };
};
enum { MESSAGE_WIDGET };
struct message { int m_codeY, m_codeX, m_extra; const char* m_extraText; };
struct button {
    int m_id;
    CSprite* m_buttonIcon;
    void setPalette(const char* paletteName);
    void setText(const char*) { events.push_back("setText"); }
    void setPlayerPaletteColors(int) { events.push_back("setPlayerPaletteColors"); }
    int dispatch(message& msg) {
        switch (MESSAGE_WIDGET) {
@ARM@
        }
        return 0;
    }
};
@HELPER@
int main() {
    unsigned cases = 0;
    auto check = [&](int command, bool available, CSprite* original,
                     const std::vector<std::string>& expected,
                     CSprite* finalSprite, int id = 7, int result = 1) {
        events.clear(); paletteAvailable = available;
        button b = {7, original}; message msg = {id, command, 2, "resource"};
        if (b.dispatch(msg) != result || b.m_buttonIcon != finalSprite
            || events != expected) return false;
        ++cases; return true;
    };
    if (!check(widget::WIDGET_SET_PALETTE, true, &oldSprite,
               {"getPalette", "sprite.setPalette", "palette.dispose"}, &oldSprite)) return 1;
    if (!check(widget::WIDGET_SET_PALETTE, false, &oldSprite,
               {"getPalette"}, &oldSprite)) return 1;
    if (!check(widget::WIDGET_SET_ICON_NAME, false, &oldSprite,
               {"sprite.dispose", "getSprite"}, &newSprite)) return 1;
    if (!check(widget::WIDGET_SET_ICON_NAME, false, 0,
               {"getSprite"}, &newSprite)) return 1;
    if (!check(widget::WIDGET_SET_TEXT, false, &oldSprite,
               {"setText"}, &oldSprite)) return 1;
    if (!check(widget::WIDGET_SET_PLAYER_PALETTE_COLORS, false, &oldSprite,
               {"setPlayerPaletteColors"}, &oldSprite)) return 1;
    if (!check(widget::WIDGET_SET_PALETTE, true, &oldSprite,
               {}, &oldSprite, 8, 0)) return 1;
    if (!check(999, true, &oldSprite, {}, &oldSprite, 7, 0)) return 1;
    std::printf("%u button resource-command cases\n", cases);
    return 0;
}
'''
variants = [
    ('actual', arm, 0),
    ('wrong-palette-failure-reload', arm.replace(
        'setPalette(msg.m_extraText);',
        'setPalette(msg.m_extraText);\n'
        '            if (!paletteAvailable)\n'
        '                m_buttonIcon = ResourceManager::getSprite(msg.m_extraText);'), 1),
    ('missing-icon-disposal', arm.replace(
        'm_buttonIcon->dispose();', '(void)m_buttonIcon;'), 1),
]
with tempfile.TemporaryDirectory(prefix='homm3-button-commands-') as directory:
    scratch = Path(directory)
    for opt in ('-O0', '-O2'):
        for name, candidate, expected in variants:
            path = scratch / (name + '.cpp')
            path.write_text(fixture.replace('@ARM@', candidate).replace('@HELPER@', helper))
            binary = scratch / name
            subprocess.run(['c++', '-std=c++17', opt, str(path), '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], timeout=30)
            assert result.returncode == expected, (opt, name, result.returncode, expected)
            print(opt, name, 'PASS' if expected == 0 else 'correctly rejected', flush=True)
