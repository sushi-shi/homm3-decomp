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

THE NAME BINDING (extraction's second, join-bearing concern - as in
gruntz). Two mechanisms, in strict precedence:

  src-VA+ir    clang IR (homm3.core.clang). `@llvm.global.annotations`
               pairs the VA() string DIRECTLY with the function's
               MSVC-mangled symbol, so there is no positional join - an
               inline header definition cannot steal a nearby address -
               and no lossy demangle key, so two functions that normalize
               to one spelling cannot swap names. The proposed name is
               then CONFIRMED against cl's own base obj: the exact string
               must be a defined symbol there. cl's object is therefore a
               CONFIRMER, never an answerer - a stale artifact can no
               longer supply a name the current source has stopped
               producing; it can only fail to confirm, which is reported.

  src-VA+base  the lexical declarator + base-obj key join, kept for the
               claims IR structurally cannot reach: the `#if 0 //
               @carcass` claim-only stubs whose real definition lives in
               a header or in the STL (no compiler sees those bodies) and
               any TU clang cannot parse. This is the mechanism the IR
               channel exists to retire, so what it still binds is
               counted and reported rather than assumed sound.

The fragment keeps the RAW declarator name alongside the bound spelling
because the model's scan-order dedup replays over raw names, exactly as
the monolith did.

