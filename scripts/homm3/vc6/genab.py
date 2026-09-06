#!/usr/bin/env python3
"""homm3.vc6.genab - Track R: the RTM-vs-SP3 back-end generation A/B harness.

Retail HEROES3.EXE's Rich header records 26 C++ objects built by CL
generation 8168 (VC6 RTM) against 145 by 8447 (our pinned SP3 C2).  Several
open matching walls - merged-return / tail-duplication divergence in BOTH
directions, whole-body callee-saved register swaps - may be generation
artifacts, not model gaps.  This harness settles that per function: compile
the owning unit under (a) the pinned SP3 toolchain and (b) an overlay
toolchain whose C2.DLL is the RTM 12.00.8168 back end, then three-way
compare each wall function against retail (the delinked target object when
one exists, else capstone over the gated image).

Two generation overlays exist, selected by --gen:

  rtm      the BACK end only (C2.DLL 12.00.8168 over the pinned SP3 front
           end).  Answered 2026-08-09: codegen-invariant on every wall.
  rtm-fe   FRONT + back end (C1XX.DLL 12.00.8168 + C2.DLL 12.00.8168).
           Retail's 26 RTM-stamped C++ objects were produced by an 8168
           front end too, and the C2-only A/B never fed the RTM back end
           anything but SP3-C1XX IL - so the front end is the remaining
           generation lever.

  build-rtm     create build/vc6/toolchain-rtm/msvc - the same copy-overlay
                mechanism as homm3.vc6.shim.build (bin/ copied so the
                driver's LoadLibraryA resolves inside the overlay, the rest
                symlinked), with the RTM C2.DLL installed as bin/C2.DLL.
  build-rtm-fe  the same overlay at build/vc6/toolchain-rtm-fe/msvc with
                BOTH bin/C1XX.DLL and bin/C2.DLL replaced by the RTM
                pressings.
                The RTM DLLs are staged OUTSIDE the repo under
                ../orig/vc6-rtm/ and are hash-gated (sha256+size+
                FileVersion+export table) before every install - a wrong
                pressing is a hard abort.
  run        A/B the wall corpus (or --fn targets), emit
             evidence/vc6/{c2,fe}-generation-verdicts.tsv.  Verdict per
             function:
               SP3-matches      SP3 output already matches retail at the
                                register-visible + branch-shape grade
               RTM-closes       SP3 diverges, the RTM generation matches
               neither          both diverge (columns show which is closer)
               RTM-unavailable  the RTM overlay could not be built/used
             --all-units sweeps every unit in config/units.toml instead of
             the wall corpus: whole-object byte compare first (masking the
             COFF TimeDateStamp and the @comp.id stamp), then a per-function
             three-way compare inside each unit that actually differs.
  verify     the overlay control: prove the selected generation is really in
             the loop (CL banner version + the produced object's @comp.id
             stamp + the SetMenus/GiveSpells jb-vs-jl sentinel).
  clean      remove the overlays and scratch.

Metric: distance = homm3.vc6._align.distance (unpaired register-visible
masked-instruction slots) + homm3.vc6._flow.distance (branch-shape
disagreements) against the retail side.  0 = matched at that grade - weaker
than byte-exact (absolute address operands stay masked), which is stated in
the TSV header; a byte ratchet still needs the real pipeline.

The pinned SP3 toolchain is never modified in place, MSVC_DIR selects the
overlay per compile through homm3.core.cc_wrap (the same wrapper ninja
runs), and nothing here lands source edits: a `RTM-closes` verdict is a
DIAGNOSIS (the wall is a generation artifact, closable with zero further
reverse-engineering), recorded for the decision log.

rc: 0 = ran, RTM side available; 1 = ran with RTM-unavailable rows;
2 = error (vc6 convention).  Run inside `nix develop .#build`.
"""
from __future__ import annotations

import argparse
import hashlib
import os
import shutil
import struct
import sys
from pathlib import Path

from homm3.core import cc_wrap
from homm3.sema import _asm
from homm3.vc6 import _align, _common, _flow, _toolchain
from homm3.vc6.shim import build as shim_build

SCRATCH = _common.REPO / "build/vc6/genab"
UNITS_TOML = _common.REPO / "config/units.toml"

# The RTM compiler binaries, staged OUTSIDE the repo (game/toolchain bytes
# never enter git).  Provenance: archive.org item
# 1998-10-01-visual-studio-6.0-enterprise-edition-disc-1, VSE600EUN1.ISO,
# VC98/BIN/<name>, cross-verified byte-identical against item
# vs-6.0-enterprise VS6.0Enterprise_Disk1.iso (docs/vc6/rtm-generation.md,
# ../orig/vc6-rtm/PROVENANCE.txt).
RTM_DIR = _common.REPO.parent / "orig/vc6-rtm"
RTM_PINNED: dict[str, tuple[str, int]] = {
    "C2.DLL":   ("45187b0b6288240f73272a7c61e6329c50048a76e57db3bb87b6f0229e09e27d", 737329),
    "C1XX.DLL": ("4095e9e8de5ac4b13dbfe310abb8de711475b1a92c5f94375919947937f3237e", 1183795),
}
RTM_FILEVERSION = "12.00.8168.0"

