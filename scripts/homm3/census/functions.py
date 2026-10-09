"""homm3.census.functions - function starts and extents of a stripped VC6 image.

Starts come from recursive descent (capstone) over seeds of decreasing
strength, each later seed admitted only outside the bodies already decoded:

  1. the entry point, every FuncInfo unwind/catch funclet, the targets of
     direct calls and tail jumps, and the code after a zero-displacement
     `jmp` (VC6 never emits one inside a body: `$E` initializer thunks);
  2. code pointers in runs of data pointers (vtables, initializer and
     dispatch tables);
  3. code addresses decoded code loads as immediates (callbacks, atexit
     thunks, `__ehhandler` stubs), even inside a decoded body when they
     follow a call there (a call that never returns); a catch funclet's
     `mov eax, offset L; ret` instead names a continuation L of its parent,
     never a function;
  4. isolated data pointers into .text;
  5. the first non-padding byte after a body's decoded extent (unreferenced
     neighbours), repeated to a fixpoint.

A data or immediate seed that the final descent places inside another
function's instruction is dropped again.

Extents partition .text: a function runs to the next start minus trailing
NOP/INT3 padding, and always covers its decoded instructions, jump tables
and byte index tables.

A C++ EH registration stub (`mov eax, offset FuncInfo; jmp
___CxxFrameHandler`, ten bytes) is never a start: it closes its parent's
`.text$x` COMDAT after the unwind funclets, as in the game's census, so no
extent covers it and the parent's `offset stub` operand reads as the last
funclet plus its size.
"""
from __future__ import annotations

import struct
from collections import defaultdict
from dataclasses import dataclass, field

import capstone
from capstone import x86

PAD = {0x90, 0xCC}
#: `mov eax, imm32` + `jmp rel32`: the EH registration stub.
STUB_SIZE = 10


