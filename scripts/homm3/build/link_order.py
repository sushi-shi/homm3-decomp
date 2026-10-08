"""homm3.build.link_order - the retail object order of the game link.

    python3 -m homm3.build.link_order            # print the derived order
    python3 -m homm3.build.link_order --runs     # with each unit's key

LINK lays `.text` out object by object in command-line order, and inside an
object in its section order; library members follow every object, in the
order LINK pulls them. Retail's code order therefore IS the original
object order, and it is recovered here from the objects themselves:

  1. A unit's key is the lowest retail RVA of a function its object defines
     out of line in a NODUPLICATES (non-inline) COMDAT, with a unique,
     unfolded retail address. Such a function can only come from that
     object, wherever its claim was written.
  2. A unit without one uses the lowest unfolded retail RVA among the
     functions it claims and that no other object defines.
  3. A unit with no retail-placed code at all (its only bodies fold into
     another object's copy) is placed by a reviewed row of
     config/retail/link-order.tsv, after a named unit.

Header COMDATs are deliberately not keys: LINK keeps the copy of the first
object that defines one, so a header body our earlier object emits and the
original did not lands in that object's run. Those are object-content
findings (`homm3 verify link-diff` reports them as order breaks), not
evidence about the object order.

Units archived into a library (`library = "<name>"` in config/units.toml)
are not on the object list: `library_members` orders each archive's
members by the same key, and LINK pulls them after every object.
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

from homm3.core import common
from homm3.core.images import path as _image_path

ROOT = common.HOMM3_DIR
REVIEWED = ROOT / "config/retail/link-order.tsv"
ZLIB_MAP = ROOT / "config/retail/zlib-map.tsv"
IDENTITIES = ROOT / _image_path("build/gen/address_identities.tsv")

_CODE = 0x20
_LNK_COMDAT = 0x1000
_SELECT_NODUPLICATES = 1


def defined_functions(path: Path) -> list[tuple[str, bool]]:
    """[(external code symbol, out-of-line)] defined at offset 0 of a `.text`
    section of the COFF object at `path`. `out-of-line` is true for a
    non-COMDAT section or a NODUPLICATES COMDAT: a function the object owns,
    never a header body any other object may also emit."""
    data = Path(path).read_bytes()
    _m, nsec, _t, symptr, nsym, optsz, _c = struct.unpack_from("<HHIIIHH", data, 0)
    strtab = symptr + nsym * 18
    chars = [struct.unpack_from("<I", data, 20 + optsz + 40 * i + 36)[0]
             for i in range(nsec)]
    selection: dict[int, int] = {}
    out = []
    i = 0
    while i < nsym:
        at = symptr + 18 * i
        raw = data[at:at + 8]
        if raw[:4] == b"\0\0\0\0":
            offset = strtab + struct.unpack_from("<I", raw, 4)[0]
            name = data[offset:data.index(b"\0", offset)].decode("latin-1")
        else:
            name = raw.rstrip(b"\0").decode("latin-1")
        value, section, _type, storage, naux = struct.unpack_from("<IhHBB", data, at + 8)
        if storage == 3 and naux and value == 0 and 1 <= section <= nsec \
                and name.startswith("."):
            selection[section] = data[at + 18 + 14]
        elif storage == 2 and value == 0 and 1 <= section <= nsec \
                and chars[section - 1] & _CODE:
            comdat = chars[section - 1] & _LNK_COMDAT
            owned = not comdat or selection.get(section) == _SELECT_NODUPLICATES
            out.append((name, owned))
        i += 1 + naux
    return out


def retail_functions() -> tuple[dict[str, set[int]], dict[int, set[str]]]:
    """({name: retail RVAs}, {RVA: names}) over the source claims, the zlib
    provider table and the proven extra names of folded addresses."""
    from homm3.core.tsv import read as read_tsv
    from homm3.retail_labels.fragments import all_claims
    by_name: dict[str, set[int]] = {}

    def add(name: str, rva: int) -> None:
        by_name.setdefault(name, set()).add(rva)

    for claim in all_claims():
        if claim.kind == "func":
            add(claim.name, claim.rva)
    for row in read_tsv(ZLIB_MAP)[2]:
        if row["kind"] == "func":
            add(row["name"], int(row["rva"], 16))
    if IDENTITIES.is_file():
        for row in read_tsv(IDENTITIES)[2]:
            add(row["name"], int(row["rva"], 16))
    by_rva: dict[int, set[str]] = {}
    for name, rvas in by_name.items():
        for rva in rvas:
            by_rva.setdefault(rva, set()).add(name)
    return by_name, by_rva


def claimed_by_unit() -> dict[str, set[str]]:
    from homm3.core.tsv import read as read_tsv
    from homm3.retail_labels.fragments import all_claims
    out: dict[str, set[str]] = {}
    for claim in all_claims():
        if claim.kind == "func":
            out.setdefault(claim.unit, set()).add(claim.name)
    for row in read_tsv(ZLIB_MAP)[2]:
        if row["kind"] == "func":
            out.setdefault(row["unit"], set()).add(row["name"])
    return out


def reviewed_rows() -> list[dict]:
    from homm3.core.tsv import read as read_tsv
    return read_tsv(REVIEWED)[2] if REVIEWED.is_file() else []


def unit_keys(objects: dict[str, Path]) -> dict[str, tuple[int, str]]:
    """{unit: (key RVA, rule)} for every unit with retail-placed code."""
    by_name, by_rva = retail_functions()
    claimed = claimed_by_unit()
    defined = {unit: defined_functions(path) for unit, path in objects.items()}
    definers: dict[str, int] = {}
    for functions in defined.values():
        for name, _owned in functions:
            definers[name] = definers.get(name, 0) + 1

    def placed(name: str) -> int | None:
        rvas = by_name.get(name, ())
        if len(rvas) != 1:
            return None
        rva = next(iter(rvas))
        return rva if len(by_rva.get(rva, ())) == 1 else None

    keys: dict[str, tuple[int, str]] = {}
    for unit, functions in defined.items():
        owned = [rva for name, own in functions if own
                 for rva in [placed(name)] if rva is not None]
        if owned:
            keys[unit] = (min(owned), "out-of-line")
            continue
        sole = [rva for name, _own in functions
                if definers[name] == 1 and name in claimed.get(unit, ())
                for rva in [placed(name)] if rva is not None]
        if sole:
            keys[unit] = (min(sole), "sole-definer")
    return keys


def order(objects: dict[str, Path]) -> tuple[list[str], dict[str, str]]:
    """(units in retail link order, {unit: how it was placed}). Raises
    ValueError for a unit that neither retail code nor a reviewed row places."""
    keys = unit_keys(objects)
    ordered = sorted((unit for unit in objects if unit in keys),
                     key=lambda unit: keys[unit][0])
    how = {unit: f"{keys[unit][1]} {keys[unit][0]:#x}" for unit in ordered}
    pending = [unit for unit in objects if unit not in keys]
    rows = {row["unit"]: row for row in reviewed_rows()}
    while pending:
        progress = False
        for unit in list(pending):
            row = rows.get(unit)
            if row is None:
                raise ValueError(
                    f"{unit}: no retail-placed code and no config/retail/"
                    "link-order.tsv row places it")
            if row["after"] in ordered:
                ordered.insert(ordered.index(row["after"]) + 1, unit)
                how[unit] = f"reviewed after {row['after']}"
                pending.remove(unit)
                progress = True
        if not progress:
            raise ValueError("config/retail/link-order.tsv places "
                             + ", ".join(pending) + " after unplaced units")
    return ordered, how


def main(argv: list[str] | None = None) -> int:
    import argparse
    from homm3.build.link import game_objects
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--runs", action="store_true",
                        help="print how each unit was placed")
    args = parser.parse_args(argv)
    objects, libraries = game_objects()
    units, how = order(objects)
    for unit in units:
        print(f"{unit}\t{how[unit]}" if args.runs else unit)
    for library, members in libraries.items():
        print(f"# {library}.lib: " + " ".join(members))
    return 0


def logged_main(argv: list[str] | None = None) -> int:
    import shlex
    from homm3.core.usage import append, run_logged
    argv = list(sys.argv[1:] if argv is None else argv)
    command = shlex.join(["python3", "-m", "homm3.build.link_order", *argv])
    return run_logged(main, argv, lambda rc, **meta: append(
        ROOT / "build/homm3_usage.log", command, rc, **meta), failure_rc=1)


if __name__ == "__main__":
    sys.exit(logged_main())
