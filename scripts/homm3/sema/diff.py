"""homm3.sema.diff - base-vs-target comparison of one function.

Both sides are the NORMALIZED objdiff copies (compiled base vs delinked
retail target) through one llvm-objdump, so only real differences
survive. Only delinked manifest units have comparison objects; use
`homm3 sema disasm` to view any retail function.

Renderings (the polarity is deliberate - the skeleton is the default,
the flat asm diff is the opt-in):
  (default)   block-skeleton diff: one row per block, FLOW/SIZE marks,
              five-way census, first branch-kind divergence
  --structure explicit spelling of the default block-skeleton diff
  --asm       flat masked unified asm diff (the old sibling default)
  --branches  the ordered conditional-branch comparison the masked views
              structurally cannot show (SIGNEDNESS/POLARITY/OTHER/
              TOPOLOGY)
  --source    block/instruction alignment grouped beneath verified candidate
              /Z7 source statements; metadata is attached after comparison.
              A compiler-generated body (no /Z7 statements) falls back to
              the block-skeleton diff with a note instead of failing.
  --calls     the ordered callee sequences, judged like `objdiff-cli diff`
              (function_reloc_diffs=name_address); an unclaimed retail label
              is marked, and the ratchet's =none verdict is stated beside it
  --relocs    the same over every relocation, calls and data
  --summary   one screen: every view's verdict, the first divergence, the
              next view to run
  --why-bytes --summary plus the first byte-level divergence unmasked
  --range     restrict both sides to one end-exclusive function-local span
  --base-range/--target-range
              independently select corresponding spans when prior codegen
              gives the candidate and retail arm different local offsets
  --verbose   more of the chosen view: block bodies for the default and
              --structure, the whole listing as context for --asm, both
              branch sequences for --branches, unchanged statement groups
              expanded for --source

Before comparing, the unit is refreshed in place (its ninja target, its
normalized copies, the objdiff report) so an edit needs no separate build:
free when nothing changed, one VC6 compile otherwise. --no-build compares
the last built object instead.

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
from homm3.sema import source as source_view
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
    for off, body in _asm.code_insns(text):
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


def _branch_view(base_text, target_text, rva, name, verbose: bool = False) -> int:
    """Render the branch-sequence comparison of the two sides; --verbose
    appends both full branch sequences after the verdict."""
    def trunc_note(side, insns):
        at = _first_bad(insns)
        if at is not None:
            print(f"[{side} stream truncated at +0x{at:x} - jump-table data "
                  "in .text; branch list is partial]")
        return at

    bi = _branch_insns(base_text)
    ti = _branch_insns(target_text)
    print(f"[branch diff: BASE (compiled) vs TARGET (retail) @ 0x{rva:08x} {name}]")
    print("[targets are named by BRANCH INDEX, so a uniform displacement "
          "shift compares EQUAL and a genuine retarget does not]")
    bstop, tstop = trunc_note("base", bi), trunc_note("target", ti)
    res = _branches_compare(bi, ti)
    if verbose:
        for side, seq in (("base", _branch_seq(bi, bstop)),
                          ("target", _branch_seq(ti, tstop))):
            print(f"  {side} branch sequence ({len(seq)}):")
            for i, (off, mn, target) in enumerate(seq):
                where = f"-> +{target:03x}" if target is not None else "-> <ext>"
                print(f"    #{i:<3} +{off:03x} {mn:<5} {where}")
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


# --- --source: candidate statements attached AFTER instruction alignment ----------

def _align_bodies(base, target):
    """[(kind, base-index|None, target-index|None)] over two block bodies
    of ``(offset, masked text)`` rows: ``=`` equal, ``~`` replaced pair,
    ``-`` base-only, ``+`` target-only. A replace run pairs its shared
    prefix positionally and leaves the remainder one-sided."""
    import difflib

    bt = [text for _off, text in base]
    tt = [text for _off, text in target]
    out = []
    matcher = difflib.SequenceMatcher(a=bt, b=tt, autojunk=False)
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            out.extend(("=", bi, ti) for bi, ti in zip(range(i1, i2), range(j1, j2)))
            continue
        if tag == "delete":
            out.extend(("-", bi, None) for bi in range(i1, i2))
            continue
        if tag == "insert":
            out.extend(("+", None, ti) for ti in range(j1, j2))
            continue
        shared = min(i2 - i1, j2 - j1)
        out.extend(("~", i1 + k, j1 + k) for k in range(shared))
        out.extend(("-", bi, None) for bi in range(i1 + shared, i2))
        out.extend(("+", None, ti) for ti in range(j1 + shared, j2))
    return out


def _aligned_instruction_rows(base, target):
    """[(kind, base-anchor, base-text, target-text)] for two block bodies.

    Insertions borrow the nearest candidate offset solely for source grouping;
    their base text remains None.  Metadata therefore never participates in
    the SequenceMatcher or in the comparison verdict.
    """
    import difflib

    bt = [text for _off, text in base]
    tt = [text for _off, text in target]
    out = []
    matcher = difflib.SequenceMatcher(a=bt, b=tt, autojunk=False)
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            for bi, ti in zip(range(i1, i2), range(j1, j2)):
                out.append(("=", base[bi][0], base[bi][1], target[ti][1]))
            continue
        if tag == "delete":
            out.extend(("-", base[bi][0], base[bi][1], None)
                       for bi in range(i1, i2))
            continue
        if tag == "insert":
            anchor = (base[i1][0] if i1 < len(base)
                      else base[i1 - 1][0] if i1 else 0)
            out.extend(("+", anchor, None, target[ti][1])
                       for ti in range(j1, j2))
            continue
        shared = min(i2 - i1, j2 - j1)
        for offset in range(shared):
            bi, ti = i1 + offset, j1 + offset
            out.append(("~", base[bi][0], base[bi][1], target[ti][1]))
        for bi in range(i1 + shared, i2):
            out.append(("-", base[bi][0], base[bi][1], None))
        anchor = (base[i1 + shared - 1][0] if shared
                  else base[i1][0] if i1 < len(base)
                  else base[i1 - 1][0] if i1 else 0)
        out.extend(("+", anchor, None, target[ti][1])
                   for ti in range(j1 + shared, j2))
    return out


def _statement_groups(rows, source_map, origin: int):
    groups = []
    for row in rows:
        statements = source_map.active_at(max(row[1] - origin, 0))
        key = tuple((statement.offset, statement.line)
                    for statement in statements)
        if not groups or groups[-1][0] != key:
            groups.append((key, statements, []))
        groups[-1][2].append(row)
    return groups


def _render_source_block(out, block_number, base_block, target_block,
                         source_map, origin, first_changed,
                         verbose: bool = False):
    base_addr, base_body, base_term = base_block
    target_addr, target_body, target_term = target_block
    rows = _aligned_instruction_rows(base_body, target_body)
    if base_term != target_term:
        anchor = base_body[-1][0] if base_body else base_addr
        rows.append(("flow", anchor, base_term or "", target_term or ""))
    changed = any(row[0] != "=" for row in rows)
    out.append(f"  B{block_number} @{base_addr:x}/@{target_addr:x}  "
               f"{'DIFFERS' if changed else '=='}:")
    for _key, statements, grouped_rows in _statement_groups(
            rows, source_map, origin):
        group_changed = any(row[0] != "=" for row in grouped_rows)
        if statements:
            for statement in statements:
                out.append("    " + source_view.format_heading(
                    statement, source_map.source, group_changed))
            if group_changed and first_changed[0] is None:
                first_changed[0] = statements[-1]
        else:
            out.append("    ; !! [no candidate source statement]"
                       if group_changed else
                       "    ; [no candidate source statement]")
        if not group_changed and not verbose:
            out.append(f"      == {len(grouped_rows)} instruction(s)")
            continue
        for kind, _anchor, base_text, target_text in grouped_rows:
            if kind == "=":
                out.append(f"       = {base_text}")
            elif kind == "~":
                out.append(f"       ~ base   {base_text}")
                out.append(f"         target {target_text}")
            elif kind == "-":
                out.append(f"       - base   {base_text}")
            elif kind == "+":
                out.append(f"       + target {target_text}")
            else:
                out.append(f"       ! flow   base [{base_text}] -> "
                           f"target [{target_text}]")


def _source_diff(base_text: str, target_text: str, source_map,
                 verbose: bool = False) -> tuple[str, bool]:
    """Statement-grouped block diff; comments are attached after alignment.
    --verbose expands the unchanged statement groups too."""
    text, exact, _first = _source_diff_full(base_text, target_text,
                                            source_map, verbose)
    return text, exact


def _source_diff_full(base_text: str, target_text: str, source_map,
                      verbose: bool = False):
    """(text, exact, first divergent Statement | None)."""
    import difflib

    base_cfg = _asm.cfg_rows(base_text)
    target_cfg = _asm.cfg_rows(target_text)
    _ordinary_output, exact = _asm.blocks_diff(base_text, target_text)
    base_keys = ["\n".join([text for _off, text in body] + [term or ""])
                 for _address, body, term in base_cfg]
    target_keys = ["\n".join([text for _off, text in body] + [term or ""])
                   for _address, body, term in target_cfg]
    origin = next((body[0][0] for _address, body, _term in base_cfg if body), 0)
    out = [f"[source diff: base {len(base_cfg)} blocks vs target "
           f"{len(target_cfg)} blocks; candidate statements from "
           f"{source_map.source}; metadata excluded from comparison]"]
    first_changed = [None]
    matcher = difflib.SequenceMatcher(a=base_keys, b=target_keys, autojunk=False)
    display_block = 0
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            for offset in range(i2 - i1):
                _render_source_block(
                    out, display_block, base_cfg[i1 + offset],
                    target_cfg[j1 + offset], source_map, origin, first_changed,
                    verbose)
                display_block += 1
            continue
        for offset in range(max(i2 - i1, j2 - j1)):
            bi = i1 + offset if i1 + offset < i2 else None
            ti = j1 + offset if j1 + offset < j2 else None
            if bi is not None and ti is not None:
                _render_source_block(
                    out, display_block, base_cfg[bi], target_cfg[ti],
                    source_map, origin, first_changed, verbose)
            elif bi is not None:
                address, body, term = base_cfg[bi]
                empty = (address, [], None)
                _render_source_block(
                    out, display_block, (address, body, term), empty,
                    source_map, origin, first_changed, verbose)
            else:
                address, body, term = target_cfg[ti]
                out.append(f"  B{display_block} @{address:x} TARGET-ONLY:")
                out.append("    ; !! [no candidate statement for target-only block]")
                out.extend(f"       + target {text}" for _off, text in body)
                if term:
                    out.append(f"       + flow   [{term}]")
            display_block += 1
    if first_changed[0] is not None:
        statement = first_changed[0]
        out.insert(1, f"[first divergent candidate statement: "
                   f"{source_map.source}:{statement.line} | {statement.text}]")
    out.append("[all aligned blocks identical]" if exact
               else "[source-aware view differs]")
    return "\n".join(out) + "\n", exact, first_changed[0]


# --- --calls / --relocs: the ordered reference sequences ---------------------------
#
# What objdiff compares, spelled out. `objdiff-cli diff` (the interactive
# tool) defaults to function_reloc_diffs=name_address: a relocation pair is
# equal when its kind, symbol name and addend match. `objdiff-cli report
# generate` (the ratchet) runs at =none and ignores the target entirely,
# which is why a function calling `exe_fopen` where we call `_fopen` scores
# 100 there. These views judge like the interactive default and state the
# report-level verdict next to it. The retail side names an UNCLAIMED
# callee/global with a carve label (sub_f6570, data_2a5d5c, exe_new) that
# MSVC can never emit - the same syntactic rule vc6/inline_model.py:81 uses
# (sema must not import vc6, so the regex is repeated here).

_EMITTABLE = re.compile(r"^[?_@]")
_IND_DISP = re.compile(r"[+-]\s*(0x[0-9a-f]+)\s*\]")
_HEX_LIT = re.compile(r"0x[0-9a-f]+")


def _synthetic(name: str) -> str | None:
    """Why *name* cannot have been spelled by our compiler, or None."""
    if name.startswith("$L"):
        return "local"
    if name.startswith("__h3cg$"):
        return "compgen"
    if not _EMITTABLE.match(name):
        return "unclaimed"
    return None


def _field(raw: bytes, site: int, off: int):
    """The relocated 32-bit field's value (the addend), or None when the
    site is not inside this instruction's bytes."""
    k = site - off
    if 0 <= k and k + 4 <= len(raw):
        return int.from_bytes(raw[k:k + 4], "little")
    return None


