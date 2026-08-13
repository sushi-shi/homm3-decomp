#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ONE-SHOT: decompiler-backed class/struct LAYOUT recovery from HEROES3.EXE.

*** THIS IS A ONE-OFF ANALYSIS SCRIPT, NOT PIPELINE CODE. ***
It is deliberately parked under scripts/archive/ so nobody mistakes it for a live
stage: it has no `homm3` CLI subcommand, no ninja edge, no gate, and nothing in the
build graph depends on it. It was run once to produce the tables under
`evidence/ghidra-structs/`; those tables are the deliverable. If a boundary or naming
input ever changes and the tables must be refreshed, run this file directly again
(`python3 scripts/archive/ghidra_recover_structs/recover_structs.py`) from a dev shell
that provides GHIDRA_INSTALL_DIR + pyghidra. Do not resurrect it as a pipeline stage.

Ported in the user-approved 2026-08-07 attempt-1 session from
`scripts/homm3/ghidra/scripts/recover_structs.py`, adapted to this repo's inputs:
attempt-1 read MSVC-decorated CodeView names straight out of its symbol table; this
tree's `build/gen/symbol_names.csv` is mostly working labels, so class ownership is
assembled from every naming corpus under `evidence/` (see CLASS ATTRIBUTION below).

WHY THIS AND NOT RTTI: the retail image was built /GR-. Only 7 `.?AV` type descriptors
exist and all are STL/exception types; `_TypeDescriptor`, `RTTICompleteObjectLocator`
and `__RTDynamicCast` are zero hits. Ghidra's RTTI Analyzer and the C++ Class Analyzer
find nothing for game classes. The decompiler-backed FillOutStructure path is the only
one that works on this target.

ENGINE
  `ghidra.app.decompiler.util.FillOutStructureHelper` - the "Auto Create Structure"
  action. For each instance method we take the `this` HighVariable (param slot 0 under
  __thiscall, ECX-input as fallback) and run processStructure(), which also recurses
  through CALLs that forward `this`. Per class we UNION the touched offsets over every
  method.

SIZE ORACLE
  FillOutStructure only ever yields a LOWER BOUND - a tail field no method touches is
  invisible, and its own sanityCheck() drops every offset past 0x1000. So per class we
  additionally read:
    * memset(this, 0, N) inside the constructor,
    * the operator new(N) immediate feeding the ctor at its call sites,
    * the vtable the class's own code installs at `this+0`, joined to
      config/retail-vtables.tsv's carved slot census for the virtual-method count. On an
      image with no RTTI and no vtable symbols this vptr store is what actually pins
      `??_7Class@@6B@` -> class, so it also NAMES vtables evidence/vtables/named.tsv left
      unnamed.
  `size_confidence` in the summary says how far ctor_size can be trusted.

READ-ONLY PROBES
  Every probe runs inside a transaction that is ROLLED BACK, so no recovered structure
  is ever committed to the Ghidra DB. (The one-time setup pass - seeding carved function
  boundaries and applying names - is committed on purpose; it is DB preparation, not
  probe output.) Flaky decompiles are skipped per-function and counted, never fatal.

INPUTS (all read-only)
  build/orig/HEROES3.EXE            hash-verified retail image
  config/retail-functions.tsv       11,932 carved boundaries -> seeded as Ghidra funcs
  config/retail-vtables.tsv         377 vtables (rva, function_count)
  evidence/vtables/named.tsv        vtable_rva -> class
  build/gen/symbol_names.csv        MSVC-decorated names, operator new, vtable sizes
  evidence/retail-function-names.csv, retail-dc-name-map.csv, retail-hd-name-map.csv,
  evidence/retail-game-tree.csv, evidence/ida/functions.csv    class attribution
  evidence/dreamcast/classes.csv, members.csv                  DC cross-check columns

OUTPUTS (evidence/ghidra-structs/)
  struct_layouts.csv      class,offset,width,n_methods,kind
  struct_summary.csv      per class: accessed_size (lower bound), ctor_size (oracle),
                          memset/new sizes, allocator, vtable_entries, DC size+base,
                          method/probe/skip counts
  struct_methods.csv      audit trail: every method that fed or failed a class union
  struct_members_dc.csv   per-class DC member list joined to the recovered offsets
  struct_calibration.csv  recovered layout vs this project's byte-proven anchors
  vtable_candidates.csv   candidate vtable -> class bindings, FOR REVIEW - this script
                          never writes to config/retail-vtables.tsv

