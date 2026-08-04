# VENDORED VERBATIM - DO NOT EDIT.
# Source: vostok-delinker (local ~/Projects/surv/vostok-delinker)
#   branch feature/find-relocs-script, commit 912a0ac, scripts/find_relocs.py
# Measured on a comparable non-stripped MSVC game exe: 0.9988 precision /
# 0.9942 recall overall; the code and switch channels were exact.
# Everything below this block is byte-identical to the upstream file.
#!/usr/bin/env python3
"""Recover absolute-relocation sites from a PE image with no `.reloc` directory.

A stripped image keeps every linked address but loses the list of fields that
hold one, so the delinker has nothing to turn back into a COFF relocation. This
script rebuilds that list from the image alone and writes it in the
`--reloc-manifest` format.

    find_relocs.py IMAGE.exe -o sites.tsv
    vostok-delinker ... --reloc-manifest sites.tsv

Two channels, both site-anchored -- they identify a *field* from the way the
image encodes it, without needing to know what the field points at:

`code`
    Disassemble every executable section and keep each 4-byte window inside an
    instruction whose value is an in-image address and which the decoder echoed
    as an operand. This is what a `.reloc` entry in code always is: a disp32, an
    imm32, or a moffs. Switch dispatch (`jmp dword ptr [4*reg + TABLE]`) also
    hands over its jump table, whose every entry is a site the surrounding
    instruction stream does not reveal.

`data`
    Keep aligned in-image dwords in the initialized data sections, minus a
    literal-pool mask. The mask matters: a 32-bit image based at 0x400000 has an
    address range that overlaps printable ASCII, so the tail of a C string
    ("TRO\\0") reads as a perfectly plausible pointer (0x004f5254).

This complements `--rediscover-relocations-from-pdb`, which is *target*-anchored:
it scans for dwords that land on an address the PDB already names, so it can only
find a site whose target is already known. A reconstruction that derives its data
symbols from recovered relocations cannot bootstrap with that alone. The channels
here need no symbol input, so they break the cycle -- and their output can be fed
straight back as the reviewed manifest.

Trust the rules by measuring them. `--validate` scores every channel against the
image's own `.reloc` directory, so point the script at a *non-stripped* build of
the same program -- an earlier release, a sibling SKU, a debug build -- and read
the precision and recall before believing anything it says about the stripped
one.

Requires `llvm-objdump` on PATH. 32-bit (`DIR32`/`HIGHLOW`) images only.
"""
from __future__ import annotations

import argparse
import re
import struct
import subprocess
import sys
from collections import Counter, namedtuple

IMAGE_SCN_MEM_EXECUTE = 0x20000000
IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080
IMAGE_REL_BASED_HIGHLOW = 3
PE32_MAGIC = 0x10B

# `size` is the readable extent (what the file actually holds); `mapped` is the
# virtual one. They differ whenever a section has a zero-filled tail, and a
# pointer may well target that tail, so containment tests use `mapped`.
Section = namedtuple("Section", "name rva size mapped raw_offset executable")
Site = namedtuple("Site", "rva target channel detail")


def die(message):
    print(f"find_relocs: {message}", file=sys.stderr)
    raise SystemExit(1)


