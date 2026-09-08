#!/usr/bin/env python3
"""Generate 60 joint scope/lifetime hypotheses for four near-exact RMG writers.

Retail artifact/resource/scholar/shrine writers (0x533500, 0x5335c0,
0x533e70, 0x533f40) expand the same canonical type_object::write and retain
all subsequent virtual writes. Their trailing byte buffers reuse [ebp+0xb],
and dword buffers reuse [ebp+8]; the current function-scoped locals occupy
separate negative slots. Preserve every write, width, constant and base call.
Cross actual buffer scopes, byte signedness, initialization and independent
versus reused dword buffers. No pasted base body, dummy work or inlining pin.
RMG has no Dreamcast counterpart; these are retail-supported source hypotheses.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source, hypotheses

RECORDS = {
    "rmgArtifactObject": (("byte", "hasCustomTreasure", 0, 1),),
    "rmgResourceObject": (("byte", "hasCustomTreasure", 0, 1),
                          ("word", "amount", 0, 4), ("word", "reserved", 0, 4)),
    "rmgScholarObject": (("byte", "rewardKind", -1, 1), ("byte", "rewardValue", 0, 1),
                         ("word", "reserved", 0, 4), ("word", "reserved", 0, 2)),
    "rmgShrineObject": (("byte", "spell", -1, 1), ("word", "reserved", 0, 2),
                        ("byte", "reservedByte", 0, 1)),
}


def symbol(owner):
    return "?write@" + owner + "@@UAEXPAVTAbstractFile@@H@Z"


def groups(records, scope):
    if scope in ("flat", "tail"):
        return [records]
    if scope == "each":
        return [(row,) for row in records]
    result = []
    for row in records:
        if result and result[-1][-1][0] == row[0] and (scope == "consecutive" or row[0] == "word"):
            result[-1].append(row)
        else:
            result.append([row])
    return result


def body(owner, scope, byte_type, initialization, words):
    lines = ["    type_object::write(outfile, parameter);"]
    for group in groups(RECORDS[owner], scope):
        scoped = scope != "flat"
        if scoped:
            lines.append("    {")
        indent = "        " if scoped else "    "
        declared = set()
        for kind, role, value, width in group:
            name = "intBuffer" if kind == "word" and words == "shared" else role
            ctype = byte_type if kind == "byte" else "int"
            if name in declared:
                lines.append(indent + name + " = " + str(value) + ";")
            elif initialization == "assigned":
                lines += [indent + ctype + " " + name + ";", indent + name + " = " + str(value) + ";"]
            else:
                lines.append(indent + ctype + " " + name + " = " + str(value) + ";")
            declared.add(name)
            size = "sizeof(short)" if width == 2 else "sizeof(" + name + ")"
            lines.append(indent + "outfile->write(&" + name + ", " + size + ");")
        if scoped:
            lines.append("    }")
    return ("void " + owner + "::write(TAbstractFile* outfile, int parameter)\n{\n"
            + "\n".join(lines) + "\n}")


def bundles():
    for scope, byte_type, initialization, words in itertools.product(
            ("flat", "tail", "each", "consecutive", "split_bytes"),
            ("char", "signed char", "unsigned char"), ("initialized", "assigned"),
            ("named", "shared")):
        label = "+".join((scope, byte_type.replace(" ", "_"), initialization, words))
        yield label, {owner: body(owner, scope, byte_type, initialization, words) for owner in RECORDS}


def definitions(source):
    result = {}
    for owner in RECORDS:
        found = _source.find_definitions(source, symbol(owner))
        if len(found) != 1:
            raise ValueError("review the unique serializer " + owner)
        item = found[0]
        start = source.rfind("\n", 0, item.head) + 1
        result[owner] = source[start:item.body_close + 1]
    return result


def make_manifest(source):
    originals = definitions(source)
    options = list(bundles())
    if not any(candidate == originals for _, candidate in options):
        raise ValueError("review the four object writers before rebasing the family")
    options.sort(key=lambda row: row[1] != originals)
    owner, *others = RECORDS
    return dict(schema=1, unit="rmg", function=symbol(owner), evidence=__doc__, axes=[dict(
        name="object_writer_buffers", find=originals[owner], options=[dict(
            name=label, replace=candidate[owner], extra_edits=[dict(
                find=originals[other], replace=candidate[other]) for other in others])
            for label, candidate in options])])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = make_manifest((HOMM3_DIR / "src/rmg.cpp").read_text())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    parsed = hypotheses.parse_manifest(args.output)
    print("generated", len(hypotheses.variants(parsed[4], parsed[5])), "unique source hypotheses ->", args.output)


if __name__ == "__main__":
    main()
