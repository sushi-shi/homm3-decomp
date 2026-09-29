"""homm3.build.identity_relocations - compare relocations by proven address.

The stripped retail image names each relocation target once. Where the linker
gave one address several names (identical-code folding, a library section's
alias symbols) or the model names an address by a placeholder, the delinked
target can spell a relocation differently from the candidate although both
reach the same retail address.

This paired pass rewrites a TARGET relocation to the candidate's symbol only
when:

* both relocations sit at the same offset of the same-named function, with
  the same type and matching instruction bytes around the operand;
* the candidate symbol has exactly one proven retail address: a model
  binding or an `address_identities.tsv` row (verified library symbol,
  identical-body fold, import thunk);
* that address plus the candidate addend equals the target symbol's retail
  address plus the target addend (REL32: the same address and inline value).

Unproven names, ambiguous identities and different resolved addresses stay
visible. The candidate symbol is appended to the target's symbol table when
absent; sections, bytes outside the rewritten operands and every other
relocation must stay unchanged.
"""

from __future__ import annotations

import csv
import re
import struct
from pathlib import Path

from homm3.compare import canonicalize as canon

DIR32 = 0x0006
REL32 = 0x0014
PLACEHOLDER = re.compile(r"(?:data|bss|const|vtbl|fn)_([0-9a-fA-F]+)|\$gap_([0-9a-fA-F]+)")


def load_identities(path: Path) -> dict[tuple[str, str], set[int]]:
    """{(unit, name): {rva}} from a generated address_identities.tsv; unit is
    '' for an image-wide identity."""
    out: dict[tuple[str, str], set[int]] = {}
    if not path.is_file():
        return out
    with path.open(newline="") as stream:
        rows = csv.DictReader((line for line in stream if not line.startswith("#")),
                              delimiter="\t")
        for row in rows:
            key = (row.get("unit") or "", row["name"])
            out.setdefault(key, set()).add(int(row["rva"], 0))
    return out


def resolve_name(name: str, symbol_rvas: dict[str, tuple[int, str]],
                 identities: dict[tuple[str, str], set[int]], unit: str = "") -> int | None:
    """The one retail address `name` is proven at, else None. A unit-scoped
    identity (that compiland's copy of a content-named datum) decides alone."""
    scoped = identities.get((unit, name)) if unit else None
    if scoped:
        return next(iter(scoped)) if len(scoped) == 1 else None
    found = set(identities.get(("", name), ()))
    known = symbol_rvas.get(name)
    if known is not None:
        found.add(known[0])
    if not found:
        match = PLACEHOLDER.fullmatch(name)
        if match:
            found.add(int(match.group(1) or match.group(2), 16))
    return next(iter(found)) if len(found) == 1 else None


def _pairs(coff: canon.CoffObject):
    ranges = canon._function_ranges(coff)
    names: dict[str, list] = {}
    for rows in ranges.values():
        for _start, _end, symbol in rows:
            names.setdefault(symbol.name, []).append(symbol)
    pairs = {}
    for relocation in coff.relocations:
        if relocation.typ not in (DIR32, REL32):
            continue
        owner = canon._function_owner(ranges, relocation.section, relocation.site)
        if owner is None or len(names.get(owner.name, ())) != 1:
            continue
        pairs[(owner.name, relocation.site - owner.value)] = relocation
    return pairs


def _context(data: bytes, site: int) -> bytes:
    return data[max(0, site - 2):site]


def _append_symbols(payload: bytearray, coff: canon.CoffObject,
                    new: list[tuple[str, int]]) -> dict[str, int]:
    """Append undefined external symbols; {name: index}."""
    if not new:
        return {}
    strings = bytearray(payload[coff.string_offset + 4:coff.string_offset + coff.string_size])
    records = bytearray()
    indices = {}
    index = coff.symbol_count
    for name, typ in new:
        raw = name.encode("latin-1")
        if len(raw) <= 8:
            field = raw.ljust(8, b"\0")
        else:
            field = struct.pack("<II", 0, 4 + len(strings))
            strings += raw + b"\0"
        records += field + struct.pack("<IhHBB", 0, 0, typ, canon.EXTERNAL_STORAGE, 0)
        indices[name] = index
        index += 1
    tail = struct.pack("<I", 4 + len(strings)) + bytes(strings)
    payload[coff.string_offset:] = bytes(records) + tail
    struct.pack_into("<I", payload, 12, index)
    return indices


