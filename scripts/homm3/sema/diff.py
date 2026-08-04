"""homm3.sema.diff - base-vs-target comparison of one function.

Both sides are the NORMALIZED objdiff copies (compiled base vs delinked
retail target) through one llvm-objdump, so only real differences
survive. Only delinked manifest units have comparison objects; use
`homm3 sema disasm` to view any retail function.

Renderings (the polarity is deliberate - the skeleton is the default,
the flat asm diff is the opt-in):
  (default)   block-skeleton diff: one row per block, FLOW/SIZE marks,
              five-way census, first branch-kind divergence
  --verbose   block-aligned body diff (per-block unified diffs)
  --asm       flat masked unified asm diff (the old sibling default)
  --branches  the ordered conditional-branch comparison the masked views
              structurally cannot show (SIGNEDNESS/POLARITY/OTHER/
              TOPOLOGY)

rc: 0 = the requested VIEW found no difference, 1 = it did, 2 = error.
The default skeleton compares flow shape + block sizes ONLY - a
function can differ inside a block (e.g. jb vs jl) and still exit 0
here; the below-100% hint then points at --branches. Conversely
--verbose/--asm can exit 1 on an objdiff-100% function: the delinked
target names data relocs synthetically (data_<rva>, or a neighbor
symbol + addend folded into the instruction immediate), so reloc
spellings and addend immediates differ across the sides without any
byte difference in the retail sense. Trust the skeleton + --branches
pair for control flow, objdiff for the match verdict.
"""
from __future__ import annotations

import re
import sys

from homm3.sema import _asm
from homm3.sema._common import die
from homm3.sema.context import get_context


def _hint_branches(ctx, rva: int, name: str, unit: str) -> None:
    """Fires only on the already-clean paths: a masked view with nothing
    to show on a function below 100% is exactly the --branches signal."""
    try:
        pct = ctx.fn_fuzzy(unit, name)
    except Exception:
        return
    if pct is None or pct >= 100.0:
        return
    print(f"[but this function is {pct:.2f}%, not 100 - and this view MASKS "
          "address operands, which also hides intra-function branch "
          f"displacements. Try `homm3 sema diff 0x{rva:08x} --branches`, "
          "which names each branch target by branch index.]")


# --- --branches: the ordered conditional-branch sequence, symbolically -------------
#
# The masked views hide intra-function branch DISPLACEMENTS: a `je` to a
# different block prints `je <addr>`/`je <tgt>` on both sides and compares
# EQUAL, so a real control-flow divergence can render as "identical asm".
# Do NOT "fix" that by unmasking - every function whose instruction sizes
# differ anywhere upstream would grow a +/- on every branch. The answer is
# naming branch targets SYMBOLICALLY - by the index of the first branch at
# or after them - so a uniform displacement shift compares equal and a
# genuine retarget does not.

_JCC_COND = frozenset(
    "je jne jz jnz jl jle jg jge ja jae jb jbe js jns jo jno jp jnp".split())
_JCC_UNCOND = frozenset(("jmp", "jmpl", "jmpw"))

# The signed/unsigned twins: two condition families over the same flags;
# picking the wrong one is a source-level type bug, not a codegen choice.
_SIGNED_TWIN = {"jl": "jb", "jb": "jl", "jle": "jbe", "jbe": "jle",
                "jg": "ja", "ja": "jg", "jge": "jae", "jae": "jge"}

_INVERSE = {"je": "jne", "jne": "je", "jz": "jnz", "jnz": "jz",
            "jl": "jge", "jge": "jl", "jle": "jg", "jg": "jle",
            "jb": "jae", "jae": "jb", "jbe": "ja", "ja": "jbe",
            "js": "jns", "jns": "js", "jo": "jno", "jno": "jo",
            "jp": "jnp", "jnp": "jp"}

_JALL = re.compile(r"^j[a-z]+$")
_JTGT = re.compile(r"^(-?0x[0-9a-f]+)\b")

# VC6 release codegen never emits these; one "decoding" out of embedded
# jump-table bytes marks exactly where linear decode left real code.
_NEVER_EMITTED = frozenset(("jecxz", "jcxz", "loop", "loope", "loopne",
                            "loopz", "loopnz"))


def _branch_insns(text: str) -> list:
    """[(offset, mnemonic, operand)] rebased so offset 0 is the first
    instruction. Branch TARGETS are rebased into the SAME space -
    normalizing offsets while leaving targets absolute would compare two
    address spaces and turn every function into a topology hit."""
    insns = []
    for ln in text.splitlines():
        p = _asm.parse_ins(ln)
        if p is None:
            continue
        off, body = p
        fields = body.split(None, 1)
        insns.append((off, fields[0].lower(), fields[1] if len(fields) > 1 else ""))
    while insns and insns[-1][1] == "nop":
        insns.pop()
    if not insns:
        return []
    base = insns[0][0]
    out = []
    for off, mn, op in insns:
        if _JALL.match(mn):
            t = _JTGT.match(op)
            if t:
                op = hex(int(t.group(1), 16) - base) + op[t.end():]
        out.append((off - base, mn, op))
    return out


