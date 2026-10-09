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
     `mov eax, offset L; ret` (a `rep movsd` may sit between them) instead
     names a continuation L of its parent, never a function;
  4. isolated data pointers into .text;
  5. the first non-padding byte after a body's decoded extent (unreferenced
     neighbours), repeated to a fixpoint.

A data or immediate seed is dropped again when the function before it,
descended without it, falls into it or runs through it.

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
    #: the image pin's `census_joins_catch_bodies`: see `tail_limit`
    catch_bodies: bool = False

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

    def is_catch(self, rva):
        return self.funclets.get(rva, (None, None))[1] == "catch"

    def tail_limit(self, start):
        """The end of the range a jump stays inside [start, limit): the next
        start. With `catch_bodies` a catch funclet is not the end: VC6 emits
        a `catch` block inside its function's code (after the try block's
        jump over it), so the parent's jumps run on past its own handlers;
        nor is a weak seed (`is_weak`)."""
        limit = self.next_start(start)
        if self.catch_bodies:
            while limit < self.text_hi and (self.is_catch(limit) or self.is_weak(limit)):
                limit = self.next_start(limit)
        return limit

    def is_weak(self, rva):
        """A start seeded only by a code-looking data or immediate value: it
        may be data (a table of four-character codes) and never bounds a
        known function's jumps; `run` drops it again when that function
        runs through it."""
        why = self.starts.get(rva, "")
        return why == "imm" or why.startswith("data@")

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
                        limit = self.tail_limit(start)
                        if self.catch_bodies and self.is_catch(start) \
                                and not (start <= t < limit) and t not in self.starts:
                            # a catch block jumps back into its parent's
                            # body (a loop head, the code after the try):
                            # the parent's code, never a tail call
                            break
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


def run(image, log=print, catch_bodies=False):
    c = Census(image, catch_bodies=catch_bodies)
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
            # `mov eax, offset L; ret` hands the parent continuation L back;
            # the handler's last copy (`rep movsd` of a returned object) may
            # sit between the two
            for r in seen:
                ins = c.insn(r)
                if not (ins.mnemonic == "mov" and len(ins.operands) == 2
                        and ins.operands[0].type == x86.X86_OP_REG
                        and ins.operands[0].reg == x86.X86_REG_EAX
                        and ins.operands[1].type == x86.X86_OP_IMM):
                    continue
                nxt = c.insn(r + ins.size)
                while nxt is not None and nxt.mnemonic in ("rep movsd", "movsd"):
                    nxt = c.insn(nxt.address - image.image_base + nxt.size)
                if nxt is not None and nxt.mnemonic == "ret":
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
    # a weak seed (a code-looking data or immediate value) is no start when
    # the function before it, descended without it, falls into it or runs
    # through it: the seed was taken before that descent covered it
    import bisect
    for s in sorted(c.starts):
        why = c.starts[s]
        if not (why == "imm" or why.startswith("data@")):
            continue
        order = sorted(c.starts)
        k = bisect.bisect_left(order, s)
        if k == 0:
            continue
        p = order[k - 1]
        # a gap-filling neighbour may be data decoded as code: only its
        # instructions' interiors are evidence against the seed
        weak_p = c.starts[p].startswith("gap-after")
        del c.starts[s]
        res = c.descend(p)
        # inside one of its instructions, or reached by falling through:
        # a jump to it may still be a tail call
        covers = res is not None and any(
            r < s < r + c.insn(r).size
            or (not weak_p and r + c.insn(r).size == s
                and c.insn(r).mnemonic not in ("jmp", "ret", "retf", "int3"))
            for r in res[0])
        if not covers:
            c.starts[s] = why
            continue
        c.reached[p] = res[0] | c.extra.get(p, set())
        c.reached.pop(s, None)
        c.calls.pop(s, None)
        for t in [t for t, (owner, _n, _k) in c.tables.items() if owner == s]:
            del c.tables[t]
        for t, n, kind in res[1]:
            c.tables[t] = (p, n, kind)
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


def catch_folds(c, parents):
    """({handler: parent}, [(handler, parent or None, reason)]): `merges`
    with catch handlers only."""
    merged, refused = merges(c, parents)
    return merged, [(rva, owner, why) for rva, owner, _kind, why in refused]


#: Instructions after which control never reaches the next byte.
ENDS = ("jmp", "ret", "retf", "iret", "int3", "hlt", "ud2")


