"""Compare the linked h3maped (`homm3 loki link`) with the retail image, section by section.

The goal is zero differing bytes over the whole file. The report breaks the
difference down so each kind of mismatch has its own line:

- the ELF header and program headers (segment layout, entry point);
- every section header (address, offset, size) and the bytes of each section,
  compared at each file's own section offset;
- .text by the census objects (config/retail/h3maped-loki/objects.tsv): the
  start files, each project object, the C libraries and the libstdc++/libgcc
  members, compared at the image's addresses;
- .dynsym order, .dynstr, .hash and the PLT's imported-symbol order;
- the .comment sequence (one entry per linked object, in link order);
- library member order: the C libraries' exported functions, in the image's
  address order, against their order in the linked file.

check() is the link gate: an address-translating, section-by-section alignment
whose remaining differences must all be stated, within their byte counts, in
config/retail/h3maped-loki/link-differences.toml.
"""
from __future__ import annotations

from dataclasses import dataclass
import bisect
import itertools
from pathlib import Path
import re
import struct

from homm3.loki import delink, link
from homm3.loki.elf import SHF_ALLOC, SHT_NOBITS, STT_FUNC, Elf
from homm3.loki.image import LokiImage


@dataclass(frozen=True)
class SectionDiff:
    name: str
    retail: tuple[int, int, int] | None   # addr, offset, size
    linked: tuple[int, int, int] | None
    differing: int                       # bytes (or size difference for NOBITS)
    total: int


def differing(a: bytes, b: bytes) -> int:
    common = min(len(a), len(b))
    return sum(1 for x, y in zip(a[:common], b[:common]) if x != y) + abs(len(a) - len(b))


def program_headers(elf: Elf) -> list[tuple]:
    phoff, = struct.unpack_from("<I", elf.data, 28)
    phentsize, phnum = struct.unpack_from("<HH", elf.data, 42)
    return [struct.unpack_from("<IIIIIIII", elf.data, phoff + i * phentsize) for i in range(phnum)]


def header_differences(retail: Elf, linked: Elf) -> list[str]:
    out = []
    if retail.entry != linked.entry:
        out.append(f"entry 0x{retail.entry:x} vs 0x{linked.entry:x}")
    a, b = program_headers(retail), program_headers(linked)
    if len(a) != len(b):
        out.append(f"{len(a)} vs {len(b)} program headers")
    fields = ("type", "offset", "vaddr", "paddr", "filesz", "memsz", "flags", "align")
    for i, (x, y) in enumerate(zip(a, b)):
        changed = [f"{f} 0x{u:x}/0x{v:x}" for f, u, v in zip(fields, x, y) if u != v]
        if changed:
            out.append(f"phdr {i}: " + ", ".join(changed))
    return out


def sections(retail: Elf, linked: Elf) -> list[SectionDiff]:
    names = [s.name for s in retail.sections if s.name] + \
            [s.name for s in linked.sections if s.name and s.name not in {r.name for r in retail.sections}]
    out = []
    for name in names:
        r = next((s for s in retail.sections if s.name == name), None)
        l = next((s for s in linked.sections if s.name == name), None)
        if r is None or l is None:
            present = r or l
            out.append(SectionDiff(name, r and (r.addr, r.offset, r.size), l and (l.addr, l.offset, l.size),
                                   present.size, present.size))
            continue
        if r.type == SHT_NOBITS or l.type == SHT_NOBITS:
            count = abs(r.size - l.size)
        else:
            count = differing(retail.bytes(r), linked.bytes(l))
        out.append(SectionDiff(name, (r.addr, r.offset, r.size), (l.addr, l.offset, l.size), count,
                               max(r.size, l.size)))
    return out


