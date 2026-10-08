"""homm3.census.eh - funclet parentage and `.CRT$XCU` initializer slots."""
from __future__ import annotations

from collections import Counter

from capstone import x86


def funclet_rows(c):
    """[(funclet rva, parent function rva, try state or unwind index)].

    A function with C++ EH loads its `__ehhandler$` stub (`mov eax, offset
    FuncInfo; jmp ___CxxFrameHandler`) into eax before its frame setup; the
    stub's FuncInfo lists the parent's unwind actions and catch handlers."""
    base = c.image.image_base
    stub_info = {}
    for start in c.starts:
        ins = c.insn(start)
        nxt = c.insn(start + ins.size) if ins else None
        if ins and nxt and ins.mnemonic == "mov" and nxt.mnemonic == "jmp" \
                and len(ins.operands) == 2 and ins.operands[1].type == x86.X86_OP_IMM:
            info = (ins.operands[1].imm & 0xFFFFFFFF) - base
            if info in c.funcinfo:
                stub_info[start] = info
    parent_of = {}
    for start, seen in c.reached.items():
        if start in stub_info or start in c.funclets:
            continue
        for r in seen:
            ins = c.insn(r)
            if ins.mnemonic in ("mov", "push") and ins.operands \
                    and ins.operands[-1].type == x86.X86_OP_IMM:
                target = (ins.operands[-1].imm & 0xFFFFFFFF) - base
                if target in stub_info:
                    parent_of.setdefault(stub_info[target], start)
    rows = []
    for rva, (info, kind, state) in sorted(c.funclets.items()):
        parent = parent_of.get(info)
        if parent is not None:
            rows.append((rva, parent, state))
    return rows, len(c.funclets) - len(rows)


def initializer_rows(c):
    """[(thunk rva, slot)] of the `.CRT$XCU` table: the larger of the two
    tables the CRT passes to the same `_initterm` (push end; push start; call)."""
    base = c.image.image_base
    data = next(s for s in c.image.sections if s.name == ".data")
    lo, hi = data.rva, data.rva + data.size
    pairs = Counter()
    sites = []
    for start, seen in c.reached.items():
        order = sorted(seen)
        for a, b, d in zip(order, order[1:], order[2:]):
            i1, i2, i3 = c.insn(a), c.insn(b), c.insn(d)
            if not (i1.mnemonic == i2.mnemonic == "push" and i3.mnemonic == "call"):
                continue
            if a + i1.size != b or b + i2.size != d:
                continue
            o1, o2, o3 = i1.operands[0], i2.operands[0], i3.operands[0]
            if not (o1.type == o2.type == o3.type == x86.X86_OP_IMM):
                continue
            end, begin = (o1.imm & 0xFFFFFFFF) - base, (o2.imm & 0xFFFFFFFF) - base
            if lo <= begin < end < hi:
                callee = (o3.imm & 0xFFFFFFFF) - base
                pairs[callee] += 1
                sites.append((callee, begin, end))
    if not pairs:
        return []
    initterm = pairs.most_common(1)[0][0]
    tables = sorted({(b, e) for t, b, e in sites if t == initterm}, key=lambda be: be[1] - be[0])
    begin, end = tables[-1]
    rows = []
    for slot, at in enumerate(range(begin, end, 4)):
        v = c.dword(at)
        if v and c.in_text(v - base):
            rows.append((v - base, slot))
    return rows
