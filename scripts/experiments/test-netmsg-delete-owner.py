"""Check actual DeletePlayer forms, canonical lookup/Clear, and record ownership.

By default use current source; an optional manifest checks the frozen family's
four forms. This tests behavior, not x86 inlining or layout.
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
struct CNetPlayerHandlerPlayer {
    unsigned long m_dpid;
    int m_playerPos, m_townIndex, m_heroIndex, untouched;
    __CLEAR__
};
struct CNetPlayerHandler {
    enum {MAX_PLAYERS=8};
    CNetPlayerHandlerPlayer m_humanPlayers[8];
    __LOOKUP__
    bool deletePlayer(unsigned long);
};
__BODY__
bool check() {
    for(int layout=0;layout<4;++layout)
    for(unsigned long requested=0;requested<12;++requested) {
        CNetPlayerHandler handler={};
        CNetPlayerHandlerPlayer expected[8]={};
        for(int i=0;i<8;++i) {
            handler.m_humanPlayers[i].m_dpid=layout==0?i+1:layout==1?i%3:layout==2?0:9-i;
            handler.m_humanPlayers[i].m_playerPos=i;
            handler.m_humanPlayers[i].m_townIndex=i+21;
            handler.m_humanPlayers[i].m_heroIndex=i+101;
            handler.m_humanPlayers[i].untouched=i+333;
            expected[i]=handler.m_humanPlayers[i];
        }
        int first=-1;
        for(int i=7;i>=0;--i)if(expected[i].m_dpid==requested)first=i;
        if(first!=-1) {
            expected[first].m_dpid=0;
            expected[first].m_playerPos=-1;
            expected[first].m_townIndex=-1;
            expected[first].m_heroIndex=-1;
        }
        bool result=handler.deletePlayer(requested);
        if(result!=(first!=-1))return false;
        for(int i=0;i<8;++i) {
            const CNetPlayerHandlerPlayer& a=handler.m_humanPlayers[i];
            const CNetPlayerHandlerPlayer& b=expected[i];
            if(a.m_dpid!=b.m_dpid || a.m_playerPos!=b.m_playerPos ||
               a.m_townIndex!=b.m_townIndex || a.m_heroIndex!=b.m_heroIndex ||
               a.untouched!=b.untouched)return false;
        }
    }
    return true;
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path, nargs="?")
    parser.add_argument("--source", type=Path, default=HOMM3_DIR)
    args = parser.parse_args()
    extract = runpy.run_path(str(Path(__file__).with_name("test-lobby-map-header.py")))["function"]
    if args.manifest:
        _, originals, axes = load_manifest(args.manifest, args.source)
        sources = [render(originals, axes, (i,))["src/singleselectionwindow.cpp"]
                   for i in range(len(axes[0].options))]
    else:
        sources = [(args.source / "src/singleselectionwindow.cpp").read_text()]
    header = (args.source / "include/singleselectionwindow.h").read_text()
    clear = extract(header[header.index("class CNetPlayerHandlerPlayer :"):], "void clear()")
    lookup = extract(header[header.index("class CNetPlayerHandler {"):], "int getNetPos(")
    fixture = FIXTURE.replace("__CLEAR__", clear).replace("__LOOKUP__", lookup)
    variants = [("source-%d" % i, extract(s, "bool CNetPlayerHandler::deletePlayer("), True)
                for i, s in enumerate(sources)]
    selected = variants[-1][1]
    # Use both pointer and direct forms so the oracle works after adoption too.
    index = "netPos" if "int netPos" in selected else "pos"
    controls = [
        ("wrong-dpid", "getNetPos(dpid)", "getNetPos(dpid + 1)"),
        ("wrong-record", "m_humanPlayers["+index+"]", "m_humanPlayers[("+index+" + 1) % 8]"),
        ("missing-clear", "player->clear();" if "player->clear();" in selected else
         "m_humanPlayers["+index+"].clear();", ""),
        ("false-success", "return true;", "return false;"),
        ("true-failure", "return false;", "return true;"),
    ]
    for name, old, new in controls:
        if selected.count(old) != 1:
            raise ValueError("Stale control: " + name)
        variants.append((name, selected.replace(old, new), False))
    code = ["#include <cstdio>"]
    for i, (_, body, _) in enumerate(variants):
        code.append("namespace v%d {\n%s\n}" % (i, fixture.replace("__BODY__", body)))
    code.append("int main() {")
    for i, (name, _, expected) in enumerate(variants):
        code.append('if(v%d::check()!=%s) {std::puts("FAIL: %s");return 1;}' %
                    (i, "true" if expected else "false", name))
    code.append("return 0;}")
    compiler = shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        raise RuntimeError("Native C++ compiler required")
    with tempfile.TemporaryDirectory(prefix="homm3-delete-owner-oracle-") as temporary:
        directory = Path(temporary)
        cpp = directory / "fixture.cpp"
        cpp.write_text("\n".join(code))
        for optimization in ("-O0", "-O2"):
            executable = directory / ("oracle" + optimization[1:])
            subprocess.run([compiler, "-std=c++11", optimization, "-fmax-errors=3",
                            str(cpp), "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True)
    print("PASS: %d lookup/record-ownership cases at -O0/-O2; five negative controls rejected" %
          (48 * len(sources)))


if __name__ == "__main__":
    main()