def text_by_object(retail: Elf, linked: Elf) -> list[tuple[str, int, int]]:
    """(group, differing, total) over the image's .text, compared at the same addresses."""
    text = retail.section(".text")
    try:
        other = linked.section(".text")
    except ValueError:
        return []
    ranges = sorted(delink.objects().items(), key=lambda item: item[1][0])
    bounds: list[tuple[int, str]] = [(text.addr, "start files")]
    for obj, (start, _end, _file) in ranges:
        group = f"object {obj}" if obj <= 102 else "libstdc++/libgcc"
        bounds.append((start, group))
        if obj == 102:
            bounds.append((_end, "C libraries"))
    stop = text.addr + text.size
    a = retail.bytes(text)
    b = linked.bytes(other)
    totals: dict[str, list[int]] = {}
    for i, (start, group) in enumerate(bounds):
        end = bounds[i + 1][0] if i + 1 < len(bounds) else stop
        theirs = a[start - text.addr:end - text.addr]
        lo, hi = start - other.addr, end - other.addr
        mine = b[lo:hi] if lo >= 0 else b""
        entry = totals.setdefault(group, [0, 0])
        entry[0] += differing(theirs, mine)
        entry[1] += len(theirs)
    return [(group, d, t) for group, (d, t) in totals.items()]


TEXT_GROUPS = ("start files", "project objects", "C libraries", "libstdc++/libgcc", "linkonce")
_MAP_INPUT = re.compile(r"^ (\.text|\.gnu\.linkonce\.t\S*)\s*\n?\s+0x([0-9a-f]+)\s+0x([0-9a-f]+) (\S+)", re.M)


def retail_text_layout(image: LokiImage) -> dict[str, int]:
    """Bytes of each .text group in the image, padding to the next group included."""
    text = image.elf.section(".text")
    objects = delink.objects()
    project_end = objects[102][1]
    cxx_start = objects[min(o for o in objects if o > 102)][0]
    bounds = [text.addr, objects[0][0], project_end, cxx_start, image.linkonce_start, text.addr + text.size]
    return {group: bounds[i + 1] - bounds[i] for i, group in enumerate(TEXT_GROUPS)}


def linked_text_layout(map_text: str) -> dict[str, int]:
    """The same groups of the linked .text, from ld's map (input sections and fill)."""
    start = map_text.index("\n.text ")
    body = map_text[start:map_text.index("\n.fini", start)]
    header = re.match(r"\n\.text\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)", body)
    end = int(header.group(1), 16) + int(header.group(2), 16)
    inputs = [(m.group(1), int(m.group(2), 16), m.group(4)) for m in _MAP_INPUT.finditer(body)]
    first: dict[str, int] = {}
    for section, address, path in inputs:
        name = path.rsplit("/", 1)[-1]
        if section != ".text":
            group = "linkonce"
        elif "/obj/" in path:
            group = "project objects"
        elif name.startswith(("libstdc++", "libgcc")) or name in ("crtend.o", "crtn.o"):
            group = "libstdc++/libgcc"
        elif name.startswith("lib"):
            group = "C libraries"
        else:
            group = "start files"
        first.setdefault(group, address)
    starts = [first.get(g, end) for g in TEXT_GROUPS] + [end]
    return {group: starts[i + 1] - starts[i] for i, group in enumerate(TEXT_GROUPS)}


def plt_names(elf: Elf) -> list[str]:
    return [name for _, name in sorted(elf.plt_slots.items())]


def comments(elf: Elf) -> list[str]:
    try:
        return elf.comments()
    except ValueError:
        return []


def runs(items: list[str]) -> list[tuple[int, str]]:
    return [(len(list(group)), key) for key, group in itertools.groupby(items)]


def longest_increasing(values: list[int]) -> int:
    tails: list[int] = []
    for v in values:
        i = bisect.bisect_left(tails, v)
        tails[i:i + 1] = [v]
    return len(tails)


def library_order(retail: Elf, linked: Elf) -> tuple[int, int, int]:
    """(in order, present in both, retail total) for the C libraries' exported functions."""
    objects = delink.objects()
    lo, hi = objects[102][1], objects[min(o for o in objects if o > 102)][0]
    names = sorted((s.value, s.name) for s in retail.dynsym if s.type == STT_FUNC and lo <= s.value < hi)
    ours = {s.name: s.value for s in linked.dynsym if s.type == STT_FUNC and s.value}
    ranks = [ours[name] for _, name in names if name in ours]
    return longest_increasing(ranks), len(ranks), len(names)


