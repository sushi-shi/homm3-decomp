"""homm3.vc6._il - record-level reader for the C1XX->C2 intermediate language.

The IL is the multi-file handoff the front end writes and C2.DLL reads
(reader.c): for an ``-il <prefix>`` the driver's suffix table (CL.EXE .rdata
file 0x7510: ex/sy/gl/in/st/db) names the streams.  A C++ TU under the game
profile produces exactly four (byte evidence in docs/vc6/il-format.md):

    <prefix>in   function index - which functions pass 2 compiles, in order
    <prefix>gl   global symbol table (files, functions, data, literals)
    <prefix>sy   per-function local-symbol blocks, in ``in``-list order
    <prefix>ex   expression/tuple stream (one span per function)

Every stream refers to symbols by a numeric u16 HANDLE allocated by one
global creation counter in the front end (probe proof: a=0xd5, b=0xd6,
add=0xd7, source file=0xd8, block=0xd9 for ``int add(int a,int b)``).  The
``gl`` header carries the counter's high-water mark; an unused ``struct
probe_t { int a; };`` consumes 9 handles and shifts every later-created
symbol's handle by +9 across all four streams - the include-set-sensitivity
mechanism (behavior catalog C1) made visible.

Framing status (v1, evidence-bounded - see docs/vc6/il-format.md section 4):

  in  EXACT for the simple form.  Small TUs are a strict sequence of 3-byte
      records ``09 <u16 handle>``; ``serialize_in(parse_in(d)) == d`` or
      ``parse_in`` returns None (round-trip-checked on every use).  Rich TUs
      (initialize.cpp) interleave OTHER record types after the leading 09
      run (observed ``00 c7 04 00 02 cc 04 00 04 01 03 04 ...``) - that
      grammar is OPEN and parse_in honestly refuses it.
  gl  HEURISTIC OVERLAY.  Header magic + high-water u32 at offset 7; name
      records located by the ``<u16 handle> 00 <cstring>`` scan below; the
      function records' ex-stream offsets extracted by the observed
      ``04 00 00 00 80 <u32>`` marker.
  sy  HEURISTIC OVERLAY.  Same name scan; block spans split on the observed
      ``0d 02 06`` terminator.
  ex  OPAQUE bytes; per-function segmentation borrowed from gl's offsets.

The overlays only ANNOTATE a byte-level diff - the byte comparison is always
the verdict.  Do not extend the grammar without new probe evidence.
"""
from __future__ import annotations

import re
import struct

# Observed constant leader of every C++ gl stream from the pinned C1XX
# 12.00.8472 (probe.cpp, p2-p4, initialize.cpp - all begin with these seven
# bytes; the u32 that follows is the symbol-handle high-water mark).
GL_MAGIC = b"\x11\x86\x03\x96\xba\x30\x01"

IN_OPCODE = 0x09              # 'in' record: 09 <u16 fn handle>
SY_BLOCK_END = b"\x0d\x02\x06"   # observed local-block terminator in 'sy'
# Function records in 'gl' carry their ex-stream start as
# ``... 00 00 80 <u32 offset> ...`` (observed tails: plain /O2 probes
# ``01 05 04 00 00 00 80 <u32>``, game profile ``01 05 04 04 00 00 80
# <u32>``; data points 0x3f4 / 0x43d / 0x3f4 for add / sub / helper each
# match where that function's tuple bytes actually begin in 'ex').  The
# short needle is disambiguated by the (0, ex_size] plausibility gate on
# the u32 - observed false candidates like ``80 9c 00 48 00 00`` fail it.
FN_EXOFF_MARKER = b"\x00\x00\x80"

# A name candidate: printable, starting like an identifier / mangled name /
# path.  Deliberately permissive - the handle-plausibility gate and the
# 00-byte prefix rule do the real filtering, and names only annotate.
_NAME_RE = re.compile(r"[A-Za-z_?$@.\\/][ -~]*$")

STREAM_ORDER = ("in", "gl", "sy", "ex", "st", "db")


def u16(data: bytes, off: int) -> int:
    return data[off] | (data[off + 1] << 8)


# ---------------------------------------------------------------------------
# 'in' - exact framing with a round-trip bound
# ---------------------------------------------------------------------------

def parse_in(data: bytes) -> list[int] | None:
    """The 'in' stream as a list of function handles, or None if any byte
    deviates from the proven ``09 <u16>`` record shape."""
    if not data or len(data) % 3:
        return None
    out = []
    for i in range(0, len(data), 3):
        if data[i] != IN_OPCODE:
            return None
        out.append(u16(data, i + 1))
    return out


def serialize_in(handles: list[int]) -> bytes:
    return b"".join(bytes((IN_OPCODE, h & 0xFF, h >> 8)) for h in handles)


