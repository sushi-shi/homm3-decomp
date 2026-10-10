"""homm3.vc6.il_cost - the front end's inline cost ``cb``, read from the IL.

C2's /Ob2 inliner compares a callee's ``cb`` (symbol ``+0x6d``) with the
caller's budget and sets the caller's budget to ``clamp(2 * cb, 1000,
35000)`` (docs/vc6/inliner.md section 2). C1XX computes that number, and it
is simple: **cb is the number of IL tuples C1XX emitted for the function's
body.** The pinned C1XX.DLL 12.00.8472 (image base 0x10400000) shows it:

* ``emitTuple`` (0x17042) writes one tuple to the ex stream and increments
  the per-function counter ``0x104dde7c`` (0x17082..0x17089). It is the only
  incrementer.
* The body-start routine (0x17b66) zeroes the counter and emits the
  function's leading info tuples; deferred bodies save it (0x1a0e1) and
  restore it (0x3f365) around their gl record.
* The gl symbol writer stores ``min(counter, 0xffff)`` as the record's cost
  (0x7c9f4, cold half of 0x2aef). C2 reads that field into ``+0x6d`` at
  0x1d23d.
* The same writer sets flag 0x40 ("body saved", C2's inline-candidate bit)
  for auto-inline functions only while ``counter < [0x104d2d24]`` (0x7cafe;
  the dword is 175 in the image). Functions declared inline, defined in
  their class, template instances and file-``static`` functions are saved
  at any size.

Info tuples (opcode 0x4f: line numbers, source-file switches, pragma and
warning state) are written by a different routine (0x7bfc) and do not
count. This module parses the ex stream with the tuple formats read from
the pinned C1XX tables (opcode formats at 0xbeaac, info formats at 0xbd9e8),
splits it into function bodies at the body-start info tuple, and attributes
every counted tuple to its source line. ``check_record`` compares the count
with the gl cost of every recorded function; a parse that drifts by one
operand byte desynchronizes and fails loudly.

    homm3 vc6 cb UNIT|SOURCE [--fn TEXT] [--explain] [--tuples] [--json]
"""
from __future__ import annotations

from dataclasses import dataclass, field
import json
import re
from pathlib import Path
import struct

from homm3.vc6 import _common

IMAGE_BASE = 0x10400000
OP_FORMATS_RVA = 0xBEAAC
INFO_FORMATS_RVA = 0xBD9E8
SAVE_LIMIT_RVA = 0xD2D24
OP_COUNT = 0xB9
INFO_COUNT = 0x29

INFO_OP = 0x4F
INFO_LINE = 0x01
INFO_FILE = 0x02
INFO_BODY = 0x1F
WARNING_STATE = 0x23
END_OF_UNIT = 0x4D
SAVED = 0x40

# Roles observed in controlled harness compiles (docs/vc6/inliner.md
# section 4). Unlisted opcodes print in hex; the count never depends on
# a name.
ROLES = {
    0x02: "add", 0x03: "sub", 0x04: "mul", 0x0F: "opassign", 0x1A: "not",
    0x1F: "eq", 0x20: "ne", 0x22: "lt", 0x24: "gt", 0x26: "addr",
    0x27: "field", 0x28: "index", 0x29: "label", 0x2A: "thisparam",
    0x2B: "thisend", 0x2C: "convert", 0x2D: "param", 0x30: "load",
    0x32: "store", 0x33: "const", 0x35: "postinc", 0x38: "jfalse",
    0x39: "jtrue", 0x3A: "jump", 0x3B: "switch", 0x3C: "jumptable",
    0x3D: "case", 0x3E: "call", 0x41: "retval", 0x42: "select",
    0x43: "arms", 0x44: "seq", 0x46: "header", 0x47: "return", 0x4B: "discard",
    0x4C: "end", 0x4D: "endunit", 0x53: "open", 0x54: "close", 0x55: "arg",
    0x5A: "this", 0x5C: "dtor", 0x5D: "cleanup", 0x5E: "cleanup",
    0x99: "ptrcast", 0x9B: "temp",
}


class Desync(ValueError):
    """The tuple grammar did not account for a byte of the ex stream."""


@dataclass(frozen=True)
class Formats:
    ops: dict
    infos: dict
    save_limit: int = 175


_FORMATS: Formats | None = None


