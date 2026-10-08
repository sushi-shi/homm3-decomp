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
     every slot symbol of the compiled table at the retail slot's target.

Steps 2 and 3 repeat to a fixpoint. Functions the image's runtime map names
(statically linked library code) are never placed. A name that reaches two addresses, or an
address that receives two names, is dropped and reported (ICF folds and
genuine ambiguity alike stay unclaimed until reviewed). The result is
config/retail/<image>/placements.tsv; the label model reads it as the image's
claims for shared units (channel `placement`).
"""
from __future__ import annotations

import argparse
import struct
import sys
from collections import defaultdict

from homm3.core import common, paths

import re

MIN_FIXED = 12
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
            body = payload[off:end]
            own = {site - off: ref for site, ref in relocs.items() if off <= site < end}
            out.append((name, number, off, body, own))
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


def derive(log=print):
    from homm3 import manifest
    from homm3.core.image import Image
    from homm3.delink.coffx import Obj
    from homm3.retail_labels import censuses

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

    by_size = defaultdict(list)
    for rva, size in functions.items():
        by_size[size].append(rva)

    names = defaultdict(set)              # name -> {rva}
    evidence = {}                         # (name, rva) -> first evidence
    bodies = {}                           # name -> (body, relocs), first definer

    def propose(name, rva, why):
        if VOLATILE.match(name) or rva not in functions:
            return False
        if rva in names[name]:
            return False
        names[name].add(rva)
        evidence.setdefault((name, rva), why)
        return True

    for unit, name, body, relocs in compiled:
        bodies.setdefault(name, (body, relocs))
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

    # reference propagation to a fixpoint over uniquely placed functions
    done = set()
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
                if value is None:
                    continue
                if kind == REL32:
                    target = (rva + site + 4 + value) & 0xFFFFFFFF
                elif kind == DIR32:
                    target = value - base
                else:
                    continue
                if ref in definers:
                    moved += propose(ref, target, f"referenced by {name} at +0x{site:x}")
                elif ref in tables:
                    moved += place_table(ref, target, f"vtable {ref} stored by {name}")
        if not moved:
            break

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
        rows.append((rva, functions[rva], "func", name, definers[name][0],
                     evidence[(name, rva)]))
    rows.sort()
    log(f"[placements] {len(rows)} functions placed from {len(compiled)} compiled "
        f"bodies of {len(units)} units; {conflicts} ambiguous names or addresses dropped")
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