class Image:
    """The parts of a PE32 file this script reasons about."""

    def __init__(self, path):
        self.path = path
        self.data = open(path, "rb").read()
        data = self.data
        if data[:2] != b"MZ":
            die(f"{path}: not a PE image")
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe:pe + 4] != b"PE\0\0":
            die(f"{path}: no PE signature")
        machine = struct.unpack_from("<H", data, pe + 4)[0]
        section_count = struct.unpack_from("<H", data, pe + 6)[0]
        optional_size = struct.unpack_from("<H", data, pe + 20)[0]
        optional = pe + 24
        if struct.unpack_from("<H", data, optional)[0] != PE32_MAGIC:
            die(f"{path}: not PE32 (machine 0x{machine:x}); "
                "only 32-bit HIGHLOW images are supported")
        self.image_base = struct.unpack_from("<I", data, optional + 28)[0]
        self.image_size = struct.unpack_from("<I", data, optional + 56)[0]
        self.image_end = self.image_base + self.image_size

        self.sections = []
        for index in range(section_count):
            entry = optional + optional_size + index * 40
            name = data[entry:entry + 8].rstrip(b"\0").decode("latin-1")
            virtual_size, rva, raw_size, raw_offset = struct.unpack_from(
                "<4I", data, entry + 8)
            flags = struct.unpack_from("<I", data, entry + 36)[0]
            if flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA or raw_size == 0:
                continue
            self.sections.append(Section(
                name, rva, min(virtual_size, raw_size) or raw_size,
                max(virtual_size, raw_size), raw_offset,
                bool(flags & IMAGE_SCN_MEM_EXECUTE)))

        self.reloc_dir = struct.unpack_from("<2I", data, optional + 96 + 5 * 8)

    def section_of(self, rva):
        for section in self.sections:
            if section.rva <= rva < section.rva + section.mapped:
                return section
        return None

    def blob(self, section):
        return self.data[section.raw_offset:section.raw_offset + section.size]

    def in_image(self, value):
        return self.image_base <= value < self.image_end

    def rva_of(self, value):
        return value - self.image_base

    def base_relocations(self):
        """HIGHLOW site RVAs from the image's own `.reloc`, or None."""
        rva, size = self.reloc_dir
        if not rva or not size:
            return None
        section = self.section_of(rva)
        if section is None:
            return None
        start = section.raw_offset + rva - section.rva
        sites = set()
        cursor, end = start, start + size
        while cursor + 8 <= end:
            page, block = struct.unpack_from("<2I", self.data, cursor)
            if block < 8:
                break
            for index in range((block - 8) // 2):
                entry = struct.unpack_from(
                    "<H", self.data, cursor + 8 + index * 2)[0]
                if entry >> 12 == IMAGE_REL_BASED_HIGHLOW:
                    sites.add(page + (entry & 0xFFF))
            cursor += block
        return sites


# llvm-objdump row: "  <va>: <bb bb ...>\t<mnemonic>\t<operands>". The byte
# column loses its trailing space once an instruction is long enough to fill
# the padding, so the pattern must not require one.
ROW = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )*[0-9a-f]{2})\s*\t(.*)$")
# Intel-syntax scaled index, as llvm-objdump prints it: "[4*ecx + 0x402054]".
SWITCH_INDEX = re.compile(r"\[\s*4\s*\*")


def disassemble(image, section):
    result = subprocess.run(
        ["llvm-objdump", "-d", "--x86-asm-syntax=intel",
         f"--section={section.name}", image.path],
        capture_output=True, text=True)
    if result.returncode != 0:
        die(f"llvm-objdump failed on {image.path}:\n{result.stderr.strip()}")
    return result.stdout


def code_sites(image, section, walk_tables=True):
    """Absolute operands in one executable section, plus its switch tables."""
    sites = {}
    tables = set()
    low, high = section.rva, section.rva + section.size
    for line in disassemble(image, section).splitlines():
        row = ROW.match(line)
        if not row:
            continue
        rva = int(row.group(1), 16) - image.image_base
        if not low <= rva < high:
            continue
        raw = bytes.fromhex(row.group(2).replace(" ", ""))
        text = row.group(3)
        mnemonic = text.split(None, 1)[0] if text else ""
        for offset in range(len(raw) - 3):
            value = struct.unpack_from("<I", raw, offset)[0]
            # The decoder having printed the value is what separates an operand
            # from four bytes that merely look like an address.
            if not image.in_image(value) or f"0x{value:x}" not in text:
                continue
            sites[rva + offset] = Site(rva + offset, value, "code", mnemonic)
            if (walk_tables and mnemonic == "jmp" and SWITCH_INDEX.search(text)
                    and low <= image.rva_of(value) < high):
                tables.add(image.rva_of(value))

    blob = image.blob(section)
    for base in sorted(tables):
        cursor = base
        while cursor + 4 <= high:
            value = struct.unpack_from("<I", blob, cursor - low)[0]
            # The table runs until an entry stops being a code address, or
            # until the next table starts.
            if not low <= image.rva_of(value) < high:
                break
            if cursor != base and cursor in tables:
                break
            sites.setdefault(cursor, Site(
                cursor, value, "switch", f"table@0x{base:x}"))
            cursor += 4
    return sites


def printable(byte):
    return 0x20 <= byte < 0x7F or byte in (9, 10, 13)


