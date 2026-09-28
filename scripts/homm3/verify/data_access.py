"""homm3.verify.data_access - the RETAIL-side data-access audit (full tier).

The instrument for the question the match score structurally cannot answer.
We choose the extent of every datum we claim, so a too-small claim ALWAYS
scores 100: objdiff only compares what we told it to compare. This gate works
from RETAIL's side - which bytes does retail's code touch, and how wide - so a
silently unmodelled field, a wrong width, a too-small array COUNT or a phantom
object shows up as a disagreement between what retail does and what we
declared.

Engine + schema: homm3.verify.access_map (retail bytes and .reloc from
sema.image, decode from tool.objdump, the claim spine from the Model, the
declared field layout from verify.layout). Artifacts:
build/gen/data_access_map.sqlite (the query index) and
build/gen/data_access_map.tsv (the grep-able access table).

CATEGORIES
  unclaimed   retail touches bytes no claim covers -> unmodelled data
  unaccessed  a claim nothing in the image references -> phantom candidate
  width       access width/kind disagrees with the declared field -> wrong type
  stride      an index scale inside a claim disagrees with its element size
  undercount  a claim declared with ONE element is INDEXED -> under-declared
              COUNT or a false split of a larger object
  shortfall   an array whose own walker over-runs the declared end -> too-small
              forward storage; SHIFTS its section (never byte-neutral)
  adjacent    two claims reached through ONE base register -> one object
  import-slot a source claim on an address retail CALLS indirectly through the
              IAT -> the pin models an import thunk slot as a game global

WHAT IS GATED. Only categories whose suppression set is RE-PROVEN on this tree
(see `--calibrate`: the control set is claims whose data section objdiff scores
at exactly 100.0, whose declared type resolves, and that retail accesses) are
allowed to fail the build; the rest report. `GATED` and `REPORT_ONLY` below
carry the measured rate and the reason, one line each.

    python3 -m homm3.verify.data_access --build | --gate | --calibrate
    python3 -m homm3.verify.data_access --selftest      inject 9 known
                                                         defects, require each
    python3 -m homm3.verify.data_access --suppressed [class]   the sites a
                                                         suppression removed
    python3 -m homm3.verify.data_access --findings [category] [--limit N]
    python3 -m homm3.verify.data_access --at 0x24e360 | --symbol NAME
    python3 -m homm3.verify.data_access --fn NAME | --sql "SELECT ..."
    python3 -m homm3.verify.data_access --touched       the byte ranges retail
                                                         touches (coverage join)
"""

from __future__ import annotations

import bisect
from collections import Counter, defaultdict

from homm3.verify.access_map import (SQLITE, STRING_OPS, TSV, Claim, connect,
                                      load, persist)

# forms that TOUCH bytes (as opposed to taking an address or holding a pointer)
TOUCH = ("direct", "indexed", "derived-disp")
# forms that merely reference the object without reading/writing it here
REFER = ("lea", "imm", "indcall", "iat")

# Structural checks from Gruntz; HoMM3 findings remain explicit until reviewed.
GATED_CATEGORIES = ("width", "undercount", "shortfall", "stride",
                    "adjacent", "import-slot")

#: Categories that REPORT ONLY, with the reason. A category nobody can
#: calibrate ships DISABLED here, never silently permissive.
REPORT_ONLY = {
    "unclaimed": "evidence-thin by construction: an indexed operand names a "
                 "LOWER BOUND, not the byte touched, so a run is a triage "
                 "worklist, not a build breaker",
    "unaccessed": "phantom CANDIDATES only - a dead-but-correct datum "
                  "and a library global "
                  "the game never calls look identical from here",
}

#: (category, sym_rva) accepted despite firing - each a measured, documented
#: non-defect. Keep this empty unless retail evidence proves an exception.
ACCEPTED_MISMODELS: frozenset = frozenset()


# --------------------------------------------------------------------------- #
# spine helpers                                                               #
# --------------------------------------------------------------------------- #
class Spine:
    """The claim spine + the census spine, with the joins the sieve needs."""

    def __init__(self, img, model, layout, claims, rows):
        self.img, self.model, self.layout = img, model, layout
        self.claims = claims
        self.starts = [c.rva for c in claims]
        self.by_rva = {c.rva: c for c in claims}
        self.rows = rows
        self.row_lo = [r[0] for r in rows]
        iat_rva, iat_size = img.pe.iat
        self.idata = (iat_rva, iat_rva + iat_size)
        self.fn_channel = {b.rva: b.channel for b in model.functions}
        self.fn_unit = {b.rva: b.unit for b in model.functions}

    def locate(self, rva):
        """(claim, offset) for a claimed byte, else (None, -1)."""
        k = bisect.bisect_right(self.starts, rva) - 1
        if k < 0:
            return None, -1
        c = self.claims[k]
        return (c, rva - c.rva) if rva < c.end else (None, -1)

    def prev_claim(self, rva):
        k = bisect.bisect_right(self.starts, rva) - 1
        return self.claims[k] if k >= 0 else None

    def next_start(self, rva):
        k = bisect.bisect_right(self.starts, rva)
        return self.starts[k] if k < len(self.starts) else None

    def census(self, rva):
        """The admitted census row covering rva: (lo, hi, kind, channel, name)."""
        k = bisect.bisect_right(self.row_lo, rva) - 1
        if k < 0:
            return None
        r = self.rows[k]
        return r if rva < r[1] else None

    def in_idata(self, rva):
        return self.idata[0] <= rva < self.idata[1]

    def is_library_fn(self, rva):
        ch = self.fn_channel.get(rva)
        return ch in ("functions_static_libs", "data_static_libs") or \
            (ch == "" and not self.fn_unit.get(rva))


def _run_kind(spine, run):
    """Triage an unclaimed run so the worklist is not drowned by data that can
    never take a source claim. Only `data` is the real worklist.

    The census is the authority here: the old tool sniffed the payload for
    printable bytes and x87-only reads, but this tree ADMITS every datum's
    extent and kind, so a run's kind is a fact, not a guess."""
    acc = run["acc"]
    if spine.in_idata(run["start"]):
        return "idata"                        # import thunk slots: the linker's
    kinds = set()
    for rva in range(run["start"], run["end"], 4):
        row = spine.census(rva)
        kinds.add(row[2] if row else "outside")
    kinds.discard("pad")
    if kinds and kinds <= {"string"}:
        return "string-pool"                  # pooled literals (inline strcmp)
    if kinds and kinds <= {"fppool"}:
        return "fp-pool"                      # an unpinnable x87 constant pool
    if kinds and kinds <= {"rtti"}:
        return "rtti"
    if kinds and kinds <= {"ehtable"}:
        return "ehtable"
    fns = [a.owner for a in acc if a.owner is not None]
    if fns and all(spine.is_library_fn(f) for f in fns):
        return "library"
    return "data"                             # incl. "no owner at all": unknown
                                              # code is not evidence of a library


def _widths(accs):
    return Counter(a.width for a in accs if a.width)


#: how far after an indexed access's relocated base a claim may start and still
#: explain it. cl folds `&array[i - k]` into `[reg*s + (base - k*s)]`, so the
#: shift is a small multiple of the element size - 4 B (`g_mapNameBuf[i-4]`) and
#: 1-2 B (zlib's 1-based `length_code[n-1]`, `g_clut[i-1]`) are the measured
#: spellings in this image.
NEG_ADDEND_WINDOW = 16


