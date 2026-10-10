"""homm3.verify.link_diff - how far the candidate link is from retail.

    homm3 verify link-diff            # compare build/exe/HEROES3.candidate.EXE
    homm3 verify link-diff --update   # bank the current counts as the ceiling
    homm3 verify link-diff --detail code-exact   # list one region's findings

The comparison target is retail with the post-link edits of
config/retail/post-link-edits.tsv reverted to the bytes LINK wrote (the
same-timestamp Collector's Edition link). Each edit row is checked against
the retail bytes first; a row that no longer describes retail fails.

While non-exact functions still change size, every later address of the
candidate differs from retail, so a byte-for-byte file comparison says
nothing about the link itself. The gated regions therefore compare
contribution by contribution:

  headers     header bytes before the first section (raw; layout fields
              such as section sizes differ until the code sizes agree)
  rich        |count difference| summed over the Rich header @comp.ids
  imports     the import table read semantically: DLL order and spelling,
              each DLL's thunk names in IAT order, hints and descriptor
              timestamps, after the import-* edit rules
  code-exact  differing bytes of every placed code contribution that the
              ledger scores 100 (or a library member), each compared at
              its own retail address; a relocated field compares the
              retail address of the candidate's target contribution
  code-order  places where the candidate's code order steps backwards in
              retail order (each is one run break)
  code-unplaced  candidate code bytes with no retail identity (an ICF
                 fold is placed through any folded name)
  code-absent    retail game/library code bytes no candidate contribution
                 covers
  data        the same contribution compare over initialized data
  rsrc        resource bytes (raw, relative to the section)

Informational only: `code-nonexact` (differing bytes inside functions the
ledger scores below 100) and `raw-<section>`/`size` (the plain file
comparison, which becomes meaningful once the layout agrees).

config/link_diff.tsv holds the highest count each gated region may have;
a rise, or a gated region without a banked ceiling, fails.

code-unplaced is also gated row by row: config/link_unplaced.tsv names
every unplaced contribution (initializers per unit, ICF groups by their
lowest name) with its bytes and the reason it has no retail identity, so
the region cannot grow without an explained ledger entry. An ICF-folded
copy takes the retail identity of its fold-mates, since which copy LINK
keeps moves with unrelated inputs.
"""
from __future__ import annotations

import argparse
import bisect
import re
import struct
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path

from homm3.core import common
from homm3.core.images import path as _image_path

ROOT = common.HOMM3_DIR
EDITS = ROOT / "config/retail/post-link-edits.tsv"
CEILING = ROOT / "config/link_diff.tsv"
UNPLACED = ROOT / "config/link_unplaced.tsv"
CANDIDATE = ROOT / _image_path("build/exe/HEROES3.candidate.EXE")
#: Regions that fail `homm3 build` when they rise: they measure the link's
#: own inputs (flags, libraries, objects read, resources) and do not move
#: with matching work elsewhere.
GATED = ("headers", "rich", "imports", "rsrc")
#: Regions banked beside them and reported, but not yet gated: they compare
#: code and data contributions, which still move when non-exact functions
#: or header COMDAT emission change. Each joins GATED once it reaches 0.
TRACKED = ("code-exact", "code-order", "code-unplaced", "code-absent", "data",
           "data-unplaced")

_EXECUTE = 0x20000000
_CODE = 0x20
_INITIALIZED = 0x40
_UNINITIALIZED = 0x80
_REL_DIR32, _REL_REL32 = 6, 0x14


# ------------------------------------------------------------------- PE ---

@dataclass
class Pe:
    data: bytes
    base: int
    sections: list[tuple[str, int, int, int, int]]   # name, rva, vsize, raw, rawsize

    @classmethod
    def read(cls, data: bytes) -> "Pe":
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        count = struct.unpack_from("<H", data, pe + 6)[0]
        optional = struct.unpack_from("<H", data, pe + 20)[0]
        base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
        rows = []
        for i in range(count):
            at = pe + 24 + optional + 40 * i
            name = data[at:at + 8].rstrip(b"\0").decode("latin-1")
            vsize, rva, rawsize, raw = struct.unpack_from("<IIII", data, at + 8)
            rows.append((name, rva, vsize, raw, rawsize))
        return cls(data, base, rows)

    @property
    def header(self) -> int:
        return struct.unpack_from("<I", self.data, 0x3C)[0]

    def directory(self, index: int) -> tuple[int, int]:
        return struct.unpack_from("<II", self.data, self.header + 24 + 96 + 8 * index)

    def offset(self, rva: int) -> int | None:
        for _name, start, vsize, raw, rawsize in self.sections:
            if start <= rva < start + max(vsize, rawsize):
                return raw + rva - start if rva - start < rawsize else None
        return rva if rva < self.sections[0][3] else None

    def read_at(self, rva: int, size: int) -> bytes | None:
        at = self.offset(rva)
        return None if at is None else self.data[at:at + size]

    def section(self, name: str):
        return next((s for s in self.sections if s[0] == name), None)

    def code_bytes(self, rva: int, size: int) -> bytes:
        return self.read_at(rva, size) or b""


