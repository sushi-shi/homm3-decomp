"""homm3.verify.rtti - retail RTTI class hierarchies against our objects.

    homm3 verify rtti                         # the selected image (homm3 --image KEY)
    homm3 verify rtti --images game,h3maped,h3ccmped
    homm3 verify rtti --eh                    # also the thrown types' catchable arrays
    homm3 verify rtti --all                   # also list the one-sided classes
    homm3 verify rtti --json
    homm3 verify rtti --gate                  # nonzero when a finding is reported

Under /GR every Complete Object Locator names its class's Class Hierarchy
Descriptor (VC6 layout: signature, attributes, base count, base class
array). The array holds the class itself and then every base in preorder,
each a Base Class Descriptor: type descriptor, contained-base count, mdisp,
pdisp, vdisp, attributes. The retail side is every locator found in .rdata
and .data; our side is each object's `??_R3` descriptor, read through its
`??_R2` array, `??_R1` descriptors and `??_R0` type descriptors by
relocation. The objects are the existing build/<image>/objdiff/base ones;
nothing is compiled.

A class present on both sides must agree in the class attributes and in
every base's name, position, contained count, displacements and attributes
(private/protected or ambiguous bases set BCD attributes, so a public base
where retail derives privately shows up here). With `--eh`, each retail
ThrowInfo's CatchableTypeArray (the thrown type and its public unambiguous
bases: properties, displacements, size and whether a copy constructor is
named) is compared with our `__CTA` records keyed by the thrown type.

VC6 spells an anonymous namespace `?%<source path><hash>@`. Our scope is
first replaced by the image's reviewed retail spelling
(config/retail[/<image>]/anon-ns-paths.tsv, by owning unit and source file),
then both sides compare by source file name alone, so checkout paths and
hashes drop out. A class found on both sides only once anonymous scopes are
removed is a `scope` finding (one side declares it outside an anonymous
namespace). A match through the file name alone, without a reviewed row,
is a note naming the retail spelling. Objects that disagree about one
class are an `inconsistent` finding.

Classes on one side only are counted (library classes compiled into no
object of ours, and header COMDATs the linker discarded); `--all` lists
them. The report is informational; `--gate` returns 1 when any finding
(not a note or one-sided class) exists.
"""
from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from collections import Counter
from pathlib import Path
from typing import NamedTuple


class Base(NamedTuple):
    name: str
    contained: int
    mdisp: int
    pdisp: int
    vdisp: int
    attributes: int


class Hierarchy(NamedTuple):
    attributes: int
    bases: tuple[Base, ...]


class Catchable(NamedTuple):
    name: str
    properties: int
    mdisp: int
    pdisp: int
    vdisp: int
    size: int
    copy: bool


#: BaseClassDescriptor attributes (ehdata.h).
BCD_FLAGS = ((0x01, "not-visible"), (0x02, "ambiguous"),
             (0x04, "private-or-protected-in-complete-object"),
             (0x08, "private-or-protected-base"), (0x10, "virtual-base-of-contained"),
             (0x20, "non-polymorphic"), (0x40, "has-hierarchy-descriptor"))
#: ClassHierarchyDescriptor attributes.
CHD_FLAGS = ((0x01, "multiple"), (0x02, "virtual"), (0x04, "ambiguous"))
#: CatchableType properties.
CT_FLAGS = ((0x01, "simple-type"), (0x02, "by-reference-only"), (0x04, "has-virtual-base"))


def flags(value: int, table) -> str:
    names = [name for bit, name in table if value & bit]
    rest = value & ~sum(bit for bit, _ in table)
    if rest:
        names.append(f"{rest:#x}")
    return f"{value:#x}" + (f" ({', '.join(names)})" if names else "")


def _s32(value: int) -> int:
    return value - (1 << 32) if value & 0x80000000 else value


# --- names ----------------------------------------------------------------

_SCOPE = re.compile(r"\?%([^@]+)@")


def _file_key(scope: str) -> str:
    """`C:\\Dev\\...\\Map Editor.cpp284242958` -> `mapeditor.cpp`."""
    base = re.split(r"[\\/]", scope)[-1]
    return re.sub(r"[0-9]+$", "", base).lower().replace(" ", "")


