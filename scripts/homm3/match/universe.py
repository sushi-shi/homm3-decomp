#!/usr/bin/env python3
"""homm3.match.universe - the single authoritative function-universe
classifier (gruntz core/function_universe ported to our evidence rules).

Every consumer of the full-engine denominator uses this module so the
filters cannot drift apart. Per function of config/retail-functions.tsv,
in precedence order:

  eh-funclet    FuncInfo unwind-action target, walked from RETAIL EH
                metadata (carve.audit.gate_eh_funclets over the pinned
                image - no evidence/ dependency). Matches with its parent.
  runtime       config/retail-runtime-map.tsv (LIBCMT + LIBCPMT): named,
                not matched from source.
  zlib          config/retail-zlib-map.tsv: a matching TARGET (vendored
                sources compile), counted inside the main table.
  init-thunk    `.CRT$XCU` dynamic-initializer bodies (build/carve/
                seed_log.tsv, the carve init-array walk; compiler-
                generated, reconstructed implicitly by VA_COMPGEN).
                Degrades to zero with a warning if the carve artifact is
                absent - regenerate with `python3 -m homm3.carve relocs`.
  import-thunk  a <=8-byte body that is a `FF 25 <IAT slot>` jump, read
                from the image directly.
  target        everything else: the game code the decompilation exists
                to reconstruct.
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

from homm3.carve import common

FUNCTIONS = common.HOMM3_DIR / "config/retail-functions.tsv"
RUNTIME_MAP = common.HOMM3_DIR / "config/retail-runtime-map.tsv"
ZLIB_MAP = common.HOMM3_DIR / "config/retail-zlib-map.tsv"
SEED_LOG = common.CARVE_DIR / "seed_log.tsv"

EXCLUDED_NOTES = {
    "eh-funclet": "compiler EH unwind funclets; match with their parent "
                  "function",
    "runtime": "CRT/C++ runtime, named not matched "
               "(config/retail-runtime-map.tsv)",
    "init-thunk": ".CRT$XCU dynamic-initializer bodies "
                  "(compiler-generated)",
    "import-thunk": "FF 25 jumps through the IAT",
}


def _rows(path):
    return [line.rstrip("\n").split("\t") for line in path.open()
            if line[0] not in "#" and not line.startswith("rva\t")]


def classify():
    """{rva: category}, sizes {rva: size} - see the module docstring."""
    functions = {int(r[0], 16): int(r[1]) for r in _rows(FUNCTIONS)}
    category = {}

    image, info = common.load_image()
    from homm3.carve import audit
    _fi, funclets, _missing = audit.gate_eh_funclets(image, set(functions))
    for rva in funclets:
        category[rva] = "eh-funclet"

    for r in _rows(RUNTIME_MAP):
        category.setdefault(int(r[0], 16), "runtime")
    for r in _rows(ZLIB_MAP):
        category.setdefault(int(r[0], 16), "zlib")

    if SEED_LOG.is_file():
        for r in common.read_tsv(SEED_LOG):
            if r["run"] == "1" and r["iter"] == "1" \
                    and r["source"] == "init-array":
                target = int(r["target_rva"], 16)
                if target in functions:
                    category.setdefault(target, "init-thunk")
    else:
        print("[universe] WARNING: no carve seed_log - init-thunk "
              "category empty (run `python3 -m homm3.carve relocs`)",
              file=sys.stderr)

    text = next(s for s in image.sections if s.name == ".text")
    blob = image.blob(text)
    data = Path(info["path"]).read_bytes()
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
