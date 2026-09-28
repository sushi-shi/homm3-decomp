"""homm3.verify.data_tu_order - the DATA analog of tu_order (normal tier).

Each retail .obj contributes one contiguous run to EACH of .rdata/.data/.bss,
so every ordinary global a TU defines falls inside that TU's own per-storage
band. A definition strictly inside ANOTHER TU's same-storage band is either
misplaced or evidence the partition is wrong.

Rebuilt on the Model: the joined rows are the winning `src`-channel data
bindings of ordinary census kind (''), whose `space` column is the PE-derived
storage class - the old delink-manifest + PE-fallback plumbing is gone, and
compiler-generated kinds (string/vtable/rtti/copy/common/fppool: linker-
pooled, not linearly attributed) never enter the band model. Bands whose
extent swallows >= POOL_THRESHOLD other units' defs are pools, exempt.

Ratcheted against config/cleanliness/data-tu-order-baseline.tsv (keyed
rva/owner/container/storage; the gate never writes - bless with --update).

    python3 -m homm3.verify.data_tu_order [--gate] [--update]
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from homm3.core.paths import CONFIG

BASELINE = CONFIG / "cleanliness/data-tu-order-baseline.tsv"
POOL_THRESHOLD = 4


def _unit_basename() -> dict[str, str]:
    """unit stem -> source basename (the committed baseline's spelling)."""
    from homm3 import manifest
    return {u["unit"]: Path(u["source"]).name for u in manifest.units()}


def _short(name: str) -> str:
    from homm3.sema.index import short_name
    s = short_name(name)
    return s.rsplit("::", 1)[-1] if s else name


def crossings():
    """([(rva, name, owner_base, container_base, storage)], n_defs, pools)."""
    from homm3.model import resolve
    base_of = _unit_basename()
    defs = []          # (rva, unit, name, storage)
    for b in resolve().data:
        if b.channel != "src" or b.kind != "" or not b.unit:
            continue
        defs.append((b.rva, b.unit, b.name, b.space))

    byband: dict[tuple[str, str], list[int]] = {}
    for rva, unit, _name, storage in defs:
        byband.setdefault((unit, storage), []).append(rva)
    bands = {key: (min(v), max(v), len(v)) for key, v in byband.items()}

    contains: dict[tuple[str, str], set] = {}
    foreign_inside: dict[tuple[str, str], int] = {}
    rows = []
    for rva, unit, name, storage in sorted(defs):
        for (cunit, cstorage), (lo, hi, _cnt) in bands.items():
            if cunit == unit or cstorage != storage:
                continue
            if lo < rva < hi:
                contains.setdefault((cunit, cstorage), set()).add(unit)
                foreign_inside[(cunit, cstorage)] = \
                    foreign_inside.get((cunit, cstorage), 0) + 1
                rows.append((rva, unit, name, cunit, storage))
    pools = set()
    for key, (lo, hi, cnt) in bands.items():
        if foreign_inside.get(key, 0) >= cnt \
                or len(contains.get(key, ())) >= POOL_THRESHOLD:
            pools.add(key)
    real = [(rva, _short(name), base_of.get(owner, owner),
             base_of.get(cont, cont), storage)
            for rva, owner, name, cont, storage in rows
            if (cont, storage) not in pools]
    return real, len(defs), pools


def _key(c):
    rva, _name, owner, cont, storage = c
    return (f"{rva:#010x}", owner, cont, storage)


def load_baseline() -> set:
    keys = set()
    if not BASELINE.is_file():
        return keys
    for ln in BASELINE.read_text().splitlines():
        ln = ln.split("#", 1)[0].strip()
        if not ln:
            continue
        parts = ln.split("\t")
        if len(parts) == 5:   # rva name owner container storage
            keys.add((parts[0], parts[2], parts[3], parts[4]))
    return keys


def write_baseline(real) -> None:
    with BASELINE.open("w") as fh:
        fh.write("# data-tu-order baseline: ordinary-data defs that land "
                 "inside another\n# TU's same-storage band (known scattered "
                 "singletons / accepted homes).\n"
                 "# Regenerate: python3 -m homm3.verify.data_tu_order "
                 "--update\n"
                 "# rva\tname\towner_cpp\tcontainer_cpp\tstorage\n")
        for rva, name, owner, cont, storage in sorted(real):
            fh.write(f"{rva:#010x}\t{name}\t{owner}\t{cont}\t{storage}\n")


def gate_findings() -> list[str]:
    real, n_defs, _pools = crossings()
    base = load_baseline()
    out = []
    if not n_defs:
        # Zero defs means zero bands means zero crossings by construction:
        # a green with nothing behind it.
        out.append("data-tu-order: the Model resolved 0 ordinary src data "
                   "definitions, so no band was built and 0 crossings is "
                   "vacuous. Run `homm3 build` and re-run.")
    for c in sorted(real):
        if _key(c) not in base:
            rva, name, owner, cont, storage = c
            out.append(f"data-tu-order: NEW interleave {rva:#010x} {name} in "
                       f"{owner} INSIDE {cont} .{storage} band - home the def "
                       f"in its real owner TU, or bless a proven scattered "
                       f"singleton with --update")
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="homm3 verify data-tu-order",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on a crossing that is not in the committed baseline")
    ap.add_argument("--update", action="store_true",
                    help="MANUAL bless: rewrite the crossing baseline")
    a = ap.parse_args(argv)

    real, n_defs, pools = crossings()
    if a.update:
        write_baseline(real)
        print(f"data-tu-order: baseline blessed ({len(real)} crossings)")
        return 0
    base = load_baseline()
    new = [c for c in real if _key(c) not in base]
    stale = base - {_key(c) for c in real}
    print(f"data-tu-order: {n_defs} ordinary src data defs, {len(pools)} "
          f"pool band(s), {len(real)} crossing(s) "
          f"({len(base)} baselined, {len(new)} NEW"
          + (f", {len(stale)} baseline row(s) now clean)" if stale else ")"))
    if new:
        for rva, name, owner, cont, storage in sorted(new):
            print(f"   {rva:#010x} {name:28s} in {owner:30s} INSIDE {cont} "
                  f".{storage} band", file=sys.stderr)
        return 1 if a.gate else 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
