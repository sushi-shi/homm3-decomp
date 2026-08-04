#!/usr/bin/env python3
"""homm3.carve.dna - static-library attribution ("the executable's DNA").

The linker emits contributions in linkage order, so library membership is a
property of CONTIGUOUS ADDRESS BANDS, not of isolated functions: engine code
sits next to engine code, zlib next to zlib, CRT next to CRT. This stage
attributes functions through independent channels and then reads the banding;
isolated hits stay candidates, runs become attributions.

Channels (independent; disagreements are reported, never arbitrated away):

  archive   masked byte identity against the PINNED VC6 archives (LIBCMT
            rtm+sp3, LIBCPMT, LIBCIMT, NAFXCW, MFCS42). Every relocated dword
            in a member's code section is masked; the remaining fixed bytes
            must match the image exactly. This is contribution-granular and
            exact - the archives are the very linker inputs, so a linked
            member's bytes ARE in the image.
  zlib      the same matcher over our recompiled vendor/zlib-1.1.3 base objs
            (weaker: recompiled-vs-retail identity, not pinned-input bytes).
  fid       the stock Ghidra Function ID verdicts already in the cached S2
            project (ghidra/export_fid.py).
  strings   version/banner strings (zlib copyright, CRT R60xx block) plus the
            S1 sites that reference them - names the library outright and
            pins band interiors.
  imports   the import directory: middleware that is NOT statically linked
            (Miles, Smack, Bink, IFC20) shows up here instead of in .text.

Durable deliverables (GENERATED - regenerate with `python3 -m homm3.carve
dna`, do not hand-edit; user decision, unlike the admitted carve TSVs):
  evidence/retail-function-libraries.tsv   rva -> library/member/symbol where
                                         known, with evidence + confidence
  evidence/retail-library-bands.tsv        address-ordered band map with
                                         per-band EH-funclet share (the tail
                                         of .text is the .text$x group: COFF
                                         $-sorting places every unwind
                                         funclet after all .text$mn code)

Scratch/regenerable evidence lands under build/dna/.
"""
from __future__ import annotations

import bisect
import struct
import sys
from collections import Counter, defaultdict

from homm3.carve import common
from homm3.carve.fixture import Coff

DNA_DIR = common.HOMM3_DIR / "build/dna"
CONFIG_FUNCTIONS = common.EVIDENCE_DIR / "retail-function-libraries.tsv"
CONFIG_BANDS = common.EVIDENCE_DIR / "retail-library-bands.tsv"

TOOLCHAIN = common.HOMM3_DIR / "build/homm3-toolchain-vc6-sp3/msvc"
# sha256 pins double as provenance: the same archives attempt-1 pinned, all
# from the sha-verified toolchain release.
ARCHIVES = (
    ("crt-libcmt", "sp3", TOOLCHAIN / "lib/LIBCMT.LIB",
     "7922ed5dfd9da6d945a601f28b17e1cd67db97a87d995b579bfcade16e6e5765"),
    ("crt-libcmt", "rtm", TOOLCHAIN / "lib-rtm/LIBCMT.LIB",
     "1ef9c27b4f76031e40fcbdf7d518adc4969f90d6a00dd1ea88697907268c1b2b"),
    ("cxx-libcpmt", "rtm", TOOLCHAIN / "lib/LIBCPMT.LIB",
     "919e3d9c8610a29f7a4a513df25ea2be6500cea742ddd613f5652f24fa4df98c"),
    ("iostream-libcimt", "rtm", TOOLCHAIN / "lib/LIBCIMT.LIB",
     "a78e4d21e853b385b043a1dfd0d65e8a3030eb0d776f4d66c2f76afeee6c115f"),
    ("mfc-nafxcw", "rtm", TOOLCHAIN / "mfc/LIB/NAFXCW.LIB",
     "f87458e19ee65901b99ab6d355d8e5798de949fb55787f0ee25db7bbd85f5216"),
    ("mfc-mfcs42", "rtm", TOOLCHAIN / "mfc/LIB/MFCS42.LIB",
     "296a88b16c5ee7daf02a60c1e5fb43c5c532e17187ef31d61986fb35302e0758"),
)
ZLIB_OBJS = common.HOMM3_DIR / "build/objdiff/base"

