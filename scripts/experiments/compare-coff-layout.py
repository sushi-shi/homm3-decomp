"""Read-only, strict same-layout COFF comparison for byte-neutral controls.

Require every section's name, flags, size and bytes to match in order. With
that premise proved, defined relocation targets can be compared by identical
section/offset, ignoring changed private label counters. Undefined targets
retain their names. Function names and locations must also agree. Only the
existing VC6 anonymous-namespace path/nonce identity rule is used.

This is not a retail scoring normalization, a relayout-equivalence checker,
or evidence of a historical C++ spelling. It never rewrites either object.
Explicit --rename OLD NEW pairs permit independently evidenced function
renames, but must be bijective and present as function symbols on both sides.
"""
import argparse
from pathlib import Path

from homm3.build.canonicalize_data_symbols import CoffObject, FUNCTION_TYPE
from homm3.vc6.source_families import identity_symbol


def compare(left, right, renames=None):
    renames = dict(renames or {})
    left_names = {s.name for s in left.symbols.values() if s.typ == FUNCTION_TYPE}
    right_names = {s.name for s in right.symbols.values() if s.typ == FUNCTION_TYPE}
    if (len(set(renames.values())) != len(renames) or
            not set(renames) <= left_names or
            not set(renames.values()) <= right_names):
        return "function rename map is not a present, bijective mapping"

    def name(obj, symbol):
        original = symbol.name
        if obj is left and symbol.typ == FUNCTION_TYPE:
            original = renames.get(original, original)
        return identity_symbol(original)

    def sections(obj):
        return [(s.name, s.characteristics, s.raw_size, obj.section_bytes(s))
                for s in obj.sections]

    def relocations(obj):
        result = []
        for relocation in obj.relocations:
            symbol = obj.symbols[relocation.symbol_index]
            if symbol.section > 0:
                target = ("defined", symbol.section, symbol.value)
            else:
                target = ("external", name(obj, symbol),
                          symbol.section, symbol.value)
            result.append((relocation.section, relocation.site, relocation.typ, target))
        return sorted(result)

    def functions(obj):
        return sorted((name(obj, s), s.section, s.value)
                      for s in obj.symbols.values() if s.typ == FUNCTION_TYPE)

    # Do not assert relocation equivalence when the target layout differs.
    if sections(left) != sections(right):
        return "section layout or bytes differ"
    if functions(left) != functions(right):
        return "function identities or locations differ"
    if relocations(left) != relocations(right):
        return "relocation sites, kinds or destinations differ"
    return None


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("control", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("--rename", nargs=2, action="append", default=[],
                        metavar=("OLD", "NEW"), help="Explicit evidenced function rename")
    args = parser.parse_args()
    control = CoffObject(args.control.read_bytes())
    candidate = CoffObject(args.candidate.read_bytes())
    if len(dict(args.rename)) != len(args.rename):
        parser.error("duplicate rename source")
    error = compare(control, candidate, args.rename)
    if error:
        print("DIFFERENT:", error)
        raise SystemExit(1)
    print("SAME:", len(control.sections), "sections,", len(control.relocations),
          "relocation destinations, function locations unchanged;",
          len(args.rename), "explicit function renames")