# Generation overlays: id -> (overlay path, the bin/ DLLs it replaces).
# "rtm" swaps the BACK end only - Track R section 4, answered: invariant.
# "rtm-fe" swaps the front end too, the one remaining generation lever.
GENERATIONS: dict[str, tuple[str, tuple[str, ...]]] = {
    "rtm":    ("build/vc6/toolchain-rtm",    ("C2.DLL",)),
    "rtm-fe": ("build/vc6/toolchain-rtm-fe", ("C1XX.DLL", "C2.DLL")),
}
VERDICT_TSV = {
    "rtm":    _common.EVIDENCE / "c2-generation-verdicts.tsv",
    "rtm-fe": _common.EVIDENCE / "fe-generation-verdicts.tsv",
}
# @comp.id, the COFF stamp CL writes into every object: (prodid << 16) |
# build.  0x0b = Utc12_C++, the Rich-header product id retail records 26
# times at build 8168 and 145 times at 8447.  Written by the BACK end, so
# it separates rtm/rtm-fe from sp3 but not rtm from rtm-fe - the CL banner
# and the sentinel functions do that (see `verify`).
COMP_ID_PRODID = 0x0b
COMP_ID_BUILD = {"sp3": 8447, "rtm": 8168, "rtm-fe": 8168}


def overlay_msvc(gen: str) -> Path:
    return _common.REPO / GENERATIONS[gen][0] / "msvc"

# The wall corpus: the merged-return / tail-duplication walls named in the
# Track R brief plus every function whose in-tree residual comment cites
# merged-return / DUP-EXIT / tail-merge / the CL-generation class
# (grep src/*.cpp, 2026-08-09).  move_toward and get_ranged_attack_value
# are byte-exact controls - their comments cite the DUP-EXIT shape as
# CLOSED, so they must come back SP3-matches or the harness is broken.
CORPUS = [
    "?FindPath@army@@QAEHHHEE@Z",                    # path - merged bounds guards
    "?GetAdjacentCellIndex@army@@QBEHHH@Z",          # path
    "?ValidAttack@army@@QBEHHHHHPAH@Z",              # path - cross-jumped inline tails
    "?AppWndProc@@YGJPAUHWND__@@IIJ@Z",              # kbwin - merged-return wall
    "?check_shipyard_square@@YIEPAVtown@@JJ@Z",      # town - 8 branches / 2 rets
    "?get_legion_bonus@town@@QAEJJ@Z",               # town
    "?VideoRealignBuffers@@YIXXZ",                   # smackmgr - CL-generation-capped lea
    "?VideoClose@@YIXXZ",                            # smackmgr - tail-dup'd top test
    "?DoCompAI@combatManager@@QAEXH@Z",              # ai - and eax,0xff generation class
    "?move_toward@combatManager@@QAEEPBVarmy@@JPBJE@Z",  # ai - EXACT control
    "?cast_enchantment@type_AI_combat_data@@QAEXAAUtype_spell_choice@@AAV1@@Z",
                                                     # ai_combat - retail tail-dups 4 epilogues
    "?get_ranged_attack_value@type_AI_combat_parameters@@QAEJPBVarmy@@0@Z",
                                                     # ai_tactical - EXACT control (DUP-EXIT closed)
    "?Main@button@@UAEHPAVmessage@@@Z",              # button - esi/edi role swap
    "?StartMP3@soundManager@@QAEXPBDHE@Z",           # soundmgr - tail-merge direction
    "?CenterWindow@heroWindow@@QAEXHH@Z",            # window - reg tie-break, stale-CL suspect
    "??0TPickANumber@@QAE@HH@Z",                     # misc - esi/edi swap signature
    "?NextRandomFrame@iconWidget@@QAEXXZ",           # iconwdgt - allocator runs out earlier
    "?NextRandomSiegeEngineFrame@iconWidget@@QAEXXZ",  # iconwdgt - same signature
    # Generation-divergent SENTINELS (2026-08-09): the ONLY functions in the
    # corpus TUs where C2 8168 emits different bytes than C2 8447 (whole-obj
    # byte compare, timestamp+comp.id masked).  Retail sides with SP3 on all
    # three - these TUs belong to the Rich header's 145-object 8447 band.
    "?GiveSpells@town@@QAEXPAVhero@@@Z",
    "?initialize_spells@town@@QAEXPBVTownExtra@@@Z",
    "?SetMenus@@YIXPAUHMENU__@@H@Z",
]


# ---------------------------------------------------------------------------
# RTM DLL gating
# ---------------------------------------------------------------------------

def _fileversion(path: Path) -> str | None:
    """FileVersion from the PE's VS_FIXEDFILEINFO (signature 0xFEEF04BD)."""
    data = path.read_bytes()
    i = data.find(struct.pack("<I", 0xFEEF04BD))
    if i < 0:
        return None
    ms, ls = struct.unpack_from("<II", data, i + 8)
    return f"{ms >> 16}.{ms & 0xffff:02d}.{ls >> 16}.{ls & 0xffff}"


