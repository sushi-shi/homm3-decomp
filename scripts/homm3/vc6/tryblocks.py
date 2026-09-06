#!/usr/bin/env python3
"""homm3.vc6.tryblocks - the retail CATCH-SCOPE census, read off the image.

`_eh.py` reads the state-store transcript out of built objects, which says
how many cleanup regions each side opens but never says whether a region is
a `try`. Retail publishes that directly: every /GX function with a catch
scope pushes a `__ehhandler` stub whose two instructions are
`mov eax,<FuncInfo>` / `jmp __CxxFrameHandler`, and the `_s_FuncInfo` in
.rdata carries

    magic, maxState, pUnwindMap, nTryBlocks, pTryBlockMap, nIPMap, pIPMap

with each TryBlockMapEntry `{tryLow, tryHigh, catchHigh, nCatches,
pHandlerArray}` and each HandlerType `{adjectives, pType, dispCatchObj,
addressOfHandler}`. So the number of try blocks in a retail body, the state
range each one covers, the type each arm catches (a NULL pType is
`catch (...)`) and the address of every catch funclet are all FACTS, not
inferences - which makes "we have no try where retail has one" a mechanical
question rather than an inliner mystery.

That matters because VC6 will not expand a callee that introduces EH state
into a caller with no EH frame, so a missing catch SCOPE shows up as
"retail expands the throw path, we call it" plus a missing `push -1`/fs:[0]
prologue - a shape that reads exactly like an inliner budget wall and is
not one (hero::HeroFn_004E2840 is the byte-proven instance).

Resolution is two hops, because the linker parks the stub in the `.text$x`
tail OUTSIDE every carved function: FuncInfo <- stub <- the body that
pushes the stub as its SEH handler. The `push imm32` scan finds the second
hop; the carve owns the third.
"""
from __future__ import annotations

import bisect
import struct

from homm3.core import common

EH_MAGICS = (0x19930520, 0x19930521, 0x19930522)
_FUNCINFO_CB = 28
_TRYBLOCK_CB = 20
_HANDLER_CB = 16
# a body may not plausibly carry more than this many states/try blocks; the
# bound is what keeps an ordinary .rdata dword that happens to equal a magic
# from being read as a record.
_STATE_MAX = 0x1000
_TRY_MAX = 0x100
_CATCH_MAX = 64


def _sections(image):
    by_name = {s.name: s for s in image.sections}
    return by_name[".text"], by_name[".rdata"]


class _Reader:
    def __init__(self, image):
        self.image = image
        self.data = image.data
        self.base = image.image_base
        self._starts = sorted(s.rva for s in image.sections)
        self._by_start = {s.rva: s for s in image.sections}

    def _raw(self, rva):
        i = bisect.bisect_right(self._starts, rva) - 1
        if i < 0:
            return None
        s = self._by_start[self._starts[i]]
        return s.raw_offset + (rva - s.rva) if rva < s.rva + s.size else None

    def u32(self, rva):
        off = self._raw(rva)
        if off is None or off + 4 > len(self.data):
            return None
        return struct.unpack_from("<I", self.data, off)[0]

    def i32(self, rva):
        v = self.u32(rva)
        return None if v is None else (v - 0x100000000 if v >= 0x80000000 else v)

    def cstr(self, rva, limit=256):
        off = self._raw(rva)
        if off is None:
            return ""
        end = self.data.find(b"\0", off, off + limit)
        return self.data[off:end if end >= 0 else off].decode("latin-1", "replace")


def func_infos(image):
    """Every valid `_s_FuncInfo` in .rdata, as dicts.

    Each carries `info_rva`, `max_state`, `unwind` [(toState, funclet_rva)],
    `n_try` and `tries` [(tryLow, tryHigh, catchHigh, handlers)] where a
    handler is (adjectives, type_name, disp_catch_obj, handler_rva) and the
    type name of a catch-all is the literal '...'."""
    text, rdata = _sections(image)
    rd = _Reader(image)
    base = image.image_base
    lo, hi = text.rva, text.rva + text.size
    rlo, rhi = rdata.rva, rdata.rva + rdata.size
    blob = image.data[rdata.raw_offset:rdata.raw_offset + rdata.size]

    out = []
    for off in range(0, len(blob) - _FUNCINFO_CB + 1, 4):
        magic, max_state, p_unwind, n_try, p_try = struct.unpack_from(
            "<IiIII", blob, off)
        if magic not in EH_MAGICS or not 0 <= max_state <= _STATE_MAX:
            continue
        if n_try > _TRY_MAX:
            continue
        if max_state and not (p_unwind and rlo <= p_unwind - base < rhi):
            continue
        if n_try and not (p_try and rlo <= p_try - base < rhi):
            continue

        unwind, ok = [], True
        for state in range(max_state):
            entry = p_unwind - base + state * 8
            to_state, action = rd.i32(entry), rd.u32(entry + 4)
            if to_state is None or action is None or not -1 <= to_state < max_state:
                ok = False
                break
            if action and not lo <= action - base < hi:
                ok = False
                break
            unwind.append((to_state, (action - base) if action else 0))
        if not ok:
            continue

        tries = []
        for index in range(n_try):
            entry = p_try - base + index * _TRYBLOCK_CB
            low, high, catch_high, n_catch, p_handlers = (
                rd.i32(entry), rd.i32(entry + 4), rd.i32(entry + 8),
                rd.i32(entry + 12), rd.u32(entry + 16))
            if None in (low, high, catch_high, n_catch, p_handlers):
                ok = False
                break
            if not -1 <= low <= high <= max_state or not 0 <= n_catch <= _CATCH_MAX:
                ok = False
                break
            if n_catch and not rlo <= p_handlers - base < rhi:
                ok = False
                break
            handlers = []
            for slot in range(n_catch):
                h = p_handlers - base + slot * _HANDLER_CB
                adjectives, p_type = rd.u32(h), rd.u32(h + 4)
                disp, addr = rd.i32(h + 8), rd.u32(h + 12)
                if None in (adjectives, p_type, disp, addr):
                    ok = False
                    break
                if not lo <= addr - base < hi:
                    ok = False
                    break
                # a TypeDescriptor is {vftable, spare, char name[]}
                name = rd.cstr(p_type - base + 8) if p_type else "..."
                handlers.append((adjectives, name, disp, addr - base))
            if not ok:
                break
            tries.append((low, high, catch_high, handlers))
        if ok:
            out.append({"info_rva": rdata.rva + off, "magic": magic,
                        "max_state": max_state, "unwind": unwind,
                        "n_try": n_try, "tries": tries})
    return out


