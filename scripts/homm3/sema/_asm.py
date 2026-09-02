"""homm3.sema._asm - shared disassembly-text machinery for diff/disasm.

Ported from the homm2 sibling's analysis/disasm.py engine. Two text
producers, one text shape:

  objdump(...)     llvm-objdump -dr over a COFF object the pipeline made
                   (base = compiled, target = delinked) - both diff sides
                   share ONE disassembler so only real byte diffs survive.
  image_text(...)  capstone over the retail image bytes, for the
                   functions no delinked unit covers yet (most of the
                   game). Same "off: bytes<TAB>mn<TAB>ops" row shape, so
                   the CFG/lite renderers below work on either producer.

The CFG comparison unit (`cfg`) names terminators by BLOCK INDEX, not
address, which is what makes a compiled object and retail bytes
comparable. Instruction masking has two grades: `mask_insn` (blocks
views - keeps stack slots and small constants) and `norm` (the flat asm
diff - masks every absolute address).
"""
from __future__ import annotations

import bisect
import re
import struct
import subprocess

from homm3.core import common
from homm3.sema._common import die

BASE = common.HOMM3_DIR / "build/objdiff/base"
TARGET = common.HOMM3_DIR / "build/objdiff/target"
NORMAL_BASE = common.HOMM3_DIR / "build/objdiff/normalized/base"
NORMAL_TARGET = common.HOMM3_DIR / "build/objdiff/normalized/target"


# --- producer 1: llvm-objdump over pipeline COFF objects ---------------------------

def _public_text_symbols(obj) -> set:
    """External text symbols of *obj*. COMDAT sections start at address
    zero per function, so numeric ranges cannot select one function -
    public-name boundaries can."""
    res = subprocess.run(["llvm-nm", "-P", str(obj)],
                         capture_output=True, text=True)
    if res.returncode != 0:
        die(f"llvm-nm failed on {obj.name}:\n{res.stderr.strip()}")
    names = set()
    for line in res.stdout.splitlines():
        fields = line.split()
        if len(fields) >= 3 and fields[1] == "T":
            names.add(fields[0])
    return names


_SYMBOL_TITLE = re.compile(r"^\s*[0-9a-fA-F]+ <(.+)>:$")


def _slice_public_symbol(text, name, ordinal, public_names):
    """One complete public function from a full object disassembly. Unlike
    --disassemble-symbols this keeps compiler-generated private $L labels
    inside the body; a new PUBLIC label or section ends the span."""
    selected = False
    occurrence = 0
    body = []
    for line in text.splitlines():
        title = _SYMBOL_TITLE.match(line)
        if not selected:
            if title and title.group(1) == name:
                if occurrence == ordinal:
                    selected = True
                    body.append(line)
                occurrence += 1
            continue
        if line.startswith("Disassembly of section "):
            break
        if title and title.group(1) in public_names:
            break
        body.append(line)
    if not selected:
        return None
    return "\n".join(body).rstrip() + "\n"


def objdump(obj, name: str, ordinal: int) -> str:
    """The named function's rows from *obj*, freshness-guarded when the
    object is a disposable normalized comparison copy."""
    if not obj.is_file():
        if NORMAL_BASE in obj.parents or NORMAL_TARGET in obj.parents:
            hint = "run `homm3 build` first (its normalize step writes it)"
        elif BASE in obj.parents:
            hint = "run `homm3 build` first"
        else:
            hint = "run `homm3 delink` first"
        die(f"{obj.relative_to(common.HOMM3_DIR)} missing - {hint}")
    if NORMAL_BASE in obj.parents or NORMAL_TARGET in obj.parents:
        from homm3.build.normalized_freshness import freshness_problems
        problems = freshness_problems(obj)
        if problems:
            die("stale normalized comparison object (re-run without "
                "--no-build to refresh this unit, or `homm3 build`):\n  "
                + "\n  ".join(problems[:5]))
    res = subprocess.run(
        ["llvm-objdump", "-dr", "--x86-asm-syntax=intel", str(obj)],
        capture_output=True, text=True)
    if res.returncode != 0:
        die(f"llvm-objdump failed on {obj.name}:\n{res.stderr.strip()}")
    body = _slice_public_symbol(
        res.stdout, name, ordinal, _public_text_symbols(obj))
    if body is None:
        die(f"symbol {name} not found in {obj.relative_to(common.HOMM3_DIR)}")
    return body


# --- the unit refresh: compile + normalize what a diff is about to read -------------

REFRESH_LOCK = common.HOMM3_DIR / "build/.sema-refresh.lock"
NINJA_FILE = common.HOMM3_DIR / "build.ninja"


