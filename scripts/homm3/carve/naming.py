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
  hd-crossbuild    NH3API's name, transferred from HD Mod's sibling build by
                   UNIQUE masked byte identity onto a carved entry of ours
                   (homm3.carve.hdmap). The name is external; the
                   identification is our own bytes.  [crossbuild-verified]
  nh3api           NH3API wrapper name whose raw address happens to hit one of
                   our entries directly. Kept below hd-crossbuild because the
                   address itself carries no evidence for our image.
                                                      [external-candidate]
  dc-linkorder     Dreamcast CodeView name transferred by link order alone
                   (homm3.carve.dcmap `linkorder` rows): equal-count bracket
                   between two order-verified anchors, agreement-gated. The
                   weakest identity tier - order evidence from another
                   pressing - but it carries the ORIGINAL name plus
                   source FILE:LINE.                  [linkorder-candidate]
  vtable-slot      the function is a slot of a class-labeled vtable, named
                   <Class>__vslot<NN>.                [external-candidate]
  eh-funclet       a FuncInfo unwind-action target, named after the PARENT
                   function that owns the FuncInfo: retail EH metadata gives
                   funcinfo -> unwind actions, and the parent is the function
                   whose code loads that FuncInfo address as an imm32 (the
                   __CxxFrameHandler setup). So a funclet reads
                   `<parent>_unwind03`, not an address.  [structural]
  init-ctor        a `.CRT$XCU` initializer, named after the class whose
                   vtable it installs where the ctor body references a
                   class-labeled vtable.               [structural]
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

from homm3.carve import audit, common

OUT = common.EVIDENCE_DIR / "retail-symbols.csv"
XREFS = common.HOMM3_DIR / "build/dna/function_xrefs.tsv"
BANDS = common.EVIDENCE_DIR / "retail-library-bands.tsv"
LIBRARIES = common.EVIDENCE_DIR / "retail-function-libraries.tsv"
NAMES = common.EVIDENCE_DIR / "retail-function-names.csv"
VTABLE_SYMBOLS = common.EVIDENCE_DIR / "retail-vtable-symbols.csv"
HD_MAP = common.EVIDENCE_DIR / "retail-hd-name-map.csv"
DC_MAP = common.EVIDENCE_DIR / "retail-dc-name-map.csv"

BAND_PREFIX = {"crt-libcmt": "crt", "cxx-libcpmt": "cxx",
               "iostream-libcimt": "ios", "zlib": "zlib", "game": "game",
               "unattributed": "sub"}
IDENT = re.compile(r"[^0-9A-Za-z_]+")
STOP = {"the", "and", "for", "s", "d", "x", "02x", "1", "2", "n", "r"}


def ident(text: str, limit: int = 64) -> str:
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


def eh_parentage(image, entries, owner_of):
    """funclet_rva -> (parent_rva, state). Retail EH metadata only.

    Walk .rdata FuncInfo records for their UnwindMapEntry action pointers
    (the funclets), then attribute each FuncInfo to the function whose body
    loads its address as an imm32 - that is the __CxxFrameHandler setup, so
    the loader IS the guarded function."""
    import struct as _struct
    rdata = next(s for s in image.sections if s.name == ".rdata")
    blob = image.blob(rdata)
    text = next(s for s in image.sections if s.name == ".text")
    lo, hi = text.rva, text.rva + text.size
    base = image.image_base

    info_actions = {}
    for offset in range(0, len(blob) - 28 + 1, 4):
        magic, max_state, unwind_va, ntry, _t, nip, _i = \
            _struct.unpack_from("<7I", blob, offset)
        if magic not in audit.EH_MAGICS or max_state == 0:
            continue
        if max_state > 0x1000 or ntry > 0x1000 or nip > 0x10000:
            continue
        start = unwind_va - base - rdata.rva
        if not 0 <= start <= len(blob) - max_state * 8:
            continue
        actions, valid = [], True
        for state in range(max_state):
            to_state, action_va = _struct.unpack_from(
                "<iI", blob, start + state * 8)
            if not -1 <= to_state < max_state:
                valid = False
                break
            if action_va:
                action_rva = action_va - base
                if not lo <= action_rva < hi:
                    valid = False
                    break
                actions.append((state, action_rva))
        if valid and actions:
            info_actions[rdata.rva + offset] = actions

    # VC6 loads the FuncInfo inside a tiny `__ehhandler$` stub
    #     mov eax, <FuncInfo>;  jmp __CxxFrameHandler
    # which the linker parks in the .text$x tail, OUTSIDE every carved
    # function. So resolution is two hops: FuncInfo <- stub <- the guarded
    # function, which pushes the stub address as its SEH handler.
    imm_sites = []
    for row in common.read_tsv(common.CARVE_DIR / "reloc_sites.tsv"):
        if row["channel"] == "code" and row.get("ctx") == "imm":
            imm_sites.append((int(row["site_rva"], 16),
                              int(row["value"], 16) - base))
    by_target = defaultdict(list)
    for site, target in imm_sites:
        by_target[target].append(site)

    parent_of_info = {}
    for info_rva in info_actions:
        for site in by_target.get(info_rva, ()):
            owner = owner_of(site)
            if owner is not None:            # rare: loaded inline
                parent_of_info.setdefault(info_rva, owner)
                break
            stub = site - 1                  # the `B8 imm32` opcode byte
            for push_site in by_target.get(stub, ()):
                handler_owner = owner_of(push_site)
                if handler_owner is not None:
                    parent_of_info.setdefault(info_rva, handler_owner)
                    break
            if info_rva in parent_of_info:
                break

    out = {}
    for info_rva, actions in info_actions.items():
        parent = parent_of_info.get(info_rva)
        for state, action_rva in actions:
            if parent is not None and action_rva not in out:
                out[action_rva] = (parent, state)
    return out


