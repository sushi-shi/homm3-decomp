"""homm3 placements - where the shared units' functions sit in another image.

    homm3 --image h3maped placements [--write] [--check]

A unit the game also compiles spells GAME addresses in its VA() claims. Its
bodies in another image are found from that image's own compile of the same
source (build/<image>/objdiff/base/<unit>.obj, the image's profile):

  1. identity: a compiled function whose bytes, with every relocation field
     masked, equal exactly one census function of the same size (at least
     MIN_FIXED fixed bytes);
  2. references: each placed function's relocations name their referents in
     retail - a REL32 call or jump lands on its callee, a DIR32 operand holds
     its referent's address - so a referenced function defined by an image
     unit is placed at that census start even when its own bytes differ.
     Where a placed body's layout differs from retail's, its DIR32 operands
     pair with the retail function's absolute sites by order, when both
     have the same number and the opcode byte before each agrees. A shared
     unit's `$E` bodies carry the game's compiler-function names
     (`__h3cg$...`, bound by homm3.compare.canonicalize's relocation roles);
  3. vtables: a census vtable whose RTTI class an image unit defines as
     `??_7Class@@6B@`, or a vtable address a placed function stores, places
     every slot symbol of the compiled table at the retail slot's target;
  4. data: a DIR32 reference of a placed function to data an image unit
     defines (globals, string-literal COMDATs) places that datum at the
     retail address less the compiled addend, with the compiled extent;
  5. string literals: a string-literal COMDAT (`??_C@`) whose bytes occur
     at exactly one retail address that retail code references anchors
     every compiled function that references it. A function still unplaced
     whose literals are referenced together by exactly one unclaimed census
     function is placed there, and the literal at its address;
  6. prefixes: a function still unplaced whose masked bytes agree with the
     start of exactly one unclaimed census function for at least
     PREFIX_FIXED fixed bytes, PREFIX_MARGIN more than any other start, is
     placed there (a body whose tail the image's compile changes).

Steps 2 and 3 repeat to a fixpoint after each of steps 1, 5 and 6; steps 5
and 6 never name an address that an earlier step claimed. Functions the image's runtime map names
(statically linked library code) are never placed. A name that reaches two addresses, or an
address that receives two names, is dropped and reported, except an /OPT:ICF
fold: the names that reach only that address, when their compiled bodies
agree byte for byte with the same relocation sites, place the first of them
there, and the comparison pairs the others' references with it. The result is
config/retail/<image>/placements.tsv; the label model reads it as the image's
claims for shared units (channel `placement`).
"""
from __future__ import annotations

import argparse
import bisect
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

from homm3.core import common, paths

import re

MIN_FIXED = 12
#: Prefix placement: fixed (unmasked) bytes the agreeing prefix must hold,
#: and its lead over the next best census start.
PREFIX_FIXED = 32
PREFIX_MARGIN = 8
#: A string-literal COMDAT: `??_C@_` plus its length and checksum.
LITERAL = re.compile(r"^\?\?_C@_")
#: Compiler initializer ordinals (`_$E22`): volatile, never a placed name.
VOLATILE = re.compile(r"^_?\$E[0-9]+$")
DIR32, REL32 = 6, 20
HEADER = "rva\tsize\tkind\tname\tunit\tevidence"


def _functions_of(obj):
    """[(name, section, offset, bytes, relocs)] for every function an object
    defines: code-section symbols, each running to the next symbol."""
    out = []
    for sec in obj.section_table:
        if not sec["characteristics"] & 0x20:
            continue
        number = sec["index"]
        payload = obj.section_payload(number)
        members = sorted((off, name) for off, name, scl in obj.section_members(number)
                         if scl in (2, 3) and not name.startswith(("$", ".")))
        if not members:
            continue
        relocs = obj.typed_relocations(number)
        for i, (off, name) in enumerate(members):
            end = members[i + 1][0] if i + 1 < len(members) else len(payload)
            own = {site - off: ref for site, ref in relocs.items() if off <= site < end}
            # /O2 objects align each function within one .text section; the
            # NOP/INT3 fill belongs to no function (the census strips it too)
            floor = off + max((site + 4 for site in own), default=0)
            while end > max(floor, off + 1) and payload[end - 1] in (0x90, 0xCC):
                end -= 1
            body = payload[off:end]
            out.append((name, number, off, body, own))
    return out