def _explained_by_next_claim(spine, ac, window=NEG_ADDEND_WINDOW):
    """Is an unclaimed-looking access just a claim's negative-addend spelling?

    For an INDEXED access the relocated operand is `base - k*stride`, not the
    byte touched: the address is `base + reg*s` for a register we cannot read,
    so the map's `target` is a LOWER BOUND. When a claim starts a few bytes
    later, that claim explains the access completely and calling the operand
    address `unmodelled data` invents a datum. A `direct` or `derived-disp`
    access names an exact byte and is never suppressed."""
    if ac.form != "indexed" or not (ac.base_reg or ac.index_reg):
        return False
    nxt = spine.next_start(ac.target_rva)
    return nxt is not None and 0 < nxt - ac.target_rva <= window


def _whole_record_copy_reads(spine, claim, accesses):
    """Prove unrolled dword copies of a complete record into one destination.

    Width alone is insufficient: require every source chunk, decoded stores of
    those unchanged register values at corresponding destination offsets, and
    no intervening control flow or destination-base modification. This covers
    VC5 by-value GUID arguments without exempting GUID names or wide scalars.
    """
    from homm3.verify.access_map import _decode, parse_mem, split_operands
    node = claim.node or {}
    size = node.get('sz', 0)
    if (node.get('k') != 'rec' or node.get('u') or node.get('poly')
            or size < 8 or size > 128 or size % 4 or size != claim.extent):
        return set()
    seq = sorted(accesses, key=lambda a: a.insn_rva)
    result = set()
    count = size // 4
    for i, first in enumerate(seq):
        if first.target_rva != claim.rva:
            continue
        group = seq[i:i + count]
        if len(group) != count or any(
                a.target_rva != claim.rva + n * 4 or a.width != 4
                or a.rw != 'r' or a.form != 'direct' or a.mnemonic != 'mov'
                or a.owner != first.owner for n, a in enumerate(group)):
            continue
        end = group[-1].insn_rva + group[-1].insn_len + 8
        if end - first.insn_rva > count * 32:
            continue
        blob = spine.img.read(first.insn_rva, end - first.insn_rva)
        if not isinstance(blob, bytes):
            continue
        decoded = _decode(blob, first.insn_rva)
        loads = {a.insn_rva: a.target_rva - claim.rva for a in group}
        values, stored = {}, set()
        dest_base, dest_bias = None, None
        valid = True
        for address, text in zip(decoded.starts, decoded.lines):
            mnemonic, operands = split_operands(text)
            if address in loads:
                if mnemonic != 'mov' or len(operands) != 2 or parse_mem(operands[0]):
                    valid = False
                    break
                if dest_base == operands[0]:
                    valid = False
                    break
                values[operands[0]] = loads[address]
                continue
            if mnemonic == 'mov' and len(operands) == 2:
                dst, src = operands
                mem = parse_mem(dst)
                if mem and src in values:
                    width, base, index, scale, disp = mem
                    offset = values[src]
                    if width != 4 or not base or index or base == 'esp':
                        valid = False
                        break
                    if dest_base is None:
                        dest_base, dest_bias = base, disp - offset
                    if base != dest_base or disp - offset != dest_bias:
                        valid = False
                        break
                    stored.add(offset)
                    if len(stored) == count:
                        break
                    continue
                if not mem:
                    if dst == dest_base or dst not in (
                            'eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp', 'esp'):
                        valid = False
                        break
                    values.pop(dst, None)
                else:
                    # An unrelated store may overwrite an already copied chunk.
                    valid = False
                    break
                continue
            # Stack argument setup before the first store does not modify the
            # record's values or its register-held destination address.
            if (not stored and mnemonic in ('sub', 'push', 'lea')
                    and not any(op in values for op in operands)
                    and (not operands or operands[0] != dest_base)):
                continue
            valid = False
            break
        if valid and stored == set(range(0, size, 4)):
            result.update(loads)
    return result


