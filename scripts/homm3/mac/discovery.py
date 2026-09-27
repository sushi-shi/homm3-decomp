"""Mac pairing leads from code references, loader pointers and literal bytes.

Unreviewed code-section scans are discovery leads, not admitted function
boundaries or an exact function census.
"""
from __future__ import annotations

from collections import Counter
import re

from homm3.mac.loader import Loader
from homm3.mac.pef import PEF, PEFError
from homm3.mac.relocations import Address


def parse_address(value: str) -> Address:
    match = re.fullmatch(r"mac:(?:(\d+):)?(0x[0-9a-fA-F]+|\d+)", value)
    if match is None:
        raise ValueError("use a Mac section address: mac:<section>:<offset>")
    return Address(int(match.group(1) or 0), int(match.group(2), 0))


class Index:
    def __init__(self, pef: PEF):
        self.pef = pef
        self.loader = Loader(pef)
        self.toc = self.loader.toc()
        self.branches = []
        self.toc_loads = []
        self.memory = []
        self.census = Counter()
        for section in pef.sections[:pef.instantiated]:
            if section.kind != 0:
                continue
            code = pef.contents(section.index)
            self.census["code_bytes"] += len(code)
            for offset in range(0, len(code) - 3, 4):
                word = int.from_bytes(code[offset:offset + 4], "big")
                op, link = word >> 26, bool(word & 1)
                at = Address(section.index, offset)
                if op in (16, 18):
                    bits = 26 if op == 18 else 16
                    delta = word & (((1 << bits) - 1) & ~3)
                    if delta & (1 << (bits - 1)):
                        delta -= 1 << bits
                    if word & 2:
                        if link:
                            self.census["absolute_link_instructions"] += 1
                        continue
                    target = offset + delta
                    if not 0 <= target < len(code):
                        if link:
                            self.census["out_of_section_link_patterns"] += 1
                        continue
                    kind = "linked_branch" if link else "branch"
                    if link and target == offset + 4:
                        kind = "local_pc_link"
                    self.branches.append((at, Address(section.index, target), kind))
                    self.census[kind + "_sites"] += 1
                elif op == 19 and link and ((word >> 1) & 0x3ff) in (16, 528):
                    self.census["indirect_link_instructions"] += 1
                if 32 <= op <= 55:
                    displacement = word & 0xffff
                    if displacement & 0x8000:
                        displacement -= 0x10000
                    base = (word >> 16) & 31
                    self.memory.append((at, base, displacement))
                    if base == 2:
                        target = Address(self.toc.section, self.toc.offset + displacement)
                        self.toc_loads.append((at, target))
                elif op == 14 and (word >> 16) & 31 == 2:
                    displacement = word & 0xffff
                    if displacement & 0x8000:
                        displacement -= 0x10000
                    self.toc_loads.append((at, Address(self.toc.section, self.toc.offset + displacement)))
        self.census["distinct_link_destinations"] = len({
            target for _, target, kind in self.branches if kind == "linked_branch"})

    def xrefs(self, target: Address) -> dict:
        self.pef.read(target.section, target.offset, 1)
        pointers = [at for at, destination in self.loader.pointers.items() if destination == target]
        pointer_set = set(pointers)
        return {
            "target": {"section": target.section, "offset": target.offset},
            "code_branches": [{"section": at.section, "offset": at.offset, "kind": kind}
                              for at, destination, kind in self.branches if destination == target],
            "loader_pointers": [{"section": at.section, "offset": at.offset} for at in pointers],
            "toc_uses": [{"section": at.section, "offset": at.offset,
                          "via_pointer": destination in pointer_set,
                          "toc_section": destination.section, "toc_offset": destination.offset}
                         for at, destination in self.toc_loads
                         if destination == target or destination in pointer_set]}

    def find_bytes(self, pattern: bytes, section: int | None = None) -> list[Address]:
        if not pattern:
            raise ValueError("empty byte/string searches are not allowed")
        if section is not None:
            self.pef.section(section)
            if section >= self.pef.instantiated:
                raise PEFError("search section must be instantiated code/data")
        result = []
        for info in self.pef.sections[:self.pef.instantiated]:
            if section is not None and info.index != section:
                continue
            data = self.pef.contents(info.index)
            offset = data.find(pattern)
            while offset >= 0:
                result.append(Address(info.index, offset))
                offset = data.find(pattern, offset + 1)
        return result

    def find_fields(self, displacements: list[int], window: int = 256) -> list[dict]:
        """Find windows containing each requested non-stack/non-TOC displacement."""
        wanted = set(displacements)
        if not wanted or window <= 0 or any(not -32768 <= n <= 32767 for n in wanted):
            raise ValueError("field searches need signed-16-bit offsets and a positive window")
        sites = [(at, base, offset) for at, base, offset in self.memory
                 if offset in wanted and base not in (0, 1, 2)]
        result = []
        last_end = {}
        for first, (at, _, _) in enumerate(sites):
            if at.offset < last_end.get(at.section, -1):
                continue
            hits = []
            observed = set()
            for index in range(first, len(sites)):
                location, base, displacement = sites[index]
                if location.section != at.section or location.offset - at.offset >= window:
                    break
                hits.append({"offset": location.offset, "base_register": base, "field_offset": displacement})
                observed.add(displacement)
                if observed == wanted:
                    end = location.offset + 4
                    result.append({"section": at.section, "start": at.offset, "end": end, "hits": hits})
                    last_end[at.section] = end
                    break
        return result
