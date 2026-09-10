#!/usr/bin/env python3
"""Test a real player-record binding at DeletePlayer's Clear operation.

DC 1126/1128/1129/1131/1133 prove the index query, failed-index early return,
canonical Clear call and true result. Retail's retained body is already exact
and materializes the selected record address before Clear. A Complete-era
pointer/reference binding is a hypothesis, not a recovered DC local: all
forms preserve the same guard, owning record, helper and write order. Score
both the retained DeletePlayer body and every caller, including HandleNetMsg.
No redundant predicate, synthetic operation, new helper or inline fence.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/singleselectionwindow.cpp").read_text()
    start = source.index("bool CNetPlayerHandler::deletePlayer(unsigned long dpid)")
    body = source[start:source.index("\n}", start) + 2]
    call = "    m_humanPlayers[pos].clear();"
    if body.count(call) != 1:
        raise ValueError("Expected the direct selected-record Clear call")
    options = [dict(name="direct_record_control")]
    for name, declaration, invoke in (
        ("record_pointer", "CNetPlayerHandlerPlayer* player = &m_humanPlayers[pos];", "player->clear();"),
        ("record_reference", "CNetPlayerHandlerPlayer& player = m_humanPlayers[pos];", "player.clear();"),
        ("fixed_record_pointer", "CNetPlayerHandlerPlayer* const player = &m_humanPlayers[pos];", "player->clear();"),
    ):
        options.append(dict(name=name, replace=body.replace(call, "    " + declaration + "\n    " + invoke)))
    manifest = dict(schema=1, source="src/singleselectionwindow.cpp",
        units=["singleselectionwindow"], evidence=__doc__, axes=[
            dict(name="selected_player_record", find=body, options=options)])
    args.output.write_text(json.dumps(manifest, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(f"{args.output}: four source states")


if __name__ == "__main__":
    main()
