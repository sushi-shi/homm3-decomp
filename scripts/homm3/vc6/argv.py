"""homm3.vc6.argv - CL.EXE option-spec table decoder + per-pass argv model.

The CL driver (12.00.8168) does not hardcode its switch handling: it walks a
table of 0x14-byte records in .rdata (base VA 0x407550, referenced from .text
0x4020b1/0x4020c3) whose strings are a small sigil language.  This module
decodes that table from the pinned binary and models the driver's dispatch, so
we can answer: for a given CL command line, what argv does each pass - C1/C1XX
(front end) and C2 (back end) - actually receive?

Record layout (file offset o, all dwords little-endian VAs or 0):
    +0x00  pattern    switch-match string        e.g. "Ob2", "d2!+", "Gs:$x[...]"
    +0x04  action     per-pass emission string   e.g. "1PM=*", "D=*,2M-Gy"
    +0x08  incompat   error D2016 list           e.g. "=GZ,=ZI"
    +0x0c  override   warning D4025 removal list e.g. "=Od,=O1,-GF,..."
    +0x10  slot4      requires-one-of list (VA into .rdata, warning D4007),
                      OR a handler function (VA into .text, '@' patterns),
                      OR 0.

Every sigil meaning used here is either proven against the pinned CL under
wine (/Bd prints each pass's command line) or marked unproven in
docs/vc6/driver-passes.md.  Proven pass-selector letters:
    1 = C1.DLL (C front end)      P = C1XX.DLL (C++ front end)
    2 = C2.DLL (back end)         C = LINK.EXE          D = driver state
    S = unproven (only ^bS!*)     M/m = modifiers (m = repeatable, M unproven)

Reusable API: decode_table(cl) and expand(flags) - importable without argparse.
Running `python3 -m homm3.vc6.argv` (re)generates evidence/vc6/cl-option-spec.tsv.
"""
from __future__ import annotations

import json as _json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

from homm3.vc6 import _common, _toolchain

RECORD_SIZE = 0x14
TSV_PATH = _common.EVIDENCE / "cl-option-spec.tsv"
SHIM_LOG = _common.REPO / "build/vc6/shim/argv.log"
UNITS_TOML = _common.REPO / "config/units.toml"

# ---------------------------------------------------------------------------
# table decoding
# ---------------------------------------------------------------------------

@dataclass
class Record:
    off: int                 # file offset of the record
    pattern: str             # dword0 string
    action: str | None       # dword1 string (per-pass emissions)
    incompat: str | None     # dword2 string (error D2016 list)
    override: str | None     # dword3 string (warning D4025 removal list)
    requires: str | None     # dword4 string when it points into .rdata (D4007)
    handler: int | None      # dword4 VA when it points into .text ('@' forms)
    # parsed pattern:
    name: str = ""           # literal switch name ("Ob2", "d2", "Gs")
    sep: str = ""            # '' (no arg) | '!' (arg required) | ':' (optional)
    argspec: str = ""        # raw text after the separator
    next_ok: bool = False    # '>' - argument may be the next argv token
    default: str | None = None  # bracket default, e.g. "1" from "d[0-4,1]"
    internal: bool = False   # '^' prefix - driver-internal, not user-typable


@dataclass
class Table:
    records: list[Record]
    base_off: int            # file offset of the first record
    base_va: int
    compound: dict[str, list[str]]   # prefix -> suboption items, e.g. O -> ['1','2','a-',...]
    compound_off: int

    def by_name(self) -> dict[str, list[Record]]:
        d: dict[str, list[Record]] = {}
        for r in self.records:
            d.setdefault(r.name, []).append(r)
        return d


def _printable(s: str | None) -> bool:
    return (s is not None and 0 < len(s) < 200
            and all(0x20 <= ord(c) < 0x7f for c in s))