# --------------------------------------------------------------------------- #
# the derived analyses                                                        #
# --------------------------------------------------------------------------- #
def derive_findings(spine, accesses, cells, owners, trace=None):
    """(rows, stats) - every category, each row a sqlite `finding`.

    Every suppression below is a MEASURED false-positive class, named so the
    next reader can re-argue it; nothing is filtered because it was noisy.
    Pass `trace` (a list) to collect the suppressed SITES - `--suppressed` -
    which is how the next reader re-proves or falsifies each class against the
    retail disassembly instead of trusting this docstring."""
    layout = spine.layout
    rows: list[tuple] = []
    st: Counter = Counter()

    def skip(cls, sym="", off=0, accs=()):
        st[cls] += 1
        if trace is None:
            return
        if accs:
            trace.extend((cls, a.insn_rva, sym, off, a.text or "")
                         for a in accs)
        else:
            trace.append((cls, 0, sym, off, ""))

    for ac in accesses:                        # attribute once
        ac.owner = owners.at(ac.insn_rva)

    per = defaultdict(list)
    unclaimed = defaultdict(list)
    for ac in accesses:
        c, _off = spine.locate(ac.target_rva)
        if c is None:
            if ac.form not in TOUCH:
                continue
            if _explained_by_next_claim(spine, ac):
                skip("unclaimed-skip-negative-addend", accs=[ac])
                continue
            unclaimed[ac.target_rva].append(ac)
        else:
            per[c.rva].append(ac)

    # ---- 1. accessed but unclaimed -> unmodelled data ----------------------
    runs, cur = [], None
    for rva in sorted(unclaimed):
        w = max((a.width or 4) for a in unclaimed[rva])
        if cur and rva - cur["end"] <= 8:
            cur["end"] = max(cur["end"], rva + w)
            cur["acc"] += unclaimed[rva]
        else:
            cur = {"start": rva, "end": rva + w, "acc": list(unclaimed[rva])}
            runs.append(cur)
    for r in runs:
        kind = _run_kind(spine, r)
        st[f"unclaimed-{kind}"] += 1
        prev = spine.prev_claim(r["start"])
        widths = _widths(r["acc"])
        wrote = any("w" in a.rw for a in r["acc"])

        # SHORTFALL runs for EVERY section, before the data-only gate below: a
        # too-small array in `.rdata`/`.bss` shifts its own section just as one
        # in `.data` does. The run continues an ARRAY claim - same element
        # width, starting exactly at its end - which is a too-small array COUNT
        # with its own forward storage (the `float[2]`-for-`float[10]` class, in
        # the direction the `undercount` index test cannot reach). This shifts
        # every symbol after it: cl emits a smaller symbol and the linked image
        # / emitted COFF both diverge. A
        # separate neighbour global is excluded by the element-width +
        # array-type + exact-adjacency + accessor-overlap quad.
        if prev is not None and r["start"] == prev.end and prev.node \
                and prev.node.get("k") == "arr":
            _count, el = layout.element(prev.node)
            pelem = (el or {}).get("sz") or 0
            run_w = max(widths) if widths else None
            # The tail must be touched by a function that ALSO accesses the
            # array body - the same loop running one element too far. Without
            # this a separate same-width global right after the array (a byte
            # flag, another unit's global) reads as a phantom shortfall.
            prev_fns = {a.owner for a in per.get(prev.rva, ())}
            tail_fns = {a.owner for a in r["acc"]}
            if (pelem and run_w == pelem
                    and spine.img.section_name(r["start"]) == prev.section
                    and ((prev_fns & tail_fns) - {None})):
                extra = (r["end"] - r["start"] + pelem - 1) // pelem
                rows.append(("shortfall", "high" if wrote else "med",
                             prev.rva, prev.name, r["start"],
                             f"declared {layout.spelling(prev.node)} but retail "
                             f"accesses ~{extra} more element(s) of the SAME "
                             f"{pelem} B width immediately past its end - the "
                             f"COUNT is too small and this SHIFTS "
                             f"{prev.section} (not byte-neutral)",
                             f"tail={r['end'] - r['start']} B same-width sites, "
                             f"declared_end=0x{prev.end:x}"))
                st["shortfall"] += 1

        if kind != "data":
            continue
        row = spine.census(r["start"])
        where = (f"census row 0x{row[0]:06x}+0x{row[1] - row[0]:x} kind="
                 f"{row[2] or 'plain'}, unclaimed" if row else
                 f"outside every admitted census row, in "
                 f"{spine.img.section_name(r['start'])}")
        rows.append(("unclaimed", "high" if wrote else "med",
                     prev.rva if prev else 0, prev.name if prev else "",
                     r["start"],
                     f"{r['end'] - r['start']} B accessed by {len(r['acc'])} "
                     f"site(s){' incl. a WRITE' if wrote else ''}, no claim "
                     f"covers it",
                     " ".join(f"w{w}x{n}" for w, n in sorted(widths.items()))
                     + f"  {where}"
                     + (f", {r['start'] - prev.end} B past {prev.name}"
                        if prev and 0 <= r["start"] - prev.end < 0x100 else "")))
        st["unclaimed"] += 1

    # ---- 2. claimed but never referenced -> phantom candidates -------------
    pointed = set()
    for cl in cells:
        c, _o = spine.locate(cl["target"])
        if c is not None:
            pointed.add(c.rva)
        s, _so = spine.locate(cl["site"])      # a claim whose OWN bytes relocate
        if s is not None:
            pointed.add(s.rva)
    for ac in accesses:                        # an undecoded reloc still NAMES
        if ac.form == "undecoded":
            c, _o = spine.locate(ac.target_rva)
            if c is not None:
                pointed.add(c.rva)
    for c in spine.claims:
        if per[c.rva] or c.rva in pointed:
            continue
        rows.append(("unaccessed", "high" if c.section != ".idata" else "low",
                     c.rva, c.name, c.rva,
                     f"nothing in the image reads, writes, takes the address "
                     f"of or points at it (extent 0x{c.extent:x})",
                     f"unit={c.unit} channel={c.channel} section={c.section} "
                     f"type={layout.spelling(c.node) if c.node else '?'}"))
        st["unaccessed"] += 1

    # ---- 3. access width vs the declared field -> wrong type ---------------
    # a `mov [obj+N],&??_7...` is a vptr STAMP, so +N is a base sub-object
    # boundary, not an unmodelled member.
    vtbl = {c.rva for c in spine.claims if c.name.startswith("??_7")}
    stamps = {ac.insn_rva for ac in accesses
              if ac.form in ("imm", "lea") and ac.target_rva in vtbl}

    def is_vptr_stamp(acs):
        return bool(acs) and all(a.width == 4 and "w" in a.rw
                                 and a.insn_rva in stamps for a in acs)

    for c in spine.claims:
        if c.node is None or not per[c.rva]:
            continue
        copied = _whole_record_copy_reads(spine, c, per[c.rva])
        seen = defaultdict(lambda: [Counter(), Counter(), Counter(), Counter(),
                                    Counter()])
        acc_at = defaultdict(list)
        for ac in per[c.rva]:
            if ac.form not in TOUCH or not ac.width:
                continue
            if ac.mnemonic.split()[-1].rstrip("bwd") in STRING_OPS:
                # A string op's operand width is the BLOCK GRANULARITY of an
                # inlined memset/memcpy/memcmp, not a field width: cl zeroes a
                # struct with `rep stosd` + `stosw`, which is not a 4-byte
                # access to a WORD member at +0x0.
                skip("width-skip-string-op", c.name,
                     ac.target_rva - c.rva, [ac])
                continue
            off = ac.target_rva - c.rva
            if ac.insn_rva in copied:
                skip("width-skip-record-copy", c.name, off, [ac])
                continue
            acc_at[off].append(ac)
            seen[off][0][ac.width] += 1
            if ac.fpu:
                seen[off][1][ac.fpu] += 1
            seen[off][2][(ac.width, ac.rw)] += 1
            seen[off][3][ac.form] += 1
            seen[off][4][(ac.width, ac.ext)] += 1
        for off, (widths, fpus, rws, forms, exts) in sorted(seen.items()):
            # "does retail STORE fewer bytes than the field" must be asked of
            # the NARROW access itself: a 4-byte store plus a 2-byte read is
            # `(u16)x`, not evidence of a u16 field
            def stores(w, _rws=rws):
                return any("w" in rw for (ww, rw) in _rws if ww == w)
            ev = " ".join(f"w{w}x{n}" for w, n in sorted(widths.items()))
            nxt = spine.next_start(c.rva)
            if set(forms) == {"indexed"} and nxt is not None \
                    and 0 < nxt - (c.rva + off) <= 4:
                # `[reg + &next - k]` is the negative-addend spelling of the
                # FOLLOWING symbol (a 1-based index into the next array), not an
                # access to this claim - assert_relocs knows the same idiom
                skip("width-skip-negative-addend", c.name, off, acc_at[off])
                continue
            f = layout.field_at(c.node, off)
            if f.tag == "vptr" or is_vptr_stamp(acc_at[off]):
                skip("width-skip-vptr", c.name, off, acc_at[off])
                continue
            if f.tag in ("out", "hole"):
                rows.append(("width", "high", c.rva, c.name, c.rva + off,
                             f"+0x{off:x} accessed but no declared field of "
                             f"{layout.spelling(c.node)} covers that offset "
                             f"({'past the type' if f.tag == 'out' else 'a HOLE between members'})",
                             ev))
                st["width"] += 1
                continue
            if not f.resolved:
                skip("width-skip-unresolved", c.name, off, acc_at[off])
                continue                       # never accuse through an unknown
            # every offset past this line was really adjudicated - the number
            # a reader needs to tell "clean" from "structurally blind"
            st["width-checked"] += 1
            foff, fsz, path, fty = f.off, f.size, f.path, f.type
            if off != foff:
                # FP class: an 8-byte scalar copied as two dwords - MSVC5 moves
                # a double/i64 constant with a pair of dword loads, so a 4-byte
                # access at +4 of an 8-byte field is the copy, not a layout bug
                if fsz == 8 and off == foff + 4 and set(widths) == {4} \
                        and not fpus:
                    skip("width-skip-dword-pair", c.name, off, acc_at[off])
                    continue
                rows.append(("width", "high", c.rva, c.name, c.rva + off,
                             f"+0x{off:x} lands INSIDE field {path or '.'} "
                             f"(+0x{foff:x} {fty}, {fsz} B) - layout is wrong",
                             ev))
                st["width"] += 1
                continue
            wmax, wmin = max(widths), min(widths)
            elem = "[" in path                 # an ARRAY element, not a scalar
            pair = fsz == 8 and set(widths) == {4} and not fpus \
                and (off + 4) in seen and set(seen[off + 4][0]) == {4}
            if wmax > fsz:
                # FP class: a wider access on a byte-ARRAY element is the
                # inlined CRT block move/compare (rep movsd over a char buffer).
                # A byte SCALAR read 4 bytes wide is a type error, not a block op.
                if elem and fsz == 1:
                    skip("width-skip-byte-buffer", c.name, off, acc_at[off])
                elif fsz == 4 and wmax == 8 and not fpus:
                    skip("width-skip-dword-pair", c.name, off, acc_at[off])
                elif exts[(wmax, f"m{fsz}")] == widths[wmax]:
                    # cl 5.0 loads a narrow global with a FULL-WIDTH read and
                    # masks the register (movzx was slow on the Pentium), so
                    # every over-wide access here is a `fsz`-byte one in disguise
                    skip("width-skip-and-mask", c.name, off, acc_at[off])
                elif not stores(wmax) and stores(fsz):
                    # the same movzx-avoidance, caught by its STORE side: the
                    # mask can sit past a branch or behind a register copy, but
                    # nobody writes a wmax-byte object only fsz bytes at a time
                    skip("width-skip-and-mask", c.name, off, acc_at[off])
                else:
                    rows.append(("width", "high", c.rva, c.name, c.rva + off,
                                 f"+0x{off:x} {path or '.'} declared {fty} "
                                 f"({fsz} B), retail accesses {wmax} B", ev))
                    st["width"] += 1
            elif pair:
                # MSVC5 copies an 8-byte constant with a pair of dword loads
                skip("width-skip-dword-pair", c.name, off, acc_at[off])
            elif wmin < fsz and f.is_float and not fpus:
                rows.append(("width", "high", c.rva, c.name, c.rva + off,
                             f"+0x{off:x} {path or '.'} declared {fty} ({fsz} B) "
                             f"but retail accesses {wmin} B with integer ops and "
                             f"never touches it with x87", ev))
                st["width"] += 1
            elif wmin < fsz and not f.is_ptr and not elem and stores(wmin):
                # a sub-field STORE is strong: nobody writes half a scalar
                rows.append(("width", "med", c.rva, c.name, c.rva + off,
                             f"+0x{off:x} {path or '.'} declared {fty} ({fsz} B) "
                             f"but retail STORES {wmin} B", ev))
                st["width"] += 1
            if any(t.startswith("f") for t in fpus) and not f.is_float:
                rows.append(("width", "high", c.rva, c.name, c.rva + off,
                             f"+0x{off:x} {path or '.'} declared {fty} but "
                             f"retail uses an x87 FLOAT access "
                             f"({'/'.join(sorted(fpus))})", ev))
                st["width"] += 1
            if any(t.startswith("i") for t in fpus) and f.is_float:
                rows.append(("width", "high", c.rva, c.name, c.rva + off,
                             f"+0x{off:x} {path or '.'} declared {fty} but "
                             f"retail uses an x87 INTEGER access "
                             f"({'/'.join(sorted(fpus))})", ev))
                st["width"] += 1

    # ---- 3b. a source claim on an IMPORT slot ------------------------------
    # `call DWORD PTR ds:0x6c43c0` where 0x2c43c0 is inside the IAT data
    # directory is an IMPORT thunk pointer the linker owns, not a game global.
    # A src-channel DATA() pin there models the import table as an `i32`, and
    # the payload (a pointer into the import name table) proves it. An indirect
    # call through a VTABLE or a function-pointer TABLE member is a real datum
    # and never fires: only the IAT range does.
    for c in spine.claims:
        if c.channel != "src":
            continue
        iat = [a for a in per[c.rva] if a.form == "iat"]
        if not iat:
            continue
        target = spine.img.u32(c.rva)
        rows.append(("import-slot", "high", c.rva, c.name, c.rva,
                     f"retail CALLS through this address ({len(iat)} indirect "
                     f"call site(s)) - it is an import thunk slot in the IAT, "
                     f"not a datum; the source models linker storage as "
                     f"{layout.spelling(c.node) if c.node else 'a global'}",
                     f"stored 0x{target or 0:06x} (import name table), "
                     f"unit={c.unit}"))
        st["import-slot"] += 1

    # ---- 4. stride evidence inside a claim -> wrong element size -----------
    for c in spine.claims:
        scales = Counter()
        for ac in per[c.rva]:
            if ac.form == "indexed" and ac.scale and ac.target_rva == c.rva:
                scales[ac.scale] += 1
        if not scales or c.node is None:
            continue
        count, el = layout.element(c.node)
        elem = (el or {}).get("sz") or None
        is_array = c.node.get("k") == "arr"
        spell = layout.spelling(c.node)
        for sc, n in sorted(scales.items()):
            # An index into a claim declared with ONE element is under-COUNTED
            # or falsely split from a larger object, independently of whether
            # the element WIDTH matches. The variable index proves a length >=
            # 2 that the claimed extent does not carry. Width/stride checks miss
            # this precisely because sc == elem.
            if is_array and count <= 1:
                rows.append(("undercount", "high", c.rva, c.name, c.rva,
                             f"indexed by *{sc} but declared with ONE element "
                             f"({spell}) - an index proves length >= 2, so the "
                             f"COUNT is under-declared (byte-invisible in .bss)",
                             f"type={spell} elem={elem} sites={n}"))
                st["undercount"] += 1
                continue
            if elem is None:
                rows.append(("stride", "med", c.rva, c.name, c.rva,
                             f"indexed by *{sc} but the claim's element type "
                             f"does not resolve", f"type={spell} sites={n}"))
                st["stride"] += 1
            elif not is_array and sc != elem:
                rows.append(("stride", "high", c.rva, c.name, c.rva,
                             f"indexed by *{sc} but declared as the SCALAR "
                             f"{spell} ({elem} B) - retail treats it as a table",
                             f"sites={n}"))
                st["stride"] += 1
            elif is_array and sc > elem:
                rows.append(("stride", "high", c.rva, c.name, c.rva,
                             f"indexed by *{sc} but the declared element is "
                             f"only {elem} B - the element is {sc // elem}x too "
                             f"small (a pair/record, not a scalar array)",
                             f"type={spell} sites={n}"))
                st["stride"] += 1
            elif is_array and sc < elem and elem % sc == 0:
                # `[i*4 + base + k]` inside a 12-byte record: MSVC scales the
                # index by the DWORD, not by the element. Benign, counted only.
                skip("stride-skip-subelement", c.name, 0)
            elif is_array and sc != elem:
                rows.append(("stride", "high", c.rva, c.name, c.rva,
                             f"indexed by *{sc}, the declared element is "
                             f"{elem} B - neither divides the other",
                             f"type={spell} sites={n}"))
                st["stride"] += 1

    # ---- 5. two claims that are really ONE object --------------------------
    # 5a a single access whose byte RANGE crosses a claim boundary: retail
    #    reads both claims with one instruction, so they are one datum
    spans = Counter()
    for ac in accesses:
        if ac.form not in TOUCH or not ac.width:
            continue
        a, _ao = spine.locate(ac.target_rva)
        if a is None or ac.target_rva + ac.width <= a.end:
            continue
        b, _bo = spine.locate(a.end)
        if b is None or b.rva == a.rva:
            continue
        spans[(a.rva, b.rva, ac.width)] += 1
    for (arva, brva, w), n in sorted(spans.items(), key=lambda kv: -kv[1]):
        a, b = spine.by_rva[arva], spine.by_rva[brva]
        rows.append(("adjacent", "high", arva, a.name, brva,
                     f"one {w}-byte access at {a.name} runs past its 0x"
                     f"{a.extent:x}-byte extent into {b.name} - one object",
                     f"other=0x{brva:x} {b.name} sites={n}"))
        st["adjacent"] += 1
    # 5b a derived `[reg+disp]` whose base register held claim A but whose
    #    target lands in claim B: retail addresses both from one base
    joins = Counter()
    for ac in accesses:
        if ac.form != "derived-disp" or not ac.disp:
            continue
        a, _ao = spine.locate(ac.target_rva - ac.disp)
        b, _bo = spine.locate(ac.target_rva)
        if a is None or b is None or a.rva == b.rva:
            continue
        joins[(a.rva, b.rva)] += 1
    for (arva, brva), n in sorted(joins.items(), key=lambda kv: -kv[1]):
        a, b = spine.by_rva[arva], spine.by_rva[brva]
        rows.append(("adjacent", "high" if b.rva == a.end else "med", arva,
                     a.name, brva,
                     f"reached as {a.name}+0x{brva - arva:x} through one base "
                     f"register - {'contiguous' if b.rva == a.end else 'gapped'}",
                     f"other=0x{brva:x} {b.name} sites={n}"))
        st["adjacent"] += 1
    return rows, st


