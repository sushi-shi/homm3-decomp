"""homm3.vc6.atlas - the headless-Ghidra C2.DLL map (phase 2).

`homm3 vc6 atlas --regen` builds a regenerable atlas of the VC6 back end
good enough to navigate the optimizer by translation unit and to locate its
global state - the substrate the inliner-model and regalloc phases consume.
NOT a decompilation.

Pipeline (pyghidra in-process; the pattern proven by the retired carve
driver and recover_structs.py - analyzeHeadless cannot run .py postScripts
in this setup):
  1. ghidra_scripts/import_c2.py   hash-gated import + auto-analysis into
                                   the gitignored build/re/vc6/ project
  2. ghidra_scripts/tu_partition.py  48 ICE source-path strings -> anchored
                                   functions -> raw funcs/anchors tables
  3. ghidra_scripts/globals_map.py .bssbe/.databe reference census -> raw
  4. post-process (here, no Ghidra): bracket propagation between anchors
     (VC6 links objects contiguously) -> evidence/vc6/c2-tu-map.tsv;
     reader/writer fold + TU join -> evidence/vc6/c2-globals.tsv
  5. corroboration: every anchor re-proven from RAW BYTES (the string VA
     as an imm32 inside the function's byte range, via _toolchain.Binary -
     no Ghidra), plus a negative control proving the checker can fail.

Determinism: all rows stably sorted; two --regen runs against the same
analyzed project produce byte-identical TSVs (the provenance date is the
only same-day-stable header field).

rc: 0 = atlas written and anchors self-consistent, 1 = written but some
anchor failed corroboration, 2 = error (wrong-hash C2.DLL dies here).
"""
from __future__ import annotations

import importlib.util
from pathlib import Path

from homm3.vc6 import _common, _toolchain

TU_MAP = _common.EVIDENCE / "c2-tu-map.tsv"
GLOBALS = _common.EVIDENCE / "c2-globals.tsv"

_SCRIPTS = Path(__file__).resolve().parent / "ghidra_scripts"


