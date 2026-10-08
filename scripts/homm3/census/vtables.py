"""homm3.census.vtables - vtable starts, slot counts and RTTI class identities.

Under /GR the dword before each vtable points at its Complete Object
Locator, whose TypeDescriptor names the class (`.?AVFoo@Bar@@` -> `Foo@Bar`);
a secondary table (locator offset != 0) is named `??_7Derived@@6BBase@@@`
after the base class at that offset. Tables without a locator (libraries
built without /GR) are found where code stores them as a vptr. A table's
slots run while each dword is a census function start.
"""
from __future__ import annotations

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


def base_names(c, chd):
    """[(name, mdisp)] of a Class Hierarchy Descriptor's base classes."""
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
        out.append((name, struct.unpack("<i", struct.pack("<I", mdisp or 0))[0]))
    return out


def census(c, starts):
    image, base = c.image, c.image.image_base
    rdata = next(s for s in image.sections if s.name == ".rdata")
    lo, hi = rdata.rva, rdata.rva + rdata.size
    found = {}
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
        offset, _cd, name, chd = col
        cls = name
        if offset:
            bases = [n for n, m in base_names(c, chd)[1:] if m == offset and n]
            cls = f"??_7{name}@@6B{bases[0]}@@@" if bases else ""
        found[rva + 4] = cls
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
