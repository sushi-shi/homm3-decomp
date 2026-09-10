"""Native protocol oracle for the actual transfer-owner source.

Checks offline/host permission, first-free ownership, DPID forwarding, and
store-before-Go ordering. This checks behavior, not x86 layout or codegen.
By default check the current source; --source selects a frozen source tree.
An optional manifest checks all eight historical forms against that tree.
"""
import argparse
from pathlib import Path
import runpy
import shutil
import subprocess
import tempfile

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest, render


FIXTURE = r"""
std::vector<int> events;
bool valid;
int g_videoPaused;
struct DPlay {
    bool host;
    unsigned char isHost() { events.push_back(1); return host; }
} dplay;
DPlay* g_dPlay = &dplay;
struct CNetMsg { unsigned long m_dpidFrom; };
struct CNewPlayerUpdateMan;
CNewPlayerUpdateMan* activeManager;
int expectedSlot;
struct CNewPlayerUpdateProc {
    unsigned long dpid;
    CNewPlayerUpdateProc() : dpid(0) {}
    virtual ~CNewPlayerUpdateProc() {}
    virtual void go();
};
struct t_map_list_update : CNewPlayerUpdateProc {
    explicit t_map_list_update(unsigned long id) {
        events.push_back(3); valid &= id == 91; dpid = id;
    }
};
struct CNewPlayerUpdateMan {
    CNewPlayerUpdateProc* m_procs[8];
    int getFirstAvailable() {
        events.push_back(2);
        for (int i = 0; i < 8; ++i) if (!m_procs[i]) return i;
        return -1;
    }
    void requestMapHeaders(unsigned long);
};
void CNewPlayerUpdateProc::go() {
    events.push_back(4);
    valid &= dpid == 91 && expectedSlot >= 0 && expectedSlot < 8;
    if (expectedSlot >= 0 && expectedSlot < 8)
        valid &= activeManager->m_procs[expectedSlot] == this;
}
struct TSingleSelectionWindow {
    CNewPlayerUpdateMan* m_newPlayerUpdateMan;
    unsigned char isHost() {
        if (!g_videoPaused) return 1;
        return g_dPlay->isHost();
    }
    void onHeadersRequestMsg(CNetMsg*);
    void dispatch(CNetMsg*);
};
__BODIES__
bool check() {
    for (int online = 0; online < 2; ++online)
    for (int host = 0; host < 2; ++host)
    for (int firstFree = 0; firstFree <= 8; ++firstFree) {
        CNewPlayerUpdateProc occupied;
        CNewPlayerUpdateMan manager;
        for (int i = 0; i < 8; ++i) manager.m_procs[i] = i < firstFree ? &occupied : 0;
        TSingleSelectionWindow window;
        window.m_newPlayerUpdateMan = &manager;
        activeManager = &manager; expectedSlot = firstFree;
        g_videoPaused = online; dplay.host = host != 0;
        events.clear(); valid = true;
        CNetMsg msg = {91};
        window.dispatch(&msg);
        std::vector<int> expected;
        if (online) expected.push_back(1);
        const bool allowed = !online || host;
        if (allowed) {
            expected.push_back(2);
            if (firstFree < 8) { expected.push_back(3); expected.push_back(4); }
        }
        bool result = valid && events == expected;
        for (int i = 0; i < 8; ++i) {
            if (i < firstFree) result &= manager.m_procs[i] == &occupied;
            else if (i == firstFree && allowed) result &= manager.m_procs[i] != 0;
            else result &= manager.m_procs[i] == 0;
            if (manager.m_procs[i] && manager.m_procs[i] != &occupied)
                delete manager.m_procs[i];
        }
        if (!result) return false;
    }
    return true;
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path, nargs="?")
    parser.add_argument("--source", type=Path, default=HOMM3_DIR)
    args = parser.parse_args()
    if args.manifest:
        _, originals, axes = load_manifest(args.manifest, args.source)
        sources = [render(originals, axes, (choice,))["src/singleselectionwindow.cpp"]
                   for choice in range(len(axes[0].options))]
    else:
        sources = [(args.source / "src/singleselectionwindow.cpp").read_text()]
    extract = runpy.run_path(str(Path(__file__).with_name("test-lobby-map-header.py")))["function"]
    variants = []
    for choice, source in enumerate(sources):
        start = source.index("\n    case RS_HEADERS_REQUEST:\n        ") + 1
        end = source.index("    case RS_GAME_TRANSMIT_INIT: {", start)
        arm = source[start:end].split("\n", 1)[1].rsplit("        break;", 1)[0]
        body = "void TSingleSelectionWindow::dispatch(CNetMsg* netMsg) {\n" + arm + "}\n"
        for signature in ("void TSingleSelectionWindow::onHeadersRequestMsg(",
                          "void CNewPlayerUpdateMan::requestMapHeaders("):
            if signature in source:
                body += extract(source, signature) + "\n"
        variants.append(("source-%d" % choice, body, True))
    correct = variants[-1][1]
    controls = [
        ("missing-go", "        m_procs[index]->go();", ""),
        ("wrong-sender", "new t_map_list_update(dpid)", "new t_map_list_update(dpid + 1)"),
        ("nonhost-start", "if (isHost())", "if (true)"),
        ("skip-first-slot", "if (index != -1)", "if (index != -1 && index != 0)"),
        ("wrong-slot", "m_procs[index]", "m_procs[(index + 1) % 8]"),
    ]
    for name, old, new in controls:
        if old not in correct:
            raise ValueError("Stale negative control: " + name)
        variants.append((name, correct.replace(old, new), False))
    code = ["#include <vector>\n#include <cstdio>"]
    for i, (_, body, _) in enumerate(variants):
        code.append("namespace v%d {\n%s\n}" % (i, FIXTURE.replace("__BODIES__", body)))
    code.append("int main() {")
    for i, (name, _, expected) in enumerate(variants):
        code.append('if (v%d::check() != %s) { std::puts("FAIL: %s"); return 1; }' %
                    (i, "true" if expected else "false", name))
    code.append("return 0; }")
    compiler = shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        raise RuntimeError("Native C++ compiler required")
    with tempfile.TemporaryDirectory(prefix="homm3-transfer-owner-oracle-") as temp:
        path = Path(temp)
        fixture = path / "fixture.cpp"
        fixture.write_text("\n".join(code))
        for optimization in ("-O0", "-O2"):
            executable = path / ("oracle" + optimization[1:])
            subprocess.run([compiler, "-std=c++11", optimization, "-Wno-unknown-pragmas", "-fmax-errors=3",
                            str(fixture), "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True)
    print("PASS: %d ownership cases at -O0/-O2; five negative controls rejected" %
          (36 * len(sources)))


if __name__ == "__main__":
    main()