# banner strings that name their library outright (exact prefixes; the full
# NUL-terminated string is read from the image)
STRING_MARKERS = (
    ("zlib", b"deflate 1.1.3 Copyright"),
    ("zlib", b"inflate 1.1.3 Copyright"),
    ("crt-libcmt", b"Microsoft Visual C++ Runtime Library"),
    ("crt-libcmt", b"R6002\r\n- floating point not loaded"),
    ("crt-libcmt", b"runtime error"),
    ("crt-libcmt", b"GetLastActivePopup"),
)

IMAGE_SCN_CNT_CODE = 0x20
MIN_SECTION = 8
MIN_FIXED = 10
MIN_ANCHOR = 5
MAX_HITS = 4


# --- archive parsing ------------------------------------------------------

def archive_members(data: bytes):
    """(member_name, coff_bytes) for every COFF member of a `!<arch>` file."""
    if data[:8] != b"!<arch>\n":
        common.die("not an archive")
    longnames = b""
    offset = 8
    while offset + 60 <= len(data):
        header = data[offset:offset + 60]
        name = header[:16].rstrip()
        size = int(header[48:58].rstrip() or b"0")
        body = data[offset + 60:offset + 60 + size]
        offset += 60 + size + (size & 1)
        if name in (b"/", b""):
            continue
        if name == b"//":
            longnames = body
            continue
        if name.startswith(b"/"):
            start = int(name[1:])
            # MSVC longnames are NUL-terminated (GNU ar uses "/\n")
            end = min((longnames.index(t, start) for t in (b"\0", b"\n")
                       if t in longnames[start:]), default=len(longnames))
            member = longnames[start:end].rstrip(b"/\x00").decode("latin-1")
        else:
            member = name.rstrip(b"/").decode("latin-1")
        if len(body) >= 20 and struct.unpack_from("<H", body, 0)[0] == 0x14C:
            yield member, body


def code_sections(coff: Coff):
    """(index, blob, mask, symbol) per code section; mask covers relocs."""
    # symbol at offset 0 of a section, preferring externals, names the match
    by_section = defaultdict(list)
    for name, value, secnum, typ, sclass in coff.symbols.values():
        if secnum > 0 and value == 0 and sclass in (2, 3):
            by_section[secnum - 1].append((0 if sclass == 2 else 1, name))
    for index, (name, blob, relocs) in enumerate(coff.sections):
        if not name.startswith(".text") or not blob:
            continue
        mask = bytearray(len(blob))
        for va, _sym, _typ in relocs:
            mask[va:va + 4] = b"\1\1\1\1"
        symbols = sorted(by_section.get(index, []))
        yield index, blob, bytes(mask), (symbols[0][1] if symbols else "?")


def longest_anchor(blob: bytes, mask: bytes):
    best_at, best_len, run_at, run_len = 0, 0, 0, 0
    for i, m in enumerate(mask):
        if m:
            run_len = 0
            continue
        if run_len == 0:
            run_at = i
        run_len += 1
        if run_len > best_len:
            best_at, best_len = run_at, run_len
    return best_at, best_len


def masked_find(text: bytes, blob: bytes, mask: bytes):
    """All offsets where blob matches text with masked bytes ignored."""
    anchor_at, anchor_len = longest_anchor(blob, mask)
    if anchor_len < MIN_ANCHOR:
        return None  # unsearchable: too little fixed contiguous material
    anchor = blob[anchor_at:anchor_at + anchor_len]
    hits = []
    pos = text.find(anchor)
    while pos >= 0 and len(hits) <= MAX_HITS:
        start = pos - anchor_at
        if 0 <= start <= len(text) - len(blob):
            if all(mask[i] or text[start + i] == blob[i]
                   for i in range(len(blob))):
                hits.append(start)
        pos = text.find(anchor, pos + 1)
    return hits