def _ref_seq(text: str) -> tuple[list, int | None]:
    """[(offset, kind, symbol, addend, insn_text)] for every relocation the
    function's CODE carries, in instruction order, offsets rebased to the
    first instruction (the target side is not rebased by objdump); plus
    the truncation offset where linear decode stopped being code (None =
    clean). Kinds: call / jmp (a reloc'd flow target), ind (a call with no
    reloc, keyed by its displacement so a register choice does not count),
    data (a DIR32 on any other instruction)."""
    keep = {off for off, _ in _asm.code_insns(text)}
    rows = [row for row in _asm.reloc_rows(text) if row[0] in keep]
    while rows and rows[-1][2].split(None, 1)[0].lower() == "nop":
        rows.pop()
    if not rows:
        return [], None
    origin = rows[0][0]
    stop = _first_bad(_branch_insns(text))
    refs = []
    for off, raw, body, relocs in rows:
        rel = off - origin
        if stop is not None and rel >= stop:
            break
        clean = _asm._NOTE.sub("", body).strip()
        mnemonic, _sp, operands = clean.partition(" ")
        mnemonic = mnemonic.lower()
        if mnemonic in ("call", "jmp"):
            if relocs:
                for site, _kind, symbol in relocs:
                    refs.append((rel, mnemonic, symbol, _field(raw, site, off),
                                 clean))
            elif mnemonic == "call":
                match = _IND_DISP.search(operands.lower())
                disp = match.group(1) if match else "0x0"
                refs.append((rel, "ind", f"<indirect>[+{disp}]", 0, clean))
            continue
        for site, _kind, symbol in relocs:
            refs.append((rel, "data", symbol, _field(raw, site, off), clean))
    return refs, stop


