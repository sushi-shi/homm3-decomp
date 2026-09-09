"""Verify the exact scope of the tactical ordinary-helper emission change.

All prior sections, function locations and relocation destinations must survive
in order. Only the three specified, unreferenced executable COMDAT sections
may be added. This is a raw object comparison, not a retail normalization or
a claim that these unused copies have retained retail addresses.
"""

import argparse
from pathlib import Path

from homm3.build.canonicalize_data_symbols import CoffObject, FUNCTION_TYPE, MEM_EXECUTE
from homm3.vc6.source_families import identity_symbol


ADDED = {
    "?getGroupDamageValue@type_AI_spellcaster@@QBEJHJJPAVhero@@@Z",
    "?considerMassDamage@type_AI_spellcaster@@QBEXAAUtype_spell_choice@@@Z",
    "?considerSummon@type_AI_spellcaster@@QBEXAAUtype_spell_choice@@@Z",
}


def verify(control, candidate):
    left = CoffObject(control.read_bytes())
    right = CoffObject(candidate.read_bytes())
    def functions(obj):
        return {identity_symbol(s.name): s for s in obj.symbols.values()
                if s.typ == FUNCTION_TYPE and s.section > 0}
    old, new = functions(left), functions(right)
    if set(old) - set(new) or set(new) - set(old) != ADDED:
        raise ValueError("unexpected lost or added function")
    added_sections = {new[name].section for name in ADDED}
    if len(added_sections) != 3:
        raise ValueError("expected three independent function COMDATs")
    for name in ADDED:
        sym = new[name]
        section = right.sections[sym.section - 1]
        if sym.value or not section.characteristics & 0x1000 or not section.characteristics & MEM_EXECUTE:
            raise ValueError("new helper is not an independent executable COMDAT")
    retained = [(index, section) for index, section in enumerate(right.sections, 1)
                if index not in added_sections]
    if len(retained) != len(left.sections):
        raise ValueError("unexpected non-helper section addition or removal")
    mapping = {}
    for before, (after, right_section) in enumerate(retained, 1):
        left_section = left.sections[before - 1]
        def identity(obj, sec):
            return sec.name, sec.characteristics, sec.raw_size, obj.section_bytes(sec)
        if identity(left, left_section) != identity(right, right_section):
            raise ValueError("prior section layout or bytes changed")
        mapping[before] = after
    for name, sym in old.items():
        other = new[name]
        if mapping[sym.section] != other.section or sym.value != other.value:
            raise ValueError("prior function location changed")
    def relocations(obj, translate):
        result = []
        for rel in obj.relocations:
            target = obj.symbols[rel.symbol_index]
            if obj is right and target.section in added_sections:
                raise ValueError("a new helper has an incoming relocation")
            if obj is right and rel.section in added_sections:
                continue
            destination = (("defined", translate[target.section], target.value)
                           if target.section > 0 else
                           ("external", identity_symbol(target.name), target.section, target.value))
            result.append((translate[rel.section], rel.site, rel.typ, destination))
        return sorted(result)
    same_indices = {i: i for i in range(1, len(right.sections) + 1)}
    if relocations(left, mapping) != relocations(right, same_indices):
        raise ValueError("prior relocation sites, kinds or destinations changed")
    print(f"PASS: all {len(left.sections)} prior sections, {len(old)} functions and "
          f"{len(left.relocations)} relocation destinations preserved; three unreferenced helper COMDATs added")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("control", type=Path)
    parser.add_argument("candidate", type=Path)
    args = parser.parse_args()
    verify(args.control, args.candidate)