@dataclass
class Census:
    image: object
    text_lo: int = 0
    text_hi: int = 0
    starts: dict = field(default_factory=dict)       # rva -> evidence
    reached: dict = field(default_factory=dict)      # start -> set of insn rvas
    tables: dict = field(default_factory=dict)       # table rva -> (owner, size, kind)
    calls: dict = field(default_factory=lambda: defaultdict(set))
    extra: dict = field(default_factory=lambda: defaultdict(set))
    funclets: dict = field(default_factory=dict)     # rva -> (FuncInfo rva, kind, state)
    funcinfo: set = field(default_factory=set)       # FuncInfo record rvas
    continuations: dict = field(default_factory=dict)  # label -> catch funclet
    imms: set = field(default_factory=set)
    stubs: dict = field(default_factory=dict)        # EH registration stub -> FuncInfo

    def __post_init__(self):
        text = next(s for s in self.image.sections if s.executable)
        self.text = text
        self.text_lo, self.text_hi = text.rva, text.rva + text.size
        self.blob = self.image.blob(text)
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.md.detail = True
        self._cache = {}
        self._order = []
        self._order_n = -1

    # -- decoding ----------------------------------------------------------
    def in_text(self, rva):
        return self.text_lo <= rva < self.text_hi

    def stub_info(self, rva):
        """The FuncInfo an EH registration stub at `rva` loads, else None."""
        if rva in self.stubs:
            return self.stubs[rva]
        if not self.in_text(rva) or not self.in_text(rva + STUB_SIZE - 1):
            return None
        o = rva - self.text_lo
        if self.blob[o] != 0xB8 or self.blob[o + 5] != 0xE9:
            return None
        info = struct.unpack_from("<I", self.blob, o + 1)[0] - self.image.image_base
        return info if info in self.funcinfo else None

    def next_start(self, start):
        import bisect
        order = self._order
        if self._order_n != len(self.starts):
            order = self._order = sorted(self.starts)
            self._order_n = len(self.starts)
        i = bisect.bisect_right(order, start)
        return order[i] if i < len(order) else self.text_hi

    def insn(self, rva):
        hit = self._cache.get(rva)
        if hit is None:
            off = rva - self.text_lo
            got = next(self.md.disasm(self.blob[off:off + 16], self.image.image_base + rva, 1), None)
            hit = got if got is not None else False
            self._cache[rva] = hit
        return hit or None

    def dword(self, rva):
        sec = self.image.section_of(rva)
        if sec is None or rva + 4 > sec.rva + sec.size:
            return None
        off = sec.raw_offset + rva - sec.rva
        return struct.unpack_from("<I", self.image.data, off)[0]

    def byte(self, rva):
        sec = self.image.section_of(rva)
        if sec is None or rva >= sec.rva + sec.size:
            return None
        return self.image.data[sec.raw_offset + rva - sec.rva]

    # -- descent -------------------------------------------------------------
    def descend_into(self, owner, label):
        """Decode a continuation label as part of its owner's body."""
        self.starts[label] = "continuation"
        res = self.descend(label)
        del self.starts[label]
        if res is None:
            return False
        self.extra[owner] |= res[0]
        self.reached[owner] = self.reached.get(owner, set()) | res[0]
        for t, n, kind in res[1]:
            self.tables[t] = (owner, n, kind)
        return True

    def descend(self, start):
        """Decode one function by recursive descent. Returns (insns, table spans,
        call targets, tail targets, imm code pointers) or None if invalid."""
        base = self.image.image_base
        seen, work = set(), [start]
        spans, calls, tails, imms = [], set(), set(), set()
        ok = True
        cmp_bound = {}
        while work:
            rva = work.pop()
            while True:
                if rva in seen:
                    break
                if not self.in_text(rva):
                    ok = False
                    break
                if rva != start and rva in self.starts and rva not in seen:
                    # fell or jumped into another known function
                    tails.add(rva)
                    break
                ins = self.insn(rva)
                if ins is None:
                    ok = False
                    break
                seen.add(rva)
                nxt = rva + ins.size
                mn = ins.mnemonic
                ops = ins.operands
                for op in ops:
                    if op.type == x86.X86_OP_IMM:
                        v = op.imm & 0xFFFFFFFF
                        if mn not in ("call", "jmp") and not mn.startswith("j") \
                                and self.in_text(v - base):
                            imms.add(v - base)
                if mn == "cmp" and len(ops) == 2 and ops[0].type == x86.X86_OP_REG \
                        and ops[1].type == x86.X86_OP_IMM:
                    cmp_bound[ops[0].reg] = ops[1].imm
                if mn in ("ret", "retf", "iret", "int3", "hlt", "ud2"):
                    break
                if mn == "call":
                    if ops and ops[0].type == x86.X86_OP_IMM:
                        t = (ops[0].imm & 0xFFFFFFFF) - base
                        if self.in_text(t):
                            calls.add(t)
                        else:
                            ok = False
                            break
                    rva = nxt
                    continue
                if mn == "jmp":
                    op = ops[0]
                    if op.type == x86.X86_OP_IMM:
                        t = (op.imm & 0xFFFFFFFF) - base
                        if not self.in_text(t):
                            ok = False
                            break
                        limit = self.next_start(start)
                        if (t in self.starts and t != start) or rva == start \
                                or not start <= t < limit:
                            # a known start, a first-instruction thunk, or a
                            # jump out of [start, next start) is a tail call
                            tails.add(t)
                        else:
                            work.append(t)
                        break
                    if op.type == x86.X86_OP_MEM and op.mem.scale == 4 and op.mem.base == 0 \
                            and op.mem.index != 0:
                        table = (op.mem.disp & 0xFFFFFFFF) - base
                        if self.in_text(table):
                            n = self._table_count(table, cmp_bound.get(op.mem.index))
                            spans.append((table, 4 * n, "jump"))
                            work.extend(self.dword(table + 4 * i) - base for i in range(n))
                    break
                if mn.startswith("j") or mn in ("loop", "loope", "loopne", "jecxz", "jcxz"):
                    op = ops[0]
                    if op.type == x86.X86_OP_IMM:
                        work.append((op.imm & 0xFFFFFFFF) - base)
                    rva = nxt
                    continue
                # byte index tables of two-level switches
                if mn in ("mov", "movzx") and len(ops) == 2 and ops[1].type == x86.X86_OP_MEM \
                        and ops[1].size == 1 and ops[1].mem.base != 0 and ops[1].mem.index == 0:
                    disp = (ops[1].mem.disp & 0xFFFFFFFF) - base
                    if self.in_text(disp):
                        bound = cmp_bound.get(ops[1].mem.base)
                        if bound is not None and 0 <= bound < 0x10000:
                            spans.append((disp, bound + 1, "byte"))
                rva = nxt
        if not ok:
            return None
        return seen, spans, calls, tails, imms

    def _table_count(self, table, bound):
        """Entries of a jump table: up to the compared bound, and never past
        an entry outside .text. The bound is the last compare of the index
        register on any path, which an assembler routine need not have
        tested (LIBCMT memcpy: `jmp TrailUpVec[ecx*4+16]` after `sub ecx, 4`
        follows a `cmp ecx, 8` on another path)."""
        base = self.image.image_base
        cap = bound + 1 if bound is not None and 0 <= bound < 4096 else None
        n = 0
        while cap is None or n < cap:
            v = self.dword(table + 4 * n)
            if v is None or not self.in_text(v - base):
                break
            if n and (table + 4 * n) in self.tables:
                break
            n += 1
        return n