def pinned_formats() -> Formats:
    """Tuple operand formats and the save limit from the gated C1XX.DLL."""
    global _FORMATS
    if _FORMATS is None:
        from homm3.vc6 import _toolchain
        binary = _toolchain.Binary("C1XX.DLL")

        def table(rva: int, count: int) -> dict:
            base = binary.rva_to_off(rva)
            out = {}
            for index in range(count):
                pointer = struct.unpack_from("<I", binary.data, base + index * 8)[0]
                if pointer and binary.is_va(pointer):
                    start = binary.va_to_off(pointer)
                    out[index] = bytes(binary.data[start:binary.data.index(b"\0", start)])
                else:
                    out[index] = b""
            return out

        limit = struct.unpack_from("<I", binary.data, binary.rva_to_off(SAVE_LIMIT_RVA))[0]
        _FORMATS = Formats(table(OP_FORMATS_RVA, OP_COUNT),
                           table(INFO_FORMATS_RVA, INFO_COUNT), limit)
    return _FORMATS


class _Reader:
    """Operand decoders matching C1XX's writers (and C2's reader)."""

    def __init__(self, data: bytes, pos: int = 0, end: int | None = None):
        self.data, self.pos = data, pos
        self.end = len(data) if end is None else end

    def byte(self) -> int:
        if self.pos >= self.end:
            raise Desync(f"operand runs past the stream end at {self.pos:#x}")
        value = self.data[self.pos]
        self.pos += 1
        return value

    def u16(self) -> int:
        return self.byte() | self.byte() << 8

    def u32(self) -> int:
        return self.u16() | self.u16() << 16

    def signed32(self) -> int:
        """Byte below 0x80, else 0x80 and a dword (0x175c8 / 0x2373d)."""
        value = self.byte()
        if value == 0x80:
            return struct.unpack("<i", struct.pack("<I", self.u32()))[0]
        return value - 0x100 if value & 0x80 else value

    def signed16(self) -> int:
        """Byte below 0x80, else 0x80 and a word (0x7de03 / 0x748e)."""
        value = self.byte()
        if value == 0x80:
            return struct.unpack("<h", struct.pack("<H", self.u16()))[0]
        return value - 0x100 if value & 0x80 else value

    def handle(self) -> int:
        """Symbol handle (0x7fa2): u16, or a dword whose bit 15 marks it."""
        value = self.u16()
        if value & 0x8000:
            value = (value & 0x7FFF) | self.u16() << 15
        return value

    def type(self) -> tuple[int, int, int]:
        """Type operand (0x1ac94): (code, kind, size)."""
        first = self.byte()
        code = ((first & 0x7F) << 8 | self.byte()) if first & 0x80 else first
        kind, size = code & 0xF, (code >> 4) & 0xFF
        if kind == 6 and size == 0:
            size = self.signed32()
        return code, kind, size


@dataclass
class Tuple:
    offset: int
    op: int
    counted: bool
    operands: list

    def role(self) -> str:
        return ROLES.get(self.op, f"op{self.op:02x}")


