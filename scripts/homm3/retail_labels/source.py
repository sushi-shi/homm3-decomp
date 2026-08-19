#!/usr/bin/env python3
"""homm3.retail_labels.source - per-TU source-claim extraction.

    homm3 labels [--unit U ...] [--all]

The extraction universe is sorted(src/*.c*), NOT the manifest (see the
package docstring). Per TU, mechanisms unchanged from the pre-port
homm3.build.labels:

  VA(0xva, size)      lexical scan (comments/strings blanked); the claim's
                      working name derives from the DECLARATOR that follows
                      (source-as-authority). Fatal: an address below the
                      image base or off a carved function entry, and an
                      orphan annotation with no declaration below it (the
                      gruntz orphan-annotation incident).
  VA_COMPGEN          compiler-generated bodies (static-init dispatch,
                      atexit, scalar deleting dtors, ...) - the claim is
                      named __h3cg$<unit>$<kind>$<owner>; unknown kinds die.
  DATA(0xva)          data claim, dense working name data_<rva>.
  DATA_COMPGEN(_GUARD) compiler-generated data pins, named
                      __h3cg$<unit>$...$<name>.

THE JOIN (extraction's second, join-bearing concern - as in gruntz): a
compiled unit's base obj carries the TRUE MSVC spellings, so uniquely
joined claims adopt them (channel src-VA+base) and the delinked target
pairs against the base by identical names. This is the interim P0.2
binding; a clang-IR channel would land here, replacing the declarator
join, never the model. The fragment keeps the RAW declarator name
alongside the joined spelling because the model's scan-order dedup
replays over raw names, exactly as the monolith did.

Fragments (build/gen/claims/<unit>.tsv) are written atomically and
content-idempotently; a stale fragment whose src file vanished is pruned
on a full run. Fragment freshness follows the base objs: run after a
build (the `homm3 delink` chain does).
"""

from __future__ import annotations

import re
import struct
import sys

from homm3.core import common
from homm3.core.tsv import write as write_tsv
from homm3.retail_labels.fragments import FRAGMENTS, HEADER, fragment_path

SRC_DIR = common.HOMM3_DIR / "src"

VA_RE = re.compile(r"^\s*VA\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*"
                   r"(0x[0-9a-fA-F]+|\d+)\s*\)")
VA_COMPGEN_RE = re.compile(
    r"^\s*VA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)"
    r"\s*,\s*(\w+)\s*,\s*(\w+)\s*\)")
DATA_RE = re.compile(r"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")
DATA_COMPGEN_RE = re.compile(
    r"\bDATA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(\w+)\s*,")
DATA_COMPGEN_GUARD_RE = re.compile(
    r"\bDATA_COMPGEN_GUARD\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(\w+)\s*,"
    r"\s*(\w+)\s*\)")
ANNOTATION_RE = re.compile(r"^\s*(?:VA|VA_COMPGEN|DATA|DC_ONLY)\s*\(")
DECLARATOR_RE = re.compile(r"([~\w:]+(?:<[^<>()]*>)?)\s*\(")
# MSVC special members render with backticks: Cls::`scalar deleting
# destructor'(...), `default constructor closure'(...)
SPECIAL_RE = re.compile(r"([\w:]+)::`([^'`]+)'\s*\(")
# Template argument list in a source declarator: `vector<int>::begin` ->
# `vector::begin`. Applied to a fixed point so nested lists collapse.
# Template arguments never take part in the join key (the mangled side
# cannot be parsed back to them without a full demangler), so both sides
# normalize them away - see TEMPLATE_MEMBER_RE.
TEMPLATE_ARGS_RE = re.compile(r"<[^<>]*>")
# One MSVC-mangled member of a class template: `?begin@?$vector@HV?$allo
# cator@H@std@@@std@@QAEPAHXZ` -> member `begin`, template `vector`,
# enclosing namespace `std`. The lazy middle skips the argument list
# without parsing it; a shape this does not match simply fails to join
# (a missed rename, never a wrong one).
TEMPLATE_MEMBER_RE = re.compile(r"^\?(\w+)@\?\$(\w+)@.*?@(\w+)@@")
IDENT_RE = re.compile(r"[^0-9A-Za-z_]+")
COMPGEN_KINDS = {"STATIC_INIT_DISPATCH", "STATIC_ATEXIT", "STATIC_DTOR",
                 "STATIC_CTOR", "SCALAR_DELETING_DTOR"}


