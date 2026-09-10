"""Check the actual lobby receive helper, bridge and virtual reader bodies.

The reduced native fixture mocks header parsing and records construction,
transport, SetupOrigData and teardown order, including failed reads and throws.
It tests behavior, not retail ABI, wire-format parsing or inlining. Six wrong
controls must fail at both -O0 and -O2.
"""

import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile


def function(source, signature):
    start = source.index(signature)
    begin = source.index("{", start)
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


FIXTURE = r"""
std::vector<int> events;
bool valid;
int loadResult, throwAtLoad, expectedLength;
struct CNetMsg { int m_from,m_dpidFrom,m_subType; unsigned long m_size; int m_uncompressedSize; };
CNetMsg* expectedMessage;
struct TAbstractFile { char* bytes; int length,position; };
struct t_memory_file:TAbstractFile {
    t_memory_file(char* data,int count) {
        events.push_back(3);bytes=data;length=count;position=0;
        valid &= data==static_cast<char*>(static_cast<void*>(expectedMessage)) && count==expectedLength;
    }
    int read(void* target,unsigned count) {
        events.push_back(4);
        const unsigned available=length-position;
        const unsigned copied=count<available?count:available;
        std::memcpy(target,bytes+position,copied);position+=copied;
        valid &= count==sizeof(CNetMsg);
        return copied;
    }
    ~t_memory_file() {events.push_back(6);}
};
struct NewSMapHeader {
    NewSMapHeader() {events.push_back(2);}
    ~NewSMapHeader() {events.push_back(8);}
    int load(TAbstractFile* file,int version) {
        events.push_back(5);
        valid &= version==42 && file->bytes==static_cast<char*>(static_cast<void*>(expectedMessage));
        valid &= file->position==std::min(expectedLength,static_cast<int>(sizeof(CNetMsg)));
        if(throwAtLoad) throw 19;
        return loadResult;
    }
};
class t_complex_net_message {
public:
    CNetMsg m_netmsg;
    t_complex_net_message():m_netmsg() {events.push_back(1);}
    virtual unsigned char read(TAbstractFile*)=0;
    unsigned char remoteFn00512E00(CNetMsg* netMsg);
};
class CNewMapHeaderInfoMsg:public t_complex_net_message {
public:
    NewSMapHeader m_header;
    CNewMapHeaderInfoMsg() {}
    virtual unsigned char read(TAbstractFile* infile);
};
struct game {void setupOrigData() {events.push_back(7);}};
game gameObject;game* g_game=&gameObject;
class TSingleSelectionWindow {public:bool onNewMapHeaderInfo(CNetMsg* netMsg);};
__BODIES__
bool check() {
    const int lengths[]={0,1,7,static_cast<int>(sizeof(CNetMsg))-1,static_cast<int>(sizeof(CNetMsg))};
    for(int sender=-32;sender<32;++sender) for(int n=0;n<5;++n)
    for(int result=-1;result<2;++result) for(int throwing=0;throwing<2;++throwing) {
        CNetMsg message={sender,sender*7,1028,static_cast<unsigned long>(lengths[n]),sender+55};
        const CNetMsg original=message;
        expectedMessage=&message;expectedLength=lengths[n];
        loadResult=result;throwAtLoad=throwing;valid=true;events.clear();
        TSingleSelectionWindow window;
        bool returned=false,caught=false;
        try {returned=window.onNewMapHeaderInfo(&message);}
        catch(int value) {if(value!=19)return false;caught=true;}
        const int normal[]={1,2,3,4,5,6,7,8},exceptional[]={1,2,3,4,5,6,8};
        const std::vector<int> expected=throwing?std::vector<int>(exceptional,exceptional+7):std::vector<int>(normal,normal+8);
        if(!valid || events!=expected || caught!=(throwing!=0) || returned!=(throwing==0))return false;
        if(message.m_from!=original.m_from || message.m_dpidFrom!=original.m_dpidFrom ||
           message.m_subType!=original.m_subType || message.m_size!=original.m_size ||
           message.m_uncompressedSize!=original.m_uncompressedSize)return false;
    }
    return true;
}
"""


def run(root):
    source = (root / "src/singleselectionwindow.cpp").read_text()
    helper = function(source, "bool TSingleSelectionWindow::onNewMapHeaderInfo(CNetMsg* netMsg)")
    reader = function(source, "unsigned char CNewMapHeaderInfoMsg::read(TAbstractFile* infile)")
    bridge = function((root / "src/netmsg.cpp").read_text(),
                      "unsigned char t_complex_net_message::remoteFn00512E00(CNetMsg* netMsg)")
    forms = [("actual-source", helper, reader, bridge, True),
             ("skip-receive", helper.replace("msg.remoteFn00512E00(netMsg);", ""), reader, bridge, False),
             ("skip-setup", helper.replace("g_game->setupOrigData();", ""), reader, bridge, False),
             ("setup-before-receive", helper.replace("msg.remoteFn00512E00(netMsg);\n    g_game->setupOrigData();",
                                                     "g_game->setupOrigData();\n    msg.remoteFn00512E00(netMsg);"), reader, bridge, False),
             ("reject-read-failure", helper.replace("msg.remoteFn00512E00(netMsg);",
                                                   "if (!msg.remoteFn00512E00(netMsg)) return false;"), reader, bridge, False),
             ("wrong-version", helper, reader.replace("load(infile, 42)", "load(infile, 41)"), bridge, False),
             ("wrong-result", helper.replace("return true;", "return false;"), reader, bridge, False)]
    rendered = ["#include <algorithm>\n#include <cstdio>\n#include <cstring>\n#include <vector>\n"]
    for index, (name, first, second, third, _) in enumerate(forms):
        if index and (first, second, third) == (helper, reader, bridge):
            raise ValueError("Negative control did not change source: " + name)
        rendered.append("namespace variant%d {\n%s\n}" % (index,
            FIXTURE.replace("__BODIES__", "\n".join((first, second, third)))))
    rendered.append("int main() {")
    for index, (name, *_, succeeds) in enumerate(forms):
        rendered.append('if(variant%d::check()!=%s) { std::puts("FAIL: %s"); return 1; }' %
                        (index, "true" if succeeds else "false", name))
    rendered.append("return 0; }")
    compiler = shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        raise RuntimeError("A native C++ compiler is required")
    with tempfile.TemporaryDirectory(prefix="homm3-map-header-oracle-") as temporary:
        directory = Path(temporary)
        source = directory / "fixture.cpp"
        source.write_text("\n".join(rendered))
        for optimization in ("-O0", "-O2"):
            executable = directory / ("oracle" + optimization[1:])
            subprocess.run([compiler, "-std=c++11", optimization,
                            str(source), "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True)
    print("PASS: 1,920 receive/lifetime cases at -O0/-O2; six negative controls rejected")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[2])
    run(parser.parse_args().source)