def report(linked_path: Path = link.IMAGE, verbose: bool = False) -> int:
    """Print the comparison; returns the total differing bytes over the file."""
    retail = LokiImage().elf
    linked = Elf(linked_path.read_bytes())
    total = differing(retail.data, linked.data)
    print(f"[loki] link-diff: {linked_path.name} {len(linked.data)} bytes vs retail {len(retail.data)}; "
          f"{total} differing bytes over the file")
    for line in header_differences(retail, linked):
        print(f"  header: {line}")
    print(f"  {'section':20} {'retail addr/off/size':30} {'linked addr/off/size':30} differing")
    for diff in sections(retail, linked):
        fmt = lambda t: "-" if t is None else f"{t[0]:08x}/{t[1]:06x}/{t[2]:06x}"
        if diff.differing or verbose:
            print(f"  {diff.name:20} {fmt(diff.retail):30} {fmt(diff.linked):30} {diff.differing}/{diff.total}")
    map_path = linked_path.with_suffix(".map")
    if map_path.is_file():
        ours = linked_text_layout(map_path.read_text(encoding="latin-1"))
        theirs = retail_text_layout(LokiImage())
        print("  .text layout (bytes): " + ", ".join(
            f"{g} {theirs[g]:#x}" + ("" if theirs[g] == ours[g] else f" vs {ours[g]:#x}") for g in TEXT_GROUPS))
    rows = text_by_object(retail, linked)
    exact = sum(1 for _, d, _ in rows if d == 0)
    print(f"  .text by object: {exact}/{len(rows)} groups identical at the image's addresses")
    for group, d, t in rows:
        if d and (verbose or not group.startswith("object")):
            print(f"    {group:20} {d}/{t}")
    project = [(g, d, t) for g, d, t in rows if g.startswith("object")]
    print(f"    project objects     {sum(d for _, d, _ in project)}/{sum(t for _, _, t in project)} "
          f"({sum(1 for _, d, _ in project if d == 0)}/{len(project)} identical)")
    a = [s.name for s in retail.dynsym]
    b = [s.name for s in linked.dynsym]
    same = sum(1 for x, y in zip(a, b) if x == y)
    print(f"  .dynsym: {len(a)} vs {len(b)} symbols, {same} at the same index; "
          f"{len(set(a) ^ set(b))} names in only one")
    pa, pb = plt_names(retail), plt_names(linked)
    print(f"  PLT: {len(pa)} vs {len(pb)} imports, {sum(1 for x, y in zip(pa, pb) if x == y)} in the same slot")
    ca, cb = runs(comments(retail)), runs(comments(linked))
    print(f"  .comment: {sum(n for n, _ in ca)} vs {sum(n for n, _ in cb)} entries; runs "
          + ("identical" if ca == cb else f"{ca} vs {cb}"))
    in_order, both, count = library_order(retail, linked)
    print(f"  C library order: {in_order}/{both} exported functions in the image's order "
          f"({count - both} of {count} absent from the link)")
    return total


# -- the gate ---------------------------------------------------------------------------------------
#
# A raw byte comparison counts one displaced object once for every byte after it. The gate
# instead aligns each allocated section piece by piece (the .dynsym addresses of the exported
# symbols anchor every piece; between two anchors whose displacements differ, the split points
# that leave the fewest differing bytes are chosen, with at most one intermediate displacement)
# and accepts a differing 4-byte word only when it is the image's word translated by that map:
# an absolute address, or a GOT-relative offset of PIC code, of a displaced byte. Every byte that
# still differs, and every linked byte that no image byte maps to, must fall in a difference
# stated in config/retail/h3maped-loki/link-differences.toml, within its byte count.

FACTS = delink.RETAIL / "link-differences.toml"


@dataclass(frozen=True)
class Piece:
    start: int        # image address where this displacement begins
    delta: int        # linked address - image address