def _data_of(obj):
    """{name: (size, bytes)} of the data an object defines: external and
    static symbols (file statics, function-local statics and their guards)
    and string-literal COMDATs in its non-code sections, each running to the
    next symbol of its section (bytes None for uninitialized storage).
    Vtables and RTTI records are the census's."""
    out = {}
    for sec in obj.section_table:
        if sec["characteristics"] & 0x20 or sec["name"].startswith((".debug", ".drectve")):
            continue
        number = sec["index"]
        rows = obj.section_members(number)
        members = sorted((off, name) for off, name, scl in rows
                         if scl in (2, 3) and not name.startswith(("$", ".", "??_7", "??_R")))
        # every symbol bounds the one before it: a function-local static or
        # guard after the last external is not part of it
        bounds = sorted({off for off, _name, _scl in rows})
        payload = None if sec["characteristics"] & 0x80 else obj.section_payload(number)
        for off, name in members:
            later = bounds[bisect.bisect_right(bounds, off):]
            end = later[0] if later else sec["size"]
            if end > off:
                out.setdefault(name, (end - off, payload[off:end] if payload else None))
    return out


def _data_relocations(obj):
    """{data name: {offset: (referent, kind)}} of the relocations inside each
    datum `_data_of` reports (exception records name their catchable types
    and those their copy constructors)."""
    out = {}
    for sec in obj.section_table:
        if sec["characteristics"] & (0x20 | 0x80) or sec["name"].startswith((".debug", ".drectve")):
            continue
        number = sec["index"]
        relocs = obj.typed_relocations(number)
        if not relocs:
            continue
        rows = obj.section_members(number)
        members = sorted((off, name) for off, name, scl in rows
                         if scl in (2, 3) and not name.startswith(("$", ".", "??_7", "??_R")))
        bounds = sorted({off for off, _name, _scl in rows})
        for off, name in members:
            later = bounds[bisect.bisect_right(bounds, off):]
            end = later[0] if later else sec["size"]
            own = {site - off: ref for site, ref in relocs.items() if off <= site < end}
            if own:
                out.setdefault(name, own)
    return out


def _vtable_slots(obj):
    """{`??_7...` vtable symbol: [slot symbol, ...]} from the object's
    compiled tables."""
    out = {}
    for sec in obj.section_table:
        number = sec["index"]
        for off, name, scl in obj.section_members(number):
            if not name.startswith("??_7"):
                continue
            relocs = obj.typed_relocations(number)
            slots = []
            k = off
            while (k in relocs) and relocs[k][1] == DIR32:
                slots.append(relocs[k][0])
                k += 4
            out[name] = slots
    return out


#: The game's source-owned compiler-function claims (homm3.model), whose
#: unit + kind + owner identify a shared unit's `$E` bodies in any compile.
GAME_COMPGEN = common.HOMM3_DIR / "build/gen/compgen_claims.tsv"


def _owner_compgen(source: str) -> Path:
    """The compiler-function claims of the image that owns a shared source:
    the game's, or those of the image whose source tree holds it (built
    before this one by `homm3 build`)."""
    from homm3.core import images
    for key in images.images(paths.ROOT)[1:]:
        tree = paths.ROOT / "src" / images.source_dir(key, paths.ROOT)
        if tree in (paths.ROOT / source).parents:
            return paths.ROOT / images.path("build/gen/compgen_claims.tsv", key)
    return GAME_COMPGEN


def _compgen_names(path, unit: str, source: str) -> dict[str, str]:
    """{volatile `$E` symbol: `__h3cg$` name} of a shared unit's object: the
    owning image's claims for the unit, bound by the comparison's relocation
    roles (homm3.compare.canonicalize) and unsized, since this image's
    compile need not give the owner's body sizes."""
    import warnings
    from homm3.compare import canonicalize as canon
    claims_path = _owner_compgen(source)
    if not claims_path.is_file():
        return {}
    claims = tuple(canon.CompgenClaim(c.name, c.kind, c.owner, 0)
                   for c in canon.load_compgen_claims(claims_path, unit))
    if not claims:
        return {}
    coff = canon.CoffObject(path.read_bytes())
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", RuntimeWarning)   # unbound claims stay unbound
        try:
            renames, _rows = canon._compgen_renames(coff, claims)
        except ValueError:
            return {}
    return {coff.symbols[index].name: name for index, name in renames.items()}


#: IMAGE_COMDAT_SELECT_ANY: inline functions, template instances and
#: compiler-generated records. /Gy packages ordinary functions as
#: IMAGE_COMDAT_SELECT_NODUPLICATES instead.
COMDAT_SELECT_ANY = 2