def gate_rtm_dll(name: str) -> Path:
    """Hash-gate one staged RTM compiler DLL; abort on any mismatch.

    Same four checks the C2-only track used, applied per binary: size,
    sha256, VS_FIXEDFILEINFO FileVersion, and the exact export table (both
    C1XX and C2 sit in the same driver slot and export exactly
    {_InvokeCompilerPass@12, _AbortCompilerPass@4}, which is what makes the
    front end drop-in at all)."""
    path = RTM_DIR / name
    if not path.is_file():
        _common.die(f"RTM {name} not staged at {path} - see "
                    "docs/vc6/rtm-generation.md for the sourcing provenance")
    want_sha, want_size = RTM_PINNED[name]
    size = path.stat().st_size
    if size != want_size:
        _common.die(f"{path}: size {size} != pinned {want_size}")
    sha = hashlib.sha256(path.read_bytes()).hexdigest()
    if sha != want_sha:
        _common.die(f"{path}: sha256 {sha} != pinned {want_sha} - "
                    "not the admitted RTM pressing")
    ver = _fileversion(path)
    if ver != RTM_FILEVERSION:
        _common.die(f"{path}: FileVersion {ver} != {RTM_FILEVERSION}")
    exports = shim_build.dll_exports(path)
    if exports != shim_build.EXPECTED_EXPORTS:
        _common.die(f"{path}: exports {sorted(exports)} != "
                    f"{sorted(shim_build.EXPECTED_EXPORTS)}")
    return path


# ---------------------------------------------------------------------------
# overlay construction (mirrors homm3.vc6.shim.build.build_overlay)
# ---------------------------------------------------------------------------

def build_overlay(gen: str, force: bool = False) -> None:
    """Copy-overlay of the pinned msvc tree with this generation's RTM DLLs
    in bin/.  bin/ is copied (the driver's LoadLibraryA must resolve real
    files inside the overlay); all other top-level entries are symlinks."""
    swapped = GENERATIONS[gen][1]
    for name in swapped:            # gate the pinned pressings BEFORE copying;
        _toolchain.resolve(name)    # resolve() also refuses to run with an
    _toolchain.resolve("CL.EXE")    # MSVC_DIR already pointed at an overlay
    real_msvc = cc_wrap.msvc_dir()
    staged = {name: gate_rtm_dll(name) for name in swapped}
    msvc = overlay_msvc(gen)
    if force and msvc.exists():
        shutil.rmtree(msvc)
    bin_dir = msvc / "bin"
    if not bin_dir.is_dir():
        msvc.mkdir(parents=True, exist_ok=True)
        for entry in sorted(real_msvc.iterdir()):
            if entry.name.lower() == "bin":
                continue
            dst = msvc / entry.name
            if not (dst.exists() or dst.is_symlink()):
                dst.symlink_to(entry.resolve())
        bin_dir.mkdir()
        for f in sorted((real_msvc / "bin").iterdir()):
            if f.is_file():
                shutil.copy2(f, bin_dir / f.name)
        print(f"[genab] overlay tree created at {msvc}")
    # Install (or re-verify) this generation's RTM passes.
    for name in swapped:
        dst = cc_wrap.find_ci(bin_dir, name)
        if dst is None:
            _common.die(f"no {name.lower()} in {bin_dir} - rebuild with --force")
        want_sha, want_size = RTM_PINNED[name]
        if (dst.stat().st_size != want_size
                or hashlib.sha256(dst.read_bytes()).hexdigest() != want_sha):
            shutil.copy2(staged[name], dst)
            installed = hashlib.sha256(dst.read_bytes()).hexdigest()
            if installed != want_sha:
                _common.die(f"{dst}: post-install sha {installed} != pinned")
            print(f"[genab] RTM {name} {RTM_FILEVERSION} installed as {dst}")
    # Every pass this generation does NOT swap must still be pinned SP3.
    for name in ("CL.EXE", "C1.DLL", "C1XX.DLL", "C2.DLL"):
        if name in swapped:
            continue
        f = cc_wrap.find_ci(bin_dir, name)
        want_sha, want_size = _toolchain.PINNED[name]
        if (f is None or f.stat().st_size != want_size
                or hashlib.sha256(f.read_bytes()).hexdigest() != want_sha):
            _common.die(f"overlay {name} is not the pinned pressing - "
                        "rebuild with --force")


def rtm_available(gen: str) -> tuple[bool, str]:
    """(usable, reason). Non-fatal probe used by `run`."""
    for name in GENERATIONS[gen][1]:
        path = RTM_DIR / name
        if not path.is_file():
            return False, f"RTM {name} not staged at {path}"
        want_sha, want_size = RTM_PINNED[name]
        if path.stat().st_size != want_size:
            return False, f"{path} size != pinned"
        if hashlib.sha256(path.read_bytes()).hexdigest() != want_sha:
            return False, f"{path} sha256 != pinned"
    return True, ""


# ---------------------------------------------------------------------------
# manifest + compile
# ---------------------------------------------------------------------------

def load_manifest() -> dict:
    """unit -> {source, flags:[...]} from config/units.toml."""
    import tomllib
    manifest = tomllib.loads(UNITS_TOML.read_text())
    profiles = manifest.get("flags", {})
    out = {}
    for unit in manifest.get("unit", []):
        out[unit["unit"]] = {
            "source": unit["source"],
            "flags": list(profiles[unit["flags"]]),
        }
    return out


