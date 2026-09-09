#!/usr/bin/env python3
"""Test size ownership in canConnect (0x532bd0).

No Dreamcast counterpart was found. Verified /Z7 and retail CFG agree on
seven blocks, three branches, and sqrt/_ftol. The first mismatch at +0x4f
assigns otherSize to EBX instead of retail ECX; combinedSize gets ECX rather
than EBX. Preserve the distance prefix and branch-local minimum; test real
slot/scalar bindings and sum lifetimes, without added helper boundaries.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def definition(source):
    signature = "unsigned char TRmgZone::canConnect(const TRmgZone* other) const"
    start = source.index(signature)
    return source[start:source.index("\n}", start) + 2]


def variants(original, snapshots=False):
    prefix = original[:original.index("    int otherSize")]
    for owner, addition, minimum in itertools.product(range(5 if snapshots else 6), range(5), range(2)):
        lines = []
        other, current = "other->m_slot->m_size", "m_slot->m_size"
        if owner == 4 and not snapshots:
            lines += ["const TRmgTownSlot* otherSlot = other->m_slot;",
                      "const TRmgTownSlot* thisSlot = m_slot;"]
            other, current = "otherSlot->m_size", "thisSlot->m_size"
        elif owner == 5 and not snapshots:
            lines += ["const TRmgTownSlot& otherSlot = *other->m_slot;",
                      "const TRmgTownSlot& thisSlot = *m_slot;"]
            other, current = "otherSlot.m_size", "thisSlot.m_size"
        ot = "const int&" if owner in (1, 2) else "int"
        ct = "const int&" if owner in (1, 3) else "int"
        if snapshots:
            ot = "const int" if owner < 2 else "const int&"
            ct = "const int" if owner in (1, 4) else "int"
            if owner >= 2:
                other = f"static_cast<int>({other})"
            if owner == 3:
                ct = "const int&"
                current = f"static_cast<int>({current})"
        lines += [f"{ot} otherSize = {other};", f"{ct} thisSize = {current};"]
        sums = [
            ["int combinedSize = thisSize + otherSize;"],
            ["int combinedSize = otherSize + thisSize;"],
            ["int combinedSize = thisSize;", "combinedSize += otherSize;"],
            ["int combinedSize = otherSize;", "combinedSize += thisSize;"],
            ["int combinedSize;", "combinedSize = thisSize + otherSize;"],
        ]
        lines += sums[addition]
        lines += ["if (other->m_levelPosition.m_z != m_levelPosition.m_z) {",
                  "    if (combinedSize < distance)", "        return 0;"]
        if not minimum:
            lines += ["    int minimumSize = thisSize;", "    if (otherSize < minimumSize)",
                      "        minimumSize = otherSize;"]
        else:
            lines += ["    int minimumSize;", "    if (otherSize < thisSize)",
                      "        minimumSize = otherSize;", "    else",
                      "        minimumSize = thisSize;"]
        lines += ["    combinedSize -= distance;", "    return combinedSize > minimumSize / 2;",
                  "}", "return 11 * combinedSize >= 10 * distance;"]
        yield f"{'snapshot' if snapshots else 'owner'}_{owner}+sum_{addition}+minimum_{minimum}", prefix + "\n".join(
            "    " + line for line in lines) + "\n}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path)
    args = parser.parse_args()
    original = definition((HOMM3_DIR / "src/rmg.cpp").read_text())
    forms = list(variants(original))
    if forms[0][1] != original:
        raise ValueError("review changed canConnect before rebasing")
    if args.parents_from:
        context = args.parents_from.parent
        checkpoint = json.loads(args.parents_from.read_text())
        if checkpoint.get("generation", 0) < 1:
            raise ValueError("parent search is unfinished")
        for relative in ("src/rmg.cpp", "include/rmg.h"):
            if (context / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
                raise ValueError("stale parent snapshot: " + relative)
        manifest = json.loads((context / "input.json").read_text())
        parents = [("original", original)]
        baseline = next(entry for entry in checkpoint["elites"]
                        if entry["labels"]["connection_size_ownership"] == "owner_0+sum_2+minimum_0")
        control = next(entry for entry in checkpoint["records"] if entry["choices"] == [0])
        if control["object_hash"] != baseline["object_hash"]:
            raise ValueError("neutral parent does not reproduce unchanged code")
        # That neutral compound-add parent has the unchanged code identity;
        # retain its baseline source instead so the finite batch stays at 60.
        for entry in checkpoint["elites"]:
            tree = context / "candidates" / entry["id"] / "repeat/tree/src/rmg.cpp"
            if not tree.is_file():
                raise ValueError("missing reproduced parent")
            name, body = forms[entry["choices"][0]]
            if definition(tree.read_text()) != body:
                raise ValueError("parent candidate/manifest mismatch")
            option = manifest["axes"][0]["options"][entry["choices"][0]]
            if option.get("replace", original) != body:
                raise ValueError("parent input option mismatch")
            if entry["id"] != baseline["id"]:
                parents.append((name, body))
        # Verify the input anchor and all recorded option choices, not scores
        # alone. All parent scores will be re-observed in the new context.
        if manifest["axes"][0]["find"] != original:
            raise ValueError("changed parent input anchor")
        forms = parents + list(variants(original, snapshots=True))
    axes = [generator("generate-rmg-position-family.py").axis(
        "connection_size_ownership", "src/rmg.cpp", original, forms)]
    args.output.write_text(json.dumps(dict(schema=1, units=["rmg"],
                                         evidence=__doc__, axes=axes), indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(forms), "connection ownership states")


if __name__ == "__main__":
    main()
