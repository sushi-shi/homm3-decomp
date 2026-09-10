"""Reduced native behavior checks for the actual lobby player helpers/arm.

Mocks observe query freshness, short-circuiting, drop ordering and transfer
fallthrough; they do not certify ABI or retail inlining. Wrong controls must
fail at both optimization levels.
"""

import argparse
from pathlib import Path
import runpy
import shutil
import subprocess
import tempfile


FIXTURE = r"""
std::vector<int> events;
bool valid, transferResult, present;
int queries;
struct CNetPlayerInfo {const char* m_name;};
struct CNetPlayerHandlerPlayer:CNetPlayerInfo {int m_playerPos;};
struct CNetMsg {int m_dpidFrom;};
const int WINDOW_MODE_6989F0_3=3;
int g_unnamed6989f0;
struct {int m_dpid;} g_thisNetPlayerInfo;
struct Players {
    CNetPlayerHandlerPlayer m_humanPlayers[2];
    CNetPlayerHandlerPlayer* getPlayer(int id) {
        events.push_back(1);valid &= id==91;++queries;
        return present?&m_humanPlayers[queries==1?0:1]:0;
    }
    void deletePlayer(int id) {events.push_back(2);valid &= id==91;m_humanPlayers[0].m_playerPos=-1;}
};
struct Manager {void playerDropped(int id) {events.push_back(4);valid &= id==91;}} manager;
struct Text {
    const char* getText(int id) {events.push_back(id);return "text";}
    const char* operator[](int id) {return getText(id);}
} text;
Text* g_generalText=&text;
struct Chat {void playerDropMsg(const char* message,const char* name);} g_chatMan;
void Chat::playerDropMsg(const char* message,const char* name) {
    events.push_back(5);valid &= this==&g_chatMan && std::strcmp(message,"text")==0 && std::strcmp(name,"Alice")==0;
}
// Historical frozen source used an explicit receiver. Keep that test adapter
// so --source can still validate the original controls after interface recovery.
void playerDropMsg(Chat* chat,const char* message,const char* name) {
    chat->playerDropMsg(message,name);
}
void destroyMsg(CNetMsg* msg) {events.push_back(10);valid &= msg->m_dpidFrom==91;}
void normalDialog(const char*,int kind,int x,int y,int a,int b,int c,int d,int e,int f,int g,int h) {
    events.push_back(12);valid &= kind==1 && x==-1 && y==-1 && a==-1 && !b && c==-1 && !d && e==-1 && !f && g==-1 && !h;
}
struct TSingleSelectionWindow {
    Players m_players;
    Manager* m_newPlayerUpdateMan;
    int m_commonGameVersion,m_receivedMaps;
    CNetPlayerHandlerPlayer* getThisPlayer();
    bool onPlayerDroppedMsg(CNetMsg*);
    int transfer(CNetMsg*,bool&);
    int getCommonGameVersion() {events.push_back(3);valid &= m_players.m_humanPlayers[0].m_playerPos==-1;return 42;}
    void updateNameLists() {events.push_back(6);valid &= m_commonGameVersion==42;}
    void displayChat() {events.push_back(7);}
    void drawWindow(int a,unsigned b,unsigned c) {events.push_back(8);valid &= !a && b==0xffff0001 && c==0xffff;}
    void update() {events.push_back(9);}
    void remoteCleanup() {events.push_back(11);}
    bool onGameTransmitInitMsg(CNetMsg* msg) {events.push_back(13);valid &= msg->m_dpidFrom==91 && m_receivedMaps==1;return transferResult;}
};
__BODIES__
bool check() {
    for(int local=0;local<2;++local) for(int exists=0;exists<2;++exists)
    for(int first=-1;first<2;++first) for(int second=-1;second<2;++second)
    for(int succeeds=0;succeeds<2;++succeeds) for(int oldCancel=0;oldCancel<2;++oldCancel) {
        TSingleSelectionWindow window={};CNetMsg msg={91};
        window.m_newPlayerUpdateMan=&manager;
        window.m_players.m_humanPlayers[0].m_playerPos=first;
        window.m_players.m_humanPlayers[1].m_playerPos=second;
        g_unnamed6989f0=local?3:1;g_thisNetPlayerInfo.m_dpid=91;
        present=exists!=0;transferResult=succeeds!=0;valid=true;queries=0;events.clear();
        bool cancel=oldCancel!=0;
        const bool rejected=local?(first==-1):(exists && second==-1);
        std::vector<int> expected;
        if(!local) {expected.push_back(1);if(exists)expected.push_back(1);}
        if(rejected) {expected.push_back(10);expected.push_back(11);expected.push_back(525);expected.push_back(12);}
        else {expected.push_back(13);if(succeeds)expected.push_back(10);}
        const int result=window.transfer(&msg,cancel);
        if(!valid || events!=expected || result!=(rejected||succeeds?1:0) ||
           cancel!=(rejected||oldCancel) || window.m_receivedMaps!=1)return false;
    }
    for(int exists=0;exists<2;++exists) {
        TSingleSelectionWindow window={};CNetMsg msg={91};window.m_newPlayerUpdateMan=&manager;
        window.m_players.m_humanPlayers[0].m_name="Alice";
        window.m_players.m_humanPlayers[0].m_playerPos=7;
        present=exists!=0;valid=true;queries=0;events.clear();
        const bool result=window.onPlayerDroppedMsg(&msg);
        std::vector<int> expected={1,2,3,4};
        if(exists) {expected.push_back(527);expected.push_back(5);}
        for(int event=6;event<=9;++event)expected.push_back(event);
        if(!result || !valid || events!=expected || window.m_commonGameVersion!=42)return false;
    }
    return true;
}
"""


