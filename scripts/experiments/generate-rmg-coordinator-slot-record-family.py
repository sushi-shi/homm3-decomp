#!/usr/bin/env python3
"""Slot availability ownership and selection lifetime in generate 0x549930.

The retail coordinator has two adjacent eight-byte slot maps, each cleared
with dword stores. Existing families kept independent arrays. Test an actual
paired availability record or two-row array with the same ordered accesses,
initialization and immutable selection. Keep every callback and repeated
selected-template query; no cached template crosses a callback. No DC RMG
counterpart exists, so aggregate source ownership remains a hypothesis.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def variants(original):
    old = "    char humanSlots[8];\n    memset(humanSlots, 0, sizeof(humanSlots));\n    char allSlots[8];\n    memset(allSlots, 0, sizeof(allSlots));"
    adopted = ("    char humanSlots[8];\n"
               "    for (int humanSlotByte = 0; humanSlotByte < 8; ++humanSlotByte)\n"
               "        humanSlots[humanSlotByte] = 0;\n"
               "    char allSlots[8];\n"
               "    for (int allSlotByte = 0; allSlotByte < 8; ++allSlotByte)\n"
               "        allSlots[allSlotByte] = 0;")
    if adopted in original:
        original = original.replace(adopted, old)
    assert original.count(old) == 1
    for ownership, init, selection in itertools.product(range(3), range(5), range(4)):
        declarations = ["char humanSlots[8];\n    char allSlots[8];",
                        "char slots[2][8];",
                        "struct SlotAvailability { char human[8]; char all[8]; } slots;"]
        human, all_slots = [("humanSlots", "allSlots"), ("slots[0]", "slots[1]"),
                            ("slots.human", "slots.all")][ownership]
        decl = declarations[ownership]
        if init in (1, 2):
            zeros = "0, 0, 0, 0, 0, 0, 0, 0" if init == 1 else "0"
            if ownership == 0:
                setup = f"char humanSlots[8] = {{{zeros}}};\n    char allSlots[8] = {{{zeros}}};"
            else:
                setup = decl[:-1] + f" = {{{{{zeros}}}, {{{zeros}}}}};"
        else:
            def clear(name):
                if init == 0:
                    return f"memset({name}, 0, sizeof({name}));"
                if init == 3:
                    return f"std::fill({name}, {name} + 8, 0);"
                counter = "humanSlotByte" if name == human else "allSlotByte"
                return f"for (int {counter} = 0; {counter} < 8; ++{counter})\n        {name}[{counter}] = 0;"
            if ownership == 0:
                setup = f"char humanSlots[8];\n    {clear(human)}\n    char allSlots[8];\n    {clear(all_slots)}"
            else:
                setup = decl + "\n    " + clear(human) + "\n    " + clear(all_slots)
        marker = "    // SLOT_AVAILABILITY_SETUP"
        body = original.replace(old, marker)
        if ownership:
            body = body.replace("humanSlots", human).replace("allSlots", all_slots)
        body = body.replace(marker, "    " + setup)
        selected_type = ["unsigned int", "int", "const unsigned int", "const int"][selection]
        body = body.replace("unsigned int selected =", selected_type + " selected =", 1)
        yield dict(name=f"ownership_{ownership}+clear_{init}+selection_{selection}", replace=body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    family = generator("generate-rmg-coordinator-family.py")
    original = family.definition((HOMM3_DIR / "src/rmg.cpp").read_text())
    options = list(variants(original))
    options.sort(key=lambda row: row["replace"] != original)
    assert len(options) == len({row['replace'] for row in options}) == 60
    assert options[0]['replace'] == original
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="slot_availability_ownership", source="src/rmg.cpp", find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("60 slot record/initialization/selection states")


if __name__ == "__main__":
    main()