@dataclass
class Span:
    where: str        # a section name, "ELF header", "section headers" or "file padding"
    start: int        # image address (allocated sections), else image file offset
    end: int
    linked: bool = False    # bytes only in the linked file, at linked addresses


def _equal(rb: bytes, ob: bytes, i: int, j: int) -> bool:
    return 0 <= j < len(ob) and ob[j] == rb[i]


def _split(rb: bytes, ob: bytes, r_addr: int, o_addr: int, a: int, b: int, da: int, db: int) -> list[Piece]:
    """Pieces covering [a, b) that move from displacement da to db with the fewest mismatches."""
    n = b - a

    def misses(d: int) -> list[int]:
        prefix = [0]
        for x in range(a, b):
            prefix.append(prefix[-1] + (not _equal(rb, ob, x - r_addr, x + d - o_addr)))
        return prefix

    pa, pb = misses(da), misses(db)
    best = None
    for dm in range(min(da, db), max(da, db) + 1):
        pm = pa if dm == da else pb if dm == db else misses(dm)
        run_min, arg = None, 0
        for s2 in range(n + 1):
            first = pa[s2] - pm[s2]
            if run_min is None or first < run_min:
                run_min, arg = first, s2
            cost = run_min + pm[s2] + (pb[n] - pb[s2])
            if best is None or cost < best[0]:
                best = (cost, a + arg, a + s2, dm)
    _, s1, s2, dm = best
    pieces = [Piece(s1, dm)] if s1 < s2 and dm not in (da, db) else []
    return pieces + [Piece(s2, db)]


def pieces(retail: Elf, linked: Elf, name: str) -> list[Piece]:
    r, o = retail.section(name), linked.section(name)
    if r.type == SHT_NOBITS:
        return [Piece(r.addr, o.addr - r.addr)]
    rb, ob = retail.bytes(r), linked.bytes(o)
    ours = {s.name: s.value for s in linked.dynsym if s.value and o.contains(s.value)}
    anchors = {r.addr: o.addr - r.addr}
    for s in retail.dynsym:
        if s.value and r.contains(s.value) and s.name in ours:
            anchors.setdefault(s.value, ours[s.name] - s.value)
    anchors[r.addr + r.size] = o.addr + o.size - (r.addr + r.size)
    points = sorted(anchors.items())
    out = [Piece(*points[0])]
    for (a, _), (b, db) in zip(points, points[1:]):
        if db != out[-1].delta:
            out += _split(rb, ob, r.addr, o.addr, a, b, out[-1].delta, db)
    return out


class AddressMap:
    def __init__(self, retail: Elf, linked: Elf):
        self.sections = [s for s in retail.sections if s.flags & SHF_ALLOC and s.name]
        self.pieces = {s.name: pieces(retail, linked, s.name) for s in self.sections}
        self.got = retail.section(".got").addr, linked.section(".got").addr

    def translate(self, value: int) -> int | None:
        for section in self.sections:
            if section.addr <= value <= section.addr + section.size:
                found = self.pieces[section.name]
                index = bisect.bisect_right([p.start for p in found], value) - 1
                return value + found[max(index, 0)].delta
        return None

    def explains(self, image_word: int, linked_word: int) -> bool:
        if image_word == linked_word:
            return False
        if self.translate(image_word) == linked_word:
            return True
        target = self.translate((image_word + self.got[0]) & 0xffffffff)
        return target is not None and (target - self.got[1]) & 0xffffffff == linked_word