def mask_lexical_noise(blob: str) -> str:
    """Blank comments and string/char literals byte-for-byte (newlines
    kept) so the macro regexes can never fire inside them. Ported from
    homm2's annotated_data._mask_lexical_noise."""
    out = list(blob)
    i, n = 0, len(blob)
    state = None  # None | "line" | "block" | '"' | "'"
    while i < n:
        c = blob[i]
        if state is None:
            if c == "/" and i + 1 < n and blob[i + 1] == "/":
                state = "line"
                out[i] = out[i + 1] = " "
                i += 2
                continue
            if c == "/" and i + 1 < n and blob[i + 1] == "*":
                state = "block"
                out[i] = out[i + 1] = " "
                i += 2
                continue
            if c == '"':
                state = c
                i += 1
                continue
            if c == "'":
                # enter char-literal state only when it closes nearby: the
                # carcass carries MSVC `scalar deleting destructor' names
                # whose lone apostrophe would otherwise swallow the file
                closer = blob.find("'", i + 1, i + 5)
                if closer != -1 and "\n" not in blob[i:closer]:
                    state = c
                i += 1
                continue
            i += 1
            continue
        if state == "line":
            if c == "\n":
                state = None
            else:
                out[i] = " "
            i += 1
            continue
        if state == "block":
            if c == "*" and i + 1 < n and blob[i + 1] == "/":
                out[i] = out[i + 1] = " "
                state = None
                i += 2
                continue
            if c != "\n":
                out[i] = " "
            i += 1
            continue
        # string/char literal
        if c == "\\" and i + 1 < n:
            out[i] = out[i + 1] = " "
            i += 2
            continue
        if c == state:
            state = None
        elif c != "\n":
            out[i] = " "
        i += 1
    return "".join(out)


def rva_of(addr_text: str, where: str) -> int:
    value = int(addr_text, 16)
    if value < common.IMAGE_BASE:
        common.die(f"{where}: address {addr_text} below image base - the "
                   "v2 contract uses ABSOLUTE VAs")
    return value - common.IMAGE_BASE


