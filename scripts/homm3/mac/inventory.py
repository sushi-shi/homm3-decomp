"""Whole-code-section byte inventory and function-boundary candidate census.

Every byte of PEF code section 0 falls in exactly one region:

  function        a config/mac/functions.tsv row; owner source, runtime,
                  glue, vendor, or unowned (a verified span with no identity yet)
  <category>      a reviewed non-function region in config/mac/code-regions.tsv
                  (for example the read-only literal/table pool)
  unresolved      everything else; never dropped from the denominator

The candidate census collects boundary leads without admitting them:
loader pointers into code (transition-vector entries), direct call targets,
reviewed vtable slots (config/mac/vtables),
and full-TU CodeWarrior hunks whose relocation-masked bytes occur exactly once
in the unresolved code. A branch target is a lead, not a function. Gaps
report which leads they contain so a reviewer can admit boundaries through
the ordinary evidence path.
"""
from __future__ import annotations

from bisect import bisect_right
from collections import Counter, defaultdict
from dataclasses import dataclass
import json
from pathlib import Path

from homm3.core.tsv import read as read_tsv, write as write_tsv
from homm3.mac import tables

CODE_REGIONS_TSV = "config/mac/code-regions.tsv"
REGION_CATEGORIES = ("readonly_data",)
#: Object-hunk byte matches shorter than this are too weak to report.
MINIMUM_OBJECT_MATCH = 32
MFLR_R0 = 0x7C0802A6
BLR = 0x4E800020


@dataclass(frozen=True)
class Region:
    start: int
    end: int
    category: str
    owner: str = ""
    name: str = ""


def read_code_regions(root: Path) -> list[tuple[int, int, str, str]]:
    path = root / CODE_REGIONS_TSV
    if not path.is_file():
        return []
    _banner, header, rows = read_tsv(path)
    if header != ["start", "end", "category", "evidence"]:
        raise tables.TableError(f"{CODE_REGIONS_TSV}: expected start/end/category/evidence columns")
    result = []
    for row in rows:
        start, end = int(row["start"], 16), int(row["end"], 16)
        if row["category"] not in REGION_CATEGORIES:
            raise tables.TableError(f"{CODE_REGIONS_TSV}: unknown category {row['category']!r}")
        if not row["evidence"].strip() or not start < end:
            raise tables.TableError(f"{CODE_REGIONS_TSV}: {start:#x}..{end:#x} needs an extent and evidence")
        result.append((start, end, row["category"], row["evidence"]))
    return result


def _source_owners(root: Path) -> dict[int, str]:
    from homm3.mac import addresses
    claims, _windows, _problems = addresses.scan(root)
    return {claim.offset: claim.identity for claim in claims}


def regions(root: Path, pef, source: dict[int, str] | None = None) -> list[Region]:
    """Every code-section byte, in address order, in exactly one region."""
    size = pef.section(tables.CODE_SECTION).packed_size
    source = _source_owners(root) if source is None else source
    runtime = tables.runtime_names(root)
    glue = {stub.offset: stub.name for stub in tables.read_glue(root)}
    zlib = {row.offset: row.name for row in tables.read_zlib(root)}
    covered = []
    for offset, length in tables.read_functions(root).items():
        if offset in source:
            owner, name = "source", source[offset]
        elif offset in runtime:
            owner, name = "runtime", runtime[offset][0]
        elif offset in glue:
            owner, name = "glue", glue[offset]
        elif offset in zlib:
            owner, name = "vendor", zlib[offset]
        else:
            owner, name = "unowned", ""
        covered.append(Region(offset, offset + length, "function", owner, name))
    for start, end, category, _evidence in read_code_regions(root):
        covered.append(Region(start, end, category))
    covered.sort(key=lambda region: region.start)
    result, cursor = [], 0
    for region in covered:
        if region.start < cursor:
            raise tables.TableError(f"code region {region.start:#x}..{region.end:#x} overlaps "
                                    f"the preceding region ending {cursor:#x}")
        if region.start > cursor:
            result.append(Region(cursor, region.start, "unresolved"))
        result.append(region)
        cursor = region.end
    if cursor > size:
        raise tables.TableError(f"code regions end at {cursor:#x}, past the section size {size:#x}")
    if cursor < size:
        result.append(Region(cursor, size, "unresolved"))
    return result