def _parse_pattern(rec: Record) -> None:
    """Split a pattern string into name / separator / argspec fields."""
    p = rec.pattern
    if p.startswith("^"):
        rec.internal = True
        p = p[1:]
    m = re.match(r"([^!:]*)([!:]?)(.*)$", p)
    rec.name, rec.sep, rec.argspec = m.group(1), m.group(2), m.group(3)
    if ">" in rec.argspec:
        rec.next_ok = True
    bm = re.search(r"\[([^\]]*)\]", rec.argspec)
    if bm:
        rec.default = bm.group(1).split(",")[-1]


def decode_table(cl: _toolchain.Binary) -> Table:
    """Locate and decode the option-spec table by shape, then anchor it.

    Scan .rdata at 4-byte alignment for record-shaped offsets, group them by
    (offset mod 0x14) phase to reject shifted overlaps (a record's own string
    pointers also look like record heads), take the dominant phase's maximal
    contiguous run, and require the run's base VA to be referenced from .text.
    """
    rdata = next(s for s in cl.sections if s.name == ".rdata")
    text = next(s for s in cl.sections if s.name == ".text")

    def rec_shape(off: int) -> Record | None:
        if off + RECORD_SIZE > rdata.raw + rdata.rsize:
            return None
        d = [cl.u32(off + 4 * i) for i in range(5)]
        if not cl.is_va(d[0]):
            return None
        pat = cl.cstr_at_va(d[0])
        if not _printable(pat):
            return None
        strs: list[str | None] = []
        for v in d[1:4]:
            if v == 0:
                strs.append(None)
                continue
            if not cl.is_va(v):
                return None
            s = cl.cstr_at_va(v)
            if not _printable(s):
                return None
            strs.append(s)
        requires = handler = None
        if d[4]:
            if not cl.is_va(d[4]):
                return None
            sec = cl.sec_of_rva(d[4] - cl.image_base)
            if sec is not None and sec.name == ".text":
                handler = d[4]
            else:
                s = cl.cstr_at_va(d[4])
                if not _printable(s):
                    return None
                requires = s
        return Record(off, pat, strs[0], strs[1], strs[2], requires, handler)

    hits = {off: r for off in range(rdata.raw, rdata.raw + rdata.rsize, 4)
            if (r := rec_shape(off)) is not None}
    if not hits:
        _common.die("no option-spec records found in CL.EXE .rdata")
    phases: dict[int, list[int]] = {}
    for off in hits:
        phases.setdefault(off % RECORD_SIZE, []).append(off)
    phase_offs = sorted(max(phases.values(), key=len))
    # maximal contiguous 0x14-stride run within the dominant phase
    runs: list[list[int]] = [[phase_offs[0]]]
    for off in phase_offs[1:]:
        if off - runs[-1][-1] == RECORD_SIZE:
            runs[-1].append(off)
        else:
            runs.append([off])
    run = max(runs, key=len)
    base_off = run[0]
    base_va = cl.image_base + cl.off_to_rva(base_off)
    if base_va.to_bytes(4, "little") not in cl.data[text.raw:text.raw + text.rsize]:
        _common.die(f"spec-table base VA {base_va:#x} not referenced from .text - "
                    "table location heuristic failed")
    records = [hits[off] for off in run]
    for r in records:
        _parse_pattern(r)

    # The compound-switch table (prefix -> ':'-separated suboption grammar)
    # sits below the record run: pairs of string VAs terminated by (0, 0).
    compound: dict[str, list[str]] = {}
    compound_off = 0
    for off in range(rdata.raw, base_off, 4):
        pairs = []
        o = off
        while o + 8 <= base_off:
            a, b = cl.u32(o), cl.u32(o + 4)
            if a == 0 and b == 0:
                break
            if not (cl.is_va(a) and cl.is_va(b)):
                pairs = None
                break
            sa, sb = cl.cstr_at_va(a), cl.cstr_at_va(b)
            if not (_printable(sa) and _printable(sb) and len(sa) <= 2 and ":" in sb):
                pairs = None
                break
            pairs.append((sa, sb))
            o += 8
        if pairs and any(p[0] == "O" for p in pairs):
            compound = {p: spec.split(":") for p, spec in pairs}
            compound_off = off
            break
    if not compound:
        _common.die("compound-switch prefix table not found below the spec table")
    return Table(records, base_off, base_va, compound, compound_off)