# ---------------------------------------------------------------- edits ---

@dataclass
class Edits:
    spans: list[tuple[int, bytes, bytes]] = field(default_factory=list)  # offset, retail, link
    dll_names: dict[str, str] = field(default_factory=dict)              # link -> retail
    zero_hints: set[str] = field(default_factory=set)                    # dll (lower)
    timestamp: int | None = None
    orphans: list[tuple[int, str]] = field(default_factory=list)


def read_edits(path: Path = EDITS) -> Edits:
    from homm3.core.tsv import read as read_tsv
    edits = Edits()
    for row in read_tsv(path)[2]:
        kind = row["kind"]
        if kind == "bytes":
            retail, link = bytes.fromhex(row["retail"]), bytes.fromhex(row["link"])
            size = int(row["size"])
            if not len(retail) == len(link) == size:
                raise ValueError(f"post-link edit {row['offset']}: size {size} disagrees "
                                 "with its byte strings")
            edits.spans.append((int(row["offset"], 16), retail, link))
        elif kind == "import-dll-name":
            edits.dll_names[row["link"].lower()] = row["retail"]
        elif kind == "import-hints":
            edits.zero_hints.add(row["offset"].lower())
        elif kind == "import-timestamp":
            edits.timestamp = int(row["retail"], 16)
        elif kind == "import-orphan":
            edits.orphans.append((int(row["offset"], 16), row["retail"]))
        else:
            raise ValueError(f"unknown post-link edit kind {kind!r}")
    return edits


def link_view(retail: bytes, edits: Edits) -> tuple[bytes, list[str]]:
    """(retail with every bytes-edit reverted to LINK's bytes, findings for
    rows that no longer describe retail)."""
    layout = Pe.read(retail)
    out = bytearray(retail)
    findings = []
    for offset, was, link in edits.spans:
        if retail[offset:offset + len(was)] != was:
            findings.append(f"post-link edit {offset:#x}: retail holds "
                            f"{retail[offset:offset + len(was)].hex()}, not {was.hex()}")
        out[offset:offset + len(link)] = link
    for rva, name in edits.orphans:
        text = name.encode() + b"\0"
        at = layout.offset(rva)
        if at is None or retail[at:at + len(text)] != text:
            findings.append(f"post-link orphan {rva:#x}: retail does not spell {name}")
            continue
        out[at:at + len(text)] = bytes(len(text))
    return bytes(out), findings


# --------------------------------------------------------------- regions ---

def _differing(a: bytes, b: bytes) -> int:
    n = min(len(a), len(b))
    return sum(1 for i in range(n) if a[i] != b[i]) + abs(len(a) - len(b))


def _layout_fields(pe: Pe) -> set[int]:
    """Offsets from the PE signature of the header fields whose values follow
    the section layout: the optional header's sizes, entry point, bases and
    image size, the data directories of linker-built tables, and each
    section header's sizes and positions."""
    at = 24
    fields = [(at + 4, 12), (at + 16, 12), (at + 56, 4)]
    for index in (1, 2, 12):                                    # imports, resources, IAT
        fields.append((at + 96 + 8 * index, 8))
    optional = struct.unpack_from("<H", pe.data, pe.header + 20)[0]
    for index in range(len(pe.sections)):
        fields.append((24 + optional + 40 * index + 8, 16))
    return {offset + i for offset, size in fields for i in range(size)}


def header_difference(expected: Pe, candidate: Pe) -> tuple[int, int]:
    """(differing header bytes, of which follow the layout). The DOS header
    and stub compare in place; the PE headers compare from each image's own
    PE signature, since `e_lfanew` follows the Rich header's length, which
    `rich` compares on its own."""
    first = min(raw for _n, _r, _v, raw, size in expected.sections if size)
    differing = sum(1 for i in range(0x40) if i not in range(0x3C, 0x40)
                    and expected.data[i] != candidate.data[i])
    stub = range(0x40, 0x80)
    differing += sum(1 for i in stub if expected.data[i] != candidate.data[i])
    layout = _layout_fields(expected)
    layout_differing = 0
    a, b = expected.header, candidate.header
    for k in range(first - max(a, b)):
        if expected.data[a + k] == candidate.data[b + k]:
            continue
        if k in layout:
            layout_differing += 1
        else:
            differing += 1
    return differing, layout_differing


def raw_regions(expected: Pe, candidate: Pe) -> dict[str, int]:
    """Plain file comparison over the retail section layout."""
    first = min(raw for _n, _r, _v, raw, size in expected.sections if size)
    headers, layout = header_difference(expected, candidate)
    counts = {"size": abs(len(expected.data) - len(candidate.data)),
              "headers": headers, "header-layout": layout}
    end = first
    for name, _rva, _vsize, raw, size in expected.sections:
        counts[f"raw-{name}"] = _differing(expected.data[raw:raw + size],
                                           candidate.data[raw:raw + size])
        end = max(end, raw + size)
    counts["raw-overlay"] = _differing(expected.data[end:], candidate.data[end:])
    return counts