# --------------------------------------------------------------------------- #
# the sweep front-end                                                         #
# --------------------------------------------------------------------------- #
def analyse(trace=None):
    """(spine, accesses, cells, stats, findings, fstats, owners) - one sweep,
    always recomputed from retail + the current claims, so no verdict can ever
    ride a stale map."""
    from homm3.verify.access_map import Owners
    img, model, layout, accesses, cells, claims, rows, stats = load()
    spine = Spine(img, model, layout, claims, rows)
    owners = Owners(model)
    findings, fstats = derive_findings(spine, accesses, cells, owners, trace)
    return spine, accesses, cells, stats, findings, fstats, owners


_ANALYSIS = None


def analysis():
    """The process-wide sweep. Two full-tier gates (this one and
    verify.data_coverage) read the same map; sweeping twice would double the
    tier's wall time for nothing."""
    global _ANALYSIS
    if _ANALYSIS is None:
        _ANALYSIS = analyse()
    return _ANALYSIS


def do_suppressed(args):
    """Print the SITES a suppression class removed, so the class can be
    re-argued (or falsified) against the retail disassembly."""
    trace: list = []
    _sp, _acc, _cells, _stats, _f, fstats, _own = analyse(trace)
    rows = [t for t in trace if not args.suppressed or args.suppressed in t[0]]
    print(", ".join(f"{k}={v}" for k, v in sorted(fstats.items())
                    if "skip" in k) or "no suppressions")
    for cls, insn, sym, off, text in rows[:args.limit]:
        print(f"  {cls:34} 0x{insn:06x} {sym[:40]:40} +0x{off:<5x} {text}")
    if len(rows) > args.limit:
        print(f"  ... {len(rows) - args.limit} more (raise --limit)")
    return 0


