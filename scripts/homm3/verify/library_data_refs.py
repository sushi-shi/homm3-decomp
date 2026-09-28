"""Recover original static-library data names from retail code references.

The retail image has no symbols, but every admitted NAFXCW/LIBCMT/LIBCIMT
function came from the pinned VC6 archive.  Its COFF member still has a DIR32
relocation at the same operand offset as retail.  Joining

    retail function RVA + operand offset -> archive function + COFF relocation

recovers the referent's original symbol and addend without guessing from data
adjacency.  This is a read-only attribution oracle for the cross-frontier rows
reported by ``homm3 verify data-coverage --tsv``.

    homm3 verify library-data-refs
    homm3 verify library-data-refs --near 0x1eafbc
    homm3 verify library-data-refs --tsv
"""

from __future__ import annotations

import struct
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

from homm3.build.canonicalize_data_symbols import CoffObject, DIR32, Symbol
from homm3.core.paths import BUILD, RETAIL, msvc_dir
from homm3.core.tsv import read as read_tsv
from homm3.delink.implib import _ar_members


ACCESS_TSV = BUILD / "gen/data_access_map.tsv"
GAPS_TSV = BUILD / "gen/data_coverage_gaps.tsv"
FUNCTIONS_TSV = RETAIL / "runtime-map.tsv"

LIB_FILES = {
    "LIBC": "LIBC.LIB",
    "LIBCMT": "LIBCMT.LIB",
    "LIBCP": "LIBCP.LIB",
    "LIBCPMT": "LIBCPMT.LIB",
}

MEM_EXECUTE = 0x20000000
UNINITIALIZED_DATA = 0x00000080
INITIALIZED_DATA = 0x00000040
MEM_WRITE = 0x80000000


@dataclass(frozen=True)
class Member:
    library: str
    name: str
    obj: CoffObject


@dataclass(frozen=True)
class Definition:
    member: Member
    symbol: Symbol
    size: int | None
    storage: str


@dataclass(frozen=True)
class Recovery:
    target_rva: int
    base_rva: int
    addend: int
    name: str
    size: int | None
    storage: str | None
    library: str
    member: str
    fn_rva: int
    fn_name: str
    site_rva: int


def _section_size(obj: CoffObject, secnum: int) -> int:
    """COFF section size, including zero-raw-byte .bss sections."""
    sec = obj.sections[secnum - 1]
    if sec.raw_size:
        return sec.raw_size
    for sym in obj.symbols.values():
        if sym.section != secnum or sym.storage_class != 3 or not sym.aux_count:
            continue
        if sym.name != sec.name:
            continue
        return struct.unpack_from("<I", obj.data, sym.offset + 18)[0]
    return 0


def _storage(obj: CoffObject, secnum: int) -> str | None:
    if not (1 <= secnum <= len(obj.sections)):
        return None
    flags = obj.sections[secnum - 1].characteristics
    if flags & MEM_EXECUTE:
        return "text"
    if flags & UNINITIALIZED_DATA:
        return "bss"
    if flags & INITIALIZED_DATA:
        return "data" if flags & MEM_WRITE else "rdata"
    return None


def _symbol_size(member: Member, symbol: Symbol) -> int | None:
    """Physical extent to the next datum in the same contribution."""
    if symbol.section == 0:
        return symbol.value or None          # COFF COMMON: Value is the size
    if symbol.section < 1:
        return None
    end = _section_size(member.obj, symbol.section)
    starts = sorted({s.value for s in member.obj.symbols.values()
                     if s.section == symbol.section
                     and s.storage_class in (2, 3)
                     and not s.name.startswith(".")
                     and s.value > symbol.value})
    if starts:
        end = min(end, starts[0])
    return end - symbol.value if end > symbol.value else None


def _named_ar_members(path: Path):
    """Archive members with MS ``/decimal`` long-name references resolved."""
    longnames = b""
    for name, body in _ar_members(path):
        if name == "//":
            longnames = body
            continue
        if name.startswith("/") and name[1:].isdigit() and longnames:
            start = int(name[1:])
            end = longnames.find(b"/\n", start)
            if end < 0:
                end = longnames.find(b"\n", start)
            if end >= 0:
                name = longnames[start:end].decode("latin1")
        yield name, body


