"""homm3.delink.data_manifest - vostok's `--data-manifest` + section manifest.

The reviewed-data-topology delinker emits each claimed global and each `??_C@`
string literal as a real named definition in its owning target object (right
storage class + alignment, interior base relocations converted to COFF
relocations, references to it becoming EXTERNALS). The companion SECTION
manifest additionally rebuilds those definitions in the CANDIDATE's section
shape: objdiff scores data all-or-nothing per SECTION, so a packed target
section can never reach 100.0 even with every payload present.

Schemas (read out of the delinker binary; the data manifest takes 9 or 10
columns):
    name  object  rva  size  storage  alignment  [section_ordinal]
        section_offset  scope  provenance
    object  ordinal  name  rva  size  alignment  characteristics
        comdat_selection  associative_ordinal  storage  provenance
`section_ordinal = -` selects the legacy reviewed-allocation form; ordinals
are per-object and must be CONTIGUOUS FROM ONE.

Inputs (the Model replaces the old tree's symbol_names.csv):
  * Model.data claimed bindings - names, units, model-resolved extents; the
    retail PE classifies each row's storage;
  * the base objs (build/objdiff/base) - cl's own string/vtable/RTTI COMDATs,
    section topology, FP pools;
  * the Model's data_vtables/data_static_libs bindings (winners AND aliases)
    - the reviewed ??_7 name -> (rva, slots) authority.

Evidence rules (never fabricate an extent): an extent is the claim-stated
size or the admitted census's derived boundary span, never guessed; an
overlap withholds BOTH sides and reports; an extent crossing a storage
boundary is withheld. Mechanisms ported from the old tree's
build/data_manifest.py (see its docstrings for the measured history).
"""

from __future__ import annotations

import bisect
import re
from collections import Counter, defaultdict
from pathlib import Path

from homm3.core import msvc_names
from homm3.core.paths import BUILD
from homm3.delink import coffx, eh_band
from homm3.delink.image import retail
from homm3.model import Model
from homm3.retail_labels import fragments

BASE_DIR = BUILD / "objdiff/base"
OUTPUT = BUILD / "gen/delink_data_manifest.tsv"
SECTION_OUTPUT = BUILD / "gen/delink_data_section_manifest.tsv"

HEADER = ("name", "object", "rva", "size", "storage", "alignment",
          "section_ordinal", "section_offset", "scope", "provenance")
SECTION_HEADER = ("object", "ordinal", "name", "rva", "size", "alignment",
                  "characteristics", "comdat_selection", "associative_ordinal",
                  "storage", "provenance")
#: retail PE storage class -> the delinker's storage keyword
STORAGE = {"rdata": "rdata", "data-initialized": "data",
           "data-loader-zero-tail": "bss"}

#: Data channels candidates() enrolls. data_vtables is label-only (its rows
#: enroll through vtable_rows, against the candidate COMDATs).
_ENROLL_CHANNELS = ("src", "data_compgen", "data_zlib", "data_static_libs",
                    "src_data_compgen")

#: `IMAGE_SCN_LNK_COMDAT`.
LNK_COMDAT = 0x00001000
#: `IMAGE_SCN_MEM_EXECUTE`.
MEM_EXECUTE = 0x20000000
#: `IMAGE_REL_I386_DIR32` - the only relocation type in the PE's `.reloc` dir.
COFF_DIR32 = 0x0006
#: candidate section name -> the delinker's storage keyword (the only two that
#: carry initialized bytes an ordinary definition can sit in).
ORDINARY_STORAGE = {".data": "data", ".rdata": "rdata"}

#: cl's floating-point literal pool member spelling.
FP_POOL_NAME = re.compile(r"^\$T[0-9]+$")

#: The value c2's per-section alignment ratchet starts at (see _alignment).
UNLATCHED_RATCHET = 4

#: MSVC data-symbol mangling: the storage-code digit; everything after is the
#: TYPE. Matched from the right - a type never opens a `@<digit>` group.
_MANGLED_STORAGE_CODE = re.compile(r"@([0-9])(?=[A-Z_?$])")
#: The mangled primitive codes for the only i386 scalars c2 aligns to 8.
_MANGLED_WIDE_SCALAR = ("N", "O", "_J", "_K")
#: ... and their clang spellings, as the fragment `type` column prints them.
_CLANG_WIDE_SCALAR = frozenset((
    "double", "long double", "__int64", "unsigned __int64", "signed __int64",
    "long long", "unsigned long long", "i64", "u64"))
_CV_QUALIFIER = re.compile(r"\b(?:const|volatile)\b")

#: Band-gap caps: bigger gaps are withheld BY NAME, never carved silently.
GAP_CAP = 0x100
GAP_CAP_BSS = 0x4000


# --- alignment ---------------------------------------------------------------

def obj_align(kind: str, size: int, ratchet: int) -> int:
    """c2 per-object alignment (validated 41/41 on a blind TU - see
    https://github.com/sushi-shi/homm3-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/compiler-data-layout.md). ratchet = section's max align so far."""
    if kind == "double":
        return 8
    if kind == "scalar":
        return 4
    # array / aggregate
    if size > 8:
        return 8
    if size < 4:
        return 4
    return ratchet


_TYPES_CACHE: dict[int, str] | None = None


def declared_types() -> dict[int, str]:
    """{rva: clang qualType} for every src DATA() claim (the fragment cache's
    `type` column - the same authority the extraction derives extents from)."""
    global _TYPES_CACHE
    if _TYPES_CACHE is None:
        out = {}
        for c in fragments.all_claims():
            if c.kind == "data" and c.meta.get("type"):
                out[c.rva] = c.meta["type"]
        _TYPES_CACHE = out
    return _TYPES_CACHE


def _object_kind(name, rva, size, types=None):
    """Compiler alignment kind for c2's rule, or None if unproven."""
    if name.startswith("$T"):
        # cl's FP pool: members are float/double literals and nothing else.
        return "double" if size == 8 else "scalar"
    qt = (declared_types() if types is None else types).get(rva)
    if qt:
        t = re.sub(r"\s+", " ", _CV_QUALIFIER.sub("", qt)).strip()
        if t.endswith("]"):
            return "array"
        if t.endswith("*") or t.endswith("&"):
            return "scalar"
        if t in _CLANG_WIDE_SCALAR:
            return "double"
        return None
    code = list(_MANGLED_STORAGE_CODE.finditer(name))
    if code:
        mangled = name[code[-1].end():]
        if mangled.startswith(_MANGLED_WIDE_SCALAR):
            return "double"
    return None


def _alignment(rva, size, kind):
    """(alignment, modelled) - usable manifest alignment and c2's object model.

    The per-section ratchet is not recoverable from a manifest (emission order
    inside the ORIGINAL TU), so the conservative branch is its un-latched
    value; the legacy allocation form then needs an alignment that divides the
    final retail RVA, so the modelled value is lowered to the largest usable
    divisor (not a source refutation - the linker places whole contributions).
    """
    modelled = obj_align(kind or ("array" if size > 8 else "scalar"), size,
                         UNLATCHED_RATCHET)
    a = modelled
    while rva % a:
        a //= 2
    return a, modelled


def member_alignment(section_alignment, offset, rva=None):
    """Alignment guaranteed for one folded symbol across all of its COMDATs
    (gcd of the member offset and the section alignment, lowered to divide the
    linked address)."""
    alignment = (section_alignment if offset == 0
                 else min(section_alignment, offset & -offset))
    if rva is not None:
        while rva % alignment:
            alignment //= 2
    return alignment


# --- shared helpers ----------------------------------------------------------

def _classify(rva: int) -> str:
    return retail().classify_storage(rva)


def _reloc_data_rvas() -> list[int]:
    """Every PE relocation-target address inside .rdata/.data (the candidate
    set the string oracle content-matches)."""
    img = retail()
    rd = img.pe.section(".rdata")
    da = img.pe.section(".data")
    out = set()
    for site in img.reloc_sites:
        value = img.u32(site)
        if value is None:
            continue
        rva = value - img.image_base
        if rd["va"] <= rva < rd["va"] + rd["vsize"] \
                or da["va"] <= rva < da["va"] + da["vsize"]:
            out.add(rva)
    return sorted(out)


_VTABLE_CHANNELS = ("data_vtables", "data_static_libs")


