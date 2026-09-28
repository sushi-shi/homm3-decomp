#!/usr/bin/env python3
"""homm3.model - the one join: censuses x claim channels -> the inventory.

    homm3 model

Consumes the typed records of homm3.retail_labels (source fragments,
provider tables, IAT slots - all parse-only) plus the function census, and
writes the two generated deliverables every downstream consumer reads:

    build/gen/symbol_names.csv    the synth-PDB inventory
                                  (rva,name,unit,size,kind,provenance)
    build/gen/compgen_claims.tsv  the source-owned compiler-function
                                  manifest (normalization's $E<n> input)

THE RULE (gruntz): this module is the ONLY place labeling policy lives -
the parsers never join, the model never parses. Policy, preserved
verbatim from the pre-port build.labels monolith:

  * authority order per rva, first writer wins: src claims > zlib-map >
    runtime-map > working-label (functions); reloc-alias > vtable census >
    IAT slots > reloc-target dense names (data);
  * the scan-order dedup of label-grade names replays over the fragments'
    RAW declarator spellings, then joined base-obj spellings take over;
    a second global pass suffixes remaining label-grade collisions and
    dies on a colliding PROVEN symbol;
  * naming fallbacks: seg_/fn_/vtbl_/const_/data_/bss_ dense spellings;
    vtable class identities come only from the reviewed config census;
  * fatal gates: a claim naming a volatile `$E<n>` ordinal, duplicate
    rvas, duplicate proven names, alias owners contradicting an existing
    row, any universe function left uncovered.

Fragments are extraction's cache: `homm3 delink` refreshes them all
before the model runs; after hand edits to src/ run `homm3 labels` first.
"""

from __future__ import annotations

import csv
import re
import sys
from pathlib import Path
from dataclasses import dataclass, field
from typing import NamedTuple

from homm3.core.tsv import write as write_tsv
from homm3.core.paths import BUILD

from homm3.build.canonicalize_data_symbols import normalize_anon_ns_name
from homm3.core import common
from homm3.retail_labels import censuses, fragments, iat, providers
from homm3.retail_labels import source as labels_source

OUT = common.HOMM3_DIR / "build/gen/symbol_names.csv"
COMPGEN_OUT = common.HOMM3_DIR / "build/gen/compgen_claims.tsv"

BUCKET_SHIFT = 16
VOLATILE_E_RE = re.compile(r"^_?\$E[0-9]+$")

#: Channels whose claims name a COMPILER-GENERATED POOLED datum. One such
#: datum is a single image-wide allocation - the linker folds `""`,
#: `"%s %s"`, `"\n"` to one address - so every TU that spells the literal
#: claims the SAME rva, legitimately. Repeated claims therefore coalesce
#: (the first unit in sorted-stem scan order names it) instead of firing
#: the duplicate-rva gate, which exists to catch two different data
#: fighting over one address.
#:
#: The coalesce is safe only because extraction PROVES the claims agree:
#: the VALUE is the claim, and retail_labels.source's pooled-agreement
#: check reports every site pair that pins one address to two different
#: values. This module never sees the values, so it may not DECIDE
#: agreement - it only acts on a fact extraction has already established.
POOLED_CHANNELS = ("src-DATA_COMPGEN", "src-DATA_COMPGEN_GUARD")

BINDINGS = BUILD / "gen/bindings.tsv"
VIOLATIONS = BUILD / "gen/violations.tsv"


class Binding(NamedTuple):
    rva: int
    size: int
    kind: str
    space: str
    name: str
    unit: str
    channel: str
    aliases: tuple
    also: tuple = ()


class Model(NamedTuple):
    functions: list[Binding]
    data: list[Binding]
    violations: list[str]

    def claimed(self, kind=None):
        rows = (self.functions if kind == 'func' else self.data if kind == 'data'
                else self.functions + self.data)
        return [b for b in rows if b.channel]


