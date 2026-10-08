"""Stub X libraries for h3maped's link, generated from the image's own .dynsym.

The image was linked against Red Hat 6.0 X libraries (a 1999 XFree86-libs erratum) of which
no copy is found: 170 of its 194 imported X functions carry the st_size of Red Hat 6.0's
XFree86-libs-3.3.3.1-49, whose .dynsym order does not give the image's import order, and the
libraries that give the order (Red Hat 6.2's libX11/libXi, Red Hat 7.0's libXext) do not give
the sizes. What ld 2.9.1 takes from a shared library it links against is its soname (DT_NEEDED)
and its dynamic symbols: their .dynsym order, each one's type and size, and the version of
each versioned reference. So the link inputs are reconstructed from the image the way HoMM1
reconstructs import libraries from stub DLLs:

    build/h3maped-loki/link/xstub/libXi.so.6, libXext.so.6, libX11.so.6 (and lib*.so links)

Each is an ELF32 i386 shared object with the soname of the image's DT_NEEDED entry. Its .dynsym
lists the rows config/retail/h3maped-loki/x-imports.tsv gives to that library, in the table's
(the image's) order: a definition with the image's type and size when the library defines the
symbol, else an undefined reference (versioned, `name@VERSION`, for a glibc symbol). Then come
its definitions of symbols an earlier library names, and the absolute symbols ld defines in
every library (STRUCTURE). The image records the X imports unversioned (versym 0), so the stubs
define no versions; a stub with versioned references carries the .gnu.version and
.gnu.version_r that name them, and DT_NEEDED for their library. Function bodies are `ret`
padded with int3 to the recorded size; nothing runs them (the launch test uses the staged
libraries of link/xlib).
"""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import struct

from homm3.loki import delink

FACTS = delink.RETAIL / "x-imports.tsv"
LIBRARIES = {"libXi": "libXi.so.6", "libXext": "libXext.so.6", "libX11": "libX11.so.6"}
TYPES = {"OBJECT": 1, "FUNC": 2}
# The absolute symbols ld defines in every shared library it builds, as each X library of the
# era does. Where the table places one in a library's run it stands there; the others follow
# the library's own entries. The image proves the end symbols are named by a library: glibc's
# libraries do not define them, yet the image's .dynsym records _etext, _edata, __bss_start and
# _end (497 to 500) where ld 2.9.1 assigns them in its script, ahead of the -export-dynamic
# symbols, which it does only for a symbol a shared library defines or references.
STRUCTURE = ("_DYNAMIC", "_GLOBAL_OFFSET_TABLE_", "_etext", "_edata", "__bss_start", "_end")

ET_DYN, EM_386 = 3, 3
PT_LOAD, PT_DYNAMIC = 1, 2
SHT_PROGBITS, SHT_STRTAB, SHT_HASH, SHT_DYNAMIC, SHT_DYNSYM = 1, 3, 5, 6, 11
SHT_GNU_VERNEED, SHT_GNU_VERSYM = 0x6ffffffe, 0x6fffffff
SHF_WRITE, SHF_ALLOC, SHF_EXECINSTR = 1, 2, 4
SHN_ABS = 0xfff1
DT_NULL, DT_NEEDED, DT_HASH, DT_STRTAB, DT_SYMTAB, DT_STRSZ, DT_SYMENT, DT_SONAME = 0, 1, 4, 5, 6, 10, 11, 14
DT_VERSYM, DT_VERNEED, DT_VERNEEDNUM = 0x6ffffff0, 0x6ffffffe, 0x6fffffff


@dataclass(frozen=True)
class Row:
    index: int        # the image's .dynsym index
    symbol: str       # name, or name@VERSION for a versioned reference
    type: str
    size: int
    library: str      # the stub library that names it
    defined: str      # the library that defines it

    @property
    def name(self) -> str:
        return self.symbol.split("@")[0]

    @property
    def version(self) -> str | None:
        return self.symbol.split("@")[1] if "@" in self.symbol else None


def rows(path: Path = FACTS) -> list[Row]:
    out = []
    for line in path.read_text().splitlines():
        if line and not line.startswith("#"):
            index, symbol, kind, size, library, defined = line.split("\t")
            out.append(Row(int(index), symbol, kind, int(size), library, defined))
    return out


@dataclass(frozen=True)
class Entry:
    name: str
    type: str
    size: int
    defined: bool
    version: str | None = None     # a versioned reference: (library, version)
    needed: str | None = None