def scan_file(path, functions: set[int]) -> list[dict]:
    """All annotation rows of one src file, in scan (line) order. Names are
    the RAW pre-join spellings; `channel` is the pre-join provenance."""
    unit = path.stem
    rows = []
    text = mask_lexical_noise(path.read_text(errors="replace"))
    lines = text.splitlines()
    for index, line in enumerate(lines):
        where = f"{path.name}:{index + 1}"
        m = VA_RE.match(line)
        if m:
            rva = rva_of(m.group(1), where)
            if rva not in functions:
                common.die(f"{where}: VA {m.group(1)} is not a carved "
                           "function entry")
            declared = int(m.group(2), 0)
            follower = next((l for l in lines[index + 1:index + 4]
                             if l.strip()
                             and not ANNOTATION_RE.match(l)), None)
            if follower is None:
                common.die(f"{where}: orphan VA annotation - no "
                           "declaration follows")
            sm = SPECIAL_RE.search(follower)
            if sm:
                raw = f"{sm.group(1)}__{sm.group(2)}"
            else:
                # full C++ declarator parsing is a tar pit (templates,
                # operator=, MSVC spellings); everything before the
                # first paren is a stable working label until a
                # clang-IR channel binds real mangled names
                raw = follower.split("(", 1)[0]
                # `std::vector<int>::begin` -> `std::vector::begin`:
                # without this the `<int>` breaks the qualified-name
                # run and only `::begin` survives, which no mangled
                # key can join.
                while TEMPLATE_ARGS_RE.search(raw):
                    raw = TEMPLATE_ARGS_RE.sub("", raw)
                last = DECLARATOR_RE.findall(raw + "(")
                raw = last[-1] if last else raw
            name = IDENT_RE.sub("_", raw).strip("_")[:64]
            if not name:
                name = f"fn_{rva:x}"
            rows.append({"rva": rva, "unit": unit, "size": declared,
                         "kind": "func", "name": name,
                         "channel": "src-VA",
                         # ctors and dtors collapse to the same
                         # class_class label; the tilde in the
                         # declarator is the only discriminator left
                         "dtor": "~" in raw})
            continue
        m = VA_COMPGEN_RE.match(line)
        if m:
            rva = rva_of(m.group(1), where)
            if m.group(3) not in COMPGEN_KINDS:
                common.die(f"{where}: unknown VA_COMPGEN kind "
                           f"{m.group(3)}")
            name = f"__h3cg${unit}${m.group(3).lower()}${m.group(4)}"
            rows.append({"rva": rva, "unit": unit,
                         "size": int(m.group(2), 0), "kind": "func",
                         "name": name,
                         "channel": "src-VA_COMPGEN",
                         "ckind": m.group(3),
                         "owner": m.group(4)})
            continue
        for m in DATA_COMPGEN_GUARD_RE.finditer(line):
            name = f"__h3cg${unit}$static_init_guard${m.group(2)}"
            rows.append({"rva": rva_of(m.group(1), where),
                         "unit": unit, "size": 4, "kind": "data",
                         "name": name,
                         "channel": "src-DATA_COMPGEN_GUARD"})
        for m in DATA_COMPGEN_RE.finditer(line):
            if DATA_COMPGEN_GUARD_RE.search(line):
                continue
            name = f"__h3cg${unit}$data${m.group(2)}"
            rows.append({"rva": rva_of(m.group(1), where),
                         "unit": unit, "size": "", "kind": "data",
                         "name": name,
                         "channel": "src-DATA_COMPGEN"})
        for m in DATA_RE.finditer(line):
            rows.append({"rva": rva_of(m.group(1), where),
                         "unit": unit, "size": "", "kind": "data",
                         "name": f"data_{rva_of(m.group(1), where):x}",
                         "channel": "src-DATA"})
    return rows


def _demangle_key(mangled: str):
    """Normalized join key for one MSVC public name: ?Method@Class@@... ->
    class_method, matching scan_file's declarator spelling (:: -> _).
    Ctors (??0) key as class_class - the same collapse the declarator
    scan produces for `armyGroup::armyGroup`; dtors (??1) key as
    class_class@dtor so an overloaded-ctor group never absorbs its
    dtor. Assignment (??4) keys to the declarator scanner's stable
    `Class_Class_operator` spelling; other special operators return None."""
    if mangled.startswith("??_G"):
        # scalar deleting destructor - joined by the VA_COMPGEN
        # SCALAR_DELETING_DTOR claims (owner = the class)
        cls = mangled[4:].split("@@", 1)[0].split("@")[0]
        return f"{cls}_{cls}".lower() + "@gdtor"
    if mangled.startswith("??0") or mangled.startswith("??1"):
        cls = mangled[3:].split("@@", 1)[0].split("@")[0]
        key = f"{cls}_{cls}".lower()
        return f"{key}@dtor" if mangled.startswith("??1") else key
    if mangled.startswith("??4"):
        cls = mangled[3:].split("@@", 1)[0].split("@")[0]
        return f"{cls}_{cls}_operator".lower()
    m = TEMPLATE_MEMBER_RE.match(mangled)
    if m:
        # member of a class template: namespace_template_member, the same
        # spelling scan_file derives from `std::vector<int>::begin`
        # once TEMPLATE_ARGS_RE has dropped the argument list
        return f"{m.group(3)}_{m.group(2)}_{m.group(1)}".lower()
    if not mangled.startswith("?") or mangled.startswith("??"):
        # C-mangled stdcall/fastcall publics: _name@N / @name@N -> name
        # (extern "C" __stdcall in a game TU; first hit _WinMain@16).
        # Plain cdecl _name is left alone - no claim needs it yet.
        m = re.match(r"^[_@]([A-Za-z_$][\w$]*)@\d+$", mangled)
        if m:
            return m.group(1).lower()
        return None
    components = mangled[1:].split("@@", 1)[0].split("@")
    qualified = list(reversed(components[1:])) + [components[0]]
    return "_".join(qualified).lower()


