"""Comparison objects for the Loki image: one canonical form for base and target.

objdiff pairs functions by symbol name and judges a relocation by its target
name and addend. GCC 2.95 objects and the linked ELF spell the same reference
differently (section-relative .rodata, resolved calls to file-static
functions, absolute addresses), so both sides are rewritten into the same
relocatable form before comparison:

- every rel32 call/jump that leaves its function, and every absolute address,
  is a relocation against a named symbol, with the addend held in the field;
- a string or constant in .rodata is named by its content, so the identity of
  each literal is checked without depending on the object's .rodata layout;
- a jump table is named by its function and ordinal (`<function>$jt<n>`);
- data is named by its symbol (exported, or a file-static local in the base).

Nothing is masked: an unresolved reference keeps a distinct name and differs.
"""
from __future__ import annotations

from dataclasses import dataclass, field
import hashlib
import struct

import capstone


_DECODER = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
_DECODER.detail = True
_REL32_BRANCH = {"call", "jmp"} | {f"j{c}" for c in (
    "o", "no", "b", "ae", "e", "ne", "be", "a", "s", "ns", "p", "np", "l", "ge", "le", "g")}


@dataclass
class Function:
    name: str
    offset: int
    size: int
    bind_global: bool
    address: int | None = None   # retail address (target side only)


@dataclass
class Reloc:
    offset: int
    type: int
    target: str
    addend: int


@dataclass
class CodeSection:
    name: str
    data: bytearray
    functions: list[Function] = field(default_factory=list)
    relocs: list[Reloc] = field(default_factory=list)


def canonical_symbol(name: str) -> str:
    """Erase what the build environment put into a mangled name.

    g++ 2.95 names an anonymous namespace `_GLOBAL_.N.<input file><6 chars>`:
    the input file as given on the command line plus append_random_chars()
    drawn from gettimeofday and the pid. Both vary between our compile and
    Loki's, so the whole length-prefixed component becomes `5_ANON`.
    `_GLOBAL_.I.<first global>`/`_GLOBAL_.D.<...>` keep only their role."""
    for role in ("_GLOBAL_.I.", "_GLOBAL_.D."):
        if name.startswith(role):
            return role[:-1]
    out, index = [], 0
    while (found := name.find("_GLOBAL_.N.", index)) >= 0:
        digits = found
        while digits > index and name[digits - 1].isdigit():
            digits -= 1
        # `Q2` + `29_GLOBAL_...`: a single-digit qualifier count is not part
        # of the component's length.
        if digits > index and name[digits - 1] == "Q" and digits < found:
            digits += 1
        if digits == found:
            out.append(name[index:found + 11])
            index = found + 11
            continue
        length = int(name[digits:found])
        out.append(name[index:digits] + "5_ANON")
        index = found + length
    out.append(name[index:])
    return "".join(out)


def literal_name(content: bytes) -> str:
    digest = hashlib.sha1(content).hexdigest()[:8]
    body = content[:-1] if content.endswith(b"\0") else None
    if body is not None and body and all(32 <= c < 127 for c in body) and b"\0" not in body:
        text = body.decode("ascii")
        if len(text) > 40:
            text = text[:37] + "..."
        return f'$s"{text}"#{digest}'
    return f"$d{content.hex()}"


def c_string(read, address: int, limit: int = 4096) -> bytes:
    data = read(address, limit)
    end = data.find(b"\0")
    return data[:end + 1] if end >= 0 else data


@dataclass(frozen=True)
class Field:
    """A 4-byte address field inside one instruction."""
    offset: int          # section offset of the field
    kind: str            # "rel" (rel32 branch), "imm" or "disp"
    value: int           # raw little-endian field value
    end: int             # section offset just past the instruction
    access_size: int     # bytes read through a memory operand (0 for address-of)
    mnemonic: str


def fields(code: bytes, start: int, end: int):
    """Every 4-byte imm/disp field of the instructions in code[start:end]."""
    for insn in _DECODER.disasm(code[start:end], start):
        if insn.mnemonic in _REL32_BRANCH and insn.imm_size == 4:
            (value,) = struct.unpack_from("<i", code, insn.address + insn.imm_offset)
            yield Field(insn.address + insn.imm_offset, "rel", value, insn.address + insn.size, 0, insn.mnemonic)
            continue
        memory_size = 0
        for operand in insn.operands:
            if operand.type == capstone.x86.X86_OP_MEM:
                memory_size = operand.size
        if insn.disp_size == 4:
            (value,) = struct.unpack_from("<I", code, insn.address + insn.disp_offset)
            access = 0 if insn.mnemonic == "lea" else memory_size
            yield Field(insn.address + insn.disp_offset, "disp", value, insn.address + insn.size, access, insn.mnemonic)
        if insn.imm_size == 4:
            (value,) = struct.unpack_from("<I", code, insn.address + insn.imm_offset)
            yield Field(insn.address + insn.imm_offset, "imm", value, insn.address + insn.size, 0, insn.mnemonic)


def put(section: CodeSection, offset: int, rtype: int, target: str, addend: int) -> None:
    section.relocs.append(Reloc(offset, rtype, target, addend))
    struct.pack_into("<i", section.data, offset, addend)


def literal_for(read, address: int, access_size: int) -> str:
    """Name a .rodata reference by what the instruction uses: the bytes a
    memory operand loads, or the C string whose address is taken."""
    if access_size:
        return literal_name(read(address, access_size))
    return literal_name(c_string(read, address))
