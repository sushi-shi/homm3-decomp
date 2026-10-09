"""Pair Loki game functions with Windows HEROES3.EXE functions without names.

Evidence, strongest first (the first evidence that pairs a function wins):
  fingerprint        an engine unit compiled at the proven profile hits one
                     image body exactly; its class::method names the Windows row
  string             both bodies are the only users of a shared C string
  vtable-signature   the same set of classes holds the function in its vtable
  vtable-slot:CLASS  equal-length slot runs between paired slots of CLASS
  call-graph         equal-length runs between paired callees of a paired caller
  call-graph-intersection
                     the only unpaired callee common to every paired caller
  call-graph-alignment
                     aligned in two or more paired callers' call sequences
Windows ICF folds and Loki duplicate bodies stay unpaired where ambiguous."""
from __future__ import annotations

import math
import re
from collections import Counter, defaultdict
from pathlib import Path

from homm3.core import common
from homm3.loki_game import refs, rtti
from homm3.loki_game.image import LokiGame, RETAIL, BUILD

ROOT = common.HOMM3_DIR


def _windows_vtables() -> dict[str, list[list[int]]]:
    from homm3.core import inputs
    from homm3.core.image import Image
    import struct
    image = Image(inputs.stage_executable(inputs.RETAIL))
    out: dict[str, list[list[int]]] = {}
    with (ROOT / "config/retail/vtables.tsv").open() as stream:
        for line in stream:
            if not line.startswith("0x"):
                continue
            rva, count, cls = line.rstrip("\n").split("\t")[:3]
            rva = int(rva, 16)
            section = image.section_of(rva)
            offset = section.raw_offset + rva - section.rva
            slots = [struct.unpack_from("<I", image.data, offset + 4 * i)[0] - image.image_base
                     for i in range(int(count))]
            out.setdefault(cls, []).append(slots)
    return out


