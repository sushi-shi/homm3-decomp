#!/usr/bin/env python3
"""Use the canonical host query and DC's typed filter-message local.

Complete's RS_HEADERS_REQUEST guard has precisely the retained IsHost
semantics (offline is host; online calls DPlay's virtual IsHost). The adjacent
hero-face handler already uses that ordinary query. No new helper is created.
DC 6561/6562 names a CSetFilterMsg* before calling SetFilter. These two
independent source facts make four states; none adds compiler-budget mass.
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
    guard = "    case RS_HEADERS_REQUEST:\n        if (!g_videoPaused || g_dPlay->isHost()) {"
    local = """    case RS_SET_FILTER:
        setFilter(static_cast<CSetFilterMsg*>(netMsg)->m_size);
        break;"""
    manifest = dict(schema=1, source="src/singleselectionwindow.cpp",
        units=["singleselectionwindow"], evidence=__doc__, axes=[
            dict(name="host_query", find=guard, options=[dict(name="flattened_guard_control"),
                dict(name="canonical_host_query", replace="    case RS_HEADERS_REQUEST:\n        if (isHost()) {")]),
            dict(name="filter_message_local", find=local, options=[dict(name="cast_expression_control"),
                dict(name="dc_typed_local", replace="""    case RS_SET_FILTER: {
        CSetFilterMsg* msg = static_cast<CSetFilterMsg*>(netMsg);
        setFilter(msg->m_size);
        break;
    }""")]),
        ])
    args.output.write_text(json.dumps(manifest, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(f"{args.output}: four source states")


if __name__ == "__main__":
    main()
