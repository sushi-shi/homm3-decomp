#!/usr/bin/env python3
"""Infer the Complete-only map-list transfer boundary from retail call depth.

HandleNetMsg +0x97..+0x11e allocates a t_map_list_update through the same
eight-slot manager protocol as NewPlayer. Retail retains its constructor;
the current direct new-expression needs a depth pin. Test ordinary message
and manager ownership boundaries, separately and together, with the existing
local-result and NewPlayer's store/reload forms. Names of these new helpers
are provisional retail-role names, not recovered Dreamcast names. Every
candidate preserves the protocol and removes, never adds, the old pin.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

CPP = "src/singleselectionwindow.cpp"
HDR = "include/singleselectionwindow.h"
PRIV = "include/singleselectionwindow_priv.h"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / CPP).read_text()
    start = source.index("    case RS_HEADERS_REQUEST:\n        if (isHost()) {")
    end = source.index("    case RS_GAME_TRANSMIT_INIT: {", start)
    original = source[start:end]

    def body(signature, after=0):
        at = source.index(signature, after)
        return source[at:source.index("\n}", at) + 2]

    window_anchor = body("bool TSingleSelectionWindow::onMapHeaderRequestMsg(")
    manager_anchor = body("void CNewPlayerUpdateMan::newPlayer(", source.index("VA(0x0058A280"))
    options = [dict(name="flattened_pinned_control"),
               dict(name="flattened_unpinned_control", replace=original.replace(
                   "#pragma inline_depth(0)\n", "").replace("#pragma inline_depth()\n", ""))]

    for owner in ("message", "manager", "message_and_manager"):
        for local in (True, False):
            extras = []
            manager = owner != "message"
            message = owner != "manager"
            job = ("""    int index = getFirstAvailable();
    if (index != -1) {
""" + ("""        t_map_list_update* proc = new t_map_list_update(dpid);
        m_procs[index] = proc;
        proc->go();
""" if local else """        m_procs[index] = new t_map_list_update(dpid);
        m_procs[index]->go();
""") + "    }\n")
            if manager:
                manager_body = ("\n\n// Provisional Complete-only transfer owner, inferred from HandleNetMsg's\n"
                    "// retained t_map_list_update ctor and the NewPlayer slot protocol.\n"
                    "void CNewPlayerUpdateMan::requestMapHeaders(unsigned long dpid)\n{\n" + job + "}")
                extras += [dict(source=CPP, insert_after=manager_anchor, text=manager_body),
                    dict(source=PRIV, insert_after="    void newPlayer(unsigned long dpid);  // retail 0x58a280",
                         text="\n    // Provisional retail-role name; Complete-only map-list transfer.\n"
                              "    void requestMapHeaders(unsigned long dpid);")]
            if message:
                if manager:
                    content = """    if (isHost())
        m_newPlayerUpdateMan->requestMapHeaders(netMsg->m_dpidFrom);
"""
                else:
                    content = """    if (isHost()) {
        unsigned long dpid = netMsg->m_dpidFrom;
        CNewPlayerUpdateMan* man = m_newPlayerUpdateMan;
        int index = man->getFirstAvailable();
        if (index != -1) {
""" + ("""            t_map_list_update* proc = new t_map_list_update(dpid);
            man->m_procs[index] = proc;
            proc->go();
""" if local else """            man->m_procs[index] = new t_map_list_update(dpid);
            man->m_procs[index]->go();
""") + "        }\n    }\n"
                message_body = ("\n\n// Provisional Complete-only handler boundary: the 1084 arm retains the\n"
                    "// specialized job ctor, like nested calls in the other lobby handlers.\n"
                    "void TSingleSelectionWindow::onHeadersRequestMsg(CNetMsg* netMsg)\n{\n" + content + "}")
                extras += [dict(source=CPP, insert_after=window_anchor, text=message_body),
                    dict(source=HDR, insert_before="    bool onMapHeaderRequestMsg(CNetMsg* netMsg);",
                         text="    // Provisional retail-role name for Complete's 1084 message.\n"
                              "    void onHeadersRequestMsg(CNetMsg* netMsg);\n")]
                replacement = "    case RS_HEADERS_REQUEST:\n        onHeadersRequestMsg(netMsg);\n        break;\n"
            else:
                replacement = """    case RS_HEADERS_REQUEST:
        if (isHost())
            m_newPlayerUpdateMan->requestMapHeaders(netMsg->m_dpidFrom);
        break;
"""
            options.append(dict(name=owner + ("_local_result" if local else "_stored_result"),
                                replace=replacement, extra_edits=extras))
    manifest = dict(schema=1, source=CPP,
                    units=["advmgr", "kb", "scenarioinfo", "singleselectionwindow"],
                    evidence=__doc__, axes=[dict(name="transfer_owner", find=original, options=options)])
    args.output.write_text(json.dumps(manifest, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(f"{args.output}: eight source states")


if __name__ == "__main__":
    main()
