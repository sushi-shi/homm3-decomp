#!/usr/bin/env python3
"""homm3.vc6._eh - the EH CLEANUP-COUNT signal, read off the built objects.

A /GX function that owns anything destructible carries a state variable at
[ebp-4]: `mov [ebp-4], N` marks the unwind-map entry that a throw from the
following region would enter. The sequence of those immediates is therefore a
transcript of the function's OBJECT LIFETIMES - one entry per region that can
throw while something is pending cleanup - and it is visible in .text on both
sides without a compile.

That makes it a wall class the other three solvers cannot see:

  * `predict-inline` reads the out-of-line CALL multiset,
  * `why-branch` reads the CFG,
  * `why-reg` reads register bindings,

and none of them notices that retail's body opens one MORE cleanup region than
ours - i.e. that retail constructs a temporary we never wrote, or that a call
we fold to a constant is a call retail could throw from.

Two divergence kinds, and they route differently:

  COUNT  the sequences have different lengths. Retail has regions we do not
         (a MISSING statement or a missing potentially-throwing call) or we
         have regions retail does not (an EXTRA lifetime, or a callee we let
         throw that retail proved nothrow). Byte-proven instance: VC6 emits a
         cleanup chain only for a region that CAN throw, so whether <new>'s
         `operator delete(void*) _THROW0()` is visible in the TU changes the
         COUNT - see Bitmap816::~Bitmap816 and docs/vc6/eh-cleanup.md.
  ORDER  same multiset, permuted. The unwind-map entries were allocated in a
         different order, i.e. the front end saw the subobjects/temporaries in
         a different order - a declaration-order or statement-order fact.

Reading the map itself: the funclets are DATA in the image, so retail
publishes the answer. `.xdata$x` on our side holds `maxState` at +4 and the
unwind map (toState, action) pairs it points at; on retail's side the funclet
group sits under a `<Class>_<fn>_unwindNN` label in the synth-PDB inventory
(build/gen/symbol_names.csv) and each entry's `add ecx,<off>` names the
subobject it cleans.
"""
from __future__ import annotations

import re

from homm3.sema import _asm

# `mov dword ptr [ebp - 0x4], 0x2` / `mov byte ptr [ebp - 0x4], 0x0` - the
# state store. VC6 narrows to a byte when only the low byte changes, so the
# WIDTH is not part of the signal; the immediate is. The SOURCE need not be an
# immediate at all: when the constant 0 is CSE'd into a register the state
# store becomes `mov [ebp-4], ebx` (mouseManager::SetPointer is retail's
# example), which is still a state store with an OPAQUE value - counted, but
# never compared by value.
_STATE = re.compile(
    r"\bmov\s+(?:dword|byte)\s+ptr\s+\[ebp\s*-\s*0x4\],\s*"
    r"(-?0x[0-9a-f]+|-?\d+|[a-z]{2,3})\s*$", re.I)


# The /GX prologue that establishes the registration node - without it there
# is no state variable and a store to [ebp-4] is an ordinary local (an FP
# constant halved into the frame reads as `mov [ebp-4], 0x3ff00000`, which is
# what this guard is for).
_EHPROLOG = re.compile(r"fs:\[0x0\]", re.I)
# A state is an index into the unwind map, or -1. Anything else is a local.
_STATE_MAX = 0x1000


def _to_int(tok: str) -> int:
    v = int(tok, 0)
    # the state is a signed int; -1 is the "nothing pending" terminator.
    return v - 0x100000000 if v >= 0x80000000 else v


def state_sequence(obj, fn: str) -> list | None:
    """The ordered EH state stores of *fn* in *obj* - an int per store, or
    None for a register-sourced (opaque) one. Returns None outright when the
    transcript cannot be read (missing object / symbol / no EH frame) or when
    the slot is demonstrably an ordinary local. Never raises: side signal."""
    from homm3.vc6 import reg_model
    try:
        body = _asm.objdump(obj, reg_model._resolve_symbol(obj, fn), 0)
    except (Exception, SystemExit):
        return None
    if not _EHPROLOG.search(body):
        return None
    out = []
    for line in body.splitlines():
        m = _STATE.search(line)
        if not m:
            continue
        tok = m.group(1)
        if not (tok[:1].isdigit() or tok[:1] == "-"):
            out.append(None)     # CSE'd constant in a register - opaque
            continue
        v = _to_int(tok)
        if v < -1 or v > _STATE_MAX:
            return None          # the slot is being used as a plain local
        out.append(v)
    return out


def divergence(unit: str, fn: str):
    """{'kind','base','target','note'} when the two EH state transcripts
    disagree, else None. Reads built objects; no compile."""
    base = state_sequence(_asm.BASE / f"{unit}.obj", fn)
    tgt = state_sequence(_asm.TARGET / f"{unit}.c.obj", fn)
    if base is None or tgt is None or base == tgt:
        return None
    # An opaque (register-sourced) store is still a region boundary, so the
    # COUNT comparison stands; a value comparison does not.
    if len(base) == len(tgt) and (None in base or None in tgt):
        return None
    if len(base) != len(tgt):
        kind = "COUNT"
        if len(tgt) > len(base):
            note = (f"retail opens {len(tgt) - len(base)} cleanup region(s) "
                    "this body does not - a statement whose temporary (or "
                    "throwing call) we never wrote, or a callee retail could "
                    "throw from that we fold away")
        else:
            note = (f"this body opens {len(base) - len(tgt)} cleanup "
                    "region(s) retail does not - an extra lifetime, or a "
                    "callee retail's TU proved nothrow (the <new> "
                    "`operator delete(void*) _THROW0()` case)")
    elif sorted(base, key=lambda v: (v is None, v)) == \
            sorted(tgt, key=lambda v: (v is None, v)):
        kind = "ORDER"
        note = ("same cleanup multiset, permuted - the unwind-map entries "
                "were allocated in a different order, i.e. declaration or "
                "statement order differs")
    else:
        kind = "COUNT"
        note = ("same length, different state values - the unwind maps are "
                "not the same map")
    return {"kind": kind, "base": base, "target": tgt, "note": note}


def format_line(div) -> str:
    """One-line rendering for diagnose, truncated so a 70-state Load/Save row
    does not bury the rest of the report."""
    def show(seq):
        body = ",".join("reg" if v is None else str(v) for v in seq[:14])
        return f"[{body}{',...' if len(seq) > 14 else ''}] ({len(seq)})"
    return (f"{div['kind']}: base {show(div['base'])} "
            f"vs retail {show(div['target'])}")
