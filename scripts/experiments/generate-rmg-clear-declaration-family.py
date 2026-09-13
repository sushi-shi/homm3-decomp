#!/usr/bin/env python3
"""Separate map-cell snapshot declarations from their actual member reads.

Retail 0x530f10 uses EDI in the vector-copy loop, then loads the connection
word into the same register. Reading connection before erase preserves its
later allocation but needs an extra register in that loop. A real snapshot
can instead be declared before erase and assigned afterward. Compare the
independent declaration/read order and the public erase/clear APIs; retain
all named field updates and the existing store sequence.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def previous():
    spec = importlib.util.spec_from_file_location(
        "rmg_clear_lifetimes", Path(__file__).with_name("generate-rmg-clear-lifetime-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def variants(original):
    prior = previous()
    begin = original.index("\n{\n") + 3
    tail = original[original.index("    connection.m_present = 0;", begin):]
    calls = list(prior.CALLS[:2])
    calls += [
        ("reference_erase", "    std::vector<type_object*>& objects = m_objects;\n"
         "    objects.erase(objects.begin(), objects.end());\n"),
        ("reference_clear", "    std::vector<type_object*>& objects = m_objects;\n"
         "    objects.clear();\n"),
    ]
    for staged, declarations, reads, (api, call) in itertools.product(
            range(8), itertools.permutations(range(3)), itertools.permutations(range(3)), calls):
        lines = [original[:begin]]
        for index in declarations:
            if staged & (1 << index):
                kind, local, _ = prior.SNAPSHOTS[index]
                lines.append(f"    {kind} {local};\n")
        lines.append(call)
        for index in reads:
            kind, local, member = prior.SNAPSHOTS[index]
            declaration = "" if staged & (1 << index) else kind + " "
            lines.append(f"    {declaration}{local} = {member};\n")
        lines += ["\n", tail]
        label = "+".join((str(staged), "".join(map(str, declarations)),
                          "".join(map(str, reads)), api))
        yield label, "".join(lines)


def make_axes(source):
    helper = previous().helpers()
    original = helper.definition(source, "TRmgMapItem::clear")
    return [helper.axis("clear_declarations", "src/rmg.cpp", original, variants(original))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / "src/rmg.cpp").read_text())
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes, evidence=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
