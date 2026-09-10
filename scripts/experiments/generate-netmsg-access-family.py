#!/usr/bin/env python3
"""Recover manager access and canonical text-resource calls in the lobby.

DC GetFirstAvailable/GetProc publics are protected (IA...), and their only
active callers are manager members. DC HandleNetMsg and OnPlayerDroppedMsg
call TTextResource::operator[], whose retained body delegates to GetText.
Cross these source facts with removal of the three existing network fences.
The 64 states add no helper, declaration, assertion or compiler-budget mass.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

CPP = "src/singleselectionwindow.cpp"
PRIV = "include/singleselectionwindow_priv.h"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("parent", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    checkpoint = json.loads(args.parent.read_text())
    if len(checkpoint["records"]) != 8:
        raise ValueError("Exhaust the eight-state transfer-owner parent first")
    selected = next(row for row in checkpoint["elites"] if row["choices"] == [5])
    tree = args.parent.parent / "candidates" / selected["id"] / "repeat/tree"
    for relative in (CPP, PRIV, "include/singleselectionwindow.h"):
        if (HOMM3_DIR / relative).read_bytes() != (tree / relative).read_bytes():
            raise ValueError("Authored source differs from reproduced parent: " + relative)
    source = (HOMM3_DIR / CPP).read_text()

    def body(signature):
        start = source.rindex(signature)
        return source[start:source.index("\n}", start) + 2]

    def text_edits(text, indices):
        edits = []
        for index in indices:
            old = "g_generalText->getText(" + str(index) + ")"
            if text.count(old) != 1:
                raise ValueError("Expected one text lookup: " + old)
            # Include its whole source line to avoid other function sites.
            line = next(line for line in text.splitlines() if old in line)
            edits.append(dict(find=line, replace=line.replace(old, "(*g_generalText)[" + str(index) + "]")))
        return edits

    handler = body("bool TSingleSelectionWindow::handleNetMsg(")
    # Whole-handler axis keeps its five text calls atomic and avoids duplicate
    # lines elsewhere in the TU. Fence axes use disjoint edits in this option.
    options = []
    for mask in range(16):
        changed = handler
        if mask & 1:
            for edit in text_edits(handler, (525, 329, 655, 731, 534)):
                changed = changed.replace(edit["find"], edit["replace"])
        for bit, call in ((2, "onBadVersionMsg(netMsg);"),
                          (4, "onPingMsg(netMsg);"),
                          (8, "onPingResponseMsg(netMsg, 0);")):
            if mask & bit:
                old = "#pragma inline_depth(0)\n        " + call + "\n#pragma inline_depth()"
                if changed.count(old) != 1:
                    raise ValueError("Missing existing fence: " + call)
                changed = changed.replace(old, "        " + call)
        option = dict(name="existing_control" if not mask else "text_and_fence_mask_" + str(mask))
        if mask:
            option["replace"] = changed
        options.append(option)
    drop = body("bool TSingleSelectionWindow::onPlayerDroppedMsg(")
    drop_edit = text_edits(drop, (527,))[0]
    protected = "    // DC GetFirstAvailable; HandleNetMsg's transfer-start arm expands it."
    resume = "    // Before normalization (function): CNewPlayerUpdateMan::Tick."
    manifest = dict(schema=1, source=CPP, units=["singleselectionwindow", "advmgr", "scenarioinfo", "kb"],
        parent_context=args.parent.parent.name, parent_object=selected["object_hash"],
        evidence=__doc__, axes=[
            dict(name="dispatcher_text_and_existing_fences", find=handler, options=options),
            dict(name="drop_text_api", find=drop, options=[dict(name="direct_getter_control"),
                dict(name="dc_subscript", replace=drop.replace(drop_edit["find"], drop_edit["replace"]))]),
            dict(name="manager_access", source=PRIV, find=protected, options=[dict(name="public_control"),
                dict(name="dc_protected", replace="protected:\n" + protected,
                    extra_edits=[dict(source=PRIV, find=resume, replace="public:\n" + resume)])]),
        ])
    args.output.write_text(json.dumps(manifest, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(f"{args.output}: 64 source states")


if __name__ == "__main__":
    main()
