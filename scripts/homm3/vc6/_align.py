"""homm3.vc6._align - masked-stream alignment + register-binding diagnosis.

Support module for reg_model (`homm3 vc6 why-reg`). All disassembly TEXT
comes from homm3.sema._asm's two producers (llvm-objdump over COFF objects,
capstone over the retail image) in the one shared row shape; this module
never disassembles anything itself - it reuses _asm's row parsing, masking
and CFG machinery and only adds the pairing/diagnosis layer on top.

Two masking grades per instruction, both built on _asm.mask_insn:

  vis    register-VISIBLE: absolute addresses and branch/call targets
         masked; registers, stack slots and immediates kept. The distance
         metric and the mutation verdict live here.
  shape  register-BLIND: vis with every register token collapsed to <r>.
         Pairing instructions by shape is what lets "same schedule,
         different physical register" (the catalog's B1 signature) surface
         as a BINDING divergence instead of a generic mismatch.

distance(a, b) = len(a) + len(b) - 2 * (aligned matches) over the two vis
streams (difflib longest-matching alignment, autojunk off): the number of
instruction slots that fail to pair. 0 <=> the register-visible masked
streams are identical - why-reg's "no divergence" (weaker than byte-exact:
absolute address operands stay masked; see docs/vc6/behavior-catalog.md C9).

Producer caveat: when the two sides come from DIFFERENT producers (compiled
obj via llvm-objdump vs retail image via capstone) small syntax skew can
survive masking and inflate the distance; prefer a delinked target object
(same producer both sides) - reg_model picks it automatically when present.
"""
from __future__ import annotations

import difflib
import re
from collections import Counter

from homm3.sema import _asm

_CALLEE_SAVED = {"ebx", "esi", "edi", "ebp"}
_ARG_REGS = {"eax", "ecx", "edx"}

_REG = re.compile(r"\b(e(?:ax|bx|cx|dx|si|di|bp|sp)|[abcd][xlh]|si|di|bp|sp)\b")
_EBP_NEG = re.compile(r"\[ebp - (0x[0-9a-f]+|\d+)\]")
_PARAM_STORE = re.compile(r"^mov (?:byte|word|dword) ptr \[ebp \+ (0x[0-9a-f]+|\d+)\],")
_PUSHPOP = re.compile(r"^(?:push|pop) (?:ebx|esi|edi|ebp)$")
_FRAME_SUB = re.compile(r"^sub esp, (0x[0-9a-f]+|\d+)$")
# Memory references OFF the frame (globals / pointer-derived); frame slots
# are the homing detector's job, not the reload detector's.
_MEMREF = re.compile(r"ptr \[(?!e[bs]p\b)([^\]]*)\]")
_ABS_OPERAND = re.compile(r"^(?:<addr>|0x[0-9a-f]+|\d+)$")


def _family(reg: str) -> str:
    """Collapse a sub-register to its 32-bit family (al/ah/ax -> eax)."""
    if reg.startswith("e"):
        return reg
    if reg in ("si", "di", "bp", "sp"):
        return "e" + reg
    return "e" + reg[0] + "x"


def shape(vis: str) -> str:
    return _REG.sub("<r>", vis)


def regs(vis: str) -> list:
    return [_family(m.group(1)) for m in _REG.finditer(vis)]


def parse_side(text: str) -> list:
    """One side's instruction records from producer text.

    Each record: {"raw", "vis", "shape", "sym"}. `sym` is the reloc symbol
    (llvm-objdump -dr interleaves IMAGE_REL lines) or the <symbol>
    annotation (capstone producer) - used ONLY to name memory operands in
    the reload diagnosis, never for alignment. Trailing padding nops drop.
    """
    recs = []
    for ln in text.splitlines():
        parsed = _asm.parse_ins(ln)
        if parsed is not None:
            _off, body = parsed
            note = re.search(r"<([^>]+)>", body)
            vis = _asm.mask_insn(body)
            recs.append({"raw": body, "vis": vis, "shape": shape(vis),
                         "sym": note.group(1) if note else None})
            continue
        if "IMAGE_REL_I386_" in ln and recs and recs[-1]["sym"] is None:
            m = re.search(r"IMAGE_REL_I386_\w+\s+(\S+)", ln)
            if m:
                recs[-1]["sym"] = m.group(1)
    while recs and recs[-1]["vis"] == "nop":
        recs.pop()
    return recs


