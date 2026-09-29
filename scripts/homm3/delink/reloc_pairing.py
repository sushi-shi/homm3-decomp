"""homm3.delink.reloc_pairing - retail relocation identities from paired code.

A retail relocation target that no source declaration claims is only a
zero-sized address anchor (`data_<rva>`, `const_<rva>`, `vtbl_<rva>`), and an
address the linker folded several bodies onto carries only one of their
names. Strict relocation comparison then fails functions whose instructions
are byte-identical: the candidate names `?g_foo@@3HA` or
`?size@?$vector@PAVwidget@@...`, retail the anchor or the other folded name.

This generator derives those identities at build time from the compiled
candidate objects, the retail image and the reviewed inventories. Nothing is
hand-admitted; deleting its outputs and rebuilding reproduces them.

VOTES. A voter is a source-claimed function whose candidate body equals the
retail body apart from relocated operands: the same length (the candidate
may carry trailing NOP/INT3 fill), identical bytes once every relocation
field is masked, exactly the retail absolute-relocation sites, and only
DIR32/REL32 relocations. Any other difference withdraws all of its votes.
Each relocation of a voter pairs candidate `S + a` with the retail operand
`V`; the vote is `S` at owner address `V - a`. Only spellings both comparison
sides keep verbatim vote.

DATA PAIRINGS (`decide`). A data owner is admitted only when every vote for
`S` names one address, every vote for that address names `S`, no other name
claims the address, `S` is bound nowhere else, and the owner is anchored:
some vote has addend 0, or votes with two different addends agree on the
base. The rest are held with their reason. Admitted owners enter the model
as zero-sized anchors (or name a census vtable, whose extent the census
states); an interior operand becomes a generated relocation-alias row. A
pairing names an address; extents and bytes still come from declarations and
the byte verification.

ADDRESS IDENTITIES (`identities`). Retail code addresses can carry several
proven names: every symbol a byte-verified library section defines at its
offset, and every candidate function a voter calls at a claimed retail
function whose candidate body is exactly that retail body (an identical-code
fold). Normalization uses these names to compare a candidate relocation
against the retail one only when both resolve to the same address.

Outputs (under build/gen/):

    reloc_pairings.tsv        every data/code pairing, verdict and evidence
    reloc_aliases.tsv         reviewed + generated alias rows for vostok
    address_identities.tsv    proven extra names of retail addresses
"""

from __future__ import annotations

import bisect
import struct
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Iterable, NamedTuple

from homm3.core import msvc_names

DIR32 = 0x0006
REL32 = 0x0014
CNT_CODE = 0x00000020
FUNCTION_TYPE = 0x0020
EXTERNAL = 2
STATIC = 3
FILL = frozenset(b"\x90\xcc")

#: Placeholder spellings a pairing may replace: the model's dense anchors and
#: census vtables without a reviewed class.
PLACEHOLDER_PREFIXES = ("data_", "const_", "bss_", "vtbl_")


class Voter(NamedTuple):
    """One source-claimed function: retail rva/size and its candidate object."""
    unit: str
    name: str
    rva: int
    size: int


class Vote(NamedTuple):
    symbol: str
    owner: int          # retail owner rva (V - addend)
    target: int         # retail operand rva V
    addend: int
    function_rva: int
    site_rva: int
    typ: int
    unit: str
    function: str


@dataclass
class Pairing:
    symbol: str
    owner: int
    kind: str                      # 'data' | 'code'
    votes: list = field(default_factory=list)
    verdict: str = ""
    reason: str = ""

    @property
    def functions(self) -> set[int]:
        return {v.function_rva for v in self.votes}

    @property
    def addends(self) -> set[int]:
        return {v.addend for v in self.votes}


def stable_name(name: str) -> bool:
    """A spelling both comparison sides keep verbatim."""
    if not name or name[0] in "$." or name.startswith("_$"):
        return False
    if "?%" in name or "$S" in name or "$RVA" in name or "__h3cg$" in name:
        return False
    return msvc_names.mask(name) == name


def is_placeholder(name: str) -> bool:
    if not name.startswith(PLACEHOLDER_PREFIXES):
        return False
    tail = name.split("_", 1)[1]
    try:
        int(tail, 16)
    except ValueError:
        return False
    return True