def refresh_unit(unit: str, *, run=subprocess.run) -> str | None:
    """Bring ONE unit's comparison copies up to date before a diff: the
    unit's ninja target (0.01 s when nothing changed, one VC6 compile
    otherwise), then its normalized copies, then the objdiff report when
    anything was rewritten. Returns a note describing what was rebuilt,
    or None when everything was already fresh. Serialized through a
    file lock: agents fire many sema calls per second and two ninja runs
    in one build directory would collide. A compile error dies with the
    compiler's output - a diff against a broken object answers nothing."""
    import fcntl
    import time
    if not NINJA_FILE.is_file() or not unit:
        return None
    target = f"build/objdiff/base/{unit}.obj"
    started = time.time()
    REFRESH_LOCK.parent.mkdir(parents=True, exist_ok=True)
    with REFRESH_LOCK.open("w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        res = run(["ninja", "-f", str(NINJA_FILE), target],
                  cwd=common.HOMM3_DIR, capture_output=True, text=True)
        if res.returncode != 0:
            tail = "\n".join((res.stdout + res.stderr).strip().splitlines()[-25:])
            die(f"{unit} does not compile - fix the source (or --no-build to "
                f"diff the last built object):\n{tail}")
        compiled = "no work to do" not in res.stdout
        from homm3.build import normalize_objs
        counts = normalize_objs.normalize_unit(unit)
        if not compiled and not counts["wrote"]:
            return None
        report = run(["objdiff-cli", "report", "generate", "-o", "report.json"],
                     cwd=common.HOMM3_DIR / "build/objdiff",
                     capture_output=True, text=True)
    did = []
    if compiled:
        did.append("compiled")
    if counts["wrote"]:
        did.append("normalized")
    if report.returncode == 0:
        did.append("report")
    return (f"[refreshed {unit}: {' + '.join(did)} in {time.time() - started:.1f}s; "
            "--no-build compares the last built object]")


# --- producer 2: capstone over the retail image ------------------------------------

def image_text(ctx, rva: int, size: int, name: str) -> str:
    """Disassemble [rva, rva+size) straight from the gated image, in the
    same row shape objdump() yields. Addresses are VAs (the image is
    fixed-base). Direct call/jmp targets and dir32 reloc operands are
    annotated `<symbol>` from the symbol db - annotations the maskers
    strip, so the CFG/diff machinery sees only real content."""
    try:
        import capstone
    except ImportError:
        die("capstone is not importable - run inside `nix develop .#build`")
    image = ctx.image
    section = image.section_of(rva)
    if section is None or not section.executable:
        die(f"0x{rva:x} is not in an executable section")
    offset = section.raw_offset + (rva - section.rva)
    # Clamp to the section's raw-backed extent: a span reaching past it
    # would silently decode the NEXT section's file bytes as code.
    size = min(size, section.size - (rva - section.rva))
    code = image.data[offset:offset + size]
    va = image.image_base + rva

    sites = ctx.relocs  # [(site_rva, operand_va)] sorted by site
    lo = bisect.bisect_left(sites, (rva, 0))
    hi = bisect.bisect_left(sites, (rva + size, 0))
    span_sites = sites[lo:hi]

    def symbolize(value_va: int) -> str | None:
        if not image.in_image(value_va):
            return None
        target = image.rva_of(value_va)
        row = ctx.symbols.funcs.get(target) or ctx.symbols.datas.get(target)
        return row[0] if row else None

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    lines = [f"{va:08x} <{name}>:"]
    consumed = 0
    si = 0
    for ins in md.disasm(code, va):
        consumed = ins.address + ins.size - va
        notes = []
        mn = ins.mnemonic
        direct_flow = (mn in ("call", "jmp") or mn.startswith("loop")
                       or re.fullmatch(r"j[a-z]{1,4}", mn) is not None)
        if direct_flow and ins.op_str.startswith("0x"):
            sym = symbolize(int(ins.op_str, 16))
            if sym:
                notes.append(sym)
        ins_rva = ins.address - image.image_base
        while si < len(span_sites) and span_sites[si][0] < ins_rva:
            si += 1
        j = si
        while j < len(span_sites) and span_sites[j][0] < ins_rva + ins.size:
            sym = symbolize(span_sites[j][1])
            if sym and sym not in notes:
                notes.append(sym)
            j += 1
        annot = "".join(f" <{n}>" for n in notes)
        byte_column = " ".join(f"{b:02x}" for b in ins.bytes)
        lines.append(f"{ins.address:8x}: {byte_column}\t{ins.mnemonic}"
                     f"\t{ins.op_str}{annot}")
    if consumed < size:
        lines.append(f"[decode stopped at +0x{consumed:x} - {size - consumed} "
                     "byte(s) undecoded (jump-table data in the span?)]")
    return "\n".join(lines) + "\n"


