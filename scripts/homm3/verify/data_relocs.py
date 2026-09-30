"""homm3.verify.data_relocs - the data-side wrong-referent sieve (full).

A relocated data word's own bytes are a linker placeholder, so a wrong
REFERENT (a vtable slot bound to the wrong method, an RTTI array pointing at
the wrong ??_R1, a reordered pointer table) moves no byte a diff can see.
Two oracles, descending authority (ported):

  retail   for a datum whose rva the Model pins, the IMAGE's own .reloc
           table is the answer: the set of relocated words inside the extent
           is a fact and each stored value is the address retail points at -
           immune to every naming artefact, ILT thunks chased on both sides.
  paired   for a datum both normalized objects define, each side's referent
           resolves to an RVA and the ADDRESSES are compared; a word neither
           side resolves is UNRESOLVED, reported, never claimed a defect.

Verdicts: WRONG / EXTRA (we relocate a word retail leaves alone) / MISSING /
ADDEND. The /GX EH state tables route to their own report (a divergence
there is a code-shape fact owned by the eh-frame sieve, not a referent we
chose). Also checked: live units objdiff cannot score (no delinked object or
paired to the dummy) and enrolled payloads carved into objects objdiff never
opens - both silent-unscored classes, both FATAL.

    python3 -m homm3.verify.data_relocs [--unit U] [--calibrate] [--gate]
"""

from __future__ import annotations

import json
import sys
from collections import Counter

from homm3.core.paths import BUILD
from homm3.delink.coffx import Obj
from homm3.walls import pairscan
from homm3.walls.pairscan import DIR32, canon

NORM = BUILD / "objdiff/normalized"
OBJDIFF_JSON = BUILD / "objdiff/objdiff.json"
DATA_MANIFEST = BUILD / "gen/delink_data_manifest.tsv"

MEM_EXECUTE = 0x20000000
MEM_DISCARDABLE = 0x02000000
LNK_INFO = 0x00000200
LNK_REMOVE = 0x00000800
CNT_INITIALIZED = 0x00000040
CNT_UNINITIALIZED = 0x00000080

KNOWN_ORPHAN_UNITS: frozenset = frozenset()
KNOWN_UNPAIRED_UNITS: frozenset = frozenset()
LIBRARY_HOLDING_UNIT = "library_data"


def _is_data(sec) -> bool:
    ch = sec["characteristics"]
    if ch & (MEM_EXECUTE | MEM_DISCARDABLE | LNK_INFO | LNK_REMOVE):
        return False
    if not ch & (CNT_INITIALIZED | CNT_UNINITIALIZED):
        return False
    return not sec["name"].startswith((".debug", ".drectve"))


class Datum:
    __slots__ = ("name", "section", "start", "lo", "hi", "rel")

    def __init__(self, name, section, start, lo, hi, rel):
        self.name, self.section = name, section
        self.start, self.lo, self.hi = start, lo, hi
        self.rel = rel                 # {rel_offset: (canon_name, addend)}


def _data(obj: Obj, dropped) -> dict[str, Datum]:
    """{canon symbol: Datum}; the FIRST owner also owns the bytes before it
    (a /GR vtable's ??_R4 COL pointer sits at ??_7 - 4), so offsets can be
    negative and still line up across the two sides."""
    out: dict[str, Datum] = {}
    for secnum in range(1, obj.nsec + 1):
        sec = obj.section_table[secnum - 1]
        if not _is_data(sec):
            continue
        owners = obj.section_members(secnum)
        payload = obj.section_payload(secnum)
        size = len(payload) or sec["size"]
        if not owners:
            if obj.typed_relocations(secnum):
                dropped["relocation in a data section with no defined "
                        "symbol"] += len(obj.typed_relocations(secnum))
            continue
        relocs = pairscan.fn_relocs(obj, secnum, 0, size)
        for i, (val, nm, _scl) in enumerate(owners):
            lo = 0 if i == 0 else val
            hi = owners[i + 1][0] if i + 1 < len(owners) else size
            rel = {off - val: (canon(rnm), addend)
                   for off, rnm, ty, addend in relocs
                   if ty == DIR32 and lo <= off < hi}
            key = canon(nm)
            if key in out:
                dropped["symbol defined twice in one object"] += 1
                continue
            out[key] = Datum(key, sec["name"], val, lo - val, hi - val, rel)
    return out


class Row:
    __slots__ = ("unit", "datum", "off", "verdict", "oracle", "base",
                 "target", "clean")

    def __init__(self, unit, datum, off, verdict, oracle, base, target,
                 clean):
        self.unit, self.datum, self.off = unit, datum, off
        self.verdict, self.oracle = verdict, oracle
        self.base, self.target, self.clean = base, target, clean

    def __str__(self):
        at = "" if self.off == 0 else \
            f" {'-' if self.off < 0 else '+'} {abs(self.off):#x}"
        return (f"{self.verdict:<7} [{self.oracle}]  {self.unit}  "
                f"{self.datum}{at}\n            ours   {self.base or '-'}"
                f"\n            retail {self.target or '-'}")


