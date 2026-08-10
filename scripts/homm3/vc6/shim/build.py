#!/usr/bin/env python3
"""homm3.vc6.shim.build - build the C2-slot shim overlay + inertness gate.

The shim (passthru.c) is a drop-in C2.DLL that logs the argv the CL driver
hands to the back end and forwards, unchanged, to the real C2.DLL.  This
module makes that mechanism usable and PROVES it inert:

  build     create build/vc6/toolchain-shim/msvc - a copy-overlay of the
            pinned toolchain whose bin/ holds the real back end renamed to
            C2_real.dll and the shim (compiled by the pinned VC6 itself)
            installed as C2.DLL.  Compiling with MSVC_DIR=<overlay>/msvc
            puts the shim in the loop; nothing pinned is modified in place.
  gate      the byte-identity gate: compile shim/sample_tu.cpp with the game
            profile through BOTH toolchains and require the .obj bytes to be
            identical outside the COFF TimeDateStamp (file bytes 4..7, the
            one measured nondeterminism - docs/vc6/shim.md).  Also compile
            once with /Bd through the overlay and require the shim-logged
            argv to equal the driver's own printed C2 command line token for
            token, then cross-check against the homm3.vc6.argv model.
  negative  the gate's negative control: install the deliberately non-inert
            shim variant (drops every "-Gy" token), require the gate to go
            RED, then restore the clean shim and require it green again.
  clean     remove the overlay and gate scratch.

rc: 0 = green, 1 = a gate answered NO, 2 = harness error (vc6 convention).
The canonical shim log is build/vc6/shim/argv.log (= homm3.vc6.argv.SHIM_LOG,
so `homm3 vc6 argv --verify` reads what the gate's last clean run captured).
Run inside `nix develop .#build` - the wine client must match the running
wineserver.
"""
from __future__ import annotations

import argparse
import hashlib
import os
import shutil
import struct
import subprocess
import sys
from pathlib import Path

from homm3.core import cc_wrap
from homm3.vc6 import _common, _toolchain

SHIM_DIR = Path(__file__).resolve().parent
OVERLAY = _common.REPO / "build/vc6/toolchain-shim"
OVERLAY_MSVC = OVERLAY / "msvc"
GATE_DIR = _common.REPO / "build/vc6/shim/gate"
ARGV_LOG = _common.REPO / "build/vc6/shim/argv.log"   # == homm3.vc6.argv.SHIM_LOG
SAMPLE = SHIM_DIR / "sample_tu.cpp"
GATE_FLAGS = ["/c", "/O2", "/Ob2", "/Oy-", "/Op", "/ML", "/Gr", "/GX"]
EXPECTED_EXPORTS = {"_InvokeCompilerPass@12", "_AbortCompilerPass@4"}
TS_BYTES = range(4, 8)  # COFF FileHeader.TimeDateStamp - the masked window


# ---------------------------------------------------------------------------
# environment / process plumbing
# ---------------------------------------------------------------------------

def _ensure_wine_env() -> None:
    """Mirror cc_wrap.main's wine anchoring for our own direct invocations."""
    if shutil.which("wine") is None:
        _common.die("wine not found - run inside `nix develop .#build`.")
    os.environ.setdefault("WINEDEBUG", "fixme-all,err-kerberos")
    if not Path(os.environ.get("WINEPREFIX", "")).is_dir():
        os.environ["WINEPREFIX"] = str(_common.REPO / "build/wineprefix")
    cc_wrap.ensure_wineserver()


def _wine(exe: Path, args: list[str], cwd: Path,
          extra_env: dict[str, str] | None = None) -> subprocess.CompletedProcess:
    env = os.environ.copy()
    env.update(extra_env or {})
    cwd.mkdir(parents=True, exist_ok=True)
    return subprocess.run(["wine", str(exe), *args], cwd=str(cwd), env=env,
                          stdin=subprocess.DEVNULL, capture_output=True,
                          encoding="latin1", errors="replace", timeout=300)


def _cc_wrap(out: Path, src: Path, flags: list[str],
             extra_env: dict[str, str] | None = None) -> subprocess.CompletedProcess:
    """One compile through the canonical wrapper (what ninja itself runs)."""
    env = os.environ.copy()
    env.pop("MSVC_DIR", None)               # default = the real toolchain
    env.update(extra_env or {})
    scripts = str(_common.REPO / "scripts")
    env["PYTHONPATH"] = scripts + os.pathsep + env.get("PYTHONPATH", "")
    cmd = [sys.executable, "-m", "homm3.core.cc_wrap",
           "--out", str(out), "--src", str(src), "--", *flags]
    return subprocess.run(cmd, env=env, capture_output=True, text=True)


def _tail(proc: subprocess.CompletedProcess, n: int = 10) -> str:
    return "\n".join((proc.stdout + proc.stderr).strip().splitlines()[-n:])


