"""Emitted function order of each project object against the image.

g++ 2.95 writes one `.eh_frame` FDE per emitted function, in emission
order, and ld keeps each object's frame block whole: a linkonce copy that
another object's copy displaced keeps its FDE with a zero `pc_begin` (its
discarded section sits at address 0). The image's frame block of an object
therefore lists every function the original compile emitted, header inlines,
template instances and `__tf` type_info functions included, and the data
those emissions bring (assert and type-name strings, exception tables)
follows the same list. A compiled object whose FDE list equals the image's
emitted the same set in the same order.

Keys compared, per FDE:

- an exported function: its symbol (anonymous-namespace suffix dropped);
- a kept linkonce copy of another object, i.e. a discarded slot: its size;
- a file-static function: its size.

The object's `.rodata` keeps its strings in parse order, so two string
sequences also follow the original source (`--headers`, `--types`): the
`__FILE__` names of assert and TRuntimeError sites give the include order,
and the g++ type names (`__ti` nodes' names, in class definition and use
order) the order of the class definitions. The image's sequence is read
from the end of the previous object's compiled `.rodata` to the next
object's base, so strings before the voted base still count.
"""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import struct

from homm3.loki.datacmp import symbol_key
from homm3.loki.elf import Elf, STB_LOCAL, STT_FUNC, STT_SECTION, SHT_REL, SHT_SYMTAB
from homm3.loki.ehframe import frames
from homm3.loki.image import LokiImage


@dataclass(frozen=True)
class Entry:
    key: str
    name: str     # mangled name, or a placeholder for an unnamed slot
    size: int


def discarded(size: int) -> str:
    return f"linkonce:{size}"


def static(size: int) -> str:
    return f"static:{size}"


def image_entries(image: LokiImage) -> list[list[Entry]]:
    """Every compiled object's FDE list, in link order."""
    cies, fdes = frames(image.elf)
    ordinal = {offset: index for index, offset in enumerate(cies)}
    out: list[list[Entry]] = [[] for _ in cies]
    for fde in fdes:
        if not fde.start:
            entry = Entry(discarded(fde.size), "(discarded linkonce)", fde.size)
        else:
            symbol = image.function_symbol(fde.start)
            if symbol is None:
                name = image.static_roles.get(fde.start, "(static)")
                entry = Entry(static(fde.size), name, fde.size)
            else:
                entry = Entry(symbol_key(symbol.name), symbol.name, fde.size)
        out[ordinal[fde.cie]].append(entry)
    return out


def compiled_entries(path: Path, kept_here: set[str], exported: set[str]) -> list[Entry]:
    """A compiled object's FDE list, keyed as the image would show it:
    `kept_here` are the linkonce names the image keeps in this object,
    `exported` every name the image exports."""
    elf = Elf(path.read_bytes())
    symtab = next(s for s in elf.sections if s.type == SHT_SYMTAB)
    symbols = elf.symbols(symtab)
    eh = elf.section(".eh_frame")
    raw = elf.bytes(eh)
    rel_section = next((s for s in elf.sections if s.type == SHT_REL and s.info == eh.index), None)
    rels = {r.offset: symbols[r.symbol] for r in elf.rels(rel_section)} if rel_section else {}
    functions: dict[tuple[int, int], object] = {}
    for symbol in symbols:
        if symbol.type == STT_FUNC and symbol.shndx and symbol.name:
            functions.setdefault((symbol.shndx, symbol.value), symbol)
    out = []
    offset = 0
    while offset + 4 <= len(raw):
        (length,) = struct.unpack_from("<I", raw, offset)
        if length == 0:
            break
        (pointer,) = struct.unpack_from("<I", raw, offset + 4)
        if pointer:
            start, size = struct.unpack_from("<II", raw, offset + 8)
            target = rels.get(offset + 8)
            if target is None:
                function = None
            elif target.type == STT_FUNC:
                function = target
            else:   # the section, or g++'s local `.LFB<n>` label at the function start
                place = (0 if target.type == STT_SECTION else target.value) + start
                function = functions.get((target.shndx, place))
            section = elf.sections[function.shndx].name if function is not None else ""
            name = function.name if function is not None else "(unnamed)"
            key = symbol_key(name)
            if section.startswith(".gnu.linkonce.t."):
                key = key if key in kept_here else discarded(size)
            elif function is None or function.bind == STB_LOCAL or key not in exported:
                key = static(size)
            out.append(Entry(key, name, size))
        offset += 4 + length
    return out