# --- row parsing + renderers (ported) ----------------------------------------------

_HEAD = re.compile(r"^\s*([0-9a-f]+):\s*(?:[0-9a-f]{2} ?)*\s*$")


def parse_ins(ln: str):
    """(code_offset, 'mnemonic operands') for an instruction row, else None."""
    if "\t" not in ln:
        return None
    parts = ln.split("\t")
    if not _HEAD.match(parts[0]):
        return None
    off = int(parts[0].split(":", 1)[0].strip(), 16)
    body = [part.strip() for part in parts[1:] if part.strip()]
    if body and re.fullmatch(r"(?:[0-9a-fA-F]{2}\s*)+", body[0]):
        body.pop(0)
    return (off, " ".join(body)) if body else None


_DIR32_RELOC = re.compile(
    r"^\s*([0-9a-fA-F]+):\s+IMAGE_REL_I386_DIR32\b")


def code_insns(text: str) -> list[tuple[int, str]]:
    """Return instruction rows, excluding a trailing in-``.text`` data pool.

    VC6 appends switch pointer tables and byte lookup tables to a function's
    COMDAT. ``llvm-objdump`` linearly decodes those bytes because COFF has no
    code/data boundary inside the section. A DIR32 relocation whose site is
    exactly an alleged instruction address cannot belong to an x86 operand
    (the opcode occupies that byte); it is a relocated data word. The final
    real return immediately before the first such word is therefore the
    compiler-published code boundary. Keep the return, and discard alignment
    bytes plus the pool.
    """
    insns = [parsed for line in text.splitlines()
             if (parsed := parse_ins(line)) is not None]
    if not insns:
        return []

    addresses = {offset for offset, _instruction in insns}
    pool_sites = []
    for line in text.splitlines():
        match = _DIR32_RELOC.match(line)
        if match:
            site = int(match.group(1), 16)
            if site in addresses:
                pool_sites.append(site)
    if not pool_sites:
        return insns

    first_pool_word = min(pool_sites)
    last_return = None
    for index, (offset, instruction) in enumerate(insns):
        if offset >= first_pool_word:
            break
        mnemonic = instruction.lower().split(None, 1)[0]
        if mnemonic.startswith("ret"):
            last_return = index
    if last_return is not None:
        return insns[:last_return + 1]
    return [(offset, instruction) for offset, instruction in insns
            if offset < first_pool_word]


_ROW_HEAD = re.compile(
    r"^\s*([0-9a-fA-F]+):\s*((?:[0-9a-fA-F]{2}\s?)*)\s*$")
_RELOC_ROW = re.compile(
    r"^\s*([0-9a-fA-F]+):\s+IMAGE_REL_I386_([A-Z0-9_]+)\s+(\S.*?)\s*$")
_NOTE = re.compile(r"\s*<([^>]*)>")
_FLOW = re.compile(r"^(?:call|jmp|j[a-z]{1,4}|loop\w*)$")


def _parse_row(ln: str):
    """(offset, raw bytes, 'mnemonic operands') for an instruction row."""
    if "\t" not in ln:
        return None
    head, *rest = ln.split("\t")
    match = _ROW_HEAD.match(head)
    if not match:
        return None
    raw = bytes.fromhex(re.sub(r"\s", "", match.group(2)))
    body = [part.strip() for part in rest if part.strip()]
    if body and re.fullmatch(r"(?:[0-9a-fA-F]{2}\s*)+", body[0]):
        body.pop(0)
    if not body:
        return None
    return int(match.group(1), 16), raw, " ".join(body)


def _fold_reloc(mnemonic: str, operands: str, kind: str, symbol: str,
                raw: bytes, field: int):
    """Fold one relocation into *operands*; (operands, note) where the
    note carries the symbol whenever the relocated field cannot be
    located in the operand text (a data word decoded as code, an
    ambiguous literal, an unexpected shape)."""
    if kind == "REL32":
        if _FLOW.match(mnemonic) and re.fullmatch(r"0x[0-9a-fA-F]+", operands):
            return symbol, None
        return operands, symbol
    if kind != "DIR32" or field <= 0 or field + 4 > len(raw):
        # field 0 = the opcode byte: a relocated pool word, not an operand
        return operands, symbol
    value = int.from_bytes(raw[field:field + 4], "little")
    signed = value - (1 << 32) if value & 0x80000000 else value
    if value == 0:
        target, pattern = symbol, r"0x0\b"
    elif signed < 0:
        target, pattern = f"{symbol}-0x{-signed:x}", rf"-\s?0x{-signed:x}\b"
    else:
        target, pattern = f"{symbol}+0x{value:x}", rf"0x{value:x}\b"
    hits = list(re.finditer(pattern, operands))
    if signed < 0 and not hits:
        hits = list(re.finditer(rf"0x{value:x}\b", operands))
    if len(hits) == 1:
        hit = hits[0]
        before = operands[:hit.start()]
        inside = before.count("[") > before.count("]")
        if inside:
            spelled = f"+ {target}" if hit.group(0).startswith("-") else target
        else:
            spelled = f"offset {target}"
        return before + spelled + operands[hit.end():], None
    if not hits and value == 0:
        close = operands.rfind("]")
        if close > 0:  # SIB form with the zero displacement elided
            return operands[:close] + f" + {symbol}" + operands[close:], None
    return operands, symbol