def clean_units() -> set[str]:
    """Units whose every .data/.rdata section objdiff scores at 100.0 - the
    calibration set (a row inside one is a bug in THIS tool)."""
    from homm3.verify import scores as sc
    try:
        doc = json.loads(sc.report_path().read_text())
    except SystemExit:
        return set()
    out = set()
    for u in doc.get("units", []):
        rows = [s for s in u.get("sections", [])
                if s.get("name") in (".data", ".rdata")]
        if rows and all(s.get("fuzzy_match_percent", 0.0) >= 100.0
                        for s in rows):
            out.add(u.get("name", "").split("/")[-1])
    return out


def units_without_a_target() -> list[str]:
    """Live units objdiff cannot score: no delinked object, or paired to the
    dummy (both forms checked - the pairing, not the file, decides)."""
    from homm3 import manifest
    dummy = set()
    if OBJDIFF_JSON.is_file():
        try:
            for unit in json.loads(OBJDIFF_JSON.read_text()).get("units", []):
                if (unit.get("target_path") or "").endswith("dummy.obj"):
                    dummy.add(unit.get("name"))
        except (OSError, json.JSONDecodeError, TypeError):
            pass
    live = {u["unit"] for u in manifest.units()}
    return sorted(u for u in live
                  if (NORM / "base" / f"{u}.obj").is_file()
                  and (u in dummy
                       or not (NORM / "target" / f"{u}.c.obj").is_file()))


def orphan_payloads() -> list[tuple[str, str, str, str]]:
    """Enrolled data carved into an object objdiff never opens."""
    from homm3 import manifest
    if not DATA_MANIFEST.is_file():
        return []
    live = {u["unit"] for u in manifest.units()}
    out = []
    header = None
    for ln in DATA_MANIFEST.read_text(errors="replace").splitlines():
        if not ln.strip() or ln.startswith("#"):
            continue
        cols = ln.split("\t")
        if header is None:
            header = cols
            continue
        row = dict(zip(header, cols))
        unit = (row.get("object") or "").split("/")[-1]
        for suffix in (".c.obj", ".obj", ".c"):
            if unit.endswith(suffix):
                unit = unit[:-len(suffix)]
                break
        if unit == LIBRARY_HOLDING_UNIT:
            continue
        if unit and unit not in live:
            out.append((unit, row.get("rva", ""), row.get("storage", ""),
                        row.get("name", "")))
    return out


def _ref(name, addend, rva) -> str:
    s = name + (f" + {addend:#x}" if addend else "")
    return f"{s}  (0x{rva:06x})" if rva is not None else f"{s}  (unresolved)"


def scan(unit_filter=None):
    """([Row], unpaired, unresolved, stats, dropped, eh_rows)."""
    from homm3.delink import eh_band
    from homm3.verify.assert_relocs import Resolver
    resolver = Resolver()
    img = resolver.img
    clean = clean_units()
    pinned_size = {canon(b.name): b.size for b in resolver.model.data
                   if b.name and b.size}
    rows, unpaired, unresolved, eh_rows = [], [], [], []
    dropped: Counter = Counter()
    stats: Counter = Counter()

    def rva_of(name, addend=0):
        cands = resolver.resolve_base(name, DIR32, addend)
        return next(iter(cands)) if len(cands) == 1 else None

    for unit, (base, target) in sorted(pairscan.pairs().items()):
        if unit_filter and unit != unit_filter:
            continue
        try:
            bobj = Obj(base)
            tobj = Obj(target)
        except (ValueError, OSError):
            continue
        bdata = _data(bobj, dropped)
        tdata = _data(tobj, dropped)
        stats["units"] += 1
        for nm in sorted(set(bdata) - set(tdata)):
            unpaired.append((unit, "base-only", nm, len(bdata[nm].rel)))
        for nm in sorted(set(tdata) - set(bdata)):
            unpaired.append((unit, "retail-only", nm, len(tdata[nm].rel)))

        for nm in sorted(bdata):
            b = bdata[nm]
            stats["data symbols"] += 1
            pin = rva_of(nm)

            # --- oracle 1: the retail .reloc table ---------------------------
            if pin is not None:
                stats["data symbols pinned"] += 1
                hi = min(b.hi, pinned_size[nm]) if nm in pinned_size else b.hi
                retail = {site - pin: resolver.chase(tgt)
                          for site, tgt in img.relocs_in(pin + b.lo,
                                                         pin + hi)}
                for off in sorted(set(b.rel) | set(retail)):
                    if off >= hi:
                        dropped["word past the datum's pinned size"] += 1
                        continue
                    br, tv = b.rel.get(off), retail.get(off)
                    if br is None:
                        stats["words compared (retail)"] += 1
                        rows.append(Row(unit, nm, off, "MISSING", "retail",
                                        None, _ref("<retail .reloc>", 0, tv),
                                        unit in clean))
                        continue
                    ours = rva_of(*br)
                    if ours is None:
                        unresolved.append((unit, nm, off, br,
                                           ('<retail .reloc>', tv)))
                        continue
                    ours = resolver.chase(ours)
                    stats["words compared (retail)"] += 1
                    if tv is None:
                        rows.append(Row(unit, nm, off, "EXTRA", "retail",
                                        _ref(br[0], br[1], ours), None,
                                        unit in clean))
                    elif tv != ours:
                        rows.append(Row(unit, nm, off, "WRONG", "retail",
                                        _ref(br[0], br[1], ours),
                                        _ref("<retail .reloc>", 0, tv),
                                        unit in clean))
                continue

            # --- oracle 2: the paired delinked object ------------------------
            t = tdata.get(nm)
            if t is None:
                if b.rel:
                    kind = b.section.split("$")[0]
                    dropped[f"{kind}: datum neither pinned nor paired"] += \
                        len(b.rel)
                continue
            stats["data symbols paired"] += 1
            for off in sorted(set(b.rel) | set(t.rel)):
                br, tr = b.rel.get(off), t.rel.get(off)
                ba = rva_of(*br) if br else None
                ta = rva_of(*tr) if tr else None
                if br and tr and br == tr:
                    stats["words compared (paired)"] += 1
                    continue
                if br and tr and (ba is None or ta is None):
                    unresolved.append((unit, nm, off, br, tr))
                    dropped["neither side's referent resolves to an rva"] += 1
                    continue
                stats["words compared (paired)"] += 1
                if br and tr:
                    ba2 = resolver.chase(ba)
                    ta2 = resolver.chase(ta)
                    if ba2 != ta2:
                        verdict = "WRONG"
                    elif br[1] != tr[1]:
                        verdict = "ADDEND"
                    else:
                        continue
                else:
                    verdict = "EXTRA" if br else "MISSING"
                row = Row(unit, nm, off, verdict, "paired",
                          _ref(*br, ba) if br else None,
                          _ref(*tr, ta) if tr else None, unit in clean)
                (eh_rows if eh_band.is_band_data_symbol(nm) else
                 rows).append(row)
    return rows, unpaired, unresolved, stats, dropped, eh_rows


