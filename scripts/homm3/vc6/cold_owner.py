"""homm3.vc6.cold_owner - attribute hot C2 functions to compiler source files.

C2.DLL 12.00.8447 is block-reordered (c2-atlas.md, inliner.md): hot code
lives in the string-less front region, while each compiler TU keeps its cold
blocks and internal-compiler-error sites in link order in the upper region.
An ICE site has the fixed shape

    mov edx, LINE ; mov ecx, offset "E:\\8447\\vc98\\p2\\src\\...\\file.c" ; call/jmp

so a hot function's jumps into the upper region land between ICE sites of
its own TU. For each selected function this lists the cold targets and the
TU of the nearest ICE site before and after each one ("a|b" when the two
differ). It is a locality inference, like the atlas brackets, not a proof.

    python3 -m homm3.vc6.cold_owner 0x13615 0x5739

Function extents come from build/vc6/c2-tu-map.tsv (`homm3 vc6 atlas --regen`).
"""
from __future__ import annotations

import bisect
import re
import struct
import sys
from collections import Counter

from homm3.vc6 import _common, _toolchain, atlas

ICE_RE = re.compile(rb"\xba(....)\xb9(....)[\xe8\xe9]", re.S)
UPPER_REGION = 0x6a9fc   # first anchored TU region (c2-atlas.md section 2)


def ice_sites(text: bytes, text_rva: int, path_of) -> list[tuple[int, str, int]]:
    """[(site rva, tu basename, line)], sorted; path_of(va) -> str | None."""
    sites = []
    for m in ICE_RE.finditer(text):
        line, va = struct.unpack("<II", m.group(1) + m.group(2))
        path = path_of(va)
        if path and path.lower().endswith(".c") and "\\src\\" in path.lower():
            sites.append((text_rva + m.start(), path.split("\\")[-1], line))
    return sorted(sites)


def tu_between(sites: list[tuple[int, str, int]], rva: int) -> str:
    """TU label of a cold address from the ICE sites around it."""
    keys = [s[0] for s in sites]
    i = bisect.bisect_right(keys, rva)
    before = sites[i - 1][1] if i else "?"
    after = sites[i][1] if i < len(sites) else "?"
    return before if before == after else f"{before}|{after}"


def cold_targets(code: bytes, entry: int, upper: int = UPPER_REGION) -> list[int]:
    """Direct jump targets at or above `upper` from one function's bytes."""
    import capstone
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.skipdata = True
    out = []
    for ins in md.disasm(code, entry):
        if ins.mnemonic.startswith("j") and ins.op_str.startswith("0x"):
            target = int(ins.op_str, 16)
            if target >= upper:
                out.append(target)
    return out


def attribute(code: bytes, entry: int, sites) -> Counter:
    return Counter(tu_between(sites, t) for t in cold_targets(code, entry))


def _extents() -> dict[int, int]:
    if not atlas.TU_MAP.is_file():
        _common.die(f"{atlas.TU_MAP} missing - run `homm3 vc6 atlas --regen`")
    sizes = {}
    for line in atlas.TU_MAP.read_text().splitlines():
        if line.startswith("#") or line.startswith("func_rva"):
            continue
        rva, size = line.split("\t")[:2]
        sizes[int(rva, 16)] = int(size)
    return sizes


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    if not args:
        print(__doc__)
        return 2
    binary = _toolchain.Binary("C2.DLL")
    text = next(s for s in binary.sections if s.name == ".text")
    data = binary.data[text.raw:text.raw + text.rsize]
    def path_of(va: int) -> str | None:
        try:
            return binary.cstr_at_va(va)
        except ValueError:
            return None

    sites = ice_sites(data, text.vaddr, path_of)
    sizes = _extents()
    for arg in args:
        entry = int(arg, 16)
        if entry >= binary.image_base:
            entry -= binary.image_base
        if entry not in sizes:
            _common.die(f"{arg}: not a function entry in {atlas.TU_MAP.name}")
        start = entry - text.vaddr
        counts = attribute(data[start:start + sizes[entry]], entry, sites)
        label = ", ".join(f"{tu} x{n}" for tu, n in counts.most_common()) or "no cold blocks"
        print(f"{entry:#07x}  {label}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