# --- channels -------------------------------------------------------------

def match_archives(text: bytes, text_rva: int):
    """The archive channel. Returns raw match rows (one per section hit)."""
    rows = []
    stats = Counter()
    for family, variant, path, sha in ARCHIVES:
        if not path.is_file():
            common.die(f"pinned archive missing: {path} (run `homm3 init`)")
        data = path.read_bytes()
        if common.sha256_of(path) != sha:
            common.die(f"{path.name}: sha256 mismatch against pin")
        for member, body in archive_members(data):
            coff = Coff(body)
            for index, blob, mask, symbol in code_sections(coff):
                stats[f"{family}-{variant}/sections"] += 1
                if len(blob) < MIN_SECTION:
                    stats[f"{family}-{variant}/too-small"] += 1
                    continue
                fixed = len(blob) - sum(mask)
                if fixed < MIN_FIXED:
                    stats[f"{family}-{variant}/too-masked"] += 1
                    continue
                hits = masked_find(text, blob, mask)
                if hits is None:
                    stats[f"{family}-{variant}/unsearchable"] += 1
                    continue
                if not hits:
                    continue
                if len(hits) > MAX_HITS:
                    stats[f"{family}-{variant}/generic"] += 1
                    continue
                stats[f"{family}-{variant}/matched"] += 1
                for hit in hits:
                    rows.append({"family": family, "variant": variant,
                                 "member": member, "symbol": symbol,
                                 "rva": text_rva + hit, "size": len(blob),
                                 "fixed": fixed, "hits": len(hits)})
    return rows, stats


def match_zlib(text: bytes, text_rva: int):
    rows = []
    stats = Counter()
    objs = sorted(ZLIB_OBJS.glob("*.obj"))
    if not objs:
        print("[carve dna] NOTE: no zlib base objs (run `homm3 build`); "
              "zlib channel limited to strings")
        return rows, stats
    for path in objs:
        coff = Coff(path.read_bytes())
        for index, blob, mask, symbol in code_sections(coff):
            stats["sections"] += 1
            if len(blob) < MIN_SECTION or len(blob) - sum(mask) < MIN_FIXED:
                stats["skipped"] += 1
                continue
            hits = masked_find(text, blob, mask)
            if hits is None:
                stats["unsearchable"] += 1
                continue
            if not hits or len(hits) > MAX_HITS:
                stats["unmatched" if not hits else "generic"] += 1
                continue
            stats["matched"] += 1
            for hit in hits:
                rows.append({"family": "zlib", "variant": "recompiled",
                             "member": path.name, "symbol": symbol,
                             "rva": text_rva + hit, "size": len(blob),
                             "fixed": len(blob) - sum(mask),
                             "hits": len(hits)})
    return rows, stats


def scan_strings(image, sites, owner_of):
    """Marker strings + the functions that reference them."""
    data = image.data
    refs_by_target = defaultdict(list)
    for row in sites:
        if row["channel"] == "code":
            refs_by_target[int(row["value"], 16)].append(
                int(row["site_rva"], 16))
    rows = []
    for family, prefix in STRING_MARKERS:
        offset = data.find(prefix)
        while offset >= 0:
            # this image maps headers+sections 1:1, so offset == rva for the
            # initialized sections; verify it lands in a mapped section
            section = image.section_of(offset)
            if section is not None:
                end = data.index(b"\0", offset)
                target_va = image.image_base + offset
                ref_sites = refs_by_target.get(target_va, [])
                owners = sorted({owner_of(site) for site in ref_sites}
                                - {None})
                rows.append((f"0x{offset:x}", family,
                             data[offset:min(end, offset + 60)].decode(
                                 "latin-1").replace("\t", " "),
                             ";".join(f"0x{s:x}" for s in ref_sites[:6]) or "-",
                             ";".join(f"0x{o:x}" for o in owners[:6]) or "-"))
            offset = data.find(prefix, offset + 1)
    return rows