def literal_mask(blob, minimum=4):
    """Flag every byte that belongs to a plausible string literal.

    A pointer never lives inside one, and on a low-based 32-bit image the tail
    of a narrow literal is indistinguishable from an address on its own.
    """
    mask = bytearray(len(blob))
    run = 0
    for index, byte in enumerate(blob):
        if printable(byte):
            run += 1
            continue
        if byte == 0 and run >= minimum:
            mask[index - run:index + 1] = b"\1" * (run + 1)   # include the NUL
        run = 0

    index = 0                                                  # UTF-16LE cells
    while index + 1 < len(blob):
        if printable(blob[index]) and blob[index + 1] == 0:
            end = index
            while (end + 1 < len(blob) and printable(blob[end])
                   and blob[end + 1] == 0):
                end += 2
            if ((end - index) // 2 >= minimum and end + 1 < len(blob)
                    and blob[end] == 0 and blob[end + 1] == 0):
                mask[index:end + 2] = b"\1" * (end + 2 - index)
            index = end + 2
        else:
            index += 1
    return mask


def data_sites(image, section, use_mask=True, aligned=True):
    """Aligned in-image dwords in one data section, outside the literal pool."""
    blob = image.blob(section)
    mask = literal_mask(blob) if use_mask else bytearray(len(blob))
    sites = {}
    for offset in range(0, len(blob) - 3, 4 if aligned else 1):
        value = struct.unpack_from("<I", blob, offset)[0]
        if not image.in_image(value) or any(mask[offset:offset + 4]):
            continue
        rva = section.rva + offset
        sites[rva] = Site(rva, value, "data", section.name)
    return sites


def target_class(image, site, masks, function_starts, neighbours=frozenset()):
    """How plausible the target is. Drives --drop and the validation report.

    Data dwords carry a second dimension: whether the dword has an adjacent
    candidate dword. Pointer arrays -- vtables, dispatch tables, string tables
    -- are contiguous, so an *isolated* dword that happens to point at code is
    almost always a coincidence rather than a callback (measured 0.07 against a
    real `.reloc`, versus 0.89 for one inside a run). A lone pointer to a string
    or a data object is ordinary, so the split is only reported where it earns
    its keep.
    """
    rva = image.rva_of(site.target)
    section = image.section_of(rva)
    if section is None:
        return "unmapped"
    if section.executable:
        if function_starts is not None:
            return "code-start" if rva in function_starts else "code-interior"
        if site.channel == "data" and not (
                site.rva - 4 in neighbours or site.rva + 4 in neighbours):
            return "code-isolated"
        return "code"
    mask = masks.get(section.name)
    offset = rva - section.rva
    if mask is not None and offset < len(mask) and mask[offset]:
        return ("literal-start" if offset == 0 or not mask[offset - 1]
                else "literal-interior")
    return "data"


def collect(image, args):
    sites, masks = {}, {}
    for section in image.sections:
        if args.sections and section.name not in args.sections:
            continue
        if section.executable:
            if "code" in args.channels:
                sites.update(code_sites(
                    image, section, walk_tables="switch" in args.channels))
        elif "data" in args.channels:
            masks[section.name] = literal_mask(image.blob(section))
            sites.update(data_sites(image, section, not args.no_literal_mask))
    neighbours = frozenset(rva for rva, site in sites.items()
                           if site.channel == "data")
    return sites, masks, neighbours


def load_function_starts(path):
    if not path:
        return None
    starts = set()
    for line in open(path):
        line = line.split("#", 1)[0].strip()
        if line:
            starts.add(int(line, 0))
    return starts


