"""The Loki Linux game 1.3.1a `heroes3.dynamic` (GCC 2.95.2 ELF i386): pinned bytes,
sections and the address bands the census assigns functions to."""
from __future__ import annotations

from functools import cached_property

from homm3.core import common
from homm3.core.project import Project
from homm3.loki.elf import Elf

INPUT = "loki_heroes3"
RETAIL = common.HOMM3_DIR / "config/retail/heroes3-loki"
BUILD = common.HOMM3_DIR / "build/heroes3-loki"


def executable():
    return Project(common.HOMM3_DIR).executable(INPUT)


def stage(source=None):
    from homm3.core import inputs
    return inputs.stage_executable(executable(), source)


def read_bytes() -> bytes:
    from homm3.core import inputs
    exe = executable()
    return inputs.read_verified(exe, inputs.stage_executable(exe))


class LokiGame:
    def __init__(self, data: bytes | None = None):
        self.elf = Elf(data if data is not None else read_bytes())

    @cached_property
    def plt(self) -> dict[int, str]:
        return {address: name.split("@")[0] for address, name in self.elf.plt_slots.items()}

    @cached_property
    def exported(self) -> dict[str, int]:
        return {s.name: s.value for s in self.elf.dynsym if s.shndx and s.value}

    @cached_property
    def library_start(self) -> int:
        """Lowest frame-described function: only libstdc++/libgcc objects were
        compiled with exceptions, so the first FDE opens the C++ runtime band."""
        from homm3.loki.ehframe import frames
        _, fdes = frames(self.elf)
        return min(f.start for f in fdes if f.start)

    @cached_property
    def linkonce_start(self) -> int:
        """ld 2.9.x places every kept .gnu.linkonce.t body after all .text; libgcc's
        __umoddi3 (exported) is the last .text function."""
        address = self.exported["__umoddi3"]
        for symbol in self.elf.dynsym:
            if symbol.name == "__umoddi3":
                return (address + symbol.size + 15) & ~15
        raise KeyError("__umoddi3")

    def band(self, address: int) -> str:
        text = self.elf.section(".text")
        if not text.contains(address):
            return self.elf.section_at(address).name if self.elf.section_at(address) else "?"
        if address >= self.linkonce_start:
            return "linkonce"
        if address >= self.library_start:
            return "runtime"
        return "text"

    def read(self, address: int, size: int) -> bytes:
        return self.elf.read(address, size)

    def cstr(self, address: int, limit: int = 512) -> bytes | None:
        try:
            raw = self.elf.read(address, limit)
        except Exception:
            return None
        end = raw.find(b"\0")
        return raw[:end] if end >= 0 else None