def _any_comdats(obj) -> set[str]:
    """The external names an object defines in pick-any COMDAT sections."""
    out = set()
    for sec in obj.section_table:
        if sec["comdat"] != COMDAT_SELECT_ANY:
            continue
        for _off, name, scl in obj.section_members(sec["index"]):
            if scl == 2:
                out.add(name)
    return out


def _own_claimed_rvas() -> set[int]:
    """Every address the selected image's own sources claim with VA()/DATA()
    and their compiler-function forms (the lexical site sweep)."""
    from homm3.retail_labels.source import sweep_sites
    return {rva for sites in sweep_sites().values() for rva in sites}


def _own_function_claims(owned) -> list:
    """The function claims of the image's own units, freshly extracted from
    their VA() macros (homm3.retail_labels; content-idempotent fragments)."""
    if not owned:
        return []
    from homm3.retail_labels import fragments
    from homm3.retail_labels import source as labels_source
    labels_source.extract(sorted(owned))
    return [claim for unit in sorted(owned) for claim in fragments.unit_claims(unit)
            if claim.kind == "func"]


def portable(text: str, unit: str) -> str:
    """`text` with each anonymous-namespace scope in its checkout-independent
    spelling (homm3.compare.canonicalize), so the table does not depend on
    where the image's units were compiled."""
    from homm3.compare.canonicalize import normalize_anon_ns_name
    if "?%" not in text:
        return text
    return " ".join(normalize_anon_ns_name(word, unit) for word in text.split(" "))


def referrer(name: str, rva: int) -> str:
    """How an evidence note names the placed body that references a symbol:
    its compiled name, or its address when that name spells an anonymous
    namespace. VC6 writes such a scope as the compiled file's full path, and
    a name that repeats it can pass the compiler's length limit in a
    checkout with a longer path, where the object then holds a shortened,
    hashed name: the note would depend on the checkout."""
    return f"0x{rva:08x}" if "?%" in name else name


def clip_data_extents(rows):
    """`rows` with each data row ending where the next placed datum starts.

    A datum's compiled extent runs to the next symbol of its section, and
    the compiler need not lay a section out in the linked order: victor's
    .bss holds g_victorCreateDibSection, g_victorSetDibColorTable and then
    g_victorUseDibSection, which the editors' retail images (as the game)
    place first, so its compiled extent of 8 covered its neighbour."""
    data = sorted((row for row in rows if row[2] == "data"), key=lambda row: row[0])
    starts = [row[0] for row in data]
    clipped = {}
    for k, row in enumerate(data):
        later = [start for start in starts[k + 1:] if start > row[0]]
        if later and row[0] + row[1] > later[0]:
            clipped[id(row)] = (row[0], later[0] - row[0], *row[2:])
    return [clipped.get(id(row), row) for row in rows]