def _constant(reader: _Reader, kind: int, size: int):
    if kind in (1, 2):                       # integral (0x1b8a7)
        first = reader.byte()
        if first != 0x80:
            return first - 0x100 if first & 0x80 else first
        value = reader.u32()
        for _ in range(max(0, size // 4 - 1)):
            reader.u32()
        return value
    if kind in (5, 10):                      # floating: 10-byte value + 2
        reader.pos += 12
        if reader.pos > reader.end:
            raise Desync("floating constant runs past the stream end")
        return "float"
    if kind in (3, 6, 7):                    # address-sized
        return reader.signed32()
    if kind == 0:
        return None
    raise Desync(f"constant of type kind {kind} at {reader.pos:#x}")


_SILENT = frozenset((0x04, 0x05, 0x09, 0x0C, 0x0F, 0x10, 0x12, 0x13,
                     0x19, 0x1B, 0x70))


def parse(ex: bytes, formats: Formats, start: int, end: int | None = None) -> list[Tuple]:
    """Every tuple of ``ex[start:end]`` in stream order."""
    reader = _Reader(ex, start, end)
    out = []
    while reader.pos < reader.end:
        at = reader.pos
        op = reader.byte()
        if op == INFO_OP:
            out.append(Tuple(at, op, False, _info(reader, formats, at)))
            continue
        if op not in formats.ops:
            raise Desync(f"unknown opcode {op:#04x} at {at:#x}")
        operands = []
        for char in formats.ops[op]:
            if char in (0x01, 0x03, 0x07, 0x0D, 0x0E):
                operands.append(reader.byte())
            elif char in (0x02, 0x14, 0x16):
                operands.append(reader.signed32())
            elif char == 0x06:
                operands.append(("type", reader.type()[0]))
            elif char in (0x08, 0x11):
                operands.append(("sym", reader.handle()))
            elif char == 0x0B:
                code, kind, size = reader.type()
                operands.append(("type", code))
                operands.append(_constant(reader, kind, size))
            elif char == 0x15:
                operands.append((reader.handle(), reader.handle(), reader.signed16()))
            elif char not in _SILENT:
                raise Desync(f"opcode {op:#04x} operand format {char:#04x} at {at:#x}")
        out.append(Tuple(at, op, True, operands))
    return out


def _info(reader: _Reader, formats: Formats, at: int) -> list:
    subtype = reader.byte()
    values = [subtype]
    if subtype == WARNING_STATE:             # (warning - 700, state)* then 80 ff ff
        while True:
            number = reader.signed16()
            if number == -1:
                return values
            values.append((number + 700, reader.byte()))
    if subtype not in formats.infos:
        raise Desync(f"unknown info tuple {subtype:#04x} at {at:#x}")
    for char in formats.infos[subtype]:
        if char == 0x14:
            values.append(reader.signed32())
        elif char in (0x01, 0x02, 0x0E, 0x15):
            values.append(reader.signed16())
        elif char == 0x03:
            values.append(reader.handle())
        elif char == 0x0D:
            values.append(reader.byte())
        elif char == 0x0C:                   # length-prefixed blob (inline asm)
            length = reader.signed16()
            reader.pos += length
        else:
            raise Desync(f"info tuple {subtype:#04x} format {char:#04x} at {at:#x}")
    return values


@dataclass
class Body:
    start: int
    tuples: list = field(default_factory=list)
    handle: int | None = None
    name: str | None = None
    record: dict | None = None

    @property
    def cb(self) -> int:
        return sum(t.counted for t in self.tuples)


def bodies(ex: bytes, formats: Formats) -> list[Body]:
    """Split the ex stream into function bodies at the body-start tuple."""
    if len(ex) < 6 or ex[0] != 0x5B or ex[1] != 0x80:
        raise Desync("ex stream lacks its leading 5b offset tuple")
    first = struct.unpack_from("<I", ex, 2)[0]
    if any(ex[6:first]):
        raise Desync("ex header padding is not zero")
    out: list[Body] = []
    tuples = parse(ex, formats, first)
    if tuples and tuples[-1].op == END_OF_UNIT:
        tuples = tuples[:-1]
    for item in tuples:
        if item.op == INFO_OP and item.operands[0] == INFO_BODY:
            out.append(Body(item.offset))
        if not out:
            if item.counted:
                raise Desync(f"counted tuple {item.op:#04x} before the first body")
            continue
        out[-1].tuples.append(item)
    for body in out:
        counted = [t for t in body.tuples if t.counted]
        if (len(counted) > 2 and counted[0].op == counted[1].op == 0x53
                and counted[2].op == 0x26):
            body.handle = counted[2].operands[0][1]
    return out


def gl_names(gl: bytes) -> dict[int, str]:
    """Handle -> name for gl records ``<handle> 00 <name> 00``.

    Handles use the 0x7fa2 encoding (u16, or a dword marked by bit 15).
    This is a lookup overlay for display; costs never come from it."""
    out = {}
    i, n = 3, len(gl)
    while i < n:
        if gl[i - 1] != 0 or not 0x21 <= gl[i] <= 0x7E:
            i += 1
            continue
        j = i
        while j < n and 0x20 <= gl[j] <= 0x7E:
            j += 1
        if j < n and gl[j] == 0:
            name = gl[i:j].decode("latin1")
            if i >= 6 and gl[i - 4] & 0x80:
                low, high = struct.unpack_from("<HH", gl, i - 5)
                out.setdefault((low & 0x7FFF) | high << 15, name)
            elif i >= 3:
                out.setdefault(struct.unpack_from("<H", gl, i - 3)[0], name)
        i = j + 1
    for match in _FILE_RECORD.finditer(gl):            # 12 <handle> <path>
        handle = match[1]
        if len(handle) == 4 and handle[1] & 0x80:
            low, high = struct.unpack("<HH", handle)
            out.setdefault((low & 0x7FFF) | high << 15, match[2].decode("latin1"))
        elif len(handle) == 2 and not handle[1] & 0x80:
            out.setdefault(struct.unpack("<H", handle)[0], match[2].decode("latin1"))
    return out


_FILE_RECORD = re.compile(rb"\x12(.[\x80-\xff]..|.[\x00-\x7f])([A-Za-z]:\\[ -~]*)\x00",
                          re.DOTALL)


def gl_record(gl: bytes, start: int) -> dict | None:
    """The gl function record whose EX field is ``start`` (C2 0x1ce0b order:
    EX, SY, cost, flags, formal count), or None when the body has none."""
    pattern = b"\x80" + struct.pack("<i", start)
    hits = []
    at = gl.find(pattern)
    while at >= 0:
        if not (at >= 5 and gl[at - 5] == 0x80):   # not the SY of another record
            hits.append(at)
        at = gl.find(pattern, at + 1)
    if len(hits) != 1:
        return None
    reader = _Reader(gl, hits[0])
    reader.signed32()
    sy = reader.signed32()
    cost = reader.signed16()
    flags = reader.u16()
    if flags & 0x8000:
        flags = (flags & 0x7FFF) | reader.u16() << 15
    return dict(offset=hits[0], sy=sy, cb=cost, flags=flags)


def functions(streams: dict[str, bytes], formats: Formats | None = None) -> list[Body]:
    """Every body of a capture with its gl record, name and cost."""
    formats = formats or pinned_formats()
    names = gl_names(streams["gl"])
    out = bodies(streams["ex"], formats)
    for body in out:
        body.record = gl_record(streams["gl"], body.start)
        body.name = names.get(body.handle) if body.handle is not None else None
    return out


def stored_cost(count: int) -> int:
    """The gl field for a tuple count: saturated at 0xffff, read signed."""
    value = min(count, 0xFFFF)
    return value - 0x10000 if value & 0x8000 else value


def check_records(found: list[Body]) -> dict:
    """Tuple count against the gl cost C2 reads, for every recorded body."""
    agree = [b for b in found if b.record and b.record["cb"] == stored_cost(b.cb)]
    differ = [b for b in found if b.record and b.record["cb"] != stored_cost(b.cb)]
    return dict(bodies=len(found), recorded=len(agree) + len(differ),
                agree=len(agree), differ=[(b.name, b.record["cb"], b.cb) for b in differ])


def lines(body: Body, files: dict[int, str]) -> list[dict]:
    """Counted tuples grouped by (file, line), in first-appearance order."""
    groups: dict[tuple, dict] = {}
    current_file, current_line = None, None
    for item in body.tuples:
        if item.op == INFO_OP:
            if item.operands[0] == INFO_LINE:
                current_line = item.operands[1]
            elif item.operands[0] == INFO_FILE:
                current_file = files.get(item.operands[1], f"file#{item.operands[1]:x}")
            continue
        key = (current_file, current_line)
        group = groups.setdefault(key, dict(file=current_file, line=current_line,
                                            cost=0, tuples=[]))
        group["cost"] += 1
        group["tuples"].append(item.role())
    return list(groups.values())


# ---------------------------------------------------------------------------
# capture and command line
# ---------------------------------------------------------------------------

def capture(source: Path, flags: list[str], output: Path) -> dict[str, bytes]:
    """Front-end-only capture of a source at its real path."""
    from homm3.vc6 import il
    from homm3.vc6.shim.build import _traceCapture
    il._gate_subjects()
    il._ensure_wine_env()
    output.mkdir(parents=True, exist_ok=True)
    return _traceCapture(source, flags, output)


def _resolve(target: str) -> tuple[Path, list[str], str]:
    from homm3.vc6 import _unit
    source = _unit.source_for_unit(target)
    unit = target if source is not None else None
    if source is None:
        source = Path(target).resolve()
        if not source.is_file():
            _common.die(f"{target}: neither a units.toml unit nor a source file")
        unit = _unit.unit_for_source(source)
    flags = _unit.flags_for_unit(unit or "game")
    if flags is None:
        _common.die(f"no flag profile for {target}")
    return source, flags, unit or source.stem


def _matches(body: Body, needle: str, demangled: dict) -> bool:
    if not needle:
        return True
    name = body.name or ""
    return needle in name or needle in demangled.get(name, "")


def _short(path: str | None) -> str:
    if not path:
        return "?"
    posix = path.replace("\\", "/")
    root = str(_common.REPO).rstrip("/") + "/"
    at = posix.find(root)
    return posix[at + len(root):] if at >= 0 else posix.rsplit("/", 1)[-1]


def _analyse(target: str):
    source, flags, label = _resolve(target)
    streams = capture(source, flags, _common.REPO / "build/vc6/il-cost" / label)
    found = functions(streams)
    return source, label, streams, found, check_records(found)


def run_verify(targets: list[str]) -> int:
    """Re-prove the tuple grammar: every recorded body's count equals its
    gl cost, over the named units (``all``: every C++ game-profile unit)."""
    from homm3.vc6 import _unit
    if targets == ["all"]:
        targets = [u["unit"] for u in _unit._manifest()["unit"]
                   if u.get("flags", "").startswith("game") and u["source"].endswith(".cpp")]
    failed = 0
    totals = dict(bodies=0, recorded=0, agree=0)
    for target in targets:
        try:
            gate = _analyse(target)[4]
        except Desync as exc:
            print(f"{target}: DESYNC {exc}")
            failed += 1
            continue
        for key in totals:
            totals[key] += gate[key]
        failed += bool(gate["differ"])
        print(f"{target}: {gate['agree']}/{gate['recorded']} recorded bodies agree; "
              f"{gate['bodies']} bodies" + (f"; DIFFER {gate['differ'][:3]}" if gate["differ"] else ""))
    print(f"total: {totals['agree']}/{totals['recorded']} recorded bodies agree over "
          f"{len(targets)} units ({totals['bodies']} bodies); {failed} failing units")
    return 1 if failed else 0


def run(args) -> int:
    from homm3.core import undname
    if args.verify:
        return run_verify(args.target)
    if len(args.target) != 1:
        _common.die("name one unit or source (several only with --verify)")
    try:
        source, label, streams, found, gate = _analyse(args.target[0])
    except Desync as exc:
        _common.die(f"ex stream did not parse: {exc}")
    if gate["differ"]:
        _common.die(f"tuple count disagrees with the gl cost for {gate['differ'][:5]}")
    files = gl_names(streams["gl"])
    demangled = undname.demangle([b.name for b in found if b.name])
    limit = pinned_formats().save_limit
    chosen = [b for b in found if _matches(b, args.fn or "", demangled)]
    if args.fn and not chosen:
        _common.die(f"no function body matches {args.fn!r} in {label}")
    if args.json:
        rows = []
        for body in chosen:
            rows.append(dict(name=body.name, demangled=demangled.get(body.name or ""),
                             handle=body.handle, cb=body.cb,
                             flags=body.record["flags"] if body.record else None,
                             saved=bool(body.record["flags"] & SAVED) if body.record else None,
                             lines=lines(body, files) if args.explain else None))
        print(json.dumps(dict(unit=label, source=str(source), save_limit=limit,
                              gate=gate, functions=rows), indent=1))
        return 0
    print(f"{label}: {gate['agree']}/{gate['recorded']} recorded bodies agree with "
          f"their gl cost; {len(found)} bodies; auto-inline save limit cb < {limit}")
    texts: dict[str, list[str]] = {}

    def snippet(path: str | None, line: int | None) -> str:
        short = _short(path)
        if short not in texts:
            local = _common.REPO / short
            if not local.is_file() and path and path[1:3] == ":\\":   # Wine's Z: is /
                local = Path(path[2:].replace("\\", "/"))
            texts[short] = (local.read_text(encoding="latin1").splitlines()
                            if local.is_file() else [])
        rows = texts[short]
        return rows[line - 1].strip() if line and 0 < line <= len(rows) else ""
    for body in chosen:
        record = body.record
        state = ("no gl record" if record is None else
                 f"flags {record['flags']:#x} {'saved' if record['flags'] & SAVED else 'NOT saved'}")
        name = demangled.get(body.name or "", body.name or f"handle {body.handle}")
        print(f"{body.cb:6d}  {name}  [{state}]")
        if not args.explain:
            continue
        for group in lines(body, files):
            text = snippet(group["file"], group["line"])
            tuples = f"  {' '.join(group['tuples'])}" if args.tuples else ""
            print(f"        {group['cost']:5d}  {_short(group['file'])}:{group['line']}  "
                  f"{text[:90]}{tuples}")
    return 0
