#!/usr/bin/env python3
"""homm3.build.labels - derive build/gen/symbol_names.csv, the synth-PDB
inventory, from source annotations + the admitted config maps.

Schema (homm2 shape): rva,name,unit,size,kind,provenance - one row per
function in the executable plus the data rows the delinker needs. Sources,
in authority order:

  src-VA           `VA(0x<va>, 0x<size>)` macros scanned lexically from
                   src/ (include/va.h contract v2). The function name is
                   derived from the DECLARATOR that follows the annotation
                   (source-as-authority); the clang-IR path that binds real
                   mangled names arrives with the first compiling game TU.
                   The full macro family - VA_COMPGEN, DATA,
                   DATA_COMPGEN(_GUARD) - is scanned NOW (lesson 1)
                   even though src/ carries only VA today.
  zlib-map         config/retail-zlib-map.tsv, unit `zlib`.
  runtime-map      config/retail-runtime-map.tsv, empty unit - vostok
                   buckets these into _msvc_internal objects, which is
                   correct: runtime code is never a reconstruction target.
  reloc-alias      reviewed owner symbols from
                   config/delink-reloc-aliases.tsv. The synthetic PDB must
                   declare these data owners before vostok can apply the
                   alias manifest.
  working-label    every remaining function of the universe
                   (config/retail-functions.tsv), bucketed `seg_%04x`
                   gruntz-style. Named by the naming layer where
                   evidence/retail-symbols.csv is still present -
                   evidence/ is SCAFFOLDING slated for removal (user
                   decision 2026-08-04), so this is enrichment only:
                   without it the name falls back to `fn_<rva>`, and
                   nothing else in the pipeline reads evidence/.
  data rows        vtable starts (config/retail-vtables.tsv; `??_7X@@6B@`
                   where the hand-admitted class column names the vtable -
                   the census is MANUAL, source VTBL() macros are retired
                   [user decision 2026-08-06]; evidence enrichment fills
                   the rest), IAT slots (parsed from
                   the import directory; `__imp_` spellings are PLACEHOLDER
                   undecorated names until import-lib proof), and every
                   absolute-relocation target in .rdata/.data
                   (config/retail-reloc-evidence.tsv) as
                   const_/data_/bss_<rva> - dense naming, required because
                   vostok panics on an .rdata target below every named
                   constant and skips targets outside known symbol sizes.

Gates (all fatal): VA addresses must be absolute (>= image base) and land
on a carved function entry; an annotation must be followed by a
declaration (the orphan-annotation incident from gruntz); no claim may
name a volatile `$E<n>` compiler ordinal; duplicate rvas and duplicate
names refuse to write.
"""
from __future__ import annotations

import csv
import re
import struct
import sys
from pathlib import Path

from homm3.core import common

OUT = common.HOMM3_DIR / "build/gen/symbol_names.csv"
COMPGEN_OUT = common.HOMM3_DIR / "build/gen/compgen_claims.tsv"
SRC_DIR = common.HOMM3_DIR / "src"
ZLIB_MAP = common.HOMM3_DIR / "config/retail-zlib-map.tsv"
RUNTIME_MAP = common.HOMM3_DIR / "config/retail-runtime-map.tsv"
FUNCTIONS = common.HOMM3_DIR / "config/retail-functions.tsv"
VTABLES = common.HOMM3_DIR / "config/retail-vtables.tsv"
RELOC_EVIDENCE = common.HOMM3_DIR / "config/retail-reloc-evidence.tsv"
RELOC_ALIASES = common.HOMM3_DIR / "config/delink-reloc-aliases.tsv"
SYMBOLS = common.EVIDENCE_DIR / "retail-symbols.csv"
VTABLE_SYMBOLS = common.EVIDENCE_DIR / "retail-vtable-symbols.csv"

IMAGE_BASE = 0x400000
BUCKET_SHIFT = 16

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
VOLATILE_E_RE = re.compile(r"^_?\$E[0-9]+$")
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


def read_tsv_body(path):
    rows = [line.rstrip("\n").split("\t") for line in path.open()
            if not line.startswith("#")]
    return rows[0], rows[1:]