def derive(log=print, want_suggestions=False):
    from homm3 import manifest
    from homm3.core.image import Image
    from homm3.delink.coffx import Obj
    from homm3.retail_labels import censuses
    from homm3.core.tsv import read as read_tsv

    image = Image(str(common.resolve_exe()))
    base = image.image_base
    retail = paths.retail_dir()
    functions = {r["rva"]: r["size"] for r in censuses.functions(retail / "functions.tsv")}
    # statically linked library code is named by the runtime map, never
    # placed, and a name the runtime map gives one address reaches no other
    from homm3.retail_labels import providers
    library_names = set()
    for claim in providers.runtime_map(retail / "runtime-map.tsv"):
        functions.pop(claim.rva, None)
        library_names.add(claim.name)
    vtables = censuses.vtables(retail / "vtables.tsv")
    vt_by_class = {r["class"]: r for r in vtables if r["class"]}

    def word(rva):
        sec = image.section_of(rva)
        if sec is None or rva + 4 > sec.rva + sec.size:
            return None
        return struct.unpack_from("<I", image.data, sec.raw_offset + rva - sec.rva)[0]

    def blob(rva, size):
        sec = image.section_of(rva)
        if sec is None:
            return None
        o = sec.raw_offset + rva - sec.rva
        return image.data[o:o + size]

    units = manifest.units(paths.manifest())
    # the image's own units spell their addresses in VA()/DATA(); their
    # placements are suggestions for that source, never table rows. A unit
    # the game or another image owns is shared.
    from homm3.core import images
    shared_sources = {u["source"] for u in manifest.units(paths.manifest("game"))}
    shared_sources |= {u["source"] for u in units
                       if images.foreign(paths.ROOT / u["source"], paths.ROOT)}
    owned = {u["unit"] for u in units if u["source"] not in shared_sources}
    compiled = []                         # (unit, name, body, relocs)
    inline_comdats = set()                # own units' pick-any COMDAT names
    definers = defaultdict(list)          # function name -> units, manifest order
    tables = {}                           # vtable symbol -> slots
    data_definers = {}                    # data name -> (unit, size, bytes)
    data_relocs = {}                      # data name -> {offset: (referent, kind)}
    for unit in units:
        path = paths.BUILD / "objdiff/base" / f"{unit['unit']}.obj"
        if not path.is_file():
            log(f"[placements] {unit['unit']}: no object at {path}; build it first")
            continue
        obj = Obj(path)
        semantic = (_compgen_names(path, unit["unit"], unit["source"])
                    if unit["source"] in shared_sources else {})
        if unit["unit"] in owned:
            inline_comdats.update(portable(name, unit["unit"])
                                  for name in _any_comdats(obj))
        for name, _sec, _off, body, relocs in _functions_of(obj):
            name = semantic.get(name, name)
            relocs = {site: (semantic.get(ref, ref), kind) for site, (ref, kind) in relocs.items()}
            compiled.append((unit["unit"], name, body, relocs))
            if unit["unit"] not in definers[name]:
                definers[name].append(unit["unit"])
        for name, slots in _vtable_slots(obj).items():
            tables.setdefault(name, slots)
        for name, (size, text) in _data_of(obj).items():
            data_definers.setdefault(name, (unit["unit"], size, text))
        for name, relocs in _data_relocations(obj).items():
            data_relocs.setdefault(name, relocs)

    by_size = defaultdict(list)
    for rva, size in functions.items():
        by_size[size].append(rva)

    names = defaultdict(set)              # name -> {rva}
    evidence = {}                         # (name, rva) -> first evidence
    bodies = {}                           # name -> (body, relocs), first definer

    data_names = defaultdict(set)         # data name -> {rva}
    data_evidence = {}

    def propose_data(name, rva, why):
        sec = image.section_of(rva)
        if VOLATILE.match(name) or sec is None or sec.executable or rva in names_by_function:
            return 0
        if rva in data_names[name]:
            return 0
        data_names[name].add(rva)
        data_evidence.setdefault((name, rva), why)
        return 0          # data never seeds further propagation

    names_by_function = set(functions)

    strict = False                        # set once steps 5 and 6 begin
    named_at = defaultdict(set)           # rva -> {name}

    def propose(name, rva, why):
        if VOLATILE.match(name) or rva not in functions:
            return False
        if rva in names[name]:
            return False
        if strict and (names[name] or named_at[rva]):
            # the weaker steps never contradict what is already placed
            return False
        names[name].add(rva)
        named_at[rva].add(name)
        evidence.setdefault((name, rva), why)
        return True

    relocs_of = {}                        # id(body) -> its relocation sites
    for unit, name, body, relocs in compiled:
        bodies.setdefault(name, (body, relocs))
        relocs_of[id(body)] = tuple(relocs)
        mask = bytearray(len(body))
        for site in relocs:
            mask[site:site + 4] = b"\1\1\1\1"
        fixed = len(body) - sum(mask)
        if fixed < MIN_FIXED:
            continue
        hits = [rva for rva in by_size.get(len(body), ())
                if all(m or a == b for a, b, m in zip(blob(rva, len(body)), body, mask))]
        if len(hits) == 1:
            propose(name, hits[0], f"masked body ({len(body)} bytes, "
                                   f"{len(relocs)} relocations) unique at a census start")

    # the image's own VA() claims: a claimed body that equals retail at its
    # claimed address (relocation fields masked) names its referents (the
    # template instances and header inlines it calls) like a placed body
    own_function_claims = _own_function_claims(owned)
    own_named = defaultdict(set)          # rva -> {name} the image's own VA() claims
    for claim in own_function_claims:
        own_named[claim.rva].add(claim.name)
    for claim in own_function_claims:
        if claim.name not in bodies or named_at[claim.rva] - {claim.name}:
            continue
        body, relocs = bodies[claim.name]
        if functions.get(claim.rva) != len(body):
            continue
        mask = bytearray(len(body))
        for site in relocs:
            mask[site:site + 4] = b"\1\1\1\1"
        if all(m or a == b for a, b, m in zip(blob(claim.rva, len(body)), body, mask)):
            propose(claim.name, claim.rva, "the image's own VA() claim, masked body equal")

    # vtables named by RTTI
    def place_table(symbol, vt_rva, why):
        moved = 0
        for k, slot in enumerate(tables.get(symbol, ())):
            target = word(vt_rva + 4 * k)
            if target is not None and slot in definers:
                moved += propose(slot, target - base, f"{why} slot {k}")
        return moved

    for symbol in tables:
        cls = symbol[4:-6] if symbol.endswith("@@6B@") else None
        row = vt_by_class.get(cls) if cls else None
        if row is not None:
            place_table(symbol, row["rva"], f"vtable of {cls} (RTTI)")

    def same_operand(body, rva, site, context=1):
        """A relocation site whose instruction bytes before the field agree
        with retail: a placed body that differs elsewhere still names its
        referents there, and nowhere else. `context` counts the fixed bytes
        compared, other relocation fields skipped."""
        fields = set()
        for other in relocs_of[id(body)]:
            if other != site:
                fields.update(range(other, other + 4))
        offsets = []
        k = site - 1
        while k >= 0 and len(offsets) < context:
            if k not in fields:
                offsets.append(k)
            k -= 1
        if not offsets:
            return True
        theirs = blob(rva + offsets[-1], site - offsets[-1])
        return all(theirs[k - offsets[-1]] == body[k] for k in offsets)

    # reference propagation to a fixpoint over uniquely placed functions
    done = set()

    from homm3.core.tsv import read as _read_tsv
    absolute = sorted(int(row["site_rva"], 16)
                      for row in _read_tsv(retail / "relocs.tsv")[2])

    def paired_sites(body, relocs, rva):
        """{compiled DIR32 site: retail site} by order, for a placed body
        whose layout differs from retail's: when both hold the same number
        of absolute operands, the n-th of each agree when the opcode byte
        before them does."""
        mine = sorted(site for site, (_ref, kind) in relocs.items() if kind == DIR32)
        lo = bisect.bisect_left(absolute, rva)
        hi = bisect.bisect_left(absolute, rva + functions[rva])
        theirs = [site - rva for site in absolute[lo:hi]]
        if len(mine) != len(theirs):
            return {}
        out = {}
        for a, b in zip(mine, theirs):
            if a and b and body[a - 1] == blob(rva + b - 1, 1)[0]:
                out[a] = b
        return out

    def divergence(body, relocs, rva):
        """The first offset where a placed body's fixed bytes leave retail's:
        past it a relocation's offset need not name the same instruction in
        retail, however its opcode byte agrees (two `push imm32` of one throw
        swap places; a call at the same offset of a body whose layout moved
        reaches another callee), so the instruction bytes before it must
        agree further back."""
        theirs = blob(rva, min(len(body), functions[rva]))
        fields = set()
        for site in relocs:
            fields.update(range(site, site + 4))
        for k, byte in enumerate(theirs):
            if k not in fields and body[k] != byte:
                return k
        return len(theirs)

    def body_at(name, target):
        """Whether `name`'s compiled body equals retail at the census start
        `target`, relocation fields masked."""
        if name not in bodies or functions.get(target) != len(bodies[name][0]):
            return False
        body, relocs = bodies[name]
        mask = bytearray(len(body))
        for site in relocs:
            mask[site:site + 4] = b"\1\1\1\1"
        return all(m or x == y for x, y, m in zip(blob(target, len(body)), body, mask))

    # the image's pin decides whether a call past the divergence needs proof
    # (config/project.toml `placements_prove_late_calls`)
    from homm3.core import images as _pins
    prove_late_calls = bool(_pins.pins(paths.ROOT).get(_pins.input_key(paths.image_key()), {})
                            .get("placements_prove_late_calls", False))

    followed = set()

    def follow_data():
        """A placed datum's DIR32 fields name their referents, as a placed
        body's do: a ThrowInfo its catchable-type array, that array its
        catchable types, and each of those the exception's copy constructor."""
        moved = 0
        while True:
            fresh = [(name, next(iter(rvas))) for name, rvas in list(data_names.items())
                     if len(rvas) == 1 and name not in followed and name in data_relocs]
            if not fresh:
                return moved
            for name, rva in fresh:
                followed.add(name)
                text = data_definers.get(name, (None, None, None))[2]
                for site, (ref, kind) in data_relocs[name].items():
                    value = word(rva + site)
                    if kind != DIR32 or value is None or text is None or site + 4 > len(text):
                        continue
                    target = value - base - struct.unpack_from("<i", text, site)[0]
                    why = f"referenced by {referrer(name, rva)} at +0x{site:x}"
                    if ref in definers:
                        moved += propose(ref, target, why)
                    elif ref in data_definers:
                        propose_data(ref, target, why)

    def propagate():
        while True:
            moved = follow_data()
            for name, rvas in list(names.items()):
                if len(rvas) != 1 or name in done or name not in bodies:
                    continue
                done.add(name)
                (rva,) = rvas
                body, relocs = bodies[name]
                order = None
                diverged = divergence(body, relocs, rva)
                for site, (ref, kind) in relocs.items():
                    value = word(rva + site)
                    at = f"+0x{site:x}"
                    if (value is None or not same_operand(
                            body, rva, site, 4 if site >= diverged
                            and (kind == DIR32 or prove_late_calls) else 1)):
                        if kind == REL32:
                            # past the divergence a call at the same offset
                            # may reach another callee: the callee's own body,
                            # equal at its target, proves the site; otherwise
                            # the site may only name a callee nothing else has
                            # placed, at an address nothing else names
                            if value is None or site < diverged or not prove_late_calls:
                                continue
                            callee = (rva + site + 4 + value) & 0xFFFFFFFF
                            if not body_at(ref, callee) and (names[ref] or named_at[callee]):
                                continue
                        elif kind != DIR32:
                            continue
                        else:
                            if order is None:
                                order = paired_sites(body, relocs, rva)
                            if site not in order:
                                continue
                            value = word(rva + order[site])
                            at = f"+0x{site:x} (retail +0x{order[site]:x}, by operand order)"
                            if value is None:
                                continue
                    if kind == REL32:
                        target = (rva + site + 4 + value) & 0xFFFFFFFF
                    elif kind == DIR32:
                        # the compiled field holds the reference's addend
                        target = value - base - struct.unpack_from("<i", body, site)[0]
                    else:
                        continue
                    if own_named[target] and ref not in own_named[target]:
                        # the image's own source names that address; a body
                        # that inlined the claimed callee reaches its callee
                        # through the same field
                        continue
                    if ref in definers:
                        moved += propose(ref, target, f"referenced by {referrer(name, rva)} at {at}")
                    elif ref in tables:
                        moved += place_table(ref, target, f"vtable {ref} stored by {referrer(name, rva)}")
                    elif ref in data_definers and kind == DIR32:
                        propose_data(ref, target, f"referenced by {referrer(name, rva)} at {at}")
            if not moved and not follow_data():
                break

    propagate()

    def claimed():
        return {rva for rvas in names.values() for rva in rvas}

    starts = sorted(functions)

    def owner(site):
        """The census function whose extent holds a retail site."""
        k = bisect.bisect_right(starts, site) - 1
        if k >= 0 and site < starts[k] + functions[starts[k]]:
            return starts[k]
        return None

    strict = True

    # 5. string-literal anchors
    referrers = defaultdict(set)          # retail data address -> {census function}
    for row in read_tsv(retail / "reloc-evidence.tsv")[2]:
        if row["disposition"] != "kept" or row["channel"] != "code":
            continue
        fn = owner(int(row["site_rva"], 16))
        if fn is not None:
            referrers[int(row["value"], 16) - base].add(fn)
    data_blobs = [(sec, image.blob(sec)) for sec in image.sections if not sec.executable]

    def literal_home(text):
        """The one retail address that holds `text` and that code references."""
        hits = []
        for sec, payload in data_blobs:
            at = payload.find(text)
            while at >= 0:
                if sec.rva + at in referrers:
                    hits.append(sec.rva + at)
                at = payload.find(text, at + 1)
        return hits[0] if len(hits) == 1 else None

    homes = {}
    for name, (unit, size, text) in data_definers.items():
        if LITERAL.match(name) and text and len(text) >= 4:
            home = literal_home(text)
            if home is not None:
                homes[name] = home
    taken = claimed()
    anchored = 0
    chosen = defaultdict(list)            # rva -> [(name, literals)]
    for unit, name, body, relocs in compiled:
        if names.get(name):
            continue
        literals = {ref for ref, kind in relocs.values() if kind == DIR32 and ref in homes}
        if not literals:
            continue
        candidates = None
        for ref in literals:
            users = referrers[homes[ref]] - taken
            candidates = users if candidates is None else candidates & users
        if candidates and len(candidates) == 1:
            chosen[next(iter(candidates))].append((name, literals))
    for rva, choices in sorted(chosen.items()):
        if len(choices) != 1:
            continue                      # two compiled bodies anchor here
        name, literals = choices[0]
        shown = ", ".join(sorted(literals))
        anchored += propose(name, rva, f"string literal anchor ({shown})")
        for ref in literals:
            propose_data(ref, homes[ref], f"string literal referenced by {referrer(name, rva)}")
    propagate()
    # a literal's unique home is its address when a placed referrer reads it
    for unit, name, body, relocs in compiled:
        if len(names.get(name, ())) != 1:
            continue
        (rva,) = names[name]
        for ref, kind in relocs.values():
            if kind == DIR32 and ref in homes and rva in referrers[homes[ref]]:
                propose_data(ref, homes[ref], f"string literal referenced by {referrer(name, rva)}")

    # 6. masked prefixes
    taken = claimed()
    windows = {}                          # offset -> {6 retail bytes: [rva]}

    def window(offset):
        if offset not in windows:
            index = defaultdict(list)
            for rva in starts:
                if rva not in taken and functions[rva] >= offset + 6:
                    index[blob(rva + offset, 6)].append(rva)
            windows[offset] = index
        return windows[offset]

    prefixed = 0
    chosen = defaultdict(list)            # rva -> [(name, evidence)]
    for unit, name, body, relocs in compiled:
        if names.get(name) or len(body) < PREFIX_FIXED:
            continue
        mask = bytearray(len(body))
        for site in relocs:
            mask[site:site + 4] = b"\1\1\1\1"
        offset = next((o for o in range(min(32, len(body) - 6))
                       if not any(mask[o:o + 6])), None)
        if offset is None:
            continue
        scores = []
        for rva in window(offset).get(bytes(body[offset:offset + 6]), ()):
            if rva in taken:
                continue
            theirs = blob(rva, min(len(body), functions[rva]))
            fixed = 0
            for k, (a, b) in enumerate(zip(theirs, body)):
                if mask[k]:
                    continue
                if a != b:
                    break
                fixed += 1
            scores.append((fixed, rva))
        scores.sort(reverse=True)
        if not scores or scores[0][0] < PREFIX_FIXED:
            continue
        if not 0.5 <= functions[scores[0][1]] / len(body) <= 2:
            continue
        if len(scores) > 1 and scores[0][0] - scores[1][0] < PREFIX_MARGIN:
            continue
        fixed, rva = scores[0]
        chosen[rva].append((name, f"masked prefix ({fixed} fixed bytes agree, "
                                  f"{len(body)} compiled, {functions[rva]} retail)"))
    for rva, choices in sorted(chosen.items()):
        if len(choices) == 1:
            prefixed += propose(choices[0][0], rva, choices[0][1])
    propagate()
    log(f"[placements] {len(homes)} string literals at unique referenced addresses; "
        f"{anchored} functions anchored by them, {prefixed} by masked prefixes")

    by_rva = defaultdict(set)
    for name, rvas in names.items():
        for rva in rvas:
            by_rva[rva].add(name)
    # A template body the image's own unit also emits (its suggestions) does
    # not compete with a shared unit's copy at the same address: the shared
    # name places it, and the own unit's VA() claims read the suggestion.
    for rva, group in by_rva.items():
        shared = {name for name in group
                  if any(unit not in owned for unit in definers.get(name, ()))}
        if shared and shared != group:
            by_rva[rva] = shared
    def folded(rva):
        """The one name of an address that several compiled bodies reach:
        /OPT:ICF folded the bodies that reach only this address when they
        agree byte for byte with the same relocation sites. The first name
        labels the address, a shared unit's before the image's own (whose
        MFC inline virtuals fold onto shared bodies but are no placement);
        the comparison pairs the others' references with it
        (normalize_objs ICF twins)."""
        # a name that also reaches another address is no witness either way
        group = sorted((name for name in by_rva[rva] if len(names[name]) == 1),
                       key=lambda name: (definers[name][0] in owned, name))
        if len(group) < 2 or any(name not in bodies for name in group):
            return None
        shapes = set()
        for name in group:
            body, relocs = bodies[name]
            masked = bytearray(body)
            for site in relocs:
                masked[site:site + 4] = b"\0\0\0\0"
            shapes.add((bytes(masked), tuple(sorted(relocs))))
        return group[0] if len(shapes) == 1 else None

    rows, conflicts = [], 0
    for name, rvas in sorted(names.items()):
        if len(rvas) != 1 or name in library_names:
            conflicts += 1
            continue
        (rva,) = rvas
        if len(by_rva[rva]) != 1:
            if folded(rva) == name:
                unit = definers[name][0]
                why = (f"{evidence[(name, rva)]}; ICF-folded with "
                       f"{len(by_rva[rva]) - 1} identical bodies")
                rows.append((rva, functions[rva], "func", portable(name, unit), unit,
                             portable(why, unit)))
            else:
                conflicts += 1
            continue
        unit = definers[name][0]
        rows.append((rva, functions[rva], "func", portable(name, unit), unit,
                     portable(evidence[(name, rva)], unit)))
    placed_functions = len(rows)
    data_by_rva = defaultdict(set)
    for name, rvas in data_names.items():
        for rva in rvas:
            data_by_rva[rva].add(name)
    data_conflicts = 0
    for name, rvas in sorted(data_names.items()):
        if len(rvas) != 1 or len(data_by_rva[next(iter(rvas))]) != 1:
            data_conflicts += 1
            continue
        (rva,) = rvas
        unit, size, _text = data_definers[name]
        rows.append((rva, size, "data", portable(name, unit), unit,
                     portable(data_evidence[(name, rva)], unit)))
    rows = clip_data_extents(rows)
    rows.sort()
    # An own unit's pick-any COMDAT is a header inline, a template or a
    # compiler-generated record: no source of the image can claim it (its
    # header may be another image's), so its placement is a table row unless
    # the image's own sources already claim that address.
    # A shared unit's name at the same address wins, and two such names for
    # one address (an /OPT:ICF fold) place neither.
    own_claims = _own_claimed_rvas()
    labelled = Counter(row[0] for row in rows if row[4] not in owned)
    candidates = Counter(row[0] for row in rows if row[4] in owned and row[3] in inline_comdats)

    def promoted(row):
        return (row[4] in owned and row[3] in inline_comdats and row[0] not in own_claims
                and not labelled[row[0]] and candidates[row[0]] == 1)

    suggestions = [row for row in rows if row[4] in owned and not promoted(row)]
    rows = [row for row in rows if row[4] not in owned or promoted(row)]
    placed_functions -= sum(row[2] == "func" for row in suggestions)
    log(f"[placements] {placed_functions} functions and {len(rows) - placed_functions} "
        f"data objects placed from {len(compiled)} compiled bodies of {len(units)} "
        f"units; {conflicts} function and {data_conflicts} data names or addresses "
        "ambiguous and dropped")
    if suggestions:
        log(f"[placements] {len(suggestions)} placements of the image's own units "
            "(`--suggest` lists them for their VA()/DATA() claims)")
    return (rows, suggestions) if want_suggestions else rows