def _compare(rb: bytes, ob: bytes, segments: list[tuple[int, int, int]], explains) -> list[int]:
    """Image indices whose byte differs and is not part of a translated address word.
    segments: (start, end, shift): image index i in [start, end) pairs with linked index i + shift."""
    starts = [a for a, _, _ in segments]

    def linked_index(i: int) -> int | None:
        k = bisect.bisect_right(starts, i) - 1
        if k < 0 or i >= segments[k][1]:
            return None
        return i + segments[k][2]

    out = []
    for start, end, shift in segments:
        for chunk in range(start, end, 4096):
            stop = min(chunk + 4096, end)
            if 0 <= chunk + shift and stop + shift <= len(ob) and rb[chunk:stop] == ob[chunk + shift:stop + shift]:
                continue
            for i in range(chunk, stop):
                if _equal(rb, ob, i, i + shift):
                    continue
                explained = False
                for k in range(4):
                    y = i - k
                    z = linked_index(y) if y >= 0 else None
                    if z is None or y + 4 > len(rb) or z < 0 or z + 4 > len(ob):
                        continue
                    if explains(struct.unpack_from("<I", rb, y)[0], struct.unpack_from("<I", ob, z)[0]):
                        explained = True
                        break
                if not explained:
                    out.append(i)
    return out


def _runs(indices: list[int]) -> list[tuple[int, int]]:
    out: list[list[int]] = []
    for i in indices:
        if out and i == out[-1][1]:
            out[-1][1] = i + 1
        else:
            out.append([i, i + 1])
    return [(a, b) for a, b in out]


def unexplained(retail: Elf, linked: Elf) -> list[Span]:
    """Every byte the address map does not account for, as spans."""
    amap = AddressMap(retail, linked)
    spans: list[Span] = []
    covered: list[tuple[int, int]] = []
    for r in retail.sections[1:]:
        o = linked.section(r.name)
        if r.type == SHT_NOBITS:
            if r.size != o.size:
                spans.append(Span(r.name, r.addr + min(r.size, o.size), r.addr + max(r.size, o.size)))
            continue
        covered.append((r.offset, r.offset + r.size))
        rb, ob = retail.bytes(r), linked.bytes(o)
        if not r.flags & SHF_ALLOC:
            spans += [Span(r.name, a, b) for a, b in _runs(_compare(rb, ob, [(0, r.size, 0)], amap.explains))]
            continue
        found = amap.pieces[r.name] + [Piece(r.addr + r.size, 0)]
        segments = [(p.start - r.addr, q.start - r.addr, p.delta - (o.addr - r.addr))
                    for p, q in zip(found, found[1:]) if p.start < q.start]
        spans += [Span(r.name, r.addr + a, r.addr + b) for a, b in _runs(_compare(rb, ob, segments, amap.explains))]
        image = sorted((a + shift, b + shift) for a, b, shift in segments)
        linked_only, cursor = [], 0
        for a, b in image + [(o.size, o.size)]:
            linked_only += range(cursor, max(cursor, min(a, o.size)))
            cursor = max(cursor, b)
        spans += [Span(r.name, o.addr + a, o.addr + b, linked=True) for a, b in _runs(linked_only)]
    # The ELF and program headers (file start) and the section header table (file end).
    headers = (0, retail.sections[1].offset)
    for label, (start, end), other in (("ELF header", headers, 0),
                                       ("section headers", (retail.shoff, retail.shoff + retail.shnum * 40),
                                        linked.shoff)):
        covered.append((start, end))
        rb, ob = retail.data[start:end], linked.data[other:other + end - start]
        spans += [Span(label, start + a, start + b)
                  for a, b in _runs(_compare(rb, ob, [(0, end - start, 0)], amap.explains))]
    # File padding between sections: compared with the linked file's padding before the same section.
    covered.sort()
    for (_, a), (b, _) in zip(covered, covered[1:]):
        if a < b:
            nxt = next(s for s in retail.sections if s.offset == b and s.type != SHT_NOBITS) \
                if any(s.offset == b for s in retail.sections) else None
            other = linked.section(nxt.name).offset - (b - a) if nxt else a
            rb, ob = retail.data[a:b], linked.data[other:other + b - a]
            spans += [Span("file padding", a + i, a + j) for i, j in _runs([k for k in range(b - a)
                                                                            if not _equal(rb, ob, k, k)])]
    if len(retail.data) != len(linked.data):
        spans.append(Span("file size", len(retail.data), max(len(retail.data), len(linked.data))))
    return spans