def unmaterialized(claims):
    """Source functions no claiming object emits; absence is a coverage gap."""
    from collections import Counter, defaultdict
    from homm3.core.coff import Coff
    from homm3.core.msvc_names import mask
    groups, objects = defaultdict(list), {}
    for c in claims:
        if c.kind == 'func':
            groups[(c.rva, mask(c.name))].append(c)
    counts = Counter(rva for rva, name in groups)
    multiple = {rva for rva, count in counts.items() if count > 1}
    absent = []
    for (rva, name), rows in groups.items():
        if rva in multiple:
            continue
        verdicts = []
        for row in rows:
            if row.unit not in objects:
                path = BUILD / 'objdiff/base' / f'{row.unit}.obj'
                objects[row.unit] = ({mask(n) for n in Coff(path).code_names()}
                                     if path.is_file() else None)
            emitted = objects[row.unit]
            verdicts.append(None if emitted is None else name in emitted)
        if None not in verdicts and not any(verdicts):
            absent.append(rows[0])
    return absent


def resolve(rows=None) -> Model:
    """The Gruntz Binding model, fed by HoMM3's admitted inventories.

    HoMM3 has exact function spans but no complete independent data-boundary
    census. Unsized relocation targets therefore stay zero-extent unclaimed
    anchors. They must never consume the gap to the next pointer or label.
    Source data obtains its size from the annotated declaration under VC6's
    ABI; the gap and access checks adjudicate incomplete source extents.
    """
    from homm3.core.pe import image
    from homm3.core import msvc_names
    rows = _collect_inventory()[0] if rows is None else rows
    regions = image().data_regions()
    vtable_extents = {r["rva"]: r["count"] * 4 for r in censuses.vtables()}
    claims = {}
    for claim in fragments.all_claims():
        claims.setdefault((claim.kind, claim.rva), []).append(claim)
    internal_rvas = {c.rva for cs in claims.values() for c in cs
                     if c.meta.get('internal') == '1'}
    channels = {'src-VA': 'src', 'src-VA+ir': 'src', 'src-VA+base': 'src',
                'src-VA_COMPGEN': 'src_compgen', 'zlib-map': 'functions_zlib',
                'zlib-data-map': 'data_zlib',
                'runtime-map': 'functions_static_libs', 'src-DATA': 'src',
                'src-DATA_COMPGEN': 'src_data_compgen',
                'src-DATA_COMPGEN_GUARD': 'data_compgen',
                'vtable': 'data_vtables', 'vtable-name': 'data_vtables'}
    functions, data, violations = [], [], []
    for rva, row in sorted(rows.items()):
        source = claims.get((row['kind'], rva), [])
        channel = channels.get(row['provenance'], '')
        size = int(row['size'] or 0)
        name = row['name'] if channel else ''
        if row['kind'] == 'func':
            functions.append(Binding(rva, size, '', 'text', name, row['unit'],
                                     channel, tuple(source[1:])))
            continue
        kind = 'vtable' if channel == 'data_vtables' else ''
        if channel == 'data_vtables':
            size = vtable_extents.get(rva, 0)
        if row['provenance'] == 'src-DATA':
            sized = [c for c in source if c.size and c.meta.get('type')]
            if not sized:
                violations.append(f'DATA {row["name"]} at {rva:#x} has no typed extent')
                channel, size = '', 0
            elif not any(c.meta.get('defined') == '1' for c in sized):
                violations.append(f'DATA {row["name"]} at {rva:#x} is extern-only')
                channel = ''
            else:
                winner = next(c for c in sized if c.meta.get('defined') == '1')
                name, size = winner.name, winner.size
        if not size:
            channel = ''
        space = next((key for key, (lo, hi) in regions.items() if lo <= rva < hi), '')
        if name:
            name = msvc_names.mask(name)
        data.append(Binding(rva, size, kind, space, name if channel else '',
                            row['unit'], channel, tuple(source[1:])))
    # Keep TU-local families distinct at separate retail addresses. This is
    # Gruntz's discriminator, not a source alias or extra allocation.
    from collections import Counter
    duplicates = Counter(b.name for b in data if b.name)
    result = []
    for b in data:
        if b.name and duplicates[b.name] > 1:
            distinct = msvc_names.discriminate(b.name, b.rva, internal=b.rva in internal_rvas)
            if distinct:
                b = b._replace(name=distinct)
            else:
                violations.append(f'data identity {b.name} binds multiple retail addresses')
        result.append(b)
    return Model(functions, result, violations)