def validate_regions(root: Path, index) -> list[str]:
    """Reviewed non-function regions must stay free of code entries."""
    problems = []
    for start, end, category, _evidence in read_code_regions(root):
        entering = [at for at, target, kind in index.branches
                    if kind in ("linked_branch", "branch") and start <= target.offset < end
                    and target.section == tables.CODE_SECTION and not start <= at.offset < end]
        if entering:
            problems.append(f"{CODE_REGIONS_TSV}: {category} {start:#x}..{end:#x} is entered by "
                            f"{len(entering)} branches, first from {entering[0].offset:#x}")
    return problems


def _masked_words(hunk) -> list[int | None]:
    """Hunk words with relocated words (and a following reload slot) masked."""
    words = [int.from_bytes(hunk.data[at:at + 4], "big") for at in range(0, len(hunk.data) - 3, 4)]
    masked: list[int | None] = list(words)
    for offset, _kind, _name in hunk.xrefs:
        index = offset // 4
        if 0 <= index < len(masked):
            masked[index] = None
            if index + 1 < len(words) and words[index + 1] == 0x60000000:
                masked[index + 1] = None  # linker may restore r2 after a cross-TOC call
    return masked


def object_leads(root: Path, pef, unresolved: list[Region], code_regions: list[Region]) -> list[dict]:
    """Full-TU hunks whose masked bytes occur exactly once in all code, inside
    a region without an identity.

    `unresolved` is every region without an identity: unresolved bytes and
    unowned function rows; `code_regions` is every non-data region. A body
    whose bytes also occur elsewhere (common for template instantiations)
    is ambiguous and never a lead. `exact_row` marks a match covering a row.
    """
    from homm3.mac.object import parse_code_hunks
    code = pef.contents(tables.CODE_SECTION)
    words = [int.from_bytes(code[at:at + 4], "big") for at in range(0, len(code) - 3, 4)]
    starts = [region.start for region in unresolved]
    positions = defaultdict(list)
    for region in code_regions:
        for at in range(region.start, region.end - 3, 4):
            positions[words[at // 4]].append(at)
    code_starts = [region.start for region in code_regions]
    leads = []
    for listing in sorted((root / "build/mac/obj").glob("*.dis.txt")):
        unit = listing.name.removesuffix(".dis.txt")
        for hunk in parse_code_hunks(listing.read_text(errors="replace")):
            if len(hunk.data) < MINIMUM_OBJECT_MATCH:
                continue
            pattern = _masked_words(hunk)
            anchor = next((i for i, word in enumerate(pattern) if word is not None), None)
            if anchor is None:
                continue
            found = []
            for at in positions.get(pattern[anchor], ()):
                start = at - 4 * anchor
                if start < 0 or start // 4 + len(pattern) > len(words):
                    continue
                area = code_regions[bisect_right(code_starts, start) - 1]
                if start + len(hunk.data) > code_regions[-1].end or area.start > start:
                    continue
                base = start // 4
                if all(word is None or words[base + i] == word for i, word in enumerate(pattern)):
                    found.append(start)
                    if len(found) > 1:
                        break
            if len(found) != 1:
                continue
            region = unresolved[bisect_right(starts, found[0]) - 1] if found[0] >= starts[0] else None
            if region is not None and region.start <= found[0] and found[0] + len(hunk.data) <= region.end:
                leads.append(dict(offset=found[0], size=len(hunk.data), unit=unit, symbol=hunk.name,
                                  exact_row=region.category == "function"
                                  and (region.start, region.end) == (found[0], found[0] + len(hunk.data))))
    return leads


def _successors(word: int, at: int) -> tuple[list[int], bool]:
    """(in-function branch targets, falls through) for one PowerPC word.

    Calls (bl, bctrl, bclrl) return and fall through. An unconditional b is a
    jump or tail call; blr and bctr end the path. bctr's targets are unknown,
    so a span that reaches code only through a jump table fails the proof.
    """
    op = word >> 26
    link = bool(word & 1)
    if op == 18:
        target = word & 0x03FFFFFC
        if target & 0x02000000:
            target -= 0x04000000
        if link:
            return [], True
        return ([] if word & 2 else [at + target]), False
    if op == 16:
        target = word & 0xFFFC
        if target & 0x8000:
            target -= 0x10000
        always = (word >> 21) & 0x14 == 0x14
        targets = [] if word & 2 else [at + target]
        return (targets if not link else []), not always or link
    if op == 19 and ((word >> 1) & 0x3FF) in (16, 528):
        always = (word >> 21) & 0x14 == 0x14
        return [], link or not always
    return [], True


def proves_single_function(code: bytes, start: int, end: int, case_labels=()) -> bool:
    """Every word of [start, end) is reachable from `start` inside the span,
    and no path falls through past `end`.

    `case_labels` are loader-relocated code pointers inside the span (jump-
    table targets reached through bctr); they seed reachability but are never
    function entries themselves.
    """
    seen, stack = set(), [start, *(label for label in case_labels if start < label < end)]
    while stack:
        at = stack.pop()
        if at in seen:
            continue
        if not start <= at < end:
            return False
        seen.add(at)
        targets, falls = _successors(int.from_bytes(code[at:at + 4], "big"), at)
        for target in targets:
            if start <= target < end:
                stack.append(target)  # a branch leaving the span is a tail call
        if falls:
            stack.append(at + 4)
    return len(seen) == (end - start) // 4


def entry_leads(root: Path, index) -> dict[int, set[str]]:
    """Proven function entries and weaker code-pointer leads, by offset.

    tvector_entry: a loader-relocated code pointer followed by the TOC anchor
    (a transition vector). call_target: a `bl` destination from code outside
    the reviewed data regions. code_pointer: any other loader pointer into
    code, usually a jump-table case label - a lead, never an entry.
    """
    from homm3.mac.relocations import Address
    data = [(start, end) for start, end, _category, _evidence in read_code_regions(root)]
    in_data = lambda offset: any(start <= offset < end for start, end in data)  # noqa: E731
    leads: dict[int, set[str]] = defaultdict(set)
    pointers = index.loader.pointers
    for cell, target in pointers.items():
        if getattr(target, "section", None) != tables.CODE_SECTION or in_data(target.offset):
            continue
        following = pointers.get(Address(cell.section, cell.offset + 4))
        leads[target.offset].add("tvector_entry" if following == index.toc else "code_pointer")
    for at, target, kind in index.branches:
        if (kind == "linked_branch" and target.section == tables.CODE_SECTION
                and not in_data(at.offset) and not in_data(target.offset)):
            leads[target.offset].add("call_target")
    for slot in vtable_slots(root):
        leads[slot["offset"]].add("vtable_slot")
    return leads


ENTRY_KINDS = ("tvector_entry", "call_target", "vtable_slot")


def vtable_slots(root: Path) -> list[dict]:
    """Code spans named by reviewed vtables (config/mac/vtables/*.toml)."""
    import tomllib
    slots = []
    for path in sorted((root / "config/mac/vtables").glob("*.toml")):
        for table in tomllib.loads(path.read_text()).get("vtables", []):
            for number, slot in enumerate(table.get("slots", [])):
                if slot.get("code_section", tables.CODE_SECTION) != tables.CODE_SECTION:
                    continue
                slots.append(dict(offset=slot["code_offset"], size=slot["code_size"],
                                  vtable=table["symbol"], slot=number,
                                  source=path.relative_to(root).as_posix()))
    return slots


def vtable_problems(root: Path) -> list[str]:
    """Every reviewed vtable slot's code span must be one verified row."""
    spans = tables.read_functions(root)
    return [f"{slot['source']}: {slot['vtable']} slot {slot['slot']} code "
            f"{slot['offset']:#x}+{slot['size']:#x} is not a {tables.FUNCTIONS_TSV} row"
            for slot in vtable_slots(root) if spans.get(slot["offset"]) != slot["size"]]


def entry_conflicts(root: Path, leads: dict[int, set[str]]) -> list[str]:
    """A verified function row must not contain another function's entry."""
    entries = sorted(offset for offset, kinds in leads.items() if kinds & set(ENTRY_KINDS))
    problems = []
    for offset, size in sorted(tables.read_functions(root).items()):
        inner = entries[bisect_right(entries, offset):bisect_right(entries, offset + size - 1)]
        if inner:
            kinds = ",".join(sorted(leads[inner[0]] & set(ENTRY_KINDS)))
            problems.append(f"{tables.FUNCTIONS_TSV}: {offset:#x}+{size:#x} contains the "
                            f"{kinds} entry {inner[0]:#x}")
    return problems


def census(root: Path, pef, index, with_objects: bool = True) -> dict:
    """Byte regions, candidate leads and per-gap summaries."""
    source = _source_owners(root)
    layout = regions(root, pef, source)
    unresolved = [region for region in layout if region.category == "unresolved"]
    anonymous = [region for region in layout if region.category == "unresolved"
                 or region.category == "function" and region.owner == "unowned"]
    code = pef.contents(tables.CODE_SECTION)
    word = lambda at: int.from_bytes(code[at:at + 4], "big")  # noqa: E731
    size = pef.section(tables.CODE_SECTION).packed_size

    leads = entry_leads(root, index)
    executable = [region for region in layout if region.category in ("function", "unresolved")]
    objects = object_leads(root, pef, anonymous, executable) if with_objects else []
    for lead in objects:
        leads[lead["offset"]].add(f"object:{lead['unit']}:{lead['symbol']}")

    starts = [region.start for region in layout]
    candidates = []
    for offset, kinds in sorted(leads.items()):
        region = layout[bisect_right(starts, offset) - 1]
        if region.category == "function":
            place = "verified_start" if offset == region.start else "inside_verified"
        elif region.category == "unresolved":
            place = "unresolved"
        else:
            place = region.category
        candidates.append(dict(offset=offset, place=place, leads=sorted(kinds)))

    by_gap = defaultdict(list)
    gap_starts = [region.start for region in unresolved]
    for row in candidates:
        if row["place"] == "unresolved":
            by_gap[bisect_right(gap_starts, row["offset"]) - 1].append(row)
    gaps = []
    proven = []
    for number, region in enumerate(unresolved):
        inside = by_gap.get(number, [])
        entries = [row for row in inside if set(row["leads"]) & set(ENTRY_KINDS)]
        last = word(region.end - 4) if region.end - region.start >= 4 else None
        closed = (len(entries) == 1 and entries[0]["offset"] == region.start
                  and region.end < size and last is not None
                  and (last == BLR or last >> 26 == 18 and not last & 1))
        labels = [row["offset"] for row in inside if "code_pointer" in row["leads"]]
        provable = closed and proves_single_function(code, region.start, region.end, labels)
        # Split at proven entries; each piece must itself be one function.
        bounds = [row["offset"] for row in entries] + [region.end]
        for piece_start, piece_end in zip(bounds, bounds[1:]):
            final = word(piece_end - 4)
            if ((final == BLR or final >> 26 == 18 and not final & 1)
                    and proves_single_function(code, piece_start, piece_end, labels)):
                proven.append((piece_start, piece_end))
        gaps.append(dict(start=region.start, end=region.end, size=region.end - region.start,
                         entry_leads=len(entries), object_leads=sum(any(k.startswith("object:") for k in row["leads"]) for row in inside),
                         starts_with_prologue=word(region.start) == MFLR_R0,
                         single_entry_closed=closed, provable_function=provable))

    bytes_by = Counter()
    rows_by = Counter()
    for region in layout:
        key = f"function:{region.owner}" if region.category == "function" else region.category
        bytes_by[key] += region.end - region.start
        rows_by[key] += 1
    summary = {
        "code_section_bytes": size,
        "bytes": dict(sorted(bytes_by.items())),
        "regions": dict(sorted(rows_by.items())),
        "unresolved_gaps": len(unresolved),
        "single_entry_closed_gaps": sum(gap["single_entry_closed"] for gap in gaps),
        "provable_function_gaps": sum(gap["provable_function"] for gap in gaps),
        "proven_function_spans": len(proven),
        "proven_function_bytes": sum(end - start for start, end in proven),
        "candidates": dict(Counter(row["place"] for row in candidates)),
        "object_leads": len(objects),
        "object_leads_covering_unowned_rows": sum(lead["exact_row"] for lead in objects),
        "full_tu_listings": len(list((root / "build/mac/obj").glob("*.dis.txt"))),
        "unowned_function_rows": rows_by.get("function:unowned", 0),
    }
    return {"summary": summary, "regions": layout, "candidates": candidates,
            "gaps": gaps, "objects": objects, "leads": leads, "proven": proven}


def write(root: Path, report: dict) -> Path:
    folder = root / "build/gen/mac"
    write_tsv(folder / "code-regions.tsv", ["# GENERATED by `homm3 mac inventory`; do not edit."],
              ["start", "end", "size", "category", "owner", "name"],
              [[f"0x{r.start:x}", f"0x{r.end:x}", f"0x{r.end - r.start:x}", r.category, r.owner, r.name]
               for r in report["regions"]])
    write_tsv(folder / "candidates.tsv", ["# GENERATED by `homm3 mac inventory`; leads, not admitted boundaries."],
              ["offset", "place", "leads"],
              [[f"0x{row['offset']:x}", row["place"], ",".join(row["leads"])] for row in report["candidates"]])
    write_tsv(folder / "gaps.tsv", ["# GENERATED by `homm3 mac inventory`; unresolved code-section regions."],
              ["start", "end", "size", "entry_leads", "object_leads", "starts_with_prologue",
               "single_entry_closed", "provable_function"],
              [[f"0x{g['start']:x}", f"0x{g['end']:x}", f"0x{g['size']:x}", g["entry_leads"], g["object_leads"],
                int(g["starts_with_prologue"]), int(g["single_entry_closed"]), int(g["provable_function"])]
               for g in report["gaps"]])
    write_tsv(folder / "object-leads.tsv",
              ["# GENERATED by `homm3 mac inventory`; relocation-masked unique byte matches",
               "# of full-TU CodeWarrior hunks. Identity leads, not admitted claims."],
              ["offset", "size", "unit", "symbol", "exact_row"],
              [[f"0x{lead['offset']:x}", f"0x{lead['size']:x}", lead["unit"], lead["symbol"],
                int(lead["exact_row"])] for lead in report["objects"]])
    path = folder / "inventory.json"
    path.write_text(json.dumps(report["summary"], indent=2) + "\n")
    return path


def admit_proven(root: Path, report: dict) -> int:
    """Add every proven function span to functions.tsv as an unowned row.

    Proof: a transition-vector or direct-call entry at the start; a blr or
    unconditional branch as the last word; every word reachable from the
    entry (and in-span jump-table labels) without leaving the span; the end is
    the next proven entry or a verified region. Identity comes later, from
    source MAC_ADDRESS claims or the runtime maps.
    """
    from homm3.core.tsv import read as read_rows
    path = root / tables.FUNCTIONS_TSV
    banner, header, rows = read_rows(path)
    spans = tables.read_functions(root)
    added = 0
    for start, end in report["proven"]:
        if start not in spans:
            spans[start] = end - start
            added += 1
    note = "# Unowned rows were admitted by `homm3 mac inventory --admit-proven`."
    if note not in banner:
        banner = [*banner, note]
    write_tsv(path, banner, header, [[f"0x{o:x}", f"0x{s:x}"] for o, s in sorted(spans.items())])
    return added


def gate(root: Path, pef, index, report: dict) -> dict:
    """The step-5 accounting gate: zero defects, with unresolved counts shown."""
    problems = (tables.validate(root, pef) + validate_regions(root, index)
                + entry_conflicts(root, report["leads"]) + vtable_problems(root))
    summary = report["summary"]
    return {"problems": problems,
            "unresolved_bytes": summary["bytes"].get("unresolved", 0),
            "unowned_function_rows": summary["unowned_function_rows"],
            "complete": not problems and not summary["bytes"].get("unresolved")
            and not summary["unowned_function_rows"]}