@dataclass
class Region:
    difference: str
    where: str
    label: str
    ranges: list[tuple[int, int]] | None   # None: the whole of `where`
    linked: bool
    bytes: int
    found: int = 0

    def holds(self, span: Span, x: int) -> bool:
        return (self.where == span.where and self.linked == span.linked
                and (self.ranges is None or any(a <= x < b for a, b in self.ranges)))


DYNSYM_FIELDS = {"st_name": (0, 4), "st_value": (4, 8), "st_size": (8, 12), "st_info": (12, 16)}


def facts(retail: Elf, path: Path = FACTS) -> tuple[list[dict], list[Region]]:
    """The stated differences. A region is `where` (a section, "ELF header", "section headers")
    with an image address range [start, end) (`linked = true`: linked addresses of bytes only
    the linked file has), or .dynsym records: `symbols` by name or every undefined symbol whose
    name starts with `imports`, optionally one `field` of each record."""
    import tomllib
    with path.open("rb") as stream:
        entries = tomllib.load(stream).get("difference", [])
    regions = []
    dynsym = retail.section(".dynsym")
    for entry in entries:
        for raw in entry.get("region", []):
            ranges, label = None, raw["where"]
            if "start" in raw:
                ranges = [(raw["start"], raw["end"])]
                label += f" 0x{raw['start']:x}..0x{raw['end']:x}"
            elif "symbols" in raw or "imports" in raw:
                lo, hi = DYNSYM_FIELDS.get(raw.get("field", ""), (0, 16))
                chosen = [s for s in retail.dynsym if s.name and (
                    s.name in raw.get("symbols", ()) or
                    ("imports" in raw and s.shndx == 0 and s.name.startswith(raw["imports"])))]
                ranges = [(dynsym.addr + s.index * 16 + lo, dynsym.addr + s.index * 16 + hi) for s in chosen]
                what = ", ".join(raw["symbols"]) if "symbols" in raw else f"{len(chosen)} {raw['imports']}* imports"
                label += f" {what}" + (f" {raw['field']}" if "field" in raw else "")
            if raw.get("linked"):
                label += " (linked only)"
            regions.append(Region(entry["id"], raw["where"], label, ranges, raw.get("linked", False), raw["bytes"]))
    return entries, regions


def check(linked_path: Path = link.IMAGE, facts_path: Path = FACTS) -> tuple[bool, list[str]]:
    """The gate: every unexplained byte lies in a stated region, within its byte count."""
    retail = LokiImage().elf
    linked = Elf(linked_path.read_bytes())
    entries, regions = facts(retail, facts_path)
    spans = unexplained(retail, linked)
    unstated: list[Span] = []
    for span in spans:
        for x in range(span.start, span.end):
            region = next((g for g in regions if g.holds(span, x)), None)
            if region is None:
                if unstated and unstated[-1].where == span.where and unstated[-1].end == x \
                        and unstated[-1].linked == span.linked:
                    unstated[-1].end = x + 1
                else:
                    unstated.append(Span(span.where, x, x + 1, span.linked))
            else:
                region.found += 1
    lines = []
    ok = not unstated
    for region in regions:
        place = region.label
        status = "ok" if region.found <= region.bytes else "EXCEEDED"
        if region.found > region.bytes:
            ok = False
        lines.append(f"  {region.difference:24} {place:48} {region.found:5}/{region.bytes} bytes {status}")
    for span in unstated[:40]:
        where = f"{span.where} 0x{span.start:x}..0x{span.end:x}" + (" (linked only)" if span.linked else "")
        lines.append(f"  UNSTATED {where}: {span.end - span.start} bytes")
    if len(unstated) > 40:
        lines.append(f"  ... {len(unstated) - 40} more unstated spans")
    stated = sum(r.found for r in regions)
    unstated_bytes = sum(s.end - s.start for s in unstated)
    lines.insert(0, f"[loki] link gate: {stated} bytes in {len(entries)} stated differences, "
                    f"{unstated_bytes} unstated -> {'PASS' if ok else 'FAIL'}")
    return ok, lines