def lcs_opcodes(a: list[str], b: list[str]) -> list[tuple[str, int, int, int, int]]:
    """A minimal alignment (longest common subsequence) as difflib-style
    opcodes; difflib's greedy longest block pairs repeated slot sizes badly."""
    n, m = len(a), len(b)
    table = [[0] * (m + 1) for _ in range(n + 1)]
    for i in range(n - 1, -1, -1):
        row, below = table[i], table[i + 1]
        for j in range(m - 1, -1, -1):
            row[j] = below[j + 1] + 1 if a[i] == b[j] else max(below[j], row[j + 1])
    pairs, i, j = [], 0, 0
    while i < n and j < m:
        if a[i] == b[j]:
            pairs.append((i, j))
            i += 1
            j += 1
        elif table[i + 1][j] >= table[i][j + 1]:
            i += 1
        else:
            j += 1
    out, i, j = [], 0, 0
    for pi, pj in pairs + [(n, m)]:
        if i < pi or j < pj:
            tag = "replace" if i < pi and j < pj else "delete" if i < pi else "insert"
            out.append((tag, i, pi, j, pj))
        if (pi, pj) != (n, m):
            if out and out[-1][0] == "equal" and out[-1][2] == pi and out[-1][4] == pj:
                out[-1] = ("equal", out[-1][1], pi + 1, out[-1][3], pj + 1)
            else:
                out.append(("equal", pi, pi + 1, pj, pj + 1))
        i, j = pi + 1, pj + 1
    return out


@dataclass
class Order:
    unit: str
    obj: int
    image: list[Entry]
    ours: list[Entry]

    @property
    def opcodes(self):
        return lcs_opcodes([e.key for e in self.image], [e.key for e in self.ours])

    @property
    def same(self) -> int:
        return sum(i2 - i1 for tag, i1, i2, _j1, _j2 in self.opcodes if tag == "equal")

    @property
    def identical(self) -> bool:
        return [e.key for e in self.image] == [e.key for e in self.ours]


def orders(objects: dict[str, tuple[int, Path]], image: LokiImage | None = None) -> list[Order]:
    image = image or LokiImage()
    lists = image_entries(image)
    exported = {symbol_key(s.name) for s in image.elf.dynsym if s.shndx and s.type == STT_FUNC}
    kept: dict[int, set[str]] = {}
    for function in image.functions:
        if function.linkonce and function.name:
            kept.setdefault(function.obj, set()).add(symbol_key(function.name))
    out = []
    for unit, (obj, path) in sorted(objects.items(), key=lambda item: item[1][0]):
        ours = compiled_entries(path, kept.get(obj, set()), exported) if path.is_file() else []
        out.append(Order(unit, obj, lists[obj] if obj < len(lists) else [], ours))
    return out


def render(order: Order, names: dict[str, str] | None = None,
           candidates: dict[int, list[str]] | None = None) -> list[str]:
    """The aligned lists: equal runs folded, image-only lines `-`, ours-only `+`.
    `candidates` names an image-only discarded slot by the kept linkonce
    functions of its size (when there are at most three)."""
    names = names or {}
    candidates = candidates or {}

    def show(e: Entry) -> str:
        label = names.get(e.name, e.name)
        if e.key.startswith("linkonce:") and e.name.startswith("(") and 0 < len(candidates.get(e.size, ())) <= 3:
            label += " ~ " + " | ".join(names.get(n, n) for n in candidates[e.size])
        return f"{e.size:6}  {label}"
    lines = []
    for tag, i1, i2, j1, j2 in order.opcodes:
        if tag == "equal":
            # Equal runs show our entries: they name the image's discarded slots.
            if j2 - j1 <= 2:
                lines += [f"  {show(e)}" for e in order.ours[j1:j2]]
            else:
                lines += [f"  {show(order.ours[j1])}", f"  ... {j2 - j1 - 2} equal",
                          f"  {show(order.ours[j2 - 1])}"]
            continue
        lines += [f"- {show(e)}" for e in order.image[i1:i2]]
        lines += [f"+ {show(e)}" for e in order.ours[j1:j2]]
    return lines


_STRING = re.compile(rb"[\x20-\x7e]{2,}\x00")
_SOURCE_FILE = re.compile(r"[\w+-]+\.(?:h|hpp|c|cpp)")
_IDENTIFIER = re.compile(r"[A-Za-z_$][\w$]*")
_TEMPLATE = re.compile(r"t\d+[A-Za-z_][\w$]*")


def rodata_strings(raw: bytes) -> list[str]:
    return [m.group()[:-1].decode() for m in _STRING.finditer(raw)]


def is_source_file(text: str) -> bool:
    """A bare `__FILE__` name (units compile from their own directory)."""
    return _SOURCE_FILE.fullmatch(text) is not None