def compile_unit(unit: str, spec: dict, side: str) -> tuple[Path | None, str]:
    """One unit compile via cc_wrap.  side='sp3' uses the default (pinned)
    toolchain; any other side is a GENERATIONS id and points MSVC_DIR at
    that overlay.  Cached per side directory; (obj|None, error-tail)."""
    out = SCRATCH / side / f"{unit}.obj"
    if out.is_file():
        return out, ""
    env = None if side == "sp3" else {"MSVC_DIR": str(overlay_msvc(side))}
    src = _common.REPO / spec["source"]
    if not src.is_file():
        return None, f"source missing: {src}"
    proc = shim_build._cc_wrap(out, src, spec["flags"], env)
    if not out.is_file():
        tail = "\n".join((proc.stdout + proc.stderr).strip().splitlines()[-6:])
        return None, tail
    return out, ""


# ---------------------------------------------------------------------------
# overlay controls: @comp.id, masked object identity, CL banner
# ---------------------------------------------------------------------------

def comp_id(obj: Path) -> tuple[int, int] | None:
    """(prodid, build) from the object's @comp.id absolute symbol, the COFF
    stamp whose counts the retail PE's Rich header aggregates."""
    for name, val in _symbols(obj.read_bytes()):
        if name == "@comp.id":
            return val >> 16, val & 0xffff
    return None


def _symbols(d: bytes):
    """(name, value) over the COFF symbol table, skipping aux records."""
    sym_off, nsyms = struct.unpack_from("<II", d, 8)
    i = 0
    while i < nsyms:
        e = sym_off + 18 * i
        name = d[e:e + 8].rstrip(b"\0").decode("latin1")
        value = struct.unpack_from("<I", d, e + 8)[0]
        yield name, value
        i += 1 + d[e + 17]


def masked_object(obj: Path) -> bytes:
    """Object bytes with the two known per-build nondeterminisms zeroed:
    the COFF TimeDateStamp (shim.md's measured window) and the @comp.id
    symbol value (which is exactly the generation stamp we are varying).
    Everything left is codegen."""
    d = bytearray(obj.read_bytes())
    d[4:8] = b"\0\0\0\0"
    sym_off, nsyms = struct.unpack_from("<II", d, 8)
    i = 0
    while i < nsyms:
        e = sym_off + 18 * i
        if bytes(d[e:e + 8]) == b"@comp.id":
            d[e + 8:e + 12] = b"\0\0\0\0"
        i += 1 + d[e + 17]
    return bytes(d)


def pinned_include(src: Path) -> str:
    """The INCLUDE list spelled against the PINNED tree, whatever overlay is
    selected.  Every overlay's include/ is a symlink to this same directory,
    so pinning the spelling changes nothing but the string - and the string
    is exactly what would otherwise pollute the comparison."""
    from homm3.vc6 import il
    saved = os.environ.pop("MSVC_DIR", None)
    try:
        return il._include_env(src)
    finally:
        if saved is not None:
            os.environ["MSVC_DIR"] = saved


def il_fingerprint(side: str, src: Path, flags: list[str], slot: str,
                   include: str) -> dict:
    """{stream: (size, sha256[:12])} for ONE front-end run of *src*.

    The IL streams are the front end's entire output - the back end reads
    nothing else - so a byte difference here measures the C1XX pressing
    directly.  It is the only control that separates rtm-fe from the
    C2-only rtm overlay: @comp.id is written by the BACK end, and the CL
    banner (measured 2026-09-06) prints 12.00.8168 under both the pinned
    SP3 tree and the RTM overlay, because the driver stub CL.EXE is
    byte-identical across the two generations and prints its own version.

    Two confounders are held fixed, both measured 2026-09-06: *include* (see
    pinned_include) and the work-directory path, whose length reaches the
    streams - hence the one-character *slot* names."""
    from homm3.vc6 import il
    saved = os.environ.get("MSVC_DIR")
    if side == "sp3":
        os.environ.pop("MSVC_DIR", None)
    else:
        os.environ["MSVC_DIR"] = str(overlay_msvc(side))
    try:
        cap = il.capture(src, [f for f in flags if not f.startswith("/Fo")],
                         SCRATCH / "il" / slot, include=include)
        return {stream: (path.stat().st_size,
                         hashlib.sha256(path.read_bytes()).hexdigest()[:12])
                for stream, path in sorted(cap.items())}
    finally:
        os.environ.pop("MSVC_DIR", None)
        if saved is not None:
            os.environ["MSVC_DIR"] = saved


# ---------------------------------------------------------------------------
# per-function extraction + scoring
# ---------------------------------------------------------------------------

def fn_text(obj: Path, name: str, ordinal: int = 0) -> str | None:
    if name not in _asm._public_text_symbols(obj):
        return None
    return _asm.objdump(obj, name, ordinal)


