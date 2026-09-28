"""homm3.verify.data_coverage - is every datum MODELLED, not merely byte-equal?

The other half of the data-access audit, asked from OUR side. We choose the
extent, so a TOO-SMALL claim always scores 100: model `float g_v` where retail
has `struct { float x, y; }` and objdiff compares four bytes, finds them equal
and calls the section exact. This module reports which retail bytes NO enrolled
datum covers, and what retail's own payload says about them.

A decoded read proves an uncovered byte is used. Absence from the access map
does not prove padding: register-relative accesses and pointer escapes can be
invisible. Each row carries the map's observed touches and address references.

THE ORACLES, ALL FROM RETAIL, NONE FROM OUR PAYLOAD
  claims   the MODEL's named data bindings (rva + extent + owner) - every
           datum this tree claims, initialized or not. The frozen tool used
           the delink manifest alone, which predates the census: a `.bss`
           global that no object carves is claimed but never enrolled, so the
           manifest-only reading reported thousands of modelled globals as
           uncovered.
  enrolled build/gen/delink_data_manifest.tsv - the subset objdiff compares.
  sections build/gen/delink_data_section_manifest.tsv - a PLACED candidate
           section rebuilds (and objdiff compares) bytes no datum names: a /GR
           vtable COMDAT carries the ??_R4 pointer 4 bytes in front of `??_7`.
           Counting those as uncovered manufactured ~180 false positives along
           the vtable frontier alone in the frozen tool.
  payload  retail's bytes at an uncovered range. cl's inter-symbol padding is
           ZERO, so a NON-ZERO uncovered byte is content nobody modelled.
  .reloc   a relocated word INSIDE an uncovered range is a POINTER nobody
           modelled - conclusive, and it cannot be alignment padding; and the
           stored addresses give the set of addresses the image POINTS AT.

ADJACENCY PROVES NOTHING. A gap is reported, never closed by inventing an
aggregate to fill it.

VERDICTS
  PAD          uncovered, all-zero, shorter than 8 B: a padding candidate.
               The complete byte report still counts it as missing.
  ZERO-GAP     uncovered, all-zero, too long to be padding.
  NONZERO      uncovered bytes that are not zero. Content nobody modelled.
  POINTER      NONZERO and retail relocates a word inside it - unmodelled
               pointer data, conclusive.
  OVERLAP      two claims covering one byte with different extents (a folded
               COMDAT seen from N objects agrees on both and is excluded).

    python3 -m homm3.verify.data_coverage [--verdict V] [--near RVA]
    python3 -m homm3.verify.data_coverage --tsv | --overlaps | --gate
"""

from __future__ import annotations

import bisect
from collections import Counter, defaultdict

from homm3.core.paths import BUILD
from homm3.core.tsv import read as read_tsv
from homm3.core.tsv import write as write_tsv

MANIFEST = BUILD / "gen/delink_data_manifest.tsv"
SECTIONS = BUILD / "gen/delink_data_section_manifest.tsv"
GAPS_TSV = BUILD / "gen/data_coverage_gaps.tsv"

#: cl aligns a standalone global to its own element size, capped at 8 for the
#: ordinary sections (16 needs `__declspec(align)`, which MSVC 5 lacks).
MAX_PAD = 8

GAP_COLUMNS = ["rva", "length", "section", "verdict", "addressed", "touched",
               "sites", "payload_nonzero", "relocs", "prev_object",
               "prev_name", "next_object", "next_name", "first_bytes"]


def load_claims(path=MANIFEST):
    """Every enrolled datum. The same retail extent appears once per object
    that defines it (a folded COMDAT literal or vtable), so a caller needing
    the covered byte SET must dedupe on (rva, size) - `coverage()` does."""
    if not path.is_file():
        raise SystemExit(f"no {path} - run `homm3 build` first")
    _b, _h, rows = read_tsv(path)
    return [{"name": r["name"], "object": r["object"], "storage": r["storage"],
             "rva": int(r["rva"], 16), "size": int(r["size"], 16)}
            for r in rows]


def load_sections(path=SECTIONS):
    """Every PLACED candidate section - the other half of what objdiff
    compares. A `-` rva is a non-affine section with no retail claim."""
    if not path.is_file():
        return []
    _b, _h, rows = read_tsv(path)
    return [{"object": r["object"], "name": r["name"],
             "rva": int(r["rva"], 16), "size": int(r["size"], 16),
             "storage": r["storage"]}
            for r in rows if r["rva"] != "-"]


