"""homm3.vc6._flow - branch-shape distance + control-flow diagnosis.

Support module for flow_model (`homm3 vc6 why-branch`), the control-flow
twin of _align (why-reg's support module). Same layering contract: every
piece of disassembly TEXT comes from homm3.sema._asm's two producers
(llvm-objdump over COFF objects, capstone over the retail image) in the
one shared row shape; this module never disassembles anything and never
builds its own CFG - it reuses _asm.parse_ins / _asm.cfg / _asm.branch_kind
(through _align.flow_kinds) and adds only the branch-shape metric and the
D-family diagnosis on top.

The metric is register-BLIND on purpose - the exact complement of _align's
register-visible distance. Three components, all structural:

  kinds   the per-block branch-kind skeleton (_align.flow_kinds: jcc/jmp/
          ret/fall/end + direction, block-index space - what makes the
          two producers comparable); unpaired slots counted difflib-style.
  tokens  the ordered conditional-branch sequence, each branch rendered
          "mnemonic#symbolic-target" where the symbolic target is the
          index of the first conditional branch at or after the target
          address (the `sema diff --branches` convention: a uniform
          displacement shift compares EQUAL, a genuine retarget does not).
  rets    the ret-count delta - the catalog's DUP-EXIT / merged-return
          signature (check_shipyard_square: "8 branches, 2 rets", D4/D5).

distance = unpaired(kinds) + unpaired(tokens) + |ret delta|. 0 <=> the two
branch shapes agree; whatever is left at 0 is register allocation /
instruction selection - why-reg's domain, not why-branch's.

Streams are clipped at the first impossible mnemonic ((bad) or the
never-emitted jecxz/loop* family): inline jump-table bytes decode as
garbage from there on, so a clipped profile carries `partial` and the
caller must say the comparison covers a prefix (same doctrine as
sema.diff's --branches view).
"""
from __future__ import annotations

import difflib
import re

from homm3.sema import _asm
from homm3.vc6 import _align

# Mnemonic tables mirroring sema.diff's --branches view (small stable ISA
# facts, restated rather than imported from that module's privates).
_JCC_COND = frozenset(
    "je jne jz jnz jl jle jg jge ja jae jb jbe js jns jo jno jp jnp".split())
_JCC_UNCOND = frozenset(("jmp", "jmpl", "jmpw"))
_SIGNED_TWIN = {"jl": "jb", "jb": "jl", "jle": "jbe", "jbe": "jle",
                "jg": "ja", "ja": "jg", "jge": "jae", "jae": "jge"}
_INVERSE = {"je": "jne", "jne": "je", "jz": "jnz", "jnz": "jz",
            "jl": "jge", "jge": "jl", "jle": "jg", "jg": "jle",
            "jb": "jae", "jae": "jb", "jbe": "ja", "ja": "jbe",
            "js": "jns", "jns": "js", "jo": "jno", "jno": "jo",
            "jp": "jnp", "jnp": "jp"}
# VC6 release codegen never emits these; one decoding out of embedded
# jump-table bytes marks where linear decode left real code.
_NEVER_EMITTED = frozenset(("jecxz", "jcxz", "loop", "loope", "loopne",
                            "loopz", "loopnz"))

_JANY = re.compile(r"^j[a-z]+$")
_DIRECT = re.compile(r"^(-?0x[0-9a-f]+)\b")


def _clip(text: str):
    """(text up to the first impossible mnemonic, truncation offset|None)."""
    out = []
    for ln in text.splitlines():
        p = _asm.parse_ins(ln)
        if p is not None:
            mn = p[1].split(None, 1)[0].lower()
            if mn == "(bad)" or mn.startswith("<") or mn in _NEVER_EMITTED:
                return "\n".join(out) + "\n", p[0]
        out.append(ln)
    return "\n".join(out) + "\n", None


def _sym_target(seq: list, tgt) -> str:
    """Index of the first conditional branch at or after `tgt` (len(seq) =
    past the last branch; '?' = indirect). Same address space on both ends
    of the comparison, so no rebasing is needed."""
    if tgt is None:
        return "?"
    for i, (off, _mn, _t) in enumerate(seq):
        if off >= tgt:
            return str(i)
    return str(len(seq))


