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

  build-rtm  create build/vc6/toolchain-rtm/msvc - the same copy-overlay
             mechanism as homm3.vc6.shim.build (bin/ copied so the driver's
             LoadLibraryA resolves inside the overlay, the rest symlinked),
             with the RTM C2.DLL installed as bin/C2.DLL.  The RTM DLL is
             staged OUTSIDE the repo at ../orig/vc6-rtm/C2.DLL and is
             hash-gated (sha256+size+FileVersion+export table) before every
             install - a wrong pressing is a hard abort.
  run        A/B the wall corpus (or --fn targets), emit
             evidence/vc6/c2-generation-verdicts.tsv.  Verdict per function:
               SP3-matches      SP3 output already matches retail at the
                                register-visible + branch-shape grade
               RTM-closes       SP3 diverges, the RTM back end matches
               neither          both diverge (columns show which is closer)
               RTM-unavailable  the RTM overlay could not be built/used
  clean      remove the overlay and scratch.

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
import shutil
import struct
import sys
from pathlib import Path

from homm3.core import cc_wrap
from homm3.sema import _asm
from homm3.vc6 import _align, _common, _flow, _toolchain
from homm3.vc6.shim import build as shim_build

OVERLAY = _common.REPO / "build/vc6/toolchain-rtm"
OVERLAY_MSVC = OVERLAY / "msvc"
SCRATCH = _common.REPO / "build/vc6/genab"
VERDICTS = _common.EVIDENCE / "c2-generation-verdicts.tsv"
UNITS_TOML = _common.REPO / "config/units.toml"

# The RTM back end, staged OUTSIDE the repo (game/toolchain bytes never
# enter git).  Provenance: archive.org item
# 1998-10-01-visual-studio-6.0-enterprise-edition-disc-1, VSE600EUN1.ISO,
# VC98/BIN/C2.DLL, cross-verified byte-identical against item
# vs-6.0-enterprise VS6.0Enterprise_Disk1.iso (docs/vc6/rtm-generation.md,
# ../orig/vc6-rtm/PROVENANCE.txt).
RTM_C2 = _common.REPO.parent / "orig/vc6-rtm/C2.DLL"
RTM_PINNED = ("45187b0b6288240f73272a7c61e6329c50048a76e57db3bb87b6f0229e09e27d",
              737329)
RTM_FILEVERSION = "12.00.8168.0"