def fold(name: str) -> str:
    """Every anonymous scope reduced to its source file name."""
    return _SCOPE.sub(lambda m: "?%" + _file_key(m[1]) + "@", name)


def strip_anonymous(name: str) -> str:
    """The name with its anonymous-namespace components removed."""
    return _SCOPE.sub("", name)


def reviewed(name: str, unit: str, canonical: dict) -> str:
    """Our scope replaced by the reviewed retail spelling of its unit's file."""
    def replace(m):
        scope = canonical.get((unit.lower(), _file_key(m[1])))
        return m[0] if scope is None else "?%" + scope + "@"
    return _SCOPE.sub(replace, name)


def _canonical(path: Path) -> dict:
    if not path.is_file():
        return {}
    from homm3.compare.canonicalize import _read_anon_ns_canonical
    return _read_anon_ns_canonical(path.read_text())


def _short(name: str) -> str:
    """`.?AVFoo@Bar@@` -> `Foo@Bar` for display."""
    if name.startswith((".?AV", ".?AU")) and name.endswith("@@"):
        return name[4:-2]
    return name


# --- retail ---------------------------------------------------------------

class Retail:
    """Dword and string reads at absolute addresses of a PE image."""

    def __init__(self, pe):
        self.pe = pe
        self.base = pe.image_base

    def u32(self, va: int | None) -> int | None:
        if va is None or va < self.base:
            return None
        raw = self.pe.read(va - self.base, 4)
        return struct.unpack("<I", raw)[0] if raw is not None and len(raw) == 4 else None

    def cstr(self, va: int) -> str | None:
        for s in self.pe.sections:
            if s["va"] <= va - self.base < s["va"] + s["rsize"]:
                off = s["rptr"] + va - self.base - s["va"]
                end = self.pe.data.find(b"\0", off, min(off + 1024, s["rptr"] + s["rsize"]))
                return self.pe.data[off:end].decode("latin-1") if end > off else None
        return None

    def type_name(self, td: int | None) -> str | None:
        if not td:
            return None
        name = self.cstr(td + 8)
        return name if name and name.startswith(".") and name[1:2] in "?PH_JKDFNM" \
            and len(name) > 2 else None

    def section(self, name: str):
        return next((s for s in self.pe.sections if s["name"] == name), None)

    def in_section(self, va: int, *names: str) -> bool:
        rva = va - self.base
        return any(s["name"] in names and s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"])
                   for s in self.pe.sections)


def _retail_hierarchy(r: Retail, chd: int) -> Hierarchy | None:
    fields = [r.u32(chd + 4 * i) for i in range(4)]
    if None in fields or fields[0] != 0 or not 0 < fields[2] < 256:
        return None
    _sig, attributes, count, array = fields
    bases = []
    for i in range(count):
        bcd = r.u32(array + 4 * i)
        f = [r.u32(bcd + 4 * k) for k in range(6)] if bcd else [None]
        if None in f:
            return None
        name = r.type_name(f[0])
        if name is None or not name.startswith(".?A"):
            return None
        bases.append(Base(fold(name), f[1], _s32(f[2]), _s32(f[3]), _s32(f[4]), f[5]))
    return Hierarchy(attributes, tuple(bases))


def retail_hierarchies(r: Retail) -> tuple[dict[str, Hierarchy], dict[str, str], int | None]:
    """({folded class: hierarchy}, {folded class: retail spelling}, the
    type_info vftable the locators' type descriptors share)."""
    out, spelled, vftables = {}, {}, Counter()
    for s in (r.section(".rdata"), r.section(".data")):
        if s is None:
            continue
        raw = r.pe.data[s["rptr"]:s["rptr"] + s["rsize"]]
        for off in range(0, len(raw) - 19, 4):
            sig, _offset, _cd, td, chd = struct.unpack_from("<5I", raw, off)
            if sig or not td or not chd or not r.in_section(td, ".data", ".rdata") \
                    or not r.in_section(chd, ".rdata", ".data"):
                continue
            name = r.type_name(td)
            if name is None or not name.startswith(".?A") or not name.endswith("@@"):
                continue
            key = fold(name)
            if key in out:
                continue
            h = _retail_hierarchy(r, chd)
            if h is None or h.bases[0].name != key:
                continue
            out[key], spelled[key] = h, name
            vftables[r.u32(td)] += 1
    vftable = vftables.most_common(1)[0][0] if vftables else None
    return out, spelled, vftable