def coverage(claims, sections=()):
    """Maximal covered runs [start, end): a byte an enrolled datum claims OR a
    placed candidate section rebuilds. Only what neither reaches is unmodelled."""
    ext = {(c["rva"], c["size"]) for c in claims}
    ext |= {(s["rva"], s["size"]) for s in sections}
    merged: list[list[int]] = []
    for a, sz in sorted(ext):
        b = a + sz
        if merged and a <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], b)
        else:
            merged.append([a, b])
    return [(a, b) for a, b in merged]


def overlaps(claims):
    """Claims covering a shared byte with DIFFERENT extents. One retail COMDAT
    defined by N objects agrees on rva AND size, so it is not an overlap; a
    genuine overlap means at least one of the two extents is the wrong shape."""
    extents = sorted({(c["rva"], c["size"]) for c in claims})
    byext = defaultdict(list)
    for c in claims:
        byext[(c["rva"], c["size"])].append(c)
    out = []
    for i, (a, sz) in enumerate(extents):
        for a2, sz2 in extents[i + 1:]:
            if a2 >= a + sz:
                break
            out.append(((a, sz, byext[(a, sz)]), (a2, sz2, byext[(a2, sz2)])))
    return out


def touched_index():
    """(starts, ends, sites) of the byte ranges retail's code TOUCHES, from the
    access map's own sweep - the join that turns a gap into a verdict."""
    from homm3.verify import data_access as da
    spine, accesses, *_rest = da.analysis()
    rows = sorted((a.target_rva, a.end_rva) for a in accesses
                  if a.form in da.TOUCH and a.width)
    starts, ends, sites = [], [], []
    for lo, hi in rows:
        if starts and lo <= ends[-1]:
            ends[-1] = max(ends[-1], hi)
            sites[-1] += 1
            continue
        starts.append(lo)
        ends.append(hi)
        sites.append(1)
    return spine, starts, ends, sites


def _touched(starts, ends, sites, lo, hi):
    """(bytes touched inside [lo, hi), number of access runs)."""
    i = max(bisect.bisect_right(starts, lo) - 1, 0)
    n = tb = 0
    while i < len(starts) and starts[i] < hi:
        a, b = max(starts[i], lo), min(ends[i], hi)
        if b > a:
            tb += b - a
            n += sites[i]
        i += 1
    return tb, n


def _name_key(claim):
    return (claim["name"].startswith("<"), claim["name"])


def gaps(img, claims, sections=(), touched=None):
    """Every uncovered range strictly between two covered runs, with a verdict.

    Ranges before the first claim and after the last are the library frontier,
    not a modelling defect: territory nobody has attributed, excluded so the
    signal is not drowned."""
    runs = coverage(claims, sections)
    ends, starts = defaultdict(list), defaultdict(list)
    for c in claims:
        starts[c["rva"]].append(c)
        ends[c["rva"] + c["size"]].append(c)
    for s in sections:                            # a section can bound a gap
        row = {"name": f"<section {s['name']}>", "object": s["object"],
               "rva": s["rva"], "size": s["size"]}
        starts[s["rva"]].append(row)
        ends[s["rva"] + s["size"]].append(row)

    tstarts, tends, tsites = touched or ([], [], [])
    out = []
    for (_a1, b1), (a2, _b2) in zip(runs, runs[1:]):
        n = a2 - b1
        pay = img.read(b1, n) or b""
        nz = sum(1 for x in pay if x)
        rel = img.relocs_in(b1, b1 + n)
        ptd = img.refs_to_range(b1, b1 + n)
        # a real datum names the boundary better than the section holding it
        prev = sorted(ends.get(b1, []), key=_name_key)
        nxt = sorted(starts.get(a2, []), key=_name_key)
        tb, tn = _touched(tstarts, tends, tsites, b1, a2)
        if rel and nz:
            verdict = "POINTER"
        elif nz:
            verdict = "NONZERO"
        elif n < MAX_PAD:
            verdict = "PAD"
        else:
            verdict = "ZERO-GAP"
        out.append({
            "rva": b1, "length": n, "section": img.section_name(b1),
            "verdict": verdict, "addressed": len({t for _s, t in ptd}),
            "touched": tb, "sites": tn, "payload_nonzero": nz,
            "relocs": len(rel),
            "prev_object": prev[0]["object"] if prev else "-",
            "prev_name": prev[0]["name"] if prev else "-",
            "next_object": nxt[0]["object"] if nxt else "-",
            "next_name": nxt[0]["name"] if nxt else "-",
            "first_bytes": pay[:16].hex()})
    return out