The Ghidra project lives under build/ghidra/ (gitignored). No game bytes reach git.
"""
import argparse
import csv
import hashlib
import os
import re
import sys
from pathlib import Path

# Worktree-safe on purpose: the repo root is derived from THIS FILE's location, never
# from $HOMM3_DIR (the dev shell bakes that to the main checkout, so a script honouring
# it would read and write the main repo no matter which worktree it was launched from).
REPO = Path(__file__).resolve().parents[3]
EXE = REPO / "build/orig/HEROES3.EXE"      # no env override, for the same reason
EXE_SHA = "057c9d88e7206f6669a4615de2c6e02ab6c4e2d570a9e2badf07fe0bd6247274"
PROJ_DIR = REPO / "build/ghidra"
PROJ_NAME = "homm3-structs"
OUT_DIR = REPO / "evidence/ghidra-structs"
TARGET_STAMP = PROJ_DIR / "target.sha256"

IMAGE_BASE = 0x400000
DECOMP_TIMEOUT = 60
MAX_CTOR_CALLERS = 16
# FillOutStructureHelper.sanityCheck() refuses any offset above this unless the structure
# has already grown past it ("arbitrary cut-off to prevent huge structures"). Nothing
# deeper than this is recoverable, which is why combatManager (0x140ec) and game (0x4e7d0)
# come back with only their first page of fields.
FILLOUT_OFFSET_CAP = 0x1000

GENERATED_BY = ("python3 scripts/archive/ghidra_recover_structs/recover_structs.py"
                " (ONE-SHOT, not a pipeline stage)")


# --------------------------------------------------------------------------------------
# input readers (plain CSV/TSV; every input file carries `#` comment banners)
# --------------------------------------------------------------------------------------
def read_table(path, delim=None):
    path = Path(path)
    if not path.is_file():
        return []
    if delim is None:
        delim = "\t" if path.suffix == ".tsv" else ","
    with path.open(newline="") as fh:
        body = [line for line in fh if not line.startswith("#")]
    return list(csv.DictReader(body, delimiter=delim))


def hexint(value):
    try:
        return int(str(value), 16) if str(value).startswith("0x") else int(value)
    except (TypeError, ValueError):
        return None


# --------------------------------------------------------------------------------------
# CLASS ATTRIBUTION
#   rva -> owning class, from (in precedence order)
#     1. MSVC-decorated names in build/gen/symbol_names.csv  (source-proven, best)
#     2. evidence/retail-function-names.csv  (the merged naming join; has `convention`)
#     3. evidence/retail-dc-name-map.csv     (Dreamcast CodeView by link order)
#     4. evidence/retail-hd-name-map.csv     (NH3API via masked byte identity)
#     5. evidence/retail-game-tree.csv       (game source tree with retail ties)
#     6. evidence/ida/functions.csv          (NH3API IDB, only where hdmap bridged)
#   Every one of these is EVIDENCE, not proof - see the report banner in the outputs.
# --------------------------------------------------------------------------------------
_INSTANCE_PROPS = set("ABEFIJMNQRUV")   # public/protected/private [virtual] member
_STATIC_PROPS = set("CDGHKLST")         # static member / free function
_IDENT_RE = re.compile(r"^[A-Za-z_]\w*$")
_VTABLE_RE = re.compile(r"^\?\?_7(.+)@@6B@$")


def parse_mangled(name):
    """MSVC decorated name -> (class_key, is_instance, is_ctor)."""
    if not name or name[0] != "?":
        return None, False, False
    body = name[1:]
    is_ctor = False
    if body and body[0] == "?":                 # ??0 ctor, ??1 dtor, ??_G/_E, ...
        rest = body[1:]
        if not rest:
            return None, False, False
        if rest[0] == "_":
            code, after = "_" + rest[1:2], rest[2:]
        else:
            code, after = rest[0], rest[1:]
        if code == "0":
            is_ctor = True
    else:                                       # ?leaf@Class@@sig
        at = body.find("@")
        if at < 0:
            return None, False, False
        after = body[at + 1:]
        if after.startswith("@"):               # ?leaf@@sig - free function, no class
            return None, False, False           # (else "@YIJPBVarmy" parses as a class)
    end = after.find("@@")
    if end < 0:
        return None, False, False
    quals = [q for q in after[:end].split("@") if q]
    if not quals:
        return None, False, is_ctor             # free function
    sig = after[end + 2:]
    prop = sig[0] if sig else ""
    is_instance = is_ctor or (prop in _INSTANCE_PROPS)
    if prop in _STATIC_PROPS and not is_ctor:
        is_instance = False
    return "::".join(reversed(quals)), is_instance, is_ctor


def parse_vtable_class(name):
    match = _VTABLE_RE.match(name or "")
    if not match:
        return None
    quals = [q for q in match.group(1).split("@") if q]
    return "::".join(reversed(quals)) if quals else None


def readable_class(name):
    """`Class::method` -> Class, for the human-readable naming corpora."""
    if not name or "<" in name or name.startswith("std::") or name.startswith("`"):
        return None
    if "::" not in name:
        return None
    head = name.split("::", 1)[0]
    return head if _IDENT_RE.match(head) else None


def is_ctor_name(cls, name):
    """`Class::Class` (any overload) - not the destructor."""
    if not name or "::" not in name:
        return False
    leaf = name.split("::", 1)[1]
    return leaf == cls


def build_attribution():
    """rva -> dict(cls, name, src, instance). First writer wins (precedence above)."""
    owner = {}

    def claim(rva, cls, name, src, instance=True, ctor=False):
        if rva is None or rva in owner:
            return
        owner[rva] = {"cls": cls, "name": name, "src": src,
                      "instance": instance, "ctor": ctor}

    # 1. decorated names (also harvests operator new + vtable sizes)
    opnew_rva = None
    vtable_size_ents = {}
    for row in read_table(REPO / "build/gen/symbol_names.csv"):
        name, kind = row.get("name") or "", row.get("kind") or ""
        rva = hexint(row.get("rva"))
        if kind == "func":
            if name == "??2@YAPAXI@Z":
                opnew_rva = rva
            cls, instance, ctor = parse_mangled(name)
            if cls and instance:
                claim(rva, cls, name, "mangled", True, ctor)
        elif kind == "data":
            cls = parse_vtable_class(name)
            size = hexint(row.get("size"))
            if cls and size:
                vtable_size_ents[cls] = size // 4

    # 2-4. the readable naming corpora (convention column where present)
    for src, rel, name_key in (
        ("function-names", "evidence/retail-function-names.csv", "name"),
        ("dc-name-map", "evidence/retail-dc-name-map.csv", "name"),
        ("hd-name-map", "evidence/retail-hd-name-map.csv", "name"),
    ):
        for row in read_table(REPO / rel):
            name = row.get(name_key) or ""
            cls = readable_class(name)
            if not cls:
                continue
            conv = (row.get("convention") or "").strip()
            leaf = name.split("::", 1)[1]
            ctor = is_ctor_name(cls, name)
            # thiscall proves `this`; an explicitly non-thiscall convention on a
            # qualified name means a static member -> no `this`, do not probe.
            instance = True
            if conv in ("cdecl", "stdcall"):
                instance = False
            if conv == "fastcall" and not (ctor or leaf.startswith("~")):
                instance = False
            claim(hexint(row.get("rva")), cls, name, src, instance, ctor)

    # 5. game tree
    for row in read_table(REPO / "evidence/retail-game-tree.csv"):
        cls = readable_class(row.get("name") or "")
        if cls:
            claim(hexint(row.get("rva")), cls, row["name"], "game-tree",
                  True, is_ctor_name(cls, row["name"]))

    # 6. NH3API IDB, only where the masked-identity bridge landed a retail rva
    for row in read_table(REPO / "evidence/ida/functions.csv"):
        name = row.get("readable") or row.get("name") or ""
        cls = readable_class(name)
        if cls:
            claim(hexint(row.get("retail_rva")), cls, name, "ida",
                  True, is_ctor_name(cls, name))

    # 7. virtual-method slots of a NAMED vtable. A slot target is a virtual method of
    #    that class or of a single-inheritance base at offset 0 - either way its `this`
    #    is the same object, so its accesses belong in the class's union. Counted
    #    separately (`n_from_vtable`) so a reader can discount them.
    vtable_class = {}
    for row in read_table(REPO / "evidence/vtables/named.tsv"):
        cls = (row.get("class") or "").strip()
        rva = hexint(row.get("vtable_rva"))
        if cls and rva is not None:
            vtable_class[rva] = cls
    for row in read_table(REPO / "evidence/retail-vtable-symbols.csv"):
        cls = vtable_class.get(hexint(row.get("vtable_rva"))) or (row.get("class") or "").strip()
        target = hexint(row.get("target_rva"))
        if cls and target is not None and row.get("target_state") == "entry":
            claim(target, cls, row.get("method") or f"{cls}::vslot{row.get('slot')}",
                  "vtable-slot", True, False)

    # BOUNDARY GATE: keep only addresses that are a carved function entry. The naming
    # corpora carry NH3API-derived addresses from a different address space - 790 of
    # retail-function-names.csv's rows land INSIDE a function body (carve_state
    # "interior"), several mid-instruction. Those are not evidence for a boundary on
    # this image (CLAUDE.md) and must never be probed.
    carved = set()
    for row in read_table(REPO / "config/retail-functions.tsv"):
        rva = hexint(row.get("rva"))
        if rva is not None:
            carved.add(rva)
    dropped = [rva for rva in owner if rva not in carved]
    for rva in dropped:
        del owner[rva]

    return owner, opnew_rva, vtable_size_ents, len(dropped)


def find_nh_malloc_rvas():
    """CRT allocator entries from the reviewed library-DNA table (retail has no
    `??2@YAPAXI@Z` symbol anywhere in this tree's naming corpora)."""
    wanted = {"__nh_malloc", "_malloc", "__heap_alloc"}
    out = []
    path = REPO / "evidence/retail-function-libraries.tsv"
    if not path.is_file():
        return out
    with path.open() as fh:
        for line in fh:
            if line.startswith("#"):
                continue
            cols = line.rstrip("\n").split("\t")
            if len(cols) >= 5 and cols[4] in wanted:
                rva = hexint(cols[0])
                if rva is not None:
                    out.append(rva)
    return out


def build_vtable_entries(vtable_size_ents):
    """(class -> virtual-method count, vtable_rva -> slot count).

    config/retail-vtables.tsv function_count is the carved slot census;
    evidence/vtables/named.tsv bridges vtable_rva -> class for the 92 named ones. The
    rest get bridged at probe time by Prober.find_vtable (the vptr the ctor installs)."""
    counts = dict(vtable_size_ents)
    slots_at = {}
    for row in read_table(REPO / "config/retail-vtables.tsv"):
        rva = hexint(row.get("rva"))
        n = hexint(row.get("function_count"))
        if rva is not None and n is not None:
            slots_at[rva] = n
        cls = (row.get("class") or "").strip()
        if cls and n is not None:
            counts.setdefault(cls, n)
    for row in read_table(REPO / "evidence/vtables/named.tsv"):
        cls = (row.get("class") or "").strip()
        rva = hexint(row.get("vtable_rva"))
        n = hexint(row.get("slots"))
        if not cls:
            continue
        if n is None:
            n = slots_at.get(rva)
        if n is not None:
            counts[cls] = n            # named.tsv is the reviewed census; it wins
    return counts, slots_at


def _find_dc_dump():
    """The Dreamcast cvdump text. CLAUDE.md places it at ../homm3-symbols/ relative to
    the MAIN checkout, so a worktree has to walk up past .claude/worktrees/<name>/ to
    find it. Returns None if absent - the type resolution simply degrades."""
    rel = Path("homm3-symbols/HoMM3-Dreamcast-Dump/dump.txt")
    for base in [REPO] + list(REPO.parents):
        cand = base.parent / rel
        if cand.is_file():
            return cand
    return None


DC_DUMP = _find_dc_dump()
_BASE_TYPE_SIZE = {
    "T_CHAR": 1, "T_UCHAR": 1, "T_RCHAR": 1, "T_BOOL08": 1,
    "T_SHORT": 2, "T_USHORT": 2, "T_WCHAR": 2,
    "T_INT4": 4, "T_UINT4": 4, "T_LONG": 4, "T_ULONG": 4, "T_REAL32": 4,
    "T_REAL64": 8, "T_QUAD": 8, "T_UQUAD": 8,
}


def build_dc_typemap(dc_sizes):
    """CV type index -> readable type, parsed out of the Dreamcast cvdump.

    evidence/dreamcast/members.csv resolves struct/class member types but leaves ARRAYS,
    BITFIELDS and POINTERS as raw indices (`0x35C6`), which is exactly the information
    that settles an array's extent or a bitfield's bit range - useless to anyone reading
    the table. Resolve them so struct_members_dc.csv stands on its own.

    Degrades to {} if the dump is not present; the raw index is then kept as-is."""
    if DC_DUMP is None:
        return {}
    head_re = re.compile(r"^(0x[0-9a-f]+) : Length = \d+, Leaf = 0x[0-9a-f]+ (LF_\w+)")
    recs = {}
    key = kind = None
    with DC_DUMP.open(errors="replace") as fh:
        for line in fh:
            m = head_re.match(line)
            if m:
                key, kind = m.group(1).lower(), m.group(2)
                if kind in ("LF_ARRAY", "LF_BITFIELD", "LF_POINTER", "LF_ENUM",
                            "LF_INTERFACE", "LF_STRUCTURE", "LF_CLASS", "LF_MODIFIER"):
                    recs[key] = [kind, {}]
                else:
                    key = None
                continue
            if key is None or key not in recs:
                continue
            body = recs[key][1]
            for pat, field in (
                (r"Element type\s*[=:]\s*(\S+)", "elem"),
                (r"^\s*length = (\d+)", "len"),
                (r"bits = (\d+), starting position = (\d+), Type = (\S+)", "bits"),
                (r"class name = (.+?)\s*$", "name"),
                (r"enum name = (.+?)\s*$", "name"),
                (r"modifies type (\S+)", "mod"),
            ):
                mm = re.search(pat, line)
                if mm:
                    body[field] = mm.groups() if field == "bits" else mm.group(1)

    def size_of(tok):
        base = re.match(r"^(T_\w+?)\(", tok or "")
        if base:
            return _BASE_TYPE_SIZE.get(base.group(1))
        if tok and tok.lower().startswith("0x"):
            rec = recs.get(tok.lower())
            if rec and rec[0] == "LF_POINTER":
                return 4
            if rec and rec[0] == "LF_ENUM":
                return 4
            nm = render(tok)
            return dc_sizes.get(nm)
        return None

    def render(tok, depth=0):
        if not tok or depth > 6:
            return tok or ""
        if not tok.lower().startswith("0x"):
            return re.sub(r"\(\d+\)$", "", tok)     # T_CHAR(0010) -> T_CHAR
        rec = recs.get(tok.lower())
        if rec is None:
            return tok
        kind, body = rec
        if kind in ("LF_INTERFACE", "LF_STRUCTURE", "LF_CLASS", "LF_ENUM"):
            return body.get("name", tok)
        if kind == "LF_POINTER":
            return render(body.get("elem"), depth + 1) + "*"
        if kind == "LF_MODIFIER":
            return "const " + render(body.get("mod"), depth + 1)
        if kind == "LF_BITFIELD":
            bits, pos, typ = body.get("bits", ("?", "?", "?"))
            return "%s:%s @bit %s" % (render(typ, depth + 1), bits, pos)
        if kind == "LF_ARRAY":
            elem = render(body.get("elem"), depth + 1)
            total = int(body.get("len", 0) or 0)
            esz = size_of(body.get("elem"))
            if esz:
                return "%s[%d]" % (elem, total // esz)
            return "%s[%d bytes]" % (elem, total)
        return tok

    return {k: render(k) for k in recs}


def build_dc_layout():
    """Dreamcast CodeView class sizes/bases/members - cross-check columns only."""
    sizes, bases = {}, {}
    for row in read_table(REPO / "evidence/dreamcast/classes.csv"):
        name = row.get("name")
        if not name:
            continue
        try:
            sizes[name] = int(row["size"])
        except (KeyError, TypeError, ValueError):
            pass
        bases[name] = (row.get("bases") or "").strip()
    members = {}
    for row in read_table(REPO / "evidence/dreamcast/members.csv"):
        cls = row.get("class")
        if not cls:
            continue
        try:
            off = int(row["offset"])
        except (KeyError, TypeError, ValueError):
            continue
        members.setdefault(cls, []).append((off, row.get("name") or "",
                                            row.get("type") or ""))
    return sizes, bases, members


# --------------------------------------------------------------------------------------
# Ghidra session
# --------------------------------------------------------------------------------------
def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def preflight():
    if not os.environ.get("GHIDRA_INSTALL_DIR"):
        sys.exit("[structs] GHIDRA_INSTALL_DIR unset - enter the dev shell")
    if not EXE.is_file():
        sys.exit(f"[structs] target not found: {EXE}")
    got = sha256(EXE)
    if got != EXE_SHA:
        sys.exit(f"[structs] target sha256 mismatch: {got}")
    try:
        import pyghidra  # noqa: F401
    except Exception as exc:
        sys.exit(f"[structs] pyghidra not importable ({exc})")


def open_project(analyze, heap):
    import pyghidra
    if not pyghidra.started():
        launcher = pyghidra.HeadlessPyGhidraLauncher()
        launcher.add_vmargs(f"-Xmx{heap}")
        launcher.start()
    from pyghidra.core import _setup_project, _analyze_program
    from ghidra.program.flatapi import FlatProgramAPI

    PROJ_DIR.mkdir(parents=True, exist_ok=True)
    gproject, program = _setup_project(
        binary_path=str(EXE), project_location=str(PROJ_DIR),
        project_name=PROJ_NAME, nested_project_location=False)
    if analyze:
        from ghidra.program.util import GhidraProgramUtilities
        if GhidraProgramUtilities.shouldAskToAnalyze(program):
            print("[structs] auto-analyzing HEROES3.EXE (several minutes) ...", flush=True)
            _analyze_program(FlatProgramAPI(program), program)
            # markProgramAnalyzed needs an open transaction to stick across a save;
            # without this the next run re-analyzes from scratch.
            tx = program.startTransaction("mark-analyzed")
            try:
                GhidraProgramUtilities.markProgramAnalyzed(program)
            finally:
                program.endTransaction(tx, True)
        else:
            print("[structs] project already analyzed; reusing", flush=True)
    return gproject, program


def save_project(gproject, program):
    """Persist the analysed DB. GhidraProject.save() alone silently no-ops here (the
    reopened project came back with 0 functions), so save the DomainFile directly and
    say so if it fails - a run that cannot cache its analysis just costs the next run
    another full auto-analysis, it is not a correctness problem."""
    from ghidra.util.task import ConsoleTaskMonitor
    try:
        df = program.getDomainFile()
        if df is not None and df.isInWritableProject():
            df.save(ConsoleTaskMonitor())
            print("[structs] analysed DB saved to build/ghidra/", flush=True)
        else:
            gproject.save(program)
    except Exception as exc:
        print(f"[structs] WARNING: could not persist the analysed DB ({exc}); "
              f"the next run will re-analyze", flush=True)


def seed_functions(program, monitor):
    """Create a Ghidra function at every carved boundary Ghidra missed.

    config/retail-functions.tsv is the hand-owned boundary inventory; Ghidra's own
    analysis misses entries that are only ever reached through a vtable or a computed
    call. Committed on purpose (DB preparation)."""
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd

    space = program.getAddressFactory().getDefaultAddressSpace()
    fm = program.getFunctionManager()
    listing = program.getListing()
    rows = read_table(REPO / "config/retail-functions.tsv")
    created = existing = failed = 0
    tx = program.startTransaction("seed-carved-boundaries")
    try:
        for row in rows:
            rva = hexint(row.get("rva"))
            if rva is None:
                continue
            addr = space.getAddress(IMAGE_BASE + rva)
            if fm.getFunctionAt(addr) is not None:
                existing += 1
                continue
            if listing.getInstructionAt(addr) is None:
                DisassembleCommand(addr, None, True).applyTo(program, monitor)
            if CreateFunctionCmd(addr).applyTo(program, monitor):
                created += 1
            else:
                failed += 1
    finally:
        program.endTransaction(tx, True)
    print(f"[structs] carved boundaries: {existing} already present, {created} created, "
          f"{failed} could not be created", flush=True)


def apply_names(program, owner, monitor):
    """Name the attributed methods and force __thiscall so `this` is param 0.

    Names help the decompiler's own inference (and make any manual follow-up in the
    Ghidra GUI legible). Committed on purpose (DB preparation)."""
    from ghidra.program.model.symbol import SourceType

    space = program.getAddressFactory().getDefaultAddressSpace()
    fm = program.getFunctionManager()
    named = converted = missing = 0
    tx = program.startTransaction("apply-attributed-names")
    try:
        for rva, info in owner.items():
            fn = fm.getFunctionAt(space.getAddress(IMAGE_BASE + rva))
            if fn is None:
                missing += 1
                continue
            label = re.sub(r"[^A-Za-z0-9_]", "_", info["name"])[:180]
            try:
                fn.setName(label, SourceType.ANALYSIS)
                named += 1
            except Exception:
                pass
            if info["instance"]:
                try:
                    if fn.getCallingConventionName() != "__thiscall":
                        fn.setCallingConvention("__thiscall")
                        converted += 1
                except Exception:
                    pass
    finally:
        program.endTransaction(tx, True)
    print(f"[structs] names applied: {named}; __thiscall set: {converted}; "
          f"no function at rva: {missing}", flush=True)


# --------------------------------------------------------------------------------------
# probes
# --------------------------------------------------------------------------------------
class Prober:
    def __init__(self, program):
        from ghidra.app.decompiler import DecompInterface, DecompileOptions
        from ghidra.util.task import ConsoleTaskMonitor

        self.program = program
        self.monitor = ConsoleTaskMonitor()
        self.ecx = program.getRegister("ECX")
        self.fm = program.getFunctionManager()
        self.space = program.getAddressFactory().getDefaultAddressSpace()
        self.refs = program.getReferenceManager()
        # FillOutStructureHelper calls ifc.getOptions().getDefaultTimeout(); without
        # options set that NPEs. Set them explicitly.
        self.ifc = DecompInterface()
        options = DecompileOptions()
        try:
            options.grabFromProgram(program)
        except Exception:
            pass
        self.ifc.setOptions(options)
        self.ifc.toggleCCode(True)
        self.ifc.openProgram(program)

    def addr(self, rva):
        return self.space.getAddress(IMAGE_BASE + rva)

    def function_at(self, rva):
        return self.fm.getFunctionAt(self.addr(rva))

    @staticmethod
    def as_int(value):
        try:
            return int(value.longValue())
        except AttributeError:
            return int(value)

    def probe(self, fn):
        """(offset->width, read offsets, write offsets) for one method, or None.

        Runs entirely inside a ROLLED-BACK transaction: the convention change and the
        structure FillOutStructure creates are both reverted."""
        from ghidra.app.decompiler.util import FillOutStructureHelper

        helper = FillOutStructureHelper(self.program, self.monitor)
        tx = self.program.startTransaction("struct-probe")
        try:
            try:
                if fn.getCallingConventionName() != "__thiscall":
                    fn.setCallingConvention("__thiscall")
            except Exception:
                pass
            res = self.ifc.decompileFunction(fn, DECOMP_TIMEOUT, self.monitor)
            if not (res and res.decompileCompleted()):
                return None
            sym = res.getHighFunction().getLocalSymbolMap().getParamSymbol(0)
            this_var = sym.getHighVariable() if sym is not None else None
            if this_var is None:                # fallback: ECX input at entry
                this_var = helper.computeHighVariable(self.ecx.getAddress(), fn, self.ifc)
            if this_var is None:
                return None
            helper.processStructure(this_var, fn, True, False, self.ifc)
            widths = {}
            it = helper.getComponentMap().iterator()
            while it.hasNext():
                entry = it.next()
                off = self.as_int(entry.getKey())
                dt = entry.getValue()
                width = dt.getLength() if dt is not None else 0
                widths[off] = max(widths.get(off, 0), width if width > 0 else 1)
            reads = set(self.as_int(p.getOffset()) for p in helper.getLoadPcodeOps())
            writes = set(self.as_int(p.getOffset()) for p in helper.getStorePcodeOps())
            return widths, reads, writes
        except Exception as exc:
            print("[structs]   probe failed @0x%x: %s"
                  % (fn.getEntryPoint().getOffset() - IMAGE_BASE, exc), flush=True)
            return None
        finally:
            self.program.endTransaction(tx, False)   # ROLL BACK

    # --- size oracle ------------------------------------------------------------------
    _MEMSET_RE = re.compile(r"memset\s*\(\s*[^,]+,\s*0\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*\)")

    def ctor_memset_size(self, fn):
        try:
            res = self.ifc.decompileFunction(fn, DECOMP_TIMEOUT, self.monitor)
            if not (res and res.decompileCompleted()):
                return None
            text = str(res.getDecompiledFunction().getC())
            best = None
            for m in self._MEMSET_RE.finditer(text):
                raw = m.group(1)
                val = int(raw, 16) if raw.lower().startswith("0x") else int(raw)
                best = val if best is None or val > best else best
            return best
        except Exception:
            return None

    @staticmethod
    def _call_target(op):
        """VA of a CALL's direct target (compare by offset - JPype `!=` on Address is
        identity-based, not value-based)."""
        from ghidra.program.model.pcode import PcodeOp
        if op.getOpcode() != PcodeOp.CALL or op.getNumInputs() < 1:
            return None
        target = op.getInput(0)
        return target.getAddress().getOffset() if target.isAddress() else None

    def ctor_new_size(self, ctor_addr, alloc_offs):
        """Size of the allocation whose result is the `this` passed to the ctor.

        The retail image has no `??2@YAPAXI@Z` symbol (nothing in this tree names
        operator new), so instead of pinning one callee we accept ANY single-constant-
        argument CALL whose RETURN VALUE is the ctor's `this`, and report which callee it
        was. In practice that is `operator new(size)` or `malloc(size)`; `alloc_offs` (the
        allocator VAs we could identify up front) only raises confidence when it matches.
        Returns (size, allocator_va, corroborating_call_sites) or None."""
        ctor_off = ctor_addr.getOffset()
        votes = {}
        seen = 0
        for ref in self.refs.getReferencesTo(ctor_addr):
            if not ref.getReferenceType().isCall():
                continue
            caller = self.fm.getFunctionContaining(ref.getFromAddress())
            if caller is None:
                continue
            seen += 1
            if seen > MAX_CTOR_CALLERS:
                break
            try:
                res = self.ifc.decompileFunction(caller, DECOMP_TIMEOUT, self.monitor)
                if not (res and res.decompileCompleted()):
                    continue
                ops = res.getHighFunction().getPcodeOps()
                while ops.hasNext():
                    op = ops.next()
                    if self._call_target(op) != ctor_off or op.getNumInputs() < 2:
                        continue
                    defn = op.getInput(1).getDef()      # def of the `this` argument
                    if defn is None or defn.getNumInputs() != 2:
                        continue
                    callee = self._call_target(defn)
                    if callee is None:
                        continue
                    size = defn.getInput(1)
                    if size.isConstant():
                        val = int(size.getOffset())
                        if 0 < val < (1 << 20):
                            key = (val, callee)
                            votes[key] = votes.get(key, 0) + 1
            except Exception:
                continue
        if not votes:
            return None
        # prefer an allocator we recognised, then the most frequently observed size
        ranked = sorted(votes.items(),
                        key=lambda kv: (kv[0][1] in alloc_offs, kv[1]), reverse=True)
        (val, callee), hits = ranked[0]
        return val, callee, hits

    def find_vtable(self, ctor_rvas, other_rvas, vtable_slots):
        """The vtable a class's own code installs at `this+0`.

        Scans constructors FIRST, then every other method, for a reference to any address
        in the carved vtable census. This is what actually pins `??_7Class@@6B@` -> class
        on an image with no RTTI and no vtable symbols, and it fills in the virtual-method
        count for classes evidence/vtables/named.tsv never named.

        A hit inside a CONSTRUCTOR is the vptr store itself and is strong; a hit in any
        other method may just be a reference to a single-inheritance BASE class's vtable,
        so the caller records which it was.
        Returns (vtable_rva, slots, method_rva, from_ctor) or (None, None, None, False)."""
        for rvas, from_ctor in ((ctor_rvas, True), (other_rvas, False)):
            for rva in rvas:
                fn = self.function_at(rva)
                if fn is None:
                    continue
                body = fn.getBody()
                it = self.refs.getReferenceSourceIterator(body.getMinAddress(), True)
                while it.hasNext():
                    src = it.next()
                    if not body.contains(src):
                        break
                    for ref in self.refs.getReferencesFrom(src):
                        target = ref.getToAddress().getOffset() - IMAGE_BASE
                        if target in vtable_slots:
                            return target, vtable_slots[target], rva, from_ctor
        return None, None, None, False

    def find_allocators(self, nh_malloc_rvas):
        """VAs of `operator new`-shaped wrappers: small functions calling __nh_malloc /
        malloc. Used only to rank the ctor allocation oracle, never to gate it."""
        allocs = set()
        for rva in nh_malloc_rvas:
            addr = self.addr(rva)
            allocs.add(addr.getOffset())
            for ref in self.refs.getReferencesTo(addr):
                if not ref.getReferenceType().isCall():
                    continue
                caller = self.fm.getFunctionContaining(ref.getFromAddress())
                if caller is not None and caller.getBody().getNumAddresses() < 0x80:
                    allocs.add(caller.getEntryPoint().getOffset())
        return allocs


# --------------------------------------------------------------------------------------
# calibration against this project's byte-proven anchors
# --------------------------------------------------------------------------------------
# (class, offset, width, label) - every one of these was proven from retail bytes by an
# earlier matcher lane. Agreement here is the only honest measure of how far the
# recovered tables can be trusted.
ANCHORS = [
    ("hero", 0x476, 1, "primarySkill[0] attack"),
    ("hero", 0x477, 1, "primarySkill[1] defense"),
    ("hero", 0x478, 1, "primarySkill[2] spellPower"),
    ("hero", 0x479, 1, "primarySkill[3] knowledge"),
    ("army", 0xC8, 4, "attackSkill"),
    ("army", 0xCC, 4, "defenseSkill"),
    ("combatManager", 0x132B8, 4, "actingSide"),
    ("combatManager", 0x132BC, 4, "actingSlot"),
]
# size anchors: (class, proven size, label)
SIZE_ANCHORS = [
    ("town", 360, "byte-proven; corroborated by ai_player's town-array stride"),
    ("type_point", 4, "byte-proven bitfield word: short x:10; short y:10; short z:4"),
    ("pathCell", 30, "byte-proven stride, #pragma pack(1) - NO instance methods exist, "
                     "so the class-method engine cannot reach it at all"),
]


def calibrate(layouts, summary):
    """layouts: cls -> {off: (width, n_methods, kind)}; summary: cls -> dict.

    NOTE the hard reach limit: FillOutStructureHelper.sanityCheck() drops any offset
    above FILLOUT_OFFSET_CAP unless the structure has already grown past it, so nothing
    beyond ~4 KB into an object is recoverable at all. An anchor past that is scored
    `out-of-reach`, NOT `absent` - the engine never had a chance at it."""
    rows = []
    for cls, off, width, label in ANCHORS:
        got = layouts.get(cls, {})
        hit = off in got
        # a byte at +0x476 is "covered" if a wider recovered field spans it
        covering = [o for o, v in got.items() if o <= off < o + max(v[0], 1)]
        verdict = ("exact" if hit and got[off][0] == width else
                   "offset-hit-width-differs" if hit else
                   "inside-wider-field" if covering else
                   "out-of-reach" if off > FILLOUT_OFFSET_CAP else
                   "not-observed")
        rows.append({
            "kind": "field", "class": cls, "anchor": f"0x{off:x}",
            "anchor_detail": label,
            "recovered": (f"0x{off:x} w{got[off][0]}" if hit else
                          (f"0x{covering[0]:x} w{got[covering[0]][0]}" if covering else "")),
            "verdict": verdict,
        })
    for cls, size, label in SIZE_ANCHORS:
        info = summary.get(cls, {})
        ctor = info.get("ctor_size")
        acc = info.get("accessed_size")
        if ctor == size:
            verdict = "exact"
        elif ctor:
            verdict = "ctor-oracle-differs"
        elif acc and acc <= size:
            verdict = "lower-bound-consistent"
        elif acc:
            verdict = "accessed-exceeds-proof"
        else:
            verdict = "absent"
        rows.append({
            "kind": "size", "class": cls, "anchor": str(size), "anchor_detail": label,
            "recovered": f"ctor={ctor if ctor else '-'} accessed={acc if acc else '-'}",
            "verdict": verdict,
        })
    return rows


# --------------------------------------------------------------------------------------
def banner(fh, what):
    fh.write(f"# GENERATED (ONE-SHOT): {GENERATED_BY}\n")
    fh.write("# ANALYSIS OUTPUT, NOT RETAIL EVIDENCE - Ghidra decompiler inference.\n")
    fh.write("# Standing: lead-generator only. Same grade as the Dreamcast dump - a\n")
    fh.write("# recovered offset still needs retail-byte proof before it is a claim.\n")
    fh.write(f"# exe: HEROES3.EXE sha256={EXE_SHA} size=2732032\n")
    fh.write(f"# contents: {what}\n")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--no-analyze", action="store_true",
                    help="reuse the cached project without auto-analysis")
    ap.add_argument("--no-seed", action="store_true",
                    help="skip the carved-boundary + naming setup pass")
    ap.add_argument("--classes", default="",
                    help="comma-separated class allowlist (default: every class)")
    ap.add_argument("--min-methods", type=int, default=1,
                    help="skip classes with fewer attributed methods than this")
    ap.add_argument("--heap", default="6G", help="JVM max heap (default 6G)")
    ap.add_argument("--out", default=str(OUT_DIR), help="output directory")
    ap.add_argument("--rescore", action="store_true",
                    help="recompute the derived columns (size_confidence) and "
                         "struct_calibration.csv from the already-written tables "
                         "(no Ghidra, no re-probe)")
    args = ap.parse_args(argv)

    if args.rescore:
        out = Path(args.out)
        rows = read_table(out / "struct_summary.csv")
        for row in rows:
            row.setdefault("size_confidence", "")
            row["size_confidence"] = size_confidence(row)
        write_summary(out, rows)
        return write_calibration(out, *load_tables(out))

    preflight()
    owner, opnew_rva, vtable_size_ents, n_offboundary = build_attribution()
    vtable_entries, vtable_slots = build_vtable_entries(vtable_size_ents)
    dc_sizes, dc_bases, dc_members = build_dc_layout()
    dc_typemap = build_dc_typemap(dc_sizes)
    vtable_census = {}
    for row in read_table(REPO / "evidence/vtables/named.tsv"):
        cls_name = (row.get("class") or "").strip()
        rva = hexint(row.get("vtable_rva"))
        if cls_name and rva is not None:
            vtable_census[rva] = cls_name

    by_class = {}
    for rva, info in owner.items():
        by_class.setdefault(info["cls"], []).append((rva, info))
    allow = set(c.strip() for c in args.classes.split(",") if c.strip())
    targets = sorted(c for c, m in by_class.items()
                     if (not allow or c in allow) and len(m) >= args.min_methods)
    print(f"[structs] {len(owner)} attributed methods over {len(by_class)} classes "
          f"({n_offboundary} attributed addresses dropped: not a carved entry); "
          f"{len(targets)} selected", flush=True)

    gproject, program = open_project(not args.no_analyze, args.heap)
    from ghidra.app.script import GhidraScriptUtil
    GhidraScriptUtil.acquireBundleHostReference()
    try:
        prober = Prober(program)
        if not args.no_seed:
            seed_functions(program, prober.monitor)
            apply_names(program, owner, prober.monitor)
        alloc_offs = prober.find_allocators(find_nh_malloc_rvas())
        if opnew_rva is not None:
            alloc_offs.add(IMAGE_BASE + opnew_rva)
        print(f"[structs] allocator candidates: {len(alloc_offs)} "
              f"({', '.join('0x%x' % (a - IMAGE_BASE) for a in sorted(alloc_offs)[:8])})",
              flush=True)

        layouts, summary, method_log = {}, {}, []
        for index, cls in enumerate(targets, 1):
            entries = sorted(by_class[cls])
            width_of, touched, read_of, write_of = {}, {}, {}, {}
            n_probed, skipped = 0, []
            ctor_rvas = [rva for rva, info in entries if info["ctor"]]
            for rva, info in entries:
                if not info["instance"]:
                    method_log.append((cls, rva, info["name"], info["src"], "static"))
                    continue
                fn = prober.function_at(rva)
                if fn is None:
                    skipped.append(rva)
                    method_log.append((cls, rva, info["name"], info["src"], "no-function"))
                    continue
                result = prober.probe(fn)
                if result is None:
                    skipped.append(rva)
                    method_log.append((cls, rva, info["name"], info["src"],
                                       "decompile-failed"))
                    continue
                widths, reads, writes = result
                method_log.append((cls, rva, info["name"], info["src"],
                                   "probed:%d" % len(set(widths) | reads | writes)))
                for off, width in widths.items():
                    width_of.setdefault(off, set()).add(width)
                    touched.setdefault(off, set()).add(rva)
                for off in reads:
                    read_of[off] = True
                for off in writes:
                    write_of[off] = True
                if widths or reads or writes:
                    n_probed += 1

            offsets = set(width_of) | set(read_of) | set(write_of)
            accessed = 0
            per_off = {}
            for off in sorted(offsets):
                widths = width_of.get(off, set())
                width = max(widths) if widths else 1
                kind = ("rw" if (read_of.get(off) and write_of.get(off))
                        else "r" if read_of.get(off)
                        else "w" if write_of.get(off) else "?")
                if len(widths) > 1:
                    kind += "!union"
                per_off[off] = (width, len(touched.get(off, set())), kind)
                if off >= 0:
                    accessed = max(accessed, off + width)
            layouts[cls] = per_off

            vt_rva, vt_slots, vt_method, vt_from_ctor = prober.find_vtable(
                ctor_rvas, [r for r, _ in entries], vtable_slots)
            if vt_slots is None and cls in vtable_entries:
                vt_slots = vtable_entries[cls]

            memset_size = new_size = None
            alloc_va = alloc_hits = None
            for ctor_rva in ctor_rvas[:6]:
                ctor_fn = prober.function_at(ctor_rva)
                if ctor_fn is None:
                    continue
                if memset_size is None:
                    memset_size = prober.ctor_memset_size(ctor_fn)
                if new_size is None:
                    got = prober.ctor_new_size(prober.addr(ctor_rva), alloc_offs)
                    if got is not None:
                        new_size, alloc_va, alloc_hits = got
                if memset_size is not None and new_size is not None:
                    break
            if memset_size is not None and new_size is not None:
                ctor_size = max(memset_size, new_size)
            else:
                ctor_size = new_size if new_size is not None else memset_size

            summary[cls] = {
                "accessed_size": accessed, "ctor_size": ctor_size,
                "memset_size": memset_size, "new_size": new_size,
                "alloc_fn": ("0x%x" % (alloc_va - IMAGE_BASE)) if alloc_va else "",
                "alloc_sites": alloc_hits or "",
                "vtable_entries": vt_slots,
                "vtable_rva": vt_rva,
                "vtable_method": vt_method, "vtable_from_ctor": vt_from_ctor,
                "dc_size": dc_sizes.get(cls), "dc_base": dc_bases.get(cls, ""),
                "n_methods": len(entries), "n_probed": n_probed,
                "n_from_vtable": sum(1 for _, i in entries if i["src"] == "vtable-slot"),
                "n_skipped": len(skipped), "skipped": skipped,
            }
            if index % 10 == 0 or index == len(targets):
                print(f"[structs] {index}/{len(targets)} classes "
                      f"(last {cls}: {len(per_off)} offsets, {len(skipped)} skipped)",
                      flush=True)
    finally:
        GhidraScriptUtil.releaseBundleHostReference()
        try:
            prober.ifc.dispose()      # else the save fails: "active transaction"
        except Exception:
            pass
        save_project(gproject, program)
        gproject.close()
        TARGET_STAMP.write_text(EXE_SHA + "\n")

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    with (out / "struct_layouts.csv").open("w", newline="") as fh:
        banner(fh, "one row per recovered offset; kind r/w/rw, !union = >1 width seen")
        w = csv.writer(fh)
        w.writerow(["class", "offset", "width", "n_methods", "kind"])
        for cls in sorted(layouts):
            for off in sorted(layouts[cls]):
                width, n, kind = layouts[cls][off]
                w.writerow([cls, ("-0x%x" % -off) if off < 0 else ("0x%x" % off),
                            width, n, kind])

    rows = []
    for cls in sorted(summary):
        s = summary[cls]
        hx = lambda v: ("0x%x" % v) if v else ""          # noqa: E731
        row = {
            "class": cls, "accessed_size": hx(s["accessed_size"]),
            "ctor_size": hx(s["ctor_size"]), "memset_size": hx(s["memset_size"]),
            "new_size": hx(s["new_size"]), "alloc_fn": s["alloc_fn"],
            "alloc_sites": s["alloc_sites"], "size_confidence": "",
            "vtable_entries": (s["vtable_entries"]
                               if s["vtable_entries"] is not None else ""),
            "vtable_rva": ("0x%x" % s["vtable_rva"]) if s["vtable_rva"] else "",
            "dc_size": hx(s["dc_size"]), "dc_base": s["dc_base"],
            "n_methods": s["n_methods"], "n_from_vtable": s["n_from_vtable"],
            "n_probed": s["n_probed"], "n_skipped": s["n_skipped"],
            "skipped_rvas": " ".join("0x%x" % r for r in s["skipped"]),
        }
        row["size_confidence"] = size_confidence(row)
        rows.append(row)
    write_summary(out, rows)

    with (out / "struct_methods.csv").open("w", newline="") as fh:
        banner(fh, "every method that fed (or failed to feed) a class union - the audit "
                   "trail behind struct_layouts.csv; `src` is the naming corpus that "
                   "attributed the address to the class")
        w = csv.writer(fh)
        w.writerow(["class", "rva", "name", "src", "outcome"])
        for cls, rva, name, src, outcome in sorted(method_log):
            w.writerow([cls, "0x%x" % rva, name, src, outcome])

    with (out / "struct_members_dc.csv").open("w", newline="") as fh:
        banner(fh, "Dreamcast CodeView members for every probed class (plus every class "
                   "named in struct_calibration.csv, so the anchors the engine cannot "
                   "reach are still documented), flagged where the recovered x86 layout "
                   "touches the same offset. dc_type is RESOLVED from the cvdump type "
                   "records - array extents and bitfield bit ranges included. DC offsets "
                   "are SH4: cross-architecture, a lead, never an x86 claim")
        w = csv.writer(fh)
        w.writerow(["class", "dc_offset", "dc_name", "dc_type", "x86_offset_touched"])
        anchored = {c for c, *_ in ANCHORS} | {c for c, *_ in SIZE_ANCHORS}
        for cls in sorted(set(summary) | (anchored & set(dc_members))):
            for off, name, typ in sorted(dc_members.get(cls, [])):
                pretty = dc_typemap.get((typ or "").lower(),
                                        re.sub(r"\(\d+\)$", "", typ or ""))
                w.writerow([cls, "0x%x" % off, name, pretty,
                            "yes" if off in layouts.get(cls, {}) else "no"])

    with (out / "vtable_candidates.csv").open("w", newline="") as fh:
        banner(fh, "candidate vtable -> class bindings recovered from the vptr a class's "
                   "own code installs at `this+0`. LEADS FOR REVIEW, NOT ADMISSIONS: "
                   "nothing here has been written to config/retail-vtables.tsv. Read "
                   "from_ctor first - a binding seen in a CONSTRUCTOR is the vptr store "
                   "itself; one seen only in some other method may be a reference to a "
                   "single-inheritance BASE class's vtable. `shared_with` is that same "
                   "tell: two classes on one vtable means one of them is the base")
        w = csv.DictWriter(fh, fieldnames=[
            "vtable_rva", "slots", "candidate_class", "from_ctor", "evidence_method",
            "census_state", "census_class", "agreement", "shared_with"])
        w.writeheader()
        by_vtable = {}
        for cls in summary:
            rva = summary[cls]["vtable_rva"]
            if rva is not None:
                by_vtable.setdefault(rva, []).append(cls)
        for cls in sorted(summary):
            s = summary[cls]
            if s["vtable_rva"] is None:
                continue
            census = vtable_census.get(s["vtable_rva"])
            if census is None:
                state, agreement = "unnamed", "new"
            elif census == cls:
                state, agreement = "named", "agree"
            else:
                state, agreement = "named", "disagree"
            w.writerow({
                "vtable_rva": "0x%x" % s["vtable_rva"], "slots": s["vtable_entries"],
                "candidate_class": cls,
                "from_ctor": "yes" if s["vtable_from_ctor"] else "no",
                "evidence_method": ("0x%x" % s["vtable_method"]
                                    if s["vtable_method"] else ""),
                "census_state": state, "census_class": census or "",
                "agreement": agreement,
                "shared_with": " ".join(sorted(c for c in by_vtable[s["vtable_rva"]]
                                               if c != cls)),
            })

    write_calibration(out, layouts, summary)

    n_layout = sum(1 for c in layouts if layouts[c])
    n_ctor = sum(1 for c in summary if summary[c]["ctor_size"])
    print(f"[structs] wrote 6 tables -> {out}")
    print(f"[structs] {len(targets)} classes, {n_layout} with a recovered layout, "
          f"{n_ctor} with a ctor-exact size")
    return 0


SUMMARY_FIELDS = ["class", "accessed_size", "ctor_size", "memset_size", "new_size",
                  "alloc_fn", "alloc_sites", "size_confidence", "vtable_entries",
                  "vtable_rva", "dc_size", "dc_base", "n_methods", "n_from_vtable",
                  "n_probed", "n_skipped", "skipped_rvas"]


def size_confidence(row):
    """How far to trust `ctor_size` as the TRUE object size.

    The allocation oracle takes the constant-argument call whose return value becomes the
    ctor's `this`. That is usually `operator new(sizeof(T))`, but it misfires when a ctor
    is reached right after an unrelated allocation (font's 0x1260 glyph buffer, sample's
    0x34) - so score it:
      high        the access union INDEPENDENTLY reaches exactly the same size
      medium      several call sites agree on the allocation size
      low         one call site only, and the access union disagrees
      lower-bound no allocation seen; accessed_size is a floor, nothing more
    """
    ctor = hexint(row.get("ctor_size")) if row.get("ctor_size") else None
    accessed = hexint(row.get("accessed_size")) if row.get("accessed_size") else None
    try:
        sites = int(row.get("alloc_sites") or 0)
    except (TypeError, ValueError):
        sites = 0
    if not ctor:
        return "lower-bound"
    if accessed and accessed == ctor:
        return "high"
    if sites >= 3:
        return "medium"
    return "low"


def write_summary(out, rows):
    with (out / "struct_summary.csv").open("w", newline="") as fh:
        banner(fh, "one row per class. accessed_size is a LOWER BOUND (FillOutStructure "
                   "cannot see untouched tail fields and drops any offset past 0x1000); "
                   "ctor_size is the memset/allocation oracle - read size_confidence "
                   "before believing it. vtable_rva is the first carved vtable any of "
                   "the class's methods references: measured against the reviewed census "
                   "in evidence/vtables/named.tsv it agrees 28/33, and every miss is the "
                   "same failure mode - it picked up a single-inheritance BASE class's "
                   "vtable (border->widget's, TGzFile->TAbstractFile's). Treat it as a "
                   "lead for naming an unnamed vtable, not as a binding")
        w = csv.DictWriter(fh, fieldnames=SUMMARY_FIELDS)
        w.writeheader()
        w.writerows(rows)


def load_tables(out):
    """Re-read struct_layouts.csv + struct_summary.csv into the in-memory shapes
    calibrate() wants (for --calibrate-only)."""
    layouts, summary = {}, {}
    for row in read_table(out / "struct_layouts.csv"):
        off = row["offset"]
        off = -int(off[3:], 16) if off.startswith("-0x") else int(off, 16)
        layouts.setdefault(row["class"], {})[off] = (
            int(row["width"]), int(row["n_methods"]), row["kind"])
    for row in read_table(out / "struct_summary.csv"):
        summary[row["class"]] = {
            "accessed_size": hexint(row["accessed_size"]) or 0,
            "ctor_size": hexint(row["ctor_size"]),
        }
    return layouts, summary


def write_calibration(out, layouts, summary):
    cal = calibrate(layouts, summary)
    with (out / "struct_calibration.csv").open("w", newline="") as fh:
        banner(fh, "recovered layout vs this project's retail-byte-proven anchors - the "
                   "only honest measure of how far these tables can be trusted. "
                   "`out-of-reach` = past FillOutStructure's 0x1000 offset cut-off, so "
                   "the engine never had a chance at it (not a disagreement)")
        w = csv.DictWriter(fh, fieldnames=["kind", "class", "anchor", "anchor_detail",
                                           "recovered", "verdict"])
        w.writeheader()
        w.writerows(cal)
    reachable = [r for r in cal if r["verdict"] != "out-of-reach"]
    agree = sum(1 for r in reachable if r["verdict"] == "exact")
    contra = [r for r in reachable
              if r["verdict"] in ("offset-hit-width-differs", "ctor-oracle-differs",
                                  "accessed-exceeds-proof")]
    print(f"[structs] calibration: {agree}/{len(reachable)} reachable anchors exact, "
          f"{len(contra)} contradicted, {len(cal) - len(reachable)} out of reach")
    for row in cal:
        print(f"[structs]   {row['class']}+{row['anchor']} ({row['anchor_detail']}): "
              f"{row['verdict']} {row['recovered']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