def _scan_rows(text: str) -> list:
    """Listing rows in order: ``("title", line, addr|None)`` for symbol
    titles and notices, ``("ins", (offset, raw, body), [(site, kind,
    symbol), ...])`` for instructions - a reloc row attaches to the
    instruction row that precedes it."""
    rows: list = []
    for ln in text.splitlines():
        title = _SYMBOL_TITLE.match(ln)
        if title:
            rows.append(("title", ln.strip(), int(ln.split(None, 1)[0], 16)))
            continue
        if ln.startswith("[decode stopped"):
            rows.append(("title", ln, None))
            continue
        parsed = _parse_row(ln)
        if parsed:
            rows.append(("ins", parsed, []))
            continue
        reloc = _RELOC_ROW.match(ln)
        if reloc and rows and rows[-1][0] == "ins":
            rows[-1][2].append((int(reloc.group(1), 16), reloc.group(2),
                                reloc.group(3)))
    return rows


def reloc_rows(text: str) -> list[tuple[int, bytes, str, list]]:
    """``(offset, raw bytes, 'mnemonic operands', [(site, kind, symbol)])``
    per instruction row of an objdump/image listing, in order; the one
    parser every reloc-aware view shares (the default disasm listing, the
    call/reloc sequence diffs, the first-divergence walk). Titles are
    dropped; the trailing pool is NOT cut here - callers keep only the
    `code_insns` offsets when they want code."""
    return [(off, raw, body, relocs)
            for kind, (off, raw, body), relocs in
            ((row[0], row[1], row[2]) for row in _scan_rows(text)
             if row[0] == "ins")]


def parse_local_range(spec: str) -> tuple[int | None, int | None]:
    """Parse an end-exclusive function-local ``START:END`` range.

    Either endpoint may be omitted.  Offsets accept the same decimal/0x
    spelling as the CLI's addresses; a leading ``+`` is deliberately
    accepted because disassembly offsets are normally written ``+0xc00``.
    """
    if spec.count(":") != 1:
        raise ValueError("expected START:END (end exclusive)")
    words = spec.split(":")

    def endpoint(word: str) -> int | None:
        word = word.strip()
        if not word:
            return None
        try:
            value = int(word, 0)
        except ValueError as exc:
            raise ValueError(f"invalid offset {word!r}") from exc
        if value < 0:
            raise ValueError("offsets cannot be negative")
        return value

    start, end = map(endpoint, words)
    if start is None and end is None:
        raise ValueError("range must have at least one endpoint")
    if start is not None and end is not None and start >= end:
        raise ValueError("range START must be less than END")
    return start, end


def slice_local_range(text: str, span: tuple[int | None, int | None]) -> str:
    """Keep instructions in one function-local range and their reloc rows.

    Obj sections do not promise that a selected function begins at address
    zero, so endpoints are added to the first real instruction.  Branches
    that leave the slice intentionally become ``<ext>`` in ``cfg``: for a
    switch arm that is the useful boundary, not missing context.
    """
    instructions = code_insns(text)
    if not instructions:
        raise ValueError("listing has no code instructions")
    origin = instructions[0][0]
    start = origin + (span[0] or 0)
    end = origin + span[1] if span[1] is not None else None
    out = [f"{start:08x} <selected-range>:"]
    keep_previous = False
    kept = 0
    for line in text.splitlines():
        parsed = _parse_row(line)
        if parsed is not None:
            offset = parsed[0]
            keep_previous = offset >= start and (end is None or offset < end)
            if keep_previous:
                out.append(line)
                kept += 1
            continue
        if _RELOC_ROW.match(line):
            if keep_previous:
                out.append(line)
            continue
        # Private labels make raw listings easier to read.  Keep only labels
        # whose address belongs to the selected interval; the synthetic title
        # above remains the sole public/function title.
        title = _SYMBOL_TITLE.match(line)
        if title:
            try:
                address = int(line.split(None, 1)[0], 16)
            except (ValueError, IndexError):
                continue
            if address != start and address >= start and (end is None or address < end):
                out.append(line)
    if not kept:
        lo = f"+0x{span[0]:x}" if span[0] is not None else "start"
        hi = f"+0x{span[1]:x}" if span[1] is not None else "end"
        raise ValueError(f"no instructions in local range {lo}:{hi}")
    return "\n".join(out) + "\n"


