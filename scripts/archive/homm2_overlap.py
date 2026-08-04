#!/usr/bin/env python3
"""homm3.analysis.homm2_overlap - ONE-SHOT cross-game symbol comparison.

Measures how much of HoMM2's engine survived into HoMM3 by joining the
sibling homm2-decomp's CodeView-authoritative symbol inventory (and its
76%-exact matched source) against our Dreamcast/NH3API name corpora and
the retail image. Lanes: TU stems, class names, normalized function
names, rare-string anchors (the lane that reaches our UNNAMED
functions), and a small cross-compiler shape calibration.

Reads the homm2 repo STRICTLY read-only ($HOMM2_DECOMP, default
~/Projects/homm2/homm2-decomp). Writes evidence/homm2-overlap/*.csv.

DESTINED FOR scripts/archive/ once docs/homm2-symbol-overlap.md lands:
the durable results are the evidence tables and the docs report, not
this script. Deliberately self-contained (stdlib + core primitives
only) so archiving it breaks nothing.
"""
from __future__ import annotations

import csv
import hashlib
import json
import os
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

from homm3.core import common
from homm3.core.image import Image

H2 = Path(os.environ.get("HOMM2_DECOMP",
                         str(Path.home() / "Projects/homm2/homm2-decomp")))
OUT = common.EVIDENCE_DIR / "homm2-overlap"

_STRIP_STACK = re.compile(r"@-?\d+$")
_RUN = re.compile(rb"[\x20-\x7e]{8,}")


# --- homm2 side -------------------------------------------------------------------

def h2_units() -> dict:
    """stem(lower) -> TIER/STEM from config/units.toml."""
    out = {}
    for line in (H2 / "config/units.toml").read_text().splitlines():
        m = re.match(r'\s*unit\s*=\s*"([^"]+)"', line)
        if m:
            out[m.group(1).split("/")[-1].lower()] = m.group(1)
    return out


def h2_report() -> dict:
    """unit -> (exact, total, mean_fuzzy); plus mangled-name -> fuzzy."""
    rep = json.loads((H2 / "build/objdiff/report.json").read_text())
    units, per_fn = {}, {}
    for unit in rep.get("units", []):
        scores = [fn.get("fuzzy_match_percent") or 0.0
                  for fn in unit.get("functions") or []]
        for fn in unit.get("functions") or []:
            per_fn[fn.get("name")] = fn.get("fuzzy_match_percent")
        if scores:
            units[unit["name"]] = (
                sum(1 for s in scores if s >= 100.0), len(scores),
                sum(scores) / len(scores))
    return units, per_fn


def demangle_h2(name: str) -> str | None:
    """Normalized 'class::method' / free name from raw MSVC decoration."""
    if name.startswith("??0") or name.startswith("??1"):
        cls = name[3:].split("@@", 1)[0].split("@")[0]
        return f"{cls}::{'~' if name[2] == '1' else ''}{cls}".lower()
    if name.startswith("??"):
        return None  # operators / special members: skip for the join
    if name.startswith("?"):
        components = name[1:].split("@@", 1)[0].split("@")
        if len(components) == 1:
            return components[0].lower()  # ?Free@@Y...
        # ?Method@Inner@Outer@@ mangles inside-out: Outer::Inner::Method
        return ("::".join(reversed(components[1:]))
                + "::" + components[0]).lower()
    if name.startswith("_") and not name.startswith("__"):
        return _STRIP_STACK.sub("", name[1:]).lower()
    return None


def h2_symbols(nwc_units: set):
    """(funcs, strings): funcs = [(rva, mangled, norm, unit, size)] for
    NWC-owned functions; strings = {rva} of ??_C@ literal starts."""
    funcs, strings = [], set()
    with (H2 / "build/gen/symbol_names.csv").open(encoding="latin-1") as fh:
        for r in csv.DictReader(fh):
            try:
                rva = int(r["rva"], 16)
                size = int(r.get("size") or "0", 0)
            except ValueError:
                continue
            if r["kind"] == "func" and r.get("unit") in nwc_units:
                funcs.append((rva, r["name"], demangle_h2(r["name"]),
                              r["unit"], size))
            elif r["kind"] == "data" and r["name"].startswith("??_C@"):
                strings.add(rva)
    return funcs, strings