def _vtable_tables(model: Model) -> dict[str, tuple[int, int]]:
    """{??_7 name: (rva, slot count)} - the reviewed vtable extents, read off
    the Model's bindings AND their aliases (a secondary spelling losing one
    rva to the primary is still a reviewed extent for that name).

    data_vtables covers the game/engine classes; the static-lib channel
    carries the MFC/CRT vtables a game TU can re-emit (CObject, ...), which
    the candidate-COMDAT cross-check needs the same way."""
    out: dict[str, tuple[int, int]] = {}
    for b in model.data:
        if b.channel in _VTABLE_CHANNELS and b.name.startswith("??_7") \
                and b.size and b.size % 4 == 0:
            out.setdefault(b.name, (b.rva, b.size // 4))
        for a in b.aliases:
            if a.channel in _VTABLE_CHANNELS and a.name.startswith("??_7") \
                    and a.size and a.size % 4 == 0:
                out.setdefault(a.name, (b.rva, a.size // 4))
    return out


def retail_col_head(vtable_rva: int) -> int:
    """4 if retail's COMDAT for this vtable opens with a `??_R4` COL pointer,
    else 0. The reloc alone proves nothing (the previous vtable's last slot is
    also relocated), so the COL structure is walked: signature word 0 and a
    `pTypeDescriptor` whose name begins `.?A`."""
    img = retail()
    sites = img.reloc_sites
    site = vtable_rva - 4
    i = bisect.bisect_left(sites, site)
    if vtable_rva < 4 or i >= len(sites) or sites[i] != site:
        return 0
    va = img.u32(vtable_rva - 4)
    if va is None:
        return 0
    col = va - img.image_base
    if img.sec_name(col) != ".rdata" or img.u32(col) != 0:
        return 0
    ptd = img.u32(col + 12)
    if ptd is None:
        return 0
    name = img.cstr(ptd - img.image_base + 8, 64)
    return 4 if name and name.startswith(".?A") else 0


def _link_position() -> dict[str, int]:
    """{unit: link-arrival position} from the derived retail link order
    (headerless: `position<TAB>unit<TAB>lo<TAB>hi...` rows)."""
    from homm3.core.paths import RETAIL
    out: dict[str, int] = {}
    path = RETAIL / "link_order.tsv"
    if not path.exists():
        return out
    for line in path.read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        fields = line.split("\t")
        try:
            out.setdefault(fields[1], int(fields[0]))
        except (IndexError, ValueError):
            continue
    return out


def _common_owner(base_dir=BASE_DIR):
    """{COMMON name: unit} - the unit whose contribution the retail linker
    ALLOCATED the COMMON from: the earliest-arriving module (link_order) among
    the base objs that emit it (a COMMON has no owning TU; cl emits a copy
    into every instantiating TU and the linker keeps the first).

    Proven on retail: `?holdrand@...GetRandomNumber@@YAHXZ...` is emitted by
    12 objs and retail's copy belongs to worldsoundset - link position 6, the
    earliest of the 12."""
    from homm3.core.coff import Coff
    pos = _link_position()
    owners: dict[str, str] = {}
    emitters = defaultdict(set)
    for obj in sorted(Path(base_dir).glob("*.obj")):
        try:
            commons = Coff(obj).commons()
        except (ValueError, OSError):
            continue
        for name in commons:
            key = msvc_names.mask(name)
            emitters[key].add(obj.stem)
            prev = owners.get(key)
            if (msvc_names.anonymous_namespaces(name) != name
                    and prev is not None and prev != obj.stem):
                raise ValueError("source-file anonymous COMMON emitted by multiple units: "
                                 + key + " in " + prev + " and " + obj.stem)
            if prev is None or pos.get(obj.stem, 1 << 30) < pos.get(prev, 1 << 30):
                owners[key] = obj.stem
    # A missing link-order inventory does not make alphabetical order evidence.
    return {name: unit for name, unit in owners.items()
            if len(emitters[name]) == 1 or all(u in pos for u in emitters[name])}


def _candidate_member_storage(base_dir=BASE_DIR):
    """{(object, symbol): "data"|"rdata"|"bss"} - the storage cl ACTUALLY gave
    it. The oracle that resolves the .data raw-edge ambiguity and refutes a
    named-static-onto-pooled-literal alias whose storage never met.

    Keyed by the MASKED symbol: cl stamps its per-object CodeView counter onto
    a TU-local static and the Model binds the ordinal-free spelling, so both
    sides of the join reduce to the same name."""
    out = {}
    for stem, c in coffx.objects(base_dir):
        for sec in c.section_table:
            storage = ORDINARY_STORAGE.get(sec["name"])
            if storage is None and sec["name"] == ".bss":
                storage = "bss"
            if storage is None:
                continue
            for _off, name, _scl in c.section_members(sec["index"]):
                out[f"{stem}.c", msvc_names.mask(name)] = storage
    return out


# --- enrolment channels ------------------------------------------------------

def claim_rows(model: Model, tail_oracle) -> tuple[list, list, Counter]:
    """The Model's claimed data rows, with the model-resolved extent: the
    claim-stated size where the claim states one, else the extent the admitted
    data census derives to the next row - a boundary proof the old tree did
    not have (measured: every old type-derived size agrees with it). Storage
    from the retail PE with the per-unit base-obj oracle at the .data
    raw-size edge."""
    common_owner = _common_owner()
    rows, withheld, skipped = [], [], Counter()
    guard_owner_rva = None
    for b in model.data:
        if b.channel not in _ENROLL_CHANNELS or not b.name:
            if b.channel == "data_vtables":
                skipped["data_vtables (label-only)"] += 1
            continue
        if b.space == "idata":
            skipped["idata space (not enrollable yet)"] += 1
            continue
        unit = b.unit
        if b.kind == "common":
            # A COMMON has no owning TU (b.unit documents the header inline);
            # the retail linker allocated it from the earliest-arriving module
            # that emits it, which is the object it belongs in.
            unit = common_owner.get(msvc_names.mask(b.name))
            if unit is None:
                withheld.append((b.rva, b.name,
                                 "COMMON with no emitting base obj"))
                continue
        size = b.size
        start = _classify(b.rva)
        end = _classify(b.rva + size - 1)
        oracle = (tail_oracle.get((f"{unit}.c", msvc_names.mask(b.name)))
                  if "data-unprovable-tail" in (start, end) else None)
        if oracle is None and "data-unprovable-tail" in (start, end) \
                and "$static_init_guard$" in b.name:
            # cl names a guard `$S<n>` in its owner static's scope and emits
            # both into one section: the owner's storage is the guard's.
            if guard_owner_rva is None:
                from homm3.core.common import HOMM3_DIR
                from homm3.verify.byte_accounting import _guard_owners
                named = {x.rva: x.name for x in model.data if x.name}
                guard_owner_rva = {g: named.get(rva) for g, (_o, rva)
                                   in _guard_owners(HOMM3_DIR).items()}
            owner = guard_owner_rva.get(b.name)
            if owner:
                oracle = tail_oracle.get((f"{unit}.c", msvc_names.mask(owner)))
        if oracle in ("data", "bss"):
            # The PE alone cannot split FileAlignment slack from content at
            # the .data raw-size edge - the claiming unit's own base obj can:
            # cl compiled the same declaration, so where IT put the symbol is
            # where retail's cl did.
            fixed = "data-initialized" if oracle == "data" \
                else "data-loader-zero-tail"
            start = fixed if start == "data-unprovable-tail" else start
            end = fixed if end == "data-unprovable-tail" else end
        if start not in STORAGE:
            withheld.append((b.rva, b.name,
                             f"storage {start} is not enrollable"))
            continue
        if start != end:
            withheld.append((b.rva, b.name,
                             f"extent crosses {start} -> {end}"))
            continue
        rows.append({"name": b.name, "object": f"{unit}.c", "rva": b.rva,
                     "size": size, "storage": STORAGE[start],
                     "provenance": "zlib-source-sizeof" if b.channel == 'data_zlib'
                                   else "src-DATA-sizeof"})
    return rows, withheld, skipped


def nonliteral_storage(model, types):
    """Named typed objects cannot impersonate anonymous string allocations.

    Keep const character arrays eligible for the existing pooled-alias check.
    Other DATA objects, including their interior bytes, have distinct storage:
    an integer 44 in a hotkey table is not a pooled comma string.
    """
    return [(b.rva, b.rva + b.size) for b in model.data
            if b.channel == "src" and b.size and types.get(b.rva)
            and not re.fullmatch(r"const (?:signed |unsigned )?char(?:\[\d+\])+",
                                 types[b.rva])]


def paired_code_votes(model: Model, wanted, base_dir=BASE_DIR):
    """{symbol: {rva}} read off retail at relocation sites paired with ours.

    A claimed function's DIR32 sites pair positionally with retail's admitted
    sites only when both counts agree, and every referent the Model already
    knows must then equal the address retail wrote (plus our addend). Only
    such a corroborated pairing may state where a ``wanted`` symbol lives.
    """
    import struct

    img = retail()
    sites = img.reloc_sites
    from homm3.delink.pdb_synth import UNIT_CHANNELS
    known, fn_extent = {}, {}
    for b in model.functions:
        if b.channel in UNIT_CHANNELS and b.name:
            known[msvc_names.mask(b.name)] = b.rva
            fn_extent[msvc_names.mask(b.name)] = (b.rva, b.size)
    for b in model.data:
        if b.channel and b.name:
            known.setdefault(msvc_names.mask(b.name), b.rva)
    votes: dict[str, set[int]] = defaultdict(set)
    for _stem, c in coffx.objects(base_dir):
        for sec in c.section_table:
            if not sec["characteristics"] & MEM_EXECUTE:
                continue
            rel = {site: nm for site, (nm, typ)
                   in c.typed_relocations(sec["index"]).items()
                   if typ == COFF_DIR32}
            if not rel or not any(wanted(n) for n in rel.values()):
                continue
            text = c.section_payload(sec["index"])
            for off, name in c.defined_symbols(sec["index"]):
                hit = fn_extent.get(msvc_names.mask(name))
                if hit is None:
                    continue
                rva, size = hit
                mine = sorted((s, n) for s, n in rel.items()
                              if off <= s < off + size)
                lo = bisect.bisect_left(sites, rva)
                hi = bisect.bisect_left(sites, rva + size)
                theirs = sites[lo:hi]
                if not mine:
                    continue
                # Pair the sites up to the first place the two bodies' site
                # offsets diverge; past it the code no longer lines up. Each
                # wanted site must also sit behind the same instruction bytes.
                found, corroborated, anchored = [], True, 0
                for (site, sym), target in zip(mine, theirs):
                    at = img.off(target)
                    if at is None or target - rva != site - off:
                        break
                    head = img.data[at - 2:at]
                    if wanted(sym) and head != text[site - 2:site]:
                        break
                    addend = struct.unpack("<i", text[site:site + 4])[0]
                    value = struct.unpack("<I", img.data[at:at + 4])[0] \
                        - img.image_base - addend
                    if wanted(sym):
                        found.append((sym, value))
                        continue
                    anchor = known.get(msvc_names.mask(sym))
                    if anchor is not None:
                        if value != anchor:
                            corroborated = False
                            break
                        anchored += 1
                exact = len(mine) == len(theirs) and len(found) + anchored > 0
                if corroborated and (anchored or exact):
                    for sym, value in found:
                        votes[sym].add(value)
    return votes


#: COMDAT data other channels already own (literals, vtables, RTTI,
#: exception records, FP pools).
_OWNED_COMDAT_PREFIXES = ("??_C@", "??_7", "??_R", "__CT", "__TI", "$T")


def paired_store_votes(model: Model, wanted, base_dir=BASE_DIR):
    """{symbol: {rva}} from immediate stores (`mov r/m32, imm32`) paired in order.

    A claimed function whose compiled body stores exactly as many wanted
    symbol addresses through `C7 /0` as retail's body stores data addresses
    the same way pairs them in order: constructors publishing vtables and
    virtual-base tables are the typical case.
    """
    import struct
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    from homm3.delink.pdb_synth import UNIT_CHANNELS
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    img = retail()
    fn_extent = {msvc_names.mask(b.name): (b.rva, b.size) for b in model.functions
                 if b.channel in UNIT_CHANNELS and b.name}
    data_lo = min(s["va"] for s in img.pe.sections if s["name"] in (".rdata", ".data"))
    data_hi = max(s["va"] + s["vsize"] for s in img.pe.sections if s["name"] in (".rdata", ".data"))
    votes: dict[str, set[int]] = defaultdict(set)
    for _stem, c in coffx.objects(base_dir):
        for sec in c.section_table:
            if not sec["characteristics"] & MEM_EXECUTE:
                continue
            rel = {site: nm for site, (nm, typ)
                   in c.typed_relocations(sec["index"]).items() if typ == COFF_DIR32}
            if not any(wanted(n) for n in rel.values()):
                continue
            text = c.section_payload(sec["index"])
            for off, name in c.defined_symbols(sec["index"]):
                hit = fn_extent.get(msvc_names.mask(name))
                if hit is None:
                    continue
                rva, size = hit
                mine = []
                for insn in md.disasm(text[off:off + size], 0):
                    if insn.bytes[0] == 0xC7 and insn.size >= 6:
                        site = off + insn.address + insn.size - 4
                        mine.append(rel.get(site))
                body = img.pe.read(rva, size) or b""
                theirs = []
                for insn in md.disasm(body, 0):
                    if insn.bytes[0] == 0xC7 and insn.size >= 6:
                        value = struct.unpack_from("<I", insn.bytes, insn.size - 4)[0] \
                            - img.image_base
                        if data_lo <= value < data_hi:
                            theirs.append(value)
                mine = [n for n in mine if n is not None]
                if mine and len(mine) == len(theirs):
                    for n, value in zip(mine, theirs):
                        if wanted(n):
                            votes[n].add(value)
    return votes


def paired_comdat_rows(model: Model, base_dir=BASE_DIR):
    """Game-emitted COMDAT data placed by corroborated code pairings.

    Constants (`__real@`), virtual-base tables (`??_8`) and template static
    members are one-symbol COMDAT sections. Where every paired game
    reference to one names a single retail address and retail holds the
    section's bytes there (relocation words aside), that COMDAT is linked
    from the game object at that address.
    """
    comdats = defaultdict(list)
    comdat_storage = {**ORDINARY_STORAGE, ".bss": "bss"}
    for stem, c in coffx.objects(base_dir):
        for sec in c.section_table:
            if not sec["characteristics"] & LNK_COMDAT \
                    or sec["characteristics"] & MEM_EXECUTE \
                    or sec["name"] not in comdat_storage:
                continue
            members = c.defined_symbols(sec["index"])
            if len(members) != 1 or members[0][0] != 0 \
                    or members[0][1].startswith(_OWNED_COMDAT_PREFIXES):
                continue
            payload = c.section_payload(sec["index"])[:sec["size"]]
            masked = bytearray(payload.ljust(sec["size"], b"\0"))
            for site in c.relocations(sec["index"]):
                masked[site:site + 4] = b"\0\0\0\0"
            comdats[members[0][1]].append((stem, sec, bytes(masked),
                                           set(c.relocations(sec["index"]))))
    if not comdats:
        return [], []
    votes = paired_code_votes(model, lambda name: name in comdats, base_dir)
    for name, rvas in paired_store_votes(model, lambda name: name in comdats
                                         and name.startswith("??_8"), base_dir).items():
        votes[name] |= rvas
    img = retail()
    rows, withheld = [], []
    for name, copies in sorted(comdats.items()):
        seen = votes.get(name, set())
        if len(seen) != 1:
            if len(seen) > 1:
                withheld.append((min(seen), name, "paired references disagree"))
            continue
        rva = next(iter(seen))
        sizes = {sec["size"] for _stem, sec, _m, _r in copies}
        if len(sizes) != 1:
            withheld.append((rva, name, "COMDAT copies disagree on the extent"))
            continue
        size = sizes.pop()
        _stem, sec, masked, sites = copies[0]
        storage = _classify(rva)
        at = img.off(rva)
        if at is None and STORAGE.get(storage) == "bss" == comdat_storage[sec["name"]]:
            # Loader-zeroed storage past .data's raw data: retail holds zeros.
            retail_bytes = bytearray(size)
        elif at is None:
            continue
        else:
            retail_bytes = bytearray(img.data[at:at + size])
        for site in sites:
            retail_bytes[site:site + 4] = b"\0\0\0\0"
        if bytes(retail_bytes) != masked or STORAGE.get(storage) != comdat_storage[sec["name"]]:
            withheld.append((rva, name, "retail bytes or storage contradict the COMDAT"))
            continue
        for stem, csec, _m, _r in copies:
            rows.append({"name": name, "object": f"{stem}.c", "rva": rva, "size": size,
                         "storage": STORAGE[storage], "alignment": csec["alignment"],
                         "section_placed": True,
                         "provenance": "candidate-COFF-comdat-paired"})
    return rows, withheld


def paired_data_votes(rows, wanted, base_dir=BASE_DIR):
    """{symbol: {rva}} from pointer words inside enrolled source data.

    A source definition's own COFF relocation at offset k names a ``wanted``
    symbol; retail's word at the definition's address plus k, less our
    addend, states where that symbol lives.
    """
    import struct

    img = retail()
    by_object = defaultdict(list)
    for r in rows:
        if r.get("provenance") == "src-DATA-sizeof":
            by_object[r["object"].removesuffix(".c")].append(r)
    votes: dict[str, set[int]] = defaultdict(set)
    for stem, c in coffx.objects(base_dir):
        wanted_rows = by_object.get(stem)
        if not wanted_rows:
            continue
        where = {}
        for idx, value, secnum in c.iter_symbols():
            if secnum > 0:
                where.setdefault(msvc_names.mask(c.sym_name(idx)), []).append((secnum, value))
        for r in wanted_rows:
            hits = where.get(msvc_names.mask(r["name"]), [])
            if len(hits) != 1:
                continue
            secnum, value = hits[0]
            payload = c.section_payload(secnum)
            rva, size = int(r["rva"]), int(r["size"])
            for site, (sym, typ) in c.typed_relocations(secnum).items():
                if typ != COFF_DIR32 or not wanted(sym) \
                        or not value <= site < value + size - 3 or not payload:
                    continue
                at = img.off(rva + site - value)
                if at is None:
                    continue
                addend = struct.unpack("<i", payload[site:site + 4])[0]
                votes[sym].add(struct.unpack("<I", img.data[at:at + 4])[0]
                               - img.image_base - addend)
    return votes


def string_rows(base_dir=BASE_DIR, *, nonliteral_ranges=(), votes=None):
    """Enrollable `??_C@` string-literal definitions + the withheld ones.

    Both facts are PROVEN: the retail RVA comes from content-matching each
    relocation-target datum's bytes against the candidate objs' `??_C@` pools
    (cl's own spelling for those exact bytes); the owner is the candidate obj
    that defines the literal. A payload emitted by SEVERAL units enrolls once
    PER OWNING UNIT (that is what a COMDAT is - the linker folded all of them
    onto one rva). Identical payloads at two retail RVAs collide on one
    content-derived name; both are withheld.
    """
    # Keyed by the literal's COMPLETE bytes: a literal with an embedded NUL
    # (for example "\0\1\1\2...") must not collide with "" or any other
    # literal sharing its leading C string.
    owners: dict[bytes, dict[str, str]] = defaultdict(dict)
    by_prefix: dict[bytes, set[bytes]] = defaultdict(set)
    for stem, c in coffx.objects(base_dir):
        for idx, value, secnum in c.iter_symbols():
            name = c.sym_name(idx)
            if name.startswith("??_C@") and secnum >= 1:
                sec = c.section_table[secnum - 1]
                raw = c.section_payload(secnum)
                if not raw and value == 0 and sec["size"]:
                    # cl omits the raw data of an all-zero COMDAT ("").
                    raw = bytes(sec["size"])
                if value != 0 or b"\0" not in raw:
                    continue
                whole = raw[:sec["size"]]
                cs = whole[:whole.index(b"\0")]
                owners[whole][stem] = name
                by_prefix[cs].add(whole)

    img = retail()
    rows, withheld, by_name = [], [], defaultdict(list)
    for rva in _reloc_data_rvas():
        # Typed DATA storage is not a ??_C allocation merely because its
        # bytes happen to form the same NUL-terminated sequence.
        if any(start <= rva < end for start, end in nonliteral_ranges):
            continue
        cs = img.cstring(rva)
        if cs is None:
            continue
        for whole in sorted(by_prefix.get(cs, ()), key=len):
            size = len(whole)
            at = img.off(rva)
            if at is None or img.data[at:at + size] != whole:
                continue
            units = owners[whole]
            start = _classify(rva)
            end = _classify(rva + size - 1)
            paired = any(rva in (votes or {}).get(n, ()) for n in units.values())
            if start == end == "data-unprovable-tail" and paired:
                # The zero tail cannot tell a literal from padding by its
                # bytes; paired relocations naming this literal can.
                start = end = "data-initialized"
            if start not in STORAGE or start != end:
                withheld.append((rva, next(iter(units.values())),
                                 f"string storage {start} not enrollable"))
                continue
            for unit, name in sorted(units.items()):
                by_name[name].append(
                    {"name": name, "object": f"{unit}.c", "rva": rva, "size": size,
                     "storage": STORAGE[start],
                     "provenance": "candidate-COFF-string"})
    for name, group in by_name.items():
        addrs = {r["rva"] for r in group}
        paired = (votes or {}).get(name, set())
        if len(addrs) == 1:
            rows += group
        elif len(paired) == 1 and paired <= addrs:
            # Content is ambiguous, but corroborated code relocations that
            # name this literal all read the same retail address.
            rows += [dict(r, provenance="candidate-COFF-string-paired")
                     for r in group if r["rva"] in paired]
        else:
            for r in group:
                withheld.append((r["rva"], name,
                                 f"identical payload at {len(addrs)} retail RVAs"))
    return rows, withheld


def vtable_rows(model: Model, base_dir=BASE_DIR):
    """Enrollable `??_7` vtable definitions + the withheld ones.

    A vtable is emitted exactly like a string literal - one COMDAT per symbol,
    folded by the linker - so it enrolls once per owning unit. Under `/GR` the
    `??_R4` COL pointer sits at COMDAT offset 0 and `??_7` at 4; our offset
    and retail's are INDEPENDENT facts, both checked (retail_col_head reads
    the shipped image, the candidate COMDAT says how WE compiled it). The
    extent is enrolled only where the reviewed table (data_vtables) and the
    candidate COMDAT agree: `offset + slots * 4 == candidate section size`.
    """
    tables = _vtable_tables(model)
    emitters: dict[str, dict[int, list]] = defaultdict(lambda: defaultdict(list))
    for stem, c in coffx.objects(base_dir):
        for sec in c.section_table:
            members = c.defined_symbols(sec["index"])
            if len(members) != 1:
                continue
            offset, name = members[0]
            if name.startswith("??_7"):
                emitters[name][offset].append((stem, sec))

    rows, withheld = [], []
    for name, by_offset in sorted(emitters.items()):
        hit = tables.get(name)
        if hit is None:
            withheld.append((0, name, "no reviewed vtable extent for this name"))
            continue
        rva, slots = hit
        head = retail_col_head(rva)
        for offset, group in sorted(by_offset.items()):
            if offset > head:
                withheld.append((rva, name,
                                 "candidate COMDAT opens with a ??_R4 COL word "
                                 "that retail's vtable does not have "
                                 f"({len(group)} obj(s): "
                                 f"{', '.join(u for u, _ in group[:4])})"))
                continue
            for unit, sec in group:
                if sec["size"] != offset + slots * 4:
                    withheld.append((rva, name,
                                     f"candidate section 0x{sec['size']:x} != "
                                     f"0x{offset:x} + reviewed {slots} slots"))
                    continue
                rows.append({"name": name, "object": f"{unit}.c", "rva": rva,
                             "size": slots * 4, "vtable_offset": offset,
                             "section_placed": offset == head,
                             "storage": "rdata",
                             "alignment": member_alignment(sec["alignment"],
                                                           offset, rva),
                             "provenance": "candidate-COFF-vtable"})
    return rows, withheld


#: MSVC's RTTI records: `??_R4` COL (+0xc pTypeDescriptor, +0x10 pCHD),
#: `??_R3` CHD (+0x8 numBaseClasses, +0xc pBaseClassArray), `??_R2` array of
#: `??_R1` pointers, `??_R1` BCD (+0 pTypeDescriptor), `??_R0` type desc.
COL_TYPE_DESC, COL_HIERARCHY = 0x0C, 0x10
CHD_NUM_BASES, CHD_BASE_ARRAY = 0x08, 0x0C


def rtti_rows(model: Model, base_dir=BASE_DIR):
    """Enrollable `??_R*` definitions + the withheld ones.

    The NAMES are read off cl's own relocations: the retail image gives the
    ADDRESSES (`vtable-4` -> COL -> hierarchy -> base-class array ->
    descriptors -> type descriptors) and the base obj that emitted the same
    vtable gives the NAMES at the identical offsets. Every node is then proven
    byte-for-byte against the candidate COMDAT (relocated dwords masked); a
    node that fails, or whose name the walk reaches at two addresses, is
    withheld.
    """
    img = retail()
    defs: dict[str, dict[str, dict]] = defaultdict(dict)
    anchors: dict[str, dict[str, str]] = defaultdict(dict)
    for stem, c in coffx.objects(base_dir):
        for sec in c.section_table:
            members = c.defined_symbols(sec["index"])
            if len(members) != 1:
                continue
            offset, name = members[0]
            if name.startswith("??_R"):
                defs[name][stem] = {
                    "sec": sec, "payload": c.section_payload(sec["index"]),
                    "rel": c.relocations(sec["index"])}
            elif name.startswith("??_7") and offset == 4:
                head = c.relocations(sec["index"]).get(0)
                if head and head.startswith("??_R4"):
                    anchors[name][stem] = head

    vtables = {name: rva
               for name, (rva, _slots) in _vtable_tables(model).items()}
    located: dict[str, int] = {}
    withheld: list[tuple] = []

    def place(name, rva):
        prev = located.get(name)
        if prev is None:
            located[name] = rva
            return True
        if prev != rva:
            withheld.append((rva, name, "RTTI walk reaches one name at two RVAs"))
            return False
        return True

    for vtable, units in sorted(anchors.items()):
        rva = vtables.get(vtable)
        if rva is None or retail_col_head(rva) != 4:
            continue                    # no reviewed extent, or retail not /GR
        col = img.u32(rva - 4) - img.image_base
        unit = sorted(units)[0]

        def shape(name):
            group = defs.get(name)
            if not group:
                return None
            return group.get(unit) or group[sorted(group)[0]]

        chain, ok = [], True
        node = shape(units[unit])
        if node is None:
            withheld.append((col, units[unit], "no candidate COMDAT for the COL"))
            continue
        chain.append((units[unit], col))
        r3name = node["rel"].get(COL_HIERARCHY)
        for field in (COL_TYPE_DESC, COL_HIERARCHY):
            nm, ptr = node["rel"].get(field), img.u32(col + field)
            if nm is None or ptr is None:
                ok = False
                break
            chain.append((nm, ptr - img.image_base))
        if not ok or r3name is None:
            withheld.append((col, units[unit], "COL record is not walkable"))
            continue
        chd, r3 = shape(r3name), img.u32(col + COL_HIERARCHY) - img.image_base
        r2name = chd["rel"].get(CHD_BASE_ARRAY) if chd else None
        bases = img.u32(r3 + CHD_NUM_BASES)
        r2 = img.u32(r3 + CHD_BASE_ARRAY)
        if r2name is None or bases is None or r2 is None:
            withheld.append((r3, r3name or "??_R3?",
                             "hierarchy record is not walkable"))
            continue
        r2 -= img.image_base
        chain.append((r2name, r2))
        array = shape(r2name)
        for i in range(bases):
            r1name = array["rel"].get(i * 4) if array else None
            r1 = img.u32(r2 + i * 4)
            if r1name is None or r1 is None:
                ok = False
                break
            r1 -= img.image_base
            chain.append((r1name, r1))
            desc = shape(r1name)
            r0name = desc["rel"].get(0) if desc else None
            r0 = img.u32(r1)
            if r0name is None or r0 is None:
                ok = False
                break
            chain.append((r0name, r0 - img.image_base))
        if not ok:
            withheld.append((r2, r2name, "base-class array is not walkable"))
            continue
        for name, addr in chain:
            place(name, addr)

    rows = []
    for name, rva in sorted(located.items()):
        group = defs.get(name) or {}
        sizes = {d["sec"]["size"] for d in group.values()}
        if len(sizes) != 1:
            withheld.append((rva, name, "candidate COMDATs disagree on the extent"))
            continue
        size = sizes.pop()
        off = img.off(rva)
        if off is None:
            withheld.append((rva, name, "RTTI record is not mapped in the image"))
            continue
        sample = next(iter(group.values()))
        want = bytearray(sample["payload"][:size])
        got = bytearray(img.data[off:off + size])
        if len(want) != size or len(got) != size:
            withheld.append((rva, name, "RTTI record is truncated"))
            continue
        for site in sample["rel"]:      # the pointers differ by construction
            want[site:site + 4] = got[site:site + 4] = b"\0\0\0\0"
        if want != got:
            withheld.append((rva, name, "retail bytes contradict the candidate record"))
            continue
        storage = _classify(rva)
        if storage not in STORAGE:
            withheld.append((rva, name, f"RTTI storage {storage} is not enrollable"))
            continue
        for unit, d in sorted(group.items()):
            rows.append({"name": name, "object": f"{unit}.c", "rva": rva,
                         "size": size, "storage": STORAGE[storage],
                         "alignment": d["sec"]["alignment"],
                         "section_placed": True,
                         "provenance": "candidate-COFF-rtti"})
    return rows, withheld


def ehfuncinfo_rows(model: Model):
    """The `.xdata$x` blob of every carved /GX EH group -> per-owner rows.

    Two rows per group - the FuncInfo record and its unwind map (the record's
    `pUnwindMap` word RELOCATES to the map, so one 40-byte row would leave a
    self-reference with an addend). Extents are read out of the record, never
    assumed (eh_band proves them)."""
    from homm3.delink.pdb_synth import unit_names
    # VC6 can place actions next to the owning body, separately from its
    # registration stub. Data records do not imply a contiguous code band.
    band = eh_band.groups(retail().pe.path, unit_names(model), contiguous=False)
    rows, withheld = [], []
    for group in band:
        if not group.funcinfo_size:
            withheld.append((group.funcinfo, eh_band.funcinfo_symbol(group.owner),
                             "FuncInfo record does not prove its own extent"))
    for rva, name, unit, size in eh_band.data_records(band):
        start = _classify(rva)
        end = _classify(rva + size - 1)
        if start not in STORAGE or start != end:
            withheld.append((rva, name,
                             f"EH funcinfo storage {start} not enrollable"))
            continue
        rows.append({"name": name, "object": f"{unit}.c", "rva": rva,
                     "size": size, "storage": STORAGE[start],
                     "provenance": "retail-EH-funcinfo"})
    return rows, withheld


def fp_pool_rows(model: Model, base_dir=BASE_DIR):
    """`$T<n>` FP-pool constants, ADDRESSED OUT OF RETAIL'S OWN RELOC TABLE.

    Content matching cannot answer this (FP pools are not /Gf-pooled, and a
    content-derived address is self-confirming). Instead, retail's DIR32 sites
    inside each claimed function pair positionally with the base obj's COFF
    relocations; the pairing PROVES ITSELF (every known symbol's rva must
    equal the address retail wrote, plus the addend in our own bytes), and
    only then is a `$T` site read off and byte-re-proven. The manifest name is
    `$T<decimal rva>` (cl's counter is volatile and N of our units may share
    one retail slot); each row carries `member`, cl's real per-object symbol,
    for ordinary_sections' name matching. A `$T<rva>` pin - a DATA_COMPGEN use
    site (src_data_compgen) or a reviewed data_compgen row - bridges to a
    still-unaddressed member when extent and bytes agree.
    """
    import struct

    img = retail()
    sites = img.reloc_sites

    # both maps are keyed by the MASKED name so a base obj's own relocation
    # symbol (cl's CodeView counter intact) meets the Model's canonical one.
    from homm3.delink.pdb_synth import UNIT_CHANNELS
    known, fn_extent = {}, {}
    for b in model.functions:
        if b.channel in UNIT_CHANNELS and b.name:
            known[msvc_names.mask(b.name)] = b.rva
            fn_extent[msvc_names.mask(b.name)] = (b.rva, b.size)
    pins: dict[str, list[tuple[int, int]]] = defaultdict(list)
    for b in model.data:
        if not b.channel or not b.name:
            continue
        known.setdefault(msvc_names.mask(b.name), b.rva)
        if b.channel in ("data_compgen", "src_data_compgen") \
                and FP_POOL_NAME.fullmatch(b.name):
            pins[b.unit].append((b.rva, b.size))

    rows, withheld = [], []
    for stem, c in coffx.objects(base_dir):
        pool = {}                       # member -> (storage, off, payload, size)
        for sec in c.section_table:
            storage = ORDINARY_STORAGE.get(sec["name"])
            if storage is None or sec["characteristics"] & LNK_COMDAT:
                continue
            members = c.section_members(sec["index"])
            offsets = sorted(o for o, _n, _s in members)
            payload = c.section_payload(sec["index"])[:sec["size"]]
            for off, name, _scl in members:
                if not FP_POOL_NAME.fullmatch(name):
                    continue
                end = next((o for o in offsets if o > off), sec["size"])
                pool[name] = (storage, off, payload[off:end], end - off)
        if not pool:
            continue

        votes: dict[str, set[int]] = defaultdict(set)
        for sec in c.section_table:
            if not sec["characteristics"] & MEM_EXECUTE:
                continue
            rel = {site: nm for site, (nm, typ)
                   in c.typed_relocations(sec["index"]).items()
                   if typ == COFF_DIR32}
            if not rel:
                continue
            text = c.section_payload(sec["index"])
            for off, name in c.defined_symbols(sec["index"]):
                hit = fn_extent.get(msvc_names.mask(name))
                if hit is None:
                    continue
                rva, size = hit
                mine = sorted((s, n) for s, n in rel.items()
                              if off <= s < off + size)
                lo = bisect.bisect_left(sites, rva)
                hi = bisect.bisect_left(sites, rva + size)
                theirs = sites[lo:hi]
                if not mine or len(mine) != len(theirs):
                    continue
                found, corroborated = [], True
                for (site, sym), target in zip(mine, theirs):
                    at = img.off(target)
                    if at is None:
                        corroborated = False
                        break
                    value = struct.unpack("<I", img.data[at:at + 4])[0] \
                        - img.image_base
                    if FP_POOL_NAME.fullmatch(sym):
                        found.append((sym, value))
                        continue
                    anchor = known.get(msvc_names.mask(sym))
                    if anchor is None:      # not ours to check
                        continue
                    addend = struct.unpack("<I", text[site:site + 4])[0]
                    if value != anchor + addend:
                        corroborated = False
                        break
                if corroborated:
                    for sym, value in found:
                        votes[sym].add(value)

        def emit(member, rva, storage, size, want, how):
            at = img.off(rva)
            if at is None or img.data[at:at + size] != want:
                withheld.append((rva, member,
                                 "retail bytes contradict the candidate FP constant"))
                return
            start = _classify(rva)
            end = _classify(rva + size - 1)
            if STORAGE.get(start) != storage or start != end:
                withheld.append((rva, member,
                                 f"FP-pool storage {start} is not {storage}"))
                return
            rows.append({"name": f"$T{rva}", "member": member,
                         "object": f"{stem}.c", "rva": rva, "size": size,
                         "storage": storage, "provenance": how})

        stranded = []
        for member in sorted(pool):
            storage, _off, want, size = pool[member]
            seen = votes.get(member) or set()
            if len(seen) == 1:
                emit(member, next(iter(seen)), storage, size, want,
                     "retail-reloc-fp-pool")
            elif seen:
                withheld.append((0, member, "referrers disagree on the rva"))
            else:
                stranded.append(member)

        taken = {r["rva"] for r in rows}
        pairs: dict[int, list[str]] = defaultdict(list)
        for rva, size in pins.get(stem, ()):
            if rva in taken:
                continue
            at = img.off(rva)
            if at is None:
                continue
            want = img.data[at:at + size]
            for member in stranded:
                # The pin states the LITERAL's size, the member its padded
                # slot extent; accept the prefix match (bytes still verified,
                # the enrolled extent stays the obj's).
                if pool[member][3] >= size and pool[member][2][:size] == want:
                    pairs[rva].append(member)
        claims = Counter(m for ms in pairs.values() for m in ms)
        for rva, ms in sorted(pairs.items()):
            if len(ms) != 1 or claims[ms[0]] != 1:
                withheld.append((rva, ms[0] if ms else "$T?",
                                 f"DATA_COMPGEN pin matches {len(ms)} pool members"))
                continue
            storage, _off, want, size = pool[ms[0]]
            emit(ms[0], rva, storage, size, want, "src-DATA_COMPGEN-fp-pool")
        for member in stranded:
            if not claims.get(member):
                withheld.append((0, member, "no relocation-paired referrer"))

    # Copies of one slot must agree on the extent; one object claims it once.
    by_rva = defaultdict(list)
    for r in rows:
        by_rva[r["rva"]].append(r)
    kept = []
    for rva, group in sorted(by_rva.items()):
        if len({r["size"] for r in group}) != 1:
            withheld += [(rva, r["member"], "FP-pool copies disagree on the extent")
                         for r in group]
            continue
        by_object = {}
        for r in group:
            by_object.setdefault(r["object"], r)
        kept += list(by_object.values())
    return kept, withheld


# --- assembly ----------------------------------------------------------------

def candidates(model: Model):
    """Enrollable rows + the withheld ones (with a reason each) + overlaps."""
    tail_oracle = _candidate_member_storage()
    rows, withheld, skipped = claim_rows(model, tail_oracle)

    types = declared_types()
    literal = lambda name: name.startswith("??_C@")  # noqa: E731
    votes = paired_code_votes(model, literal)
    # Source data pointers are exact offsets; where they state one address,
    # they take precedence over code pairings for the same literal.
    for name, rvas in paired_data_votes(rows, literal).items():
        if len(rvas) == 1:
            votes[name] = rvas
    strings, w = string_rows(nonliteral_ranges=nonliteral_storage(model, types),
                             votes=votes)
    rows += strings
    withheld += w
    vtables, w = vtable_rows(model)
    rows += vtables
    withheld += w
    rtti, w = rtti_rows(model)
    rows += rtti
    withheld += w
    from homm3.delink.exception_data import rows as exception_rows
    exceptions, w = exception_rows(model, BASE_DIR, existing=rtti)
    rows += exceptions
    withheld += w
    ehfi, w = ehfuncinfo_rows(model)
    rows += ehfi
    withheld += w
    comdat, w = paired_comdat_rows(model)
    rows += comdat
    withheld += w

    # cl's `$T` FP pool. A slot some OTHER channel already names is left to
    # that channel: two names at one rva withhold BOTH, so a pool row must
    # never displace an enrolled row.
    fp, w = fp_pool_rows(model)
    withheld += w
    spoken_for = {r["rva"]: r["name"] for r in rows}
    for r in fp:
        other = spoken_for.get(r["rva"])
        if other in (None, r["name"]):
            rows.append(r)
        else:
            withheld.append((r["rva"], r["member"],
                             f"the pool slot is already named {other}"))

    # One definition stated by several provenances (a claim row and the
    # auto-inferred candidate-COFF string/vtable are the SAME datum) collapses,
    # preferring the candidate-COFF statement: those take the candidate SECTION
    # shape, and a fold must not mix section-form and legacy-form rows.
    seen, deduped = {}, []
    for r in rows:
        key = (r["name"], r["object"], r["rva"], r["size"])
        prev = seen.get(key)
        if prev is None:
            seen[key] = r
            deduped.append(r)
        elif (not prev["provenance"].startswith("candidate-COFF")
                and r["provenance"].startswith("candidate-COFF")):
            prev.update(r)
        elif r.get("member") and "member" not in prev:
            prev["member"] = r["member"]
    rows = deduped

    # A NAMED STATIC THAT /Gf FOLDED ONTO A POOLED LITERAL IS NOT AN OVERLAP:
    # both claims are true and the extents are EXACTLY equal. The pooled
    # literal keeps the authoritative claim; the static is re-provenanced
    # `provisional-` (carved, never owning the address). BUT an alias is only
    # a fold when both sides live in the same storage - a `.rdata` candidate
    # pinned onto a `.data` literal is a mis-modelled declaration, refuted.
    candidate_storage = _candidate_member_storage()
    alias_of, refuted = {}, set()
    by_extent = defaultdict(list)
    for r in rows:
        by_extent[(r["rva"], r["size"])].append(r)
    for (rva, _size), group in by_extent.items():
        names = {r["name"] for r in group}
        pooled = {n for n in names if n.startswith("??_C@")}
        if len(names) != 2 or len(pooled) != 1:
            continue
        literal = next(iter(pooled))
        for r in group:
            if r["name"] == literal:
                continue
            mine = candidate_storage.get(
                (r["object"], msvc_names.mask(r["name"])))
            if mine != r["storage"]:
                refuted.add(r["name"])
                withheld.append((rva, r["name"],
                                 f"our {mine or '?'} copy cannot be the "
                                 f"{r['storage']} literal {literal} it is "
                                 "pinned onto"))
                continue
            r["provenance"] = "provisional-pooled-literal-alias"
            alias_of[r["name"]] = literal
    rows = [r for r in rows if r["name"] not in refuted]

    # Collapse each fold to one extent, then the neighbour check: an overlap
    # proves one of the pair is mis-modelled but not which - neither enrolls.
    rows, aliases = ([r for r in rows if r["name"] not in alias_of],
                     [r for r in rows if r["name"] in alias_of])
    # One FP-pool slot, two channels: the `$T<rva>` pin (literal size) nested
    # inside the pool extent is one claim, not an overlap - keep the pool row.
    pool_ext = {(r["rva"], r["name"]): r["size"] for r in rows
                if "fp-pool" in (r.get("provenance") or "")}
    rows = [r for r in rows
            if "fp-pool" in (r.get("provenance") or "")
            or r["size"] >= pool_ext.get((r["rva"], r["name"]), 0)]
    rows.sort(key=lambda x: (x["rva"], x["size"], x["name"]))
    extents = []
    for r in rows:
        if extents and (extents[-1]["rva"], extents[-1]["size"],
                        extents[-1]["name"]) == (r["rva"], r["size"], r["name"]):
            extents[-1]["copies"].append(r)
        else:
            extents.append({"rva": r["rva"], "size": r["size"],
                            "name": r["name"], "copies": [r]})
    bad, overlaps = set(), []
    active = []
    for i, b in enumerate(extents):
        active = [(j, a) for j, a in active
                  if a["rva"] + a["size"] > b["rva"]]
        for j, a in active:
            bad.update((j, i))
            overlaps.append((a, b, min(a["rva"] + a["size"],
                                       b["rva"] + b["size"]) - b["rva"]))
        active.append((i, b))
    enrolled = [r for i, e in enumerate(extents) if i not in bad
                for r in e["copies"]]
    for i in sorted(bad):
        withheld.append((extents[i]["rva"], extents[i]["name"],
                         "overlaps a neighbour"))
    # The delinker admits N objects claiming one rva under one name (a fold)
    # and nothing looser.
    extent_of, name_at = defaultdict(set), defaultdict(set)
    for r in enrolled:
        extent_of[r["name"]].add((r["rva"], r["size"]))
        name_at[r["rva"]].add(r["name"])
    per_object = Counter((r["name"], r["object"]) for r in enrolled)
    final = []
    for r in enrolled:
        if len(extent_of[r["name"]]) > 1:
            withheld.append((r["rva"], r["name"], "duplicate name in manifest"))
        elif len(name_at[r["rva"]]) > 1:
            withheld.append((r["rva"], r["name"], "duplicate rva in manifest"))
        elif per_object[(r["name"], r["object"])] > 1:
            withheld.append((r["rva"], r["name"],
                             "duplicate definition in one object"))
        else:
            final.append(r)
    # A pooled-literal alias enrolls only if the literal it aliases did.
    live = {r["name"] for r in final}
    for r in aliases:
        if alias_of[r["name"]] in live:
            final.append(r)
        else:
            withheld.append((r["rva"], r["name"],
                             "aliases a withheld pooled literal"))
    return final, withheld, overlaps, skipped


def ordinary_sections(rows, base_dir=BASE_DIR):
    """Non-affine candidate sections for cl's ordinary, non-COMDAT
    `.data`/`.rdata` - admitted only when PROVABLY COMPLETE: every member has
    an enrolled unplaced definition of the matching storage, no overlaps, no
    overruns, and every uncovered byte is ZERO in the candidate payload."""
    # keyed by the MASKED name: cl's per-object CodeView counter on a TU-local
    # static is exactly the number the enrolled spelling drops.
    by_obj: dict[str, dict[str, dict]] = defaultdict(dict)
    for r in rows:
        by_obj[r["object"]][msvc_names.mask(r.get("member") or r["name"])] = r

    secs, placed = [], []
    for obj, named in sorted(by_obj.items()):
        path = Path(base_dir) / (obj[:-2] + ".obj")     # "foo.c" -> foo.obj
        if not path.exists():
            continue
        c = coffx.Obj(path)
        for sec in c.section_table:
            storage = ORDINARY_STORAGE.get(sec["name"])
            if storage is None or sec["characteristics"] & LNK_COMDAT:
                continue
            members = c.section_members(sec["index"])
            if not members:
                continue
            payload = c.section_payload(sec["index"])[:sec["size"]]
            covered, mine, complete = bytearray(sec["size"]), [], True
            for offset, name, _scl in members:
                r = named.get(msvc_names.mask(name))
                if r is None or r["storage"] != storage or "section" in r:
                    complete = False
                    break
                end = offset + r["size"]
                if end > sec["size"] or any(covered[offset:end]):
                    complete = False
                    break
                covered[offset:end] = b"\1" * r["size"]
                mine.append((r, offset))
            if not complete or len(payload) != sec["size"]:
                continue
            if any(payload[i] for i in range(sec["size"]) if not covered[i]):
                continue
            for r, offset in mine:
                r["section"] = sec
                r["section_offset"] = offset
            placed += [r for r, _o in mine]
            secs.append({"object": obj, "index": sec["index"],
                         "name": sec["name"], "rva": None, "size": sec["size"],
                         "alignment": sec["alignment"],
                         "characteristics": sec["characteristics"],
                         "comdat": sec["comdat"], "assoc": sec["assoc"],
                         "storage": storage,
                         "provenance": "candidate-COFF-ordinary-nonaffine"})
    return secs, placed


def section_rows(rows, base_dir=BASE_DIR):
    """Candidate COMDAT sections for the enrolled literals/vtables/RTTI + the
    withheld. A row that cannot claim a retail range still gets a NON-AFFINE
    section (`rva = -`): the candidate COFF shape without a range claim, which
    the delinker copies/relocates from the definition's own rva. The ordinary
    non-COMDAT `.data`/`.rdata` get the same treatment via ordinary_sections.
    """
    secs, withheld = [], []
    by_obj: dict[str, list] = {}
    for r in rows:
        if r.get("provenance") in ("candidate-COFF-string",
                                   "candidate-COFF-vtable",
                                   "candidate-COFF-rtti",
                                   "candidate-COFF-exception-exact"):
            by_obj.setdefault(r["object"], []).append(r)

    for obj, rs in sorted(by_obj.items()):
        path = Path(base_dir) / (obj[:-2] + ".obj")
        if not path.exists():
            withheld += [(r["rva"], r["name"], f"no candidate obj {path.name}")
                         for r in rs]
            continue
        c = coffx.Obj(path)
        owner = {}
        for sec in c.section_table:
            members = c.defined_symbols(sec["index"])
            if len(members) != 1:
                continue
            offset, name = members[0]
            if name.startswith(("??_C@", "??_R", "__CT", "__TI")) \
                    and offset == 0:
                owner[name] = (sec, 0)
            elif name.startswith("??_7"):
                owner[name] = (sec, offset)
        for r in rs:
            hit = owner.get(r["name"])
            if hit is None:
                withheld.append((r["rva"], r["name"],
                                 "no single-symbol COMDAT in the candidate obj"))
                continue
            sec, offset = hit
            group = sec['name'].split('$', 1)[0]
            candidate_storage = {'.data': 'data', '.rdata': 'rdata',
                                 '.xdata': 'rdata', '.bss': 'bss'}.get(group)
            if candidate_storage != r['storage']:
                withheld.append((r['rva'], r['name'],
                                 'candidate section storage differs from retail'))
                continue
            if offset != r.get("vtable_offset", 0):
                withheld.append((r["rva"], r["name"],
                                 f"candidate COMDAT offset 0x{offset:x} != "
                                 f"enrolled 0x{r.get('vtable_offset', 0):x}"))
                continue
            if sec["size"] != offset + r["size"]:
                withheld.append((r["rva"], r["name"],
                                 f"candidate section 0x{sec['size']:x} != "
                                 f"0x{offset:x} + retail extent 0x{r['size']:x}"))
                continue
            r["section"] = sec
            r["section_offset"] = offset
            placed_here = r.get("section_placed", True)
            secs.append({"object": obj, "index": sec["index"],
                         "name": sec["name"],
                         "rva": r["rva"] - offset if placed_here else None,
                         "size": sec["size"], "alignment": sec["alignment"],
                         "characteristics": sec["characteristics"],
                         "comdat": sec["comdat"], "assoc": sec["assoc"],
                         "storage": r["storage"],
                         "provenance": "candidate-COFF-section" if placed_here
                         else "candidate-COFF-section-nonaffine"})

    # Two placed sections may not claim overlapping retail bytes unless they
    # are the SAME folded COMDAT seen from two objects. The delinker BAILS on
    # a violation, so screen here and withhold the placement instead.
    placed = sorted((s for s in secs if s["rva"] is not None),
                    key=lambda s: (s["rva"], s["size"], s["object"], s["index"]))
    conflicted = set()
    active = []
    for second in placed:
        active = [first for first in active if first['rva'] + first['size'] > second['rva']]
        for first in active:
            alias = (first["object"] != second["object"]
                     and (first["rva"], first["size"], first["name"],
                          first["alignment"], first["characteristics"],
                          first["comdat"])
                     == (second["rva"], second["size"], second["name"],
                         second["alignment"], second["characteristics"],
                         second["comdat"]))
            if not alias:
                conflicted.add((first["rva"], first["size"]))
                conflicted.add((second["rva"], second["size"]))
        active.append(second)
    if conflicted:
        secs = [s for s in secs
                if s["rva"] is None or (s["rva"], s["size"]) not in conflicted]
        for obj, rs in by_obj.items():
            for r in rs:
                if "section" not in r or not r.get("section_placed", True):
                    continue
                key = (r["rva"] - r["section_offset"], r["section"]["size"])
                if key in conflicted:
                    withheld.append((r["rva"], r["name"],
                                     "candidate section 0x%x+0x%x overlaps "
                                     "another object's" % key))
                    del r["section"], r["section_offset"]

    ordinary, ordinary_rows = ordinary_sections(rows, base_dir)
    secs += ordinary
    for r in ordinary_rows:
        by_obj.setdefault(r["object"], []).append(r)

    # Ordinals are per-object, contiguous from one, in the candidate COFF's
    # own section order (objdiff stable-sorts same-named sections when
    # combining, so order decides the combined layout on both sides).
    for obj in {s["object"] for s in secs}:
        mine = sorted([s for s in secs if s["object"] == obj],
                      key=lambda s: s["index"])
        remap = {s["index"]: i for i, s in enumerate(mine, 1)}
        for s in mine:
            s["ordinal"] = remap[s["index"]]
        for r in by_obj.get(obj, []):
            if "section" in r:
                r["section_ordinal"] = remap[r["section"]["index"]]
    secs.sort(key=lambda s: (s["object"], s["ordinal"]))
    return secs, withheld


def gap_rows(enrolled, secs):
    """Band-completion rows: retail bytes strictly between two claims of ONE
    unit, carved with no base counterpart so a datum src/ never models becomes
    a visible per-unit diff instead of silence. Fail-closed: single-owner
    witness pair required, nonzero payload (or above the next claim's
    alignment), capped, `provisional-` (never owns the address); the sum is
    asserted."""
    img = retail()

    claims = [r for r in enrolled
              if not str(r.get("provenance", "")).startswith("provisional-")]
    sec_claims = [s for s in secs if s.get("rva") is not None]
    owners = defaultdict(set)
    for w in claims + sec_claims:
        owners[w["name"]].add(w["object"])

    starts, ends = defaultdict(list), defaultdict(list)
    intervals = set()
    for w in claims + sec_claims:
        starts[w["rva"]].append(w)
        ends[w["rva"] + w["size"]].append(w)
        intervals.add((w["rva"], w["size"]))

    merged = []
    for a, sz in sorted(intervals):
        b = a + sz
        if merged and a <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], b)
        else:
            merged.append([a, b])

    rows, withheld, considered = [], [], 0
    for (_, b1), (a2, _) in zip(merged, merged[1:]):
        considered += 1
        n = a2 - b1
        name = f"$gap_{b1:06x}"
        cls1 = _classify(b1)
        cls2 = _classify(a2 - 1)
        if cls1 != cls2 or STORAGE.get(cls1) not in ("rdata", "data", "bss"):
            withheld.append((b1, name, "band gap outside enrollable storage "
                             f"({cls1} -> {cls2})"))
            continue
        strong_prev = {w["object"] for w in ends.get(b1, ())
                       if len(owners[w["name"]]) == 1}
        strong_next = {w["object"] for w in starts.get(a2, ())
                       if len(owners[w["name"]]) == 1}
        both = strong_prev & strong_next
        if len(both) != 1:
            withheld.append((b1, name, "band gap unowned (no single-unit "
                             f"single-owner witness pair, 0x{n:x} B)"))
            continue
        unit = next(iter(both))
        is_bss = STORAGE[cls1] == "bss"
        pay = b"" if is_bss else img.payload(b1, n)
        next_align = max((w.get("alignment")
                          or _alignment(w["rva"], w["size"],
                                        w.get("storage", "data"))[0]
                          for w in starts.get(a2, ())), default=0)
        if not is_bss and not any(pay):
            # A hole strictly smaller than the next claim's alignment exists
            # BECAUSE of that alignment and is slack; a bigger all-zero hole
            # is a real zero-valued datum src never modelled - carve it.
            if n < max(next_align, 4):
                withheld.append((b1, name, "band gap below the next claim's "
                                 f"alignment (slack; 0x{n:x} B, unit {unit})"))
                continue
        if is_bss and n < max(next_align, 4):
            withheld.append((b1, name, "band gap below the next claim's "
                             f"alignment (slack; 0x{n:x} B, unit {unit})"))
            continue
        cap = GAP_CAP_BSS if is_bss else GAP_CAP
        if n > cap:
            withheld.append((b1, name, "band gap over cap "
                             f"(0x{n:x} > 0x{cap:x}, unit {unit})"))
            continue
        kind = ("bss" if is_bss
                else "pointer" if img.relocs_in(b1, b1 + n)
                else "nonzero" if any(pay) else "zero")
        rows.append({"name": name, "object": unit, "rva": b1, "size": n,
                     "storage": STORAGE[cls1],
                     "provenance": "provisional-band-gap-" + kind})

    assert len(rows) + len(withheld) == considered, \
        "band-gap census dropped a gap silently"
    for r in rows:                       # never claim an enrolled byte
        for a, b in merged:
            assert r["rva"] >= b or r["rva"] + r["size"] <= a, (r, (a, b))
    return rows, withheld


# --- serialization -----------------------------------------------------------

def manifest_bytes(rows, refuted=None) -> bytes:
    """The --data-manifest. A row placed in a candidate section carries its
    (section_ordinal, section_offset); the rest keep the legacy `-` form. A
    placed row's alignment is cl's own symbol alignment; only a legacy row
    needs the c2 rule modelled (`refuted` collects rows whose alignment had to
    be lowered for placement - not source defects)."""
    out = ["\t".join(HEADER)]
    types = declared_types()
    for r in rows:
        placed = "section_ordinal" in r
        if placed:
            align = r.get("alignment", r["section"]["alignment"])
        else:
            kind = _object_kind(r["name"], r["rva"], r["size"], types)
            align, modelled = _alignment(r["rva"], r["size"], kind)
            if align != modelled and refuted is not None:
                refuted.append((r["rva"], r["name"], r["size"], kind,
                                modelled, align))
        out.append("\t".join([
            r["name"], r["object"], "0x%x" % r["rva"], "0x%x" % r["size"],
            r["storage"], "0x%x" % align,
            str(r["section_ordinal"]) if placed else "-",
            "0x%x" % r["section_offset"] if placed else "-",
            "external", r.get("provenance", "src-DATA-sizeof")]))
    return ("\n".join(out) + "\n").encode("utf-8")


def section_manifest_bytes(secs) -> bytes:
    """The --data-section-manifest. `rva = -` is a NON-AFFINE section."""
    out = ["\t".join(SECTION_HEADER)]
    for s in secs:
        out.append("\t".join([
            s["object"], str(s["ordinal"]), s["name"],
            "0x%x" % s["rva"] if s["rva"] is not None else "-",
            "0x%x" % s["size"], "0x%x" % s["alignment"],
            "0x%x" % s["characteristics"], str(s["comdat"]),
            str(s["assoc"]) if s["assoc"] else "-", s["storage"],
            s["provenance"]]))
    return ("\n".join(out) + "\n").encode("utf-8")


def _write_if_changed(path: Path, data: bytes) -> bool:
    path = Path(path)
    if path.is_file() and path.read_bytes() == data:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    return True


def generate(model: Model, output: Path = OUTPUT,
             section_output: Path = SECTION_OUTPUT, report: bool = False):
    """Build + write both manifests; returns (enrolled, secs, withheld)."""
    enrolled, withheld, overlaps, skipped = candidates(model)
    secs, sec_withheld = section_rows(enrolled)
    withheld += sec_withheld
    gaps, gap_withheld = gap_rows(enrolled, secs)
    enrolled += gaps
    withheld += gap_withheld
    print(f"[data-manifest] band-completion: {len(gaps)} gap row(s), "
          f"{sum(r['size'] for r in gaps)} B carved with no base counterpart; "
          f"{len(gap_withheld)} gap(s) withheld")
    refuted: list = []
    _write_if_changed(output, manifest_bytes(enrolled, refuted))
    _write_if_changed(section_output, section_manifest_bytes(secs))
    folds = len(enrolled) - len({(r["name"], r["rva"]) for r in enrolled})
    print(f"[data-manifest] enrolled {len(enrolled)} row(s) "
          f"({folds} folded-COMDAT copies) -> {output}")
    print("[data-manifest] storage: " + ", ".join(
        f"{k}={v}" for k, v in sorted(
            Counter(r["storage"] for r in enrolled).items())))
    print(f"[data-manifest] {sum(1 for r in enrolled if 'section_ordinal' in r)}"
          f" row(s) placed in {len(secs)} candidate section(s)"
          f" -> {section_output}")
    print(f"[data-manifest] withheld {len(withheld)} (never guessed): " + ", ".join(
        f"{k}={v}" for k, v in sorted(Counter(
            w[2].split("(")[0].strip() for w in withheld).items())))
    if skipped:
        print("[data-manifest] skipped: " + ", ".join(
            f"{k}={v}" for k, v in sorted(skipped.items())))
    if refuted:
        print(f"[data-manifest] {len(refuted)} legacy row(s) use lowered "
              "placement alignment (not a source refutation)")
    if report:
        print("\n--- overlap contradictions (a mis-modelling worklist) ---")
        for a, b, by in overlaps:
            print(f"  0x{a['rva']:06x} {a['name'][:42]:<42} +0x{a['size']:<4x} "
                  f"overlaps 0x{b['rva']:06x} {b['name'][:38]} by 0x{by:x}")
        print("\n--- withheld (no proven extent) ---")
        for rva, name, why in withheld[:20]:
            print(f"  0x{rva:06x} {name[:46]:<46} {why}")
    return enrolled, secs, withheld


def main(argv=None) -> int:
    import argparse
    from homm3.model import resolve
    ap = argparse.ArgumentParser(
        prog="python3 -m homm3.delink.data_manifest", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-o", "--output", type=Path, default=OUTPUT)
    ap.add_argument("--section-output", type=Path, default=SECTION_OUTPUT)
    ap.add_argument("--report", action="store_true",
                    help="print withheld + overlaps")
    args = ap.parse_args(argv)
    generate(resolve(), args.output, args.section_output, args.report)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
