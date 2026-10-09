"""Per-function references on both sides: ordered direct calls, absolute data
references and the C strings they name. Windows uses retail bytes with its
base-relocation sites (config/retail/relocs.tsv); Loki uses the census."""
from __future__ import annotations

import bisect
import struct
from dataclasses import dataclass, field
from functools import lru_cache

from homm3.core import common
from homm3.core.image import Image

ROOT = common.HOMM3_DIR


@dataclass
class Body:
    size: int
    calls: list[int] = field(default_factory=list)
    data: list[int] = field(default_factory=list)
    strings: list[str] = field(default_factory=list)


def _printable(raw: bytes) -> bool:
    return len(raw) >= 3 and all(32 <= c < 127 or c in (9, 10, 13) or c >= 0xa0 for c in raw)


def _rows(path):
    with path.open() as stream:
        for line in stream:
            if line.startswith("#") or not line.strip() or not line.startswith("0x"):
                continue
            yield line.rstrip("\n").split("\t")


@lru_cache(maxsize=1)
def ledger() -> dict[int, dict]:
    """Windows game functions by RVA: unit, decorated name and CUR/MAX."""
    out = {}
    with (ROOT / "config/match_baseline.tsv").open() as stream:
        for line in stream:
            parts = line.rstrip("\n").split("\t")
            if line.startswith("#") or len(parts) < 6 or not parts[5].startswith("0x"):
                continue
            out[int(parts[5], 16)] = {"unit": parts[0], "name": parts[1],
                                      "cur": float(parts[2]), "max": float(parts[3])}
    return out


@lru_cache(maxsize=1)
def windows_runtime() -> dict[int, str]:
    """CRT functions of the retail runtime band by their C symbol (no leading _)."""
    out = {}
    for parts in _rows(ROOT / "config/retail/runtime-functions.tsv"):
        symbol = parts[6] if len(parts) > 6 else parts[2]
        if symbol.startswith("_") and "@" not in symbol and "?" not in symbol:
            out[int(parts[0], 16)] = symbol[1:]
    return out


@lru_cache(maxsize=1)
def windows() -> dict[int, Body]:
    from homm3.core import inputs
    image = Image(inputs.stage_executable(inputs.RETAIL))
    sizes = {int(p[0], 16): int(p[1]) for p in _rows(ROOT / "config/retail/functions.tsv")}
    sites = sorted(int(p[0], 16) for p in _rows(ROOT / "config/retail/relocs.tsv"))

    def read(rva, n):
        section = image.section_of(rva)
        if section is None:
            return None
        offset = section.raw_offset + rva - section.rva
        return image.data[offset:offset + n]

    import capstone
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    out = {}
    for rva, size in sizes.items():
        code = read(rva, size) or b""
        body = Body(size)
        lo, hi = bisect.bisect_left(sites, rva), bisect.bisect_left(sites, rva + size)
        for site in sites[lo:hi]:
            target = struct.unpack_from("<I", code, site - rva)[0] - image.image_base
            body.data.append(target)
            raw = read(target, 256)
            end = raw.find(b"\0") if raw else -1
            if end > 0 and _printable(raw[:end]):
                body.strings.append(raw[:end].decode("latin-1"))
        for insn in decoder.disasm(code, rva):
            if insn.bytes[0] == 0xE8 and insn.size == 5:
                body.calls.append(struct.unpack("<i", insn.bytes[1:5])[0] + insn.address + 5)
        out[rva] = body
    return out


@lru_cache(maxsize=1)
def loki() -> dict[int, Body]:
    from homm3.loki_game import census
    from homm3.loki_game.image import LokiGame
    game = LokiGame()
    out = {a: Body(f.size) for a, f in census.functions().items()}
    for fn, _, target in census.calls():
        if fn in out:
            out[fn].calls.append(target)
    for fn, _, target in census.refs():
        if fn in out:
            out[fn].data.append(target)
            raw = game.cstr(target, 256)
            if raw and _printable(raw):
                out[fn].strings.append(raw.decode("latin-1"))
    return out
