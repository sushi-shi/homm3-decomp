"""Exercise actual chat member bodies, ring ownership, varargs and sound order.

The reduced native fixture is not an x86 ABI test; the separate VC6 probe and
retail byte comparisons cover that. --source can select a frozen source tree.
"""
import argparse
from pathlib import Path
import runpy
import shutil
import subprocess
import tempfile


FIXTURE = r"""
#define DATA_COMPGEN(address, name, literal) literal
const int GENERAL_TEXT_TURN_DURATION_PREFIX = 54, AIL_SAMPLE_PLAYING = 2;
struct sample { int id; } chatSample={31}, dropSample={32};
struct ds_memsample {} oldHandle, newHandle;
std::vector<int> events;
bool valid, playing;
struct SoundManager {
    int m_playSounds;
    bool getSampleInfo(ds_memsample* handle, int selector) {
        valid &= handle == &oldHandle && selector == AIL_SAMPLE_PLAYING;
        events.push_back(17); return playing;
    }
    ds_memsample* memorySample(sample* value) {
        valid &= m_playSounds == 1;
        events.push_back(value->id); return &newHandle;
    }
} sound;
SoundManager* g_soundManager=&sound;
struct Text {
    const char* getText(int id) {valid &= id == 54; return "[sys] ";}
} text;
Text* g_generalText=&text;
struct CChatManager {
    struct CChatStr {char m_text[128]; unsigned long m_killTime; unsigned char m_isSystem;};
    CChatStr* m_msgArray;
    int m_currMsg, m_msgCount, m_position, m_maxLines;
    unsigned char m_changed, m_isSysMsg;
    ds_memsample* m_chatMemSample;
    sample* m_chatSample;
    sample* m_playerDropSample;
    int getNextFreeMsgNbr();
    void addChat(const char*, ...);
    void playerDropMsg(const char*, ...);
};
__BODIES__
bool check() {
    const int counts[]={0,1,19,20};
    const char* payloads[]={"", "Alice", "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ"};
    for(int countIndex=0;countIndex<4;++countIndex)
    for(int maximum=20;maximum<=24;maximum+=4)
    for(int nearEnd=0;nearEnd<2;++nearEnd)
    for(int newest=0;newest<2;++newest)
    for(int payload=0;payload<3;++payload)
    for(int drop=0;drop<2;++drop)
    for(int system=0;system<2;++system)
    for(int live=0;live<2;++live)
    for(int busy=0;busy<2;++busy)
    for(int haveChat=0;haveChat<2;++haveChat)
    for(int haveDrop=0;haveDrop<2;++haveDrop)
    for(int enabled=0;enabled<2;++enabled) {
        CChatManager::CChatStr records[24]={}, expected[24]={};
        for(int i=0;i<24;++i) {
            std::sprintf(records[i].m_text,"old-%d",i);
            records[i].m_killTime=100+i; records[i].m_isSystem=i%2;
        }
        std::memcpy(expected,records,sizeof(records));
        CChatManager manager={}; manager.m_msgArray=records;
        manager.m_currMsg=nearEnd?maximum-1:0;
        manager.m_msgCount=counts[countIndex]; manager.m_maxLines=maximum;
        manager.m_position=manager.m_msgCount-(newest?1:2);
        manager.m_isSysMsg=system; manager.m_chatMemSample=live?&oldHandle:0;
        manager.m_chatSample=haveChat?&chatSample:0;
        manager.m_playerDropSample=haveDrop?&dropSample:0;
        int expectedCurrent=manager.m_currMsg, expectedCount=manager.m_msgCount;
        if(expectedCount==20) {
            expected[expectedCurrent].m_killTime=0;
            expectedCurrent=(expectedCurrent+1)%maximum; --expectedCount;
        }
        int slot=(expectedCurrent+expectedCount)%maximum;
        std::string message=(drop?"[sys] ":"");
        message+=payloads[payload]; message+=":9";
        for(int i=0;i<127;++i)
            expected[slot].m_text[i]=i<int(message.size())?message[i]:0;
        expected[slot].m_killTime=0; expected[slot].m_isSystem=drop?1:system;
        ++expectedCount;
        int expectedPosition=newest?expectedCount-1:manager.m_position;
        std::vector<int> expectedEvents;
        ds_memsample* expectedHandle=manager.m_chatMemSample;
        if(drop || !system) {
            if(live)expectedEvents.push_back(17);
            if(!(live&&busy)) {
                int chosen=drop&&haveDrop?32:(haveChat?31:0);
                if(chosen) {expectedEvents.push_back(chosen);expectedHandle=&newHandle;}
            }
        }
        valid=true; playing=busy!=0; events.clear(); sound.m_playSounds=enabled;
        if(drop)manager.playerDropMsg("%s:%d",payloads[payload],9);
        else manager.addChat("%s:%d",payloads[payload],9);
        if(!valid || std::memcmp(records,expected,sizeof(records)) ||
           manager.m_currMsg!=expectedCurrent || manager.m_msgCount!=expectedCount ||
           manager.m_position!=expectedPosition || manager.m_changed!=1 ||
           manager.m_isSysMsg!=(drop?0:system) || events!=expectedEvents ||
           manager.m_chatMemSample!=expectedHandle || sound.m_playSounds!=enabled)
            return false;
    }
    return true;
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    source = (args.source / "src/remote.cpp").read_text()
    extract = runpy.run_path(str(Path(__file__).with_name("test-lobby-map-header.py")))["function"]
    bodies = [extract(source, signature) for signature in (
        "void CChatManager::addChat(const char* format, ...)",
        "void CChatManager::playerDropMsg(const char* format, ...)",
        "inline int CChatManager::getNextFreeMsgNbr()",
    )]
    actual = "\n".join(bodies)
    variants = [("actual-source", actual, True)]
    controls = [
        ("drop-not-system", "    m_isSysMsg = 1;", "    m_isSysMsg = 0;"),
        ("system-state-not-cleared", "    m_isSysMsg = 0;", "    m_isSysMsg = 1;"),
        ("overflow-count", "        --m_msgCount;", ""),
        ("wrong-ring-slot", "return (m_currMsg + m_msgCount) % m_maxLines;", "return m_currMsg;"),
        ("wrong-drop-sample", "sample* sampleToPlay = m_playerDropSample;", "sample* sampleToPlay = m_chatSample;"),
        ("short-truncation", "chatText, 127);", "chatText, 126);"),
    ]
    for name, old, new in controls:
        if actual.count(old) != 1:
            raise ValueError("Stale control: " + name)
        variants.append((name, actual.replace(old, new), False))
    code = ["#include <cstdarg>\n#include <cstdio>\n#include <cstring>\n#include <string>\n#include <vector>"]
    for index, (_, body, _) in enumerate(variants):
        code.append("namespace v%d {\n%s\n}" % (index, FIXTURE.replace("__BODIES__", body)))
    code.append("int main() {")
    for index, (name, _, expected) in enumerate(variants):
        code.append('if(v%d::check()!=%s) {std::puts("FAIL: %s");return 1;}' %
                    (index, "true" if expected else "false", name))
    code.append("return 0;}")
    compiler = shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        raise RuntimeError("Native C++ compiler required")
    with tempfile.TemporaryDirectory(prefix="homm3-chat-member-oracle-") as temporary:
        directory = Path(temporary)
        fixture = directory / "fixture.cpp"
        fixture.write_text("\n".join(code))
        for optimization in ("-O0", "-O2"):
            executable = directory / ("oracle" + optimization[1:])
            subprocess.run([compiler, "-std=c++11", optimization, "-fmax-errors=3",
                            str(fixture), "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True)
    print("PASS: 12,288 chat/ring/sound cases at -O0/-O2; six negative controls rejected")


if __name__ == "__main__":
    main()
