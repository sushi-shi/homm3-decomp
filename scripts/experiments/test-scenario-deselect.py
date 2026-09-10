"""Exercise the actual deselect bodies and dispatcher against independent oracles.

The reduced native fixture checks flag preservation, accepted widget identity,
qualified base dispatch, message priority, virtual callback arguments and end-
dialog message mutation. It tests C++ behavior, not VC6 layout or inlining.
Five deliberately incorrect controls must fail at both -O0 and -O2.
"""

import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile


def function(source, signature):
    start = source.index(signature)
    begin = source.index("{", start)
    depth = 1
    end = begin + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def bodies(root):
    header = (root / "include/window.h").read_text()
    window = (root / "src/window.cpp").read_text()
    declaration = "virtual int onWidgetDeselect(int id, bool& exitFlag)"
    tail = header[header.index(declaration) + len(declaration):].lstrip()
    if tail.startswith("{"):
        base = function(header, declaration).replace(declaration,
            "int CHeroWindowEx::onWidgetDeselect(int id, bool& exitFlag)", 1)
    else:
        base = function(window, "int CHeroWindowEx::onWidgetDeselect(int id, bool& exitFlag)")
    return (base,
            function((root / "src/scenarioinfo.cpp").read_text(),
                     "int CScenarioInfoDlg::onWidgetDeselect(int id, bool& exitFlag)"),
            function(window, "int CHeroWindowEx::windowHandler(message* msg)"))


FIXTURE = r"""
#include <climits>
#include <cstdio>
enum { MESSAGE_MOUSE_MOVE=4, MESSAGE_WIDGET=512, MESSAGE_MODIFIER_RIGHT=512 };
struct widget { enum { WIDGET_END_DIALOG=10, WIDGET_SELECT=12,
                      WIDGET_DESELECT=13, WIDGET_RIGHT_SELECT=14 }; };
struct message { int m_id,m_qualifier,m_codeX,m_codeY,m_mouseX,m_mouseY; };
class CHeroWindowEx {
public:
    int rightCalls,hoverCalls,deselectCalls,lastId,lastX,lastY;
    bool rightResult,hoverResult,callbackExit;
    CHeroWindowEx():rightCalls(0),hoverCalls(0),deselectCalls(0),lastId(0),
        lastX(0),lastY(0),rightResult(false),hoverResult(false),callbackExit(false) {}
    virtual unsigned char processRightSelect(int id) { ++rightCalls;lastId=id;return rightResult; }
    virtual unsigned char processHover(int x,int y) { ++hoverCalls;lastX=x;lastY=y;return hoverResult; }
    int windowHandler(message* msg);
    int callBase(int id,bool& flag) { return CHeroWindowEx::onWidgetDeselect(id,flag); }
protected:
    virtual int onWidgetDeselect(int id,bool& flag);
};
struct CScenarioInfoDlg:CHeroWindowEx {
    enum { SCENARIO_INFO_ACCEPT_ID=188 };
    virtual int onWidgetDeselect(int id,bool& flag);
};
struct Probe:CHeroWindowEx {
    virtual int onWidgetDeselect(int id,bool& flag) {
        ++deselectCalls;lastId=id;flag=callbackExit;return 17;
    }
};
__BODIES__
bool direct(int id) {
    CHeroWindowEx base; CScenarioInfoDlg scenario;
    for(int initial=0;initial<2;++initial) {
        bool flag=initial!=0;
        if(base.callBase(id,flag)!=0 || flag!=(initial!=0)) return false;
        flag=initial!=0;
        if(scenario.onWidgetDeselect(id,flag)!=0 || flag!=(initial!=0 || id==188)) return false;
    }
    return true;
}
bool check() {
    for(int id=-32768;id<32768;++id) if(!direct(id)) return false;
    if(!direct(INT_MIN) || !direct(INT_MAX) || !direct(-65536) || !direct(65536)) return false;
    const int ids[]={4,512,999},qualifiers[]={0,512,513},codes[]={12,13,14,999},widgets[]={-1,188,189};
    for(int a=0;a<3;++a) for(int b=0;b<3;++b) for(int c=0;c<4;++c)
    for(int d=0;d<3;++d) for(int r=0;r<2;++r) for(int h=0;h<2;++h) for(int e=0;e<2;++e) {
        Probe probe;probe.rightResult=r!=0;probe.hoverResult=h!=0;probe.callbackExit=e!=0;
        message msg={ids[a],qualifiers[b],codes[c],widgets[d],123,-456};
        message expected=msg;
        int right=0,hover=0,deselect=0,result=0;
        if((qualifiers[b]&512) && (codes[c]==12 || codes[c]==14)) { right=1;result=r; }
        else if(ids[a]==4) { hover=1;result=h; }
        else if(ids[a]==512 && codes[c]==13) {
            deselect=1;
            if(e) { result=2;expected.m_id=512;expected.m_codeX=10;expected.m_codeY=10; }
        }
        if(probe.windowHandler(&msg)!=result || probe.rightCalls!=right ||
           probe.hoverCalls!=hover || probe.deselectCalls!=deselect) return false;
        if((right || deselect) && probe.lastId!=widgets[d]) return false;
        if(hover && (probe.lastX!=123 || probe.lastY!=-456)) return false;
        if(msg.m_id!=expected.m_id || msg.m_codeX!=expected.m_codeX || msg.m_codeY!=expected.m_codeY ||
           msg.m_qualifier!=expected.m_qualifier || msg.m_mouseX!=123 || msg.m_mouseY!=-456) return false;
    }
    return true;
}
"""