def serialize(model: Model):
    """Gruntz's canonical bindings/violations tables, write-if-changed."""
    header = ['rva', 'size', 'kind', 'space', 'name', 'unit', 'channel',
              'also_units', 'aliases']
    rows = [[f'0x{b.rva:08x}', f'0x{b.size:x}', b.kind, b.space, b.name,
             b.unit, b.channel, ';'.join(b.also),
             ';'.join(f'{a.channel}|{a.name}|0x{a.size or 0:x}|{a.unit}'
                      for a in b.aliases)] for b in model.functions + model.data]
    changed = write_tsv(BINDINGS, ['# GENERATED by homm3.model - resolved claim set.'],
                        header, rows)
    write_tsv(VIOLATIONS, ['# GENERATED by homm3.model.'],
              ['violation'], [[v] for v in model.violations])
    return changed


def choose_carrier(rva: int, emitters: set[str], banked: dict[int, str],
                   anchors: list[tuple[int, str]],
                   reviewed: dict[int, str] | None = None) -> str | None:
    if reviewed and rva in reviewed:
        return reviewed[rva]
    if banked.get(rva) in emitters:
        return banked[rva]
    if len(emitters) == 1:
        return next(iter(emitters))
    if not emitters:
        return banked.get(rva)
    lower = [a for a in anchors if a[0] < rva]
    upper = [a for a in anchors if a[0] > rva]
    neighbours = set()
    if lower:
        neighbours.add(max(lower)[1])
    if upper:
        neighbours.add(min(upper)[1])
    plausible = neighbours & emitters
    return next(iter(plausible)) if len(plausible) == 1 else banked.get(rva)


@dataclass(frozen=True)
class CarrierPolicy:
    """Reviewed comparison bindings; missing bodies retain their banked carrier."""
    banked: dict[int, str]
    bindings: tuple[tuple[str, int], ...] = ()
    reviewed: dict[int, str] = field(default_factory=dict)

    def choose(self, rva, emitters, banked, anchors):
        return choose_carrier(rva, emitters, banked, anchors, self.reviewed)

    def for_units(self, units) -> dict[int, str]:
        # Filter before resolving duplicate RVAs, matching the ledger's order.
        result = ({rva: unit for unit, rva in self.bindings if unit in units}
                  if self.bindings else
                  {rva: unit for rva, unit in self.banked.items() if unit in units})
        result.update({rva: unit for rva, unit in self.reviewed.items() if unit in units})
        return result


def carrier_policy(baseline, *, reviewed=None) -> CarrierPolicy:
    return CarrierPolicy({row.rva: unit for (unit, _name), row in baseline.items()
                          if row.rva is not None},
                         tuple((unit, row.rva) for (unit, _name), row in baseline.items()
                                   if row.rva is not None), reviewed or {})


def header_data_problems(sites: dict, rows: dict) -> list[str]:
    """The other half of the macro-site completeness oracle.

    `DATA()` on a header extern is a DECLARATION-SITE annotation. The
    datum is retail's and our source only REFERENCES it - not one of
    those 100 externs has a definition anywhere in src/ - so there is no
    storage for extraction to bind and no TU to own the claim.
    `retail_labels.source` therefore excuses header DATA() sites from its
    own sweep, and points here.

    Excused is not unchecked. Their channel is THIS inventory, and the
    claim is only worth writing if the address actually lands in it: a
    header DATA() naming an address no channel carries is as lost as any
    other silently dropped label. Measured 2026-08-20: 101 header DATA()
    addresses, 101 carried (99 reloc-target, 1 reloc-alias, 1
    src-DATA_COMPGEN) - so the floor is zero, not a tolerated backlog.

    Pure in (sites, rows) so the negative control can drive it."""
    problems = []
    for rva, wheres in sorted(sites.get("DATA", {}).items()):
        if not all(w.split(":")[0].endswith(".h") for w in wheres):
            continue                  # a src site: extraction's sweep owns it
        va = rva + common.IMAGE_BASE
        row = rows.get(rva)
        if row is None:
            problems.append(
                f"DATA(0x{va:08x}) at {wheres[0]} is a header extern whose "
                f"address reaches NO inventory row - no channel carries it, "
                f"so the claim names nothing")
        elif row["kind"] != "data":
            problems.append(
                f"DATA(0x{va:08x}) at {wheres[0]} claims a datum, but the "
                f"inventory carries {row['name']!r} there as {row['kind']}")
    return problems


