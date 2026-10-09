"""homm3.census.vtables - vtable starts, slot counts and RTTI class identities.

Under /GR the dword before each vtable points at its Complete Object
Locator, whose TypeDescriptor names the class (`.?AVFoo@Bar@@` -> `Foo@Bar`);
a secondary table (locator offset != 0) is named `??_7Derived@@6BBase@@@`
after the base class at that offset. Tables without a locator (libraries
built without /GR) are found where code stores them as a vptr. A table's
slots run while each dword is a census function start.
"""
from __future__ import annotations

import re
import struct


def _cstr(image, rva):
    sec = image.section_of(rva)
    if sec is None:
        return None
    off = sec.raw_offset + rva - sec.rva
    end = image.data.find(b"\0", off, off + 512)
    return image.data[off:end].decode("latin-1") if end > off else None


def type_name(c, td):
    """`.?AVFoo@Bar@@` -> `Foo@Bar` (the mangled qualified class fragment)."""
    raw = _cstr(c.image, td + 8)
    if not raw or not raw.startswith((".?AV", ".?AU")) or not raw.endswith("@@"):
        return None
    return raw[4:-2]


def locator(c, rva):
    """Parse a Complete Object Locator; None if it is not one."""
    base = c.image.image_base
    fields = [c.dword(rva + 4 * i) for i in range(5)]
    if None in fields or fields[0] != 0:
        return None
    _sig, offset, cd_offset, td, chd = fields
    if not (c.image.in_image(td) and c.image.in_image(chd)):
        return None
    name = type_name(c, td - base)
    if name is None:
        return None
    return offset, cd_offset, name, chd - base


def base_names(c, chd, virtual=False):
    """[(name, mdisp)] of a Class Hierarchy Descriptor's base classes;
    with `virtual`, [(name, mdisp, reached through a virtual base)]."""
    base = c.image.image_base
    count, array = c.dword(chd + 8), c.dword(chd + 12)
    if count is None or array is None or not 0 < count < 256:
        return []
    out = []
    for i in range(count):
        bcd = c.dword(array - base + 4 * i)
        if bcd is None:
            break
        td, mdisp = c.dword(bcd - base), c.dword(bcd - base + 8)
        name = type_name(c, td - base) if td else None
        mdisp = struct.unpack("<i", struct.pack("<I", mdisp or 0))[0]
        if virtual:
            pdisp = c.dword(bcd - base + 12)
            out.append((name, mdisp, pdisp is not None and pdisp != 0xFFFFFFFF))
        else:
            out.append((name, mdisp))
    return out


def _bca(c, chd):
    """[(name, contained, mdisp, pdisp)] of a Class Hierarchy Descriptor's
    base class array, the class itself first, its bases in preorder."""
    base = c.image.image_base
    count, array = c.dword(chd + 8), c.dword(chd + 12)
    if count is None or array is None or not 0 < count < 256:
        return None
    out = []
    for i in range(count):
        bcd = c.dword(array - base + 4 * i)
        if bcd is None:
            return None
        td, contained, mdisp, pdisp = (c.dword(bcd - base + 4 * k) for k in range(4))
        name = type_name(c, td - base) if td else None
        if name is None or contained is None or mdisp is None or pdisp is None:
            return None
        out.append((name, contained, struct.unpack("<i", struct.pack("<I", mdisp))[0],
                    struct.unpack("<i", struct.pack("<I", pdisp))[0]))
    return out


_SIMPLE = re.compile(r"^[A-Za-z_][A-Za-z_0-9]*$")


def _mangled_vftable(name, path):
    """`??_7` + the class + `6B` + its vfptr's mangled path + `@`, with
    MSVC's back references to the simple name components already spelled
    (`??_7TLinkableObject@@6B0@@`)."""
    parts = name.split("@")
    if not all(_SIMPLE.match(part) for frag in path for part in frag.split("@")) \
            or not all(_SIMPLE.match(part) for part in parts):
        return f"??_7{name}@@6B" + "".join(f"{frag}@@" for frag in path) + "@"
    memo = list(parts)
    out = f"??_7{name}@@6B"
    for frag in path:
        for part in frag.split("@"):
            if part in memo and memo.index(part) < 10:
                out += str(memo.index(part))
            else:
                memo.append(part)
                out += part + "@"
        out += "@"
    return out + "@"


