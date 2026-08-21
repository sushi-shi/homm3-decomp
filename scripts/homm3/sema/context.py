"""homm3.sema.context - one lazy load per process for every sema tool.

Each backing store loads at most once per invocation and only when a
tool actually asks for it: the gated retail image, the symbol db
(build/gen/symbol_names.csv - the complete 11,943-function universe,
every function named at some provenance grade), the admitted dir32
reloc sites with their operands, the universe classification, the
vtable inventory, the objdiff report, and the whole-.text call index.
There is deliberately NO on-disk cache: staleness traps cost more than
the sub-second loads they would save.
"""
from __future__ import annotations

import bisect
import csv
import json
import struct

from homm3.core import common
from homm3.sema._common import die

SYMCSV = common.HOMM3_DIR / "build/gen/symbol_names.csv"
RELOCS = common.HOMM3_DIR / "config/retail-relocs.tsv"
VTABLES = common.HOMM3_DIR / "config/retail-vtables.tsv"
REPORT = common.HOMM3_DIR / "build/objdiff/report.json"


class SymbolDb:
    """Name/unit/size lookups over symbol_names.csv, ownership size-bounded."""

    def __init__(self, extent_of=None):
        if not SYMCSV.is_file():
            die(f"{SYMCSV.relative_to(common.HOMM3_DIR)} missing - run "
                "`homm3 delink` (or `homm3 labels --all && homm3 model`)")
        self._extent_of = extent_of  # () -> image virtual size, for resolve()
        self.funcs = {}   # rva -> (name, unit, size, provenance)
        self.datas = {}   # rva -> (name, unit, size, provenance)
        self.byname = {}
        with SYMCSV.open() as fh:
            rows = (line for line in fh if not line.startswith("#"))
            for r in csv.DictReader(rows):
                try:
                    rva = int(r["rva"], 16)
                    size = int(r.get("size") or "0", 16)
                except (ValueError, TypeError):
                    continue
                row = (r["name"], r.get("unit", ""), size,
                       r.get("provenance", ""))
                (self.funcs if r.get("kind") == "func" else self.datas)[rva] = row
                self.byname.setdefault(r["name"], rva)
        self.starts = sorted(self.funcs)
        self.data_starts = sorted(self.datas)

    def label(self, rva: int) -> str:
        row = self.funcs.get(rva)
        if row:
            return f"0x{rva:08x} {row[0]} [{row[1]}]"
        row = self.datas.get(rva)
        if row:
            return f"0x{rva:08x} {row[0]} (data)"
        return f"0x{rva:08x} ?"

    def owner(self, rva: int) -> int | None:
        """The function CONTAINING rva, or None. Sizes are complete
        (config/retail-functions.tsv), so a miss means padding/data."""
        k = bisect.bisect_right(self.starts, rva) - 1
        if k < 0:
            return None
        start = self.starts[k]
        size = self.funcs[start][2]
        if size and rva >= start + size:
            return None
        return start

    def nearest_data(self, rva: int, within: int = 0x400):
        """(start, name, +offset) of the nearest preceding data symbol."""
        k = bisect.bisect_right(self.data_starts, rva) - 1
        if k < 0:
            return None
        start = self.data_starts[k]
        if rva - start >= within:
            return None
        return start, self.datas[start][0], rva - start

    def resolve(self, arg: str) -> int:
        """RVA for '0x<rva>' / '0x<va>' (VA reduces) or an exact name.
        Numbers require the 0x prefix - bare hex is ambiguous with names."""
        if arg.lower().startswith("0x"):
            try:
                n = int(arg, 16)
            except ValueError:
                die(f"'{arg}' is not a hex number")
            rva = n - common.IMAGE_BASE if n >= common.IMAGE_BASE else n
            if self._extent_of is not None and not (0 <= rva < self._extent_of()):
                die(f"'{arg}' (rva 0x{rva:x}) is outside the image")
            return rva
        if arg in self.byname:
            return self.byname[arg]
        die(f"'{arg}' is not an 0x-address and not a name in "
            "build/gen/symbol_names.csv")

    def resolve_fn(self, arg: str):
        """(name, unit, rva, size, ordinal) for a FUNCTION target; an
        address inside a body snaps to its owner with a note. The ordinal
        distinguishes duplicate public names within one unit's object."""
        rva = self.resolve(arg)
        if rva not in self.funcs:
            owner = self.owner(rva)
            if owner is None:
                die(f"0x{rva:x} is not a function start and not inside any "
                    "function body")
            print(f"[0x{rva:x} is +0x{rva - owner:x} into "
                  f"{self.funcs[owner][0]} - using the owner]")
            rva = owner
        name, unit, size, _prov = self.funcs[rva]
        same = sorted(r for r, row in self.funcs.items()
                      if row[0] == name and row[1] == unit)
        return name, unit, rva, size, same.index(rva)