def run(root):
    extract = runpy.run_path(str(Path(__file__).with_name("test-lobby-map-header.py")))["function"]
    source = (root / "src/singleselectionwindow.cpp").read_text()
    get = extract(source, "CNetPlayerHandlerPlayer* TSingleSelectionWindow::getThisPlayer()")
    drop = extract(source, "bool TSingleSelectionWindow::onPlayerDroppedMsg(CNetMsg* netMsg)")
    arm = source.split("    case RS_GAME_TRANSMIT_INIT: {", 1)[1].split(
        "        // fall through - a failed transfer is a lost session", 1)[0].rstrip()
    transfer = "int TSingleSelectionWindow::transfer(CNetMsg* netMsg,bool& cancel) {" + arm[:-1] + "return 0;\n}"
    body = "\n".join((get, drop, transfer))
    controls = [
        ("cache-player-query", "if (getThisPlayer() && getThisPlayer()->m_playerPos == -1)",
         "CNetPlayerHandlerPlayer* cached=getThisPlayer(); if (cached && cached->m_playerPos == -1)"),
        ("skip-version-store", "m_commonGameVersion = getCommonGameVersion();", "getCommonGameVersion();"),
        ("version-before-delete", "m_players.deletePlayer(netMsg->m_dpidFrom);\n    m_commonGameVersion = getCommonGameVersion();",
         "m_commonGameVersion = getCommonGameVersion();\n    m_players.deletePlayer(netMsg->m_dpidFrom);"),
        ("skip-manager-drop", "m_newPlayerUpdateMan->playerDropped(netMsg->m_dpidFrom);", ""),
        ("skip-cancel", "cancel = true;", ""),
        ("wrong-helper-result", "return true;", "return false;"),
    ]
    forms = [("actual-source", body, True)]
    for name, before, after in controls:
        if body.count(before)!=1:
            raise ValueError("Stale control: " + name)
        forms.append((name, body.replace(before, after), False))
    rendered = ["#include <vector>\n#include <cstring>\n#include <cstdio>"]
    for index, (_, variant, _) in enumerate(forms):
        rendered.append("namespace variant%d {\n%s\n}" % (index,FIXTURE.replace("__BODIES__",variant)))
    rendered.append("int main() {")
    for index, (name, _, succeeds) in enumerate(forms):
        rendered.append('if(variant%d::check()!=%s) {std::puts("FAIL: %s");return 1;}' %
                        (index,"true" if succeeds else "false",name))
    rendered.append("return 0;}")
    compiler=shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        raise RuntimeError("Native C++ compiler required")
    with tempfile.TemporaryDirectory(prefix="homm3-player-helper-oracle-") as temporary:
        directory=Path(temporary)
        fixture=directory/"fixture.cpp"
        fixture.write_text("\n".join(rendered))
        for optimization in ("-O0","-O2"):
            executable=directory/("oracle"+optimization[1:])
            subprocess.run([compiler,"-std=c++11",optimization,str(fixture),"-o",str(executable)],check=True)
            subprocess.run([str(executable)],check=True)
    print("PASS: 146 player/transfer cases at -O0/-O2; six negative controls rejected")


if __name__ == "__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source",type=Path,default=Path(__file__).resolve().parents[2])
    run(parser.parse_args().source)