# ---------------------------------------------------------------------------
# switch matching
# ---------------------------------------------------------------------------

def _match_one(table: Table, text: str, allow_internal: bool = False):
    """Match one switch body (no leading / or -) against the records.

    Returns (record, arg or None) or None.  Exact-literal match wins; among
    arg-taking records the longest name wins.
    """
    best = None
    for r in table.records:
        if r.internal and not allow_internal:
            continue
        if not r.sep:
            if text == r.pattern.lstrip("^"):
                return (r, None)     # exact literal - always wins
            continue
        if r.handler is not None and r.argspec == "@" and r.sep == ":":
            # 'link:@' - match "link" exactly (rest of command line consumed)
            if text == r.name:
                return (r, None)
            continue
        if text == r.name:
            cand = (r, None)         # arg-form matched with empty arg
        elif text.startswith(r.name) and r.name:
            cand = (r, text[len(r.name):])
        else:
            continue
        if best is None or len(r.name) > len(best[0].name):
            best = cand
    return best


def _compound_split(table: Table, text: str) -> list[str] | None:
    """Split a combined switch (/Ogtb2, /EHsc) via the prefix grammar table."""
    for prefix, items in table.compound.items():
        if not text.startswith(prefix) or len(text) <= len(prefix):
            continue
        pos, out = len(prefix), []
        ok = True
        while pos < len(text):
            c = text[pos]
            item = next((it for it in items if it and it[0] == c), None)
            if item is None:
                ok = False
                break
            pos += 1
            got = c
            mod = item[1:] if len(item) > 1 else ""
            if mod == "-" and pos < len(text) and text[pos] == "-":
                got += "-"
                pos += 1
            elif mod == "#":
                while pos < len(text) and text[pos].isdigit():
                    got += text[pos]
                    pos += 1
            elif mod == "*":
                got += text[pos:]
                pos = len(text)
            out.append(prefix + got)
        if ok and out:
            return out
    return None


# ---------------------------------------------------------------------------
# the expansion engine
# ---------------------------------------------------------------------------

PASS_OF = {"1": "c1", "P": "c1xx", "2": "c2", "C": "link", "D": "driver", "S": "S"}

# Driver-seeded default switches, in seed order.  OBSERVED, not decoded: this
# list reproduces the /Bd-printed pass command lines of the pinned CL for a
# plain `cl /c file.cpp` (see docs/vc6/driver-passes.md section 6); the seeding
# itself lives in driver code, not in the spec table.  '^'-internal switches
# (il, f, pc, dos) are matchable here only because seeds set allow_internal.
DEFAULT_SEEDS: list[tuple[str, str | None]] = [
    ("il", "<tmp>"), ("f", "%f"), ("W", None), ("Ze", None), ("Zp", None),
    ("ZB", None), ("G5", None), ("Gs", None), ("dos", None), ("Ot", None),
    ("Ob0", None), ("Fo", None), ("pc", "\\:/"), ("ML", None), ("Fd", None),
    ("EHc", None), ("D", "_MSC_VER=1200"), ("D", "_WIN32"),
]


@dataclass
class Token:
    words: list[str]         # argv words, e.g. ['-W', '1'] or ['-Ob2']
    name: str                # keying name ('W', 'Ob2', 'D')
    arg: str | None          # keying arg ('_WIN32' for defines, else None-ish)


@dataclass
class Switch:
    record: Record
    spelling: str            # as typed ('O2', 'D' with arg '_WINDOWS')
    arg: str | None
    user: bool
    emissions: dict[str, list[Token]] = field(default_factory=dict)  # pass -> tokens
    driver: list[str] = field(default_factory=list)