def _archive(library: str) -> list[Member]:
    path = msvc_dir() / "lib" / LIB_FILES[library]
    out = []
    for name, body in _named_ar_members(path):
        try:
            obj = CoffObject(body)
        except (ValueError, struct.error):
            continue
        out.append(Member(library, name.rstrip("/"), obj))
    return out


def _indexes(libraries: set[str]):
    by_symbol: dict[tuple[str, str], list[Definition]] = defaultdict(list)
    members: dict[str, list[Member]] = {}
    for library in sorted(libraries):
        rows = _archive(library)
        members[library] = rows
        for member in rows:
            for symbol in member.obj.symbols.values():
                storage = _storage(member.obj, symbol.section)
                if symbol.section == 0 and symbol.value:
                    storage = "common"
                if storage is None:
                    continue
                by_symbol[(library, symbol.name)].append(Definition(
                    member, symbol, _symbol_size(member, symbol), storage))
    return members, by_symbol


def _function_claims(path=FUNCTIONS_TSV):
    _banner, _header, rows = read_tsv(path)
    out: dict[int, list[tuple[str, str]]] = defaultdict(list)
    for row in rows:
        for library in LIB_FILES:
            out[int(row["rva"], 16)].append((library, row["name"]))
    return out


def _worklist(gaps_path=GAPS_TSV, near: int | None = None):
    _banner, _header, rows = read_tsv(gaps_path)
    out = []
    for row in rows:
        lo = int(row["rva"], 16)
        hi = lo + int(row["length"])
        if near is not None and not lo <= near < hi:
            continue
        if near is None and (not int(row["touched"]) or row["verdict"] == "PAD"):
            continue
        out.append((lo, hi, row))
    return out


def _accesses(ranges, access_path=ACCESS_TSV):
    _banner, _header, rows = read_tsv(access_path)
    out = []
    for row in rows:
        if int(row["site_rva"], 16) == 0:
            continue                         # address census row, no code operand
        target = int(row["target_rva"], 16)
        if any(lo <= target < hi for lo, hi, _gap in ranges):
            out.append(row)
    return out


def _code_definitions(members, library: str, name: str):
    for member in members.get(library, ()):
        for symbol in member.obj.symbols.values():
            if symbol.name == name and _storage(member.obj, symbol.section) == "text":
                yield member, symbol


def _definition(by_symbol, library: str, member: Member, symbol: Symbol):
    if symbol.section > 0:
        return Definition(member, symbol, _symbol_size(member, symbol),
                          _storage(member.obj, symbol.section) or "unknown")
    defs = list(by_symbol.get((library, symbol.name), ()))
    if not defs:
        defs = [d for (lib, name), rows in by_symbol.items()
                if name == symbol.name for d in rows]
    if not defs:
        return None
    shapes = {(d.size, d.storage) for d in defs}
    return defs[0] if len(shapes) == 1 else None