def load_csv(path):
    with path.open() as fh:
        return list(csv.DictReader(
            line for line in fh if not line.startswith("#")))


def rva_of(addr_text: str, where: str) -> int:
    value = int(addr_text, 16)
    if value < IMAGE_BASE:
        common.die(f"{where}: address {addr_text} below image base - the "
                   "v2 contract uses ABSOLUTE VAs")
    return value - IMAGE_BASE


def scan_sources(functions):
    """All annotation rows from src/ (lexical pass)."""
    rows = []
    for path in sorted(SRC_DIR.glob("*.c*")):
        unit = path.stem
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
                    # first paren is a stable working label until the
                    # clang-IR path binds real mangled names
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
                             "provenance": "src-VA",
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
                             "provenance": "src-VA_COMPGEN",
                             "compgen_kind": m.group(3),
                             "owner": m.group(4)})
                continue
            for m in DATA_COMPGEN_GUARD_RE.finditer(line):
                name = f"__h3cg${unit}$static_init_guard${m.group(2)}"
                rows.append({"rva": rva_of(m.group(1), where),
                             "unit": unit, "size": 4, "kind": "data",
                             "name": name,
                             "provenance": "src-DATA_COMPGEN_GUARD"})
            for m in DATA_COMPGEN_RE.finditer(line):
                if DATA_COMPGEN_GUARD_RE.search(line):
                    continue
                name = f"__h3cg${unit}$data${m.group(2)}"
                rows.append({"rva": rva_of(m.group(1), where),
                             "unit": unit, "size": "", "kind": "data",
                             "name": name,
                             "provenance": "src-DATA_COMPGEN"})
            for m in DATA_RE.finditer(line):
                rows.append({"rva": rva_of(m.group(1), where),
                             "unit": unit, "size": "", "kind": "data",
                             "name": f"data_{rva_of(m.group(1), where):x}",
                             "provenance": "src-DATA"})
    return rows


_IMPLIB_DECORATIONS = None


def implib_decorations() -> dict:
    """Import-lib PROOF for stdcall decoration: import-directory name ->
    full __imp_ symbol, read from the VC6 toolchain import libraries'
    archive symbol tables (the linker generation that built retail).
    A name whose libraries disagree on decoration stays unproven."""
    global _IMPLIB_DECORATIONS
    if _IMPLIB_DECORATIONS is not None:
        return _IMPLIB_DECORATIONS
    out, ambiguous = {}, set()
    libdir = common.HOMM3_DIR / "build/homm3-toolchain-vc6-sp3/msvc/lib"
    for lib in sorted(libdir.glob("*.LIB")) if libdir.is_dir() else []:
        data = lib.read_bytes()
        if not data.startswith(b"!<arch>\n"):
            continue
        try:
            size = int(data[8 + 48:8 + 58].split()[0])
            count = int.from_bytes(data[68:72], "big")
        except (ValueError, IndexError):
            continue
        blob = data[72 + 4 * count:8 + 60 + size]
        for raw_sym in blob.split(b"\0")[:count]:
            sym = raw_sym.decode("latin-1")
            if not sym.startswith("__imp__") or "@" not in sym:
                continue
            key = sym[7:].rsplit("@", 1)[0]
            if out.setdefault(key, sym) != sym:
                ambiguous.add(key)
    for key in ambiguous:
        out.pop(key, None)
    _IMPLIB_DECORATIONS = out
    return out