class Engine:
    """Sequential model of the driver's switch processing.

    Semantics status (see docs/vc6/driver-passes.md for the proof table):
    proven   - selectors 1/P/2/C/D, '=' vs '-' both emit, '*' name/arg rule,
               '!' concat, ':' word break, '(a|b)' fallback, '<'/'<<' ext,
               '%b'/'%f', bracket defaults, override/incompat/requires slots,
               're-fire replaces', 'm' accumulates.
    modelled - '-X' override entries recurse one level into X's own override
               list (needed to reproduce the seeded -Ot removal under /O2);
               'M' modifier carried but given no behaviour.
    """

    def __init__(self, table: Table, source: str = "probe.cpp"):
        self.table = table
        self.source = source
        self.switches: list[Switch] = []
        self.diags: list[str] = []
        self.link_extra: list[str] = []

    # -- rendering ----------------------------------------------------------

    def _subst(self, text: str, sw: Switch) -> str:
        base = Path(self.source).stem
        return (text.replace("%b", base).replace("%f", self.source)
                .replace("%X", "exe").replace("%x", base).replace("%m", base))

    def _arg_value(self, sw: Switch) -> str:
        if sw.arg:
            return sw.arg
        return sw.record.default or ""

    def _render(self, text: str, sw: Switch) -> Token:
        """Render one action's text into an argv token (possibly multi-word).

        '*' before any of '!:(' is the switch NAME; later '*' are the ARG.
        '!' joins with no separator; ':' breaks the argv word; '\\c' escapes;
        '(a|b)' takes a unless empty; trailing '<ext' defaults the extension,
        '<<ext' forces it.
        """
        ext_default = ext_force = None
        m = re.search(r"(<<|<)([A-Za-z0-9%]+)$", text)
        if m:
            if m.group(1) == "<<":
                ext_force = self._subst(m.group(2), sw)
            else:
                ext_default = self._subst(m.group(2), sw)
            text = text[:m.start()]

        words, cur = [], ""
        name_zone = True          # '*' resolves to the name until '!','(' or ':'
        key_name, key_arg = "", None
        i = 0
        while i < len(text):
            c = text[i]
            if c == "\\":
                i += 1
                if i < len(text):
                    cur += text[i]
            elif c == "!":
                name_zone = False
            elif c == ":":
                name_zone = False
                words.append(cur)
                cur = ""
            elif c == "(":
                j = text.index(")", i)
                left, _, right = text[i + 1:j].partition("|")
                val = self._arg_value(sw) if left == "*" else self._subst(left, sw)
                if not val:
                    val = self._subst(right, sw)
                if key_arg is None:
                    key_arg = val
                cur += val
                i = j
            elif c == "*":
                if name_zone:
                    cur += sw.record.name or sw.spelling
                else:
                    val = self._arg_value(sw)
                    if key_arg is None:
                        key_arg = val
                    cur += val
            elif c == "%":
                cur += self._subst(text[i:i + 2], sw)
                i += 1
            else:
                cur += c
            i += 1
        words.append(cur)
        if ext_force and words[-1]:
            words[-1] = str(Path(words[-1]).with_suffix("." + ext_force))
        elif ext_default and words[-1] and "." not in Path(words[-1]).name:
            words[-1] += "." + ext_default
        # keying name: the leading name-zone part of the first word
        first = words[0]
        arg_val = self._arg_value(sw)
        key_name = first
        if key_arg is not None and key_arg and first.endswith(key_arg):
            key_name = first[:-len(key_arg)]
        words = ["-" + words[0]] + [w for w in words[1:] if w != ""]
        return Token(words, key_name, key_arg)

    # -- removal (override slot) --------------------------------------------

    def _remove(self, entry: str, by: Switch, warn: bool) -> None:
        """Apply one override-list entry ('=X', '-X', 'X:?', 'X!arg')."""
        m = re.match(r"([^!:]*)(?:(!)(.*)|(:)\?)?$", entry)
        name = m.group(1) if m else entry
        exact_arg = m.group(3) if m and m.group(2) else None
        any_arg = bool(m and m.group(4))
        for sw in list(self.switches):
            if sw is by or sw.record.name != name:
                continue
            if exact_arg is not None and sw.arg != exact_arg:
                continue
            if not any_arg and exact_arg is None and sw.arg and sw.record.sep:
                # plain entry: match any instance (observed: '=Oy' removes /Oy)
                pass
            if warn and sw.user:
                self.diags.append(
                    f"D4025: overriding '/{sw.spelling}' with '/{by.spelling}'")
            self.switches.remove(sw)
        # token-level removal: '=Oy' kills O2's emitted -Oy; '=D!_CPPUNWIND'
        # kills the define token wherever it came from.
        for sw in self.switches:
            for toks in sw.emissions.values():
                toks[:] = [t for t in toks
                           if not (t.name == name
                                   and (any_arg
                                        or (exact_arg is not None and t.arg == exact_arg)
                                        or (exact_arg is None and not t.arg)))]

    def _apply_override_list(self, listing: str, by: Switch) -> None:
        for entry in listing.split(","):
            if not entry:
                continue
            op, body = entry[0], entry[1:]
            self._remove(body, by, warn=(op == "="))
            if op == "-":
                # modelled: '-X' recurses once into X's own override list
                # (reproduces /O2's '-Os' killing the seeded default -Ot).
                for r in self.table.records:
                    if r.name == body.split("!")[0].split(":")[0] and r.override:
                        for e2 in r.override.split(","):
                            if e2:
                                self._remove(e2[1:], by, warn=False)
                        break

    # -- one switch ---------------------------------------------------------

    def process(self, spelling: str, rec: Record, arg: str | None, user: bool) -> None:
        sw = Switch(rec, spelling, arg, user)
        # re-firing the same record replaces the earlier instance (observed:
        # user /ML moves the seeded -ML to the user position)
        for old in list(self.switches):
            if old.record is rec and (rec.name != "D" or old.arg == arg):
                self.switches.remove(old)
        # incompatibility check (error D2016)
        if rec.incompat:
            for entry in rec.incompat.split(","):
                nm = entry.lstrip("=-").split("!")[0].split(":")[0]
                for old in self.switches:
                    if old.user and old.record.name == nm:
                        self.diags.append(
                            f"D2016: '/{old.spelling}' and '/{spelling}' "
                            "command-line options are incompatible")
        if rec.override:
            self._apply_override_list(rec.override, sw)
        if rec.handler is not None and rec.name == "D":
            # /D handler (record 0x7c1c, .text 0x4030ce): modelled as 1PM-D!*
            tok = Token(["-D" + (arg or "")], "D", arg)
            for p in ("c1", "c1xx"):
                sw.emissions.setdefault(p, []).append(tok)
        elif rec.action:
            selectors: set[str] = set()
            repeatable = False
            for action in rec.action.split(","):
                m = re.match(r"([D12PCSMm]*)([=-])(.*)$", action)
                if not m:
                    continue
                letters, _op, text = m.groups()
                if letters:
                    selectors = {c for c in letters if c in PASS_OF}
                    repeatable = "m" in letters
                for letter in sorted(selectors):
                    p = PASS_OF[letter]
                    if p == "driver":
                        sw.driver.append(text)
                        continue
                    tok = self._render(text, sw)
                    if not repeatable:
                        # emitting a token removes any earlier identical token
                        # in the SAME pass (observed: /GX's -EHc supersedes the
                        # seeded -EHc; the seed position empties out)
                        for other in self.switches + [sw]:
                            toks = other.emissions.get(p)
                            if toks is not None:
                                toks[:] = [t for t in toks if t.name != tok.name
                                           or t.words[0] != tok.words[0]]
                    sw.emissions.setdefault(p, []).append(tok)
        self.switches.append(sw)

    def feed(self, argv: list[str]) -> None:
        it = iter(argv)
        for raw in it:
            if raw in ("", "--"):
                continue
            if raw[0] not in "/-":
                self.source = raw
                continue
            text = raw[1:]
            m = _match_one(self.table, text)
            if m is None:
                parts = _compound_split(self.table, text)
                if parts is None:
                    self.diags.append(f"D4002: ignoring unknown option '/{text}'")
                    continue
                for part in parts:
                    pm = _match_one(self.table, part)
                    if pm is None:
                        self.diags.append(
                            f"D4002: ignoring unknown option '/{part}' (from /{text})")
                        continue
                    self.process(part, pm[0], pm[1], user=True)
                continue
            rec, arg = m
            if rec.handler is not None and rec.argspec == "@" and rec.sep == ":":
                self.link_extra.extend(it)   # /link consumes the rest
                continue
            if arg is None and rec.sep == "!" and rec.next_ok:
                arg = next(it, None)         # '>' - argument in the next token
            self.process(text if arg is None else rec.name, rec, arg, user=True)

    def seed(self) -> None:
        for name, arg in DEFAULT_SEEDS:
            m = _match_one(self.table, name, allow_internal=True)
            if m is None:
                continue
            rec = m[0]
            a = arg if arg is not None else m[1]
            if a == "%f":
                a = self.source
            self.process(name, rec, a, user=False)

    def finalize(self) -> dict:
        # requires-one-of (slot 4): drop switches whose requirement is unmet;
        # warn (D4007) only when the switch was user-typed.
        active = {sw.record.name for sw in self.switches}
        for sw in list(self.switches):
            req = sw.record.requires
            if not req:
                continue
            names = [e.lstrip("=-").split("!")[0].split(":")[0]
                     for e in req.split(",")]
            if not any(n in active for n in names):
                if sw.user:
                    pretty = " or ".join("/" + n for n in names)
                    self.diags.append(f"D4007: '/{sw.spelling}' requires "
                                      f"'{pretty}'; option ignored")
                self.switches.remove(sw)

        lang = "c++" if not self.source.lower().endswith(".c") else "c"
        for sw in self.switches:
            for d in sw.driver:
                if d.startswith("bt:"):
                    lang = {"C": "c", "P": "c++", "O": lang}.get(d[3:], lang)
        fe_letter = "c1xx" if lang == "c++" else "c1"
        out = {"front_end": "C1XX.DLL" if lang == "c++" else "C1.DLL",
               "language": lang, "source": self.source,
               "c1": [], "c2": [], "link": list(self.link_extra),
               "driver": [], "diagnostics": self.diags, "provenance": {}}
        prov: dict[str, list] = {"c1": [], "c2": [], "link": []}
        for sw in self.switches:
            for p, toks in sw.emissions.items():
                view = ("c1" if p == fe_letter else
                        "c2" if p == "c2" else
                        "link" if p == "link" else None)
                if view is None:
                    continue
                for t in toks:
                    out[view].extend(t.words)
                    prov[view].append({"tokens": t.words,
                                       "switch": "/" + sw.spelling,
                                       "record_off": f"{sw.record.off:#x}",
                                       "default": not sw.user})
            for d in sw.driver:
                shown = d.replace("*", sw.arg or sw.spelling)
                out["driver"].append(f"{shown}  (/{sw.spelling} @{sw.record.off:#x})")
        out["provenance"] = prov
        return out


