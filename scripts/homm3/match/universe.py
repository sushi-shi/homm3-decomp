#!/usr/bin/env python3
"""homm3.match.universe - the single authoritative function-universe
classifier (gruntz core/function_universe ported to our evidence rules).

Every consumer of the full-engine denominator uses this module so the
filters cannot drift apart. Per function of config/retail/functions.tsv,
in precedence order:

  eh-funclet    config/retail/funclets.tsv (admitted from the retail EH
                metadata walk, with parentage). Matches with its parent.
  runtime       config/retail/runtime-map.tsv (CRT/C++, MFC and SDK), plus
                whole census bodies inside one reviewed pinned-library code
                contribution: library-owned, not matched from game source.
  zlib          config/retail/zlib-map.tsv: a matching TARGET (vendored
                sources compile), counted inside the main table.
  init-thunk    config/retail/init-thunks.tsv: `.CRT$XCU` dynamic-
                initializer bodies and reviewed atexit cleanups (slot "-").
                Both are compiler-generated from source declarations.
  import-thunk  a <=8-byte body that is a `FF 25 <IAT slot>` jump, read
                from the image directly.
  target        everything else: the game code the decompilation exists
                to reconstruct.

Everything comes from config/ + the pinned image: no evidence/, no
build/carve artifacts, no homm3.carve stage calls.
"""
from __future__ import annotations

import struct
import sys

from homm3.core import common
from homm3.core.images import path as _image_path

FUNCTIONS = common.HOMM3_DIR / _image_path("config/retail/functions.tsv")
RUNTIME_MAP = common.HOMM3_DIR / _image_path("config/retail/runtime-map.tsv")
ZLIB_MAP = common.HOMM3_DIR / _image_path("config/retail/zlib-map.tsv")
FUNCLETS = common.HOMM3_DIR / _image_path("config/retail/funclets.tsv")
INIT_THUNKS = common.HOMM3_DIR / _image_path("config/retail/init-thunks.tsv")
RUNTIME_CONTRIBUTIONS = common.HOMM3_DIR / _image_path("config/retail/runtime-contributions.tsv")

EXCLUDED_NOTES = {
    "eh-funclet": "compiler EH unwind funclets; match with their parent "
                  "function",
    "runtime": "platform library code, named not matched "
               "(config/retail/runtime-map.tsv)",
    "init-thunk": "compiler-generated CRT initializer/cleanup bodies",
    "import-thunk": "FF 25 jumps through the IAT",
}

#: How `homm3 verify generated-code` verifies each category, for the README.
VERIFIED_NOTES = {
    "runtime": "bytes and relocations equal pinned library "
               "members (config/retail/runtime-functions.tsv)",
    "eh-funclet": "parent's `.text$x` COMDAT: bytes and every relocation "
                  "target (library parents: their library section)",
    "init-thunk": "source-emitted CRT bodies, bytes and named relocations",
    "import-thunk": "`FF 25` through a named IAT slot (and the pinned "
                    "import library where one exists)",
}


def _rows(path):
    return [line.rstrip("\n").split("\t") for line in path.open()
            if line[0] not in "#" and not line.startswith("rva\t")]


def _runtime_contributions():
    from homm3.core.tsv import read
    return read(RUNTIME_CONTRIBUTIONS)[2] if RUNTIME_CONTRIBUTIONS.is_file() else []


def contribution_runtime_bodies(functions, contributions):
    """Whole bodies owned by a reviewed pinned-library code contribution.

    Only an explicit ``ownership=whole-section`` declaration admits internal
    entries, after complete native byte/relocation verification. Older rows
    may describe just a symbol prefix and supply no interior ownership.
    Internal assembler entries need not have their own symbols. Do not invent labels, stitch partial coverage, accept
    aliases/data as ownership, or choose between overlapping section owners.
    Like runtime-map, this is an inventory classification; library-code and
    generated-code verification remain the byte/relocation verdicts.
    """
    from homm3.verify.library_code import RUNTIME_LIBRARIES, MFC_LIBRARY
    libraries = set(RUNTIME_LIBRARIES) | {MFC_LIBRARY}
    spans = set()
    for row in contributions:
        if (row.get('ownership') != 'whole-section' or row['kind'] != 'code'
                or row['library'] not in libraries):
            continue
        lo, size, section = int(row['rva'], 16), int(row['size']), int(row['section'])
        if (lo < 0 or size <= 0 or section <= 0
                or row['member'] in ('', '-') or row['symbol'] in ('', '-')):
            continue
        spans.add((lo, lo + size, row['library'], row['member'], section))
    out = set()
    for rva, size in functions.items():
        if size <= 0:
            continue
        end = rva + size
        overlapping = [span for span in spans if span[0] < end and rva < span[1]]
        if len(overlapping) == 1:
            lo, hi, *_owner = overlapping[0]
            if lo <= rva and end <= hi:
                out.add(rva)
    return out


def classify(image=None):
    """{rva: category}, sizes {rva: size} - see the module docstring.

    `image` accepts an already-loaded gated Image so an in-process caller
    (homm3.sema Context) does not re-load and re-hash the exe; when
    omitted the image is loaded (and gated) here as before."""
    functions = {int(r[0], 16): int(r[1]) for r in _rows(FUNCTIONS)}
    category = {}

    if image is None:
        image, _info = common.load_image()
    for r in _rows(FUNCLETS):
        category[int(r[0], 16)] = "eh-funclet"
    for r in _rows(RUNTIME_MAP):
        category.setdefault(int(r[0], 16), "runtime")
    for r in _rows(ZLIB_MAP):
        if len(r) < 5 or r[4] == 'func':
            category.setdefault(int(r[0], 16), "zlib")
    for r in _rows(INIT_THUNKS):
        rva = int(r[0], 16)
        if rva in functions:
            category.setdefault(rva, "init-thunk")

    # A reviewed assembly section can contain local entry points with no COFF
    # symbol. Keep more specific EH/zlib/initializer categories above intact.
    for rva in contribution_runtime_bodies(functions, _runtime_contributions()):
        category.setdefault(rva, "runtime")

    text = next(s for s in image.sections if s.name == ".text")
    blob = image.blob(text)
    data = image.data  # the image already holds the whole file's bytes
    pe_off = struct.unpack_from("<I", data, 0x3C)[0]
    iat_rva, iat_size = struct.unpack_from(
        "<II", data, pe_off + 24 + 96 + 12 * 8)
    iat_lo, iat_hi = iat_rva, iat_rva + iat_size
    for rva, size in functions.items():
        if rva in category or size > 8:
            continue
        off = rva - text.rva
        if blob[off:off + 2] == b"\xff\x25":
            target = struct.unpack_from("<I", blob, off + 2)[0] \
                - image.image_base
            if iat_lo <= target < iat_hi:
                category[rva] = "import-thunk"

    for rva in functions:
        category.setdefault(rva, "target")
    return category, functions


def summary():
    """[(category, functions, bytes)] + the target/zlib tallies."""
    category, sizes = classify()
    tally = {}
    for rva, cat in category.items():
        entry = tally.setdefault(cat, [0, 0])
        entry[0] += 1
        entry[1] += sizes[rva]
    return category, sizes, tally


def main(argv=None) -> int:
    _category, _sizes, tally = summary()
    for cat in ("target", "zlib", "eh-funclet", "runtime", "init-thunk",
                "import-thunk"):
        fns, code = tally.get(cat, (0, 0))
        print(f"  {cat:14} {fns:6} fns  {code:9,} B")
    return 0


if __name__ == "__main__":
    sys.exit(main())