class Context:
    """The lazy per-process load shared by every sema tool."""

    def __init__(self):
        self._image = None
        self._symbols = None
        self._classes = None
        self._relocs = None
        self._vtables = None
        self._report = ()
        self._call_index = None

    @property
    def image(self):
        if self._image is None:
            self._image, self._info = common.load_image()
        return self._image

    @property
    def text(self):
        """(section, bytes) of retail .text."""
        section = next(s for s in self.image.sections if s.name == ".text")
        return section, self.image.blob(section)

    @property
    def symbols(self) -> SymbolDb:
        if self._symbols is None:
            self._symbols = SymbolDb(
                extent_of=lambda: self.image.image_size)
        return self._symbols

    @property
    def classes(self):
        """rva -> universe category (target/zlib/runtime/eh-funclet/...)."""
        if self._classes is None:
            from homm3.match import universe
            # Pass the already-gated image: classify() would otherwise
            # re-load and re-hash the exe a second time per process.
            self._classes, _sizes = universe.classify(self.image)
        return self._classes

    @property
    def relocs(self):
        """[(site_rva, operand_va)] for every admitted dir32 site, sorted
        by site - the exact reference edges of the fixed-base image."""
        if self._relocs is None:
            image = self.image
            out = []
            for line in RELOCS.open():
                if line.startswith("#") or line.startswith("site_rva"):
                    continue
                site = int(line.split("\t", 1)[0], 16)
                section = image.section_of(site)
                if section is None:
                    continue
                off = section.raw_offset + (site - section.rva)
                out.append((site, struct.unpack_from("<I", image.data, off)[0]))
            self._relocs = out
        return self._relocs

    @property
    def vtables(self):
        """[(rva, slot_count)] from config/retail-vtables.tsv, sorted."""
        if self._vtables is None:
            rows = []
            for line in VTABLES.open():
                if line.startswith("#") or line.startswith("rva"):
                    continue
                cols = line.split("\t")
                rows.append((int(cols[0], 16), int(cols[1])))
            self._vtables = sorted(rows)
        return self._vtables

    def vtable_of(self, rva: int):
        """(vtable_rva, slot_index) when `rva` is a slot address."""
        starts = [v for v, _ in self.vtables]
        k = bisect.bisect_right(starts, rva) - 1
        if k < 0:
            return None
        vt, count = self.vtables[k]
        if rva < vt + 4 * count and (rva - vt) % 4 == 0:
            return vt, (rva - vt) // 4
        return None

    @property
    def report(self):
        """Parsed build/objdiff/report.json, or None when absent."""
        if self._report == ():
            try:
                self._report = json.loads(REPORT.read_text())
            except Exception:
                self._report = None
        return self._report

    def fn_fuzzy(self, unit: str, name: str):
        """fuzzy_match_percent, or None when the report/unit/function is
        missing entirely.

        A listed function whose fuzzy_match_percent KEY is absent scored
        exactly 0.0: report.json is protobuf-JSON, which omits
        default-valued fields. The earlier reading here ("absent = not
        diffed") was measured wrong on cmbtmgr LootDeadHero 2026-08-21 -
        objdiff pairs and diffs the function, and its insert/delete
        penalty (both sides' unmatched rows against a one-sided
        max_score, then clamped) produces the exact 0.0 the omission
        encodes. Absent therefore reads as 0.0, never as "skip"."""
        rep = self.report
        if not rep:
            return None
        for u in rep.get("units", []):
            if u.get("name") == unit:
                for fn in u.get("functions") or []:
                    if fn.get("name") == name:
                        return fn.get("fuzzy_match_percent", 0.0)
                break
        return None

    @property
    def call_index(self):
        """callee_rva -> [(site_rva, opcode)] from ONE linear E8/E9 scan
        of .text. Data-in-text (jump tables inside function spans) can
        fake a site; size-bounded attribution downstream bounds the harm."""
        if self._call_index is None:
            section, blob = self.text
            index = {}
            n = len(blob) - 4
            i = 0
            while i < n:
                op = blob[i]
                if op == 0xE8 or op == 0xE9:
                    rel = struct.unpack_from("<i", blob, i + 1)[0]
                    tgt = section.rva + i + 5 + rel
                    if section.rva <= tgt < section.rva + section.size:
                        index.setdefault(tgt, []).append((section.rva + i, op))
                i += 1
            self._call_index = index
        return self._call_index


_CONTEXT = None


def get_context() -> Context:
    global _CONTEXT
    if _CONTEXT is None:
        _CONTEXT = Context()
    return _CONTEXT
