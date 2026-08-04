#!/usr/bin/env python3
"""homm3.carve.naming - give EVERY carved function a name.

Total coverage by construction: each of the 11,943 functions gets exactly one
row, and the last tier is a structural fallback that cannot fail. A name here
is a working label, not a claim - the `tier`/`confidence` columns say what
kind of evidence produced it, and only `retail-proven` rows assert an
original identity.

Tiers, highest evidence first (first match wins):

  library-symbol   masked-archive/zlib byte identity gave the real linker
                   symbol; MSVC mangling is decoded to a readable form and
                   the raw symbol is preserved.       [retail-proven]
  fid              stock Ghidra Function ID verdict.  [retail-proven]
  nh3api           NH3API wrapper name for this ENTRY (interior addresses are
                   excluded upstream - they describe another pressing).
                                                      [external-candidate]
  vtable-slot      the function is a slot of a class-labeled vtable, named
                   <Class>__vslot<NN>.                [external-candidate]
  eh-funclet       a FuncInfo unwind-action target (retail EH metadata).
                                                      [structural]
  init-ctor        a `.CRT$XCU` initializer slot, numbered in link order.
                                                      [structural]
  import-wrapper   a thunk to, or lone caller of, one imported API.
                                                      [structural]
  string           an owned literal names the routine's subject.
                                                      [structural]
  caller           named by its dominant caller (callee ordinal), which keeps
                   related code lexically adjacent.   [structural]
  band             library/band prefix + address.      [structural]

Every synthesized name carries its rva, so names are unique and stable across
reruns; retail-proven names keep their symbol and only take an rva suffix on
collision.
"""
from __future__ import annotations

import bisect
import csv
import re
import sys
from collections import defaultdict

from homm3.carve import common

OUT = common.HOMM3_DIR / "config/retail-symbols.csv"
XREFS = common.HOMM3_DIR / "build/dna/function_xrefs.tsv"
BANDS = common.HOMM3_DIR / "config/retail-library-bands.tsv"
LIBRARIES = common.HOMM3_DIR / "config/retail-function-libraries.tsv"
NAMES = common.HOMM3_DIR / "config/retail-function-names.csv"
VTABLE_SYMBOLS = common.HOMM3_DIR / "config/retail-vtable-symbols.csv"

BAND_PREFIX = {"crt-libcmt": "crt", "cxx-libcpmt": "cxx",
               "iostream-libcimt": "ios", "zlib": "zlib", "game": "game",
               "unattributed": "sub"}
IDENT = re.compile(r"[^0-9A-Za-z_]+")
STOP = {"the", "and", "for", "s", "d", "x", "02x", "1", "2", "n", "r"}


def ident(text: str, limit: int = 40) -> str:
    out = IDENT.sub("_", text).strip("_")
    out = re.sub(r"_{2,}", "_", out)
    if out and out[0].isdigit():
        out = "n" + out
    return out[:limit]


def demangle(symbol: str) -> str:
    """Readable form of a VC6 symbol: `?name@class@ns@@...` -> ns_class__name,
    `_memcmp`/`@zcfree@8` -> memcmp/zcfree. Not a full demangler - a label."""
    if symbol.startswith("?"):
        body = symbol[1:].split("@@", 1)[0]
        parts = [p for p in body.split("@") if p]
        if not parts:
            return ident(symbol)
        name, scopes = parts[0], parts[1:]
        if name.startswith("?"):
            # special names: ?0 ctor, ?1 dtor, ?_7 vftable - the class often
            # trails in the same token (`?1locale` -> locale dtor), so keep it
            for code, label in (("?_7", "vftable"), ("?0", "ctor"),
                                ("?1", "dtor")):
                if name.startswith(code):
                    trailing = name[len(code):]
                    name = f"{trailing}_{label}" if trailing else label
                    break
            else:
                name = "op_" + name.lstrip("?")
        prefix = "_".join(reversed(scopes))
        return ident(f"{prefix}__{name}" if prefix else name)
    return ident(symbol.lstrip("_@").split("@")[0])


