#!/usr/bin/env python3
"""homm3.build.link - link the recompiled base objs into a candidate EXE (+ .map).

Runs the genuine VC6 SP3 `link.exe` (LINK 6.00.8447 - the generation that built
retail HEROES3.EXE) under wine over our base `.obj`s. The reconstruction is
partial, but the game links with the real CRT and vendor imports. Every link
requires a successful exit and no unresolved or duplicate symbols. A clean
link alone does not establish runtime correctness. The `.map`
gives every function's link-assigned address and its source object, which is
what lets us reverse-engineer the retail object order later (intra-TU order =
source-definition order; cross-TU order = object link order).

What it does:
  1. assemble the obj list (a dir of <unit>.obj, explicit --obj, or an --order
     file giving the exact link order to test), winepath-translate every path,
     and write a `@response` file (link's argv limit under wine is short);
  2. run `wine link.exe @rsp`; require a successful exit, an
     executable, and no unresolved or duplicate symbol diagnostics;
  3. save the unresolved-externals punch list next to the EXE (the
     drive-to-linkable worklist).

VC6 note (divergence from the Gruntz/VC5 template): no MSDIS stub is needed.
VC6 LINK.EXE's static imports are only mspdb60/msvcrt/kernel32 (verified by
walking its import table); MSDIS110.DLL is loaded dynamically and only by the
`/dump /disasm` path. MSPDB60.DLL ships next to link.exe in the toolchain.

The game links as retail did (config/retail facts, docs/vc6/runtime-link.md):
the objects in retail object order (homm3.build.link_order), the resources
of src/heroes3.rc (homm3.build.resources, gated against retail), the units a
`library` key archives (victor, zlib) as libraries, the retail library
line (RETAIL_LIBRARIES) and the objects' own default libraries (LIBCMT,
LIBCPMT, OLDNAMES), with
  /SUBSYSTEM:WINDOWS /BASE:0x400000 /INCREMENTAL:NO /OPT:REF
  /NODEFAULTLIB:LIBC   (the /ML victor and zlib objects ask for LIBC)
No /ENTRY: LINK's default for a Windows subsystem is the CRT's
WinMainCRTStartup, and naming it would pull wincrt0.obj ahead of the rest.

`--study` keeps the layout study of other images and experiments: objects
in name order, /OPT:NOREF /OPT:NOICF (every COMDAT kept, so the map is
complete) and /NODEFAULTLIB with explicit libraries.

Run inside `nix develop .#build`:
    homm3 link [-- <extra link flags>]
    homm3 link --study --order ORDER.txt
"""
from __future__ import annotations

import argparse
import os
import re
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from homm3.core.cc_wrap import (HOMM3_DIR, ensure_wineserver, find_ci, msvc_dir,
                                winepath_w)
from homm3.core.images import path as _image_path


