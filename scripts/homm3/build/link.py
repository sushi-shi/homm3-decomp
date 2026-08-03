#!/usr/bin/env python3
"""homm3.build.link - link the recompiled base objs into a candidate EXE (+ .map).

Runs the genuine VC6 SP3 `link.exe` (LINK 6.00.8447 - the generation that built
retail HEROES3.EXE) under wine over our base `.obj`s. The reconstruction is
partial (only zlib compiles today), so the EXE is NOT runnable; we link it
anyway, with `/FORCE`, to study the LAYOUT the linker produces: the `.map`
gives every function's link-assigned address and its source object, which is
what lets us reverse-engineer the retail object order later (intra-TU order =
source-definition order; cross-TU order = object link order).

What it does:
  1. assemble the obj list (a dir of <unit>.obj, explicit --obj, or an --order
     file giving the exact link order to test), winepath-translate every path,
     and write a `@response` file (link's argv limit under wine is short);
  2. run `wine link.exe @rsp`; success signal is "the .EXE exists" (wine spews
     unrelated noise and can return odd exit codes, exactly like cc_wrap);
  3. save the unresolved-externals punch list next to the EXE (the
     drive-to-linkable worklist).

VC6 note (divergence from the Gruntz/VC5 template): no MSDIS stub is needed.
VC6 LINK.EXE's static imports are only mspdb60/msvcrt/kernel32 (verified by
walking its import table); MSDIS110.DLL is loaded dynamically and only by the
`/dump /disasm` path. MSPDB60.DLL ships next to link.exe in the toolchain.

Defaults are tuned for layout study, not a shippable binary:
  /FORCE /NODEFAULTLIB /SUBSYSTEM:WINDOWS /BASE:0x400000 /INCREMENTAL:NO /MAP
  /OPT:NOREF /OPT:NOICF   (keep EVERY function so the map is complete)

Run inside `nix develop .#build`:
    homm3 link [-- <extra link flags>]
    python3 -m homm3.build.link --order config/link-order.txt
"""
from __future__ import annotations

import argparse
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
from pathlib import Path

from homm3.core.cc_wrap import (HOMM3_DIR, ensure_wineserver, find_ci, msvc_dir,
                                winepath_w)