def scan_imports(image):
    data = image.data
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    imp_rva = struct.unpack_from("<I", data, pe + 24 + 96 + 8)[0]
    rows = []
    cursor = imp_rva
    while True:
        ilt, _ts, _fc, name_rva, iat = struct.unpack_from("<5I", data, cursor)
        if not (ilt or name_rva or iat):
            break
        name = data[name_rva:data.index(b"\0", name_rva)].decode("latin-1")
        count = 0
        thunk = ilt or iat
        while struct.unpack_from("<I", data, thunk + 4 * count)[0]:
            count += 1
        rows.append((name, f"0x{iat:x}", count))
        cursor += 20
    return rows


def export_fid():
    """Run the in-project stock-FID bookmark export (cached S2 project)."""
    import os
    if not os.environ.get("GHIDRA_INSTALL_DIR"):
        common.die("GHIDRA_INSTALL_DIR unset - enter the dev shell")
    info = common.intake()
    import pyghidra
    pyghidra.start()
    from pyghidra.core import _setup_project
    from ghidra.app.script import GhidraScriptUtil
    script = common.SCRIPT_DIR / "ghidra/export_fid.py"
    gproject, program = _setup_project(
        binary_path=info["path"],
        project_location=str(common.CARVE_DIR / "ghidra"),
        project_name="carve", nested_project_location=False)
    GhidraScriptUtil.acquireBundleHostReference()
    try:
        pyghidra.ghidra_script(str(script), gproject.getProject(),
                               program=program)
    finally:
        GhidraScriptUtil.releaseBundleHostReference()
        gproject.close()


# --- attribution + banding ------------------------------------------------

def attribute_functions(functions, match_rows, fid_rows, string_rows):
    """Fold every channel into one verdict per function entry."""
    per_function = {}
    conflicts = []
    by_rva = defaultdict(list)
    for row in match_rows:
        by_rva[row["rva"]].append(row)

    entries = sorted(functions)
    for rva, rows in sorted(by_rva.items()):
        if rva not in functions:
            continue  # interior/non-entry matches reported separately
        families = {r["family"] for r in rows}
        if len(families) > 1:
            # identical bytes in two archives (operator delete, shared
            # C++/iostream members): real library code, undecidable member
            conflicts.append((rva, sorted(families)))
            per_function[rva] = {
                "library": "shared(" + "+".join(sorted(families)) + ")",
                "variant": "-", "member": f"ambiguous({len(rows)})",
                "symbol": "-", "evidence": "masked-archive",
                "confidence": "shared-member"}
            continue
        family = families.pop()
        variants = sorted({r["variant"] for r in rows})
        members = sorted({r["member"] for r in rows})
        symbols = sorted({r["symbol"] for r in rows})
        unique = all(r["hits"] == 1 for r in rows)
        per_function[rva] = {
            "library": family, "variant": "+".join(variants),
            "member": members[0] if len(members) == 1 else
                      f"ambiguous({len(members)})",
            "symbol": symbols[0] if len(symbols) == 1 else
                      f"ambiguous({len(symbols)})",
            "evidence": "masked-archive" if family != "zlib"
                        else "masked-zlib-obj",
            "confidence": "exact-unique" if unique and len(members) == 1
                          else "exact-ambiguous"}

    for row in fid_rows:
        rva = int(row["entry_rva"], 16)
        if rva in functions and rva not in per_function:
            per_function[rva] = {
                "library": "library-unassigned", "variant": "-",
                "member": "-", "symbol": row["evidence"][:80],
                "evidence": "stock-fid", "confidence": "fid-" +
                row["category"].lower()}
    return per_function, conflicts


MIN_CLUSTER = 3     # a band needs at least this many attributed functions
MAX_CLUSTER_GAP = 32  # consecutive same-family hits within this many entries


def in_allowed(rva, allowed):
    i = bisect.bisect_right([lo for lo, _hi in allowed], rva) - 1
    return i >= 0 and allowed[i][0] <= rva < allowed[i][1]