def die(msg: str) -> None:
    print(f"[link] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def run_wine(cmd: list, cwd, env: dict | None = None):
    """Run a wine command hang-proof; return (output, rc). Mirrors cc_wrap:
    wine can leave a finished-but-unreaped grandchild holding stdio open, so log
    to a temp FILE (no pipe to block on), own process group, bounded wait."""
    timeout = float(os.environ.get("HOMM3_LINK_TIMEOUT", "300"))
    with tempfile.TemporaryFile() as logf:
        proc = subprocess.Popen(cmd, cwd=str(cwd), stdin=subprocess.DEVNULL,
                                stdout=logf, stderr=subprocess.STDOUT,
                                start_new_session=True, env=env)
        try:
            proc.wait(timeout=timeout)
            rc = proc.returncode
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except (ProcessLookupError, PermissionError):
                pass
            proc.wait()
            rc = 124
        logf.seek(0)
        return logf.read().decode("latin1", "replace"), rc


#: The retail library line, before the objects' default libraries. Retail
#: import descriptors are LINK's pull order, one library after another:
#: VERSION, WINMM, mss32, smackw32, DDRAW, WSOCK32, KERNEL32, USER32, GDI32,
#: ADVAPI32, SHELL32, ole32, binkw32, IFC20 (config/retail facts in
#: docs/reference/executable-libraries.md). KERNEL32 through ole32 follow
#: the VC6 AppWizard line (kernel32 user32 gdi32 winspool comdlg32 advapi32
#: shell32 ole32 oleaut32 uuid odbc32 odbccp32); the game's own libraries
#: surround it. victor.lib and zlib.lib come first: their members follow the
#: game objects in `.text` (victor 0x603590, zlib 0x604830), victor's first.
#: GUID_NULL is UUID.LIB's and the DirectPlay identifiers DXGUID.LIB's.
RETAIL_LIBRARIES = [
    "victor.lib", "zlib.lib",
    "version.lib", "winmm.lib", "mss32.lib", "smackw32.lib", "ddraw.lib",
    "wsock32.lib",
    "kernel32.lib", "user32.lib", "gdi32.lib", "winspool.lib", "comdlg32.lib",
    "advapi32.lib", "shell32.lib", "ole32.lib", "oleaut32.lib", "uuid.lib",
    "odbc32.lib", "odbccp32.lib",
    "binkw32.lib", "ifc20.lib", "dxguid.lib",
]

#: Retail links LIBCMT only; the /ML victor and zlib objects name LIBC.
RETAIL_FLAGS = ["/OPT:REF", "/NODEFAULTLIB:LIBC"]


def retail_clock() -> dict[str, str]:
    """The environment that runs LINK with its clock frozen at retail's
    TimeDateStamp (0x39b83835, 2000-09-08 00:52:05 UTC): libfaketime from
    the build shell. LINK stamps the header with CRT time(), which is UTC."""
    import datetime
    from homm3.core.common import load_image
    library = os.environ.get("HOMM3_FAKETIME_LIB")
    if not library or not Path(library).is_file():
        die("HOMM3_FAKETIME_LIB unset - run inside `nix develop .#build`")
    data = load_image()[0].data
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    stamp = struct.unpack_from("<I", data, pe + 8)[0]
    at = datetime.datetime.fromtimestamp(stamp, datetime.timezone.utc)
    return {**os.environ, "LD_PRELOAD": library, "TZ": "UTC",
            "FAKETIME": at.strftime("%Y-%m-%d %H:%M:%S"),
            "FAKETIME_DONT_FAKE_MONOTONIC": "1"}


#: LINK sorts each DLL's import thunks with its C runtime's qsort, so the
#: runtime decides the IAT order within a DLL. Wine's builtin msvcrt and the
#: Windows runtime pick different pivots; retail was linked on Windows. The
#: game links against the VC6 SP3 MSVCRT.DLL (6.00.8397) from the pinned SP3
#: media (config/project.toml [toolchain.linker_runtime]).
NATIVE_CRT_LINKER = "link-native-crt.exe"


def _wine_server_of(prefix: Path) -> None:
    """Stop the wineserver of `prefix` so the next one maps the runtime."""
    server = shutil.which("wineserver")
    if server:
        subprocess.run([server, "-k"], env={**os.environ, "WINEPREFIX": str(prefix)},
                       check=False, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL)


def linker_runtime() -> Path:
    """The pinned native MSVCRT.DLL, staged in build/linker-runtime/ from the
    pinned VC6 SP3 media and hash-checked."""
    import hashlib
    from homm3.core.project import Project
    spec = Project(HOMM3_DIR).specification.get("toolchain", {}).get("linker_runtime", {})
    if not spec:
        die("config/project.toml has no [toolchain.linker_runtime]")
    staged = HOMM3_DIR / "build/linker-runtime/MSVCRT.DLL"

    def pinned(path: Path) -> bool:
        return path.is_file() and hashlib.sha256(path.read_bytes()).hexdigest() == spec["sha256"]

    if pinned(staged):
        return staged
    media = Path(os.environ.get("HOMM3_SP3_MEDIA") or HOMM3_DIR / spec["media"]).resolve()
    if not media.is_dir():
        from homm3.init.vc6_rtm import _main_worktree
        main = _main_worktree()
        if main is not None:
            media = (main / spec["media"]).resolve()
    staged.parent.mkdir(parents=True, exist_ok=True)
    for cab in sorted(media.glob("*.cab")):
        subprocess.run(["7z", "e", "-y", f"-o{staged.parent}", str(cab), spec["member"]],
                       check=False, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL)
        extracted = staged.parent / Path(spec["member"]).name
        if extracted.is_file() and pinned(extracted):
            extracted.replace(staged)
            return staged
    die(f"the pinned linker runtime ({spec['member']}, sha256 {spec['sha256']}) "
        f"is not in {media}; set HOMM3_SP3_MEDIA to the SP3 cab directory")


def _system_runtime(prefix: Path) -> Path:
    windows = prefix / "drive_c/windows"
    system = windows / "syswow64" if (windows / "syswow64").is_dir() else windows / "system32"
    return system / "msvcrt.dll"


def _server_zones(prefix: Path) -> list[str | None]:
    """The TZ of every running wineserver of `prefix`."""
    zones = []
    for proc in Path("/proc").iterdir():
        if not proc.name.isdigit():
            continue
        try:
            if not (proc / "exe").resolve().name.startswith("wineserver"):
                continue
            environ = dict(item.split("=", 1) for item in
                           (proc / "environ").read_bytes().decode("latin-1").split("\0")
                           if "=" in item)
        except OSError:
            continue
        if Path(environ.get("WINEPREFIX", "")).resolve() == prefix.resolve():
            zones.append(environ.get("TZ"))
    return zones


def utc_wineserver(prefix: Path) -> None:
    """Make the prefix's wineserver run in UTC: the native runtime's time()
    converts LINK's clock through the server's zone, and the header
    TimeDateStamp must be retail's UTC value."""
    if any(zone != "UTC" for zone in _server_zones(prefix)):
        _wine_server_of(prefix)
    server = shutil.which("wineserver")
    if server:
        subprocess.run([server, "-p60"], env={**os.environ, "WINEPREFIX": str(prefix), "TZ": "UTC"},
                       check=False, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL)


def install_linker_runtime(prefix: Path, runtime: Path) -> None:
    """Put the native runtime in the prefix's 32-bit system directory,
    restarting its wineserver when the file changes."""
    import filecmp
    target = _system_runtime(prefix)
    if not (target.is_file() and filecmp.cmp(target, runtime, shallow=False)):
        _wine_server_of(prefix)
        shutil.copyfile(runtime, target)


def runtime_intact() -> bool:
    """True while the prefix still holds the pinned native runtime."""
    import filecmp
    prefix = Path(os.environ.get("WINEPREFIX") or HOMM3_DIR / "build/wineprefix")
    target = _system_runtime(prefix)
    return target.is_file() and filecmp.cmp(target, linker_runtime(), shallow=False)


def native_crt_linker(link: Path) -> Path:
    """A copy of LINK.EXE that loads the pinned native MSVCRT.DLL.

    MSVCRT is a KnownDLL, so wine loads it from the prefix's 32-bit system
    directory: the native file goes there and a Wine AppDefaults override
    selects it for NATIVE_CRT_LINKER alone; every other program keeps the
    builtin runtime. The wineserver maps KnownDLLs when it starts, so it is
    restarted when the file changes (the prefix is this worktree's own).
    """
    import filecmp
    runtime = linker_runtime()
    prefix = Path(os.environ.get("WINEPREFIX") or HOMM3_DIR / "build/wineprefix")
    folder = HOMM3_DIR / "build/exe/native-crt"
    folder.mkdir(parents=True, exist_ok=True)
    for path in link.parent.iterdir():
        if path.name.lower() in ("mspdb60.dll", "msobj10.dll", "msdis110.dll", "cvtres.exe"):
            copy = folder / path.name
            if not (copy.is_file() and filecmp.cmp(copy, path, shallow=False)):
                shutil.copyfile(path, copy)
    copy = folder / NATIVE_CRT_LINKER
    if not (copy.is_file() and filecmp.cmp(copy, link, shallow=False)):
        shutil.copyfile(link, copy)
    key = rf"HKEY_CURRENT_USER\Software\Wine\AppDefaults\{NATIVE_CRT_LINKER}\DllOverrides"
    output, _rc = run_wine(["wine", "reg", "query", key, "/v", "msvcrt"], folder)
    if "native" not in output:
        run_wine(["wine", "reg", "add", key, "/v", "msvcrt", "/d", "native", "/f"], folder)
    # Install the runtime last: starting a wine process can update the prefix
    # and restore wine's placeholder.
    install_linker_runtime(prefix, runtime)
    utc_wineserver(prefix)
    return copy


def game_objects(objs_dir: Path | None = None) -> tuple[dict[str, Path], dict[str, list[str]]]:
    """({unit: object} linked as objects, {library: [units]} archived), in
    manifest order; a unit's `library` key names its archive."""
    from homm3 import manifest
    objs_dir = Path(objs_dir or _image_path("build/objdiff/base"))
    objects: dict[str, Path] = {}
    libraries: dict[str, list[str]] = {}
    for unit in manifest.units():
        name = unit["unit"]
        if unit.get("library"):
            libraries.setdefault(unit["library"], []).append(name)
        else:
            objects[name] = objs_dir / f"{name}.obj"
    return objects, libraries


def build_library(link: Path, library: Path, members: list[Path], cwd: Path) -> None:
    """Archive `members` (in order) into `library` with the pinned LIB."""
    library.unlink(missing_ok=True)
    rsp = library.with_suffix(".lib.rsp")
    rsp.write_text("\n".join(["/NOLOGO", f'/OUT:"{winepath_w(library)}"',
                               *[f'"{winepath_w(m)}"' for m in members]]) + "\n")
    # `-lib` must be LINK's first argument; in a response file it is an option.
    output, rc = run_wine(["wine", str(link), "-lib", f"@{winepath_w(rsp)}"], cwd)
    if rc or not library.is_file():
        die(f"LIB failed for {library.name} (exit {rc}):\n{output.strip()}")


def retail_inputs(out_dir: Path, link: Path, objs_dir: Path | None = None) -> tuple[list[Path], list[Path]]:
    """(objects in retail order, archived libraries) of the game link."""
    from homm3.build import link_order
    objects, libraries = game_objects(objs_dir)
    missing = [str(p) for p in objects.values() if not p.is_file()]
    if missing:
        die("missing objects: " + ", ".join(missing[:5]))
    units, _how = link_order.order(objects)
    objs_dir = Path(objs_dir or _image_path("build/objdiff/base"))
    archives = []
    for name, members in libraries.items():
        paths = {unit: objs_dir / f"{unit}.obj" for unit in members}
        keys = link_order.unit_keys(paths)
        ordered = sorted(members, key=lambda unit: keys.get(unit, (1 << 32,))[0])
        library = out_dir / f"{name}.lib"
        build_library(link, library, [paths[u] for u in ordered], out_dir)
        archives.append(library)
    return [objects[u] for u in units], archives


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


def unresolved_symbols(output: str) -> list[str]:
    """Keep LINK's decorated identity, not the first word of its display name."""
    names = set()
    for line in output.splitlines():
        if "unresolved external symbol " not in line:
            continue
        name = line.split("unresolved external symbol ", 1)[1].strip()
        # C++ diagnostics print: "return type readable signature" (?decorated).
        decorated = re.search(r'\s\(([^\s]+)\)$', name)
        names.add(decorated[1] if decorated else name)
    return sorted(names)


def link_succeeded(output: str, rc: int, exists: bool) -> bool:
    return (exists and rc == 0 and not unresolved_symbols(output)
            and not re.search(r"\bLNK(?:2005|4006|4088)\b", output))


def main(argv: list[str] | None = None) -> int:
    from homm3.core.images import DEFAULT_IMAGE, IMAGE_ENV
    ap = argparse.ArgumentParser(description="VC6 link.exe wrapper (candidate link).")
    ap.add_argument("--out", default=_image_path("build/exe/HEROES3.candidate.EXE"))
    ap.add_argument("--map", dest="mapfile", default=None,
                    help="map path (default: <out> with .map suffix).")
    ap.add_argument("--objs-dir", default=_image_path("build/objdiff/base"))
    ap.add_argument("--study", action="store_true",
                    help="the layout study: name order (or --order/--obj), "
                         "/OPT:NOREF /OPT:NOICF, /NODEFAULTLIB + explicit libraries")
    ap.add_argument("--obj", action="append", help="explicit obj (repeatable; study).")
    ap.add_argument("--order", help="file listing obj stems/paths in link order (study).")
    ap.add_argument("--lib", action="append", default=[],
                    help="extra import/static lib to pass to link (repeatable).")
    ap.add_argument("--base", default=None, help="image base (/BASE).")
    ap.add_argument("--opt-ref", dest="keep_all", action="store_false", default=True,
                    help="study: let the linker strip/fold unreferenced COMDATs.")
    ap.add_argument("flags", nargs=argparse.REMAINDER,
                    help="extra link flags after `--`.")
    args = ap.parse_args(argv)
    extra = args.flags[1:] if args.flags and args.flags[0] == "--" else args.flags
    if any(re.match(r"^[-/]force(?:[:=]|$)", flag, re.I) for flag in extra):
        ap.error("/FORCE is unsupported: the game must link without unresolved or duplicate symbols")
    # Only the game has a reconstructed retail link line; other images and
    # explicit object lists stay layout studies.
    study = (args.study or args.order or args.obj
             or os.environ.get(IMAGE_ENV, DEFAULT_IMAGE) != DEFAULT_IMAGE)
    if args.base is None:
        from homm3.core import common
        args.base = hex(common.load_image()[0].image_base)

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

    os.environ.setdefault("WINEDEBUG", "fixme-all,err-kerberos")
    ensure_wineserver()

    from homm3.build.import_libraries import build_vendor_libraries
    from homm3.core.common import load_image
    vendor = {path.name.lower(): path for path in build_vendor_libraries(
        load_image()[0].data, out.parent / "imports")}

    rsp_lines = [
        f'/OUT:"{winepath_w(out)}"',
        f'/MAP:"{winepath_w(mapf)}"',
        "/NOLOGO", "/SUBSYSTEM:WINDOWS",
        f"/BASE:{args.base}", "/INCREMENTAL:NO",
    ]
    if study:
        objs = collect_objs(args)
        if not objs:
            die("no objects to link.")
        libraries = list(args.lib) + [str(p) for p in vendor.values()]
        for name in ("LIBCMT.LIB", "LIBCPMT.LIB", "KERNEL32.LIB", "USER32.LIB",
                     "GDI32.LIB", "ADVAPI32.LIB", "WINMM.LIB", "VERSION.LIB",
                     "WSOCK32.LIB", "DDRAW.LIB", "DINPUT.LIB", "DXGUID.LIB",
                     "UUID.LIB", "OLE32.LIB", "SHELL32.LIB", "OLDNAMES.LIB"):
            library = find_ci(msvc / "lib", name)
            if library is None:
                die(f"missing toolchain library {name}")
            libraries.append(str(library))
        rsp_lines += ["/NODEFAULTLIB", "/ENTRY:WinMainCRTStartup"]
        rsp_lines += ["/OPT:NOREF", "/OPT:NOICF"] if args.keep_all else ["/OPT:REF"]
    else:
        objs, archives = retail_inputs(out.parent, link, Path(args.objs_dir))
        built = {path.name.lower(): path for path in archives}
        libraries = list(args.lib)
        for name in RETAIL_LIBRARIES:
            library = built.get(name) or vendor.get(name) or find_ci(msvc / "lib", name)
            if library is None:
                die(f"missing library {name}")
            libraries.append(str(library))
        rsp_lines += RETAIL_FLAGS + [f'/LIBPATH:"{winepath_w(msvc / "lib")}"']
        from homm3.build import resources
        try:
            res = resources.compile_resources(out=out.parent / "heroes3.res")
        except (ValueError, OSError) as exc:
            die(f"resources: {exc}")
        objs = objs + [res]
    rsp_lines += list(extra)
    rsp_lines += [f'"{winepath_w(o)}"' for o in objs]
    rsp_lines += [f'"{winepath_w(Path(lib)) if os.path.exists(lib) else lib}"'
                  for lib in libraries]

    rsp = out.parent / (out.stem + ".objs.rsp")
    rsp.write_text("\n".join(rsp_lines) + "\n")

    linker = link if study else native_crt_linker(link)
    output, rc = run_wine(["wine", str(linker), f"@{winepath_w(rsp)}"],
                          out.parent, env=None if study else retail_clock())
    if not study and not runtime_intact():
        die("wine replaced the native linker runtime during the link; relink")

    log = out.with_suffix(".link.log")
    log.write_text(output)
    unresolved = unresolved_symbols(output)
    punch = out.parent / (out.stem + ".unresolved.txt")
    punch.write_text("\n".join(unresolved) + ("\n" if unresolved else ""))
    if not link_succeeded(output, rc, out.exists()):
        sys.stderr.write(f"[link] FAILED: {out} (link exit {rc}; see {log.name})\n")
        sys.stderr.write("\n".join(output.strip().splitlines()[-20:]) + "\n")
        return rc or 1

    warns = sum(1 for ln in output.splitlines() if "LNK4006" in ln)
    shown = out.relative_to(HOMM3_DIR) if out.is_relative_to(HOMM3_DIR) else out
    mode = "study" if study else "retail line"
    print(f"[link] {len(objs)} objs ({mode}) -> {shown} ({out.stat().st_size} B) + {mapf.name}")
    print(f"[link] {len(unresolved)} unresolved externals -> {punch.name}, "
          f"{warns} dup-symbol warnings")
    return 0


if __name__ == "__main__":
    sys.exit(main())