def merges(c, parents, catches=True, tails=False, alignment=0):
    """({piece: owner}, [(piece, owner or None, kind, reason)]): the census
    starts that belong to the function before them, and every candidate
    left as a row with its reason (kind "catch" or "tail").

    Catch handlers (`catches`). VC6 emits a `catch` block inside its
    function's own COMDAT, after the body, where the game's hand-owned census
    keeps it (config/retail/funclets.tsv, the retired 0xe29dc row). A catch
    handler folds into its parent only when the parent's own FuncInfo
    TryBlockMap names it (`parents`: FuncInfo -> parent,
    homm3.census.eh.funcinfo_parents) and it directly follows the parent's
    extent: the parent's body, or a piece already merged into it, then only
    padding (or the decoded body runs on past the handler, which it then
    embeds).

    Tails (`tails`). A start the descent took as a tail call while an early,
    later dropped or embedded start bounded the function (a catch handler
    inside the body, a weak seed) cuts the body at that jump target. Such a
    piece joins the function before it when it directly follows that
    function's extent (only padding between), does not start on a function
    boundary of the image (`alignment`: /O2 starts every function on 16
    bytes, /O1 on none), the function's own instructions jump or fall into it, and
    nothing outside the function references it: no call, jump, immediate or
    jump-table entry from another function, and no data pointer. The
    function's own catch handlers are part of it. Candidates are the starts
    the function before them reaches, and the census's tail-call starts
    that directly follow it unaligned."""
    if not tails:
        return _merge_pass(c, parents, catches, None, {}, alignment)
    flow = _flow(c)
    vetoed = {}
    while True:
        merged, refused = _merge_pass(c, parents, catches, flow, vetoed, alignment)
        found = _outside_references(c, parents, merged, flow, vetoed)
        if not found:
            break
        vetoed.update(found)
    refused += [(rva, owner, "tail", why) for rva, (owner, why) in vetoed.items()]
    refused.sort(key=lambda row: row[0])
    return merged, refused


def _merge_pass(c, parents, catches, flow, vetoed, alignment):
    import bisect
    order = sorted(c.starts)
    stubs = sorted(c.stubs)
    merged, refused, ends = {}, [], {}

    def aligned(rva):
        return alignment > 1 and not rva % alignment

    def follows(prev, owner, piece):
        """Why `piece` does not directly follow the extent of `owner` and the
        pieces merged into it up to `prev`, else None."""
        s = bisect.bisect_right(stubs, prev)
        if s < len(stubs) and stubs[s] < piece:
            return "an EH registration stub separates it"
        # the decoded body may run on past a piece it embeds (the code after
        # the try block, where the catch returns)
        gap = max(ends.get(owner, 0), extent_end(c, prev))
        while gap < piece and c.byte(gap) in PAD:
            gap += 1
        return f"non-padding bytes at 0x{gap:x}" if gap < piece else None

    for k, piece in enumerate(order):
        prev = order[k - 1] if k else None
        owner = merged.get(prev, prev)
        entry = c.funclets.get(piece)
        if entry is not None and entry[1] == "catch":
            if not catches:
                continue
            parent = parents.get(entry[0])
            if parent is None:
                refused.append((piece, None, "catch", "no parent loads the handler's FuncInfo"))
                continue
            if owner != parent:
                refused.append((piece, parent, "catch", f"follows 0x{prev:x}, not its parent"
                                if prev is not None else "first start"))
                continue
            why = follows(prev, owner, piece)
            if why is not None:
                refused.append((piece, parent, "catch", why))
                continue
            merged[piece] = parent
            ends[owner] = max(ends.get(owner, 0), extent_end(c, prev), extent_end(c, piece))
            continue
        if flow is None or prev is None or piece in vetoed:
            continue
        reached = any(r < piece and merged.get(order[bisect.bisect_right(order, r) - 1],
                                                order[bisect.bisect_right(order, r) - 1]) == owner
                      for r in flow.into.get(piece, ()))
        why = follows(prev, owner, piece)
        if not reached:
            if why is None and not aligned(piece) and c.starts[piece] == "tail":
                refused.append((piece, owner, "tail", f"0x{owner:x} never jumps or falls into it"))
            continue
        if why is None and aligned(piece):
            why = f"starts on a {alignment}-byte boundary"
        if why is not None:
            refused.append((piece, owner, "tail", why))
            continue
        merged[piece] = owner
        ends[owner] = max(ends.get(owner, 0), extent_end(c, prev), extent_end(c, piece))
    return merged, refused