def _refs_compare(base_refs: list, target_refs: list) -> dict:
    """Pair the two reference sequences and judge every pair.

    rows: [(tag, base_ref|None, target_ref|None, note)] with tag = same /
    ~ different / - base-only / + target-only; note = why the retail name
    could never match by spelling (unclaimed / local / compgen) or None.
    agree = no ~/-/+ (objdiff name_address); report_agree = what the
    ratchet's function_reloc_diffs=none sees: every pair the same kind and
    nothing one-sided. Pairing is a SequenceMatcher over (class, name)
    keys where a name the other side never spells is a wildcard, so one
    retail-inlined call shifts nothing - positional pairing would mislabel
    every later row."""
    import difflib
    from collections import Counter

    def cls(kind):
        return "data" if kind == "data" else "call"

    base_names = {(cls(k), s) for _o, k, s, _a, _t in base_refs}
    target_names = {(cls(k), s) for _o, k, s, _a, _t in target_refs}

    def keys(refs, other):
        out = []
        for _off, kind, symbol, _addend, _text in refs:
            key = (cls(kind), symbol)
            out.append(key if kind == "ind" or key in other else (cls(kind), "*"))
        return out

    def judge(b, t):
        _bo, bk, bs, ba, _bt = b
        _to, tk, ts, ta, _tt = t
        if bk == tk and bs == ts and ba == ta:
            return "=", None
        return "~", _synthetic(ts) or _synthetic(bs)

    rows = []
    matcher = difflib.SequenceMatcher(a=keys(base_refs, target_names),
                                      b=keys(target_refs, base_names),
                                      autojunk=False)
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            for bi, ti in zip(range(i1, i2), range(j1, j2)):
                verdict, note = judge(base_refs[bi], target_refs[ti])
                rows.append((verdict, base_refs[bi], target_refs[ti], note))
            continue
        if tag == "delete":
            rows.extend(("-", base_refs[bi], None, None) for bi in range(i1, i2))
            continue
        if tag == "insert":
            rows.extend(("+", None, target_refs[ti], None) for ti in range(j1, j2))
            continue
        shared = min(i2 - i1, j2 - j1)
        for k in range(shared):
            verdict, note = judge(base_refs[i1 + k], target_refs[j1 + k])
            rows.append((verdict, base_refs[i1 + k], target_refs[j1 + k], note))
        rows.extend(("-", base_refs[bi], None, None) for bi in range(i1 + shared, i2))
        rows.extend(("+", None, target_refs[ti], None) for ti in range(j1 + shared, j2))
    counts = Counter(row[0] for row in rows)
    counts["synthetic"] = sum(1 for row in rows if row[0] == "~" and row[3])
    counts["real"] = counts["~"] - counts["synthetic"]
    agree = counts["~"] + counts["-"] + counts["+"] == 0
    report_agree = (counts["-"] + counts["+"] == 0 and all(
        cls(row[1][1]) == cls(row[2][1]) for row in rows if row[0] in "=~"))
    return {"rows": rows, "counts": counts, "agree": agree,
            "report_agree": report_agree}