FUNCINFO_MAGIC = 0x19930520


def data_sections(image):
    return [s for s in image.sections if not s.executable and s.name not in (".rsrc", ".reloc")]


def funcinfo_funclets(census):
    """(funclet rva, FuncInfo rva, kind, state) from every FuncInfo record:
    an unwind action's state is its unwind-map index, a catch handler's the
    tryLow of its try block."""
    image, base = census.image, census.image.image_base
    out = []
    for sec in data_sections(image):
        blob = image.blob(sec)
        for off in range(0, len(blob) - 20, 4):
            if struct.unpack_from("<I", blob, off)[0] != FUNCINFO_MAGIC:
                continue
            rva = sec.rva + off
            max_state, p_unwind, n_try, p_try = struct.unpack_from("<iIiI", blob, off + 4)
            if not (0 <= max_state < 4096 and 0 <= n_try < 256):
                continue
            if max_state and not image.in_image(p_unwind):
                continue
            census.funcinfo.add(rva)
            for i in range(max_state):
                action = census.dword(p_unwind - base + 8 * i + 4)
                if action and census.in_text(action - base):
                    out.append((action - base, rva, "unwind", i))
            for t in range(n_try):
                entry = p_try - base + 20 * t
                try_low = census.dword(entry) or 0
                n_catch = census.dword(entry + 12) or 0
                p_handlers = census.dword(entry + 16) or 0
                for h in range(min(n_catch, 64)):
                    handler = census.dword(p_handlers - base + 16 * h + 12)
                    if handler and census.in_text(handler - base):
                        out.append((handler - base, rva, "catch", try_low))
    return out


def data_code_pointers(census):
    """Aligned data dwords that point into .text: (target, site)."""
    image, base = census.image, census.image.image_base
    out = []
    for sec in data_sections(image):
        blob = image.blob(sec)
        for off in range(0, len(blob) - 3, 4):
            v = struct.unpack_from("<I", blob, off)[0]
            if census.in_text(v - base):
                out.append((v - base, sec.rva + off))
    return out