def lite_rows(text: str) -> list[tuple[int | None, str]]:
    """The default listing as ``(offset, line)`` rows; titles and notices
    carry ``None``. What a matcher reads survives: the address column
    (branch targets stay locatable), every call/data symbol folded from
    its reloc row into the operand (``call ?f@@YAXXZ``, ``[gVar]``,
    ``push offset gVar``), cross-symbol ``<notes>``. What only encoding
    questions need is left to ``--verbose``: byte columns, raw reloc
    rows, and the ``<ownfn+0xNNN>`` notes the address column makes
    redundant."""
    rows = _scan_rows(text)
    titles = [row[1].split("<", 1)[1].rstrip(">:") for row in rows
              if row[0] == "title" and row[2] is not None]
    own = titles[0] if titles else None
    labels = set(titles)

    def keep_note(note: str) -> bool:
        root, _plus, rest = note.partition("+")
        if root == own:
            return not rest  # a bare self-reference is recursion; +off is body
        return root not in labels

    # A DIR32 site AT an instruction address is a relocated data word: the
    # trailing switch/lookup pool objdump decodes as code (see code_insns).
    # Render the pool as data rows instead of the garbage instructions.
    pool_start = min((site for row in rows if row[0] == "ins"
                      for site, kind, _sym in row[2]
                      if kind == "DIR32" and site == row[1][0]), default=None)

    offsets = [row[1][0] for row in rows if row[0] == "ins"]
    width = max((len(f"{off:x}") for off in offsets), default=1)
    out: list[tuple[int | None, str]] = []
    pool: list = []  # (offset, raw, relocs) rows from pool_start on
    pool_titles: list = []  # label titles inside the pool, by address
    for row in rows:
        if row[0] == "title":
            if (pool_start is not None and row[2] is not None
                    and row[2] >= pool_start):
                pool_titles.append((row[2], row[1]))
            else:
                out.append((None, row[1]))
            continue
        (off, raw, body), relocs = row[1], row[2]
        if pool_start is not None and off >= pool_start:
            pool.append((off, raw, relocs))
            continue
        notes = [n for n in _NOTE.findall(body) if keep_note(n)]
        stripped = _NOTE.sub("", body).strip() or body.strip()
        mnemonic, _sp, operands = stripped.partition(" ")
        for site, kind, symbol in relocs:
            operands, note = _fold_reloc(mnemonic, operands, kind, symbol,
                                         raw, site - off)
            if note and note not in notes:
                notes.append(note)
        text_row = f"{mnemonic} {operands}".strip()
        text_row += "".join(f" <{n}>" for n in notes)
        out.append((off, f"  {off:>{width}x}: {text_row}"))
    merged = ([(addr, 0, None, line) for addr, line in pool_titles]
              + [(off, 1, off, line) for off, line in _pool_rows(pool, width)])
    out.extend((off, line) for _addr, _rank, off, line in sorted(
        merged, key=lambda item: (item[0], item[1])))
    return out


def _pool_rows(pool, width: int) -> list[tuple[int, str]]:
    """``dd symbol[+addend]`` per relocated word, ``db ..`` for the bytes
    between them (byte lookup tables, alignment)."""
    if not pool:
        return []
    base = pool[0][0]
    data = bytearray()
    sites = {}
    for off, raw, relocs in pool:
        if off - base > len(data):  # objdump skipped bytes; keep alignment
            data.extend(b"\0" * (off - base - len(data)))
        data[off - base:off - base + len(raw)] = raw
        for site, kind, symbol in relocs:
            if kind == "DIR32":
                sites[site] = symbol
    out = []
    at = 0
    plain: list[int] = []

    def flush(start: int):
        if plain:
            chunk = " ".join(f"{b:02x}" for b in plain)
            out.append((start, f"  {start:>{width}x}: db {chunk}"))
            plain.clear()

    plain_start = base
    while at < len(data):
        site = base + at
        if site in sites and at + 4 <= len(data):
            flush(plain_start)
            value = int.from_bytes(data[at:at + 4], "little")
            addend = f"+0x{value:x}" if value else ""
            out.append((site, f"  {site:>{width}x}: dd {sites[site]}{addend}"))
            at += 4
            plain_start = base + at
            continue
        plain.append(data[at])
        at += 1
        if len(plain) == 16:
            flush(plain_start)
            plain_start = base + at
    flush(plain_start)
    return out


def lite(text: str) -> str:
    """The default `disasm` listing - see lite_rows()."""
    return "\n".join(line for _off, line in lite_rows(text)) + "\n"