def rich_entries(data: bytes) -> Counter:
    end = data.find(b"Rich", 0, 0x400)
    if end < 0:
        return Counter()
    key = struct.unpack_from("<I", data, end + 4)[0]
    start = data.find(struct.pack("<I", 0x536E6144 ^ key), 0, end)
    out = Counter()
    for at in range(start + 16, end, 8):
        comp, count = struct.unpack_from("<II", data, at)
        out[comp ^ key] = count ^ key
    return out


def rich_difference(expected: bytes, candidate: bytes) -> tuple[int, list[str]]:
    a, b = rich_entries(expected), rich_entries(candidate)
    rows = []
    for comp in sorted(set(a) | set(b)):
        if a[comp] != b[comp]:
            rows.append(f"@comp.id {comp >> 16}/{comp & 0xffff}: retail {a[comp]}, "
                        f"candidate {b[comp]}")
    return sum(abs(a[c] - b[c]) for c in set(a) | set(b)), rows


@dataclass
class Import:
    dll: str
    timestamp: int
    thunks: list[tuple[object, int, int]]   # (name or ordinal, hint, IAT rva)


def import_table(pe: Pe) -> list[Import]:
    rva, _size = pe.directory(1)
    out = []
    while True:
        lookup, stamp, _chain, name, iat = struct.unpack("<5I", pe.read_at(rva, 20))
        if not name:
            return out
        raw = pe.read_at(name, 64)
        dll = raw[:raw.index(b"\0")].decode("latin-1")
        thunks = []
        table = lookup or iat
        slot = iat
        while True:
            entry = struct.unpack("<I", pe.read_at(table, 4))[0]
            if not entry:
                break
            if entry & 0x80000000:
                thunks.append((entry & 0xFFFF, -1, slot))
            else:
                record = pe.read_at(entry, 256)
                hint = struct.unpack_from("<H", record)[0]
                thunks.append((record[2:record.index(b"\0", 2)].decode("latin-1"), hint, slot))
            table += 4
            slot += 4
        out.append(Import(dll, stamp, thunks))
        rva += 20


def import_difference(expected: Pe, candidate: Pe, edits: Edits) -> tuple[int, list[str]]:
    """Mismatches between the import tables: one per differing DLL position,
    thunk position, hint or descriptor timestamp, after the edit rules."""
    want, got = import_table(expected), import_table(candidate)
    rows = []
    for index in range(max(len(want), len(got))):
        a = want[index] if index < len(want) else None
        b = got[index] if index < len(got) else None
        if a is None or b is None:
            rows.append(f"descriptor {index}: retail {a and a.dll}, candidate {b and b.dll}")
            continue
        spelled = edits.dll_names.get(b.dll.lower(), b.dll)
        if a.dll != spelled:
            rows.append(f"descriptor {index}: retail {a.dll}, candidate {b.dll}")
        stamp = edits.timestamp if edits.timestamp is not None else 0
        if a.timestamp != stamp or b.timestamp != 0:
            rows.append(f"{a.dll}: descriptor timestamp {b.timestamp:#x}")
        zero = a.dll.lower() in edits.zero_hints or spelled.lower() in edits.zero_hints
        for slot in range(max(len(a.thunks), len(b.thunks))):
            x = a.thunks[slot] if slot < len(a.thunks) else None
            y = b.thunks[slot] if slot < len(b.thunks) else None
            if x is None or y is None or x[0] != y[0]:
                rows.append(f"{a.dll}[{slot}]: retail {x and x[0]}, candidate {y and y[0]}")
            elif not zero and x[1] != y[1]:
                rows.append(f"{a.dll}[{slot}] {x[0]}: hint {y[1]}, retail {x[1]}")
    return len(rows), rows


# ------------------------------------------------------------ identities ---

@dataclass
class MapSymbol:
    va: int
    name: str
    tag: str          # object or lib:member as the map prints it
    static: bool