def profile(text: str) -> dict:
    """One side's branch-shape profile from producer text."""
    clipped, trunc = _clip(text)
    insns = []
    for off, body in _asm.code_insns(clipped):
        fields = body.split(None, 1)
        insns.append((off, fields[0].lower(),
                      fields[1] if len(fields) > 1 else ""))
    while insns and insns[-1][1] == "nop":
        insns.pop()

    seq = []
    for off, mn, op in insns:
        # An unlisted j* mnemonic stays in the sequence under its own name:
        # it can only pair against itself, which is safe in a distance and
        # honest in a diff (mirrors sema.diff's refusal to let one slip).
        if mn in _JCC_COND or (_JANY.match(mn) and mn not in _JCC_UNCOND):
            m = _DIRECT.match(op)
            seq.append((off, mn, int(m.group(1), 16) if m else None))
    tokens = [f"{mn}#{_sym_target(seq, tgt)}" for _off, mn, tgt in seq]

    graph = _asm.cfg(clipped)
    back_jcc = back_jmp = 0
    for _addr, _body, term in graph:
        if "^" not in (term or ""):
            continue
        if (term or "").startswith("jmp"):
            back_jmp += 1
        else:
            back_jcc += 1

    return {
        "kinds": _align.flow_kinds(clipped),
        "tokens": tokens,
        "rets": sum(1 for _o, mn, _p in insns if mn.startswith("ret")),
        "blocks": len(graph),
        "n": len(insns),
        "back_jcc": back_jcc,
        "back_jmp": back_jmp,
        "indirect": sum(1 for _o, mn, op in insns
                        if mn == "jmp" and "[" in op),
        "branchless": sum(1 for _o, mn, _p in insns
                          if mn.startswith("set") or mn == "sbb"),
        "partial": trunc is not None,
    }


def _unpaired(a: list, b: list) -> int:
    sm = difflib.SequenceMatcher(a=a, b=b, autojunk=False)
    matched = sum(blk.size for blk in sm.get_matching_blocks())
    return len(a) + len(b) - 2 * matched


def distance(base: dict, target: dict) -> int:
    """Branch-shape disagreement count between two profiles; 0 = the flow
    skeletons, conditional-branch sequences and exit counts all agree."""
    return (_unpaired(base["kinds"], target["kinds"])
            + _unpaired(base["tokens"], target["tokens"])
            + abs(base["rets"] - target["rets"]))


# --- diagnosis ---------------------------------------------------------------------


def _mn(token: str) -> str:
    return token.split("#", 1)[0]


def _tg(token: str) -> str:
    return token.split("#", 1)[1]


def _classify_flip(a: str, b: str) -> str:
    if _SIGNED_TWIN.get(a) == b:
        return "SIGNEDNESS"
    if _INVERSE.get(a) == b:
        return "POLARITY"
    return "OTHER"