def retail_catchables(r: Retail, type_info: int | None) -> dict[str, tuple[Catchable, ...]]:
    """{folded thrown type: its catchable types} from every ThrowInfo."""
    text = r.section(".text")
    lo, hi = (r.base + text["va"], r.base + text["va"] + text["vsize"]) if text else (0, 0)

    def descriptor(td):
        name = r.type_name(td)
        if name is None or (type_info is not None and r.u32(td) != type_info):
            return None
        return name

    out = {}
    for s in (r.section(".rdata"), r.section(".data")):
        if s is None:
            continue
        raw = r.pe.data[s["rptr"]:s["rptr"] + s["rsize"]]
        for off in range(0, len(raw) - 15, 4):
            attributes, unwind, forward, cta = struct.unpack_from("<4I", raw, off)
            if attributes > 3 or forward or (unwind and not lo <= unwind < hi) or not cta \
                    or not r.in_section(cta, ".rdata", ".data"):
                continue
            count = r.u32(cta)
            if count is None or not 0 < count <= 64:
                continue
            types = []
            for i in range(count):
                ct = r.u32(cta + 4 + 4 * i)
                f = [r.u32(ct + 4 * k) for k in range(7)] if ct else [None]
                if None in f or f[0] > 7 or (f[6] and not lo <= f[6] < hi):
                    break
                name = descriptor(f[1])
                if name is None:
                    break
                types.append(Catchable(fold(name), f[0], _s32(f[2]), _s32(f[3]),
                                       _s32(f[4]), f[5], bool(f[6])))
            if len(types) == count:
                out.setdefault(types[0].name, tuple(types))
    return out


# --- our objects ----------------------------------------------------------

class _Records:
    """One object's defined symbols, read as relocated dwords."""

    def __init__(self, obj):
        self.obj = obj
        self.defined = {}
        for idx, value, secnum in obj.iter_symbols():
            if secnum >= 1:
                self.defined.setdefault(obj.sym_name(idx), (secnum, value))
        self._sections = {}

    def words(self, name: str, count: int):
        """[(value, relocation target or None)] of `count` dwords at `name`."""
        secnum, value = self.defined[name]
        if secnum not in self._sections:
            self._sections[secnum] = (self.obj.section_payload(secnum),
                                      self.obj.relocations(secnum))
        data, relocs = self._sections[secnum]
        out = []
        for i in range(count):
            at = value + 4 * i
            word = struct.unpack_from("<I", data, at)[0] if data else 0
            out.append((word, relocs.get(at)))
        return out


def _r0_name(symbol: str | None) -> str | None:
    """`??_R0?AVFoo@@@8` -> `.?AVFoo@@`."""
    if symbol and symbol.startswith("??_R0") and symbol.endswith("@8"):
        return "." + symbol[5:-2]
    return None


def _our_hierarchy(rec: _Records, r3: str, rename) -> Hierarchy | None:
    (sig, _), (attributes, _), (count, _), (_v, r2) = rec.words(r3, 4)
    if sig or r2 not in rec.defined or not 0 < count < 256:
        return None
    bases = []
    for _v, r1 in rec.words(r2, count):
        if r1 not in rec.defined:
            return None
        f = rec.words(r1, 6)
        name = _r0_name(f[0][1])
        if name is None:
            return None
        bases.append(Base(rename(name), f[1][0], _s32(f[2][0]), _s32(f[3][0]),
                          _s32(f[4][0]), f[5][0]))
    return Hierarchy(attributes, tuple(bases))


def _our_catchables(rec: _Records, cta: str, rename) -> tuple[Catchable, ...] | None:
    count = rec.words(cta, 1)[0][0]
    if not 0 < count <= 64:
        return None
    types = []
    for _v, ct in rec.words(cta, count + 1)[1:]:
        if ct not in rec.defined:
            return None
        f = rec.words(ct, 7)
        name = _r0_name(f[1][1])
        if name is None:
            return None
        types.append(Catchable(rename(name), f[0][0], _s32(f[2][0]), _s32(f[3][0]),
                               _s32(f[4][0]), f[5][0], bool(f[6][1] or f[6][0])))
    return tuple(types)