def norm(text: str) -> list:
    """Instruction stream for the flat asm diff: mnemonic+operands only,
    every absolute address masked. Both sides come from the same
    llvm-objdump so only real byte-level diffs survive."""
    out = []
    for ln in text.splitlines():
        p = parse_ins(ln)
        if p:
            ins = p[1].lower()
            ins = re.sub(r"\s*<[^>]*>", "", ins)          # drop <sym> notes
            ins = re.sub(r"0x[0-9a-f]+", "<addr>", ins)   # mask addrs/disps
            out.append(ins)
        elif "IMAGE_REL_I386_" in ln:
            # Keep the reloc target symbol: a retargeted reloc is a real
            # diff. CAVEAT (differs from the homm2 original, where both
            # sides carried CodeView names): here the base names data by
            # source symbol and the delinked target by synthetic label
            # (data_<rva>, sometimes owner+addend), so --asm shows
            # reloc-name diffs objdiff proves equivalent - see diff.py.
            out.append("reloc " + ln.split("IMAGE_REL_I386_")[1].strip())
    while out and out[-1] == "nop":
        out.pop()  # trailing COMDAT alignment padding (base only)
    return out


_JCC = {
    "jmp", "je", "jne", "jz", "jnz", "ja", "jae", "jb", "jbe", "jc", "jnc",
    "jg", "jge", "jl", "jle", "js", "jns", "jo", "jno", "jp", "jnp",
    "jcxz", "jecxz",
}


def _branch_target(text: str, addresses: set) -> int | None:
    fields = text.lower().split(None, 1)
    if len(fields) != 2:
        return None
    op, operand = fields
    if op not in _JCC and not op.startswith("loop"):
        return None
    match = re.match(r"0x([0-9a-f]+)\b", operand)
    if not match:
        return None
    target = int(match.group(1), 16)
    return target if target in addresses else None


def mask_insn(text: str) -> str:
    """Normalize one instruction without hiding stack slots or constants."""
    text = re.sub(r"\s+", " ", text.strip().lower())
    text = re.sub(r"\s*<[^>]*>", "", text)
    text = re.sub(r"\[\s*0x[0-9a-f]+\s*\]", "[<addr>]", text)
    text = re.sub(r"^((?:j[a-z]{1,4}|call|loop\w*)\s+)0x[0-9a-f]+\b.*$",
                  r"\1<tgt>", text)
    return text


def cfg_rows(text: str):
    """Ordered blocks as ``(address, [(insn_addr, masked)], terminator)``.

    Terminators use block indices rather than code addresses, making
    normalized candidate and retail objects comparable. The block count
    is diagnostic, not authoritative: MSVC TU state can split or merge
    blocks without any source change."""
    insns = code_insns(text)
    while insns and insns[-1][1].lower() == "nop":
        insns.pop()
    if not insns:
        return []

    addresses = {address for address, _ in insns}
    leaders = {insns[0][0]}
    for i, (address, instruction) in enumerate(insns):
        op = instruction.lower().split(None, 1)[0]
        target = _branch_target(instruction, addresses)
        following = insns[i + 1][0] if i + 1 < len(insns) else None
        if target is not None:
            leaders.add(target)
            # The byte after every branch starts a new block even when an
            # unconditional jump makes it unreachable; otherwise alignment
            # bytes get folded into the jump block and eat its terminator.
            if following is not None:
                leaders.add(following)
        elif (op == "jmp" or op.startswith("ret") or op in _JCC
              or op.startswith("loop")) and following is not None:
            leaders.add(following)

    order = sorted(leaders)
    index = {address: i for i, address in enumerate(order)}

    def block_of(address: int) -> int:
        return bisect.bisect_right(order, address) - 1

    result = [[address, [], None] for address in order]
    for i, (address, instruction) in enumerate(insns):
        block = result[block_of(address)]
        op = instruction.lower().split(None, 1)[0]
        target = _branch_target(instruction, addresses)
        following = insns[i + 1][0] if i + 1 < len(insns) else None
        block[1].append((address, mask_insn(instruction)))
        if following is not None and following not in index:
            continue
        if target is not None:
            destination = f"B{block_of(target)}" + ("^" if target <= address else "")
            if op == "jmp":
                block[2] = f"jmp {destination}"
            elif following is not None:
                block[2] = f"jcc {destination} | fall B{block_of(following)}"
            else:
                block[2] = f"jcc {destination}"
        elif op.startswith("ret"):
            block[2] = "ret"
        elif op == "jmp":
            block[2] = "jmp <ext>"
        elif op in _JCC or op.startswith("loop"):
            block[2] = (f"jcc <ext> | fall B{block_of(following)}"
                        if following is not None else "jcc <ext>")
        elif following is not None:
            block[2] = f"fall B{block_of(following)}"
        else:
            block[2] = "end"
    return [(address, body, term) for address, body, term in result]


