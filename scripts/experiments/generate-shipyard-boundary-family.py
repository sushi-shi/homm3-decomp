"""Combine shipyard pragma deletions with ordinary helper/source boundaries.

The fresh 241-region audit finds four improving removals in mark_shipyards
and a score-flat, emitted-code-changing cell removal in clear_shipyards.
Test every subset of their seven existing fences, not only isolated peaks.

DC 0x10e6e8/0x10e894 establishes static helpers in philai.cpp:833/896.
mark_shipyards' 839-840 resource guard exits before the town/shipyard loops;
cost[7] is its real local. Compare the old force-inline/positive-enclosure
control with ordinary static helpers and that early-return scope. Preserve
their canonical GetTown/HasBuilding/get_cell/cell/size/subscript calls.
No new helper, inline fence, assertion or dummy operation is introduced.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = "src/philai.cpp"
START = "// E:\\gamedcs\\philai.cpp:833\n"
END = "#if 0  // @carcass -- philai body-evidence claims"


def make_manifest():
    source = (ROOT / SOURCE).read_text()
    start = source.index(START)
    original = source[start:source.index(END, start)]
    lines = original.splitlines(keepends=True)
    regions = []
    opened = None
    for i, line in enumerate(lines):
        if line.strip() == "#pragma inline_depth(0)":
            if opened is not None:
                raise ValueError("Nested source override")
            opened = i
        elif line.strip() == "#pragma inline_depth()":
            if opened is None:
                raise ValueError("Unpaired source override")
            regions.append((opened, i))
            opened = None
    if len(regions) != 7 or opened is not None:
        raise ValueError("Review the shipyard override inventory")
    options = []
    for canonical, mask in itertools.product(range(2), range(1 << len(regions))):
        removed = {line for bit, pair in enumerate(regions) if mask & (1 << bit)
                   for line in pair}
        candidate = "".join(line for i, line in enumerate(lines) if i not in removed)
        if canonical:
            if candidate.count("static __forceinline void ") != 2:
                raise ValueError("Review helper declaration boundaries")
            candidate = candidate.replace("static __forceinline void ", "static void ")
            before = """    if (player->m_resources[WOOD] >= 10
        && player->m_resources[GOLD] >= 1000) {
"""
            begin = candidate.index(before)
            end = candidate.index("// E:\\gamedcs\\philai.cpp:896", begin)
            body = candidate[begin + len(before):end]
            suffix = "    }\n}\n\n"
            if not body.endswith(suffix):
                raise ValueError("Review resource guard scope")
            body = body[:-len(suffix)]
            body = "".join(line[4:] if line.startswith("    ") else line
                           for line in body.splitlines(keepends=True))
            candidate = (candidate[:begin] + """    if (player->m_resources[WOOD] < 10
        || player->m_resources[GOLD] < 1000)
        return;

""" + body + "}\n\n" + candidate[end:])
        option = dict(name=f"ordinary-scoped-{canonical}-remove-{mask:07b}")
        if candidate != original:
            option["replace"] = candidate
        options.append(option)
    return dict(schema=1, source=SOURCE, units=["philai"], evidence=__doc__,
                axes=[dict(name="shipyard-helper-boundaries", find=original, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