# The wall corpus: the merged-return / tail-duplication walls named in the
# Track R brief plus every function whose in-tree residual comment cites
# merged-return / DUP-EXIT / tail-merge / the CL-generation class
# (grep src/*.cpp, 2026-08-09).  move_toward and get_ranged_attack_value
# are byte-exact controls - their comments cite the DUP-EXIT shape as
# CLOSED, so they must come back SP3-matches or the harness is broken.
CORPUS = [
    "?FindPath@army@@QAEHHHEE@Z",                    # path - merged bounds guards
    "?GetAdjacentCellIndex@army@@QBEHHH@Z",          # path
    "?ValidAttack@army@@QAEHHHHHPAH@Z",              # path - cross-jumped inline tails
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


def gate_rtm_dll() -> Path:
    """Hash-gate the staged RTM C2.DLL; abort on any mismatch."""
    if not RTM_C2.is_file():
        _common.die(f"RTM C2.DLL not staged at {RTM_C2} - see "
                    "docs/vc6/rtm-generation.md for the sourcing provenance")
    want_sha, want_size = RTM_PINNED
    size = RTM_C2.stat().st_size
    if size != want_size:
        _common.die(f"{RTM_C2}: size {size} != pinned {want_size}")
    sha = hashlib.sha256(RTM_C2.read_bytes()).hexdigest()
    if sha != want_sha:
        _common.die(f"{RTM_C2}: sha256 {sha} != pinned {want_sha} - "
                    "not the admitted RTM pressing")
    ver = _fileversion(RTM_C2)
    if ver != RTM_FILEVERSION:
        _common.die(f"{RTM_C2}: FileVersion {ver} != {RTM_FILEVERSION}")
    exports = shim_build.dll_exports(RTM_C2)
    if exports != shim_build.EXPECTED_EXPORTS:
        _common.die(f"{RTM_C2}: exports {sorted(exports)} != "
                    f"{sorted(shim_build.EXPECTED_EXPORTS)}")
    return RTM_C2


# ---------------------------------------------------------------------------
# overlay construction (mirrors homm3.vc6.shim.build.build_overlay)
# ---------------------------------------------------------------------------

def build_overlay(force: bool = False) -> None:
    """Copy-overlay of the pinned msvc tree with the RTM C2.DLL in bin/.
    bin/ is copied (the driver's LoadLibraryA must resolve real files
    inside the overlay); all other top-level entries are symlinks."""
    _toolchain.resolve("C2.DLL")     # gate the pinned pressing BEFORE copying;
    _toolchain.resolve("CL.EXE")     # also refuses to run with MSVC_DIR
    real_msvc = cc_wrap.msvc_dir()   # already pointed at an overlay
    rtm = gate_rtm_dll()
    if force and OVERLAY_MSVC.exists():
        shutil.rmtree(OVERLAY_MSVC)
    bin_dir = OVERLAY_MSVC / "bin"
    if not bin_dir.is_dir():
        OVERLAY_MSVC.mkdir(parents=True, exist_ok=True)
        for entry in sorted(real_msvc.iterdir()):
            if entry.name.lower() == "bin":
                continue
            dst = OVERLAY_MSVC / entry.name
            if not (dst.exists() or dst.is_symlink()):
                dst.symlink_to(entry.resolve())
        bin_dir.mkdir()
        for f in sorted((real_msvc / "bin").iterdir()):
            if f.is_file():
                shutil.copy2(f, bin_dir / f.name)
        print(f"[genab] overlay tree created at {OVERLAY_MSVC}")
    # Install (or re-verify) the RTM back end as the overlay's C2.DLL.
    c2 = cc_wrap.find_ci(bin_dir, "c2.dll")
    if c2 is None:
        _common.die(f"no c2.dll in {bin_dir} - rebuild with --force")
    want_sha, want_size = RTM_PINNED
    if (c2.stat().st_size != want_size
            or hashlib.sha256(c2.read_bytes()).hexdigest() != want_sha):
        shutil.copy2(rtm, c2)
        installed = hashlib.sha256(c2.read_bytes()).hexdigest()
        if installed != want_sha:
            _common.die(f"{c2}: post-install sha {installed} != pinned")
        print(f"[genab] RTM C2 12.00.8168 installed as {c2}")
    # The rest of the overlay bin/ must still be the pinned pressing.
    for name in ("CL.EXE", "C1.DLL", "C1XX.DLL"):
        f = cc_wrap.find_ci(bin_dir, name)
        want_sha, want_size = _toolchain.PINNED[name]
        if (f is None or f.stat().st_size != want_size
                or hashlib.sha256(f.read_bytes()).hexdigest() != want_sha):
            _common.die(f"overlay {name} is not the pinned pressing - "
                        "rebuild with --force")


def rtm_available() -> tuple[bool, str]:
    """(usable, reason). Non-fatal probe used by `run`."""
    if not RTM_C2.is_file():
        return False, f"RTM C2.DLL not staged at {RTM_C2}"
    want_sha, want_size = RTM_PINNED
    if RTM_C2.stat().st_size != want_size:
        return False, f"{RTM_C2} size != pinned"
    if hashlib.sha256(RTM_C2.read_bytes()).hexdigest() != want_sha:
        return False, f"{RTM_C2} sha256 != pinned"
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
    toolchain; side='rtm' points MSVC_DIR at the RTM overlay.  Cached per
    invocation directory; (obj|None, error-tail)."""
    out = SCRATCH / side / f"{unit}.obj"
    if out.is_file():
        return out, ""
    env = {"MSVC_DIR": str(OVERLAY_MSVC)} if side == "rtm" else None
    src = _common.REPO / spec["source"]
    if not src.is_file():
        return None, f"source missing: {src}"
    proc = shim_build._cc_wrap(out, src, spec["flags"], env)
    if not out.is_file():
        tail = "\n".join((proc.stdout + proc.stderr).strip().splitlines()[-6:])
        return None, tail
    return out, ""


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
    shim_build._ensure_wine_env()
    manifest = load_manifest()
    from homm3.sema.context import get_context
    ctx = get_context()

    rtm_ok, rtm_reason = rtm_available()
    if rtm_ok:
        try:
            build_overlay()
        except SystemExit:
            raise                      # rc-2 die(): a WRONG pressing must abort,
    else:                              # only ABSENCE degrades to RTM-unavailable
        print(f"[genab] RTM side unavailable: {rtm_reason}")

    if args.fresh and SCRATCH.exists():
        shutil.rmtree(SCRATCH)

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

        rtm_obj, tail = compile_unit(unit, manifest[unit], "rtm")
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
        if sp3_total == 0:
            verdict = "SP3-matches"
            note = "" if rtm_total == 0 else \
                f"RTM diverges by {rtm_total} where SP3 is exact"
        elif rtm_total == 0:
            verdict = "RTM-closes"
            note = "generation artifact - closable with zero RE"
        else:
            verdict = "neither"
            if ab_align + ab_flow == 0:
                note = "RTM output identical to SP3 - not a generation wall"
            elif rtm_total < sp3_total:
                note = f"RTM closer ({rtm_total} vs {sp3_total})"
            elif rtm_total > sp3_total:
                note = f"RTM farther ({rtm_total} vs {sp3_total})"
            else:
                note = f"equidistant ({sp3_total}), different bytes"
        rows.append(_row(name, unit, rva, size, sp3=(sp3_align, sp3_flow),
                         rtm=(rtm_align, rtm_flow), ab=(ab_align, ab_flow),
                         producer=producer, verdict=verdict, note=note))

    _write_tsv(rows, rtm_ok)
    _print_rows(rows)
    return 0 if rtm_ok and all(r["verdict"] != "RTM-unavailable"
                               for r in rows) else 1


def _row(name, unit, rva, size, sp3=None, rtm=None, ab=None,
         producer="", verdict="", note=""):
    return {"fn": name, "unit": unit, "rva": f"0x{rva:x}",
            "size": f"0x{size:x}",
            "sp3_align": "" if sp3 is None else sp3[0],
            "sp3_flow": "" if sp3 is None else sp3[1],
            "rtm_align": "" if rtm is None else rtm[0],
            "rtm_flow": "" if rtm is None else rtm[1],
            "sp3_vs_rtm": "" if ab is None else ab[0] + ab[1],
            "retail_producer": producer, "verdict": verdict, "note": note}


_COLS = ["fn", "unit", "rva", "size", "sp3_align", "sp3_flow",
         "rtm_align", "rtm_flow", "sp3_vs_rtm", "retail_producer",
         "verdict", "note"]


def _write_tsv(rows: list, rtm_ok: bool) -> None:
    want_sha, want_size = RTM_PINNED
    sp3_sha, sp3_size = _toolchain.PINNED["C2.DLL"]
    head = _common.provenance("python3 -m homm3.vc6.genab run", extra=[
        f"# sp3-c2: 12.00.8447 sha256 {sp3_sha} size {sp3_size} (pinned toolchain)",
        f"# rtm-c2: {RTM_FILEVERSION} sha256 {want_sha} size {want_size} "
        f"({'staged ../orig/vc6-rtm/C2.DLL' if rtm_ok else 'NOT STAGED'})",
        "# metric: align = _align.distance (unpaired register-visible masked "
        "slots), flow = _flow.distance (branch-shape); verdict grades at "
        "align+flow==0 - REGISTER-VISIBLE+BRANCH-SHAPE, weaker than "
        "byte-exact (absolute addresses masked)",
        "# retail_producer: delinked = build/objdiff/target/<unit>.c.obj "
        "(same disassembler both sides); capstone = image producer "
        "(cross-producer skew inflates distances)",
    ])
    VERDICTS.parent.mkdir(parents=True, exist_ok=True)
    lines = head + ["\t".join(_COLS)]
    for r in rows:
        lines.append("\t".join(str(r[c]) for c in _COLS))
    VERDICTS.write_text("\n".join(lines) + "\n")
    print(f"[genab] wrote {VERDICTS.relative_to(_common.REPO)} "
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
    for p in (OVERLAY, SCRATCH):
        if p.exists():
            shutil.rmtree(p)
            print(f"[genab] removed {p}")
    return 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    ap = argparse.ArgumentParser(
        prog="python3 -m homm3.vc6.genab", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build-rtm", help="create/refresh the RTM overlay")
    b.add_argument("--force", action="store_true",
                   help="recreate the overlay tree from scratch")
    r = sub.add_parser("run", help="A/B the wall corpus, write the verdicts TSV")
    r.add_argument("--fn", action="append",
                   help="target function (mangled name or 0x-address); "
                        "repeatable; default = the built-in wall corpus")
    r.add_argument("--fresh", action="store_true",
                   help="discard cached unit objects first")
    sub.add_parser("clean", help="remove overlay and scratch")
    args = ap.parse_args(argv)

    if args.cmd == "build-rtm":
        shim_build._ensure_wine_env()
        build_overlay(force=args.force)
        rc = 0
    elif args.cmd == "run":
        rc = run(args)
    else:
        rc = run_clean()
    import shlex
    _common.log_invocation(rc, shlex.join(
        ["python3", "-m", "homm3.vc6.genab", *argv]))
    return rc


if __name__ == "__main__":
    sys.exit(main())