def model_claims(spine):
    """The Model's named data bindings in `load_claims()` shape - the claim
    authority (the manifest is only what objdiff got to compare)."""
    return [{"name": c.name, "object": c.unit, "storage": c.space,
             "rva": c.rva, "size": c.extent} for c in spine.claims]


def census():
    """(rows, claims, sections, img) - the whole census, one sweep."""
    spine, ts, te, tn = touched_index()
    claims = model_claims(spine) + load_claims()
    sections = load_sections()
    return gaps(spine.img, claims, sections, (ts, te, tn)), claims, sections, \
        spine.img


#: units whose "territory" is the library frontier, not ours
FRONTIER_UNITS = frozenset({"library_data", "-", ""})


def gate_rows(rows):
    """The FATAL subset. Four conditions, each one earning its place:

      touched     retail's own code reads or writes those bytes, so they are
                  not alignment padding;
      NONZERO/    retail's payload there is content, and cl's inter-symbol
      POINTER     padding is zero;
      same unit   BOTH neighbours are claims of ONE live unit - the range sits
                  inside territory we have attributed, so it is a lost field
                  and not the un-attributed library frontier (which is most of
                  the census and is a worklist, never a breaker);
      not .idata  the import tables are the linker's storage; a source pin on
                  an IAT slot is the access map's `import-slot` finding, and
                  double-reporting it here would just make two gates red for
                  one defect.
    """
    return [r for r in rows
            if r["touched"] and r["verdict"] in ("NONZERO", "POINTER")
            and r["section"] != ".idata"
            and r["prev_object"] == r["next_object"]
            and r["prev_object"] not in FRONTIER_UNITS]


def gate_findings() -> list[str]:
    rows, claims, _sections, _img = census()
    out = [f"data-coverage: 0x{r['rva']:06x}+0x{r['length']:x} {r['verdict']} "
           f"is touched by retail ({r['sites']} site(s), {r['touched']} B) but "
           f"no claim of {r['prev_object']} covers it - between "
           f"{r['prev_name'][:40]} and {r['next_name'][:40]}"
           for r in gate_rows(rows)]
    for (a, sz, c1), (a2, sz2, c2) in overlaps(claims):
        out.append(f"data-coverage: OVERLAP 0x{a:06x}+0x{sz:x} "
                   f"{c1[0]['name'][:40]} vs 0x{a2:06x}+0x{sz2:x} "
                   f"{c2[0]['name'][:40]} - one extent is the wrong shape")
    return out