def roundtrip_in(data: bytes) -> bool:
    """The framing bound: parse + re-serialize must reproduce the bytes."""
    handles = parse_in(data)
    return handles is not None and serialize_in(handles) == data


# ---------------------------------------------------------------------------
# 'gl' - header + name-record scan (heuristic overlay)
# ---------------------------------------------------------------------------

def gl_highwater(data: bytes) -> int | None:
    """The symbol-handle high-water u32 at gl offset 7 (behind GL_MAGIC)."""
    if data[:len(GL_MAGIC)] == GL_MAGIC and len(data) >= len(GL_MAGIC) + 4:
        return struct.unpack_from("<I", data, len(GL_MAGIC))[0]
    return None


def scan_names(data: bytes, highwater: int | None = None,
               min_len: int = 1) -> list[dict]:
    """Named symbol records in stream order: [{off, handle, name}].

    Heuristic (documented in docs/vc6/il-format.md).  Observed named-record
    shapes, tried in this order for each printable 0-terminated run at
    offset o with data[o-1] == 0:

      embedded  ``<00> <u16 handle> <name>`` - no separator; when both
                handle bytes are printable they head the run itself
                (initialize.cpp: ``02 00  49 27  '$kTown0Buildings'`` =
                handle 0x2749).  Tried first because the symbol-form
                reading of these records false-fires on the preceding
                opcode bytes; a mangled name's own first two chars are an
                implausibly large u16, which keeps true symbol records out.
      symbol    ``<u16 handle> 00 <name>`` (0e/01-opcode records) -
                handle at o-3.
      file      ``<u16 handle> <name>`` where the handle's high 00 byte is
                the byte before the name (21 12 records) - handle at o-2.

    Acceptance = handle plausibility (0 < h <= high-water) plus the name
    charset; *min_len* filters junk runs (gl uses 3 - all real gl names are
    mangled / paths / $-names; sy keeps 1 for bare locals).  Both sides of
    a diff see the same residual noise, so annotations stay comparable.
    """
    hw = highwater if highwater else 0x10000
    out: list[dict] = []
    i, n = 3, len(data)
    while i < n:
        if not (0x21 <= data[i] <= 0x7E) or data[i - 1] != 0:
            i += 1
            continue
        j = i
        while j < n and 0x20 <= data[j] <= 0x7E:
            j += 1
        if j >= n or data[j] != 0:
            i = j + 1
            continue
        name = data[i:j].decode("latin1")
        h = u16(data, i)
        # The embedded reading skips the run's leading handle bytes, which
        # may be ANY printable pair - so it is gated only on the name TAIL,
        # before the full-run charset check (a renumbering that turns a
        # handle byte into ']' must not flip whether the record is seen).
        if (j - i >= min_len + 2 and j - i >= 5 and 0 < h <= hw
                and _NAME_RE.match(name[2:])):
            out.append({"off": i, "handle": h, "name": name[2:]})
        elif len(name) >= min_len and _NAME_RE.match(name):
            for back in (3, 2):
                h = u16(data, i - back)
                if 0 < h <= hw:
                    out.append({"off": i - back, "handle": h, "name": name})
                    break
        i = j + 1
    return out


def fn_ex_offsets(gl: bytes, names: list[dict], ex_size: int) -> list[tuple[int, str]]:
    """(ex_offset, name) per gl function record, sorted by offset.

    Scans the window between one name record and the next for the
    FN_EXOFF_MARKER; records without the marker (data symbols, files,
    literals) contribute nothing.  Offsets outside (0, ex_size] are dropped.
    """
    out: list[tuple[int, str]] = []
    for k, rec in enumerate(names):
        start = rec["off"] + 3 + len(rec["name"]) + 1
        end = names[k + 1]["off"] if k + 1 < len(names) else len(gl)
        w = gl[start:min(end, start + 96)]
        p = w.find(FN_EXOFF_MARKER)
        while p >= 0 and p + len(FN_EXOFF_MARKER) + 4 <= len(w):
            off = struct.unpack_from("<I", w, p + len(FN_EXOFF_MARKER))[0]
            if 0 < off <= ex_size:
                out.append((off, rec["name"]))
                break
            p = w.find(FN_EXOFF_MARKER, p + 1)
    out.sort()
    return out


# ---------------------------------------------------------------------------
# 'sy' - block spans (heuristic overlay)
# ---------------------------------------------------------------------------

def sy_block_spans(data: bytes) -> list[tuple[int, int]]:
    """(start, end) spans split after each observed 0d 02 06 terminator.
    Blocks appear in 'in'-list order (probe proof: p2/p4)."""
    spans, start = [], 0
    i = data.find(SY_BLOCK_END)
    while i != -1:
        spans.append((start, i + len(SY_BLOCK_END)))
        start = i + len(SY_BLOCK_END)
        i = data.find(SY_BLOCK_END, start)
    if start < len(data):
        spans.append((start, len(data)))
    return spans