def entries(library: str, table: list[Row]) -> list[Entry]:
    """The library's .dynsym after the null entry."""
    own = [Entry(r.name, r.type, r.size, r.defined == library, r.version, None if r.defined in LIBRARIES
                 else r.defined) for r in table if r.library == library]
    own += [Entry(r.name, r.type, r.size, True) for r in table if r.defined == library and r.library != library]
    named = {e.name for e in own}
    return own + [Entry(name, "OBJECT", 0, True) for name in STRUCTURE if name not in named]


def elf_hash(name: bytes) -> int:
    h = 0
    for c in name:
        h = ((h << 4) + c) & 0xffffffff
        g = h & 0xf0000000
        if g:
            h ^= g >> 24
        h &= ~g & 0xffffffff
    return h


class _Strings:
    def __init__(self):
        self.data = bytearray(b"\0")
        self.index: dict[str, int] = {}

    def add(self, text: str) -> int:
        if text not in self.index:
            self.index[text] = len(self.data)
            self.data += text.encode() + b"\0"
        return self.index[text]


def _align(value: int, alignment: int) -> int:
    return (value + alignment - 1) & -alignment


def shared_object(soname: str, symbols: list[Entry]) -> bytes:
    """A minimal ELF32 i386 shared object: .hash, .dynsym, .dynstr, the version sections when it
    has versioned references, .text, .got and .dynamic."""
    dynstr = _Strings()
    soname_offset = dynstr.add(soname)
    names = [dynstr.add(s.name) for s in symbols]
    # Versioned references: one Verneed per library, one Vernaux per version, numbered from 2.
    needs: dict[str, list[str]] = {}
    for s in symbols:
        if s.version:
            versions = needs.setdefault(s.needed, [])
            if s.version not in versions:
                versions.append(s.version)
    number, verneed = {}, bytearray()
    for i, (file, versions) in enumerate(needs.items()):
        next_need = 0 if i == len(needs) - 1 else 16 + 16 * len(versions)
        verneed += struct.pack("<HHIII", 1, len(versions), dynstr.add(file), 16, next_need)
        for j, version in enumerate(versions):
            number[file, version] = 2 + len(number)
            verneed += struct.pack("<IHHII", elf_hash(version.encode()), 0, number[file, version],
                                   dynstr.add(version), 0 if j == len(versions) - 1 else 16)
    needed = [dynstr.add(file) for file in needs]
    versym = struct.pack(f"<{len(symbols) + 1}H", 0, *(number[s.needed, s.version] if s.version else 1
                                                      for s in symbols)) if needs else b""

    count = len(symbols) + 1
    nbucket = max(1, count // 2) | 1
    buckets, chains = [0] * nbucket, [0] * count
    for i, s in enumerate(symbols, 1):
        b = elf_hash(s.name.encode()) % nbucket
        chains[i], buckets[b] = buckets[b], i
    hash_bytes = struct.pack(f"<II{nbucket}I{count}I", nbucket, count, *buckets, *chains)
    text, text_offsets = bytearray(), {}
    for s in symbols:
        if s.defined and s.type == "FUNC":
            text_offsets[s.name] = len(text)
            text += b"\xc3" + b"\xcc" * (max(s.size, 1) - 1)
            text += b"\xcc" * (-len(text) % 4)
        elif s.defined and s.name not in STRUCTURE:
            raise ValueError(f"{soname}: no stub definition for data symbol {s.name}")

    tags = [(DT_NEEDED, n) for n in needed] + [(DT_SONAME, soname_offset), (DT_HASH, 0), (DT_STRTAB, 0),
                                               (DT_SYMTAB, 0), (DT_STRSZ, len(dynstr.data)), (DT_SYMENT, 16)]
    if needs:
        tags += [(DT_VERSYM, 0), (DT_VERNEED, 0), (DT_VERNEEDNUM, len(needs))]
    tags.append((DT_NULL, 0))

    phnum, ehsize, phentsize = 2, 52, 32
    sections = [(".hash", SHT_HASH, SHF_ALLOC, ".dynsym", 0, 4, 4, len(hash_bytes)),
                (".dynsym", SHT_DYNSYM, SHF_ALLOC, ".dynstr", 1, 4, 16, 16 * count),
                (".dynstr", SHT_STRTAB, SHF_ALLOC, None, 0, 1, 0, len(dynstr.data))]
    if needs:
        sections += [(".gnu.version", SHT_GNU_VERSYM, SHF_ALLOC, ".dynsym", 0, 2, 2, len(versym)),
                     (".gnu.version_r", SHT_GNU_VERNEED, SHF_ALLOC, ".dynstr", len(needs), 4, 0, len(verneed))]
    sections += [(".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, None, 0, 16, 0, len(text)),
                 (".got", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE, None, 0, 4, 4, 12),
                 (".dynamic", SHT_DYNAMIC, SHF_ALLOC | SHF_WRITE, ".dynstr", 0, 4, 8, 8 * len(tags))]
    offset, layout = ehsize + phnum * phentsize, {}
    for name, _, _, _, _, alignment, _, size in sections:
        offset = _align(offset, alignment)
        layout[name] = offset
        offset += size
    end = offset
    addresses = {DT_HASH: ".hash", DT_STRTAB: ".dynstr", DT_SYMTAB: ".dynsym", DT_VERSYM: ".gnu.version",
                 DT_VERNEED: ".gnu.version_r"}
    dynamic = b"".join(struct.pack("<II", tag, layout[addresses[tag]] if tag in addresses else value)
                       for tag, value in tags)
    got = struct.pack("<3I", layout[".dynamic"], 0, 0)
    text_end = layout[".text"] + len(text)
    structure = {"_DYNAMIC": layout[".dynamic"], "_GLOBAL_OFFSET_TABLE_": layout[".got"], "_etext": text_end,
                 "_edata": end, "__bss_start": end, "_end": end}
    text_index = 1 + [s[0] for s in sections].index(".text")
    dynsym = bytearray(16)
    for s, name in zip(symbols, names):
        info = (1 << 4) | TYPES[s.type]
        if not s.defined:
            dynsym += struct.pack("<IIIBBH", name, 0, s.size, info, 0, 0)
        elif s.name in structure:
            dynsym += struct.pack("<IIIBBH", name, structure[s.name], 0, info, 0, SHN_ABS)
        else:
            dynsym += struct.pack("<IIIBBH", name, layout[".text"] + text_offsets[s.name], s.size, info, 0,
                                  text_index)

    contents = {".hash": hash_bytes, ".dynsym": bytes(dynsym), ".dynstr": bytes(dynstr.data),
                ".gnu.version": versym, ".gnu.version_r": bytes(verneed), ".text": bytes(text), ".got": got,
                ".dynamic": dynamic}
    image = bytearray(end)
    for name in layout:
        image[layout[name]:layout[name] + len(contents[name])] = contents[name]
    shstr = _Strings()
    index = {s[0]: i + 1 for i, s in enumerate(sections)}
    headers = [bytes(40)]
    for name, kind, flags, link, info, alignment, entsize, size in sections:
        headers.append(struct.pack("<10I", shstr.add(name), kind, flags, layout[name], layout[name], size,
                                   index[link] if link else 0, info, alignment, entsize))
    shstrtab_name = shstr.add(".shstrtab")
    headers.append(struct.pack("<10I", shstrtab_name, SHT_STRTAB, 0, 0, len(image), len(shstr.data), 0, 0, 1, 0))
    image += shstr.data
    image += bytes(-len(image) % 4)
    shoff = len(image)
    for header in headers:
        image += header
    ident = b"\x7fELF\x01\x01\x01" + bytes(9)
    image[:ehsize] = ident + struct.pack("<HHIIIIIHHHHHH", ET_DYN, EM_386, 1, layout[".text"], ehsize, shoff,
                                         0, ehsize, phentsize, phnum, 40, len(headers), len(headers) - 1)
    image[ehsize:ehsize + phnum * phentsize] = (
        struct.pack("<8I", PT_LOAD, 0, 0, 0, end, end, 7, 0x1000) +
        struct.pack("<8I", PT_DYNAMIC, layout[".dynamic"], layout[".dynamic"], layout[".dynamic"],
                    len(dynamic), len(dynamic), 6, 4))
    return bytes(image)


def write(directory: Path, path: Path = FACTS) -> list[Path]:
    """Write the three stub libraries (and their link-time lib*.so names) into directory."""
    table = rows(path)
    directory.mkdir(parents=True, exist_ok=True)
    written = []
    for library, soname in LIBRARIES.items():
        target = directory / soname
        target.write_bytes(shared_object(soname, entries(library, table)))
        alias = directory / f"{library}.so"
        alias.unlink(missing_ok=True)
        alias.symlink_to(soname)
        written.append(target)
    return written