def distance(base: list, target: list) -> int:
    """Unpaired-slot count between the two register-visible streams."""
    a = [r["vis"] for r in base]
    b = [r["vis"] for r in target]
    sm = difflib.SequenceMatcher(a=a, b=b, autojunk=False)
    matched = sum(blk.size for blk in sm.get_matching_blocks())
    return len(a) + len(b) - 2 * matched


def flow_kinds(text: str) -> list:
    """Branch-kind skeleton (block-index space) - _asm's CFG comparison
    unit, which is what makes the two producers comparable."""
    return [_asm.branch_kind(term, i)
            for i, (_addr, _body, term) in enumerate(_asm.cfg(text))]


# --- diagnosis ---------------------------------------------------------------------


def _neg_slots(recs) -> set:
    return {m.group(1) for r in recs for m in _EBP_NEG.finditer(r["vis"])}


def _param_store_slots(recs) -> set:
    out = set()
    for r in recs:
        m = _PARAM_STORE.match(r["vis"])
        if m:
            off = int(m.group(1), 16 if m.group(1).startswith("0x") else 10)
            if off >= 8:
                out.add(m.group(1))
    return out


def _saves(recs) -> Counter:
    return Counter(r["vis"] for r in recs if _PUSHPOP.match(r["vis"]))


def _frame_bytes(recs):
    for r in recs[:8]:
        m = _FRAME_SUB.match(r["vis"])
        if m:
            return m.group(1)
    return None


def _memref_counts(recs) -> Counter:
    """How often each NAMEABLE off-frame memory operand is referenced.
    Named = a reloc/annotation symbol, or an absolute operand; register-
    derived operands are skipped (a renamed base register would fabricate
    'different operand' rows for the same value)."""
    counts = Counter()
    for r in recs:
        for m in _MEMREF.finditer(r["vis"]):
            if r["sym"]:
                counts[r["sym"]] += 1
            elif _ABS_OPERAND.match(m.group(1).strip()):
                counts["mem " + m.group(1).strip()] += 1
    return counts


