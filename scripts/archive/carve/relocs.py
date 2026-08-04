#!/usr/bin/env python3
"""homm3.carve.relocs - S1: the reloc-site sweep over the retail image.

Runs the vendored find_relocs channels (code / switch / data) over every mapped
section and records EVERY candidate site - nothing is dropped here, because
downstream stages filter for their own purposes and each needs a different
slice: S2 seeds from data-channel .rdata code pointers (isolated ones included -
EH funclet action pointers are isolated by construction), S4 owns switch-channel
tables and back-scans with code-channel sites, S5 builds vtable runs from
data-channel adjacency. The manifest TSV find_relocs writes for the delinker
discards exactly those fields (channel, mnemonic, table base, target class), so
we import the module and keep them.
"""
from __future__ import annotations

import sys
from collections import Counter

import re
import struct

from homm3.carve import common
from homm3.carve.find_relocs import (ROW, Image, code_sites, data_sites,
                                     disassemble, literal_mask, target_class)

SITES_TSV = common.CARVE_DIR / "reloc_sites.tsv"
SUMMARY_TXT = common.CARVE_DIR / "reloc_summary.txt"
MANIFEST = common.HOMM3_DIR / "config/retail-relocs.tsv"
EVIDENCE = common.HOMM3_DIR / "config/retail-reloc-evidence.tsv"
MANIFEST_HEADER = "site_rva\tkind"


def sweep(image: Image):
    """All channels, all sections, nothing dropped. Returns (sites, masks,
    neighbours) exactly as find_relocs.collect would with default args."""
    sites, masks = {}, {}
    for section in image.sections:
        if section.executable:
            sites.update(code_sites(image, section, walk_tables=True))
        else:
            masks[section.name] = literal_mask(image.blob(section))
            sites.update(data_sites(image, section))
    neighbours = frozenset(rva for rva, site in sites.items()
                           if site.channel == "data")
    return sites, masks, neighbours


def operand_context(image, sites):
    """imm vs mem per code-channel site: is the printed operand bracketed?

    A vptr STORE (`mov [esi], offset VT`) carries the address as an imm32; a
    slot LOAD (`mov eax, [VT+4]`) carries it as a memory displacement. S5's
    run cutting must only trust imm references - cutting at loaded slot
    addresses fabricates phantom vtable starts. The vendored module discards
    the operand text, so this second pass re-scans the disassembly for the
    sites the code channel already anchored.
    """
    ctx = {}
    for section in image.sections:
        if not section.executable:
            continue
        low = section.rva
        for line in disassemble(image, section).splitlines():
            row = ROW.match(line)
            if not row:
                continue
            rva = int(row.group(1), 16) - image.image_base
            raw = bytes.fromhex(row.group(2).replace(" ", ""))
            text = row.group(3)
            for offset in range(len(raw) - 3):
                site = sites.get(rva + offset)
                if site is None or site.channel != "code":
                    continue
                if struct.unpack_from("<I", raw, offset)[0] != site.target:
                    continue
                pos = text.find(f"0x{site.target:x}")
                if pos < 0:
                    continue
                before = text[:pos]
                bracketed = before.rfind("[") > before.rfind("]")
                ctx[rva + offset] = "mem" if bracketed else "imm"
    return ctx