def report(image, sites, masks, function_starts, neighbours, truth, stream):
    """Per-channel and per-target-class breakdown; scored when truth exists."""
    rows = Counter()
    correct = Counter()
    for site in sites.values():
        key = (site.channel,
               target_class(image, site, masks, function_starts, neighbours))
        rows[key] += 1
        if truth is not None and site.rva in truth:
            correct[key] += 1

    width = max(len(f"{c}/{t}") for c, t in rows) if rows else 10
    print(f"{'channel/target':<{width + 2}}{'sites':>8}"
          + (f"{'correct':>9}{'precision':>11}" if truth is not None else ""),
          file=stream)
    for key in sorted(rows, key=lambda k: -rows[k]):
        label = f"{key[0]}/{key[1]}"
        line = f"{label:<{width + 2}}{rows[key]:>8}"
        if truth is not None:
            line += f"{correct[key]:>9}{correct[key] / rows[key]:>11.4f}"
        print(line, file=stream)

    total = len(sites)
    print(f"{'total':<{width + 2}}{total:>8}", file=stream)
    if truth is None:
        return
    found = set(sites) & truth
    missed = truth - set(sites)
    print(f"\nground truth: {len(truth)} HIGHLOW sites in `.reloc`", file=stream)
    print(f"  precision {len(found)}/{total} = {len(found) / max(1, total):.4f}",
          file=stream)
    print(f"  recall    {len(found)}/{len(truth)} = "
          f"{len(found) / max(1, len(truth)):.4f}", file=stream)
    by_section = Counter()
    for site in missed:
        section = image.section_of(site)
        by_section[section.name if section else "(none)"] += 1
    if missed:
        print(f"  missed {len(missed)} "
              f"({sum(1 for s in missed if s % 4)} unaligned) "
              f"by section: {dict(by_section)}", file=stream)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("image", help="the PE to recover sites from")
    parser.add_argument("-o", "--output",
                        help="write the reloc manifest here (default: stdout)")
    parser.add_argument("--report", action="store_true",
                        help="print the channel/target breakdown to stderr")
    parser.add_argument("--validate", action="store_true",
                        help="score every channel against the image's own "
                             "`.reloc` directory and write no manifest")
    parser.add_argument("--sections", type=lambda s: set(s.split(",")),
                        help="restrict to these section names")
    parser.add_argument("--channels", default="code,switch,data",
                        type=lambda s: set(s.split(",")),
                        help="any of code,switch,data (default: all)")
    parser.add_argument("--function-starts", metavar="FILE",
                        help="one RVA per line; splits code targets into "
                             "code-start/code-interior instead of using the "
                             "isolation test. Only worth it for a complete "
                             "inventory -- a partial one rejects real vtable "
                             "slots, since a callback need never be called "
                             "directly")
    parser.add_argument("--drop", default="unmapped,data/code-isolated",
                        type=lambda s: set(x for x in s.split(",") if x),
                        help="target classes to reject, as `class` or "
                             "`channel/class` "
                             "(default: unmapped,data/code-isolated)")
    parser.add_argument("--no-literal-mask", action="store_true",
                        help="keep data dwords that fall inside string cells")
    args = parser.parse_args(argv)

    image = Image(args.image)
    truth = image.base_relocations()
    if args.validate and truth is None:
        die(f"{args.image} has no `.reloc` directory, so there is nothing to "
            "validate against; point --validate at a non-stripped build")
    if not args.validate and truth is not None:
        print(f"find_relocs: note: {args.image} still has its `.reloc` "
              "directory; the delinker reads it directly and needs no manifest",
              file=sys.stderr)

    function_starts = load_function_starts(args.function_starts)
    sites, masks, neighbours = collect(image, args)

    # A data dword pointing into the middle of a function, or outside every
    # mapped section, is overwhelmingly a coincidence rather than a pointer.
    # The same target class means something else per channel, though: a switch
    # table entry targets a mid-function label by construction, so the filter
    # accepts `channel/class` as well as a bare `class`.
    def rejected(site):
        klass = target_class(image, site, masks, function_starts,
                             neighbours)
        return klass in args.drop or f"{site.channel}/{klass}" in args.drop

    kept = {rva: site for rva, site in sites.items() if not rejected(site)}
    dropped = len(sites) - len(kept)

    if args.validate:
        print(f"{len(sites)} candidate sites, {dropped} rejected by "
              f"--drop {','.join(sorted(args.drop))}\n", file=sys.stdout)
        report(image, kept, masks, function_starts, neighbours, truth,
               sys.stdout)
        return 0

    if args.report or dropped:
        print(f"find_relocs: {len(kept)} sites "
              f"({dropped} dropped by --drop)", file=sys.stderr)
    if args.report:
        report(image, kept, masks, function_starts, neighbours, truth,
               sys.stderr)

    stream = open(args.output, "w") if args.output else sys.stdout
    try:
        print(f"# Absolute-relocation sites recovered from "
              f"{args.image.split('/')[-1]} by scripts/find_relocs.py.", file=stream)
        print(f"# channels={','.join(sorted(args.channels))} "
              f"dropped={','.join(sorted(args.drop)) or '(none)'}", file=stream)
        print("site_rva\tkind", file=stream)
        for rva in sorted(kept):
            print(f"0x{rva:x}\tdir32", file=stream)
    finally:
        if args.output:
            stream.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