def _ref_name(ref) -> str:
    _off, kind, symbol, addend, _text = ref
    if kind != "ind" and addend:
        signed = addend - (1 << 32) if addend & 0x80000000 else addend
        return f"{symbol}{signed:+#x}"
    return symbol


def _refs_view(base_text: str, target_text: str, rva: int, name: str,
               calls_only: bool, verbose: bool = False) -> tuple[str, bool]:
    """Render --calls / --relocs; (text, agree)."""
    base_refs, bstop = _ref_seq(base_text)
    target_refs, tstop = _ref_seq(target_text)
    flow = ("call", "jmp", "ind")
    if calls_only:
        base_refs = [r for r in base_refs if r[1] in flow]
        target_refs = [r for r in target_refs if r[1] in flow]
    what = "call" if calls_only else "reloc"
    out = [f"[{what} diff: BASE (compiled) vs TARGET (retail) @ 0x{rva:08x} {name}]",
           "[judged like `objdiff-cli diff` (function_reloc_diffs=name_address): "
           "kind, symbol and addend must match; the ratchet report runs at =none "
           "and ignores names]"]
    for side, stop in (("base", bstop), ("target", tstop)):
        if stop is not None:
            out.append(f"[{side} stream truncated at +0x{stop:x} - jump-table "
                       "data in .text; sequence is partial]")

    def census(refs):
        if calls_only:
            direct = sum(1 for r in refs if r[1] in ("call", "jmp"))
            ind = sum(1 for r in refs if r[1] == "ind")
            return f"{len(refs)} call(s) ({direct} direct, {ind} indirect)"
        kinds = {k: sum(1 for r in refs if r[1] == k) for k in ("call", "jmp", "ind", "data")}
        return (f"{len(refs)} ref(s): {kinds['call']} call, {kinds['jmp']} jmp, "
                f"{kinds['ind']} indirect, {kinds['data']} data")

    if not base_refs and not target_refs:
        out.append(f"  no {'calls' if calls_only else 'relocations'} on either side.")
        return "\n".join(out) + "\n", True
    out.append(f"  base {census(base_refs)}   |   target {census(target_refs)}")
    res = _refs_compare(base_refs, target_refs)
    for i, (tag, b, t, note) in enumerate(res["rows"]):
        kind_col = "" if calls_only else f"{(b or t)[1]:<5}"
        where = f"+{b[0]:03x}" if b else f"t+{t[0]:03x}"
        if tag == "=":
            text = _ref_name(b)
        elif tag == "~":
            text = f"{_ref_name(b)} -> {_ref_name(t)}"
            if note:
                text += f"  ({'retail label - ' if note == 'unclaimed' else ''}{note})"
        elif tag == "-":
            text = _ref_name(b)
        else:
            text = f"-> {_ref_name(t)}"
        out.append(f"  #{i:<3} {where:<6} {tag}  {kind_col}{text}")
        if verbose and tag != "=":
            if b:
                out.append(f"        base   +{b[0]:03x}: {b[4]}")
            if t:
                out.append(f"        target t+{t[0]:03x}: {t[4]}")
    c = res["counts"]
    out.append(f"  {c['=']} same, {c['~']} different ({c['synthetic']} unclaimed "
               f"retail labels, {c['real']} real), {c['-']} base-only, "
               f"{c['+']} target-only")
    seq = "CALL SEQUENCES" if calls_only else "REFERENCE SEQUENCES"
    if res["agree"]:
        out.append(f"  name_address: {seq} AGREE   |   report (none): AGREE - "
                   "whatever is left is not the reference structure.")
    else:
        report = "AGREE" if res["report_agree"] else "DIFFER"
        if c["-"] or c["+"]:
            hint = ("a one-sided reference is an inlining decision (we call out "
                    "of line what retail expanded, or the reverse); fix the "
                    "callee's inline shape before touching registers")
        elif c["real"]:
            hint = ("a different symbol at the same position - the wrong "
                    "overload/helper, or a mislabeled retail function; check it "
                    "with `homm3 sema rva`")
        else:
            hint = (f"all {c['synthetic']} differing rows are unclaimed retail "
                    "labels; claim them (VA) to compare by name")
        if c["synthetic"] and (c["-"] or c["+"] or c["real"]):
            hint += f"; claim the {c['synthetic']} unclaimed callees (VA) to compare them by name"
        out.append(f"  name_address: {seq} DIFFER   |   report (none): {report} - {hint}.")
    return "\n".join(out) + "\n", res["agree"]


