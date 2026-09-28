"""Gruntz's shared relocation-address resolver for data verification.

All candidates are retained when a name is ambiguous. Consumers must require
one address before attributing a referent; no closest-address fallback.
"""
from __future__ import annotations
import struct
from homm3.walls.pairscan import DIR32, REL32, canon

class Resolver:
    def __init__(self):
        from homm3.model import resolve
        from homm3.sema.image import retail
        self.img = retail()
        m = resolve()
        self.model = m
        names: dict[str, set[int]] = {}
        for b in m.functions + m.data:
            if b.name:
                names.setdefault(canon(b.name), set()).add(b.rva)
            for a in b.aliases:
                if a.name:
                    names.setdefault(canon(a.name), set()).add(b.rva)
        # underscore-tolerant lookups for C-decorated spellings
        self.names = names
        self.fn_extent = {b.rva: b.size for b in m.functions}
        self.fn_of_name = {b.name: b.rva for b in m.functions if b.name}
        self.units = {}
        for b in m.functions:
            if b.name and b.unit:
                self.units.setdefault(b.unit, {})[b.name] = b.rva

    def chase(self, rva: int, depth: int = 0) -> int:
        while depth < 4:
            nxt = self.img.jmp_target(rva)
            if nxt is None:
                return rva
            rva = nxt
            depth += 1
        return rva

    def rva_of(self, name: str) -> set[int]:
        c = canon(name)
        hits = self.names.get(c)
        if hits:
            return hits
        if c.startswith("_"):
            hits = self.names.get(c[1:])
            if hits:
                return hits
        return self.names.get("_" + c, set())

    def resolve_base(self, name: str, typ: int, addend: int) -> set[int]:
        rvas = self.rva_of(name)
        if not rvas:
            return set()
        if typ == REL32:
            return {self.chase(r) for r in rvas}
        return {self.chase((r + addend) & 0xFFFFFFFF) for r in rvas}

    def retail_value(self, site_rva: int, typ: int) -> int | None:
        if typ == REL32:
            b = self.img.read(site_rva, 4)
            if b is None:
                return None
            disp = struct.unpack("<i", b)[0]
            return self.chase((site_rva + 4 + disp) & 0xFFFFFFFF)
        v = self.img.u32(site_rva)
        if v is None or not (self.img.base <= v < self.img.base + 0x400000):
            return None
        return self.chase(v - self.img.base)