# ---------------------------------------------------------------------------
# PE export reader (local: the shim DLL is ours, not a pinned binary)
# ---------------------------------------------------------------------------

def dll_exports(path: Path) -> set[str]:
    d = path.read_bytes()
    pe = struct.unpack_from("<I", d, 0x3c)[0]
    if d[pe:pe + 4] != b"PE\0\0":
        _common.die(f"{path}: no PE header")
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    optsize = struct.unpack_from("<H", d, pe + 20)[0]
    exp_rva, exp_size = struct.unpack_from("<II", d, pe + 24 + 96)
    secs = []
    off = pe + 24 + optsize
    for _ in range(nsec):
        vsize, va, rsize, raw = struct.unpack_from("<IIII", d, off + 8)
        secs.append((va, max(vsize, rsize), raw))
        off += 40

    def rva2off(rva: int) -> int:
        for va, size, raw in secs:
            if va <= rva < va + size:
                return raw + (rva - va)
        _common.die(f"{path}: rva {rva:#x} outside sections")

    if exp_rva == 0:
        return set()
    e = rva2off(exp_rva)
    nnames = struct.unpack_from("<I", d, e + 0x18)[0]
    names_off = rva2off(struct.unpack_from("<I", d, e + 0x20)[0])
    out = set()
    for i in range(nnames):
        no = rva2off(struct.unpack_from("<I", d, names_off + 4 * i)[0])
        out.add(d[no:d.index(b"\0", no)].decode("latin1"))
    return out


# ---------------------------------------------------------------------------
# overlay construction
# ---------------------------------------------------------------------------

def build_overlay(force: bool = False) -> None:
    """Copy-overlay of the pinned msvc tree: bin/ copied (DLLs must be real
    files so the driver's LoadLibraryA hits the shim), the rest symlinked."""
    _toolchain.resolve("C2.DLL")            # hash-gate BEFORE copying; also
    _toolchain.resolve("CL.EXE")            # refuses to run with MSVC_DIR
    _toolchain.resolve("LINK.EXE")          # already pointed at an overlay
    real_msvc = cc_wrap.msvc_dir()
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
        cc_wrap.find_ci(bin_dir, "c2.dll").rename(bin_dir / "C2_real.dll")
        print(f"[shim] overlay tree created at {OVERLAY_MSVC}")
    want_sha, want_size = _toolchain.PINNED["C2.DLL"]
    real_copy = bin_dir / "C2_real.dll"
    if (real_copy.stat().st_size != want_size
            or hashlib.sha256(real_copy.read_bytes()).hexdigest() != want_sha):
        _common.die(f"{real_copy}: not the pinned C2 pressing - "
                    "rebuild the overlay with `build --force`")


def compile_shim(negative: bool = False) -> set[str]:
    """Compile passthru.c with the pinned VC6 into <overlay>/bin/C2.DLL."""
    real_cl = _toolchain.resolve("CL.EXE")
    real_link = _toolchain.resolve("LINK.EXE")
    real_msvc = cc_wrap.msvc_dir()
    work = OVERLAY / "shim-build"
    work.mkdir(parents=True, exist_ok=True)
    w = cc_wrap.winepath_w
    obj, dll = work / "passthru.obj", work / "C2.DLL"
    for p in (obj, dll):
        p.unlink(missing_ok=True)

    cc_args = ["/c", "/nologo", "/W3", "/O1"]
    if negative:
        cc_args.append("/DSHIM_NEGATIVE_CONTROL")
    cc_args += [f"/Fo{w(obj)}", w(SHIM_DIR / "passthru.c")]
    proc = _wine(real_cl, cc_args, work, {"INCLUDE": w(real_msvc / "include")})
    if not obj.is_file():
        _common.die(f"shim compile failed:\n{_tail(proc)}")

    k32 = cc_wrap.find_ci(real_msvc / "lib", "kernel32.lib")
    if k32 is None:
        _common.die(f"kernel32.lib not under {real_msvc}/lib")
    ld_args = ["/nologo", "/DLL", "/NOENTRY",
               f"/DEF:{w(SHIM_DIR / 'passthru.def')}",
               f"/OUT:{w(dll)}", w(obj), w(k32)]
    proc = _wine(real_link, ld_args, work)
    if not dll.is_file():
        _common.die(f"shim link failed:\n{_tail(proc)}")

    exports = dll_exports(dll)
    if exports != EXPECTED_EXPORTS:
        _common.die(f"shim exports {sorted(exports)} != "
                    f"expected {sorted(EXPECTED_EXPORTS)}")
    shutil.copy2(dll, OVERLAY_MSVC / "bin" / "C2.DLL")
    variant = "NEGATIVE-CONTROL" if negative else "clean"
    print(f"[shim] {variant} shim installed as {OVERLAY_MSVC / 'bin' / 'C2.DLL'}"
          f" (exports: {', '.join(sorted(exports))})")
    return exports