def gate_findings() -> list[str]:
    """The tier gate: a NEW data-mismodel finding in a gated category."""
    _spine, _acc, _cells, _stats, findings, _fs, _own = analysis()
    gated = [r for r in findings if r[0] in GATED_CATEGORIES]
    new = [r for r in gated if (r[0], r[2]) not in ACCEPTED_MISMODELS]
    out = [f"data-access: [{cat}] 0x{rva:06x} {name} - {detail}"
           for cat, _sev, rva, name, _addr, detail, _ev in new]
    stale = ACCEPTED_MISMODELS - {(r[0], r[2]) for r in gated}
    for cat, rva in sorted(stale):
        out.append(f"data-access: STALE accept {cat} 0x{rva:06x} no longer "
                   f"fires - remove it from ACCEPTED_MISMODELS")
    return out


def do_build(args):
    spine, accesses, cells, stats, findings, fstats, _own = analyse()
    na, nc = persist(spine.img, spine.layout, accesses, cells, spine.claims,
                     stats, args.sqlite, args.tsv, findings, spine.model)
    touch = sum(1 for a in accesses if a.form in TOUCH)
    refer = sum(1 for a in accesses if a.form in REFER)
    print(f"[access-map] {na} references, {nc} pointer cells, "
          f"{len(spine.claims)} claims -> {args.sqlite}")
    print(f"[access-map] byte-touching {touch}, address-taking {refer}")
    for pfx, label in (("form-", "forms"), ("to-", "out of scope"),
                       ("cell-", "cells")):
        print(f"[access-map] {label}: " + ", ".join(
            f"{k[len(pfx):]}={v}" for k, v in sorted(stats.items())
            if k.startswith(pfx)))
    print("[access-map] findings: " + (", ".join(
        f"{k}={v}" for k, v in sorted(fstats.items())) or "none"))
    print_coverage(accesses, stats)
    return 0


def print_coverage(accesses, stats):
    """State what the map sees and what it structurally cannot.

    The .reloc table is a COMPLETE index of absolute data references, so the
    absolute half of the map is exhaustive by construction. Register-relative
    accesses carry no relocation: only the ones whose base was loaded from an
    absolute operand in the same basic block are recoverable, and the rest are
    invisible. The escape counts below bound that blind region."""
    esc = Counter()
    for a in accesses:
        if a.form not in ("imm", "lea"):
            continue
        m = (a.text or "").split(None, 1)[0] if a.text else "?"
        if m == "push":
            esc["push (call argument)"] += 1
        elif m == "mov" and a.text and "PTR" in a.text.split(",")[0]:
            esc["stored into memory"] += 1
        elif m in ("mov", "lea"):
            esc["loaded into a register"] += 1
        else:
            esc[m] += 1
    touch = sum(1 for a in accesses if a.form in TOUCH)
    derived = sum(1 for a in accesses if a.form == "derived-disp")
    print("[coverage] SEEN - reloc-anchored byte accesses: "
          f"{touch - derived} (exhaustive: every absolute operand is relocated)")
    print(f"[coverage] SEEN - register-relative recovered by provenance: "
          f"{derived}")
    print("[coverage] BLIND - address escapes we cannot follow: " + ", ".join(
        f"{k}={v}" for k, v in esc.most_common()))
    print(f"[coverage] BLIND - register loads handed straight to a callee: "
          f"{stats.get('seed-handed-to-callee', 0)} of "
          f"{stats.get('seed-total', 0)} seeds (the callee's field accesses are "
          f"`this`-relative)")
    print("[coverage] BLIND - structurally invisible classes: `this`-relative "
          "field accesses inside a callee; any access through a pointer loaded "
          "FROM memory; a member SWAP between two same-sized members (no width "
          "difference exists to observe)")
    print(f"[coverage] undecodable .text reloc sites: "
          f"{stats.get('form-undecoded', 0)} (each still counted as a "
          f"reference and as a pointer-cell candidate)")


def do_gate(_args):
    bad = gate_findings()
    if bad:
        print(f"[data-access] GATE FAILED - {len(bad)} new data mismodel(s):")
        for b in bad:
            print(f"  {b}")
        print("  A wrong COUNT/width/aggregation changes bytes (or .bss/.data "
              "placement). Fix the declaration; accept an exception only when "
              "retail evidence proves the finding is not a source mismodel.")
        return 1
    print(f"[data-access] gate OK - 0 new data mismodels "
          f"({len(ACCEPTED_MISMODELS)} documented exception(s), gated: "
          f"{', '.join(GATED_CATEGORIES)}; report-only: "
          f"{', '.join(sorted(REPORT_ONLY))})")
    return 0