def game_channel(functions, per_function, allowed, funclet_entries,
                 head_limit, funclet_parent):
    """The game's own code is a library too - label it by POSITIVE evidence,
    never by absence of library hits:

      hd-crossbuild-name  entry rows of evidence/retail-hd-name-map.csv: an
                      NH3API name carried onto OUR bytes by unique masked
                      identity against HD Mod's sibling build (carve hdmap).
                                                        [crossbuild-verified]
      nh3api-name     entry functions the names export attributes to game
                      classes (external-unverified candidates, entry rows
                      only - interior addresses are another pressing);
      init-array-ctor the `.CRT$XCU` prefix in slot order: static ctors of
                      the game's own objects come before every library ctor
                      (linkage order), so the prefix up to the first
                      library-attributed target is game code, retail-derived.
      xref-to-game    call-graph closure from every function the channels
                      above named: relative `E8` calls are retail bytes, so
                      "calls or is called by known game code" is positive
                      retail evidence for game-ness even though the seed
                      names are external. `-direct` = shares an edge with a
                      NAMED game function, `-transitive` = reached through
                      other closure members only.
      game-vtable-link a retail vtable ties functions together: the function
                      that stores the vtable pointer (its ctor/dtor) and the
                      slot targets belong to one class, so game-ness flows
                      across the vtable in both directions.

    Library attributions always win; this channel only labels virgin rvas.
    Restricted to `allowed` (the spans no library band claims): NH3API also
    models the game's std-like wrappers (exe_string, exe_vector), whose retail
    bodies ARE LIBCPMT code - letting those names vote inside a library band
    would shred it. The closure additionally never enters an EH funclet
    (`.text$x` is every object's funclets sorted together - band structure,
    not game evidence; calls FROM a funclet are folded into the function it
    guards via `funclet_parent` instead) and never propagates THROUGH a
    library-attributed function (game code calling memcpy must not paint
    memcpy's other callers). Every channel is also cut at `head_limit`, the start of the
    first library band: linkage order makes game contributions a contiguous
    PREFIX of .text, so an address past the first library band cannot be
    game code no matter what calls it (the generic `operator delete` sliver
    between zlib and LIBCPMT is the case that forced this guard).
    """
    import csv as _csv

    def load_csv(path):
        if not path.is_file():
            return []
        with path.open() as fh:
            return list(_csv.DictReader(
                line for line in fh if not line.startswith("#")))

    hits = {}
    for row in load_csv(common.EVIDENCE_DIR / "retail-hd-name-map.csv"):
        if row["our_state"] != "entry":
            continue
        rva = int(row["rva"], 16)
        if rva in functions and rva not in per_function \
                and rva < head_limit and in_allowed(rva, allowed):
            hits[rva] = "hd-crossbuild-name"
    for row in load_csv(common.EVIDENCE_DIR / "retail-function-names.csv"):
        if row["carve_state"] != "entry" or "nh3api" not in row["sources"]:
            continue
        rva = int(row["rva"], 16)
        if rva in functions and rva not in per_function \
                and rva < head_limit and in_allowed(rva, allowed):
            hits.setdefault(rva, "nh3api-name")
    named = set(hits)
    seed_log = common.CARVE_DIR / "seed_log.tsv"
    if seed_log.is_file():
        for row in common.read_tsv(seed_log):
            if row["run"] != "1" or row["iter"] != "1" or \
                    row["source"] != "init-array":
                continue
            target = int(row["target_rva"], 16)
            if target in per_function or not in_allowed(target, allowed):
                break  # first library ctor: the game prefix ends here
            if target in functions:
                hits.setdefault(target, "init-array-ctor")

    # call-graph closure. Edges: Ghidra's call graph (rel32 calls carry no
    # relocation, so only the export sees them) + vtable links (vptr-store
    # site <-> slot targets, both retail-derived).
    calls = defaultdict(set)
    xrefs_tsv = DNA_DIR / "function_xrefs.tsv"
    if xrefs_tsv.is_file():
        for row in common.read_tsv(xrefs_tsv):
            if row["callers"] == "-":
                continue
            # a call FROM an unwind funclet is a call from the function the
            # funclet guards (retail EH parentage) - that is how a dtor
            # reached only on exception paths still ties to its owner
            callee = funclet_parent.get(int(row["entry_rva"], 16),
                                        int(row["entry_rva"], 16))
            for caller in row["callers"].split(";"):
                caller = int(caller, 16)
                caller = funclet_parent.get(caller, caller)
                if caller == callee:
                    continue
                calls[caller].add(callee)
                calls[callee].add(caller)
    else:
        print("[carve dna] function_xrefs.tsv missing - game closure runs "
              "on vtable links only")
    slots = defaultdict(list)
    for row in load_csv(common.EVIDENCE_DIR / "retail-vtable-symbols.csv"):
        if row["target_state"] == "entry":
            slots[int(row["vtable_rva"], 16)].append(
                int(row["target_rva"], 16))
    entries = sorted(functions)

    def owner_of(rva):
        i = bisect.bisect_right(entries, rva) - 1
        if i >= 0 and entries[i] <= rva < entries[i] + functions[entries[i]]:
            return entries[i]
        return None

    vlinks = defaultdict(set)
    reloc_sites = common.CARVE_DIR / "reloc_sites.tsv"
    if slots and reloc_sites.is_file():
        for row in common.read_tsv(reloc_sites):
            if row["channel"] != "code" or row.get("ctx") != "imm":
                continue
            targets = slots.get(int(row["value"], 16) - common.IMAGE_BASE)
            if not targets:
                continue
            owner = owner_of(int(row["site_rva"], 16))
            if owner is None:
                continue
            for target in targets:
                vlinks[owner].add(target)
                vlinks[target].add(owner)

    def eligible(rva):
        return rva in functions and rva not in per_function \
            and rva not in hits and rva not in funclet_entries \
            and rva < head_limit and in_allowed(rva, allowed)

    frontier = set(hits)
    while frontier:
        reached = set()
        for rva in frontier:
            for other in calls[rva] | vlinks[rva]:
                if eligible(other):
                    reached.add(other)
        for rva in reached:
            if calls[rva] & named:
                hits[rva] = "xref-to-game-direct"
            elif calls[rva] & hits.keys():
                hits[rva] = "xref-to-game-transitive"
            else:
                hits[rva] = "game-vtable-link"
        frontier = reached

    for rva, evidence in hits.items():
        if rva not in per_function:
            per_function[rva] = {
                "library": "game", "variant": "-", "member": "-",
                "symbol": "-", "evidence": evidence,
                "confidence": "crossbuild-verified"
                if evidence == "hd-crossbuild-name" else
                "external-candidate"}
    return hits