def cfg(text: str):
    """Ordered basic blocks as (address, [masked insns], terminator)."""
    return [(address, [instruction for _off, instruction in body], term)
            for address, body, term in cfg_rows(text)]


def predecessor_counts(graph) -> dict:
    counts = {}
    for _, _, term in graph:
        for match in re.finditer(r"B(\d+)", term or ""):
            target = int(match.group(1))
            counts[target] = counts.get(target, 0) + 1
    return counts


def blocks(text: str, skeleton: bool = True) -> str:
    """Render one side's CFG: one line per block (skeleton, the default)
    or with masked instruction bodies."""
    graph = cfg(text)
    if not graph:
        return "(no instruction rows found)\n"
    predecessors = predecessor_counts(graph)
    out = []
    for i, (address, body, term) in enumerate(graph):
        tail = "  <== shared tail" if term == "ret" and predecessors.get(i, 0) > 2 else ""
        loop = "  LOOP" if "^" in (term or "") else ""
        if skeleton:
            first = body[0].split(None, 1)[0] if body else "?"
            out.append(f"  B{i:<3} @{address:<8x} {len(body):>3}i  "
                       f"[{term}]{loop}{tail}  ({first}..)")
            continue
        out.append("")
        out.append(f"block B{i} @{address:x}: {len(body)} instruction(s)  "
                   f"[{term}]{loop}{tail}")
        out.extend(f"    {instruction}" for instruction in body)
    return "\n".join(out).lstrip("\n") + "\n"


def branch_kind(term: str | None, at: int) -> str:
    parts = []
    for match in re.finditer(r"(jcc|jmp|ret|fall|end)(?: B(\d+)(\^?))?", term or ""):
        op, target, back = match.groups()
        if target is None:
            parts.append(op)
        else:
            direction = "^" if back else ">" if int(target) > at else "<"
            parts.append(op + direction)
    return " ".join(parts)


def skeleton_census(base_cfg, target_cfg) -> dict:
    """The block-skeleton comparison as data: ``blocks`` (nb, nt), the
    ``exact``/``size``/``shift``/``flow``/``missing`` counts, ``rows`` =
    [(i, base_summary, flow_mark, size_mark, target_summary)],
    ``first_flow`` = (i, base_kind, target_kind) | None, ``first_differs``
    = (i, kind) | None for the first non-exact block, and ``same``."""
    same = len(base_cfg) == len(target_cfg)
    counts = {"exact": 0, "size": 0, "shift": 0, "flow": 0, "missing": 0}
    first_flow = None
    first_differs = None
    rows = []

    def mnemonic(line):
        """A blank line inside a block (jump-table padding renders as
        one) has no mnemonic - render it rather than crashing."""
        parts = line.split(None, 1)
        return parts[0] if parts else "?"

    def summary(graph, i):
        if i >= len(graph):
            return "-"
        _, body, term = graph[i]
        first = mnemonic(body[0]) if body else "?"
        last = mnemonic(body[-1]) if body else "?"
        return f"{len(body):>3}i {first}..{last} [{term}]"

    for i in range(max(len(base_cfg), len(target_cfg))):
        base, target = summary(base_cfg, i), summary(target_cfg, i)
        if i >= len(base_cfg) or i >= len(target_cfg):
            flow_mark, size_mark = "--", "--"
            counts["missing"] += 1
            same = False
            if first_differs is None:
                first_differs = (i, "missing")
        else:
            base_term, target_term = base_cfg[i][2], target_cfg[i][2]
            base_kind = branch_kind(base_term, i)
            target_kind = branch_kind(target_term, i)
            if base_term == target_term:
                flow_mark = "=="
            elif base_kind == target_kind:
                flow_mark = "~="
                counts["shift"] += 1
            else:
                flow_mark = "!!"
                counts["flow"] += 1
                if first_flow is None:
                    first_flow = (i, base_kind, target_kind)
            size_mark = "==" if len(base_cfg[i][1]) == len(target_cfg[i][1]) else "##"
            if flow_mark == "==" and size_mark == "==":
                counts["exact"] += 1
            elif flow_mark == "==" and size_mark == "##":
                counts["size"] += 1
            same &= flow_mark == "==" and size_mark == "=="
            if first_differs is None and not (flow_mark == "==" and size_mark == "=="):
                first_differs = (i, "flow" if flow_mark == "!!" else
                                 "shift" if flow_mark == "~=" else "size")
        rows.append((i, base, flow_mark, size_mark, target))
    return {"blocks": (len(base_cfg), len(target_cfg)), **counts,
            "rows": rows, "first_flow": first_flow,
            "first_differs": first_differs, "same": bool(same)}


