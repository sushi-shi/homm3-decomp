"""homm3.census.eh - funclet parentage and `.CRT$XCU` initializer slots."""
from __future__ import annotations

from collections import Counter

from capstone import x86


def funclet_rows(c):
    """[(funclet rva, parent function rva, try state or unwind index)].

    A function with C++ EH loads its `__ehhandler$` stub (`mov eax, offset
    FuncInfo; jmp ___CxxFrameHandler`) into eax before its frame setup; the
    stub's FuncInfo lists the parent's unwind actions and catch handlers.
    The census keeps the stubs apart from its starts (`Census.stubs`)."""
    base = c.image.image_base
    stub_info = dict(c.stubs)             # never starts (homm3.census.functions)
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
            # an `/O1` slot holds `jmp $+5`: the initializer body follows it
            body = v - base + 5
            if c.blob[v - base - c.text_lo:v - base - c.text_lo + 5] == JMP0 \
                    and body in c.starts:
                rows.append((body, slot))
    return rows


JMP0 = b"\xe9\x00\x00\x00\x00"


def cleanup_rows(c, initializers, atexit):
    """[(cleanup rva, "-")]: the functions the initializers register with
    `_atexit` (`push offset cleanup` with `call _atexit` the next call), the
    compiler's static destructors; the game's table admits them as slot -."""
    base = c.image.image_base
    out = set()
    for start in initializers:
        order = sorted(c.reached.get(start, ()))
        for k, r in enumerate(order):
            ins = c.insn(r)
            if ins.mnemonic != "push" or ins.operands[0].type != x86.X86_OP_IMM:
                continue
            target = (ins.operands[0].imm & 0xFFFFFFFF) - base
            if target not in c.starts or target in initializers:
                continue
            call = next((c.insn(q) for q in order[k + 1:] if c.insn(q).mnemonic == "call"), None)
            if call is not None and call.operands[0].type == x86.X86_OP_IMM \
                    and (call.operands[0].imm & 0xFFFFFFFF) - base == atexit:
                out.add(target)
    return [(rva, "-") for rva in sorted(out)]