def retail_text(ctx, name: str, unit: str, rva: int, size: int,
                ordinal: int) -> tuple[str, str]:
    """(text, producer): the delinked target object when it carries the
    symbol, else capstone over the gated image (cross-producer skew -
    _align's caveat - is recorded in the producer column)."""
    tobj = _asm.TARGET / f"{unit}.c.obj"
    if tobj.is_file() and name in _asm._public_text_symbols(tobj):
        return _asm.objdump(tobj, name, ordinal), "delinked"
    if not size:
        _common.die(f"{name}: no delinked symbol and no recorded size")
    return _asm.image_text(ctx, rva, size, name), "capstone"


def distances(a_text: str, b_text: str) -> tuple[int, int]:
    """(align, flow) distances between two producer texts."""
    align = _align.distance(_align.parse_side(a_text),
                            _align.parse_side(b_text))
    flow = _flow.distance(_flow.profile(a_text), _flow.profile(b_text))
    return align, flow


def run(args) -> int:
    gen = args.gen
    shim_build._ensure_wine_env()
    manifest = load_manifest()
    from homm3.sema.context import get_context
    ctx = get_context()

    rtm_ok, rtm_reason = rtm_available(gen)
    if rtm_ok:
        try:
            build_overlay(gen)
        except SystemExit:
            raise                      # rc-2 die(): a WRONG pressing must abort,
    else:                              # only ABSENCE degrades to RTM-unavailable
        print(f"[genab] {gen} side unavailable: {rtm_reason}")

    if args.fresh and SCRATCH.exists():
        shutil.rmtree(SCRATCH)

    if args.all_units:
        return sweep_units(gen, manifest, ctx, rtm_ok, rtm_reason)

    targets = args.fn or CORPUS
    rows = []
    for spec in targets:
        name, unit, rva, size, ordinal = ctx.symbols.resolve_fn(spec)
        if unit not in manifest:
            rows.append(_row(name, unit, rva, size, verdict="neither",
                             note="unit not in config/units.toml manifest"))
            continue
        ref_text, producer = retail_text(ctx, name, unit, rva, size, ordinal)

        sp3_obj, tail = compile_unit(unit, manifest[unit], "sp3")
        if sp3_obj is None:
            _common.die(f"SP3 compile of {unit} failed:\n{tail}")
        sp3_text = fn_text(sp3_obj, name)
        if sp3_text is None:
            rows.append(_row(name, unit, rva, size, producer=producer,
                             verdict="neither",
                             note="symbol not public in compiled obj"))
            continue
        sp3_align, sp3_flow = distances(sp3_text, ref_text)

        if not rtm_ok:
            rows.append(_row(name, unit, rva, size, sp3=(sp3_align, sp3_flow),
                             producer=producer, verdict="RTM-unavailable",
                             note=rtm_reason))
            continue

        rtm_obj, tail = compile_unit(unit, manifest[unit], gen)
        if rtm_obj is None:
            rows.append(_row(name, unit, rva, size, sp3=(sp3_align, sp3_flow),
                             producer=producer, verdict="RTM-unavailable",
                             note="RTM compile failed (front/back IL "
                                  "incompat?): " + tail.replace("\t", " ")
                                  .replace("\n", " | ")[:200]))
            continue
        rtm_text = fn_text(rtm_obj, name)
        if rtm_text is None:
            rows.append(_row(name, unit, rva, size, sp3=(sp3_align, sp3_flow),
                             producer=producer, verdict="RTM-unavailable",
                             note="symbol not public in RTM obj"))
            continue
        rtm_align, rtm_flow = distances(rtm_text, ref_text)
        ab_align, ab_flow = distances(sp3_text, rtm_text)

        sp3_total = sp3_align + sp3_flow
        rtm_total = rtm_align + rtm_flow
        verdict = _verdict(sp3_total, rtm_total)
        note = _note(sp3_total, rtm_total, ab_align + ab_flow)
        rows.append(_row(name, unit, rva, size, sp3=(sp3_align, sp3_flow),
                         rtm=(rtm_align, rtm_flow), ab=(ab_align, ab_flow),
                         producer=producer, verdict=verdict, note=note))

    _write_tsv(rows, gen, rtm_ok)
    _print_rows(rows)
    return 0 if rtm_ok and all(r["verdict"] != "RTM-unavailable"
                               for r in rows) else 1


# ---------------------------------------------------------------------------
# whole-corpus sweep
# ---------------------------------------------------------------------------