def expand(flags: list[str], source: str | None = None) -> dict:
    """Reusable entry: per-pass argv model for a CL flag list."""
    cl = _toolchain.Binary("CL.EXE")
    table = decode_table(cl)
    eng = Engine(table, source or "probe.cpp")
    if source is None:
        for f in flags:
            if f and f[0] not in "/-":
                eng.source = f
    eng.seed()
    eng.feed(flags)
    return eng.finalize()


# ---------------------------------------------------------------------------
# evidence TSV
# ---------------------------------------------------------------------------

def dump_table_tsv(path: Path = TSV_PATH) -> int:
    cl = _toolchain.Binary("CL.EXE")
    table = decode_table(cl)
    sha = _toolchain.PINNED["CL.EXE"][0]
    lines = _common.provenance("homm3.vc6.argv", [
        f"# subject: CL.EXE sha256={sha} (driver 12.00.8168)",
        f"# spec table: .rdata file 0x{table.base_off:x}..0x"
        f"{table.base_off + len(table.records) * RECORD_SIZE:x}, "
        f"{len(table.records)} records of 0x14 bytes (base VA {table.base_va:#x}, "
        "referenced from .text 0x20b1/0x20c3)",
        f"# compound-prefix table: .rdata file 0x{table.compound_off:x} "
        f"({', '.join(sorted(table.compound))})",
        "# slot4 = requires-one-of list (D4007) or handler:<.text VA> for '@' patterns",
        "# columns: file_off\tpattern\texpansion1\texpansion2\texpansion3\tslot4",
    ])
    for r in table.records:
        slot4 = (f"handler:{r.handler:#010x}" if r.handler is not None
                 else r.requires or ".")
        lines.append("\t".join([
            f"0x{r.off:x}", r.pattern, r.action or ".", r.incompat or ".",
            r.override or ".", slot4]))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n")
    print(f"[homm3 vc6] wrote {path} ({len(table.records)} records)")
    return 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _unit_flags(unit: str) -> tuple[list[str], str]:
    import tomllib
    if not UNITS_TOML.is_file():
        _common.die(f"{UNITS_TOML} not found")
    cfg = tomllib.loads(UNITS_TOML.read_text())
    u = next((x for x in cfg.get("unit", []) if x.get("unit") == unit), None)
    if u is None:
        _common.die(f"unit '{unit}' not in {UNITS_TOML}")
    profile = u["flags"]
    flags = cfg.get("flags", {}).get(profile)
    if flags is None:
        _common.die(f"flag profile '{profile}' not in {UNITS_TOML} [flags]")
    return list(flags), Path(u["source"]).name