def selftest() -> list[str]:
    """Negative control for the header-DATA gate: synthetic defects that
    MUST be detected plus a clean sample that MUST pass. Run before the
    gate judges the tree."""
    failures = []
    sites = {"DATA": {0x1000: ["include/hero.h:9"]}}
    if header_data_problems(sites, {0x1000: {"name": "const_1000",
                                             "kind": "data"}}):
        failures.append("clean sample did not pass")
    if not header_data_problems(sites, {}):
        failures.append("an uncarried header DATA() was not detected")
    if not header_data_problems(sites, {0x1000: {"name": "fn_1000",
                                                 "kind": "func"}}):
        failures.append("a header DATA() landing on a func row was not "
                        "detected")
    if header_data_problems({"DATA": {0x2000: ["src/hero.cpp:9"]}}, {}):
        failures.append("a src DATA() site was wrongly judged by this gate")
    if header_data_problems({"DATA": {0x3000: ["include/a.h:1",
                                               "src/a.cpp:2"]}}, {}):
        failures.append("a site with a src twin was wrongly judged here")
    return failures


def _write_compgen(src_claims) -> None:
    """The source-owned compiler-function manifest, RAW names by design:
    a `$E<n>` ordinal is volatile, so normalization keys these bodies by
    unit + owner, never by the joined spelling."""
    rows = [c for c in src_claims if c.meta.get("ckind")]
    COMPGEN_OUT.parent.mkdir(parents=True, exist_ok=True)
    with COMPGEN_OUT.open("w", newline="") as fh:
        fh.write("# GENERATED: python3 -m homm3.model - source-owned "
                 "compiler-function claims.\n")
        for prov in common.provenance("homm3.model"):
            fh.write(prov + "\n")
        writer = csv.writer(fh, delimiter="\t")
        writer.writerow(["unit", "name", "kind", "owner", "size"])
        for c in sorted(rows, key=lambda c: (c.unit, c.rva)):
            writer.writerow([c.unit, c.meta["raw"], c.meta["ckind"],
                             c.meta["owner"], f"0x{c.size:x}"])


def _upgrade_dense_data_alias(row: dict, claim) -> dict:
    """Let a reviewed owner replace only a source DATA dense placeholder.

    ``DATA()`` proves the address but deliberately contributes no source-level
    spelling.  A reloc-alias row proves that missing spelling.  Treating the
    two as contradictory prevents exactly the aggregate+addend recovery the
    alias channel exists for; accepting any other replacement would hide a
    genuine name/address conflict.
    """
    if row["name"] == claim.name:
        return row
    dense = f"data_{claim.rva:x}"
    if row["provenance"] != "src-DATA" or row["name"] != dense:
        raise ValueError(
            f"reloc alias owner {claim.name!r} conflicts with "
            f"{row['name']!r} at 0x{claim.rva:x}")
    upgraded = dict(row)
    upgraded.update(name=claim.name, unit=claim.unit,
                    kind="data", provenance=claim.channel)
    return upgraded


