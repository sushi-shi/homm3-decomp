"""Find GCC-compiled function bodies in the Loki game image, and prove its profile.

A compiled function is searched in the image's .text with only the fields the
link decides left open: relocated fields and the rel32 of calls leaving the
function. Everything else must be byte-identical, so a single hit is an exact
fingerprint of that body (used to pair it and to rank compiler flags)."""
from __future__ import annotations

import re
import struct
import subprocess
import tarfile
import io
import tomllib
from pathlib import Path

from homm3.core import common
from homm3.loki.elf import Elf
from homm3.loki_game.image import BUILD, LokiGame

CONFIG = common.HOMM3_DIR / "config/loki/game.toml"
SHT_REL, STT_FUNC = 9, 2


def compiled_functions(path: Path) -> list[tuple[str, bytes, set[int]]]:
    import capstone
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    obj = Elf(path.read_bytes())
    symtab = next(s for s in obj.sections if s.type == 2)
    symbols = obj.symbols(symtab)
    out = []
    for section in obj.sections:
        if not (section.name.startswith(".text") or section.name.startswith(".gnu.linkonce.t")):
            continue
        data = obj.bytes(section)
        relocated = set()
        for rel in obj.sections:
            if rel.type == SHT_REL and rel.info == section.index:
                for r in obj.rels(rel):
                    relocated.update(range(r.offset, r.offset + 4))
        for symbol in symbols:
            if symbol.shndx != section.index or symbol.type != STT_FUNC or not symbol.size:
                continue
            start, end = symbol.value, symbol.value + symbol.size
            body = data[start:end]
            mask = {i - start for i in relocated if start <= i < end}
            for insn in decoder.disasm(body, start):
                if insn.bytes[0] == 0xE8 and insn.size == 5:
                    target = struct.unpack("<i", insn.bytes[1:5])[0] + insn.address + 5
                    if not start <= target < end:
                        mask.update(range(insn.address - start + 1, insn.address - start + 5))
            out.append((symbol.name, body, mask))
    return out


class Scanner:
    def __init__(self, game: LokiGame | None = None):
        self.game = game or LokiGame()
        text = self.game.elf.section(".text")
        self.base, self.blob = text.addr, self.game.elf.bytes(text)

    def search(self, body: bytes, mask: set[int]) -> list[int]:
        pattern = b"".join(b"." if i in mask else re.escape(body[i:i + 1]) for i in range(len(body)))
        return [self.base + m.start() for m in re.finditer(pattern, self.blob, re.DOTALL)]

    def scan(self, objects: list[Path]) -> list[tuple[str, int, list[int]]]:
        return [(name, len(body), self.search(body, mask))
                for path in objects for name, body, mask in compiled_functions(path)]


def profile_spec() -> dict:
    with CONFIG.open("rb") as stream:
        return tomllib.load(stream)


def stage_proof_sources(ref: str) -> Path:
    """The engine units' sources at `ref` (the Loki editor branch, whose units are
    byte-exact for h3maped at -O0), unpacked under build/heroes3-loki/profile/."""
    root = BUILD / "profile" / re.sub(r"[^A-Za-z0-9_.-]", "_", ref)
    if not (root / "src").is_dir():
        archive = subprocess.run(["git", "-C", str(common.HOMM3_DIR), "archive", ref, "src", "include"],
                                 check=True, capture_output=True).stdout
        root.mkdir(parents=True, exist_ok=True)
        with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
            tar.extractall(root, filter="data")
    return root


def driver_command(*arguments: str, stl: str = "libstdc++") -> list[str]:
    """g++ 2.95.2 with libstdc++ 2.95's own headers (`stl = "libstdc++"`, the
    game's: its std::string is the reference-counted bastring) or with SGI STL
    3.2 first (`"sgi"`, the h3maped editor's)."""
    from homm3.loki import toolchain
    command = toolchain.driver_command(*arguments)
    if stl == "libstdc++":
        at = command.index(str(toolchain.SGI_STL))
        del command[at - 1:at + 1]
    return command


def compile_units(root: Path, units: list[str], flags: list[str], out: Path,
                  extra_includes: list[Path], stl: str = "libstdc++") -> list[Path]:
    from homm3.loki import toolchain
    out.mkdir(parents=True, exist_ok=True)
    objects = []
    for unit in units:
        obj = out / (Path(unit).stem + ".o")
        command = driver_command(
            *flags, "-fpermissive", "-w", "-DHOMM3_TARGET_LOKI=1",
            "-include", str(root / "include/gcc_prefix.h"),
            f"-I{root / 'include'}", f"-I{root / 'include/editor'}",
            *(f"-I{p}" for p in extra_includes), "-c", Path(unit).name, "-o", str(obj), stl=stl)
        done = subprocess.run(command, cwd=root / Path(unit).parent, env=toolchain.environment(),
                              capture_output=True, text=True)
        if done.returncode == 0:
            objects.append(obj)
        else:
            print(f"[loki-game] {unit}: compile failed under {' '.join(flags)}")
    return objects


def profile(variants: dict[str, list[str]] | None = None) -> dict[str, tuple[int, int]]:
    """Unique exact hits of the proof units under each flag variant."""
    from homm3.loki import toolchain
    spec = profile_spec()["proof"]
    variants = variants or spec["variants"]
    root = stage_proof_sources(spec["ref"])
    includes = [common.HOMM3_DIR / "vendor/zlib-1.1.3", toolchain.GTK / "usr/include",
                toolchain.GTK / "usr/lib/glib/include"]
    scanner = Scanner()
    results = {}
    for tag, flags in variants.items():
        stl = "sgi" if tag.startswith("sgi") else "libstdc++"
        objects = compile_units(root, spec["units"], flags, BUILD / "profile/objects" / tag,
                                includes, stl)
        rows = scanner.scan(objects)
        results[tag] = (len(rows), sum(1 for _, _, hits in rows if len(hits) == 1))
        print(f"[loki-game] {tag:12} {' '.join(flags):55} {results[tag][1]:4} unique exact of {results[tag][0]}")
    return results