def _first_bad(insns):
    """Offset where linear decode stopped being real instructions, or
    None. Jump-table data in .text is the usual cause; everything after
    it is unreliable, and callers must SAY the list is a partial prefix."""
    for off, mn, _ in insns:
        if mn == "(bad)" or mn.startswith("<") or mn in _NEVER_EMITTED:
            return off
    return None


def _branch_seq(insns, stop=None) -> list:
    """The ordered conditional-branch list [(offset, mnemonic, target|None)].
    An unclassified jump mnemonic dies rather than falling through - a jmp
    alias inside the conditional sequence fabricates flips."""
    out = []
    for off, mn, op in insns:
        if stop is not None and off >= stop:
            break
        if mn in _JCC_COND:
            m = _JTGT.match(op)
            out.append((off, mn, int(m.group(1), 16) if m else None))
        elif _JALL.match(mn) and mn not in _JCC_UNCOND:
            die(f"unclassified jump mnemonic {mn!r} - add it to _JCC_COND or "
                "_JCC_UNCOND, do not let it fall through")
    return out


def _ret_count(insns, stop=None) -> int:
    return sum(1 for off, mn, _ in insns
               if mn.startswith("ret") and (stop is None or off < stop))


def _sym_branch_target(brs, tgt):
    """The index of the first branch at or after `tgt` (len(brs) = past
    the last branch). This is what makes a uniform displacement shift
    compare EQUAL. Residue: two blocks both past the last branch, so
    TOPOLOGY under-reports at the epilogue."""
    if tgt is None:
        return None
    for i, (off, _, _) in enumerate(brs):
        if off >= tgt:
            return i
    return len(brs)


def _classify_flip(a: str, b: str) -> str:
    if _SIGNED_TWIN.get(a) == b:
        return "SIGNEDNESS"
    if _INVERSE.get(a) == b:
        return "POLARITY"
    return "OTHER"


def _branches_compare(bi, ti, max_flips: int = 4) -> dict:
    """Compare two instruction streams' branch sequences; see the renderer
    for the status vocabulary."""
    bstop, tstop = _first_bad(bi), _first_bad(ti)
    bb, tb = _branch_seq(bi, bstop), _branch_seq(ti, tstop)
    res = {"kind": None, "rows": [], "nbr": len(bb), "nbr_t": len(tb),
           "rets": (_ret_count(bi, bstop), _ret_count(ti, tstop)),
           "trunc": (bstop, tstop), "partial": False}
    # A jump table's bytes decode as garbage on one side and as plausible
    # instructions on the other, so the two truncation points are NOT
    # comparable; compare the common PREFIX and say so.
    if (bstop is not None or tstop is not None) and len(bb) != len(tb):
        keep = min(len(bb), len(tb))
        bb, tb = bb[:keep], tb[:keep]
        res["partial"] = True
    if len(bb) != len(tb):
        res["status"] = "struct"
        return res
    if not bb:
        res["status"] = "no-branches"
        return res
    flips = [(i, x[1], y[1]) for i, (x, y) in enumerate(zip(bb, tb)) if x[1] != y[1]]
    if len(flips) > max_flips:
        res["status"] = "many-flips"
        return res
    if flips:
        kinds = {_classify_flip(a, b) for _, a, b in flips}
        res["status"] = "flips"
        res["kind"] = ("SIGNEDNESS" if "SIGNEDNESS" in kinds else
                       "OTHER" if "OTHER" in kinds else "POLARITY")
        res["rows"] = flips
        return res
    # Same mnemonics everywhere: the only thing left a masked diff can
    # hide is a branch landing on a different block.
    bt = [_sym_branch_target(bb, t) for _, _, t in bb]
    tt = [_sym_branch_target(tb, t) for _, _, t in tb]
    moved = [(i, x, y) for i, (x, y) in enumerate(zip(bt, tt)) if x != y]
    if not moved:
        res["status"] = "clean"
    elif len(moved) > max_flips:
        res["status"] = "many-flips"
    else:
        res["status"] = "topology"
        res["kind"] = "TOPOLOGY"
        res["rows"] = moved
    return res