def _collect_inventory():
    functions = {r["rva"]: r["size"] for r in censuses.functions()}

    rows = {}       # rva -> row dict (first writer wins per authority order)
    problems = []

    def put(rva, name, unit, size, kind, provenance):
        name = normalize_anon_ns_name(name, unit)
        if VOLATILE_E_RE.match(name):
            common.die(f"0x{rva:x}: claim names volatile compiler ordinal "
                       f"{name!r} - record it as evidence, never a label")
        if rva in rows:
            problems.append(f"duplicate rva 0x{rva:x}: "
                            f"{rows[rva]['name']} vs {name}")
            return
        rows[rva] = {"rva": rva, "name": name, "unit": unit,
                     "size": size, "kind": kind, "provenance": provenance}

    # 1. src claims (fragments in scan order). The dedup replays over RAW
    # declarator names - the monolith suffixed BEFORE its join, and its
    # join keys stripped the suffix back off - then the bound spelling
    # takes over, from either binder: src-VA+ir (clang paired the
    # annotation with the symbol, cl's obj confirmed the string) or
    # src-VA+base (the lexical key join, for what IR cannot reach).
    src_claims = fragments.all_claims()
    _write_compgen(src_claims)
    seen_names = set()
    pooled: dict[int, str] = {}     # rva -> unit that first claimed it
    for c in src_claims:
        name = c.meta["raw"]
        if name in seen_names:
            name = f"{name}_{c.rva:x}"
        seen_names.add(name)
        if c.channel in ("src-VA+ir", "src-VA+base") or (
                c.channel == "src-DATA" and c.meta.get("type")
                and c.meta.get('defined') == '1'):
            name = c.name
        if c.channel in POOLED_CHANNELS:
            if c.rva in pooled:
                continue        # one pooled datum, many claiming TUs
            pooled[c.rva] = c.unit
        put(c.rva, name, c.unit, c.size if c.size is not None else "",
            c.kind, c.channel)

    # 2. zlib map - the admitted table carries the owning TU (unit column),
    # so the delinked objects pair 1:1 against our compiled base objs
    # (inflate.c.obj vs base/inflate.obj)
    for c in providers.zlib_map():
        put(c.rva, c.name, c.unit, c.size, c.kind, c.channel)

    # 3. runtime map (sizes from the universe)
    for c in providers.runtime_map():
        put(c.rva, c.name, "", functions[c.rva], "func", c.channel)

    # 4. the rest of the universe, working labels, seg buckets
    for rva, size in sorted(functions.items()):
        if rva in rows:
            continue
        put(rva, f"fn_{rva:x}",
            f"seg_{rva >> BUCKET_SHIFT:04x}", size, "func",
            "working-label")

    # 5. data rows -------------------------------------------------------
    image, info = common.load_image()
    secmap = {s.name: s for s in image.sections}
    rdata, dat = secmap[".rdata"], secmap[".data"]

    # Reviewed relocation aliases name the real source-level data owner.
    # Vostok requires that owner to exist in the PDB before it can rewrite
    # an otherwise anonymous stripped-image relocation to it.
    for c in providers.reloc_aliases():
        if c.rva in rows:
            try:
                rows[c.rva] = _upgrade_dense_data_alias(rows[c.rva], c)
            except ValueError as exc:
                common.die(str(exc))
            continue
        put(c.rva, c.name, "", "", "data", c.channel)

    # Reviewed vtable identities live in the retail census.
    vt_rows = censuses.vtables()
    for r in vt_rows:
        if r["rva"] in rows:
            continue  # a src claim owns the address
        cls = r["class"] or None
        name = f"??_7{cls}@@6B@" if cls else f"vtbl_{r['rva']:x}"
        put(r["rva"], name, "", r["count"] * 4, "data",
            "vtable-name" if cls else "vtable")

    from homm3.core.project import Project
    project = Project(common.HOMM3_DIR)
    for c in iat.claims(Path(info["path"]), project.toolchain / "lib"):
        put(c.rva, c.name, "", c.size, "data", c.channel)

    # dense naming for every absolute-relocation target, required because
    # vostok panics on an .rdata target below every named constant and
    # skips targets outside known symbol sizes
    skipped_targets = 0
    for target in providers.reloc_targets():
        if target in rows:
            continue
        if rdata.rva <= target < rdata.rva + rdata.mapped:
            put(target, f"const_{target:x}", "", "", "data", "reloc-target")
        elif dat.rva <= target < dat.rva + dat.size:
            put(target, f"data_{target:x}", "", "", "data", "reloc-target")
        elif dat.rva + dat.size <= target < dat.rva + dat.mapped:
            put(target, f"bss_{target:x}", "", "", "data", "reloc-target")
        else:
            skipped_targets += 1

    if problems:
        for p in problems[:10]:
            print(f"[model] {p}", file=sys.stderr)
        common.die(f"{len(problems)} duplicate-rva claims")

    # the header-DATA half of the completeness oracle - the gate proves
    # it can still fail before it judges the tree
    broken = selftest()
    if broken:
        for b in broken:
            print(f"[model] header-data SELFTEST BROKEN: {b}",
                  file=sys.stderr)
        common.die("the header-DATA gate cannot prove it detects its defect")
    header_lost = header_data_problems(labels_source.sweep_sites(), rows)
    for p in header_lost:
        print(f"[model] {p}", file=sys.stderr)
    if header_lost:
        common.die(f"{len(header_lost)} header DATA() claim(s) reach no "
                   f"inventory row")
    # global name uniqueness: label-grade names (declarator/working) take
    # an rva suffix on collision; a colliding PROVEN symbol is a defect
    from collections import Counter
    internal_rvas = {c.rva for c in fragments.all_claims() if c.meta.get("internal") == "1"}
    counts = Counter(r["name"] for r in rows.values())
    seen = set()
    for rva in sorted(rows):
        r = rows[rva]
        if counts[r['name']] > 1 and r['provenance'] == 'src-DATA':
            from homm3.core.msvc_names import discriminate
            distinct = discriminate(r['name'], rva, internal=rva in internal_rvas)
            if distinct:
                r['name'] = distinct
        if counts[r["name"]] > 1 and r["name"] in seen:
            if r["provenance"] not in ("src-VA", "working-label"):
                common.die(f"duplicate proven name {r['name']!r} at "
                           f"0x{rva:x}")
            r["name"] = f"{r['name']}_{rva:x}"
        seen.add(r["name"])
    names = [r["name"] for r in rows.values()]
    if len(set(names)) != len(names):
        common.die("name dedup failed to converge")
    missing = set(functions) - set(rows)
    if missing:
        common.die(f"{len(missing)} functions uncovered - first "
                   f"0x{min(missing):x}")

    return rows, skipped_targets