def sweep_units(gen: str, manifest: dict, ctx, rtm_ok: bool,
                rtm_reason: str) -> int:
    """Every unit in config/units.toml, both sides.  Stage 1 is a masked
    whole-object byte compare (TimeDateStamp + @comp.id zeroed): a unit
    whose two objects are identical has NO generation-sensitive function in
    it and needs no per-function work.  Stage 2 three-way compares every
    public text symbol inside each unit that does differ - that list is the
    candidate attribution of the Rich header's RTM-stamped band."""
    if not rtm_ok:
        _common.die(f"--all-units needs the {gen} overlay: {rtm_reason}")
    units = sorted(manifest)
    differing, failed, rows = [], [], []
    for n, unit in enumerate(units, 1):
        sp3_obj, tail = compile_unit(unit, manifest[unit], "sp3")
        if sp3_obj is None:
            failed.append((unit, "sp3", tail))
            continue
        gen_obj, tail = compile_unit(unit, manifest[unit], gen)
        if gen_obj is None:
            failed.append((unit, gen, tail))
            continue
        same = masked_object(sp3_obj) == masked_object(gen_obj)
        print(f"  [{n:>3}/{len(units)}] {unit:<28} "
              f"{'identical' if same else 'DIFFERS'}")
        if same:
            continue
        differing.append(unit)
        be_obj = None
        if gen == "rtm-fe":
            # Separate the two swapped passes: recompile with the C2-only
            # overlay.  Same bytes as rtm-fe => the front end contributed
            # nothing and the delta is entirely C2 8168-vs-8447.
            be_obj, be_tail = compile_unit(unit, manifest[unit], "rtm")
            if be_obj is None:
                failed.append((unit, "rtm", be_tail))
            else:
                print("        unit attribution: "
                      + ("back-end-only"
                         if masked_object(be_obj) == masked_object(gen_obj)
                         else "front-end-contributes"))
        rows.extend(_sweep_unit_functions(ctx, unit, sp3_obj, gen_obj, gen,
                                          be_obj))
    _write_tsv(rows, gen, rtm_ok, sweep=True)
    _print_rows(rows)
    print(f"[genab] {len(units)} units, {len(differing)} generation-sensitive"
          f"{': ' + ', '.join(differing) if differing else ''}")
    for unit, side, tail in failed:
        print(f"[genab] compile FAILED {unit} ({side}): "
              + tail.replace('\n', ' | ')[:160])
    return 0 if not failed else 1


def _sweep_unit_functions(ctx, unit: str, sp3_obj: Path, gen_obj: Path,
                          gen: str, be_obj: Path | None = None) -> list:
    """Per-function three-way rows for one generation-sensitive unit.

    *be_obj* is the same unit built with the BACK-END-only overlay; when it
    is given, each row is attributed per function - identical text there
    means this function's whole delta is C2 8168-vs-8447 and the RTM front
    end contributed nothing to it."""
    rows = []
    be_syms = _asm._public_text_symbols(be_obj) if be_obj else set()
    shared = sorted(_asm._public_text_symbols(sp3_obj)
                    & _asm._public_text_symbols(gen_obj))
    tobj = _asm.TARGET / f"{unit}.c.obj"
    retail_syms = (_asm._public_text_symbols(tobj) if tobj.is_file() else set())
    for name in shared:
        sp3_text = _asm.objdump(sp3_obj, name, 0)
        gen_text = _asm.objdump(gen_obj, name, 0)
        ab_align, ab_flow = distances(sp3_text, gen_text)
        if ab_align + ab_flow == 0 and sp3_text == gen_text:
            continue                      # not generation-sensitive
        attrib = ""
        if name in be_syms:
            attrib = ("back-end-only"
                      if _asm.objdump(be_obj, name, 0) == gen_text
                      else "front-end-contributes")
        rva = size = 0
        claimed = ctx.symbols.byname.get(name)
        if claimed is not None and claimed in ctx.symbols.funcs:
            rva, size = claimed, ctx.symbols.funcs[claimed][2]
        if name in retail_syms:
            ref_text, producer = _asm.objdump(tobj, name, 0), "delinked"
        elif size:
            ref_text, producer = _asm.image_text(ctx, rva, size, name), "capstone"
        else:
            rows.append(_row(name, unit, rva, size, ab=(ab_align, ab_flow),
                             attrib=attrib, verdict="neither",
                             note="no retail body"))
            continue
        sp3_a, sp3_f = distances(sp3_text, ref_text)
        gen_a, gen_f = distances(gen_text, ref_text)
        rows.append(_row(name, unit, rva, size, sp3=(sp3_a, sp3_f),
                         rtm=(gen_a, gen_f), ab=(ab_align, ab_flow),
                         attrib=attrib, producer=producer,
                         verdict=_verdict(sp3_a + sp3_f, gen_a + gen_f),
                         note=_note(sp3_a + sp3_f, gen_a + gen_f,
                                    ab_align + ab_flow)))
    return rows


def _verdict(sp3_total: int, gen_total: int) -> str:
    if sp3_total == 0:
        return "SP3-matches"
    return "RTM-closes" if gen_total == 0 else "neither"


def _note(sp3_total: int, gen_total: int, ab_total: int) -> str:
    if sp3_total == 0:
        return "" if gen_total == 0 else \
            f"RTM diverges by {gen_total} where SP3 is exact"
    if gen_total == 0:
        return "generation artifact - closable with zero RE"
    if ab_total == 0:
        return "RTM output identical to SP3 - not a generation wall"
    if gen_total < sp3_total:
        return f"RTM closer ({gen_total} vs {sp3_total})"
    if gen_total > sp3_total:
        return f"RTM farther ({gen_total} vs {sp3_total})"
    return f"equidistant ({sp3_total}), different bytes"