def skeleton_diff(base_cfg, target_cfg) -> tuple:
    """Side-by-side block sizes/terminators with separate flow/size marks."""
    census = skeleton_census(base_cfg, target_cfg)
    out = [census_line(census), f"       {'BASE':48} FLOW SIZE TARGET"]
    for i, base, flow_mark, size_mark, target in census["rows"]:
        out.append(f"  B{i:<3} {base:48}  {flow_mark:^4} {size_mark:^4} {target}")
    out.append("[legend: FLOW == exact, ~= same branch kind/direction with "
               "shifted block target, !! branch-kind mismatch; SIZE ## differs]")
    if census["first_flow"] is not None:
        i, base_kind, target_kind = census["first_flow"]
        out.append(f"[first branch-kind divergence B{i}: base [{base_kind}] vs "
                   f"target [{target_kind}]]")
    return "\n".join(out) + "\n", census["same"]


def census_line(census: dict) -> str:
    nb, nt = census["blocks"]
    return (f"[skeleton diff: base {nb} vs target {nt} blocks; "
            f"{census['exact']} exact, {census['size']} size-only, "
            f"{census['shift']} target-shift, {census['flow']} flow-kind, "
            f"{census['missing']} missing]")


def blocks_diff(base_text: str, target_text: str) -> tuple:
    """Block-aligned body diff; exact only when all normalized blocks align."""
    import difflib
    base_cfg, target_cfg = cfg(base_text), cfg(target_text)
    base_rows = ["\n".join(body + [term or ""]) for _, body, term in base_cfg]
    target_rows = ["\n".join(body + [term or ""]) for _, body, term in target_cfg]
    base_flow = [term or "" for _, _, term in base_cfg]
    target_flow = [term or "" for _, _, term in target_cfg]
    out = [f"[block diff: base {len(base_cfg)} blocks vs target "
           f"{len(target_cfg)} blocks; flow "
           f"{'SAME' if base_flow == target_flow else 'DIFFERS'}]"]

    if base_flow != target_flow:
        base_kinds = [branch_kind(term, i) for i, term in enumerate(base_flow)]
        target_kinds = [branch_kind(term, i) for i, term in enumerate(target_flow)]
        first = next((i for i, pair in enumerate(zip(base_kinds, target_kinds))
                      if pair[0] != pair[1]), None)
        if first is not None:
            out.append(f"[skeleton diverges at B{first}: base "
                       f"[{base_flow[first]}] vs target [{target_flow[first]}]]")
        elif len(base_kinds) != len(target_kinds):
            out.append(f"[same branch-kind skeleton for "
                       f"{min(len(base_kinds), len(target_kinds))} shared "
                       f"blocks; {abs(len(base_kinds) - len(target_kinds))} "
                       "extra block(s)]")
        else:
            out.append("[same branch-kind skeleton; block-index targets differ]")

    matcher = difflib.SequenceMatcher(a=base_rows, b=target_rows, autojunk=False)
    differences = 0
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            for offset in range(i2 - i1):
                bi, ti = i1 + offset, j1 + offset
                out.append(f"  B{ti} @{base_cfg[bi][0]:x}/@{target_cfg[ti][0]:x}"
                           f"  == ({len(target_cfg[ti][1])} insns)  "
                           f"[{target_cfg[ti][2]}]")
            continue
        for offset in range(max(i2 - i1, j2 - j1)):
            bi = i1 + offset if i1 + offset < i2 else None
            ti = j1 + offset if j1 + offset < j2 else None
            differences += 1
            if bi is not None and ti is not None:
                out.append(f"  B{ti} @{base_cfg[bi][0]:x}/"
                           f"@{target_cfg[ti][0]:x}  DIFFERS:")
                diff = difflib.unified_diff(
                    base_cfg[bi][1] + [base_cfg[bi][2] or ""],
                    target_cfg[ti][1] + [target_cfg[ti][2] or ""],
                    lineterm="", n=2)
                out.extend("      " + line for line in diff
                           if not line.startswith(("---", "+++", "@@")))
            elif bi is not None:
                out.append(f"  -- @{base_cfg[bi][0]:x}  BASE-ONLY "
                           f"({len(base_cfg[bi][1])} insns) [{base_cfg[bi][2]}]")
            else:
                out.append(f"  B{ti} @{target_cfg[ti][0]:x}  TARGET-ONLY "
                           f"({len(target_cfg[ti][1])} insns) "
                           f"[{target_cfg[ti][2]}]")
    out.append(f"[{differences} block(s) differ]" if differences
               else "[all aligned blocks identical]")
    return "\n".join(out) + "\n", differences == 0 and base_flow == target_flow
