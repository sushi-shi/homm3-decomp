"""The admitted Loki h3maped image: ELF bytes, exported names and frame-derived functions."""
from __future__ import annotations

from dataclasses import dataclass
from functools import cached_property
import bisect

from homm3.core import common
from homm3.core.project import Project
from homm3.loki.ehframe import frames
from homm3.loki.elf import Elf, STB_GLOBAL, STB_WEAK, STT_FUNC, STT_OBJECT

INPUT = "loki_h3maped"
IMAGE = "h3maped-loki"


def executable():
    return Project(common.HOMM3_DIR).executable(INPUT)


def read_bytes() -> bytes:
    from homm3.core import inputs
    exe = executable()
    return inputs.read_verified(exe, inputs.stage_executable(exe))


@dataclass(frozen=True)
class Function:
    address: int
    size: int
    obj: int          # compiled object ordinal (its .eh_frame CIE), -1 without a frame
    linkonce: bool    # a kept .gnu.linkonce.t copy (template/inline), placed after all .text
    bind: str         # global / weak / static
    name: str         # GNU v2 mangled name, or "" for a file-static function


class LokiImage:
    def __init__(self, data: bytes | None = None):
        self.elf = Elf(data if data is not None else read_bytes())

    @cached_property
    def exported(self) -> dict[int, list]:
        out: dict[int, list] = {}
        for symbol in self.elf.dynsym:
            if symbol.shndx and symbol.bind in (STB_GLOBAL, STB_WEAK) and symbol.type in (STT_FUNC, STT_OBJECT):
                out.setdefault(symbol.value, []).append(symbol)
        return out

    def function_symbol(self, address: int):
        for symbol in self.exported.get(address, ()):
            if symbol.type == STT_FUNC:
                return symbol
        return None

    @cached_property
    def linkonce_start(self) -> int:
        """First kept linkonce function: the default script places every
        .gnu.linkonce.t* input after every object's .text."""
        cies, fdes = frames(self.elf)
        weak = sorted(f.start for f in fdes if f.start
                      and (s := self.function_symbol(f.start)) is not None and s.bind == STB_WEAK)
        strong = [f.start for f in fdes if f.start
                  and (s := self.function_symbol(f.start)) is not None and s.bind == STB_GLOBAL]
        # libgcc/libstdc++ objects still define strong functions beside a few
        # weak ones; the linkonce block starts after the last strong body.
        last_strong = max(strong)
        return min(w for w in weak if w > last_strong)

    @cached_property
    def functions(self) -> list[Function]:
        cies, fdes = frames(self.elf)
        ordinal = {offset: index for index, offset in enumerate(cies)}
        out = []
        seen = set()
        for fde in fdes:
            if not fde.start or fde.start in seen:
                continue   # discarded linkonce duplicates keep a zero pc_begin
            seen.add(fde.start)
            symbol = self.function_symbol(fde.start)
            bind = "static" if symbol is None else "weak" if symbol.bind == STB_WEAK else "global"
            name = symbol.name if symbol else self.static_roles.get(fde.start, "")
            out.append(Function(fde.start, fde.size, ordinal[fde.cie], fde.start >= self.linkonce_start,
                                bind, name))
        text = self.elf.section(".text")
        for address, symbols in self.exported.items():
            symbol = next((s for s in symbols if s.type == STT_FUNC), None)
            if symbol is not None and text.contains(address) and address not in seen:
                seen.add(address)
                out.append(Function(address, symbol.size, -1, address >= self.linkonce_start,
                                    "weak" if symbol.bind == STB_WEAK else "global", symbol.name))
        out.sort(key=lambda f: f.address)
        return out

    @cached_property
    def static_roles(self) -> dict[int, str]:
        """Names of the compiler-generated file-static init/fini functions.

        g++ 2.95 emits `_GLOBAL_.I.<first global>` (listed in .ctors),
        `_GLOBAL_.D.<first global>` (in .dtors) and the
        `__static_initialization_and_destruction_0` both call. Only the
        roles are recoverable; homm3.loki.cmpobj drops the suffix on both sides."""
        import struct
        roles: dict[int, str] = {}
        for section, role in ((".ctors", "_GLOBAL_.I"), (".dtors", "_GLOBAL_.D")):
            raw = self.elf.bytes(self.elf.section(section))
            for (address,) in struct.iter_unpack("<I", raw):
                if address not in (0, 0xFFFFFFFF) and self.function_symbol(address) is None:
                    roles[address] = role
        for address, role in list(roles.items()):
            code = self.elf.read(address, 64)
            index = code.find(b"\xe8")
            if index >= 0:
                (displacement,) = struct.unpack_from("<i", code, index + 1)
                callee = address + index + 5 + displacement
                if self.function_symbol(callee) is None:
                    roles.setdefault(callee, "__static_initialization_and_destruction_0")
        return roles

    @cached_property
    def c_library_end(self) -> int:
        """End of the frameless C-library block (GTK+, glib, libglade, libxml,
        zlib) that the link places between the editor and libstdc++ objects."""
        frameless = [f for f in self.functions if f.obj < 0 and f.address < self.linkonce_start]
        runs, current = [], []
        for function in frameless:
            if current and function.address - (current[-1].address + current[-1].size) > 0x1000:
                runs.append(current)
                current = []
            current.append(function)
        runs.append(current)
        largest = max(runs, key=len)
        return largest[-1].address + largest[-1].size

    @cached_property
    def _starts(self) -> list[int]:
        return [f.address for f in self.functions]

    def function_at(self, address: int) -> Function | None:
        index = bisect.bisect_right(self._starts, address) - 1
        if index >= 0:
            function = self.functions[index]
            if address < function.address + max(function.size, 1):
                return function
        return None

    def object_at(self, address: int):
        """Exported data object containing address (vtables, type_info nodes, globals)."""
        best = None
        for start in self._object_starts[max(0, bisect.bisect_right(self._object_starts, address) - 4):
                                         bisect.bisect_right(self._object_starts, address)]:
            for symbol in self.exported[start]:
                if symbol.type == STT_OBJECT and start <= address < start + max(symbol.size, 1):
                    best = symbol
        return best

    @cached_property
    def _object_starts(self) -> list[int]:
        return sorted(a for a, symbols in self.exported.items() if any(s.type == STT_OBJECT for s in symbols))