def gate_findings() -> list[str]:
    rows, _unpaired, _unres, stats, _dropped, _eh = scan()
    out = [str(r).replace("\n", " | ") for r in rows]
    if not sum(stats.values()):
        # Nothing was compared: no normalized base/target pairs on disk. That
        # is an unbuilt tree, not a clean referent set.
        out.append("data-relocs: 0 base/target pairs scanned - the "
                   "normalized objdiff copies are absent, so no referent was "
                   "compared at all. Run `homm3 build` and re-run.")
    for u in units_without_a_target():
        if u not in KNOWN_UNPAIRED_UNITS:
            out.append(f"data-relocs: live unit {u!r} has no scored target "
                       f"(no delinked obj, or paired to the dummy) - "
                       f"silently unscored")
    for unit, rva, storage, name in orphan_payloads():
        if unit not in KNOWN_ORPHAN_UNITS:
            out.append(f"data-relocs: enrolled payload {name} ({rva}, "
                       f"{storage}) carved into non-live unit {unit!r} - "
                       f"objdiff never opens it")
    return out


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm3 verify data-relocs",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit",
                    help="scan one unit only")
    ap.add_argument("--calibrate", action="store_true",
                    help="list the rows landing on 100%%-clean sections "
                         "(false positives)")
    ap.add_argument("--unpaired", action="store_true",
                    help="list relocated data symbols only one side defines")
    ap.add_argument("--unresolved", action="store_true",
                    help="list words neither side resolves to an address")
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on any defect row")
    a = ap.parse_args(argv)

    rows, unpaired, unresolved, stats, dropped, eh_rows = scan(a.unit)
    fp = [r for r in rows if r.clean]
    print("stats: " + ", ".join(f"{k}={v}" for k, v in sorted(stats.items())))
    if dropped:
        print("filtered: " + ", ".join(f"{k}={v}"
                                       for k, v in sorted(dropped.items())))
    print(f"defect rows: {len(rows)}  (on sections currently scored 100%: "
          f"{len(fp)})  eh-band rows routed: {len(eh_rows)}  "
          f"unresolved: {len(unresolved)}  unpaired: {len(unpaired)}")
    if a.calibrate:
        for r in fp:
            print(r)
        return 0
    if a.unpaired:
        for unit, side, nm, n in unpaired:
            print(f"  {unit:<20} {side:<12} {nm}  ({n} reloc(s))")
        return 0
    if a.unresolved:
        for unit, nm, off, br, tr in unresolved:
            print(f"  {unit:<20} {nm} +{off:#x}  ours {br}  retail {tr}")
        return 0
    for r in rows:
        print(r)
    extra = gate_findings()[len(rows):]
    for e in extra:
        print(e, file=sys.stderr)
    if (rows or extra) and a.gate:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