class CandidateObject:
    """Code symbols, bodies and relocations of one compiled object."""

    def __init__(self, payload: bytes, absolute: dict[str, int] | None = None):
        from homm3.compare.canonicalize import CoffObject
        absolute = absolute or {}
        coff = CoffObject(payload)
        by_section: dict[int, list] = defaultdict(list)
        for symbol in coff.symbols.values():
            if symbol.section <= 0 or symbol.storage_class not in (EXTERNAL, STATIC):
                continue
            if symbol.name.startswith("."):
                continue
            section = coff.sections[symbol.section - 1]
            if section.characteristics & CNT_CODE:
                by_section[symbol.section].append(symbol)
        relocations: dict[int, list] = defaultdict(list)
        for relocation in coff.relocations:
            relocations[relocation.section].append(relocation)
        self.functions: dict[str, tuple[bytes, list]] = {}
        duplicate = set()
        for index, symbols in by_section.items():
            section = coff.sections[index - 1]
            data = coff.section_bytes(section)
            starts = sorted({s.value for s in symbols})
            for symbol in symbols:
                if symbol.typ != FUNCTION_TYPE:
                    continue
                later = [v for v in starts if v > symbol.value]
                end = later[0] if later else section.raw_size
                # An absolute symbol (`__except_list`, defined by the runtime
                # library) is a linker constant, not an image relocation: its
                # field holds inline + value and compares as ordinary bytes.
                body = bytearray(data[symbol.value:end])
                relocs = []
                for r in relocations[index]:
                    if not symbol.value <= r.site < end:
                        continue
                    target = coff.symbols[r.symbol_index]
                    site = r.site - symbol.value
                    constant = (target.value if target.section == -1 else
                                absolute.get(target.name) if target.section == 0
                                else None)
                    if constant is not None and r.typ == DIR32 and site + 4 <= len(body):
                        inline = struct.unpack_from("<I", body, site)[0]
                        struct.pack_into("<I", body, site, (inline + constant) & 0xFFFFFFFF)
                        continue
                    relocs.append((site, r.typ, target.name))
                relocs.sort(key=lambda row: row[0])
                if symbol.name in self.functions:
                    duplicate.add(symbol.name)
                self.functions[symbol.name] = (bytes(body), relocs)
        for name in duplicate:
            del self.functions[name]


def _masked(body: bytes, sites: Iterable[int]) -> bytes:
    out = bytearray(body)
    for site in sites:
        out[site:site + 4] = b"\0\0\0\0"
    return bytes(out)


def compare_body(body: bytes, relocs: list, retail: bytes | None, size: int,
                 retail_sites: list[int]) -> str:
    """'' when `body` equals `retail` apart from relocated operands."""
    if retail is None or len(retail) != size:
        return "no retail body"
    if len(body) < size:
        return "instruction: shorter"
    if any(site + 4 > size for site, _t, _s in relocs):
        return "instruction: relocation past retail end"
    if len(body) > size and not set(body[size:]) <= FILL:
        return "instruction: longer"
    if any(typ not in (DIR32, REL32) for _site, typ, _s in relocs):
        return "instruction: relocation type"
    if [site for site, typ, _s in relocs if typ == DIR32] != sorted(retail_sites):
        return "instruction: absolute relocation sites"
    sites = [site for site, _t, _s in relocs]
    if _masked(body[:size], sites) != _masked(retail, sites):
        return "instruction: bytes"
    return ""


def operand_targets(body: bytes, relocs: list, retail: bytes, rva: int,
                    image_base: int) -> list[tuple[int, int, str, int, int]]:
    """[(site, typ, name, addend, retail target rva)] for a compared body."""
    out = []
    for site, typ, name in relocs:
        addend = struct.unpack_from("<i", body, site)[0]
        if typ == REL32:
            target = rva + site + 4 + struct.unpack_from("<i", retail, site)[0]
        else:
            target = struct.unpack_from("<I", retail, site)[0] - image_base
        out.append((site, typ, name, addend, target))
    return out