class Ours(NamedTuple):
    hierarchies: dict          # folded class -> {Hierarchy: [units]}
    spelled: dict              # folded class -> {our reviewed spelling: [units]}
    catchables: dict           # folded thrown type -> {tuple: [units]}


def our_records(objects, canonical: dict, eh: bool = False) -> Ours:
    """Our hierarchies (and catchable arrays) from [(unit, Obj)]."""
    hierarchies, spelled, catchables = {}, {}, {}
    for unit, obj in objects:
        rec = _Records(obj)

        def rename(name, unit=unit):
            return fold(reviewed(name, unit, canonical))

        for name in rec.defined:
            try:
                if name.startswith("??_R3"):
                    h = _our_hierarchy(rec, name, rename)
                    if h is None:
                        continue
                    key = h.bases[0].name
                    hierarchies.setdefault(key, {}).setdefault(h, []).append(unit)
                    r1 = rec.words(rec.words(name, 4)[3][1], 1)[0][1]
                    own = _r0_name(rec.words(r1, 1)[0][1])
                    spelled.setdefault(key, {}).setdefault(
                        reviewed(own, unit, canonical), []).append(unit)
                elif eh and name.startswith("__CTA"):
                    types = _our_catchables(rec, name, rename)
                    if types:
                        catchables.setdefault(types[0].name, {}).setdefault(
                            types, []).append(unit)
            except (KeyError, struct.error, IndexError):
                continue
    return Ours(hierarchies, spelled, catchables)


# --- comparison -----------------------------------------------------------

def _base_text(b: Base) -> str:
    text = f"{_short(b.name)} +{b.mdisp:#x} contains {b.contained}"
    if b.pdisp != -1:
        text += f" pdisp {b.pdisp} vdisp {b.vdisp}"
    return text + f" attributes {flags(b.attributes, BCD_FLAGS)}"


def _ct_text(c: Catchable) -> str:
    text = f"{_short(c.name)} properties {flags(c.properties, CT_FLAGS)} +{c.mdisp:#x}"
    if c.pdisp != -1:
        text += f" pdisp {c.pdisp} vdisp {c.vdisp}"
    return text + f" size {c.size}" + (" copy" if c.copy else "")


def differences(retail, ours, fields, label=lambda e: _short(e.name)) -> list[str]:
    """Readable field differences between two entry sequences."""
    out = []
    rn, on = [e.name for e in retail], [e.name for e in ours]
    if rn != on:
        if sorted(rn) == sorted(on):
            out.append("base order: retail " + ", ".join(map(_short, rn))
                       + "; ours " + ", ".join(map(_short, on)))
        else:
            only_r = [_short(n) for n in rn if n not in on]
            only_o = [_short(n) for n in on if n not in rn]
            out.append(f"bases: retail {len(rn)}, ours {len(on)}"
                       + (f"; retail only {', '.join(only_r)}" if only_r else "")
                       + (f"; ours only {', '.join(only_o)}" if only_o else ""))
        return out
    for r, o in zip(retail, ours):
        for field, show in fields:
            a, b = getattr(r, field), getattr(o, field)
            if a != b:
                out.append(f"{label(r)}: {field} retail {show(a)}, ours {show(b)}")
    return out


_BCD_FIELDS = (("contained", str), ("mdisp", hex), ("pdisp", str), ("vdisp", str),
               ("attributes", lambda v: flags(v, BCD_FLAGS)))
_CT_FIELDS = (("properties", lambda v: flags(v, CT_FLAGS)), ("mdisp", hex),
              ("pdisp", str), ("vdisp", str), ("size", str), ("copy", str))


def hierarchy_differences(r: Hierarchy, o: Hierarchy) -> list[str]:
    out = []
    if r.attributes != o.attributes:
        out.append(f"class attributes retail {flags(r.attributes, CHD_FLAGS)}, "
                   f"ours {flags(o.attributes, CHD_FLAGS)}")
    return out + differences(r.bases, o.bases, _BCD_FIELDS)