def load_rows(path, tsv=False):
    if not path.is_file():
        return []
    if tsv:
        return common.read_tsv(path)
    with path.open() as fh:
        return list(csv.DictReader(
            line for line in fh if not line.startswith("#")))


def main(argv=None) -> int:
    functions = [(int(r["rva"], 16), int(r["size"])) for r in
                 common.read_tsv(common.need(
                     common.CARVE_DIR / "functions.tsv", "audit"))]
    functions.sort()
    entries = [rva for rva, _s in functions]
    size_of = dict(functions)

    bands = [(int(r["band_lo"], 16), int(r["band_hi"], 16), r["library"])
             for r in load_rows(BANDS, tsv=True)]
    band_lo = [b[0] for b in bands]

    def band_of(rva):
        i = bisect.bisect_right(band_lo, rva) - 1
        if i >= 0 and bands[i][0] <= rva < bands[i][1]:
            return bands[i][2]
        return "unattributed"

    libraries = {int(r["rva"], 16): r for r in load_rows(LIBRARIES, tsv=True)}
    names_rows = {}
    for r in load_rows(NAMES):
        if r["carve_state"] == "entry":
            names_rows.setdefault(int(r["rva"], 16), r)
    xrefs = {int(r["entry_rva"], 16): r for r in load_rows(XREFS, tsv=True)}

    vtable_slot = {}
    for r in load_rows(VTABLE_SYMBOLS):
        target = int(r["target_rva"], 16)
        if r["class"] and target not in vtable_slot:
            vtable_slot[target] = (r["class"], int(r["slot"]),
                                   r["vtable_rva"])

    # structural sets
    image, _info = common.load_image()
    from homm3.carve import audit
    _fi, funclets, _missing = audit.gate_eh_funclets(image, set(entries))
    ctor_index = {}
    seed_log = common.CARVE_DIR / "seed_log.tsv"
    if seed_log.is_file():
        index = 0
        for r in common.read_tsv(seed_log):
            if r["run"] == "1" and r["iter"] == "1" \
                    and r["source"] == "init-array":
                ctor_index.setdefault(int(r["target_rva"], 16), index)
                index += 1

    # callee ordinal within each caller, for the `caller` tier
    callee_rank = {}
    for rva in entries:
        row = xrefs.get(rva)
        if not row or row["callers"] == "-":
            continue
        callee_rank.setdefault(int(row["callers"].split(";")[0], 16),
                               []).append(rva)
    ordinal = {}
    for caller, callees in callee_rank.items():
        for n, callee in enumerate(sorted(callees)):
            ordinal[callee] = (caller, n)

    rows = []
    used = defaultdict(int)
    tiers = defaultdict(int)

    def emit(rva, name, tier, confidence, evidence, symbol=""):
        base = ident(name) or f"sub_{rva:x}"
        if used[base]:
            base = f"{base}_{rva:x}"
        used[base] += 1
        tiers[tier] += 1
        rows.append({"rva": f"0x{rva:x}", "size": size_of[rva], "name": base,
                     "tier": tier, "confidence": confidence,
                     "library": band_of(rva), "symbol": symbol,
                     "evidence": evidence})

    for rva, _size in functions:
        band = band_of(rva)
        prefix = BAND_PREFIX.get(band, "sub")
        lib = libraries.get(rva)
        xref = xrefs.get(rva)

        symbol = (lib or {}).get("symbol", "")
        if lib and symbol not in ("-", "", "?") \
                and not symbol.startswith(("ambiguous", ".text")) \
                and lib["evidence"].startswith("masked"):
            # an `-island` hit is byte-identical to a library member but sits
            # in game space: a header-template COMDAT instantiated in a game
            # TU. The bytes are proven, the library ATTRIBUTION is not.
            island = lib["confidence"].endswith("-island")
            emit(rva, demangle(symbol), "library-symbol",
                 "external-candidate" if island else "retail-proven",
                 f"{lib['library']}/{lib['member']}"
                 + (" (island: template COMDAT in game space)"
                    if island else ""), symbol)
            continue
        if lib and lib["evidence"] == "stock-fid":
            fid_name = (lib["symbol"].rsplit(" ", 1)[-1]
                        if lib["symbol"] else "")
            if fid_name:
                emit(rva, demangle(fid_name), "fid", "retail-proven",
                     "ghidra-function-id", fid_name)
                continue
        row = names_rows.get(rva)
        if row and row["name"]:
            emit(rva, row["name"].replace("::", "__"), "nh3api",
                 "external-candidate", row["evidence"] or row["sources"])
            continue
        if rva in vtable_slot:
            cls, slot, vt = vtable_slot[rva]
            emit(rva, f"{cls}__vslot{slot:02d}", "vtable-slot",
                 "external-candidate", f"vtable {vt} slot {slot}")
            continue
        if rva in funclets:
            emit(rva, f"eh_funclet_{rva:x}", "eh-funclet", "structural",
                 "FuncInfo unwind action target")
            continue
        if rva in ctor_index:
            emit(rva, f"cinit{ctor_index[rva]:04d}_{rva:x}", "init-ctor",
                 "structural", f".CRT$XCU slot {ctor_index[rva]}")
            continue
        if xref and xref["externals"] != "-":
            # Ghidra's own placeholders (FUN_/thunk_FUN_) are not API names
            apis = [a for a in xref["externals"].split(";")
                    if "FUN_" not in a]
            if apis and (xref["is_thunk"] == "1" or len(apis) == 1):
                verb = "thunk" if xref["is_thunk"] == "1" else "calls"
                emit(rva, f"{verb}_{apis[0]}_{rva:x}", "import-wrapper",
                     "structural", f"imports: {xref['externals']}")
                continue
        if xref and xref["strings"] != "-":
            token = next((t for t in (ident(s, 24).lower()
                                      for s in xref["strings"].split(";"))
                          if t and t.lower() not in STOP), "")
            if token:
                emit(rva, f"{prefix}_{token}_{rva:x}", "string", "structural",
                     f"literal: {xref['strings'].split(';')[0][:32]}")
                continue
        if rva in ordinal:
            caller, n = ordinal[rva]
            emit(rva, f"{prefix}_{caller:x}_sub{n:02d}_{rva:x}", "caller",
                 "structural", f"called from 0x{caller:x}")
            continue
        emit(rva, f"{prefix}_{rva:x}", "band", "structural",
             f"band {band}")

    names_seen = {r["name"] for r in rows}
    if len(names_seen) != len(rows):
        common.die(f"name collision: {len(rows)} rows, {len(names_seen)} names")
    if len(rows) != len(functions):
        common.die(f"coverage: {len(rows)} rows for {len(functions)} functions")
    bad = [r for r in rows if not re.fullmatch(r"[A-Za-z_]\w*", r["name"])]
    if bad:
        common.die(f"{len(bad)} invalid identifiers, first {bad[0]['name']!r}")

    with OUT.open("w", newline="") as fh:
        fh.write("# GENERATED: python3 -m homm3.carve naming - one name per "
                 "carved function (total coverage).\n")
        for prov in common.provenance("homm3.carve.naming"):
            fh.write(prov + "\n")
        fh.write("# tier/confidence say what the name IS: retail-proven "
                 "asserts the original symbol;\n# external-candidate is "
                 "NH3API-derived (unverified); structural names are working "
                 "labels.\n")
        writer = csv.DictWriter(fh, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    print(f"[carve naming] {len(rows)} functions named (100% coverage, "
          f"{len(names_seen)} unique) -> {OUT.name}")
    for tier, count in sorted(tiers.items(), key=lambda kv: -kv[1]):
        print(f"  {tier}: {count}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