def _virtual_vftable_names(c, cols):
    """{vtable rva: symbol} for the classes with virtual bases, after the
    MSVC vfptr paths (Clang's MicrosoftVTableContext::computeVTablePaths):
    a class's own new vfptr and every vfptr it inherits through its direct
    bases, each mangled by the bases needed to tell the tables apart. A
    class whose hierarchy the image's RTTI does not describe keeps the
    offset naming."""
    chds, offsets = {}, {}
    for rva, (offset, _cd, name, chd) in cols:
        chds.setdefault(name, chd)
        offsets.setdefault(name, []).append((offset, rva))
    memo = {}

    def direct_bases(entries):
        out, i = [], 1
        while i < len(entries):
            name, contained, mdisp, pdisp = entries[i]
            out.append((name, pdisp != -1, mdisp, entries[i:i + contained + 1]))
            i += contained + 1
        return out

    def paths(name):
        if name in memo:
            return memo[name]
        memo[name] = None                 # a cycle is not a hierarchy
        entries = _bca(c, chds[name]) if name in chds else None
        if entries is None:
            return None
        bases = direct_bases(entries)
        result, seen = [], set()
        own_slot = any(off == 0 for off, _rva in offsets.get(name, ()))
        primary = False
        for base, virtual, _mdisp, _sub in bases:
            sub = paths(base)
            if sub is None:
                return None
            if not virtual and any(not p["vbases"] for p in sub):
                primary = True
        if own_slot and not primary:
            result.append({"path": [], "next": name, "vbases": [], "nv": 0})
        for base, virtual, mdisp, sub_entries in bases:
            for p in paths(base):
                if seen & set(p["vbases"]):
                    continue
                q = {"path": list(p["path"]), "next": p["next"],
                     "vbases": list(p["vbases"]), "nv": p["nv"]}
                if not q["path"] or q["path"][-1] != base:
                    q["next"] = base
                if virtual:
                    q["vbases"].append(base)
                elif not q["vbases"]:
                    q["nv"] += mdisp
                result.append(q)
            if virtual:
                seen.add(base)
            seen.update(n for n, _c, _m, pdisp in sub_entries[1:] if pdisp != -1)
        changed = True
        while changed:
            changed = False
            groups = {}
            for p in result:
                groups.setdefault(tuple(p["path"]), []).append(p)
            for group in groups.values():
                if len(group) < 2:
                    continue
                for p in group:
                    if p["next"]:
                        p["path"].append(p["next"])
                        p["next"] = None
                        changed = True
        memo[name] = result
        return result

    out = {}
    for name, tables in offsets.items():
        entries = _bca(c, chds[name])
        if not entries or not any(pdisp != -1 for _n, _c, _m, pdisp in entries[1:]):
            continue
        found = paths(name)
        if not found or len(found) != len(tables):
            continue
        direct = [p for p in found if not p["vbases"]]
        shared = [p for p in found if p["vbases"]]
        by_offset = {p["nv"]: p for p in direct}
        if len(by_offset) != len(direct):
            continue
        rest = sorted(t for t in tables if t[0] not in by_offset)
        if len(rest) != len(shared):
            continue
        named = {rva: by_offset[off] for off, rva in tables if off in by_offset}
        named.update((rva, p) for (_off, rva), p in zip(rest, shared))
        if len(named) != len(tables):
            continue
        for rva, p in named.items():
            out[rva] = (name if len(found) == 1 and not p["path"]
                        else _mangled_vftable(name, p["path"]))
    return out


def census(c, starts):
    image, base = c.image, c.image.image_base
    rdata = next(s for s in image.sections if s.name == ".rdata")
    lo, hi = rdata.rva, rdata.rva + rdata.size
    found = {}
    cols = []
    for rva in range(lo, hi - 4, 4):
        v = c.dword(rva)
        if v is None or not lo <= v - base < hi:
            continue
        col = locator(c, v - base)
        if col is None:
            continue
        first = c.dword(rva + 4)
        if first is None or (first - base) not in starts:
            continue
        cols.append((rva + 4, col))
    # A class whose tables sit at several non-virtual base offsets
    # qualifies every one, its primary too, by the base at that offset
    # (MSVC's `??_7T16bppDIBSection@@6B?$T16bppBitmapBase@K@@@`).
    def named_base(chd, offset):
        bases = [n for n, m, virtual in base_names(c, chd, True)[1:]
                 if m == offset and n and not virtual]
        return bases[0] if bases else None
    several = {col[2] for _rva, col in cols if col[0] and named_base(col[3], col[0])}
    virtual_names = _virtual_vftable_names(c, cols)
    for rva, (offset, _cd, name, chd) in cols:
        cls = name
        if rva in virtual_names:
            cls = virtual_names[rva]
        elif offset:
            bases = [n for n, m in base_names(c, chd)[1:] if m == offset and n]
            cls = f"??_7{name}@@6B{bases[0]}@@@" if bases else ""
        elif name in several and named_base(chd, 0):
            cls = f"??_7{name}@@6B{named_base(chd, 0)}@@@"
        found[rva] = cls
    # vptr stores: `mov dword ptr [reg(+d)], offset VT`
    for s0, seen in c.reached.items():
        for r in seen:
            ins = c.insn(r)
            if ins.mnemonic != "mov" or len(ins.operands) != 2:
                continue
            dst, src = ins.operands
            if dst.type != 3 or src.type != 2:      # MEM, IMM
                continue
            t = (src.imm & 0xFFFFFFFF) - base
            if lo <= t < hi and t not in found and (c.dword(t) or 0) - base in starts:
                found[t] = ""
    rows = []
    order = sorted(found)
    for i, rva in enumerate(order):
        stop = order[i + 1] if i + 1 < len(order) else hi
        n = 0
        while rva + 4 * n < stop:
            v = c.dword(rva + 4 * n)
            if v is None or (v - base) not in starts:
                break
            n += 1
        rows.append((rva, n, found[rva]))
    return rows
