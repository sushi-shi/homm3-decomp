#!/usr/bin/env python3
"""homm3.analysis.vtable_names - name every retail vtable from its slots.

A vtable's slots hold that class's virtual methods, so the class
qualifier on a NAMED slot names the vtable. Two independent name
sources feed the vote:

  dc     the Dreamcast CodeView transfer (evidence/retail-dc-name-map)
         - real NWC method names carrying `Class::Method` qualifiers
  ida    the NH3API IDB corpus (evidence/ida/{functions,names}.csv),
         retail-bridged where the masked-identity join landed

MOST-DERIVED WINS, NOT THE MAJORITY. A derived class's vtable repeats
its base's method pointers in every slot it does not override, so slot
votes skew hard toward the BASE. Measured on the 12 vtables where slot
evidence met an independent NH3API name, the majority rule lost 12/12:
0x23ba24's one named slot is an inherited `widget::` method but the
table is `border`'s; at 0x23b8f8 the 3-vote CHeroWindowEx majority was
inherited and the 2-vote TSplitWindow minority was the answer. So the
class is the most-derived voter per the Dreamcast hierarchy, and an
NH3API name that is at-or-below every voter is CONFIRMED by them.

Evidence grades, strongest first:
  ida+slots     NH3API names this exact address AND every slot-voted
                class is that class or an ancestor of it - two
                independent sources agreeing
  slot-derived  no NH3API name; one voter is a descendant of all the
                others, so it is the table's owner
  ida-address   NH3API names the address, no slot evidence to check it
  at-or-below   only base-class slots resolved: the table belongs to
                that class OR a subclass - partial, honest, and enough
                to bound the search
  conflict      NH3API name is not at-or-below the slot votes, or the
                voters are unrelated in the hierarchy - reported, never
                guessed

Outputs (evidence/vtables/): named.tsv, unnamed.tsv, README.md
Run: python3 -m homm3.analysis.vtable_names
"""
from __future__ import annotations

import csv
import re
import struct
import sys
from collections import Counter

from homm3.core import common

VTABLES = common.HOMM3_DIR / "config/retail-vtables.tsv"
DC_MAP = common.EVIDENCE_DIR / "retail-dc-name-map.csv"
IDA_FUNCS = common.EVIDENCE_DIR / "ida/functions.csv"
IDA_VTABLES = common.EVIDENCE_DIR / "ida/vtables.csv"
OUT = common.EVIDENCE_DIR / "vtables"

_DTOR = re.compile(r"^(?:type_func_)?(.+?)::(?:`?scalar deleting destructor|~)")
_QUALIFIED = re.compile(r"^([A-Za-z_]\w*)::")


def load_vtables():
    rows = []
    for line in VTABLES.open():
        if line.startswith("#") or line.startswith("rva"):
            continue
        cols = line.rstrip("\n").split("\t")
        rows.append((int(cols[0], 16), int(cols[1])))
    return rows


def load_names():
    """rva -> [names] from every corpus that carries qualifiers."""
    names = {}

    def add(rva, name):
        if name:
            names.setdefault(rva, []).append(name)

    if DC_MAP.is_file():
        with DC_MAP.open() as fh:
            for r in csv.DictReader(ln for ln in fh if not ln.startswith("#")):
                try:
                    add(int(r["rva"], 16), r.get("name"))
                except (ValueError, KeyError):
                    continue
    if IDA_FUNCS.is_file():
        with IDA_FUNCS.open() as fh:
            for r in csv.DictReader(ln for ln in fh if not ln.startswith("#")):
                if r.get("retail_rva"):
                    try:
                        rva = int(r["retail_rva"], 16)
                    except ValueError:
                        continue
                    add(rva, r.get("name"))
                    add(rva, r.get("readable"))
    return names


def load_ida_vtables():
    out = {}
    if IDA_VTABLES.is_file():
        with IDA_VTABLES.open() as fh:
            for r in csv.DictReader(ln for ln in fh if not ln.startswith("#")):
                try:
                    out[int(r["hd_va"], 16) - common.IMAGE_BASE] = (
                        r.get("class") or "", r.get("name") or "")
                except (ValueError, KeyError):
                    continue
    return out