def run(root):
    base, scenario, dispatcher = bodies(root)
    forms = [("actual", base, scenario, dispatcher, True),
             ("base-result", base.replace("return 0;", "return 1;"), scenario, dispatcher, False),
             ("wrong-accept-id", base, scenario.replace("id == SCENARIO_INFO_ACCEPT_ID",
                                                       "id == SCENARIO_INFO_ACCEPT_ID + 1"), dispatcher, False),
             ("lost-exit-flag", base, scenario.replace("exitFlag = 1;", "exitFlag = 0;"), dispatcher, False),
             ("skipped-callback", base, scenario, dispatcher.replace(
                 "onWidgetDeselect(msg->m_codeY, exitFlag);", "(void)msg->m_codeY;"), False),
             ("wrong-end-result", base, scenario, dispatcher.replace("return 2;", "return 1;"), False)]
    rendered = []
    for index, (name, first, second, third, _) in enumerate(forms):
        if index and (first, second, third) == (base, scenario, dispatcher):
            raise ValueError("Negative control did not change source: " + name)
        rendered.append("namespace variant%d {\n%s\n}" % (index,
            FIXTURE.replace("__BODIES__", "\n".join((first, second, third)))))
    rendered.insert(0, "#include <climits>\n#include <cstdio>\n")
    rendered.append("int main() {")
    for index, (name, *_, succeeds) in enumerate(forms):
        rendered.append('if(variant%d::check()!=%s) { std::puts("FAIL: %s"); return 1; }' %
                        (index, "true" if succeeds else "false", name))
    rendered.append("return 0; }")
    compiler = shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        raise RuntimeError("A native C++ compiler is required")
    with tempfile.TemporaryDirectory(prefix="homm3-deselect-oracle-") as temporary:
        directory = Path(temporary)
        source = directory / "fixture.cpp"
        source.write_text("\n".join(rendered))
        for optimization in ("-O0", "-O2"):
            executable = directory / ("oracle" + optimization[1:])
            subprocess.run([compiler, "-std=c++11", "-Wno-unknown-pragmas", optimization,
                            str(source), "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True)
    print("PASS: 262,160 direct callback checks and 864 dispatcher cases at -O0/-O2; five negative controls rejected")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[2])
    run(parser.parse_args().source)