def iat_slots(exe_path: Path):
    """slot rva -> __imp_ spelling, from the import directory."""
    data = exe_path.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    osz = struct.unpack_from("<H", data, pe + 20)[0]
    sections = []
    for i in range(nsec):
        off = pe + 24 + osz + i * 40
        vs, va, rs, ro = struct.unpack_from("<4I", data, off + 8)
        sections.append((va, max(vs, rs), ro))

    def raw(rva):
        for va, span, ro in sections:
            if va <= rva < va + span:
                return ro + (rva - va)
        common.die(f"import walk: rva 0x{rva:x} in no section")

    imp_rva = struct.unpack_from("<II", data, pe + 24 + 96 + 1 * 8)[0]
    slots = {}
    off = raw(imp_rva)
    while True:
        ilt, _ts, _fc, name_rva, iat = struct.unpack_from("<IIIII",
                                                          data, off)
        if not (ilt or name_rva):
            break
        dll = data[raw(name_rva):raw(name_rva) + 64].split(b"\0")[0] \
            .decode("latin-1")
        entry = raw(ilt)
        index = 0
        while True:
            thunk = struct.unpack_from("<I", data, entry + index * 4)[0]
            if not thunk:
                break
            slot = iat + index * 4
            if thunk & 0x80000000:
                stem = dll.rsplit(".", 1)[0].lower()
                slots[slot] = (f"__imp__{stem}_ordinal_{thunk & 0xFFFF}",
                               "iat-ordinal")
            else:
                name = data[raw(thunk) + 2:raw(thunk) + 2 + 256] \
                    .split(b"\0")[0].decode("latin-1")
                proven = (None if name.startswith("?")
                          else implib_decorations().get(name))
                if proven:
                    slots[slot] = (proven, "iat-implib")
                else:
                    prefix = "__imp_" if name.startswith("?") else "__imp__"
                    slots[slot] = (prefix + name, "iat-undecorated")
            index += 1
        off += 20
    return slots