Fragments (build/gen/claims/<unit>.tsv) are written atomically and
content-idempotently; a stale fragment whose src file vanished is pruned
on a full run. Fragment freshness follows the base objs: run after a
build (the `homm3 delink` chain does).
"""

from __future__ import annotations

import os
import re
import struct
import sys

from homm3.core import clang, common
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


# --- the IR channel ---------------------------------------------------
# `@llvm.global.annotations` is an appending array of tuples whose first
# two members are the annotated symbol and the annotation string; the
# strings themselves are `@.str* = ... c"..."` constants above it.
IR_STR_DEF_RE = re.compile(r'^(@[\w.$"]+)\s*=.*?\bc"((?:[^"\\]|\\.)*)"', re.M)
IR_ANN_TUPLE_RE = re.compile(
    r'\{\s*ptr\s+(@(?:"[^"]+"|[\w.$]+))\s*,\s*ptr\s+(@(?:"[^"]+"|[\w.$]+))\s*,')
IR_VA_ANN_RE = re.compile(r"^va:(0x[0-9a-fA-F]+) size:(?:0x[0-9a-fA-F]+|\d+)$")


def _unescape_ir_cstr(text: str) -> str:
    out = bytearray()
    i = 0
    while i < len(text):
        if (text[i] == "\\" and len(text) - i >= 3
                and all(c in "0123456789abcdefABCDEF"
                        for c in text[i + 1:i + 3])):
            out.append(int(text[i + 1:i + 3], 16))
            i += 3
        else:
            out.append(ord(text[i]))
            i += 1
    if out and out[-1] == 0:
        out.pop()
    return out.decode("utf-8", "replace")


def _ir_symbol_name(ref: str) -> str:
    """`@"?Value@Widget@@QBEHH@Z"` / `@_foo` -> the bare symbol. A `\\01`
    prefix means clang already wrote the final object name."""
    ref = ref[1:]
    if ref.startswith('"') and ref.endswith('"'):
        ref = ref[1:-1]
    return ref[3:] if ref.startswith("\\01") else ref


def ir_va_names(ir: str) -> dict:
    """{rva: mangled name} from the TU's IR - each pair produced by the
    compiler itself, never by a scan of the text around the macro."""
    strings = {m.group(1): _unescape_ir_cstr(m.group(2))
               for m in IR_STR_DEF_RE.finditer(ir)}
    out = {}
    for line in ir.splitlines():
        if "@llvm.global.annotations" not in line:
            continue
        for sym_ref, str_ref in IR_ANN_TUPLE_RE.findall(line):
            annotation = strings.get(str_ref)
            if annotation is None:
                continue
            m = IR_VA_ANN_RE.match(annotation)
            if m:
                rva = int(m.group(1), 16) - common.IMAGE_BASE
                out[rva] = _ir_symbol_name(sym_ref)
    return out


def unit_ir_names(path) -> dict | None:
    """The unit's IR name map, or None when clang could not read the TU
    (no toolchain, or a source construct cl accepts and clang does not).
    None is always reported by the caller - a silent empty map would look
    exactly like a TU with no claims."""
    ir = clang.emit_ir(path)
    return None if ir is None else ir_va_names(ir)


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


def ir_bind(unit: str, rows: list[dict], ir_names: dict,
            problems: list[str]) -> set:
    """Bind VA() claims to the mangled names clang paired them with, in
    place; returns the mangled names taken (which the lexical join must
    then leave alone).

    cl's own obj CONFIRMS: the exact string clang proposed must be a
    defined symbol there. Confirmation is what makes the mirror and the
    two compilers' manglers self-checking - and it is one-directional, so
    a stale obj can only fail to confirm, never answer with a name the
    source has stopped producing."""
    content = {name: size
               for group in _base_authority_names(unit).values()
               for name, size in group}
    taken = set()
    for row in rows:
        if row["channel"] != "src-VA":
            continue
        mangled = ir_names.get(row["rva"])
        if mangled is None:
            continue          # not compiled here (a `#if 0` carcass stub)
        if content and mangled not in content:
            # The obj contradicts the compiler's own pairing, so it is the
            # LAST thing that may name this claim: handing the row to the
            # lexical key join would let that same disputed object answer
            # by a weaker match. The claim keeps its raw declarator label
            # and the disagreement is stated.
            row["ir_unconfirmed"] = True
            problems.append(
                f"{unit}: VA(0x{row['rva'] + common.IMAGE_BASE:08x}) - clang "
                f"names it {mangled!r}, which {unit}.obj does not define; "
                f"the object may be stale, or the two manglers disagree. "
                f"Claim left UNJOINED on its raw declarator label.")
            continue
        row["joined"] = mangled
        row["channel"] = "src-VA+ir"
        taken.add(mangled)
        _report_size_mismatch(unit, row, mangled, content, problems)
    return taken


#: A claim whose retail extent dwarfs the compiled body of the symbol the
#: annotation actually landed on. Measured spread over the 1396 bound
#: claims: 1240 are size-EQUAL (the byte-exact ones) and the widest
#: legitimate gap is 2.2x (`game::Save`, a reconstruction still short of
#: retail). 4x is therefore comfortably outside the real distribution and
#: far below a mis-binding: the mapcell incident of 2026-08-20 put a
#: 0x1e3-byte claim on a small local helper, ~10x.
SIZE_MISMATCH_RATIO = 4


def _report_size_mismatch(unit, row, mangled, content, problems) -> None:
    """The cross-check the IR channel makes possible at all.

    Knowing WHICH symbol the annotation landed on lets us ask cl how big
    that symbol's body is. It catches the one mis-binding IR cannot
    prevent: a definition written BETWEEN the VA() line and the declarator
    it was meant to annotate. A leading `__attribute__` binds to the next
    declaration, so clang attaches the annotation to the intervening
    definition exactly as the lexical follower scan does - the source
    genuinely says the helper is annotated, and no reader can know
    otherwise. The retail extent can: a claim sized for a real function
    does not fit a helper."""
    compiled = content.get(mangled)
    if not compiled or not isinstance(row["size"], int):
        return
    if row["size"] < compiled * SIZE_MISMATCH_RATIO:
        return
    problems.append(
        f"{unit}: VA(0x{row['rva'] + common.IMAGE_BASE:08x}) claims "
        f"0x{row['size']:x} retail bytes but the symbol it annotates, "
        f"{mangled!r}, compiles to only 0x{compiled:x} - "
        f"{row['size'] / compiled:.0f}x. Check for a definition written "
        f"between the VA() line and its intended declarator (the "
        f"annotation binds to whatever declaration FOLLOWS it).")


def join_unit(unit: str, rows: list[dict], taken: set | None = None) -> None:
    """The base-obj name-authority join, in place: a compiled unit's public
    symbols carry the TRUE MSVC spellings; uniquely-joined claims adopt
    them (channel src-VA+base). Keys are built from RAW names - equivalent
    to the monolith's post-dedup key stripping, since a `_<rva>` suffix
    always stripped back to the raw spelling before keying.

    Runs only over what the IR channel did not bind. `taken` names are
    removed from the authority groups: a symbol clang already paired with
    its own claim must not also be offered to a second, weaker key match."""
    taken = taken or set()
    unit_rows = [r for r in rows
                 if not r.get("ir_unconfirmed")
                 and (r["channel"] == "src-VA"
                      or (r["channel"] == "src-VA_COMPGEN"
                          and "$scalar_deleting_dtor$" in r["name"]))]
    if not unit_rows:
        return
    authority = {key: [(n, c) for n, c in group if n not in taken]
                 for key, group in _base_authority_names(unit).items()}
    authority = {key: group for key, group in authority.items() if group}
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


def _extract_one(path, functions: set, ir_names: dict | None,
                 problems: list[str]) -> list[dict]:
    """One unit's rows, IR-bound where clang reached the TU and lexically
    joined for the rest."""
    unit = path.stem
    rows = scan_file(path, functions)
    taken = set()
    if ir_names is not None:
        taken = ir_bind(unit, rows, ir_names, problems)
    join_unit(unit, rows, taken)
    return rows


def run(only_units: list[str] | None = None,
        jobs: int | None = None) -> tuple[list[str], list[str], list[str]]:
    """Extract fragments; returns (changed units, pruned fragments,
    problems). Fragment writes are content-idempotent so an unchanged TU
    never dirties downstream freshness probes.

    The clang probes are independent per TU and each costs about 0.4 s, so
    they run in a thread pool; everything after them is per-unit local."""
    from concurrent.futures import ThreadPoolExecutor

    paths = src_files()
    known = {p.stem for p in paths}
    if only_units is not None:
        for name in only_units:
            if name not in known:
                raise SystemExit(f"[labels] unknown unit {name!r} - units "
                                 f"are src/ file stems, e.g. 'advmgr'")
    functions = {r["rva"] for r in _census_functions()}
    todo = [p for p in paths
            if only_units is None or p.stem in only_units]
    changed, pruned, problems = [], [], []

    if clang.clang_bin() is None:
        problems.append("clang is not on PATH and $HOMM3_CLANG is unset - "
                        "every claim falls back to the lexical declarator "
                        "join; run inside `nix develop`")
        ir_maps = [None] * len(todo)
    else:
        clang.mirror()          # once, before the pool shares it
        workers = jobs or min(16, (os.cpu_count() or 4))
        with ThreadPoolExecutor(max_workers=workers) as pool:
            ir_maps = list(pool.map(unit_ir_names, todo))

    no_ir = []
    for path, ir_names in zip(todo, ir_maps):
        if ir_names is None:
            no_ir.append(path.stem)
        rows = _extract_one(path, functions, ir_names, problems)
        banner = [f"# GENERATED claim fragment for unit {path.stem} - the "
                  f"macros in src/{path.name} are the storage; do not edit."]
        if write_tsv(fragment_path(path.stem), banner, HEADER,
                     _fragment_rows(rows)):
            changed.append(path.stem)
    if no_ir:
        problems.append(
            f"{len(no_ir)} TU(s) clang could not read, so their claims keep "
            f"the lexical declarator join: {', '.join(sorted(no_ir))}")
    if only_units is None and FRAGMENTS.is_dir():
        for stale in sorted(FRAGMENTS.glob("*.tsv")):
            if stale.stem not in known:
                stale.unlink()
                pruned.append(stale.stem)
    return changed, pruned, problems


MACRO_SITE_RE = re.compile(
    r"\b(VA_COMPGEN|DATA_COMPGEN_GUARD|DATA_COMPGEN|VA|DATA)"
    r"\s*\(\s*(0x[0-9a-fA-F]+)")


def sweep_sites() -> dict:
    """Tree-wide macro-site census over src/ + include/ (comments blanked,
    va.h's own #defines excluded): {macro: {rva: ['file:line', ...]}}."""
    out: dict = {}
    for base in ("src", "include"):
        root = common.HOMM3_DIR / base
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix not in (".c", ".cpp", ".cxx", ".h") \
                    or path.name == "va.h":
                continue
            text = mask_lexical_noise(path.read_text(errors="replace"))
            for m in MACRO_SITE_RE.finditer(text):
                lineno = text.count("\n", 0, m.start()) + 1
                rva = int(m.group(2), 16) - common.IMAGE_BASE
                out.setdefault(m.group(1), {}).setdefault(rva, []).append(
                    f"{path.relative_to(common.HOMM3_DIR)}:{lineno}")
    return out


def check_completeness() -> list[str]:
    """The oracle OVER extraction: every macro site in the tree is either
    carried by a fragment or named here as lost.

    The lexical scan reads src/*.c* only, so a claim written in a HEADER
    reaches no fragment at all and its address silently keeps a dense
    working label - the failure class gruntz's own sweep exists to catch,
    and the one this tree still has (include/game.h). Reported, not fatal:
    the fix is a source edit that moves the claim onto a definition."""
    from homm3.retail_labels.fragments import all_claims
    sites = sweep_sites()
    have: dict = {}
    for claim in all_claims():
        have.setdefault(claim.kind, set()).add(claim.rva)
    problems = []
    for macro, kind in (("VA", "func"), ("VA_COMPGEN", "func"),
                        ("DATA", "data"), ("DATA_COMPGEN", "data"),
                        ("DATA_COMPGEN_GUARD", "data")):
        for rva, wheres in sorted(sites.get(macro, {}).items()):
            if rva in have.get(kind, set()):
                continue
            problems.append(
                f"{macro}(0x{rva + common.IMAGE_BASE:08x}) at {wheres[0]} is "
                f"in NO fragment - extraction reads src/*.c* only, so this "
                f"claim is a silently lost label")
    return problems


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
    ap.add_argument("-j", "--jobs", type=int, default=None,
                    help="clang probe parallelism (default: cpu count)")
    a = ap.parse_args(argv)
    if not a.unit and not a.all:
        ap.error("pick --unit U or --all")
    changed, pruned, problems = run(a.unit if not a.all else None, a.jobs)
    for problem in problems:
        print(f"[labels] {problem}", file=sys.stderr)
    if a.unit is None:
        for problem in check_completeness():
            print(f"[labels] {problem}", file=sys.stderr)
    print(f"[labels] {len(changed)} fragment(s) changed"
          + (f", {len(pruned)} pruned" if pruned else "")
          + f" -> {FRAGMENTS.relative_to(common.HOMM3_DIR)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