# --- --summary / --why-bytes: every verdict on one screen ---------------------------

_DIVERGENCE_KINDS = (
    "[kinds: missing = one-sided instruction; flow = same instructions, different "
    "branch target; opcode = different mnemonic; immediate = different literal; "
    "register = different register operands; encoding = same asm, different bytes; "
    "reloc-target = same bytes, a different symbol or addend ('unclaimed' = a retail "
    "label our side cannot emit, which the ratchet report ignores)]")


def _zeroed(raw: bytes, off: int, relocs) -> bytes:
    data = bytearray(raw)
    for site, _kind, _symbol in relocs:
        k = site - off
        if 0 <= k and k + 4 <= len(data):
            data[k:k + 4] = b"\0\0\0\0"
    return bytes(data)


def _first_divergence(base_text: str, target_text: str) -> dict | None:
    """The first place the two sides' BYTES disagree, walking the aligned
    blocks and the aligned instructions inside them; None when nothing
    differs. A divergence that is only a reloc symbol spelling
    (``cosmetic``) is reported only when nothing real follows."""
    import difflib

    base_cfg = _asm.cfg_rows(base_text)
    target_cfg = _asm.cfg_rows(target_text)
    braw = {off: (raw, body, relocs) for off, raw, body, relocs in _asm.reloc_rows(base_text)}
    traw = {off: (raw, body, relocs) for off, raw, body, relocs in _asm.reloc_rows(target_text)}
    borigin = next((body[0][0] for _a, body, _t in base_cfg if body), 0)
    torigin = next((body[0][0] for _a, body, _t in target_cfg if body), 0)

    def row(table, off):
        raw, body, relocs = table.get(off, (b"", "", []))
        return {"offset": off, "raw": raw, "text": _asm._NOTE.sub("", body).strip(),
                "relocs": [(k, s, _field(raw, site, off)) for site, k, s in relocs]}

    def hit(kind, block, bblock, tblock, brow, trow, prev, note=None):
        return {"kind": kind, "cosmetic": kind == "reloc-target" and note is not None,
                "note": note, "block": block,
                "base_addr": bblock[0] if bblock else None,
                "target_addr": tblock[0] if tblock else None,
                "base_row": brow, "target_row": trow, "context": prev,
                "base_origin": borigin, "target_origin": torigin}

    def in_block(block, bblock, tblock):
        baddr, bbody, bterm = bblock
        taddr, tbody, tterm = tblock
        prev = None
        cosmetic = None
        for kind, bi, ti in _align_bodies(bbody, tbody):
            if bi is None or ti is None:
                brow = row(braw, bbody[bi][0]) if bi is not None else None
                trow = row(traw, tbody[ti][0]) if ti is not None else None
                return hit("missing", block, bblock, tblock, brow, trow, prev)
            brow, trow = row(braw, bbody[bi][0]), row(traw, tbody[ti][0])
            bmn = brow["text"].split(None, 1)[0].lower() if brow["text"] else ""
            tmn = trow["text"].split(None, 1)[0].lower() if trow["text"] else ""
            if bmn != tmn:
                return hit("opcode", block, bblock, tblock, brow, trow, prev)
            if _JALL.match(bmn) and bi == len(bbody) - 1 and ti == len(tbody) - 1:
                # the block's branch: its DISPLACEMENT differing is a
                # consequence of whatever changed size in between unless
                # the symbolic target block differs too - that is flow
                if bterm != tterm:
                    return hit("flow", block, bblock, tblock, brow, trow, prev,
                               note=f"[{bterm}] vs [{tterm}]")
                if brow["raw"] != trow["raw"]:
                    shifted = hit("flow", block, bblock, tblock, brow, trow, prev,
                                  note="same target block; the displacement "
                                       "shifted with a size change in between")
                    shifted["cosmetic"] = True
                    cosmetic = cosmetic or shifted
                prev = (brow, trow)
                continue
            zb = _zeroed(brow["raw"], brow["offset"], braw.get(brow["offset"], (b"", "", []))[2])
            zt = _zeroed(trow["raw"], trow["offset"], traw.get(trow["offset"], (b"", "", []))[2])
            if zb != zt:
                bt, tt = brow["text"].lower(), trow["text"].lower()
                if bt == tt:
                    kind = "encoding"
                elif _HEX_LIT.sub("<imm>", bt) == _HEX_LIT.sub("<imm>", tt):
                    kind = "immediate"
                else:
                    kind = "register"
                return hit(kind, block, bblock, tblock, brow, trow, prev)
            if brow["relocs"] != trow["relocs"]:
                note = None
                for _k, symbol, _a in trow["relocs"] + brow["relocs"]:
                    note = note or _synthetic(symbol)
                found = hit("reloc-target", block, bblock, tblock, brow, trow, prev, note)
                if not found["cosmetic"]:
                    return found
                cosmetic = cosmetic or found
            prev = (brow, trow)
        if bterm != tterm:
            return hit("flow", block, bblock, tblock,
                       row(braw, bbody[-1][0]) if bbody else None,
                       row(traw, tbody[-1][0]) if tbody else None, prev,
                       note=f"[{bterm}] vs [{tterm}]")
        return cosmetic

    def key(block):
        return "\n".join([text for _off, text in block[1]] + [block[2] or ""])

    matcher = difflib.SequenceMatcher(a=[key(b) for b in base_cfg],
                                      b=[key(b) for b in target_cfg], autojunk=False)
    cosmetic = None
    block = 0
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        pairs = []
        if tag == "equal":
            pairs = list(zip(range(i1, i2), range(j1, j2)))
        else:
            shared = min(i2 - i1, j2 - j1)
            pairs = [(i1 + k, j1 + k) for k in range(shared)]
            pairs += [(bi, None) for bi in range(i1 + shared, i2)]
            pairs += [(None, ti) for ti in range(j1 + shared, j2)]
        for bi, ti in pairs:
            if bi is None or ti is None:
                bblock = base_cfg[bi] if bi is not None else None
                tblock = target_cfg[ti] if ti is not None else None
                brow = row(braw, bblock[1][0][0]) if bblock and bblock[1] else None
                trow = row(traw, tblock[1][0][0]) if tblock and tblock[1] else None
                return hit("missing", block, bblock, tblock, brow, trow, None,
                           note="whole block")
            found = in_block(block, base_cfg[bi], target_cfg[ti])
            if found is not None:
                if not found["cosmetic"]:
                    return found
                cosmetic = cosmetic or found
            block += 1
    return cosmetic