def function_votes(voter: Voter, candidate: CandidateObject, retail: bytes | None,
                   retail_sites: list[int], image_base: int,
                   rename: Callable[[str], str] = lambda name: name,
                   ) -> tuple[list[Vote], str]:
    """(votes, '') for a voter, or ([], reason) when its instructions differ."""
    found = candidate.functions.get(voter.name)
    if found is None:
        return [], "no candidate body"
    body, relocs = found
    reason = compare_body(body, relocs, retail, voter.size,
                          [site - voter.rva for site in retail_sites])
    if reason:
        return [], reason
    votes = []
    for site, typ, name, addend, target in operand_targets(
            body, relocs, retail, voter.rva, image_base):
        if not stable_name(name):
            continue
        votes.append(Vote(rename(name), target - addend, target, addend,
                          voter.rva, voter.rva + site, typ, voter.unit, voter.name))
    return votes, ""


def collect(voters: Iterable[Voter], objects: Callable[[str], CandidateObject | None],
            read_retail: Callable[[int, int], bytes | None],
            retail_sites: list[int], image_base: int,
            ) -> tuple[list[Vote], dict[str, int], int]:
    sites = sorted(retail_sites)
    votes: list[Vote] = []
    withdrawn: dict[str, int] = defaultdict(int)
    admitted = 0
    for voter in voters:
        candidate = objects(voter.unit)
        if candidate is None:
            withdrawn["no candidate object"] += 1
            continue
        lo = bisect.bisect_left(sites, voter.rva)
        hi = bisect.bisect_left(sites, voter.rva + voter.size)
        found, reason = function_votes(
            voter, candidate, read_retail(voter.rva, voter.size), sites[lo:hi],
            image_base)
        if reason:
            withdrawn[reason] += 1
            continue
        admitted += 1
        votes.extend(found)
    return votes, dict(withdrawn), admitted


def decide(votes: list[Vote], *, region_of: Callable[[int], str | None],
           claimed_name_at: Callable[[int], str | None],
           claimed_rva_of: Callable[[str], int | None],
           ) -> tuple[list[Pairing], list[Vote]]:
    """Admit unanimous, anchored, unclaimed DATA pairings; hold the rest.

    ``region_of(rva)`` -> 'text'/'rdata'/'data'/'bss'/None;
    ``claimed_name_at(rva)`` -> a non-placeholder name owning that rva;
    ``claimed_rva_of(name)`` -> where a non-placeholder claim binds that name.
    Code votes are returned as 'code' pairings for `identities` to prove.
    """
    by_symbol: dict[str, list[Vote]] = defaultdict(list)
    by_owner: dict[int, set[str]] = defaultdict(set)
    for vote in votes:
        by_symbol[vote.symbol].append(vote)
        by_owner[vote.owner].add(vote.symbol)
    pairings: list[Pairing] = []
    aliases: list[Vote] = []
    for symbol, rows in sorted(by_symbol.items()):
        owners = sorted({v.owner for v in rows})
        for owner in owners:
            mine = [v for v in rows if v.owner == owner]
            region = region_of(owner)
            kind = "code" if region == "text" else "data"
            pairing = Pairing(symbol, owner, kind, mine)
            pairings.append(pairing)
            claimed = claimed_name_at(owner)
            bound = claimed_rva_of(symbol)
            if kind == "code":
                # Several names at one code address are ordinary linker
                # folding; `identities` decides them from body evidence.
                if claimed == symbol:
                    pairing.verdict, pairing.reason = "confirmed", "claim agrees"
                elif len(owners) > 1:
                    pairing.verdict, pairing.reason = "held", (
                        "symbol votes for " + ",".join(f"{o:#x}" for o in owners))
                elif any(v.addend not in (0, -4) or v.target != owner
                         and v.typ == DIR32 for v in mine):
                    pairing.verdict, pairing.reason = "held", "interior code operand"
                else:
                    pairing.verdict, pairing.reason = "candidate", "needs body proof"
                continue
            if len(owners) > 1:
                pairing.verdict, pairing.reason = "held", (
                    "symbol votes for " + ",".join(f"{o:#x}" for o in owners))
            elif len(by_owner[owner]) > 1:
                pairing.verdict, pairing.reason = "held", (
                    "address votes for " + ",".join(sorted(by_owner[owner])))
            elif region is None or any(region_of(v.target) != region for v in mine):
                pairing.verdict, pairing.reason = "held", "owner outside the operand's region"
            elif claimed == symbol:
                pairing.verdict, pairing.reason = "confirmed", "claim agrees"
            elif claimed is not None:
                pairing.verdict, pairing.reason = "held", f"address claimed as {claimed}"
            elif bound is not None and bound != owner:
                pairing.verdict, pairing.reason = "held", f"symbol claimed at {bound:#x}"
            elif 0 not in pairing.addends and len(pairing.addends) < 2:
                pairing.verdict, pairing.reason = "held", "unanchored addend"
            elif any(v.addend < 0 for v in mine):
                pairing.verdict, pairing.reason = "held", "negative addend"
            else:
                pairing.verdict, pairing.reason = "admitted", ""
            if pairing.verdict == "admitted":
                aliases.extend(v for v in mine if v.addend and v.typ == DIR32)
    return pairings, aliases