def ensure_overlay() -> None:
    if not (OVERLAY_MSVC / "bin" / "C2_real.dll").is_file():
        build_overlay()
        compile_shim(negative=False)
    else:
        build_overlay()   # re-gates the pinned hashes even when present
        if not (OVERLAY_MSVC / "bin" / "C2.DLL").is_file():
            compile_shim(negative=False)


# ---------------------------------------------------------------------------
# the gate
# ---------------------------------------------------------------------------

def _masked_diff(a: bytes, b: bytes) -> list[int]:
    """Byte positions that differ outside the COFF TimeDateStamp window."""
    n = min(len(a), len(b))
    diffs = [i for i in range(n) if a[i] != b[i] and i not in TS_BYTES]
    diffs += list(range(n, max(len(a), len(b))))
    return diffs


def _last_c2_line(text: str) -> list[str] | None:
    """Last non-comment line mentioning c2.dll, tokenized (log or /Bd)."""
    hit = None
    for line in text.splitlines():
        line = line.strip()
        if line.startswith("#"):
            continue
        if "c2.dll" in line.lower():
            hit = line.strip("`'")
    return hit.split() if hit else None


def _identity_check(tag: str, overlay_env: dict[str, str]) -> tuple[bool, str]:
    """Compile SAMPLE via real and overlay toolchains; compare masked bytes."""
    out_dir = GATE_DIR / tag
    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True)
    ref, shim = out_dir / "ref.obj", out_dir / "shim.obj"
    p1 = _cc_wrap(ref, SAMPLE, GATE_FLAGS)
    if not ref.is_file():
        _common.die(f"reference compile failed:\n{_tail(p1)}")
    p2 = _cc_wrap(shim, SAMPLE, GATE_FLAGS, overlay_env)
    if not shim.is_file():
        _common.die(f"overlay compile failed:\n{_tail(p2)}")
    a, b = ref.read_bytes(), shim.read_bytes()
    diffs = _masked_diff(a, b)
    if not diffs:
        return True, (f"byte-identical outside TimeDateStamp "
                      f"({len(a)} bytes, ref={ref}, shim={shim})")
    head = ", ".join(f"0x{i:x}" for i in diffs[:10])
    return False, (f"{len(diffs)} byte(s) differ (sizes {len(a)}/{len(b)}), "
                   f"first at: {head}  [ref={ref} shim={shim}]")


def _bd_check() -> tuple[bool, str, list[str] | None]:
    """/Bd oracle: one overlay compile; driver-printed C2 line must equal the
    shim-logged argv token for token (same process, same temp names)."""
    bd_dir = GATE_DIR / "bd"
    if bd_dir.exists():
        shutil.rmtree(bd_dir)
    bd_dir.mkdir(parents=True)
    bd_log = bd_dir / "bd-argv.log"
    w = cc_wrap.winepath_w
    cl = cc_wrap.find_ci(OVERLAY_MSVC / "bin", "cl.exe")
    args = [*GATE_FLAGS, "/Bd", f"/Fo{w(bd_dir / 'sample_bd.obj')}", w(SAMPLE)]
    proc = _wine(cl, args, bd_dir,
                 {"INCLUDE": w(OVERLAY_MSVC / "include"),
                  "HOMM3_VC6_SHIM_LOG": w(bd_log)})
    if not (bd_dir / "sample_bd.obj").is_file():
        _common.die(f"/Bd overlay compile failed:\n{_tail(proc)}")
    printed = _last_c2_line(proc.stdout)
    if printed is None:
        _common.die(f"/Bd printed no c2 command line:\n{_tail(proc)}")
    if not bd_log.is_file():
        return False, "shim wrote no log - is the shim actually loaded?", None
    logged = _last_c2_line(bd_log.read_text(errors="replace"))
    if logged is None:
        return False, f"no argv line in {bd_log}", None
    if logged == printed:
        return True, f"{len(logged)} tokens equal to the driver's /Bd line", logged
    delta = next((i for i, (x, y) in enumerate(zip(logged, printed)) if x != y),
                 min(len(logged), len(printed)))
    return False, (f"token {delta} differs: logged={logged[delta:delta+1]} "
                   f"printed={printed[delta:delta+1]}"), logged