def _render_divergence(div: dict) -> list[str]:
    def where(row, origin, side):
        if row is None:
            return f"  {side:<7} (no counterpart)"
        raw = " ".join(f"{b:02x}" for b in row["raw"])
        relocs = "".join(
            f"   [{k} {s}{('+%#x' % a) if a else ''}]" for k, s, a in row["relocs"])
        return f"  {side:<7} +{row['offset'] - origin:03x}: {raw:<18} {row['text']}{relocs}"

    kind = div["kind"].upper()
    why = {"MISSING": "an instruction only one side has",
           "FLOW": f"same instructions, different block terminator {div.get('note')}",
           "OPCODE": "a different mnemonic", "IMMEDIATE": "a different literal",
           "REGISTER": "different register operands",
           "ENCODING": "the same asm with different bytes",
           "RELOC-TARGET": "the same bytes with a different relocation target"
           + (f" ({div['note']})" if div.get("note") else "")}[kind]
    baddr = f"@0x{div['base_addr']:x}" if div["base_addr"] is not None else "@-"
    taddr = f"@0x{div['target_addr']:x}" if div["target_addr"] is not None else "@-"
    out = [f"[first byte-level divergence: B{div['block']} {baddr}/{taddr}, "
           f"kind {kind} - {why}]"]
    if div.get("context"):
        pb, pt = div["context"]
        out.append(where(pb, div["base_origin"], "base"))
        out.append(where(pt, div["target_origin"], "target"))
    out.append(where(div["base_row"], div["base_origin"], "base"))
    out.append(where(div["target_row"], div["target_origin"], "target"))
    out.append(_DIVERGENCE_KINDS)
    return out


def _masked_asm_delta(base_text: str, target_text: str) -> dict:
    """How many masked-asm rows differ, instruction rows and reloc rows apart."""
    import difflib
    base, target = _asm.norm(base_text), _asm.norm(target_text)
    ins = rel = 0
    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(
            a=base, b=target, autojunk=False).get_opcodes():
        if tag == "equal":
            continue
        for line in base[i1:i2] + target[j1:j2]:
            if line.startswith("reloc "):
                rel += 1
            else:
                ins += 1
    return {"equal": base == target, "instructions": ins, "relocs": rel}


