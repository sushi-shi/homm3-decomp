"""Check actual lobby wait arms against the independently decoded retail table.

Mock only the wait/recursive-dispatch boundary. Verify subtype-to-title mapping,
sender, cancel reference, recursive message and destructor order at O0/O2.
This is not a wire-format, GUI, or x86 ABI test.
"""
from pathlib import Path
import re
import runpy
import shutil
import subprocess
import tempfile

from homm3.core.common import HOMM3_DIR


FIXTURE = r"""
__ENUMS__
std::vector<int> events;
bool valid, recursiveCancel;
unsigned long expectedSender;
bool* expectedCancel;
struct CNetMsg { unsigned long m_dpidFrom; } reply;
struct Text { int operator[](int n) { return n; } } text;
Text* g_generalText=&text;
struct CHostWaitDlg {
    CNetMsg* m_msg;
    CHostWaitDlg():m_msg(&reply) {events.push_back(1);}
    void wait(unsigned long sender,int title) {
        valid &= sender==expectedSender;
        events.push_back(title);
    }
    ~CHostWaitDlg() {events.push_back(3);}
};
bool handleNetMsg(CNetMsg* msg,bool& cancel) {
    valid &= msg==&reply && &cancel==expectedCancel;
    events.push_back(2);
    cancel=recursiveCancel;
    return true;
}
void dispatch(int subtype,CNetMsg* netMsg,bool& cancel) {
    switch(subtype) {__ARMS__}
    events.push_back(4);
}
bool check() {
    const int subtypes[]={1045,1082};
    const int titles[]={534,731}; // Independently decoded retail destinations.
    for(int arm=0;arm<2;++arm) for(int sender=0;sender<4;++sender)
    for(int initial=0;initial<2;++initial) for(int result=0;result<2;++result) {
        valid=true;events.clear();
        CNetMsg input={static_cast<unsigned long>(sender*97+1)};
        expectedSender=input.m_dpidFrom;
        bool cancel=initial!=0;
        expectedCancel=&cancel;recursiveCancel=result!=0;
        dispatch(subtypes[arm],&input,cancel);
        const int expected[]={1,titles[arm],2,3,4};
        if(!valid || cancel!=recursiveCancel || input.m_dpidFrom!=expectedSender ||
           events!=std::vector<int>(expected,expected+5))return false;
    }
    return true;
}
"""


def main():
    extract = runpy.run_path(str(Path(__file__).with_name("test-lobby-map-header.py")))["function"]
    source = (HOMM3_DIR / "src/singleselectionwindow.cpp").read_text()
    body = extract(source, "bool TSingleSelectionWindow::handleNetMsg(")
    names = ("RS_GAME_TRANSMIT_PENDING", "RS_LAUNCHING_GAME")
    arms = "\n".join(extract(body, "case " + name + ": {") for name in names)
    header = (HOMM3_DIR / "include/netmsg.h").read_text()
    enums = "enum {" + ",".join(
        re.search(r"\b" + name + r"\s*=\s*\d+", header).group()
        for name in names) + "};"
    swapped = arms.replace(names[0], "SWAP_TEMP").replace(names[1], names[0]).replace("SWAP_TEMP", names[1])
    variants = [("actual", arms, True), ("swapped-labels", swapped, False)]
    for label, old, new in (
        ("wrong-sender", "netMsg->m_dpidFrom", "netMsg->m_dpidFrom + 1"),
        ("missing-recursion", "handleNetMsg(dlg.m_msg, cancel);", ""),
        ("wrong-recursive-message", "handleNetMsg(dlg.m_msg, cancel);", "handleNetMsg(netMsg, cancel);"),
    ):
        if arms.count(old) != 2:
            raise ValueError("Stale negative control: " + label)
        variants.append((label, arms.replace(old, new), False))
    code = ["#include <vector>\n#include <cstdio>"]
    for i, (_, candidate, _) in enumerate(variants):
        code.append("namespace v%d {\n%s\n}" %
                    (i, FIXTURE.replace("__ENUMS__", enums).replace("__ARMS__", candidate)))
    code.append("int main() {")
    for i, (name, _, expected) in enumerate(variants):
        code.append('if(v%d::check()!=%s){std::puts("FAIL: %s");return 1;}' %
                    (i, "true" if expected else "false", name))
    code.append("return 0;}")
    compiler = shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        raise RuntimeError("Native C++ compiler required")
    with tempfile.TemporaryDirectory(prefix="homm3-wait-arm-oracle-") as temporary:
        cpp = Path(temporary) / "fixture.cpp"
        cpp.write_text("\n".join(code))
        for optimization in ("-O0", "-O2"):
            executable = Path(temporary) / ("oracle" + optimization[1:])
            subprocess.run([compiler, "-std=c++11", optimization, str(cpp), "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True)
    print("PASS: 32 wait-arm cases at -O0/-O2; four negative controls rejected")


if __name__ == "__main__":
    main()
