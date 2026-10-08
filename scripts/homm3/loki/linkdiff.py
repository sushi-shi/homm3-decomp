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
"""
from __future__ import annotations

from dataclasses import dataclass
import bisect
import itertools
from pathlib import Path
import re
import struct

from homm3.loki import delink, link
from homm3.loki.elf import SHT_NOBITS, STT_FUNC, Elf
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