def load_hierarchy():
    """derived -> base, from the Dreamcast class records."""
    bases = {}
    path = common.EVIDENCE_DIR / "dreamcast/classes.csv"
    if not path.is_file():
        return bases
    with path.open() as fh:
        for r in csv.DictReader(ln for ln in fh if not ln.startswith("#")):
            name = (r.get("name") or "").strip()
            base = (r.get("bases") or "").split(",")[0].strip()
            if name and base:
                bases[name] = base
    return bases


def ancestors(cls, bases, limit=16):
    out, cur = [], bases.get(cls)
    while cur and len(out) < limit:
        out.append(cur)
        cur = bases.get(cur)
    return out


def classify(slot_names, bases, ida_class):
    """(class, grade, detail) for one vtable."""
    votes = Counter()
    for name in slot_names:
        m = _QUALIFIED.match(name)
        if m:
            votes[m.group(1)] += 1
    detail = f"{sum(votes.values())} named slots"

    if ida_class:
        chain = set(ancestors(ida_class, bases)) | {ida_class}
        stray = [c for c in votes if c not in chain]
        if votes and not stray:
            return ida_class, "ida+slots", (
                detail + " all at-or-above " + ida_class)
        if stray:
            if ida_class not in bases and ida_class not in bases.values():
                # the Dreamcast hierarchy (RoE-era) does not know this
                # class at all - cannot verify, do not cry conflict
                return ida_class, "ida-unverified", (
                    f"{ida_class} absent from the DC hierarchy; slots vote "
                    + ",".join(sorted(votes)))
            return "", "conflict", (
                f"nh3api says {ida_class}; slots vote "
                + ",".join(sorted(votes)) + " (not its ancestors)")
        return ida_class, "ida-address", "no slot evidence to corroborate"

    if not votes:
        return "", "", "no named slots"
    # most-derived voter: the one every other voter is an ancestor of
    for cand in votes:
        others = set(votes) - {cand}
        if others and others <= set(ancestors(cand, bases)):
            return cand, "slot-derived", (
                detail + "; bases " + ",".join(sorted(others)))
    if len(votes) == 1:
        only = next(iter(votes))
        return "", "at-or-below", (
            f"slots name only {only} - this table is {only} or a subclass")
    return "", "conflict", "unrelated voters: " + ",".join(sorted(votes))


def main(argv=None) -> int:
    image, _info = common.load_image()
    vtables = load_vtables()
    names = load_names()
    ida_vt = load_ida_vtables()
    bases = load_hierarchy()
    OUT.mkdir(parents=True, exist_ok=True)

    named, unnamed = [], []
    grades = Counter()
    for rva, count in vtables:
        section = image.section_of(rva)
        slots = []
        if section is not None:
            off = section.raw_offset + (rva - section.rva)
            for k in range(count):
                va = struct.unpack_from("<I", image.data, off + 4 * k)[0]
                if image.in_image(va):
                    slots.append(image.rva_of(va))
        slot_names = [n for s in slots for n in names.get(s, [])]
        cls, grade, detail = classify(
            slot_names, bases, ida_vt.get(rva, ("", ""))[0])
        grades[grade or "none"] += 1
        row = (rva, count, cls, grade, len(slots), len(slot_names), detail)
        (named if cls else unnamed).append(row)

    header = ("vtable_rva\tslots\tclass\tgrade\tresolved_slots\t"
              "named_slots\tdetail\n")
    prov = common.provenance("homm3.analysis.vtable_names")
    for path, rows in (("named.tsv", named), ("unnamed.tsv", unnamed)):
        with (OUT / path).open("w") as fh:
            fh.write("\n".join(prov) + "\n" + header)
            for rva, count, cls, grade, res, nm, detail in rows:
                fh.write(f"0x{rva:x}\t{count}\t{cls}\t{grade}\t{res}\t{nm}\t"
                         f"{detail}\n")

    total = len(vtables)
    print(f"[vtable_names] {len(named)}/{total} vtables named "
          f"({100.0 * len(named) / total:.1f}%)")
    for grade, n in grades.most_common():
        print(f"    {grade:14} {n:4}")
    print(f"[vtable_names] -> {OUT.relative_to(common.HOMM3_DIR)}/"
          "{named,unnamed}.tsv")
    return 0


if __name__ == "__main__":
    sys.exit(main())