def check_manifest(path) -> int:
    """Conformance check mirroring vostok's src/reloc_manifest.rs @ 1393e24b:
    `#`/blank lines skipped; first data line must be byte-equal to the header
    (a second header is an error); every row exactly two tab-separated
    columns; kind == dir32; hex/decimal site_rva; duplicates fatal."""
    saw_header = False
    seen = set()
    for number, line in enumerate(path.read_bytes().decode().splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        if not saw_header:
            if line != MANIFEST_HEADER:
                common.die(f"{path}:{number}: invalid manifest header")
            saw_header = True
            continue
        if line == MANIFEST_HEADER:
            common.die(f"{path}:{number}: duplicate manifest header")
        cells = line.split("\t")
        if len(cells) != 2:
            common.die(f"{path}:{number}: expected exactly two columns")
        site, kind = cells
        if kind != "dir32":
            common.die(f"{path}:{number}: unsupported kind {kind}")
        try:
            value = int(site, 16) if site.lower().startswith("0x") \
                else int(site)
        except ValueError:
            common.die(f"{path}:{number}: invalid site_rva")
        if value in seen:
            common.die(f"{path}:{number}: duplicate site RVA")
        seen.add(value)
    if not saw_header:
        common.die(f"{path}: missing manifest header")
    return len(seen)


def emit_manifest(argv=None) -> int:
    """Render the durable config/ reloc deliverables from S1 + the carve.

    The default find_relocs policy drops the whole data/code-isolated class
    (isolated pointers measured 0.07 precision on a bare image). We are not
    bare: the carved inventory is a function-starts oracle, and on this image
    the isolated sites' targets land on carved entries at ~190x the random
    rate - so isolated sites are kept iff their target is a function ENTRY,
    interior-target sites become a held-back review class, and unowned ones
    drop. Circularity is bounded and reported: entries that S2 itself seeded
    from .rdata pointers are excluded from the enrichment statistic.
    """
    rows = common.read_tsv(common.need(SITES_TSV, "relocs"))
    funcs = {int(r["rva"], 16): int(r["size"]) for r in common.read_tsv(
        common.need(common.CARVE_DIR / "functions.tsv", "audit"))}
    entries = sorted(funcs)

    import bisect

    def target_state(rva):
        if rva in funcs:
            return "entry"
        i = bisect.bisect_right(entries, rva) - 1
        if i >= 0 and entries[i] <= rva < entries[i] + funcs[entries[i]]:
            return "interior"
        return "unowned"

    # circularity bound: entries the seed fixpoint itself created (any
    # source; the .rdata-scalar ones are the directly circular subset)
    seeded, seeded_scalar = set(), set()
    for r in common.read_tsv(common.need(
            common.CARVE_DIR / "seed_log.tsv", "ghidra")):
        if r["result"] == "seeded":
            seeded.add(int(r["target_rva"], 16))
            if r["source"] == "rdata-scalar":
                seeded_scalar.add(int(r["target_rva"], 16))

    kept, evidence_rows = [], []
    counts = {"kept": 0, "kept-isolated-entry": 0,
              "review-isolated-interior": 0, "dropped-isolated-unowned": 0,
              "dropped-unmapped": 0}
    iso_entry_independent = 0
    for r in rows:
        site = int(r["site_rva"], 16)
        target_rva = int(r["value"], 16) - common.IMAGE_BASE
        if r["target_class"] == "unmapped":
            disposition = "dropped-unmapped"
        elif r["target_class"] == "code-isolated":
            state = target_state(target_rva)
            if state == "entry":
                disposition = "kept-isolated-entry"
                iso_entry_independent += target_rva not in seeded
            elif state == "interior":
                disposition = "review-isolated-interior"
            else:
                disposition = "dropped-isolated-unowned"
        else:
            disposition = "kept"
        counts[disposition] += 1
        if disposition.startswith("kept"):
            kept.append(site)
        evidence_rows.append((f"0x{site:x}", r["value"], r["channel"],
                              r["detail"], r["target_class"], r["ctx"],
                              disposition))

    iso_total = (counts["kept-isolated-entry"]
                 + counts["review-isolated-interior"]
                 + counts["dropped-isolated-unowned"])
    notes = [
        f"# kept {len(kept)} of {len(rows)} candidate sites:",
        f"#   {counts['kept']} run/operand sites (code, switch, data in-run)",
        f"#   {counts['kept-isolated-entry']} of {iso_total} isolated data "
        "pointers whose target is a carved function entry",
        f"#     ({iso_entry_independent} of those target entries S2 did NOT "
        "itself seed; the remaining "
        f"{counts['kept-isolated-entry'] - iso_entry_independent} land on "
        "S2-seeded entries and are the only possible circularity; "
        f"{len(seeded_scalar)} entries were scalar-seeded in total)",
        f"#   {counts['review-isolated-interior']} isolated sites with "
        "function-INTERIOR targets HELD BACK for review (see "
        "retail-reloc-evidence.tsv)",
        f"#   {counts['dropped-isolated-unowned']} isolated sites with "
        "unowned targets dropped",
        f"#   {counts['dropped-unmapped']} unmapped-target sites dropped",
        "# NO VALIDATION ORACLE EXISTS FOR THIS IMAGE: every local pressing "
        "ships an",
        "# empty .reloc directory. The channels measured 0.9988 precision / "
        "0.9942",
        "# recall against a DIFFERENT MSVC game exe that still had its "
        ".reloc; those",
        "# figures are the method's credentials, not this image's measured "
        "accuracy.",
    ]

    MANIFEST.write_text(
        common.MANUAL_BANNER
        + "\n".join(common.provenance("homm3.carve.relocs emit-relocs")
                    + notes) + "\n"
        + MANIFEST_HEADER + "\n"
        + "".join(f"0x{site:x}\tdir32\n" for site in sorted(kept)))
    n = check_manifest(MANIFEST)
    if n != len(kept):
        common.die(f"manifest conformance re-read {n} != {len(kept)} written")

    EVIDENCE.write_text(
        common.MANUAL_BANNER
        + "\n".join(common.provenance("homm3.carve.relocs emit-relocs")
                    + ["# per-site evidence for retail-relocs.tsv; "
                       "dispositions explained there"]) + "\n"
        + "\t".join(["site_rva", "value", "channel", "detail", "target_class",
                     "ctx", "disposition"]) + "\n"
        + "".join("\t".join(row) + "\n" for row in evidence_rows))

    print(f"[carve emit-relocs] {len(kept)} sites -> {MANIFEST} "
          "(vostok manifest conformance OK)")
    for key, value in counts.items():
        print(f"  {key}: {value}")
    return 0


def main(argv=None) -> int:
    image, _info = common.load_image()
    sites, masks, neighbours = sweep(image)
    ctx = operand_context(image, sites)

    rows = []
    counts = Counter()
    for rva in sorted(sites):
        site = sites[rva]
        klass = target_class(image, site, masks, None, neighbours)
        counts[(site.channel, klass)] += 1
        rows.append((f"0x{rva:x}", f"0x{site.target:x}", site.channel,
                     site.detail, klass, ctx.get(rva, "-")))

    common.write_tsv(SITES_TSV, "homm3.carve.relocs",
                     ["site_rva", "value", "channel", "detail", "target_class",
                      "ctx"],
                     rows, ["# nothing dropped: downstream stages filter"])

    lines = [f"{channel}/{klass}\t{n}"
             for (channel, klass), n in counts.most_common()]
    lines.append(f"total\t{len(rows)}")
    SUMMARY_TXT.write_text("\n".join(lines) + "\n")

    print(f"[carve relocs] {len(rows)} sites -> {SITES_TSV.name}")
    for line in lines:
        print("  " + line.expandtabs(30))
    # Sanity: the pinned image is 2.7 MB of MSVC output; a sweep an order of
    # magnitude off means the image or the vendored module is wrong.
    if len(rows) < 10000:
        common.die(f"implausibly few reloc sites ({len(rows)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