def build_bands(functions, per_function, funclet_entries):
    """Contiguous same-library bands; unattributed runs between two bands of
    the same library are inferred into it (linkage-order argument); the rest
    stays `unattributed`. `game` is a first-class family here, labeled by the
    game_channel's positive evidence - regions with no evidence at all keep
    the `unattributed` label (the funclet zone, edge slivers).

    Library membership is a property of contiguous bands, so a hit only
    participates in banding as part of a CLUSTER (>=MIN_CLUSTER same-family
    hits, each within MAX_CLUSTER_GAP entries of the next). Isolated hits are
    real byte-identities but usually header-template COMDATs compiled into
    game TUs, or 8-byte generic stubs - they stay candidates (confidence
    suffixed `-island`) and never found a band."""
    entries = sorted(functions)
    labels = [per_function.get(rva, {}).get("library") for rva in entries]
    # generic FID hits and shared-member ties corroborate library-ness but
    # carry no single family: neutral for banding
    labels = [None if lab is None or lab == "library-unassigned"
              or lab.startswith("shared(") else lab for lab in labels]

    by_family = defaultdict(list)
    for index, label in enumerate(labels):
        if label is not None:
            by_family[label].append(index)
    for family, positions in by_family.items():
        cluster = [positions[0]]
        clusters = [cluster]
        for position in positions[1:]:
            if position - cluster[-1] <= MAX_CLUSTER_GAP:
                cluster.append(position)
            else:
                cluster = [position]
                clusters.append(cluster)
        for cluster in clusters:
            if len(cluster) < MIN_CLUSTER:
                for position in cluster:
                    labels[position] = None
                    p = per_function[entries[position]]
                    if not p["confidence"].endswith("-island"):
                        p["confidence"] += "-island"

    filled = list(labels)
    i = 0
    while i < len(filled):
        if filled[i] is not None:
            i += 1
            continue
        j = i
        while j < len(filled) and filled[j] is None:
            j += 1
        left = filled[i - 1] if i else None
        right = filled[j] if j < len(filled) else None
        if left is not None and left == right:
            for k in range(i, j):
                filled[k] = left
        i = j

    bands = []
    start = 0
    for i in range(1, len(entries) + 1):
        if i == len(entries) or (filled[i] or "-") != (filled[start] or "-"):
            lo = entries[start]
            hi = entries[i - 1] + functions[entries[i - 1]]
            attributed = sum(1 for k in range(start, i)
                             if labels[k] is not None)
            library = filled[start] or "unattributed"
            funclets = sum(1 for k in range(start, i)
                           if entries[k] in funclet_entries)
            evidence = {"unattributed": "no-evidence",
                        "zlib": "masked-zlib-obj-run",
                        "game": "hd+nh3api-names+init-order+xref-closure",
                        }.get(library, "masked-archive-run")
            bands.append({"lo": lo, "hi": hi, "library": library,
                          "functions": i - start, "attributed": attributed,
                          "bytes": sum(functions[entries[k]]
                                       for k in range(start, i)),
                          "funclet_share": f"{funclets / (i - start):.2f}",
                          "evidence": evidence})
            start = i
    return bands