def read_map(path: Path) -> list[MapSymbol]:
    rows, mode = [], None
    pattern = re.compile(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s+(?:f\s+)?(?:i\s+)?(\S+)$")
    for line in path.read_text(encoding="latin-1").splitlines():
        if "Publics by Value" in line:
            mode = "public"
            continue
        if "Static symbols" in line:
            mode = "static"
            continue
        if mode is None or line.startswith(" entry point"):
            continue
        match = pattern.match(line)
        if match:
            rows.append(MapSymbol(int(match[2], 16), match[1], match[3], mode == "static"))
    return rows


def unit_of(tag: str) -> str:
    return tag.rsplit(":", 1)[-1].removesuffix(".obj").removesuffix(".OBJ")


class Identities:
    """Retail RVAs by (unit, name) and by name, from the source claims, the
    proven extra names of folded addresses, the reviewed runtime and zlib
    placements and the synth-PDB inventory's data names."""

    def __init__(self) -> None:
        from homm3.core.tsv import read as read_tsv
        from homm3.retail_labels.fragments import all_claims
        self.by_unit: dict[tuple[str, str], set[int]] = defaultdict(set)
        self.by_name: dict[str, set[int]] = defaultdict(set)

        def add(unit: str, name: str, rva: int) -> None:
            self.by_unit[(unit, name)].add(rva)
            self.by_name[name].add(rva)

        for claim in all_claims():
            add(claim.unit, claim.name, claim.rva)
        identities = ROOT / _image_path("build/gen/address_identities.tsv")
        if identities.is_file():
            for row in read_tsv(identities)[2]:
                add(row.get("unit", ""), row["name"], int(row["rva"], 16))
        for row in read_tsv(ROOT / "config/retail/runtime-contributions.tsv")[2]:
            if row["symbol"] not in ("", "-"):
                member = row["member"].removesuffix(".obj")
                add(member, row["symbol"], int(row["rva"], 16))
        for row in read_tsv(ROOT / "config/retail/zlib-map.tsv")[2]:
            add(row["unit"], row["name"], int(row["rva"], 16))
        inventory = ROOT / _image_path("build/gen/symbol_names.csv")
        if inventory.is_file():
            import csv
            lines = [l for l in inventory.read_text().splitlines() if not l.startswith("#")]
            for row in csv.DictReader(lines):
                if row["provenance"] != "working-label":
                    self.by_name[row["name"]].add(int(row["rva"], 16))

    def lookup(self, unit: str, name: str, static: bool = False) -> set[int]:
        """A static symbol is known only by its own unit's claims."""
        found = self.by_unit.get((unit, name))
        if found or static:
            return found or set()
        return self.by_name.get(name, set())


# ---------------------------------------------------------- contributions ---

@dataclass
class Contribution:
    tag: str
    obj: object
    section: object
    candidate: int | None = None     # VA in the candidate
    retail: int | None = None        # RVA in retail
    name: str = ""
    mates: tuple[str, ...] = ()      # other names ICF folded onto this copy

    @property
    def size(self) -> int:
        return self.section.raw_size

    @property
    def code(self) -> bool:
        return bool(self.section.characteristics & (_EXECUTE | _CODE))


def _objects(tags: set[str], objs_dir: Path, lib_dirs: list[Path]) -> dict[str, object]:
    from homm3.compare.canonicalize import CoffObject
    from homm3.verify.library_code import archive
    out = {}
    archives = {}
    for directory in lib_dirs:
        if directory.is_dir():
            for path in directory.iterdir():
                if path.suffix.lower() == ".lib":
                    archives.setdefault(path.stem.lower(), path)
    for tag in sorted(tags):
        if ":" in tag:
            library, member = tag.split(":", 1)
            path = archives.get(library.lower())
            if path is None:
                continue
            objects, _imports = archive(path)
            obj = objects.get(member) or next(
                (o for n, o in objects.items() if n and n.lower() == member.lower()), None)
            if obj is not None:
                out[tag] = obj
        else:
            path = objs_dir / tag
            if path.is_file():
                out[tag] = CoffObject(path.read_bytes())
    return out


def _linked(section) -> bool:
    flags = section.characteristics
    return not (flags & _UNINITIALIZED or not section.raw_size or flags & 0x800
                or section.name.startswith((".debug", ".drectve")))


def contributions(symbols: list[MapSymbol], identities: Identities, objs_dir: Path,
                  lib_dirs: list[Path]) -> dict[tuple[str, int], Contribution]:
    """{(tag, section number): contribution} of every linked object section,
    placed in the candidate by a map symbol it defines and in retail by that
    symbol's retail identity. `propagate` places the rest."""
    by_tag: dict[str, dict[str, int]] = defaultdict(dict)
    at: dict[tuple[str, int], list[MapSymbol]] = defaultdict(list)
    for sym in symbols:
        by_tag[sym.tag].setdefault(sym.name, sym.va)
        at[(sym.tag, sym.va)].append(sym)
    objects = _objects(set(by_tag), objs_dir, lib_dirs)
    out = {}
    for tag, obj in objects.items():
        placed = by_tag[tag]
        unit = unit_of(tag)
        named: dict[int, list] = defaultdict(list)
        for symbol in obj.symbols.values():
            if symbol.section > 0 and symbol.storage_class in (2, 3) \
                    and not symbol.name.startswith("."):
                named[symbol.section].append(symbol)
        for section in obj.sections:
            if not _linked(section):
                continue
            c = Contribution(tag, obj, section)
            for symbol in sorted(named[section.index], key=lambda s: s.value):
                if c.candidate is None and symbol.name in placed:
                    c.candidate = placed[symbol.name] - symbol.value
                    c.name = c.name or symbol.name
                if c.retail is None:
                    rvas = identities.lookup(unit, symbol.name, symbol.storage_class == 3)
                    if len(rvas) == 1:
                        c.retail = next(iter(rvas)) - symbol.value
                        c.name = symbol.name
            if c.retail is None and c.candidate is not None:
                c.retail = _folded_identity(c, named[section.index], at, identities)
            if not c.name and named[section.index]:
                c.name = min(named[section.index], key=lambda s: s.value).name
            out[(tag, section.index)] = c
    return out


def _folded_identity(c: Contribution, own: list, at: dict, identities: Identities) -> int | None:
    """The retail RVA of an ICF-folded contribution through its fold-mates.

    /OPT:ICF keeps one copy of identical COMDATs and the map lists every
    folded name at that copy's address under the kept copy's object. Which
    copy LINK keeps moves with unrelated inputs, so a group whose kept copy
    has no retail name of its own is still the retail body its mates name.
    When the mates name several retail bodies the lowest stands for the
    group, so the choice does not follow the kept copy either."""
    unit = unit_of(c.tag)
    rvas, mates = set(), set()
    for symbol in own:
        for mate in at.get((c.tag, c.candidate + symbol.value), ()):
            if mate.name != symbol.name:
                mates.add(mate.name)
                rvas |= {rva - symbol.value
                         for rva in identities.lookup(unit, mate.name, mate.static)}
    c.mates = tuple(sorted(mates - {symbol.name for symbol in own}))
    return min(rvas, default=None)


def _fields(c: Contribution):
    for reloc in c.obj.relocations:
        if reloc.section == c.section.index and reloc.typ in (_REL_DIR32, _REL_REL32) \
                and reloc.site + 4 <= c.size:
            yield reloc


def _target(c: Contribution, reloc, image: Pe, at: int) -> int:
    """The address a relocated field of `c` (placed at RVA `at`) reaches."""
    value = struct.unpack("<I", image.read_at(at + reloc.site, 4))[0]
    if reloc.typ == _REL_DIR32:
        return (value - image.base) & 0xFFFFFFFF
    return (at + reloc.site + 4 + value) & 0xFFFFFFFF


def _addend(c: Contribution, reloc) -> int:
    raw = c.obj.section_bytes(c.section)
    return struct.unpack_from("<i", raw, reloc.site)[0]


def _same_bytes(c: Contribution, expected: Pe, candidate: Pe) -> bool:
    got = candidate.read_at(c.candidate - candidate.base, c.size)
    want = expected.read_at(c.retail, c.size)
    if not got or not want or len(got) < c.size or len(want) < c.size:
        return False
    skip = {i for r in _fields(c) if c.obj.symbols[r.symbol_index].section >= 0
            for i in range(r.site, r.site + 4)}
    return all(got[i] == want[i] for i in range(c.size) if i not in skip)


def propagate(contribs: dict[tuple[str, int], Contribution], expected: Pe,
              candidate: Pe) -> int:
    """Place sections through the relocations of placed, byte-identical
    contributions: a field naming a symbol of an unplaced section of the
    same object gives that section's address on each side. Returns the
    number of conflicting placements."""
    conflicts = 0
    queue = [c for c in contribs.values() if c.candidate is not None and c.retail is not None]
    queue.reverse()                       # pop() visits them in object order
    done = set()
    while queue:
        c = queue.pop()
        key = (c.tag, c.section.index)
        if key in done or not _same_bytes(c, expected, candidate):
            continue
        done.add(key)
        for reloc in _fields(c):
            symbol = c.obj.symbols[reloc.symbol_index]
            if symbol.section <= 0:
                continue
            other = contribs.get((c.tag, symbol.section))
            if other is None:
                continue
            addend = _addend(c, reloc) if reloc.typ == _REL_DIR32 else _addend(c, reloc)
            got = _target(c, reloc, candidate, c.candidate - candidate.base) - addend
            want = _target(c, reloc, expected, c.retail) - addend
            got_base = got - symbol.value + candidate.base
            want_base = want - symbol.value
            changed = False
            if other.candidate is None:
                other.candidate, changed = got_base, True
            if other.retail is None:
                other.retail, changed = want_base, True
            elif other.retail != want_base:
                conflicts += 1
            if changed and other.candidate is not None and other.retail is not None:
                queue.append(other)
    return conflicts


class Translator:
    """Candidate RVA -> the retail RVAs it may stand for: those of every
    contribution placed there (ICF-folded twins share one address), else
    the nearest preceding map symbol with a retail identity (COMMON and
    other uninitialized data), and for import slots the import tables."""

    def __init__(self, contribs: list[Contribution], symbols: list[MapSymbol],
                 identities: Identities, expected: Pe, candidate: Pe):
        self.base = candidate.base
        at: dict[tuple[int, int], set[int]] = defaultdict(set)
        for c in contribs:
            if c.candidate is not None and c.retail is not None:
                at[(c.candidate - candidate.base, c.size)].add(c.retail)
        self.spans = sorted((start, size, retails) for (start, size), retails in at.items())
        self.starts = [s for s, _n, _r in self.spans]
        anchors: dict[int, set[int]] = defaultdict(set)
        for sym in symbols:
            for retail in identities.lookup(unit_of(sym.tag), sym.name, sym.static):
                anchors[sym.va - candidate.base].add(retail)
        self.anchors = sorted(anchors.items())
        self.anchor_starts = [a for a, _r in self.anchors]
        retail_slots = {}
        for entry in import_table(expected):
            for name, _hint, slot in entry.thunks:
                retail_slots[(entry.dll.lower(), name)] = slot
        self.slots = {}
        for entry in import_table(candidate):
            for name, _hint, slot in entry.thunks:
                key = (entry.dll.lower(), name)
                if key in retail_slots:
                    self.slots[slot] = retail_slots[key]
        start, size = candidate.directory(12)
        self.iat = (start, start + size)

    def __call__(self, rva: int) -> set[int]:
        if self.iat[0] <= rva < self.iat[1]:
            slot = self.slots.get(rva - rva % 4)
            return set() if slot is None else {slot + rva % 4}
        out: set[int] = set()
        index = bisect.bisect_right(self.starts, rva) - 1
        # The end of a contribution is a valid target (one-past pointers).
        while index >= 0 and self.starts[index] + 0x10000 > rva:
            start, size, retails = self.spans[index]
            if start <= rva <= start + size:
                out |= {retail + rva - start for retail in retails}
            index -= 1
        if out:
            return out
        index = bisect.bisect_right(self.anchor_starts, rva) - 1
        if index >= 0 and rva - self.anchors[index][0] < 0x10000:
            start, retails = self.anchors[index]
            return {retail + rva - start for retail in retails}
        return set()


def compare(c: Contribution, expected: Pe, candidate: Pe, translate: Translator,
            identities: Identities) -> tuple[int, int, int]:
    """(differing bytes, untranslatable relocated fields, fields whose target
    is placed elsewhere) of one contribution at its retail address.

    A relocated field matches when the candidate's target stands for the
    retail target. A field whose symbol's own retail identity is the retail
    target, but whose linked target is placed elsewhere, is a placement
    finding of the target (counted by the target's own contribution), not a
    difference of this one."""
    size = c.size
    got = candidate.read_at(c.candidate - candidate.base, size)
    want = expected.read_at(c.retail, size)
    if got is None or want is None or len(want) < size or len(got) < size:
        return size, 0, 0
    unit = unit_of(c.tag)
    differing = unknown = elsewhere = 0
    skip = set()
    for reloc in _fields(c):
        if c.obj.symbols[reloc.symbol_index].section < 0:
            continue                      # an absolute symbol: compare the bytes
        skip.update(range(reloc.site, reloc.site + 4))
        targets = translate(_target(c, reloc, candidate, c.candidate - candidate.base))
        retail_target = _target(c, reloc, expected, c.retail)
        if retail_target in targets:
            continue
        symbol = c.obj.symbols[reloc.symbol_index]
        named = identities.lookup(unit, symbol.name, symbol.storage_class == 3)
        if named and retail_target - _addend(c, reloc) in named:
            elsewhere += 1
        elif not targets:
            unknown += 1
        else:
            differing += 4
    differing += sum(1 for i in range(size) if i not in skip and got[i] != want[i])
    return differing, unknown, elsewhere


# ----------------------------------------------------------------- driver ---

def ledger_scores() -> dict[int, float]:
    out = {}
    for line in (ROOT / _image_path("config/match_baseline.tsv")).read_text().splitlines():
        cells = line.split("\t")
        if len(cells) > 5 and cells[5].startswith("0x"):
            out[int(cells[5], 16)] = float(cells[2])
    return out


def retail_code(path: Path = ROOT / "config/retail/functions.tsv") -> list[tuple[int, int]]:
    from homm3.core.tsv import read as read_tsv
    return [(int(r["rva"], 16), int(r["size"])) for r in read_tsv(path)[2]]


@dataclass
class Report:
    counts: dict[str, int]
    details: dict[str, list[str]]
    #: code-unplaced bytes by `unplaced_key`, gated against config/link_unplaced.tsv
    unplaced: dict[tuple[str, str], int] = field(default_factory=dict)


_INITIALIZER = re.compile(r"_\$E\d+")
_ANONYMOUS = re.compile(r"\?%[^@]*")


def unplaced_key(c: Contribution) -> tuple[str, str]:
    """A stable ledger key for an unplaced code contribution: VC6 numbers a
    unit's dynamic initializers (`_$E<n>`) in source order, so they share one
    row per unit; an anonymous namespace spells the build path, which is
    dropped; an ICF-folded group is named by its lowest name, whichever copy
    LINK kept, and only unit-local names keep their unit."""
    unit = unit_of(c.tag)
    if _INITIALIZER.fullmatch(c.name):
        return unit, "_$E*"
    name = _ANONYMOUS.sub("?%anon", min((c.name, *c.mates)))
    return (unit if "?%anon" in name or name.startswith("_$") else "*"), name


def measure(candidate_path: Path = CANDIDATE) -> Report:
    retail = common.load_image()[0].data
    edits = read_edits()
    view, findings = link_view(bytes(retail), edits)
    expected = Pe.read(view)
    candidate = Pe.read(candidate_path.read_bytes())
    details: dict[str, list[str]] = defaultdict(list)
    details["edits"] = findings
    counts = raw_regions(expected, candidate)
    counts["rich"], details["rich"] = rich_difference(expected.data, candidate.data)
    counts["imports"], details["imports"] = import_difference(expected, candidate, edits)

    symbols = read_map(candidate_path.with_suffix(".map"))
    from homm3.core.cc_wrap import msvc_dir
    identities = Identities()
    contribs = contributions(symbols, identities, ROOT / _image_path("build/objdiff/base"),
                             [candidate_path.parent, msvc_dir() / "lib"])
    counts["placement-conflicts"] = propagate(contribs, expected, candidate)
    linked = [c for c in contribs.values() if c.candidate is not None]
    translate = Translator(linked, symbols, identities, expected, candidate)
    scores = ledger_scores()
    for region in ("code-exact", "code-nonexact", "code-unplaced", "data",
                   "data-unplaced", "untranslated", "target-elsewhere"):
        counts[region] = 0
    seen: set[tuple[int, int | None]] = set()
    unplaced: dict[tuple[str, str], int] = {}
    covered: list[tuple[int, int]] = []
    order = []
    for c in sorted(linked, key=lambda c: (c.candidate, c.tag, c.section.index)):
        if (c.candidate, c.retail) in seen:
            continue                                  # an ICF-folded twin
        seen.add((c.candidate, c.retail))
        if c.retail is None:
            region = "code-unplaced" if c.code else "data-unplaced"
            counts[region] += c.size
            details[region].append(f"{c.candidate:#x} {c.tag} {c.section.name} "
                                   f"{c.name} ({c.size} B)")
            if c.code:
                key = unplaced_key(c)
                unplaced[key] = unplaced.get(key, 0) + c.size
            continue
        if c.code:
            order.append((c.candidate, c.retail, c))
            covered.append((c.retail, c.retail + c.size))
        diff, unknown, elsewhere = compare(c, expected, candidate, translate, identities)
        counts["untranslated"] += unknown
        counts["target-elsewhere"] += elsewhere
        if unknown:
            details["untranslated"].append(f"{c.retail + expected.base:#x} {c.tag} {c.name}: "
                                           f"{unknown} field(s)")
        if not diff:
            continue
        line = f"{c.retail + expected.base:#x} {c.tag} {c.name}: {diff} B"
        if not c.code:
            counts["data"] += diff
            details["data"].append(line)
        elif scores.get(c.retail, 100.0) < 100.0:
            counts["code-nonexact"] += diff
            details["code-nonexact"].append(line)
        else:
            counts["code-exact"] += diff
            details["code-exact"].append(line)
    breaks = 0
    for (_a, ra, ca), (_b, rb, cb) in zip(order, order[1:]):
        if rb < ra:
            breaks += 1
            details["code-order"].append(
                f"{cb.retail + expected.base:#x} {cb.tag} {cb.name} follows "
                f"{ca.retail + expected.base:#x} {ca.tag} {ca.name}")
    counts["code-order"] = breaks
    covered.sort()
    starts = [a for a, _b in covered]
    absent = 0
    text = expected.section(".text")
    for rva, size in retail_code():
        if not text[1] <= rva < text[1] + text[2]:
            continue
        index = bisect.bisect_right(starts, rva) - 1
        if index >= 0 and covered[index][0] <= rva < covered[index][1]:
            continue
        absent += size
        details["code-absent"].append(f"{rva + expected.base:#x} ({size} B)")
    counts["code-absent"] = absent
    rsrc_want, rsrc_got = expected.section(".rsrc"), candidate.section(".rsrc")
    if rsrc_got is None:
        counts["rsrc"] = rsrc_want[2]
    else:
        a = expected.data[rsrc_want[3]:rsrc_want[3] + rsrc_want[2]]
        b = candidate.data[rsrc_got[3]:rsrc_got[3] + rsrc_got[2]]
        counts["rsrc"] = _rsrc_difference(a, rsrc_want[1], b, rsrc_got[1])
    return Report(counts, details, unplaced)


def _rsrc_difference(want: bytes, want_rva: int, got: bytes, got_rva: int) -> int:
    """Resource bytes compared after rebasing the data-entry RVAs."""
    def rebased(blob: bytes, rva: int) -> bytes:
        out = bytearray(blob)

        def walk(at: int) -> None:
            named, ids = struct.unpack_from("<HH", blob, at + 12)
            for i in range(named + ids):
                _name, child = struct.unpack_from("<II", blob, at + 16 + 8 * i)
                if child & 0x80000000:
                    walk(child & 0x7FFFFFFF)
                else:
                    data = struct.unpack_from("<I", blob, child)[0]
                    struct.pack_into("<I", out, child, data - rva)
        try:
            walk(0)
        except struct.error:
            pass
        return bytes(out)
    return _differing(rebased(want, want_rva), rebased(got, got_rva))


# --------------------------------------------------------------- ceiling ---

HEADER = ("# Highest count each gated region of `homm3 verify link-diff` may reach.\n"
          "# Written by `homm3 verify link-diff --update`; lower it when the link\n"
          "# moves closer to retail, never raise it to admit a regression.\n"
          "region\tcount\n")


def read_ceiling(path: Path = CEILING) -> dict[str, int]:
    if not path.is_file():
        return {}
    from homm3.core.tsv import read as read_tsv
    return {row["region"]: int(row["count"]) for row in read_tsv(path)[2]}


def write_ceiling(counts: dict[str, int], path: Path = CEILING) -> None:
    path.write_text(HEADER + "".join(f"{name}\t{counts[name]}\n"
                                     for name in GATED + TRACKED))


def tracked_rises(report: Report, ceiling: dict[str, int]) -> list[str]:
    """TRACKED regions above their banked count (reported, not fatal)."""
    return [f"{name}: {report.counts[name]} > banked {ceiling[name]}"
            for name in TRACKED if name in ceiling and report.counts[name] > ceiling[name]]


def gate_findings(report: Report, ceiling: dict[str, int],
                  unplaced: dict[tuple[str, str], tuple[int, str]] | None = None) -> list[str]:
    out = list(report.details.get("edits", []))
    for name in GATED:
        count = report.counts[name]
        limit = ceiling.get(name)
        if limit is None:
            out.append(f"{name}: {count}, no banked ceiling")
        elif count > limit:
            out.append(f"{name}: {count} > ceiling {limit}")
    if unplaced is not None:
        out += unplaced_findings(report, unplaced)
    return out


# -------------------------------------------------------- unplaced ledger ---

UNPLACED_HEADER = (
    "# Every code-unplaced contribution of `homm3 verify link-diff` (candidate\n"
    "# code with no retail identity), keyed by `unplaced_key`, with the most\n"
    "# bytes it may take and why it has no retail identity. `homm3 build` fails\n"
    "# on a key without a row, above its bytes or without a reason.\n"
    "# `homm3 verify link-diff --update` rewrites the bytes; a new or grown row\n"
    "# gets an empty reason that must be written by hand before the gate passes.\n"
    "unit\tname\tbytes\treason\n")


def read_unplaced(path: Path = UNPLACED) -> dict[tuple[str, str], tuple[int, str]]:
    if not path.is_file():
        return {}
    from homm3.core.tsv import read as read_tsv
    return {(row["unit"], row["name"]): (int(row["bytes"]), row["reason"])
            for row in read_tsv(path)[2]}


def unplaced_findings(report: Report,
                      ledger: dict[tuple[str, str], tuple[int, str]]) -> list[str]:
    out = []
    for key, size in sorted(report.unplaced.items()):
        row = ledger.get(key)
        where = f"code-unplaced {key[0]} {key[1]} ({size} B)"
        if row is None:
            out.append(f"{where}: no row in {UNPLACED.relative_to(ROOT)}")
        elif size > row[0]:
            out.append(f"{where}: above its ledger bytes {row[0]}")
        elif not row[1].strip():
            out.append(f"{where}: ledger row has no reason")
    return out


def write_unplaced(report: Report, ledger: dict[tuple[str, str], tuple[int, str]],
                   path: Path = UNPLACED) -> list[tuple[str, str]]:
    """Rewrite the ledger from the report; returns the keys left unexplained
    (new, grown, or never given a reason)."""
    rows, unexplained = [], []
    for key, size in sorted(report.unplaced.items()):
        old = ledger.get(key)
        reason = old[1] if old is not None and size <= old[0] else ""
        if not reason.strip():
            unexplained.append(key)
        rows.append(f"{key[0]}\t{key[1]}\t{size}\t{reason}\n")
    path.write_text(UNPLACED_HEADER + "".join(rows))
    return unexplained


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="homm3 verify link-diff",
                                     description=__doc__.splitlines()[0])
    parser.add_argument("--candidate", type=Path, default=CANDIDATE)
    parser.add_argument("--update", action="store_true",
                        help="bank the current gated counts as the ceiling")
    parser.add_argument("--detail", action="append", default=[],
                        help="print one region's findings (repeatable)")
    args = parser.parse_args(argv)
    if not args.candidate.is_file():
        print(f"[link-diff] no candidate at {args.candidate}; run `homm3 link`",
              file=sys.stderr)
        return 1
    report = measure(args.candidate)
    ceiling = read_ceiling()
    for name, count in report.counts.items():
        banked = name in GATED + TRACKED
        limit = ceiling.get(name) if banked else None
        mark = ("" if not banked else
                " (no ceiling)" if limit is None else
                " (above ceiling)" if count > limit else
                " (bankable)" if count < limit else "")
        shown = "" if limit is None else f"  ceiling {limit}"
        tag = ("" if name in GATED else "  [tracked]" if name in TRACKED
               else "  [informational]")
        print(f"[link-diff] {name:14} {count:9d}{shown}{mark}{tag}")
    for name in args.detail:
        for line in report.details.get(name, []):
            print(f"  {name}: {line}")
    if args.update:
        if report.details.get("edits"):
            for line in report.details["edits"]:
                print(f"[link-diff] {line}", file=sys.stderr)
            return 1
        write_ceiling(report.counts)
        print(f"[link-diff] ceiling written to {CEILING.relative_to(ROOT)}")
        unexplained = write_unplaced(report, read_unplaced())
        print(f"[link-diff] unplaced ledger written to {UNPLACED.relative_to(ROOT)}")
        for unit, name in unexplained:
            print(f"[link-diff] explain code-unplaced {unit} {name} "
                  f"({report.unplaced[(unit, name)]} B) in its ledger row", file=sys.stderr)
        return 1 if unexplained else 0
    findings = gate_findings(report, ceiling, read_unplaced())
    for line in findings:
        print(f"[link-diff] REGRESSION {line}", file=sys.stderr)
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