def _model_check(canonical_flags: list[str]) -> tuple[bool, str]:
    """Compare the canonical run's logged argv with the homm3.vc6.argv model
    (same flags cc_wrap sent, -il value normalized to the model's <tmp>)."""
    from homm3.vc6 import argv as argv_mod
    logged = _last_c2_line(ARGV_LOG.read_text(errors="replace"))
    if logged is None:
        return False, f"no argv line in {ARGV_LOG}"
    model = argv_mod.expand(canonical_flags)

    def norm(tokens: list[str]) -> list[str]:
        out, skip = [], False
        for t in tokens:
            out.append("<tmp>" if skip else t)
            skip = (t == "-il")
        return out

    actual, predicted = norm(logged[1:]), norm(model["c2"])
    if actual == predicted:
        return True, f"{len(actual)} tokens agree with homm3.vc6.argv"
    delta = next((i for i, (x, y) in enumerate(zip(actual, predicted))
                  if x != y), min(len(actual), len(predicted)))
    return False, (f"token {delta}: logged={actual[delta:delta+1]} "
                   f"model={predicted[delta:delta+1]} "
                   f"(logged n={len(actual)}, model n={len(predicted)})")


def run_gate(rebuild: bool = False) -> int:
    _ensure_wine_env()
    if rebuild:
        build_overlay(force=True)
        compile_shim(negative=False)
    else:
        ensure_overlay()
    rc = 0

    # 1. argv capture vs the /Bd oracle (throwaway log, separate obj)
    ok, detail, _ = _bd_check()
    print(f"[shim] gate /Bd-argv    {'PASS' if ok else 'FAIL'}  {detail}")
    rc = rc or (0 if ok else 1)

    # 2. byte identity, canonical log LAST so argv --verify sees a plain run
    ARGV_LOG.parent.mkdir(parents=True, exist_ok=True)
    ARGV_LOG.write_text("")
    w = cc_wrap.winepath_w
    overlay_env = {"MSVC_DIR": str(OVERLAY_MSVC),
                   "HOMM3_VC6_SHIM_LOG": w(ARGV_LOG)}
    ok, detail = _identity_check("identity", overlay_env)
    print(f"[shim] gate identity    {'PASS' if ok else 'FAIL'}  {detail}")
    rc = rc or (0 if ok else 1)
    if not ARGV_LOG.read_text(errors="replace").strip():
        print(f"[shim] gate shim-log    FAIL  {ARGV_LOG} empty - shim not in loop")
        rc = rc or 1
        return rc

    # 3. model agreement on exactly what cc_wrap sent for the shim compile
    shim_obj = GATE_DIR / "identity" / "shim.obj"
    canonical = [*GATE_FLAGS, f"/Fo{w(shim_obj)}", w(SAMPLE)]
    ok, detail = _model_check(canonical)
    print(f"[shim] gate argv-model  {'PASS' if ok else 'FAIL'}  {detail}")
    rc = rc or (0 if ok else 1)

    print(f"[shim] gate {'GREEN' if rc == 0 else 'RED'}")
    return rc


def run_negative() -> int:
    """Negative control: the gate MUST reject a shim that mutates the argv."""
    _ensure_wine_env()
    ensure_overlay()
    rc = 0
    compile_shim(negative=True)
    try:
        overlay_env = {"MSVC_DIR": str(OVERLAY_MSVC)}   # scratch run, no log
        identical, detail = _identity_check("negative", overlay_env)
        if identical:
            print("[shim] negative control FAIL  mutated shim went undetected "
                  f"({detail})")
            rc = 1
        else:
            print(f"[shim] negative control PASS  gate went red as required "
                  f"({detail})")
    finally:
        compile_shim(negative=False)
    identical, detail = _identity_check("restore", {"MSVC_DIR": str(OVERLAY_MSVC)})
    print(f"[shim] restore identity {'PASS' if identical else 'FAIL'}  {detail}")
    return rc or (0 if identical else 1)


def run_clean() -> int:
    for p in (OVERLAY, _common.REPO / "build/vc6/shim"):
        if p.exists():
            shutil.rmtree(p)
            print(f"[shim] removed {p}")
    return 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    ap = argparse.ArgumentParser(
        prog="python3 -m homm3.vc6.shim.build", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build", help="create/refresh the shim overlay")
    b.add_argument("--force", action="store_true",
                   help="recreate the overlay tree from scratch")
    g = sub.add_parser("gate", help="byte-identity + argv gates")
    g.add_argument("--rebuild", action="store_true",
                   help="force-rebuild the overlay first")
    sub.add_parser("negative", help="negative control (must go red, then restore)")
    sub.add_parser("clean", help="remove overlay and gate scratch")
    args = ap.parse_args(argv)

    if args.cmd == "build":
        _ensure_wine_env()
        build_overlay(force=args.force)
        compile_shim(negative=False)
        rc = 0
    elif args.cmd == "gate":
        rc = run_gate(rebuild=args.rebuild)
    elif args.cmd == "negative":
        rc = run_negative()
    else:
        rc = run_clean()
    import shlex
    _common.log_invocation(rc, shlex.join(
        ["python3", "-m", "homm3.vc6.shim.build", *argv]))
    return rc


if __name__ == "__main__":
    sys.exit(main())