def _summary_lines(facts: dict) -> tuple[list[str], bool, str]:
    """The digest: one line per view, the first divergence, the next view.
    Pure over the collected verdicts so tests can drive it."""
    census, br, calls, relocs = facts["census"], facts["branches"], facts["calls"], facts["relocs"]
    asm, div, source = facts["asm"], facts["divergence"], facts["source"]
    rva = facts["rva"]
    lines = [f"[summary: BASE (compiled) vs TARGET (retail) @ 0x{rva:08x} "
             f"{facts['name']} [{facts['unit']}]]"]
    pct = facts.get("pct")
    lines.append("  objdiff       " + (f"{pct:.2f}%  (report; function_reloc_diffs=none)"
                                       if pct is not None else "n/a (no report entry)"))
    nb, nt = census["blocks"]
    lines.append(f"  skeleton      {'same' if census['same'] else 'DIFFERS':<11} base {nb} vs "
                 f"target {nt} blocks; {census['exact']} exact, {census['size']} size-only, "
                 f"{census['shift']} target-shift, {census['flow']} flow-kind, "
                 f"{census['missing']} missing")
    br_ok = br["status"] in ("clean", "no-branches")
    br_word = br["status"] + (f" {br['kind']}" if br.get("kind") else "")
    lines.append(f"  branches      {br_word:<11} base {br['nbr']} branch(es), {br['rets'][0]} "
                 f"ret(s)  |  target {br['nbr_t']} branch(es), {br['rets'][1]} ret(s)")

    def refline(label, res):
        c = res["counts"]
        status = "AGREE" if res["agree"] else "DIFFERS"
        report = "AGREE" if res["report_agree"] else "DIFFERS"
        parts = [f"{c['=']} same"]
        if c["~"]:
            parts.append(f"{c['~']} different ({c['synthetic']} unclaimed, {c['real']} real)")
        if c["-"]:
            parts.append(f"{c['-']} base-only")
        if c["+"]:
            parts.append(f"{c['+']} target-only")
        return f"  {label:<13} {status:<11} {', '.join(parts)}   | report-level: {report}"
    lines.append(refline("calls", calls))
    lines.append(refline("relocs", relocs))
    lines.append("  asm (masked)  " + (f"{'equal':<11}" if asm["equal"] else
                 f"{'DIFFERS':<11} {asm['instructions']} instruction row(s); "
                 f"{asm['relocs']} reloc row(s)"))
    if div is None:
        lines.append("  first block   (none)")
    else:
        baddr = f"@0x{div['base_addr']:x}" if div["base_addr"] is not None else "@-"
        taddr = f"@0x{div['target_addr']:x}" if div["target_addr"] is not None else "@-"
        at = (f" at +{div['base_row']['offset'] - div['base_origin']:03x}"
              if div.get("base_row") else "")
        note = f" ({div['note']})" if div.get("note") and div["kind"] == "reloc-target" else ""
        lines.append(f"  first block   B{div['block']} {baddr}/{taddr}  {div['kind']}{at}{note}")
    lines.append(f"  first source  {source}")
    agree = (census["same"] and br_ok and calls["agree"] and relocs["agree"]
             and asm["equal"] and (div is None or div["cosmetic"]))
    if not calls["agree"]:
        nxt = "--calls"
    elif not br_ok:
        nxt = "--branches"
    elif not census["same"]:
        nxt = "--source" if facts.get("source_loaded") else "--structure --verbose"
    elif not relocs["agree"]:
        nxt = "--relocs"
    elif not asm["equal"] or (div is not None and not div["cosmetic"]):
        nxt = ("--why-bytes" if not facts.get("why_bytes") else
               "--source" if facts.get("source_loaded") else "--asm")
    else:
        nxt = None
    lines.append(f"  next: homm3 sema diff 0x{rva:08x} {nxt}" if nxt
                 else "  next: (nothing - all views agree)")
    return lines, agree, nxt


def _summary_view(ctx, base_text, target_text, rva, name, unit, ordinal,
                  verbose: bool, why_bytes: bool) -> int:
    try:
        pct = ctx.fn_fuzzy(unit, name)
    except Exception:
        pct = None
    census = _asm.skeleton_census(_asm.cfg(base_text), _asm.cfg(target_text))
    branches = _branches_compare(_branch_insns(base_text), _branch_insns(target_text))
    brefs, _bs = _ref_seq(base_text)
    trefs, _ts = _ref_seq(target_text)
    flow = ("call", "jmp", "ind")
    calls = _refs_compare([r for r in brefs if r[1] in flow],
                          [r for r in trefs if r[1] in flow])
    relocs = _refs_compare(brefs, trefs)
    asm = _masked_asm_delta(base_text, target_text)
    div = _first_divergence(base_text, target_text)
    source_loaded = False
    try:
        source_map = source_view.load(unit, name, ordinal, _asm.BASE / f"{unit}.obj")
    except source_view.NoLineRecords:
        source = "(unavailable: compiler-generated body - no /Z7 statements)"
    except source_view.SourceError as exc:
        source = f"(unavailable: {str(exc).splitlines()[0]})"
    else:
        source_loaded = True
        _text, _exact, first = _source_diff_full(base_text, target_text, source_map)
        source = (f"{source_map.source}:{first.line} | {first.text}   [/Z7 verified]"
                  if first else "(none - no divergent statement)")
    facts = {"rva": rva, "name": name, "unit": unit, "pct": pct, "census": census,
             "branches": branches, "calls": calls, "relocs": relocs, "asm": asm,
             "divergence": div, "source": source, "source_loaded": source_loaded,
             "why_bytes": why_bytes}
    lines, agree, _nxt = _summary_lines(facts)
    if verbose:
        lines = _summary_verbose(lines, census, branches, calls, relocs)
    for line in lines:
        print(line)
    if why_bytes and div is not None:
        for line in _render_divergence(div):
            print(line)
    if why_bytes and (not agree or (pct is not None and pct < 100)):
        print(f"[hint: homm3 vc6 diagnose {unit}:{name} routes this residual to "
              "its solver]")
    print("[all views agree]" if agree else "[views differ]")
    return 0 if agree else 1