class _Flow:
    """The decoded instructions' control transfers and code references."""

    def __init__(self):
        self.into = defaultdict(set)    # target -> insns jumping or falling into it
        self.refs = defaultdict(set)    # target -> (site, how) of any operand naming it


def _flow(c):
    base = c.image.image_base
    flow = _Flow()
    insns = set()
    for seen in c.reached.values():
        insns |= seen
    for r in insns:
        ins = c.insn(r)
        mn = ins.mnemonic
        for op in ins.operands:
            if op.type == x86.X86_OP_IMM:
                v = (op.imm & 0xFFFFFFFF) - base
                how = "call" if mn == "call" else "jump" if mn.startswith(("j", "loop")) \
                    else "immediate"
                flow.refs[v].add((r, how))
                # a jump that is its function's first instruction is a thunk's
                # tail call (`Census.descend`), never a branch of its body
                if how == "jump" and not (mn == "jmp" and r in c.starts):
                    flow.into[v].add(r)
            elif op.type == x86.X86_OP_MEM and op.mem.base == 0:
                flow.refs[(op.mem.disp & 0xFFFFFFFF) - base].add((r, "operand"))
        if mn not in ENDS and mn != "call":
            flow.into[r + ins.size].add(r)
        elif mn == "jmp" and ins.size == 5 and ins.bytes[1:] == b"\0\0\0\0":
            # `jmp $+5` hands off to the next function (`Census` seeds)
            flow.into[r + 5].discard(r)
    for table, (owner, n, kind) in c.tables.items():
        if kind == "jump":
            for i in range(n // 4):
                v = c.dword(table + 4 * i)
                if v is not None:
                    flow.refs[v - base].add((table + 4 * i, "jump table"))
    # a string's tail can read as an address (`.?AVTScenario@@` ends in
    # 0x0040406f): no pointer lives inside a literal
    from homm3.census.find_relocs import literal_mask
    masks = {sec.rva: (sec, literal_mask(c.image.blob(sec))) for sec in data_sections(c.image)}
    for target, site in data_code_pointers(c):
        sec, mask = masks[c.image.section_of(site).rva]
        if not any(mask[site - sec.rva:site - sec.rva + 4]):
            flow.refs[target].add((site, "data"))
    return flow


def _outside_references(c, parents, merged, flow, vetoed):
    """{piece: (owner, reason)} for each joined tail something outside its
    function references."""
    import bisect
    order = sorted(c.starts)
    found = {}
    for piece, owner in sorted(merged.items()):
        entry = c.funclets.get(piece)
        if piece in vetoed or (entry is not None and entry[1] == "catch"):
            continue
        for site, how in sorted(flow.refs.get(piece, ())):
            if how == "data":
                found[piece] = (owner, f"data reference at 0x{site:x}")
                break
            k = bisect.bisect_right(order, site) - 1
            if not c.in_text(site) or k < 0:
                found[piece] = (owner, f"{how} at 0x{site:x}")
                break
            at = order[k]
            if merged.get(at, at) == owner:
                continue
            handler = c.funclets.get(at)
            if handler is not None and handler[1] == "catch" \
                    and parents.get(handler[0]) == owner:
                continue
            found[piece] = (owner, f"{how} at 0x{site:x} in 0x{at:x}")
            break
    return found


def partition(c, folded=None):
    """[(start, size, decoded size)]: a function runs to the next start or EH
    registration stub minus trailing padding, and never ends before its
    decoded extent. A piece in `folded` (a catch handler or a joined tail,
    `merges`) is part of its owner's extent and no start of its own."""
    import bisect
    folded = folded or {}
    order = sorted(r for r in c.starts if r not in folded)
    stubs = sorted(c.stubs)
    tails = {}
    for handler, parent in folded.items():
        tails[parent] = max(tails.get(parent, 0), extent_end(c, handler))
    rows = []
    for a, b in zip(order, order[1:] + [c.text_hi]):
        reached = max(extent_end(c, a), tails.get(a, 0))
        k = bisect.bisect_right(stubs, a)
        if k < len(stubs) and stubs[k] < b:
            b = stubs[k]
        end = b
        while end > a and c.byte(end - 1) in PAD:
            end -= 1
        end = max(end, min(reached, b))
        rows.append((a, end - a, reached - a))
    return rows
