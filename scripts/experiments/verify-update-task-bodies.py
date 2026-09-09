"""Read-only bounds for the task-fence removal, including untracked bodies.

Require the complete function inventory to stay fixed, and every function's
raw bytes and relocation destinations except the generated Proc destructor to
stay identical. Ignore .debug metadata; prove all remaining sections keep
their order, attributes and bytes before comparing targets by that section map.
The changed Proc must exactly equal the retained Task body, while
the control Proc must be its one-relocation jump thunk. Optional genuine VC6
map controls verify the observed non-folding, not an assumed ICF alias.
"""

import argparse
from pathlib import Path

from homm3.build.canonicalize_data_symbols import CoffObject, _function_ranges
from homm3.vc6.source_families import identity_symbol


TASK = "??1CNewPlayerUpdateTask@@QAE@XZ"
PROC = "??1CNewPlayerUpdateProc@@QAE@XZ"


def functions(path):
    obj = CoffObject(path.read_bytes())
    sections = [section for section in obj.sections if not section.name.startswith(".debug")]
    positions = {section.index: index for index, section in enumerate(sections)}

    def target(rel):
        symbol = obj.symbols[rel.symbol_index]
        if symbol.section > 0:
            return ("defined", positions[symbol.section], symbol.value)
        return ("external", identity_symbol(symbol.name), symbol.section, symbol.value)

    result = {}
    for section, ranges in _function_ranges(obj).items():
        for start, end, symbol in ranges:
            name = identity_symbol(symbol.name)
            if name in result:
                raise ValueError("ambiguous function identity: " + name)
            refs = [(rel.site - start, rel.typ, target(rel))
                    for rel in obj.relocations if rel.section == section and start <= rel.site < end]
            names = [(rel.site - start, rel.typ, identity_symbol(obj.symbols[rel.symbol_index].name))
                     for rel in obj.relocations if rel.section == section and start <= rel.site < end]
            result[name] = dict(code=obj.section_bytes(obj.sections[section - 1])[start:end],
                                refs=refs, names=names, section=positions[section], start=start, end=end)
    all_refs = [(positions[rel.section], rel.site, rel.typ, target(rel))
                for rel in obj.relocations if rel.section in positions]
    layouts = [(section.name, section.characteristics, obj.section_bytes(section)) for section in sections]
    return result, layouts, all_refs


def verify(control, candidate):
    before, old_sections, old_refs = functions(control)
    after, new_sections, new_refs = functions(candidate)
    if before.keys() != after.keys():
        raise ValueError("emitted function inventory changed")
    if len(old_sections) != len(new_sections):
        raise ValueError("non-debug section inventory changed")
    proc_section = before[PROC]["section"]
    if after[PROC]["section"] != proc_section:
        raise ValueError("generated destructor changed section order")
    for index, (left, right) in enumerate(zip(old_sections, new_sections)):
        if left[:2] != right[:2] or (index != proc_section and left[2] != right[2]):
            raise ValueError("non-debug section layout or bytes changed: " + str(index))
    if [rel for rel in old_refs if rel[0] != proc_section] != [rel for rel in new_refs if rel[0] != proc_section]:
        raise ValueError("relocation destinations changed outside generated destructor")

    def identity(record):
        return record["code"], record["refs"]

    changed = {name for name in before if identity(before[name]) != identity(after[name])}
    if changed != {PROC}:
        raise ValueError("unexpected changed functions: " + repr(changed))
    if identity(after[PROC]) != identity(after[TASK]) or identity(before[TASK]) != identity(after[TASK]):
        raise ValueError("generated destructor is not the unchanged exact retained body")
    thunk, refs = before[PROC]["code"], before[PROC]["names"]
    if (thunk[:5] != b"\xe9\0\0\0\0" or any(byte != 0x90 for byte in thunk[5:])
            or len(refs) != 1 or refs[0][:3] != (1, 0x14, TASK)):
        raise ValueError("control is not the one-call-target jump thunk")
    print(f"PASS: {len(before) - 1}/{len(before)} function bytes and relocation destinations unchanged;")
    print(f"  {len(old_sections)} non-debug sections preserve order/attributes, only Proc bytes/relocation differ")
    print(f"  generated Proc now equals retained Task; {len(after[PROC]['code']) - len(thunk)} extra padded bytes")


def verify_map(path):
    addresses = {}
    for line in path.read_text().splitlines():
        columns = line.split()
        if len(columns) >= 3 and columns[1] in (TASK, PROC):
            addresses[columns[1]] = int(columns[2], 16)
    if addresses.keys() != {TASK, PROC} or addresses[TASK] == addresses[PROC]:
        raise ValueError("map does not reproduce distinct retained/generated bodies: " + str(path))
    print(f"  non-folding {path.name}: Task={addresses[TASK]:#x}, Proc={addresses[PROC]:#x}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("control", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("--map", type=Path, action="append", default=[])
    args = parser.parse_args()
    verify(args.control, args.candidate)
    for path in args.map:
        verify_map(path)