def recover(gaps_path=GAPS_TSV, access_path=ACCESS_TSV,
            functions_path=FUNCTIONS_TSV, near: int | None = None):
    from homm3.core.pe import image
    from homm3.retail_labels.censuses import functions
    pe = image()
    extents = {r['rva']: r['size'] for r in functions()}
    ranges = _worklist(gaps_path, near)
    accesses = _accesses(ranges, access_path)
    claims = _function_claims(functions_path)
    libraries = {lib for aliases in claims.values() for lib, _name in aliases}
    members, by_symbol = _indexes(libraries)

    found: list[Recovery] = []
    unresolved = []
    for row in accesses:
        fn_rva = int(row["fn_rva"], 16)
        site_rva = int(row["site_rva"], 16)
        target_rva = int(row["target_rva"], 16)
        hits = []
        for library, fn_name in claims.get(fn_rva, ()):
            for member, fn_symbol in _code_definitions(members, library, fn_name):
                extent = extents.get(fn_rva, 0)
                if not extent:
                    continue
                section = member.obj.sections[fn_symbol.section - 1]
                begin = section.raw_offset + fn_symbol.value
                raw_body = member.obj.data[begin:begin + extent]
                retail_body = pe.read(fn_rva, extent)
                if retail_body is None or fn_symbol.value + extent > section.raw_size:
                    continue
                masked = set()
                unsupported = False
                for relocation in member.obj.relocations:
                    if (relocation.section == fn_symbol.section and
                            fn_symbol.value <= relocation.site < fn_symbol.value + extent):
                        relative = relocation.site - fn_symbol.value
                        if relocation.typ not in (6, 20) or relative + 4 > extent:
                            unsupported = True
                            break
                        masked.update(range(relative, relative + 4))
                if unsupported or any(a != b for i, (a, b) in enumerate(
                        zip(raw_body, retail_body)) if i not in masked):
                    continue
                site = fn_symbol.value + site_rva - fn_rva
                for reloc in member.obj.relocations:
                    if reloc.section != fn_symbol.section or reloc.site != site \
                            or reloc.typ != DIR32:
                        continue
                    sec = member.obj.sections[reloc.section - 1]
                    raw = member.obj.data[sec.raw_offset + reloc.site:
                                          sec.raw_offset + reloc.site + 4]
                    if len(raw) != 4:
                        continue
                    addend = struct.unpack("<i", raw)[0]
                    referent = member.obj.symbols[reloc.symbol_index]
                    definition = _definition(by_symbol, library, member, referent)
                    hits.append(Recovery(
                        target_rva=target_rva,
                        base_rva=target_rva - addend,
                        addend=addend,
                        name=referent.name,
                        size=definition.size if definition else None,
                        storage=definition.storage if definition else None,
                        library=(definition.member.library if definition else library),
                        member=(definition.member.name if definition else member.name),
                        fn_rva=fn_rva,
                        fn_name=fn_name,
                        site_rva=site_rva))
        unique = {(h.base_rva, h.addend, h.name, h.size, h.storage,
                   h.library, h.member) for h in hits}
        if len(unique) == 1:
            found.append(hits[0])
        else:
            unresolved.append((row, hits))
    return found, unresolved, ranges, accesses


def _group(found):
    groups = defaultdict(list)
    for row in found:
        groups[(row.base_rva, row.name, row.size, row.storage,
                row.library, row.member)].append(row)
    return sorted(groups.items(), key=lambda item: (item[0][0], item[0][1]))


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm3 verify library-data-refs",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--near", type=lambda s: int(s, 0),
                    help="only the coverage range containing this RVA")
    ap.add_argument("--tsv", action="store_true",
                    help="print machine-readable recovered rows")
    args = ap.parse_args(argv)

    for required in (GAPS_TSV, ACCESS_TSV):
        if not required.is_file():
            ap.error(f"missing {required}; run homm3 verify data-coverage --tsv")
    found, unresolved, ranges, accesses = recover(near=args.near)
    groups = _group(found)
    if args.tsv:
        print("rva\tsize\tstorage\tname\tlibrary\tmember\trefs")
        for (rva, name, size, storage, library, member), refs in groups:
            print(f"0x{rva:x}\t{f'0x{size:x}' if size else '-'}\t"
                  f"{storage or '-'}\t{name}\t{library}\t{member}\t{len(refs)}")
    else:
        print(f"library data recovery: {len(ranges)} coverage range(s), "
              f"{len(accesses)} access(es), {len(found)} resolved, "
              f"{len(unresolved)} unresolved/ambiguous")
        for (rva, name, size, storage, library, member), refs in groups:
            suffix = f"+0x{size:x}" if size else "+?"
            print(f"  0x{rva:06x}{suffix:<8} {(storage or '-'):<6} {name}")
            print(f"      {library}:{member}  {len(refs)} retail reference(s); "
                  f"witness {refs[0].fn_name}+0x{refs[0].site_rva-refs[0].fn_rva:x}")
        if unresolved:
            print("unresolved accesses:")
            for row, hits in unresolved[:40]:
                why = "no library COFF relocation" if not hits else \
                    f"{len(hits)} conflicting recoveries"
                print(f"  {row['site_rva']} -> {row['target_rva']} in "
                      f"{row['fn_name'] or '<unknown>'}: {why}")
    return 1 if unresolved else 0


if __name__ == "__main__":
    raise SystemExit(main())
