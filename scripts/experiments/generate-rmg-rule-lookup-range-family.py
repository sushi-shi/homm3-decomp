#!/usr/bin/env python3
"""Keep the retail short-circuit guard and construct only the consumed lookup.

Persistent reverse cursors recover insertion/helper boundaries but load end
eagerly; outer guards alter the null-size CFG. Retail decrements the shared
match index before deriving subtype begin+match. These temporary canonical
iterator accesses occur on the guarded RHS only: no empty-range arithmetic,
new persistent cursor, copied helper implementation or extra size call.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

parent = generator("generate-rmg-rule-ranges-family.py")


def variant(original, mode):
    if mode == 0:
        return original
    group = "subtypesByType[mappedType][terrain]"
    body = original
    if mode == 4:
        anchor = "                int match = rulesByType[mappedType][terrain].size();"
        body = parent.replace(body, anchor, anchor +
            f"\n                const std::vector<int>& subtypeGroup = {group};")
        group = "subtypeGroup"
    expression = {
        1: f"*({group}.begin() + match)",
        2: f"{group}.begin()[match]",
        3: f"*std::vector<int>::reverse_iterator({group}.begin() + match + 1)",
        4: f"*std::vector<int>::const_reverse_iterator({group}.begin() + match + 1)",
        5: f"*({group}.rend() - (match + 1))",
    }[mode]
    return parent.replace(body,
        "match-- && subtypesByType[mappedType][terrain][match] != subtype",
        "match-- && " + expression + " != subtype")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    retained = parent.parents(args.checkpoint)
    options = [dict(name=f"{row['id']}+lookup_range_{mode}", replace=variant(body, mode))
               for row, body in retained for mode in range(6)]
    if len({row["replace"] for row in options}) != 60:
        raise ValueError("expected sixty distinct source states")
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__,
        parent_checkpoint=str(args.checkpoint.resolve()),
        reproduced_parents=[dict(id=row["id"], choices=row["choices"], object_hash=row["object_hash"])
                            for row, _ in retained], axes=[dict(name="rule_lookup_range", source=parent.SOURCE,
        find=parent.definition((HOMM3_DIR / parent.SOURCE).read_text()), options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("60 coupled short-circuited subtype lookup and rule-lifetime states")


if __name__ == "__main__":
    main()
