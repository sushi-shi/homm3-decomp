"""Compare a function compiled at the Loki game profile with its Loki body.

The image is stripped and linked, the object relocatable, so both are put in
one normal form before they are compared:

- a field the object relocates, and a 32-bit field of the image that holds an
  address inside the image, become the same sentinel;
- a rel32 call or jump leaving the function becomes `call .+5`;
- branches inside the function are disassembled from offset 0 on both sides.

`exact` means the normal forms are byte-identical (the scan's criterion: only
link-decided fields open). The score is the instruction-level similarity of
the two listings (difflib ratio, 0..100). Call targets are listed beside the
listing on both sides (object symbols demangled, image targets named by their
Windows pair), so a call-set difference is visible even where bytes agree.
"""
from __future__ import annotations

import difflib
import re
import struct
import subprocess
from dataclasses import dataclass, field
from functools import lru_cache
from pathlib import Path

import capstone

from homm3.loki.elf import Elf, R_386_PC32, SHF_ALLOC, SHT_REL, STT_FUNC, STT_SECTION
from homm3.loki_game.image import LokiGame

SENTINEL = 0x7EADBEE0

_DECODER = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
_DECODER.detail = True
_TEXT = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
_TEXT.syntax = capstone.CS_OPT_SYNTAX_ATT


@dataclass
class Body:
    name: str
    code: bytes                       # normal form
    refs: dict[int, str] = field(default_factory=dict)   # field offset -> referent name

    def listing(self) -> list[tuple[int, str, str]]:
        """(offset, instruction, referents named in it)."""
        out = []
        for insn in _TEXT.disasm(self.code, 0):
            names = [self.refs[o] for o in range(insn.address, insn.address + insn.size) if o in self.refs]
            out.append((insn.address, f"{insn.mnemonic} {insn.op_str}".strip(), ", ".join(names)))
        return out


def _patch(code: bytearray, offset: int, value: int) -> None:
    code[offset:offset + 4] = struct.pack("<I", value & 0xFFFFFFFF)


# --- object side -----------------------------------------------------------

@dataclass
class ObjectFunction:
    symbol: str
    body: Body


def object_functions(path: Path) -> list[ObjectFunction]:
    obj = Elf(path.read_bytes())
    symtab = next(s for s in obj.sections if s.type == 2)
    symbols = obj.symbols(symtab)
    out = []
    for section in obj.sections:
        if not (section.name.startswith(".text") or section.name.startswith(".gnu.linkonce.t")):
            continue
        data = obj.bytes(section)
        relocs = {}
        for rel in obj.sections:
            if rel.type == SHT_REL and rel.info == section.index:
                for r in obj.rels(rel):
                    target = symbols[r.symbol]
                    if target.type == STT_SECTION:
                        (addend,) = struct.unpack_from("<i", data, r.offset)
                        name = f"{obj.sections[target.shndx].name}+{addend:#x}"
                        name = _section_referent(obj, target.shndx, addend) or name
                    else:
                        name = target.name
                    relocs[r.offset] = (r.type, name)
        for symbol in symbols:
            if symbol.shndx != section.index or symbol.type != STT_FUNC or not symbol.size:
                continue
            start, end = symbol.value, symbol.value + symbol.size
            code = bytearray(data[start:end])
            refs = {}
            for offset, (kind, name) in relocs.items():
                if not start <= offset < end:
                    continue
                local = offset - start
                if kind == R_386_PC32:
                    _patch(code, local, 0)
                else:
                    _patch(code, local, SENTINEL)
                refs[local] = name
            for insn in _DECODER.disasm(bytes(code), 0):
                if insn.bytes[0] in (0xE8, 0xE9) and insn.size == 5:
                    (rel32,) = struct.unpack_from("<i", code, insn.address + 1)
                    target = insn.address + 5 + rel32
                    if not 0 <= target < len(code) and insn.address + 1 not in refs:
                        refs[insn.address + 1] = _local_symbol(symbols, section.index, start + target)
                        _patch(code, insn.address + 1, 0)
            out.append(ObjectFunction(symbol.name, Body(symbol.name, bytes(code), refs)))
    return out


def _local_symbol(symbols, shndx: int, value: int) -> str:
    for s in symbols:
        if s.shndx == shndx and s.value == value and s.type == STT_FUNC:
            return s.name
    return f"local+{value:#x}"


def _section_referent(obj: Elf, shndx: int, addend: int) -> str | None:
    section = obj.sections[shndx]
    if section.name.startswith(".rodata") and 0 <= addend < section.size:
        raw = obj.bytes(section)[addend:addend + 80]
        end = raw.find(b"\0")
        if end > 0 and all(32 <= c < 127 for c in raw[:end]):
            return repr(raw[:end].decode())
    return None