def _load_script(name: str):
    """Load a ghidra_scripts module by path (that directory has no __init__:
    the scripts are in-Ghidra headless tools, not package members)."""
    spec = importlib.util.spec_from_file_location(
        f"homm3_vc6_ghidra_{name}", _SCRIPTS / f"{name}.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def _read_raw(path: Path) -> list[dict]:
    if not path.is_file():
        _common.die(f"missing raw table {path} - the Ghidra pass did not run")
    lines = [l.rstrip("\n") for l in path.read_text().splitlines()
             if l and not l.startswith("#")]
    header = lines[0].split("\t")
    return [dict(zip(header, l.split("\t"))) for l in lines[1:]]


def _write_evidence(path: Path, extra: list[str], header: list[str],
                    rows) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w") as fh:
        for line in _common.provenance("homm3.vc6.atlas", extra):
            fh.write(line + "\n")
        fh.write("\t".join(header) + "\n")
        for row in rows:
            fh.write("\t".join(str(c) for c in row) + "\n")


def _c2_provenance() -> list[str]:
    sha, size = _toolchain.PINNED["C2.DLL"]
    return [f"# subject: C2.DLL sha256={sha} size={size} "
            "(12.00.8447, image base 0x10700000)"]


# ---------------------------------------------------------------------------
# post-processing
# ---------------------------------------------------------------------------
def _build_tu_map(funcs: list[dict], anchors: list[dict]):
    """The bracket propagation. Returns (rows, stats, anchor_of, dropped).

    Anchor hygiene before propagation (both cases measured on this DLL):
    - a function whose sites name MORE THAN ONE TU (the compiler sources
      carry #line-style generated code: two coff.c ICE blocks sit physically
      inside other TUs' regions) resolves by SITE MAJORITY; a tie drops the
      function from anchoring.
    - an anchor whose TU contradicts BOTH anchored neighbors while those
      neighbors agree with each other is demoted as an out-of-place outlier
      (same #line cause) - the address region's owner is what a link-order
      map needs.

    VC6 links objects contiguously, so between two consecutive anchored
    functions of the SAME TU every function belongs to that TU
    (confidence=bracket). Across a TU boundary the gap is ambiguous: those
    functions get "prevTU|nextTU" - honest, still parseable.
    """
    weight: dict[int, dict[str, int]] = {}
    for row in anchors:
        if row["func_rva"] == "-":
            continue
        rva = int(row["func_rva"], 16)
        n_sites = max(int(row["n_ref"]), int(row["n_imm"]))
        weight.setdefault(rva, {})
        weight[rva][row["tu"]] = weight[rva].get(row["tu"], 0) + n_sites

    anchor_of: dict[int, str] = {}
    dropped: dict[int, str] = {}
    for rva, tus in weight.items():
        ranked = sorted(tus.items(), key=lambda kv: (-kv[1], kv[0]))
        if len(ranked) > 1 and ranked[0][1] == ranked[1][1]:
            dropped[rva] = "tie:" + "/".join(t for t, _ in ranked)
            continue
        if len(ranked) > 1:
            dropped[rva] = f"majority:{ranked[0][0]}-over-" + \
                           "/".join(t for t, _ in ranked[1:])
        anchor_of[rva] = ranked[0][0]

    # sequence-outlier demotion: isolated anchor contradicting both
    # neighbors. Decided against a frozen snapshot (no cascade), deleted
    # after the sweep.
    seq = sorted(anchor_of)
    demote: dict[int, str] = {}
    for i in range(1, len(seq) - 1):
        rva = seq[i]
        left, right = anchor_of[seq[i - 1]], anchor_of[seq[i + 1]]
        if left == right != anchor_of[rva]:
            demote[rva] = f"outlier:{anchor_of[rva]}-inside-{left}"
    for rva in demote:
        del anchor_of[rva]
    dropped.update(demote)

    ordered = sorted((int(f["entry_rva"], 16), int(f["size"])) for f in funcs)
    n = len(ordered)
    next_tu: list[str | None] = [None] * n
    later = None
    for i in range(n - 1, -1, -1):
        if ordered[i][0] in anchor_of:
            later = anchor_of[ordered[i][0]]
        next_tu[i] = later

    rows, stats = [], {"anchor": 0, "bracket": 0, "gap": 0, "edge": 0}
    prev = None
    for i, (rva, size) in enumerate(ordered):
        if rva in anchor_of:
            tu, conf, ev = anchor_of[rva], "anchor", "ICE-string"
            stats["anchor"] += 1
            prev = tu
        else:
            nxt = next_tu[i]
            if prev is not None and prev == nxt:
                tu = prev
                stats["bracket"] += 1
            elif prev is not None and nxt is not None:
                tu = f"{prev}|{nxt}"
                stats["gap"] += 1
            else:
                tu = f"{prev or '?'}|{nxt or '?'}"
                stats["edge"] += 1
            conf, ev = "bracket", "locality"
        rows.append(("0x%x" % rva, size, tu, conf, ev))
    return rows, stats, anchor_of, dropped


def _build_globals(raw: list[dict], funcs: list[dict], tu_of: dict[int, str]):
    """Fold the reference census per address; join writers to their TUs.

    Site -> function attribution is by link-order interval over the sorted
    function entries (same rationale as the TU anchors: Ghidra's fragmented
    bodies scatter getFunctionContaining, the raw func_rva column is kept in
    the raw table as a diagnostic only).
    """
    import bisect
    entries = sorted(int(f["entry_rva"], 16) for f in funcs)

    def owner(site: int) -> int | None:
        i = bisect.bisect_right(entries, site) - 1
        return entries[i] if i >= 0 else None

    per: dict[int, dict] = {}
    for row in raw:
        rva = int(row["target_rva"], 16)
        rec = per.setdefault(rva, {"section": row["section"], "size": 0,
                                   "readers": set(), "writers": set()})
        rec["size"] = max(rec["size"], int(row["width"]))
        if row["channel"] == "data-ref":
            continue  # reference from data, not from a function
        func = owner(int(row["site_rva"], 16))
        if func is None:
            continue
        kind = row["kind"]
        # address-taken ("addr") counts as a read: the pointer escapes.
        if kind in ("r", "rw", "addr"):
            rec["readers"].add(func)
        if kind in ("w", "rw"):
            rec["writers"].add(func)
    rows = []
    for rva in sorted(per):
        rec = per[rva]
        tus = sorted({tu_of.get(f, "?") for f in rec["writers"]})
        rows.append(("0x%x" % rva, rec["section"], rec["size"],
                     len(rec["readers"]), len(rec["writers"]),
                     ";".join(tus) if tus else "-"))
    return rows


# ---------------------------------------------------------------------------
# corroboration (the negative-control-bearing check)
# ---------------------------------------------------------------------------
def _corroborate(binary, tu_rows, funcs, tu_partition) -> tuple[int, int]:
    """Re-prove every anchor row from raw bytes, with no Ghidra involvement:
    the TU string's VA must occur as a little-endian imm32 inside the
    function's link-order span [entry, next_entry). Dies if the negative
    control cannot fail."""
    import bisect
    entries = sorted(int(f["entry_rva"], 16) for f in funcs)
    text_end = max(int(f["entry_rva"], 16) + int(f["size"]) for f in funcs)
    sites_of = {tu: set(tu_partition.imm_sites(binary, va))
                for _, va, _, tu in tu_partition.tu_strings(binary)}

    def holds(rva: int, tu: str) -> bool:
        i = bisect.bisect_right(entries, rva)
        hi = entries[i] if i < len(entries) else text_end
        return any(rva <= s < hi for s in sites_of.get(tu, ()))

    anchors = [(int(r[0], 16), r[2]) for r in tu_rows if r[3] == "anchor"]
    ok = sum(1 for rva, tu in anchors if holds(rva, tu))

    # negative control: the checker MUST reject a deliberately wrong pairing.
    control_ok = False
    if anchors:
        rva, tu = anchors[0]
        wrong = next((t for t in sorted(sites_of) if t != tu
                      and not holds(rva, t)), None)
        control_ok = wrong is not None
    if not control_ok:
        _common.die("corroboration negative control failed: the checker "
                    "could not reject a wrong (function, TU) pairing")
    return ok, len(anchors)


# ---------------------------------------------------------------------------
def run(args) -> int:
    regen = getattr(args, "regen", False)
    reimport = getattr(args, "reimport", False)
    if not regen:
        for path in (TU_MAP, GLOBALS):
            if path.is_file():
                n = sum(1 for l in path.read_text().splitlines()
                        if l and not l.startswith("#"))
                print(f"[atlas] {path.relative_to(_common.REPO)}: {n - 1} rows")
            else:
                print(f"[atlas] {path.relative_to(_common.REPO)}: MISSING - "
                      "run `homm3 vc6 atlas --regen`")
                return 1
        return 0

    binary = _toolchain.Binary("C2.DLL")  # the hash gate, before anything
    import_c2 = _load_script("import_c2")
    tu_partition = _load_script("tu_partition")
    globals_map = _load_script("globals_map")

    gproject, program = import_c2.open_program(reimport=reimport)
    try:
        tu_partition.run(program, binary, import_c2.RAW_DIR)
        globals_map.run(program, import_c2.RAW_DIR)
    finally:
        import_c2.close_program(gproject, program)

    funcs = _read_raw(import_c2.RAW_DIR / "c2_funcs_raw.tsv")
    anchors = _read_raw(import_c2.RAW_DIR / "c2_anchors_raw.tsv")
    graw = _read_raw(import_c2.RAW_DIR / "c2_globals_raw.tsv")

    tu_rows, stats, anchor_of, dropped = _build_tu_map(funcs, anchors)
    for rva in sorted(dropped):
        print(f"[atlas] anchor hygiene 0x{rva:x}: {dropped[rva]}")
    _write_evidence(
        TU_MAP, _c2_provenance() + [
            "# .text partition by translation unit. anchor = the function",
            "# itself references its TU's ICE source-path string; bracket =",
            "# assigned by link-order locality between anchors. Ambiguous",
            "# boundary gaps are spelled prevTU|nextTU.",
            f"# anchored={stats['anchor']} bracketed={stats['bracket']} "
            f"gap={stats['gap']} edge={stats['edge']} "
            f"hygiene-dropped={len(dropped)}",
        ],
        ["func_rva", "func_size", "tu", "confidence", "evidence"], tu_rows)

    tu_of = {int(r[0], 16): r[2] for r in tu_rows}
    global_rows = _build_globals(graw, funcs, tu_of)
    _write_evidence(
        GLOBALS, _c2_provenance() + [
            "# every referenced address in the back-end state sections",
            "# .bssbe (zero-init) and .databe (init). size = widest direct",
            "# access seen (0 = only address-taken/indexed). n_readers",
            "# counts address-taken as a read; writer_tus joins writers",
            "# through c2-tu-map.tsv (prevTU|nextTU where ambiguous).",
        ],
        ["rva", "section", "size", "n_readers", "n_writers", "writer_tus"],
        global_rows)

    ok, total = _corroborate(binary, tu_rows, funcs, tu_partition)
    n_tus = len(set(anchor_of.values()))
    print(f"[atlas] wrote {TU_MAP.relative_to(_common.REPO)} "
          f"({len(tu_rows)} functions, {n_tus} TUs anchored) and "
          f"{GLOBALS.relative_to(_common.REPO)} ({len(global_rows)} globals)")
    print(f"[atlas] corroboration: {ok}/{total} anchor rows re-proven from "
          "raw bytes (negative control passed)")
    if ok != total:
        print(f"[atlas] WARNING: {total - ok} anchors did NOT corroborate - "
              "inspect build/re/vc6/raw/c2_anchors_raw.tsv")
        return 1
    return 0


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--regen", action="store_true")
    ap.add_argument("--reimport", action="store_true",
                    help="delete and re-create the Ghidra project first")
    return run(ap.parse_args(argv))


if __name__ == "__main__":
    import sys
    sys.exit(main())