class Pairing:
    def __init__(self):
        self.game = LokiGame()
        self.W = refs.windows()
        self.K = refs.loki()
        self.ledger = refs.ledger()
        self.lib = refs.windows_runtime()
        self.plt = self.game.plt
        self.pairs: dict[int, tuple[int, str]] = {}
        # Call-graph evidence pairs project code only: Loki's linkonce and
        # runtime bands hold libstdc++ templates (bastring, SGI containers)
        # with no Dinkumware counterpart, and Windows rows without a ledger
        # entry are its runtime.
        self.k_project = {a for a in self.K if self.game.band(a) == "text"}
        self.w_project = {a for a in self.W if a in self.ledger}
        self.k_twins = self._twins()

    def _twins(self) -> set[int]:
        """Loki bodies identical to another but for call targets and absolute
        addresses (file-static copies in several objects, each with its own
        constants): call graphs cannot tell them apart."""
        import hashlib
        import re as _re
        import capstone
        decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        address_like = _re.compile(r"0x8[0-9a-f]{6}")
        seen: dict[bytes, list[int]] = {}
        for address in self.k_project:
            size = self.K[address].size
            if size < 16:
                continue
            digest = hashlib.sha256()
            for insn in decoder.disasm(self.game.read(address, size), address):
                operands = address_like.sub("A", insn.op_str) if insn.mnemonic not in ("call", "jmp") \
                    or not insn.op_str.startswith("0x") else "T"
                digest.update(f"{insn.mnemonic} {operands};".encode())
            seen.setdefault(digest.digest(), []).append(address)
        return {a for group in seen.values() if len(group) > 1 for a in group}

    # -- bookkeeping --------------------------------------------------------
    def k2w(self) -> dict[int, int]:
        return {k: w for w, (k, _) in self.pairs.items()}

    def add(self, found: dict[int, tuple[int, str]]) -> int:
        taken = self.k2w()
        new = 0
        for w, (k, evidence) in found.items():
            if w in self.pairs or k in taken:
                continue
            if evidence.startswith("call-graph") and (k not in self.k_project or w not in self.w_project
                                                      or k in self.k_twins):
                continue
            self.pairs[w] = (k, evidence)
            taken[k] = w
            new += 1
        return new

    # -- anchors ------------------------------------------------------------
    def strings(self) -> int:
        def index(side):
            ix = defaultdict(set)
            for a, body in side.items():
                for s in set(body.strings):
                    ix[s].add(a)
            return ix
        wi, ki = index(self.W), index(self.K)
        found = defaultdict(dict)
        for s, ws in wi.items():
            if len(s) >= 4 and len(ws) == 1 and len(ki.get(s, ())) == 1:
                found[next(iter(ws))].setdefault(next(iter(ki[s])), s)
        chosen = {w: next(iter(ks)) for w, ks in found.items() if len(ks) == 1}
        rev = Counter(chosen.values())
        return self.add({w: (k, "string") for w, k in chosen.items() if rev[k] == 1})

    def _dtor(self, w: int) -> int:
        """A Windows vtable holds the scalar deleting destructor (??_G); GCC 2.95's
        single destructor slot pairs with the class's ??1 where the ledger has one."""
        name = self.ledger.get(w, {}).get("name", "")
        if name.startswith("??_G"):
            target = "??1" + name[4:].split("@@")[0] + "@@"
            for rva, row in self.ledger.items():
                if row["name"].startswith(target):
                    return rva
        return w

    def vtables(self) -> int:
        wv = _windows_vtables()
        lv = defaultdict(set)
        for v in rtti.vtables(self.game):
            if v.cls:
                lv[v.cls].add(v.slots)
        common_classes = sorted(c for c in wv if c in lv and len(wv[c]) == 1 and len(lv[c]) == 1)
        Wt = {c: wv[c][0] for c in common_classes}
        Kt = {c: list(next(iter(lv[c]))) for c in common_classes}
        pure = {a for a, n in self.plt.items() if n == "__pure_virtual"}

        def signatures(table):
            s = defaultdict(set)
            for c, slots in table.items():
                for f in slots:
                    s[f].add(c)
            return {f: frozenset(cs) for f, cs in s.items()}
        byw, byk = defaultdict(list), defaultdict(list)
        for f, s in signatures(Wt).items():
            byw[s].append(f)
        for f, s in signatures(Kt).items():
            if f not in pure:
                byk[s].append(f)
        found = {}
        for s, fs in byw.items():
            if len(fs) == 1 and len(byk.get(s, ())) == 1:
                found[fs[0]] = (byk[s][0], "vtable-signature")
        anchors = {**{w: k for w, (k, _) in self.pairs.items()}, **{w: k for w, (k, _) in found.items()}}
        changed = True
        while changed:
            changed = False
            taken = set(anchors.values())
            for c in common_classes:
                ws, ks = Wt[c], Kt[c]
                pos = [(i, ks.index(anchors[f])) for i, f in enumerate(ws)
                       if f in anchors and anchors[f] in ks]
                pos = [(-1, -1)] + pos + [(len(ws), len(ks))]
                if not all(a2 > a1 and b2 > b1 for (a1, b1), (a2, b2) in zip(pos, pos[1:])):
                    continue
                for (a1, b1), (a2, b2) in zip(pos, pos[1:]):
                    if a2 - a1 != b2 - b1:
                        continue
                    for d in range(1, a2 - a1):
                        w, k = ws[a1 + d], ks[b1 + d]
                        if k in pure or w in anchors or k in taken:
                            continue
                        anchors[w] = k
                        taken.add(k)
                        found[w] = (k, f"vtable-slot:{c}")
                        changed = True
        dup = Counter(k for k, _ in found.values())
        return self.add({self._dtor(w): (k, e) for w, (k, e) in found.items() if dup[k] == 1})

    def fingerprints(self, objects: list[Path]) -> int:
        from homm3.loki_game.scan import Scanner, compiled_functions

        def gnu_key(n):
            for pattern, kind in ((r"^_\._(\d+)(\w+)$", "~"), (r"^__(\d+)(\w+)", "ctor")):
                m = re.match(pattern, n)
                if m:
                    return (m.group(2)[:int(m.group(1))].lower(), kind)
            m = re.match(r"^(\w+?)__(C?)(\d+)(\w+)", n)
            if m and not n.startswith("__"):
                return (m.group(4)[:int(m.group(3))].lower(), m.group(1).lower())
            m = re.match(r"^(\w+?)__F", n)
            return ("", m.group(1).lower()) if m else None

        def ms_key(n):
            m = re.match(r"^\?\?([01])(\w+)@@", n)
            if m:
                return (m.group(2).lower(), "ctor" if m.group(1) == "0" else "~")
            m = re.match(r"^\?(\w+)@(\w+)@@", n)
            if m:
                return (m.group(2).lower(), m.group(1).lower())
            m = re.match(r"^\?(\w+)@@Y", n)
            return ("", m.group(1).lower()) if m else None
        wk = defaultdict(list)
        for rva, row in self.ledger.items():
            key = ms_key(row["name"])
            if key and rva in self.W:
                wk[key].append(rva)
        scanner = Scanner(self.game)
        hits = defaultdict(set)
        for path in objects:
            for name, body, mask in compiled_functions(path):
                key = gnu_key(name)
                found = scanner.search(body, mask)
                if key and len(found) == 1:
                    hits[key].add(found[0])
        return self.add({wk[key][0]: (next(iter(addresses)), "fingerprint")
                         for key, addresses in hits.items()
                         if len(addresses) == 1 and len(wk.get(key, ())) == 1})

    # -- propagation --------------------------------------------------------
    def _wkey(self, c):
        if c in self.pairs:
            return ("P", self.pairs[c][0])
        return ("L", self.lib[c]) if c in self.lib else None

    def _kkey(self, c):
        return ("L", self.plt[c]) if c in self.plt else ("P", c)

    @staticmethod
    def _lcs(a, b):
        n, m = len(a), len(b)
        T = [[0] * (m + 1) for _ in range(n + 1)]
        for i in range(n - 1, -1, -1):
            for j in range(m - 1, -1, -1):
                T[i][j] = T[i + 1][j + 1] + 1 if a[i] is not None and a[i] == b[j] \
                    else max(T[i + 1][j], T[i][j + 1])
        i = j = 0
        out = []
        while i < n and j < m:
            if a[i] is not None and a[i] == b[j]:
                out.append((i, j))
                i += 1
                j += 1
            elif T[i + 1][j] >= T[i][j + 1]:
                i += 1
            else:
                j += 1
        return out

    def gaps(self) -> int:
        taken = self.k2w()
        proposals = defaultdict(set)
        for w, (k, _) in list(self.pairs.items()):
            if w not in self.W or k not in self.K:
                continue
            wc, kc = self.W[w].calls, self.K[k].calls
            if not wc or not kc or len(wc) > 400 or len(kc) > 400:
                continue
            a, b = [self._wkey(c) for c in wc], [self._kkey(c) for c in kc]
            aligned = [(-1, -1)] + self._lcs(a, b) + [(len(a), len(b))]
            for (i1, j1), (i2, j2) in zip(aligned, aligned[1:]):
                if i2 - i1 == j2 - j1 > 1:
                    for d in range(1, i2 - i1):
                        cw, ck = wc[i1 + d], kc[j1 + d]
                        if cw in self.W and ck in self.K and ck not in self.plt \
                                and cw not in self.lib and cw not in self.pairs and ck not in taken:
                            proposals[cw].add(ck)
        rev = defaultdict(set)
        for cw, cks in proposals.items():
            for ck in cks:
                rev[ck].add(cw)
        return self.add({cw: (next(iter(cks)), "call-graph") for cw, cks in proposals.items()
                         if len(cks) == 1 and len(rev[next(iter(cks))]) == 1})

    def intersections(self) -> int:
        kcallers, wcallers = defaultdict(set), defaultdict(set)
        for a, body in self.K.items():
            for c in body.calls:
                kcallers[c].add(a)
        for a, body in self.W.items():
            for c in body.calls:
                wcallers[c].add(a)
        taken = self.k2w()
        proposals = defaultdict(set)
        for c, ks in kcallers.items():
            if c in taken or c in self.plt or c not in self.K:
                continue
            paired = [k for k in ks if k in taken]
            if len(paired) < 2:
                continue
            common_set = set.intersection(*(
                {x for x in self.W.get(taken[k], refs.Body(0)).calls if x not in self.pairs and x in self.W}
                for k in paired))
            if len(common_set) == 1:
                x = next(iter(common_set))
                if all(self.pairs[w][0] in ks for w in wcallers[x] if w in self.pairs):
                    proposals[x].add(c)
        used = Counter(next(iter(cs)) for cs in proposals.values() if len(cs) == 1)
        return self.add({x: (next(iter(cs)), "call-graph-intersection") for x, cs in proposals.items()
                         if len(cs) == 1 and used[next(iter(cs))] == 1})

    def alignments(self, min_votes: int = 2) -> int:
        taken = self.k2w()
        votes = defaultdict(Counter)
        NEG = -1e9

        def score(x, y):
            if x in self.pairs:
                return 10 if self.pairs[x][0] == y else NEG
            if y in taken:
                return NEG
            if x in self.lib or y in self.plt:
                return 10 if x in self.lib and y in self.plt and self.lib[x] == self.plt[y] else NEG
            if x not in self.W or y not in self.K:
                return NEG
            ratio = abs(math.log(max(self.W[x].size, 1) / max(self.K[y].size, 1)))
            return 1 if ratio < math.log(3) else -2
        for w, (k, _) in list(self.pairs.items()):
            if w not in self.W or k not in self.K:
                continue
            a, b = self.W[w].calls, self.K[k].calls
            if not a or not b or len(a) * len(b) > 40000:
                continue
            n, m = len(a), len(b)
            D = [[0.0] * (m + 1) for _ in range(n + 1)]
            B = [[0] * (m + 1) for _ in range(n + 1)]
            for i in range(1, n + 1):
                D[i][0], B[i][0] = -i, 1
            for j in range(1, m + 1):
                D[0][j], B[0][j] = -j, 2
            for i in range(1, n + 1):
                for j in range(1, m + 1):
                    best, move = D[i - 1][j - 1] + score(a[i - 1], b[j - 1]), 0
                    if D[i - 1][j] - 1 > best:
                        best, move = D[i - 1][j] - 1, 1
                    if D[i][j - 1] - 1 > best:
                        best, move = D[i][j - 1] - 1, 2
                    D[i][j], B[i][j] = best, move
            i, j = n, m
            while i > 0 and j > 0:
                if B[i][j] == 0:
                    x, y = a[i - 1], b[j - 1]
                    if x not in self.pairs and y not in taken and x in self.W and y in self.K \
                            and x not in self.lib and y not in self.plt:
                        votes[x][y] += 1
                    i, j = i - 1, j - 1
                elif B[i][j] == 1:
                    i -= 1
                else:
                    j -= 1
        byk = defaultdict(Counter)
        for x, c in votes.items():
            for y, v in c.items():
                byk[y][x] += v
        found = {}
        for x, c in votes.items():
            ranked = c.most_common(2)
            y, v = ranked[0]
            if v < min_votes or len(ranked) > 1 and ranked[1][1] >= v:
                continue
            back = byk[y].most_common(2)
            if back[0][0] != x or len(back) > 1 and back[1][1] >= v:
                continue
            found[x] = (y, "call-graph-alignment")
        return self.add(found)

    def run(self, fingerprint_objects: list[Path]) -> Counter:
        if fingerprint_objects:
            print(f"[loki-game] fingerprint: {self.fingerprints(fingerprint_objects)}")
        print(f"[loki-game] string: {self.strings()}")
        print(f"[loki-game] vtable: {self.vtables()}")
        for _ in range(8):
            new = self.gaps() + self.intersections() + self.alignments()
            if not new:
                break
        return Counter(e.split(":")[0] for _, e in self.pairs.values())

    # -- output -------------------------------------------------------------
    def write(self, path: Path = RETAIL / "functions.tsv") -> None:
        by_k = {k: (w, e) for w, (k, e) in self.pairs.items()}
        lines = ["# GENERATED by `homm3 loki-game pair` from the Ghidra census (build/heroes3-loki/census)",
                 "# and the Windows retail tables. One row per Loki function; band = text (project and C",
                 "# objects), runtime (libstdc++/libgcc, frame-described), linkonce (kept template and",
                 "# inline bodies) or plt. win_rva/evidence pair it with a Windows HEROES3.EXE function.",
                 "address\tsize\tband\twin_rva\tevidence\twin_name"]
        for address in sorted(self.K):
            band = self.game.band(address)
            w, e = by_k.get(address, (None, ""))
            name = self.ledger.get(w, {}).get("name", "") if w is not None else ""
            lines.append(f"0x{address:08x}\t{self.K[address].size}\t{band}\t"
                         f"{'' if w is None else f'0x{w:x}'}\t{e}\t{name}")
        path.write_text("\n".join(lines) + "\n")


def load(path: Path = RETAIL / "functions.tsv") -> dict[int, tuple[int, str]]:
    """Windows RVA -> (Loki address, evidence) from the committed table."""
    out = {}
    with path.open() as stream:
        for line in stream:
            if not line.startswith("0x"):
                continue
            address, _, _, w, evidence = line.rstrip("\n").split("\t")[:5]
            if w:
                out[int(w, 16)] = (int(address, 16), evidence)
    return out


def default_objects() -> list[Path]:
    """The profile's objects and the SGI-first ones (units that need <limits>);
    an exact body is evidence whichever headers produced it."""
    return sorted(p for tag in ("profile", "sgi-profile")
                  for p in (BUILD / "profile/objects" / tag).glob("*.o"))