def generate() -> Path:
    rows, skipped_targets = _collect_inventory()
    model = resolve(rows)
    serialize(model)
    for binding in model.data:
        if binding.name:
            rows[binding.rva].update(name=binding.name, size=binding.size)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w", newline="") as fh:
        fh.write("# GENERATED: python3 -m homm3.model - the "
                 "synth-PDB inventory.\n")
        for prov in common.provenance("homm3.model"):
            fh.write(prov + "\n")
        writer = csv.writer(fh)
        writer.writerow(["rva", "name", "unit", "size", "kind",
                         "provenance"])
        for rva in sorted(rows):
            r = rows[rva]
            size = (f"0x{r['size']:x}" if isinstance(r["size"], int)
                    else r["size"])
            writer.writerow([f"0x{rva:x}", r["name"], r["unit"], size,
                             r["kind"], r["provenance"]])

    funcs = sum(1 for r in rows.values() if r["kind"] == "func")
    data = len(rows) - funcs
    by_prov = {}
    for r in rows.values():
        by_prov[r["provenance"]] = by_prov.get(r["provenance"], 0) + 1
    print(f"[model] {len(rows)} rows ({funcs} func, {data} data) "
          f"-> {OUT}")
    for prov in sorted(by_prov, key=by_prov.get, reverse=True):
        print(f"  {prov}: {by_prov[prov]}")
    if skipped_targets:
        print(f"  reloc targets outside .rdata/.data: {skipped_targets} "
              "skipped")
    return OUT


def main(argv=None) -> int:
    import argparse
    # Parse even though there are no options (the gruntz lesson: `--help`
    # must never run the join and rewrite the inventory as a side effect).
    argparse.ArgumentParser(
        prog="homm3 model", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter).parse_args(argv)

    generate()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