def is_type_name(text: str) -> bool:
    """A g++ 2.95 type name: `<length><identifier>`, a `Q<n>` qualified
    name of such parts, or a `t<length><name>...` template instance."""
    def part(at: int) -> int | None:
        if text.startswith("t", at):
            return len(text) if _TEMPLATE.fullmatch(text, at) else None
        digits = re.match(r"\d+", text[at:])
        if digits is None:
            return None
        start = at + digits.end()
        end = start + int(digits.group())
        return end if end <= len(text) and _IDENTIFIER.fullmatch(text, start, end) else None
    at, count = 0, 1
    qualified = re.match(r"Q(?:(\d)|_(\d+)_)", text)
    if qualified:
        at, count = qualified.end(), int(qualified.group(1) or qualified.group(2))
    for _ in range(count):
        at = part(at) if at is not None and at < len(text) else None
    return at == len(text)


@dataclass
class Strings:
    unit: str
    image: list[str]
    ours: list[str]

    @property
    def opcodes(self):
        return lcs_opcodes(self.image, self.ours)

    @property
    def identical(self) -> bool:
        return self.image == self.ours


def _kept(strings: list[str], keep) -> list[str]:
    """The strings `keep` selects; a string that runs on from the byte
    before it (no NUL between, e.g. `@11TRandomHero` after a 0x40) counts
    without that byte."""
    out = []
    for text in strings:
        if keep(text):
            out.append(text)
        elif len(text) > 2 and keep(text[1:]):
            out.append(text[1:])
    return out


def string_orders(objects: dict[str, tuple[int, Path]], keep, image: LokiImage | None = None) -> list[Strings]:
    """Each object's `.rodata` strings that `keep` selects, image against ours."""
    from homm3.loki import datacmp
    layout = datacmp.build_layout(objects, image)
    out, previous_end = [], None
    for compiled, header in layout.sequence(".rodata"):
        base = layout.base.get((compiled.obj, header.index))
        following = datacmp._next_base(layout, compiled, header)
        start = previous_end if previous_end is not None and base is not None and previous_end <= base else base
        previous_end = None if base is None else base + header.size
        if start is None or following is None:
            continue
        theirs = _kept(rodata_strings(layout.index.read(start, following - start)), keep)
        ours = _kept(rodata_strings(compiled.elf.bytes(header)), keep)
        out.append(Strings(compiled.unit, theirs, ours))
    return out


def strings_main(units: list[str], kind: str) -> int:
    from homm3.loki import build
    objects = {u.name: (u.obj, build.OUT / "obj" / f"{u.name}.o") for u in build.units()}
    keep = is_source_file if kind == "headers" else is_type_name
    results = string_orders(objects, keep)
    identical = 0
    for result in results:
        identical += result.identical
        if units and result.unit not in units or result.identical and not units:
            continue
        same = sum(i2 - i1 for tag, i1, i2, _j1, _j2 in result.opcodes if tag == "equal")
        print(f"[loki] {result.unit:24} {same:4}/{len(result.image):<4} {kind} in order"
              f"  -{len(result.image) - same} image-only  +{len(result.ours) - same} ours-only")
        for tag, i1, i2, j1, j2 in result.opcodes:
            if tag == "equal":
                print(f"      = {i2 - i1}: {' '.join(result.image[i1:i2]) if kind == 'headers' else result.image[i1] + ' .. ' + result.image[i2 - 1]}")
                continue
            for text in result.image[i1:i2]:
                print(f"      - {text}")
            for text in result.ours[j1:j2]:
                print(f"      + {text}")
    print(f"[loki] {identical}/{len(results)} objects keep the image's {kind} sequence")
    return 0


def main(units: list[str], verbose: bool = False) -> int:
    from homm3.loki import build
    chosen = build.units(units or None)
    objects = {u.name: (u.obj, build.OUT / "obj" / f"{u.name}.o") for u in chosen}
    image = LokiImage()
    results = orders(objects, image)
    candidates: dict[int, list[str]] = {}
    for function in image.functions:
        if function.linkonce and function.name:
            candidates.setdefault(function.size, []).append(function.name)
    identical = 0
    for order in results:
        identical += order.identical
        if not order.ours:
            print(f"[loki] {order.unit:24} not built")
            continue
        if not order.identical or units:
            extra = len(order.ours) - order.same
            missing = len(order.image) - order.same
            print(f"[loki] {order.unit:24} {order.same:4}/{len(order.image):<4} in order"
                  f"  -{missing} image-only  +{extra} ours-only")
        if units or verbose:
            names = {}
            if not order.identical:
                try:
                    from homm3.loki.census import demangle
                    mangled = {e.name for e in order.image + order.ours if not e.name.startswith("(")}
                    mangled |= {n for e in order.image if e.name.startswith("(")
                                for n in candidates.get(e.size, ())[:3]}
                    mangled = sorted(mangled)
                    names = dict(zip(mangled, demangle(mangled)))
                except Exception:   # demangling is a convenience; the toolchain may be unstaged
                    names = {}
            for line in render(order, names, candidates) if not order.identical else []:
                print("    " + line)
    print(f"[loki] {identical}/{len(results)} objects emit the image's function list")
    return 0