def handler_stubs(image, infos):
    """FuncInfo rva -> the `mov eax,<FuncInfo>; jmp __CxxFrameHandler` stub."""
    text, _ = _sections(image)
    blob = image.data[text.raw_offset:text.raw_offset + text.size]
    base = image.image_base
    wanted = {i["info_rva"] for i in infos}
    stubs = {}
    for off in range(len(blob) - 10):
        if blob[off] != 0xB8 or blob[off + 5] != 0xE9:   # mov eax,imm32 ; jmp
            continue
        target = struct.unpack_from("<I", blob, off + 1)[0] - base
        if target in wanted:
            stubs.setdefault(target, text.rva + off)
    return stubs


def push_sites(image, stub_rvas):
    """stub rva -> every `push <stub>` site (the /GX prologue's handler push)."""
    text, _ = _sections(image)
    blob = image.data[text.raw_offset:text.raw_offset + text.size]
    base = image.image_base
    wanted = set(stub_rvas)
    sites = {}
    for off in range(len(blob) - 5):
        if blob[off] != 0x68:                            # push imm32
            continue
        target = struct.unpack_from("<I", blob, off + 1)[0] - base
        if target in wanted:
            sites.setdefault(target, []).append(text.rva + off)
    return sites


def carved_functions():
    """[(rva, size)] from the admitted carve, ascending."""
    rows = []
    header = None
    path = common.HOMM3_DIR / "config/retail-functions.tsv"
    for line in path.read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        cells = line.split("\t")
        if header is None:
            header = cells
            continue
        rows.append((int(cells[0], 16), int(cells[1], 0)))
    rows.sort()
    return rows


def _owner(functions, starts, rva):
    i = bisect.bisect_right(starts, rva) - 1
    if i < 0:
        return None
    start, size = functions[i]
    return start if start <= rva < start + size else None


def census(image):
    """Every try-bearing FuncInfo, attributed to the carve row that pushed it.

    Rows carry `parent` (a carve rva or None) and `funclets_outside`, the
    catch handlers the carve did not put inside their own parent - the
    boundary-correction candidates."""
    infos = [i for i in func_infos(image) if i["n_try"]]
    stubs = handler_stubs(image, infos)
    sites = push_sites(image, stubs.values())
    functions = carved_functions()
    starts = [f[0] for f in functions]

    rows = []
    for info in infos:
        stub = stubs.get(info["info_rva"])
        parents = sorted({o for site in sites.get(stub, ())
                          if (o := _owner(functions, starts, site)) is not None})
        parent = parents[0] if parents else None
        outside = [addr for _, _, _, handlers in info["tries"]
                   for *_, addr in handlers
                   if _owner(functions, starts, addr) != parent]
        rows.append(dict(info, stub=stub, parent=parent, parents=parents,
                         funclets_outside=outside))
    rows.sort(key=lambda r: (r["parent"] is None, r["parent"] or 0))
    return rows


def _baseline():
    """retail rva -> (unit, symbol, cur, max)."""
    out = {}
    path = common.HOMM3_DIR / "config/match_baseline.tsv"
    for line in path.read_text().splitlines():
        if line.startswith("#"):
            continue
        cells = line.split("\t")
        if len(cells) < 7 or cells[0] == "unit":
            continue
        try:
            out.setdefault(int(cells[5], 16), (cells[0], cells[1], cells[2], cells[3]))
        except ValueError:
            continue
    return out


def run(args) -> int:
    image, _ = common.load_image()
    rows = census(image)
    baseline = _baseline()
    sizes = dict(carved_functions())
    print(f"# retail catch scopes: {len(rows)} function(s) with nTryBlocks > 0")
    print(f"# {'rva':>9} {'size':>6} {'nTry':>4} {'mS':>3} {'max':>8}  unit / symbol")
    for row in rows:
        parent = row["parent"]
        unit, symbol, _, best = baseline.get(parent, ("?", "?", "-", "-"))
        print(f"  0x{(parent or 0):07x} {sizes.get(parent, 0):6d} "
              f"{row['n_try']:4d} {row['max_state']:3d} {best:>8}  {unit} / {symbol}")
        for low, high, catch_high, handlers in row["tries"]:
            arms = "; ".join(
                f"catch({name}) @0x{addr:x}" + (f" obj@{disp}" if disp else "")
                for _, name, disp, addr in handlers)
            print(f"      try[{low}..{high}] catchHigh={catch_high}  {arms}")
        if row["funclets_outside"]:
            print("      FUNCLET OUTSIDE THE CARVE ROW: "
                  + ", ".join(f"0x{a:x}" for a in row["funclets_outside"])
                  + "  (boundary-correction candidate)")
    return 0
