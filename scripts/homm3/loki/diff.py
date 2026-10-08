"""Per-function listings of the Loki comparison objects, and a base/target diff.

Both sides are rendered from the canonical form objdiff scores
(homm3.loki.cmpobj): every relocated field prints as its symbol and addend,
and branches inside the function print as function-relative offsets, so the
listing differs exactly where the comparison does.
"""
from __future__ import annotations

import difflib

import capstone

from homm3.loki.cmpobj import CodeSection, Function, canonical_symbol
from homm3.loki.elf import R_386_PC32

_DECODER = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
_DECODER.syntax = capstone.CS_OPT_SYNTAX_ATT
_DECODER.detail = True


def find(sections: list[CodeSection], selector: str) -> list[tuple[CodeSection, Function]]:
    exact, partial = [], []
    for section in sections:
        for function in section.functions:
            name = canonical_symbol(function.name)
            if selector in (function.name, name):
                exact.append((section, function))
            elif selector in name:
                partial.append((section, function))
    return exact or partial


def render(section: CodeSection, function: Function) -> list[str]:
    relocs = {r.offset: r for r in section.relocs}
    start, end = function.offset, function.offset + function.size
    lines = []
    for insn in _DECODER.disasm(bytes(section.data[start:end]), start):
        text = f"{insn.mnemonic:7} {insn.op_str}"
        names = []
        for offset in range(insn.address, insn.address + insn.size):
            reloc = relocs.get(offset)
            if reloc is None:
                continue
            addend = reloc.addend + 4 if reloc.type == R_386_PC32 else reloc.addend
            names.append(canonical_symbol(reloc.target) + (f"{addend:+#x}" if addend else ""))
        if names and reloc_is_branch(insn):
            text = f"{insn.mnemonic:7} <{names[0]}>"
        elif names:
            text += "  ; " + ", ".join(f"<{n}>" for n in names)
        elif reloc_is_branch(insn) and insn.operands and insn.operands[0].type == capstone.x86.X86_OP_IMM:
            text = f"{insn.mnemonic:7} .{insn.operands[0].imm - start:+#x}"
        lines.append((insn.address - start, text))
    return [f"{offset:5x}  {text}" for offset, text in lines]


def reloc_is_branch(insn) -> bool:
    return insn.group(capstone.CS_GRP_JUMP) or insn.group(capstone.CS_GRP_CALL)


def side_by_side(base: list[str], target: list[str], width: int = 60) -> list[str]:
    strip = [line[7:] for line in base], [line[7:] for line in target]
    out = []
    matcher = difflib.SequenceMatcher(a=strip[0], b=strip[1], autojunk=False)
    for tag, a0, a1, b0, b1 in matcher.get_opcodes():
        for k in range(max(a1 - a0, b1 - b0)):
            left = base[a0 + k] if a0 + k < a1 else ""
            right = target[b0 + k] if b0 + k < b1 else ""
            mark = " " if tag == "equal" else "|" if left and right else "<" if left else ">"
            out.append(f"{left[:width]:{width}} {mark} {right[:width]}")
    return out