# --- image side ------------------------------------------------------------

class Image:
    def __init__(self):
        self.game = LokiGame()
        elf = self.game.elf
        loaded = [s for s in elf.sections if s.flags & SHF_ALLOC and s.addr]
        self.low = min(s.addr for s in loaded)
        self.high = max(s.addr + s.size for s in loaded)

    def body(self, address: int, size: int, name_of) -> Body:
        code = bytearray(self.game.read(address, size))
        refs = {}
        for insn in _DECODER.disasm(bytes(code), 0):
            if insn.bytes[0] in (0xE8, 0xE9) and insn.size == 5:
                (rel32,) = struct.unpack_from("<i", code, insn.address + 1)
                target = insn.address + 5 + rel32
                if not 0 <= target < size:
                    refs[insn.address + 1] = name_of(address + target)
                    _patch(code, insn.address + 1, 0)
                continue
            for where, width in ((insn.disp_offset, insn.disp_size), (insn.imm_offset, insn.imm_size)):
                if not where or width != 4:
                    continue
                (value,) = struct.unpack_from("<I", code, insn.address + where)
                if self.low <= value < self.high:
                    refs[insn.address + where] = name_of(value)
                    _patch(code, insn.address + where, SENTINEL)
        return Body(f"0x{address:08x}", bytes(code), refs)


PADDING = (b"\x90", b"\x89\xf6", b"\x8d\x76\x00", b"\x8d\x74\x26\x00",
           b"\x8d\xb6\x00\x00\x00\x00", b"\x8d\xb4\x26\x00\x00\x00\x00",
           b"\x8d\xbc\x27\x00\x00\x00\x00", b"\x8d\x74\x26\x00\x90")


def strip_padding(code: bytes, minimum: int) -> int:
    """Length of `code` without the alignment fill gas put after its last
    instruction (never below `minimum`)."""
    end = len(code)
    while end > minimum:
        for fill in PADDING:
            if end - len(fill) >= minimum and code[end - len(fill):end] == fill:
                end -= len(fill)
                break
        else:
            break
    return end


# --- names -----------------------------------------------------------------

@lru_cache(maxsize=1)
def _toolchain_cxxfilt() -> list[str]:
    from homm3.loki import toolchain
    return [str(toolchain.LOADER), "--library-path",
            f"{toolchain.SYSROOT}/lib:{toolchain.SYSROOT}/usr/lib",
            str(toolchain.SYSROOT / "usr/bin/c++filt")]


def gnu_demangle(names: list[str]) -> dict[str, str]:
    """GCC 2.95 (GNU v2 ABI) names through the era's own c++filt."""
    from homm3.loki import toolchain
    if not names:
        return {}
    done = subprocess.run(_toolchain_cxxfilt(), input="\n".join(names) + "\n",
                          capture_output=True, text=True, env=toolchain.environment())
    return dict(zip(names, done.stdout.splitlines()))


_ARGS = re.compile(r"\(.*\)( const)?$")


def qualified(demangled: str) -> str:
    return _ARGS.sub("", demangled).strip()


def arity(demangled: str) -> int:
    m = re.search(r"\((.*)\)( const)?$", demangled)
    if not m or not m.group(1).strip() or m.group(1).strip() == "void":
        return 0
    depth, count = 0, 1
    for c in m.group(1):
        if c in "<(":
            depth += 1
        elif c in ">)":
            depth -= 1
        elif c == "," and depth == 0:
            count += 1
    return count


def side_by_side(left: Body, right: Body, width: int = 64) -> list[str]:
    a, b = left.listing(), right.listing()
    ta, tb = [t for _, t, _ in a], [t for _, t, _ in b]
    out = []
    matcher = difflib.SequenceMatcher(a=ta, b=tb, autojunk=False)
    for tag, a0, a1, b0, b1 in matcher.get_opcodes():
        for k in range(max(a1 - a0, b1 - b0)):
            def cell(rows, i, hi):
                if i >= hi:
                    return ""
                offset, text, names = rows[i]
                return f"{offset:4x} {text}" + (f"  <{names}>" if names else "")
            l, r = cell(a, a0 + k, a1), cell(b, b0 + k, b1)
            mark = " " if tag == "equal" else "|" if l and r else "<" if l else ">"
            out.append(f"{l[:width]:{width}} {mark} {r[:width]}")
    return out


def score(left: Body, right: Body) -> float:
    ta = [t for _, t, _ in left.listing()]
    tb = [t for _, t, _ in right.listing()]
    if not ta and not tb:
        return 100.0
    return 100.0 * difflib.SequenceMatcher(a=ta, b=tb, autojunk=False).ratio()