def _row(name, unit, rva, size, sp3=None, rtm=None, ab=None,
         producer="", verdict="", note="", attrib=""):
    return {"fn": name, "unit": unit, "rva": f"0x{rva:x}",
            "size": f"0x{size:x}",
            "sp3_align": "" if sp3 is None else sp3[0],
            "sp3_flow": "" if sp3 is None else sp3[1],
            "rtm_align": "" if rtm is None else rtm[0],
            "rtm_flow": "" if rtm is None else rtm[1],
            "sp3_vs_rtm": "" if ab is None else ab[0] + ab[1],
            "gen_attrib": attrib,
            "retail_producer": producer, "verdict": verdict, "note": note}


_COLS = ["fn", "unit", "rva", "size", "sp3_align", "sp3_flow",
         "rtm_align", "rtm_flow", "sp3_vs_rtm", "gen_attrib",
         "retail_producer", "verdict", "note"]


def _write_tsv(rows: list, gen: str, rtm_ok: bool, sweep: bool = False) -> None:
    swapped = GENERATIONS[gen][1]
    extra = []
    for name in ("C1XX.DLL", "C2.DLL"):
        sha, size = _toolchain.PINNED[name]
        ver = "12.00.8472" if name == "C1XX.DLL" else "12.00.8447"
        extra.append(f"# sp3-{name.split('.')[0].lower()}: {ver} sha256 {sha} "
                     f"size {size} (pinned toolchain)")
    for name in swapped:
        sha, size = RTM_PINNED[name]
        extra.append(
            f"# {gen}-{name.split('.')[0].lower()}: {RTM_FILEVERSION} sha256 "
            f"{sha} size {size} "
            f"({'staged ../orig/vc6-rtm/' + name if rtm_ok else 'NOT STAGED'})")
    extra += [
        f"# generation: {gen} swaps {', '.join(swapped)}; every other pass is "
        "the pinned SP3 pressing",
        "# metric: align = _align.distance (unpaired register-visible masked "
        "slots), flow = _flow.distance (branch-shape); verdict grades at "
        "align+flow==0 - REGISTER-VISIBLE+BRANCH-SHAPE, weaker than "
        "byte-exact (absolute addresses masked)",
        "# retail_producer: delinked = build/objdiff/target/<unit>.c.obj "
        "(same disassembler both sides); capstone = image producer "
        "(cross-producer skew inflates distances)",
        "# gen_attrib (rtm-fe only): back-end-only = this function's bytes "
        "are the same under the C2-only overlay, so the RTM FRONT end "
        "contributed nothing to its delta; front-end-contributes = C1XX "
        "8168 changed it",
    ]
    if sweep:
        extra.append("# --all-units sweep: only functions whose two sides "
                     "differ are listed; a unit absent here compiled "
                     "byte-identically under both generations")
    cmd = f"python3 -m homm3.vc6.genab run --gen {gen}" + (
        " --all-units" if sweep else "")
    head = _common.provenance(cmd, extra=extra)
    out = VERDICT_TSV[gen]
    out.parent.mkdir(parents=True, exist_ok=True)
    lines = head + ["\t".join(_COLS)]
    for r in rows:
        lines.append("\t".join(str(r[c]) for c in _COLS))
    out.write_text("\n".join(lines) + "\n")
    print(f"[genab] wrote {out.relative_to(_common.REPO)} "
          f"({len(rows)} row(s))")


def _print_rows(rows: list) -> None:
    print(f"  {'function':<44} {'unit':<11} {'sp3':>7} {'rtm':>7} "
          f"{'a/b':>5}  verdict")
    for r in rows:
        sp3 = ("" if r["sp3_align"] == "" else
               f"{r['sp3_align']}+{r['sp3_flow']}")
        rtm = ("" if r["rtm_align"] == "" else
               f"{r['rtm_align']}+{r['rtm_flow']}")
        print(f"  {r['fn'][:44]:<44} {r['unit']:<11} {sp3:>7} {rtm:>7} "
              f"{str(r['sp3_vs_rtm']):>5}  {r['verdict']}"
              + (f"  ({r['note']})" if r["note"] else ""))
    tally = {}
    for r in rows:
        tally[r["verdict"]] = tally.get(r["verdict"], 0) + 1
    print("[genab] " + ", ".join(f"{v}: {n}" for v, n in sorted(tally.items())))


def run_clean() -> int:
    dirs = [_common.REPO / d for d, _ in GENERATIONS.values()] + [SCRATCH]
    for p in dirs:
        if p.exists():
            shutil.rmtree(p)
            print(f"[genab] removed {p}")
    return 0


# ---------------------------------------------------------------------------
# verify - prove the selected generation is really in the loop
# ---------------------------------------------------------------------------

SENTINELS = ("?SetMenus@@YIXPAUHMENU__@@H@Z",
             "?GiveSpells@town@@QAEXPAVhero@@@Z")