def main(argv=None) -> int:
    argv = list(argv or [])
    image, _info = common.load_image()
    text = next(s for s in image.sections if s.name == ".text")
    blob = image.blob(text)
    DNA_DIR.mkdir(parents=True, exist_ok=True)

    functions = {int(r["rva"], 16): int(r["size"]) for r in common.read_tsv(
        common.need(common.CARVE_DIR / "functions.tsv", "audit"))}
    sites = common.read_tsv(common.need(
        common.CARVE_DIR / "reloc_sites.tsv", "relocs"))

    import bisect
    entries = sorted(functions)

    def owner_of(rva):
        i = bisect.bisect_right(entries, rva) - 1
        if i >= 0 and entries[i] <= rva < entries[i] + functions[entries[i]]:
            return entries[i]
        return None

    if "--skip-fid" not in argv:
        export_fid()
    fid_rows = (common.read_tsv(DNA_DIR / "fid_bookmarks.tsv")
                if (DNA_DIR / "fid_bookmarks.tsv").is_file() else [])

    print("[carve dna] matching pinned VC6 archives ...", flush=True)
    archive_rows, archive_stats = match_archives(blob, text.rva)
    print("[carve dna] matching recompiled zlib objs ...", flush=True)
    zlib_rows, zlib_stats = match_zlib(blob, text.rva)
    match_rows = archive_rows + zlib_rows

    common.write_tsv(
        DNA_DIR / "archive_matches.tsv", "homm3.carve.dna",
        ["family", "variant", "member", "symbol", "rva", "size", "fixed",
         "hits"],
        [(r["family"], r["variant"], r["member"], r["symbol"],
          f"0x{r['rva']:x}", r["size"], r["fixed"], r["hits"])
         for r in sorted(match_rows, key=lambda r: r["rva"])])
    for key in sorted(archive_stats):
        print(f"  {key}: {archive_stats[key]}")
    for key in sorted(zlib_stats):
        print(f"  zlib/{key}: {zlib_stats[key]}")

    string_rows = scan_strings(image, sites, owner_of)
    common.write_tsv(DNA_DIR / "strings.tsv", "homm3.carve.dna",
                     ["string_rva", "library", "text", "ref_sites",
                      "ref_functions"], string_rows)
    import_rows = scan_imports(image)
    common.write_tsv(DNA_DIR / "imports.tsv", "homm3.carve.dna",
                     ["dll", "iat_rva", "imports"], import_rows)

    per_function, conflicts = attribute_functions(
        functions, match_rows, fid_rows, string_rows)
    for rva, families in conflicts:
        print(f"[carve dna] CONFLICT at 0x{rva:x}: {families} - left "
              "unattributed, see archive_matches.tsv")
    interior = [r for r in match_rows if r["rva"] not in functions
                and owner_of(r["rva"]) is not None]
    if interior:
        print(f"[carve dna] {len(interior)} matches at non-entry addresses "
              "(see archive_matches.tsv; merged funclets or matcher noise)")

    # the unwind-funclet set (audit's FuncInfo walk) characterizes the
    # .text$x group: COFF $-sorting places every EH funclet contribution
    # after all .text$mn code, so funclet share is band structure, not noise
    from homm3.carve import audit
    _fi, funclet_entries, _missing = audit.gate_eh_funclets(
        image, set(functions))
    # two-pass: band on LIBRARY evidence alone, then let the game channel
    # speak only where no library band claims the address, then re-band
    library_bands = build_bands(functions, per_function, funclet_entries)
    allowed = sorted((b["lo"], b["hi"]) for b in library_bands
                     if b["library"] == "unattributed")
    head_limit = min((b["lo"] for b in library_bands
                      if b["library"] not in ("unattributed", "game")),
                     default=text.rva + text.size)
    from homm3.carve.naming import eh_parentage
    funclet_parent = {funclet: parent for funclet, (parent, _state) in
                      eh_parentage(image, entries, owner_of).items()}
    game_hits = game_channel(functions, per_function, allowed,
                             funclet_entries, head_limit, funclet_parent)
    game_kinds = Counter(game_hits.values())
    print(f"[carve dna] game channel: {len(game_hits)} positive hits")
    for kind, count in game_kinds.most_common():
        print(f"    {kind}: {count}")
    bands = build_bands(functions, per_function, funclet_entries)

    banner = ("# GENERATED by `python3 -m homm3.carve dna` - regenerate "
              "rather than hand-edit.\n")

    rows = []
    for rva in sorted(per_function):
        p = per_function[rva]
        rows.append((f"0x{rva:x}", p["library"], p["variant"], p["member"],
                     p["symbol"], p["evidence"], p["confidence"]))
    CONFIG_FUNCTIONS.write_text(banner + "\n".join(
        common.provenance("homm3.carve.dna")
        + ["\t".join(["rva", "library", "variant", "member", "symbol",
                      "evidence", "confidence"])]
        + ["\t".join(str(c) for c in r) for r in rows]) + "\n")

    band_rows = [(f"0x{b['lo']:x}", f"0x{b['hi']:x}", b["library"],
                  b["functions"], b["attributed"], b["bytes"],
                  b["funclet_share"], b["evidence"]) for b in bands]
    CONFIG_BANDS.write_text(banner + "\n".join(
        common.provenance("homm3.carve.dna")
        + ["\t".join(["band_lo", "band_hi", "library", "functions",
                      "attributed", "bytes", "eh_funclet_share", "evidence"])]
        + ["\t".join(str(c) for c in r) for r in band_rows]) + "\n")

    by_lib = Counter()
    lib_bytes = Counter()
    for rva, p in per_function.items():
        by_lib[p["library"]] += 1
        lib_bytes[p["library"]] += functions[rva]
    print(f"[carve dna] attributed {len(per_function)}/{len(functions)} "
          f"functions -> {CONFIG_FUNCTIONS.name}")
    for lib, n in by_lib.most_common():
        print(f"  {lib}: {n} functions, {lib_bytes[lib]} bytes")
    print(f"[carve dna] {len(bands)} bands -> {CONFIG_BANDS.name}")
    for b in bands:
        print(f"  0x{b['lo']:06x}..0x{b['hi']:06x} {b['library']:>18} "
              f"{b['functions']:5} fns ({b['attributed']} attributed, "
              f"{b['bytes']} B, funclets {b['funclet_share']})")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