def _strip_hierarchy(h: Hierarchy) -> Hierarchy:
    return Hierarchy(h.attributes, tuple(b._replace(name=strip_anonymous(b.name))
                                         for b in h.bases))


def compare(retail: dict, retail_spelled: dict, ours: Ours,
            retail_ct: dict | None = None) -> dict:
    """Findings, notes and one-sided classes of one image."""
    findings, notes = [], []
    agree = 0
    matched_ours = set()
    for key, units in sorted(ours.hierarchies.items()):
        if len(units) > 1:
            findings.append({
                "kind": "inconsistent", "class": key,
                "ours": [{"units": sorted(u), "bases": [_base_text(b) for b in h.bases],
                          "attributes": h.attributes} for h, u in units.items()]})
    stripped = {}
    for key in ours.hierarchies:
        stripped.setdefault(strip_anonymous(key), []).append(key)
    retail_only = []
    for key, r in sorted(retail.items()):
        if key in ours.hierarchies:
            matched_ours.add(key)
            variants = ours.hierarchies[key]
            o, units = next(iter(variants.items()))
            if len(variants) == 1 and o == r:
                agree += 1
            elif len(variants) == 1:
                findings.append({"kind": "hierarchy", "class": key, "units": sorted(units),
                                 "differences": hierarchy_differences(r, o),
                                 "retail": [_base_text(b) for b in r.bases],
                                 "ours": [_base_text(b) for b in o.bases]})
            for spelling, where in ours.spelled.get(key, {}).items():
                if "?%" in spelling and spelling != retail_spelled.get(key):
                    notes.append({"kind": "unreviewed-scope", "class": key,
                                  "units": sorted(where),
                                  "retail": retail_spelled.get(key)})
            continue
        partners = [k for k in stripped.get(strip_anonymous(key), []) if k != key
                    and k not in retail]
        if partners:
            other = partners[0]
            matched_ours.add(other)
            o, units = next(iter(ours.hierarchies[other].items()))
            rest = hierarchy_differences(_strip_hierarchy(r), _strip_hierarchy(o))
            findings.append({"kind": "scope", "class": key, "ours_class": other,
                             "units": sorted(units),
                             "differences": [f"retail {_short(key)}, ours {_short(other)}"]
                             + rest})
            continue
        retail_only.append(key)
    ours_only = sorted(set(ours.hierarchies) - matched_ours)
    summary = {"retail": len(retail), "ours": len(ours.hierarchies), "agree": agree,
               "retail_only": len(retail_only), "ours_only": len(ours_only)}
    for kind in ("hierarchy", "scope", "inconsistent"):
        summary[kind] = sum(f["kind"] == kind for f in findings)
    result = {"summary": summary, "findings": findings, "notes": notes,
              "retail_only": retail_only, "ours_only": ours_only}
    if retail_ct is not None:
        ct_agree, ct_retail_only = 0, []
        for key, r in sorted(retail_ct.items()):
            variants = ours.catchables.get(key)
            if not variants:
                ct_retail_only.append(key)
                continue
            if r in variants:
                ct_agree += 1
                continue
            o, units = next(iter(variants.items()))
            findings.append({"kind": "catchable", "class": key, "units": sorted(units),
                             "differences": differences(r, o, _CT_FIELDS),
                             "retail": [_ct_text(c) for c in r],
                             "ours": [_ct_text(c) for c in o]})
        ct_ours_only = sorted(set(ours.catchables) - set(retail_ct))
        summary.update(eh_retail=len(retail_ct), eh_ours=len(ours.catchables),
                       eh_agree=ct_agree, eh_retail_only=len(ct_retail_only),
                       eh_ours_only=len(ct_ours_only),
                       catchable=sum(f["kind"] == "catchable" for f in findings))
        result["eh_retail_only"] = ct_retail_only
        result["eh_ours_only"] = ct_ours_only
    return result


# --- driver ---------------------------------------------------------------