def die(msg: str) -> None:
    print(f"[link] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def run_wine(cmd: list, cwd, produced: Path):
    """Run a wine command hang-proof; return (output, rc). Mirrors cc_wrap:
    wine can leave a finished-but-unreaped grandchild holding stdio open, so log
    to a temp FILE (no pipe to block on), own process group, bounded wait."""
    timeout = float(os.environ.get("HOMM3_LINK_TIMEOUT", "300"))
    with tempfile.TemporaryFile() as logf:
        proc = subprocess.Popen(cmd, cwd=str(cwd), stdin=subprocess.DEVNULL,
                                stdout=logf, stderr=subprocess.STDOUT,
                                start_new_session=True)
        try:
            proc.wait(timeout=timeout)
            rc = proc.returncode
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except (ProcessLookupError, PermissionError):
                pass
            proc.wait()
            rc = 0 if produced.exists() else 1
        logf.seek(0)
        return logf.read().decode("latin1", "replace"), rc


def collect_objs(args) -> list:
    """Resolve the obj list + their link ORDER. Priority:
       --order FILE  (one obj stem or path per line; blank/`#` ignored) - the
                     order is significant, this is how we test a hypothesised
                     retail link order;
       --obj ...     explicit paths, in the given order;
       --objs-dir    every *.obj in the dir, sorted by name (stable default).
    """
    objs_dir = Path(args.objs_dir)
    if args.order:
        objs = []
        for line in Path(args.order).read_text().splitlines():
            stem = line.strip()
            if not stem or stem.startswith("#"):
                continue
            path = Path(stem)
            if not path.suffix:
                path = objs_dir / f"{stem}.obj"
            if not path.exists():
                die(f"order entry not found: {stem} ({path})")
            objs.append(path)
        return objs
    if args.obj:
        return [Path(o) for o in args.obj]
    if not objs_dir.is_dir():
        die(f"--objs-dir not found: {objs_dir}")
    return sorted(objs_dir.glob("*.obj"))


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="VC6 link.exe wrapper (candidate link).")
    ap.add_argument("--out", default="build/exe/HEROES3.candidate.EXE")
    ap.add_argument("--map", dest="mapfile", default=None,
                    help="map path (default: <out> with .map suffix).")
    ap.add_argument("--objs-dir", default="build/objdiff/base")
    ap.add_argument("--obj", action="append", help="explicit obj (repeatable).")
    ap.add_argument("--order", help="file listing obj stems/paths in link order.")
    ap.add_argument("--lib", action="append", default=[],
                    help="extra import/static lib to pass to link (repeatable).")
    ap.add_argument("--base", default="0x400000", help="image base (/BASE).")
    ap.add_argument("--entry", default="_x", help="forced /ENTRY symbol.")
    ap.add_argument("--keep-all", dest="keep_all", action="store_true", default=True,
                    help="/OPT:NOREF /OPT:NOICF - keep every COMDAT (default).")
    ap.add_argument("--opt-ref", dest="keep_all", action="store_false",
                    help="let the linker strip/fold unreferenced COMDATs (/OPT:REF).")
    ap.add_argument("flags", nargs=argparse.REMAINDER,
                    help="extra link flags after `--`.")
    args = ap.parse_args(argv)

    if shutil.which("wine") is None:
        die("wine not found - run inside `nix develop .#build`.")
    msvc = msvc_dir()
    link = find_ci(msvc / "bin", "link.exe")
    if not link:
        die(f"link.exe not found under {msvc}/bin - run `homm3 init` first.")
    if not Path(os.environ.get("WINEPREFIX", "")).is_dir():
        os.environ["WINEPREFIX"] = str(HOMM3_DIR / "build/wineprefix")

    out = Path(args.out).resolve()
    mapf = Path(args.mapfile).resolve() if args.mapfile else out.with_suffix(".map")
    out.parent.mkdir(parents=True, exist_ok=True)
    for f in (out, mapf):
        if f.exists():
            f.unlink()

    objs = collect_objs(args)
    if not objs:
        die("no objects to link.")

    os.environ.setdefault("WINEDEBUG", "fixme-all,err-kerberos")
    ensure_wineserver()

    rsp_lines = [
        f"/OUT:{winepath_w(out)}",
        f"/MAP:{winepath_w(mapf)}",
        "/NOLOGO", "/FORCE", "/NODEFAULTLIB", "/SUBSYSTEM:WINDOWS",
        f"/BASE:{args.base}", "/INCREMENTAL:NO", f"/ENTRY:{args.entry}",
    ]
    if args.keep_all:
        rsp_lines += ["/OPT:NOREF", "/OPT:NOICF"]
    extra = args.flags[1:] if args.flags and args.flags[0] == "--" else args.flags
    rsp_lines += list(extra)
    rsp_lines += [winepath_w(Path(lib)) if os.path.exists(lib) else lib
                  for lib in args.lib]
    rsp_lines += [f'"{winepath_w(o)}"' for o in objs]

    rsp = out.parent / (out.stem + ".objs.rsp")
    rsp.write_text("\n".join(rsp_lines) + "\n")

    output, rc = run_wine(["wine", str(link), f"@{winepath_w(rsp)}"],
                          out.parent, out)

    if not out.exists():
        sys.stderr.write(f"[link] FAILED to produce {out}\n")
        sys.stderr.write("\n".join(output.strip().splitlines()[-20:]) + "\n")
        return rc or 1

    # /FORCE means unresolved externals are EXPECTED (partial reconstruction);
    # surface the counts but treat the produced EXE as success.
    warns = sum(1 for ln in output.splitlines() if "LNK4006" in ln)
    unresolved = sorted({m.group(1) for ln in output.splitlines()
                         if (m := re.search(r"unresolved external symbol (\S+)", ln))})
    punch = out.parent / (out.stem + ".unresolved.txt")
    punch.write_text("\n".join(unresolved) + "\n")
    shown = out.relative_to(HOMM3_DIR) if out.is_relative_to(HOMM3_DIR) else out
    print(f"[link] {len(objs)} objs -> {shown} ({out.stat().st_size} B) + {mapf.name}")
    print(f"[link] {len(unresolved)} unresolved externals -> {punch.name}, "
          f"{warns} dup-symbol warnings (expected: partial reconstruction, /FORCE)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