def _branch_view(ctx, rva, name, unit, ordinal) -> int:
    """Render the branch-sequence comparison of the two sides."""
    def trunc_note(side, insns):
        at = _first_bad(insns)
        if at is not None:
            print(f"[{side} stream truncated at +0x{at:x} - jump-table data "
                  "in .text; branch list is partial]")
        return at

    bi = _branch_insns(_asm.objdump(
        _asm.NORMAL_BASE / f"{unit}.obj", name, ordinal))
    ti = _branch_insns(_asm.objdump(
        _asm.NORMAL_TARGET / f"{unit}.c.obj", name, ordinal))
    print(f"[branch diff: BASE (compiled) vs TARGET (retail) @ 0x{rva:08x} {name}]")
    print("[targets are named by BRANCH INDEX, so a uniform displacement "
          "shift compares EQUAL and a genuine retarget does not]")
    bstop, tstop = trunc_note("base", bi), trunc_note("target", ti)
    res = _branches_compare(bi, ti)
    br, tr = res["rets"]
    dup = ("  DUP-EXIT (we duplicate an exit retail merges - respell the "
           "gate)" if br > tr else
           "  (retail has MORE exits than we do - the tail-merge direction)"
           if br < tr else "")
    print(f"  base {res['nbr']} branch(es), {br} ret(s)   |   "
          f"target {res['nbr_t']} branch(es), {tr} ret(s){dup}")
    status = res["status"]
    if status == "struct":
        print("  BRANCH COUNTS DIFFER - a structural difference (a block we "
              "did not reconstruct, a folded `if`, an inlining decision), "
              "NOT the one-line condition signal. Reconstruct, don't re-spell.")
        return 1
    if status == "many-flips":
        print("  more than 4 rows differ - the two functions are differently "
              "shaped and the positional pairing is meaningless.")
        return 1
    if status == "no-branches":
        print("  no conditional branches on either side.")
        return 0
    if status == "clean":
        scope = ("the compared PREFIX agrees" if res["partial"]
                 else "branch sequences AGREE")
        print(f"  {scope} (mnemonics and symbolic targets). Whatever is left "
              "is instruction selection / slot layout, not control flow.")
        return 0
    print(f"  {res['kind']}:")
    bb = _branch_seq(bi, bstop)
    for row in res["rows"]:
        i = row[0]
        if res["kind"] == "TOPOLOGY":
            _, x, y = row
            print(f"    #{i:<3} +{bb[i][0]:03x} {bb[i][1]:<4}  target lands "
                  f"on blk{y}, we land on blk{x}")
        else:
            _, a, b = row
            print(f"    #{i:<3} +{bb[i][0]:03x}  base {a:<4} -> target {b}")
    hint = {
        "SIGNEDNESS": "a signed/unsigned twin is nearly always a REAL source "
                      "bug - an operand that wants the other signedness",
        "POLARITY": "same test, opposite sense - read where each side's "
                    "branch GOES",
        "TOPOLOGY": "identical instruction for instruction - a branch just "
                    "lands on a different block; the shape masking hides "
                    "hardest",
        "OTHER": "neither a signed twin nor an inversion - read it by hand",
    }[res["kind"]]
    print(f"  [{hint}]")
    return 1


def run(args) -> None:
    ctx = get_context()
    name, unit, rva, _size, ordinal = ctx.symbols.resolve_fn(args.target)
    normal_base = _asm.NORMAL_BASE / f"{unit}.obj"
    normal_target = _asm.NORMAL_TARGET / f"{unit}.c.obj"
    if not (normal_base.is_file() and normal_target.is_file()):
        die(f"{name} [{unit or 'no unit'}] has no comparison objects - only "
            "delinked manifest units (config/units.toml) can diff; "
            "`homm3 sema disasm` views any retail function")
    if args.verbose and (args.asm or args.branches):
        die("--verbose modifies the default block diff; --asm and "
            "--branches each have one rendering")

    if args.branches:
        sys.exit(_branch_view(ctx, rva, name, unit, ordinal))

    base_text = _asm.objdump(normal_base, name, ordinal)
    target_text = _asm.objdump(normal_target, name, ordinal)

    if args.asm:
        import difflib
        base = _asm.norm(base_text)
        target = _asm.norm(target_text)
        if base == target:
            print(f"identical asm ({len(target)} instruction(s); "
                  "addresses/relocs masked)")
            _hint_branches(ctx, rva, name, unit)
            sys.exit(0)
        print(f"[asm diff: BASE (compiled) vs TARGET (retail) @ 0x{rva:08x} "
              f"{name}; addresses masked as <addr>]")
        for ln in difflib.unified_diff(base, target, "base", "target",
                                       lineterm=""):
            print(ln)
        sys.exit(1)

    if args.verbose:
        output, exact = _asm.blocks_diff(base_text, target_text)
    else:
        output, exact = _asm.skeleton_diff(_asm.cfg(base_text),
                                           _asm.cfg(target_text))
    print(output, end="")
    if exact:
        _hint_branches(ctx, rva, name, unit)
    sys.exit(0 if exact else 1)
