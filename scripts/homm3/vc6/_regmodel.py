"""homm3.vc6._regmodel - the pure register-assignment model (phase 4).

Reverse-engineered from the pinned C2.DLL 12.00.8447 (method + address
ledger: docs/vc6/regalloc.md; byte evidence measured 2026-08-10):

* Internal GPR numbering is machine encoding + 1 - handles 1..8 of the
  pre-created register SYMBOLS in the back-end symbol hash (buckets at
  .bssbe rva 0x9d88c, key at sym+0x1c): 1=EAX 2=ECX 3=EDX 4=EBX 5=ESP
  6=EBP 7=ESI 8=EDI.  Proven two ways: the listing name table at .rdata
  0xa9194 is indexed ``[0xa9190 + reg*4]`` (mdlist.c, rva 0x74b23), and
  the byte-width exclusion drops exactly {7,8,6} = the three GPRs without
  an 8-bit subregister (regasg.c rva 0x8c1dc).

* ONE allocation preference order for every GPR pseudo: the 0-terminated
  dword table {1,2,3,7,8,4,6} = EAX ECX EDX ESI EDI EBX EBP (const copy
  .rdata rva 0xa09f0 with a begin/end pointer pair at 0xa0a14; runtime
  copy .databe rva 0xadff4).  regasg.c walks it FIRST-FIT: 0x8be6c
  (re-bind walk over DAT_107adff4/DAT_107adff8 with the per-register
  binding array .bssbe 0x9d6ec and per-register conflict sets 0x9d6c8)
  and 0x8c1dc (candidate set = the table; first-fit pick at the loop
  head).  There is no per-class order - the classes fall out of
  exclusions:
    - byte-sized pseudo        -> minus {ESI, EDI, EBP}   (0x8c1dc)
    - crosses a call           -> minus {EAX, ECX, EDX}   (clobbered)
    - ESP(5)                   never allocatable (regasg.c region checks
                               ``reg && reg < 9 && reg != 5``)
    - EBP(6)                   only when the function is frameless (/Oy)

* Pseudos receive their register at their DEFINITION, in stream order;
  for named locals that order follows the front end's symbol-handle
  creation order (the IL ``sy`` stream, docs/vc6/il-format.md).  Measured
  (2026-08-10, pinned SP3 CL, game profile): three call-crossing locals
  a,b,c created in that order take ESI, EDI, EBX; a fourth is homed to
  the frame; swapping two values' creation order swaps ESI/EDI exactly -
  the catalog's B1 signature.

The model is deliberately a MINIMUM SLICE for B1 (+ the B8/B14 corners
the same walk explains): given call-crossing GPR pseudos in creation
order it predicts which lands in ESI/EDI/EBX.  It does NOT model spill
cost, coalescing, or live-range splitting - a prediction is a hypothesis
and the Wine VC6 compile stays the verdict.
"""
from __future__ import annotations

from dataclasses import dataclass, field

# The C2 preference table {1,2,3,7,8,4,6,0} (.rdata 0xa09f0 / .databe
# 0xadff4), spelled as register names via the 1..8 = machine+1 numbering.
PREFERENCE: tuple[str, ...] = ("eax", "ecx", "edx", "esi", "edi", "ebx", "ebp")

CALLER_SAVED = frozenset({"eax", "ecx", "edx"})
# PREFERENCE restricted to what a call-crossing pseudo can take (/Oy- body):
CALLEE_ORDER: tuple[str, ...] = ("esi", "edi", "ebx")
# GPRs with no 8-bit subregister - dropped for byte-sized pseudos (0x8c1dc
# removes handles 7, 8, 6 when the node's width field is 1):
NO_BYTE = frozenset({"esi", "edi", "ebp"})


@dataclass
class Pseudo:
    """One allocation candidate, in creation order."""
    name: str
    crosses_call: bool = True
    byte_sized: bool = False
    # Optional creation-order key (e.g. the IL symbol handle); listing
    # position is the order when None for all.
    handle: int | None = None


@dataclass
class Assignment:
    binding: dict[str, str | None] = field(default_factory=dict)

    def register_of(self, name: str) -> str | None:
        return self.binding.get(name)


def assign(pseudos: list[Pseudo], ebp_allocatable: bool = False) -> Assignment:
    """First-fit walk of PREFERENCE per pseudo, in creation order.

    Minimum-slice liveness: every listed pseudo is assumed live to the end
    of the function (the B1 shape - long-lived values that all overlap),
    so a register once taken stays taken.  None = not assignable within
    the modelled slice (C2 memory-homes it; measured: the 4th call-
    crossing local lands in a frame slot).
    """
    order = [p for p in pseudos]
    if any(p.handle is not None for p in order):
        if any(p.handle is None for p in order):
            raise ValueError("mixed handle/positional creation order")
        order = sorted(order, key=lambda p: p.handle)
    out = Assignment()
    taken: set[str] = set()
    for p in order:
        reg = _first_fit(p, taken, ebp_allocatable)
        out.binding[p.name] = reg
        if reg is not None:
            taken.add(reg)
    return out


def _first_fit(p: Pseudo, taken: set[str], ebp_allocatable: bool) -> str | None:
    for reg in PREFERENCE:
        if reg in taken:
            continue
        if reg == "ebp" and not ebp_allocatable:
            continue
        if p.crosses_call and reg in CALLER_SAVED:
            continue
        if p.byte_sized and reg in NO_BYTE:
            continue
        return reg
    return None


def callee_saved_binding(names: list[str]) -> dict[str, str | None]:
    """The B1 slice: call-crossing dword pseudos in creation order ->
    {name: esi/edi/ebx or None (homed)}."""
    a = assign([Pseudo(n) for n in names])
    return {n: a.register_of(n) for n in names}


def creation_order_for(target: dict[str, str]) -> list[str] | None:
    """The creation order that produces *target* ({name: reg}), or None
    if no order can (a register outside CALLEE_ORDER, or a gap).

    This is the solver's direction: to make value V land in ESI, V's
    pseudo must be created before every other call-crossing pseudo."""
    by_reg = {}
    for name, reg in target.items():
        if reg not in CALLEE_ORDER or reg in by_reg:
            return None
        by_reg[reg] = name
    order = []
    for reg in CALLEE_ORDER[:len(by_reg)]:
        if reg not in by_reg:
            return None  # a gap (e.g. EDI+EBX used, ESI free) - not first-fit
        order.append(by_reg[reg])
    return order