def canonicalize(base_payload: bytes, target_payload: bytes,
                 symbol_rvas: dict[str, tuple[int, str]],
                 identities: dict[tuple[str, str], set[int]],
                 unit: str = "") -> tuple[bytes, int]:
    base = canon.CoffObject(base_payload)
    target = canon.CoffObject(target_payload)
    base_pairs = _pairs(base)
    target_pairs = _pairs(target)
    rewrites = []                     # (target relocation, base symbol, addend)
    for key, trel in target_pairs.items():
        brel = base_pairs.get(key)
        if brel is None or brel.typ != trel.typ:
            continue
        bsym = base.symbols[brel.symbol_index]
        tsym = target.symbols[trel.symbol_index]
        if bsym.name == tsym.name:
            continue
        mine = resolve_name(bsym.name, symbol_rvas, identities, unit)
        theirs = resolve_name(tsym.name, symbol_rvas, identities)
        if mine is None or theirs is None:
            continue
        bdata = base.section_bytes(base.sections[brel.section - 1])
        tdata = target.section_bytes(target.sections[trel.section - 1])
        if brel.site + 4 > len(bdata) or trel.site + 4 > len(tdata):
            continue
        badd = struct.unpack_from("<i", bdata, brel.site)[0]
        tadd = struct.unpack_from("<i", tdata, trel.site)[0]
        if trel.typ == REL32:
            if mine != theirs or badd != tadd:
                continue
        elif (mine + badd) & 0xFFFFFFFF != (theirs + tadd) & 0xFFFFFFFF:
            continue
        if _context(bdata, brel.site) != _context(tdata, trel.site):
            continue
        rewrites.append((trel, bsym, badd))
    if not rewrites:
        return target_payload, 0

    data = bytearray(target_payload)
    existing = {}
    for symbol in target.symbols.values():
        if symbol.storage_class == canon.EXTERNAL_STORAGE:
            existing.setdefault(symbol.name, symbol.index)
    wanted = []
    for _trel, bsym, _add in rewrites:
        if bsym.name not in existing and bsym.name not in dict(wanted):
            wanted.append((bsym.name, bsym.typ))
    existing.update(_append_symbols(data, target, wanted))
    for trel, bsym, badd in rewrites:
        section = target.sections[trel.section - 1]
        struct.pack_into("<I", data, trel.offset + 4, existing[bsym.name])
        struct.pack_into("<i", data, section.raw_offset + trel.site, badd)

    # Postconditions: topology, unrelated relocations and bytes are unchanged.
    normalized = canon.CoffObject(bytes(data))
    if normalized.section_count != target.section_count or \
            normalized.symbol_count != target.symbol_count + len(wanted):
        raise RuntimeError("identity-relocation normalization changed COFF topology")
    for index, symbol in target.symbols.items():
        if normalized.symbols[index].name != symbol.name or \
                normalized.symbols[index].section != symbol.section:
            raise RuntimeError("identity-relocation normalization changed a symbol")
    changed = {trel.offset: (existing[bsym.name], badd) for trel, bsym, badd in rewrites}
    before = {(r.section, r.site): r for r in target.relocations}
    for row in normalized.relocations:
        original = before[(row.section, row.site)]
        expected = changed.get(original.offset, (original.symbol_index, None))[0]
        if row.symbol_index != expected or row.typ != original.typ:
            raise RuntimeError("identity-relocation normalization changed unrelated relocations")
    for original, now in zip(target.sections, normalized.sections):
        if (original.name, original.raw_size, original.raw_offset, original.reloc_offset,
                original.reloc_count, original.characteristics) != \
                (now.name, now.raw_size, now.raw_offset, now.reloc_offset,
                 now.reloc_count, now.characteristics):
            raise RuntimeError("identity-relocation normalization changed section metadata")
        old = bytearray(target.section_bytes(original))
        new = bytearray(normalized.section_bytes(now))
        for trel, _bsym, _badd in rewrites:
            if trel.section == original.index:
                old[trel.site:trel.site + 4] = new[trel.site:trel.site + 4] = bytes(4)
        if old != new:
            raise RuntimeError("identity-relocation normalization changed section bytes")
    return bytes(data), len(rewrites)