def h2_classes() -> set:
    out = set()
    for path in (H2 / "include").rglob("*.h"):
        for m in re.finditer(r"\b(?:class|struct)\s+([A-Za-z_]\w+)",
                             path.read_text(errors="ignore")):
            out.add(m.group(1))
    return out


# --- homm3 side -------------------------------------------------------------------

def dc_functions():
    """[(offset, norm_name, module)] + class-name set from the DC corpus."""
    rows, classes = [], set()
    with (common.EVIDENCE_DIR / "dreamcast/functions.csv").open() as fh:
        lines = (ln for ln in fh if not ln.startswith("#"))
        for r in csv.DictReader(lines):
            name = _STRIP_STACK.sub("", r["name"])
            rows.append((int(r["offset"], 16), name.lower(), r["module"]))
            if "::" in name:
                classes.add(name.split("::")[0].split("<")[0])
    return rows, classes


def dc_classes() -> set:
    out = set()
    with (common.EVIDENCE_DIR / "dreamcast/classes.csv").open() as fh:
        lines = (ln for ln in fh if not ln.startswith("#"))
        for r in csv.DictReader(lines):
            out.add((r.get("name") or "").split("<")[0])
    return {c for c in out if c}


def dcmap():
    """norm name -> retail rva (the proven/candidate DC->retail joins)."""
    out = {}
    with (common.EVIDENCE_DIR / "retail-dc-name-map.csv").open() as fh:
        lines = (ln for ln in fh if not ln.startswith("#"))
        for r in csv.DictReader(lines):
            out.setdefault(r["name"].lower(), int(r["rva"], 16))
    return out


def ida_names() -> set:
    out = set()
    with (common.EVIDENCE_DIR / "ida/functions.csv").open() as fh:
        lines = (ln for ln in fh if not ln.startswith("#"))
        for r in csv.DictReader(lines):
            for col in ("name", "readable"):
                if r.get(col):
                    out.add(_STRIP_STACK.sub("", r[col]).lower())
    return out


def h3_symbols():
    """{rva: (name, unit, size, provenance)} for our function rows."""
    out = {}
    with (common.HOMM3_DIR / "build/gen/symbol_names.csv").open() as fh:
        lines = (ln for ln in fh if not ln.startswith("#"))
        for r in csv.DictReader(lines):
            if r.get("kind") != "func":
                continue
            out[int(r["rva"], 16)] = (r["name"], r.get("unit", ""),
                                      int(r.get("size") or "0", 16),
                                      r.get("provenance", ""))
    return out


# --- shared string machinery (self-contained; sema stays un-imported) -------------

def string_map(image: Image) -> dict:
    """{va: text} for printable runs (>=8) in non-executable sections."""
    out = {}
    for section in image.sections:
        if section.executable:
            continue
        blob = image.blob(section)
        for m in _RUN.finditer(blob):
            out[image.image_base + section.rva + m.start()] = \
                m.group().decode("latin1")
    return out


def owner_of(starts, sizes, rva):
    import bisect
    k = bisect.bisect_right(starts, rva) - 1
    if k < 0:
        return None
    start = starts[k]
    if sizes.get(start) and rva >= start + sizes[start]:
        return None
    return start


def immediate_refs(image: Image, wanted: set, starts, sizes) -> dict:
    """{string_va: {owner_fn_rva}} by scanning .text 4-byte immediates."""
    text = next(s for s in image.sections if s.name == ".text")
    blob = image.blob(text)
    refs = defaultdict(set)
    for i in range(len(blob) - 3):
        value = struct.unpack_from("<I", blob, i)[0]
        if value in wanted:
            owner = owner_of(starts, sizes, text.rva + i)
            if owner is not None:
                refs[value].add(owner)
    return refs


def reloc_refs(image: Image, wanted: set, starts, sizes) -> dict:
    """Same shape, but exact: our admitted dir32 sites."""
    refs = defaultdict(set)
    for line in (common.HOMM3_DIR / "config/retail-relocs.tsv").open():
        if line.startswith("#") or line.startswith("site_rva"):
            continue
        site = int(line.split("\t", 1)[0], 16)
        section = image.section_of(site)
        if section is None:
            continue
        value = struct.unpack_from(
            "<I", image.data, section.raw_offset + (site - section.rva))[0]
        if value in wanted:
            owner = owner_of(starts, sizes, site)
            if owner is not None:
                refs[value].add(owner)
    return refs