def image_inputs(image: str) -> tuple[Path, Path, Path]:
    """(retail executable, base object directory, anon-ns table) of an image."""
    from homm3.core import common, images, paths
    from homm3.core.inputs import stage_executable
    from homm3.core.project import Project
    exe = stage_executable(Project(common.HOMM3_DIR).executable(images.input_key(image)))
    return (exe, paths.image_build(image) / "objdiff/base",
            paths.retail_dir(image) / "anon-ns-paths.tsv")


def audit(exe: Path, base_dir: Path, table: Path, eh: bool = False) -> dict:
    from homm3.core.pe import Pe
    from homm3.delink.coffx import objects
    r = Retail(Pe(exe))
    retail, spelled, type_info = retail_hierarchies(r)
    retail_ct = retail_catchables(r, type_info) if eh else None
    ours = our_records(objects(base_dir), _canonical(table), eh)
    return compare(retail, spelled, ours, retail_ct)


def render(image: str, result: dict, show_all: bool = False) -> list[str]:
    s = result["summary"]
    lines = [f"rtti {image}: retail {s['retail']} hierarchies, ours {s['ours']}; "
             f"{s['agree']} agree, {s['hierarchy']} differ, {s['scope']} scope, "
             f"{s['inconsistent']} inconsistent; {s['retail_only']} retail-only, "
             f"{s['ours_only']} ours-only"]
    if "eh_retail" in s:
        lines.append(f"  eh: retail {s['eh_retail']} thrown types, ours {s['eh_ours']}; "
                     f"{s['eh_agree']} agree, {s['catchable']} differ; "
                     f"{s['eh_retail_only']} retail-only, {s['eh_ours_only']} ours-only")
    for f in result["findings"]:
        units = f.get("units") or sorted({u for v in f.get("ours", []) if isinstance(v, dict)
                                          for u in v["units"]})
        lines.append(f"{f['kind'].upper()} {_short(f['class'])}  [{', '.join(units)}]")
        if f["kind"] == "inconsistent":
            for variant in f["ours"]:
                lines.append(f"    {', '.join(variant['units'])}: "
                             f"class attributes {flags(variant['attributes'], CHD_FLAGS)}")
                lines += [f"      {b}" for b in variant["bases"]]
            continue
        lines += [f"    {d}" for d in f["differences"]]
    for n in result["notes"]:
        lines.append(f"NOTE {n['kind']} {_short(n['class'])} [{', '.join(n['units'])}]: "
                     f"retail spells {n['retail']}")
    if show_all:
        lines += [f"retail-only {_short(k)}" for k in result["retail_only"]]
        lines += [f"ours-only {_short(k)}" for k in result["ours_only"]]
        lines += [f"eh retail-only {_short(k)}" for k in result.get("eh_retail_only", [])]
        lines += [f"eh ours-only {_short(k)}" for k in result.get("eh_ours_only", [])]
    return lines


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="homm3 verify rtti", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--images", metavar="KEYS",
                    help="comma-separated image keys (default: the selected image)")
    ap.add_argument("--eh", action="store_true",
                    help="also compare thrown types' catchable-type arrays")
    ap.add_argument("--all", action="store_true", help="also list one-sided classes")
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    ap.add_argument("--gate", action="store_true",
                    help="nonzero when any finding (not a note) is reported")
    args = ap.parse_args(argv)
    from homm3.core import common, images as image_keys
    keys = (args.images.split(",") if args.images
            else [image_keys.selected(common.HOMM3_DIR)])
    known = image_keys.images(common.HOMM3_DIR)
    unknown = [k for k in keys if k not in known]
    if unknown:
        ap.error(f"unknown image(s) {', '.join(unknown)} (pinned: {', '.join(known)})")
    results = {}
    for key in keys:
        exe, base_dir, table = image_inputs(key)
        if not base_dir.is_dir():
            print(f"rtti {key}: no objects at {base_dir}; build the image first",
                  file=sys.stderr)
            return 2
        results[key] = audit(exe, base_dir, table, args.eh)
    if args.json:
        json.dump(results, sys.stdout, indent=1)
        print()
    else:
        for key, result in results.items():
            print("\n".join(render(key, result, args.all)))
    found = any(r["findings"] for r in results.values())
    return 1 if args.gate and found else 0


if __name__ == "__main__":
    raise SystemExit(main())