def run(image, log=print):
    c = Census(image)
    pe = struct.unpack_from("<I", image.data, 0x3C)[0]
    entry = struct.unpack_from("<I", image.data, pe + 24 + 16)[0]
    funclets = funcinfo_funclets(c)
    for r, info, kind, state in funclets:
        c.funclets.setdefault(r, (info, kind, state))
    pointers = data_code_pointers(c)
    sites = {site for _t, site in pointers}
    # data pointers inside a run of code pointers (vtables, dispatch and
    # initializer tables) are strong; isolated ones are weak
    run_targets = [(t, s) for t, s in pointers if s - 4 in sites or s + 4 in sites]
    lone_targets = [(t, s) for t, s in pointers if not (s - 4 in sites or s + 4 in sites)]
    covered, table_bytes = {}, {}

    def index():
        covered.clear()
        table_bytes.clear()
        for s0, seen in c.reached.items():
            for r in seen:
                ins = c.insn(r)
                for b in range(r, r + ins.size):
                    covered.setdefault(b, s0)
        for t, (owner, n, _k) in c.tables.items():
            for b in range(t, t + n):
                table_bytes.setdefault(b, owner)

    def take(start, why, check=True):
        if start in c.starts:
            return False
        info = c.stub_info(start)
        if info is not None:
            c.stubs[start] = info
            return False
        if check and (start in covered or start in table_bytes):
            return False
        c.starts[start] = why
        res = c.descend(start)
        if res is None:
            del c.starts[start]
            return False
        seen, spans, calls, tails, imms = res
        c.reached[start] = seen
        for t, n, kind in spans:
            c.tables[t] = (start, n, kind)
        c.calls[start] = calls
        work.extend((t, "call") for t in calls)
        work.extend((t, "tail") for t in tails)
        if why.startswith("funclet:catch"):
            # `mov eax, offset L; ret` hands the parent continuation L back
            for r in seen:
                ins = c.insn(r)
                nxt = c.insn(r + ins.size)
                if ins.mnemonic == "mov" and nxt is not None and nxt.mnemonic == "ret" \
                        and len(ins.operands) == 2 and ins.operands[1].type == x86.X86_OP_IMM:
                    c.continuations[(ins.operands[1].imm & 0xFFFFFFFF) - image.image_base] = start
            imms = {i for i in imms if i not in c.continuations}
        c.imms.update(imms)
        return True

    work = [(entry, "entry")] + [(r, f"funclet:{k}") for r, _p, k, _s in funclets]
    # zero-displacement jumps hand off to the next function
    blob, lo = c.blob, c.text_lo
    k = blob.find(b"\xe9\x00\x00\x00\x00")
    while k >= 0:
        work.append((lo + k + 5, "jmp0"))
        k = blob.find(b"\xe9\x00\x00\x00\x00", k + 1)

    def drain(check):
        while work:
            start, why = work.pop()
            strong = why in ("call", "tail", "entry", "jmp0") or why.startswith("funclet")
            if c.in_text(start):
                take(start, why, check=check and not strong)

    drain(False)
    index()
    for t, s in run_targets:
        work.append((t, f"table@0x{s:x}"))
    drain(True)
    index()
    # an address code loads as an immediate that is the fall-through of a
    # call is a function the descent ran into past a call that never
    # returns (`_CxxThrowException`, then the `atexit` destructor thunk of
    # the thrower's local static array)
    ends = {}
    for seen in c.reached.values():
        for r in seen:
            ins = c.insn(r)
            if ins.mnemonic == "call":
                ends[r + ins.size] = r
    for t in sorted(c.imms):
        if t in covered and t in ends and c.in_text(t):
            take(t, "imm-after-call", check=False)
    index()
    for t in sorted(c.imms):
        work.append((t, "imm"))
    drain(True)
    index()
    for t, s in lone_targets:
        work.append((t, f"data@0x{s:x}"))
    drain(True)
    index()
    rounds = 0
    while True:
        rounds += 1
        added = 0
        order = sorted(c.starts)
        for a, b in zip(order, order[1:] + [c.text_hi]):
            end = extent_end(c, a)
            gap = end
            while gap < b:
                while gap < b and c.byte(gap) in PAD:
                    gap += 1
                if gap < b and c.stub_info(gap) is not None:
                    c.stubs[gap] = c.stub_info(gap)
                    gap += STUB_SIZE      # the stub closes a's group
                    continue
                break
            if gap >= b:
                continue
            if gap in c.continuations:
                extra = c.descend_into(a, gap)
                if extra:
                    added += 1
                continue
            if take(gap, f"gap-after:0x{a:x}"):
                added += 1
        drain(True)
        index()
        if not added:
            break
    # final pass: re-descend with the final start set
    for s in sorted(c.starts):
        res = c.descend(s)
        if res is not None:
            c.reached[s] = res[0] | c.extra.get(s, set())
    # a weak seed (a code-looking data or immediate value) that the final
    # descent puts inside another function's instruction is no start: the
    # owner's earlier, shorter descent had left those bytes uncovered
    inner = {}
    for s0, seen in c.reached.items():
        for r in seen:
            for b in range(r + 1, r + c.insn(r).size):
                inner[b] = s0
    for s in sorted(c.starts):
        why = c.starts[s]
        if inner.get(s, s) != s and (why == "imm" or why.startswith("data@")):
            del c.starts[s]
            c.reached.pop(s, None)
            c.calls.pop(s, None)
            for t in [t for t, (owner, _n, _k) in c.tables.items() if owner == s]:
                del c.tables[t]
    log(f"[census] {len(c.starts)} starts, {rounds} gap rounds, {len(c.tables)} tables, "
        f"{len(c.continuations)} catch continuations")
    return c


def extent_end(c, start):
    end = start
    for r in c.reached.get(start, ()):
        ins = c.insn(r)
        end = max(end, r + ins.size)
    for t, (owner, n, _k) in c.tables.items():
        if owner == start:
            end = max(end, t + n)
    return end


def partition(c):
    """[(start, size, decoded size)]: a function runs to the next start or EH
    registration stub minus trailing padding, and never ends before its
    decoded extent."""
    import bisect
    order = sorted(c.starts)
    stubs = sorted(c.stubs)
    rows = []
    for a, b in zip(order, order[1:] + [c.text_hi]):
        reached = extent_end(c, a)
        k = bisect.bisect_right(stubs, a)
        if k < len(stubs) and stubs[k] < b:
            b = stubs[k]
        end = b
        while end > a and c.byte(end - 1) in PAD:
            end -= 1
        end = max(end, min(reached, b))
        rows.append((a, end - a, reached - a))
    return rows
