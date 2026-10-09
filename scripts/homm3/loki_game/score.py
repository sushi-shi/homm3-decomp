"""Loki game score: paired Windows functions whose Loki body the game units
reproduce at the Loki profile.

A paired Windows function (config/retail/heroes3-loki/functions.tsv) is looked
up in its Windows unit's Loki object by qualified name (MSVC and GNU v2 names
demangled to `Class::method`; overloads by arity, then by size). Its body is
compared with the Loki body in the normal form of homm3.loki_game.diff.
"""
from __future__ import annotations

from dataclasses import dataclass
from functools import cached_property

from homm3.core import undname
from homm3.loki_game import diff, pair, refs
from homm3.loki_game.image import RETAIL


@dataclass
class Row:
    win: int
    loki: int
    unit: str
    name: str
    windows: float
    symbol: str | None = None
    mangled: str | None = None
    score: float | None = None
    exact: bool = False
    note: str = ""


class Scorer:
    def __init__(self):
        self.pairs = pair.load()
        self.ledger = refs.ledger()
        self.image = diff.Image()
        self.sizes = {}
        with (RETAIL / "functions.tsv").open() as stream:
            for line in stream:
                if line.startswith("0x"):
                    parts = line.rstrip("\n").split("\t")
                    self.sizes[int(parts[0], 16)] = int(parts[1])
        self.loki_to_win = {k: w for w, (k, _) in self.pairs.items()}
        self.starts = sorted(self.sizes)
        self.learned: dict[int, str] = {}
        self._scanner = None

    def learn(self, functions: list[diff.ObjectFunction], demangled: dict[str, str]) -> None:
        """Name unpaired image bodies (template and inline instances above
        all) by the object functions found exactly once in the image."""
        from homm3.loki_game.scan import Scanner
        if self._scanner is None:
            self._scanner = Scanner(self.image.game)
        for f in functions:
            if len(f.body.code) < 16:
                continue
            mask = {o + k for o in f.body.refs for k in range(4)}
            name = diff.qualified(demangled.get(f.symbol, f.symbol))
            hits = [h for h in self._scanner.search(f.body.code, mask) if h in self.sizes]
            # identical template copies (vector<T*> for several T) share one name
            for hit in hits[:4]:
                if hit not in self.loki_to_win:
                    self.learned.setdefault(hit, name)

    def extent(self, loki: int) -> int:
        """The census counts a body's addresses, which leaves out alignment
        padding between its blocks; the extent runs to the next function,
        less the padding before it."""
        import bisect
        i = bisect.bisect_right(self.starts, loki)
        if i >= len(self.starts):
            return self.sizes[loki]
        end = self.starts[i]
        if end - loki > 4 * self.sizes[loki] + 64:
            return self.sizes[loki]
        code = self.image.game.read(loki, end - loki)
        return diff.strip_padding(code, self.sizes[loki])

    @cached_property
    def windows_demangled(self) -> dict[str, str]:
        return undname.demangle(row["name"] for row in self.ledger.values())

    def windows_qualified(self, win: int) -> str | None:
        name = self.ledger.get(win, {}).get("name")
        if name is None:
            return None
        dem = self.windows_demangled.get(name)
        return undname.qualified(dem) if dem else None

    def name_of(self, address: int) -> str:
        win = self.loki_to_win.get(address)
        if win is not None:
            return self.windows_qualified(win) or f"win:{win:#x}"
        plt = self.image.game.plt.get(address)
        if plt:
            return plt
        if address in self.learned:
            return self.learned[address]
        if address in self.sizes:
            return f"sub_{address:08x}"
        text = self.image.game.cstr(address, 80)
        if text and len(text) >= 2 and all(32 <= c < 127 for c in text):
            return repr(text.decode())
        return f"{address:#010x}"

    def rows(self, units: set[str] | None = None) -> list[Row]:
        out = []
        for win, (loki, _) in sorted(self.pairs.items()):
            entry = self.ledger.get(win)
            if entry is None or (units is not None and entry["unit"] not in units):
                continue
            out.append(Row(win, loki, entry["unit"], entry["name"], entry["cur"]))
        return out

    def body(self, row: Row):
        return self.image.body(row.loki, self.extent(row.loki), self.name_of)

    def score_unit(self, rows: list[Row], functions: list[diff.ObjectFunction]) -> None:
        demangled = diff.gnu_demangle([f.symbol for f in functions])
        self.learn(functions, demangled)
        by_name: dict[str, list[tuple[str, diff.ObjectFunction]]] = {}
        for f in functions:
            dem = demangled.get(f.symbol, f.symbol)
            by_name.setdefault(diff.qualified(dem), []).append((dem, f))
        for row in rows:
            qualified = self.windows_qualified(row.win)
            candidates = by_name.get(qualified or "", [])
            if not candidates:
                row.note = "not in object"
                continue
            if len(candidates) > 1:
                wdem = self.windows_demangled.get(row.name, "")
                want = diff.arity(wdem.replace("(void)", "()")) if wdem else None
                same = [c for c in candidates if diff.arity(c[0]) == want]
                candidates = same or candidates
                size = self.extent(row.loki)
                candidates.sort(key=lambda c: abs(len(c[1].body.code) - size))
            dem, f = candidates[0]
            target = self.body(row)
            row.symbol = dem
            row.mangled = f.symbol
            row.exact = f.body.code == target.code
            row.score = 100.0 if row.exact else diff.score(f.body, target)