def call_count(image: Image, rva: int, size: int) -> int:
    text = next(s for s in image.sections if s.name == ".text")
    blob = image.blob(text)
    body = blob[rva - text.rva:rva - text.rva + size]
    return sum(1 for i in range(max(0, len(body) - 4))
               if body[i] == 0xE8)


# --- output -----------------------------------------------------------------------

def write_rows(path: Path, header: list, rows, extra=None):
    with path.open("w", newline="") as fh:
        for line in common.provenance("homm3.analysis.homm2_overlap",
                                      extra or []):
            fh.write(line + "\n")
        w = csv.writer(fh)
        w.writerow(header)
        w.writerows(rows)
    print(f"  {path.relative_to(common.HOMM3_DIR)}: {len(rows)} rows")


def main(argv=None) -> int:
    if not (H2 / "build/gen/symbol_names.csv").is_file():
        print(f"[homm2_overlap] homm2 repo not found/ready at {H2}",
              file=sys.stderr)
        return 1
    OUT.mkdir(parents=True, exist_ok=True)
    h2_exe = H2 / "build/orig/HEROES2W.EXE"
    h2_sha = hashlib.sha256(h2_exe.read_bytes()).hexdigest()
    prov = [f"# homm2: HEROES2W.EXE sha256={h2_sha} (read-only sibling)",
            f"# homm2 repo: {H2}"]

    units = h2_units()
    unit_scores, fn_fuzzy = h2_report()
    h2_funcs, h2_strs = h2_symbols(set(units.values()))
    h2_cls = h2_classes()
    dc_rows, dc_name_cls = dc_functions()
    dc_cls = dc_classes() | dc_name_cls
    name_to_rva = dcmap()
    ida = ida_names()
    ours = h3_symbols()
    print(f"[homm2_overlap] h2: {len(h2_funcs)} NWC funcs, "
          f"{len(h2_cls)} classes, {len(h2_strs)} literals; "
          f"h3: {len(dc_rows)} DC names, {len(dc_cls)} DC classes")

    # lane 1: TU stems
    dc_modules = defaultdict(int)
    for _off, _name, module in dc_rows:
        dc_modules[module.rsplit(".", 1)[0].lower()] += 1
    unit_rows = []
    for stem, unit in sorted(units.items()):
        if stem in dc_modules:
            exact, total, fuzzy = unit_scores.get(unit, (0, 0, 0.0))
            unit_rows.append((stem, unit, exact, total, f"{fuzzy:.2f}",
                              dc_modules[stem]))
    write_rows(OUT / "units.csv",
               ["stem", "h2_unit", "h2_exact", "h2_fns", "h2_fuzzy",
                "h3_dc_fns"], unit_rows, prov)

    # lane 2: classes
    dc_fold = {c.lower(): c for c in dc_cls}
    class_rows = sorted((c, dc_fold[c.lower()]) for c in h2_cls
                        if c.lower() in dc_fold)
    write_rows(OUT / "classes.csv", ["h2_class", "h3_class"],
               class_rows, prov)

    # lane 3: normalized function names
    dc_by_norm = defaultdict(list)
    for off, norm, module in dc_rows:
        dc_by_norm[norm].append((off, module))
    fn_rows = []
    for rva, mangled, norm, unit, size in h2_funcs:
        if not norm or norm not in dc_by_norm:
            continue
        retail = name_to_rva.get(norm)
        for off, module in dc_by_norm[norm][:1]:
            fn_rows.append((
                norm, f"0x{rva:x}", unit, size,
                fn_fuzzy.get(mangled, ""),
                f"0x{off:x}", module,
                f"0x{retail:x}" if retail else "",
                "yes" if norm in ida else ""))
    write_rows(OUT / "functions.csv",
               ["name", "h2_rva", "h2_unit", "h2_size", "h2_fuzzy",
                "h3_dc_offset", "h3_dc_module", "h3_retail_rva",
                "in_nh3api"], sorted(fn_rows), prov)

    # lane 4: rare-string anchors
    h3_image, _info = common.load_image()
    h2_image = Image(str(h2_exe))
    h3_strings = string_map(h3_image)
    h2_strings = string_map(h2_image)
    shared_texts = set(h3_strings.values()) & set(h2_strings.values())
    h3_by_text = defaultdict(set)
    for va, text in h3_strings.items():
        if text in shared_texts:
            h3_by_text[text].add(va)
    h2_by_text = defaultdict(set)
    for va, text in h2_strings.items():
        if text in shared_texts:
            h2_by_text[text].add(va)

    h3_starts = sorted(ours)
    h3_sizes = {rva: row[2] for rva, row in ours.items()}
    h2_sizes = {rva: size for rva, _m, _n, _u, size in h2_funcs}
    h2_starts = sorted(h2_sizes)
    h3_refs = reloc_refs(h3_image,
                         {va for vas in h3_by_text.values() for va in vas},
                         h3_starts, h3_sizes)
    h2_refs = immediate_refs(h2_image,
                             {va for vas in h2_by_text.values() for va in vas},
                             h2_starts, h2_sizes)
    h2_names = {rva: (mangled, norm, unit)
                for rva, mangled, norm, unit, _s in h2_funcs}
    anchor_rows = []
    for text in sorted(shared_texts):
        h3_owners = {fn for va in h3_by_text[text] for fn in h3_refs.get(va, ())}
        h2_owners = {fn for va in h2_by_text[text] for fn in h2_refs.get(va, ())}
        if not (1 <= len(h3_owners) <= 3 and 1 <= len(h2_owners) <= 3):
            continue
        for h3_fn in sorted(h3_owners):
            for h2_fn in sorted(h2_owners):
                mangled, norm, unit = h2_names.get(h2_fn, ("?", "", "?"))
                name, _u, _s, provenance = ours.get(h3_fn,
                                                    ("?", "", 0, ""))
                anchor_rows.append((
                    text[:60], f"0x{h3_fn:x}", name, provenance,
                    f"0x{h2_fn:x}", norm or mangled, unit,
                    fn_fuzzy.get(mangled, "")))
    write_rows(OUT / "string_anchors.csv",
               ["literal", "h3_rva", "h3_label", "h3_provenance",
                "h2_rva", "h2_name", "h2_unit", "h2_fuzzy"],
               anchor_rows, prov)

    # lane 5 output: the boost list (new info for homm3 only)
    from homm3.match import universe
    classes, _sizes = universe.classify(h3_image)
    boost = []
    for row in anchor_rows:
        rva = int(row[1], 16)
        if row[3] == "working-label" and classes.get(rva) == "target":
            boost.append(row + ("string-anchor",))
    named = {int(r[7], 16) for r in fn_rows if r[7]}
    for r in fn_rows:
        if not r[7]:
            continue
        rva = int(r[7], 16)
        _n, _u, _s, provenance = ours.get(rva, ("", "", 0, ""))
        fuzzy = r[4]
        if classes.get(rva) == "target" and fuzzy and float(fuzzy) >= 99.0:
            boost.append((r[0], r[7], ours.get(rva, ("?",))[0], provenance,
                          r[1], r[0], r[2], fuzzy, "h2-source-template"))
    write_rows(OUT / "boost.csv",
               ["key", "h3_rva", "h3_label", "h3_provenance", "h2_rva",
                "h2_name", "h2_unit", "h2_fuzzy", "grade"],
               boost, prov)

    # calibration: shape drift across MSVC 4.2 -> 6.0 on known name-pairs
    h2_by_rva_size = {rva: size for rva, _m, _n, _u, size in h2_funcs}
    pairs = [(r[0], int(r[1], 16), int(r[7], 16)) for r in fn_rows
             if r[7]][:20]
    print("\n[calibration] name-paired functions, h2 vs h3 shape:")
    for norm, h2_rva, h3_rva in pairs:
        s2 = h2_by_rva_size.get(h2_rva, 0)
        s3 = h3_sizes.get(h3_rva, 0)
        if not (s2 and s3):
            continue
        c2 = call_count(h2_image, h2_rva, s2)
        c3 = call_count(h3_image, h3_rva, s3)
        print(f"  {norm[:44]:44} size {s2:5}/{s3:5} "
              f"({s3 / s2:4.2f}x)  calls {c2:3}/{c3:3}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