def run_verify(gen: str) -> int:
    """Three independent controls, in ascending strength:

      1. CL banner - printed by the FRONT end, so it is the only control
         that separates rtm-fe from the C2-only rtm overlay.
      2. @comp.id - written by the back end into every produced object;
         must read prodid 0x0b build 8168 on the RTM sides, 8447 on SP3.
      3. the sentinels - SetMenus / GiveSpells, the two functions Track R
         section 4 measured as the ENTIRE codegen delta between C2 8168 and
         C2 8447 (a jb-vs-jl loop-guard twin plus one back-edge threading).
         Their bytes must differ from SP3 under any RTM generation; if they
         did not, the overlay would not be in the loop at all.

    rc 0 = every control agrees, 1 = a control answered NO."""
    shim_build._ensure_wine_env()
    build_overlay(gen)
    SCRATCH.mkdir(parents=True, exist_ok=True)
    manifest = load_manifest()
    ok = True
    # The front end is swapped only by rtm-fe; under the C2-only overlay
    # the IL must come back IDENTICAL, which makes that run this control's
    # negative control.
    want_il_differs = "C1XX.DLL" in GENERATIONS[gen][1]

    units = sorted({u for u in ("kbwin", "town") if u in manifest})
    for unit in units:
        sp3_obj, tail = compile_unit(unit, manifest[unit], "sp3")
        if sp3_obj is None:
            _common.die(f"SP3 compile of {unit} failed:\n{tail}")
        gen_obj, tail = compile_unit(unit, manifest[unit], gen)
        if gen_obj is None:
            _common.die(f"{gen} compile of {unit} failed:\n{tail}")
        for side, obj in (("sp3", sp3_obj), (gen, gen_obj)):
            got = comp_id(obj)
            want = (COMP_ID_PRODID, COMP_ID_BUILD[side])
            mark = "ok " if got == want else "NO "
            ok = ok and got == want
            print(f"  comp.id {mark}{unit:<10} {side:<7} "
                  f"prodid 0x{got[0]:02x} build {got[1]} (want {want[1]})")
        same = masked_object(sp3_obj) == masked_object(gen_obj)
        print(f"  object     {unit:<10} sp3 vs {gen}: "
              f"{'identical' if same else 'DIFFERS'}")
        for name in SENTINELS:
            if name not in _asm._public_text_symbols(sp3_obj):
                continue
            a = _asm.objdump(sp3_obj, name, 0)
            b = _asm.objdump(gen_obj, name, 0)
            same_fn = a == b
            ok = ok and not same_fn
            print(f"  sentinel {'NO ' if same_fn else 'ok '}{name:<34} "
                  f"{'identical' if same_fn else 'DIFFERS'}")
        src = _common.REPO / manifest[unit]["source"]
        include = pinned_include(src)
        flags = manifest[unit]["flags"]
        il_sp3 = il_fingerprint("sp3", src, flags, "a", include)
        il_gen = il_fingerprint(gen, src, flags, "b", include)
        differs = il_sp3 != il_gen
        ok = ok and differs == want_il_differs
        print(f"  front-end {'ok ' if differs == want_il_differs else 'NO '}"
              f"{unit:<10} IL sp3 vs {gen}: "
              f"{'DIFFERS' if differs else 'identical'} "
              f"(want {'DIFFERS' if want_il_differs else 'identical'})")
        for stream in sorted(set(il_sp3) | set(il_gen)):
            a, b = il_sp3.get(stream), il_gen.get(stream)
            print(f"    il{stream}  sp3 {a}  {gen} {b}"
                  + ("" if a == b else "   <- differs"))
    return 0 if ok else 1


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    ap = argparse.ArgumentParser(
        prog="python3 -m homm3.vc6.genab", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    for verb, gen in (("build-rtm", "rtm"), ("build-rtm-fe", "rtm-fe")):
        b = sub.add_parser(verb, help=f"create/refresh the {gen} overlay "
                                      f"({', '.join(GENERATIONS[gen][1])})")
        b.add_argument("--force", action="store_true",
                       help="recreate the overlay tree from scratch")
    r = sub.add_parser("run", help="A/B the wall corpus, write the verdicts TSV")
    r.add_argument("--gen", choices=sorted(GENERATIONS), default="rtm",
                   help="which generation overlay to A/B against SP3 "
                        "(default: rtm, the back end alone)")
    r.add_argument("--fn", action="append",
                   help="target function (mangled name or 0x-address); "
                        "repeatable; default = the built-in wall corpus")
    r.add_argument("--all-units", action="store_true",
                   help="sweep every unit in config/units.toml instead of "
                        "the wall corpus")
    r.add_argument("--fresh", action="store_true",
                   help="discard cached unit objects first")
    v = sub.add_parser("verify", help="prove the overlay is in the loop "
                                      "(banner + @comp.id + sentinels)")
    v.add_argument("--gen", choices=sorted(GENERATIONS), default="rtm-fe")
    sub.add_parser("clean", help="remove overlays and scratch")
    args = ap.parse_args(argv)

    if args.cmd in ("build-rtm", "build-rtm-fe"):
        shim_build._ensure_wine_env()
        build_overlay("rtm" if args.cmd == "build-rtm" else "rtm-fe",
                      force=args.force)
        rc = 0
    elif args.cmd == "run":
        rc = run(args)
    elif args.cmd == "verify":
        rc = run_verify(args.gen)
    else:
        rc = run_clean()
    import shlex
    _common.log_invocation(rc, shlex.join(
        ["python3", "-m", "homm3.vc6.genab", *argv]))
    return rc


if __name__ == "__main__":
    sys.exit(main())