def ctor_subjects(functions, size_of, owner_of, vtable_class):
    """init-array ctor -> class whose vtable it installs (if any)."""
    subjects = {}
    for row in common.read_tsv(common.CARVE_DIR / "reloc_sites.tsv"):
        if row["channel"] != "code" or row.get("ctx") != "imm":
            continue
        target = int(row["value"], 16) - common.IMAGE_BASE
        cls = vtable_class.get(target)
        if not cls:
            continue
        owner = owner_of(int(row["site_rva"], 16))
        if owner is not None:
            subjects.setdefault(owner, cls)
    return subjects


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
    hd_names = {}
    for r in load_rows(HD_MAP):
        if r["our_state"] == "entry":
            hd_names.setdefault(int(r["rva"], 16), r)
    dc_names = {}
    for r in load_rows(DC_MAP):
        if r["role"] == "linkorder":
            dc_names.setdefault(int(r["rva"], 16), r)

    vtable_slot = {}
    vtable_class = {}
    for r in load_rows(VTABLE_SYMBOLS):
        target = int(r["target_rva"], 16)
        if r["class"]:
            vtable_class.setdefault(int(r["vtable_rva"], 16), r["class"])
            if target not in vtable_slot:
                vtable_slot[target] = (r["class"], int(r["slot"]),
                                       r["vtable_rva"])

    # structural sets
    image, _info = common.load_image()
    _fi, funclets, _missing = audit.gate_eh_funclets(image, set(entries))

    def owner_of(rva):
        i = bisect.bisect_right(entries, rva) - 1
        if i >= 0 and entries[i] <= rva < entries[i] + size_of[entries[i]]:
            return entries[i]
        return None

    eh_parent = eh_parentage(image, entries, owner_of)
    ctor_class = ctor_subjects(functions, size_of, owner_of, vtable_class)
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
    by_rva = {}
    used = defaultdict(int)
    tiers = defaultdict(int)

    def emit(rva, name, tier, confidence, evidence, symbol=""):
        base = ident(name) or f"sub_{rva:x}"
        if used[base]:
            base = f"{base}_{rva:x}"
        used[base] += 1
        tiers[tier] += 1
        row = {"rva": f"0x{rva:x}", "size": size_of[rva], "name": base,
               "tier": tier, "confidence": confidence,
               "library": band_of(rva), "symbol": symbol,
               "evidence": evidence}
        rows.append(row)
        by_rva[rva] = row

    # pass 1 skips EH funclets: they are named after their parent, which must
    # have its own name first (parents live in code bands, funclets in $x)
    for rva, _size in functions:
        if rva in eh_parent:
            continue
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
        hd = hd_names.get(rva)
        if hd:
            emit(rva, hd["name"].replace("::", "__"), "hd-crossbuild",
                 "crossbuild-verified",
                 f"HD {hd['hd_va']} masked-identity {hd['match_bytes']}B/"
                 f"{hd['fixed_bytes']} fixed; {hd['evidence']}")
            continue
        row = names_rows.get(rva)
        if row and row["name"]:
            emit(rva, row["name"].replace("::", "__"), "nh3api",
                 "external-candidate", row["evidence"] or row["sources"])
            continue
        dc = dc_names.get(rva)
        if dc:
            emit(rva, dc["name"].replace("::", "__"), "dc-linkorder",
                 "linkorder-candidate",
                 f"DC {dc['dc_module']} {dc['dc_offset']} by link order; "
                 f"{dc['source']}")
            continue
        if rva in vtable_slot:
            cls, slot, vt = vtable_slot[rva]
            emit(rva, f"{cls}__vslot{slot:02d}", "vtable-slot",
                 "external-candidate", f"vtable {vt} slot {slot}")
            continue
        if rva in ctor_index:
            cls = ctor_class.get(rva)
            label = (f"cinit_{cls}_{rva:x}" if cls
                     else f"cinit{ctor_index[rva]:04d}_{rva:x}")
            emit(rva, label, "init-ctor", "structural",
                 f".CRT$XCU slot {ctor_index[rva]}"
                 + (f"; installs {cls} vtable" if cls else ""))
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

    # pass 2: EH funclets inherit their guarded parent's identity
    orphan = 0
    for rva, _size in functions:
        if rva not in eh_parent:
            continue
        parent, state = eh_parent[rva]
        parent_row = by_rva.get(parent)
        if parent_row is None:
            orphan += 1
            emit(rva, f"eh_funclet_{rva:x}", "eh-funclet", "structural",
                 "FuncInfo unwind action target (parent unresolved)")
            continue
        # keep the discriminating suffix: truncate the parent stem, not it
        emit(rva, f"{ident(parent_row['name'], 46)}_unwind{state:02d}",
             "eh-funclet", "structural",
             f"unwind action of 0x{parent:x} ({parent_row['name']}), "
             f"state {state}")
    for rva in sorted(funclets - set(eh_parent)):
        if rva in by_rva:
            continue
        emit(rva, f"eh_funclet_{rva:x}", "eh-funclet", "structural",
             "FuncInfo unwind action target (no FuncInfo owner found)")

    rows.sort(key=lambda r: int(r["rva"], 16))
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
                 "NH3API-derived (unverified); linkorder-candidate is a "
                 "Dreamcast name\n# carried by link order alone; structural "
                 "names are working labels.\n")
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