def diagnose(base: list, target: list,
             base_text: str, target_text: str) -> tuple:
    """(findings, stats): heuristic classification of the divergence into
    the catalog's register families. Each finding cites its catalog ID;
    the IDs are the behavior-catalog's, the mapping here is heuristic."""
    findings = []

    base_kinds, target_kinds = flow_kinds(base_text), flow_kinds(target_text)
    flow_same = base_kinds == target_kinds
    if not flow_same:
        findings.append({
            "kind": "flow", "catalog": "A*/D*",
            "detail": (f"CFG skeletons differ ({len(base_kinds)} vs "
                       f"{len(target_kinds)} blocks / branch kinds) - the "
                       "divergence is not (only) a register binding; "
                       "why-reg's knobs may not apply (inliner or control "
                       "flow, A/D families)")})

    # Shape-level alignment: equal-shape pairs feed the binding table,
    # everything unpaired feeds the homing/spill/reload detectors.
    sm = difflib.SequenceMatcher(a=[r["shape"] for r in base],
                                 b=[r["shape"] for r in target],
                                 autojunk=False)
    corr = Counter()
    example = {}
    binding_pairs = 0
    paired = 0
    base_only, target_only = [], []
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            paired += i2 - i1
            for k in range(i2 - i1):
                ra, rt = base[i1 + k], target[j1 + k]
                if ra["vis"] == rt["vis"]:
                    continue
                fa, ft = regs(ra["vis"]), regs(rt["vis"])
                hit = False
                for x, y in zip(fa, ft):
                    if x != y:
                        corr[(x, y)] += 1
                        example.setdefault(
                            (x, y), f"{ra['vis']}  vs  {rt['vis']}")
                        hit = True
                if hit:
                    binding_pairs += 1
            continue
        base_only.extend(base[i1:i2])
        target_only.extend(target[j1:j2])

    if corr:
        fams = {f for pair in corr for f in pair}
        swap = any((y, x) in corr for (x, y) in corr
                   if {x, y} <= _CALLEE_SAVED)
        if swap:
            cat, note = "B1", "callee-saved role swap, schedule aligned"
        elif fams <= _ARG_REGS:
            cat, note = ("B10/B14", "scratch-register preference "
                         "(argument-chain / pseudo creation order)")
        else:
            cat, note = "B1", "register binding differs, schedule aligned"
        rows = ", ".join(f"{x}->{y} x{n}" for (x, y), n in corr.most_common())
        first = example[corr.most_common(1)[0][0]]
        findings.append({"kind": "binding", "catalog": cat,
                         "detail": f"{note}: {rows}; e.g. `{first}`"})

    target_extra = sorted(_neg_slots(target_only) - _neg_slots(base))
    if target_extra:
        findings.append({
            "kind": "homing", "catalog": "B2/B3",
            "detail": ("reference touches frame slot(s) "
                       + ", ".join(f"[ebp-{s}]" for s in target_extra)
                       + " the base never does - memory-homed where the "
                         "base promotes")})
    base_extra = sorted(_neg_slots(base_only) - _neg_slots(target))
    if base_extra:
        findings.append({
            "kind": "homing", "catalog": "B2/B3",
            "detail": ("base touches frame slot(s) "
                       + ", ".join(f"[ebp-{s}]" for s in base_extra)
                       + " the reference never does - base homes a value "
                         "the reference keeps in a register")})

    target_spill = sorted(_param_store_slots(target_only)
                          - _param_store_slots(base))
    if target_spill:
        findings.append({
            "kind": "param-slot", "catalog": "B4",
            "detail": ("reference stores to parameter slot(s) "
                       + ", ".join(f"[ebp+{s}]" for s in target_spill)
                       + " - spill-to-(dead-)parameter-slot coloring")})
    base_spill = sorted(_param_store_slots(base_only)
                        - _param_store_slots(target))
    if base_spill:
        findings.append({
            "kind": "param-slot", "catalog": "B4",
            "detail": ("base stores to parameter slot(s) "
                       + ", ".join(f"[ebp+{s}]" for s in base_spill)
                       + " the reference does not")})

    saves_base, saves_target = _saves(base), _saves(target)
    if saves_base != saves_target:
        def _fmt(c):
            return ", ".join(sorted(c.elements())) or "(none)"
        findings.append({
            "kind": "save-restore", "catalog": "B1/B2",
            "detail": (f"callee-saved save/restore differs: base "
                       f"{_fmt(saves_base)} vs reference "
                       f"{_fmt(saves_target)} - a value promoted on one "
                       "side only (register pressure shifted)")})

    frame_base, frame_target = _frame_bytes(base), _frame_bytes(target)
    if frame_base != frame_target:
        findings.append({
            "kind": "frame-size", "catalog": "B2/B5",
            "detail": (f"frame allocation differs: base `sub esp, "
                       f"{frame_base or '0'}` vs reference `sub esp, "
                       f"{frame_target or '0'}` - a memory-homed value or "
                       "slot-coloring difference")})

    if (base_only or target_only) and sorted(r["vis"] for r in base) \
            == sorted(r["vis"] for r in target):
        findings.append({
            "kind": "schedule", "catalog": "B16/C3/C4",
            "detail": ("identical instruction MULTISET, transposed order - "
                       "post-RA schedule / store order, not a register "
                       "binding (statement order, chained-assignment "
                       "spelling, or the scheduler)")})

    reads_base, reads_target = _memref_counts(base), _memref_counts(target)
    for key in sorted(set(reads_base) | set(reads_target)):
        na, nt = reads_base.get(key, 0), reads_target.get(key, 0)
        if na and nt and na != nt and max(na, nt) >= 2:
            who = "reference re-reads" if nt > na else "base re-reads"
            findings.append({
                "kind": "reload", "catalog": "B13",
                "detail": (f"{key} referenced {na}x in base vs {nt}x in "
                           f"reference - {who} where the other caches "
                           "(cached-member-local vs reload)")})

    stats = {"flow_same": flow_same, "paired": paired,
             "binding_pairs": binding_pairs,
             "base_n": len(base), "target_n": len(target)}
    return findings, stats
