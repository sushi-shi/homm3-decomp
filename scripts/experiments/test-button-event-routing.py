"""Check the authored button dispatcher against retail event routing.

The portable fixture stubs resource/input services; it is not a VC6 verdict.
Retail's right-button dispatch alone enters +0x61d, while unhandled widget,
keyboard and mouse events reach widget::Main at +0x621. A negative control
moves the right hit test into the shared tail, reproducing the former defect.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'src/button.cpp').read_text()
start = source.index('int button::main(message& msg)')
handler = source[start:source.index('\n}', start) + 2]
right_start = handler.index('    case MESSAGE_RIGHT_BUTTON_DOWN: {')
right_end = handler.index('    default:', right_start)
right = handler[right_start:right_end]
body = right[right.index('        short rightX'):right.rindex('\n    }')]
wrong = handler[:right_start] + '    case MESSAGE_RIGHT_BUTTON_DOWN:\n        break;\n' + handler[right_end:]
wrong = wrong.replace('    return widget::main(msg);\n}',
    '    if (!(m_status & WIDGET_DRAWN))\n        return widget::main(msg);\n' + body + '\n}')

wrong_cancel = handler.replace('        return 1;\n    }\n    case MESSAGE_LEFT_BUTTON_UP:',
                               '        return 0;\n    }\n    case MESSAGE_LEFT_BUTTON_UP:')
assert wrong_cancel != handler

fixture = r'''
#include <cstdio>
#include <vector>
struct message {
    int m_id, m_codeX, m_codeY, m_qualifier, m_extra;
    const char* m_extraText;
};
enum { MESSAGE_KEY_DOWN=1, MESSAGE_KEY_UP=2, MESSAGE_MOUSE_MOVE=4,
       MESSAGE_LEFT_BUTTON_DOWN=8, MESSAGE_LEFT_BUTTON_UP=16,
       MESSAGE_RIGHT_BUTTON_DOWN=32, MESSAGE_RIGHT_BUTTON_UP=64,
       MESSAGE_WIDGET=512, MESSAGE_MODIFIER_RIGHT=512 };
struct widget {
    enum { WIDGET_SELECTED=1, WIDGET_ACTIVE=2, WIDGET_DRAWN=4,
           WIDGET_DIMMED=8, WIDGET_DISABLED=32, WIDGET_STYLE_AUTO_REPEAT=4096,
           WIDGET_SET_PALETTE=10, WIDGET_SET_ICON_NAME=11, WIDGET_SET_TEXT=3,
           WIDGET_SET_PLAYER_PALETTE_COLORS=13, WIDGET_RIGHT_SELECT=14 };
    int main(message&) { return 77; }
};
struct Sprite { void dispose() {} };
struct ResourceManager { static Sprite* getSprite(const char*) { return 0; } };
struct Window { int m_x, m_y; };
static std::vector<message> queued;
static unsigned eventIndex;
struct Manager {
    void main(message&) {}
    message getEvent() { return queued.at(eventIndex++); }
};
static Manager manager, *g_mouseManager=&manager, *g_inputManager=&manager;
static unsigned long g_timers[1];
enum { GLOBAL_BUTTON_REPEAT_TIMER_SLOT=0 };
struct GameTime { static int elapsedSince(unsigned long) { return 0; } };
static void process1WindowsMessage() {}
static void pollSound() {}
struct button : widget {
    int m_style=0, m_status=0, m_sleepCount=0, m_id=7;
    int m_x=10, m_y=20, m_width=30, m_height=40;
    Window* m_parentWindow;
    Sprite* m_buttonIcon=0;
    std::vector<int> m_hotKeyCodes={15};
    void setPalette(const char*) {}
    void setText(const char*) {}
    void setPlayerPaletteColors(int) {}
    int select(message&) { m_status |= WIDGET_SELECTED; return 102; }
    int deselect(message&) { int result=(m_status & WIDGET_SELECTED) ? 103 : 0; m_status &= ~WIDGET_SELECTED; return result; }
    int main(message&);
};
@HANDLER@
int main() {
    const int ids[]={MESSAGE_KEY_DOWN,MESSAGE_KEY_UP,MESSAGE_MOUSE_MOVE,
        MESSAGE_LEFT_BUTTON_UP,MESSAGE_RIGHT_BUTTON_DOWN,MESSAGE_RIGHT_BUTTON_UP,
        MESSAGE_WIDGET,1024};
    const int coords[][2]={{15,25},{9,25},{40,25},{15,19},{15,60},{10,20},{39,59}};
    unsigned count=0;
    Window parent={0,0};
    for (int status=0;status<64;++status) for (int id:ids) for (auto& xy:coords) {
        button b; b.m_status=status; b.m_parentWindow=&parent;
        message msg={id,xy[0],xy[1],123,0,0};
        int expected=77;
        bool right=false;
        if (!(status & widget::WIDGET_ACTIVE)) {
            expected=id==MESSAGE_WIDGET ? 77 : 0;
        } else if (id==MESSAGE_RIGHT_BUTTON_DOWN) {
            if (status & widget::WIDGET_DRAWN) {
                right=xy[0]>=10 && xy[0]<40 && xy[1]>=20 && xy[1]<60;
                expected=right ? 2 : 0;
            }
        } else if (id==MESSAGE_WIDGET) {
            expected=77; // Every tested command has a foreign widget id.
        } else if (status & widget::WIDGET_DISABLED) {
            expected=0;
        } else if (id==MESSAGE_KEY_DOWN || id==MESSAGE_KEY_UP) {
            if ((status & widget::WIDGET_DRAWN) && !(status & widget::WIDGET_DIMMED))
                expected=xy[0]!=15 ? 0 : id==MESSAGE_KEY_DOWN ? 102 :
                    (status & widget::WIDGET_SELECTED) ? 103 : 0;
        } else if (id==MESSAGE_LEFT_BUTTON_UP) {
            if ((status & widget::WIDGET_DRAWN) && (status & widget::WIDGET_SELECTED))
                expected=103;
        }
        int actual=b.main(msg);
        if (actual!=expected || (right ?
            msg.m_id!=MESSAGE_WIDGET || msg.m_codeX!=widget::WIDGET_RIGHT_SELECT ||
            msg.m_codeY!=7 || msg.m_qualifier!=MESSAGE_MODIFIER_RIGHT :
            msg.m_id!=id || msg.m_codeX!=xy[0] || msg.m_codeY!=xy[1] || msg.m_qualifier!=123)) {
            std::fprintf(stderr,"routing mismatch: status=%d id=%d xy=%d,%d got=%d expected=%d\n",
                         status,id,xy[0],xy[1],actual,expected);
            return 1;
        }
        ++count;
    }
    for (bool cancel:{false,true}) {
        button b; b.m_status=widget::WIDGET_ACTIVE|widget::WIDGET_DRAWN;
        b.m_parentWindow=&parent; queued.clear(); eventIndex=0;
        if (cancel) queued.push_back({MESSAGE_MOUSE_MOVE,0,0,0,0,0});
        queued.push_back({MESSAGE_LEFT_BUTTON_UP,15,25,0,0,0});
        message msg={MESSAGE_LEFT_BUTTON_DOWN,15,25,0,0,0};
        if (b.main(msg)!=(cancel ? 1 : 2)) return 1;
        ++count;
    }
    std::printf("%u button event-routing cases\n",count);
}
'''
with tempfile.TemporaryDirectory(prefix='homm3-button-routing-') as directory:
    scratch=Path(directory)
    for opt in ('-O0','-O2'):
        for name,candidate,expected in [('actual',handler,0),('wrong-shared-right-tail',wrong,1),('wrong-cancel-result',wrong_cancel,1)]:
            path=scratch/(name+'.cpp');path.write_text(fixture.replace('@HANDLER@',candidate))
            binary=scratch/name
            subprocess.run(['c++','-std=c++17',opt,str(path),'-o',str(binary)],check=True)
            result=subprocess.run([str(binary)],timeout=30)
            assert result.returncode==expected,(opt,name,result.returncode,expected)
            print(opt,name,'PASS' if expected==0 else 'correctly rejected',flush=True)