def _verify(model: dict) -> int:
    if not SHIM_LOG.is_file():
        print(f"--verify: shim log not found at {SHIM_LOG}; "
              "run the shim first (phase 0 rig). Model output stands unverified.")
        return 0
    rc = 0
    logged: dict[str, list[str]] = {}
    for line in SHIM_LOG.read_text().splitlines():
        low = line.lower()
        for key, names in (("c1", ("c1.dll", "c1xx.dll")), ("c2", ("c2.dll",))):
            if any(n in low for n in names):
                toks = line.strip().strip("`'").split()
                logged[key] = [t for t in toks[1:] if t]
    for key in ("c1", "c2"):
        if key not in logged:
            continue
        skip_next = False
        actual = []
        for i, t in enumerate(logged[key]):
            if skip_next:
                skip_next = False
                continue
            if t in ("-il", "-f"):
                skip_next = True
                continue
            actual.append(t)
        predicted = [t for i, t in enumerate(model[key])
                     if model[key][i - 1:i] != ["-il"] and t not in ("-il",)]
        missing = [t for t in actual if t not in predicted]
        extra = [t for t in predicted if t not in actual and not t.startswith("<")]
        status = "AGREES" if not missing and not extra else "DISAGREES"
        if status == "DISAGREES":
            rc = 1
        print(f"--verify {key}: {status}"
              + (f"  missing={missing} extra={extra}" if rc else ""))
    return rc