def prove_folds(pairings: list[Pairing], *, name_at: Callable[[int], str | None],
                prove: Callable[[str, int], str]) -> None:
    """Admit code pairings whose candidate body is the retail body.

    ``prove(symbol, rva)`` returns '' when some candidate object's body of
    `symbol` is exactly the retail function at `rva` (named relocations
    included), else the reason. A code pairing at an address the model
    names differently is then a proven fold of the two names.
    """
    for pairing in pairings:
        if pairing.kind != "code" or pairing.verdict != "candidate":
            continue
        if name_at(pairing.owner) is None:
            pairing.verdict, pairing.reason = "held", "no retail function at the address"
            continue
        why = prove(pairing.symbol, pairing.owner)
        if why:
            pairing.verdict, pairing.reason = "held", why
        else:
            pairing.verdict, pairing.reason = "folded", f"identical body of {name_at(pairing.owner)}"


PAIRINGS_HEADER = ["owner_rva", "symbol", "kind", "verdict", "reason", "votes",
                   "functions", "addends", "example_function", "example_site"]
ALIAS_HEADER = ["function_rva", "target_rva", "site_rva", "owner", "addend",
                "occurrences"]
IDENTITY_HEADER = ["rva", "name", "proof", "evidence"]


def _hex(value: int) -> str:
    return f"{value:#x}" if value >= 0 else f"-{-value:#x}"


def pairing_rows(pairings: list[Pairing]) -> list[list[str]]:
    rows = []
    for p in sorted(pairings, key=lambda p: (p.kind, p.verdict, p.owner, p.symbol)):
        first = min(p.votes, key=lambda v: v.site_rva)
        rows.append([f"0x{p.owner:08x}", p.symbol, p.kind, p.verdict, p.reason,
                     str(len(p.votes)), str(len(p.functions)),
                     ",".join(_hex(a) for a in sorted(p.addends)),
                     first.function, f"0x{first.site_rva:08x}"])
    return rows


def alias_rows(aliases: list[Vote], reviewed: list[dict]) -> list[list[str]]:
    """Reviewed rows verbatim, then generated exact-site rows for sites no
    reviewed row covers."""
    covered = set()
    out = []
    for row in reviewed:
        out.append([row[k] for k in ALIAS_HEADER])
        covered.add((int(row["function_rva"], 16), int(row["target_rva"], 16),
                     row["site_rva"]))
    for vote in sorted(aliases, key=lambda v: v.site_rva):
        if ((vote.function_rva, vote.target, "*") in covered or
                (vote.function_rva, vote.target, f"0x{vote.site_rva:08x}") in covered):
            continue
        out.append([f"0x{vote.function_rva:08x}", f"0x{vote.target:08x}",
                    f"0x{vote.site_rva:08x}", vote.symbol, f"{vote.addend:#x}", "1"])
    return out


# ------------------------------------------------------------ build driver --

def _gen_dir() -> Path:
    from homm3.core import common
    return common.HOMM3_DIR / "build/gen"


PAIRINGS_OUT = "reloc_pairings.tsv"
ALIASES_OUT = "reloc_aliases.tsv"
IDENTITIES_OUT = "address_identities.tsv"


