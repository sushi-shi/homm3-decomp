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
     unit is placed at that census start even when its own bytes differ;
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
address that receives two names, is dropped and reported (ICF folds and
genuine ambiguity alike stay unclaimed until reviewed). The result is
config/retail/<image>/placements.tsv; the label model reads it as the image's
claims for shared units (channel `placement`).
"""
from __future__ import annotations

import argparse
import bisect
import struct
import sys
from collections import defaultdict

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
    """{name: (size, bytes)} of the data an object defines: external symbols
    and string-literal COMDATs in its non-code sections, each running to the
    next symbol of its section (bytes None for uninitialized storage).
    Vtables and RTTI records are the census's."""
    out = {}
    for sec in obj.section_table:
        if sec["characteristics"] & 0x20 or sec["name"].startswith((".debug", ".drectve")):
            continue
        number = sec["index"]
        members = sorted((off, name) for off, name, scl in obj.section_members(number)
                         if scl == 2 and not name.startswith(("$", ".", "??_7", "??_R")))
        payload = None if sec["characteristics"] & 0x80 else obj.section_payload(number)
        for i, (off, name) in enumerate(members):
            end = members[i + 1][0] if i + 1 < len(members) else sec["size"]
            if end > off:
                out.setdefault(name, (end - off, payload[off:end] if payload else None))
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


def portable(text: str, unit: str) -> str:
    """`text` with each anonymous-namespace scope in its checkout-independent
    spelling (homm3.compare.canonicalize), so the table does not depend on
    where the image's units were compiled."""
    from homm3.compare.canonicalize import normalize_anon_ns_name
    if "?%" not in text:
        return text
    return " ".join(normalize_anon_ns_name(word, unit) for word in text.split(" "))


def derive(log=print):
    from homm3 import manifest
    from homm3.core.image import Image
    from homm3.delink.coffx import Obj
    from homm3.retail_labels import censuses
    from homm3.core.tsv import read as read_tsv

    image = Image(str(common.resolve_exe()))
    base = image.image_base
    retail = paths.retail_dir()
    functions = {r["rva"]: r["size"] for r in censuses.functions(retail / "functions.tsv")}
    # statically linked library code is named by the runtime map, never placed
    from homm3.retail_labels import providers
    for claim in providers.runtime_map(retail / "runtime-map.tsv"):
        functions.pop(claim.rva, None)
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
    compiled = []                         # (unit, name, body, relocs)
    definers = defaultdict(list)          # function name -> units, manifest order
    tables = {}                           # vtable symbol -> slots
    data_definers = {}                    # data name -> (unit, size, bytes)
    for unit in units:
        path = paths.BUILD / "objdiff/base" / f"{unit['unit']}.obj"
        if not path.is_file():
            log(f"[placements] {unit['unit']}: no object at {path}; build it first")
            continue
        obj = Obj(path)
        for name, _sec, _off, body, relocs in _functions_of(obj):
            compiled.append((unit["unit"], name, body, relocs))
            if unit["unit"] not in definers[name]:
                definers[name].append(unit["unit"])
        for name, slots in _vtable_slots(obj).items():
            tables.setdefault(name, slots)
        for name, (size, text) in _data_of(obj).items():
            data_definers.setdefault(name, (unit["unit"], size, text))

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

    def same_operand(body, rva, site):
        """A relocation site whose instruction bytes before the field agree
        with retail: a placed body that differs elsewhere still names its
        referents there, and nowhere else."""
        start = max(0, site - 1)
        theirs = blob(rva + start, site - start)
        fields = [other for other in relocs_of[id(body)] if other != site]
        return all(theirs[k - start] == body[k] for k in range(start, site)
                   if not any(other <= k < other + 4 for other in fields))

    # reference propagation to a fixpoint over uniquely placed functions
    done = set()

    def propagate():
        while True:
            moved = 0
            for name, rvas in list(names.items()):
                if len(rvas) != 1 or name in done or name not in bodies:
                    continue
                done.add(name)
                (rva,) = rvas
                body, relocs = bodies[name]
                for site, (ref, kind) in relocs.items():
                    value = word(rva + site)
                    if value is None or not same_operand(body, rva, site):
                        continue
                    if kind == REL32:
                        target = (rva + site + 4 + value) & 0xFFFFFFFF
                    elif kind == DIR32:
                        # the compiled field holds the reference's addend
                        target = value - base - struct.unpack_from("<i", body, site)[0]
                    else:
                        continue
                    if ref in definers:
                        moved += propose(ref, target, f"referenced by {name} at +0x{site:x}")
                    elif ref in tables:
                        moved += place_table(ref, target, f"vtable {ref} stored by {name}")
                    elif ref in data_definers and kind == DIR32:
                        propose_data(ref, target, f"referenced by {name} at +0x{site:x}")
            if not moved:
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
            propose_data(ref, homes[ref], f"string literal referenced by {name}")
    propagate()
    # a literal's unique home is its address when a placed referrer reads it
    for unit, name, body, relocs in compiled:
        if len(names.get(name, ())) != 1:
            continue
        (rva,) = names[name]
        for ref, kind in relocs.values():
            if kind == DIR32 and ref in homes and rva in referrers[homes[ref]]:
                propose_data(ref, homes[ref], f"string literal referenced by {name}")

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
    rows, conflicts = [], 0
    for name, rvas in sorted(names.items()):
        if len(rvas) != 1:
            conflicts += 1
            continue
        (rva,) = rvas
        if len(by_rva[rva]) != 1:
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
    rows.sort()
    log(f"[placements] {placed_functions} functions and {len(rows) - placed_functions} "
        f"data objects placed from {len(compiled)} compiled bodies of {len(units)} "
        f"units; {conflicts} function and {data_conflicts} data names or addresses "
        "ambiguous and dropped")
    return rows


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
    args = parser.parse_args(argv)
    if paths.is_game():
        print("[placements] the game spells its addresses in source; select "
              "another image with `homm3 --image KEY placements`", file=sys.stderr)
        return 1
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
