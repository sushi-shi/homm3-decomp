"""homm3.verify.access_map - THE static map of every data access retail makes.

The match score cannot answer "which bytes does retail's code actually touch,
and how wide?", because WE choose the extent of every claim: a too-small claim
always scores 100 (objdiff only compares what we told it to compare). Model
`float x` where retail has `struct { float x, y; }` and the four bytes match,
the section reports exact, and `y` is silently unmodelled.

This module reads the RETAIL image and records, per instruction, WHAT data byte
range it touches and HOW WIDE. That is the evidence the score structurally
cannot produce. `verify/data_access.py` turns it into verdicts.

SITE ENUMERATION IS THE .reloc HIGHLOW TABLE, not a from-scratch disassembly.
Every absolute operand in the image is relocated, so `.reloc` is a complete and
reliable index of *where* the absolute references are; we disassemble only at
each site, to learn the width and the addressing form. That sidesteps needing a
correct whole-image disassembly and it covers code we have not reconstructed -
which is the point.

DECODE. Two objdump passes over .text:
  linear    the whole section decoded from its start (desyncs on data-in-text)
  anchored  the same bytes with every byte OUTSIDE a claimed function extent
            overwritten by 0x90, so the decoder re-syncs exactly on each
            function start instead of drifting through an embedded jump table
A site is decoded by the anchored pass when it lies inside a known function,
else by the linear pass; a site neither pass can place stays `undecoded` and is
counted, never guessed.

FORMS (what the map can and cannot see):
  direct        `ds:addr`               reloc-anchored, the datum itself
  indexed       `[reg*s+addr]`          reloc-anchored, addr is a TABLE base and
                                        s is a hard element-size witness
  derived-disp  `[reg+disp]` after a
                proven `mov reg,&sym`   NOT in .reloc - recovered by a bounded,
                                        single-block forward propagation
  lea / imm     address-taken           the object escapes; width unknown
  indcall       `call/jmp [addr]`       the cell holds a function pointer
  iat           indcall into the IAT    an import, excluded from symbol evidence

STRUCTURAL BLIND SPOT, stated honestly: an access through a base register whose
value did not come from an absolute operand in the same basic block carries no
relocation and no local provenance, so it is invisible here. That is every
`this`-relative field access, every access through a pointer loaded FROM memory,
and every escape through a call argument. `derived-disp` recovers only the
single-block case. The map is exhaustive for ABSOLUTE references and partial for
register-relative ones; `coverage()` reports both numbers.

PLUMBING (the port): retail bytes + relocations from homm3.sema.image, the
decode from homm3.tool.objdump, the claim spine from homm3.model (which
replaced symbol_names.csv), and the declared field layout from
homm3.verify.layout (pylibclang under the TU's own i386/MSVC flags).
"""

from __future__ import annotations

import bisect
import re
import sqlite3
import struct
from collections import Counter, defaultdict
from pathlib import Path

from homm3.core.paths import BUILD

SQLITE = BUILD / "gen/data_access_map.sqlite"
TSV = BUILD / "gen/data_access_map.tsv"