# --------------------------------------------------------------------------- #
# calibration                                                                 #
# --------------------------------------------------------------------------- #
def do_calibrate(args):
    """Measure the sieve's flag rate against a control set, WITH the
    denominator.

    CONTROL SET: claims that live in a data section objdiff scores at exactly
    100.0, whose declared type fully resolves, and that retail actually
    accesses. Those bytes are byte-identical to retail, so nothing about their
    CONTENT can be wrong.

    What that does and does NOT prove, stated precisely, because getting this
    wrong is how a sieve gets believed:
      * It does NOT make a `width` finding a false positive. A section at 100.0
        means the BYTES match; the declared TYPE can still be wrong, and that is
        the entire premise of this map. So every control-set finding has to be
        ADJUDICATED against the retail disassembly by hand - the number below is
        a FLAG RATE, and the report states the adjudicated split.
      * It DOES bound the noise: a sieve that flags a large fraction of
        byte-exact, fully-typed claims is measuring its own bugs.

    `unclaimed` is reported separately and split by whether the run begins
    exactly at a control claim's END (an extent claim about that claim) or
    somewhere else, because attributing an unclaimed run to the nearest
    preceding symbol would inflate the rate."""
    spine, accesses, _cells, _stats, findings, _fs, _own = analyse()
    claims = spine.claims
    exact = {c.rva for c in claims if c.pct is not None and c.pct >= 100.0}
    resolved = {c.rva for c in claims
                if c.node is not None and spine.layout.sizeof(c.node)}
    starts = spine.starts
    ntouch = Counter()
    for a in accesses:
        if a.form not in TOUCH:
            continue
        k = bisect.bisect_right(starts, a.target_rva) - 1
        if k >= 0 and a.target_rva < claims[k].end:
            ntouch[claims[k].rva] += 1
    control = {r for r in (exact & resolved) if ntouch[r]}
    ends = {c.end: c for c in claims}

    print(f"[calibrate] claims                                {len(claims)}")
    print(f"[calibrate]   data section at exactly 100.0         {len(exact)}")
    print(f"[calibrate]   declared type resolves                {len(resolved)}")
    print(f"[calibrate]   retail accesses it                    {len(ntouch)}")
    print(f"[calibrate]   CONTROL SET (all three)               {len(control)}")

    direct = defaultdict(list)
    contig, elsewhere = [], []
    for f in findings:
        cat, _sev, srva, _sname, addr, _detail, _ev = f
        if cat == "unclaimed":
            owner = ends.get(addr)
            (contig if owner is not None and owner.rva in control
             else elsewhere).append(f)
        elif srva in control:
            direct[cat].append(f)
    n = sum(len(v) for v in direct.values())
    print(f"[calibrate] TYPE findings on the control set: {n} over "
          f"{len(control)} claims = {100.0 * n / max(len(control), 1):.2f}%")
    for cat, v in sorted(direct.items()):
        print(f"[calibrate]   {cat:11} {len(v):4} on "
              f"{len({f[2] for f in v})} claim(s)"
              + ("   [GATED]" if cat in GATED_CATEGORIES else "   [report]"))
    print(f"[calibrate] unclaimed runs starting exactly at a control claim's "
          f"end: {len(contig)}  (elsewhere, not attributable: {len(elsewhere)})")
    for cat, v in sorted(direct.items()):
        for f in v[:args.limit]:
            print(f"    [{cat}/{f[1]}] 0x{f[4]:08x} {f[3][:50]:50} {f[5]}")
            if f[6]:
                print(f"        {f[6]}")
    for f in contig[:args.limit]:
        print(f"    [unclaimed-contiguous] 0x{f[4]:08x} {f[3][:50]:50} {f[5]}")
    return 0