def _base_authority_names(unit: str) -> dict:
    """key -> [(mangled, content_size)...] defined text symbols (external
    or file-static function) of the
    unit's compiled base obj, each key's list in DEFINITION order (COFF
    section order - VC6 emits one COMDAT per function in source order,
    so an overload group's order matches the claims' rva order).
    content_size is the symbol's section raw size minus the trailing
    0x90 COMDAT-alignment fill (same rule the comparison normalization
    applies) - the discriminator for overload groups that carry
    unclaimed members retail dropped."""
    obj = common.HOMM3_DIR / f"build/objdiff/base/{unit}.obj"
    if not obj.is_file():
        return {}
    data = obj.read_bytes()
    nsec, = struct.unpack_from("<H", data, 2)
    section_sizes = {}
    for index in range(nsec):
        header = 20 + index * 40
        raw_size, raw_offset = struct.unpack_from("<II", data, header + 16)
        content = raw_size
        if raw_offset:
            raw = data[raw_offset:raw_offset + raw_size]
            run = 0
            while run < len(raw) and run < 15 and raw[len(raw) - 1 - run] == 0x90:
                run += 1
            content = raw_size - run
        section_sizes[index + 1] = content
    symoff, nsyms = struct.unpack_from("<II", data, 8)
    strtab = symoff + nsyms * 18
    def symname(o):
        if struct.unpack_from("<I", data, o)[0] == 0:
            so = struct.unpack_from("<I", data, o + 4)[0]
            return data[strtab + so:data.index(b"\0", strtab + so)].decode(
                errors="replace")
        return data[o:o + 8].rstrip(b"\0").decode(errors="replace")
    ordered = []
    o, i = symoff, 0
    while i < nsyms:
        section = struct.unpack_from("<h", data, o + 12)[0]
        storage = data[o + 16]
        # Defined externals, plus file-static FUNCTIONS (storage 3 with
        # a C++-mangled name; first: monframeinfo's static
        # InitializeCreatureAnimationTraits, which retail keeps static -
        # the mangled spelling is still the true pairing name). Static
        # DATA never mangles (`_name`), so _demangle_key drops it.
        if storage in (2, 3) and section > 0:
            name = symname(o)
            key = _demangle_key(name)
            if key:
                ordered.append((section, key, name,
                                section_sizes.get(section, 0)))
        aux = data[o + 17]
        o += 18 * (1 + aux)
        i += 1 + aux
    groups = {}
    for _section, key, name, content in sorted(ordered):
        groups.setdefault(key, []).append((name, content))
    return groups


