"""Historical pre-adoption family for the DC-proven HeaderRequested boundary.

Run against the ab315284 source, with --adjacent for all 16 states. The
adopted source deliberately fails the old-boundary anchors below; this is
not a command to reintroduce pasted helper bodies or removed declarations.

DC 0x14886c calls GetProc and Proc::HeaderRequested at lines 1493/1496;
the local is CNewPlayerUpdateProc*. The earlier helper at dc 0x148348 owns
request insertion. Complete replaces the old allocated integer/virtual queue
with an eight-byte SHeaderRequest vector (retail 0x5892b0). The family keeps
that retail payload, tests one ordinary definition in the original TU, and
independently removes the existing manager auto_inline override. It adds no
inline qualifier, assertion, alternate declaration, or dummy compiler work.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = "src/singleselectionwindow.cpp"
HEADER = "include/singleselectionwindow_priv.h"
MANAGER = """void CNewPlayerUpdateMan::headerRequested(unsigned long dpid, unsigned char flag, int number)
{
    CNewPlayerUpdateTask* proc = getProc(dpid);
    if (proc) {
        SHeaderRequest req;
        req.m_flag = flag;
        req.m_number = number;
        proc->m_requests.push_back(req);
    }
}"""
RESTORED = """void CNewPlayerUpdateMan::headerRequested(unsigned long dpid, unsigned char flag, int number)
{
    CNewPlayerUpdateProc* proc = getProc(dpid);
    if (proc)
        proc->headerRequested(flag, number);
}"""
HELPER = """// DC HeaderRequested (0x148348), singleselectionwindow.cpp:1346.
// Complete's expanded copy at 0x5892b0 queues flag/number by value instead
// of the older port's allocated integer. This is the same ordinary helper,
// visible in its owning TU; the manager calls it at DC line 1496.
void CNewPlayerUpdateProc::headerRequested(unsigned char flag, int number)
{
    SHeaderRequest req;
    req.m_flag = flag;
    req.m_number = number;
    m_requests.push_back(req);
}

"""
HELPER_ANCHOR = "// DC keeps this source helper out of Tick. Complete VC6 /Ob2 expands its\n"
DECL_ANCHOR = "    // Before normalization (function): CNewPlayerUpdateProc::RequestConfirmation.\n"
DECL = "    // Before normalization: CNewPlayerUpdateProc::HeaderRequested.\n" \
       "    void headerRequested(unsigned char flag, int number);\n"
PIN = "#pragma auto_inline(off)\n// E:\\gamedcs\\singleselectionwindow.cpp:1492\n"
END = "\n#pragma auto_inline(on)\n\n// E:\\gamedcs\\singleselectionwindow.cpp:1500\n"
CONFIRM = """void CNewPlayerUpdateMan::headerConfirmed(unsigned long dpid)
{
    CNewPlayerUpdateTask* proc = getProc(dpid);
    if (proc) {
        proc->m_finished = 1;
        proc->finish();
    }
}"""
CONFIRM_RESTORED = """void CNewPlayerUpdateMan::headerConfirmed(unsigned long dpid)
{
    CNewPlayerUpdateProc* proc = getProc(dpid);
    if (proc)
        proc->headerConfirmed();
}"""
CONFIRM_HELPER = """// DC HeaderConfirmed (0x148384), lines 1360/1362; retail 0x589270
// expands the finished store followed by Complete's virtual Finish call.
void CNewPlayerUpdateProc::headerConfirmed()
{
    m_finished = 1;
    finish();
}

"""
LOOKUP = """    CNewPlayerUpdateProc* getProc(unsigned long dpid)
    {
        for (int i = 0; i < 8; ++i)
            if (m_procs[i] && m_procs[i]->m_dpid == dpid)
                return m_procs[i];
        return 0;
    }"""
LOOKUP_DEFINITION = """// DC GetProc is an ordinary cpp helper at 0x148998, source line 1544.
CNewPlayerUpdateProc* CNewPlayerUpdateMan::getProc(unsigned long dpid)
{
    for (int i = 0; i < 8; ++i) {
        if (m_procs[i] && m_procs[i]->m_dpid == dpid) {
            return m_procs[i];
        }
    }
    return 0;
}

"""
LOOKUP_ANCHOR = "// Bounce the setup ping straight back at its sender, echoing the\n"


def make_manifest(adjacent=False):
    source = (ROOT / SOURCE).read_text()
    header = (ROOT / HEADER).read_text()
    for anchor in (MANAGER, HELPER_ANCHOR, PIN, END):
        if source.count(anchor) != 1:
            raise ValueError("Review changed header-request source anchor: " + anchor)
    if header.count(DECL_ANCHOR) != 1:
        raise ValueError("Review changed header-request declaration")
    manifest = dict(schema=1, source=SOURCE, units=["singleselectionwindow"],
                evidence=__doc__, axes=[
        dict(name="canonical-helper", find=MANAGER, options=[
            dict(name="unchanged"),
            dict(name="ordinary-helper", replace=RESTORED, extra_edits=[
                dict(source=HEADER, insert_before=DECL_ANCHOR, text=DECL),
                dict(source=SOURCE, insert_before=HELPER_ANCHOR, text=HELPER)])]),
        dict(name="manager-inline-policy", find=PIN, options=[
            dict(name="unchanged"),
            dict(name="remove-override", replace=PIN.replace("#pragma auto_inline(off)\n", ""),
                 extra_edits=[dict(source=SOURCE, find=END,
                                  replace=END.replace("\n#pragma auto_inline(on)", ""))])])])
    if adjacent:
        if source.count(CONFIRM) != 1 or header.count(LOOKUP) != 1:
            raise ValueError("Review changed confirmation/lookup boundary")
        options = manifest["axes"][0]["options"]
        for with_request in (False, True):
            options.append(dict(
                name="both-ordinary-helpers" if with_request else "ordinary-confirmation",
                replace=RESTORED if with_request else MANAGER, extra_edits=[
                    dict(source=SOURCE, find=CONFIRM, replace=CONFIRM_RESTORED),
                    dict(source=HEADER, insert_before=DECL_ANCHOR,
                         text=(DECL if with_request else "") + "    void headerConfirmed();\n"),
                    dict(source=SOURCE, insert_before=HELPER_ANCHOR,
                         text=(HELPER if with_request else "") + CONFIRM_HELPER)]))
        manifest["axes"].extend([
            dict(name="lookup-definition", source=HEADER, find=LOOKUP, options=[
                dict(name="unchanged"),
                dict(name="ordinary-cpp-lookup",
                     replace="    CNewPlayerUpdateProc* getProc(unsigned long dpid);",
                     extra_edits=[dict(source=SOURCE, insert_before=LOOKUP_ANCHOR,
                                       text=LOOKUP_DEFINITION)])])])
    return manifest


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--adjacent", action="store_true",
                        help="cross the confirmed DC confirmation/lookup boundaries")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(args.adjacent), indent=2) + "\n")