def _summary_verbose(lines, census, branches, calls, relocs, cap: int = 8):
    """Under each DIFFERS line, that view's differing rows (capped)."""
    out = []
    for line in lines:
        out.append(line)
        if line.startswith("  skeleton      DIFFERS"):
            rows = [r for r in census["rows"] if r[2] != "==" or r[3] != "=="]
            for i, base, fm, sm, target in rows[:cap]:
                out.append(f"      B{i:<3} {base:48}  {fm:^4} {sm:^4} {target}")
            if len(rows) > cap:
                out.append(f"      ... (+{len(rows) - cap} more)")
        elif line.startswith("  branches") and branches.get("rows"):
            for r in branches["rows"][:cap]:
                out.append(f"      #{r[0]:<3} {r[1]} -> {r[2]}")
        elif line.startswith("  calls         DIFFERS") or line.startswith("  relocs        DIFFERS"):
            res = calls if line.startswith("  calls") else relocs
            rows = [r for r in res["rows"] if r[0] != "="]
            for tag, b, t, note in rows[:cap]:
                text = (f"{_ref_name(b)} -> {_ref_name(t)}" if tag == "~" else
                        _ref_name(b) if tag == "-" else f"-> {_ref_name(t)}")
                out.append(f"      {tag}  {text}{('  (' + note + ')') if note else ''}")
            if len(rows) > cap:
                out.append(f"      ... (+{len(rows) - cap} more)")
    return out


def run(args) -> None:
    ctx = get_context()
    name, unit, rva, _size, ordinal = ctx.symbols.resolve_fn(args.target)
    if not getattr(args, "no_build", False) and (_asm.TARGET / f"{unit}.c.obj").is_file():
        note = _asm.refresh_unit(unit)
        if note:
            print(note)
    normal_base = _asm.NORMAL_BASE / f"{unit}.obj"
    normal_target = _asm.NORMAL_TARGET / f"{unit}.c.obj"
    if not (normal_base.is_file() and normal_target.is_file()):
        die(f"{name} [{unit or 'no unit'}] has no comparison objects - only "
            "delinked manifest units (config/units.toml) can diff; "
            "`homm3 sema disasm` views any retail function")
    base_text = _asm.objdump(normal_base, name, ordinal)
    target_text = _asm.objdump(normal_target, name, ordinal)

    if args.range and (args.base_range or args.target_range):
        die("--range applies to both sides; do not combine it with "
            "--base-range/--target-range")
    base_spec = args.base_range or args.range
    target_spec = args.target_range or args.range
    if bool(args.base_range) != bool(args.target_range):
        die("side-specific slicing requires both --base-range and "
            "--target-range")
    for side, spec in (("base", base_spec), ("target", target_spec)):
        if not spec:
            continue
        try:
            span = _asm.parse_local_range(spec)
            if side == "base":
                base_text = _asm.slice_local_range(base_text, span)
            else:
                target_text = _asm.slice_local_range(target_text, span)
        except ValueError as exc:
            die(f"invalid --{side}-range {spec!r}: {exc}")
    if base_spec:
        print(f"[scoped diff: base {base_spec}; target {target_spec}; "
              "ranges are function-local and end exclusive]")

    if args.branches:
        sys.exit(_branch_view(base_text, target_text, rva, name, args.verbose))
    if args.calls or args.relocs:
        output, agree = _refs_view(base_text, target_text, rva, name,
                                   calls_only=bool(args.calls), verbose=args.verbose)
        print(output, end="")
        sys.exit(0 if agree else 1)
    if args.summary or args.why_bytes:
        sys.exit(_summary_view(ctx, base_text, target_text, rva, name, unit,
                               ordinal, args.verbose, bool(args.why_bytes)))

    if args.source:
        try:
            source_map = source_view.load(
                unit, name, ordinal, _asm.BASE / f"{unit}.obj")
        except source_view.NoLineRecords as exc:
            print(f"[{exc}]")
            print("[no statements to label - showing the block-structure "
                  "diff instead; --branches carries the in-block signal]")
        except source_view.SourceError as exc:
            die(str(exc))
        else:
            output, exact = _source_diff(base_text, target_text, source_map,
                                         args.verbose)
            print(output, end="")
            sys.exit(0 if exact else 1)

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
              f"{name}; addresses masked as <addr>"
              f"{'; full listing as context' if args.verbose else ''}]")
        context = max(len(base), len(target)) if args.verbose else 3
        for ln in difflib.unified_diff(base, target, "base", "target",
                                       n=context, lineterm=""):
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