def run(args) -> int:
    if args.flags is None and args.unit is None:
        _common.die("argv: give --flags \"<cl command line>\" or --unit <name>")
    if args.unit:
        flags, source = _unit_flags(args.unit)
    else:
        flags, source = args.flags.split(), None
    model = expand(flags, source=source)

    if args.json:
        print(_json.dumps(model, indent=2))
    else:
        which = [args.which] if args.which else ["c1", "c2"]
        fe = model["front_end"]
        print(f"# source: {model['source']}  front end: {fe}")
        for d in model["diagnostics"]:
            print(f"# diag: {d}")
        print(f"{'pass':6}  {'argv':34}  {'switch':14}  record")
        for key in which:
            label = fe.split(".")[0].lower() if key == "c1" else "c2"
            for p in model["provenance"][key]:
                src = p["switch"] + (" (default)" if p["default"] else "")
                print(f"{label:6}  {' '.join(p['tokens']):34}  {src:14}  "
                      f"{p['record_off']}")
        if model["link"]:
            print(f"# /link passthrough: {' '.join(model['link'])}")
        if model["driver"]:
            print("# driver state: " + "; ".join(model["driver"]))
    if args.verify:
        return _verify(model)
    return 0


if __name__ == "__main__":
    # `python3 -m homm3.vc6.argv` (re)generates the evidence table.
    sys.exit(dump_table_tsv())
