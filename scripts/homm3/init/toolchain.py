#!/usr/bin/env python3
"""homm3.init.toolchain - one-time Wine/VC6 environment setup.

Prepares everything `wine cl.exe` (and `wine link.exe`) needs, idempotently:

  1. TOOLCHAIN TREE: if $MSVC_DIR/bin/cl.exe is missing, unpack the release
     tarball build/homm3-toolchain-vc6-sp3.tar.xz; if the tarball is missing
     too, download it from the private GitHub release (RELEASE_REPO/RELEASE_TAG
     below, via authenticated `gh`) and verify its pinned SHA-256 first. The
     tarball is built from original media by scripts/create-toolchain-release.py;
     init only downloads and unpacks, never rebuilds.
  2. WINE PREFIX: `wineboot --init` at $WINEPREFIX on first run (--force
     re-creates it).
  3. SMOKE COMPILE through the real wrapper (homm3.core.cc_wrap), so init ends
     having proven the exact code path ninja's `cl` rule uses.

DIVERGENCE from the Gruntz template: no Wine-registry PATH/INCLUDE/LIB writes.
homm3.core.cc_wrap passes INCLUDE per invocation and calls cl.exe by absolute
path, and homm3.build.link passes libraries per invocation, so the prefix
carries no toolchain state that `clean` + re-init could leave stale.

VC6 notes: CL.EXE needs no PDB writer for plain /c builds; MSPDB60.DLL ships
next to it in the tarball for /Zi later. LINK.EXE's static imports are only
mspdb60/msvcrt/kernel32 (verified by walking its import table) - MSDIS110.DLL
is a dynamic load used only by `link /dump /disasm`, so unlike the VC5 Gruntz
toolchain no stub DLL is required for linking.

Run inside `nix develop .#build` (exports HOMM3_TOOLCHAIN, MSVC_DIR, WINEPREFIX):
    homm3 init [--force] [--no-smoke]        # via the CLI
    python3 -m homm3.init.toolchain [--force] [--no-smoke]
"""
from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tarfile
from pathlib import Path

from homm3.core.cc_wrap import HOMM3_DIR, find_ci, msvc_dir

RELEASE_REPO = "sushi-shi/homm3-decomp"
RELEASE_TAG = "toolchain-vc6-sp3"
TARBALL_SHA256 = "9fd1b3b3df143b03a5f9f50184058b9d2edfc02d0520a71164e9d1866ed61bc9"


def log(msg: str) -> None:
    print(f"[init] {msg}", flush=True)


def die(msg: str) -> None:
    log("ERROR: " + msg)
    sys.exit(1)


def sha256_of(path: Path) -> str:
    import hashlib
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def ensure_tarball(tarball: Path) -> None:
    """Download the release tarball if absent; always verify the pinned hash."""
    if not tarball.is_file():
        import shutil
        if shutil.which("gh") is None:
            die(f"no tarball at {tarball} and `gh` not found - install/auth the "
                f"GitHub CLI or fetch the {RELEASE_TAG} release asset manually")
        log(f"downloading {tarball.name} from {RELEASE_REPO}@{RELEASE_TAG} ...")
        tarball.parent.mkdir(parents=True, exist_ok=True)
        rc = subprocess.run(
            ["gh", "release", "download", RELEASE_TAG, "--repo", RELEASE_REPO,
             "--pattern", tarball.name, "--output", str(tarball)]).returncode
        if rc or not tarball.is_file():
            die(f"download failed - build it with scripts/create-toolchain-release.py "
                f"or fetch the {RELEASE_TAG} release asset manually")
    actual = sha256_of(tarball)
    if actual != TARBALL_SHA256:
        die(f"tarball hash mismatch: {tarball}\n"
            f"  expected {TARBALL_SHA256}\n  actual   {actual}\n"
            "delete the file and re-run init to re-download")


def ensure_toolchain() -> Path:
    """Return the msvc/ root, downloading/unpacking the tarball if cl.exe is absent."""
    msvc = msvc_dir()
    if find_ci(msvc / "bin", "cl.exe"):
        log(f"toolchain present: {msvc}")
        return msvc
    toolchain_root = msvc.parent
    tarball = toolchain_root.with_name(toolchain_root.name + ".tar.xz")
    ensure_tarball(tarball)
    log(f"unpacking {tarball.name} -> {toolchain_root.parent}/ ...")
    with tarfile.open(tarball, "r:xz") as tar:
        try:
            tar.extractall(path=toolchain_root.parent, filter="data")
        except TypeError:  # Python < 3.12 has no extraction filter
            tar.extractall(path=toolchain_root.parent)
    if not find_ci(msvc / "bin", "cl.exe"):
        die(f"tarball unpacked but cl.exe still missing under {msvc}/bin")
    log(f"toolchain unpacked: {msvc}")
    return msvc


def init_prefix(force: bool = False) -> None:
    prefix = Path(os.environ.get("WINEPREFIX") or HOMM3_DIR / "build/wineprefix")
    os.environ["WINEPREFIX"] = str(prefix)
    if force and (prefix / "drive_c").is_dir():
        import shutil
        subprocess.run(["wineserver", "-k"], check=False,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        subprocess.run(["wineserver", "--wait"], check=False)
        shutil.rmtree(prefix)
        log(f"removed prefix {prefix} (--force)")
    if (prefix / "drive_c").is_dir():
        log(f"wine prefix present: {prefix}")
        return
    prefix.mkdir(parents=True, exist_ok=True)
    log(f"initialising wine prefix at {prefix} ...")
    subprocess.run(["wineboot", "--init"], check=True)
    subprocess.run(["wineserver", "--wait"], check=False)


def smoke_test() -> None:
    """Compile hello.c through homm3.core.cc_wrap - the exact ninja code path."""
    work = HOMM3_DIR / "build/smoke"
    work.mkdir(parents=True, exist_ok=True)
    src = work / "hello.c"
    src.write_text('#include <stdio.h>\n'
                   'int main(void) { printf("hello from VC6\\n"); return 0; }\n')
    obj = work / "hello.obj"
    if obj.exists():
        obj.unlink()
    log("smoke test: cc_wrap /nologo /c /O2 /ML hello.c")
    rc = subprocess.run(
        [sys.executable, "-m", "homm3.core.cc_wrap",
         "--out", str(obj), "--src", str(src), "--", "/nologo", "/c", "/O2", "/ML"],
        cwd=HOMM3_DIR).returncode
    if rc or not obj.exists():
        die("smoke compile failed - the toolchain/prefix is not usable "
            "(see cc_wrap output above)")
    log(f"OK - produced {obj.relative_to(HOMM3_DIR)} ({obj.stat().st_size} bytes)")


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Set up the Wine/VC6 build environment.")
    ap.add_argument("--force", action="store_true", help="re-init the wine prefix")
    ap.add_argument("--no-smoke", action="store_true", help="skip the smoke compile")
    args = ap.parse_args(argv)

    import shutil
    if shutil.which("wine") is None:
        die("wine not found - run inside `nix develop .#build`")
    os.environ.setdefault("WINEDEBUG", "fixme-all,err-kerberos")
    os.environ.setdefault("WINEDLLOVERRIDES", "mscoree,mshtml=")

    ensure_toolchain()
    init_prefix(force=args.force)
    if not args.no_smoke:
        smoke_test()
    log("done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