def render(rows) -> str:
    image = paths.image_key()
    lines = [f"# GENERATED by `homm3 --image {image} placements --write` from the",
             "# image's compile of its units and its census; regenerate after a",
             "# census, manifest or shared-source change.",
             *common.provenance("homm3.census.placements"), HEADER]
    lines += [f"0x{rva:08x}\t0x{size:x}\t{kind}\t{name}\t{unit}\t{why}"
              for rva, size, kind, name, unit, why in rows]
    return "\n".join(lines) + "\n"


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm3 placements", description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--write", action="store_true",
                        help="store config/retail/<image>/placements.tsv")
    parser.add_argument("--check", action="store_true",
                        help="fail when the committed table differs from the derivation")
    parser.add_argument("--suggest", metavar="UNIT", action="append",
                        help="list where the image's own UNIT's compiled functions "
                             "and data sit, for its VA()/DATA() claims (repeatable)")
    args = parser.parse_args(argv)
    if paths.is_game():
        print("[placements] the game spells its addresses in source; select "
              "another image with `homm3 --image KEY placements`", file=sys.stderr)
        return 1
    if args.suggest:
        _rows, suggestions = derive(want_suggestions=True)
        for rva, size, kind, name, unit, why in suggestions:
            if unit in args.suggest:
                macro = "VA" if kind == "func" else "DATA"
                spelled = f"0x{rva + common.IMAGE_BASE:08x}"
                print(f"{unit}\t{macro}({spelled}{f', 0x{size:x}' if kind == 'func' else ''})"
                      f"\t{name}\t{why}")
        return 0
    text = render(derive())
    target = paths.retail_dir() / "placements.tsv"

    def body(text: str) -> list[str]:
        return [line for line in text.splitlines() if not line.startswith("#")]
    if args.check:
        if not target.is_file() or body(target.read_text()) != body(text):
            print(f"[placements] {target} differs from the derivation", file=sys.stderr)
            return 1
        return 0
    if args.write:
        target.write_text(text)
        print(f"[placements] wrote {target.relative_to(paths.ROOT)}")
    return 0


def logged_main(argv=None) -> int:
    from homm3.core.usage import append, run_logged
    import shlex
    argv = list(sys.argv[1:] if argv is None else argv)
    cmd = shlex.join(["homm3", f"--image={paths.image_key()}", "placements", *argv])
    return run_logged(main, argv,
                      lambda rc, **meta: append(paths.SHARED_BUILD / "homm3_usage.log",
                                                cmd, rc, **meta), failure_rc=1)


if __name__ == "__main__":
    sys.exit(logged_main())