def diagnose(bp: dict, rp: dict) -> tuple:
    """(findings, stats): classify the branch-shape divergence into the
    behavior catalog's D families. Heuristic mapping; the catalog IDs are
    docs/vc6/behavior-catalog.md's, and the VC6 compile - not this table -
    is the verdict on any proposed fix."""
    findings = []
    bt, rt = bp["tokens"], rp["tokens"]
    nb, nr = len(bt), len(rt)
    rb, rr = bp["rets"], rp["rets"]

    if bp["partial"] or rp["partial"]:
        who = ("both sides" if bp["partial"] and rp["partial"] else
               "the base" if bp["partial"] else "the reference")
        findings.append({
            "kind": "partial", "catalog": "(decode)",
            "detail": (f"{who} clipped at an impossible mnemonic - inline "
                       "jump-table data (a switch?); the comparison covers "
                       "the decoded prefix only")})

    if nb != nr:
        findings.append({
            "kind": "structural", "catalog": "STRUCT",
            "detail": (f"branch COUNTS DIFFER: base {nb} vs reference {nr} "
                       "conditional branch(es) - a block not reconstructed, "
                       "a folded if, or an inlining decision (the `sema "
                       "diff --branches` structural signal). The loop-form "
                       "and exit-merge knobs legitimately add/remove a "
                       "compare site, so the search still runs")})
    elif nb:
        mnflips = [(i, _mn(a), _mn(b)) for i, (a, b) in enumerate(zip(bt, rt))
                   if _mn(a) != _mn(b)]
        retargets = [(i, _tg(a), _tg(b)) for i, (a, b)
                     in enumerate(zip(bt, rt))
                     if _mn(a) == _mn(b) and _tg(a) != _tg(b)]
        if mnflips:
            kinds = {_classify_flip(a, b) for _i, a, b in mnflips}
            rows = ", ".join(f"#{i} {a}->{b}" for i, a, b in mnflips[:4])
            if "SIGNEDNESS" in kinds:
                note = ("signed/unsigned twin - nearly always a REAL "
                        "source-type bug, not a spelling knob")
            elif kinds == {"POLARITY"}:
                note = ("opposite branch sense - arm order / fall-through "
                        "path (swap the if/else arms, or the D13 negative-"
                        "vs-positive case spelling)")
            else:
                note = "different condition computed"
            findings.append({"kind": "flip", "catalog": "D8/D13",
                             "detail": f"{note}: {rows}"})
        if retargets:
            rows = ", ".join(f"#{i} ->br{y} (base ->br{x})"
                             for i, x, y in retargets[:4])
            findings.append({
                "kind": "topology", "catalog": "D3/D7/D12",
                "detail": ("same branch mnemonics, different landing "
                           f"blocks: {rows} - jump-threading / cross-"
                           "jumping territory (often NOT source-"
                           "addressable, see D3; the two-break for(;;) "
                           "routing is the D12 spelling)")})

    if bp["back_jcc"] > rp["back_jcc"] and bp["back_jmp"] < rp["back_jmp"]:
        findings.append({
            "kind": "loop-rotation", "catalog": "D1/D2",
            "detail": (f"base loop is ROTATED (conditional back edge x"
                       f"{bp['back_jcc']}) where the reference is top-"
                       f"tested-unrotated (jmp back edge x{rp['back_jmp']})"
                       " - the `while (1) { if (!(c)) break; .. }` family")})
    elif rp["back_jcc"] > bp["back_jcc"] and rp["back_jmp"] < bp["back_jmp"]:
        findings.append({
            "kind": "loop-rotation", "catalog": "D1/D2",
            "detail": (f"reference loop is ROTATED (conditional back edge x"
                       f"{rp['back_jcc']}) where the base is top-tested "
                       f"(jmp back edge x{bp['back_jmp']}) - respell as a "
                       "plain `while (c)` / do-while so VC6 rotates it")})

    if rb > rr:
        findings.append({
            "kind": "exit-merge", "catalog": "D4/D5",
            "detail": (f"DUP-EXIT: base {rb} ret(s) vs reference {rr} - we "
                       "duplicate an exit the reference merges. Respell the "
                       "gate: nested single-return (D5) or the goto-into-"
                       "shared-block merged return (D4)")})
    elif rr > rb:
        findings.append({
            "kind": "exit-merge", "catalog": "D6",
            "detail": (f"reference has MORE exits ({rr} ret(s) vs base "
                       f"{rb}) - the tail-duplication direction: give each "
                       "arm its own return / duplicate the mutation per "
                       "arm so VC6 lays out retail's epilogues")})

    if bool(bp["indirect"]) != bool(rp["indirect"]):
        who, other = (("reference", "base") if rp["indirect"]
                      else ("base", "reference"))
        findings.append({
            "kind": "dispatch", "catalog": "D9",
            "detail": (f"{who} dispatches through a jump table (indirect "
                       f"jmp) where the {other} compares serially - switch "
                       "vs if-chain; remember case bodies emit in SOURCE "
                       "order, not ascending")})

    if rp["branchless"] > bp["branchless"] and nb > nr:
        findings.append({
            "kind": "branchless", "catalog": "D8/D13",
            "detail": (f"reference materializes branchlessly (setcc/sbb x"
                       f"{rp['branchless']} vs base x{bp['branchless']}) "
                       "where the base branches - a ternary selector (D8) "
                       "or bool-flag spelling (D13) folds the branch")})
    elif bp["branchless"] > rp["branchless"] and nr > nb:
        findings.append({
            "kind": "branchless", "catalog": "D8/D13",
            "detail": (f"base is branchless (setcc/sbb x{bp['branchless']} "
                       f"vs reference x{rp['branchless']}) where the "
                       "reference BRANCHES - unfold the ternary into an "
                       "if/else (the TPickANumber clamp shape)")})

    stats = {"base_n": bp["n"], "target_n": rp["n"],
             "base_branches": nb, "target_branches": nr,
             "base_rets": rb, "target_rets": rr,
             "base_blocks": bp["blocks"], "target_blocks": rp["blocks"],
             "partial": bp["partial"] or rp["partial"]}
    return findings, stats
