"""GCC 2.95 RTTI of the Loki game: type-name strings -> `__tf` type_info functions
-> vtables (thunk layout: a zero word, the `__tf` function, then the virtual
functions in declaration order) -> virtual functions by slot."""
from __future__ import annotations

import re
import struct
from dataclasses import dataclass

from homm3.loki_game import census
from homm3.loki_game.image import LokiGame


@dataclass(frozen=True)
class Vtable:
    address: int      # the zero word before the __tf slot
    type_name: str    # GNU v2 encoding, e.g. "13TCastleWindow"
    tf: int
    slots: tuple[int, ...]

    @property
    def cls(self) -> str | None:
        m = re.fullmatch(r"(\d+)(.*)", self.type_name)
        return m.group(2) if m and int(m.group(1)) == len(m.group(2)) else None


def type_names(game: LokiGame) -> dict[int, str]:
    rodata = game.elf.section(".rodata")
    blob = game.elf.bytes(rodata)
    return {rodata.addr + m.start(1): m.group(1).decode()
            for m in re.finditer(rb"(?<=\0)([0-9QtP][A-Za-z0-9_]{1,200})\0", blob)}


def tf_functions(game: LokiGame) -> dict[int, str]:
    """Small functions that reference exactly one type-name string."""
    names = type_names(game)
    functions = census.functions()
    seen: dict[int, set[str]] = {}
    for fn, _, target in census.refs():
        if target in names:
            seen.setdefault(fn, set()).add(names[target])
    return {fn: next(iter(ns)) for fn, ns in seen.items()
            if len(ns) == 1 and fn in functions and functions[fn].size < 200}


def vtables(game: LokiGame | None = None) -> list[Vtable]:
    game = game or LokiGame()
    tfs = tf_functions(game)
    text, plt = game.elf.section(".text"), game.elf.section(".plt")
    out = []
    for name in (".data", ".rodata"):
        section = game.elf.section(name)
        blob = game.elf.bytes(section)
        for offset in range(4, len(blob) - 3, 4):
            (word,) = struct.unpack_from("<I", blob, offset)
            if word not in tfs or struct.unpack_from("<I", blob, offset - 4)[0] != 0:
                continue
            slots, at = [], offset + 4
            while at + 4 <= len(blob):
                (entry,) = struct.unpack_from("<I", blob, at)
                if not (text.contains(entry) or plt.contains(entry)) or entry in tfs:
                    break
                slots.append(entry)
                at += 4
            out.append(Vtable(section.addr + offset - 4, tfs[word], word, tuple(slots)))
    return out