_WIDTH = {"BYTE": 1, "WORD": 2, "DWORD": 4, "QWORD": 8, "TBYTE": 10, "FWORD": 6}
_WPTR = re.compile(r"\b(BYTE|WORD|DWORD|QWORD|TBYTE|FWORD) PTR\b")
_R32 = ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")
_TERM = re.compile(r"([+-])?\s*([a-z0-9]+)(?:\*([1248]))?")
_REGWIDTH = {**{r: 4 for r in _R32},
             **{r: 2 for r in ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp")},
             **{r: 1 for r in ("al", "bl", "cl", "dl", "ah", "bh", "ch", "dh")}}

# mnemonic -> how a MEMORY FIRST-operand is used (default: dest of a plain move)
_RMW = {"add", "sub", "adc", "sbb", "and", "or", "xor", "not", "neg", "inc",
        "dec", "sal", "shl", "sar", "shr", "rol", "ror", "rcl", "rcr", "xchg",
        "xadd", "btc", "btr", "bts"}
_READ_FIRST = {"cmp", "test", "push", "bt", "imul"}
_FPU_LOAD = {"fld", "fild", "fadd", "fsub", "fsubr", "fmul", "fdiv", "fdivr",
             "fcom", "fcomp", "ficom", "ficomp", "fiadd", "fisub", "fimul",
             "fidiv", "fidivr", "fisubr"}
_FPU_STORE = {"fst", "fstp", "fist", "fistp", "fisttp", "fbstp"}
_FPU_INT = {"fild", "fist", "fistp", "fisttp", "ficom", "ficomp", "fiadd",
            "fisub", "fisubr", "fimul", "fidiv", "fidivr"}
# instructions that end a basic block for the derived-disp propagation
_XFER = re.compile(r"^(j\w+|call|ret\w*|loop\w*|iret\w*|int3?|hlt|leave)$")
# implicit register clobbers: the destination is not an operand, so the
# "ops[0] is our register" test cannot see them
_CLOBBER = {"mul": "eax edx", "imul": "eax edx", "div": "eax edx",
            "idiv": "eax edx", "cdq": "edx", "cwd": "edx", "cbw": "eax",
            "cwde": "eax", "lodsb": "eax esi", "lodsd": "eax esi",
            "lodsw": "eax esi", "stosb": "edi", "stosd": "edi", "stosw": "edi",
            "movsb": "esi edi", "movsd": "esi edi", "movsw": "esi edi",
            "scasb": "edi", "scasd": "edi", "scasw": "edi"}

STRING_OPS = ("stos", "movs", "scas", "cmps", "lods")


class Access:
    """One decoded data reference. `width` 0 means the instruction takes the
    ADDRESS (lea/imm/push) rather than touching bytes, so it covers no range."""
    __slots__ = ("insn_rva", "insn_len", "mnemonic", "site_rva", "target_rva",
                 "width", "rw", "form", "base_reg", "index_reg", "scale", "disp",
                 "fpu", "ext", "origin", "text", "owner")

    def __init__(self, **kw):
        for k in self.__slots__:
            setattr(self, k, kw.get(k))

    @property
    def end_rva(self):
        return self.target_rva + (self.width or 0)


# --- disassembly --------------------------------------------------------------
_JMP = re.compile(r"^(j\w+|loop\w*)\s+0x([0-9a-f]+)")


class Decode:
    """A decoded view of .text: instruction starts, text, and branch targets."""

    def __init__(self, starts, lines, targets):
        self.starts, self.lines, self.targets = starts, lines, targets

    def at(self, rva):
        """Index of the instruction CONTAINING rva, or None."""
        k = bisect.bisect_right(self.starts, rva) - 1
        return k if k >= 0 else None

    def length(self, k):
        return (self.starts[k + 1] - self.starts[k]
                if k + 1 < len(self.starts) else 1)


def _decode(blob: bytes, vma: int) -> Decode:
    from homm3.tool import objdump
    starts, lines, targets = [], [], set()
    for ln in objdump.disassemble(blob, vma=vma).splitlines():
        if ":\t" not in ln:
            continue
        addr, rest = ln.split(":\t", 1)
        text = rest.split("\t")[-1].strip()
        if not text:
            continue
        try:
            starts.append(int(addr.strip(), 16))
        except ValueError:
            continue
        lines.append(text)
        j = _JMP.match(text)
        if j:
            targets.add(int(j.group(2), 16))
    return Decode(starts, lines, targets)


def disasm_text(img, fstarts=None, fsize=None):
    """(linear, anchored) decodes of .text.

    `anchored` overwrites every byte outside a claimed function extent with
    0x90 so the decoder cannot drift past a function start; 0x90 is 1 byte, so
    every function start stays on an instruction boundary no matter the gap
    parity. Returns (linear, None) when no function inventory is supplied."""
    t = img.pe.section(".text")
    va, blob = t["va"], img.pe.data[t["rptr"]:t["rptr"] + t["rsize"]]
    linear = _decode(blob, va)
    if not fstarts:
        return linear, None
    keep = bytearray(b"\x90" * len(blob))
    hi = va + len(blob)
    for f in fstarts:
        if not (va <= f < hi):
            continue
        n = fsize.get(f) or 0
        if not n:
            continue
        a, b = f - va, min(f - va + n, len(blob))
        keep[a:b] = blob[a:b]
    return linear, _decode(bytes(keep), va)


# --- operand decoding ---------------------------------------------------------
def split_operands(asm):
    """(mnemonic, [operand, ...]) - Intel syntax, no commas inside brackets."""
    asm = asm.split(" <")[0]                 # strip objdump's <offset+N> notes
    asm = asm.split("#")[0].strip()          # strip objdump's trailing comment
    parts = asm.split(None, 1)
    if len(parts) == 1:
        return parts[0], []
    mnem = parts[0]
    if mnem in ("rep", "repz", "repnz", "repe", "repne", "lock"):
        return split_operands(parts[1])
    return mnem, [o.strip() for o in parts[1].split(",")]


def parse_mem(op):
    """(width, base, index, scale, disp) for one memory operand, else None.

    `disp` is the SUM of every numeric term in the effective address (the
    absolute VA plus any literal displacement), so `[eax*4+0x5f1234]` and
    `[eax+0x8]` are described by the same three fields."""
    if "[" not in op and "ds:" not in op and "PTR" not in op:
        return None
    m = _WPTR.search(op)
    width = _WIDTH[m.group(1)] if m else 0
    if "[" in op:
        inner = op[op.index("[") + 1:op.rindex("]")]
    else:
        inner = op.split("ds:")[-1].strip()
    base = index = ""
    scale = 0
    disp = 0
    for sign, tok, sc in _TERM.findall(inner.replace(" ", "")):
        if not tok:
            continue
        neg = sign == "-"
        if tok in _REGWIDTH:
            if sc:
                index, scale = tok, int(sc)
            elif not base:
                base = tok
            else:
                index, scale = tok, 1
            continue
        try:
            v = int(tok, 16)
        except ValueError:
            continue
        disp += -v if neg else v
    return width, base, index, scale, disp


def _direction(mnem, opidx, ops):
    if mnem in ("movzx", "movsx"):
        return "r"
    if mnem in _FPU_STORE:
        return "w"
    if mnem in _FPU_LOAD:
        return "r"
    if opidx != 0:
        return "r"
    if mnem in _READ_FIRST:
        return "r"
    if mnem in _RMW:
        return "rw"
    if mnem == "pop" or mnem.startswith("set") or mnem.startswith("mov"):
        return "w"
    return "rw"                              # unknown mnemonic, memory dest


def _fpu_tag(mnem, width):
    if mnem in _FPU_INT:
        return f"i{width * 8}" if width else "i?"
    return {4: "f32", 8: "f64", 10: "f80"}.get(width, "f?")


_ANDMASK = {"0xffff": 2, "0xff": 1}


def and_mask(dec, k, reg, window=6):
    """`mov r32,[mem]` + `and r32,0xffff` is MSVC5's movzx-avoidance.

    cl 5.0 loads a narrow global with a FULL-WIDTH read and masks the register
    (movzx was slow on the Pentium), so the 4-byte access is a 2-byte one in
    disguise. Return the masked width, or 0 if the register is not masked
    before it is redefined / the block ends. Ambiguous by construction - a real
    `u32 & 0xffff` looks the same - so callers must SUPPRESS on it, never
    rewrite the recorded width."""
    for j in range(k + 1, min(k + 1 + window, len(dec.starts))):
        rva, asm = dec.starts[j], dec.lines[j]
        if rva in dec.targets:
            break
        mnem, ops = split_operands(asm)
        if _XFER.match(mnem):
            break
        if len(ops) == 2 and ops[0] == reg:
            if mnem == "and":
                return _ANDMASK.get(ops[1], 0)
            return 0                         # redefined without a mask
        if reg in _CLOBBER.get(mnem, ""):
            break
    return 0


def classify(site, stored, dec, k, iat):
    """Decode the instruction at index `k` into an Access for reloc `site`.
    `stored` is the VA the operand holds (the image base is still on it)."""
    base = iat[2]
    target = stored - base
    insn_rva, asm = (dec.starts[k], dec.lines[k]) if k is not None else (site, "")
    ac = Access(insn_rva=insn_rva, insn_len=dec.length(k) if k is not None else 0,
                mnemonic="", site_rva=site, target_rva=target, width=0, rw="-",
                form="undecoded", base_reg="", index_reg="", scale=0, disp=0,
                fpu="", ext="", origin="reloc", text=asm)
    va_hex = f"0x{stored:x}"
    if va_hex not in asm:
        return ac                            # desync / data-in-text: never guess
    mnem, ops = split_operands(asm)
    ac.mnemonic = mnem
    opidx = next((i for i, o in enumerate(ops) if va_hex in o), None)
    if opidx is None:
        return ac
    op = ops[opidx]
    if mnem == "lea":
        mem = parse_mem(op)
        ac.form, ac.rw = "lea", "-"
        if mem:
            _w, ac.base_reg, ac.index_reg, ac.scale, _d = mem
        return ac
    mem = parse_mem(op)
    if mem is None:
        ac.form, ac.rw = "imm", "-"          # push 0xVA / mov reg,0xVA
        return ac
    width, breg, index, scale, disp = mem
    if not width and len(ops) == 2:
        width = _REGWIDTH.get(ops[1 - opidx], 0)   # moffs prints no PTR keyword
    ac.width, ac.base_reg, ac.index_reg, ac.scale = width, breg, index, scale
    ac.disp = disp - stored                  # literal displacement past the VA
    ac.form = "indexed" if (breg or index) else "direct"
    if mnem in ("call", "jmp"):
        ac.form = "iat" if iat[0] <= target < iat[1] else "indcall"
        ac.rw = "r"
        return ac
    ac.rw = _direction(mnem, opidx, ops)
    if mnem in ("movzx", "movsx"):
        ac.ext = "u" if mnem == "movzx" else "i"
    elif mnem == "mov" and opidx == 1 and ac.rw == "r" and width == 4 \
            and k is not None and _REGWIDTH.get(ops[0]) == 4:
        m = and_mask(dec, k, ops[0])
        if m:
            ac.ext = f"m{m}"                 # movzx-avoidance: a narrow load
    if mnem in _FPU_LOAD or mnem in _FPU_STORE:
        ac.fpu = _fpu_tag(mnem, width)
    return ac


# --- derived (register-relative) accesses -------------------------------------
def derive(dec, seeds, in_data, stop=frozenset(), budget=48):
    """Recover `[reg+disp]` accesses whose base came from an absolute operand.

    A seed is a `mov r32,&sym` / `lea r32,[&sym]` at instruction index k. We walk
    forward inside the SAME basic block - stopping at any control transfer, at
    any instruction that is a branch target or a function start, and at any
    write to the base register (explicit operand OR implicit clobber) - and emit
    an Access for each memory operand based on that register. Single-block only,
    no register copies: deliberately conservative, because a wrong provenance
    would INVENT a data reference, which is worse than missing one."""
    out = []
    for k, reg, addr in seeds:
        for j in range(k + 1, min(k + 1 + budget, len(dec.starts))):
            rva, asm = dec.starts[j], dec.lines[j]
            if rva in dec.targets or rva in stop:
                break                        # a join: the register may differ
            mnem, ops = split_operands(asm)
            if _XFER.match(mnem):
                break
            if reg in _CLOBBER.get(mnem, ""):
                break
            for i, op in enumerate(ops):
                mem = parse_mem(op)
                if not mem or mem[1] != reg or mem[2]:
                    continue                 # base must be OUR reg, no index
                width, _b, _ix, _sc, disp = mem
                t = addr + disp
                if not in_data(t):
                    continue
                if not width and len(ops) == 2:
                    width = _REGWIDTH.get(ops[1 - i], 0)
                out.append(Access(
                    insn_rva=rva, insn_len=dec.length(j), mnemonic=mnem,
                    site_rva=0, target_rva=t, width=width,
                    rw=_direction(mnem, i, ops), form="derived-disp",
                    base_reg=reg, index_reg="", scale=0, disp=disp,
                    fpu=_fpu_tag(mnem, width)
                    if mnem in _FPU_LOAD or mnem in _FPU_STORE else "",
                    ext="u" if mnem == "movzx" else
                        "i" if mnem == "movsx" else "",
                    origin="derived", text=asm))
            # the register is redefined -> our provenance ends
            if ops and ops[0].strip() == reg and mnem not in _READ_FIRST:
                break
    return out


def seeds_from(dec, accesses):
    """Seed list for derive(): every lea/imm access that lands an address in a
    register. `push 0xVA` and `mov [mem],0xVA` are excluded - the address leaves
    the block, so its later use has no local provenance."""
    idx = {r: i for i, r in enumerate(dec.starts)}
    out = []
    for ac in accesses:
        if ac.form not in ("lea", "imm"):
            continue
        k = idx.get(ac.insn_rva)
        if k is None:
            continue
        mnem, ops = split_operands(ac.text)
        if not ops:
            continue
        dst = ops[0].strip()
        if dst not in _R32:
            continue
        if mnem == "lea" and (ac.base_reg or ac.index_reg):
            continue                         # lea r,[base+&sym]: not a pure addr
        if mnem not in ("lea", "mov"):
            continue
        out.append((k, dst, ac.target_rva))
    return out


# --- the claim spine ----------------------------------------------------------
class Claim:
    """One MODELLED datum: the Model's binding plus its declared layout."""
    __slots__ = ("rva", "name", "unit", "channel", "kind", "section", "space",
                 "extent", "node", "pct")

    def __init__(self, **kw):
        for k in self.__slots__:
            setattr(self, k, kw.get(k))

    @property
    def end(self):
        return self.rva + self.extent


def section_pct():
    """{(unit, section): fuzzy%} from the compare report - the calibration
    control set's admission test."""
    import json
    from homm3.verify import scores as sc
    out = {}
    try:
        doc = json.loads(sc.report_path().read_text())
    except SystemExit:
        return out
    for u in doc.get("units", []):
        for s in u.get("sections", []):
            if s["name"] != ".text":
                out[(u["name"].split("/")[-1], s["name"])] = float(
                    s.get("fuzzy_match_percent") or 0.0)
    return out


def build_claims(model, layout):
    """([Claim] for every NAMED binding, [census rows] for the whole spine).

    The Model owns the extent (a claimed size, else the census-derived one), so
    the old declared/next-symbol/clamp arithmetic is gone: a size that crossed
    its neighbour is already a model violation. The declared TYPE - and with it
    every width verdict - comes from the layout oracle, and only for a datum
    whose unit really declares that symbol."""
    pct = section_pct()
    claims, rows = [], []
    for b in model.data:
        # the retail section a byte lives in (this image has no separate .bss
        # header) vs the COFF section OUR object puts it in - the score is
        # reported per COFF section, so the calibration join needs `.bss`
        sec = {"rdata": ".rdata", "data": ".data", "bss": ".data",
               "idata": ".idata"}.get(b.space, b.space)
        rows.append((b.rva, b.rva + b.size, b.kind, b.channel, b.name))
        if not b.channel:
            continue
        node = layout.var_node(b.unit, b.name) if b.unit else None
        claims.append(Claim(rva=b.rva, name=b.name, unit=b.unit,
                            channel=b.channel, kind=b.kind, section=sec,
                            space=b.space, extent=b.size, node=node,
                            pct=pct.get((b.unit, f".{b.space}"))))
    claims.sort(key=lambda c: c.rva)
    rows.sort()
    return claims, rows


# --- the sweep ----------------------------------------------------------------
def iat_range(pe):
    """[lo, hi) of the import address table (data directory 12)."""
    off = struct.unpack_from("<I", pe.data, 0x3C)[0]
    opt = off + 24
    rva, size = struct.unpack_from("<II", pe.data, opt + 96 + 12 * 8)
    return (rva, rva + size) if rva else (0, 0)


def data_ranges(pe):
    """[(name, lo, hi)] of every region a datum can live in."""
    reg = pe.data_regions()
    return [("." + name, lo, hi) for name, (lo, hi) in reg.items()]


class Owners:
    """rva -> the claimed function containing it (Model function bindings)."""

    def __init__(self, model):
        fns = sorted((b.rva, b.size, b.name, b.unit) for b in model.functions)
        self.starts = [f[0] for f in fns]
        self.rows = fns
        self.fsize = {f[0]: f[1] for f in fns}
        self.names = {f[0]: (f[2], f[3]) for f in fns}

    def at(self, rva):
        i = bisect.bisect_right(self.starts, rva) - 1
        if i < 0:
            return None
        start, size, _n, _u = self.rows[i]
        return start if not size or rva < start + size else None


def sweep(img, model):
    """(accesses, cells, stats) over the whole retail image."""
    pe = img.pe
    dr = data_ranges(pe)
    iat = (*iat_range(pe), img.base)
    tlo, thi = pe.text_span()
    owners = Owners(model)
    fstarts, fsize = owners.starts, owners.fsize

    lows = [lo for _, lo, _ in dr]

    def in_data(rva):
        k = bisect.bisect_right(lows, rva) - 1
        return k >= 0 and rva < dr[k][2]

    linear, anchored = disasm_text(img, fstarts, fsize)

    def pick(site):
        """(decode, index) - the anchored decode inside a known function, the
        linear one otherwise; whichever actually places the site wins."""
        inside = owners.at(site) is not None
        order = (anchored, linear) if inside and anchored else (linear, anchored)
        for dec in order:
            if dec is None:
                continue
            k = dec.at(site)
            if k is None:
                continue
            yield dec, k

    stats = Counter()
    accesses, cells = [], []
    for site, target in img.relocs_in(tlo, thi):
        stored = target + img.base
        if not in_data(target):
            if iat[0] <= target < iat[1]:
                stats["to-iat"] += 1
            elif tlo <= target < thi:
                stats["to-text"] += 1
            else:
                stats["to-other"] += 1
            continue
        best = None
        for dec, k in pick(site):
            ac = classify(site, stored, dec, k, iat)
            if ac.form != "undecoded":
                best = ac
                break
            best = best or ac
        if best is None:
            best = classify(site, stored, linear, None, iat)
        if best.form == "undecoded":
            # Neither decode could read this site as an operand. It is usually
            # a relocated POINTER CELL living in .text - a data table the
            # linker placed in the code section (dinput's 16-byte device table
            # inside _DirectInputCreateA@16's census extent is the measured
            # example) - and sometimes a decode desync. We refuse to pick:
            # the row stays an `undecoded` reference (never given a width, so
            # no width/stride verdict can ride it) AND is recorded as a
            # pointer-cell candidate, because either way the image NAMES that
            # address, which is all the phantom test may conclude from it.
            cells.append({"site": site, "target": target, "kind": "undecoded",
                          "where": ".text"})
            stats["cell-in-text"] += 1
        accesses.append(best)
        stats[f"form-{best.form}"] += 1

    # derived pass: run on BOTH decodes' seeds, dedup by (insn_rva, target)
    seen = set()
    derived = []
    stop = frozenset(fstarts)
    for dec in (anchored, linear):
        if dec is None:
            continue
        sd = seeds_from(dec, accesses)
        if dec is (anchored or linear):
            stats["seed-total"] = len(sd)
            # a register load followed within 3 instructions by a call is an
            # object handed to a CALLEE: every field access it makes is
            # `this`-relative and therefore invisible to this map
            stats["seed-handed-to-callee"] = sum(
                1 for k, _r, _a in sd
                if any(dec.lines[j].startswith("call")
                       for j in range(k + 1, min(k + 4, len(dec.lines)))))
        for ac in derive(dec, sd, in_data, stop):
            key = (ac.insn_rva, ac.target_rva, ac.width)
            if key in seen:
                continue
            seen.add(key)
            derived.append(ac)
    stats["form-derived-disp"] = len(derived)
    accesses.extend(derived)

    for sec, lo, hi in dr:
        for site, t in img.relocs_in(lo, hi):
            kind = ("fnptr" if tlo <= t < thi
                    else "dataptr" if in_data(t) else "otherptr")
            cells.append({"site": site, "target": t, "kind": kind, "where": sec})
            stats[f"cell-{kind}"] += 1
    return accesses, cells, stats


# --- persistence --------------------------------------------------------------
SCHEMA = """
PRAGMA journal_mode=OFF;
CREATE TABLE meta   (key TEXT PRIMARY KEY, value TEXT);
CREATE TABLE access (
  id INTEGER PRIMARY KEY, insn_rva INT, insn_len INT, mnemonic TEXT,
  site_rva INT, target_rva INT, width INT, end_rva INT, rw TEXT, form TEXT,
  base_reg TEXT, index_reg TEXT, scale INT, disp INT, fpu TEXT, ext TEXT,
  origin TEXT, fn_rva INT, fn_name TEXT, fn_unit TEXT,
  sym_rva INT, sym_name TEXT, sym_off INT, in_extent INT, text TEXT);
CREATE TABLE cell   (
  id INTEGER PRIMARY KEY, site_rva INT, target_rva INT, kind TEXT,
  where_sec TEXT, sym_rva INT, sym_name TEXT, sym_off INT,
  tgt_sym_rva INT, tgt_sym_name TEXT, tgt_sym_off INT);
CREATE TABLE claim  (
  rva INT PRIMARY KEY, name TEXT, unit TEXT, channel TEXT, type TEXT,
  section TEXT, extent INT, sect_pct REAL,
  n_access INT, n_read INT, n_write INT, n_addr INT, n_cells INT);
CREATE TABLE field  (
  sym_rva INT, off INT, size INT, path TEXT, type TEXT, is_ptr INT,
  is_float INT, resolved INT);
CREATE TABLE finding(
  id INTEGER PRIMARY KEY, category TEXT, severity TEXT, sym_rva INT,
  sym_name TEXT, addr INT, detail TEXT, evidence TEXT);
"""
INDEXES = """
CREATE INDEX ix_acc_target ON access(target_rva);
CREATE INDEX ix_acc_sym    ON access(sym_rva);
CREATE INDEX ix_acc_fn     ON access(fn_rva);
CREATE INDEX ix_acc_insn   ON access(insn_rva);
CREATE INDEX ix_cell_site  ON cell(site_rva);
CREATE INDEX ix_cell_tgt   ON cell(target_rva);
CREATE INDEX ix_fld_sym    ON field(sym_rva);
CREATE INDEX ix_fnd_cat    ON finding(category);
CREATE INDEX ix_fnd_sym    ON finding(sym_rva);
"""

TSV_COLS = ["insn_rva", "insn_len", "mnemonic", "site_rva", "target_rva",
            "width", "end_rva", "rw", "form", "base_reg", "index_reg", "scale",
            "disp", "fpu", "ext", "origin", "fn_rva", "fn_name", "fn_unit",
            "sym_rva", "sym_name", "sym_off", "in_extent", "text"]
_HEXCOLS = {"insn_rva", "site_rva", "target_rva", "end_rva", "fn_rva",
            "sym_rva"}


def locate(starts, claims, rva):
    """(claim, offset, in_extent) for a data rva, or (None, -1, 0)."""
    k = bisect.bisect_right(starts, rva) - 1
    if k < 0:
        return None, -1, 0
    c = claims[k]
    return c, rva - c.rva, int(rva < c.end)


def persist(img, layout, accesses, cells, claims, stats, sqlite_path=SQLITE,
            tsv_path=TSV, findings=(), model=None):
    """Write the map. sqlite is the query index; the TSV is the grep-able,
    diffable copy of the access table (the artifact a human reads, written
    only when it changed)."""
    owners = Owners(model) if model is not None else None
    starts = [c.rva for c in claims]

    arows, per = [], defaultdict(Counter)
    for ac in accesses:
        f = owners.at(ac.insn_rva) if owners else None
        fname, funit = owners.names.get(f, ("", "")) if f else ("", "")
        c, off, inx = locate(starts, claims, ac.target_rva)
        arows.append((ac.insn_rva, ac.insn_len, ac.mnemonic or "", ac.site_rva,
                      ac.target_rva, ac.width or 0, ac.end_rva, ac.rw, ac.form,
                      ac.base_reg or "", ac.index_reg or "", ac.scale or 0,
                      ac.disp or 0, ac.fpu or "", ac.ext or "", ac.origin,
                      f or 0, fname or "", funit or "",
                      c.rva if c else 0, c.name if c else "", off, inx,
                      ac.text or ""))
        if c and inx:
            p = per[c.rva]
            if ac.form in ("lea", "imm"):
                p["addr"] += 1
            else:
                p["access"] += 1
                if "r" in ac.rw:
                    p["read"] += 1
                if "w" in ac.rw:
                    p["write"] += 1

    crows = []
    for cl in cells:
        c, off, inx = locate(starts, claims, cl["site"])
        t, toff, tinx = locate(starts, claims, cl["target"])
        if c and inx:
            per[c.rva]["cells"] += 1
        crows.append((cl["site"], cl["target"], cl["kind"], cl.get("where", ""),
                      c.rva if c and inx else 0, c.name if c and inx else "",
                      off if inx else -1,
                      t.rva if t and tinx else 0, t.name if t and tinx else "",
                      toff if tinx else -1))

    sqlite_path.parent.mkdir(parents=True, exist_ok=True)
    if sqlite_path.exists():
        sqlite_path.unlink()
    con = sqlite3.connect(sqlite_path)
    con.executescript(SCHEMA)
    con.executemany(
        "INSERT INTO access(insn_rva,insn_len,mnemonic,site_rva,target_rva,"
        "width,end_rva,rw,form,base_reg,index_reg,scale,disp,fpu,ext,origin,"
        "fn_rva,fn_name,fn_unit,sym_rva,sym_name,sym_off,in_extent,text) "
        "VALUES(" + ",".join("?" * 24) + ")", arows)
    con.executemany(
        "INSERT INTO cell(site_rva,target_rva,kind,where_sec,sym_rva,sym_name,"
        "sym_off,tgt_sym_rva,tgt_sym_name,tgt_sym_off) VALUES("
        + ",".join("?" * 10) + ")", crows)
    con.executemany(
        "INSERT INTO claim(rva,name,unit,channel,type,section,extent,sect_pct,"
        "n_access,n_read,n_write,n_addr,n_cells) VALUES("
        + ",".join("?" * 13) + ")",
        [(c.rva, c.name, c.unit, c.channel, layout.spelling(c.node) if c.node
          else "", c.section, c.extent,
          c.pct if c.pct is not None else -1.0,
          per[c.rva]["access"], per[c.rva]["read"], per[c.rva]["write"],
          per[c.rva]["addr"], per[c.rva]["cells"]) for c in claims])
    con.executemany(
        "INSERT INTO field(sym_rva,off,size,path,type,is_ptr,is_float,resolved) "
        "VALUES(?,?,?,?,?,?,?,?)",
        [(c.rva, f.off, f.size, f.path, f.type, int(f.is_ptr),
          int(f.is_float), int(f.resolved))
         for c in claims if c.node
         for f in layout.flatten(c.node, cap=512)])
    con.executemany("INSERT INTO meta(key,value) VALUES(?,?)",
                    [(k, str(v)) for k, v in stats.items()])
    if findings:
        con.executemany(
            "INSERT INTO finding(category,severity,sym_rva,sym_name,addr,"
            "detail,evidence) VALUES(?,?,?,?,?,?,?)", findings)
    con.executescript(INDEXES)
    con.commit()
    con.close()

    from homm3.core.tsv import write as write_tsv
    rows = []
    for r in sorted(arows, key=lambda r: (r[4], r[0])):
        rows.append([f"0x{v:x}" if name in _HEXCOLS and isinstance(v, int)
                     else str(v) for name, v in zip(TSV_COLS, r)])
    write_tsv(tsv_path, ["# GENERATED by homm3.verify.access_map - every "
                         "absolute data reference retail's code makes."],
              TSV_COLS, rows)
    return len(arows), len(crows)


def connect(path=SQLITE):
    if not Path(path).exists():
        raise SystemExit(f"no access map at {path} - run "
                         f"`python3 -m homm3.verify.data_access --build`")
    con = sqlite3.connect(f"file:{path}?mode=ro", uri=True)
    con.row_factory = sqlite3.Row
    return con


def load():
    """(img, model, layout, accesses, cells, claims, rows, stats) - one sweep."""
    from homm3.model import resolve
    from homm3.sema.image import retail
    from homm3.verify.layout import harvest
    img = retail()
    model = resolve()
    layout, _problems = harvest()
    accesses, cells, stats = sweep(img, model)
    claims, rows = build_claims(model, layout)
    return img, model, layout, accesses, cells, claims, rows, stats