class Objects:
    """Lazily parsed candidate objects under build/objdiff/base."""

    def __init__(self, base_dir: Path, absolute: dict[str, int]):
        self.base_dir = Path(base_dir)
        self.absolute = absolute
        self.cache: dict[str, CandidateObject | None] = {}

    def __call__(self, unit: str) -> CandidateObject | None:
        if unit not in self.cache:
            path = self.base_dir / f"{unit}.obj"
            try:
                self.cache[unit] = CandidateObject(path.read_bytes(), self.absolute)
            except (OSError, ValueError):
                self.cache[unit] = None
        return self.cache[unit]


@dataclass
class State:
    votes: list
    withdrawn: dict
    voters: int
    pairings: list = field(default_factory=list)
    aliases: list = field(default_factory=list)
    names: dict = field(default_factory=dict)      # inventory rva -> name


_STATE: State | None = None


def _library():
    from homm3.verify.library_code import load_libraries
    return load_libraries(zlib_units=())


def _region_of(pe):
    lo, hi = pe.text_span()
    regions = pe.data_regions()

    def region_of(rva: int) -> str | None:
        if lo <= rva < hi:
            return "text"
        for name, (start, end) in regions.items():
            if start <= rva < end:
                return name
        return None
    return region_of


def data_pairings(claims, sizes: dict[int, int], rows: dict[int, dict],
                  base_dir: Path | None = None) -> State:
    """Phase 1 (model inventory): votes and data-pairing verdicts.

    `claims` are the source fragments' Claims, `sizes` the census extents and
    `rows` the model's inventory so far ({rva: {name, provenance, ...}})."""
    global _STATE
    from homm3.core import common
    from homm3.delink.image import retail
    base_dir = Path(base_dir or common.HOMM3_DIR / "build/objdiff/base")
    img = retail()
    voters = [Voter(c.unit, c.name, c.rva, sizes[c.rva]) for c in claims
              if c.kind == "func" and c.channel in ("src-VA+ir", "src-VA+base")
              and c.rva in sizes]
    library = _library()
    objects = Objects(base_dir, dict(library.absolute))
    votes, withdrawn, admitted = collect(
        voters, objects, lambda rva, size: img.pe.read(rva, size),
        img.reloc_sites, img.image_base)
    by_name = {}
    for rva, row in rows.items():
        if not is_placeholder(row["name"]):
            by_name.setdefault(row["name"], rva)

    def claimed_name_at(rva):
        row = rows.get(rva)
        if row is None or is_placeholder(row["name"]):
            return None
        return row["name"]
    pairings, aliases = decide(votes, region_of=_region_of(img.pe),
                               claimed_name_at=claimed_name_at,
                               claimed_rva_of=by_name.get)
    # An interior operand must not already be another claimed object.
    held = set()
    for vote in aliases:
        other = claimed_name_at(vote.target)
        if other is not None and other != vote.symbol:
            held.add((vote.symbol, vote.owner))
    for pairing in pairings:
        if (pairing.symbol, pairing.owner) in held and pairing.verdict == "admitted":
            pairing.verdict, pairing.reason = "held", "interior operand claimed by another name"
    aliases = [v for v in aliases if (v.symbol, v.owner) not in held]
    _STATE = State(votes, withdrawn, admitted, pairings, aliases,
                   {rva: row["name"] for rva, row in rows.items()})
    return _STATE


def write_aliases(state: State) -> Path:
    """Reviewed alias rows plus generated exact-site rows, for vostok."""
    from homm3.core.paths import RETAIL
    from homm3.core.tsv import read, write
    banner, _header, reviewed = read(RETAIL / "reloc-aliases.tsv")
    out = _gen_dir() / ALIASES_OUT
    out.parent.mkdir(parents=True, exist_ok=True)
    write(out, ["# GENERATED by homm3.delink.reloc_pairing: config/retail/reloc-aliases.tsv",
                "# plus exact-site rows of admitted relocation pairings."],
          ALIAS_HEADER, alias_rows(state.aliases, reviewed))
    return out