def _demangle_key(mangled: str):
    """Normalized join key for one MSVC public name: ?Method@Class@@... ->
    class_method, matching scan_sources' declarator spelling (:: -> _).
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
        # spelling scan_sources derives from `std::vector<int>::begin`
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


def main(argv=None) -> int:
    _header, fn_rows = read_tsv_body(FUNCTIONS)
    functions = {int(r[0], 16): int(r[1]) for r in fn_rows}
    # evidence/ is enrichment only (scaffolding, slated for removal)
    labels = ({int(r["rva"], 16): r["name"] for r in load_csv(SYMBOLS)}
              if SYMBOLS.is_file() else {})

    rows = {}       # rva -> row dict (first writer wins per authority order)
    problems = []

    def put(rva, name, unit, size, kind, provenance):
        if VOLATILE_E_RE.match(name):
            common.die(f"0x{rva:x}: claim names volatile compiler ordinal "
                       f"{name!r} - record it as evidence, never a label")
        if rva in rows:
            problems.append(f"duplicate rva 0x{rva:x}: "
                            f"{rows[rva]['name']} vs {name}")
            return
        rows[rva] = {"rva": rva, "name": name, "unit": unit,
                     "size": size, "kind": kind, "provenance": provenance}

    # 1. src annotations (declarator-derived names; disambiguate overloads
    # by rva suffix, matching the naming layer's convention)
    src_rows = scan_sources(functions)
    compgen_rows = [r for r in src_rows
                    if r["provenance"] == "src-VA_COMPGEN"]
    COMPGEN_OUT.parent.mkdir(parents=True, exist_ok=True)
    with COMPGEN_OUT.open("w", newline="") as fh:
        fh.write("# GENERATED: python3 -m homm3.build.labels - source-owned "
                 "compiler-function claims.\n")
        for prov in common.provenance("homm3.build.labels"):
            fh.write(prov + "\n")
        writer = csv.writer(fh, delimiter="\t")
        writer.writerow(["unit", "name", "kind", "owner", "size"])
        for r in sorted(compgen_rows,
                        key=lambda row: (row["unit"], row["rva"])):
            writer.writerow([r["unit"], r["name"], r["compgen_kind"],
                             r["owner"], f"0x{r['size']:x}"])
    dtor_rvas = {r["rva"] for r in src_rows if r.get("dtor")}
    seen_names = set()
    for r in src_rows:
        if r["name"] in seen_names:
            r["name"] = f"{r['name']}_{r['rva']:x}"
        seen_names.add(r["name"])
        put(r["rva"], r["name"], r["unit"], r["size"], r["kind"],
            r["provenance"])

    # 1b. base-obj name authority: a compiled unit's public symbols carry
    # the TRUE MSVC spellings; adopt them for uniquely-joined claims so
    # the delinked target pairs against the base by identical names (the
    # interim binding until the clang-IR labels path lands - P0.2).
    src_by_unit = {}
    for row in rows.values():
        if row["provenance"] == "src-VA":
            src_by_unit.setdefault(row["unit"], []).append(row)
        elif (row["provenance"] == "src-VA_COMPGEN"
              and "$scalar_deleting_dtor$" in row["name"]):
            # ??_G claims join the base publics like source functions do
            src_by_unit.setdefault(row["unit"], []).append(row)
    for unit, unit_rows in src_by_unit.items():
        authority = _base_authority_names(unit)
        if not authority:
            continue
        claim_keys = {}
        for row in unit_rows:
            if "$scalar_deleting_dtor$" in row["name"]:
                owner = row["name"].rsplit("$", 1)[1].lower()
                claim_keys.setdefault(f"{owner}_{owner}@gdtor",
                                      []).append(row)
                continue
            key = row["name"].lower()
            suffix = f"_{row['rva']:x}"
            if key.endswith(suffix):
                key = key[:-len(suffix)]  # step-1 overload dedup suffix
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
                    row["name"] = mangled
                    row["provenance"] = "src-VA+base"
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
                row["name"] = mangled
                row["provenance"] = "src-VA+base"

    # 2. zlib map - the admitted table carries the owning TU (unit column),
    # so the delinked objects pair 1:1 against our compiled base objs
    # (inflate.c.obj vs base/inflate.obj)
    _h, zlib_rows = read_tsv_body(ZLIB_MAP)
    for rva_text, size, name, unit in zlib_rows:
        put(int(rva_text, 16), name, unit, int(size), "func", "zlib-map")

    # 3. runtime map (sizes from the universe)
    _h, runtime_rows = read_tsv_body(RUNTIME_MAP)
    for rva_text, name in runtime_rows:
        rva = int(rva_text, 16)
        put(rva, name, "", functions[rva], "func", "runtime-map")

    # 4. the rest of the universe, working labels, seg buckets
    for rva, size in sorted(functions.items()):
        if rva in rows:
            continue
        put(rva, labels.get(rva, f"fn_{rva:x}"),
            f"seg_{rva >> BUCKET_SHIFT:04x}", size, "func",
            "working-label")

    # 5. data rows -------------------------------------------------------
    image, info = common.load_image()
    secmap = {s.name: s for s in image.sections}
    rdata, dat = secmap[".rdata"], secmap[".data"]

    # Reviewed relocation aliases name the real source-level data owner.
    # Vostok requires that owner to exist in the PDB before it can rewrite
    # an otherwise anonymous stripped-image relocation to it. Multiple
    # reviewed sites may share one owner, but conflicting names at one
    # target are a manifest defect.
    _h, alias_rows = read_tsv_body(RELOC_ALIASES)
    alias_owner_by_target = {}
    for alias in alias_rows:
        target = int(alias[1], 16)
        owner = alias[3]
        prior = alias_owner_by_target.get(target)
        if prior and prior != owner:
            common.die(f"reloc aliases disagree at data rva 0x{target:x}: "
                       f"{prior!r} vs {owner!r}")
        alias_owner_by_target[target] = owner
    for target, owner in sorted(alias_owner_by_target.items()):
        if target in rows:
            if rows[target]["name"] != owner:
                common.die(f"reloc alias owner {owner!r} conflicts with "
                           f"{rows[target]['name']!r} at 0x{target:x}")
            continue
        put(target, owner, "", "", "data", "reloc-alias")

    vt_class = {}
    if VTABLE_SYMBOLS.is_file():  # analysis enrichment for unnamed rows
        for r in load_csv(VTABLE_SYMBOLS):
            if r["class"] and r["class_addr_offset"] == "0":
                vt_class.setdefault(int(r["vtable_rva"], 16), r["class"])
    _h, vt_rows = read_tsv_body(VTABLES)
    # An ADMITTED census name outranks the candidate enrichment: when the
    # hand census places a class, a conflicting enrichment attribution of
    # the SAME class to another rva is dropped (first case: NH3API-derived
    # rows put mouseManager on 0x240038 while the retail ctor at 0x10cb50
    # stores 0x240028 - retail bytes win).
    admitted_names = {row[2] for row in vt_rows if len(row) > 2 and row[2]}
    vt_class = {rva: cls for rva, cls in vt_class.items()
                if cls not in admitted_names}
    for row in vt_rows:
        rva, count = int(row[0], 16), int(row[1])
        if rva in rows:
            continue  # a src claim owns the address
        admitted = row[2] if len(row) > 2 and row[2] else None
        cls = admitted or vt_class.get(rva)
        name = f"??_7{cls}@@6B@" if cls else f"vtbl_{rva:x}"
        put(rva, name, "", count * 4, "data",
            "vtable-name" if admitted else
            ("vtable-class" if cls else "vtable"))

    for slot, (name, provenance) in sorted(iat_slots(
            Path(info["path"])).items()):
        put(slot, name, "", 4, "data", provenance)

    _h, ev_rows = read_tsv_body(RELOC_EVIDENCE)
    col = {c: i for i, c in enumerate(_h)}
    skipped_targets = 0
    for r in ev_rows:
        if r[col["target_class"]] not in ("data", "literal-start",
                                          "literal-interior"):
            continue
        target = int(r[col["value"]], 16) - IMAGE_BASE
        if target in rows:
            continue
        if rdata.rva <= target < rdata.rva + rdata.mapped:
            put(target, f"const_{target:x}", "", "", "data", "reloc-target")
        elif dat.rva <= target < dat.rva + dat.size:
            put(target, f"data_{target:x}", "", "", "data", "reloc-target")
        elif dat.rva + dat.size <= target < dat.rva + dat.mapped:
            put(target, f"bss_{target:x}", "", "", "data", "reloc-target")
        else:
            skipped_targets += 1

    if problems:
        for p in problems[:10]:
            print(f"[build labels] {p}", file=sys.stderr)
        common.die(f"{len(problems)} duplicate-rva claims")
    # global name uniqueness: label-grade names (declarator/working) take
    # an rva suffix on collision; a colliding PROVEN symbol is a defect
    from collections import Counter
    counts = Counter(r["name"] for r in rows.values())
    seen = set()
    for rva in sorted(rows):
        r = rows[rva]
        if counts[r["name"]] > 1 and r["name"] in seen:
            if r["provenance"] not in ("src-VA", "working-label"):
                common.die(f"duplicate proven name {r['name']!r} at "
                           f"0x{rva:x}")
            r["name"] = f"{r['name']}_{rva:x}"
        seen.add(r["name"])
    names = [r["name"] for r in rows.values()]
    if len(set(names)) != len(names):
        common.die("name dedup failed to converge")
    missing = set(functions) - set(rows)
    if missing:
        common.die(f"{len(missing)} functions uncovered - first "
                   f"0x{min(missing):x}")

    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w", newline="") as fh:
        fh.write("# GENERATED: python3 -m homm3.build.labels - the "
                 "synth-PDB inventory.\n")
        for prov in common.provenance("homm3.build.labels"):
            fh.write(prov + "\n")
        writer = csv.writer(fh)
        writer.writerow(["rva", "name", "unit", "size", "kind",
                         "provenance"])
        for rva in sorted(rows):
            r = rows[rva]
            size = (f"0x{r['size']:x}" if isinstance(r["size"], int)
                    else r["size"])
            writer.writerow([f"0x{rva:x}", r["name"], r["unit"], size,
                             r["kind"], r["provenance"]])

    funcs = sum(1 for r in rows.values() if r["kind"] == "func")
    data = len(rows) - funcs
    by_prov = {}
    for r in rows.values():
        by_prov[r["provenance"]] = by_prov.get(r["provenance"], 0) + 1
    print(f"[build labels] {len(rows)} rows ({funcs} func, {data} data) "
          f"-> {OUT}")
    for prov in sorted(by_prov, key=by_prov.get, reverse=True):
        print(f"  {prov}: {by_prov[prov]}")
    if skipped_targets:
        print(f"  reloc targets outside .rdata/.data: {skipped_targets} "
              "skipped")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