# --------------------------------------------------------------------------- #
# the injected-defect self test (a sieve returning 0 rows while BLIND is the   #
# failure mode this control exists to catch)                                  #
# --------------------------------------------------------------------------- #
def injection_plans(spine, accesses):
    """[(tag, expected category, victim claim, mutate)] - each injection is a
    defect class this campaign has actually shipped, applied to the in-memory
    claim set only; src/ is never touched."""
    layout = spine.layout
    claims, starts = spine.claims, spine.starts
    per, fpu, scale, wrote = (defaultdict(Counter), defaultdict(Counter),
                              defaultdict(Counter), defaultdict(set))
    for a in accesses:
        if a.form not in TOUCH:
            continue
        k = bisect.bisect_right(starts, a.target_rva) - 1
        if k < 0 or a.target_rva >= claims[k].end:
            continue
        c = claims[k]
        per[c.rva][(a.target_rva - c.rva, a.width)] += 1
        if a.fpu:
            fpu[c.rva][(a.target_rva - c.rva, a.fpu)] += 1
        if a.form == "indexed" and a.scale and a.target_rva == c.rva:
            scale[c.rva][a.scale] += 1
        if "w" in a.rw:
            wrote[c.rva].add(a.target_rva - c.rva)

    def pick(pred):
        return next((c for c in claims if pred(c)), None)

    def clone(c, **kw):
        f = {k: getattr(c, k) for k in Claim.__slots__}
        f.update(kw)
        return Claim(**f)

    def prim(t, sz):
        return {"k": "prim", "t": t, "sz": sz}

    plans = []
    c = pick(lambda c: per[c.rva] and c.extent >= 4
             and all(o == 0 and w == 4 for (o, w) in per[c.rva]))
    if c:
        plans.append(("narrow", "width", c,
                      lambda c: [clone(c, node=prim("u8", 1), extent=1)]))
    c = pick(lambda c: per[c.rva]
             and all(o == 0 and w == 1 for (o, w) in per[c.rva]))
    if c:
        plans.append(("widen", "width", c,
                      lambda c: [clone(c, node=prim("double", 8), extent=8)]))
    c = pick(lambda c: c.extent >= 4
             and any(o == 0 and t.startswith("f") for (o, t) in fpu[c.rva]))
    if c:
        plans.append(("float", "width", c, lambda c: [clone(
            c, node={"k": "arr", "t": "int[]", "sz": c.extent,
                     "n": max(c.extent // 4, 1), "el": prim("int", 4)})]))
    # the g_idleGeom bug: two members declared in the wrong order. The victim
    # carries a synthetic pair built from its OWN observed widths at +0 and +4,
    # reversed. (A swap between two SAME-SIZED members is invisible to a width
    # map at all - see the coverage note; that needs value evidence.)
    _TY = {1: "u8", 2: "u16", 4: "int", 8: "double"}

    def _wat(c, o):
        return {w for (oo, w) in per[c.rva] if oo == o and w in _TY}
    c = pick(lambda c: c.extent >= 8 and len(_wat(c, 0)) == 1
             and len(_wat(c, 4)) == 1 and _wat(c, 0) != _wat(c, 4))
    if c:
        def _swap(c):
            a, b = next(iter(_wat(c, 0))), next(iter(_wat(c, 4)))
            return [clone(c, node={"k": "rec", "t": "SwappedPair",
                                   "sz": c.extent,
                                   "m": [[0, ".m_second", prim(_TY[b], b)],
                                         [4, ".m_first", prim(_TY[a], a)]]})]
        plans.append(("swap", "width", c, _swap))
    c = pick(lambda c: c.extent >= 16
             and any(o >= c.extent // 2 for o in wrote[c.rva]))
    if c:
        plans.append(("halve", "unclaimed", c,
                      lambda c: [clone(c, extent=c.extent // 2)]))
    c = pick(lambda c: scale[c.rva] and max(scale[c.rva]) >= 4)
    if c:
        plans.append(("stride", "stride", c, lambda c: [clone(
            c, node={"k": "arr", "t": "char[]", "sz": c.extent,
                     "n": c.extent, "el": prim("char", 1)})]))
    c = pick(lambda c: c.extent >= 8
             and any(o + w > 4 and o < 4 for (o, w) in per[c.rva]))
    if c:
        plans.append(("split", "adjacent", c, lambda c: [
            clone(c, extent=4, node=prim("int", 4)),
            clone(c, rva=c.rva + 4, name=c.name + "$SPLIT",
                  extent=c.extent - 4, node=prim("int", 4))]))
    # shrink: an ARRAY a SINGLE function walks, cut to its first element, so the
    # same function's own tail accesses become a shortfall (a too-small COUNT
    # with forward storage).
    arr_fn = defaultdict(lambda: defaultdict(set))
    for a in accesses:
        if a.form not in TOUCH or not a.width:
            continue
        k = bisect.bisect_right(starts, a.target_rva) - 1
        if k < 0 or a.target_rva >= claims[k].end:
            continue
        c = claims[k]
        if c.node is None or c.node.get("k") != "arr":
            continue
        n, el = layout.element(c.node)
        if n <= 1 or ((el or {}).get("sz") or 0) != a.width:
            continue
        if a.owner is not None:
            arr_fn[c.rva][a.owner].add(a.target_rva - c.rva)
    shrink = None
    for c in claims:
        if c.node is None or c.node.get("k") != "arr":
            continue
        _n, el = layout.element(c.node)
        esz = (el or {}).get("sz") or 0
        if not esz:
            continue
        for offs in arr_fn.get(c.rva, {}).values():
            if 0 in offs and max(offs) >= esz and max(offs) + esz <= c.extent:
                shrink = c
                break
        if shrink:
            break
    if shrink:
        def _shrink(c):
            _n, el = layout.element(c.node)
            esz = el["sz"]
            return [clone(c, extent=esz,
                          node={"k": "arr", "t": c.node["t"], "sz": esz,
                                "n": 1, "el": el})]
        plans.append(("shrink", "shortfall", shrink, _shrink))
    return plans


def _dead_space(spine, accesses, cells):
    """An address in .data no claim covers and nothing in the image references
    - where a planted phantom must show up as unaccessed."""
    hot = {a.target_rva for a in accesses} | {c["target"] for c in cells} \
        | {c["site"] for c in cells}
    lo, hi = spine.img.pe.data_regions()["data"]
    for rva in range(hi - 0x400, lo, -0x40):
        c, _o = spine.locate(rva)
        if c is not None:
            continue
        if any((rva + d) in hot for d in range(-8, 24)):
            continue
        return rva
    return None


def run_selftest(limit=1):
    """[(tag, want, label, caught_rows)] - the injected-defect controls."""
    spine, accesses, cells, _stats, _f, _fs, owners = analyse()
    base = spine.claims
    plans = injection_plans(spine, accesses)
    dead = _dead_space(spine, accesses, cells)
    if dead is not None:
        plans.append(("phantom", "unaccessed", None, None))
    out = []
    for tag, want, victim, mutate in plans:
        if tag == "phantom":
            ghost = Claim(rva=dead, name="?g_injectedPhantom@@3HA",
                          unit="selftest", channel="src", kind="", extent=4,
                          section=".data", node={"k": "prim", "t": "int",
                                                 "sz": 4}, pct=100.0)
            mutated = sorted(base + [ghost], key=lambda c: c.rva)
            key, label = dead, f"synthetic claim at 0x{dead:x} in dead space"
        else:
            mutated = []
            for c in base:
                mutated.extend(mutate(c) if c.rva == victim.rva else [c])
            mutated.sort(key=lambda c: c.rva)
            key = victim.rva
            label = f"{victim.name[:40]} 0x{victim.rva:x}"
        probe = Spine(spine.img, spine.model, spine.layout, mutated,
                      spine.rows)
        rows, _st = derive_findings(probe, accesses, cells, owners)
        caught = [r for r in rows if r[0] == want and
                  (r[2] == key or r[4] == key
                   or (victim is not None
                       and victim.rva <= r[4] < victim.rva + victim.extent))]
        out.append((tag, want, label, caught[:limit]))
    return out


def do_selftest(_args):
    res = run_selftest()
    ok = sum(1 for _t, _w, _l, caught in res if caught)
    print(f"[selftest] {len(res)} injection(s) planted")
    for tag, want, label, caught in res:
        print(f"  {'CAUGHT' if caught else 'MISSED':6} {tag:8} -> {want:10} "
              f"{label}")
        for r in caught:
            print(f"           [{r[0]}/{r[1]}] {r[5]}")
    print(f"[selftest] {ok}/{len(res)} injected defects detected")
    return 0 if ok == len(res) else 1


# --------------------------------------------------------------------------- #
# queries (read-only, against the persisted map)                              #
# --------------------------------------------------------------------------- #
def _print_accesses(rows, base=None):
    for r in rows:
        off = f"+0x{r['target_rva'] - base:<5x}" if base is not None else \
            f"0x{r['target_rva']:08x}"
        w = f"w{r['width']}" if r["width"] else "addr"
        tag = r["fpu"] or r["ext"] or ""
        print(f"  0x{r['insn_rva']:06x} {r['form']:12} {r['rw']:2} {w:<5} "
              f"{tag:4} {off} {(r['text'] or '')[:46]:46} "
              f"{r['fn_name'] or '<gap>'}")


def do_at(con, rva):
    a = con.execute(
        "SELECT * FROM access WHERE (target_rva<=? AND (target_rva+width)>?) "
        "OR (width=0 AND target_rva=?) ORDER BY insn_rva",
        (rva, rva, rva)).fetchall()
    print(f"address 0x{rva:x}: {len(a)} reference(s)")
    c = con.execute("SELECT * FROM claim WHERE rva<=? AND rva+extent>?",
                    (rva, rva)).fetchone()
    if c:
        print(f"  claim {c['name']} [{c['unit']}] 0x{c['rva']:x} "
              f"+0x{c['extent']:x} ({c['channel']}) type={c['type'] or '?'} "
              f"offset +0x{rva - c['rva']:x}")
        f = con.execute("SELECT * FROM field WHERE sym_rva=? AND off<=? AND "
                        "off+size>?", (c["rva"], rva - c["rva"],
                                       rva - c["rva"])).fetchone()
        if f:
            print(f"  field {f['path'] or '.'} +0x{f['off']:x} {f['type']} "
                  f"{f['size']} B")
    else:
        print("  claim: NONE - unclaimed byte")
    _print_accesses(a)
    for cl in con.execute("SELECT * FROM cell WHERE target_rva=? OR site_rva=?",
                          (rva, rva)):
        print(f"  cell @0x{cl['site_rva']:x} ({cl['where_sec']}) {cl['kind']} "
              f"-> 0x{cl['target_rva']:x} {cl['tgt_sym_name'] or ''}")
    return 0


def do_range(con, lo, hi):
    a = con.execute("SELECT * FROM access WHERE target_rva>=? AND target_rva<? "
                    "ORDER BY target_rva, insn_rva", (lo, hi)).fetchall()
    print(f"range 0x{lo:x}..0x{hi:x}: {len(a)} reference(s)")
    _print_accesses(a)
    return 0


def do_symbol(con, key):
    if key.startswith("0x"):
        c = con.execute("SELECT * FROM claim WHERE rva=?",
                        (int(key, 16),)).fetchone()
    else:
        c = con.execute("SELECT * FROM claim WHERE name=?", (key,)).fetchone() \
            or con.execute("SELECT * FROM claim WHERE name LIKE ?",
                           (f"%{key}%",)).fetchone()
    if c is None:
        print(f"no claim matching {key}")
        return 1
    print(f"{c['name']}  [{c['unit']}]  0x{c['rva']:x} +0x{c['extent']:x} "
          f"({c['channel']})  {c['section']} "
          f"{'%.2f%%' % c['sect_pct'] if c['sect_pct'] >= 0 else 'n/a'}")
    print(f"  type: {c['type'] or '?'}")
    print(f"  accesses={c['n_access']} (r{c['n_read']}/w{c['n_write']}) "
          f"address-taken={c['n_addr']} reloc-cells-inside={c['n_cells']}")
    flds = con.execute("SELECT * FROM field WHERE sym_rva=? ORDER BY off",
                       (c["rva"],)).fetchall()
    acc = con.execute("SELECT * FROM access WHERE sym_rva=? AND in_extent=1 "
                      "ORDER BY sym_off, insn_rva", (c["rva"],)).fetchall()
    hits = defaultdict(Counter)
    for a in acc:
        if a["form"] in TOUCH:
            hits[a["sym_off"]][a["width"]] += 1
    if flds:
        print(f"  field map ({len(flds)}):")
        for f in flds[:64]:
            h = hits.get(f["off"])
            mark = ("  <- " + " ".join(f"w{w}x{n}" for w, n in sorted(h.items()))
                    if h else "")
            print(f"    +0x{f['off']:<5x} {f['size']:<3} {f['type']:<20} "
                  f"{f['path'] or '.'}{mark}")
        if len(flds) > 64:
            print(f"    ... {len(flds) - 64} more")
    untouched = sorted(set(hits) - {f["off"] for f in flds})
    if untouched:
        print("  accessed offsets with NO declared field: "
              + " ".join(f"+0x{o:x}" for o in untouched[:24]))
    print(f"  {len(acc)} reference(s):")
    _print_accesses(acc, c["rva"])
    for f in con.execute("SELECT * FROM finding WHERE sym_rva=?", (c["rva"],)):
        print(f"  [{f['category']}/{f['severity']}] 0x{f['addr']:x} "
              f"{f['detail']}  ({f['evidence']})")
    return 0


def do_fn(con, key):
    if key.startswith("0x"):
        rows = con.execute("SELECT * FROM access WHERE fn_rva=? "
                           "ORDER BY insn_rva", (int(key, 16),)).fetchall()
    else:
        rows = con.execute("SELECT * FROM access WHERE fn_name LIKE ? "
                           "ORDER BY insn_rva", (f"%{key}%",)).fetchall()
    print(f"{key}: {len(rows)} data reference(s)")
    for r in rows:
        w = f"w{r['width']}" if r["width"] else "addr"
        off = f"+0x{r['sym_off']:x}" if r["in_extent"] else ""
        print(f"  0x{r['insn_rva']:06x} {r['form']:12} {r['rw']:2} {w:<5} "
              f"0x{r['target_rva']:08x} "
              f"{(r['sym_name'] if r['in_extent'] else '<unclaimed>')[:44]:44} "
              f"{off}")
    return 0


def do_findings(con, cat, limit):
    q = "SELECT * FROM finding"
    args = ()
    if cat:
        q += " WHERE category=?"
        args = (cat,)
    q += " ORDER BY category, severity DESC, sym_rva"
    rows = con.execute(q, args).fetchall()
    by = Counter((r["category"], r["severity"]) for r in rows)
    print(f"{len(rows)} finding(s): "
          + ", ".join(f"{c}/{s}={n}" for (c, s), n in sorted(by.items())))
    for r in rows[:limit]:
        print(f"  [{r['category']}/{r['severity']}] 0x{r['addr']:08x} "
              f"{(r['sym_name'] or '-')[:48]:48} {r['detail']}")
        if r["evidence"]:
            print(f"      {r['evidence']}")
    if len(rows) > limit:
        print(f"  ... {len(rows) - limit} more (raise --limit)")
    return 0


def do_touched(args):
    """Emit the coalesced byte ranges retail's code TOUCHES - the join input
    for the claim-side coverage census (verify/data_coverage.py):

        uncovered AND touched   -> unmodelled data
        uncovered AND untouched -> padding

    One row per maximal run of bytes reached by at least one byte-touching
    access, with the widest access over it, the read/write split and the claim
    (if any) it falls in. Address-taking references are NOT included - they
    prove the object is used, not which of its bytes are."""
    from homm3.core.tsv import write as write_tsv
    con = connect(args.sqlite)
    rows = con.execute(
        "SELECT target_rva, end_rva, width, rw, form, sym_rva, sym_name "
        "FROM access WHERE width>0 AND form IN ('direct','indexed',"
        "'derived-disp') ORDER BY target_rva").fetchall()
    out, cur = [], None
    for r in rows:
        if cur and r["target_rva"] <= cur["end"]:
            cur["end"] = max(cur["end"], r["end_rva"])
        else:
            cur = {"start": r["target_rva"], "end": r["end_rva"], "sites": 0,
                   "reads": 0, "writes": 0, "maxw": 0, "forms": set(),
                   "sym": r["sym_name"], "sym_rva": r["sym_rva"]}
            out.append(cur)
        cur["sites"] += 1
        cur["reads"] += "r" in r["rw"]
        cur["writes"] += "w" in r["rw"]
        cur["maxw"] = max(cur["maxw"], r["width"])
        cur["forms"].add(r["form"])
    write_tsv(args.touched,
              ["# GENERATED by homm3.verify.data_access --touched - the byte "
               "ranges retail's code actually touches."],
              ["start", "end", "bytes", "sites", "reads", "writes",
               "max_width", "forms", "claim_rva", "claim"],
              [[f"0x{r['start']:x}", f"0x{r['end']:x}", r["end"] - r["start"],
                r["sites"], r["reads"], r["writes"], r["maxw"],
                ",".join(sorted(r["forms"])),
                f"0x{r['sym_rva']:x}" if r["sym_rva"] else "",
                r["sym"] or ""] for r in out])
    total = sum(r["end"] - r["start"] for r in out)
    print(f"[access-map] {len(out)} touched ranges, {total} bytes -> "
          f"{args.touched}")
    return 0


def main(argv=None) -> int:
    import argparse
    from pathlib import Path

    from homm3.core.paths import BUILD
    ap = argparse.ArgumentParser(prog="homm3 verify data-access",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build", action="store_true", help="sweep and persist")
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on a finding in a GATED category")
    ap.add_argument("--calibrate", action="store_true",
                    help="re-prove each category's suppression set on this tree")
    ap.add_argument("--selftest", action="store_true",
                    help="inject the known defect classes and require each to fire")
    ap.add_argument("--suppressed", nargs="?", const="",
                    help="the SITES a suppression class removed (re-argue it)")
    ap.add_argument("--at", help="every reference touching one address")
    ap.add_argument("--range", dest="rng", help="LO:HI byte range")
    ap.add_argument("--symbol", help="one claim: field map + every reference")
    ap.add_argument("--fn", help="every data reference one function makes")
    ap.add_argument("--findings", nargs="?", const="",
                    help="the derived worklist")
    ap.add_argument("--sql", help="raw SQL over the map")
    ap.add_argument("--touched", nargs="?", type=Path,
                    const=BUILD / "gen/data_touched_ranges.tsv",
                    help="write the byte ranges retail touches (the coverage join)")
    ap.add_argument("--limit", type=int, default=40,
                    help="cap the printed rows")
    ap.add_argument("--sqlite", type=Path, default=SQLITE,
                    help="the query index to read/write")
    ap.add_argument("--tsv", type=Path, default=TSV,
                    help="the grep-able access table to write")
    a = ap.parse_args(argv)

    if a.gate:
        return do_gate(a)
    if a.calibrate:
        return do_calibrate(a)
    if a.selftest:
        return do_selftest(a)
    if a.suppressed is not None:
        return do_suppressed(a)
    if a.touched:
        return do_touched(a)
    if a.build or not any((a.at, a.rng, a.symbol, a.fn, a.sql,
                           a.findings is not None)):
        return do_build(a)

    con = connect(a.sqlite)
    if a.at:
        return do_at(con, int(a.at, 16))
    if a.rng:
        lo, hi = (int(x, 16) for x in a.rng.replace("-", ":").split(":"))
        return do_range(con, lo, hi)
    if a.symbol:
        return do_symbol(con, a.symbol)
    if a.fn:
        return do_fn(con, a.fn)
    if a.sql:
        cur = con.execute(a.sql)
        cols = [d[0] for d in cur.description]
        addrish = [c.endswith("_rva") or c in ("addr", "start", "end", "off")
                   for c in cols]
        print("\t".join(cols))
        for row in cur:
            print("\t".join(f"0x{v:x}" if h and isinstance(v, int) else str(v)
                            for v, h in zip(row, addrish)))
        return 0
    return do_findings(con, a.findings or None, a.limit)


if __name__ == "__main__":
    raise SystemExit(main())