def address_identities(model, state: State | None = None,
                       base_dir: Path | None = None) -> list[tuple[int, str, str, str]]:
    """Phase 2 (resolved model): proven extra names of retail code addresses.

    Library: every symbol a byte-verified library section defines there.
    Fold: a code pairing whose candidate body is exactly the retail body at a
    differently named claimed function. Thunk: a candidate import thunk name
    at a retail `jmp [slot]` whose IAT slot the model names `__imp_<name>`.
    """
    from homm3.core import common
    from homm3.core.pe import image
    from homm3.delink.coffx import Obj
    from homm3.delink.image import retail
    from homm3.verify.byte_accounting import library_ranges
    from homm3.verify.startup_bodies import Candidate, match
    state = state or _STATE
    base_dir = Path(base_dir or common.HOMM3_DIR / "build/objdiff/base")
    rows: list[tuple[int, str, str, str]] = []
    _ranges, _summary, defined = library_ranges(image(), model)
    for rva, names in sorted(defined.items()):
        for name in sorted(names):
            rows.append((rva, name, "library", "verified library section"))
    if state is None:
        return rows

    sizes = {b.rva: b.size for b in model.functions}
    names_at = {b.rva: b.name for b in model.functions if b.name and b.channel}
    data_names = dict(state.names)
    data_names.update((b.rva, b.name) for b in model.data if b.name)
    targets: dict[str, set[int]] = defaultdict(set)
    for b in model.functions + model.data:
        for entry in (b, *b.aliases):
            if entry.name:
                targets[msvc_names.mask(entry.name)].add(b.rva)
    for rva, names in defined.items():
        for name in names:
            targets[name].add(rva)
    img = retail()
    definers: dict[str, list[str]] = defaultdict(list)
    candidates: dict[str, Candidate] = {}
    for path in sorted(base_dir.glob("*.obj")):
        try:
            obj = Obj(path)
        except (OSError, ValueError, struct.error):
            continue
        candidate = Candidate(obj)
        candidates[path.stem] = candidate
        for name, places in candidate.symbols.items():
            if any(obj.section_table[section - 1]["characteristics"] & CNT_CODE
                   for _offset, section in places):
                definers[name].append(path.stem)

    def prove(symbol: str, rva: int) -> str:
        reasons = []
        for unit in dict.fromkeys(definers.get(symbol, ())):
            verdict, _deps, why = match(candidates[unit], symbol, rva, img, sizes, targets)
            if verdict == "exact":
                return ""
            reasons.append(f"{unit}: {why}")
        return "; ".join(reasons[:2]) or "no candidate body"

    library_names = {rva: names for rva, names in defined.items()}
    for pairing in state.pairings:
        if pairing.kind != "code" or pairing.verdict != "candidate":
            continue
        if pairing.symbol in library_names.get(pairing.owner, ()):
            pairing.verdict, pairing.reason = "library", "verified library symbol"
            continue
        body = img.pe.read(pairing.owner, 6) or b""
        if body[:2] == b"\xff\x25":
            slot = struct.unpack_from("<I", body, 2)[0] - img.image_base
            if data_names.get(slot) == "__imp_" + pairing.symbol:
                pairing.verdict, pairing.reason = "thunk", f"jmp through {data_names[slot]}"
                rows.append((pairing.owner, pairing.symbol, "thunk",
                             f"jmp [{data_names[slot]}]"))
                continue
    prove_folds(state.pairings, name_at=names_at.get, prove=prove)
    for pairing in state.pairings:
        if pairing.kind == "code" and pairing.verdict == "folded":
            first = min(pairing.votes, key=lambda v: v.site_rva)
            rows.append((pairing.owner, pairing.symbol, "fold",
                         f"{pairing.reason}; called by {first.function}"))
    return rows


def write_outputs(model) -> None:
    """Phase 2 outputs: identities plus the final pairing report."""
    from homm3.core.tsv import write
    state = _STATE
    identities = address_identities(model, state)
    gen = _gen_dir()
    gen.mkdir(parents=True, exist_ok=True)
    write(gen / IDENTITIES_OUT,
          ["# GENERATED by homm3.delink.reloc_pairing - proven extra names of retail addresses."],
          IDENTITY_HEADER,
          [[f"0x{rva:08x}", name, proof, evidence]
           for rva, name, proof, evidence in sorted(set(identities))])
    if state is not None:
        write(gen / PAIRINGS_OUT,
              ["# GENERATED by homm3.delink.reloc_pairing - relocation pairings and verdicts.",
               f"# voters: {state.voters}; withdrawn: " + ", ".join(
                   f"{k}={v}" for k, v in sorted(state.withdrawn.items()))],
              PAIRINGS_HEADER, pairing_rows(state.pairings))