def _summary(rows, claims, sections):
    tally = Counter(r["verdict"] for r in rows)
    by = Counter()
    touched = Counter()
    for r in rows:
        by[r["verdict"]] += r["length"]
        touched[r["verdict"]] += r["touched"]
    covered = sum(b - a for a, b in coverage(claims, sections))
    print(f"enrolled claims {len(claims)} "
          f"({len({(c['rva'], c['size']) for c in claims})} distinct extents) "
          f"+ {len(sections)} placed sections, covering {covered} B")
    print(f"uncovered interior ranges: {len(rows)}, {sum(by.values())} B")
    for v in ("POINTER", "NONZERO", "ZERO-GAP", "PAD"):
        print(f"  {v:<9} {tally[v]:5} ranges  {by[v]:9} B  "
              f"{sum(r['addressed'] for r in rows if r['verdict'] == v):5} "
              f"addressed  {touched[v]:7} B TOUCHED by retail")


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm3 verify data-coverage",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tsv", nargs="?", const=GAPS_TSV,
                    help="write the join-shaped gap census")
    ap.add_argument("--all-bytes", action="store_true",
                    help="account for every file/image byte and compare raw initializers")
    ap.add_argument("--overlaps", action="store_true",
                    help="list claims whose extents share a byte, and exit")
    ap.add_argument("--verdict",
                    help="only rows with this verdict (PAD/ZERO-GAP/NONZERO/POINTER)")
    ap.add_argument("--touched-only", action="store_true",
                    help="only gaps retail's code actually reads or writes")
    ap.add_argument("--min-len", type=int, default=0,
                    help="ignore gaps shorter than this many bytes")
    ap.add_argument("--near", help="claims and payload around one rva")
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on a gated gap or an overlap")
    ap.add_argument("--limit", type=int, default=40,
                    help="cap the printed worklist")
    a = ap.parse_args(argv)

    if a.all_bytes:
        from homm3.verify.byte_accounting import report
        doc = report()
        for domain, totals in doc['totals'].items():
            print(f"{domain}: " + ', '.join(f'{k}={v:,} B' for k, v in totals.items()))
        print('initializers: ' + ', '.join(
            f'{k}={v}' for k, v in Counter(
                r['verdict'] for r in doc['initializers']).items()))
        print(f"reports: {BUILD / 'gen/data_coverage.json'}")
        defects = (doc['model_violations'] or any(
            t.get('overlap', 0) or t.get('missing', 0) for t in doc['totals'].values()) or any(
            r['verdict'] != 'exact' for r in doc['initializers']))
        return int(bool(a.gate and defects))

    if a.near:
        return _near(int(a.near, 0))

    rows, claims, sections, _img = census()
    if a.overlaps:
        ov = overlaps(claims)
        print(f"overlapping claims (different extents sharing a byte): "
              f"{len(ov)}")
        for (a1, s1, c1), (a2, s2, c2) in ov[:a.limit]:
            print(f"  0x{a1:06x}+0x{s1:<5x} {c1[0]['name'][:44]:44} "
                  f"[{c1[0]['object']}]")
            print(f"  0x{a2:06x}+0x{s2:<5x} {c2[0]['name'][:44]:44} "
                  f"[{c2[0]['object']}]")
        return 1 if ov else 0

    if a.tsv:
        changed = write_tsv(
            a.tsv, ["# GENERATED by homm3.verify.data_coverage - retail bytes "
                    "no enrolled datum covers, joined with the access map."],
            GAP_COLUMNS,
            [[f"0x{r['rva']:06x}", r["length"], r["section"], r["verdict"],
              r["addressed"], r["touched"], r["sites"], r["payload_nonzero"],
              r["relocs"], r["prev_object"], r["prev_name"], r["next_object"],
              r["next_name"], r["first_bytes"]] for r in rows])
        print(f"wrote {a.tsv} ({len(rows)} rows, join key rva+length, "
              f"{'updated' if changed else 'unchanged'})")

    _summary(rows, claims, sections)
    sel = [r for r in rows if r["length"] >= a.min_len
           and (not a.verdict or r["verdict"] == a.verdict)
           and (not a.touched_only or r["touched"])]
    print("\nworklist (verdict != PAD, TOUCHED first, then addressed):")
    work = sorted([r for r in sel if r["verdict"] != "PAD"],
                  key=lambda r: (-r["touched"], -r["addressed"],
                                 -r["payload_nonzero"]))
    for r in work[:a.limit]:
        print(f"  0x{r['rva']:06x} len={r['length']:<6} {r['verdict']:<8} "
              f"touched={r['touched']:<5} addr={r['addressed']:<4} "
              f"nz={r['payload_nonzero']:<5} rel={r['relocs']:<4} "
              f"{r['prev_object']} | {r['next_object']}")
        print(f"      {r['prev_name'][:46]:46} -> {r['next_name'][:46]}")
    if a.gate:
        bad = gate_findings()
        for b in bad:
            print(b)
        return 1 if bad else 0
    return 0


def _near(rva, span=0x80):
    """The claims, sections and retail payload around one address."""
    from homm3.sema.image import retail
    img = retail()
    claims, sections = load_claims(), load_sections()
    lo, hi = rva - span, rva + span
    print(f"=== claims overlapping [0x{lo:06x}, 0x{hi:06x}) ===")
    for c in sorted(claims, key=lambda c: (c["rva"], c["name"])):
        if c["rva"] + c["size"] <= lo or c["rva"] >= hi:
            continue
        print(f"  0x{c['rva']:06x}+0x{c['size']:<5x} {c['storage']:<8} "
              f"{c['name'][:46]:46} [{c['object']}]")
    hits = [s for s in sections if s["rva"] < hi and s["rva"] + s["size"] > lo]
    if hits:
        print("=== placed candidate sections here ===")
        for s in sorted(hits, key=lambda s: s["rva"]):
            print(f"  0x{s['rva']:06x}+0x{s['size']:<5x} {s['name']:<10} "
                  f"{s['object']}")
    print("=== retail payload ===")
    pay = img.read(lo, hi - lo) or b""
    rel = {s for s, _t in img.relocs_in(lo, hi)}
    ptd = {t for _s, t in img.refs_to_range(lo, hi)}
    for off in range(0, len(pay), 16):
        at = lo + off
        marks = "".join("R" if (at + i) in rel else
                        "*" if (at + i) in ptd else " " for i in range(16))
        print(f"  0x{at:06x}  {pay[off:off + 16].hex(' '):47}  |{marks}|")
    print("  (R = retail relocates this word, * = something points AT it)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