def join_unit(unit: str, rows: list[dict]) -> None:
    """The base-obj name-authority join, in place: a compiled unit's public
    symbols carry the TRUE MSVC spellings; uniquely-joined claims adopt
    them (channel src-VA+base). Keys are built from RAW names - equivalent
    to the monolith's post-dedup key stripping, since a `_<rva>` suffix
    always stripped back to the raw spelling before keying."""
    unit_rows = [r for r in rows
                 if r["channel"] == "src-VA"
                 or (r["channel"] == "src-VA_COMPGEN"
                     and "$scalar_deleting_dtor$" in r["name"])]
    if not unit_rows:
        return
    authority = _base_authority_names(unit)
    if not authority:
        return
    dtor_rvas = {r["rva"] for r in rows if r.get("dtor")}
    claim_keys = {}
    for row in unit_rows:
        if "$scalar_deleting_dtor$" in row["name"]:
            # ??_G claims join the base publics like source functions do
            owner = row["name"].rsplit("$", 1)[1].lower()
            claim_keys.setdefault(f"{owner}_{owner}@gdtor",
                                  []).append(row)
            continue
        key = row["name"].lower()
        if row["rva"] in dtor_rvas:
            key = f"{key}@dtor"
        claim_keys.setdefault(key, []).append(row)
    for key, mangled_group in authority.items():
        candidates = claim_keys.get(key)
        if not candidates:
            continue  # unimplemented group: leave labeled
        if len(candidates) == len(mangled_group):
            # overload groups zip in order: claims by rva (link
            # order), mangled names by COFF section (definition
            # order) - the same order for a VC6 TU
            for row, (mangled, _content) in zip(
                    sorted(candidates, key=lambda r: r["rva"]),
                    mangled_group):
                row["joined"] = mangled
                row["channel"] = "src-VA+base"
            continue
        # count mismatch: the base emits overloads retail dropped
        # (/Ob2 keeps every definition, OPT:REF discarded the
        # unreferenced ones). Pair by EXACT content size, and only
        # when the assignment is unambiguous both ways.
        for row in candidates:
            fits = [name for name, content in mangled_group
                    if content == row["size"]]
            if len(fits) != 1:
                continue
            mangled = fits[0]
            claim_fits = [r for r in candidates
                          if any(c == r["size"]
                                 for n, c in mangled_group
                                 if n == mangled)]
            if len(claim_fits) != 1:
                continue
            row["joined"] = mangled
            row["channel"] = "src-VA+base"


def _fragment_rows(rows: list[dict]) -> list[list[str]]:
    out = []
    for r in rows:
        size = r["size"]
        out.append([f"0x{r['rva']:x}",
                    f"0x{size:x}" if isinstance(size, int) else "",
                    r.get("joined", r["name"]), r["kind"], r["channel"],
                    r["name"], "1" if r.get("dtor") else "",
                    r.get("ckind", ""), r.get("owner", "")])
    return out


def src_files() -> list:
    """The extraction universe: sorted src/*.c*, one unit per stem. A stem
    collision would silently merge two files' claims into one fragment -
    fatal, has never existed."""
    paths = sorted(SRC_DIR.glob("*.c*"))
    stems = [p.stem for p in paths]
    for stem in stems:
        if stems.count(stem) > 1:
            common.die(f"src/ stem collision: two files share unit {stem!r}")
    return paths


def run(only_units: list[str] | None = None) -> tuple[list[str], list[str]]:
    """Extract fragments; returns (changed units, pruned fragments).
    Fragment writes are content-idempotent so an unchanged TU never
    dirties downstream freshness probes."""
    paths = src_files()
    known = {p.stem for p in paths}
    if only_units is not None:
        for name in only_units:
            if name not in known:
                raise SystemExit(f"[labels] unknown unit {name!r} - units "
                                 f"are src/ file stems, e.g. 'advmgr'")
    functions = {r["rva"] for r in _census_functions()}
    changed, pruned = [], []
    for path in paths:
        unit = path.stem
        if only_units is not None and unit not in only_units:
            continue
        rows = scan_file(path, functions)
        join_unit(unit, rows)
        banner = [f"# GENERATED claim fragment for unit {unit} - the macros "
                  f"in src/{path.name} are the storage; do not edit."]
        if write_tsv(fragment_path(unit), banner, HEADER,
                     _fragment_rows(rows)):
            changed.append(unit)
    if only_units is None and FRAGMENTS.is_dir():
        for stale in sorted(FRAGMENTS.glob("*.tsv")):
            if stale.stem not in known:
                stale.unlink()
                pruned.append(stale.stem)
    return changed, pruned


def _census_functions():
    from homm3.retail_labels import censuses
    return censuses.functions()


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="homm3 labels", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit", action="append",
                    help="extract one unit (repeatable)")
    ap.add_argument("--all", action="store_true",
                    help="extract every src/ unit")
    a = ap.parse_args(argv)
    if not a.unit and not a.all:
        ap.error("pick --unit U or --all")
    changed, pruned = run(a.unit if not a.all else None)
    print(f"[labels] {len(changed)} fragment(s) changed"
          + (f", {len(pruned)} pruned" if pruned else "")
          + f" -> {FRAGMENTS.relative_to(common.HOMM3_DIR)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
