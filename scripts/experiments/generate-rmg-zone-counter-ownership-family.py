#!/usr/bin/env python3
"""Recover the shared zone index lifetime proven by retail stack reuse.

The shared-query parents have the correct cleanup sequence at retail offsets.
Retail reuses ebp-0x14 for initial, radial and final traversals; the candidate
radial for-initializer shadows the original index in a new ebp-0x54 home.
Test reuse of the original index against that shadow, across all four
reproduced shared-query parents. Eight genuine states include the unchanged
control. No padding, new helper or dummy operation is introduced.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

previous = generator("generate-rmg-zone-query-construction-family.py")
parent = previous.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    retained = list(previous.parents(args.checkpoint, "zone_shared_query", 4))
    original = parent.definition((HOMM3_DIR / parent.SOURCE).read_text())
    retained.sort(key=lambda item: item[1] != original)
    if retained[0][1] != original:
        raise ValueError("unchanged parent required")
    options = []
    for row, body in retained:
        reused = parent.replace(body,
            "        for (int zone = 0; zone < originalZones; ++zone) {",
            "        for (zone = 0; zone < originalZones; ++zone) {")
        options += [dict(name=row["id"] + "+shadow", replace=body),
                    dict(name=row["id"] + "+shared_index", replace=reused)]
    if len({row["replace"] for row in options}) != 8:
        raise ValueError("expected eight distinct index ownership states")
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__,
        parent_checkpoint=str(args.checkpoint.resolve()),
        reproduced_parents=[dict(id=r["id"], object_hash=r["object_hash"]) for r, _ in retained],
        axes=[dict(name="zone_counter_ownership", source=parent.SOURCE,
                   find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("8 zone-index ownership states including unchanged control")


if __name__ == "__main__":
    main()
